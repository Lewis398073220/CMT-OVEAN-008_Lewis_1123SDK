/**
 * @copyright Copyright (c) 2015-2021 BES Technic.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 */

/*****************************header include********************************/
#include "hal_trace.h"
#include "nvrecord_ble.h"
#include "bt_drv_interface.h"
#include "bluetooth_ble_api.h"

#ifdef BLE_STACK_NEW_DESIGN
#include "app_ble.h"
#else
#include "app.h"
#endif

#include "ble_audio_core_api.h"
#include "ble_audio_core_evt.h"
#include "ble_audio_core.h"
#include "ble_audio_earphone_info.h"
#include "ble_audio_core.h"

#include "aob_csip_api.h"
#include "aob_gatt_cache.h"
#include "app_gaf_dbg.h"
#include "aob_conn_api.h"
#include "aob_media_api.h"
#include "aob_bis_api.h"

#ifdef AOB_MOBILE_ENABLED
#include "ble_audio_mobile_info.h"
#endif

#include "gaf_bis_media_stream.h"

#include "app_acc_mcc_msg.h"
#include "app_arc_vcs_msg.h"
#include "app_acc_tbc_msg.h"

/*********************external variable declaration*************************/
uint8_t ADV_FLAG_CUSTOM = 0;

#ifdef CUSTOMER_DEFINE_ADV_DATA
BLE_ADV_PARAM_T ble_audio_adv_param[BLE_ADV_ACTIVITY_USER_NUM];
#endif

#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
#define AOB_ADV_ACTIVITY_USER      BLE_ADV_ACTIVITY_USER_3
#else
#define AOB_ADV_ACTIVITY_USER      BLE_ADV_ACTIVITY_USER_0
#endif

/********************internal variable declaration**************************/
static struct aob_connection_info
{
    /// One bit represent one connection link
    bool encrypted_bf;
    /// GATT Cache restore bit
    uint16_t gatt_c_restore_state[BLE_CONNECTION_MAX];
} aob_conn_info;
/*********************external function declaration*************************/

/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declaration*************************/

/************************private variable defination************************/

/****************************function defination****************************/
#if BLE_BUDS
void aob_conn_create_tws_link(uint32_t timeout_ms)
{
    // TODO: to use the timeout_ms parameter
    LOG_I("%s", __func__);
    ble_audio_tws_connect_request();
}

int aob_conn_disconnect_tws_link(void)
{
    LOG_I("%s", __func__);
    ble_audio_tws_disconnect_request();
    return 0;
}

void aob_conn_cancel_connecting_tws(void)
{
    LOG_I("%s", __func__);
    ble_audio_tws_cancel_connecting_request();
}

bool aob_conn_is_tws_connected(void)
{
    return ble_audio_tws_link_is_connected();
}

uint8_t aob_conn_get_tws_con_lid(void)
{
    return ble_audio_get_tws_link_conidx();
}
#endif

#ifdef CUSTOMER_DEFINE_ADV_DATA
void aob_conn_adv_data_refresh(void)
{
    uint8_t adv_addr_set[6]  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    ble_audio_advData_prepare(&ble_audio_adv_param[AOB_ADV_ACTIVITY_USER], ADV_FLAG_CUSTOM);
    app_ble_custom_adv_write_data(AOB_ADV_ACTIVITY_USER,
                                  true,
                                  BLE_ADV_RPA,
                                  adv_addr_set,
                                  NULL,
                                  BLE_ADVERTISING_INTERVAL,
                                  ADV_TYPE_CONN_EXT_ADV,
                                  ADV_MODE_EXTENDED,
                                  9,
                                  (uint8_t *)ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advData,
                                  ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advDataLen,
                                  NULL, 0);
    app_ble_refresh_adv_state_by_custom_adv(BLE_ADVERTISING_INTERVAL);
}
#endif
bool aob_conn_start_adv(bool br_edr_support, bool in_pairing)
{
    // if ble_audio_adv is enabled, do not call start_adv
    if (false == app_ble_audio_adv_enable())
    {
        LOG_W("%s adv is already enabled", __func__);
    }

    // Start adv will refresh adv data, so clear RSI refresh flag
    ble_audio_clear_adv_rsi_refresh_need_flag();

    ADV_FLAG_CUSTOM = 0;

    if (in_pairing)
    {
        ADV_FLAG_CUSTOM |= (br_edr_support ? SIMU_BR_EDR_LE_BIT : 0) | PAIRING_MODE_ADV_BIT;
        LOG_D("%s connectable pairing adv", __func__);
    }
    else
    {
        ADV_FLAG_CUSTOM |= (br_edr_support ? SIMU_BR_EDR_LE_BIT : 0) | TARGET_ANNOUN_EN_BIT;
        LOG_D("%s connectable reconnect adv", __func__);
    }

    // If ble_audio_adv is initiated by custom adv api
#ifdef CUSTOMER_DEFINE_ADV_DATA
    uint8_t adv_addr_set[6]  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    app_ble_custom_init();
    ble_audio_advData_prepare(&ble_audio_adv_param[AOB_ADV_ACTIVITY_USER], ADV_FLAG_CUSTOM);
    app_ble_custom_adv_write_data(AOB_ADV_ACTIVITY_USER,
                                  true,
                                  BLE_ADV_RPA,
                                  adv_addr_set,
                                  NULL,
                                  ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advInterval,
                                  ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advType,
                                  ADV_MODE_EXTENDED,
                                  12,
                                  (uint8_t *)ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advData,
                                  ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advDataLen,
                                  NULL, 0);
    app_ble_custom_adv_start(AOB_ADV_ACTIVITY_USER);
#else
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
    return true;
}

bool aob_conn_stop_adv(void)
{
    // if ble_audio_adv is disabled, do not call force_switch_adv
    if (false == app_ble_audio_adv_disable())
    {
        LOG_W("%s adv is already disabled", __func__);
    }

    ADV_FLAG_CUSTOM = 0;

    // If ble_audio_adv is stopped by custom adv api
#ifdef CUSTOMER_DEFINE_ADV_DATA
    app_ble_custom_adv_stop(AOB_ADV_ACTIVITY_USER);
#else
#ifdef BLE_STACK_NEW_DESIGN
    app_ble_disable_advertising(BLE_AUDIO_ADV_HANDLE);
#endif
#ifdef CUSTOMER_DEFINE_ADV_DATA
    app_ble_refresh_adv_state(ble_audio_adv_param[AOB_ADV_ACTIVITY_USER].advInterval);
#else
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
#endif
    return true;
}

void aob_conn_create_mobile_link(ble_bdaddr_t *addr)
{
    LOG_I("%s", __func__);
    ble_audio_start_mobile_connecting(addr, 1);
}

void aob_conn_disconnect_mobile_link(ble_bdaddr_t *addr)
{
    LOG_I("%s", __func__);
    ble_audio_mobile_req_disconnect(addr);
}

uint8_t aob_conn_get_connected_mobile_lid(uint8_t con_lid[])
{
    return ble_audio_get_mobile_connected_dev_lids(con_lid);
}

BLE_AUDIO_TWS_ROLE_E aob_conn_get_cur_role(void)
{
    return (BLE_AUDIO_TWS_ROLE_E)ble_audio_request_tws_current_role();
}

ble_bdaddr_t *aob_conn_get_remote_address(uint8_t con_lid)
{
    return ble_audio_get_mobile_address_by_lid(con_lid);
}

static uint8_t test_ble_addr[2][6];

POSSIBLY_UNUSED void aob_test_config_peer_ble_addr(uint8_t index, uint8_t *pAddr)
{
    if (index < 2)
    {
        memcpy(test_ble_addr[index], pAddr, 6);
        TRACE(0, "Config peer ble addr as:");
        DUMP8("%02x ", pAddr, 6);
    }
}

POSSIBLY_UNUSED static bool aob_test_match_peer_ble_addr(uint8_t *pAddr)
{
    for (uint8_t index = 0; index < 2; index++)
    {
        if (!memcmp(pAddr, test_ble_addr[index], 6))
        {
            TRACE(0, "Find the matched device!");
            return true;
        }
    }

    return false;
}

static void aob_conn_scan_data_report_callback(ble_event_t *event)
{
#ifdef AOB_MOBILE_ENABLED
    if (ble_audio_is_ux_mobile())
    {
        ble_event_handled_t *ble_event = &event->p;
        BLE_ADV_DATA_T *particle;

        for (uint8_t offset = 0; offset < ble_event->scan_data_report_handled.length;)
        {
            /// get the particle of adv data
            particle = (BLE_ADV_DATA_T *)(ble_event->scan_data_report_handled.data + offset);
            bool ble_start_connect = false;
            bool isConnecting = app_ble_is_connection_on_by_addr(ble_event->scan_data_report_handled.trans_addr.addr);
            bool isConnected = ble_audio_mobile_check_device_connected(ble_event->scan_data_report_handled.trans_addr.addr);

#if !defined(BLE_AUDIO_CSIP_SUPPORT)
            uint16_t servicesUuid = GATT_SVC_AUDIO_STREAM_CTRL;
            if ((GAP_AD_TYPE_SERVICE_16_BIT_DATA == particle->type) &&
                    (!memcmp(particle->value, (void *)&servicesUuid, sizeof(servicesUuid))) &&
                    (!isConnecting) && (!isConnected))
            {
                ble_start_connect = true;
            }
#else
            if ((BLE_ADV_TYPE_COMPLETE_NAME == particle->type) && \
                    (!memcmp(particle->value, app_ble_get_dev_name(), particle->length - 1)) &&
                    (!isConnecting) && (!isConnected))
            {
                ble_start_connect = true;
            }
#endif

            if (!ble_start_connect)
            {
                // TODO: can set unicast server's name here
                /*
                for (uint16_t index = 0;index < particle->length;index++)
                {
                    if (0 == memcmp(&(particle->value[index]), "test", 4))
                    {
                        ble_start_connect = true;
                    }
                }
                */
                ble_start_connect = aob_test_match_peer_ble_addr(
                                        ble_event->scan_data_report_handled.trans_addr.addr);
            }

            if (ble_start_connect)
            {
                LOG_I("AOB-CONN: found the fox!");
                app_ble_stop_scan();
#ifdef BLE_STACK_NEW_DESIGN
                app_ble_connect_ble_audio_device((ble_bdaddr_t *)&ble_event->scan_data_report_handled.trans_addr, APP_GAPM_STATIC_ADDR);
#else
                app_ble_start_connect((ble_bdaddr_t *)&ble_event->scan_data_report_handled.trans_addr, APP_GAPM_STATIC_ADDR);
#endif
                break;
            }
            else if ((!isConnected) && (AOB_CSIP_RSI_ADV_AD_TYPE == particle->type))
            {
                if (aob_csip_mobile_if_has_connected_device())
                {
                    aob_csip_mobile_if_resolve_rsi_data(particle->value, particle->length - 1, &(ble_event->scan_data_report_handled.trans_addr));
                }
            }
            offset += (ADV_PARTICLE_HEADER_LEN + particle->length);
        }
    }
#endif
}

#ifdef BLE_STACK_NEW_DESIGN
void aob_conn_check_svc_restore_and_cfg_cccd(uint8_t con_lid)
{
#if APP_GAF_ACC_ENABLE
    uint8_t mcs_load_result = get_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_MCS_POS);
    uint8_t tbs_load_result = get_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_TBS_POS);

    if (tbs_load_result == 0)
    {
#ifdef AOB_MOBILE_ENABLED
        ble_audio_discovery_modify_interval(con_lid, DISCOVER_START, SERVICE_ACC_TBC);
#else
        ble_audio_connection_interval_mgr(con_lid, LEA_CI_MODE_SVC_DISC_START);
#endif
        tbs_load_result = app_acc_tbc_simplified_start(con_lid) == BT_STS_SUCCESS;
    }

    if (mcs_load_result == 0)
    {
#ifdef AOB_MOBILE_ENABLED
        ble_audio_discovery_modify_interval(con_lid, DISCOVER_START, SERVICE_ACC_MCC);
#else
        ble_audio_connection_interval_mgr(con_lid, LEA_CI_MODE_SVC_DISC_START);
#endif
        mcs_load_result = app_acc_mcc_simplified_start(con_lid) == BT_STS_SUCCESS;
    }

    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_MCS_POS * mcs_load_result);
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_TBS_POS * tbs_load_result);
#endif

    LOG_I("[%d] after check restore gatt service cache, state bf 0x%x", con_lid,
          aob_conn_info.gatt_c_restore_state[con_lid]);
}
#else
void aob_conn_restore_gatt_svc_cache_data(uint8_t con_lid)
{
#if APP_GAF_ACC_ENABLE
    ble_bdaddr_t remote_addr = {{0}};
    uint8_t mcs_load_result = get_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_MCS_POS);
    uint8_t tbs_load_result = get_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_TBS_POS);
    /// check device public addr
    uint8_t ret = app_ble_get_peer_solved_addr(con_lid, &remote_addr);
    if (!ret)
    {
        LOG_W("[%d] has no solved addr yet, do not do service restore", con_lid);
        return;
    }

    /// restore all service data in gatt cahce
    tbs_load_result  = tbs_load_result ? 1 : aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_GENERIC_TELEPHONE_BEARER);
    mcs_load_result  = mcs_load_result ? 1 : aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_GENERIC_MEDIA_CONTROL);

    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_MCS_POS * mcs_load_result);
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_TBS_POS * tbs_load_result);

    LOG_I("[%d] after restore gatt service cache, state bf 0x%x", con_lid, aob_conn_info.gatt_c_restore_state[con_lid]);
#endif
}

void aob_conn_check_svc_restore_and_cfg_cccd(uint8_t con_lid)
{
#if APP_GAF_ACC_ENABLE
    ble_bdaddr_t remote_addr = {{0}};
    /// check device public addr
    uint8_t ret = app_ble_get_peer_solved_addr(con_lid, &remote_addr);
    if (!ret)
    {
        LOG_W("[%d] has no solved addr yet, do not check service restore", con_lid);
        return;
    }
    if (get_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_TBS_POS))
    {
        app_acc_tbc_set_cfg(con_lid, 0, AOB_MGR_TB_CHAR_TYPE_PROV_NAME, 1);
        clr_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_TBS_POS);
    }
    else if (aob_gattc_is_cache_item_exist(remote_addr.addr, GATT_SVC_GENERIC_TELEPHONE_BEARER) != true)
    {
#ifdef AOB_MOBILE_ENABLED
        ble_audio_discovery_modify_interval(con_lid, DISCOVER_START, SERVICE_ACC_TBC);
#else
        ble_audio_connection_interval_mgr(con_lid, LEA_CI_MODE_SVC_DISC_START);
#endif
        app_acc_tbc_start(con_lid);
    }

    if (get_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_MCS_POS))
    {
        app_acc_mcc_set_cfg(con_lid, 0, AOB_MGR_TB_CHAR_TYPE_PROV_NAME, 1);
        clr_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_MCS_POS);
    }
    else if (aob_gattc_is_cache_item_exist(remote_addr.addr, GATT_SVC_GENERIC_MEDIA_CONTROL) != true)
    {
#ifdef AOB_MOBILE_ENABLED
        ble_audio_discovery_modify_interval(con_lid, DISCOVER_START, SERVICE_ACC_MCC);
#else
        ble_audio_connection_interval_mgr(con_lid, LEA_CI_MODE_SVC_DISC_START);
#endif
        app_acc_mcc_start(con_lid);
    }
#endif

}
#endif

void aob_conn_restore_gatt_cccd_cache_data(uint8_t con_lid)
{
#if APP_GAF_ACC_ENABLE
    ble_bdaddr_t remote_addr = {{0}};
    uint8_t pacs_load_result = 0;
    uint8_t vcs_load_result = 0;
    uint8_t ascs_load_result = 0;
    uint8_t bass_load_result = 0;
    /// check device public addr
    uint8_t ret = app_ble_get_peer_solved_addr(con_lid, &remote_addr);
    if (!ret)
    {
        LOG_W("[%d] has no solved addr yet, do not do service restore", con_lid);
        return;
    }
    /// restore all service data in gatt cahce
    vcs_load_result  = aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_VOLUME_CONTROL);
    pacs_load_result = aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_PUBLISHED_AUDIO_CAPA);
    ascs_load_result = aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_AUDIO_STREAM_CTRL);
    bass_load_result = aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_BCAST_AUDIO_SCAN);

    /// record cache restore state
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_VCS_POS * vcs_load_result);
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_PACS_POS * pacs_load_result);
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_ASCS_POS * ascs_load_result);
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_BASS_POS * bass_load_result);
#ifdef BLE_BATT_ENABLE
    uint8_t batt_load_result = 0;
    batt_load_result = aob_gattc_cache_load(con_lid, remote_addr.addr, GATT_SVC_BATTERY_SERVICE);
    set_bit(aob_conn_info.gatt_c_restore_state[con_lid], GATT_C_BATT_POS * batt_load_result);
#endif

    LOG_I("[%d] after restore gatt cccd cache, state bf 0x%x", con_lid, aob_conn_info.gatt_c_restore_state[con_lid]);
    /// custom callback
    const BLE_AUD_CORE_EVT_CB_T *p_con_cb = ble_audio_get_evt_cb();
    if (p_con_cb && p_con_cb->ble_con_pacs_cccd_gatt_load_cb != NULL)
    {
        //notify customer the result of GATT_SVC_PUBLISHED_AUDIO_CAPA load
        p_con_cb->ble_con_pacs_cccd_gatt_load_cb(con_lid, pacs_load_result);
    }
#endif
}

void aob_conn_clr_gatt_cache_restore_state(uint8_t con_lid, gatt_c_restore_svc_pos_e svc_pos)
{
#if APP_GAF_ACC_ENABLE
    clr_bit(aob_conn_info.gatt_c_restore_state[con_lid], svc_pos);
    LOG_I("[%d] now restore state bf 0x%x", con_lid, aob_conn_info.gatt_c_restore_state[con_lid]);
#endif
}

void aob_conn_clr_all_gatt_cache_restore_state(uint8_t con_lid)
{
#if APP_GAF_ACC_ENABLE
    aob_conn_info.gatt_c_restore_state[con_lid] = 0;
#endif
}

void aob_conn_get_tx_power(uint8_t conidx, ble_tx_object_e object, ble_phy_pwr_value_e phy)
{
    bes_ble_gap_get_tx_pwr_value(conidx, (bes_ble_tx_object_e)object, (bes_ble_phy_pwr_value_e)phy);
}

void aob_conn_tx_power_report_enable(uint8_t conidx, bool local_enable, bool remote_enable)
{
    bes_ble_gap_tx_power_report_enable(conidx, local_enable, remote_enable);
}

void aob_conn_set_path_loss_param(uint8_t conidx, uint8_t enable, uint8_t high_threshold,
                                  uint8_t high_hysteresis, uint8_t low_threshold,
                                  uint8_t low_hysteresis, uint8_t min_time)
{
#ifdef CFG_LE_PWR_CTRL
    appm_set_path_loss_rep_param_cmd(conidx, enable, high_threshold, high_hysteresis, low_threshold, low_hysteresis, min_time);
#endif
}

void aob_conn_set_default_subrate(uint16_t subrate_min, uint16_t subrate_max, uint16_t latency_max,
                                  uint16_t continuation_num, uint16_t timeout)
{
    bes_ble_gap_subrate_request(INVALID_BLE_CONIDX, subrate_min, subrate_max, latency_max, continuation_num, timeout);
}

void aob_conn_subrate_request(uint8_t conidx, uint16_t subrate_min, uint16_t subrate_max, uint16_t latency_max,
                              uint16_t continuation_num, uint16_t timeout)
{
    bes_ble_gap_subrate_request(conidx, subrate_min, subrate_max, latency_max, continuation_num, timeout);
}

void ble_audio_event_callback(ble_event_t *event)
{
    ble_event_handled_t *ble_event = &event->p;
    bool is_mobile = ble_audio_is_ux_mobile();
    uint8_t con_lid = ble_event->connect_handled.conidx;

    switch (event->evt_type)
    {
        case BLE_LINK_CONNECTED_EVENT:
            if (!is_mobile)
            {
                // Mark all bond state invalid
                aob_conn_clr_all_gatt_cache_restore_state(con_lid);
#if !defined (BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT) && !defined (BLE_STACK_NEW_DESIGN)
                aob_conn_restore_gatt_svc_cache_data(con_lid);
#endif
                aob_conn_restore_gatt_cccd_cache_data(con_lid);
            }
            break;
        case BLE_CONNECT_BOND_EVENT:
            if (!ble_event->connect_bond_handled.success)
            {
                break;
            }
            // Mark all bond state invalid
            aob_conn_clr_all_gatt_cache_restore_state(con_lid);
            // clear bond data
            app_bap_uc_srv_restore_bond_data_req(con_lid, 0, 0);
            app_arc_vcs_restore_bond_data_req(con_lid, 0);
            app_bap_capa_srv_restore_bond_data_req(con_lid, 0, 0);
        //fall through
        case BLE_CONNECT_ENCRYPT_EVENT:
            aob_conn_set_encrypt_state(con_lid, true);
            // Only restore csis after ltk is used in link encrypt, ensure that ltk is valid to avoid re pair
            aob_gattc_cache_load(con_lid, ble_event->connect_encrypt_handled.addr, GATT_SVC_COORD_SET_IDENTIFICATION);
            // Start service info restore
            aob_conn_check_svc_restore_and_cfg_cccd(con_lid);
            break;
        case BLE_CONNECT_BOND_FAIL_EVENT:
            break;
        case BLE_CONNECTING_STOPPED_EVENT:
            break;
        case BLE_CONNECTING_FAILED_EVENT:
            break;
        case BLE_DISCONNECT_EVENT:
            aob_conn_set_encrypt_state(con_lid, false);
#if (IS_KEEP_BLE_ADV_ON_ENABLED)
            {
                bool is_nv_master = (BLE_AUDIO_TWS_MASTER == ble_audio_get_tws_nv_role());
                aob_conn_start_adv(is_nv_master, NULL);
            }
#endif

#if defined(AOB_MOBILE_ENABLED)
#ifndef BLE_STACK_NEW_DESIGN
            app_bap_capa_cli_clean_up_resource(con_lid);
#endif
#endif
            break;
        case BLE_CONN_PARAM_UPDATE_REQ_EVENT:
            break;
        case BLE_CONN_PARAM_UPDATE_FAILED_EVENT:
            break;
        case BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT:
            break;
        case BLE_SET_RANDOM_BD_ADDR_EVENT:
            break;
        case BLE_ADV_STARTED_EVENT:
            break;
        case BLE_ADV_STARTING_FAILED_EVENT:
            break;
        case BLE_ADV_STOPPED_EVENT:
            break;
        case BLE_SCAN_STARTED_EVENT:
            break;
        case BLE_SCAN_DATA_REPORT_EVENT:
            aob_conn_scan_data_report_callback(event);
            break;
        case BLE_SCAN_STARTING_FAILED_EVENT:
            break;
        case BLE_SCAN_STOPPED_EVENT:
            break;
        case BLE_CREDIT_BASED_CONN_REQ_EVENT:
            break;
        case BLE_RPA_ADDR_PARSED_EVENT:
            break;
        case BLE_GET_TX_PWR_LEVEL:
            break;
        case BLE_TX_PWR_REPORT_EVENT:
            break;
        case BLE_PATH_LOSS_REPORT_EVENT:
            break;
        case BLE_SUBRATE_CHANGE_EVENT:
            break;
        default:
            break;
    }

    ble_audio_core_evt_handle(event);
}

void aob_conn_api_init(void)
{
    app_ble_core_evt_cb_register(ble_audio_event_callback);
#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
    aob_enable_le_channel_map_report(true);
#endif
    memset(&aob_conn_info, 0, sizeof(struct aob_connection_info));
}

void aob_conn_dump_state_info(void)
{
    uint8_t mob_lid[AOB_COMMON_MOBILE_CONNECTION_MAX] = {0};

    memset(mob_lid, INVALID_BLE_CONIDX, sizeof(mob_lid));

    uint8_t dev_count = aob_conn_get_connected_mobile_lid(mob_lid);

    LOG_I("%s", __func__);
    LOG_I("role %d mob1_lid %d mob2_lid %d", aob_conn_get_cur_role(), mob_lid[0], mob_lid[1]);

#ifdef AOB_MOBILE_ENABLED
    uint8_t ADDR_EMPTY[GAP_BD_ADDR_LEN] = {0};

    uint8_t pa_lid = ble_audio_earphone_info_get_bis_pa_lid();

    const app_gaf_bap_adv_id_t *pa_addr = aob_bis_sink_get_pa_addr_info(pa_lid);

    if (pa_addr != NULL && memcmp(pa_addr->addr, ADDR_EMPTY, sizeof(ADDR_EMPTY)))
    {
        LOG_I("PA sync[%d] address: %02x:%02x:%02x:%02x:%02x:%02x", pa_lid,
                  pa_addr->addr[0], pa_addr->addr[1], pa_addr->addr[2],
                  pa_addr->addr[3], pa_addr->addr[4], pa_addr->addr[5]);
    }
    else
    {
        LOG_I("PA sync[255] address: 00:00:00:00:00:00");
    }

    gaf_bis_stream_dump_dma_trigger_status();
#endif

    for (uint8_t index = 0; index < dev_count; index++)
    {
        ble_bdaddr_t *p_addr = aob_conn_get_remote_address(mob_lid[index]);
        if (NULL != p_addr)
        {
            LOG_I("Mob[%d] address: %02x:%02x:%02x:%02x:%02x:%02x", index,
                  p_addr->addr[0], p_addr->addr[1], p_addr->addr[2],
                  p_addr->addr[3], p_addr->addr[4], p_addr->addr[5]);
        }
        uint8_t ase_lid = 0;
        uint8_t nb_ase = 0;
        uint8_t ase_lid_list[APP_BAP_DFT_ASCS_NB_ASE_CHAR] = {0};
        nb_ase = aob_media_get_curr_streaming_ase_lid_list(mob_lid[index], ase_lid_list);
        if (nb_ase == 0)
        {
            LOG_I("Mob[%d]: direction 0 ase state idle", index);
            LOG_I("Mob[%d]: direction 1 ase state idle", index);
        }
        else
        {
            gaf_stream_dump_dma_trigger_status();
        }
        for (int idx = 0; idx < nb_ase; idx++)
        {
            ase_lid = ase_lid_list[idx];
            LOG_I("Mob[%d]: dir %d context %d ase state %d media state %d mic %d sample_rate %d codec_type %d",
                  index, aob_media_get_ase_direction(ase_lid),
                  aob_media_get_cur_context_type(ase_lid), aob_media_get_cur_ase_state(ase_lid), aob_media_get_state(index),
                  aob_media_get_mic_state(index), aob_media_get_sample_rate(ase_lid), aob_media_get_codec_type(ase_lid));
        }
    }
}

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
void aob_enable_le_channel_map_report(bool isEnabled)
{
    btif_me_enable_ble_audio_dbg_trc_report(isEnabled);
}
#endif

uint16_t aob_ble_get_conhdl(uint8_t conidx)
{
    return bes_ble_gap_get_conhdl_from_conidx(conidx);
}

void aob_conn_set_encrypt_state(uint8_t con_lid, bool is_encrypt)
{
    is_encrypt ?\
    set_bit(aob_conn_info.encrypted_bf, con_lid) :
    clr_bit(aob_conn_info.encrypted_bf, con_lid);
}

bool aob_conn_get_encrypt_state(uint8_t con_lid)
{
    return get_bit(aob_conn_info.encrypted_bf, con_lid);
}

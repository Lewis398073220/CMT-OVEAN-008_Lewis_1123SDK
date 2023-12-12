/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "string.h"
#include "co_math.h" // Common Maths Definition
#include "cmsis_os.h"
#include "ble_app_dbg.h"
#include "stdbool.h"
#include "app_thread.h"
#include "app_utils.h"
#include "bluetooth_bt_api.h"
#include "app_a2dp.h"
#include "apps.h"
#include "gapc_msg.h"
#include "gapm_msg.h"
#include "app.h"
#include "app_sec.h"
#include "app_ble_include.h"
#include "nvrecord_bt.h"
#include "nvrecord_ble.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "besbt.h"
#include "rwprf_config.h"
#include "app_sec.h"
#if defined(IBRT)
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#endif

#ifdef ANCC_ENABLED
#include "app_ancc.h"
#endif

#ifdef BLE_HID_ENABLE
#include "app_hid.h"
#endif

#ifdef BLE_BATT_ENABLE
#include "app_batt.h"
#endif

/************************private macro defination***************************/
#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID       "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN   (4)
#endif //(BLE_APP_HID)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE   "\x03\x19\xC2\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN  (4)

/************************private type defination****************************/

/************************extern function declearation***********************/

/**********************private function declearation************************/

/************************private variable defination************************/
static APP_BLE_CORE_GLOBAL_HANDLER_FUNC g_ble_core_global_handler_ind = NULL;

static bool ble_core_stub_adv_enable = true;

APP_BLE_CORE_EVENT_CALLBACK app_ble_core_evt_cb_p;

/****************************function defination****************************/
void app_ble_core_register_global_handler_ind(APP_BLE_CORE_GLOBAL_HANDLER_FUNC handler)
{
    g_ble_core_global_handler_ind = handler;
}

static void ble_connect_event_handler(ble_event_t *event, void *output)
{
    app_stop_fast_connectable_ble_adv_timer();
}

static void ble_connect_bond_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection pairing complete
    POSSIBLY_UNUSED uint8_t conidx = event->p.connect_bond_handled.conidx;

    if (!event->p.connect_bond_handled.success)
    {
        return;
    }

#if defined(ANCC_ENABLED)
    app_ancc_enable(event->p.connect_bond_handled.conidx);
#endif

#if defined(BLE_HID_ENABLE)
    // Enable HID Service
    app_hid_enable_prf(event->p.connect_bond_handled.conidx, true);
#endif //(BLE_APP_HID)

#if defined(BLE_BATT_ENABLE)
    // Enable Battery Service
    app_batt_enable_prf(event->p.connect_bond_handled.conidx);
#endif


}

static void ble_connect_nc_exch_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection numeric comparison - Exchange of Numeric Value
}

static void ble_connect_encrypt_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection encrypt complete
    if ((GAP_PAIRING_BOND_AUTH == event->p.connect_encrypt_handled.pairing_lvl) ||
        (GAP_PAIRING_BOND_SECURE_CON == event->p.connect_encrypt_handled.pairing_lvl))
    {
        ble_connect_bond_event_handler(event, output);
    }
}

static void ble_stopped_connecting_event_handler(ble_event_t *event, void *output)
{
}

static void ble_connecting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble start connecting failed. Mostly it's param invalid.
}

static void ble_disconnect_event_handler(ble_event_t *event, void *output)
{
    TRACE(0, "%s errCode 0x%x", __func__, event->p.disconnect_handled.errCode);

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);

#if defined(BLE_HID_ENABLE)
    // Enable HID Service
    app_hid_enable_prf(event->p.disconnect_handled.conidx, false);
#endif //(BLE_APP_HID)
}

static void ble_conn_param_update_req_event_handler(ble_event_t *event, void *output)
{
#if defined(BT_A2DP_SUPPORT)
    if (a2dp_is_music_ongoing())
    {
        *(bool *)output = false;
        return;
    }
#endif

#if defined(BT_HFP_SUPPORT)
    if (btapp_hfp_is_sco_active())
    {
        *(bool *)output = false;
    }
#endif
}

static void ble_conn_param_update_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update failed
}

static void ble_conn_param_update_successful_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update successful
}

static void ble_set_random_bd_addr_event_handler(ble_event_t *event, void *output)
{
    // Indicate that a new random BD address set in lower layers
}

static void ble_adv_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been started success
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

static void ble_adv_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv starting failed
}

static void ble_adv_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been stopped success
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

static void ble_scan_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been started success
}

__attribute__((unused)) static void _ble_audio_handle_adv_data(ble_event_t *event)
{
}

static void ble_scan_data_report_event_handler(ble_event_t *event, void *output)
{
    // Scan data report
}

static void ble_scan_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan starting failed
}

static void ble_scan_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_credit_based_conn_req_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_get_tx_pwr_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_tx_pwr_report_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_path_loss_report_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_set_ral_cmp_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_subrate_change_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void ble_encrypt_ltk_report_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static const ble_event_handler_t ble_event_handler_tab[] =
{
    {BLE_LINK_CONNECTED_EVENT, ble_connect_event_handler},
    {BLE_CONNECT_BOND_EVENT, ble_connect_bond_event_handler},
    {BLE_CONNECT_NC_EXCH_EVENT, ble_connect_nc_exch_event_handler},
    {BLE_CONNECT_ENCRYPT_EVENT, ble_connect_encrypt_event_handler},
    {BLE_CONNECTING_STOPPED_EVENT, ble_stopped_connecting_event_handler},
    {BLE_CONNECTING_FAILED_EVENT, ble_connecting_failed_event_handler},
    {BLE_DISCONNECT_EVENT, ble_disconnect_event_handler},
    {BLE_CONN_PARAM_UPDATE_REQ_EVENT, ble_conn_param_update_req_event_handler},
    {BLE_CONN_PARAM_UPDATE_FAILED_EVENT, ble_conn_param_update_failed_event_handler},
    {BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT, ble_conn_param_update_successful_event_handler},
    {BLE_SET_RANDOM_BD_ADDR_EVENT, ble_set_random_bd_addr_event_handler},
    {BLE_ADV_STARTED_EVENT, ble_adv_started_event_handler},
    {BLE_ADV_STARTING_FAILED_EVENT, ble_adv_starting_failed_event_handler},
    {BLE_ADV_STOPPED_EVENT, ble_adv_stopped_event_handler},
    {BLE_SCAN_STARTED_EVENT, ble_scan_started_event_handler},
    {BLE_SCAN_DATA_REPORT_EVENT, ble_scan_data_report_event_handler},
    {BLE_SCAN_STARTING_FAILED_EVENT, ble_scan_starting_failed_event_handler},
    {BLE_SCAN_STOPPED_EVENT, ble_scan_stopped_event_handler},
    {BLE_CREDIT_BASED_CONN_REQ_EVENT, ble_credit_based_conn_req_event_handler},
    {BLE_GET_TX_PWR_LEVEL, ble_get_tx_pwr_event_handler},
    {BLE_TX_PWR_REPORT_EVENT, ble_tx_pwr_report_event_handler},
    {BLE_PATH_LOSS_REPORT_EVENT, ble_path_loss_report_event_handler},
    {BLE_SET_RAL_CMP_EVENT, ble_set_ral_cmp_event_handler},
    {BLE_SUBRATE_CHANGE_EVENT, ble_subrate_change_event_handler},
    {BLE_ENCRYPT_LTK_REPORT_EVENT, ble_encrypt_ltk_report_event_handler},
};

//handle the event that from ble lower layers
void app_ble_core_global_handle(ble_event_t *event, void *output)
{
    uint8_t evt_type = event->evt_type;
    uint16_t index = 0;
    const ble_event_handler_t *p_ble_event_hand = NULL;

    for (index = 0; index < ARRAY_SIZE(ble_event_handler_tab); index++)
    {
        p_ble_event_hand = &ble_event_handler_tab[index];
        if (p_ble_event_hand->evt_type == evt_type)
        {
            p_ble_event_hand->func(event, output);
            if (app_ble_core_evt_cb_p)
            {
                app_ble_core_evt_cb_p(event);
            }
            break;
        }
    }

    if (g_ble_core_global_handler_ind)
    {
        g_ble_core_global_handler_ind(event, output);
    }

    app_ble_core_print_ble_state();
}

void app_ble_core_evt_cb_register(APP_BLE_CORE_EVENT_CALLBACK cb)
{
    LOG_I("%s cb %p", __func__, cb);
    app_ble_core_evt_cb_p = cb;
}

void ble_roleswitch_start(void)
{
    LOG_I("%s", __func__);
#if (BLE_AUDIO_ENABLED == 0) && defined(IBRT)
    if(TWS_UI_SLAVE != app_ibrt_if_get_ui_role())
    {
        app_ble_sync_ble_info();
    }
#endif

    // disable adv after role switch start
    app_ble_force_switch_adv(BLE_SWITCH_USER_RS, false);
}

void ble_roleswitch_complete(uint8_t newRole)
{
#ifdef BLE_ADV_RPA_ENABLED
    app_ble_refresh_irk();
#else
    btif_me_set_ble_bd_address(bt_get_ble_local_address());
#endif

#if defined(IBRT)
    LOG_I("%s newRole %d", __func__, newRole);
    app_ble_force_switch_adv(BLE_SWITCH_USER_RS, true);
#if (BLE_AUDIO_ENABLED == 0)
    if (newRole == IBRT_SLAVE)
    {
        app_ble_disconnect_all();
    }
#endif
#endif
}

void ble_role_update(uint8_t newRole)
{
#if defined(IBRT)
    LOG_I("%s newRole %d", __func__, newRole);
#if (BLE_AUDIO_ENABLED == 0)
    if (newRole == IBRT_SLAVE)
    {
        app_ble_disconnect_all();
    }
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
#endif
}

void ble_ibrt_event_entry(uint8_t ibrt_evt_type)
{
#if defined(IBRT) && defined(IBRT_UI)
    LOG_I("%s evt_type %d", __func__, ibrt_evt_type);
    if (APP_UI_EV_CASE_OPEN == ibrt_evt_type)
    {
        app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, true);
    }
    else if (APP_UI_EV_UNDOCK == ibrt_evt_type)
    {
        app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, true);
    }
    else if (APP_UI_EV_CASE_CLOSE == ibrt_evt_type)
    {
        // disconnect all of the BLE connections when box closed
        app_ble_disconnect_all();
        app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, false);
    }
#endif
}

void ble_core_enable_stub_adv(void)
{
    ble_core_stub_adv_enable = true;
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void ble_core_disable_stub_adv(void)
{
    ble_core_stub_adv_enable = false;
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

static void app_ble_stub_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    bool adv_enable = false;
    do {
#if (BLE_APP_HID)
        BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T*)param;
        memcpy(&cmd->advData[cmd->advDataLen],
            APP_HID_ADV_DATA_UUID, APP_HID_ADV_DATA_UUID_LEN);
        cmd->advDataLen += APP_HID_ADV_DATA_UUID_LEN;
        memcpy(&cmd->advData[cmd->advDataLen],
            APP_HID_ADV_DATA_APPEARANCE, APP_ADV_DATA_APPEARANCE_LEN);
        cmd->advDataLen += APP_ADV_DATA_APPEARANCE_LEN;


        //cmd->advUserInterval[USER_BLE_DEMO0] = BLE_ADVERTISING_INTERVAL;
        adv_enable = true;
        break;
#endif //(BLE_APP_HID)

#if defined(IBRT)
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

        if (!p_ibrt_ctrl->init_done)
        {
            LOG_I("%s ibrt don't init", __func__);
            break;
        }
        else if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
        {
            // LOG_I("%s role %d isn't MASTER", __func__, p_ibrt_ctrl->current_role);
            break;
        }
#endif

        // ctkd needs ble adv no matter whether a mobile bt link has been established or not
#ifdef CTKD_ENABLE
        set_rsp_dist_lk_bit_field_func dist_lk_set_cb = app_sec_reg_dist_lk_bit_get_callback();
        if ((dist_lk_set_cb && dist_lk_set_cb()) || (!dist_lk_set_cb))
        {
            adv_enable = true;
        }
#else
        if (app_ble_get_user_register() & ~(1 << USER_STUB))
        {
            LOG_I("%s have other user register 0x%x", __func__, app_ble_get_user_register());
        }
        else if (false == ble_core_stub_adv_enable)
        {
            LOG_I("%s stub adv disable", __func__);
        }
        else
        {
#ifdef CUSTOMER_DEFINE_ADV_DATA
            adv_enable = false;
#else
            adv_enable = true;
#endif
        }
#endif
    } while(0);

    app_ble_data_fill_enable(USER_STUB, adv_enable);
}

void app_ble_stub_user_init(void)
{
    LOG_I("%s", __func__);
    ble_core_enable_stub_adv();
    app_ble_register_data_fill_handle(USER_STUB, (BLE_DATA_FILL_FUNC_T)app_ble_stub_user_data_fill_handler, false);
}

#ifdef TWS_SYSTEM_ENABLED
static void ble_sync_info_prepare_handler(uint8_t *buf,
    uint16_t *totalLen, uint16_t *len, uint16_t expectLen)
{
    uint8_t headerLen = OFFSETOF(NV_RECORD_PAIRED_BLE_DEV_INFO_T, ble_nv);
    uint8_t infoLen = sizeof(BleDeviceinfo);

    *totalLen = sizeof(NV_RECORD_PAIRED_BLE_DEV_INFO_T);
    uint16_t sentLen = 0; // The length of sent done
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *pBleInfo = nv_record_blerec_get_ptr();

    // If the total length exceeds the maximum, sent in segments
    if (*totalLen <= TWS_SYNC_BUF_SIZE) {
        *len = *totalLen;
    } else if (!expectLen) {
        // Start of info
        *len = headerLen + 2 * infoLen;
    } else {
        // End of info
        *len = expectLen;
        sentLen = *totalLen - expectLen;
        pBleInfo = pBleInfo + sentLen;
    }

    memcpy(buf, pBleInfo, *len);
}

static void ble_sync_info_receive_continue_process(uint8_t *buf, uint16_t length, bool isContinueInfo)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *pReceivedBleInfo = (NV_RECORD_PAIRED_BLE_DEV_INFO_T *)buf;
    if (!isContinueInfo) {
        // Refresh basic info once
        nv_record_extension_update_tws_ble_info(pReceivedBleInfo);
    }

    // for le audio case, no need to exchange ble pairing info
#if (BLE_AUDIO_ENABLED == 0)
    // pair info
    uint8_t recordNum = 0;
    if (isContinueInfo) {
        recordNum = length / sizeof(BleDeviceinfo);
    } else {
        uint8_t headerLen = OFFSETOF(NV_RECORD_PAIRED_BLE_DEV_INFO_T, ble_nv);
        recordNum = (length - headerLen) / sizeof(BleDeviceinfo);
    }

    for (uint32_t index = 0; index < recordNum; index++)
    {
        nv_record_blerec_add(&pReceivedBleInfo->ble_nv[index].pairingInfo);
    }

    appm_refresh_ble_irk();

    if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
    {
        ble_gap_ral_dev_info_t devicesInfo[RESOLVING_LIST_MAX_NUM];
        uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
        appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);
    }
#endif
}

static void ble_sync_info_received_handler(uint8_t *buf, uint16_t length, bool isContinueInfo)
{
    TRACE(2, "%s length:%d %d", __func__, length, isContinueInfo);
    ble_sync_info_receive_continue_process(buf, length, isContinueInfo);
#ifdef CFG_APP_SEC
    app_sec_init();
#endif
}

static void ble_sync_info_rsp_received_handler(uint8_t *buf, uint16_t length, bool isContinueInfo)
{
    TRACE(2, "%s length:%d %d", __func__, length, isContinueInfo);
    ble_sync_info_receive_continue_process(buf, length, isContinueInfo);
}

void app_ble_mode_tws_sync_init(void)
{
    TWS_SYNC_USER_T userBle = {
        ble_sync_info_prepare_handler,
        ble_sync_info_received_handler,
        ble_sync_info_prepare_handler,
        ble_sync_info_rsp_received_handler,
        NULL,
    };

    app_ibrt_if_register_sync_user(TWS_SYNC_USER_BLE_INFO, &userBle);
}

void app_ble_sync_ble_info(void)
{
    app_ibrt_if_prepare_sync_info();
    app_ibrt_if_sync_info(TWS_SYNC_USER_BLE_INFO);
    app_ibrt_if_flush_sync_info();
}
#endif

void app_ble_core_print_ble_state(void)
{
#ifdef BES_AUTOMATE_TEST
    BLE_MODE_ENV_T *ble_mode_env_p = app_ble_get_mode_env();
    AUTO_TEST_BLE_STATE_T auto_test_ble_state;
    auto_test_ble_state.head[0] = 0xD2;
    auto_test_ble_state.head[1] = 0x7B;
    auto_test_ble_state.head[2] = 0x84;
    auto_test_ble_state.length = sizeof(auto_test_ble_state);

    auto_test_ble_state.ble_is_busy = ble_mode_env_p->ble_is_busy;
    for (uint8_t i=0; i<BES_BLE_ACTIVITY_MAX; i++)
    {
        auto_test_ble_state.actv_state[i] = app_ble_get_actv_state(i);
    }
    auto_test_ble_state.conn_cnt = app_ble_connection_count();

    DUMP8("%02x", (uint8_t *)&auto_test_ble_state, auto_test_ble_state.length);
#endif
}


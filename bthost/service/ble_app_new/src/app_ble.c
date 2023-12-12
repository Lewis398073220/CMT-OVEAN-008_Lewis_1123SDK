/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
 *
 ****************************************************************************/
#ifdef BLE_HOST_SUPPORT
#undef MOUDLE
#define MOUDLE APP_BLE
#include "apps.h"
#include "app_ble.h"
#include "app_a2dp.h"
#include "app_hfp.h"
#include "app_bt.h"
#include "nvrecord_ble.h"
#include "bt_drv_reg_op.h"
#include "hci_i.h"
#ifndef BLE_ONLY_ENABLED
#include "sdp_service.h"
#endif
#ifdef IBRT
#include "app_ibrt_internal.h"
#include "app_tws_ibrt.h"
#include "app_tws_ibrt_conn_api.h"
#if BLE_AUDIO_ENABLED
#include "app_ui_api.h"
#endif
#endif
#ifdef GFPS_ENABLED_X
#include "gfps_ble.h"
#endif

#define APP_BLE_GAP_STACK_READY 0x02 // STACK_READY_BLE

extern bool app_is_power_off_in_progress(void);
static void app_ble_impl_refresh_adv(void);
#if (BLE_AUDIO_ENABLED)
bool aob_conn_start_adv(bool br_edr_support, bool in_paring);
#endif

static ble_global_t g_ble_global;

ble_global_t *ble_get_global(void)
{
    return &g_ble_global;
}

void app_ble_core_evt_cb_register(APP_BLE_CORE_EVENT_CALLBACK cb)
{
    ble_global_t *g = ble_get_global();
    g->ble_core_evt_cb = cb; // app_ble_core_evt_cb_p
}

void app_ble_core_register_global_handler_ind(APP_BLE_CORE_GLOBAL_HANDLER_FUNC handler)
{
    ble_global_t *g = ble_get_global();
    g->ble_global_handler = handler; // g_ble_core_global_handler_ind
}

void app_sec_reg_dist_lk_bit_set_callback(set_rsp_dist_lk_bit_field_func callback)
{
    ble_global_t *g = ble_get_global();
    g->dist_lk_set_cb = callback;
}

void *app_sec_reg_dist_lk_bit_get_callback(void)
{
    ble_global_t *g = ble_get_global();
    return g->dist_lk_set_cb;
}

void app_ble_register_ia_exchanged_callback(smp_identify_addr_exch_complete callback)
{
    ble_global_t *g = ble_get_global();
    g->ble_smp_info_derived_from_bredr_complete = callback;
}

void app_ble_mtu_exec_ind_callback_register(app_ble_mtu_exch_cb_t cb)
{
    ble_global_t *g = ble_get_global();
    g->ble_link_mtu_exch_ind_callback = cb;
}

void app_ble_mtu_exec_ind_callback_deregister(void)
{
    ble_global_t *g = ble_get_global();
    g->ble_link_mtu_exch_ind_callback = NULL;
}

uint8_t app_ble_own_addr_type(void)
{
    const gap_local_info_t *local = gap_local_info();
    bt_addr_type_t own_addr_type = BT_ADDR_TYPE_PUBLIC;
    if (local->use_random_identity_address)
    {
        own_addr_type = BT_ADDR_TYPE_RANDOM;
    }
    return own_addr_type;
}

static bt_addr_type_t app_ble_get_own_addr_type(uint8_t ia_rpa_npa)
{
    const gap_local_info_t *local = gap_local_info();
    bt_addr_type_t own_addr_type = app_ble_own_addr_type();

    CO_LOG_MAIN_S_3(BT_STS_OWN_ADDRESS_TYPE, OWNT, local->use_random_identity_address, local->address_reso_support, ia_rpa_npa);

    if (ia_rpa_npa == APP_GAPM_GEN_RSLV_ADDR && local->address_reso_support)
    {
        own_addr_type = (bt_addr_type_t)(own_addr_type + 2);
    }

    return own_addr_type;
}

bool app_ble_get_peer_solved_addr(uint8_t conidx, ble_bdaddr_t* p_addr)
{
    ble_bdaddr_t peer_bdaddr;
    bt_bdaddr_t zero_bdaddr = {{0}};

    peer_bdaddr = gap_get_peer_resolved_address(gap_zero_based_ble_conidx_as_hdl(conidx));
    if (memcmp(peer_bdaddr.addr, &zero_bdaddr, sizeof(bt_bdaddr_t)) == 0)
    {
        return false;
    }

    if (p_addr)
    {
        *p_addr = peer_bdaddr;
    }

    return true;
}

ble_bdaddr_t app_get_current_ble_addr(void)
{
    return gap_local_identity_address(app_ble_own_addr_type());
}

ble_bdaddr_t app_ble_get_local_identity_addr(uint8_t conidx)
{
    ble_bdaddr_t ble_ia_addr;
    gap_conn_item_t *conn = NULL;

    conn = gap_get_conn_item(gap_conn_idx_as_hdl(conidx));
    if (conn)
    {
        ble_ia_addr = gap_conn_own_identity_address(conn);
    }
    else
    {
        ble_ia_addr = gap_local_identity_address(app_ble_own_addr_type());
    }

    return ble_ia_addr;
}

const uint8_t *app_ble_get_local_rpa_addr(uint8_t conidx)
{
    gap_conn_item_t *conn = gap_get_conn_item(gap_conn_idx_as_hdl(conidx));
    if (conn && (conn->own_addr_type == BT_ADDR_TYPE_PUB_IA || conn->own_addr_type == BT_ADDR_TYPE_RND_IA))
    {
        return conn->own_rpa.address;
    }
    else
    {
        return gap_local_host_rpa()->address;
    }
}

static void app_ble_read_local_rpa_cmpl(uint16_t cmd_opcode, struct hci_cmd_evt_param_t *param)
{
    struct hci_le_read_local_rpa_cmpl *cmpl = (struct hci_le_read_local_rpa_cmpl *)param->cmpl_event;
    if (cmpl->status == HCI_ERROR_NO_ERROR)
    {
        gap_local_info_t *local = gap_local_info();
        local->host_local_rpa = *((bt_bdaddr_t *)cmpl->local_rpa);
    }
    else
    {
        CO_LOG_ERR_1(BT_STS_INVALID_STATUS, cmpl->status);
    }
}

#if defined (BLE_ADV_RPA_ENABLED)
static inline void app_ble_fake_resolving_list_item_using_self_info(void)
{
    CO_LOG_MAIN_S_0(BT_STS_SET_FILTER_LIST, FRLS);
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    // Always add local address into RPA list
    gap_resolving_list_add(BT_ADDR_TYPE_PUBLIC,
                           (bt_bdaddr_t *)bt_get_ble_local_address(),
                           dev_info->self_info.ble_irk, false, false, false);
}
#endif

void app_ble_read_local_rpa_addr(bt_addr_type_t addr_type, const bt_bdaddr_t *peer_addr)
{
    gap_read_local_rpa(addr_type, peer_addr, app_ble_read_local_rpa_cmpl, NULL);
}

bool app_ble_is_remote_mobile_connected(const ble_bdaddr_t *p_addr)
{
    gap_conn_item_t *conn = NULL;
    conn = gap_get_conn_by_le_address((bt_addr_type_t)p_addr->addr_type, (bt_bdaddr_t *)p_addr->addr);
    return (conn != NULL);
}

uint8_t app_ble_connection_count(void)
{
    return gap_count_ble_connections();
}

bool app_is_arrive_at_max_ble_connections(void)
{
    return app_ble_connection_count() >= BLE_CONNECTION_MAX;
}

bool app_ble_is_any_connection_exist(void)
{
    return app_ble_connection_count() > 0;
}

bool app_ble_is_connection_on(uint8_t index)
{
    return gap_get_conn_item(gap_zero_based_ble_conidx_as_hdl(index)) != NULL;
}

uint16_t app_ble_get_conhdl_from_conidx(uint8_t conidx)
{
    return gap_conn_hdl(gap_zero_based_conidx_to_ble_conidx(conidx));
}

void app_ble_set_white_list(BLE_WHITE_LIST_USER_E user, ble_bdaddr_t *bdaddr, uint8_t size)
{
    CO_LOG_MAIN_S_3(BT_STS_SET_FILTER_LIST, STFL, (user<<16)|size, bdaddr->addr_type,
        (bdaddr->addr[0]<<24)|(bdaddr->addr[1]<<16)|(bdaddr->addr[2]<<8)|bdaddr->addr[5]);
    gap_filter_list_clear();
    gap_filter_list_add_user_item(user, bdaddr, size);
}

void app_ble_clear_white_list(BLE_WHITE_LIST_USER_E user)
{
    gap_filter_list_remove_user_item(user);
}

void app_ble_clear_all_white_list(void)
{
    gap_filter_list_clear();
}

void app_ble_add_dev_to_rpa_list_in_controller(const ble_bdaddr_t *ble_addr, const uint8_t *irk)
{
    gap_resolving_list_add((bt_addr_type_t)ble_addr->addr_type, (bt_bdaddr_t *)ble_addr->addr, irk, true, true, false);
}

static uint32_t app_ble_count_irk_distributed_records(void)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    BleDeviceinfo *record = NULL;
    uint32_t count = dev_info->saved_list_num;
    uint32_t irk_distributed_records = 0;
    int i = 0;

    for (; i < count; i++)
    {
        record = dev_info->ble_nv + i;
        if (record->pairingInfo.bond_info_bf & CO_BIT_MASK(BONDED_WITH_IRK_DISTRIBUTED))
        {
            irk_distributed_records += 1;
        }
    }

    return irk_distributed_records;
}

void app_ble_add_devices_info_to_resolving(void)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = NULL;
    BleDeviceinfo *record = NULL;
    bool addr_reso_prev_state = 0;
    bool set_reso_enable = false;
    uint32_t total_ble_records = 0;
    uint32_t irk_distributed_records = 0;
    uint32_t handled_irk_records = 0;
    uint32_t i = 0;

    addr_reso_prev_state = gap_address_reso_is_enabled();

    // Clear resolving list first to trigger AdvA regenerated
    gap_resolving_list_clear();

#if defined (BLE_ADV_RPA_ENABLED)
    // Add an fake item that peer addr is device itself, seems like bonded with itself before,
    // it makes chance to start adv with local addr filled in peer addr field, then controller
    // can search resolving list and get this fake item to generate rpa with local irk filled.
    app_ble_fake_resolving_list_item_using_self_info();
#endif

    dev_info = nv_record_blerec_get_ptr();
    total_ble_records = dev_info->saved_list_num;
    irk_distributed_records = app_ble_count_irk_distributed_records();

    if (irk_distributed_records)
    {
        for (; i < total_ble_records; i++)
        {
            record = dev_info->ble_nv + i;
            if (record->pairingInfo.bond_info_bf & CO_BIT_MASK(BONDED_WITH_IRK_DISTRIBUTED))
            {
                handled_irk_records += 1;
                set_reso_enable = ((handled_irk_records == irk_distributed_records) && addr_reso_prev_state);
                gap_resolving_list_add((bt_addr_type_t)record->pairingInfo.peer_addr.addr_type,
                    (bt_bdaddr_t *)record->pairingInfo.peer_addr.addr, record->pairingInfo.IRK, true, false, set_reso_enable);
                if (handled_irk_records == irk_distributed_records)
                {
                    break;
                }
            }
        }
    }

    if (handled_irk_records == 0 && addr_reso_prev_state)
    {
        gap_enable_address_resolution(true);
    }
}

void app_ble_send_security_req(uint8_t conidx)
{
    gap_start_authentication(gap_zero_based_ble_conidx_as_hdl(conidx), GAP_AUTH_STARTED_BY_UPPER_APP);
}

void app_ble_data_fill_enable(BLE_ADV_USER_E user, bool enable)
{
    ble_global_t *g = ble_get_global();
    if (user < BLE_ADV_USER_NUM)
    {
        g->data_fill_enable[user] = enable;
    }
}

void app_ble_register_data_fill_handle(BLE_ADV_USER_E user, BLE_DATA_FILL_FUNC_T func, bool enable)
{
    ble_global_t *g = ble_get_global();
    if (user < BLE_ADV_USER_NUM)
    {
        g->data_fill_func[user] = func;
    }
}

bool ble_adv_is_allowed(void)
{
    if (!gap_stack_is_ready())
    {
        return false;
    }

    if (app_is_power_off_in_progress())
    {
        return false;
    }

#ifndef CUSTOMER_DEFINE_ADV_DATA
    if (ble_get_global()->adv_force_disabled)
    {
        CO_LOG_ERR_1(BT_STS_ADV_FORCE_DISABLED, ble_get_global()->adv_force_disabled);
        return false;
    }
#endif

#if ((BLE_AUDIO_ENABLED == 0) && (!defined SASS_ENABLED)) && defined(BT_HFP_SUPPORT)
    if (btapp_hfp_is_sco_active())
    {
        return false;
    }
#endif

    if (app_is_arrive_at_max_ble_connections())
    {
        CO_LOG_WAR_0(BT_STS_REACH_MAX_NUMBER);
        return false;
    }

    return true;
}

void app_ble_dt_set_flags(gap_adv_param_t *adv_param, bool simu_bredr_support)
{
    uint8_t discoverable_mode = GAP_FLAGS_LE_NON_DISCOVERABLE_MODE;

    if (adv_param->connectable || adv_param->scannable)
    {
        discoverable_mode = adv_param->limited_discoverable_mode  ?
            GAP_FLAGS_LE_LIMITED_DISCOVERABLE_MODE :
            GAP_FLAGS_LE_GENERAL_DISCOVERABLE_MODE;
    }
    else
    {
        discoverable_mode = GAP_FLAGS_LE_NON_DISCOVERABLE_MODE;
    }

    if (discoverable_mode && adv_param->start_bg_advertising)
    {
        discoverable_mode = GAP_FLAGS_LE_GENERAL_DISCOVERABLE_MODE;
    }

    gap_dt_add_flags(&adv_param->adv_data, discoverable_mode, simu_bredr_support);
}

void app_ble_dt_set_local_name(gap_adv_param_t *adv_param, const char *cust_le_name)
{
    if (!gap_dt_buf_find_type(&adv_param->adv_data, GAP_DT_SHORT_LOCAL_NAME, GAP_DT_COMPLETE_LOCAL_NAME))
    {
        gap_dt_add_local_le_name(&adv_param->adv_data, adv_param->use_legacy_pdu, cust_le_name);
    }
}

static void app_ble_clean_adv_param(gap_adv_param_t *adv_param)
{
    gap_dt_buf_clear(&adv_param->adv_data);
    gap_dt_buf_clear(&adv_param->scan_rsp_data);
    memset(adv_param, 0, sizeof(gap_adv_param_t));
}

static ble_adv_activity_t *app_ble_find_adv_activity(uint8_t adv_handle)
{
    ble_global_t *g = ble_get_global();
    ble_adv_activity_t *adv = NULL;
    int activity_count = ARRAY_SIZE(g->adv);
    int i = 0;

    if (adv_handle == GAP_INVALID_ADV_HANDLE)
    {
        return NULL;
    }

    for (; i < activity_count; i += 1)
    {
        adv = g->adv + i;
        if (adv->adv_handle == adv_handle)
        {
            return adv;
        }
    }

    return NULL;
}

bool app_ble_check_device_master_role(void)
{
#if defined(IBRT)
#if defined(FREEMAN_ENABLED_STERO)
    return true;
#else
    if (app_ibrt_conn_is_freeman_mode())
    {
        return true;
    }
    return (app_ibrt_conn_get_ui_role() != TWS_UI_SLAVE);
#endif
#else
    return true;
#endif
}

static void app_ble_connection_encrypted(gap_conn_param_t *encrpted)
{
    if (app_ble_check_device_master_role())
    {
#if defined(ANCC_ENABLED)
        ble_ancc_start_discover(encrpted->connhdl);
#endif

#if defined(BLE_HID_HOST)
        ble_hid_host_start_discover(encrpted->connhdl);
#endif

#if defined (BLE_IAS_ENABLED)
        ble_iac_start_discover(encrpted->connhdl);
#endif
    }
}

// bit mask of the existing conn param modes
static uint32_t existingBleConnParamModes[BLE_CONNECTION_MAX] = {0};

// interval in the unit of 1.25ms
static const BLE_CONN_PARAM_CONFIG_T ble_conn_param_config[BLE_CONN_PARAM_MODE_NUM] =
{
    // default value: for the case of BLE just connected and the BT idle state
    {BLE_CONN_PARAM_MODE_DEFAULT, BLE_CONN_PARAM_PRIORITY_NORMAL, 36, 36, 0},

    {BLE_CONN_PARAM_MODE_AI_STREAM_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL1, 16, 16, 0},

    {BLE_CONN_PARAM_MODE_A2DP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL0, 48, 48, 0},

    {BLE_CONN_PARAM_MODE_HFP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL2, 48, 48, 0},

    {BLE_CONN_PARAM_MODE_OTA, BLE_CONN_PARAM_PRIORITY_HIGH, 12, 12, 0},

    {BLE_CONN_PARAM_MODE_OTA_SLOWER, BLE_CONN_PARAM_PRIORITY_HIGH, 20, 20, 0},

    {BLE_CONN_PARAM_MODE_SNOOP_EXCHANGE, BLE_CONN_PARAM_PRIORITY_HIGH, 8, 24, 0},

    // BAP Spec Table 8.4
    {BLE_CONN_PARAM_MODE_SVC_DISC, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL2, 8, 24, 0},

    {BLE_CONN_PARAM_MODE_ISO_DATA, BLE_CONN_PARAM_PRIORITY_HIGH, 48, 48, 0},
    // TODO: add mode cases if needed
};

typedef struct {
    BLE_CONN_PARAM_MODE_E mode;
    bool enable;
} app_ble_updata_conn_param_t;

static bool app_ble_update_conn_foreach(gap_conn_item_t *conn, void *priv)
{
    app_ble_updata_conn_param_t *p = (app_ble_updata_conn_param_t *)priv;
    app_ble_update_conn_param_mode_of_specific_connection(conn->con_idx, p->mode, p->enable);
    return gap_continue_loop;
}

void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool enable)
{
    app_ble_updata_conn_param_t param = {0};
    param.mode = mode;
    param.enable = enable;
    gap_conn_foreach(app_ble_update_conn_foreach, &param);
}

void app_ble_reset_conn_param_mode(uint8_t con_idx)
{
    uint8_t conidx = gap_zero_based_conidx(con_idx);
    existingBleConnParamModes[conidx] = 0;
}

void app_ble_update_conn_param_mode_of_specific_connection(uint8_t con_idx, BLE_CONN_PARAM_MODE_E mode, bool enable)
{
#ifdef IS_INITIATIVE_BLE_UPDATE_PARAMETER_DISABLED
    return;
#endif

    gap_update_params_t params = {0};
    const BLE_CONN_PARAM_CONFIG_T *pConfig = &ble_conn_param_config[mode];
    uint8_t conidx = gap_zero_based_conidx(con_idx);
    uint32_t index = 0;

    CO_LOG_INFO_4(BT_STS_CONN_STATUS, conidx, existingBleConnParamModes[conidx], mode, enable);

    if (enable)
    {
        if (existingBleConnParamModes[conidx] & (1 << mode))
        {
            // already existing, directly return
            return;
        } else {
            // update the bit-mask
            existingBleConnParamModes[conidx] |= (1 << mode);
            // not existing yet, need to go throuth the existing params to see whether
            // we need to update the param
            for (index = 0; index < ARRAY_SIZE(ble_conn_param_config); index++)
            {
                if (((uint32_t)1 << ble_conn_param_config[index].ble_conn_param_mode) &
                    existingBleConnParamModes[conidx])
                {
                    if (ble_conn_param_config[index].priority > pConfig->priority)
                    {
                        // one of the exiting param has higher priority than this one,
                        // so do nothing but update the bit-mask
                        return;
                    }
                }
            }
            // no higher priority conn param existing, so we need to apply this one
        }
    }
    else
    {
        // doesn't exist, directly return
        if (!(existingBleConnParamModes[conidx] & (1 << mode)))
        {
            return;
        }
        else
        {
            uint8_t priorityDisabled = pConfig->priority;
            // update the bit-mask
            existingBleConnParamModes[conidx] &= (~(1 << mode));

            if (0 == existingBleConnParamModes[conidx])
            {
                // restore to the default CI
                pConfig = &ble_conn_param_config[0];
                goto label_update;
            }

            pConfig = NULL;
            // existing, need to apply for the highest priority conn param
            for (index = 0; index < ARRAY_SIZE(ble_conn_param_config); index++)
            {
                if ((( uint32_t )1 << ( uint8_t )ble_conn_param_config[index].ble_conn_param_mode) &
                    existingBleConnParamModes[conidx])
                {
                    if (NULL != pConfig)
                    {
                        if (ble_conn_param_config[index].priority > pConfig->priority)
                        {
                            pConfig = &ble_conn_param_config[index];
                        }
                    }
                    else
                    {
                        pConfig = &ble_conn_param_config[index];
                    }
                }
            }

            if (pConfig && priorityDisabled < pConfig->priority) {
                // The priority of the disabled mode is less than that of the existing one,
                // indicating that it is already latest
                return;
            }
        }
    }

label_update:
    params.conn_interval_min_1_25ms = pConfig->conn_interval_min;
    params.conn_interval_max_1_25ms = pConfig->conn_interval_max;
    params.max_peripheral_latency = pConfig->conn_slave_latency_cnt;
    gap_update_le_conn_parameters(gap_zero_based_ble_conidx_as_hdl(conidx), &params);
}

#if GFPS_ENABLED
uint8_t delay_update_conidx = GAP_INVALID_CONIDX;
#define FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE (10000)
osTimerId fp_update_ble_param_timer = NULL;
static void fp_update_ble_connect_param_timer_handler(void const *param);
osTimerDef (FP_UPDATE_BLE_CONNECT_PARAM_TIMER, (void (*)(void const *))fp_update_ble_connect_param_timer_handler);
extern uint8_t amgr_is_bluetooth_sco_on (void);

static void fp_update_ble_connect_param_timer_handler(void const *param)
{
    if (delay_update_conidx != GAP_INVALID_CONIDX)
    {
        if (amgr_is_bluetooth_sco_on())
        {
            app_ble_update_conn_param_mode_of_specific_connection(delay_update_conidx, BLE_CONN_PARAM_MODE_HFP_ON, true);
        }
        else
        {
            app_ble_update_conn_param_mode_of_specific_connection(delay_update_conidx, BLE_CONN_PARAM_MODE_DEFAULT, true);
        }

        delay_update_conidx = GAP_INVALID_CONIDX;
    }
}

void fp_update_ble_connect_param_start(uint8_t ble_conidx)
{
    if (delay_update_conidx != GAP_INVALID_CONIDX)
    {
        return;
    }

    if (fp_update_ble_param_timer == NULL)
    {
        fp_update_ble_param_timer = osTimerCreate(osTimer(FP_UPDATE_BLE_CONNECT_PARAM_TIMER), osTimerOnce, NULL);
        if (fp_update_ble_param_timer == NULL)
        {
            return;
        }
    }

    delay_update_conidx = ble_conidx;
    osTimerStart(fp_update_ble_param_timer, FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE);
}

void fp_update_ble_connect_param_stop(uint8_t ble_conidx)
{
    if (delay_update_conidx == ble_conidx)
    {
        if (fp_update_ble_param_timer)
        {
            osTimerStop(fp_update_ble_param_timer);
        }
        delay_update_conidx = GAP_INVALID_CONIDX;
    }
}
#endif

static void app_ble_conn_param_update_req(gap_conn_update_req_t *update_req)
{
    bool accept = true;
    uint16_t conn_interval_min_1_25ms = update_req->params_req.conn_interval_min_1_25ms;
    uint32_t conn_interval_min_us = conn_interval_min_1_25ms * 1250;

#ifdef GFPS_ENABLED
    //make sure ble cnt interval is not less than 15ms, in order to prevent bt collision
    if (conn_interval_min_us < 15000)
    {
        fp_update_ble_connect_param_start(gap_zero_based_conidx(update_req->con_idx));
    }
    else
    {
        fp_update_ble_connect_param_stop(gap_zero_based_conidx(update_req->con_idx));
    }
#else
    bool music_ongoing = false;
    bool sco_active = false;
#ifdef BT_A2DP_SUPPORT
    music_ongoing = a2dp_is_music_ongoing();
#endif
#ifdef BT_HFP_SUPPORT
    sco_active = btapp_hfp_is_sco_active();
#endif
    if (music_ongoing || sco_active)
    {
        // when a2dp or sco streaming is on-going
        // make sure ble cnt interval is not less than 15ms, in order to prevent bt collision
        if (conn_interval_min_us < 15000)
        {
            accept = false;
        }
    }
#endif

    gap_accept_le_conn_parameters(update_req->connhdl, &update_req->params_req, accept);
}

static void app_ble_global_handle(ble_event_t *event, void *output)
{
    ble_global_t *g = ble_get_global();

    if (g->ble_core_evt_cb)
    {
        g->ble_core_evt_cb(event);
    }

    if (g->ble_global_handler)
    {
        g->ble_global_handler(event, output);
    }
}

static void app_ble_stack_ready(void)
{
    ble_global_t *g = ble_get_global();

    CO_LOG_MAIN_S_0(BT_STS_GLOBAL_STATUS, STKR);

    if (g->default_tx_pref_phy_bits || g->default_rx_pref_phy_bits)
    {
        gap_set_le_default_phy(g->default_tx_pref_phy_bits, g->default_rx_pref_phy_bits);
    }

    gap_set_le_host_feature_support(GAP_HOST_FEAT_BIT_CONN_SUBRAT_HOST_SUPP, true);

    gap_set_send_sec_error_rsp_directly(true);

    app_ble_add_devices_info_to_resolving();

    gap_enable_address_resolution(true);

    app_notify_stack_ready(APP_BLE_GAP_STACK_READY);
}

void app_ble_enable_and_start_adv(void)
{
#if (BLE_AUDIO_ENABLED)
    aob_conn_start_adv(false, true);
#else
    ble_core_enable_stub_adv();
#endif
}

void app_ble_ready_and_init_done(nvrec_appmode_e mode)
{
    ble_global_t *g = ble_get_global();
    uint16_t total_cccd_count = gap_local_info()->total_cccd_count;

    CO_LOG_MAIN_S_3(BT_STS_GLOBAL_STATUS, INTD, mode, total_cccd_count, (sizeof(gatt_client_cache_t)<<16) |
#if BLE_AUDIO_ENABLED || defined(KEEP_BLE_AUDIO_IN_NV_RECORD)
        sizeof(GATTC_SRV_ATTR_t)
#else
        0x0000
#endif
        );

    if (total_cccd_count > GATT_MAX_SERVER_CCCD)
    {
        CO_LOG_ERR_2(BT_STS_TOO_MANY_ACTIVE_CCCD, GATT_MAX_SERVER_CCCD, total_cccd_count);
    }

    g->app_mode = mode;

    app_ble_gap_update_local_database_hash();
}

int app_ble_recv_stack_global_event(uintptr_t priv, gap_global_event_t event, gap_global_event_param_t param)
{
    switch (event)
    {
        case GAP_EVENT_RECV_STACK_READY:
        {
            app_ble_stack_ready();
            break;
        }
        case GAP_EVENT_REFRESH_ADVERTISING:
        {
            CO_LOG_MAIN_S_1(BT_STS_REFRESH_ADV, RFSH, event);
            app_ble_refresh_adv();
            break;
        }
        case GAP_EVENT_RECV_TX_POWER_LEVEL:
        {
            gap_le_tx_power_param_t *tx_power = param.tx_power;
            if (tx_power->type == GAP_LE_ADV_TX_POWER_LEVEL)
            {
                CO_LOG_MAIN_S_2(BT_STS_GLOBAL_STATUS, ADPW,
                    tx_power->u.adv.adv_handle, tx_power->u.adv.curr_tx_power);
            }
            else
            {
                CO_LOG_MAIN_S_2(BT_STS_GLOBAL_STATUS, PWRG,
                    tx_power->u.range.min_tx_power, tx_power->u.range.max_tx_power);
            }
            break;
        }
        case GAP_EVENT_RECV_DERIVED_BLE_LTK:
        {
            gap_recv_derived_ltk_t *p = param.recv_derived_ltk;

            CO_LOG_MAIN_S_1(BT_STS_GLOBAL_STATUS, RLTK, p->ltk_generated_but_still_wait_peer_kdist);

            if (p->ltk_generated_but_still_wait_peer_kdist)
            {
                break;
            }

#if BLE_AUDIO_ENABLED
#ifdef IBRT // ble ltk derived from bredr, may prepare receive peer ble connection
            ble_global_t *g = ble_get_global();
            if (g->ble_smp_info_derived_from_bredr_complete != NULL)
            {
                ble_bdaddr_t ble_addr;
                ble_addr.addr_type = (p->peer_type & 0x01);
                memcpy(ble_addr.addr, &p->peer_addr, sizeof(bt_bdaddr_t));
                g->ble_smp_info_derived_from_bredr_complete(&ble_addr);
            }
#else
            app_ble_enable_and_start_adv();
#endif
#endif
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

static int app_ble_conn_event_handle(uintptr_t connhdl, gap_conn_event_t event, gap_conn_callback_param_t param)
{
    ble_global_t *g = ble_get_global();
    ble_event_t cb_event;

    memset(&cb_event, 0, sizeof(ble_event_t));

    switch (event)
    {
        case GAP_CONN_EVENT_OPENED:
        {
            gap_conn_param_t *opened = param.opened;
            const bt_bdaddr_t *la = &opened->conn->own_addr;
            const bt_bdaddr_t *pa = &opened->peer_addr;

            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, CNNS,
                ((opened->con_idx) << 24) |
                ((opened->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((opened->connhdl)),
                (la->address[0]<<24)|(la->address[1]<<16)|(la->address[5]<<8)|opened->own_addr_type,
                (pa->address[0]<<24)|(pa->address[1]<<16)|(pa->address[5]<<8)|opened->peer_type);

            app_ble_reset_conn_param_mode(opened->con_idx);

            cb_event.evt_type = BLE_LINK_CONNECTED_EVENT;
            cb_event.p.connect_handled.conidx = gap_zero_based_conidx(opened->con_idx);
            cb_event.p.connect_handled.peer_bdaddr.addr_type = (opened->peer_type & 0x01);
            memcpy(cb_event.p.connect_handled.peer_bdaddr.addr, opened->peer_addr.address, sizeof(bt_bdaddr_t));
            app_ble_global_handle(&cb_event, NULL);
            break;
        }
        case GAP_CONN_EVENT_FAILED:
        {
            gap_conn_failed_t *failed = param.conn_failed;

            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, CNNF,
                (failed->error_code<<24) |
                (failed->role << 16) | /* 0 central, 1 perpherial */
                (failed->peer_type),
                CO_COMBINE_UINT32_BE(failed->peer_addr.address),
                CO_COMBINE_UINT16_BE(failed->peer_addr.address+4));

            if (failed->role == 0) // central
            {
                cb_event.evt_type = BLE_CONNECTING_STOPPED_EVENT;
                cb_event.p.stopped_connecting_handled.peer_type = (failed->peer_type & 0x01);
                memcpy(cb_event.p.stopped_connecting_handled.peer_bdaddr, failed->peer_addr.address, sizeof(bt_bdaddr_t));
                app_ble_global_handle(&cb_event, NULL);
            }
            break;
        }
        case GAP_CONN_EVENT_CLOSED:
        {
            gap_conn_param_t *closed = param.closed;

            CO_LOG_MAIN_S_2(BT_STS_CONN_STATUS, CNNE,
                ((closed->con_idx) << 24) |
                ((closed->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((closed->connhdl)),
                closed->error_code);

            app_ble_reset_conn_param_mode(closed->con_idx);

            cb_event.evt_type = BLE_DISCONNECT_EVENT;
            cb_event.p.disconnect_handled.errCode = closed->error_code;
            cb_event.p.disconnect_handled.conidx = gap_zero_based_conidx(closed->con_idx);
            cb_event.p.disconnect_handled.peer_bdaddr.addr_type = (closed->peer_type & 0x01);
            memcpy(cb_event.p.disconnect_handled.peer_bdaddr.addr, closed->peer_addr.address, sizeof(bt_bdaddr_t));
            app_ble_global_handle(&cb_event, NULL);
            break;
        }
        case GAP_CONN_EVENT_MTU_EXCHANGED:
        {
            gap_conn_mtu_exchanged_t *p = param.mtu_exchanged;

            CO_LOG_MAIN_S_2(BT_STS_CONN_STATUS, MTUE,
                ((p->con_idx) << 24) |
                ((p->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((p->connhdl)),
                p->mtu);

            if (g->ble_link_mtu_exch_ind_callback)
            {
                g->ble_link_mtu_exch_ind_callback(gap_zero_based_conidx(p->con_idx), p->mtu);
            }
            break;
        }
        case GAP_CONN_EVENT_USER_CONFIRM:
        {
            gap_user_confirm_t *confirm = param.user_confirm;

            CO_LOG_MAIN_S_2(BT_STS_CONN_STATUS, USRC,
                ((confirm->con_idx) << 24) |
                ((confirm->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((confirm->connhdl)),
                confirm->type);

            if (confirm->type == GAP_USER_NUMERIC_CONFIRM)
            {
#ifdef BLE_SEC_ACCEPT_BY_CUSTOMER
                cb_event.evt_type = BLE_CONNECT_NC_EXCH_EVENT;
                cb_event.p.connect_nc_exch_handled.conidx = gap_zero_based_conidx(confirm->con_idx);
                cb_event.p.connect_nc_exch_handled.connhdl = confirm->connhdl;
                cb_event.p.connect_nc_exch_handled.confirm_value = confirm->numeric_confirm_value;
                app_ble_global_handle(&cb_event, NULL);
#else
                gap_input_numeric_confirm(confirm->connhdl, NULL, true);
#endif
            }
            break;
        }
        case GAP_CONN_EVENT_ENCRYPTED:
        {
            gap_conn_param_t *encrpt = param.encrypted;
            gap_conn_item_t *conn = encrpt->conn;
            uint8_t pairing_lvl = 0;

            CO_LOG_MAIN_S_2(BT_STS_CONN_STATUS, ENCT,
                ((encrpt->con_idx) << 24) |
                ((encrpt->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((encrpt->connhdl)),
                encrpt->error_code);

            if (encrpt->error_code)
            {
                cb_event.evt_type = BLE_CONNECT_BOND_FAIL_EVENT;
                cb_event.p.connect_bond_handled.conidx = gap_zero_based_conidx(encrpt->con_idx);
                cb_event.p.connect_bond_handled.success = false;
                cb_event.p.connect_bond_handled.reason = encrpt->error_code;
                app_ble_global_handle(&cb_event, NULL);
            }
            else
            {
                app_ble_connection_encrypted(encrpt);

                cb_event.evt_type = BLE_CONNECT_BOND_EVENT;
                cb_event.p.connect_bond_handled.conidx = gap_zero_based_conidx(encrpt->con_idx);
                cb_event.p.connect_bond_handled.success = true;
                cb_event.p.connect_bond_handled.reason = 0;
                app_ble_global_handle(&cb_event, NULL);

                cb_event.evt_type = BLE_CONNECT_ENCRYPT_EVENT;
                cb_event.p.connect_encrypt_handled.conidx = gap_zero_based_conidx(encrpt->con_idx);
                cb_event.p.connect_encrypt_handled.addr_type = (encrpt->peer_type & 0x01);
                memcpy(cb_event.p.connect_encrypt_handled.addr, encrpt->peer_addr.address, sizeof(bt_bdaddr_t));
                if (conn->sec.device_bonded)
                {
                    if (conn->sec.secure_pairing)
                    {
                        pairing_lvl = GAP_PAIRING_BOND_SECURE_CON;
                    }
                    else
                    {
                        pairing_lvl = conn->conn_flag.auth_mitm_protection ?
                            GAP_PAIRING_BOND_AUTH : GAP_PAIRING_BOND_UNAUTH;
                    }
                }
                else
                {
                    if (conn->sec.secure_pairing)
                    {
                        pairing_lvl = GAP_PAIRING_SECURE_CON;
                    }
                    else
                    {
                        pairing_lvl = conn->conn_flag.auth_mitm_protection ?
                            GAP_PAIRING_AUTH : GAP_PAIRING_UNAUTH;
                    }
                }
                cb_event.p.connect_encrypt_handled.pairing_lvl = pairing_lvl;
                app_ble_global_handle(&cb_event, NULL);
            }
            break;
        }
        case GAP_CONN_EVENT_RECV_KEY_DIST:
        {
            gap_recv_key_dist_t *key_dist = param.recv_key_dist;

            CO_LOG_MAIN_S_2(BT_STS_CONN_STATUS, KEYD,
                ((key_dist->con_idx) << 24) |
                ((key_dist->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((key_dist->connhdl)),
                key_dist->key_type);

            if (key_dist->key_type == GAP_RECV_PEER_IRK_IDENTITY)
            {
                app_ble_add_devices_info_to_resolving();
            }
            else if (key_dist->key_type == GAP_RECV_DERIVED_BT_LINK_KEY)
            {
                gap_conn_item_t *conn = key_dist->conn;
                btif_device_record_t record = {{{0}},0};

                if (key_dist->link_key_generated_but_still_wait_peer_ia)
                {
                    break;
                }

                record.trusted = true;
                record.pinLen = key_dist->enc_key_size;
                record.bdAddr = key_dist->recv_ia_addr;
                memcpy(record.linkKey, key_dist->recv_key, GAP_KEY_LEN);

                if (conn->sec.bonded_with_num_compare ||
                    conn->sec.bonded_with_passkey_entry ||
                    conn->sec.bonded_with_oob_method)
                {
                    record.keyType = BTIF_AUTH_SC_COMBINATION_KEY;
                }
                else
                {
                    record.keyType = BTIF_UNAUTH_SC_COMBINATION_KEY;
                }

                nv_record_ddbrec_delete(&record.bdAddr);
                nv_record_add(section_usrdata_ddbrecord,(void *)&record);
            }
            break;
        }
        case GAP_CONN_EVENT_UPDATE_REQ:
        {
            gap_conn_update_req_t *update_req = param.update_req;

            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, UPRQ,
                ((update_req->con_idx) << 24) |
                ((update_req->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((update_req->connhdl)),
                (update_req->params_req.conn_interval_min_1_25ms << 16) |
                (update_req->params_req.conn_interval_max_1_25ms),
                (update_req->params_req.max_peripheral_latency << 16) |
                (update_req->params_req.superv_timeout_ms));

            app_ble_conn_param_update_req(update_req);
            break;
        }
        case GAP_CONN_EVENT_PARAMS_UPDATE:
        {
            gap_conn_param_t *update = param.params_update;
            gap_conn_item_t *conn = update->conn;

            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, UPED,
                ((update->con_idx) << 24) |
                ((update->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((update->connhdl)),
                (conn->timing.subrate_factor << 16) |
                (conn->timing.conn_interval_1_25ms),
                (conn->timing.peripheral_latency << 16) |
                (conn->timing.superv_timeout_ms));

            cb_event.evt_type = BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT;
            cb_event.p.conn_param_update_successful_handled.conidx = gap_zero_based_conidx(update->con_idx);
            cb_event.p.conn_param_update_successful_handled.con_interval = conn->timing.conn_interval_1_25ms;
            cb_event.p.conn_param_update_successful_handled.con_latency = conn->timing.peripheral_latency;
            cb_event.p.conn_param_update_successful_handled.sup_to = conn->timing.superv_timeout_ms / 10;
            app_ble_global_handle(&cb_event, NULL);
            break;
        }
        case GAP_CONN_EVENT_SUBRATE_CHANGE:
        {
            gap_conn_param_t *change = param.subrate_change;
            gap_conn_item_t *conn = change->conn;

            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, SBRC,
                ((change->con_idx) << 24) |
                ((change->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((change->connhdl)),
                (conn->timing.subrate_factor << 16) |
                (conn->timing.conn_continuation_number),
                (conn->timing.peripheral_latency << 16) |
                (conn->timing.superv_timeout_ms));

            cb_event.evt_type = BLE_SUBRATE_CHANGE_EVENT;
            cb_event.p.subrate_change_handled.conidx = gap_zero_based_conidx(change->con_idx);
            cb_event.p.subrate_change_handled.sub_factor = conn->timing.subrate_factor;
            cb_event.p.subrate_change_handled.per_latency = conn->timing.peripheral_latency;
            cb_event.p.subrate_change_handled.cont_num = conn->timing.conn_continuation_number;
            cb_event.p.subrate_change_handled.timeout = conn->timing.superv_timeout_ms / 10;
            app_ble_global_handle(&cb_event, NULL);
            break;
        }
        case GAP_CONN_EVENT_PHY_UPDATE:
        {
            gap_conn_phy_update_t *phy_update = param.phy_update;
            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, PHYU,
                ((phy_update->con_idx) << 24) |
                ((phy_update->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((phy_update->connhdl)),
                phy_update->tx_phy,
                phy_update->rx_phy);
            break;
        }
        case GAP_CONN_EVENT_TX_POWER_REPORT:
        {
            gap_le_tx_power_report_t *tx_power = param.tx_power;
            gap_le_tx_power_param_t *param = &tx_power->param;
            CO_LOG_MAIN_S_3(BT_STS_CONN_STATUS, TXPW,
                ((tx_power->con_idx) << 24) |
                ((tx_power->conn->conn_flag.is_central ? 0 : 1) << 16) | /* 0 central, 1 perpherial */
                ((tx_power->connhdl)),
                (param->type << 24) | (param->u.remote.phy << 16) |
                (param->u.remote.is_at_min_level << 12) | (param->u.remote.is_at_max_level << 8) |
                (param->u.remote.delta),
                param->u.remote.curr_tx_power);
            break;
        }
        case GAP_CONN_EVENT_RECV_KEY_METERIAL:
        {
            gap_recv_key_material_t *key = param.recv_key_material;
            if (key->error_code == BT_STS_SUCCESS)
            {
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

#if BLE_GAP_CENTRAL_ROLE
static int app_ble_client_callback(uintptr_t connhdl, gap_init_event_t event, gap_init_callback_param_t param)
{
    if (event < GAP_INIT_EVENT_CONN_OPENED)
    {
        ble_event_t cb_event;

        memset(&cb_event, 0, sizeof(ble_event_t));

        if (event == GAP_INIT_EVENT_SCAN_STARTED)
        {
            CO_LOG_MAIN_S_0(BT_STS_SCAN_STATUS, SCNS);
            cb_event.evt_type = BLE_SCAN_STARTED_EVENT;
            app_ble_global_handle(&cb_event, NULL);
        }
        else if (event == GAP_INIT_EVENT_SCAN_STOPPED)
        {
            CO_LOG_MAIN_S_0(BT_STS_SCAN_STATUS, SCNE);
            cb_event.evt_type = BLE_SCAN_STOPPED_EVENT;
            app_ble_global_handle(&cb_event, NULL);
        }
        else if (event == GAP_INIT_EVENT_SCAN_ADV_REPORT)
        {
            const gap_adv_report_t *adv_report = param.adv_report;
            uint16_t data_len = adv_report->data_length;
            cb_event.evt_type = BLE_SCAN_DATA_REPORT_EVENT;
            cb_event.p.scan_data_report_handled.trans_addr.addr_type = adv_report->peer_type;
            memcpy(cb_event.p.scan_data_report_handled.trans_addr.addr, &adv_report->peer_addr, sizeof(bt_bdaddr_t));
            cb_event.p.scan_data_report_handled.rssi = adv_report->rssi;
            if (data_len > EXT_ADV_DATA_LEN)
            {
                data_len = EXT_ADV_DATA_LEN;
            }
            cb_event.p.scan_data_report_handled.length = data_len;
            cb_event.p.scan_data_report_handled.adv = adv_report;
            cb_event.p.scan_data_report_handled.data = adv_report->data;
            if (adv_report->adv.scan_rsp)
            {
                cb_event.p.scan_data_report_handled.info = adv_report->adv.extended_adv ?
                    GAPM_REPORT_TYPE_SCAN_RSP_EXT : GAPM_REPORT_TYPE_SCAN_RSP_LEG;
            }
            else
            {
                cb_event.p.scan_data_report_handled.info = adv_report->adv.extended_adv ?
                    GAPM_REPORT_TYPE_ADV_EXT : GAPM_REPORT_TYPE_ADV_LEG;
            }
            if (adv_report->adv.cmpl_adv_data)
            {
                cb_event.p.scan_data_report_handled.info |= GAPM_REPORT_INFO_COMPLETE_BIT;
            }
            if (adv_report->adv.connectable)
            {
                cb_event.p.scan_data_report_handled.info |= GAPM_REPORT_INFO_CONN_ADV_BIT;
            }
            if (adv_report->adv.scannable)
            {
                cb_event.p.scan_data_report_handled.info |= GAPM_REPORT_INFO_SCAN_ADV_BIT;
            }
            if (adv_report->adv.directed)
            {
                cb_event.p.scan_data_report_handled.info |= GAPM_REPORT_INFO_DIR_ADV_BIT;
            }
            app_ble_global_handle(&cb_event, NULL);
        }
        else if (event == GAP_INIT_EVENT_INIT_STARTED)
        {
            CO_LOG_MAIN_S_0(BT_STS_INIT_STATUS, INTS);
        }
        else if (event == GAP_INIT_EVENT_INIT_STOPPED)
        {
            CO_LOG_MAIN_S_0(BT_STS_INIT_STATUS, INTE);
            gap_filter_list_remove_user_item(BLE_WHITE_LIST_USER_CENTRAL);
        }
    }
    else
    {
        app_ble_conn_event_handle(connhdl, (gap_conn_event_t)event, (gap_conn_callback_param_t){param.param_ptr});
    }

    return 0;
}
#endif /* BLE_GAP_CENTRAL_ROLE */

static void app_ble_impl_start_connection(const ble_bdaddr_t *peer_addr_or_list, uint32_t ListSize_IaRpaNpa, uint32_t conn_time_ms, bool is_ble_audio)
{
#if BLE_GAP_CENTRAL_ROLE
    uint16_t list_size = (uint16_t)(ListSize_IaRpaNpa >> 16);
    uint8_t ia_rpa_npa = (uint8_t)(ListSize_IaRpaNpa & 0xff);
    bt_addr_type_t own_addr_type = app_ble_get_own_addr_type(ia_rpa_npa);
    const ble_bdaddr_t *peer_addr = peer_addr_or_list;

    if (list_size)
    {
        gap_init_param_t init_param = {own_addr_type, 0};
        gap_filter_list_add_user_item(BLE_WHITE_LIST_USER_CENTRAL, peer_addr, list_size);
        gap_start_auto_initiating(&init_param, app_ble_client_callback);
    }
    else
    {
        if (is_ble_audio)
        {
            gap_connect_ble_audio_device(own_addr_type, (bt_addr_type_t)peer_addr->addr_type,
                (bt_bdaddr_t *)peer_addr->addr, app_ble_client_callback);
        }
        else
        {
            gap_start_direct_initiating(own_addr_type, (bt_addr_type_t)peer_addr->addr_type,
                (bt_bdaddr_t *)peer_addr->addr, app_ble_client_callback);
        }
    }
#endif /*BLE_GAP_CENTRAL_ROLE*/
}

void app_ble_start_auto_connect(const ble_bdaddr_t *addr_list, uint16_t list_size, uint8_t ia_rpa_npa, uint32_t connect_time)
{
    app_ble_impl_start_connection(addr_list, ((list_size)<<16)|((ia_rpa_npa)&0xff), connect_time*10, false);
}

void app_ble_start_connect(const ble_bdaddr_t *peer_addr, uint8_t ia_rpa_npa)
{
    app_ble_impl_start_connection(peer_addr, ia_rpa_npa, 0, false);
}

void app_ble_connect_ble_audio_device(const ble_bdaddr_t *peer_addr, uint8_t ia_rpa_npa)
{
    app_ble_impl_start_connection(peer_addr, ia_rpa_npa, 0, true);
}

void app_ble_cancel_connecting(void)
{
    gap_cancel_initiating();
}

void app_ble_disconnect(uint16_t connhdl)
{
    gap_terminate_connection(connhdl, 0);
}

void app_ble_disconnect_all(void)
{
    gap_terminate_all_ble_connection();
}

void app_ble_start_disconnect(uint8_t conidx)
{
    app_ble_disconnect(gap_zero_based_ble_conidx_as_hdl(conidx));
}

static void app_ble_impl_start_scanning(bool use_filter_list, bool scan_use_rpa, uint16_t window_ms, uint16_t interval_ms)
{
#if BLE_GAP_CENTRAL_ROLE
    bt_addr_type_t own_addr_type = BT_ADDR_TYPE_PUBLIC;
    gap_scan_param_t param = {0};

    if (interval_ms < 50)
    {
        interval_ms = 50;
    }

    if (window_ms > interval_ms / 2)
    {
        window_ms = interval_ms / 2; // max occupy half interval space
    }

    own_addr_type = app_ble_get_own_addr_type(scan_use_rpa ? APP_GAPM_GEN_RSLV_ADDR : APP_GAPM_STATIC_ADDR);

    param.use_filter_list = use_filter_list;
    param.active_scan = true;
    param.filter_duplicated = true;
    param.slow_scan = (window_ms * 3 > interval_ms) ? false : true;

    param.own_addr_type = own_addr_type;

    gap_start_scanning(&param, (gap_scan_callback_t)app_ble_client_callback);
#endif /*BLE_GAP_CENTRAL_ROLE*/
}

void app_ble_start_scan(BLE_SCAN_PARAM_T *param)
{
    uint8_t scan_policy = param->scanFolicyType;
    uint16_t window_ms = param->scanWindowMs;
    uint16_t interval_ms = param->scanIntervalMs;
    app_ble_impl_start_scanning(((scan_policy)&0x1), ((scan_policy) >= 2), window_ms, interval_ms);
}

void app_ble_stop_scan(void)
{
    gap_disable_scanning();
}

static int app_ble_server_callback(uintptr_t connhdl, gap_adv_event_t event, gap_adv_callback_param_t param)
{
    if (event < GAP_ADV_EVENT_CONN_OPENED)
    {
        ble_adv_activity_t *adv = NULL;
        uint8_t adv_handle = (uint8_t)connhdl;
        ble_event_t cb_event;

        memset(&cb_event, 0, sizeof(ble_event_t));

        adv = app_ble_find_adv_activity(adv_handle);
        if (adv == NULL)
        {
            CO_LOG_ERR_2(BT_STS_INVALID_ADV_HANDLE, adv_handle, event);
            return 0;
        }
        if (event == GAP_ADV_EVENT_STARTED)
        {
            CO_LOG_MAIN_S_1(BT_STS_ADV_STATUS, ADVS, adv_handle);
            adv->adv_is_started = true;
            cb_event.evt_type = BLE_ADV_STARTED_EVENT;
            cb_event.p.adv_started_handled.actv_user = adv->user;
            app_ble_global_handle(&cb_event, NULL);
        }
        else if (event == GAP_ADV_EVENT_STOPPED)
        {
            CO_LOG_MAIN_S_1(BT_STS_ADV_STATUS, ADVT, adv_handle);
            adv->adv_is_started = false;
            cb_event.evt_type = BLE_ADV_STOPPED_EVENT;
            cb_event.p.adv_stopped_handled.actv_user = adv->user;
            app_ble_global_handle(&cb_event, NULL);
        }
    }
    else
    {
        if (event == GAP_ADV_EVENT_CONN_OPENED)
        {
            app_stop_fast_connectable_ble_adv_timer();
        }
        else if (event == GAP_ADV_EVENT_CONN_FAILED || event == GAP_ADV_EVENT_CONN_CLOSED)
        {
            CO_LOG_MAIN_S_1(BT_STS_ADV_STATUS, RFSH, event);
            app_ble_refresh_adv();
        }

        app_ble_conn_event_handle(connhdl, (gap_conn_event_t)event, (gap_conn_callback_param_t){param.param_ptr});
    }

    return 0;
}

static void app_ble_refresh_advertising(uint8_t adv_handle, gap_adv_param_t *adv_param)
{
    APP_GAPM_OWN_ADDR_E addr_type = adv_param->own_addr_use_rpa ? APP_GAPM_GEN_RSLV_ADDR : APP_GAPM_STATIC_ADDR;

    adv_param->own_addr_type = app_ble_get_own_addr_type(addr_type);

    if (addr_type == APP_GAPM_GEN_RSLV_ADDR && adv_param->use_custom_local_addr)
    {
        bt_bdaddr_t zero_addr = {{0}};
        if (memcmp(&adv_param->custom_local_addr, &zero_addr, sizeof(bt_bdaddr_t)) != 0)
        {
            adv_param->own_addr_type = BT_ADDR_TYPE_RANDOM;
        }
    }

    gap_refresh_advertising(adv_handle, adv_param, app_ble_server_callback);
}

bool app_ble_is_in_advertising_state(void)
{
    ble_global_t *g = ble_get_global();
    ble_adv_activity_t *adv = NULL;
    int max_size = ARRAY_SIZE(g->adv);
    int i = 0;

    for (i = 0; i < max_size; i += 1)
    {
        adv = g->adv + i;
        if (adv->adv_is_started)
        {
            return true;
        }
    }

    return false;
}

ble_adv_activity_t *app_ble_get_advertising(uint8_t adv_handle)
{
    ble_global_t *g = ble_get_global();
    ble_adv_activity_t *adv = NULL;
    int max_size = ARRAY_SIZE(g->adv);
    int i = 0;

    if (adv_handle == GAP_INVALID_ADV_HANDLE)
    {
        return NULL;
    }

    for (i = 0; i < max_size; i += 1)
    {
        adv = g->adv + i;
        if (adv->adv_handle == adv_handle)
        {
            return adv;
        }
    }

    return NULL;
}

ble_adv_activity_t *app_ble_register_advertising(uint8_t adv_handle, app_ble_adv_activity_func adv_activity_func)
{
    ble_global_t *g = ble_get_global();
    ble_adv_activity_t *adv = NULL;
    ble_adv_activity_t *found = NULL;
    int max_size = ARRAY_SIZE(g->adv);
    int i = 0;

    if (adv_activity_func == NULL)
    {
        CO_LOG_ERR_0(BT_STS_INVALID_PARAMS);
        return NULL;
    }

    if (adv_handle >= BLE_MAX_FIXED_ADV_HANDLE)
    {
        CO_LOG_ERR_1(BT_STS_INVALID_ADV_HANDLE, adv_handle);
        return NULL;
    }

    adv = app_ble_get_advertising(adv_handle);
    if (adv)
    {
        adv->adv_activity_func = adv_activity_func;
        found = adv;
        goto label_found_and_return;
    }

    for (i = 0; i < max_size; i += 1)
    {
        adv = g->adv + i;
        if (adv->adv_handle == GAP_INVALID_ADV_HANDLE)
        {
            adv->adv_activity_func = adv_activity_func;
            adv->adv_handle = adv_handle;
            found = adv;
            break;
        }
    }

label_found_and_return:
    if (found)
    {
        app_ble_clean_adv_param(&adv->adv_param);
        return found;
    }

    return NULL;
}

ble_adv_activity_t *app_ble_get_advertising_by_user(BLE_ADV_USER_E user)
{
    ble_global_t *g = ble_get_global();
    ble_adv_activity_t *adv = NULL;
    int max_size = ARRAY_SIZE(g->adv);
    int i = 0;

    for (i = 0; i < max_size; i += 1)
    {
        adv = g->adv + i;
        if (adv->adv_activity_func && adv->user == user)
        {
            return adv;
        }
    }

    return NULL;
}

void app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_E adv_intv_user,
        BLE_ADV_USER_E adv_user, uint32_t interval)
{
    ble_adv_activity_t *adv = NULL;
    adv = app_ble_get_advertising_by_user(adv_user);
    if (adv)
    {
        adv->custom_adv_interval_ms = interval;
    }
}

static void app_ble_set_adv_interval(ble_adv_activity_t *adv)
{
    if (!adv->adv_param.fast_advertising && adv->custom_adv_interval_ms && adv->custom_adv_interval_ms < 100)
    {
        adv->adv_param.fast_advertising = true;
    }
}

void app_ble_set_adv_tx_power_dbm(ble_adv_activity_t *adv, int8_t tx_power_dbm)
{
    if (tx_power_dbm != -127 && tx_power_dbm != 127)
    {
        adv->adv_param.has_prefer_adv_tx_power = true;
        adv->adv_param.adv_tx_power = tx_power_dbm;
    }
    else
    {
        adv->adv_param.has_prefer_adv_tx_power = false;
        adv->adv_param.adv_tx_power = GAP_UNKNOWN_TX_POWER;
    }
}

void app_ble_set_adv_tx_power_level(ble_adv_activity_t *adv, BLE_ADV_TX_POWER_LEVEL_E tx_power_level)
{
    app_ble_set_adv_tx_power_dbm(adv, btdrv_reg_op_txpwr_idx_to_rssidbm(tx_power_level));
}

void app_ble_set_adv_txpwr_by_adv_user(BLE_ADV_USER_E user, int8_t txpwr_dbm)
{
    ble_adv_activity_t *adv = NULL;

    adv = app_ble_get_advertising_by_user(user);
    if (adv == NULL)
    {
        return;
    }

    app_ble_set_adv_tx_power_dbm(adv, txpwr_dbm);
}

static uint16_t app_ble_parse_out_service_uuid(uint8_t *data, uint16_t len, gap_dt_buf_t *out_uuid_16, gap_dt_buf_t *out_uuid_128)
{
    gap_dt_head_t *curr = (gap_dt_head_t *)data;
    const uint8_t *data_type_ptr = NULL;
    uint16_t removed_len = 0;
    uint16_t left = len;
    uint16_t item_data_len = 0;
    uint16_t data_type_len = 0;
    bool remove_data_type = false;

    if (data == NULL || len <= sizeof(gap_dt_head_t) || len > EXT_ADV_DATA_LEN)
    {
        return 0;
    }

    while (curr->length && left >= curr->length + 1)
    {
        item_data_len = curr->length + 1;
        data_type_len = curr->length - 1;
        data_type_ptr = ((uint8_t *)curr) + 2;
        remove_data_type = false;
        if (curr->ad_type == GAP_DT_SRVC_UUID_16_INCP_LIST || curr->ad_type == GAP_DT_SRVC_UUID_16_CMPL_LIST)
        {
            if (data_type_len && (data_type_len % 2) == 0)
            {
                gap_dt_add_raw_data(out_uuid_16, data_type_ptr, data_type_len);
            }
            else
            {
                CO_LOG_ERR_2(BT_STS_INVALID_ADV_DATA, curr->ad_type, data_type_len);
            }
            remove_data_type = true;
        }
        else if (curr->ad_type == GAP_DT_SRVC_UUID_128_INCP_LIST || curr->ad_type == GAP_DT_SRVC_UUID_128_CMPL_LIST)
        {
            if (data_type_len && (data_type_len % 16) == 0)
            {
                gap_dt_add_raw_data(out_uuid_128, data_type_ptr, data_type_len);
            }
            else
            {
                CO_LOG_ERR_2(BT_STS_INVALID_ADV_DATA, curr->ad_type, data_type_len);
            }
            remove_data_type = true;
        }
        else if (curr->ad_type == GAP_DT_FLAGS || curr->ad_type == GAP_DT_APPEARANCE)
        {
            remove_data_type = true;
        }
        if (remove_data_type)
        {
            removed_len += item_data_len;
            left -= item_data_len;
            if (left)
            {
                memmove(curr, ((uint8_t *)curr)+item_data_len, left);
            }
        }
        else
        {
            left -= item_data_len;
            curr = (gap_dt_head_t *)(((uint8_t *)curr) + item_data_len);
        }
    }

    return len - removed_len;
}

#if defined(IBRT)
bool app_ble_check_ibrt_allow_adv(BLE_ADV_USER_E user)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (!p_ibrt_ctrl->init_done)
    {
        return false;
    }
#if defined(FREEMAN_ENABLED_STERO)
    return true;
#else
    if (app_ibrt_conn_is_freeman_mode())
    {
        return true;
    }
    return (app_ibrt_conn_get_ui_role() != TWS_UI_SLAVE);
#endif
}
#endif

/**
 * old stack adv functions:
 *
 * app_ble_register_data_fill_handle
 * bes_ble_gap_register_data_fill_handle
 * ble_adv_fill_param
 * app_ble_start_adv
 * ble_adv_config_param
 * app_ble_adv_param_is_different
 * ble_adv_refreshing
 * BLE_ADV_RPA_ENABLED
 * app_ai_ble_data_fill_handler
 * app_ama_start_advertising
 * app_dma_start_advertising
 * app_gma_start_advertising
 * app_sv_start_advertising
 * app_tencent_start_advertising
 * voice_start_advertising
 * app_interconnection_ble_data_fill_handler
 * ota_ble_data_fill_handler
 * tile_ble_data_fill_handler
 * app_ble_stub_user_data_fill_handler
 * app_ble_custom_0_user_data_fill_handler
 * app_ble_custom_1_user_data_fill_handler
 * app_ble_custom_2_user_data_fill_handler
 * app_ble_custom_3_user_data_fill_handler
 * app_ble_demo0_user_data_fill_handler
 * app_ble_demo1_user_data_fill_handler
 * gfps_ble_data_fill_handler
 * spot_ble_data_fill_handler
 * swift_ble_data_fill_handler
 * app_ble_audio_user_data_fill_handler
 *
 * ble_start_adv
 * appm_start_advertising
 * gapm_activity_create_cmd_handler
 * gapm_adv_legacy_create/gapm_adv_ext_create/gapm_adv_periodic_create
 * gapm_adv_create
 * gapm_adv_create_transition
 * own_addr_type = gapm_le_actv_get_hci_own_addr_type(p_actv->own_addr_type)
 * gapm_adv_proc_start_transition
 *
 */

bool app_ble_get_user_adv_data(ble_adv_activity_t *adv, BLE_ADV_PARAM_T *param, int user_group)
{
    ble_global_t *g = ble_get_global();
    BLE_ADV_USER_E *consider_user = NULL; // ble_adv_fill_param
    BLE_ADV_USER_E user_group_0[] = {USER_GSOUND, USER_AI, USER_INTERCONNECTION, USER_TILE, USER_OTA};
    uint32_t smallest_adv_interval = 0xFFFFFFFF;
    bool has_custom_adv_interval = false;
    bool has_user_enable_adv = false;
    BLE_ADV_USER_E user;
    int user_count = 0;
    int i = 0;

    if (user_group > BLE_ADV_ACTIVITY_USER_0)
    {
        return false; // currently no other group adv data
    }

    consider_user = user_group_0;
    user_count = ARRAY_SIZE(user_group_0);

    if (!ble_adv_is_allowed())
    {
        return false;
    }

#if defined(IBRT)
    if (!app_ble_check_ibrt_allow_adv(USER_INUSE))
    {
        return false;
    }
#endif

    for (i = 0; i < user_count; i += 1)
    {
        user = consider_user[i];
        if (g->data_fill_func[user])
        {
            g->data_fill_func[user](param);
            if (g->data_fill_enable[user])
            {
                has_user_enable_adv = true;
            }
            // Update to user specific interval
            param->advUserInterval[i] = param->advInterval;
        }
    }

    if (has_user_enable_adv)
    {
        for (i = 0; i < BLE_ADV_USER_NUM; i += 1)
        {
            if (param->advUserInterval[i] && param->advUserInterval[i] < smallest_adv_interval)
            {
                smallest_adv_interval = param->advUserInterval[i];
                has_custom_adv_interval = true;
            }
        }
    }

    if (has_custom_adv_interval)
    {
        adv->custom_adv_interval_ms = smallest_adv_interval;
    }

#if (BLE_APP_HID)
    memcpy(&param->advData[param->advDataLen], APP_HID_ADV_DATA_UUID, APP_HID_ADV_DATA_UUID_LEN);
    param->advDataLen += APP_HID_ADV_DATA_UUID_LEN;
    has_user_enable_adv = true;
#endif

    return has_user_enable_adv;
}

void app_ble_dt_add_adv_data(ble_adv_activity_t *adv, BLE_ADV_PARAM_T *a, const app_ble_adv_data_param_t *b)
{
    gap_dt_buf_t adv_data_uuid_16_list = {0};
    gap_dt_buf_t adv_data_uuid_128_list = {0};
    gap_dt_buf_t scan_rsp_uuid_16_list = {0};
    gap_dt_buf_t scan_rsp_uuid_128_list = {0};
    gap_dt_buf_t *dest_adv_data = &adv->adv_param.adv_data;
    gap_dt_buf_t *dest_scan_rsp = &adv->adv_param.scan_rsp_data;
    gap_dt_buf_t adv_buf_b = {0};
    gap_dt_buf_t scan_buf_b = {0};
    const uint8_t *left_adv_data_a = NULL;
    const uint8_t *left_adv_data_b = NULL;
    const uint8_t *left_scan_rsp_a = NULL;
    const uint8_t *left_scan_rsp_b = NULL;
    uint8_t left_adv_data_len_a = 0;
    uint8_t left_adv_data_len_b = 0;
    uint8_t left_scan_rsp_len_a = 0;
    uint8_t left_scan_rsp_len_b = 0;

    left_adv_data_len_a = app_ble_parse_out_service_uuid(a->advData, a->advDataLen, &adv_data_uuid_16_list, &adv_data_uuid_128_list);
    left_adv_data_a = a->advData;

    left_scan_rsp_len_a = app_ble_parse_out_service_uuid(a->scanRspData, a->scanRspDataLen, &scan_rsp_uuid_16_list, &scan_rsp_uuid_128_list);
    left_scan_rsp_a = a->scanRspData;

    if (b && b->adv_data && b->adv_data_len)
    {
        gap_dt_add_raw_data(&adv_buf_b, b->adv_data, b->adv_data_len);
        left_adv_data_len_b = app_ble_parse_out_service_uuid(ppb_get_data(adv_buf_b.ppb), adv_buf_b.ppb->len, &adv_data_uuid_16_list, &adv_data_uuid_128_list);
        left_adv_data_b = ppb_get_data(adv_buf_b.ppb);
    }

    if (b && b->scan_rsp_data && b->scan_rsp_len)
    {
        gap_dt_add_raw_data(&scan_buf_b, b->scan_rsp_data, b->scan_rsp_len);
        left_scan_rsp_len_b = app_ble_parse_out_service_uuid(ppb_get_data(scan_buf_b.ppb), scan_buf_b.ppb->len, &scan_rsp_uuid_16_list, &scan_rsp_uuid_128_list);
        left_scan_rsp_b = ppb_get_data(scan_buf_b.ppb);
    }

    /**
     * adv data
     *
     */
    if (adv_data_uuid_16_list.ppb && adv_data_uuid_16_list.ppb->len)
    {
        gap_dt_head_t head;
        head.length = adv_data_uuid_16_list.ppb->len + 1;
        head.ad_type = GAP_DT_SRVC_UUID_16_CMPL_LIST;
        gap_dt_add_raw_data(dest_adv_data, (uint8_t *)&head, sizeof(head));
        gap_dt_add_raw_data(dest_adv_data, ppb_get_data(adv_data_uuid_16_list.ppb), adv_data_uuid_16_list.ppb->len);
    }

    if (adv_data_uuid_128_list.ppb && adv_data_uuid_128_list.ppb->len)
    {
        gap_dt_head_t head;
        head.length = adv_data_uuid_128_list.ppb->len + 1;
        head.ad_type = GAP_DT_SRVC_UUID_128_CMPL_LIST;
        gap_dt_add_raw_data(dest_adv_data, (uint8_t *)&head, sizeof(head));
        gap_dt_add_raw_data(dest_adv_data, ppb_get_data(adv_data_uuid_128_list.ppb), adv_data_uuid_128_list.ppb->len);
    }

    if (left_adv_data_a && left_adv_data_len_a)
    {
        gap_dt_add_raw_data(dest_adv_data, left_adv_data_a, left_adv_data_len_a);
    }

    if (left_adv_data_b && left_adv_data_len_b)
    {
        gap_dt_add_raw_data(dest_adv_data, left_adv_data_b, left_adv_data_len_b);
    }

    /**
     * scan rsp data
     *
     */
    if (adv->adv_param.scannable || (adv->adv_param.use_legacy_pdu && adv->adv_param.connectable))
    {
        if (scan_rsp_uuid_16_list.ppb && scan_rsp_uuid_16_list.ppb->len)
        {
            gap_dt_head_t head;
            head.length = scan_rsp_uuid_16_list.ppb->len + 1;
            head.ad_type = GAP_DT_SRVC_UUID_16_CMPL_LIST;
            gap_dt_add_raw_data(dest_scan_rsp, (uint8_t *)&head, sizeof(head));
            gap_dt_add_raw_data(dest_scan_rsp, ppb_get_data(scan_rsp_uuid_16_list.ppb), scan_rsp_uuid_16_list.ppb->len);
        }

        if (scan_rsp_uuid_128_list.ppb && scan_rsp_uuid_128_list.ppb->len)
        {
            gap_dt_head_t head;
            head.length = scan_rsp_uuid_128_list.ppb->len + 1;
            head.ad_type = GAP_DT_SRVC_UUID_128_CMPL_LIST;
            gap_dt_add_raw_data(dest_scan_rsp, (uint8_t *)&head, sizeof(head));
            gap_dt_add_raw_data(dest_scan_rsp, ppb_get_data(scan_rsp_uuid_128_list.ppb), scan_rsp_uuid_128_list.ppb->len);
        }

        if (left_scan_rsp_a && left_scan_rsp_len_a)
        {
            gap_dt_add_raw_data(dest_scan_rsp, left_scan_rsp_a, left_scan_rsp_len_a);
        }

        if (left_scan_rsp_b && left_scan_rsp_len_b)
        {
            gap_dt_add_raw_data(dest_scan_rsp, left_scan_rsp_b, left_scan_rsp_len_b);
        }
    }

    gap_dt_buf_clear(&adv_data_uuid_16_list);
    gap_dt_buf_clear(&adv_data_uuid_128_list);
    gap_dt_buf_clear(&scan_rsp_uuid_16_list);
    gap_dt_buf_clear(&scan_rsp_uuid_128_list);
    gap_dt_buf_clear(&adv_buf_b);
    gap_dt_buf_clear(&scan_buf_b);
}

POSSIBLY_UNUSED static uint8_t app_ble_get_ad_flags(BLE_ADV_PARAM_T *adv_param)
{
    uint8_t ltv_pos = 0;
    // Fetch out ad flags
    if ((adv_param->advData[ltv_pos++] == 2)
            && (adv_param->advData[ltv_pos++] == GAP_DT_FLAGS))
    {
        return adv_param->advData[ltv_pos];
    }

    return 0;
}

bool app_ble_stub_adv_activity_prepare(ble_adv_activity_t *adv)
{
    gap_adv_param_t *adv_param = &adv->adv_param;
    BLE_ADV_PARAM_T legacy_param = {0};
    bool adv_legacy_enable = false;

    if (!ble_adv_is_allowed())
    {
        return false;
    }

#if defined(IBRT)
    if (!app_ble_check_ibrt_allow_adv(USER_STUB))
    {
        return false;
    }
#endif

#ifdef CTKD_ENABLE // ctkd needs ble adv no matter whether a mobile bt link has been established or not
    set_rsp_dist_lk_bit_field_func dist_lk_set_cb = app_sec_reg_dist_lk_bit_get_callback();
    if (dist_lk_set_cb && !dist_lk_set_cb())
    {
        CO_LOG_WAR_0(BT_STS_NOT_ALLOW);
        return false;
    }
#else
    ble_global_t *g = ble_get_global();
    if (!g->ble_stub_adv_enable)
    {
        return false;
    }
#endif

    adv_legacy_enable = app_ble_get_user_adv_data(adv, &legacy_param, BLE_ADV_ACTIVITY_USER_0);
    if (!adv_legacy_enable)
    {
        return false;
    }

    adv->user = USER_STUB;
    adv_param->connectable = true;
    adv_param->scannable = true;
    adv_param->use_legacy_pdu = true;
    adv_param->include_tx_power_data = true;

    app_ble_set_adv_tx_power_level(adv, BLE_ADV_TX_POWER_LEVEL_0);

    app_ble_dt_set_flags(adv_param, false);

    app_ble_dt_add_adv_data(adv, &legacy_param, NULL);

    app_ble_dt_set_local_name(adv_param, NULL);

    return true;
}

#if (BLE_AUDIO_ENABLED)
static bool app_ble_audio_adv_activity_prepare(ble_adv_activity_t *adv)
{
    ble_global_t *g = ble_get_global();
    gap_adv_param_t *adv_param = &adv->adv_param;
    BLE_ADV_PARAM_T extended_param = {0};
    bool adv_extended_enable = false;
    uint16_t appearance = 0;

    if (!ble_adv_is_allowed())
    {
        return false;
    }

    if (g->data_fill_func[USER_BLE_AUDIO])
    {
        g->data_fill_func[USER_BLE_AUDIO](&extended_param);
        if (g->data_fill_enable[USER_BLE_AUDIO])
        {
            adv_extended_enable = true;
        }
    }

    if (!adv_extended_enable)
    {
        return false;
    }

    adv->user = USER_BLE_AUDIO;
    // Use custom specifc adv interval
    adv->custom_adv_interval_ms = extended_param.advInterval;
    /// TODO:Add custom adv interval into timing
    // BLE AUDIO adv specification param
    adv_param->connectable = true;
    adv_param->scannable = true;
    adv_param->use_legacy_pdu = false;
    adv_param->include_tx_power_data = true;

    if ((gap_filter_list_user_item_exist(BLE_WHITE_LIST_USER_MOBILE)
        || gap_filter_list_user_item_exist(BLE_WHITE_LIST_USER_TWS))
        && gap_resolving_list_curr_size() != 0)
    {
        adv_param->policy = GAP_ADV_ACCEPT_ALL_CONN_SCAN_REQS_IN_LIST;
    }
    else
    {
        adv_param->policy = GAP_ADV_ACCEPT_ALL_CONN_SCAN_REQS;
    }

    app_ble_set_adv_tx_power_level(adv, BLE_ADV_TX_POWER_LEVEL_0);

    uint8_t ad_flags = app_ble_get_ad_flags(&extended_param);

    gap_dt_add_data_type(&adv_param->adv_data, GAP_DT_FLAGS, &ad_flags, sizeof(ad_flags));

    appearance = co_host_to_uint16_le(APPEARANCE_VALUE);
    if (appearance)
    {
        gap_dt_add_data_type(&adv_param->adv_data, GAP_DT_APPEARANCE, (uint8_t *)&appearance, sizeof(uint16_t));
    }

    app_ble_dt_add_adv_data(adv, &extended_param, NULL);

    app_ble_dt_set_local_name(adv_param, NULL);

    return true;
}
#endif /* BLE_AUDIO_ENABLED */

void app_ble_enable_advertising(uint8_t adv_handle)
{
    if (bt_defer_curr_func_1(app_ble_enable_advertising, bt_fixed_param(adv_handle)))
    {
        return;
    }

    uint32_t ca = CO_LR_ADDRESS;
    ble_adv_activity_t *adv = NULL;

    if (adv_handle == GAP_INVALID_ADV_HANDLE)
    {
        return;
    }

    adv = app_ble_get_advertising(adv_handle);
    if (adv == NULL)
    {
        return;
    }

    if (ble_adv_is_allowed())
    {
        CO_LOG_MAIN_S_2(BT_STS_START_ADV, SADV, adv_handle, ca);
        app_ble_set_adv_interval(adv);
        app_ble_refresh_advertising(adv_handle, &adv->adv_param);
    }
}

void app_ble_disable_advertising(uint8_t adv_handle)
{
    uint32_t ca = CO_LR_ADDRESS;
    ble_adv_activity_t *adv = NULL;

    if (bt_defer_curr_func_1(app_ble_disable_advertising, bt_fixed_param(adv_handle)))
    {
        return;
    }

    if (adv_handle == GAP_INVALID_ADV_HANDLE)
    {
        return;
    }

    CO_LOG_MAIN_S_2(BT_STS_TERMINATE, TADV, adv_handle, ca);

    gap_disable_advertising(adv_handle, GAP_ADV_DISABLE_BY_APP_BLE);

    adv = app_ble_get_advertising(adv_handle);
    if (adv)
    {
        adv->adv_is_started = false;
    }
}

static void app_ble_impl_refresh_adv(void)
{
    ble_global_t *g = ble_get_global();
    ble_adv_activity_t *adv = NULL;
    int activity_count = ARRAY_SIZE(g->adv);
    bool adv_enable = false;
    int i = 0;

    if (!ble_adv_is_allowed())
    {
        for (i = 0; i < activity_count; i += 1)
        {
            adv = g->adv + i;
            app_ble_disable_advertising(adv->adv_handle);
        }
        return;
    }

    for (i = 0; i < activity_count; i += 1)
    {
        adv = g->adv + i;
        if (adv->adv_activity_func == NULL)
        {
            continue;
        }
        app_ble_clean_adv_param(&adv->adv_param);
        adv_enable = adv->adv_activity_func(adv);
        if (adv_enable)
        {
            CO_LOG_MAIN_S_1(BT_STS_REFRESH_ADV, FAEN, adv->adv_handle);
            app_ble_set_adv_interval(adv);
            app_ble_refresh_advertising(adv->adv_handle, &adv->adv_param);
        }
        else
        {
            CO_LOG_MAIN_S_1(BT_STS_ADV_STATUS, FDIS, adv->adv_handle);
            app_ble_disable_advertising(adv->adv_handle);
        }
    }
}

void app_ble_refresh_adv(void)
{
    CO_LOG_MAIN_S_1(BT_STS_REFRESH_ADV, FADV, CO_LR_ADDRESS);
    bt_defer_call_func_0(app_ble_impl_refresh_adv);
}

void app_ble_start_adv(void)
{
    CO_LOG_MAIN_S_1(BT_STS_START_ADV, FADV, CO_LR_ADDRESS);
    bt_defer_call_func_0(app_ble_impl_refresh_adv);
}

void app_ble_start_connectable_adv(uint16_t advInterval)
{
#ifndef CUSTOMER_DEFINE_ADV_DATA
    CO_LOG_MAIN_S_1(BT_STS_START_CONN_ADV, FADV, CO_LR_ADDRESS);
    bt_defer_call_func_0(app_ble_impl_refresh_adv);
#endif
}

void app_ble_refresh_adv_state(uint16_t advInterval)
{
#ifndef CUSTOMER_DEFINE_ADV_DATA
    CO_LOG_MAIN_S_1(BT_STS_REFRESH_ADV_STATE, FADV, CO_LR_ADDRESS);
    bt_defer_call_func_0(app_ble_impl_refresh_adv);
#endif
}

BLE_ADV_ACTIVITY_USER_E app_ble_param_get_actv_user_from_adv_user(BLE_ADV_USER_E user)
{
    return user;
}

void app_ble_force_switch_adv(enum BLE_ADV_SWITCH_USER_E user, bool enable_adv)
{
    uint32_t ca = CO_LR_ADDRESS;
    ble_global_t *g = ble_get_global();

    if (enable_adv)
    {
        g->adv_force_disabled &= ~(1 << user);
    }
    else
    {
        g->adv_force_disabled |= (1 << user);
    }

    CO_LOG_MAIN_S_3(BT_STS_ADV_STATUS, SWTC, (user<<4)|enable_adv, g->adv_force_disabled, ca);

    app_ble_refresh_adv();
}

void ble_core_enable_stub_adv(void)
{
    ble_global_t *g = ble_get_global();
    CO_LOG_MAIN_S_0(BT_STS_ADV_STATUS, STBE);
    g->ble_stub_adv_enable = true;
    app_ble_refresh_adv();
}

void ble_core_disable_stub_adv(void)
{
    ble_global_t *g = ble_get_global();
    CO_LOG_MAIN_S_0(BT_STS_ADV_STATUS, STBD);
    g->ble_stub_adv_enable = false;
    app_ble_disable_advertising(BLE_BASIC_ADV_HANDLE);
    app_ble_refresh_adv();
}

void app_ble_stub_user_init(void)
{
    // do nothing
}

typedef struct {
    const uint8_t *peer_addr;
    bool connected;
} app_ble_foreach_param_t;

static bool app_ble_check_device_address(gap_conn_item_t *conn, void *priv)
{
    app_ble_foreach_param_t *param = (app_ble_foreach_param_t *)priv;
    if (memcmp(&conn->peer_addr, param->peer_addr, sizeof(bt_bdaddr_t)) == 0)
    {
        param->connected = false;
        return gap_end_loop;
    }
    return gap_continue_loop;
}

bool app_ble_is_connection_on_by_addr(uint8_t *addr)
{
    app_ble_foreach_param_t param = {addr, false};
    gap_conn_foreach(app_ble_check_device_address, &param);
    return param.connected;
}

int8_t app_ble_get_rssi(uint8_t con_idx)
{
    int8_t rssi = 127;
    rx_agc_t tws_agc = {0};
    uint8_t conidx = gap_zero_based_conidx(con_idx);

    if (con_idx != GAP_INVALID_CONIDX)
    {
        rssi = bt_drv_reg_op_read_ble_rssi_in_dbm(conidx, &tws_agc);
        rssi = bt_drv_reg_op_rssi_correction(tws_agc.rssi);
    }

    return rssi;
}

void app_ble_set_tx_rx_pref_phy(uint32_t tx_pref_phy, uint32_t rx_pref_phy)
{
    ble_global_t *g = ble_get_global();
    g->default_rx_pref_phy_bits = (uint8_t)tx_pref_phy;
    g->default_tx_pref_phy_bits = (uint8_t)rx_pref_phy;
}

uint8_t *app_ble_get_dev_name(void)
{
    return (uint8_t *)gap_local_le_name(NULL);
}

static const uint8_t *app_ble_get_nv_local_irk(void)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    return dev_info->self_info.ble_irk;
}

static const uint8_t *app_ble_get_nv_local_csrk(void)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    return dev_info->self_info.ble_csrk;
}

static uint32_t app_ble_get_nv_ble_device_count(void)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    return dev_info->saved_list_num;
}

static const uint8_t *app_ble_get_nv_local_database_hash(void)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    return dev_info->local_database_hash;
}

static void app_ble_parse_record_to_bond_device(const BleDeviceinfo *record, gap_bond_sec_t *out)
{
    const BleDevicePairingInfo *pair_info = &record->pairingInfo;
    uint8_t peer_ediv_le[2] = {CO_SPLIT_UINT16_LE(pair_info->EDIV)};
    uint8_t local_ediv_le[2] = {CO_SPLIT_UINT16_LE(pair_info->LOCAL_EDIV)};
    out->device_paired = 1;
    out->device_bonded = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_STATUS_POS)) ? 1 : 0;
    out->secure_pairing = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_SECURE_CONNECTION)) ? 1 : 0;
    out->bonded_with_num_compare = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_WITH_NUM_COMPARE)) ? 1 : 0;
    out->bonded_with_passkey_entry = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_WITH_PASSKEY_ENTRY)) ? 1 : 0;
    out->bonded_with_oob_method = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_WITH_OOB_METHOD)) ? 1 : 0;
    out->peer_irk_distributed = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_WITH_IRK_DISTRIBUTED)) ? 1 : 0;
    out->peer_csrk_distributed = (pair_info->bond_info_bf & CO_BIT_MASK(BONDED_WITH_CSRK_DISTRIBUTED)) ? 1 : 0;
    out->peer_type = (bt_addr_type_t)(pair_info->peer_addr.addr_type & 0x01);
    out->peer_addr = *((bt_bdaddr_t *)pair_info->peer_addr.addr);
    out->enc_key_size = pair_info->enc_key_size;
    out->server_cache = pair_info->server_cache;
    memcpy(out->ltk, pair_info->LTK, GAP_KEY_LEN);
    memcpy(out->peer_irk, pair_info->IRK, GAP_KEY_LEN);
    memcpy(out->peer_csrk, pair_info->CSRK, GAP_KEY_LEN);
    memcpy(out->local_ltk, pair_info->LOCAL_LTK, GAP_KEY_LEN);
    memcpy(out->rand, pair_info->RANDOM, sizeof(out->rand));
    memcpy(out->local_rand, pair_info->LOCAL_RANDOM, sizeof(out->local_rand));
    memcpy(out->ediv, peer_ediv_le, 2);
    memcpy(out->local_ediv, local_ediv_le, 2);
    out->client_cache = NULL;
}

static bool app_ble_get_nv_ble_device_by_index(uint32_t i, gap_bond_sec_t *out)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    BleDeviceinfo *record = NULL;
    uint32_t count = dev_info->saved_list_num;
    if (count && count <= BLE_RECORD_NUM && i < count)
    {
        record = dev_info->ble_nv + i;
        app_ble_parse_record_to_bond_device(record, out);
    }
    return record != NULL;
}

static bool app_ble_get_nv_ble_device_by_addr(bt_addr_type_t peer_type, const bt_bdaddr_t *peer_addr, gap_bond_sec_t *out)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    BleDeviceinfo *curr = NULL;
    BleDeviceinfo *record = NULL;
    BleDevicePairingInfo *pair_info = NULL;
    uint32_t count = dev_info->saved_list_num;
    uint32_t i = 0;
    if (count && count <= BLE_RECORD_NUM)
    {
        for (; i < count; i += 1)
        {
            curr = dev_info->ble_nv + i;
            pair_info = &(curr->pairingInfo);
            if (pair_info->peer_addr.addr_type == (peer_type & 0x01) &&
                memcmp(pair_info->peer_addr.addr, peer_addr->address, sizeof(bt_bdaddr_t)) == 0)
            {
                record = curr;
                if (out)
                {
                    app_ble_parse_record_to_bond_device(record, out);
                }
                break;
            }
        }
    }
    return record != NULL;
}

static gatt_client_cache_t *app_ble_has_client_cache(gap_bond_sec_t *bond)
{
#ifdef BLE_GATT_CLIENT_CACHE
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *dev_info = nv_record_blerec_get_ptr();
    gatt_client_cache_t *curr = NULL;
    for (int i = 0; i < BLE_GATT_CACHE_NUM; i += 1)
    {
        curr = dev_info->client_cache + i;
        if (curr->client_cache_seqn && curr->peer_addr.addr_type == (bond->peer_type & 0x01) &&
            memcmp(curr->peer_addr.addr, bond->peer_addr.address, sizeof(bt_bdaddr_t)) == 0)
        {
            return curr;
        }
    }
#endif
    return NULL;
}

static void app_ble_get_local_smp_requirements(const gap_conn_item_t *conn, smp_requirements_t *p_requirements)
{
    // Only adjust LE SMP requirements
    if (conn->conn_type != HCI_CONN_TYPE_LE_ACL)
    {
        return;
    }

#ifdef GFPS_ENABLED_X
    // Only in SMP paring start with peer request, we can set gfps flag
    if (conn->smp_conn != NULL && gfps_get_flag())
    {
        p_requirements->auth_req |= SMP_AUTH_MITM_PROTECT;
        p_requirements->io_cap = SMP_IO_DISPLAY_YES_NO;
        gfps_set_flag(false);
    }
    else
#endif
    {
        p_requirements->auth_req &= ~SMP_AUTH_MITM_PROTECT;
        p_requirements->io_cap = SMP_IO_NO_INPUT_NO_OUTPUT;
    }

#if defined(CTKD_ENABLE)
    ble_global_t *g = ble_get_global();

    uint8_t key_dist = 0;

    if (g->dist_lk_set_cb != NULL)
    {
        key_dist |= g->dist_lk_set_cb();
    }

    if (key_dist & GAP_KDIST_LINKKEY)
    {
        p_requirements->init_key_dist |= GAP_KDIST_LINKKEY;
        p_requirements->resp_key_dist |= GAP_KDIST_LINKKEY;
    }
    else
    {
        p_requirements->init_key_dist &= ~GAP_KDIST_LINKKEY;
        p_requirements->resp_key_dist &= ~GAP_KDIST_LINKKEY;
    }
#endif
}

void app_ble_add_nv_ble_device(const gap_bond_sec_t *bond, bool clear_client_cache)
{
    BleDevicePairingInfo record;

    memset(&record, 0, sizeof(record));

    record.peer_addr.addr_type = (bond->peer_type & 0x01);
    record.EDIV = CO_COMBINE_UINT16_LE(bond->ediv);
    record.LOCAL_EDIV = CO_COMBINE_UINT16_LE(bond->local_ediv);
    record.enc_key_size = bond->enc_key_size;
    record.bond_info_bf |= bond->device_bonded ? CO_BIT_MASK(BONDED_STATUS_POS) : 0;
    record.bond_info_bf |= bond->peer_irk_distributed ? CO_BIT_MASK(BONDED_WITH_IRK_DISTRIBUTED) : 0;
    record.bond_info_bf |= bond->peer_csrk_distributed ? CO_BIT_MASK(BONDED_WITH_CSRK_DISTRIBUTED) : 0;
    record.bond_info_bf |= bond->secure_pairing ? CO_BIT_MASK(BONDED_SECURE_CONNECTION) : 0;
    record.bond_info_bf |= bond->bonded_with_num_compare ? CO_BIT_MASK(BONDED_WITH_NUM_COMPARE) : 0;
    record.bond_info_bf |= bond->bonded_with_passkey_entry ? CO_BIT_MASK(BONDED_WITH_PASSKEY_ENTRY) : 0;
    record.server_cache = bond->server_cache;
    memcpy(record.peer_addr.addr, bond->peer_addr.address, sizeof(bt_bdaddr_t));
    memcpy(record.IRK, bond->peer_irk, GAP_KEY_LEN);
    memcpy(record.CSRK, bond->peer_csrk, GAP_KEY_LEN);
    memcpy(record.LTK, bond->ltk, GAP_KEY_LEN);
    memcpy(record.LOCAL_LTK, bond->local_ltk, GAP_KEY_LEN);
    memcpy(record.RANDOM, bond->rand, sizeof(bond->rand));
    memcpy(record.LOCAL_RANDOM, bond->local_rand, sizeof(bond->local_rand));
    if (clear_client_cache)
    {
        nv_record_blerec_ext_add(&record, NULL, true);
    }
    else
    {
        nv_record_blerec_ext_add(&record, bond->client_cache, false);
    }
}

bool app_ble_get_nv_bt_device_by_addr(const bt_bdaddr_t *bd_addr, gap_bredr_sec_t *out)
{
    nvrec_btdevicerecord *record = NULL;
    if (!nv_record_btdevicerecord_find(bd_addr, &record))
    {
        out->peer_addr = record->record.bdAddr;
        out->trusted = record->record.trusted;
        out->for_bt_source = record->record.for_bt_source;
        out->key_type = record->record.keyType;
        memcpy(out->link_key, record->record.linkKey, GAP_KEY_LEN);
        memcpy(out->cod, record->record.cod, sizeof(record->record.cod));
        return true;
    }
    return false;
}

#ifdef CFG_SEC_CON
uint8_t ble_public_key[64];
uint8_t ble_private_key[32];
const uint8_t bes_demo_Public_key[64] = { //MSB->LSB
    0x3E,0x08,0x3B,0x0A,0x5C,0x04,0x78,0x84,0xBE,0x41,
    0xBE,0x7E,0x52,0xD1,0x0C,0x68,0x64,0x6C,0x4D,0xB6,
    0xD9,0x20,0x95,0xA7,0x32,0xE9,0x42,0x40,0xAC,0x02,
    0x54,0x48,0x99,0x49,0xDA,0xE1,0x0D,0x9C,0xF5,0xEB,
    0x29,0x35,0x7F,0xB1,0x70,0x55,0xCB,0x8C,0x8F,0xBF,
    0xEB,0x17,0x15,0x3F,0xA0,0xAA,0xA5,0xA2,0xC4,0x3C,
    0x1B,0x48,0x60,0xDA
};
const uint8_t bes_demo_private_key[32]= { //MSB->LSB
     0xCD,0xF8,0xAA,0xC0,0xDF,0x4C,0x93,0x63,0x2F,0x48,
     0x20,0xA6,0xD8,0xAB,0x22,0xF3,0x3A,0x94,0xBF,0x8E,
     0x4C,0x90,0x25,0xB3,0x44,0xD2,0x2E,0xDE,0x0F,0xB7,
     0x22,0x1F
};
#endif

static void app_ble_set_local_sec_levels(void)
{
    POSSIBLY_UNUSED gap_local_info_t *local = gap_local_info();

#if BLE_AUDIO_ENABLED
    /**
     * The security requirements for all characteristics defined in ASCS/PACS/BASS shall be
     *      Security Mode 1 Level 2, Unauthenticated pairing with encryption
     *
     * Access to all characterisitcs defined in ASCS, PACS, and BASS shall require an encryption
     * key with at least 128 bits of entropy, derived from any of the following:
     *      LE Secure Conections
     *      BR/EDR Secure Connections; if CTKD is used
     *      Out-of-band (OOB) method
     */
    local->sec_levels.link_sec_level = GAP_SEC_UNAUTHENTICATED;
#endif
}

const gap_ext_func_cbs_t gap_external_func_cbs =
{
    .nv_get_local_irk = app_ble_get_nv_local_irk,
    .nv_get_local_csrk = app_ble_get_nv_local_csrk,
    .nv_get_local_db_hash =  app_ble_get_nv_local_database_hash,
    .nv_get_ble_device_cnt = app_ble_get_nv_ble_device_count,
    .nv_get_ble_device_by_index = app_ble_get_nv_ble_device_by_index,
    .nv_get_ble_device_by_addr = app_ble_get_nv_ble_device_by_addr,
    .nv_add_ble_device = app_ble_add_nv_ble_device,
    .nv_del_ble_device = nv_record_ble_delete_entry,
    .nv_has_client_cache = app_ble_has_client_cache,
    .nv_set_ble_dev_db_hash = nv_record_blerec_update_database_hash,
    .nv_set_ble_dev_change_unaware = nv_record_blerec_set_change_unaware,
    .smp_get_upper_requirements = app_ble_get_local_smp_requirements,
    .nv_get_bt_device_by_addr = app_ble_get_nv_bt_device_by_addr,
};

const gap_ext_func_cbs_t *app_ble_get_gap_ext_callbacks(void)
{
    return &gap_external_func_cbs;
}

/**
 * GAP Service
 * ===========
 *
 * The GATT Server shall contain the GAP Service. A device shall have only one instance
 * of the GAP Service in the GATT Server. The GAP Service is a GATT based service with
 * the service UUID as <<GAP Service>>.
 *
 * GAP Service shall be Mandatory support by LE Peripheral, LE Central, and BR/EDR/LE type
 * devices. It is optional for non BR/EDR/LE type devices. The suport for LE Broadcaster and
 * LE Observer devices is exclueded.
 *
 * The charactersitics requirements for the GAP Service in each of the GAP roles:
 *      Characteristics                         BR/EDR GAP Role     LE Peripheral       LE Central
 *      1) Device Name                          C1                  M                   M
 *      2) Appearance                           C1                  M                   M
 *      3) Peripheral Prefer Conn Prams         O                   O                   E
 *      4) Central Address Resolution           O                   C3                  C2
 *      5) Resolvable Private Address Only      O                   C3                  C2
 *      6) Encrypted Data Key Material          O                   O                   E
 *      7) LE GATT Security Levels              E                   O                   O
 *  C1 - Mandatory for BR/EDR/LE type devices, otherwise O
 *  C2 - Mandatory if LL privacy is supported, otherwise E
 *  C3 - Opitonal if LL privacy is supported, otherwise E
 *
 * A device that supports multiple GAP roles contains all the characteristics to meet the requirements
 * for the supported roles. The device must continue to expose the characteristics when the device
 * is operating in the role in which the characteristics are not valid.
 *
 * ## Device Name Characteristic (M)
 *
 * The Device Name shall contain the name of the device as an UTF-8 string. When the device is discoverable,
 * the Device Name shall be readable without authentication or authorization. When the device is not
 * discoverable, the Device Name should not be readable. The Device Name value may be writable. If writable,
 * authentication and authorization may be defined by a higher layer specification or be implementation
 * specific.
 *
 * The Device Name value shall be 0 to 248 octets in length. A device shall have only one instance of the
 * Device Name Charactersitic.
 *
 * Device Name Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value     Attribute Permission
 * 0xMMMM               0x2A00              Device Name         Readable W/O authen or author when discoverable
 *                                                              Optionally writable, authen and author may be defined
 */

GATT_DECL_CHAR(g_gap_service_device_name,
    GATT_CHAR_UUID_DEVICE_NAME,
    GATT_RD_REQ|GATT_WR_REQ, // Readable only when discoverable, Optional writable
    ATT_SEC_NONE);

/**
 * ## Appearance Characteristic (M)
 *
 * The Apperance defines the representation of the external appearance of the device. This
 * enable the discovering device to represent the device to the user using an icon, or a
 * string, or similar. The Appearance value shall be readable without authentication or
 * authorization. The value may be writable, if writable, authen and author may be defined
 * by a higher layer specification or be implementation specific. A device shall have only
 * one instance of the Appearance characteristic.
 *
 * Appearance Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value                 Attribute Permission
 * 0xMMMM               0x2A01              appearance enum value (2-byte)  Readable W/O authen or author
 *                                                                          Optionally writable, authen and author may be defined
 */

GATT_DECL_CHAR(g_gap_service_appearance,
    GATT_CHAR_UUID_APPEARANCE,
    GATT_RD_REQ|GATT_WR_REQ,
    ATT_SEC_NONE);

/**
 * ## Central Address Resolution Chacteristic (M)
 *
 * The Peripheral should check if the peer device supports address resolution by reading
 * this characteristic before using directed advertising where the target address is set
 * to RPA. This characterisitc defines whether the device supports privacy with address
 * resolution.
 *
 * The value shall be 1 octet in length. A device shall have only one instance of the
 * characteristic. If the characteristic is not present, then it shall be assumed that
 * the Central Address Resolution is not supported.
 *
 * Central Address Resolution Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value     Attribute Permission
 * 0xMMMM               0x2AA6              Support (1-byte)    Read Only W/O authen and author
 *
 */

GATT_DECL_CHAR(g_gap_service_central_addr_reso_supp,
    GATT_CHAR_UUID_CENTRAL_ADDRESS_RESOLUTION,
    GATT_RD_REQ,
    ATT_SEC_NONE);

/**
 * ## Resolvable Private Address Only Characteristic (O)
 *
 * The device should check if the peer will only use RPA after bonding by reading this
 * characteristic, in order to determine if it will satisfy its privacy mode as defined
 * in Section GAP 10.7.
 *
 * This characteristic defines whether the device will only use RPAs as local addresses.
 * The value shall be 1 octet in length: 0x00 only RPAs will be used as local addresses
 * after bonding; 0x01 ~ 0xFF reserved for future use.
 *
 * A device shall have only one instance of this characteristic. If the characteristic
 * is not present, then it cannot be assumed that only RPAs will be used over the air.
 *
 * Resolvable Private Address Only Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value         Attribute Permission
 * 0xMMMM               0x2AC9              RPA Only (1-byte)       Read Only W/O authen and author
 *
 */

GATT_DECL_OPTIONAL_CHAR_WITH_CONST_VALUE(g_gap_service_use_only_rpa_after_bonding,
    GATT_CHAR_UUID_RPA_ONLY,
    GATT_RD_REQ,
    ATT_SEC_NONE,
    0x00 /* 0 - only RPAs will be used as local addresses after bonding */ );

/**
 * ## Peripheral Preferred Connection Parameters (PPCP) Chacteristic (O)
 *
 * The PPCP characteristic contains the preferred connection parameters of the Peripheral.
 * The PPCP value shall be readable, authen or author may be defined by a higher layer
 * specification or be implementation specific. The PPCP value shall not be writable.
 *
 * The PPCP value shall be 8 octets in length. A device shall have oly one instance of the
 * PPCP characteristic. Each field of the value shall have the same meaning and requirements
 * as the field of LL_CONNECTION_PARAM_REQ PDU with the same name, or shall contain the
 * value 0xFFFF which indicates that no specific value is requested.
 *
 * PPCP Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xMMMM               0x2A04              Parameters (8-byte)         Read Only, authen or author may be defined
 *
 */

GATT_DECL_OPTIONAL_CHAR(g_gap_service_periph_prefer_conn_params,
    GATT_CHAR_UUID_PERIPH_PREFER_CONN_PARAMS,
    GATT_RD_REQ,
    ATT_SEC_NONE);

/**
 * ## LE GATT Security Levels Characterisitc (O)
 *
 * This Char shall contian the highest security requirements of the GATT server when
 * operating on a LE connection. The value of the LE GATT Security Levels Char shall be
 * static during a connection. A device shall have at most one instance of a LE GATT
 * Security Levels Char.
 *
 * The Attribute Value is a sequence of Security Level Requirements, each with the type uint8[2].
 * Each security Level Requirement consits of a Security Mode field followed by a Secuirty Level
 * field. The Security Mode and Security Level shall be expressed as the same number as used in
 * ther definitions; e.g., mode 1 is represented as 0x01 and level 4 is represended as 0x04.
 *
 * Meeting any one of the security requirements provided for a given mode shall be sufficient for
 * the GATT server to allow the GATT client to use all the GATT procedures permitted by the
 * characteristic properties for all characteristics on the server operating in that mode.
 *
 * If any one of the security requirements specified in the LE GATT Security Levels is met, no GATT
 * procedure will fail for a secuirity-related reason. For example, the attribute value 0x01 0x04
 * for the LE GATT Security Levels characteristic means that the GATT server requires level 4
 * when operating in security mode 1 on a LE connection.
 *
 * LE GATT Security Levels Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value         Attribute Permission
 * 0xMMMM               0x2BF5              one or more sec levels  Read Only No enc No Authen No Author
 *
 * LE security mode 1 levels:
 *      Level 1 - No security (no authentication and no encryption)
 *      Level 2 - Unauthenticated pairing with encryption
 *      Level 3 - Authenticated pairing with encryption
 *      Level 4 - Authenticated LE Secure Connections pairing with encryption using a 128-bit enc key
 * LE security mode 2 levels:
 *      shall not be used when a connection is operating in mode 1 level 2/3/4
 *      Level 1 - Unauthenticated pairing with data signing
 *      Level 2 - Authenticated pairing with data signing
 * LE security mode 3 levels:
 *      shall be used to broadcast a BIG in an ISO Broadcaster or receive a BIS in a Synced Receiver
 *      Level 1 - No security (no authentication and no encryption)
 *      Level 2 - Use of unauthenticated Broadcast_Code
 *      Level 3 - Use of authenticated Broadcast_Code
 *
 */

typedef struct {
    uint8_t mode;
    uint8_t level;
} gatt_security_level_t;

GATT_DECL_OPTIONAL_CHAR(g_gap_service_le_gatt_security_levels,
    GATT_CHAR_UUID_LE_GATT_SECURITY_LEVELS,
    GATT_RD_REQ,
    ATT_SEC_NONE);

/**
 * ## Encrypted Data Key Material (O)
 *
 * The Encrypted Data Key Material chanaracterisc allows advertising data associated with
 * the GAP service to be decrypted and authenticated using the key material. The character
 * shall not be writtable. It shall only be readable when authenticated and authorized.
 * When read, the key material is stored on the local device. The key material may be
 * discarded at any time on a local device.
 *
 * The Encrypted Data Key Material characteristic may support indications. When authenticated
 * and authorized, if the Encrypted Data Key characteristic value changes and the client
 * has configured the characteristic for indications, thne the characteristic shall be
 * indicated by the server to the client.
 *
 * This characteristic can be used by other services to allow those services to expose
 * separate key material for encrypted advertising data used by those services.
 *
 * Encrypted Data Key Material Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value         Attribute Permission
 * 0xMMMM               0x2B88              Key Material            Read/Indicate when auth and author, Not writable
 *
 * The Key Material is composed of a 128-bit value that is used as the session key, and
 * a 64-bit vlaue that is used as the IV for encrypting and authenticating the Encrypted
 * Data. The server should update the Key Material periodically.
 *
 */

GATT_DECL_OPTIONAL_CHAR_WITH_FLAG(g_gap_service_enc_data_key_material,
    GATT_CHAR_UUID_ENCRYPTED_DATA_KEY_MATERIAL,
    GATT_RD_REQ|GATT_IND_PROP,
    ATT_RD_MITM_AUTH|ATT_RD_AUTHOR,
    ATT_FLAG_IND_AUTH);

// GAP Service Declaration

GATT_DECL_PRI_SERVICE(g_gap_service, GATT_UUID_GAP_SERVICE);

static const gatt_attribute_t g_gap_service_attr_list[] = {
    /* Service */
    gatt_attribute(g_gap_service),
    /* Characteristics */
    gatt_attribute(g_gap_service_device_name),
    gatt_attribute(g_gap_service_appearance),
    gatt_attribute(g_gap_service_central_addr_reso_supp),
    gatt_attribute(g_gap_service_le_gatt_security_levels),
#if (BLE_USE_ENC_DATA_KEY_MATERIAL_CHAR == 1)
    gatt_attribute(g_gap_service_enc_data_key_material),
#endif
#if (BLE_LOCAL_USE_RPA_ONLY_AFTER_BONDING == 1)
    gatt_attribute(g_gap_service_use_only_rpa_after_bonding),
#endif
    gatt_attribute(g_gap_service_periph_prefer_conn_params), // keep this as last characteristic
};

/**
 * Gatt Service
 * ============
 *
 * Only one instance of the Gatt Service shall be exposed on a Gatt Server. Below are
 * the characteristics may be present in the server and may be supported be the client.
 *      1) Service Changed
 *         - M if server can add/change/remove service, otherwise O
 *         - M for client
 *      2) Client Supported Features
 *         - M if server support Database Hash and Service Changed or
 *             if server support EATT or Multiple Variable Length Notification, otherwise excluded
 *         - O for client
 *      3) Database Hash
 *         - O for server
 *         - O for client
 *      4) Server Supported Features
 *         - M if server support any of the features in <server supported fetures>, otherwise O
 *         - O for client
 *
 * ## Service Changed Characteristic (M)
 *
 * The Service Changed characteristic is a control-point attribute that shall be used to indicate
 * to connected device that services have changed. It shall be used to indicate to clients that
 * have a trusted relationship (i.e. bond) with the server when GATT based services have changed
 * when they re-connect to the server.
 *
 * This Char Value shall be configured to be indicated using the CCCD by a client. Indications
 * caused by changes to this Char Value shall be considered lost if the client has erroneously
 * not enabled indications in the CCCD.
 *
 * The Char Value is two 16-bit Attribute Handles concatenated together indicating the beginning
 * and ending Attribute Handles affected by an addition, removeal, or modification to a service
 * on the server. A change to a characteristic value is not considered a modification of the
 * service. If a change has been made to any of the <<Gatt Service>> characteristic values other than
 * the Service Changed Char Value and the Client Supported Features Char Value, i.e., the Database Hash
 * Char Value or Server Supported Features Char Value is changed, the range shall also include the
 * beginning and ending Attribute Handle for the <<Gatt Service>>.
 *
 * A GATT based service is considered modified if the binding of the Attribute Handles to the
 * associated Attributes grouped within a server definition are changed. Any change to the <<Gatt
 * Service>> definition characteristic values other than the Service Changed Char Value and
 * the Client Supported Features Char Value themselves shall also be considered a modification.
 *
 * The Service Changed characteristic Attribute Handle on the server shall not change if the
 * server has a trusted relationship with any client.
 *
 * The Attribute information that shall be cached by a client is the Attribute Handles of all server
 * attributes and the <<GATT Service>> characteristics values.
 *
 * There shall be only one instance of the Service Changed within the Gatt Service definition.
 * A Service Changed Char Value shall exist for each client with a trusted relationship.
 *
 * If GATT based services on the server cannot be changed during the usable lifetime of the device,
 * the Service Changed characteristic shall not exist on the server and the client does not need
 * to need to ever perform service discovery after the initial service discovery for that server.
 *
 * If the Service Changed Char exists on the server, the Characteristic Value Indication support
 * on the server is mandatory. The client shall support Characteristic Value Indication of the
 * Service Changed Char.
 *
 * Service Changed Characteristic Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xNNNN               0x2803              Char Prop = 0x20            Read Only
 *                                          0xMMMM Char Value Handle
 *                                          0x2A05 - <Service Changed>
 * Service Changed Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xMMMM               0x2A05              0xSSSS Start Affected Range Not Readable, Not Writable
 *                                          0xTTTT End Affected Range
 */

GATT_DECL_CHAR(g_gatt_service_changed,
    GATT_CHAR_UUID_SERVICE_CHANGED,
    GATT_IND_PROP,
    ATT_SEC_NONE);

GATT_DECL_CCCD_DESCRIPTOR(g_gatt_service_changed_cccd,
    ATT_SEC_NONE);

typedef struct {
    uint16_t start_affected_handle;
    uint16_t end_affected_handle;
} gatt_service_changed_t;

/**
 * ## Client Supported Features Characteristic (M)
 *
 * The Client Support Features characteristic is used by the client to inform the server which
 * features are supported by the client. If the char exists on the server, the client may update
 * the Client Support Features bit field. If a client feature bit is set by a client and the
 * server supports that feature, the server shall fulfill all requirements associated with
 * this feature when communicating with this client. If a client feature bit is not set by the
 * client, then the server shall not use any of the features associated with that bit when
 * communicating with this client.
 *
 * Client Support Features Characteristic Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xNNNN               0x2803              Char Prop = 0x0A            Read Only
 *                                          0xMMMM Char Value Handle
 *                                          0x2B29
 * Client Support Features Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xMMMM               0x2B29              0xXX...XX -                 Readable, Writable
 *                                          Variable Length Client Features
 *
 * The Client Supported Features Char Value is an array of octets, each of which is a bit field.
 * Below is the allocation of these bits, all bits not listed are reserved for future use. The
 * array should not have any trailing zero octets. If any octet does not appear in the attrubte
 * value because it is too short, the server shall behave as if that octet were present with a
 * value of zero. The default value shall be all bits set to zero.
 *
 * There shall be only one instance of the Client Supported Features characteristic within the
 * Gatt Service definition. A Client Supported Features Char Value shall exist for each connected
 * client. For clients with a trusted relationship, the Char Value shall be persistent across
 * connections. For clients without a trusted relationship the Char Value shall be set to the
 * default value at each connection.
 *
 * The Attribute Handle of the Client Supported Features characteristic on the server shall
 * not change during a connection or if the server has a trusted relationship with any client.
 *
 * A client shall not clear any bits it has set. The server shall respond to any such request
 * with the Error Code of Value Not Allowed (0x13).
 *
 */

GATT_DECL_CHAR(g_gatt_client_supp_features,
    GATT_CHAR_UUID_CLIENT_SUPP_FEATURES,
    GATT_RD_REQ|GATT_WR_REQ,
    ATT_SEC_NONE);

/**
 * ## Server Supported Features Characteristic (M)
 *
 * The Server Supported features characteristic is a read-only characteristic that shall be used
 * to indicate support for server features. The server shall set a bit only if the corresponding
 * feature is supported. There shall be only one instance of the Server Supported Features
 * characteristic within the Gatt Service definition.
 *
 * Server Support Features Characteristic Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xNNNN               0x2803              Char Prop = 0x02            Read Only
 *                                          0xMMMM Char Value Handle
 *                                          0x2B3A
 * Server Support Features Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xMMMM               0x2B3A              0xUU - Features             Read Only
 *
 * The Server Supported Features Char Value is an array of octets, each of which is a bit field.
 * Below is the allocation of these bits, all bits not listed are reserved for future use. The
 * array should not have any trailing zero octets.
 *
 * If any octet does not appear in the attribute value because it is too short, the client shall
 * behave as if that octet were presnet with the value of zero.
 *
 */

GATT_DECL_CHAR(g_gatt_server_supp_features,
    GATT_CHAR_UUID_SERVER_SUPP_FEATURES,
    GATT_RD_REQ,
    ATT_SEC_NONE);

/**
 * Database Hash Characteristic (O)
 *
 * The Database Hash characteristic contains the result of a hash function applied to the service
 * definitions in the GATT database. The client may read the characteristic at any time to
 * determine if services have been added, removed, or modified. If any of the input fields to the
 * hash function have changed, the server shall calculate a new Database Hash and update the
 * characteristic value. The Database Hash characteristic is a read-only attribute.
 *
 * There is only one instance of the Database Hash characteristic within the Gatt Service
 * definition. The same Database Hash value is used for all clients, whether a trusted
 * relationship exists or not.
 *
 * In order to read the value of this characterisitc the client shall always use the
 * Gatt Read Using Char UUID. The Starting Handle should be set to 0x0001 and the Ending
 * Handle should be set to 0xFFFF. If a client reads the value of this characteristic
 * while the server is re-calculating the hash following a change to the database, the
 * server shall return the new hash, delaying its response until it is available.
 *
 * Database Hash Characteristic Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xNNNN               0x2803              Char Prop = 0x02            Read Only
 *                                          0xMMMM Char Value Handle
 *                                          0x2B2A
 * Database Hash Char Value Declaration
 * Attribute Handle     Attribute Type      Attribute Value             Attribute Permission
 * 0xMMMM               0x2B2A              uint128 database hash       Read Only
 *
 * Database Hash Calaculation
 *
 * The Database Hash shall be cacluated according to RFC-4493. This RFC defines
 * the Cipher-based Message Authentication Code (CMAC) using AES-128 as the
 * block cipher function, also known as AES-CMAC.
 *
 * The inputs to AES-CMAC are:
 *      - m is the variable length data to be hashed
 *      - k is the 128-bit key, which shall be all zero
 *
 * The 128-bit databash hash is generated as AES-CMACk(m), where m is calculated as:
 *
 * In ascending order of attribute handles, starting with the first handle,
 * concatenate the fields Attribute Handle, Attribute Type, and Attribute Value
 * if the attribute has one of the following types: <<Primary Service>>, <<Secondary
 * Service>>, <<Included Service>>, <<Characteristic>>, or <<Characteristic Extended
 * Properties>>; concatenate the fields Attribute Handle and Attribute Type if the
 * attribute has one of the following types: <<Characteristic User Description>>,
 * <<Client Characteristic Configuration>>, <<Server Characteristic Configuration>>,
 * <<Characteristic Format>>, or <<Charactersitic Aggregate Format>>; and ignore the
 * attribute if it has any other type, such attributes are not parot of the concatenation.
 *
 * For each Attribute Handle, the fields shall be concatenated in the order given above.
 * The byte order used for each field or sub-field value shall be little-endian. If a
 * field contains sub-fields, the subfields shall be concatenated in the order they
 * appear. For instance, a characteristic declaration value of {0x02, 0x0005, 0x2A00}
 * is represented after contenation as 02 05 00 00 2A.
 *
 * If the length of m is not a multiple of the AES-CMAC block length of 128 bits,
 * padding shall be applied as specified in RFC-4493 Section 2.4.
 *
 */

GATT_DECL_OPTIONAL_CHAR(g_gatt_server_database_hash,
    GATT_CHAR_UUID_DATABASE_HASH,
    GATT_RD_REQ,
    ATT_SEC_NONE);

// Gatt Service Declaration

GATT_DECL_PRI_SERVICE(g_gatt_service, GATT_UUID_GATT_SERVICE);

static const gatt_attribute_t g_gatt_service_attr_list[] = {
    /* Service */
    gatt_attribute(g_gatt_service),
    /* Characteristics */
    gatt_attribute(g_gatt_service_changed),
        /* Descriptor */
        gatt_attribute(g_gatt_service_changed_cccd),
    gatt_attribute(g_gatt_client_supp_features),
    gatt_attribute(g_gatt_server_supp_features),
    gatt_attribute(g_gatt_server_database_hash), // keep this as last characteristic
};

#ifndef BLE_ONLY_ENABLED

/**
 * SDP Requirements - GAP Service
 *
 * A BR/EDR or BR/EDR/LE device that supports a GATT Server accessible over the
 * BR/EDR physical and that supports only one of ATT or EATT shall public the
 * SDP record shown below first table; if both ATT and EATT are supported, the
 * device shall publish the SDP record shown below second table.
 *
 * The GAP Service Start Handle shall be set to the attribute handle of the
 * <<GAP Service>> service declaration. The GAP Service End Handle shall be set
 * to the attribute handle of the last attribute within the <<GAP Service>>
 * service definition group.
 *
 * If a BR/EDR or BR/EDR/LE device supports a GATT-based service on the BR/EDR
 * transport, the service shall exist in the SDP Server and the GATT Server.
 *
 */

const uint8_t _gap_service_browse_group[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(3),
        SDP_DE_UUID_H1_D2,
        0x10, 0x02,
};

const uint8_t _gap_service_class_id_list[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(3),
        SDP_DE_UUID_H1_D2,
        SDP_SPLIT_16BITS_BE(GATT_UUID_GAP_SERVICE),
};

const uint8_t _gap_service_protocol_descriptor_list[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(19),
        SDP_DE_DESEQ_8BITSIZE_H2_D(6),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_L2CAP,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(PSM_ATT),
        SDP_DE_DESEQ_8BITSIZE_H2_D(9),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_ATT,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GAP_SERVICE_START_HANDLE), // GAP Service Start Handle
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GAP_SERVICE_END_HANDLE), // GAP Service End Handle
};

#if (EATT_CHAN_SUPPORT == 1)
const uint8_t _gap_service_additional_protocol_descriptor_list[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(21),
    SDP_DE_DESEQ_8BITSIZE_H2_D(19),
        SDP_DE_DESEQ_8BITSIZE_H2_D(6),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_L2CAP,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(PSM_EATT),
        SDP_DE_DESEQ_8BITSIZE_H2_D(9),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_ATT,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GAP_SERVICE_START_HANDLE), // GAP Service Start Handle
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GAP_SERVICE_END_HANDLE), // GAP Service End Handle
};
#endif

static const bt_sdp_record_attr_t _gap_service_sdp_attrs[] = { // list attr id in ascending order
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_SERVICE_CLASS_ID_LIST, _gap_service_class_id_list),
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_PROTOCOL_DESC_LIST, _gap_service_protocol_descriptor_list),
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_BROWSE_GROUP_LIST, _gap_service_browse_group),
#if (EATT_CHAN_SUPPORT == 1)
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_ADDITIONAL_PROT_DESC_LISTS, _gap_service_additional_protocol_descriptor_list),
#endif
};

/**
 * SDP Requirements - Gatt Service
 *
 * A device that supports Gatt Over BR/EDER and only one of ATT or EATT shall publish
 * the SDP record shown below frist table; if both ATT and EATT are supported, the device
 * shall publish the SDP record shown below the second table.
 *
 * The Gatt Service Start Handle shall be set to the attribute handle of the <<Gatt Service>>
 * service declaration. The Gatt Service End Handle shall be set to the attribute handle of
 * the last attribute within the <<Gatt Service>> service definition group.
 *
 */

const uint8_t _gatt_service_browse_group[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(3),
        SDP_DE_UUID_H1_D2,
        0x10, 0x02,
};

const uint8_t _gatt_service_class_id_list[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(3),
        SDP_DE_UUID_H1_D2,
        SDP_SPLIT_16BITS_BE(GATT_UUID_GATT_SERVICE),
};

const uint8_t _gatt_service_protocol_descriptor_list[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(19),
        SDP_DE_DESEQ_8BITSIZE_H2_D(6),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_L2CAP,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(PSM_ATT),
        SDP_DE_DESEQ_8BITSIZE_H2_D(9),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_ATT,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GATT_SERVICE_START_HANDLE), // GATT Service Start Handle
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GATT_SERVICE_END_HANDLE), // GATT Service End Handle
};

#if (EATT_CHAN_SUPPORT == 1)
const uint8_t _gatt_service_additional_protocol_descriptor_list[] = {
    SDP_DE_DESEQ_8BITSIZE_H2_D(21),
    SDP_DE_DESEQ_8BITSIZE_H2_D(19),
        SDP_DE_DESEQ_8BITSIZE_H2_D(6),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_L2CAP,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(PSM_EATT),
        SDP_DE_DESEQ_8BITSIZE_H2_D(9),
            SDP_DE_UUID_H1_D2,
                SERV_UUID_ATT,
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GATT_SERVICE_START_HANDLE), // GATT Service Start Handle
            SDP_DE_UINT_H1_D2,
                SDP_SPLIT_16BITS_BE(GATT_SERVICE_END_HANDLE), // GATT Service End Handle
};
#endif

static const bt_sdp_record_attr_t _gatt_service_sdp_attrs[] = { // list attr id in ascending order
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_SERVICE_CLASS_ID_LIST, _gatt_service_class_id_list),
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_PROTOCOL_DESC_LIST, _gatt_service_protocol_descriptor_list),
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_BROWSE_GROUP_LIST, _gatt_service_browse_group),
#if (EATT_CHAN_SUPPORT == 1)
    SDP_DEF_ATTRIBUTE(SERV_ATTRID_ADDITIONAL_PROT_DESC_LISTS, _gatt_service_additional_protocol_descriptor_list),
#endif
};

#endif /* BLE_ONLY_ENABLED */

static bt_status_t app_ble_gatt_server_send_service_change(uint32_t con_bfs)
{
    gatt_service_changed_t change;
    gatt_char_notify_t indicate = {NULL};

    change.start_affected_handle = co_host_to_uint16_le(0x0001);
    change.end_affected_handle = co_host_to_uint16_le(0xFFFF);

    indicate.character = g_gatt_service_changed;
    indicate.force_send_value = true;
    return gatts_send_value_indication(con_bfs, &indicate, (uint8_t *)&change, sizeof(change));
}

#if BLE_GATT_CLIENT_SUPPORT
static bt_status_t app_ble_gatt_send_local_client_supp_features(gatt_prf_t *prf, gatt_peer_character_t *c)
{
    if (prf == NULL)
    {
        return BT_STS_INVALID_PARM;
    }

    const gap_local_info_t *local_info = gap_local_info();
    gap_conn_item_t *conn = gap_get_conn_item(prf->connhdl);

    /**
     * The Client Support Features characteristic is used by the client to inform the server which
     * features are supported by the client. If the char exists on the server, the client may update
     * the Client Support Features bit field.
     *
     */
    if (conn)
    {
        uint8_t local_client_supp_features = 0;
        local_client_supp_features |= local_info->gatt_client_supp_eatt_bearer ? GATT_CLIENT_FEAT_EATT_BEARER : 0;
        local_client_supp_features |= local_info->gatt_client_supp_robust_caching ? GATT_CLIENT_FEAT_ROBUST_CACHING : 0;
        local_client_supp_features |= local_info->gatt_client_supp_recv_multi_notify ? GATT_CLIENT_FEAT_MULT_VALUE_NOTIFY : 0;
        return gattc_write_character_value(prf, c, &local_client_supp_features, sizeof(uint8_t));
    }
    else
    {
        CO_LOG_ERR_1(BT_STS_INVALID_CONN_HANDLE, conn->connhdl);
        return BT_STS_FAILED;
    }
}
#endif

static void app_ble_gatt_cache_peer_client_aware_the_change(uint16_t connhdl)
{
    gap_conn_item_t *conn = gap_get_conn_item(connhdl);
    if (conn == NULL)
    {
        return;
    }

    conn->sec.server_cache.service_change_unaware = false;
    gap_add_ble_device_record(&conn->sec); // refresh change_unawre to nv
}

static void app_ble_gatt_cache_recv_service_changed_confirm(uint16_t connhdl)
{
    app_ble_gatt_cache_peer_client_aware_the_change(connhdl);
}

static void app_ble_gatt_cache_recv_databash_hash_request(uint16_t connhdl)
{
    app_ble_gatt_cache_peer_client_aware_the_change(connhdl);
}

static int app_ble_gatt_server_callback(gatt_svc_t *svc, gatt_server_event_t event, gatt_server_callback_param_t param)
{
    gap_local_info_t *local_info = gap_local_info();
    switch (event)
    {
        case GATT_SERV_EVENT_CONN_OPENED:
        {
            CO_LOG_MAIN_S_3(BT_STS_GATT_SERVICE, GOPN, param.opened->service, g_gatt_service, g_gap_service);
            if (param.opened->service == g_gatt_service)
            {
                gatt_server_conn_opened_t *opened = param.opened;
                gap_conn_item_t *conn = opened->conn;
                if (local_info->server_database_hash_support && conn->sec.server_cache.service_change_unaware)
                {
                    app_ble_gatt_server_send_service_change(gap_conn_bf(conn->con_idx)); // server shall not send other ntf before recv cfm
                }
            }
            break;
        }
        case GATT_SERV_EVENT_CHAR_READ:
        {
            gatt_server_char_read_t *p = param.char_read;
            const uint8_t *c = p->character;
            if (c == g_gap_service_device_name)
            {
                uint8_t name_length = 0;
                const char *device_name = NULL;
                device_name = gap_local_le_name(&name_length);
                if (name_length == 0 || device_name == NULL)
                {
                    return true; // return empty name;
                }
                /* when the device is not discoverable, the device name characteristic
                 * should not be readable. */
                gatts_write_read_rsp_data(p->ctx, (uint8_t *)device_name, name_length);
                return true;
            }
            else if (c == g_gap_service_appearance)
            {
                uint8_t appearance[2] = {CO_SPLIT_UINT16_LE(local_info->device_appearance)};
                gatts_write_read_rsp_data(p->ctx, appearance, sizeof(uint16_t));
                return true;
            }
            else if (c == g_gap_service_central_addr_reso_supp)
            {
                uint8_t central_addr_reso_support = local_info->address_reso_support;
                gatts_write_read_rsp_data(p->ctx, (uint8_t *)&central_addr_reso_support, sizeof(uint8_t));
                return true;
            }
            else if (c == g_gap_service_use_only_rpa_after_bonding)
            {
                if (local_info->only_use_rpa_after_bonding)
                {
                    uint8_t only_use_rpa_after_bonding = 0; /* 0 - only RPAs will be used as local addresses after bonding */
                    gatts_write_read_rsp_data(p->ctx, &only_use_rpa_after_bonding, sizeof(uint8_t));
                    return true;
                }
            }
            else if (c == g_gap_service_periph_prefer_conn_params)
            {
                gap_conn_prefer_params_t param_le;
                if (local_info->peripheral_has_prefer_conn_params)
                {
                    param_le.conn_interval_min_1_25ms = co_host_to_uint16_le(local_info->perheral_prefer_conn_params.conn_interval_min_1_25ms);
                    param_le.conn_interval_max_1_25ms = co_host_to_uint16_le(local_info->perheral_prefer_conn_params.conn_interval_max_1_25ms);
                    param_le.max_peripheral_latency = co_host_to_uint16_le(local_info->perheral_prefer_conn_params.max_peripheral_latency);
                    param_le.superv_timeout_ms = co_host_to_uint16_le(local_info->perheral_prefer_conn_params.superv_timeout_ms/10);
                }
                else
                {
                    param_le.conn_interval_min_1_25ms = 0xFFFF; // shall contain the value 0xFFF which indicates that no specific value is requested
                    param_le.conn_interval_max_1_25ms = 0xFFFF;
                    param_le.max_peripheral_latency = 0xFFFF;
                    param_le.superv_timeout_ms = 0xFFFF;
                }
                gatts_write_read_rsp_data(p->ctx, (uint8_t *)&param_le, sizeof(gap_conn_prefer_params_t));
                return true;
            }
            else if (c == g_gap_service_le_gatt_security_levels)
            {
                gatt_security_level_t sec[3];
                gap_security_levels_t sec_levels = local_info->sec_levels;
                int count = 0;
                if (sec_levels.link_sec_level)
                {
                    sec[count].mode = 1;
                    sec[count].level = sec_levels.link_sec_level;
                    count += 1;
                }
                if (sec_levels.data_sign_sec_exist)
                {
                    sec[count].mode = 2;
                    sec[count].level = sec_levels.data_sign_sec_level;
                    count += 1;
                }
                if (sec_levels.big_sec_level)
                {
                    sec[count].mode = 3;
                    sec[count].level = sec_levels.big_sec_level;
                    count += 1;
                }
                if (count)
                {
                    gatts_write_read_rsp_data(p->ctx, (uint8_t *)&sec, sizeof(gatt_security_level_t) * count);
                    return true;
                }
            }
            else if (c == g_gap_service_enc_data_key_material)
            {
                const gap_encrypted_data_key_material_t *key = &local_info->key_material;
                if (key->is_exist)
                {
                    gatts_write_read_rsp_data(p->ctx, (uint8_t *)&key->data, sizeof(gap_key_material_t));
                    return true;
                }
            }
            else if (c == g_gatt_client_supp_features)
            {
                /**
                 * The Client Support Features characteristic is used by the client to inform the server which
                 * features are supported by the client. If the char exists on the server, the client may update
                 * the Client Support Features bit field.
                 *
                 * A Client Supported Features Char Value shall exist for each connected
                 * client. For clients with a trusted relationship, the Char Value shall be persistent across
                 * connections. For clients without a trusted relationship the Char Value shall be set to the
                 * default value at each connection.
                 *
                 */
                gap_conn_item_t *conn = p->conn;
                if (conn)
                {
                    uint8_t peer_client_supp_features = 0;
                    peer_client_supp_features |= conn->peer.gatt_client_supp_eatt_bearer ? GATT_CLIENT_FEAT_EATT_BEARER : 0;
                    peer_client_supp_features |= conn->peer.gatt_client_supp_robust_caching ? GATT_CLIENT_FEAT_ROBUST_CACHING : 0;
                    peer_client_supp_features |= conn->peer.gatt_client_supp_recv_multi_notify ? GATT_CLIENT_FEAT_MULT_VALUE_NOTIFY : 0;
                    gatts_write_read_rsp_data(p->ctx, &peer_client_supp_features, sizeof(uint8_t));
                    return true;
                }
            }
            else if (c == g_gatt_server_supp_features)
            {
                uint8_t features = local_info->gatt_server_supp_eatt_bearer ? GATT_SERVER_FEAT_EATT_BEARER : 0;
                gatts_write_read_rsp_data(p->ctx, &features, sizeof(uint8_t));
                return true;
            }
            else if (c == g_gatt_server_database_hash)
            {
                if (local_info->server_database_hash_support)
                {
                    gap_local_info_t *local = gap_local_info();
                    gatts_write_read_rsp_data(p->ctx, local->local_database_hash, 16);
                    app_ble_gatt_cache_recv_databash_hash_request(svc->connhdl);
                    return true;
                }
            }
            break;
        }
        case GATT_SERV_EVENT_CHAR_WRITE:
        {
            gatt_server_char_write_t *p = param.char_write;
            const uint8_t *c = p->character;
            if (c == g_gap_service_device_name)
            {
                if (p->value_offset == 0 && p->value_len && p->value)
                {
                    CO_LOG_MAIN_WITH_STR_1(BT_STS_GATT_DEVICE_NAME, (char *)p->value, p->value_len);
                    gap_set_local_le_name(p->value, p->value_len);
                    return true;
                }
                else
                {
                    p->rsp_error_code = ATT_ERROR_VALUE_NOT_ALLOWED;
                    return false;
                }
            }
            else if (c == g_gap_service_appearance)
            {
                if (p->value_offset == 0 && p->value_len == sizeof(uint16_t))
                {
                    uint16_t appearance = CO_COMBINE_UINT16_LE(p->value);
                    CO_LOG_MAIN_S_1(BT_STS_GATT_APPEARANCE, WRAP, appearance);
                    local_info->device_appearance = appearance;
                    return true;
                }
                else
                {
                    p->rsp_error_code = (p->value_offset > sizeof(uint16_t )) ? ATT_ERROR_INVALID_OFFSET : ATT_ERROR_VALUE_NOT_ALLOWED;
                    return false;
                }
            }
            else if (c == g_gatt_client_supp_features)
            {
                gap_conn_item_t *conn = p->conn;
                if (conn && p->value_offset == 0 && p->value_len)
                {
                    /* A client shall not clear any bits it has set. The server shall respond to any such request
                     * with the Error Code of Value Not Allowed (0x13). */
                    uint8_t peer_client_supp_features = 0;
                    peer_client_supp_features |= conn->peer.gatt_client_supp_eatt_bearer ? GATT_CLIENT_FEAT_EATT_BEARER : 0;
                    peer_client_supp_features |= conn->peer.gatt_client_supp_robust_caching ? GATT_CLIENT_FEAT_ROBUST_CACHING : 0;
                    peer_client_supp_features |= conn->peer.gatt_client_supp_recv_multi_notify ? GATT_CLIENT_FEAT_MULT_VALUE_NOTIFY : 0;
                    if ((p->value[0] & peer_client_supp_features) == peer_client_supp_features)
                    {
                        peer_client_supp_features = p->value[0];
                        conn->peer.gatt_client_supp_eatt_bearer = (peer_client_supp_features & GATT_CLIENT_FEAT_EATT_BEARER) ? 1 : 0;
                        conn->peer.gatt_client_supp_robust_caching = (peer_client_supp_features & GATT_CLIENT_FEAT_ROBUST_CACHING) ? 1 : 0;
                        conn->peer.gatt_client_supp_recv_multi_notify = (peer_client_supp_features & GATT_CLIENT_FEAT_MULT_VALUE_NOTIFY) ? 1 : 0;
                        CO_LOG_MAIN_S_2(BT_STS_GATT_CLIENT_SUPP_FEATURE, CFET, conn->connhdl, peer_client_supp_features);
                        return true;
                    }
                    else
                    {
                        p->rsp_error_code = ATT_ERROR_VALUE_NOT_ALLOWED;
                        return false;
                    }
                }
            }
            break;
        }
        case GATT_SERV_EVENT_INDICATE_CFM:
        {
            gatt_server_indicate_cfm_t *confirm = param.confirm;
            if (gatts_get_char_byte_16_bit_uuid(confirm->character) == GATT_CHAR_UUID_SERVICE_CHANGED)
            {
                app_ble_gatt_cache_recv_service_changed_confirm(svc->connhdl);
            }
            break;
        }
        case GATT_SERV_EVENT_DESC_READ:
        case GATT_SERV_EVENT_DESC_WRITE:
        default:
        {
            break;
        }
    }

    return 0;
}

#if BLE_GATT_CLIENT_SUPPORT
static int app_ble_gatt_client_callback(gatt_prf_t *prf, gatt_profile_event_t event, gatt_profile_callback_param_t param)
{
    switch (event)
    {
        case GATT_PROF_EVENT_OPENED:
        {
            gattc_discover_service(prf, GATT_UUID_GAP_SERVICE, NULL);
            gattc_discover_service(prf, GATT_UUID_GATT_SERVICE, NULL);
            break;
        }
        case GATT_PROF_EVENT_CLOSED:
        {
            break;
        }
        case GATT_PROF_EVENT_SERVICE:
        {
            gatt_profile_service_t *p = param.service;
            gatt_peer_service_t *s = p->service;
            if (p->error_code != ATT_ERROR_NO_ERROR)
            {
                CO_LOG_ERR_2(BT_STS_SERVICE_NOT_FOUND, p->service_uuid, p->error_code);
                break;
            }
            if (s->service_uuid == GATT_UUID_GAP_SERVICE)
            {
                uint16_t gap_chars[] = {
                        GATT_CHAR_UUID_DEVICE_NAME,
                        GATT_CHAR_UUID_APPEARANCE,
                        GATT_CHAR_UUID_CENTRAL_ADDRESS_RESOLUTION,
                        GATT_CHAR_UUID_RPA_ONLY,
                        GATT_CHAR_UUID_PERIPH_PREFER_CONN_PARAMS,
                        GATT_CHAR_UUID_LE_GATT_SECURITY_LEVELS,
                        GATT_CHAR_UUID_ENCRYPTED_DATA_KEY_MATERIAL,
                    };
                gattc_discover_multi_characters(prf, s, gap_chars, sizeof(gap_chars)/sizeof(uint16_t));
                break;
            }
            if (s->service_uuid == GATT_UUID_GATT_SERVICE)
            {
                uint16_t gatt_chars[] = {
                        GATT_CHAR_UUID_SERVICE_CHANGED,
                        GATT_CHAR_UUID_CLIENT_SUPP_FEATURES,
                        GATT_CHAR_UUID_SERVER_SUPP_FEATURES,
                        GATT_CHAR_UUID_DATABASE_HASH};
                gattc_discover_multi_characters(prf, s, gatt_chars, sizeof(gatt_chars)/sizeof(uint16_t));
            }
            break;
        }
        case GATT_PROF_EVENT_CHARACTER:
        {
            gatt_profile_character_t *p = param.character;
            gatt_peer_service_t *s = p->service;
            gatt_peer_character_t *c = p->character;
            gap_conn_item_t *conn = p->conn;
            if (p->error_code != ATT_ERROR_NO_ERROR)
            {
                CO_LOG_ERR_2(BT_STS_CHARACTER_NOT_FOUND, p->char_uuid, p->error_code);
                break;
            }
            if (s->service_uuid == GATT_UUID_GAP_SERVICE)
            {
                if (c->char_uuid == GATT_CHAR_UUID_ENCRYPTED_DATA_KEY_MATERIAL)
                {
                    p->conn->peer.enc_data_key_material_char = (uint8_t *)c;
                }
                else
                {
                    gattc_read_character_value(prf, c);
                }
                break;
            }
            if (s->service_uuid == GATT_UUID_GATT_SERVICE)
            {
                if (c->char_uuid == GATT_CHAR_UUID_SERVICE_CHANGED)
                {
                    CO_LOG_MAIN_S_2(BT_STS_SERVICE_CHANGED, HSHG, conn->connhdl, c->char_value_handle);
                    conn->peer.gatt_server_has_service_changed_char = true;
                    gattc_write_cccd_descriptor(prf, c, false, true);
                }
                else if (c->char_uuid == GATT_CHAR_UUID_CLIENT_SUPP_FEATURES)
                {
                    app_ble_gatt_send_local_client_supp_features(prf, c);
                }
                else if (c->char_uuid == GATT_CHAR_UUID_DATABASE_HASH)
                {
                    uint8_t zero_hash[16] = {0};
                    CO_LOG_MAIN_S_2(BT_STS_DATABASE_HASH, HDSH, conn->connhdl, c->char_value_handle);
                    conn->peer.gatt_server_has_database_hash_char = true;
                    if (memcmp(conn->peer.service_database_hash, zero_hash, 16) == 0)
                    {
                        // only read for the 1st time, otherwise it is read in gatt_conn_ready_handler before profile open
                        gattc_read_character_value(prf, c);
                    }
                }
                else
                {
                    gattc_read_character_value(prf, c);
                }
            }
            break;
        }
        case GATT_PROF_EVENT_CHAR_READ_RSP:
        {
            gatt_profile_char_read_rsp_t *p = param.char_read_rsp;
            bool read_success = (p->error_code == ATT_ERROR_NO_ERROR) && p->value_len;
            uint16_t char_uuid = p->character->char_uuid;
            gap_conn_item_t *conn = p->conn;
            if (char_uuid == GATT_CHAR_UUID_ENCRYPTED_DATA_KEY_MATERIAL)
            {
                gap_key_material_t *key = (gap_key_material_t *)p->value;
                gap_peer_enc_data_key_material_received(conn, read_success ? key : NULL, p->error_code);
                break;
            }
            if (!read_success)
            {
                break;
            }
            if (char_uuid == GATT_CHAR_UUID_DEVICE_NAME)
            {
                uint8_t value_len = 0;
                if (p->value && p->value_len)
                {
                    value_len = (p->value_len <= GAP_MAX_DEVICE_NAME_LEN) ? p->value_len : GAP_MAX_DEVICE_NAME_LEN;
                    if (conn->peer.device_name)
                    {
                        cobuf_free((unsigned char *)conn->peer.device_name);
                        conn->peer.device_name = NULL;
                        conn->peer.name_length = 0;
                    }
                    conn->peer.device_name = (uint8_t *)cobuf_malloc(value_len+1);
                    if (conn->peer.device_name)
                    {
                        memcpy(conn->peer.device_name, p->value, value_len);
                        conn->peer.device_name[value_len] = 0;
                        conn->peer.name_length = value_len;
                    }
                    CO_LOG_MAIN_WITH_STR_2(BT_STS_GATT_DEVICE_NAME, (char *)conn->peer.device_name,
                        p->value_len, prf->connhdl);
                }
            }
            else if (char_uuid == GATT_CHAR_UUID_APPEARANCE)
            {
                uint16_t appearance = CO_COMBINE_UINT16_LE(p->value);
                CO_LOG_MAIN_S_2(BT_STS_GATT_APPEARANCE, APRA, prf->connhdl, appearance);
                conn->peer.device_appearance = appearance;
            }
            else if (char_uuid == GATT_CHAR_UUID_CENTRAL_ADDRESS_RESOLUTION)
            {
                CO_LOG_MAIN_S_2(BT_STS_GATT_CENTRAL_ADDR_RESOLUTION, CRES, prf->connhdl, p->value[0]);
                if (conn)
                {
                    conn->peer.central_addr_reso_support = p->value[0] ? true : false;
                }
            }
            else if (char_uuid == GATT_CHAR_UUID_RPA_ONLY)
            {
                CO_LOG_MAIN_S_2(BT_STS_ONLY_USE_RPA_AFTER_BONDING, ORPA, prf->connhdl, p->value[0]);
                if (conn)
                {
                    conn->peer.only_use_rpa_after_bonding = p->value[0] ? true : false;
                }
            }
            else if (char_uuid == GATT_CHAR_UUID_PERIPH_PREFER_CONN_PARAMS)
            {
                gap_conn_prefer_params_t *conn_params = (gap_conn_prefer_params_t *)p->value;
                uint16_t interval_min = co_uint16_le_to_host(conn_params->conn_interval_min_1_25ms);
                uint16_t interval_max = co_uint16_le_to_host(conn_params->conn_interval_max_1_25ms);
                uint16_t latency = co_uint16_le_to_host(conn_params->max_peripheral_latency);
                uint16_t timeout_ms = co_uint16_le_to_host(conn_params->superv_timeout_ms) * 10;
                CO_LOG_MAIN_S_3(BT_STS_PERIPH_PERFERRED_CONN_PARAMS, PPAR, (p->value_len<<16)|prf->connhdl,
                    (interval_min<<16)|interval_max, (latency<<16)|timeout_ms);
                if (conn)
                {
                    if (interval_min && interval_max && latency && timeout_ms)
                    {
                        conn->peer.peripheral_has_perfer_conn_params = true;
                        conn->peer.peripheral_prefer_conn_params.conn_interval_min_1_25ms = interval_min;
                        conn->peer.peripheral_prefer_conn_params.conn_interval_max_1_25ms = interval_max;
                        conn->peer.peripheral_prefer_conn_params.max_peripheral_latency = latency;
                        conn->peer.peripheral_prefer_conn_params.superv_timeout_ms = timeout_ms;
                    }
                    else
                    {
                        conn->peer.peripheral_has_perfer_conn_params = false;
                    }
                }
            }
            else if (char_uuid == GATT_CHAR_UUID_LE_GATT_SECURITY_LEVELS)
            {
                gatt_security_level_t *sec = (gatt_security_level_t *)p->value;
                uint8_t mode = 0, level = 0;
                int count = p->value_len / sizeof(gatt_security_level_t);
                int i = 0;
                for (; i < count; i += 1)
                {
                    mode = sec[i].mode;
                    level = sec[i].level;

                    CO_LOG_MAIN_S_3(BT_STS_GATT_PEER_SEC_LEVEL, SLVL, (i<<16)|count, mode, level);

                    if (mode == 1)
                    {
                        conn->peer.sec_levels.link_sec_level = level;
                    }
                    else if (mode == 2)
                    {
                        conn->peer.sec_levels.data_sign_sec_exist = true;
                        conn->peer.sec_levels.data_sign_sec_level = level;
                    }
                    else if (mode == 3)
                    {
                        conn->peer.sec_levels.big_sec_level = level;
                    }

                    gap_peer_security_levels_received(conn, conn->peer.sec_levels);
                }
            }
            else if (char_uuid == GATT_CHAR_UUID_SERVER_SUPP_FEATURES)
            {
                CO_LOG_MAIN_S_2(BT_STS_GATT_SERVER_SUPP_FEATURE, SFET, prf->connhdl, p->value[0]);
                if (conn)
                {
                    conn->peer.gatt_server_supp_eatt_bearer =
                        (p->value[0] & GATT_SERVER_FEAT_EATT_BEARER) ? true : false;
                }
            }
            else if (char_uuid == GATT_CHAR_UUID_DATABASE_HASH)
            {
                gattc_recv_peer_database_hash(conn->connhdl, p->value, false);
            }
            break;
        }
        case GATT_PROF_EVENT_NOTIFY:
        {
            gatt_profile_recv_notify_t *notify = param.notify;
            if (notify->service->service_uuid == GATT_UUID_GATT_SERVICE && notify->character->char_uuid == GATT_CHAR_UUID_SERVICE_CHANGED)
            {
                const gatt_service_changed_t *value = (gatt_service_changed_t *)notify->value;
                uint16_t start_handle = co_uint16_le_to_host(value->start_affected_handle);
                uint16_t end_handle = co_uint16_le_to_host(value->end_affected_handle);
                if (notify->value_len != sizeof(gatt_service_changed_t))
                {
                    CO_LOG_ERR_1(BT_STS_INVALID_LENGTH, notify->value_len);
                    break;
                }
                CO_LOG_MAIN_S_3(BT_STS_PEER_SERVICE_CHANGED, SCHG, (prf->prf_id<<16)|notify->character->char_value_handle, start_handle, end_handle);
                gattc_recv_peer_service_changed(prf->connhdl, start_handle, end_handle);
            }
            break;
        }
        case GATT_PROF_EVENT_INCLUDE:
        case GATT_PROF_EVENT_DESC_READ_RSP:
        case GATT_PROF_EVENT_CHAR_WRITE_RSP:
        case GATT_PROF_EVENT_DESC_WRITE_RSP:
        default:
        {
            break;
        }
    }

    return 0;
}
#endif /* BLE_GATT_CLIENT_SUPPORT */

bt_status_t app_ble_gatt_update_enc_data_key_material(const gap_key_material_t *key_material)
{
    gap_local_info_t *local = gap_local_info();
    local->key_material.is_exist = true;
    local->key_material.data = *key_material;
    return gatts_send_indication(GAP_ALL_CONNS, g_gap_service_enc_data_key_material,
        (uint8_t *)key_material, sizeof(gap_key_material_t));
}

bt_status_t app_ble_gatt_read_peer_enc_data_key_material(uint16_t connhdl)
{
#if BLE_GATT_CLIENT_SUPPORT
    if (bt_defer_curr_func_1(app_ble_gatt_read_peer_enc_data_key_material,
            bt_fixed_param(connhdl)))
    {
        return BT_STS_SUCCESS;
    }

    gap_local_info_t *g = gap_local_info();
    gap_conn_item_t *conn = NULL;
    gatt_prf_t prf;

    conn = gap_get_conn_item(connhdl);
    if (conn == NULL)
    {
        CO_LOG_ERR_1(BT_STS_INVALID_CONN_HANDLE, connhdl);
        return BT_STS_INVALID_CONN_HANDLE;
    }

    connhdl = conn->connhdl; // update to real conn handle

    if (!conn->peer.enc_data_key_material_char)
    {
        CO_LOG_ERR_0(BT_STS_NOT_FOUND);
        return BT_STS_NOT_FOUND; // peer not support or gatt is reading
    }

    prf.prf_id = g->gatt_self_prf_id;
    prf.connhdl = conn->connhdl;
    prf.con_idx = conn->con_idx;

    return gattc_read_character_value(&prf, (gatt_peer_character_t *)conn->peer.enc_data_key_material_char);
#else
    return BT_STS_NOT_SUPPORT;
#endif
}

static void app_ble_gatt_cache_server_service_changed(void)
{
    gap_local_info_t *local_info = gap_local_info();

    CO_LOG_MAIN_S_0(BT_STS_SERVICE_CHANGED, LSHG);

    if (local_info->server_database_hash_support == false)
    {
        return;
    }

    gap_set_nv_all_device_change_unaware();

    app_ble_gatt_server_send_service_change(GAP_ALL_CONNS);
}

static void app_ble_gatt_gen_local_hash_complete(void *priv, int error_code, const uint8_t *hash)
{
    gap_local_info_t *local = gap_local_info();
    struct pp_buff *ppb = (struct pp_buff *)priv;
    const uint8_t *nv_database_hash = NULL;
    bool local_service_changed = false;
    uint8_t zero_hash[16] = {0};

    if (error_code || hash == NULL)
    {
        CO_LOG_ERR_1(BT_STS_GEN_DATABASE_HASH_FAILED, error_code);
        goto label_free_ppb;
    }

    gap_set_nv_local_database_hash(hash);

    nv_database_hash = gap_get_nv_local_database_hash();

    CO_LOG_MAIN_S_3(BT_STS_GATT_DATABASE_HASH_VALUE, LDHS, CO_COMBINE_UINT32_BE(hash),
        CO_COMBINE_UINT32_BE(hash+4), CO_COMBINE_UINT32_BE(hash+12));

    if (memcmp(local->local_database_hash, zero_hash, 16) != 0 &&
        memcmp(local->local_database_hash, nv_database_hash, 16) != 0)
    {
        local_service_changed = true;
    }

    memcpy(local->local_database_hash, nv_database_hash, 16);

    if (local_service_changed)
    {
        app_ble_gatt_cache_server_service_changed();
    }

label_free_ppb:
    ppb_free(ppb);
}

void app_ble_gap_update_local_database_hash(void)
{
    if (bt_defer_curr_func_0(app_ble_gap_update_local_database_hash))
    {
        return;
    }

    gap_local_info_t *local = gap_local_info();
    const uint8_t *nv_database_hash = gap_get_nv_local_database_hash();
    memcpy(local->local_database_hash, nv_database_hash, 16);
    gatts_gen_local_database_hash(app_ble_gatt_gen_local_hash_complete);
}

void app_ble_gatt_standard_service_init(void)
{
    gap_local_info_t *g = gap_local_info();
    gatts_cfg_t gatt_svc_cfg = {0};
    uint16_t gap_service_attrs = 0;
    uint16_t gatt_service_attrs = 0;

    gap_service_attrs = ARRAY_SIZE(g_gap_service_attr_list);

    gatt_service_attrs = ARRAY_SIZE(g_gatt_service_attr_list);

    if (g->server_database_hash_support == false)
    {
        gatt_service_attrs -= 1;
    }

    gatts_register_service(g_gap_service_attr_list, gap_service_attrs, app_ble_gatt_server_callback, NULL);

    gatt_svc_cfg.dont_delay_report_conn_open = true;

    gatts_register_service(g_gatt_service_attr_list, gatt_service_attrs, app_ble_gatt_server_callback, &gatt_svc_cfg);

#if BLE_GATT_CLIENT_SUPPORT
    g->gatt_self_prf_id = gattc_register_profile(app_ble_gatt_client_callback, NULL);
#endif

#ifndef BLE_ONLY_ENABLED
    if (bt_l2cap_get_config().gatt_over_br_edr && bt_l2cap_get_config().stack_new_design)
    {
        uint8_t btgatt_add_std_gap_service_sdp = true;
        uint8_t btgatt_add_std_gatt_service_sdp = true;
        int gap_sdp_attrs = ARRAY_SIZE(_gap_service_sdp_attrs);
        int gatt_sdp_attrs = ARRAY_SIZE(_gatt_service_sdp_attrs);

#if (EATT_CHAN_SUPPORT == 1)
        if (!bt_l2cap_get_config().eatt_over_br_edr)
        {
            gap_sdp_attrs -= 1;

            gatt_sdp_attrs -= 1;
        }
#endif
        if (btgatt_add_std_gap_service_sdp)
        {
            bt_sdp_create_record(_gap_service_sdp_attrs, gap_sdp_attrs);
        }

        if (btgatt_add_std_gatt_service_sdp)
        {
            bt_sdp_create_record(_gatt_service_sdp_attrs, gatt_sdp_attrs);
        }
    }
#endif
}

void app_ble_init(void)
{
    ble_global_t *g = ble_get_global();
    gap_conn_prefer_params_t params = {0};
    ble_adv_activity_t *adv = NULL;
    int max_size = ARRAY_SIZE(g->adv);
    int i = 0;

    memset(g, 0, sizeof(ble_global_t));

    for (; i < max_size; i += 1)
    {
        adv = g->adv + i;
        memset(adv, 0, sizeof(ble_adv_activity_t));
        adv->adv_handle = GAP_INVALID_ADV_HANDLE;
    }

    g->default_tx_pref_phy_bits = GAP_PHY_BIT_LE_2M;
    g->default_rx_pref_phy_bits = GAP_PHY_BIT_LE_2M;

    app_ble_set_local_sec_levels();

    app_ble_gatt_standard_service_init();

    params.conn_interval_min_1_25ms = 200; // 250ms, connection interal = interval * 1.25ms
    params.conn_interval_max_1_25ms = 960; // 1200ms, connection interal = interval * 1.25ms
    params.max_peripheral_latency = 0x00; // 0x00 to 0x01F3, max peripheral latency in units of subrated conn intervals
    params.superv_timeout_ms = 30*1000; // 0x0A to 0x0C80 * 10ms, 100ms to 32s
    gap_set_peripheral_prefer_conn_parameters(&params);

    nv_record_blerec_init();

    app_ble_customif_init();

#ifdef CFG_SEC_CON
    // Note: for the product that needs secure connection feature while google
    // fastpair is not included, the ble private/public key should be
    // loaded from custom parameter section
    memcpy(ble_public_key, bes_demo_Public_key, sizeof(ble_public_key));
    memcpy(ble_private_key, bes_demo_private_key, sizeof(ble_private_key));
#endif

    app_ble_register_advertising(BLE_BASIC_ADV_HANDLE, app_ble_stub_adv_activity_prepare);

#if (BLE_AUDIO_ENABLED)
    app_ble_register_advertising(BLE_AUDIO_ADV_HANDLE, app_ble_audio_adv_activity_prepare);
#endif

#if defined(BLE_HID_ENABLE)
    ble_hid_device_init();
#endif

#if defined(BLE_HID_HOST)
    ble_hid_host_init();
#endif

#ifdef CFG_APP_AHP_SERVER
    ble_ahp_init();
#endif

#ifdef ANCS_ENABLED
    ble_ancs_init();
#endif

#if defined(ANCC_ENABLED)
    ble_ancc_init();
#endif

#if defined(BLE_IAS_ENABLED)
    ble_iac_init();
#endif

#if defined(CFG_APP_SAS_SERVER)
    ble_sass_init();
#endif

#if defined(BES_MOBILE_SAS)
    ble_sasc_init();
#endif

#ifdef CFG_APP_GFPS
    ble_gfps_init();
#endif

#ifdef SWIFT_ENABLED
    app_swift_init();
#endif

#if defined(BES_OTA) && !defined(OTA_OVER_TOTA_ENABLED)
    ble_ota_init();
#endif

#ifdef BLE_TOTA_ENABLED
    ble_tota_init();
#endif

#if defined(__AMA_VOICE__)
    ble_ai_ama_init();
#endif

#if defined(__DMA_VOICE__)
    ble_ai_dma_init();
#endif

#if defined(__GMA_VOICE__)
    ble_ai_gma_init();
#endif

#if defined(__SMART_VOICE__)
    ble_ai_smart_voice_init();
#endif

#if defined(__TENCENT_VOICE__)
    ble_ai_tencent_voice_init();
#endif

#if defined(DUAL_MIC_RECORDING)
    ble_ai_recording_init();
#endif

#if defined(__CUSTOMIZE_VOICE__)
    ble_ai_customize_init();
#endif

#ifdef CFG_APP_DATAPATH_SERVER
    ble_datapath_init();
#endif

#ifdef TILE_DATAPATH
    ble_tile_init();
#endif

#ifdef BLE_IAS_ENABLED
    ble_ias_init();
#endif
}

#endif /* BLE_HOST_SUPPORT */

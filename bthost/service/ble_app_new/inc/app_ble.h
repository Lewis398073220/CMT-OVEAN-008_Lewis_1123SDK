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
#ifndef __APP_BLE_H__
#define __APP_BLE_H__
#include "gatt_service.h"
#include "ble_core_common.h"
#include "ble_ai_voice.h"
#include "app_ble_test.h"
#include "app_custom.h"
#include "nvrecord_extension.h"
#include "ble_datapath.h"
#include "ble_tws.h"
#ifdef __cplusplus
extern "C" {
#endif

#define BLE_BASIC_ADV_HANDLE        0x00 // for legacy ble, ota, ai, dp, tile etc. advertising
#define BLE_AUDIO_ADV_HANDLE        0x01 // for ble audio adertising
#define BLE_GFPS_ADV_HANDLE         0x02
#define BLE_SPOT_ADV_HANDLE         0x03
#define BLE_SWIFT_ADV_HANDLE        0x04
#define BLE_CUSTOMER0_ADV_HANDLE    0x05
#define BLE_CUSTOMER1_ADV_HANDLE    0x06
#define BLE_CUSTOMER2_ADV_HANDLE    0x07
#define BLE_CUSTOMER3_ADV_HANDLE    0x08
#define BLE_MAX_FIXED_ADV_HANDLE    0x09

#if defined(BISTO_ENABLED)||defined(__AI_VOICE_BLE_ENABLE__)|| \
    defined(CTKD_ENABLE)||defined(GFPS_ENABLED)||(BLE_AUDIO_ENABLED)
#ifndef BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON
#ifndef CFG_SEC_CON
#define CFG_SEC_CON
#endif
#endif
#endif

#define FAST_PAIR_REV_2_0   1
#define BLE_APP_GFPS_VER    FAST_PAIR_REV_2_0
#undef CFG_APP_GFPS

#ifdef GFPS_ENABLED
#if BLE_APP_GFPS_VER==FAST_PAIR_REV_2_0
#define CFG_APP_GFPS
#endif
#endif

typedef struct ble_adv_activity_t {
    uint8_t adv_handle;
    bool adv_is_started;
    BLE_ADV_USER_E user;
    uint32_t custom_adv_interval_ms;
    gap_adv_param_t adv_param;
    bool (*adv_activity_func)(struct ble_adv_activity_t *adv);
} ble_adv_activity_t;

typedef struct {
    ble_callback_evnet_type_e evt_type;
    union {
    struct {
        uint8_t newRole;
    } rs_complete_handled;
    struct {
        uint8_t newRole;
    } role_update_handled;
    struct {
        uint8_t event;
    } ibrt_event_entry_handled;
    } p;
} ble_callback_event_t;

typedef void (*APP_BLE_CORE_EVENT_CALLBACK)(ble_event_t *);
typedef void (*app_ble_mtu_exch_cb_t)(uint8_t con_lid, uint32_t mtu_size);

typedef struct {
    ble_adv_activity_t adv[BLE_ADV_USER_NUM];
    BLE_DATA_FILL_FUNC_T data_fill_func[BLE_ADV_USER_NUM];
    bool data_fill_enable[BLE_ADV_USER_NUM];
    uint32_t adv_force_disabled;
    APP_BLE_CORE_EVENT_CALLBACK ble_core_evt_cb;
    APP_BLE_CORE_GLOBAL_HANDLER_FUNC ble_global_handler;
    app_ble_mtu_exch_cb_t ble_link_mtu_exch_ind_callback;
    set_rsp_dist_lk_bit_field_func dist_lk_set_cb;
    smp_identify_addr_exch_complete ble_smp_info_derived_from_bredr_complete;
    void (*ble_global_handle)(ble_event_t *event, void *output);
    bool ble_stub_adv_enable;
    uint8_t default_tx_pref_phy_bits;
    uint8_t default_rx_pref_phy_bits;
    nvrec_appmode_e app_mode;
} ble_global_t;

#if defined(BLE_HID_ENABLE)
void ble_hid_device_init(void);
#endif

#if defined(BLE_HID_HOST)
void ble_hid_host_init(void);
bt_status_t ble_hid_host_start_discover(uint16_t connhdl);
#endif

#ifdef CFG_APP_AHP_SERVER
void ble_ahp_init(void);
#endif

#ifdef ANCS_ENABLED
void ble_ancs_init(void);
#endif

#if defined(ANCC_ENABLED)
void ble_ancc_init(void);
bt_status_t ble_ancc_start_discover(uint16_t connhdl);
#endif

#if defined (BLE_IAS_ENABLED)
int ble_ias_init(void);
int ble_ias_deinit(void);

int ble_iac_init(void);
int ble_iac_deinit(void);
int ble_iac_start_discover(uint8_t con_lid);
int iac_write_alert_level(uint8_t con_lid, uint8_t alert_level);
#endif

#if defined(CFG_APP_SAS_SERVER)
void ble_sass_init(void);
#endif

#if defined(BES_MOBILE_SAS)
void ble_sasc_init(void);
bt_status_t ble_sasc_start_discover(uint16_t connhdl);
#endif

#ifdef CFG_APP_GFPS
#include "ble_gfps_common.h"
struct ble_gfps_t;

void ble_gfps_init(void);
void app_gfps_ble_register_callback(fp_event_cb callback);
void app_enter_fastpairing_mode(void);
void app_exit_fastpairing_mode(void);
bool app_is_in_fastpairing_mode(void);
void app_gfps_enable_battery_info(bool enable);
void app_fast_pairing_timeout_timehandler(void);
void app_fast_pair_timeout_handler(void);
bool app_is_mobile_connected_via_fastpair(void);
void app_gfps_tws_sync_init(void);
void app_fast_pairing_timeout_timehandler(void);
void app_gfps_set_battery_datatype(GFPS_BATTERY_DATA_TYPE_E batteryDataType);
void app_gfps_set_battery_info_acquire_handler(gfps_get_battery_info_cb cb);
void app_gfps_send_keybase_pairing_via_notification(struct ble_gfps_t *gfps, const uint8_t *data, uint32_t length);

uint8_t app_gfps_l2cap_send(uint8_t conidx, uint8_t *ptrData, uint32_t length);
void app_gfps_l2cap_disconnect(uint8_t conidx);

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
void app_ibrt_share_fastpair_info(uint8_t *p_buff, uint16_t length);
void app_ibrt_shared_fastpair_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif
#endif

#ifdef SWIFT_ENABLED
void app_swift_init(void);
#endif

#if defined(BES_OTA) && !defined(OTA_OVER_TOTA_ENABLED)
typedef enum {
    APP_OTA_CCC_CHANGED = 0,
    APP_OTA_DIS_CONN,
    APP_OTA_RECEVICE_DATA,
    APP_OTA_MTU_UPDATE,
    APP_OTA_SEND_DONE,
} APP_OTA_EVENT_TYPE_E;
typedef struct {
    uint8_t event_type;
    uint8_t conidx;
    uint16_t connhdl;
    union {
    uint8_t ntf_en; // APP_OTA_CCC_CHANGED
    uint16_t mtu; // APP_OTA_MTU_UPDATE
    uint8_t status; // APP_OTA_SEND_DONE
    struct { // APP_OTA_RECEVICE_DATA
        uint16_t data_len;
        uint8_t *data;
    } receive_data;
    } param;
} app_ota_event_param_t;
typedef void(*app_ota_event_callback)(app_ota_event_param_t *param);
void ble_ota_init(void);
void app_ota_event_reg(app_ota_event_callback cb);
void app_ota_event_unreg(void);
void app_ota_send_rx_cfm(uint8_t conidx);
bool app_ota_send_notification(uint8_t conidx, uint8_t* data, uint32_t len);
bool app_ota_send_indication(uint8_t conidx, uint8_t* data, uint32_t len);
#endif

#ifdef BLE_TOTA_ENABLED
typedef enum {
    APP_TOTA_CCC_CHANGED = 0,
    APP_TOTA_DIS_CONN_EVENT,
    APP_TOTA_RECEVICE_DATA,
    APP_TOTA_MTU_UPDATE,
    APP_TOTA_SEND_DONE,
} APP_TOTA_EVENT_TYPE_E;
typedef struct {
    uint8_t event_type;
    uint8_t conidx;
    uint16_t connhdl;
    union {
    uint8_t ntf_en; // APP_TOTA_CCC_CHANGED
    uint16_t mtu; // APP_TOTA_MTU_UPDATE
    uint8_t status; // APP_TOTA_SEND_DONE
    struct { // APP_TOTA_RECEVICE_DATA
        uint16_t data_len;
        uint8_t *data;
    } receive_data;
    } param;
} app_tota_event_param_t;
typedef void(*app_tota_event_callback)(app_tota_event_param_t *param);
void ble_tota_init(void);
void app_tota_event_reg(app_tota_event_callback cb);
void app_tota_event_unreg(void);
bool app_tota_send_notification(uint8_t conidx, uint8_t* data, uint32_t len);
bool app_tota_send_indication(uint8_t conidx, uint8_t* data, uint32_t len);
#endif

typedef struct {
    const uint8_t *adv_data;
    const uint8_t *scan_rsp_data;
    uint16_t adv_data_len;
    uint16_t scan_rsp_len;
} app_ble_adv_data_param_t;

typedef struct {
    uint8_t scanType;
    uint8_t scanFolicyType;
    uint16_t scanWindowMs;
    uint16_t scanIntervalMs;
    uint16_t scanDurationMs;
} BLE_SCAN_PARAM_T;

typedef bool (*app_ble_adv_activity_func)(ble_adv_activity_t *adv);

void app_ble_init(void);
const gap_ext_func_cbs_t *app_ble_get_gap_ext_callbacks(void);
ble_global_t *ble_get_global(void);
void app_ble_ready_and_init_done(nvrec_appmode_e mode);
void app_ble_core_evt_cb_register(APP_BLE_CORE_EVENT_CALLBACK cb);
void app_ble_core_register_global_handler_ind(APP_BLE_CORE_GLOBAL_HANDLER_FUNC handler);
int app_ble_recv_stack_global_event(uintptr_t priv, gap_global_event_t event, gap_global_event_param_t param);
void app_ble_mtu_exec_ind_callback_register(app_ble_mtu_exch_cb_t cb);
void app_ble_mtu_exec_ind_callback_deregister(void);
void app_sec_reg_dist_lk_bit_set_callback(set_rsp_dist_lk_bit_field_func callback);
void *app_sec_reg_dist_lk_bit_get_callback(void);
void app_ble_start_connect(const ble_bdaddr_t *peer_addr, uint8_t ia_rpa_npa);
void app_ble_connect_ble_audio_device(const ble_bdaddr_t *peer_addr, uint8_t ia_rpa_npa);
void app_ble_start_auto_connect(const ble_bdaddr_t *addr_list, uint16_t list_size, uint8_t ia_rpa_npa, uint32_t connect_time);
void app_ble_cancel_connecting(void);
void app_ble_disconnect(uint16_t connhdl);
void app_ble_disconnect_all(void);
void app_ble_start_disconnect(uint8_t conidx);
bool app_ble_get_peer_solved_addr(uint8_t conidx, ble_bdaddr_t* p_addr);
ble_bdaddr_t app_ble_get_local_identity_addr(uint8_t conidx);
const uint8_t *app_ble_get_local_rpa_addr(uint8_t conidx);
void app_ble_read_local_rpa_addr(bt_addr_type_t addr_type, const bt_bdaddr_t *peer_addr);
bool app_ble_is_remote_mobile_connected(const ble_bdaddr_t *p_addr);
uint8_t app_ble_connection_count(void);
bool app_is_arrive_at_max_ble_connections(void);
bool app_ble_is_any_connection_exist(void);
bool app_ble_is_connection_on(uint8_t index);
uint16_t app_ble_get_conhdl_from_conidx(uint8_t conidx);
void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool enable);
void app_ble_update_conn_param_mode_of_specific_connection(uint8_t con_idx, BLE_CONN_PARAM_MODE_E mode, bool enable);
void app_ble_register_ia_exchanged_callback(smp_identify_addr_exch_complete callback);
void app_ble_set_white_list(BLE_WHITE_LIST_USER_E user, ble_bdaddr_t *bdaddr, uint8_t size);
void app_ble_clear_white_list(BLE_WHITE_LIST_USER_E user);
void app_ble_clear_all_white_list(void);
void app_ble_add_dev_to_rpa_list_in_controller(const ble_bdaddr_t *ble_addr, const uint8_t *irk);
void app_ble_add_devices_info_to_resolving(void);
void app_ble_send_security_req(uint8_t conidx);
void app_ble_start_scan(BLE_SCAN_PARAM_T *param);
void app_ble_stop_scan(void);
void app_ble_data_fill_enable(BLE_ADV_USER_E user, bool enable);
void app_ble_register_data_fill_handle(BLE_ADV_USER_E user, BLE_DATA_FILL_FUNC_T func, bool enable);
void app_ble_dt_set_flags(gap_adv_param_t *adv_param, bool simu_bredr_support);
void app_ble_dt_set_local_name(gap_adv_param_t *adv_param, const char *cust_le_name);
void app_ble_parse_out_adv_data_service_uuid(BLE_ADV_PARAM_T *data, gap_dt_buf_t *out_uuid_16, gap_dt_buf_t *out_uuid_128);
void app_ble_parse_out_scan_rsp_service_uuid(BLE_ADV_PARAM_T *data, gap_dt_buf_t *out_uuid_16, gap_dt_buf_t *out_uuid_128);
BLE_ADV_ACTIVITY_USER_E app_ble_param_get_actv_user_from_adv_user(BLE_ADV_USER_E user);
void app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_E adv_intv_user, BLE_ADV_USER_E adv_user, uint32_t interval);
void app_ble_force_switch_adv(enum BLE_ADV_SWITCH_USER_E user, bool enable_adv);
bool app_ble_is_connection_on_by_addr(uint8_t *addr);
void app_ble_start_connectable_adv(uint16_t advInterval);
void app_ble_refresh_adv_state(uint16_t advInterval);
bool app_ble_is_in_advertising_state(void);
bool ble_adv_is_allowed(void);
void ble_core_enable_stub_adv(void);
void ble_core_disable_stub_adv(void);
void app_ble_stub_user_init(void);
void app_ble_start_adv(void);
void app_ble_refresh_adv(void);
void app_ble_enable_advertising(uint8_t adv_handle);
void app_ble_disable_advertising(uint8_t adv_handle);
void app_ble_sync_ble_info(void);
void app_ble_mode_tws_sync_init(void);
void ble_roleswitch_start(void);
void ble_roleswitch_complete(uint8_t newRole);
void ble_role_update(uint8_t newRole);
void ble_ibrt_event_entry(uint8_t ibrt_evt_type);
int8_t app_ble_get_rssi(uint8_t conidx);
uint8_t *app_ble_get_dev_name(void);
uint8_t app_ble_own_addr_type(void);
ble_bdaddr_t app_get_current_ble_addr(void);
ble_adv_activity_t *app_ble_register_advertising(uint8_t adv_handle, app_ble_adv_activity_func adv_activity_func);
ble_adv_activity_t *app_ble_get_advertising_by_user(BLE_ADV_USER_E user);
void app_ble_set_tx_rx_pref_phy(uint32_t tx_pref_phy, uint32_t rx_pref_phy);
void app_ble_set_adv_txpwr_by_adv_user(BLE_ADV_USER_E user, int8_t txpwr_dbm);
void app_ble_set_adv_tx_power_dbm(ble_adv_activity_t *adv, int8_t tx_power_dbm);
void app_ble_set_adv_tx_power_level(ble_adv_activity_t *adv, BLE_ADV_TX_POWER_LEVEL_E tx_power_level);
void app_ble_dt_add_adv_data(ble_adv_activity_t *adv, BLE_ADV_PARAM_T *a, const app_ble_adv_data_param_t *b);
bool app_ble_get_user_adv_data(ble_adv_activity_t *adv, BLE_ADV_PARAM_T *param, int user_group);
bt_status_t app_ble_gatt_update_enc_data_key_material(const gap_key_material_t *key_material);
bt_status_t app_ble_gatt_read_peer_enc_data_key_material(uint16_t connhdl);
void app_ble_gap_update_local_database_hash(void);

#if defined(IBRT)
bool app_ble_check_ibrt_allow_adv(BLE_ADV_USER_E user);
void ble_audio_tws_sync_init(void);
int app_ble_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __APP_BLE_H__ */

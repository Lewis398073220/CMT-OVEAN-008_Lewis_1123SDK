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

#ifndef __APP_BLE_MODE_SWITCH_H__
#define __APP_BLE_MODE_SWITCH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************header include********************************/
#include "bluetooth_bt_api.h"
#include "co_bt_defines.h"
#include "app_ble_param_config.h"
#include "ble_core_common.h"
/******************************macro defination*****************************/
#define BLE_ADV_DATA_STRUCT_HEADER_LEN (2)
#define BLE_ADV_TYPE_SHORTENED_NAME 0x08
#define BLE_ADV_TYPE_COMPLETE_NAME 0x09

#ifndef BLE_CONNECTION_MAX
#define BLE_CONNECTION_MAX (1)
#endif
#ifndef BLE_AUDIO_CONNECTION_CNT
#define BLE_AUDIO_CONNECTION_CNT (2)
#endif
#ifndef ADV_DATA_LEN
#define ADV_DATA_LEN                    0x1F
#endif
#ifndef BLE_ADV_FLAG_PART_LEN
#define BLE_ADV_FLAG_PART_LEN           0x03
#endif
#ifndef BLE_ADV_DATA_WITHOUT_FLAG_LEN
#define BLE_ADV_DATA_WITHOUT_FLAG_LEN   (ADV_DATA_LEN)
#endif
#ifndef BLE_ADV_DATA_WITH_FLAG_LEN
#define BLE_ADV_DATA_WITH_FLAG_LEN      (ADV_DATA_LEN - BLE_ADV_FLAG_PART_LEN)
#endif

#define BLE_EXT_ADV_DATA_STRUCT_HEADER_LEN (2)

#define WHITE_LIST_MAX_NUM 8
#define RESOLVING_LIST_MAX_NUM 8

#define BLE_ADV_SVC_FLAG  0x16
#define BLE_ADV_MANU_FLAG 0xFF

// Maximal length of the Device Name value
#define APP_DEVICE_NAME_MAX_LEN      (32)

#define ADV_PARTICLE_HEADER_LEN (1)

#define __ARRAY_EMPTY_F 1

#define BLE_ADV_INVALID_TXPWR (100)

/******************************type defination******************************/

/// Initiating Types
typedef enum app_ble_init_type
{
    /// Direct connection establishment, establish a connection with an indicated device
    APP_BLE_INIT_TYPE_DIRECT_CONN_EST = 0,
    /// Automatic connection establishment, establish a connection with all devices whose address is
    /// present in the white list
    APP_BLE_INIT_TYPE_AUTO_CONN_EST,
    /// Name discovery, Establish a connection with an indicated device in order to read content of its
    /// Device Name characteristic. Connection is closed once this operation is stopped.
    APP_BLE_INIT_TYPE_NAME_DISC,
} BLE_INIT_TYPE_E;

/// Activity action
typedef enum
{
    /// Activity do nothing
    BLE_ACTV_ACTION_IDLE = 0,

    /// Activity starting advertise
    BLE_ACTV_ACTION_STARTING_ADV,
    /// Activity stopping advertise
    BLE_ACTV_ACTION_STOPPING_ADV,

    /// Activity starting scan
    BLE_ACTV_ACTION_STARTING_SCAN,
    /// Activity stopping scan
    BLE_ACTV_ACTION_STOPPING_SCAN,

    /// Activity connecting
    BLE_ACTV_ACTION_CONNECTING,
    /// Activity stop connecting
    BLE_ACTV_ACTION_STOP_CONNECTING,
    /// Activity disconnecting
    BLE_ACTV_ACTION_DISCONNECTING,

    /// Activity Set WL
    BLE_ACTV_ACTION_SET_WHITE_LIST,
    /// Activity Set RL
    BLE_ACTV_ACTION_ADD_RESLO_LIST,
} BLE_ACTV_ACTION_E;

typedef struct {
    uint8_t scanType;
    uint8_t scanFolicyType;
    uint16_t scanWindowMs;
    uint16_t scanIntervalMs;
    uint16_t scanDurationMs;
} BLE_SCAN_PARAM_T;

typedef struct
{
    /// Initiating type (@see enum gapm_init_type)
    uint8_t gapm_init_type;
    /// Timeout for automatic connection establishment (in unit of 10ms). Cancel the procedure if not all
    /// indicated devices have been connected when the timeout occurs. 0 means there is no timeout
    uint16_t conn_to;
    /// Own address type (see enum #APP_GAPM_OWN_ADDR_E)
    uint8_t  own_addr_type;
    /// Address of peer device in case white list is not used for connection
    ble_bdaddr_t peer_addr;
    /// mark all phy conn en
    bool init_conn_all_phy_en;
} __attribute__((__packed__)) BLE_INIT_PARAM_T;

typedef struct {
    bool pendingConnect;
    /// Initiating param
    BLE_INIT_PARAM_T init_param;
} __attribute__((__packed__)) BLE_INIT_INFO_T;

typedef struct
{
    ///Connection interval value
    uint16_t            con_interval;
    ///Connection latency value
    uint16_t            con_latency;
    ///Supervision timeout
    uint16_t            sup_to;
} APP_BLE_CONN_PARAM_T;

typedef struct
{
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t conLatency;
    uint16_t supTO;
    bool exist;
    /// ble connection interval update times
    uint8_t ciUpdateTimes;
} __attribute ((packed)) APP_BLE_UPDATE_CI_T;

typedef struct {
    bool enable;
    /// Number of entries to be added in the list. 0 means that list content has to be cleared
    uint8_t size;
    /// BD Address of device
    ble_bdaddr_t bdaddr[WHITE_LIST_MAX_NUM];
} BLE_WHITE_LIST_PARAM_T;

typedef struct {
    uint8_t                                                       ble_is_busy : 1;
    uint8_t                                                 ble_is_connecting : 1;
    uint8_t                                                               rfu : 6;

    BLE_ACTV_ACTION_E                                             ble_actv_action;
    uint8_t                                                       ble_op_actv_idx;

    uint32_t                                                       disablerBitmap; //one bit represent one user
    uint32_t                                                    adv_user_register; //one bit represent one user
    uint32_t                           adv_user_enable[BLE_ADV_ACTIVITY_USER_NUM];   //one bit represent one user

    BLE_DATA_FILL_FUNC_T                        bleDataFillFunc[BLE_ADV_USER_NUM];

    // param used for BLE adv
    bool                                                       adv_has_pending_op;
    uint16_t                                                   advPendingInterval;
    BLE_ADV_TYPE_E                                                 advPendingType;
    ADV_DISC_MODE_E                                            advPendingDiscmode;
    ADV_MODE_E                                             advertisingPendingMode;
    ble_bdaddr_t                                               advPendingPeerAddr;
    int8_t                             advPendingTxpwr[BLE_ADV_ACTIVITY_USER_NUM];
    uint8_t     advPendingLocalAddr[BLE_ADV_ACTIVITY_USER_NUM][BTIF_BD_ADDR_SIZE];
    int8_t                             advCurrentTxpwr[BLE_ADV_ACTIVITY_USER_NUM];
    BLE_ADV_PARAM_T                     advCurrentInfo[BLE_ADV_ACTIVITY_USER_NUM];
    BLE_ADV_PARAM_T                                                  advParamInfo;
    /// BLE_ADV_ACTIVITY_USER_NUM Bits field that adv start fail
    uint8_t                                                      isStartAdvFailbf;

    // prarm used for BLE scan
    bool                                                      scan_has_pending_op;
    bool                                                 scan_has_pending_stop_op;
    BLE_SCAN_PARAM_T                                              scanCurrentInfo;
    BLE_SCAN_PARAM_T                                              scanPendingInfo;

    // prarm used for BLE initiating
    bool                                                  pending_stop_connecting;
    BLE_INIT_INFO_T                          bleToConnectInfo[BLE_CONNECTION_MAX];
    bool                                    pendingDisconnect[BLE_CONNECTION_MAX];

    // prarm used for set white list
    bool                                                   pending_set_white_list;
    BLE_WHITE_LIST_PARAM_T                ble_white_list[BLE_WHITE_LIST_USER_NUM];

    bool                                                pending_add_resloving_list;
    uint8_t                                    numberOfDevicesAddedToResolvingList;
    ble_gap_ral_dev_info_t  devicesInfoAddedToResolvingList[RESOLVING_LIST_MAX_NUM];

    // pointer of @see app_env
    struct app_env_tag                                                    *bleEnv;
} __attribute__((__packed__)) BLE_MODE_ENV_T;

/*---------------------------------------------------------------------------
 *            app_ble_mode_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    init the bleModeEnv
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_mode_init(void);

/*---------------------------------------------------------------------------
 *            app_ble_register_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    register a BLE advertisement and scan response data fill handler for a
 *    specific user(see @BLE_ADV_USER_E), so that the adv/scan response data
 *    could present in BLE adv/scan response data
 *
 * Parameters:
 *    user - see the defination in BLE_ADV_USER_E
 *    func - adv/scan response data fill handler for specific user
 *
 * Return:
 *    void
 */
void app_ble_register_data_fill_handle(BLE_ADV_USER_E user, BLE_DATA_FILL_FUNC_T func, bool enable);

/*---------------------------------------------------------------------------
 *            app_ble_data_fill_enable
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    enable/disable specific user to fill the adv/scan response data
 *
 * Parameters:
 *    user : user to enable/disable fill data
 *    enable : true - enable user
 *             false - disable user
 *
 * Return:
 *    void
 */
void app_ble_data_fill_enable(BLE_ADV_USER_E user, bool enable);

/*---------------------------------------------------------------------------
 *            app_ble_start_connectable_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connetable BLE advertise
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    None
 */
void app_ble_start_connectable_adv(uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_start_connectable_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connetable BLE advertise, only called by custom adv api
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    None
 */
void app_ble_start_connectable_adv_by_custom_adv(uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_refresh_adv_state_by_custom_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    refresh adv state, only called by custom adv api
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    None
 */
void app_ble_refresh_adv_state_by_custom_adv(uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_refresh_adv_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    refresh adv state
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    None
 */
void app_ble_refresh_adv_state(uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_set_adv_type
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble to set adv type
 *
 * Parameters:
 *    advType: advertising type
 *    peer_addr: peer address
 *
 * Return:
 *    None
 */
void app_ble_set_adv_type(BLE_ADV_TYPE_E advType, ble_bdaddr_t *peer_addr);

/*---------------------------------------------------------------------------
 *            app_ble_set_adv_disc_mode
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble to set adv discoverable mode
 *
 * Parameters:
 *    discMode: advertising discoverable mode
 *
 * Return:
 *    None
 */
void app_ble_set_adv_disc_mode(ADV_DISC_MODE_E discMode);

/*---------------------------------------------------------------------------
 *            app_ble_set_adv_txpwr_by_actv_user
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    set adv tx power value by activity user
 *
 * Parameters:
 *    actv_user: activity user
 *    txpwr_dbm: the value of tx power, -21 -> 16
 *
 * Return:
 *    None
 */
void app_ble_set_adv_txpwr_by_actv_user(BLE_ADV_ACTIVITY_USER_E actv_user, int8_t txpwr_dbm);

/*---------------------------------------------------------------------------
 *            app_ble_set_adv_txpwr_by_adv_user
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    set adv tx power value by adv user
 *
 * Parameters:
 *    user: adv user
 *    txpwr_dbm: the value of tx power, -21 -> 16
 *
 * Return:
 *    None
 */
void app_ble_set_adv_txpwr_by_adv_user(BLE_ADV_USER_E user, int8_t txpwr_dbm);

/*---------------------------------------------------------------------------
 *            app_ble_set_all_adv_txpwr
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    set adv tx power value of all activity
 *
 * Parameters:
 *    txpwr_dbm: the value of tx power, -21 -> 16
 *
 * Return:
 *    None
 */
void app_ble_set_all_adv_txpwr(int8_t txpwr_dbm);

/*---------------------------------------------------------------------------
 *            app_ble_set_adv_local_addr_by_adv_user
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble to set adv local address by advertising user
 *
 * Parameters:
 *    user: advertising user
 *    addr: the address that needed to set
 *
 * Return:
 *    None
 */
void app_ble_set_adv_local_addr_by_adv_user(BLE_ADV_USER_E user, uint8_t *addr);

/*---------------------------------------------------------------------------
 *            app_ble_set_adv_local_addr_by_actv_user
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble to set adv local address by adv activity user
 *
 * Parameters:
 *    actv_user: adv activity user
 *    addr: the address that needed to set
 *
 * Return:
 *    None
 */
void app_ble_set_adv_local_addr_by_actv_user(BLE_ADV_ACTIVITY_USER_E actv_user, uint8_t *addr);

/*---------------------------------------------------------------------------
 *            app_ble_force_switch_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    enable/disable all BLE adv request for specific UI user
 *
 * Parameters:
 *    user : UI user
 *    isToEnableAdv : true - enable BLE adv for specific UI user
 *            false -  disable BLE adv for specific UI user
 *
 * Return:
 *    void
 */
void app_ble_force_switch_adv(enum BLE_ADV_SWITCH_USER_E user, bool isToEnableAdv);

/*---------------------------------------------------------------------------
 *            app_ble_start_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE scan
 *
 * Parameters:
 *    BLE_SCAN_PARAM_T
 *
 * Return:
 *    void
 */
void app_ble_start_scan(BLE_SCAN_PARAM_T *param);

/*---------------------------------------------------------------------------
 *            app_ble_stop_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop BLE scan
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_stop_scan(void);

/*---------------------------------------------------------------------------
 *            app_ble_start_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connect BLE with the given address
 *
 * Parameters:
 *    peer_addr :      BLE address to connnect
 *    owner_addr_type: is indicates the type of address being used in the connection request packets
 *                     see@APP_GAPM_OWN_ADDR_E
 *
 * Return:
 *    void
 */
void app_ble_start_connect(const ble_bdaddr_t *peer_addr, uint8_t owner_addr_type);


/*---------------------------------------------------------------------------
 *            app_ble_start_connect
 *---------------------------------------------------------------------------
 * @brief connect with white list
 *
 * @param addr_list
 * @param size
 * @param owner_addr_type
 * @param conn_to
 */
void app_ble_start_connect_with_white_list(ble_bdaddr_t *addr_list, uint8_t size,
                                           uint8_t owner_addr_type, uint16_t conn_to);

/*---------------------------------------------------------------------------
 *            app_ble_start_connect_with_init_type
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connect BLE with the given address
 *
 * Parameters:
 *    bdAddrToConnect : BLE address to connnect
 *
 * Return:
 *    void
 */
void app_ble_start_connect_with_init_type(BLE_INIT_TYPE_E init_type,
                    ble_bdaddr_t *peer_addr, uint8_t owner_addr_type, uint16_t conn_to);

/*---------------------------------------------------------------------------
 *            app_ble_cancel_connecting
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    cancel connecting as connecting cost too long time.
 *
 * Parameters:
 *    NULL
 *
 * Return:
 *    void
 */
void app_ble_cancel_connecting(void);

/*---------------------------------------------------------------------------
 *            app_ble_is_connection_on
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is a specific BLE connection exist
 *
 * Parameters:
 *    index: Index of the BLE connection to check
 *
 * Return:
 *    true - BLE connection exists
 *    false - BLE connection doesn't exist
 */
bool app_ble_is_connection_on(uint8_t index);

/*---------------------------------------------------------------------------
 *            ble_execute_pending_op
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    execute pended BLE op
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void ble_execute_pending_op(BLE_ACTV_ACTION_E finish_action, uint8_t actv_idx);

/*---------------------------------------------------------------------------
 *            app_ble_is_any_connection_exist
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is there any BLE connection exist
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - at least one BLE connection exist
 *    false - no BLE connection exists
 */
bool app_ble_is_any_connection_exist();

/*---------------------------------------------------------------------------
 *            app_ble_start_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    disconnect the BLE connection with given connection index
 *
 * Parameters:
 *    conIdx: connection index to disconnect
 *
 * Return:
 *    void
 */
void app_ble_start_disconnect(uint8_t conIdx);

/*---------------------------------------------------------------------------
 *            app_ble_disconnect_all
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    disconnect all BLE connections
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_disconnect_all(void);

/*---------------------------------------------------------------------------
 *            app_ble_is_in_advertising_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is BLE in advertising progress
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - BLE adv is in progress
 *    false - BLE adv is not in progress
 */
bool app_ble_is_in_advertising_state(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_user_register
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get adv user register
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint32_t -- adv user register
 */
uint32_t app_ble_get_user_register(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_runtime_adv_param
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get current ble advertising parameters
 *
 * Parameters:
 *    pAdvType: Output pointer of adv type
 *    pAdvIntervalMs: Output pointer of adv internal in ms
 *
 * Return:
 *    void
 */
void app_ble_get_runtime_adv_param(uint8_t* pAdvType, uint32_t* pAdvIntervalMs);

/*---------------------------------------------------------------------------
 *            app_ble_refresh_irk
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to refresh ble identity key
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_refresh_irk(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_conhdl
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get ble connection handle by ble connection index
 *
 * Parameters:
 *    conidx: ble connection index
 *
 * Return:
 *    uint16_t: ble connection handle
 */
uint16_t app_ble_get_conhdl(uint8_t conidx);

/*---------------------------------------------------------------------------
 *            app_ble_get_conhdl
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get ble device name
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t *: ble device name
 */
uint8_t *app_ble_get_dev_name(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_mode_env
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get ble mode env
 *
 * Parameters:
 *    void
 *
 * Return:
 *    BLE_MODE_ENV_T *: ble mode env
 */
BLE_MODE_ENV_T *app_ble_get_mode_env(void);

/*---------------------------------------------------------------------------
 *           app_ble_get_user_activity_idx
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    Get the activity corresponding to the user
 *
 * Parameters:
 *    BLE_ADV_ACTIVITY_USER_E  activity user
 *
 * Return:
 *    uint8_t : activity idx
 */
uint8_t app_ble_get_user_activity_idx(BLE_ADV_ACTIVITY_USER_E actv_user);

/**
 * @brief Add all paired devices into white list
 *
 * @return uint8_t add success device num, if no record, return 0
 */
uint8_t app_ble_add_all_paired_remote_dev_into_white_list(void);

uint32_t ble_get_manufacture_data_ptr(uint8_t *advData,uint32_t dataLength,uint8_t *manufactureData);
bool app_ble_is_white_list_enable(void);
void app_ble_set_white_list(BLE_WHITE_LIST_USER_E user, ble_bdaddr_t *bdaddr, uint8_t size);
void app_ble_add_resloving_list(ble_gap_ral_dev_info_t *devicesInfoAddedToResolvingList, uint8_t numberOfDevicesAddedToResolvingList);
void app_ble_clear_white_list(BLE_WHITE_LIST_USER_E user);
void app_ble_clear_all_white_list(void);
bool app_ble_push_update_ci_list(uint8_t conidx, uint32_t min_interval,
    uint32_t max_interval, uint32_t supervision_timeout, uint8_t slaveLatency);

/**
 * @brief set ble connection req - cfm tx rx pref phy
 *
 * @param tx_pref_phy
 * @param rx_pref_phy
 */
void app_ble_set_tx_rx_pref_phy(uint32_t tx_pref_phy, uint32_t rx_pref_phy);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_BLE_MODE_SWITCH_H__ */


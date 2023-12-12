/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#ifndef APP_GFPS_H_
#define APP_GFPS_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief Device Information Application Module Entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW Configuration
#include "app_key.h"

#if (BLE_APP_GFPS)
#include <stdint.h>
#include "ble_gfps_common.h"
#include "gfps.h"
#ifdef SASS_ENABLED
#include "gfps_sass.h"
#endif
/*
 * DEFINES
 ****************************************************************************************
 */

// enable IS_USE_CUSTOM_FP_INFO if wanna use custom fastpair tx power, model id and anti-proof key
#define IS_USE_CUSTOM_FP_INFOx

/// Manufacturer Name Value
#define APP_GFPS_MANUFACTURER_NAME       ("RivieraWaves SAS")
#define APP_GFPS_MANUFACTURER_NAME_LEN   (16)

/// Model Number String Value
#define APP_GFPS_MODEL_NB_STR            ("RW-BLE-1.0")
#define APP_GFPS_MODEL_NB_STR_LEN        (10)

/// Serial Number
#define APP_GFPS_SERIAL_NB_STR           ("1.0.0.0-LE")
#define APP_GFPS_SERIAL_NB_STR_LEN       (10)

/// Firmware Revision
#define APP_GFPS_FIRM_REV_STR            ("6.1.2")
#define APP_GFPS_FIRM_REV_STR_LEN        (5)

/// System ID Value - LSB -> MSB
#define APP_GFPS_SYSTEM_ID               ("\x12\x34\x56\xFF\xFE\x9A\xBC\xDE")
#define APP_GFPS_SYSTEM_ID_LEN           (8)

/// Hardware Revision String
#define APP_GFPS_HARD_REV_STR           ("1.0.0")
#define APP_GFPS_HARD_REV_STR_LEN       (5)

/// Software Revision String
#define APP_GFPS_SW_REV_STR              ("6.3.0")
#define APP_GFPS_SW_REV_STR_LEN          (5)

/// IEEE
#define APP_GFPS_IEEE                    ("\xFF\xEE\xDD\xCC\xBB\xAA")
#define APP_GFPS_IEEE_LEN                (6)

/**
 * PNP ID Value - LSB -> MSB
 *      Vendor ID Source : 0x02 (USB Implementer’s Forum assigned Vendor ID value)
 *      Vendor ID : 0x045E      (Microsoft Corp)
 *      Product ID : 0x0040
 *      Product Version : 0x0300
 */
#define APP_GFPS_PNP_ID               ("\x02\x5E\x04\x40\x00\x00\x03")
#define APP_GFPS_PNP_ID_LEN           (7)

#define APP_GFPS_ADV_POWER_UUID             "\x02\x0a\xf5"
#define APP_GFPS_ADV_POWER_UUID_LEN         (3)
#define APP_GFPS_ADV_APPEARANCE_UUID        "\x03\x19\xda\x96"
#define APP_GFPS_ADV_APPEARANCE_UUID_LEN    (4)
#define APP_GFPS_ADV_MANU_SPE_UUID_TEST     "\x07\xFF\xe0\x00\x01\x5B\x32\x01"
#define APP_GFPS_ADV_MANU_SPE_UUID_LEN      (8)

#define APP_GFPS_RANDOM_RESOLVABLE_DATA_TYPE    (0x06)

#ifdef SASS_ENABLED
#ifdef SASS_SECURE_ENHACEMENT
#define APP_GFPS_ADV_VERSION                     (0x01)
#else
#define APP_GFPS_ADV_VERSION                     (0x00)
#endif
#define APP_GFPS_ADV_LEN_SALT                    (2)
#else
#define APP_GFPS_ADV_VERSION                     (0x00)
#define APP_GFPS_ADV_LEN_SALT                    (1)
#endif

#define APP_GFPS_ADV_FLAG                        (0x0)
#define APP_GFPS_ADV_TYPE_ACCKEY_FILETER_SHOW_UI (0x0)
#define APP_GFPS_ADV_TYPE_ACCKEY_FILETER_HIDE_UI (0x10)
#define APP_GFPS_ADV_TYPE_SALT                   (0x01)

#if (BLE_APP_HID)
#define APP_GFPS_FEATURES             (GFPSP_MANUFACTURER_NAME_CHAR_SUP |\
                                      GFPSP_MODEL_NB_STR_CHAR_SUP      |\
                                      GFPSP_SYSTEM_ID_CHAR_SUP         |\
                                      GFPSP_PNP_ID_CHAR_SUP)
#else
#define APP_GFPS_FEATURES             (GFPSP_ALL_FEAT_SUP)
#endif //(BLE_APP_HID)

#define GFPS_BATTERY_VALUE_MAX_COUNT    3

#ifdef SPOT_ENABLED
#define GFPS_NONCE_SIZE                 8
#define GFPS_AUTH_KEY_SIZE              8
#define GFPS_RECOVERY_KEY_SIZE          8
#define GFPS_RING_KEY_SIZE              8

#define GFPS_BEACON_READ_BEACON_PARAM                 0x00
#define GFPS_BEACON_READ_PROVISION_STATE              0x01
#define GFPS_BEACON_SET_EPHEMERAL_IDENTITY_KEY        0x02
#define GFPS_BEACON_CLEAR_EPHEMERAL_IDENTITY_KEY      0x03

#define GFPS_BEACON_READ_EPHEMERAL_IDENTITY_KEY       0x04
#define GFPS_BEACON_RING                              0x05
#define GFPS_BEACON_READ_RING_STATE                   0x06
#define GFPS_BEACON_ACTIVATE_UNWANTED_TRACK_MODE      0x07
#define GFPS_BEACON_DEACTIVATE_UNWANTED_TRACK_MODE    0x08

#define GFPS_BEACON_RINGING_STATE_STATED              0x00
#define GFPS_BEACON_RINGING_STATE_FAILED              0x01
#define GFPS_BEACON_RINGING_STATE_STOPPED_TIMEOUT     0x02
#define GFPS_BEACON_RINGING_STATE_STOPPED_PRESS       0x03
#define GFPS_BEACON_RINGING_STATE_STOPPED_REQUEST     0x04

#define GFPS_BEACON_RINGING_NONE                      0x00
#define GFPS_BEACON_RINGING_RIGHT                     0x01
#define GFPS_BEACON_RINGING_LEFT                      0x02
#define GFPS_BEACON_RINGING_RIGHT_AND_LEFT            0x03
#define GFPS_BEACON_RINGING_BOX                       0x04
#define GFPS_BEACON_RINGING_ALL                       0xFF

#define GFPS_BEACON_INCAPABLE_OF_RING                 0x00
#define GFPS_BEACON_ONE_CAPABLE_OF_RING               0x01
#define GFPS_BEACON_TWO_CAPABLE_OF_RING               0x02
#define GFPS_BEACON_THREE_CAPABLE_OF_RING             0x03
#define GFPS_BEACON_SECP_160R1_METHOD                 0x00
#define GFPS_BEACON_SECP_256R1_METHOD                 0x01

#define GFPS_BEACON_RINGING_VOLUME_NOT_AVAILABLE      0x00
#define GFPS_BEACON_RINGING_VOLUME_AVAILABLE          0x01

#define GFPS_BEACON_PROTOCOL_VERSION                  0x01

#define GFPS_BEACON_CONTROL_FLAG_SKIP_RING_AUT        0x01
#endif

struct gfps_ble_env_tag
{
    uint8_t connectionIndex;
    uint8_t isKeyBasedPairingNotificationEnabled;
    uint8_t isPassKeyNotificationEnabled;
    uint8_t isInitialPairing;
    bt_bdaddr_t seeker_bt_addr;
    bt_bdaddr_t local_le_addr;
    bt_bdaddr_t local_bt_addr;
    uint8_t keybase_pair_key[16];
    uint8_t aesKeyFromECDH[16];
    uint8_t isPendingForWritingNameReq;
    uint8_t advRandSalt;
    uint8_t isInFastPairing;
    uint8_t bondMode;
#ifdef SPOT_ENABLED
    uint8_t protocol_version;
    uint8_t nonce[8];
    uint8_t adv_identifer[20];
    uint8_t beacon_time[4];
    uint16_t remaining_ring_time;
    uint8_t control_flag;
    bool enable_unwanted_tracking_mode;
    uint8_t orignal_flag;
    uint8_t hashed_flag;
    uint8_t Ecc_private_key[20];
    bool ring_state;
#endif
    fp_event_cb cb;
};

typedef struct {
  uint32_t model_id;
  uint8_t public_anti_spoofing_key[64];
  uint8_t private_anti_spoofing_key[32];
} FastPairInfo;

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

/// Table of message handlers
extern const struct app_subtask_handlers app_gfps_table_handler;

/*
 * GLOBAL FUNCTIONS DECLARATION
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 ****************************************************************************************
 * @brief Initialize Device Information Service Application
 ****************************************************************************************
 */
void app_gfps_init(void);

/**
 ****************************************************************************************
 * @brief Add a Device Information Service instance in the DB
 ****************************************************************************************
 */
void app_gfps_add_gfps(void);

/**
 ****************************************************************************************
 * @brief Enable the Device Information Service
 ****************************************************************************************
 */
void app_gfps_enable_prf(uint16_t conhdl);

void app_gfps_connected_evt_handler(uint8_t conidx);
void app_gfps_disconnected_evt_handler(uint8_t conidx);

uint8_t app_gfps_generate_accountkey_data(uint8_t* outputData);

void app_gfps_update_random_salt(void);

void app_gfps_set_key_info(FastPairInfo fast_pair_info);

uint32_t app_bt_get_model_id(void);

void app_bt_get_fast_pair_info(void);

uint8_t *app_gfps_get_ble_addr();

void app_gfps_ble_register_callback(fp_event_cb callback);

uint8_t app_gfps_l2cap_send(uint8_t conidx, uint8_t *ptrData, uint32_t length);

void app_gfps_l2cap_disconnect(uint8_t conidx);

void app_gfps_send_keybase_pairing_via_notification(uint8_t *ptrData, uint32_t length);

uint8_t app_gfps_is_connected(uint8_t conidx);

#ifdef IBRT
void app_ibrt_share_fastpair_info(uint8_t *p_buff, uint16_t length);

void app_ibrt_shared_fastpair_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif


#ifdef SPOT_ENABLED
void app_gfps_generate_nonce(void);
uint8_t* app_gfps_get_nonce(void);
void app_spot_press_stop_ring_handle(APP_KEY_STATUS *status, void *param);
void app_gfps_set_protocol_version(void);
uint8_t app_gfps_get_protocol_version(void);
#endif


#ifdef __cplusplus
}
#endif

#endif //BLE_APP_GFPS

/// @} APP

#endif //APP_GFPS_H_

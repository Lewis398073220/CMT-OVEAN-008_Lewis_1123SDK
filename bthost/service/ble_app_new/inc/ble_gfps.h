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
#ifndef __BLE_GFPS_H__
#define __BLE_GFPS_H__
#include "gatt_service.h"
#ifdef __cplusplus
extern "C" {
#endif

#define BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL (160)
#define BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL (48)

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

#define IN_USE_ACCOUNT_KEY_HEADER           (0x06)
#define NONE_IN_USET_ACCOUNT_KEY_HEADER     (0x04)
#define MOST_RECENT_USED_ACCOUNT_KEY_HEADER (0x05)

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


#if (BLE_HID_ENABLE)
#define APP_GFPS_FEATURES             (GFPSP_MANUFACTURER_NAME_CHAR_SUP |\
                                      GFPSP_MODEL_NB_STR_CHAR_SUP      |\
                                      GFPSP_SYSTEM_ID_CHAR_SUP         |\
                                      GFPSP_PNP_ID_CHAR_SUP)
#else
#define APP_GFPS_FEATURES             (GFPSP_ALL_FEAT_SUP)
#endif //(BLE_HID_ENABLE)

typedef void (*gfps_enter_pairing_mode)(void);
typedef uint8_t (*gfps_bt_io_cap_set)(uint8_t mode);
typedef uint8_t (*gfps_bt_io_authrequirements_set)(uint8_t authrequirements);
typedef void (*gfps_get_battery_info_handler)(uint8_t* batteryValueCount, uint8_t* batteryValue);

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

typedef struct {
  uint32_t model_id;
  uint8_t public_anti_spoofing_key[64];
  uint8_t private_anti_spoofing_key[32];
} FastPairInfo;

#define USE_BLE_ADDR_AS_SALT 0
#define USE_RANDOM_NUM_AS_SALT 1
#define GFPS_ACCOUNTKEY_SALT_TYPE USE_BLE_ADDR_AS_SALT

#define FP_SERVICE_LEN          0x06
#define FP_SERVICE_UUID         0x2CFE
#define FP_DEVICE_MODEL_ID      0x2B677D

#ifdef SPOT_ENABLED
#define FP_SPOT_SERVICE_UUID                            0xAAFE
#define FP_EID_FRAME_TYPE_WHEN_ENABLE_UTP               0x41
#define FP_EID_FRAME_TYPE_WHEN_DISABLE_UTP              0x40
#define FP_SPOT_SERVICE_LEN                             0x18
#endif


#define RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_EN           (1)
#define RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_DIS          (0)
#define RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_EN     (1)
#define RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_DIS    (0)

#define GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITH_PUBLIC_KEY     (80)
#define GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITHOUT_PUBLIC_KEY  (16)

#define GFPSP_IDX_MAX        (BLE_CONNECTION_MAX)

#define GFPSP_ENCRYPTED_RSP_LEN     16

enum {
    GFPS_SUCCESS,
    GFPS_ERROR_EXEC_FAIL,
    GFPS_ERROR_NULL,
    GFPS_ERROR_INVALID_PARAM,
    GFPS_ERROR_DATA_SIZE,

    GFPS_ERROR_UNAUTHENTICATED = 0x80,
    GFPS_ERROR_INVALID_VALUE = 0x81,
    GFPS_ERROR_NO_USER_CONSENT = 0x82
};

#define Gfps_CheckParm(code, exp) if (!(exp)) return (code)

#ifdef SPOT_ENABLED
typedef struct _gfpsp_reading_beacon_additional_data{
    int8_t power_value;
    uint8_t clock_value[4];
    uint8_t SECP_method;
    uint8_t numbesr_of_ringing;
    uint8_t ringing_capability;
    uint8_t padgding[8];
}gfpsp_reading_beacon_additional_data;
typedef struct _gfpsp_reading_beacon_state_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t additional_data[16];
}gfpsp_reading_beacon_state_resp;

typedef struct _gfpsp_reading_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data;
}gfpsp_reading_beacon_provision_resp;

typedef struct _gfpsp_reading_EIK_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data;
    uint8_t EIK[20];
}gfpsp_reading_EIK_beacon_provision_resp;

typedef struct _gfpsp_set_EIK_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_reading_set_beacon_provision_resp;

typedef struct _gfpsp_clearing_EIK_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_clearing_EIK_beacon_provision_resp;

typedef struct _gfpsp_reading_beacon_identity_key_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data[32];
}gfpsp_reading_beacon_identity_key_resp;

typedef struct _gfpsp_beacon_ring_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data[4];
}gfpsp_beacon_ring_resp;

typedef struct _gfpsp_beacon_read_ring_state_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data[3];
}gfpsp_reading_beacon_ring_state_resp;

typedef struct _gfpsp_beacon_activate_unwanted_tracking_mode_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_beacon_activate_unwanted_tracking_mode_resp;

typedef struct _gfpsp_beacon_deactivate_unwanted_tracking_mode_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_beacon_deactivate_unwanted_tracking_mode_resp;
#endif

void gfps_crypto_init(void);
uint32_t gfps_crypto_get_secret_decrypt(const uint8_t* in_encryptdata ,const uint8_t *in_public_key,uint8_t * out_key,uint8_t *out_decryptdata );
uint32_t gfps_crypto_encrypt(const uint8_t *in_data,uint8_t len,const uint8_t *AESkey,uint8_t *out_encrypt);
uint32_t gfps_crypto_decrypt(const uint8_t *in_data,uint8_t len,const uint8_t *AESkey,uint8_t *out_encrypt);
uint32_t gfps_crypto_gen_DHKey(const uint8_t *in_PubKey,const uint8_t *in_PrivateKey,uint8_t *out_DHKey);
uint32_t gfps_crypto_make_P256_key(uint8_t * out_public_key,uint8_t * out_private_key);
uint32_t gfps_crypto_set_p256_key(const uint8_t* in_public_key,const uint8_t* in_private_key);
uint32_t gfps_SHA256_hash(const void* in_data, int len, void* out_data);
void gfps_encrypt_name(uint8_t* aesKey, uint8_t* inputRawName, uint32_t inputLen, 
    uint8_t* outputEncryptedName, uint8_t* hmacFirst8Bytes, uint8_t* nonce);
bool gfps_decrypt_name(uint8_t* aesKey, uint8_t* hmacFirst8Bytes, 
    uint8_t* nonce, uint8_t* encryptedName, uint8_t* rawName, uint32_t nameLen);
int rand_generator(uint8_t *dest, unsigned size);
void gfps_encrypt_messasge(uint8_t* accKey, uint8_t* nonce,
    uint8_t* inputData, uint32_t inputDataLen, uint8_t* outputHmacFirst8Bytes);
void gfps_hdkf(uint8_t *salt, uint8_t saltLen, uint8_t* ikm, uint8_t ikmLen,
    uint8_t*inputData, uint32_t inputDataLen, uint8_t* output, uint8_t outputLen);
void gfps_beacon_encrpt_data(uint8_t* accKey,
    uint8_t* inputData, uint32_t inputDataLen, uint8_t* outputHmacFirst8Bytes);

#ifdef __cplusplus
    }
#endif

#endif /* __BLE_GFPS_H__ */

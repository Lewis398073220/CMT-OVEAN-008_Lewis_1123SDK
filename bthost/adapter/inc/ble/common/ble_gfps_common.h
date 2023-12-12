/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#ifndef __BLE_GFPS_COMMON_H__
#define __BLE_GFPS_COMMON_H__
#include "ble_common_define.h"
#ifdef BLE_HOST_SUPPORT
#ifdef GFPS_ENABLED
#ifdef __cplusplus
extern "C" {
#endif

/*
 * DEFINES
 ****************************************************************************************
 */
#define RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_EN           (1)
#define RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_DIS          (0)
#define RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_EN     (1)
#define RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_DIS    (0)

#define GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITH_PUBLIC_KEY     (80)
#define GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITHOUT_PUBLIC_KEY  (16)

#define BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL (160)
#define BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL (48)
#define BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL (1000)

#define GFPSP_IDX_MAX        (BLE_CONNECTION_MAX)

#define GFPSP_ENCRYPTED_RSP_LEN     16

#define L2CAP_SPSM_GFPS             (0x0041)

#define STATE_READY_CONNECT             (1)
#define STATE_NOT_READY_CONNECT         (0)

#define KEY_BASE_RSP_LEN                (16)
#define KEY_BASE_RSP_SALT_LEN           (9)
#define KEY_BASE_EXT_RSP_SALT_LEN       (7)

#define IN_USE_ACCOUNT_KEY_HEADER           (0x06)
#define NONE_IN_USET_ACCOUNT_KEY_HEADER     (0x04)
#define MOST_RECENT_USED_ACCOUNT_KEY_HEADER (0x05)

#define GFPS_BOND_OVER_BLE                       (1)
#define GFPS_BOND_OVER_BT                        (2)

#define FP_MSG_MAX_LEN     (16)

typedef void (*gfps_get_battery_info_handler)(uint8_t* batteryValueCount, uint8_t* batteryValue);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the GFPSP task
enum
{
    /// Idle state
    GFPSP_IDLE,
    /// Busy state
    GFPSP_BUSY,
    /// Number of defined states.
    GFPSP_STATE_MAX
};

#define BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL (160)
#define BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL (48)

#define APP_GFPS_RANDOM_RESOLVABLE_DATA_TYPE    (0x06)

typedef enum
{
    BATTERY_NOT_CHARGING = 0,
    BATTERY_CHARGING = 1,
} GFPS_BATTERY_STATUS_E;

typedef enum
{
    HIDE_SUBSEQUENT_INDICATION = 2,
    SHOW_UI_INDICATION = 3,
    HIDE_UI_INDICATION = 4
} GFPS_BATTERY_DATA_TYPE_E;

typedef enum
{
    KEY_BASED_PAIRING_REQ = 0x00,
    KEY_BASED_PAIRING_RSP = 0x01,
    KEY_BASED_PAIRING_EXT_RSP = 0x02,    
    ACTION_REQUEST        = 0x10,
} GFPS_MESSAGE_TYPE_E; 

typedef struct _gfpsp_encrypted_req_uint128{
    uint8_t uint128_array[16];
}gfpsp_encrypted_req_uint128;

typedef struct _gfpsp_64B_public_key_{
    uint8_t public_key_64B[64];
}gfpsp_64B_public_key;

typedef struct _gfpsp_Key_based_Pairing_req{
    gfpsp_encrypted_req_uint128 * en_req;
    gfpsp_64B_public_key        * pub_key;
}gfpsp_Key_based_Pairing_req;

typedef struct {
    uint8_t message_type;
    uint8_t reserved[15];
} gfpsp_raw_req_t;

typedef struct {
    uint8_t message_type;   // KEY_BASED_PAIRING_REQ
    uint8_t flags_reserved          :   2;
    uint8_t flags_support_le_audio  :   1;   
    uint8_t flags_support_le        :   1;
    uint8_t flags_retroactively_write_account_key   :   1;
    uint8_t flags_get_existing_name :   1;
    uint8_t flags_bonding_addr      :   1;    
    uint8_t flags_discoverability   :   1; 

    uint8_t provider_addr[6];
    uint8_t seeker_addr[6];
    uint8_t reserved[2];
} gfpsp_key_based_pairing_req_t;

typedef struct {
    uint8_t message_type;   // ACTION_REQUEST
    uint8_t flags_reserved          :   6;    
    uint8_t isFollowedByAdditionalDataCh   :   1; 
    uint8_t isDeviceAction          :   1;       

    uint8_t provider_addr[6];   
    uint8_t messageGroup;           // Mandatory if Flags Bit 1 is set.
    uint8_t messageCode;            // Mandatory if Flags Bit 1 is set.
    uint8_t additionalDataLen;      // Mandatory if Flags Bit 1 is set. Less than 6
    uint8_t additionalData[6];      // Mandatory if Flags Bit 1 is set.
} gfpsp_action_req_t;

typedef struct {
    uint8_t message_type; // KEY_BASED_PAIRING_EXT_RSP
    uint8_t flags_reserved        :  5;
    uint8_t flags_msg_via_le      :  1;
    uint8_t flags_ble_bonding     :  1;
    uint8_t flags_is_le_only      :  1;

    uint8_t addrNum;
    uint8_t local_addr[6];
    uint8_t peerAddrandSalt[KEY_BASE_EXT_RSP_SALT_LEN];
}gfpsp_raw_ext_resp;

typedef struct {
    uint8_t message_type; // KEY_BASED_PAIRING_RSP
    uint8_t provider_addr[6];
    uint8_t salt[KEY_BASE_RSP_SALT_LEN];
}gfpsp_raw_resp;

typedef struct _gfpsp_raw_passkey_resp{
    uint8_t message_type;
    uint8_t passkey[3];
    uint8_t reserved[12];
}gfpsp_raw_pass_key_resp;

typedef struct _gfpsp_encrypted_resp{
    uint8_t uint128_array[GFPSP_ENCRYPTED_RSP_LEN];
}gfpsp_encrypted_resp;

typedef struct _gfpsp_req_resp{
    union{
        gfpsp_raw_req_t                 raw_req;
        gfpsp_encrypted_req_uint128     en_req;
        gfpsp_key_based_pairing_req_t   key_based_pairing_req;     
        gfpsp_action_req_t              action_req;
        gfpsp_raw_resp                  key_based_pairing_rsp;
        gfpsp_encrypted_resp            en_rsp;
    }rx_tx;
}gfpsp_req_resp;

typedef enum
{
    FP_SRV_EVENT_CONNECTED = 0,
    FP_SRV_EVENT_DISCONNECTED,
    FP_SRV_EVENT_DATA_IND,
    FP_SRV_EVENT_SENT_DONE,
}GFPS_SRV_EVENT_TYPE_T;

typedef struct
{
    uint8_t *pBuf;
    uint16_t len;
} GFPS_PACKET_T;

typedef enum  {
    GFPS_EVENT_CONNECTION,
    GFPS_EVENT_PROFILE,
    GFPS_EVENT_FROM_SEEKER,
} GFPS_EVENT_TYPE_E;

typedef enum  {
    GFPS_EVENT_LINK_CONNECTED,
    GFPS_EVENT_LINK_DISCONNECTED,
    GFPS_EVENT_LINK_DESTORY,
    GFPS_EVENT_INVALID,
} GFPS_EVENT_CONNECTION_TYPE_E;

typedef enum
{
    GFPS_PROFILE_A2DP = 1,
    GFPS_PROFILE_AVRCP,
    GFPS_PROFILE_HFP,
    GFPS_PROFILE_INVALID = 0xFF,
} GFPS_PROFILE_ID_E;

typedef struct
{
    uint8_t event;
    union{
        bt_bdaddr_t      addr;
        GFPS_PACKET_T    data;
    }p;
}GFPS_SRV_EVENT_PARAM_T;

typedef struct {
    GFPS_EVENT_CONNECTION_TYPE_E     event;
    uint8_t                          reason;
    bt_bdaddr_t                      addr;
} GFPS_CONNECTION_EVENT;

typedef struct {
    GFPS_PROFILE_ID_E     pro;
    bt_bdaddr_t           btAddr;
    uint8_t               btEvt;
    uint16_t              len;
    uint8_t               param[FP_MSG_MAX_LEN];
} GFPS_SASS_PROFILE_EVENT;

typedef struct {
    GFPS_EVENT_CONNECTION_TYPE_E     event;
    uint8_t                          reason;
    bt_bdaddr_t                      addr;
} GFPS_SRV_EVENT;

typedef struct {
    uint8_t     devId;
    uint8_t     event;
    uint16_t    len;
    union
    {
        GFPS_SASS_PROFILE_EVENT proParam;
        GFPS_CONNECTION_EVENT   conParam;
        GFPS_SRV_EVENT_PARAM_T  srvParam;
    } p;
} GFPS_MESSAGE_BLOCK;

typedef struct {
    gfpsp_encrypted_resp enc_resp;
    void *priv;
} gfps_last_response_t;

typedef uint16_t (*fp_event_cb)(uint8_t event, GFPS_SRV_EVENT_PARAM_T *param);
typedef void (*gfps_get_battery_info_cb)(uint8_t* batteryValueCount, uint8_t* batteryValue);

bool gfps_is_last_response_pending(void);
void gfps_enter_connectable_mode_req_handler(gfps_last_response_t *response);
bool gfps_is_in_fastpairing_mode(void);
uint8_t gfps_get_bt_iocap(void);
uint8_t gfps_get_bt_auth(void);
GFPS_BATTERY_DATA_TYPE_E gfps_get_battery_datatype(void);
void gfps_get_battery_levels(uint8_t *pCount, uint8_t *pBatteryLevel);
bool gfps_is_battery_enabled(void);

void gfps_crypto_init(void);
void big_little_switch(const uint8_t *in, uint8_t *out, uint8_t len);

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

#ifdef SASS_ENABLED

#define NTF_CAP_LEN          (4)
#define SASS_ADV_LEN_MAX     (4)

#define SASS_SECURE_ENHACEMENT

uint8_t gfps_sass_get_active_dev();
void gfps_sass_set_inuse_acckey(uint8_t *accKey, bt_bdaddr_t *addr);
bool gfps_sass_get_inuse_acckey(uint8_t *accKey);
bool gfps_sass_is_sass_dev(uint8_t device_id);
bool gfps_sass_is_there_in_use_dev();
bool gfps_sass_is_there_sass_dev();

#ifdef SASS_SECURE_ENHACEMENT
void gfps_sass_encrypt_connection_state(uint8_t *iv,  uint8_t *inUseKey, 
                                               uint8_t *outputData, uint8_t *dataLen, bool LT,
                                               bool isUseAdv, uint8_t *inputData);
void gfps_sass_encrypt_adv_data(uint8_t *iv,  uint8_t *inUseKey, uint8_t *outputData, uint8_t *dataLen, bool LT);
#else
void gfps_sass_encrypt_adv_data(uint8_t *FArray, uint8_t sizeOfFilter, uint8_t *inUseKey, uint8_t *outputData, uint8_t *dataLen);
#endif

#endif /* SASS_ENABLED */

#ifdef __cplusplus
}
#endif
#endif /* GFPS_ENABLED */
#endif /* BLE_HOST_SUPPORT */
#endif /* __BLE_GFPS_COMMON_H__ */

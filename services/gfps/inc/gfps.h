#ifndef __GFPS_H__
#define __GFPS_H__
#ifdef GFPS_ENABLED
#include "bluetooth_ble_api.h"

#define FP_TWS_MAX_LEN              (APP_TWS_CTRL_BUFFER_MAX_LEN)

#define FP_MESSAGE_RESERVED_LEN     (4)   // at least 4 bytes
// TODO: increase this value if needed
#define FP_MESSAGE_STREAM_MAX_ADDITIONAL_DATA_LEN   (64)
#define FP_MSG_MAX_LEN     (16)

#define IS_BT_DEVICE(id)  (id & 0x80)
#define GET_BT_ID(id)     (id & 0x7F)
#define SET_BT_ID(id)     (id | 0x80)

// The values is posted at FP spec 8/27/19 revision
#define FP_MSG_GROUP_BLUETOOTH_EVENT                0x01
#define FP_MSG_BT_EVENT_ENABLE_SILENCE_MODE         0x01
#define FP_MSG_BT_EVENT_DISABLE_SILENCE_MODE        0x02

#define FP_MSG_GROUP_COMPANION_APP_EVENT            0x02
#define FP_MSG_COMPANION_APP_LOG_BUF_FULL           0x01

#define FP_MSG_GROUP_DEVICE_INFO                    0x03
#define FP_MSG_DEVICE_INFO_MODEL_ID                 0x01
#define FP_MSG_DEVICE_INFO_BLE_ADD_UPDATED          0x02
#define FP_MSG_DEVICE_INFO_BATTERY_UPDATED          0x03
#define FP_MSG_DEVICE_INFO_REMAINING_BATTERY_TIME   0x04
#define FP_MSG_DEVICE_INFO_ACTIVE_COMPONENTS_REQ    0x05
#define FP_MSG_DEVICE_INFO_ACTIVE_COMPONENTS_RSP    0x06
#define FP_MSG_DEVICE_INFO_TELL_CAPABILITIES        0x07
#define FP_MSG_DEVICE_INFO_PLATFORM_TYPE            0x08
#define FP_MSG_DEVICE_INFO_FIRMWARE_VERSION         0x09
#define FP_MSG_DEVICE_INFO_SESSION_NONCE            0x0A
#define FP_MSG_DEVICE_INFO_EDD_IDENTIFIER           0x0B

#define FP_MSG_GROUP_DEVICE_ACTION                  0x04
#define FP_MSG_DEVICE_ACTION_RING                   0x01

#define FP_MSG_NEITHER_BUD_ACTIVE                   0x00
#define FP_MSG_RIGHT_BUD_ACTIVE                     0x01
#define FP_MSG_LEFT_BUD_ACTIVE                      0x02
#define FP_MSG_BOTH_BUDS_ACTIVE                     0x03

#define FP_MSG_GROUP_SASS                           0x07
#define FP_MSG_SASS_GET_CAPBILITY                   0x10
#define FP_MSG_SASS_NTF_CAPBILITY                   0x11
#define FP_MSG_SASS_SET_MULTIPOINT_STATE            0x12
#define FP_MSG_SASS_SET_SWITCH_PREFERENCE           0x20
#define FP_MSG_SASS_GET_SWITCH_PREFERENCE           0x21
#define FP_MSG_SASS_NTF_SWITCH_PREFERENCE           0x22
#define FP_MSG_SASS_SWITCH_ACTIVE_SOURCE            0x30
#define FP_MSG_SASS_SWITCH_BACK                     0x31
#define FP_MSG_SASS_NTF_SWITCH_EVT                  0x32
#define FP_MSG_SASS_GET_CONN_STATUS                 0x33
#define FP_MSG_SASS_NTF_CONN_STATUS                 0x34
#define FP_MSG_SASS_NTF_INIT_CONN                   0x40
#define FP_MSG_SASS_IND_INUSE_ACCOUNT_KEY           0x41
#define FP_MSG_SASS_SEND_CUSTOM_DATA                0x42
#define FP_MSG_SASS_SET_DROP_TGT                    0x43

#define FP_MSG_GROUP_ACKNOWLEDGEMENT                0xFF
#define FP_MSG_ACK                                  0x01
#define FP_MSG_NAK                                  0x02

#define FP_MSG_NAK_REASON_NOT_SUPPORTED             0x00
#define FP_MSG_NAK_REASON_DEVICE_BUSY               0x01
#define FP_MSG_NAK_REASON_NOT_ALLOWED               0x02
#define FP_MSG_NAK_REASON_REDUNDANT_ACTION          0x04

#define FP_MSG_GROUP_DEVICE_CAPABLITY_SYNC          0x06
#define FP_MSG_DEVICE_CAPABLITY_CAP_UPDATE_REQ      0x01
#define FP_MSG_DEVICE_CAPABLITY_DYN_BUF_SIZE        0x02
#define FP_MSG_DEVICE_CAPABLITY_ENABLE_EDD_TRACK    0x03

#define FIND_MY_BUDS_STATUS_SLAVE_MASK              0x01
#define FIND_MY_BUDS_STATUS_MASTER_MASK             0x02

#define GFPS_FIND_MY_BUDS_CMD_STOP                  0x00
#define GFPS_FIND_MY_BUDS_CMD_START                 0x01

#define GFPS_FIND_MY_BUDS_CMD_STOP_DUAL             0x00
#define GFPS_FIND_MY_BUDS_CMD_START_MASTER_ONLY     0x01
#define GFPS_FIND_MY_BUDS_CMD_START_SLAVE_ONLY      0x02
#define GFPS_FIND_MY_BUDS_CMD_START_DUAL            0x03

typedef union
{
    struct
    {
        uint8_t     isCompanionAppInstalled :   1;
        uint8_t     isSilentModeSupported   :   1;
        uint8_t     reserve                 :   6;
    } env;
    uint8_t content;
} FpCapabilitiesEnv_t;

typedef struct
{
    uint8_t isRightRinging  :   1;
    uint8_t isLeftRinging   :   1;
    uint8_t reserve         :   6;
} FpRingStatus_t;

typedef struct
{
    uint8_t   devId;
    bool      isRfcomm;
    bool      isBtBond;
    uint8_t   reserved;
    bt_bdaddr_t addr;
}FpBtInfo_t;

typedef void (*gfps_enter_pairing_mode_cb)(void);
typedef uint8_t (*gfps_bt_io_cap_set_cb)(uint8_t mode);
typedef uint8_t (*gfps_bt_io_authrequirements_set_cb)(uint8_t authrequirements);

typedef struct
{
    FpBtInfo_t btInfo[BT_DEVICE_NUM];
    uint8_t    isBatteryInfoIncluded;
    uint8_t    bt_iocap;
    uint8_t    bt_authrequirements;
    uint8_t    isLastResponsePending;
    uint8_t    pendingLastResponse[16];
    bool       isFastPairMode;
    FpCapabilitiesEnv_t                fpCap;
    gfps_enter_pairing_mode_cb         enterPairingMode;
    gfps_bt_io_cap_set_cb              btSetIocap;
    gfps_bt_io_authrequirements_set_cb btSetAuthrequirements;
    gfps_get_battery_info_cb           getBatteryHandler;
    GFPS_BATTERY_DATA_TYPE_E           batteryDataType;
} GFPSEnv_t;

typedef struct
{
    uint8_t messageGroup;
    uint8_t messageCode;
    uint8_t dataLenHighByte;
    uint8_t dataLenLowByte;
    uint8_t data[FP_MESSAGE_STREAM_MAX_ADDITIONAL_DATA_LEN];
}  __attribute__((packed)) FP_MESSAGE_STREAM_T;


#ifdef __cplusplus
extern "C" {
#endif

void gfps_init(void);

void gfps_disconnect(uint8_t devId);

void gfps_link_connect_handler(uint8_t devId, const bt_bdaddr_t *addr);

void gfps_link_disconnect_handler(uint8_t devId, const bt_bdaddr_t *addr, uint8_t errCode);

void gfps_link_destory_handler(void);

void gfps_role_switch_prepare(void);

void gfps_sync_info(void);

void gfps_send_battery_levels(uint8_t devId);

void gfps_send_ble_addr(uint8_t devId);

void gfps_set_multi_status(const bt_bdaddr_t *addr, bool enable_multipoint);

void gfps_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void gfps_set_battery_datatype(GFPS_BATTERY_DATA_TYPE_E batteryDataType);

void gfps_reg_battery_handler(gfps_get_battery_info_cb cb);

void gfps_set_bt_iocap(uint8_t ioCap);

void gfps_set_bt_auth(uint8_t auth);

uint8_t gfps_send(uint8_t devId, uint8_t *ptrData, uint32_t length);

void gfps_send_msg_ack(uint8_t devId, uint8_t msgGroup, uint8_t msgCode);

void gfps_send_msg_nak(uint8_t devId, uint8_t reason, uint8_t msgGroup, uint8_t msgCode);

void gfps_ntf_ble_bond_over_bt(uint8_t devId, bool bond);

void gfps_enter_fastpairing_mode(void);

void gfps_set_in_fastpairing_mode_flag(bool isEnabled);

void gfps_exit_fastpairing_mode(void);

int gfps_mailbox_put(uint8_t devId, uint8_t event, uint8_t *param, uint16_t len);

uint16_t gfps_data_handler(uint8_t devId, uint8_t* ptr, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif

#endif  // __GFPS_H__
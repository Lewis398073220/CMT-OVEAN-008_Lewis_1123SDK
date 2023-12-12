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
#ifndef __USB_HID_APP_H__
#define __USB_HID_APP_H__

#include "hal_trace.h"
#include "hal_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t __usb_hid_handler_table_start[];
extern uint32_t __usb_hid_handler_table_end[];
#define HID_ASP_VERSION_POSITIVE                1
#define HID_ASP_VERSION_NEGATIVE                1

#define HID_REPORT_ID_REQ                       0x24
#define HID_REPORT_ID_RSP                       0x27
#define HID_REPORT_RSP_PACKET_SIZE              0x3D

#define HID_REPORT_CONTAINER_ID_LEN             0x10
#define HID_REPORT_COLOR_NAME_LEN               0x10
#define HID_REPORT_SERIAL_NUMBER_LEN            0x14
#define HID_REPORT_DEVICE_CAPABILITY_LEN        0x10

#define HID_DEFAULT_MTE_MODE_ENABLE             0
#define HID_DEFAULT_TONE_STATUS                 0x01
#define HID_DEFAULT_TONE_VOLUME                 0x32
#define HID_DEFAULT_PTT_MODE                    0x01
#define HID_DEFAULT_KID_MODE                    0x00
#define HID_DEFAULT_KID_MODE_MAX_VOLUME         0x50
#define HID_DEFAULT_SERIAL_NUMBER               "0000000000000"
#define HID_DEFAULT_COLOR_NAME                  "Black"
#define HID_DEFAULT_SCHEME_CODE                 0x00
#define HID_DEFAULT_PRODUCT_BRAND               0x00
#define HID_DEFAULT_DATE_YEAR                   0x00
#define HID_DEFAULT_DATE_MONTH                  0x00
#define HID_DEFAULT_DATE_DAY                    0x00
#define HID_DEFAULT_HW_MINOR_VERSION            0x00
#define HID_DEFAULT_HW_MAJOR_VERSION            0x00
#define HID_DEFAULT_BUILD_STAGES                0x00

#ifdef USB_AUDIO_CUSTOM_ASP
#define HID_REPORT_ID_ASP                       0x9A
#define HID_REPORT_ASP_PACKET_SIZE              0x3F
#define HID_REPORT_ASP_LOCALE_SIZE              0X20
#define HID_REPORT_ASP_CONTEXT_T_SIZE           0x3F
#define HID_REPORT_ASP_TELEMETRY_STRING_SIZE    0X20

typedef enum{
    NT_NoCurrentScheduled = 0x00,
    NT_MissIncomingCall = 0x01,
    NT_UpcomingScheduledMeeting = 0x02,
    NT_UncheckedVoiceMail = 0x04,
    NT_Telemetry = 0x20,
    NT_MAX,
}APP_USB_ASP_CMD_NOTIFICATION_EVENT_E;

typedef enum{
    CT_TeamsButtonInvoked = 0x00,
    CT_TeamsButtonCallTab = 0x01,
    CT_TeamsButtonScheduledMeeting = 0x02,
    CT_TeamsButtonVoiceMail = 0x04,
    CT_Telemetry = 0x20,
    CT_MAX,
}APP_USB_ASP_CMD_CONTEXT_EVENT_E;

typedef enum{
    ASP_MESSAGE_VERSION = 0x00,
    ASP_MESSAGE_ENUM = 0x01,
    ASP_MESSAGE_LOCALE = 0x02,
    ASP_MESSAGE_NOTIFICATION = 0x03,
    ASP_MESSAGE_CONTEXT = 0x04,
    ASP_MESSAGE_CALL_EVENT = 0x05,
    ASP_MESSAGE_TELEMETRY = 0x06,
    APP_USB_HID_ASP_MAX,
}APP_USB_HID_ASP_MESSADE_CMD_E;

typedef enum{
    TM_CurrentFirmwareEndpoint = 0x01,
    TM_CurrentFirmwareDongle = 0x02,
    TM_DonToAnswerSetting = 0x03,
    TM_EndpointDeviceModelId = 0x04,
    TM_DeviceSerialNumberEndpoint = 0x06,
    TM_DeviceSerialNumberDongle = 0x07,
    TM_UserModifiedSideToneLevel = 0x08,
    TM_AudioCodecUsed = 0x09,
    TM_DspEffectsEnableByHeadset = 0x0A,
    TM_Hardmute = 0x0B,
    TM_HeadsetWorn = 0x0C,
    TM_BatteryLevel = 0x0D,
    TM_DeviceReady = 0x0E,
    TM_RadioLinkQuality = 0x0F,
    TM_ErrorMessage = 0x10,
    TM_ButtonPressInfo = 0x11,
    TM_ConnectedWirelessDeviceChange = 0x12,
    TM_PeopleCount = 0x13,
    TM_LocalConferenceCount = 0x14,
    TM_DeviceMicrophoneStatus = 0x15,
    TM_DeviceSpeakerStatus = 0x16,
    TM_DeviceAudioStreamsStatus = 0x17,
    TM_DeviceVideoAndMetadataStreamsStatus = 0x18,
    TM_AiModelNameAndVersion = 0x19,
    TM_DeviceResourceUsage = 0x20,
    TM_VoiceActivity = 0x21,
    TM_MAX,
}APP_USB_HID_ASP_TELEMETRY_EVENT_E;

typedef struct{
    uint8_t metadata_flags : 4;
    uint8_t message_id : 4;
}APP_USB_HID_MESSAGE_PAYLOAD_T;

typedef struct{
    uint8_t report_id;
    APP_USB_HID_MESSAGE_PAYLOAD_T message_data;
    uint8_t primary_payload_len_high_byte;
    uint8_t primary_payload_len_low_byte;
    uint8_t primary_payload_data[HID_REPORT_ASP_PACKET_SIZE - 4];
}APP_USB_HID_ASP_CMD_PAYLOAD_T;

typedef struct{
    uint8_t locale_str[HID_REPORT_ASP_LOCALE_SIZE];
}APP_USB_HID_ASP_LOCALE_PARAM_T;

typedef struct{
    APP_USB_ASP_CMD_CONTEXT_EVENT_E report_primary_event;
    uint8_t telemetry_event_buf_len;
    uint8_t telemetry_event_buf[HID_REPORT_ASP_CONTEXT_T_SIZE];
}APP_USB_HID_ASP_CONTEXT_PARAM_T;

typedef struct{
    uint8_t cur_firmware_endpoint_buf[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t cur_firmware_dongle_buf[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t no_answersetting;
    uint8_t endpoint_device_model_id[HID_REPORT_ASP_TELEMETRY_STRING_SIZE + 0x10];
    uint8_t side_tone_level[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t audio_codec_used;
    uint32_t dsp_effect_enable;
    uint8_t hardmute;
    uint8_t headset_worn;
    uint8_t battery_level;
    uint8_t device_ready;
    uint8_t radio_link_quality;
    uint8_t error_message[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t button_press_info[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t button_press_info_report_len;
    uint8_t wirless_device_conn_status;
    uint8_t people_count;
    uint32_t local_conference_count;
    uint8_t microphone_device_status[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t spk_device_status[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t audio_stream_status[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t video_device_metadata_stream_status[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t ai_model_name_version[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t device_resource_usage[HID_REPORT_ASP_TELEMETRY_STRING_SIZE];
    uint8_t voice_activity;
}APP_USB_HID_ASP_TELEMETRY_PARAM_T;

typedef struct{
    APP_USB_HID_ASP_LOCALE_PARAM_T locale;
    APP_USB_HID_ASP_CONTEXT_PARAM_T context;
    APP_USB_HID_ASP_TELEMETRY_EVENT_E cur_telemetry_event;
    APP_USB_ASP_CMD_NOTIFICATION_EVENT_E notification_event;
    APP_USB_HID_ASP_TELEMETRY_PARAM_T telemetry_event;
}APP_USB_HID_ASP_PARAM_T;

uint8_t usb_hid_asp_device_conn_status_get(void);

void usb_hid_asp_notification_event_set(APP_USB_ASP_CMD_NOTIFICATION_EVENT_E e);

APP_USB_ASP_CMD_NOTIFICATION_EVENT_E usb_hid_asp_notification_event_get(void);

void usb_hid_asp_report_start(uint32_t ms);

void usb_hid_asp_report_button_press_info(uint8_t hook_onoff, uint8_t mute_onoff);

void usb_hid_asp_message_context_report(APP_USB_ASP_CMD_CONTEXT_EVENT_E event, uint8_t *telemetry_event_buf, uint8_t telemetry_event_buf_len, bool report_onoff);
#endif

typedef enum
{
    OP_GetDeviceCapability = 0x02,
    OP_GetSerialNumber = 0x18,
    OP_GetContainerId = 0x19,
    OP_GetColorName = 0x1C,
    OP_ResetUserConfiguration = 0x2E,
    OP_GetFirmwareVersion = 0x2F,
    OP_SetCurrentEQ = 0x32,
    OP_GetCurrentEQ = 0x33,
    OP_SetTonesStatus = 0x34,
    OP_GetTonesStatus = 0x35,
    OP_SetTonesVolume = 0x36,
    OP_GetTonesVolume = 0x37,
    OP_SetKidMode = 0x38,
    OP_GetKidMode = 0x39,
    OP_SetEnableMute = 0x3A,
    OP_GetEnableMute = 0x3B,
    OP_SetTestMode = 0x80,
    OP_GetTestMode = 0x81,
    OP_SetDeviceCapability = 0x82,
    OP_SetSerialNumber = 0x83,
    OP_SetLedStatus = 0x87,
    OP_SetContainerID = 0x89,
    OP_DeviceReboot = 0x8A,
    OP_FactoryReset = 0x8B,
    OP_SetColorName = 0x91,
    OP_EnableMicEQ = 0x92,
    OP_EnableSpeakerEQ = 0x93,
    OP_EnableMic = 0x95,
    OP_GetComponentStatus = 0x96,
    /* commond count */
    OP_USB_HID_COMMAND_COUNT,
} APP_USB_HID_CMD_CODE_E;

typedef enum
{
    STATUS_OK = 0,
    STATUS_INVALID_PARAMETER = 2,
    STATUS_FAIL = 3,
} APP_USB_HID_CMD_RET_STATUS_E;

enum USB_AUDIO_HID_EPINT_OUT_ID_T {
    USB_AUDIO_HID_EPINT_OUT0_ID,
    USB_AUDIO_HID_EPINT_OUT1_ID,
    USB_AUDIO_HID_EPINT_OUT2_ID,
    USB_AUDIO_HID_EPINT_ID_QTY
};


typedef void (*app_usb_hid_cmd_handler_t)(APP_USB_HID_CMD_CODE_E cmdCode, uint8_t* ptrParam, uint16_t paramLen);

typedef void (*app_usb_hid_cmd_response_handler_t)(APP_USB_HID_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint16_t paramLen);

typedef struct
{
    uint8_t    cmdCode;        /**< command code, from APP_USB_HID_CMD_CODE_E */
    uint8_t    paramLen;       /**< length of the following parameter */
    uint8_t     param[HID_REPORT_RSP_PACKET_SIZE-3];
} APP_USB_HID_CMD_PAYLOAD_T;

typedef struct
{
    uint8_t    cmdCodeToRsp;    /**< tell which command code to response */
    uint8_t    cmdRetStatus;    /**< handling result of the command, from APP_USB_HID_CMD_RET_STATUS_E */
    uint8_t    rspDataLen;      /**< length of the response data */
    uint8_t     rspData[HID_REPORT_RSP_PACKET_SIZE-4];
} APP_USB_HID_CMD_RSP_T;

typedef struct
{
    uint8_t     report_id;
    uint8_t     param[HID_REPORT_RSP_PACKET_SIZE-1];
} APP_USB_HID_DATA_RSP_T;

typedef struct
{
    uint16_t                    cmdCode;
    app_usb_hid_cmd_handler_t   cmdHandler;    /**< command handler function */
} APP_USB_HID_CMD_INSTANCE_T;

typedef struct{
    uint32_t reserved1 : 3;
    uint32_t mode : 1;
    uint32_t reserved2 : 28;
}APP_USB_HID_CMD_MTE_MODE_T;

typedef struct{
    uint16_t teams_led_status : 1;
    uint16_t hook_switch_led_status : 1;
    uint16_t mute_led_status : 1;
    uint16_t reserved : 13;
}APP_USB_HID_CMD_KEY_LEN_STATUS;

typedef struct{
    uint32_t teams_button : 1;
    uint32_t hook_switch_button : 1;
    uint32_t mute_button : 1;
    uint32_t volume_up_button : 1;
    uint32_t volume_down_button : 1;
    uint32_t reserved : 27;
}APP_USB_HID_CMD_COMPONENT_BUTTON_STATUS_T;

typedef struct{
    uint8_t component_type;
    APP_USB_HID_CMD_COMPONENT_BUTTON_STATUS_T button_status;
}APP_USB_HID_CMD_COMPONENT_STATUS_T;

typedef struct{
    uint8_t hw_major_version : 4;
    uint8_t build_stages : 4;
}APP_USB_HID_CMD_HW_MAJOR_VERSION_T;

typedef struct{
    uint8_t hw_minor_version;
    APP_USB_HID_CMD_HW_MAJOR_VERSION_T major_version;
}APP_USB_HID_CMD_HW_VERSION_T;

typedef struct{
    uint8_t scheme_code;
    uint8_t product_brand;
    uint8_t out_factory_date_year;
    uint8_t out_factory_date_month;
    uint8_t out_factory_date_day;
    APP_USB_HID_CMD_HW_VERSION_T hw_version;
    uint8_t reserved[9];
}APP_USB_HID_CMD_DEVICE_CAPABILITY_T;

typedef struct{
    uint8_t flash_init;
    uint8_t kid_mode;
    uint8_t volume_max;
    uint8_t tones_status;
    uint8_t tones_volume;
    uint8_t container_id[HID_REPORT_CONTAINER_ID_LEN];
    uint8_t color_name[HID_REPORT_COLOR_NAME_LEN];
    uint8_t serial_number_len;
    uint8_t serial_number[HID_REPORT_SERIAL_NUMBER_LEN];
    uint8_t eq_index;
    APP_USB_HID_CMD_MTE_MODE_T mte_mode;
    APP_USB_HID_CMD_DEVICE_CAPABILITY_T device_capability;
    uint8_t mic_eq_enable_flag;
    uint8_t spk_eq_enable_flag;
    uint8_t ptt_mode;
    APP_USB_HID_CMD_COMPONENT_STATUS_T component_status;
}APP_USB_HID_CMD_DEVICE_PARAM_T;

#define USB_HID_COMMAND_PTR_FROM_ENTRY_INDEX(index) \
    ((APP_USB_HID_CMD_INSTANCE_T *)((uint32_t)__usb_hid_handler_table_start + (index)*sizeof(APP_USB_HID_CMD_INSTANCE_T)))

#define USB_HID_COMMAND_TO_ADD(cmdCode, cmdHandler) \
    static const APP_USB_HID_CMD_INSTANCE_T cmdCode##_entry __attribute__((used, section(".usb_hid_handler_table"))) =  \
        {(cmdCode), (cmdHandler)};

void usb_hid_app_flash_flush_start(uint32_t ms);

uint8_t usb_hid_app_kid_mode_get(void);

uint8_t usb_hid_app_volume_max_get(void);

uint8_t usb_hid_app_tones_status_get(void);

uint8_t usb_hid_app_tones_volume_get(void);

uint8_t usb_hid_app_get_ptt_mode(void);

void usb_hid_app_set_component_status(uint8_t teams, uint8_t hook, uint8_t mute, uint8_t vol_up, uint8_t vol_down);

int usb_hid_app_init_onoff(bool onoff);

void app_usb_hid_send_rsp(uint8_t* cmdParam, uint16_t cmdParam_len);

#ifdef __cplusplus
}
#endif

#endif


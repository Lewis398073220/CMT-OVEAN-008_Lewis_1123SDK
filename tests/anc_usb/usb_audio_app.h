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
#ifndef __USB_AUDIO_APP_H__
#define __USB_AUDIO_APP_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "plat_types.h"
#include "hal_key.h"
#include "audio_process.h"
#ifdef DONGLE_SUPPORT
#include CHIP_SPECIFIC_HDR(dongle)
#endif

#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
#define HID_REPORT_KEY_PACKET_SIZE          0x02
#define HID_REPORT_ID_KEY_VOL               0x19
#define USB_AUDIO_HID_KEY_VOL_UP            (1 << 0)
#define USB_AUDIO_HID_KEY_VOL_DOWN          (1 << 1)
#define USB_AUDIO_HID_KEY_VOL_MUTE          (1 << 2)
#define USB_AUDIO_HID_KEY_PLAY              (1 << 3)
#define USB_AUDIO_HID_KEY_PAUSE             (1 << 4)
#define USB_AUDIO_HID_KEY_NEXT_TRACK        (1 << 5)
#define USB_AUDIO_HID_KEY_PREVIOUS_TRACK    (1 << 6)
#define USB_AUDIO_HID_KEY_PLAY_PAUSE        (1 << 7)
#define HID_REPORT_ID_KEY_MIC_MUTE          0x08
#define USB_AUDIO_HID_KEY_MIC_MUTE          (1 << 0)
#define USB_AUDIO_HID_KEY_HOOK_SWITCH       (1 << 1)
#define USB_AUDIO_HID_KEY_REDIAL            (1 << 2)
#define USB_AUDIO_HID_KEY_HOLD_RESUME       (1 << 3)
#define USB_AUDIO_HID_KEY_HOOK_REJECT       (1 << 5)
#define HID_REPORT_ID_HID_LED               0x09
#define HID_REPORT_ID_KEY_TEAMS             0x9B
#define USB_AUDIO_HID_KEY_MS_TEAM           (1 << 0)

typedef struct{
    bool incoming_state;
    bool active_state;
    bool hook_hold_state;
}USB_AUDIO_HID_CALL_EVENT_T;

typedef struct{
    uint8_t report_id;
    uint8_t cmd;
}USB_AUDIO_HID_KEY_T;

typedef struct{
    uint8_t mic_mute_led : 1;
    uint8_t off_hook_led : 1;
    uint8_t ring_incoming_led : 1;
    uint8_t microphone_led : 1;
    uint8_t on_line_led : 1;
    uint8_t hold_led : 1;
    uint8_t reserved : 2;
}USB_AUDIO_HID_LED_T;

#endif

enum AUDIO_CMD_T {
    AUDIO_CMD_START_PLAY = 0,
    AUDIO_CMD_STOP_PLAY,
    AUDIO_CMD_START_CAPTURE,
    AUDIO_CMD_STOP_CAPTURE,
    AUDIO_CMD_SET_VOLUME,
    AUDIO_CMD_SET_CAP_VOLUME,
    AUDIO_CMD_MUTE_CTRL,
    AUDIO_CMD_CAP_MUTE_CTRL,
    AUDIO_CMD_USB_RESET,
    AUDIO_CMD_USB_DISCONNECT,
    AUDIO_CMD_USB_CONFIG,
    AUDIO_CMD_USB_SLEEP,
    AUDIO_CMD_USB_WAKEUP,
    AUDIO_CMD_RECV_PAUSE,
    AUDIO_CMD_RECV_CONTINUE,
    AUDIO_CMD_SET_RECV_RATE,
    AUDIO_CMD_SET_SEND_RATE,
    AUDIO_CMD_RESET_CODEC,
    AUDIO_CMD_NOISE_GATING,
    AUDIO_CMD_NOISE_REDUCTION,
    AUDIO_CMD_TUNE_RATE,
    AUDIO_CMD_SET_DSD_CFG,
    TEST_CMD_PERF_TEST_POWER,
    TEST_CMD_PA_ON_OFF,
#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
    AUDIO_MEDIA_PROMPT_WARNING,
#endif
    AUDIO_CMD_QTY
};

struct USB_AUDIO_BUF_CFG {
    uint8_t *play_buf;
    uint32_t play_size;
    uint8_t *cap_buf;
    uint32_t cap_size;
    uint8_t *recv_buf;
    uint32_t recv_size;
    uint8_t *send_buf;
    uint32_t send_size;
    uint8_t *eq_buf;
    uint32_t eq_size;
    uint8_t *resample_buf;
    uint32_t resample_size;
#ifdef USB_AUDIO_MULTIFUNC
    uint8_t *recv2_buf;
    uint32_t recv2_size;
#endif
};

typedef void (*USB_AUDIO_ENQUEUE_CMD_CALLBACK)(uint32_t data);

void usb_audio_app(bool on);
void usb_audio_keep_streams_running(bool enable);
void usb_audio_app_init(const struct USB_AUDIO_BUF_CFG *cfg);

void usb_audio_app_term(void);
int usb_audio_app_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event);
#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
void usb_audio_app_hid_key_set_report_data_parse(USB_AUDIO_HID_KEY_T *pPayload);
#endif
void usb_audio_app_loop(void);
uint32_t usb_audio_get_capture_sample_rate(void);

uint32_t usb_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t index);
uint8_t usb_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status);

void usb_audio_set_enqueue_cmd_callback(USB_AUDIO_ENQUEUE_CMD_CALLBACK cb);

#ifdef DONGLE_SUPPORT
void usb_dongle_callback(DONGLE_NOTIFY_EVENT_E, DONGLE_INFORM_PARAM_T*);
#endif

#ifdef USB_HID_COMMAND_ENABLE
void usb_audio_enqueue_cmd(uint32_t data);
#endif

typedef void (*USB_AUDIO_SOURCE_INIT_CALLBACK)(void);
typedef void (*USB_AUDIO_SOURCE_DEINIT_CALLBACK)(void);
typedef bool (*USB_AUDIO_SOURCE_CHECK_FIRST_PROCESSED_USB_DATA_CALLBACK)(void);
typedef uint32_t  (*USB_AUDIO_SOURCE_DATA_PLAYBACK_CALLBACK)(uint8_t * pcm_buf, uint32_t len);
typedef void (*USB_AUDIO_SOURCE_DATA_CAPTURE_CALLBACK)(uint8_t * pcm_buf, uint32_t len);
typedef void (*USB_AUDIO_SOURCE_PLAYBACK_STREAM_START_CALLBACK)(void);
typedef void (*USB_AUDIO_SOURCE_PLAYBACK_STREAM_STOP_CALLBACK)(void);
typedef void (*USB_AUDIO_SOURCE_CAPTURE_STREAM_START_CALLBACK)(void);
typedef void (*USB_AUDIO_SOURCE_CAPTURE_STREAM_STOP_CALLBACK)(void);
typedef bool (*USB_AUDIO_SOURCE_RESET_CODEC_FEASIBILITY_CHECK_CALLBACK)(void);
typedef void (*USB_AUDIO_SOURCE_DATA_PREPARATION_CALLBACK)(enum AUD_STREAM_T stream);
typedef void (*USB_ADUIO_SOURCE_PLAYBACK_VOL_CHANGE)(uint32_t level);
typedef void (*USB_ADUIO_SOURCE_COMMAND_RECEIVED)(enum AUDIO_CMD_T cmd);

struct USB_AUDIO_SOURCE_EVENT_CALLBACK_T {
    USB_AUDIO_SOURCE_INIT_CALLBACK                              init_cb;
    USB_AUDIO_SOURCE_DEINIT_CALLBACK                            deinit_cb;
    USB_AUDIO_SOURCE_CHECK_FIRST_PROCESSED_USB_DATA_CALLBACK    check_first_processed_usb_data_cb;
    USB_AUDIO_SOURCE_DATA_PLAYBACK_CALLBACK                     data_playback_cb;
    USB_AUDIO_SOURCE_DATA_CAPTURE_CALLBACK                      data_capture_cb;
    USB_AUDIO_SOURCE_RESET_CODEC_FEASIBILITY_CHECK_CALLBACK     reset_codec_feasibility_check_cb;
    USB_AUDIO_SOURCE_DATA_PREPARATION_CALLBACK                  data_prep_cb;
    USB_AUDIO_SOURCE_PLAYBACK_STREAM_START_CALLBACK             playback_start_cb;
    USB_AUDIO_SOURCE_PLAYBACK_STREAM_STOP_CALLBACK              playback_stop_cb;
    USB_AUDIO_SOURCE_CAPTURE_STREAM_START_CALLBACK              capture_start_cb;
    USB_AUDIO_SOURCE_CAPTURE_STREAM_STOP_CALLBACK               capture_stop_cb;

    USB_ADUIO_SOURCE_PLAYBACK_VOL_CHANGE                        playback_vol_change_cb;
    USB_ADUIO_SOURCE_COMMAND_RECEIVED                           cmd_received_cb;
};

void send_usb_audio_buffer(uint32_t stream);
void usb_audio_source_config_init(const struct USB_AUDIO_SOURCE_EVENT_CALLBACK_T *cb_list);
bool usb_audio_check_capture_need_start(void);

#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
#define FLASH_BASIC_CYCLE_TIME              1000 // ms
#define FLASH_TEAMS_FAIL_CONN_COUNT         20 // ms
#define FLASH_TEAMS_NO_CONN_COUNT           6 // ms
void usb_audio_app_teams_led_pulsing_onoff(bool on);
void usb_audio_app_teams_led_flash_onoff(bool on, uint32_t flash_count, uint32_t flash_cycle_ms);
void usb_audio_app_teams_led_solid_onoff(bool on);
void usb_audio_app_hook_led_solid_onoff(bool on);
void usb_audio_app_mute_led_solid_onoff(bool on);
int usb_audio_app_led_init_onoff(bool onoff);
#endif

#ifdef __cplusplus
}
#endif

#endif

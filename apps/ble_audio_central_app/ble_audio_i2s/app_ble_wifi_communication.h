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

#ifndef __APP_BLE_WIFI_AUDIO_MANAGER_H__
#define __APP_BLE_WIFI_AUDIO_MANAGER_H__

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct{
    uint32_t message_id;
    uint32_t message_param0;
    uint32_t message_param1;
} APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T;

typedef enum {
    APP_BLE_WIFI_COMM_PLAYBACK_STREAM_START = 0,
    APP_BLE_WIFI_COMM_PLAYBACK_STREAM_STOP,
    APP_BLE_WIFI_COMM_CAPTURE_STREAM_START,
    APP_BLE_WIFI_COMM_CAPTURE_STREAM_STOP,
    APP_BLE_WIFI_COMM_PLAYBACK_VOLUME_CTRL,
} APP_BLE_WIFI_DUAL_MODE_COMM_ID_E;

typedef enum {
    APP_BLE_WIFI_MEDIA_CTRL_VOLUME_UP     = (1 << 0),
    APP_BLE_WIFI_MEDIA_CTRL_VOLUME_DOWN   = (1 << 1),
    APP_BLE_WIFI_MEDIA_CTRL_PLAY_PAUSE    = (1 << 2),
    APP_BLE_WIFI_MEDIA_CTRL_SCAN_NEXT     = (1 << 3),
    APP_BLE_WIFI_MEDIA_CTRL_SCAN_PREV     = (1 << 4),
    APP_BLE_WIFI_MEDIA_CTRL_STOP          = (1 << 5),
    APP_BLE_WIFI_MEDIA_CTRL_FAST_FWD      = (1 << 6),
    APP_BLE_WIFI_MEDIA_CTRL_REWIND        = (1 << 7),
    APP_BLE_WIFI_MEDIA_CTRL_VOL_MUTE      = (1 << 8),
    APP_BLE_WIFI_MEDIA_CTRL_VOICE_CMD     = (1 << 9),
    APP_BLE_WIFI_MEDIA_CTRL_REDIAL        = (1 << 10),
    APP_BLE_WIFI_MEDIA_CTRL_HOOK_SWITCH   = (1 << 11),
    APP_BLE_WIFI_MEDIA_CTRL_MIC_MUTE      = (1 << 12),
    APP_BLE_WIFI_MEDIA_CTRL_VOICE_MAIL    = (1 << 13),
} APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E;

#define PAIRING_INFO_SSID_MAX_LENGTH        (64)
#define PAIRING_INFO_PASSWORD_MAX_LENGTH    (64)

typedef struct{
    uint8_t  ap_ssid[PAIRING_INFO_SSID_MAX_LENGTH];
    uint8_t  ap_password[PAIRING_INFO_PASSWORD_MAX_LENGTH];
    uint16_t ap_ssid_length;
    uint16_t ap_password_length;
} APP_BLE_WIFI_PAIRING_INFO_PACKET_T;

#define BLE_ADV_TYPE_WIFI_HEADER        (0xFF)
#define BLE_ADV_DATA_WIFI_HEADER        "WIFI"
#define BLE_ADV_DATA_WIFI_HEADER_LEN    (4)

void app_ble_wifi_pairing_init(void);

void app_ble_wifi_enter_pairing_mode(void);

bool app_ble_wifi_is_pairing_mode(void);

APP_BLE_WIFI_PAIRING_INFO_PACKET_T* app_get_wifi_pairing_info_ptr(void);

void app_ble_wifi_dual_mode_media_control_send_cmd(APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E cmd);

void app_ble_wifi_pairing_info_exchange_send_cmd(APP_BLE_WIFI_PAIRING_INFO_PACKET_T pairing_info);

#ifdef DONGLE_AS_I2S_MASTER
void app_ble_wifi_dual_mode_audio_manager_playback_stream_onoff(void);

void app_ble_wifi_dual_mode_audio_manager_capture_stream_onoff(void);
#endif

void app_ble_wifi_dual_mode_audio_manager_send_request_remotely(APP_BLE_WIFI_DUAL_MODE_COMM_ID_E cmd, uint32_t param0, uint32_t param1);

void app_ble_wifi_dual_mode_init(void);

#if defined(__cplusplus)
}
#endif
#endif //__APP_BLE_WIFI_AUDIO_MANAGER_H__

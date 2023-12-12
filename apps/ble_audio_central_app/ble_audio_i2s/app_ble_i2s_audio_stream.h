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

#ifndef __APP_BLE_I2S_AUDIO_STREAM_H__
#define __APP_BLE_I2S_AUDIO_STREAM_H__

#include "app_ble_i2s_audio.h"
// #if defined(__cplusplus)
// extern "C" {
// #endif

void i2s_audio_source_config_init(const struct I2S_AUDIO_SOURCE_EVENT_CALLBACK_T *cb_list);

void app_ble_audio_i2s_playback_stream_start(void);

void app_ble_audio_i2s_playback_stream_stop(void);

void app_ble_audio_i2s_capture_stream_start(void);

void app_ble_audio_i2s_capture_stream_stop(void);

void app_ble_audio_i2s_playback_stream_vol_control(uint32_t volume_level);

uint8_t app_ble_audio_i2s_playback_stream_vol_get(void);

void app_ble_audio_stream_start_pre_handler(void);

void app_ble_audio_stream_stop_post_handler(void);

void app_ble_audio_low_latency_playback_stream_start(void);

void app_ble_audio_low_latency_playback_stream_stop(void);

void app_ble_audio_low_latency_playback_stream_vol_control(uint32_t volume_level);

// #if defined(__cplusplus)
// }
// #endif
#endif // __APP_BLE_I2S_AUDIO_STREAM_H__
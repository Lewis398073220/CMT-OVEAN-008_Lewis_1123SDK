/***************************************************************************
 *
 * Copyright (c) 2015-2023 BES Technic.
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
 */

#ifndef __APP_BLE_I2S_AUDIO_H__
#define __APP_BLE_I2S_AUDIO_H__
#include "bluetooth_ble_api.h"
#include "cmsis.h"
#include "cmsis_os.h"
#if defined(__cplusplus)
extern "C" {
#endif

#define I2S_AUD_STREAM_ID_PLAYBACK      AUD_STREAM_ID_0
#define I2S_AUD_STREAM_ID_CAPTURE       AUD_STREAM_ID_1

typedef void (*I2S_AUDIO_SOURCE_INIT_CALLBACK)(void);
typedef void (*I2S_AUDIO_SOURCE_DEINIT_CALLBACK)(void);
typedef void (*I2S_AUDIO_SOURCE_DATA_PLAYBACK_CALLBACK)(uint8_t * pcm_buf, uint32_t len);
typedef void (*I2S_AUDIO_SOURCE_DATA_CAPTURE_CALLBACK)(uint8_t * pcm_buf, uint32_t len);
typedef void (*I2S_AUDIO_SOURCE_PLAYBACK_STREAM_START_CALLBACK)(void);
typedef void (*I2S_AUDIO_SOURCE_PLAYBACK_STREAM_STOP_CALLBACK)(void);
typedef void (*I2S_AUDIO_SOURCE_CAPTURE_STREAM_START_CALLBACK)(void);
typedef void (*I2S_AUDIO_SOURCE_CAPTURE_STREAM_STOP_CALLBACK)(void);
typedef bool (*I2S_AUDIO_SOURCE_RESET_CODEC_FEASIBILITY_CHECK_CALLBACK)(void);
typedef void (*I2S_AUDIO_SOURCE_DATA_PREPARATION_CALLBACK)(enum AUD_STREAM_T stream);
typedef void (*I2S_ADUIO_SOURCE_PLAYBACK_VOL_CHANGE)(uint32_t level);

struct I2S_AUDIO_SOURCE_EVENT_CALLBACK_T {
    I2S_AUDIO_SOURCE_INIT_CALLBACK                     init_cb;
    I2S_AUDIO_SOURCE_DEINIT_CALLBACK                   deinit_cb;
    I2S_AUDIO_SOURCE_DATA_PLAYBACK_CALLBACK            data_playback_cb;
    I2S_AUDIO_SOURCE_DATA_CAPTURE_CALLBACK             data_capture_cb;
    I2S_AUDIO_SOURCE_RESET_CODEC_FEASIBILITY_CHECK_CALLBACK reset_codec_feasibility_check_cb;
    I2S_AUDIO_SOURCE_DATA_PREPARATION_CALLBACK         data_prep_cb;
    I2S_AUDIO_SOURCE_PLAYBACK_STREAM_START_CALLBACK    playback_start_cb;
    I2S_AUDIO_SOURCE_PLAYBACK_STREAM_STOP_CALLBACK     playback_stop_cb;
    I2S_AUDIO_SOURCE_CAPTURE_STREAM_START_CALLBACK     capture_start_cb;
    I2S_AUDIO_SOURCE_CAPTURE_STREAM_STOP_CALLBACK      capture_stop_cb;

    I2S_ADUIO_SOURCE_PLAYBACK_VOL_CHANGE               playback_vol_change_cb;
};

void app_ble_i2s_audio_init(void);
void app_ble_i2s_media_ascc_release_stream_test(void);
AOB_MGR_STREAM_STATE_E app_ble_i2s_media_get_cur_ase_state(void);
void app_ble_i2s_media_ascc_start_stream_test(void);

void gap_mobile_start_i2s_audio_receiving_dma(void);
void gap_mobile_start_i2s_audio_transmission_dma(void);
void gaf_mobile_i2s_dma_playback_stop(void);
void gaf_mobile_i2s_dma_capture_stop(void);

int gaf_mobile_i2s_audio_media_stream_start_handler(void* _pStreamEnv);
int gaf_mobile_i2s_audio_media_stream_stop_handler(void* _pStreamEnv);
int gaf_mobile_i2s_audio_capture_start_handler(void* _pStreamEnv);
int gaf_mobile_i2s_audio_capture_stop_handler(void* _pStreamEnv);
void gaf_mobile_usb_reconn_start(void);
void gaf_mobile_usb_reconn_stop(void);
bool gaf_mobile_i2s_audio_check_capture_need_start(void);

void app_ble_i2s_stream_inform_playback_state(uint32_t event);
void app_ble_i2s_stream_inform_capture_state(uint32_t event);

#if defined(__cplusplus)
}
#endif
#endif /* __APP_BLE_I2S_AUDIO_H__ */


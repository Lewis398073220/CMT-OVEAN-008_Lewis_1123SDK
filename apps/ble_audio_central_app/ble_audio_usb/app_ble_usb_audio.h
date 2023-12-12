/**
 * @file app_ble_audio_audio.h
 * @author BES AI team
 * @version 0.1
 * @date 2021-12-23
 *
 * @copyright Copyright (c) 2015-2021 BES Technic.
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

#ifndef __APP_BLE_USB_AUDIO_H__
#define __APP_BLE_USB_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

void app_ble_usb_audio_init(void);
void app_ble_usb_media_ascc_release_stream_test(void);
uint32_t app_ble_usb_media_get_cur_ase_state(void);
void app_ble_usb_media_ascc_start_stream_test(void);

void gap_mobile_start_usb_audio_receiving_dma(void);
void gap_mobile_start_usb_audio_transmission_dma(void);
void gaf_mobile_usb_dma_playback_stop(void);
void gaf_mobile_usb_dma_capture_stop(void);

int gaf_mobile_usb_audio_media_stream_start_handler(void* _pStreamEnv);
int gaf_mobile_usb_audio_media_stream_stop_handler(void* _pStreamEnv);
int gaf_mobile_usb_audio_capture_start_handler(void* _pStreamEnv);
int gaf_mobile_usb_audio_capture_stop_handler(void* _pStreamEnv);
void gaf_mobile_usb_reconn_start(void);
void gaf_mobile_usb_reconn_stop(void);
bool gaf_mobile_usb_audio_check_capture_need_start(void);

void app_ble_usb_stream_inform_playback_state(uint32_t event);
void app_ble_usb_stream_inform_capture_state(uint32_t event);

#ifdef __cplusplus
}
#endif
#endif /* __APP_BLE_USB_AUDIO_H__ */


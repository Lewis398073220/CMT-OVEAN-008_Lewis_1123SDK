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
#ifndef __APP_A2DP_H__
#define __APP_A2DP_H__
#include "btapp.h"
#if defined(__cplusplus)
extern "C" {
#endif

#define A2DP_NON_CODEC_TYPE_NON         0
#define A2DP_NON_CODEC_TYPE_LHDC        1
#define A2DP_NON_CODEC_TYPE_LHDCV5      2

typedef enum
{
    APP_A2DP_PROFILE_STATE_CONNECTING = 0x01,
    APP_A2DP_PROFILE_STATE_CONNECTED = 0x02,
    APP_A2DP_PROFILE_STATE_DISCONNECTING = 0x03,
    APP_A2DP_PROFILE_STATE_DISCONNECTED = 0x04
} app_a2dp_conn_state;

typedef enum
{
    APP_A2DP_STATE_NOT_PLAYING = 0x00,
    APP_A2DP_STATE_PLAYING = 0x01,
} app_a2dp_play_state;

typedef enum
{
    APP_A2DP_EVENT_CONNECTION_STATE = 0x00,
    APP_A2DP_EVENT_PLAY_STATUS = 0x01,
} app_a2dp_event_e;

typedef enum
{
    APP_A2DP_CUSTOM_AF_CTRL_NONE = 0,
    APP_A2DP_CUSTOM_AF_CTRL_START,
    APP_A2DP_CUSTOM_AF_CTRL_STOP,
} app_a2dp_custom_af_ctrl_e;

typedef void (*app_btif_a2dp_event_callback)(app_a2dp_event_e type, uint8_t* addr, uint32_t para);

typedef void (*media_info_report_t)(const bt_bdaddr_t *addr, const avrcp_adv_rsp_parms_t *mediaPkt);

const char* avrcp_get_track_element_name(uint32_t element_id);

void app_a2dp_state_change_callback_register(app_btif_a2dp_event_callback cb);

void app_a2dp_state_change_callback_deregister(void);

void app_a2dp_notify_receive_stream_callback_register(void (*cb)(void));

void app_a2dp_notify_receive_stream_callback_deregister(void);

bool app_a2dp_is_notify_receive_stream_cb_registed(void);

void app_a2dp_set_custom_af_ctrl_state(app_a2dp_custom_af_ctrl_e state);

app_a2dp_custom_af_ctrl_e app_a2dp_get_custom_af_ctrl_state(void);

uint8_t a2dp_get_current_lhdc_codec_type(uint8_t *elements, uint8_t device_id);

uint8_t app_bt_a2dp_adjust_volume(uint8_t device_id, bool up, bool adjust_local_vol_level);

void app_avrcp_reg_media_info_report_callback(media_info_report_t cb);

void avrcp_callback_CT(uint8_t device_id, btif_avrcp_channel_t* btif_avrcp, const avrcp_callback_parms_t* parms);

#ifdef BT_SOURCE
void a2dp_source_callback(uint8_t device_id, a2dp_stream_t *Stream, const a2dp_callback_parms_t *info);
#endif

#ifdef BT_A2DP_SUPPORT
#define MAX_A2DP_VOL   (127)
#define APP_A2DP_REJECT_SNIFF_TIMEOUT (10000)
 
#define A2DP_NON_CODEC_TYPE_NON         0
#define A2DP_NON_CODEC_TYPE_LHDC        1
#define A2DP_NON_CODEC_TYPE_LHDCV5      2

typedef void (*media_info_report_t)(const bt_bdaddr_t *addr, const avrcp_adv_rsp_parms_t *mediaPkt);

const char* avrcp_get_track_element_name(uint32_t element_id);
uint8_t a2dp_get_current_codec_type(uint8_t *elements, uint8_t device_id);

uint8_t app_bt_a2dp_adjust_volume(uint8_t device_id, bool up, bool adjust_local_vol_level);

void app_avrcp_reg_media_info_report_callback(media_info_report_t cb);

void a2dp_init(void);

bool a2dp_is_music_ongoing(void);

bool app_bt_is_a2dp_disconnected(uint8_t device_id);

void app_avrcp_get_capabilities_start(int device_id);

void app_a2dp_bt_driver_callback(uint8_t device_id, btif_a2dp_event_t event);

void app_bt_a2dp_disable_aac_codec(bool disable);

void app_bt_a2dp_disable_vendor_codec(bool disable);

bool app_bt_a2dp_send_volume_change(int device_id);

bool app_bt_a2dp_report_current_volume(int device_id);

void btapp_a2dp_report_speak_gain(void);

void app_bt_a2dp_reconfig_to_aac(a2dp_stream_t *stream);

void app_bt_a2dp_reconfig_to_vendor_codec(a2dp_stream_t *stream, uint8_t codec_id, uint8_t a2dp_non_type);

void app_bt_a2dp_reconfig_to_sbc(a2dp_stream_t *stream);

void app_bt_a2dp_reconfig_codec(a2dp_stream_t *stream, uint8_t code_type);

uint8_t a2dp_convert_local_vol_to_bt_vol(uint8_t localVol);

void app_pts_ar_volume_change(void);
void app_pts_ar_set_absolute_volume(void);
void app_pts_av_disc_channel(void);
void app_pts_av_close_channel(void);
void app_pts_ar_connect(bt_bdaddr_t *btaddr);
void app_pts_ar_disconnect(void);
void app_pts_ar_panel_stop(void);
void app_pts_ar_panel_play(void);
void app_pts_ar_panel_pause(void);
void app_pts_ar_panel_forward(void);
void app_pts_ar_panel_backward(void);
void app_pts_ar_volume_up(void);
void app_pts_ar_volume_down(void);
void app_pts_ar_volume_notify(void);

#if defined(A2DP_LDAC_ON)
void app_ibrt_restore_ldac_info(uint8_t device_id, uint8_t sample_freq);
#endif

#if defined(A2DP_LHDC_ON) || defined(A2DP_LHDCV5_ON)
bool a2dp_lhdc_get_ext_flags(uint32_t flags);
uint8_t a2dp_lhdc_config_llc_get(void);
uint8_t a2dp_lhdc_get_non_type_by_device_id(uint8_t device_id);
#endif

#ifdef __TENCENT_VOICE__
uint8_t avrcp_get_media_status(void);
void avrcp_set_media_status(uint8_t status);
#endif
void app_a2dp_reject_sniff_start(uint8_t device_id, uint32_t timeout);

#ifdef IBRT
int a2dp_ibrt_session_new(uint8_t devId);
int a2dp_ibrt_session_set(uint8_t session,uint8_t devId);
uint32_t a2dp_ibrt_session_get(uint8_t devId);

typedef struct ibrt_a2dp_status_t ibrt_a2dp_status_t;
int a2dp_ibrt_sync_get_status(uint8_t device_id, ibrt_a2dp_status_t *a2dp_status);
int a2dp_ibrt_sync_set_status(uint8_t device_id, ibrt_a2dp_status_t *a2dp_status);
#endif

#endif /* BT_A2DP_SUPPORT */
#ifdef __cplusplus
}
#endif
#endif

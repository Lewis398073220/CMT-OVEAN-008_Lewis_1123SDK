
/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#include "bluetooth_bt_api.h"
#include "hal_trace.h"
#include "ble_audio_define.h"
#ifndef BLE_STACK_NEW_DESIGN
#include "app_ble_mode_switch.h"
#else
#include "app_ble.h"
#endif
#include "app_audio_focus_control.h"
#include "aob_media_api.h"
#include "aob_call_api.h"
#include "gaf_media_stream.h"
#include "aob_stream_handler.h"
#include "app_audio_bt_device.h"
#include "app_audio_active_device_manager.h"
#include "app_bt_media_manager.h"
#include "apps.h"
#include "app_audio.h"
#include "app_media_player.h"
#include "ble_audio_earphone_info.h"
#include "bt_drv_reg_op.h"
#include "app_audio_call_mediator.h"
#include "app_audio_control.h"
#include "audioflinger.h"
#include "gaf_media_stream.h"
#include "gaf_bis_media_stream.h"
#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED)
#ifndef BLE_STACK_NEW_DESIGN
#include "buds.h"
#endif
#endif
#include "app_bap_data_path_itf.h"

#define BLE_AUDIO_PLAY_RING_TONE_INTERVAL    (3000)

typedef struct
{
    ble_bdaddr_t address;
    uint8_t con_lid;
    uint8_t music_sink_ase_lid;
    uint8_t call_sink_ase_lid;
    uint8_t call_src_ase_lid;
    uint8_t game_sink_ase_lid;
    uint8_t game_src_ase_lid;
    uint8_t bis_sink_stream_lid;
    int8_t music_audio_focus;
    int8_t call_audio_focus;
    int8_t ring_audio_focus;
    int8_t game_audio_focus;
    bool set_as_bg_device;
    bool music_streaming_available;
    bool call_streaming_available;
    bool game_streaming_available;
    bool bis_streaming_available;

    osTimerDefEx_t ring_tone_play_timer_def;
    osTimerId ring_tone_play_timer;

    uint32_t current_context_type;

    on_audio_focus_change_listener music_focus_changed_listener;
    on_audio_focus_change_listener call_focus_changed_listener;
    on_audio_focus_change_listener game_focus_changed_listener;
    on_audio_focus_change_listener ring_focus_changed_listener;
} BLE_AUDIO_SINK_DEVICE_T;

static BLE_AUDIO_SINK_DEVICE_T ble_audio_sink_device[AOB_COMMON_MOBILE_CONNECTION_MAX];
static BLE_AUDIO_POLICY_CONFIG_T ble_audio_policy_config;

void app_ble_audio_sink_stream_start(uint8_t conlid, uint16_t stream_type, uint8_t stream_lid)
{
    if (stream_type == GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS)
    {
#ifdef APP_BLE_BIS_SINK_ENABLE
        ///for bis frist param is stream_lid
        gaf_bis_audio_stream_start_handler(stream_lid);
#endif
    }
    else
    {
        gaf_audio_stream_start(conlid, stream_lid);
    }
}

void app_ble_audio_sink_stream_stop(uint8_t conlid, uint16_t stream_type, uint8_t stream_lid)
{
    if (stream_type == GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS)
    {
#ifdef APP_BLE_BIS_SINK_ENABLE
        ///for bis frist param is stream_lid
        gaf_bis_audio_stream_stop_handler(stream_lid);
#endif
    }
    else
    {
        gaf_audio_stream_stop(conlid, stream_lid);
    }
}

void app_ble_audio_policy_config(void)
{
    ble_audio_policy_config.mute_new_music_stream = false;
    ble_audio_policy_config.pause_new_music_stream = true;
}

BLE_AUDIO_POLICY_CONFIG_T *app_ble_audio_get_policy_config(void)
{
    return &ble_audio_policy_config;
}

BLE_AUDIO_SINK_DEVICE_T *app_ble_audio_get_device(uint8_t con_lid)
{
    if (con_lid < AOB_COMMON_MOBILE_CONNECTION_MAX)
    {
        return ble_audio_sink_device + con_lid;
    }

    return NULL;
}

static int app_ble_audio_music_ext_policy(focus_device_info_t *cdevice_info, focus_device_info_t *rdevice_info)
{
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(cdevice_info->device_idx);

    if ((rdevice_info->stream_type == USAGE_MEDIA) && (device->current_context_type != GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS))
    {
        TRACE(0, "[%s][%d]:Allow new music play!", __FUNCTION__, __LINE__);
        // Allow request music play
        return AUDIOFOCUS_REQUEST_FAILED;
    }

    return AUDIOFOCUS_REQUEST_GRANTED;
}

static int app_ble_audio_flexible_ext_policy(focus_device_info_t *cdevice_info, focus_device_info_t *rdevice_info)
{
    if (rdevice_info->stream_type == USAGE_MEDIA)
    {
        TRACE(0, "[%s][%d]:Allow new music play!", __FUNCTION__, __LINE__);
        // Allow request music play
        return AUDIOFOCUS_REQUEST_FAILED;
    }
    else if (rdevice_info->stream_type == USAGE_CALL ||
             rdevice_info->stream_type == USAGE_RINGTONE)
    {
        TRACE(0, "call always prompt");
        return AUDIOFOCUS_REQUEST_GRANTED;
    }
    else if (rdevice_info->stream_type == USAGE_FLEXIBLE)
    {
#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED)
        /*
         * Allow new media play if upload streaming is not exist.
        */
        uint8_t startedStreamTypes = 0;
        BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(cdevice_info->device_idx);
        if (device->game_src_ase_lid == 0xFF)
        {
            TRACE(0, "unconfigured src ase id,allow new streaming play ");
            return AUDIOFOCUS_REQUEST_GRANTED;
        }
        startedStreamTypes = gaf_audio_get_stream_started_type(device->con_lid);
        if ((startedStreamTypes & GAF_AUDIO_STREAM_TYPE_CAPTURE) == 0)
        {
            TRACE(0, "no upstreaming, accept new streaming");
            return AUDIOFOCUS_REQUEST_GRANTED;
        }
        else
        {
            return AUDIOFOCUS_REQUEST_FAILED;
        }
#else
        return AUDIOFOCUS_REQUEST_GRANTED;
#endif
    }
    return AUDIOFOCUS_REQUEST_FAILED;
}

static int app_ble_audio_ringtone_stream_ext_policy(focus_device_info_t *cdevice_info, focus_device_info_t *rdevice_info)
{
    if (rdevice_info->stream_type == USAGE_CALL)
    {
        return AUDIOFOCUS_REQUEST_GRANTED;
    }
    return AUDIOFOCUS_REQUEST_FAILED;
}

static int app_ble_audio_request_audio_focus(
    BLE_AUDIO_SINK_DEVICE_T *device, int request_type, AUDIO_USAGE_TYPE_E stream_type, bool allow_delay)
{
    audio_focus_req_info_t request_info;
    int result;

    memcpy((uint8_t *)&request_info.device_info.device_addr, (void *)&device->address, sizeof(bt_bdaddr_t));
    request_info.device_info.device_idx = device->con_lid;
    request_info.device_info.audio_type = AUDIO_TYPE_LE_AUDIO;
    request_info.device_info.focus_request_type = request_type;
    request_info.device_info.stream_type = stream_type;
    request_info.device_info.delayed_focus_allow = allow_delay;

    if (stream_type == USAGE_MEDIA)
    {
        request_info.focus_changed_listener = device->music_focus_changed_listener;
        request_info.ext_policy = app_ble_audio_music_ext_policy;
    }
    else if (stream_type == USAGE_FLEXIBLE)
    {
        request_info.focus_changed_listener = device->game_focus_changed_listener;
        request_info.ext_policy = app_ble_audio_flexible_ext_policy;
    }
    else if (stream_type == USAGE_CALL)
    {
        request_info.focus_changed_listener = device->call_focus_changed_listener;
        request_info.ext_policy = app_ble_audio_flexible_ext_policy;
    }
    else if (stream_type == USAGE_RINGTONE)
    {
        request_info.focus_changed_listener = device->ring_focus_changed_listener;
        request_info.ext_policy = app_ble_audio_ringtone_stream_ext_policy;
    }
    result = app_audio_request_focus(&request_info);
    TRACE(1, "le_audio_policy:requst audio focus result = %d", result);
    return result;
}

static int app_ble_audio_abandon_audio_focus(BLE_AUDIO_SINK_DEVICE_T *device, AUDIO_USAGE_TYPE_E stream_type)
{
    audio_focus_req_info_t removed_fouces;

    //memcpy((uint8_t*)&removed_fouces.device_addr,(void*)&device->remote,sizeof(bt_bdaddr_t));
    removed_fouces.device_info.device_idx = device->con_lid;
    removed_fouces.device_info.audio_type = AUDIO_TYPE_LE_AUDIO;
    removed_fouces.device_info.stream_type = stream_type;

    return app_audio_abandon_focus(&removed_fouces);
}

void app_ble_audio_stream_start_handler(uint8_t ase_lid)
{
    gaf_stream_context_state_t updatedContextStreamState = gaf_audio_stream_update_and_start_handler(ase_lid);

    if (APP_GAF_CONTEXT_STREAM_STARTED == updatedContextStreamState)
    {
        app_bap_ascs_ase_t *p_bap_ase_info = app_bap_uc_srv_get_ase_info(ase_lid);
        app_ble_audio_event_t evt = BLE_AUDIO_MAX_IND;

        if (p_bap_ase_info->init_context_bf & AOB_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)
        {
            evt = BLE_AUDIO_CALL_STREAM_START_IND;
        }
        else if (p_bap_ase_info->init_context_bf & AOB_AUDIO_CONTEXT_TYPE_MEDIA)
        {
            evt = BLE_AUDIO_MUSIC_STREAM_START_IND;
        }
        else
        {
            evt = BLE_AUDIO_FLEXIBLE_STREAM_START_IND;
        }

        if (ase_lid == 0)
        {
            app_bap_dp_itf_data_come_callback_deregister();
        }

        app_ble_audio_sink_streaming_handle_event(p_bap_ase_info->con_lid, ase_lid, APP_GAF_DIRECTION_MAX, evt);
    }
}

static void app_ble_audio_stream_check_and_start_all_handler(uint8_t con_lid)
{
    uint8_t ase_lid_list[APP_BAP_MAX_ASCS_NB_ASE_CHAR] = {0};
    uint8_t streaming_ase_cnt = aob_media_get_curr_streaming_ase_lid_list(con_lid, ase_lid_list);
    uint8_t ase_cnt = 0;

    for (ase_cnt = 0; ase_cnt < streaming_ase_cnt; ase_cnt++)
    {
        app_ble_audio_stream_start_handler(ase_lid_list[ase_cnt]);
    }
}

static void app_ble_audio_on_music_focus_changed(uint8_t device_id, AUDIO_USAGE_TYPE_E media_type, int audio_focus_change)
{
    uint8_t stream_lid;
    TRACE(3, "le_audio_policy:music focus change device_id:%d,media_type:%d,focus_changed = %d", device_id, media_type, audio_focus_change);
    ASSERT(media_type == USAGE_MEDIA, "media_type error %d", media_type);
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(device_id);
    if (!device)
    {
        return;
    }
    TRACE(2, "le_audio_policy:music streaming status = %d,bg device = %d", device->music_streaming_available, device->set_as_bg_device);

    if (device->bis_streaming_available)
    {
        stream_lid = device->bis_sink_stream_lid;
    }
    else
    {
        stream_lid = device->music_sink_ase_lid;
    }

    switch (audio_focus_change)
    {
        case AUDIOFOCUS_GAIN:
            device->music_audio_focus = AUDIOFOCUS_GAIN;
            if (device->set_as_bg_device == true)
            {
                aob_media_play(device_id);
            }
            // audio_manager_stream_ctrl_start_ble_audio(device_id, GAF_AUDIO_STREAM_CONTEXT_TYPE_MEDIA);
            /// ringtone and LE Audio cocurr situation, because ringtone start but stream not stop.
            /// so need check start after ringtone stopped
            app_ble_audio_stream_check_and_start_all_handler(device_id);
            break;
        case AUDIOFOCUS_LOSS:
            if (device->music_streaming_available)
            {
                device->set_as_bg_device = true;
                aob_media_pause(device_id);
                audio_manager_stream_ctrl_stop_ble_audio(device_id, device->current_context_type, stream_lid);
            }
            device->music_audio_focus = AUDIOFOCUS_NONE;
            app_ble_audio_abandon_audio_focus(device, USAGE_MEDIA);
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT:
            device->music_audio_focus = AUDIOFOCUS_NONE;
            if (device->music_streaming_available)
            {
                device->set_as_bg_device = true;
                aob_media_pause(device_id);
                audio_manager_stream_ctrl_stop_ble_audio(device_id, device->current_context_type, stream_lid);
            }
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
            if (device->music_streaming_available)
            {
                audio_manager_stream_ctrl_stop_ble_audio(device_id, device->current_context_type, stream_lid);
                TRACE(0, "waiting phone stop media!");
            }
            break;
        default:
            break;
    }
}

static void app_ble_audio_pull_call_backto_earbud(uint8_t sink_ase_lid, uint8_t src_ase_lid)
{
#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED)
    app_bap_ascs_ase_t *call_sink_ase_info = app_bap_uc_srv_get_ase_info(sink_ase_lid);
    app_bap_ascs_ase_t *call_src_ase_info = app_bap_uc_srv_get_ase_info(src_ase_lid);

    // TODO: If ase state is idle, no bap_cfg is stored
    aob_media_ascs_srv_set_codec(sink_ase_lid, &call_sink_ase_info->codec_id, &call_sink_ase_info->qos_req, call_sink_ase_info->p_cfg);
    aob_media_ascs_srv_set_codec(src_ase_lid, &call_src_ase_info->codec_id, &call_src_ase_info->qos_req, call_src_ase_info->p_cfg);
#endif
}

static void app_ble_audio_on_phone_call_focus_changed(uint8_t device_id, AUDIO_USAGE_TYPE_E media_type, int audio_focus_change)
{
    TRACE(3, "le_audio_policy:call focus change device_id:%d,media_type:%d,focus_changed = %d",
          device_id, media_type, audio_focus_change);
    ASSERT((USAGE_CALL == media_type) || (USAGE_RINGTONE == media_type), "call type error:%d", media_type);
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(device_id);
    AOB_CALL_ENV_INFO_T *pCallEnvInfo = ble_audio_earphone_info_get_call_env_info(device_id);

    switch (audio_focus_change)
    {
        case AUDIOFOCUS_GAIN:
            if (media_type == USAGE_CALL)
            {
                if (device->call_streaming_available)
                {
                    device->call_audio_focus = AUDIOFOCUS_GAIN;
                    gaf_audio_stream_update_and_start_handler(device->call_sink_ase_lid);
                }
                else
                {
                    if (pCallEnvInfo)
                    {
                        if ((pCallEnvInfo->status_flags.inband_ring_enable &&
                                ble_audio_earphone_info_get_incoming_call_id_by_conidx(device_id) != INVALID_CALL_ID)
                                || ble_audio_earphone_info_get_calling_call_id_by_conidx(device_id) != INVALID_CALL_ID)
                        {
                            app_ble_audio_pull_call_backto_earbud(device->call_sink_ase_lid, device->call_src_ase_lid);
                        }
                    }
                    else
                    {
                        app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
                        device->call_audio_focus = AUDIOFOCUS_NONE;
                    }
                }
            }
            else if (media_type == USAGE_RINGTONE)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_RINGTONE);
            }
            break;
        case AUDIOFOCUS_LOSS:
            device->call_audio_focus = AUDIOFOCUS_NONE;
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT:
            TRACE(1, "le_audio_policy phone streaming status = %d", device->call_streaming_available);
            device->call_audio_focus = AUDIOFOCUS_NONE;
            if (device->call_streaming_available && media_type == USAGE_CALL)
            {
                device->set_as_bg_device = true;
                app_bap_uc_srv_stream_release(device_id, false);
                audio_manager_stream_ctrl_stop_ble_audio(device_id, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, device->call_sink_ase_lid);
            }
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
            break;
        default:
            break;
    }
}

static void app_ble_audio_on_phone_ring_focus_changed(uint8_t device_id, AUDIO_USAGE_TYPE_E media_type, int audio_focus_change)
{
    TRACE(3, "le_audio_policy:ring focus change device_id:%d,media_type:%d,focus_changed = %d", device_id, media_type, audio_focus_change);
    ASSERT(media_type == USAGE_RINGTONE, "ring type error %d", media_type);
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(device_id);
    CALL_STATE_INFO_T *curr_call_state = app_bt_get_call_state_info();
    TRACE(1, "le_audio_policy:LE_Audio curr call state = %d", curr_call_state->state);

    switch (audio_focus_change)
    {
        case AUDIOFOCUS_GAIN:
            if (AOB_CALL_STATE_INCOMING == (AOB_CALL_STATE_E)curr_call_state->state)
            {
                device->ring_audio_focus = AUDIOFOCUS_GAIN;
                app_audio_adm_le_audio_handle_action(device->con_lid, RINGTONE_CHANGED, RINGTONE_START);
#ifdef MEDIA_PLAYER_SUPPORT
                media_PlayAudio(AUD_ID_LE_AUD_INCOMING_CALL, device->con_lid);
#endif
            }
            else
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_RINGTONE);
                device->ring_audio_focus = AUDIOFOCUS_NONE;
            }
            break;
        case AUDIOFOCUS_LOSS:
            app_ble_audio_abandon_audio_focus(device, USAGE_RINGTONE);
            device->ring_audio_focus = AUDIOFOCUS_NONE;
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT:
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
            break;
        default:
            break;
    }
}

static void app_ble_audio_sink_ringtone_timer_handler(void const *param)
{
    BLE_AUDIO_SINK_DEVICE_T *device = (BLE_AUDIO_SINK_DEVICE_T *)param;
    if (device == NULL)
    {
        return;
    }

    TRACE(0, "app_ble_audio_ring_tone_play");
    if (app_ble_audio_request_audio_focus(
                device, AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK, USAGE_RINGTONE, false) == AUDIOFOCUS_REQUEST_GRANTED)
    {
        app_audio_adm_le_audio_handle_action(device->con_lid, RINGTONE_CHANGED, RINGTONE_START);
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_LE_AUD_INCOMING_CALL, device->con_lid);
#endif
    }
    else
    {
        TRACE(0, "le_audio_policy:ringtone request focus fail/delay");
    }

    osTimerStop(device->ring_tone_play_timer);
    osTimerStart(device->ring_tone_play_timer, BLE_AUDIO_PLAY_RING_TONE_INTERVAL);
}

void app_ble_audio_stop_ring_tone_play(BLE_AUDIO_SINK_DEVICE_T *device)
{
    osTimerStop(device->ring_tone_play_timer);
    app_audio_adm_le_audio_handle_action(device->con_lid, RINGTONE_CHANGED, RINGTONE_STOP);
    //app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA, BT_STREAM_MEDIA, device->con_lid, 0);
}

#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED)
static void  app_ble_audio_pullback_dongle_audio()
{
    bool ret = false;
#if BLE_BUDS
    uint8_t cmd_buf[1] = {0xbb};
    ret = buds_send_data_via_write_command(0, cmd_buf, 1);
#endif
    TRACE(0, "%s,ret %d", __func__, ret);
}
#endif

static void app_ble_audio_disable_game_ase(BLE_AUDIO_SINK_DEVICE_T *device)
{
    app_bap_ascs_ase_t *p_ascs_ase = NULL;

    p_ascs_ase = app_bap_uc_srv_get_ase_info(device->game_src_ase_lid);
    if (p_ascs_ase && p_ascs_ase->ase_state == AOB_MGR_STREAM_STATE_STREAMING)
    {
        TRACE(1, "le_audio_policy:disable game ase src id = %d", device->game_src_ase_lid);
        app_bap_uc_srv_stream_disable(device->game_src_ase_lid);
    }
    p_ascs_ase = app_bap_uc_srv_get_ase_info(device->game_sink_ase_lid);
    if (p_ascs_ase && p_ascs_ase->ase_state == AOB_MGR_STREAM_STATE_STREAMING)
    {
        TRACE(1, "le_audio_policy:disable game ase sink id = %d", device->game_sink_ase_lid);
        app_bap_uc_srv_stream_disable(device->game_sink_ase_lid);
    }
}

static void app_ble_audio_on_game_focus_changed(uint8_t device_id, AUDIO_USAGE_TYPE_E media_type, int audio_focus_change)
{
    TRACE(3, "le_audio_policy:game focus change device_id:%d,media_type:%d,focus_changed = %d", device_id, media_type, audio_focus_change);
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(device_id);

    switch (audio_focus_change)
    {
        case AUDIOFOCUS_GAIN:
            device->game_audio_focus = AUDIOFOCUS_GAIN;
            if (device->game_streaming_available)
            {
            }
            else
            {
#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED)
                app_ble_audio_pullback_dongle_audio();
#endif
            }
            break;
        case AUDIOFOCUS_LOSS:
            if (device->game_streaming_available)
            {
                app_ble_audio_disable_game_ase(device);
                audio_manager_stream_ctrl_stop_ble_audio(device_id, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, device->game_sink_ase_lid);
            }
            else
            {
                device->game_audio_focus = AUDIOFOCUS_NONE;
                app_ble_audio_abandon_audio_focus(device, USAGE_FLEXIBLE);
            }
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT:
            TRACE(1, "le_audio_policy game streaming status = %d", device->game_streaming_available);
            if (device->game_streaming_available)
            {
                app_ble_audio_disable_game_ase(device);
                audio_manager_stream_ctrl_stop_ble_audio(device_id, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, device->game_sink_ase_lid);
            }
            else
            {
                device->game_audio_focus = AUDIOFOCUS_LOSS_TRANSIENT;
                app_ble_audio_abandon_audio_focus(device, USAGE_FLEXIBLE);
            }
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
            break;
        default:
            break;
    }
}

static void app_ble_audio_stop_ringtone(uint8_t con_lid)
{
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(con_lid);
    TRACE(0, "le_audio_policy,stop ringtone");
    app_ble_audio_stop_ring_tone_play(device);
    app_ble_audio_abandon_audio_focus(device, USAGE_RINGTONE);
    device->ring_audio_focus = AUDIOFOCUS_NONE;
}

void app_ble_audio_sink_streaming_handle_event(uint8_t con_lid, uint8_t stream_lid, uint8_t direction, app_ble_audio_event_t event)
{
    TRACE(2, "d(%d)ble_audio_sink_streaming handle event = %d,0x%x", con_lid, stream_lid, event);
    BLE_AUDIO_SINK_DEVICE_T *device = app_ble_audio_get_device(con_lid);

    if (NULL == device)
    {
        ASSERT(0, "can't find ble audio device!");
    }

    device->con_lid = con_lid;
    switch (event)
    {
        case BLE_AUDIO_MUSIC_QOS_CONFIG_IND:
            /* QOS configed means le audio connected */
            app_audio_adm_le_audio_handle_action(con_lid, DEVICE_CONN_STATUS_CHANGED, STATE_CONNECTED);
            break;
        case BLE_AUDIO_CALL_QOS_CONFIG_IND:
            app_audio_adm_le_audio_handle_action(con_lid, DEVICE_CONN_STATUS_CHANGED, STATE_CONNECTED);
            break;

        /*
         * Audio focus rule:Request audio focus when enable and abandon focus when cis stop.
         * if earbud accept enable but setup cis fail,no streaming stop
         * indication,so after recv release need also abandon audio focus.
        */
        case BLE_AUDIO_MUSIC_ENABLE_REQ:
            device->music_sink_ase_lid = stream_lid;
            if (device->music_audio_focus == AUDIOFOCUS_GAIN)
            {
                aob_media_send_enable_rsp(device->music_sink_ase_lid, true);
            }
            else
            {
                if (app_ble_audio_request_audio_focus(device, AUDIOFOCUS_GAIN,
                                                      USAGE_MEDIA, ble_audio_policy_config.mute_new_music_stream) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->music_audio_focus = AUDIOFOCUS_GAIN;
                    aob_media_send_enable_rsp(device->music_sink_ase_lid, true);
                }
                else
                {
                    aob_media_send_enable_rsp(device->music_sink_ase_lid, false);
                }
            }
            break;
        case BLE_AUDIO_CALL_ENABLE_REQ:
            if (APP_GAF_DIRECTION_SINK == direction)
            {
                device->call_sink_ase_lid = stream_lid;
            }
            else
            {
                device->call_src_ase_lid = stream_lid;
            }

            /*
             * Request audio focus must before estanblish streaming,if always
             * rsp ok,the cellphone will send streaming data once receive rsp.
            */
            if (device->call_audio_focus == AUDIOFOCUS_GAIN)
            {
                aob_media_send_enable_rsp(stream_lid, true);
            }
            else
            {
                app_ble_audio_stop_ringtone(con_lid);
                // BT call is ongoing,end it
                app_audio_bt_switch_to_le_call_check(con_lid);
                if (app_ble_audio_request_audio_focus(
                            device, AUDIOFOCUS_GAIN_TRANSIENT, USAGE_CALL, false) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->call_audio_focus = AUDIOFOCUS_GAIN;
                    aob_media_send_enable_rsp(stream_lid, true);
                }
                else
                {
                    aob_media_send_enable_rsp(stream_lid, false);
                }
            }
            break;
        case BLE_AUDIO_MUSIC_RELEASE_REQ:
            app_ble_audio_abandon_audio_focus(device, USAGE_MEDIA);
            device->music_audio_focus = AUDIOFOCUS_NONE;
            break;
        case BLE_AUDIO_CALL_RELEASE_REQ:
            app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
            device->call_audio_focus = AUDIOFOCUS_NONE;
            break;
        case BLE_AUDIO_MUSIC_STREAM_START_IND:
            device->music_streaming_available = true;
            device->set_as_bg_device = false;

            if (device->music_audio_focus == AUDIOFOCUS_GAIN)
            {
                device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
                audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            }
            else
            {
                if (app_ble_audio_request_audio_focus(device, AUDIOFOCUS_GAIN, USAGE_MEDIA,
                                                      ble_audio_policy_config.mute_new_music_stream) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->music_audio_focus = AUDIOFOCUS_GAIN;
                    device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
                    audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
                }
            }

            app_audio_adm_le_audio_handle_action(con_lid, LE_MUSIC_STREAMING_CHANGED, STRAEMING_START);
            app_audio_ctrl_update_ble_audio_music_state(con_lid, STRAEMING_START);
            break;
        case BLE_AUDIO_MUSIC_STREAM_STOP_IND:
            device->music_streaming_available = false;
            audio_manager_stream_ctrl_stop_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            app_audio_adm_le_audio_handle_action(con_lid, LE_MUSIC_STREAMING_CHANGED, STRAEMING_STOP);

            /* */
            //if (!device->set_as_bg_device)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_MEDIA);
                device->music_audio_focus = AUDIOFOCUS_NONE;
            }
            app_audio_ctrl_update_ble_audio_music_state(con_lid, STRAEMING_STOP);
            break;
        case BLE_AUDIO_CALL_STREAM_START_IND:
            device->call_streaming_available = true;
            if (device->call_audio_focus == AUDIOFOCUS_GAIN)
            {
                // it's possible that down or up stream is started asynchronously, for this case,
                // when the second stream is started, the call's
                // focus has already been gained, so just send the request to let gaf audio to take
                // care which stream direction to start
                device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
                audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            }
            else
            {
                if (app_ble_audio_request_audio_focus(
                            device, AUDIOFOCUS_GAIN_TRANSIENT, USAGE_CALL, false) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->call_audio_focus = AUDIOFOCUS_GAIN;
                    device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
                    audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
                }
            }
            app_audio_adm_le_audio_handle_action(con_lid, LE_CALL_STREAMING_CHANGED, STRAEMING_START);
            break;
        case BLE_AUDIO_CALL_CAPTURE_STREAM_STOP_IND:
        case BLE_AUDIO_CALL_PLAYBACK_STREAM_STOP_IND:
        case BLE_AUDIO_CALL_SINGLE_STREAM_STOP_IND:
            audio_manager_stream_ctrl_stop_single_ble_audio_stream(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            break;
        case BLE_AUDIO_CALL_ALL_STREAMS_STOP_IND:
            /*
             * How to handle wechat behavior.When streaming stop it means call is end.
            */
            device->call_streaming_available = false;
            audio_manager_stream_ctrl_stop_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            app_audio_adm_le_audio_handle_action(con_lid, LE_CALL_STREAMING_CHANGED, STRAEMING_STOP);
            if (!aob_call_is_device_call_active(con_lid))
            {
                TRACE(0, "le_audio_policy:voip/wechat/switch to bt ... call end");
                app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
                device->call_audio_focus = AUDIOFOCUS_NONE;
            }
            break;
        case BLE_AUDIO_EVENT_ROUTE_CALL_TO_BT:
            app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
            device->call_audio_focus = AUDIOFOCUS_NONE;
            break;
        case BLE_AUDIO_CALL_RINGING_IND:
            app_ble_audio_sink_ringtone_timer_handler(device);
            break;
        case BLE_AUDIO_CALL_ACTIVE_IND:
            app_ble_audio_stop_ringtone(con_lid);
            break;
        case BLE_AUDIO_CALL_TERMINATE_IND:
            app_ble_audio_stop_ringtone(con_lid);
            app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
            device->call_audio_focus = AUDIOFOCUS_NONE;
            break;
        case BLE_AUDIO_FLEXIBLE_ENABLE_REQ:
            if (APP_GAF_DIRECTION_SINK == direction)
            {
                device->game_sink_ase_lid = stream_lid;
            }
            else
            {
                device->game_src_ase_lid = stream_lid;
            }

            if (device->game_audio_focus == AUDIOFOCUS_GAIN)
            {
                aob_media_send_enable_rsp(stream_lid, true);
            }
            else
            {
                if (app_ble_audio_request_audio_focus(
                            device, AUDIOFOCUS_GAIN, USAGE_FLEXIBLE, false) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->game_audio_focus = AUDIOFOCUS_GAIN;
                    aob_media_send_enable_rsp(stream_lid, true);
                }
                else
                {
                    aob_media_send_enable_rsp(stream_lid, false);
                }
            }
            break;
        case BLE_AUDIO_FLEXIBLE_STREAM_START_IND:
            device->game_streaming_available = true;
            if (device->game_audio_focus == AUDIOFOCUS_GAIN)
            {
                device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
                audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            }
            else
            {
                if (app_ble_audio_request_audio_focus(
                            device, AUDIOFOCUS_GAIN, USAGE_FLEXIBLE, false) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->game_audio_focus = AUDIOFOCUS_GAIN;
                    device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
                    audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);

                }
                // TODO:
                app_audio_adm_le_audio_handle_action(con_lid, LE_CALL_STREAMING_CHANGED, STRAEMING_START);
            }
            break;
        case BLE_AUDIO_FLEXIBLE_CAPTURE_STREAM_STOP_IND:
        case BLE_AUDIO_FLEXIBLE_PLAYBACK_STREAM_STOP_IND:
            audio_manager_stream_ctrl_stop_single_ble_audio_stream(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            break;
        case BLE_AUDIO_FLEXIBLE_ALL_STREAMS_STOP_IND:
            device->game_streaming_available = false;
            audio_manager_stream_ctrl_stop_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE, stream_lid);
            app_audio_adm_le_audio_handle_action(con_lid, LE_CALL_STREAMING_CHANGED, STRAEMING_STOP);
            TRACE(1, "ble_audio_policy game focus:d%x", device->game_audio_focus);
            if (device->music_audio_focus == AUDIOFOCUS_GAIN)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_MEDIA);
                device->music_audio_focus = AUDIOFOCUS_NONE;
            }
            else if (device->call_audio_focus == AUDIOFOCUS_GAIN)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
                device->call_audio_focus = AUDIOFOCUS_NONE;
            }
            else if (device->game_audio_focus == AUDIOFOCUS_GAIN)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_FLEXIBLE);
                device->game_audio_focus = AUDIOFOCUS_NONE;
            }

            // TODO: Only for dongle
            if (device->game_audio_focus != AUDIOFOCUS_LOSS_TRANSIENT)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_FLEXIBLE);
                device->game_audio_focus = AUDIOFOCUS_NONE;
            }
            break;
        case BLE_AUDIO_FLEXIBLE_RELEASE_REQ:
            break;
        case BLE_AUDIO_LE_LINK_DISCONCETED_IND:
            app_ble_audio_stop_ringtone(con_lid);
            af_codec_set_bt_sync_source(AUD_STREAM_PLAYBACK, 0);
            af_codec_set_bt_sync_source(AUD_STREAM_CAPTURE, 0);
            app_audio_adm_le_audio_handle_action(con_lid, DEVICE_CONN_STATUS_CHANGED, STATE_DISCONNECTED);

            /*
             * The Released operation shall be initiated autonomously by the server if:
             * The server has detected the loss of the LE-ACL for an ASE in any state.
             * If the server does not want to cache a codec configuration:
             * Transition the ASE to the Idle state and write a value of 0x00 (Idle) to the ASE_State field.
             *
             * The service received client Enable and request audio focus success but link loss occure,the
             * service ase state transition to idle,in this case,audio policy need to abandon audio focus.
            */
            if (device->music_audio_focus == AUDIOFOCUS_GAIN)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_MEDIA);
                device->music_audio_focus = AUDIOFOCUS_NONE;
            }
            else if (device->call_audio_focus == AUDIOFOCUS_GAIN)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_CALL);
                device->call_audio_focus = AUDIOFOCUS_NONE;
            }
            else if (device->game_audio_focus == AUDIOFOCUS_GAIN)
            {
                app_ble_audio_abandon_audio_focus(device, USAGE_FLEXIBLE);
                device->game_audio_focus = AUDIOFOCUS_NONE;
            }
            app_audio_adm_le_audio_handle_action(con_lid, DEVICE_CONN_STATUS_CHANGED, STATE_DISCONNECTED);
            break;
        case BLE_AUDIO_CALL_ALERTING_IND:
            app_audio_adm_le_audio_handle_action(device->con_lid, RINGTONE_CHANGED, RINGTONE_START);
            break;
        case BLE_AUDIO_BIS_STREAM_START_IND:
            device->music_streaming_available = true;
            device->bis_streaming_available   = true;
            if (device->music_audio_focus == AUDIOFOCUS_GAIN)
            {
                device->bis_sink_stream_lid = stream_lid;
                device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS;
                audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS, stream_lid);
            }
            else
            {
                if (app_ble_audio_request_audio_focus(device, AUDIOFOCUS_GAIN, USAGE_MEDIA,
                                                      ble_audio_policy_config.mute_new_music_stream) == AUDIOFOCUS_REQUEST_GRANTED)
                {
                    device->music_audio_focus = AUDIOFOCUS_GAIN;
                    device->current_context_type = GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS;
                    audio_manager_stream_ctrl_start_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS, stream_lid);
                }
            }

            app_audio_adm_le_audio_handle_action(con_lid, LE_MUSIC_STREAMING_CHANGED, STRAEMING_START);
            break;
        case BLE_AUDIO_BIS_STREAM_STOP_IND:
            device->music_streaming_available = false;
            device->bis_streaming_available   = false;
            audio_manager_stream_ctrl_stop_ble_audio(con_lid, GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS, stream_lid);
            app_audio_adm_le_audio_handle_action(con_lid, LE_MUSIC_STREAMING_CHANGED, STRAEMING_STOP);
            app_ble_audio_abandon_audio_focus(device, USAGE_MEDIA);
            device->music_audio_focus = AUDIOFOCUS_NONE;
            break;
        default:
            break;
    }
}

void app_ble_audio_sink_streaming_init()
{
    for (uint32_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        ble_audio_sink_device[i].con_lid = i;
        ble_audio_sink_device[i].music_streaming_available = false;
        ble_audio_sink_device[i].music_audio_focus  = AUDIOFOCUS_NONE;
        ble_audio_sink_device[i].music_sink_ase_lid = 0xFF;
        ble_audio_sink_device[i].music_focus_changed_listener = app_ble_audio_on_music_focus_changed;

        ble_audio_sink_device[i].call_audio_focus = AUDIOFOCUS_NONE;
        ble_audio_sink_device[i].call_sink_ase_lid = 0xFF;
        ble_audio_sink_device[i].call_src_ase_lid = 0xFF;
        ble_audio_sink_device[i].call_streaming_available = false;
        ble_audio_sink_device[i].call_focus_changed_listener = app_ble_audio_on_phone_call_focus_changed;

        ble_audio_sink_device[i].game_audio_focus = AUDIOFOCUS_NONE;
        ble_audio_sink_device[i].game_sink_ase_lid = 0xFF;
        ble_audio_sink_device[i].game_src_ase_lid = 0xFF;
        ble_audio_sink_device[i].game_streaming_available = false;
        ble_audio_sink_device[i].game_focus_changed_listener = app_ble_audio_on_game_focus_changed;

        ble_audio_sink_device[i].ring_focus_changed_listener = app_ble_audio_on_phone_ring_focus_changed;
        ble_audio_sink_device[i].ring_audio_focus = AUDIOFOCUS_NONE;

        ble_audio_sink_device[i].set_as_bg_device = false;

        ble_audio_sink_device[i].bis_streaming_available = false;

        osTimerInit(ble_audio_sink_device[i].ring_tone_play_timer_def, app_ble_audio_sink_ringtone_timer_handler);
        ble_audio_sink_device[i].ring_tone_play_timer = osTimerCreate(
                                                            &ble_audio_sink_device[i].ring_tone_play_timer_def.os_timer_def, osTimerOnce, (void *)&ble_audio_sink_device[i]);
    }

    app_ble_audio_policy_config();
    /// Audio manager register
    app_ble_audio_register_start_callback(app_ble_audio_sink_stream_start);
    app_ble_audio_register_stop_callback(app_ble_audio_sink_stream_stop);
    app_ble_audio_register_stop_single_stream_callback(gaf_audio_stream_refresh_and_stop);
}

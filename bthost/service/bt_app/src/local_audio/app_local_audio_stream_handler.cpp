/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "app_local_audio_stream_handler.h"
#include "app_audio_focus_control.h"
#include "audio_player_adapter.h"
#include "smf_local_play.h"
struct local_audio_addr_t {
    uint8_t address[6];
}__attribute__ ((packed));
typedef struct local_audio_addr_t local_aud_addr_t;

typedef void (*a2dp_method)(uint8_t device_id, uint16_t stream_type);
typedef struct
{
    local_aud_addr_t addr;
    uint8_t dev_id;
    int8_t a2dp_audio_focus_state;
    int8_t call_audio_focus_state;
    uint8_t audio_type;
    AUDIO_USAGE_TYPE_E stream_type;
    uint8_t focus_request_type;
    bool allow_delay;
    af_ext_policy ext_policy;
    on_audio_focus_change_listener focus_changed_listener;
    a2dp_method a2dp_play;
    a2dp_method a2dp_stop;
    // a2dp play method
    // a2dp stop method
}LOCAL_AUDIO_SOURCE_DEVICE_T;

LOCAL_AUDIO_SOURCE_DEVICE_T local_mp3_audio;

bool is_enter_mp3_mode_succ = false;

static void app_local_mp3_player_play(uint8_t device_id, uint16_t stream_type)
{
    TRACE(0, "app_local_mp3_player_play start");
    smf_local_player_config();
    smf_local_player_start(0);

}

static void app_local_mp3_player_stop(uint8_t device_id, uint16_t stream_type)
{
    TRACE(0, "app_local_mp3_player_stop start");
    smf_local_player_stop(0);
}

void app_local_mp3_a2dp_audio_focus_change(uint8_t device_id,AUDIO_USAGE_TYPE_E media_type,int audio_focus_change)
{
    LOCAL_AUDIO_SOURCE_DEVICE_T *device = &local_mp3_audio;
    switch (audio_focus_change)
    {
        case AUDIOFOCUS_GAIN:
            device->a2dp_audio_focus_state = AUDIOFOCUS_GAIN;
            device->a2dp_play(device->dev_id, AUDIO_STREAM_MUSIC);
            break;
        case AUDIOFOCUS_LOSS:
            device->a2dp_audio_focus_state = AUDIOFOCUS_LOSS;
            device->a2dp_stop(device->dev_id, AUDIO_STREAM_MUSIC);
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT:
            device->a2dp_audio_focus_state = AUDIOFOCUS_LOSS_TRANSIENT;
            device->a2dp_stop(device->dev_id, AUDIO_STREAM_MUSIC);
            break;
        case AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
            break;
        default:
            break;
    }
}

static int app_local_mp3_player_ext_policy(focus_device_info_t* cdevice_info, focus_device_info_t *rdevice_info)
{
    if (cdevice_info == NULL || rdevice_info == NULL)
    {
        TRACE(0, "cdevice_info %p rdevice_info %p", cdevice_info, rdevice_info);
        return AUDIOFOCUS_REQUEST_FAILED;
    }

    TRACE(0, "_mp3_player_ext_policy req audio_type 0x%x stream_type 0x%x", rdevice_info->audio_type, rdevice_info->stream_type);
    if( rdevice_info->audio_type == AUDIO_TYPE_BT)
    {
        if (rdevice_info->stream_type == USAGE_CALL)
        {
            TRACE(0, "call can prompt local mp3");
            return AUDIOFOCUS_REQUEST_GRANTED;
        }

        if (rdevice_info->stream_type == USAGE_MEDIA)
        {
            TRACE(0, "music cant prompt local mp3");
            return AUDIOFOCUS_REQUEST_DELAYED;
        }
        TRACE(0, "stream_type error");
        return AUDIOFOCUS_REQUEST_FAILED;
    }
    else
    {
        TRACE(0, "NOT SUPPORT LE");
        return AUDIOFOCUS_REQUEST_FAILED;
    }
}

static void app_local_mp3_player_init(void)
{
    if (is_enter_mp3_mode_succ == true)
    {
        return;
    }
    TRACE(0, "app_local_mp3_player_init");
    local_mp3_audio.addr = {0};
    local_mp3_audio.dev_id = 0xEF;
    local_mp3_audio.a2dp_audio_focus_state = AUDIOFOCUS_NONE;
    local_mp3_audio.call_audio_focus_state = AUDIOFOCUS_NONE;
    local_mp3_audio.audio_type = AUDIO_TYPE_LOCAL_AUDIO;
    local_mp3_audio.stream_type = USAGE_MEDIA;
    local_mp3_audio.focus_request_type = AUDIOFOCUS_GAIN;
    local_mp3_audio.a2dp_play = app_local_mp3_player_play;
    local_mp3_audio.a2dp_stop = app_local_mp3_player_stop;
    local_mp3_audio.focus_changed_listener = app_local_mp3_a2dp_audio_focus_change;
    local_mp3_audio.ext_policy = app_local_mp3_player_ext_policy;
}

static int app_local_audio_request_audio_focus(LOCAL_AUDIO_SOURCE_DEVICE_T *device)
{
    audio_focus_req_info_t request_info;
    int ret;

    memcpy((uint8_t*)&request_info.device_info.device_addr.address[0], (void*)&(device->addr), sizeof(local_audio_addr_t));
    request_info.device_info.device_idx = device->dev_id;
    request_info.device_info.audio_type = device->audio_type;
    request_info.device_info.focus_request_type = device->focus_request_type;
    request_info.device_info.stream_type = device->stream_type;
    request_info.device_info.delayed_focus_allow = device->allow_delay;
    request_info.focus_changed_listener = device->focus_changed_listener;
    request_info.ext_policy = device->ext_policy;

    ret = app_audio_request_focus(&request_info);

    TRACE(0, "ret %d", ret);

    request_info.focus_changed_listener(device->dev_id, device->stream_type, ret);
    return ret;
}

void app_local_mp3_audio_request_to_play(void)
{
    if (is_enter_mp3_mode_succ == true)
    {
        TRACE(0, "enter mp3 mode repeatly");
        return;
    }

    audio_focus_req_info_t* top_focus =  app_audio_focus_ctrl_stack_top();

    if (top_focus != NULL && top_focus->device_info.stream_type == USAGE_CALL)
    {
        TRACE(0, "d(%d) CALL going, cant enter mp3 mode", top_focus->device_info.device_idx);
        return;
    }

    app_local_mp3_player_init();
    if (app_local_audio_request_audio_focus(&local_mp3_audio) == AUDIOFOCUS_REQUEST_GRANTED)
    {
        TRACE(0, "enter mp3 mode success");
        is_enter_mp3_mode_succ = true;
    }

    return;
}

static void app_local_audio_abandon_audio_focus(LOCAL_AUDIO_SOURCE_DEVICE_T *device)
{
    audio_focus_req_info_t removed_fouces;

    memcpy((uint8_t*)&removed_fouces.device_info.device_addr, (void*)&(device->addr), sizeof(local_audio_addr_t));
    removed_fouces.device_info.device_idx = device->dev_id;
    removed_fouces.device_info.audio_type = device->audio_type;
    removed_fouces.device_info.stream_type = device->stream_type;
    removed_fouces.focus_changed_listener = device->focus_changed_listener;
    removed_fouces.ext_policy = device->ext_policy;
    device->focus_changed_listener(device->dev_id, device->stream_type, AUDIOFOCUS_LOSS);
    app_audio_abandon_focus(&removed_fouces);
}


void app_local_mp3_audio_request_to_stop(void)
{
    if (is_enter_mp3_mode_succ == false)
    {
        TRACE(0, "exit mp3 mode repeatly");
        return;
    }
    app_local_audio_abandon_audio_focus(&local_mp3_audio);
    is_enter_mp3_mode_succ = false;

    TRACE(0, "app_local_mp3_audio_request_to_stop");

    return;
}
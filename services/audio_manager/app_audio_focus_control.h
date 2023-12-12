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
#ifndef __AUDIO_FOCUS_CONTROL_H__
#define __AUDIO_FOCUS_CONTROL_H__

/* Used to indicate no audio focus has been gained or lost, or requested */
#define    AUDIOFOCUS_NONE                      0
/* Used to indicate a gain of audio focus. */
#define    AUDIOFOCUS_GAIN                      1
/**
 * Used to indicate a temporary gain or request of audio focus, anticipated to last a short
 * amount of time. Examples of temporary changes are the playback of driving directions, or an
 * event notification.
 */
#define    AUDIOFOCUS_GAIN_TRANSIENT            2
/* Used to indicate a loss of audio focus of unknown duration.*/

#define  AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK     3


#define    AUDIOFOCUS_LOSS                     -1

/* sed to indicate a transient loss of audio focus. */
#define    AUDIOFOCUS_LOSS_TRANSIENT           -2
/**
* Used to indicate a transient loss of audio focus where the loser of the audio focus can
* lower its output volume if it wants to continue playing (also referred to as "ducking"), as
* the new focus owner doesn't require others to be silent.
*/
#define    AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK  -3

typedef enum
{
    /* A failed focus change request.*/
    AUDIOFOCUS_REQUEST_FAILED   =     0,
    /*A successful focus change request.*/
    AUDIOFOCUS_REQUEST_GRANTED  =     1,
    /**
    * A focus change request whose granting is delayed: the request was successful, but the
    * requester will only be granted audio focus once the condition that prevented immediate
    * granting has ended.
    */
    AUDIOFOCUS_REQUEST_DELAYED  =     2,
}AUDIO_FOCUS_REQ_RESULT_E;

typedef enum
{
    USAGE_UNKNOWN        = 0,
    /* Usage value to use when the usage is media, such as music.*/
    USAGE_MEDIA          = 1,
    /* Usage value to use when the usage is voice communications, such as AI VOICE */
    USAGE_AI_VOICE       = 2,
    USAGE_CALL           = 3,
    USAGE_RINGTONE       = 4,
    USAGE_FLEXIBLE       = 5,
}AUDIO_USAGE_TYPE_E;

typedef struct
{
    bt_bdaddr_t device_addr;
    uint8_t device_idx;
    uint8_t audio_type;
    uint8_t focus_request_type;
    AUDIO_USAGE_TYPE_E stream_type; 
    bool delayed_focus_allow;
}focus_device_info_t;

typedef void (*on_audio_focus_change_listener)(uint8_t device_id,AUDIO_USAGE_TYPE_E media_type,int audio_focus_change);
typedef int (*af_ext_policy)(focus_device_info_t* cdevice_info, focus_device_info_t *rdevice_info);
typedef void(*audio_focus_empty_search_stream)(uint8_t device_id);

typedef struct
{
    focus_device_info_t device_info;
    af_ext_policy ext_policy;
    on_audio_focus_change_listener focus_changed_listener;
}audio_focus_req_info_t;

/**
 ****************************************************************************************
 * @brief Initialize the audio focus module.
 *
 * @param[in] big_idx          BIS Group(BIG) local index
 * @param[in] is_encrypted     0:Not encrypted, !=0:encrypted
 * @param[in] bcast_code       Broadcast Code, @see app_gaf_bc_code_t, only meaningful when is_encrypted != 0
 *
 ****************************************************************************************
 */
void app_audio_focus_init();

/**
 ****************************************************************************************
 * @brief requst audio focus to play audio
 *
 * @param[in] requester      request audio focus info
 *
 ****************************************************************************************
 */
int app_audio_request_focus(audio_focus_req_info_t* requester);

/**
 ****************************************************************************************
 * @brief abandon specified audio focus(remove from audio focus stack)
 *
 * @param[in] requester      audio focus info
 *
 ****************************************************************************************
 */
bool app_audio_abandon_focus(audio_focus_req_info_t* requester);

/**
 ****************************************************************************************
 * @brief hands over audio focus to the src which under the stack top
 *
 * @param[in] requester      audio focus info
 *
 ****************************************************************************************
 */
bool app_audio_hands_over_audio_focus(audio_focus_req_info_t* focus_info);

/**
 ****************************************************************************************
 * @brief judge current device is playing source
 *
 * @param[in] device_type,ibrt or ble audio
 * @param[in] device_id
 * @param[in] media_type
 *
 ****************************************************************************************
 */
bool app_audio_is_this_curr_play_src(uint8_t device_type,uint8_t device_id,AUDIO_USAGE_TYPE_E media_type);

const char* app_audio_focus_change_to_string(int focus);

const char* app_audio_media_type_to_string(AUDIO_USAGE_TYPE_E media_type);

const char* app_audio_audio_type_to_string(uint8_t audio_type);

AUDIO_USAGE_TYPE_E app_audio_get_curr_audio_stream_type(uint8_t device_type,uint8_t device_id);

int app_audio_focus_ctrl_stack_length();

audio_focus_req_info_t* app_audio_focus_ctrl_stack_top();

AUDIO_USAGE_TYPE_E app_audio_get_curr_audio_focus_type();

void app_audio_focus_insert_blow_top_focus(audio_focus_req_info_t* requester);

audio_focus_req_info_t* app_audio_get_curr_audio_focus();

int app_audio_switch_sco_audio_focus_get(audio_focus_req_info_t* requester);

void app_af_empty_search_init(audio_focus_empty_search_stream af_search_valid_stream);

uint16_t app_audio_focus_get_device_info(uint8_t *buff);

int app_audio_handle_peer_focus(audio_focus_req_info_t* requester, bool top);

int app_audio_request_switch_focus(audio_focus_req_info_t* requester);

void app_audio_notify_focus_changed(audio_focus_req_info_t* focus, int audio_focus_change);
#endif /* __AUDIO_FOCUS_CONTROL_H__ */

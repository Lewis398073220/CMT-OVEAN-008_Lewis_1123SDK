/**
 * @copyright Copyright (c) 2015-20223 BES Technic.
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
#ifndef APP_BIS_TRANSPOND_MEDIA_H_
#define APP_BIS_TRANSPOND_MEDIA_H_

/*****************************header include********************************/


/*********************external function declaration*************************/


/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declaration*************************/
typedef struct
{
    // pcm sampling param
    bool     only_af;
    bool     external_trig;
    uint8_t  bits_depth;
    uint8_t  channel_num;
    uint16_t volume_level;
    uint32_t sample_rate;
    uint32_t duration_ms;
    void     (*triggered_cb)(void);
    void     (*play_read_data_cb)(uint8_t *data, uint32_t data_len);
} app_bis_tran_media_param_t;
/************************private variable defination************************/
void app_bis_tran_media_start(app_bis_tran_media_param_t *audio_info);

void app_bis_tran_media_stop();

void app_bis_tran_media_audio_trigger(uint32_t expected_play_time);

void app_bis_tran_media_audio_set_volume(uint8_t volume);

uint32_t app_bis_tran_media_audio_get_start_play_time();

#endif  //APP_BIS_TRANSPOND_MEDIA_H_
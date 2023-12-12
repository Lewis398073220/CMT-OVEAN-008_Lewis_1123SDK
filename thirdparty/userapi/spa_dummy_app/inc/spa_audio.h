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
#ifndef __SPA_AUDIO_H__
#define __SPA_AUDIO_H__

#include "spa_common.h"

typedef data_info_body_t spa_audio_codec_cfg_t;

typedef struct{
    unsigned int conf_id;
    unsigned char * conf_params_ptr;
    unsigned int conf_params_size;
    spa_data_update_source_e conf_source;;
    unsigned int conf_opt;
    spa_audio_chan_id_e chan_id; 
}audio_algo_setting_conf_t;

void app_spatial_audio_pcm_data_send_to_algo_thread(unsigned char* pcm_in, unsigned char *pcm_out,unsigned short pcm_in_length,unsigned short pcm_out_length);
void app_spatial_audio_conf_notify_update(audio_algo_setting_conf_t * cfg);
void spatial_audio_algo_init(data_info_body_t *config,ext_alloc_func_t alloc);
void spatial_audio_algo_deinit(void);
void spatial_audio_codec_config(spa_audio_codec_cfg_t *cfg);
#endif

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

#include "hal_trace.h"
#include "hal_timer.h"
#include "spa_common.h"
#include "spa_audio.h"
#include "spa_ctrl.h"
#include "spa_ext_tws_handler.h"
#include "spa_ext_m55_handler.h"
#include "spa_app_main.h"

static int spatial_audio_algo_post_handler(void * buf,unsigned int len)
{
    data_aud_buffer_t * buf_ptr = &(((data_aud_t*)buf)->payload.buffer);
#if defined(DSP_M55)
    app_spa_audio_aud_data_snd((unsigned char*)buf_ptr,sizeof(data_aud_buffer_t));
#else
    app_spatial_audio_pcm_data_send_to_algo_thread(buf_ptr->data_ptr,buf_ptr->data_ptr,buf_ptr->data_size,buf_ptr->data_size);
#endif
    return 0;
}

static int spatial_audio_relay_handler(void * buf,unsigned int len)
{
#if defined(SPA_AUDIO_BTH)
    app_spa_tws_sens_data_sync_snd((unsigned char *)buf,len);
#endif
    return 0;
}

static void spatial_audio_app_algo_init(data_info_body_t *config,ext_alloc_func_t alloc)
{
    spatial_audio_algo_init(config,alloc);
#if defined(DSP_M55)
    app_spa_audio_codec_config_snd(config,sizeof(spa_audio_codec_cfg_t));
#endif
}

static void spatial_audio_app_algo_deinit(void)
{
    spatial_audio_algo_deinit();
}

static const spa_algo_op_handler_t algo_op_handler_instant = 
{
    .func_algo_run = spatial_audio_algo_post_handler,
    .func_relay = spatial_audio_relay_handler,
};

void spatial_audio_app_init_if(unsigned int sample_rate, unsigned char sample_bits, unsigned char channel, unsigned int frame_len,void * alloc_func,unsigned char role,unsigned char chan_id)
{
    data_info_body_t info;
    info.data_channel = channel;
    info.sample_rate = sample_rate;
    info.data_bits =sample_bits;
    info.block_samples = frame_len;

    TRACE(0,"%s info: chnl %d fs %d bits %d blk_samps %d, role %d, ch_id %d",__func__,info.data_channel,info.sample_rate,info.data_bits,info.block_samples, role, chan_id);

    spa_ctrl_create(&info,(ext_alloc_func_t )alloc_func,&algo_op_handler_instant,(spa_algo_op_role_e)role);

    spatial_audio_app_algo_init(&info,(ext_alloc_func_t)alloc_func);

    audio_algo_setting_conf_t cfg = {0};
    cfg.conf_source = SPA_DATA_UPDATE_SOURCE_BTH;
    cfg.chan_id = (spa_audio_chan_id_e)chan_id;
    app_spatial_audio_conf_notify_update(&cfg);
}

void spatial_audio_app_deinit_if(void)
{
    spatial_audio_app_algo_deinit();
}

void spatial_audio_app_audio_conf_udpate_if(unsigned char id, unsigned char * param_ptr, unsigned int param_size)
{
    audio_algo_setting_conf_t cfg = {0};
    cfg.conf_source = SPA_DATA_UPDATE_SOURCE_TUNE;
    if (param_ptr == NULL) {
        cfg.conf_id = id;
        cfg.conf_params_ptr = NULL;
        cfg.conf_params_size = 0;
    } else {
        cfg.conf_params_ptr = param_ptr;
        cfg.conf_params_size = param_size;
    }

    app_spatial_audio_conf_notify_update(&cfg);
}

void spatial_audio_app_run_if(unsigned char * data_ptr, int data_size, unsigned char role)
{
    spa_ctrl_proc(data_ptr,data_size,NULL,(spa_algo_op_role_e)role);
}

void spatial_audio_app_tws_role_update_if(unsigned char role)
{
    spa_ctrl_update_cfg_op_role((spa_algo_op_role_e)role);
}

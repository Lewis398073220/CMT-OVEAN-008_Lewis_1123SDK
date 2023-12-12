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
#include "spa_common.h"
#ifdef DSP_M55
#include "app_dsp_m55.h"
#endif
#include "spa_ctrl.h"
#include "spa_audio.h"
#include "hal_aud.h"

#if defined(SPA_AUDIO_BTH) && defined(DSP_M55)

void app_spa_audio_aud_data_snd(unsigned char * pmsg, int32_t opt)
{
    ASSERT(pmsg != NULL, "%s Invalid ptr",__func__);

    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_TASK_CMD_SPA_DATA_SYNC,pmsg,opt);
}

static void app_spa_audio_aud_data_snd_handler(unsigned char * ptr, unsigned short len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(MCU_DSP_M55_TASK_CMD_SPA_DATA_SYNC,ptr,len);
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_SPA_DATA_SYNC,
                                "spa-audio-aud-data-sync",
                                app_spa_audio_aud_data_snd_handler,
                                NULL,
                                0,
                                NULL,
                                NULL,
                                NULL);

void app_spa_audio_codec_config_snd(spa_audio_codec_cfg_t* config, unsigned short length)
{
    ASSERT(config && length, "%s invalid codec config", __func__);

    TRACE(1, "send spa codec config:");
    TRACE(1, "*** bits        :%d ***", config->data_bits);
    TRACE(1, "*** frame size  :%d ***", config->block_samples);
    TRACE(1, "*** sample rate :%d ***", config->sample_rate);
    TRACE(1, "*** channel num :%d ***", config->data_channel);

    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_TASK_CMD_SPA_CODEC_CFG, (unsigned char*)config, length);
}

static void app_spa_audio_codec_config_snd_handler(unsigned char * ptr, unsigned short len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(MCU_DSP_M55_TASK_CMD_SPA_CODEC_CFG,ptr,len);
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_SPA_CODEC_CFG,
                                    "spa audio  codec config",
                                    app_spa_audio_codec_config_snd_handler,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL);

#endif /* SPA_AUDIO_BTH */

#if defined(SPA_AUDIO_M55)

void app_spa_audio_data_rcv_handler(unsigned char * ptr, unsigned short len)
{
    // msg_header_t * pmsg = (msg_header_t*)ptr;
    data_aud_buffer_t *buffer_ptr = (data_aud_buffer_t*)ptr;
    ASSERT(ptr != NULL, "%s Invalid ptr",__func__);
    // ASSERT(len == pmsg->len, "%s Invalid ptr %d %d ",__func__,len,pmsg->len);

    // spa_ctrl_update_sens_data(ptr, pmsg->len);
    //spa_ctrl_proc(buffer_ptr->data_ptr,buffer_ptr->data_size,NULL); 
    spa_ctrl_proc(buffer_ptr->data_ptr, buffer_ptr->data_size, NULL, buffer_ptr->algo_op_role); //note add role
    // app_dsp_m55_bridge_send_rsp(MCU_DSP_M55_TASK_CMD_SPA_DATA_SYNC, (unsigned char*)&pcm_rsp, sizeof(pcm_rsp));
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_SPA_DATA_SYNC,
                                "spa-audio-aud-data-sync",
                                NULL,
                                app_spa_audio_data_rcv_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);

static void app_spa_audio_codec_config_rcv_handler(unsigned char *config, unsigned short length)
{
    ASSERT(config, "%s received invalid codec config", __func__);
    ASSERT(length > 0 && length <= sizeof(spa_audio_codec_cfg_t), "%s invalid codec config length", __func__);
    
    spa_audio_codec_cfg_t* codec_info_ptr = (spa_audio_codec_cfg_t*)config;

    TRACE(1, "recv bth codec config:");
    TRACE(1, "*** bits        :%d ***", codec_info_ptr->data_bits);
    TRACE(1, "*** frame size  :%d ***", codec_info_ptr->block_samples);
    TRACE(1, "*** sample rate :%d ***", codec_info_ptr->sample_rate);
    TRACE(1, "*** channel num :%d ***", codec_info_ptr->data_channel);

    ASSERT(codec_info_ptr->data_bits == AUD_BITS_16 || 
           codec_info_ptr->data_bits == AUD_BITS_24 , 
           "invalid codec bits");
    
    ASSERT(codec_info_ptr->data_channel== AUD_CHANNEL_NUM_1 || 
           codec_info_ptr->data_channel == AUD_CHANNEL_NUM_2 , 
           "invalid codec channel num");    

    spatial_audio_codec_config(codec_info_ptr);
    spa_ctrl_update_cfg_data_info(config,length);
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_SPA_CODEC_CFG,
                                    "spa audio  codec config",
                                    NULL,
                                    app_spa_audio_codec_config_rcv_handler,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL);

#endif /* SPA_AUDIO_M55 */
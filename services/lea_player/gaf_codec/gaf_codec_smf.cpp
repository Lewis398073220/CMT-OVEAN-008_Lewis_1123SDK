/**
 * @file gaf_codec_smf.cpp
 * @author BES AI team
 * @version 0.1
 * @date 2020-08-31
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

/*****************************header include********************************/
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "app_trace_rx.h"
#include "app_utils.h"
#include "plat_types.h"
#include "app_gaf_dbg.h"
#include "audio_dump.h"
#include "app_overlay.h"
#include "hal_location.h"
#include "gaf_codec_common.h"
#include "cc_stream_common.h"

#include "smf_codec_lc3.h"
#include "smf_debug.h"

#ifdef BLE_SMF
using namespace smf;
/*********************external function declaration*************************/


/************************private macro defination***************************/
#define AOB_CALCULATE_CODEC_MIPS       0
//#define LC3_AUDIO_UPLOAD_DUMP          0
//#define LC3_AUDIO_DOWNLOAD_DUMP        0
//#undef dbg_test_printf
//#define dbg_test_printf(...)

/************************private type defination****************************/
typedef struct{
    const char* codec;
    CODEC_INFO_T *codec_info;
    void* handle;
    //void* pool_buff;
    int sample_align;
    void* vlc_frame_buff;
    int frame_samples;
    int pcm_frame_size;
    int vlc_frame_size;
    int vlc_frame_max;
} app_aob_smf_t;

/**********************private function declaration*************************/

/************************private variable defination************************/
app_aob_smf_t smf_decoder;
app_aob_smf_t smf_encoder;

CC_HEAP_ALLOC buf_alloc = NULL;

/****************************function defination****************************/
static void* lc3_alloc(unsigned size){
    ASSERT(buf_alloc != NULL, "%s", __func__);
    void *data=0;
    data = buf_alloc(NULL, size);
    dbgTestPXL("%p,%d",data,size);
    return data;
}

static void lc3_free(void* data){
    dbgTestPXL("%p",data);
    return ;
}

static void ctx_init(app_aob_smf_t *ctx){
    auto codec_info = ctx->codec_info;
    ctx->sample_align = codec_info->bits_depth;
    if(codec_info->bits_depth==24)
        ctx->sample_align=32;
    ctx->frame_samples = (int)(codec_info->sample_rate*codec_info->frame_ms/1000);
    if(codec_info->sample_rate==44100)
        ctx->frame_samples = (int)(48000*codec_info->frame_ms/1000);
    ctx->pcm_frame_size = (int)(ctx->frame_samples*codec_info->num_channels*ctx->sample_align/8);
    ctx->vlc_frame_size = (int)(codec_info->frame_size*codec_info->num_channels);
    ctx->vlc_frame_max = ctx->vlc_frame_size<<1;
    //if(ctx->vlc_frame_max> LC3_MAX_FRAME_SIZE)
    //    ctx->vlc_frame_max= LC3_MAX_FRAME_SIZE;

    dbgLogPPL(ctx->codec);
    dbgLogPSL(ctx->codec);
    dbgLogPDL(ctx->sample_align);
    dbgLogPDL(ctx->frame_samples);
    dbgLogPDL(ctx->vlc_frame_size);
    dbgLogPDL(ctx->vlc_frame_max);
    dbgLogPDL(ctx->pcm_frame_size);
}

static void media_init(app_aob_smf_t *ctx,smf_media_info_t* media){
    dbgTestPPL(ctx);
    auto codec_info = ctx->codec_info;
    //
    media->codec = fcc64(ctx->codec);
    media->audio.sample_rate = codec_info->sample_rate;
    media->audio.channels = codec_info->num_channels;
    media->audio.sample_bits = codec_info->bits_depth;
    media->audio.sample_width = (ctx->sample_align+7)>>3;
    media->frame_dms = (int)(codec_info->frame_ms*10.f);
    media->frame_size = ctx->vlc_frame_size;
    media->audio.frame_samples = ctx->frame_samples;
    //media->is_non_interlace = false;

    dbgLogPDL(media->audio.sample_rate);
    dbgLogPDL(media->audio.channels);
    dbgLogPDL(media->audio.sample_bits);
    dbgLogPDL(media->audio.sample_width);
    dbgLogPDL(media->frame_dms);
    dbgLogPDL(media->frame_size);

}

static bool media_update(app_aob_smf_t *ctx,smf_media_info_t* media){
    dbgTestPPL(ctx);
    auto hd = ctx->handle;
    memset(media,0,sizeof(smf_media_info_t));
    returnIfErrC(false,!smf_get(hd,SMF_GET_MEDIA_INFO,media));
    ctx->frame_samples = media->audio.frame_samples;
    ctx->pcm_frame_size = media->audio.frame_samples*
        media->audio.channels*media->audio.sample_width;
    ctx->vlc_frame_size = media->frame_size;
    return true;
}

static bool decoder_media_init(app_aob_smf_t *ctx,void* param){
    dbgTestPPL(ctx);
    media_init(ctx,(smf_media_info_t*)param);

    switch(fcc64(ctx->codec)){
    case fcc64("lc3"):{
        LOG_D("[LC3]decode");
        auto lc3 = (smf_lc3_dec_open_param_t*)param;
        lc3->plcMeth    = SMF_LC3_PLCMODE_ADVANCED;
        lc3->epmode     = SMF_LC3_EPMODE_OFF;
        lc3->hrmode     = false;
        break;
    }
    default:
        dbgErrPXL("unknown codec:%s",ctx->codec);
        break;
    }

    return true;
}

static bool decoder_media_update(app_aob_smf_t *ctx){
    dbgTestPPL(ctx);
    smf_media_info_t media;
    media_update(ctx,&media);
    return true;
}

static void decoder_stream_init(app_aob_smf_t *ctx){
    dbgTestPPL(ctx);
    //overlay load by smf
    //app_overlay_select(ctx->overlay);
    LOG_I("%s", __FUNCTION__);
    //AOB_CALCULATE_CODEC_MIPS
    //memset(&aob_decode_time_info, 0, sizeof(aob_codec_time_info_t));
    //
    if(ctx->handle){
        smf_destroy(ctx->handle);
        ctx->handle = 0;
    }
    //
    auto hd = smf_create_decoder(ctx->codec);
#ifdef GAF_DSP
    if(!smf_set(hd,smf::Hash("Wait"),(void*)FIFO_MODE))
    dbgTestPXL("smf set fifo_mode error");
#endif
    returnIfErrC0(!hd);
    ctx->handle = hd;
    //runtime pool
    smf_register_pool_with_callback(hd, &lc3_alloc, &lc3_free);
    /*int poolsize = 0;
    smf_get(hd,SMF_GET_DYNAMIC_MEMOEY_SIZE_MAX,&poolsize);
    if(poolsize){
        dbgTestPDL(poolsize);
        if(!ctx->pool_buff){
            app_audio_mempool_get_buff((uint8_t**)&ctx->pool_buff, poolsize);
            returnIfErrC0(!ctx->pool_buff);
        }
        dbgTestPPL(ctx->pool_buff);
        if(ctx->pool_buff){
            returnIfErrC0(!smf_register_pool_with_buffer(hd,ctx->pool_buff,poolsize));
        }
    }*/
    //open
    void* param = smf_alloc_open_param(hd);
    ExitFree ep(hd,param);
    returnIfErrC0(!param);
    returnIfErrC0(!decoder_media_init(ctx,param));
    returnIfErrC0(!smf_open(hd,param));
    //update media params
    decoder_media_update(ctx);

}

static void decoder_stream_deinit(app_aob_smf_t *ctx){
    dbgTestPPL(ctx);
    void*& hd = ctx->handle;
    if(hd){
        smf_destroy(hd);
        ctx->handle = 0;
    }
}

static void decoder_buf_init(uint8_t instance_handle, void *_codec_info, void *alloc_cb)
{
    dbgTestPDL(instance_handle);
    if (NULL == buf_alloc) {
        buf_alloc = (CC_HEAP_ALLOC)alloc_cb;
    }
}

static void decoder_init(uint8_t instance_handle, void *_codec_info)
{
    LOG_D("%s", __func__);
    dbgTestPL();
    app_aob_smf_t* ctx = &smf_decoder;
    memset(ctx,0,sizeof(app_aob_smf_t));
    ctx->codec = "lc3";
    ctx->codec_info = (CODEC_INFO_T*)_codec_info;
    ctx_init(ctx);
    decoder_stream_init(ctx);
    LOG_D("%s end", __func__);
}

static void decoder_deinit(uint8_t instance_handle)
{
    dbgTestPL();
    app_overlay_unloadall();

    LOG_D("%s", __FUNCTION__);
}

static int decode(uint8_t instance_handle, void *codec_info,
            uint32_t inputDataLength, void *input, void *output, bool isPlc)
{
    app_aob_smf_t* ctx = &smf_decoder;
    //dbgTestPXL("%d->%d",inputDataLength,ctx->pcm_frame_size);
    void* hd = ctx->handle;
    returnIfErrC(-1,!hd);
    smf_frame_t ifrm;
    memset(&ifrm, 0, sizeof(smf_frame_t));
    ifrm.buff = input;
    ifrm.max = inputDataLength;
    ifrm.offset = 0;
    ifrm.size = inputDataLength;
    ifrm.flags = 0;
    smf_frame_t ofrm;
    memset(&ofrm, 0, sizeof(smf_frame_t));
    ofrm.buff = output;
    ofrm.max = ctx->pcm_frame_size;
    ofrm.offset = 0;
    ofrm.size = 0;
    ofrm.flags = 0;
    if(!smf_decode(hd,&ifrm,&ofrm)){
        dbgErrPPL(ctx->codec);
        dbgErrPSL(ctx->codec);
        dbgErrPPL(ctx->vlc_frame_buff);
        dbgErrPDL(ctx->frame_samples);
        dbgErrPDL(ctx->pcm_frame_size);
        dbgErrPDL(ctx->vlc_frame_size);
        smf_print_error(hd);
        return smf_get_err32(hd);
    }
    return 0;
}

static void decoder_buf_deinit(uint8_t instance_handle)
{
    dbgTestPDL(instance_handle);
    app_aob_smf_t* ctx = &smf_decoder;
    decoder_stream_deinit(ctx);
}

static uint32_t decoder_set_freq(void *_codec_info, uint32_t base_freq, uint32_t core_type)
{
    return APP_SYSFREQ_208M;
}

static GAF_DECODER_FUNC_LIST_T gaf_audio_smf_decoder_func_list =
{
    .decoder_init_buf_func = decoder_buf_init,
    .decoder_init_func = decoder_init,
    .decoder_deinit_func = decoder_deinit,
    .decoder_decode_frame_func = decode,
    .decoder_deinit_buf_func = decoder_buf_deinit,
    .decoder_set_freq = decoder_set_freq,
};

static bool encoder_media_init(app_aob_smf_t *ctx,void* param){
    dbgTestPPL(ctx);
    media_init(ctx,(smf_media_info_t*)param);

    switch(fcc64(ctx->codec)){
    case fcc64("lc3"):{
        LOG_I("[lc3]encoder");
        auto lc3 = (smf_lc3_enc_open_param_t*)param;
        lc3->epmode     = SMF_LC3_EPMODE_OFF;
        lc3->hrmode     = false;
        break;
    }
    default:
        dbgErrPXL("unknown codec:%s",ctx->codec);
        break;
    }

    return true;
}
static bool encoder_media_update(app_aob_smf_t *ctx){
    dbgTestPPL(ctx);
    smf_media_info_t media;
    media_update(ctx,&media);

    return true;
}

static void encoder_stream_init(app_aob_smf_t *ctx)
{
    dbgTestPPL(ctx);
    //overlay load by smf
    //app_overlay_select(ctx->overlay);
    LOG_I("%s", __FUNCTION__);
    //AOB_CALCULATE_CODEC_MIPS
    //memset(&aob_decode_time_info, 0, sizeof(aob_codec_time_info_t));
    if(ctx->handle){
        smf_destroy(ctx->handle);
        ctx->handle = 0;
    }

    auto hd = smf_create_encoder(ctx->codec);
    TRACE(0, "hd:%p", hd);
#ifdef GAF_DSP
    if(!smf_set(hd,smf::Hash("Wait"),(void*)FIFO_MODE))
    dbgTestPXL("smf set fifo_mode error");
#endif
    returnIfErrC0(!hd);
    ctx->handle = hd;
    //encode vlc buff
    ctx->vlc_frame_buff = lc3_alloc(ctx->vlc_frame_max);
    returnIfErrC0(!ctx->vlc_frame_buff);
    memset(ctx->vlc_frame_buff,0,ctx->vlc_frame_max);
    //runtime pool
    smf_register_pool_with_callback(hd, &lc3_alloc, &lc3_free);
    /*int poolsize = 0;
    smf_get(hd,SMF_GET_DYNAMIC_MEMOEY_SIZE_MAX,&poolsize);
    if(poolsize){
        if(!ctx->pool_buff){
            app_audio_mempool_get_buff((uint8_t**)&ctx->pool_buff, poolsize);
            returnIfErrC0(!ctx->pool_buff);
        }
        dbgTestPPL(ctx->pool_buff);
        if(ctx->pool_buff){
            returnIfErrC0(!smf_register_pool_with_buffer(hd,ctx->pool_buff,poolsize));
        }
    }*/

    //open
    void* param = ctx->vlc_frame_buff;//smf_alloc_open_param(hd);
    //ExitFree ep(hd,param);
    returnIfErrC0(!param);
    returnIfErrC0(!encoder_media_init(ctx,param));
    returnIfErrC0(!smf_open(hd,param));
    //update media params
    encoder_media_update(ctx);

}

static void encoder_stream_deinit(app_aob_smf_t *ctx)
{
    dbgTestPPL(ctx);
    void*& hd = ctx->handle;
    if(hd){
        smf_destroy(hd);
        ctx->handle = 0;
    }
    if(ctx->vlc_frame_buff){
        lc3_free(ctx->vlc_frame_buff);
        ctx->vlc_frame_buff = 0;
    }
}

static void encoder_buf_init(uint8_t instance_handle, void *_codec_info, void *alloc_cb)
{
    dbgTestPL();
    //Capture should always be less than 1
    if (NULL == buf_alloc) {
        buf_alloc = (CC_HEAP_ALLOC)alloc_cb;
    }
}

static void encoder_init(uint8_t instance_handle, void *codec_info)
{
    dbgTestPL();
    //Capture should always be less than 1
    LOG_D("%s", __func__);
    app_aob_smf_t* ctx = &smf_encoder;
    memset(ctx,0,sizeof(app_aob_smf_t));
    ctx->codec = "lc3";
    ctx->codec_info = (CODEC_INFO_T*)codec_info;
    ctx_init(ctx);
    encoder_stream_init(ctx);
    LOG_D("%s end", __func__);

}

static void encoder_deinit(uint8_t instance_handle)
{
    LOG_D("%s", __FUNCTION__);
    app_aob_smf_t* ctx = &smf_encoder;
    encoder_stream_deinit(ctx);
}

static int encode(uint8_t instance_handle, void *codec_info,
    uint32_t inputDataLength, void *input, uint16_t frame_size, uint8_t* output)
{
    returnIfErrC(CODEC_ERR,!input);
    returnIfErrC(CODEC_ERR,!inputDataLength);
    app_aob_smf_t* ctx = &smf_encoder;
    ctx->vlc_frame_size = ctx->codec_info->frame_size*ctx->codec_info->num_channels;
    //dbgTestPXL("%u->%u[%ums]%ums",inputDataLength,ctx->vlc_frame_size,TICKS_TO_MS(hal_sys_timer_get()),timeStamp/1000);
    void* hd = ctx->handle;
    returnIfErrC(CODEC_ERR,!hd);

    smf_frame_t ifrm;
    memset(&ifrm, 0, sizeof(smf_frame_t));
    ifrm.buff = input;
    ifrm.max = inputDataLength;
    ifrm.offset = 0;
    ifrm.size = inputDataLength;
    ifrm.flags = 0;//SMF_FRAME_IS_PCM_NONINTERLACE;
    smf_frame_t ofrm;
    memset(&ofrm, 0, sizeof(smf_frame_t));
    ofrm.buff = ctx->vlc_frame_buff;
    ofrm.max = ctx->vlc_frame_size;
    ofrm.offset = 0;
    ofrm.size = 0;
    ofrm.flags = 0;
    //dbgTestDump(input[0],16);
    //dbgTestDump(input[1],16);
    if(!smf_encode(hd,&ifrm,&ofrm)){
        dbgErrPL();
        smf_print_error(hd);
    }
    //
    //dbgTestPXL("%d,%d,%p",inputDataLength,ofrm.size,ofrm.buff);
    return CODEC_OK;
}

static void encoder_buf_deinit(uint8_t instance_handle)
{
    //Capture should always be less than 1
}

static uint32_t encoder_set_freq(void *_codec_info, uint32_t base_freq, uint32_t core_type)
{
    return APP_SYSFREQ_208M;
}

static GAF_ENCODER_FUNC_LIST_T gaf_audio_smf_encoder_func_list =
{
    .encoder_init_buf_func = encoder_buf_init,
    .encoder_init_func = encoder_init,
    .encoder_deinit_func = encoder_deinit,
    .encoder_encode_frame_func = encode,
    .encoder_deinit_buf_func = encoder_buf_deinit,
    .encoder_set_freq = encoder_set_freq,
};

//++enable lc3 with smf
EXTERNC void gaf_audio_lc3_update_decoder_func_list(void *_dec_func_list)
{
    LOG_I("[LC3]");
    smf_init();
    TRACE(0, "%s, %d", __func__, __LINE__);
    smf_lc3_decoder_register();
    dbgTestPL();
    GAF_DECODER_FUNC_LIST_T **dec_func_list = (GAF_DECODER_FUNC_LIST_T **)_dec_func_list;
    *dec_func_list = &gaf_audio_smf_decoder_func_list;
}

EXTERNC void gaf_audio_lc3_update_encoder_func_list(void *_enc_func_list)
{
    LOG_I("[LC3]");
    smf_init();
    smf_lc3_encoder_register();
    dbgTestPL();
    GAF_ENCODER_FUNC_LIST_T **enc_func_list = (GAF_ENCODER_FUNC_LIST_T **)_enc_func_list;
    *enc_func_list = &gaf_audio_smf_encoder_func_list;
}

EXTERNC int gaf_audio_lc3_encoder_get_max_frame_size(){
    int framesize_max = 870;
    dbgTestPDL(framesize_max);
    return framesize_max;
}
//--enable lc3 with smf

#endif

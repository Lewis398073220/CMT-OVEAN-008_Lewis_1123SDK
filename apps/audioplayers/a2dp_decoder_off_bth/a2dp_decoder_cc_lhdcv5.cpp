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

#include "cmsis.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "a2dp_decoder_cc_common.h"
#include "a2dp_decoder_cc_off_bth.h"
#include "btapp.h"
#include "lhdcv5_util_dec.h"
#include "dwt.h"
#if defined(SPAZIO_ON)
#include "spazio_util.h"
#define SPAZIO_MEASUTE_DECODE_TIME
#define SPAZIO_AUTO_ANGLE_DEMO
#endif
#if defined(BASS_ENHANCEMENT_ON)
#include "BassEnhancement_util.h"
#define BASS_ENHANCEMENT_DECODE_TIME
#endif
#ifndef LHDCV5_MTU_LIMITER
#define LHDCV5_MTU_LIMITER (120)
#endif
#define DECODE_LHDC_PCM_FRAME_LENGTH  (960*4*2*2)

#define LHDC_LIST_SAMPLES (960)

#define LHDC_EXT_FLAGS_LLAC   0x04
#define LHDC_EXT_FLAGS_V4     0x40
#define LHDC_DWT_DEBUG

#define BES_MEASURE_DECODE_TIMEx

void *lhdcv5_cc_ptr;

static void print_log_cb_lhdcv5(char* msg) {
    //CC_DECODE_LOG_I("%s\n", msg);
}

#if defined(SPAZIO_ON)
spazio_param_t* spazio;
static void print_log_cb(char* msg) {
     CC_DECODE_LOG_I("%s\n", msg);
}
#if defined(SPAZIO_AUTO_ANGLE_DEMO)
uint32_t spazio_frame_cnt;
float angle;
#endif
#endif
#if defined(BASS_ENHANCEMENT_ON)
bass_enhancement_param_t* bass;

static void print_log_cb_bass(char* msg) {
     CC_DECODE_LOG_I("%s\n", msg);
}
/* temp global variable for test*/
lhdc_channel_t  channel_output;
uint8_t* out_l;
uint8_t* out_r;

uint8_t ext_fac[256]={
    0x21, 0x40, 0x23, 0x24, 0x53, 0x61, 0x76, 0x69, 0x74, 0x65, 0x63, 0x68, 0x20, 0x42, 0x61, 0x73, 
    0x73, 0x45, 0x6E, 0x68, 0x61, 0x6E, 0x63, 0x65, 0x6D, 0x65, 0x6E, 0x74, 0x20, 0x45, 0x66, 0x66, 
    0x65, 0x63, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x53, 0x61, 0x76, 0x65, 0x20, 0x46, 0x72, 0x6F, 0x6D, 0x20, 0x33, 0x44, 0x41, 0x52, 0x20, 0x44, 
    0x4C, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x64, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3E, 
    0x00, 0x00, 0xC8, 0x42, 0x00, 0x00, 0xA0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x41, 
    0x00, 0x00, 0xA0, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif
extern "C"
{
    int32_t lhdcv5_util_set_license_check_period (void *ptr, uint8_t period);
    int32_t lhdcv5_util_dec_channel_selsect(void *ptr, lhdcv5_channel_t channel_type);

}
/**********************private function declaration*************************/
static void a2dp_decoder_cc_lhdc_init(void);
static void a2dp_decoder_cc_lhdc_deinit(void);
static bool a2dp_decoder_cc_lhdc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame);
int a2dp_audio_lhdcv5_channel_select(A2DP_DECODER_CHANNEL_SELECT_E chnl_sel);


/************************private variable defination************************/


A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_lhdcv5_decoder_config =
{
    {44100, 1},
    a2dp_decoder_cc_lhdc_init,
    a2dp_decoder_cc_lhdc_deinit,
    a2dp_decoder_cc_lhdc_decode_frame,
};

/****************************function defination****************************/
static void a2dp_decoder_cc_lhdc_init(void)
{
    hal_sysfreq_req(HAL_SYSFREQ_USER_DSP, HAL_CMU_FREQ_104M);
    uint8_t *pLhdc_data = a2dp_decoder_cc_get_lhdc_data();
        CC_DECODE_LOG_D("[BRUCE DEBUG][M55] LIC first 8 byte Received:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ", pLhdc_data[0], \
        pLhdc_data[1], \
        pLhdc_data[2], \
        pLhdc_data[3], \
        pLhdc_data[4], \
        pLhdc_data[5], \
        pLhdc_data[6], \
        pLhdc_data[7]
        );
    uint8_t lhdc_license_key = 0;
    A2DP_DECODER_CHANNEL_SELECT_E channel_sel = (A2DP_DECODER_CHANNEL_SELECT_E)a2dp_decoder_cc_get_channel_sel();

    lhdcv5_ver_t version = VERSION_5;
    uint32_t size = 0;
    //uint32_t *ptr = NULL;
    uint32_t lossless_enable = 0;
    uint8_t *heap_lhdcv5 = NULL;

    //lossless_enable = a2dp_lhdc_get_ext_flags(LHDCV5_EXT_FLAGS_LOSSLESS);
    lhdcv5_util_dec_register_log_cb(&print_log_cb_lhdcv5);
    lhdcv5_util_dec_get_mem_req(version, a2dp_decoder_cc_get_sample_rate(), LHDCV5_AUDIO_FMT_OUT_PCM , lossless_enable, &size);

    /*memory allocate*/
    off_bth_syspool_get_buff(&heap_lhdcv5, size);
    ASSERT(heap_lhdcv5, "%s size:%d", __func__, size);
    lhdcv5_cc_ptr = (void*)heap_lhdcv5;

    CC_DECODE_LOG_I("%s: LHDC V5 heap size %d, lossless:%d", __func__, size, lossless_enable);
    if (lhdcv5_cc_ptr)
    {
        lhdcv5_util_init_decoder(lhdcv5_cc_ptr, a2dp_decoder_cc_get_bits_depth(), a2dp_decoder_cc_get_sample_rate(), LHDCV5_AUDIO_FMT_OUT_PCM, 50, lossless_enable, version);
        lhdcv5_util_set_license_check_period(lhdcv5_cc_ptr, 10);
    }

    lhdc_license_key = lhdcv5_util_set_license(lhdcv5_cc_ptr, pLhdc_data, NULL);
    if(lhdc_license_key) {
        CC_DECODE_LOG_I("[BRUCE DEBUG][M55] LHDC5 KEY OK");
    }
    else {
        CC_DECODE_LOG_W("[BRUCE DEBUG][M55] LHDC5 KEY ERROR");
    }

    CC_DECODE_LOG_I("[BRUCE DEBUG][M55] sample rate:%d bit depth:%d, version:%d, %d", a2dp_decoder_cc_get_sample_rate(),a2dp_decoder_cc_get_bits_depth(), version, channel_sel);

    a2dp_audio_lhdcv5_channel_select(channel_sel);

#ifdef SPAZIO_ON
    uint8_t *heap_buff = NULL;
    uint32_t heap_size = sizeof(spazio_param_t);

    CC_DECODE_LOG_I("Spazio-lite %s\n", spazio_util_get_version());
    spazio_util_reg_log_cb((void*)&print_log_cb);
    /*memory allocate*/
    off_bth_syspool_get_buff(&heap_buff, heap_size);
    ASSERT(heap_buff, "%s size:%d", __func__, heap_size);
    spazio = (spazio_param_t *)heap_buff;

    spazio->channel_num_in = 2;
    spazio->bits_per_sample_in = a2dp_decoder_cc_get_bits_depth();
    spazio->sample_rate_in = a2dp_decoder_cc_get_sample_rate();
    if (spazio->sample_rate_in == 44100){
        spazio->sample_per_frame = 220;
    }else if (spazio->sample_rate_in == 48000){
        spazio->sample_per_frame = 240;
    }

    if (spazio_util_get_mem_req_size(spazio) != SPAZIO_UTIL_SUCCESS){
        CC_DECODE_LOG_I("spazio_util_get_mem_req_size failed!\n");
    }else{
        CC_DECODE_LOG_I("mem_size:%d", spazio->mem_req_size);
        heap_size = spazio->mem_req_size;
        /*memory allocate*/
        off_bth_syspool_get_buff(&heap_buff, heap_size);
        ASSERT(heap_buff, "%s size:%d", __func__, heap_size);
        spazio->ptr = (spazio_param_t *)heap_buff;
        if (spazio_util_init(spazio) != SPAZIO_UTIL_SUCCESS){
            CC_DECODE_LOG_I("spazio_util_init failed!\n");
        }
    }
    if (channel_sel == 3){ //A2DP_AUDIO_CHANNEL_SELECT_RCHNL
        spazio->channel_out = SPAZIO_OUTPUT_CH_RIGHT;
    }
    else if (channel_sel == 2){ //A2DP_AUDIO_CHANNEL_SELECT_LCHNL
        spazio->channel_out = SPAZIO_OUTPUT_CH_LEFT;
    }
    else{
        spazio->channel_out = SPAZIO_OUTPUT_CH_LEFT;
    }
    spazio_util_set_channel(spazio);
#endif
#if defined(BASS_ENHANCEMENT_ON)
    uint8_t *heap_buff_bass = NULL;
    uint32_t heap_size_bass = sizeof(bass_enhancement_param_t);

    CC_DECODE_LOG_I("bass enhancement %s", bass_enhancement_util_get_version());
    bass_enhancement_util_reg_log_cb((void*)&print_log_cb_bass);
    /*memory allocate*/
    off_bth_syspool_get_buff(&heap_buff_bass, heap_size_bass);
    ASSERT(heap_buff_bass, "%s size:%d", __func__, heap_size_bass);
    bass = (bass_enhancement_param_t *)heap_buff_bass;

    bass->channel_num = 1;
    bass->sample_rate = a2dp_decoder_cc_get_sample_rate();
    bass->bits_per_sample = a2dp_decoder_cc_get_bits_depth();
    if (bass->sample_rate == 44100){
        bass->sample_per_frame = 220;
    }else if (bass->sample_rate == 48000){
        bass->sample_per_frame = 240;
    }
    /*bass enhancement active on*/
    bass->set_info.bass_active = 1;

    if (bass_enhancement_util_get_mem_req_size(bass) != BASS_ENHANCEMENT_UTIL_SUCCESS){
        CC_DECODE_LOG_I("[bass_enhancement_util_get_mem_req_size error");
    }else{
        CC_DECODE_LOG_I("mem_size:%d", bass->mem_req_size);
        heap_size_bass = bass->mem_req_size;
        /*memory allocate*/
        off_bth_syspool_get_buff(&heap_buff_bass, heap_size_bass);
        ASSERT(heap_buff_bass, "%s size:%d", __func__, heap_size_bass);
        bass->mem_addr  = (void*)heap_buff_bass;
        /* load script */
        memcpy(bass->set_info.factor, ext_fac, 256);

        if (bass_enhancement_util_init(bass) != BASS_ENHANCEMENT_UTIL_SUCCESS){
            CC_DECODE_LOG_I("bass_enhancement_util_init error");
        }else{
            if (bass_enhancement_util_set_bass_active(bass) != BASS_ENHANCEMENT_UTIL_SUCCESS){
                CC_DECODE_LOG_I("bass_enhancement_util_set_bass_active error");
            }
        }

        bass_enhancement_util_get_state(bass);

        /*Current bass enhancement only support one channel, need to convert*/
        heap_size_bass = bass->sample_per_frame * sizeof(float);
        off_bth_syspool_get_buff(&heap_buff_bass, heap_size_bass);
        ASSERT(heap_buff_bass, "%s size:%d", __func__, heap_size_bass);
        out_l = (uint8_t*)heap_buff_bass;

        off_bth_syspool_get_buff(&heap_buff_bass, heap_size_bass);
        ASSERT(heap_buff_bass, "%s size:%d", __func__, heap_size_bass);
        out_r = (uint8_t*)heap_buff_bass;
    }
#ifdef SPAZIO_AUTO_ANGLE_DEMO
    spazio_frame_cnt = 0;
    angle = 0;
#endif
#endif

}


static void a2dp_decoder_cc_lhdc_deinit(void)
{
    lhdcv5_util_dec_destroy(lhdcv5_cc_ptr);
}


static uint8_t decoded_buf[1024*8] = {0};

static bool a2dp_decoder_cc_lhdc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame)
{
#if 0
    CC_DECODE_LOG_W("[BRUCE DEBUG][M55] encoded data:\n");
    for(int i = 0; ((i*8+7) < 16/*pEncodedPacket->data_len*/); i++){
         CC_DECODE_LOG_W("0x%02X|  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",i*8,pEncodedPacket->data[i*8+0],\
                                                                                                 pEncodedPacket->data[i*8+1],\
                                                                                                 pEncodedPacket->data[i*8+2],\
                                                                                                 pEncodedPacket->data[i*8+3],\
                                                                                                 pEncodedPacket->data[i*8+4],\
                                                                                                 pEncodedPacket->data[i*8+5],\
                                                                                                 pEncodedPacket->data[i*8+6],\
                                                                                                 pEncodedPacket->data[i*8+7]);
    }
#endif

    POSSIBLY_UNUSED int encoded_len = pEncodedPacket->data_len;
    //int wrote_bytes = 0;
    uint32_t wrote_bytes = 0;

    POSSIBLY_UNUSED uint8_t * temp_buf_out = pPcmFrame->data;

    lhdcv5_frame_Info_t lhdc_frame_Info_dsp;
    int lhdc_should_decode = 1;
#ifdef BES_MEASURE_DECODE_TIME
    uint32_t start_time_lhdc = hal_sys_timer_get();
#endif

#ifdef LHDC_DWT_DEBUG
    watchpoint_enable(0, (uint32_t *)&pEncodedPacket->data, WPT_WRITE);
#endif

    lhdcv5_util_dec_fetch_frame_info(lhdcv5_cc_ptr, pEncodedPacket->data, 10, &lhdc_frame_Info_dsp);
    if(lhdc_frame_Info_dsp.frame_len){
        if(lhdc_frame_Info_dsp.frame_len != pEncodedPacket->data_len){
            CC_DECODE_LOG_W("%s frame_info:%d data_len:%d", __func__, \
            lhdc_frame_Info_dsp.frame_len, pEncodedPacket->data_len);
            lhdc_should_decode = 0;
        }
    } else{
        lhdc_should_decode = 0;
    }



    //uint32_t lhdc_out_len = 0;
    POSSIBLY_UNUSED int32_t ret = 0;
    if(lhdc_should_decode == 1){
        ret = lhdcv5_util_dec_process(lhdcv5_cc_ptr, decoded_buf, pEncodedPacket->data, encoded_len, &wrote_bytes);
        // CC_DECODE_LOG_I("%s ret:%d", __func__, ret);
    } else {
        wrote_bytes = pPcmFrame->data_len;
        // CC_DECODE_LOG_I("%s wrote_bytes:%d", __func__, wrote_bytes);
    }
#ifdef LHDC_DWT_DEBUG
    watchpoint_disable(0);
#endif

#ifdef BES_MEASURE_DECODE_TIME
    uint32_t end_time_lhdc = hal_sys_timer_get();
    uint32_t deltaMs_lhdc = TICKS_TO_US(end_time_lhdc - start_time_lhdc);
    TRACE(2, ":[BRUCE DEBUG][M55][DECODE TIME] LHDC Decode time %d uS.", deltaMs_lhdc);
#endif
    if(wrote_bytes > 0 && wrote_bytes <= pPcmFrame->data_len)
    {
#ifdef SPAZIO_ON
#ifdef SPAZIO_AUTO_ANGLE_DEMO
        /* 5ms*1000(5s)=5000 */
        if (spazio_frame_cnt < 5000 ){
            spazio_frame_cnt++;
        }
        else{
            spazio_frame_cnt = 0;

            if (angle < 360){
                angle += 30;
            }else{
                angle = 0;
            }
            spazio->angle = angle;
            spazio_util_set_angle(spazio);
        }
#endif
        spazio->buf_in = decoded_buf;
        spazio->buf_out = decoded_buf;
#ifdef SPAZIO_MEASUTE_DECODE_TIME
        uint32_t start_time_spazio = hal_sys_timer_get();
#endif
        spazio_util_process(spazio);
#ifdef SPAZIO_MEASUTE_DECODE_TIME
        uint32_t end_time_spazio = hal_sys_timer_get();
        uint32_t deltaMs_spazio = TICKS_TO_US(end_time_spazio - start_time_spazio);
        TRACE(1, "spazio process time = %d uS", deltaMs_spazio);
#endif
#endif
#if defined(BASS_ENHANCEMENT_ON)
        // convet 2ch to 1ch
        uint32_t ns = bass->sample_per_frame;
        uint8_t* in = decoded_buf;
        uint32_t i;

        if (bass->bits_per_sample == 16){
            short* pwav_l = (short*)out_l;
            short* pwav_r = (short*)out_r;

            for (i = 0; i < ns; i++) {
                *(pwav_l + i) = *(short*)in; in += 2;
                *(pwav_r + i) = *(short*)in; in += 2;
            }
        } else{
            int* pwav_l = (int*)out_l;
            int* pwav_r = (int*)out_r;

            for (i = 0; i < ns; i++) {
                *(pwav_l + i) = *(int*)in; in += 4;
                *(pwav_r + i) = *(int*)in; in += 4;
            }
        }
        if (channel_output == LHDCV5_OUTPUT_RIGHT_CAHNNEL){
            bass->buf_in = out_r;
            bass->buf_out = out_r;
        }else{
            bass->buf_in = out_l;
            bass->buf_out = out_l;
        }
#ifdef BASS_ENHANCEMENT_DECODE_TIME
        uint32_t start_time_bass = hal_sys_timer_get();
#endif
        if (bass_enhancement_util_process(bass) != BASS_ENHANCEMENT_UTIL_SUCCESS){
            TRACE(1, "bass process error");
        }
#ifdef BASS_ENHANCEMENT_DECODE_TIME
        uint32_t end_time_bass = hal_sys_timer_get();
        uint32_t deltaMs_bass = TICKS_TO_US(end_time_bass - start_time_bass);
        TRACE(1, "bass time = %d uS", deltaMs_bass);
#endif
        // convet 1ch to 2ch
        uint8_t* in_l =  out_l;
        uint8_t* in_r =  out_r;
        if (bass->bits_per_sample == 16){
            short* pwav = (short*)decoded_buf;
            for (i = 0; i < ns; i++){
                *pwav = *(short*)in_l;
                pwav++; in_l += 2;
                *pwav = *(short*)in_r;
                pwav++; in_r += 2;
            }
        }else{
            int* pwav = (int*)decoded_buf;
            for (i = 0; i < ns; i++){
                *pwav = *(int*)in_l;
                pwav++; in_l += 4;
                *pwav = *(int*)in_r;
                pwav++; in_r += 4;
            }
        }
#endif //BASS_ENHANCEMENT_ON
#if 0
        for(int i = 0; ((i*8+7) < wrote_bytes>>3); i++){
         CC_DECODE_LOG_W("0x%02X|  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",i*8,decoded_buf[i*8+0],\
                                                                                                 decoded_buf[i*8+1],\
                                                                                                 decoded_buf[i*8+2],\
                                                                                                 decoded_buf[i*8+3],\
                                                                                                 decoded_buf[i*8+4],\
                                                                                                 decoded_buf[i*8+5],\
                                                                                                 decoded_buf[i*8+6],\
                                                                                                 decoded_buf[i*8+7]);
    }

#endif
        // CC_DECODE_LOG_I("wr %d",wrote_bytes);
        memcpy(pPcmFrame->data,decoded_buf,wrote_bytes);
        pPcmFrame->data_len = wrote_bytes;
    } else
    {
//ASSERT(0,"LHDC WRONG DECODED BYTES: %d/%d", wrote_bytes,pPcmFrame->data_len);
        CC_DECODE_LOG_E("[Bruce Debug][M55] LHDC WRONG DECODED BYTES: %d/%d", wrote_bytes,pPcmFrame->data_len);
        memcpy(pPcmFrame->data,decoded_buf,pPcmFrame->data_len);
    }
    return true;
}

int a2dp_audio_lhdcv5_channel_select(A2DP_DECODER_CHANNEL_SELECT_E chnl_sel)
{
    lhdcv5_channel_t channel_type;
    switch(chnl_sel){
        case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
        channel_type = LHDCV5_OUTPUT_STEREO;
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] channel select stereo.");
        break;
        case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
        channel_type = LHDCV5_OUTPUT_LEFT_CAHNNEL;
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] channel select L channel.");
        break;
        case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
        channel_type = LHDCV5_OUTPUT_RIGHT_CAHNNEL;
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] channel select R channel.");
        break;

        default:
        case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] Unsupported channel config(%d).", chnl_sel);
        return A2DP_DECODER_NOT_SUPPORT;
    }
    lhdcv5_util_dec_channel_selsect(lhdcv5_cc_ptr, channel_type);

#ifdef BASS_ENHANCEMENT_ON
    channel_output = channel_type;
#endif
    return A2DP_DECODER_NO_ERROR;
}





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
#include "plat_types.h"
#include "audioflinger.h"
#include "speech_ssat.h"
#include "app_anc_assist.h"
#include "adj_eq.h"
#include "audio_dump.h"
#include "app_utils.h"

//#define VOICE_ASSIST_ADJ_EQ_DUMP
//#define AEC_CAL_MIPS
#if defined(AEC_CAL_MIPS)
uint32_t aec_cal_ticks;
uint32_t aec_cal_ticks_pre;
uint32_t aec_cal_us;
uint32_t aec_cal_end_ticks;
uint32_t aec_cal_used_us;

#include "hal_timer.h"
#endif

static int32_t _app_voice_assist_adj_eq_callback(void *buf, uint32_t len, void *other);

int32_t app_voice_assist_adj_eq_init(void)
{
    app_anc_assist_register(ANC_ASSIST_USER_ADJ_EQ, _app_voice_assist_adj_eq_callback);
#ifdef VOICE_ASSIST_ADJ_EQ_DUMP
    audio_dump_init(30, sizeof(short), 2);
#endif
    return 0;
}

static int g_working_status = 0;
int32_t app_voice_assist_adj_eq_open(void)
{
    TRACE(0, "[%s] ADJ EQ open stream", __func__);
    app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, APP_SYSFREQ_26M);
    adj_eq_reset();
    app_anc_assist_open(ANC_ASSIST_USER_ADJ_EQ);
    g_working_status = 1;
    return 0;
}

int32_t app_voice_assist_adj_eq_close(void)
{
    TRACE(0, "[%s] ADJ EQ close stream", __func__);
    g_working_status = 0;
    app_anc_assist_close(ANC_ASSIST_USER_ADJ_EQ);
    return 0;
}

static float rsp_fb_buf[30] = {0};
static float rsp_ref_buf[30] = {0};

static void resample_4k(float *fb_buf, float *ref_buf, uint32_t pcm_len, float *rsp_fb_buf, float *rsp_ref_buf)
{
    const float den[5] = {1.000000, -3.18063854887472, 3.86119434899421, -2.11215535511097, 0.438265142261980};  //a
    const float num[5] = {0.000416599204406599, 0.00166639681762640, 0.00249959522643960, 0.00166639681762640, 0.000416599204406599};  //b
    static float Y0 = 0, Y1 = 0, Y2 = 0, Y3 = 0, Y4 = 0, X0 = 0, X1 = 0, X2 = 0, X3 = 0, X4 = 0;
    static float w0 = 0, w1 = 0, w2 = 0, w3 = 0, w4 = 0, z0 = 0, z1 = 0, z2 = 0, z3 = 0, z4 = 0;
    
    for (uint32_t i=0; i<pcm_len; i++) {
        
        X0 = fb_buf[i];
        z0 = ref_buf[i];

        Y0 = X0 * num[0] + X1 * num[1] + X2 * num[2] + X3 * num[3] + X4 * num[4]
             - Y1 * den[1] - Y2 * den[2] - Y3 * den[3] - Y4 * den[4];

        w0 = z0 * num[0] + z1 * num[1] + z2 * num[2] + z3 * num[3] + z4 * num[4]
             - w1 * den[1] - w2 * den[2] - w3 * den[3] - w4 * den[4];

        Y4 = Y3;
        Y3 = Y2;
        Y2 = Y1;
        Y1 = Y0;
        X4 = X3;
        X3 = X2;
        X2 = X1;
        X1 = X0;

        w4 = w3;
        w3 = w2;
        w2 = w1;
        w1 = w0;
        z4 = z3;
        z3 = z2;
        z2 = z1;
        z1 = z0;

        if (i%4 == 0) {
             rsp_fb_buf[i/4] = Y0;
             rsp_ref_buf[i/4] = w0;
        }
    }
}

#ifdef VOICE_ASSIST_ADJ_EQ_DUMP
static int16_t tmp_dump_buf[30];
#endif

static int16_t fb_buf16[30];
static int16_t ref_buf16[30];
extern uint8_t is_a2dp_mode(void);
static int32_t _app_voice_assist_adj_eq_callback(void *buf, uint32_t len, void *other)
{
    if(g_working_status == 0){
        g_working_status = -1;
        app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, APP_SYSFREQ_32K);
    }
    if(is_a2dp_mode()==false || g_working_status == -1){
        
        return 0;
    }

#if defined(AEC_CAL_MIPS)
    aec_cal_ticks = hal_fast_sys_timer_get();
    aec_cal_us = FAST_TICKS_TO_US(aec_cal_ticks - aec_cal_ticks_pre);
    TRACE(0,"[%s] adj eq callback period = %d", __func__, aec_cal_us);
    aec_cal_ticks_pre = aec_cal_ticks;
#endif

    //TRACE(0, "[%s] len = %d", __func__, len);
    float *fb_pcm_buf = (float *)buf;
    float *ref_pcm_buf = (float *)other;

    resample_4k(fb_pcm_buf, ref_pcm_buf, len, rsp_fb_buf, rsp_ref_buf);
    
    for(int i=0; i<len/4; i++){
        fb_buf16[i] = (int16_t)(rsp_fb_buf[i]/256);
#ifdef VOICE_ASSIST_ADJ_EQ_DUMP
        tmp_dump_buf[i] = fb_buf16[i];
#endif
    }
#ifdef VOICE_ASSIST_ADJ_EQ_DUMP
    audio_dump_add_channel_data(0, tmp_dump_buf, len/4);
#endif
    for(int i=0; i<len/4; i++){
        ref_buf16[i] = (int16_t)(rsp_ref_buf[i]/256);
#ifdef VOICE_ASSIST_ADJ_EQ_DUMP
        tmp_dump_buf[i] = ref_buf16[i];
#endif
    }
#ifdef VOICE_ASSIST_ADJ_EQ_DUMP
    audio_dump_add_channel_data(1, tmp_dump_buf, len/4);
    audio_dump_run();
#endif
    adj_eq_filter_estimate(fb_buf16, ref_buf16, len/4);

#if defined(AEC_CAL_MIPS)
    aec_cal_end_ticks = hal_fast_sys_timer_get();
    aec_cal_used_us = FAST_TICKS_TO_US(aec_cal_end_ticks - aec_cal_ticks);
    TRACE(0,"[%s] aec cal period = %d", __func__, aec_cal_used_us);
#endif

    return 0;
}

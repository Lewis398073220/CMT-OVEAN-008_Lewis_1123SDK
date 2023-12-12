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
#ifdef VOICE_ASSIST_FF_FIR_LMS
#include "hal_trace.h"
#include "app_anc_assist.h"
#include "anc_assist.h"
#include "anc_process.h"
#include "audio_dump.h"
#include "app_utils.h"
#include "ae_math.h"
#include "hal_timer.h"
#include "app_anc.h"
#include "cmsis.h"
#include "anc_ff_fir_lms.h"
#include "hal_codec.h"
#ifndef FIR_ADAPT_ANC_M55
#include "app_voice_assist_fir_lms_cfg.h"
#endif

// #define VOICE_FIR_LMS_DUMP
#ifdef VOICE_FIR_LMS_DUMP
static short tmp_data[240];
#endif

extern const struct_anc_cfg * anc_coef_list_50p7k[12];
static enum HAL_CODEC_ECHO_PATH_T path_def = HAL_CODEC_ECHO_PATH_QTY;

#ifndef FIR_ADAPT_ANC_M55

#define HEAP_BUFF_SIZE (200 * 1024)
#include "ext_heap.h"

static struct_anc_fir_cfg fir_cache_cfg = {
    .anc_fir_cfg_ff_l.fir_bypass_flag = 0,
    .anc_fir_cfg_ff_l.fir_len = LOCAL_FIR_LEN,
    .anc_fir_cfg_ff_l.fir_coef[0] = 0 * 32768 * 256,
    .anc_fir_cfg_mc_l.fir_bypass_flag = 0,
    .anc_fir_cfg_mc_l.fir_len = LOCAL_FIR_LEN,
    .anc_fir_cfg_mc_l.fir_coef[0] = 0.5 * 32768 * 256,
};

static int stop_flag = 0;
int32_t *fir_coeff_cache = NULL;
int32_t *fir_coeff_cache2 = NULL;
int32_t *mc_fir_coeff_cache = NULL;
#endif

extern int anc_set_fir_cfg(struct_anc_fir_cfg *cfg, enum ANC_TYPE_T anc_type);

#ifdef FIR_ADAPT_ANC_M55
static int32_t _assist_anc_dsp_result_callback(void *buf, uint32_t len, void *other, uint32_t sub_cmd)
{
    // TRACE(0,"[%s] get data is %d and %d", __func__, ((struct_anc_fir_cfg *)buf)->anc_fir_cfg_ff_l.fir_len, sub_cmd);
    anc_set_fir_cfg(((struct_anc_fir_cfg *)buf), sub_cmd);

    return 0;
}
#endif

static int32_t _voice_assist_fir_lms_callback(void *buf, uint32_t len, void *other);

int32_t app_voice_assist_fir_lms_init(void)
{
#ifdef FIR_ADAPT_ANC_M55
    app_anc_assist_result_register(ANC_ASSIST_USER_FIR_LMS, _assist_anc_dsp_result_callback);
#endif

    app_anc_assist_register(ANC_ASSIST_USER_FIR_LMS, _voice_assist_fir_lms_callback);
    return 0;
}

int32_t app_voice_assist_fir_lms_reset(void)
{
#ifdef FIR_ADAPT_ANC_M55
    // TODO: Add app_anc_assist_ctrl(ANC_ASSIST_USER_FIR_LMS);
#else
    anc_ff_fir_lms_reset(fir_st, 0);
#endif
    return 0;
}

void set_fixed_fir_filter(void)
{
#ifdef FIR_ADAPT_ANC_M55
    // TODO: Add app_anc_assist_ctrl(ANC_ASSIST_USER_FIR_LMS);
#else
    anc_ff_fir_lms_reset(fir_st, 1);
#endif
}

void app_voice_assist_fir_lms_update_Sz(float **Sz, uint32_t len)
{
#ifdef FIR_ADAPT_ANC_M55
    app_anc_assist_ctrl(ANC_ASSIST_USER_FIR_LMS, 0, (uint8_t *)Sz, len * sizeof(float));
#endif
}

int32_t app_voice_assist_fir_lms_open(void)
{
    TRACE(0, "[%s] fir lms start stream", __func__);
#ifndef FIR_ADAPT_ANC_M55
    stop_flag = 0;
    POSSIBLY_UNUSED uint16_t frame_size = FIR_BLOCK_SIZE;
    POSSIBLY_UNUSED uint16_t blocks = 1;
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_340M);
    TRACE(0, "[%s] Sys freq: %d", __func__, hal_sys_timer_calc_cpu_freq(5, 0));

    ext_heap_init();

    anc_ff_fir_lms_register_anc_set_fir_cfg_handler(anc_set_fir_cfg);
    fir_st = anc_ff_fir_lms_create(FIR_SAMPLE_RATE, FIR_BLOCK_SIZE, &cfg, &ext_allocator);

    fir_coeff_cache = fir_lms_coeff_cache(fir_st);
#endif
    app_anc_assist_open(ANC_ASSIST_USER_FIR_LMS);

    //set echo patch
    path_def = hal_codec_get_echo_path();
#if defined(CODEC_ECHO_PATH_VER) && (CODEC_ECHO_PATH_VER >= 2)
        hal_codec_set_echo_path(HAL_CODEC_ECHO_PATH_HBF4_DATA_IN);
#else
        hal_codec_set_echo_path(HAL_CODEC_ECHO_PATH_ALL);
#endif

    //fir_coeff_cache = fir_lms_coeff_cache(fir_st);
    //mc_fir_coeff_cache = mc_fir_lms_coeff_cache(fir_st);

#ifdef VOICE_FIR_LMS_DUMP
    audio_dump_init(FIR_BLOCK_SIZE, sizeof(short), 2);
#endif

    // close fb anc for adaptive anc, it is better not to open it during the init state
    // anc_set_gain(0, 0, ANC_FEEDBACK);

    return 0;
}

int32_t app_voice_assist_fir_lms_close(void)
{
    TRACE(0, "[%s] fir lms close stream", __func__);
    app_anc_assist_close(ANC_ASSIST_USER_FIR_LMS);

#ifndef FIR_ADAPT_ANC_M55
    anc_ff_fir_lms_destroy(fir_st);

    // anc_set_gain(512, 512, ANC_FEEDBACK);
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);

    ext_heap_deinit();
#endif


#ifdef VOICE_FIR_LMS_DUMP
    audio_dump_deinit();
#endif
    hal_codec_set_echo_path(path_def);

    return 0;
}

#if (FIR_SAMPLE_RATE == 32000)
static int16_t dump_data[240];
static POSSIBLY_UNUSED void dump_fir_coeff(int32_t *fir_coeff_cache, uint32_t len)
{
    int32_t *tmp_p = fir_coeff_cache;
    for (uint32_t i = 0; i < 240; i++) {
        dump_data[i] = 0x5555;
    }
    audio_dump_add_channel_data(0, dump_data, 240);

    for (uint32_t j = 0; j < 240; j++) {
        dump_data[j] = *tmp_p++ >> 8;
    }
    audio_dump_add_channel_data(1, dump_data, 240);

    for (uint32_t j = 0; j < 232; j++) {
        dump_data[j] = *tmp_p++ >> 8;
    }
    audio_dump_add_channel_data(2, dump_data, 232);

    audio_dump_run();
}
#elif (FIR_SAMPLE_RATE == 16000)
static int16_t dump_data[120];

static void dump_fir_coeff(int32_t *fir_coeff_cache, uint32_t len)
{
    int32_t *tmp_p = fir_coeff_cache;
    for (uint32_t i = 0; i < 8; i++) {
        dump_data[i] = 0x5555;
    }

    for (uint32_t j = 8; j < FIR_BLOCK_SIZE; j++) {
        dump_data[j] = *tmp_p++ >> 8;
    }
    audio_dump_add_channel_data(0, dump_data, FIR_BLOCK_SIZE);
    for (uint32_t j = 0; j < FIR_BLOCK_SIZE; j++) {
        dump_data[j] = *tmp_p++ >> 8;
    }
    audio_dump_add_channel_data(1, dump_data, FIR_BLOCK_SIZE);

    audio_dump_run();
}
#endif


static int32_t _voice_assist_fir_lms_callback(void * buf, uint32_t len, void *other)
{
#ifndef FIR_ADAPT_ANC_M55
    if (stop_flag == 0) {
        float ** input_data = buf;
        float * ff_data = input_data[0];  // error
        float * fb_data = input_data[1];  // error
        float * talk_data = input_data[2];
        float * vpu_data = input_data[3];
        float * ref_data = input_data[4];

        int32_t res = anc_ff_fir_lms_process(fir_st, ff_data, fb_data, talk_data, vpu_data, ref_data, FIR_BLOCK_SIZE);
        if (res == 1) {
            stop_flag = 1;
            fir_coeff_cache = fir_lms_coeff_cache(fir_st);
            dump_fir_coeff(fir_coeff_cache, LOCAL_FIR_LEN);

            // mc_fir_coeff_cache = mc_fir_lms_coeff_cache(fir_st);
            // dump_fir_coeff(mc_fir_coeff_cache, LOCAL_FIR_LEN);
            // fir_coeff_cache = get_local_fir_cache(fir_st);
            // dump_fir_coeff(fir_coeff_cache, LOCAL_FIR_LEN);
            // fir_coeff_cache2 = get_local_fir_cache2(fir_st);
            // dump_fir_coeff(fir_coeff_cache2, LOCAL_FIR_LEN);
            app_voice_assist_fir_lms_close();
        } else if (res == 2) {      // speaking
            stop_flag = 1;
            app_voice_assist_fir_lms_close();
        } else if (res == 3) {     // large leak
            stop_flag = 1;
            app_voice_assist_fir_lms_close();
        }
    } else {
        return 0;
    }
#endif
    return 0;
}

#ifndef FIR_ADAPT_ANC_M55
void set_fir_cache(void)
{
    for (uint32_t i = 0; i < LOCAL_FIR_LEN; i++)
        fir_cache_cfg.anc_fir_cfg_ff_l.fir_coef[i]=fir_coeff_cache[i];
    anc_set_fir_cfg(&fir_cache_cfg, ANC_FEEDFORWARD);
}

void set_mc_fir_cache(void)
{
    for (uint32_t i = 0; i < LOCAL_FIR_LEN; i++)
        fir_cache_cfg.anc_fir_cfg_mc_l.fir_coef[i]=fir_coeff_cache[i];
    anc_set_fir_cfg(&fir_cache_cfg, ANC_MUSICCANCLE);
}
#endif
#endif

#if defined (FIR_ADAPT_ANC_M55) && defined(ANC_ALGO_DSP)
#include "hal_trace.h"
#include "app_anc_assist.h"
#include "anc_assist.h"
#include "anc_process.h"
#include "audio_dump.h"
#include "app_utils.h"
// #include "fastmath.h"
// #include "ae_common.h"
#include "ae_math.h"
#include "hal_timer.h"
#include "app_anc.h"
#include <string.h>
#include "plat_types.h"
#include "anc_ff_fir_lms.h"
#include "app_dsp_m55.h"
#include "anc_assist_dsp.h"
#include "app_anc_assist_thirdparty.h"
#include "app_voice_assist_fir_lms_cfg.h"

#ifdef VOICE_FIR_LMS_DUMP
static short tmp_data[240];
#endif

WEAK float Sz_estimate[FIR_BLOCK_SIZE] = {1.0,0.0};

static int stop_flag = 0;
int32_t *fir_coeff_cache = NULL;
#define HEAP_BUFF_SIZE (70 * 1024)
#include "ext_heap.h"

int _voice_assist_fir_lms_return_res_handler(struct_anc_fir_cfg *cfg, enum ANC_TYPE_T cmd)
{
    uint8_t user = ANC_ASSIST_USER_FIR_LMS;
    uint32_t data_len = sizeof(struct_anc_fir_cfg);

    anc_assist_dsp_send_result_to_bth_via_fifo(user, (uint8_t *)cfg, data_len, (uint8_t)cmd);

    return 0;
}

int voice_assist_fir_lms_m55_open(void)
{
    stop_flag = 0;
    POSSIBLY_UNUSED uint16_t frame_size = FIR_BLOCK_SIZE;
    POSSIBLY_UNUSED uint16_t blocks = 1;
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);

    // ext_heap_init();

    anc_ff_fir_lms_register_anc_set_fir_cfg_handler(_voice_assist_fir_lms_return_res_handler);
    // fir_st = anc_ff_fir_lms_create(FIR_SAMPLE_RATE, FIR_BLOCK_SIZE, &cfg, &ext_allocator);

    // arm_fir_init_f32(&fir_st->fir_Sv, frame_size * blocks, fir_Sv_coeffs, fir_st->fir_Sv_Pstate, frame_size * blocks);

    // fir_coeff_cache = fir_lms_coeff_cache(fir_st);
    return 0;
}

int voice_assist_fir_lms_m55_close(void)
{
    TRACE(0, "[%s] fir lms deinit...", __func__);
    anc_ff_fir_lms_destroy(fir_st);
    // anc_set_gain(512, 512, ANC_FEEDBACK);
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    ext_heap_deinit();
    return 0;
}

int voice_assist_fir_lms_m55_reset(void)
{
    TRACE(0, "[%s] fir lms reset...", __func__);
    anc_ff_fir_lms_reset(fir_st,0);
    return 0;
}

int voice_assist_fir_lms_m55_get_freq(void)
{
    return 100;
}

int voice_assist_fir_lms_m55_set_cfg(uint32_t ctrl, uint8_t *tgt_cfg, uint32_t ptr_len)
{
    TRACE(0, "[%s] fir lms change value...", __func__);

    float *rec_Sz = *(float **)tgt_cfg;

    memcpy(Sz_estimate, rec_Sz, FIR_BLOCK_SIZE * sizeof(float));

    // TRACE(0,"Sz lms %d %d %d %d",(int)(Sz_estimate[0] * 10000),(int)(Sz_estimate[1] * 10000),(int)(Sz_estimate[238] * 10000),(int)(Sz_estimate[239] * 10000));

    ext_heap_init();
    fir_st = anc_ff_fir_lms_create(FIR_SAMPLE_RATE, FIR_BLOCK_SIZE, &cfg, &ext_allocator);
    fir_coeff_cache = fir_lms_coeff_cache(fir_st);

    return 0;
}

int voice_assist_fir_lms_m55_process(process_frame_data_t *process_frame_data)
{
    if (stop_flag == 0) {
        int loop_cnt = 1;
        int offset = 0;

        ASSERT(((process_frame_data->frame_len % FIR_BLOCK_SIZE) == 0), "[%s] the 120 data_len is error ", __func__);
        loop_cnt = process_frame_data->frame_len / FIR_BLOCK_SIZE;
        for (int i = 0; i < loop_cnt; i++) {
            float POSSIBLY_UNUSED *ff_mic_buf[MAX_FF_CHANNEL_NUM] = {NULL,};
            float POSSIBLY_UNUSED *fb_mic_buf[MAX_FB_CHANNEL_NUM] = {NULL,};
            float POSSIBLY_UNUSED *talk_mic_buf[MAX_TALK_CHANNEL_NUM] = {NULL,};
            float POSSIBLY_UNUSED *ref_mic_buf[MAX_REF_CHANNEL_NUM] = {NULL,};

            for (uint8_t i = 0; i < process_frame_data->ff_ch_num; i++) {
                ff_mic_buf[i] = process_frame_data->ff_mic[i] + offset;
            }
            for (uint8_t i = 0; i < process_frame_data->fb_ch_num; i++) {
                fb_mic_buf[i] = process_frame_data->fb_mic[i] + offset;
            }

            for (uint8_t i = 0; i < process_frame_data->talk_ch_num; i++) {
                talk_mic_buf[i] = process_frame_data->talk_mic[i] + offset;
            }

            for (uint8_t i = 0; i < process_frame_data->ref_ch_num; i++) {
                ref_mic_buf[i] = process_frame_data->ref[i] + offset;
            }

            // TRACE(0, "[%s] fir lms process...", __func__);

            int32_t POSSIBLY_UNUSED res = 0;
            if (fir_st != NULL)
                res = anc_ff_fir_lms_process(fir_st, ff_mic_buf[0], fb_mic_buf[0], talk_mic_buf[0], process_frame_data->vpu_mic + offset, ref_mic_buf[0], FIR_BLOCK_SIZE);
            offset += FIR_BLOCK_SIZE;
        }
    } else {
        return 0;
    }
    return 0;
}
#include "app_voice_assist_dsp.h"
app_voice_assist_dsp_t voice_assist_fir_lms_m55 = {
    .open          = voice_assist_fir_lms_m55_open,
    .close         = voice_assist_fir_lms_m55_close,
    .reset         = voice_assist_fir_lms_m55_reset,
    .ctrl          = voice_assist_fir_lms_m55_set_cfg,
    .process       = voice_assist_fir_lms_m55_process,
    .get_freq      = voice_assist_fir_lms_m55_get_freq,
};

int32_t assist_fir_lms_dsp_init(void)
{
    TRACE(1, "[%s]fir lms m55 init", __func__);
    anc_assist_dsp_register(ANC_ASSIST_USER_FIR_LMS, &voice_assist_fir_lms_m55, ANC_ASSIST_USER_FS_32K);
    return 0;
}
#endif
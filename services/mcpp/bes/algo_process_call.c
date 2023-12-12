/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#include "algo_process.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "string.h"
#include "hal_timer.h"
#include "speech_cfg.h"
#include "stream_mcps.h"

static algo_process_cfg_t g_capture_cfg = {0};
static algo_process_cfg_t g_playback_cfg = {0};

SpeechNs4State *ns4_st = NULL;
Ec2FloatState *ec2float_st = NULL;

#define SPEECH_TX_NS
#define SPEECH_TX_AEC

/***************************capture***************************/

int32_t algo_process_call_capture_open(algo_process_cfg_t *cfg)
{
    g_capture_cfg = *cfg;
    TRACE(4, "[%s] sample_rate=%d, frame_len=%d, mic_num=%d, mode=%d", __func__, \
                                        g_capture_cfg.sample_rate, \
                                        g_capture_cfg.frame_len, \
                                        g_capture_cfg.mic_num, \
                                        g_capture_cfg.mode);

#ifdef SPEECH_TX_NS
    const SpeechNs4Config ns4_cfg = {
        /*.bypass =*/ 0,
        /*.mode =*/ -18,
    };
#endif

#ifdef SPEECH_TX_AEC
    const Ec2FloatConfig config = {
        /*.bypass =*/ 0,
        /*.hpf_enabled =*/ 0,
        /*.af_enabled =*/ 1,
        /*.adprop_enabled =*/ 0,
        /*.varistep_enabled =*/ 0,
        /*.nlp_enabled =*/ 1,
        /*.clip_enabled =*/ 0,
        /*.stsupp_enabled =*/ 0,
        /*.hfsupp_enabled =*/ 0,
        /*.constrain_enabled =*/ 0,
        /*.ns_enabled =*/ 0,
        /*.cng_enabled =*/ 0,
        /*.blocks =*/ 6,
        /*.delay =*/ 0,
        /*.gamma =*/ 0.9,
        /*.echo_band_start =*/ 300,
        /*.echo_band_end =*/ 1800,
        /*.min_ovrd =*/ 2,
        /*.target_supp =*/ -40,
        /*.highfre_band_start =*/ 4000,
        /*.highfre_supp   =   */ 8.f,
        /*.noise_supp =*/ -15,
        /*.cng_type =*/ 1,
        /*.cng_level =*/ -60,
        /*.clip_threshold =*/ -20.f,
        /*.banks =*/ 64,
    };
#endif

    POSSIBLY_UNUSED int fs = g_capture_cfg.sample_rate;
    POSSIBLY_UNUSED int frame_size = g_capture_cfg.frame_len;

    TRACE(1,"Gary test!!");
#ifdef SPEECH_TX_NS
    ns4_st = speech_ns4_create(fs, frame_size, &ns4_cfg);
#endif
#ifdef SPEECH_TX_AEC
    ec2float_st = ec2float_create(fs, frame_size, 0, &config);
#endif
#if defined(CALC_MIPS_CNT_THD_MS)
    float frame_ms = (float)(g_capture_cfg.frame_len * 1000) / (float)g_capture_cfg.sample_rate;
    stream_mcps_start("CALL_TX", SYSFREQ_VALUE_FOR_CALC_MCPS, frame_ms, CALC_MIPS_CNT_THD_MS);
    TRACE(4, "[%s] sample rate=%d, frams ms=%.2f, cnt_ms=%d", __func__,
                        g_capture_cfg.sample_rate, (double)frame_ms, CALC_MIPS_CNT_THD_MS);
#endif
    return 0;
}

int32_t algo_process_call_capture_close(void)
{
    TRACE(1, "[%s] ...", __func__);

    // Add capture algo close func

#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_stop("CALL_TX");
#endif
    return 0;
}

int32_t algo_process_call_capture_set_cfg(uint8_t *cfg, uint32_t len)
{
    ASSERT(cfg != NULL, "[%s] cfg is NULL", __func__);

    // Set your algo config or tuning algo config

    return 0;
}

int32_t algo_process_call_capture_ctl(uint32_t ctl, uint8_t *ptr, uint32_t ptr_len)
{
    ASSERT(ptr != NULL, "[%s] ptr is NULL", __func__);

    // Ctrl your algo params or return algo params

    return 0;
}

int32_t algo_process_call_capture_process(uint8_t *in[PCM_CHANNEL_INDEX_QTY], uint8_t *out, uint32_t frame_len)
{
#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_run_pre("CALL_TX");
#endif

    if (g_capture_cfg.sample_bytes == sizeof(int16_t)){
        int16_t **pcm_in = (int16_t **)in;
        int16_t *pcm_out = (int16_t *)out;
#if 1
        int16_t *pcm_buf = pcm_in[PCM_CHANNEL_INDEX_MIC1];
        int16_t *ref_buf = pcm_in[PCM_CHANNEL_INDEX_REF];

#ifdef SPEECH_TX_AEC
        ec2float_process(ec2float_st, pcm_buf, ref_buf, frame_len, pcm_buf);
#endif
#ifdef SPEECH_TX_NS
        speech_ns4_process(ns4_st, pcm_buf, frame_len);
#endif

        for(uint32_t i = 0; i < frame_len; i++) {
            pcm_out[i] = pcm_buf[i];
        }
#else
        for (uint32_t i = 0; i < frame_len; i++) {
            pcm_out[i] = pcm_in[0][i];
        }
#endif
    } else if (g_capture_cfg.sample_bytes == sizeof(int32_t)){
        int32_t **pcm_in = (int32_t **)in;
        int32_t *pcm_out = (int32_t *)out;
#if 0
        // Add your 24bit algo process
#else
        for (uint32_t i = 0; i < frame_len; i++) {
            pcm_out[i] = pcm_in[0][i] >> 1;
        }
#endif
    }
#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_run_post("CALL_TX");
#endif
    return 0;
}

const algo_process_stream_t algo_process_call_capture = {
    .open    = algo_process_call_capture_open,
    .close   = algo_process_call_capture_close,
    .set_cfg = algo_process_call_capture_set_cfg,
    .ctl     = algo_process_call_capture_ctl,
    .process = algo_process_call_capture_process,
};

/***************************playback***************************/

int32_t algo_process_call_playback_open(algo_process_cfg_t *cfg)
{
    g_playback_cfg = *cfg;
    TRACE(4, "[%s] sample_rate=%d, frame_len=%d, mic_num=%d, mode=%d", __func__, \
                                        g_playback_cfg.sample_rate, \
                                        g_playback_cfg.frame_len, \
                                        g_playback_cfg.mic_num, \
                                        g_playback_cfg.mode);

    // Add playback algo open func

#if defined(CALC_MIPS_CNT_THD_MS)
    float frame_ms = (float)(g_playback_cfg.frame_len * 1000) / (float)g_playback_cfg.sample_rate;
    stream_mcps_start("CALL_RX", SYSFREQ_VALUE_FOR_CALC_MCPS, frame_ms, CALC_MIPS_CNT_THD_MS);
    TRACE(4, "[%s] sample rate=%d, frams ms=%.2f, cnt_ms=%d", __func__,
                        g_playback_cfg.sample_rate, (double)frame_ms, CALC_MIPS_CNT_THD_MS);
#endif
    return 0;
}

int32_t algo_process_call_playback_close(void)
{
    TRACE(1, "[%s] ...", __func__);

    // Add playback algo close func

#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_stop("CALL_RX");
#endif
    return 0;
}

int32_t algo_process_call_playback_set_cfg(uint8_t *cfg, uint32_t len)
{
    ASSERT(cfg != NULL, "[%s] cfg is NULL", __func__);

    // Set your algo config or tuning algo config

    return 0;
}

int32_t algo_process_call_playback_ctl(uint32_t ctl, uint8_t *ptr, uint32_t ptr_len)
{
    ASSERT(ptr != NULL, "[%s] cfg is NULL", __func__);

    // Ctrl your algo params or return algo params

    return 0;
}

static int32_t _algo_process_call_playback_process(uint8_t *in, uint8_t *out, uint32_t frame_len)
{
#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_run_pre("CALL_RX");
#endif

    if (g_playback_cfg.sample_bytes == sizeof(int16_t)){
        int16_t *pcm_in = (int16_t *)in;
        int16_t *pcm_out = (int16_t *)out;
#if 0
        // Add your 16bit algo process
#else
        for (uint32_t i = 0; i < frame_len; i++) {
            pcm_out[i] = pcm_in[i];
        }
#endif
    } else if (g_playback_cfg.sample_bytes == sizeof(int32_t)){
        int32_t *pcm_in = (int32_t *)in;
        int32_t *pcm_out = (int32_t *)out;
#if 0
        // Add your 24bit algo process
#else
        for (uint32_t i = 0; i < frame_len; i++) {
            pcm_out[i] = pcm_in[i];
        }
#endif
    }

#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_run_post("CALL_RX");
#endif
    return 0;
}


int32_t algo_process_call_playback_process(uint8_t *in[PCM_CHANNEL_INDEX_QTY], uint8_t *out, uint32_t frame_len)
{
    return _algo_process_call_playback_process(in[0], out, frame_len);
}

const algo_process_stream_t algo_process_call_playback = {
    .open    = algo_process_call_playback_open,
    .close   = algo_process_call_playback_close,
    .set_cfg = algo_process_call_playback_set_cfg,
    .ctl     = algo_process_call_playback_ctl,
    .process = algo_process_call_playback_process,
};
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
#include "stream_mcps.h"

static algo_process_cfg_t g_playback_cfg = {0};

/***************************playback***************************/

int32_t algo_process_audio_playback_open(algo_process_cfg_t *cfg)
{
    g_playback_cfg = *cfg;
    TRACE(1,"%s",__func__);
    TRACE(4, "sample_rate=%d, frame_len=%d, mic_num=%d, mode=%d", \
                                        g_playback_cfg.sample_rate, \
                                        g_playback_cfg.frame_len, \
                                        g_playback_cfg.mic_num, \
                                        g_playback_cfg.mode);

    // Add playback algo open func

#if defined(CALC_MIPS_CNT_THD_MS)
    float frame_ms = (float)(g_playback_cfg.frame_len * 1000) / (float)g_playback_cfg.sample_rate;
    stream_mcps_start("AUDIO_RX", SYSFREQ_VALUE_FOR_CALC_MCPS, frame_ms, CALC_MIPS_CNT_THD_MS);
    TRACE(4, "[%s] sample rate=%d, frams ms=%.2f, cnt_ms=%d", __func__,
                        g_playback_cfg.sample_rate, (double)frame_ms, CALC_MIPS_CNT_THD_MS);
#endif
    return 0;
}

int32_t algo_process_audio_playback_close(void)
{
    TRACE(1, "[%s] ...", __func__);

    // Add playback algo close func

#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_stop("AUDIO_RX");
#endif
    return 0;
}

int32_t algo_process_audio_playback_set_cfg(uint8_t *cfg, uint32_t len)
{
    ASSERT(cfg != NULL, "[%s] cfg is NULL", __func__);

    // Set your algo config or tuning algo config

    return 0;
}

int32_t algo_process_audio_playback_ctl(uint32_t ctl, uint8_t *ptr, uint32_t ptr_len)
{
    ASSERT(ptr != NULL, "[%s] cfg is NULL", __func__);

    // Ctl your algo or return algo params

    return 0;
}

static int32_t _algo_process_audio_playback_process(uint8_t *in, uint8_t *out, uint32_t frame_len)
{
#if defined(CALC_MIPS_CNT_THD_MS)
    stream_mcps_run_pre("AUDIO_RX");
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
    stream_mcps_run_post("AUDIO_RX");
#endif
    return 0;
}

int32_t algo_process_audio_playback_process(uint8_t *in[PCM_CHANNEL_INDEX_QTY], uint8_t *out, uint32_t frame_len)
{
    return _algo_process_audio_playback_process(in[0], out, frame_len);
}

const algo_process_stream_t algo_process_audio_playback = {
    .open    = algo_process_audio_playback_open,
    .close   = algo_process_audio_playback_close,
    .set_cfg = algo_process_audio_playback_set_cfg,
    .ctl     = algo_process_audio_playback_ctl,
    .process = algo_process_audio_playback_process,
};
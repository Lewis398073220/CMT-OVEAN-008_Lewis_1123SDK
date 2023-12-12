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
#include "gaf_cc_stream_process.h"
#include "cmsis.h"
#include "string.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "speech_memory.h"
#include "iir_resample.h"
#include "app_mcpp.h"
#include "algo_process.h"
#include "cc_stream_common.h"
#define GAF_CC_UPSAMPLING_POOL_SIZE   (2 * 1024)


static TX_ALGO_CFG_T g_tx_algo_cfg = {0};
static bool g_tx_algo_enabled = false;
static uint8_t *gaf_cc_speech_heap_buf = NULL;
static CC_HEAP_ALLOC g_buf_alloc = NULL;

static uint32_t g_capture_upsampling_factor = 1;
static uint8_t *g_capture_upsampling_buf = NULL;
static uint32_t g_capture_upsampling_buf_size = 0;
static IirResampleState *g_capture_upsampling_st = NULL;

int32_t gaf_cc_stream_process_init(void)
{
    return 0;
}

int32_t gaf_cc_stream_process_deinit(void)
{
    return 0;
}

int32_t gaf_cc_stream_process_ctrl(void)
{
    return 0;
}

uint32_t gaf_cc_stream_process_need_capture_buf_size(void)
{
    uint32_t size = 0;

    size += GAF_CC_UPSAMPLING_POOL_SIZE;

#ifdef APP_MCPP_CLI_M55
    size += get_algo_process_heap_buffer_size(APP_MCPP_USER_CALL);
#endif
    TRACE(0, "[%s] size: %d", __func__, size);

    return size;
}

int32_t gaf_cc_stream_process_set_buf_alloc(void *buf_alloc)
{
    g_buf_alloc = (CC_HEAP_ALLOC)buf_alloc;

    return 0;
}

static void gaf_cc_stream_capture_upsampling_open(TX_ALGO_CFG_T *algo_cfg, CODEC_INFO_T *encoder_info)
{
    if (algo_cfg->sample_rate < encoder_info->sample_rate) {
        uint32_t encoder_sample = encoder_info->sample_rate;
        uint32_t algo_sample = algo_cfg->sample_rate;
        ASSERT(encoder_sample % algo_sample == 0,
            "%s error. %d, %d. Can not do resample", __func__, encoder_sample, algo_sample);

        g_capture_upsampling_factor = encoder_info->sample_rate / algo_cfg->sample_rate;

        if (algo_cfg->channel_num < g_capture_upsampling_factor){
            TRACE(0, "[%s] ch_num: %d, factor: %d",
                __func__, algo_cfg->channel_num, g_capture_upsampling_factor);
            g_capture_upsampling_buf_size = encoder_info->pcm_size;
            if (!g_capture_upsampling_buf) {
                g_capture_upsampling_buf = (uint8_t*)g_buf_alloc(NULL, g_capture_upsampling_buf_size);
            }
        } else {
            g_capture_upsampling_buf_size = 0;
            g_capture_upsampling_buf = NULL;
        }
        TRACE(0, "[%s] Resample %d--> %d. factor: %d",
            __func__, algo_sample, encoder_sample, g_capture_upsampling_factor);

        g_capture_upsampling_st = multi_iir_resample_init(algo_cfg->frame_len,
                                                        algo_cfg->bits,
                                                        1,
                                                        iir_resample_choose_mode(algo_sample, encoder_sample));
    } else {
        g_capture_upsampling_st = NULL;
        g_capture_upsampling_factor = 1;
        g_capture_upsampling_buf_size = 0;
    }
}

int32_t gaf_cc_stream_capture_process_open(void *algo_cfg, void *encoder_info)
{
    g_tx_algo_cfg = *(TX_ALGO_CFG_T*)algo_cfg;

    if (g_tx_algo_cfg.channel_num == 0) {
        return 0;
    }

    uint32_t heap_size = gaf_cc_stream_process_need_capture_buf_size();
    if (!gaf_cc_speech_heap_buf) {
        gaf_cc_speech_heap_buf = (uint8_t *)g_buf_alloc(NULL, heap_size);
    }
    speech_heap_init(gaf_cc_speech_heap_buf, heap_size);
    gaf_cc_stream_capture_upsampling_open((TX_ALGO_CFG_T*)algo_cfg, (CODEC_INFO_T*)encoder_info);

    if (g_tx_algo_cfg.bypass) {
        g_tx_algo_enabled = false;
        return 0;
    } else {
        g_tx_algo_enabled = true;
    }

    TRACE(0, "[LE Call] bypass:%d,frame_len:%d,sample_rate:%d,ch_num:%d,bits:%d",
        g_tx_algo_cfg.bypass,
        g_tx_algo_cfg.frame_len,
        g_tx_algo_cfg.sample_rate,
        g_tx_algo_cfg.channel_num,
        g_tx_algo_cfg.bits);

#if defined(APP_MCPP_CLI_M55)
    APP_MCPP_CFG_T dsp_cfg;
    uint32_t sample_bytes = g_tx_algo_cfg.bits <= 16 ? 2 : 4;

    memset(&dsp_cfg, 0, sizeof(APP_MCPP_CFG_T));
    dsp_cfg.capture.stream_enable  = true;
    dsp_cfg.capture.sample_rate    = g_tx_algo_cfg.sample_rate;
    dsp_cfg.capture.sample_bytes   = sample_bytes;
    dsp_cfg.capture.frame_len      = g_tx_algo_cfg.frame_len;
    dsp_cfg.capture.algo_frame_len = g_tx_algo_cfg.algo_frame_len;
    dsp_cfg.capture.channel_num    = g_tx_algo_cfg.channel_num;
    dsp_cfg.capture.core_server    = APP_MCPP_CORE_M55;

    dsp_cfg.playback.stream_enable  = false;

    app_mcpp_open(APP_MCPP_USER_CALL, &dsp_cfg);
#endif

    return 0;
}

static void gaf_cc_stream_capture_upsampling_close(void)
{
    if (g_capture_upsampling_st) {
        iir_resample_destroy(g_capture_upsampling_st);
        g_capture_upsampling_st = NULL;
        g_capture_upsampling_factor = 1;
    }

    g_capture_upsampling_buf_size = 0;
}

int32_t gaf_cc_stream_capture_process_close(void)
{
    if (g_tx_algo_cfg.channel_num == 0) {
        memset(&g_tx_algo_cfg, 0, sizeof(TX_ALGO_CFG_T));
        return 0;
    }

    if (g_buf_alloc == NULL) {
        return 0;
    }

    gaf_cc_stream_capture_upsampling_close();

    if (g_tx_algo_enabled) {
#if defined(APP_MCPP_CLI_M55)
        app_mcpp_close(APP_MCPP_USER_CALL);
#endif
    }

    g_tx_algo_enabled = false;
    g_buf_alloc = NULL;
    memset(&g_tx_algo_cfg, 0, sizeof(TX_ALGO_CFG_T));

    uint32_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(4, "[%s] total=%d, used=%d, max_used=%d.", __func__, total, used, max_used);

    return 0;
}

uint32_t POSSIBLY_UNUSED gaf_cc_stream_capture_upsampling_run(uint8_t *buf, uint32_t len)
{
    uint32_t POSSIBLY_UNUSED sample_bytes = g_tx_algo_cfg.bits <= 16 ? 2 : 4;
    uint32_t POSSIBLY_UNUSED pcm_len = len / sample_bytes;

    if (g_capture_upsampling_st) {
        if (g_capture_upsampling_buf){
            iir_resample_process(g_capture_upsampling_st, buf, g_capture_upsampling_buf, pcm_len);
        } else {
            iir_resample_process(g_capture_upsampling_st, buf, buf, pcm_len);
        }
        len *= g_capture_upsampling_factor;
    }
    return len;
}

uint32_t gaf_cc_stream_capture_process_run(uint8_t *buf, uint32_t len)
{
    if (g_tx_algo_cfg.channel_num == 0) {
        return len;
    }

    if (g_buf_alloc == NULL) {
        return len;
    }

#if defined(APP_MCPP_CLI_M55)
    if (g_tx_algo_enabled) {
        APP_MCPP_CAP_PCM_T pcm_cfg;
        memset(&pcm_cfg, 0, sizeof(pcm_cfg));
        pcm_cfg.in = buf;
        pcm_cfg.out = buf;
        pcm_cfg.frame_len = g_tx_algo_cfg.frame_len;

        app_mcpp_capture_process(APP_MCPP_USER_CALL, &pcm_cfg);
    } else
#endif
    {
        if (g_tx_algo_cfg.bits == 16) {
            int16_t *pcm_buf = (int16_t *)buf;
            for (uint32_t i = 0; i < g_tx_algo_cfg.frame_len; i++) {
                pcm_buf[i] = pcm_buf[i * g_tx_algo_cfg.channel_num + 0];
            }
        } else if (g_tx_algo_cfg.bits == 24) {
            int32_t *pcm_buf = (int32_t *)buf;
            for (uint32_t i = 0; i < g_tx_algo_cfg.frame_len; i++) {
                pcm_buf[i] = pcm_buf[i * g_tx_algo_cfg.channel_num + 0];
            }
        }
    }

    len /= g_tx_algo_cfg.channel_num;

    len = gaf_cc_stream_capture_upsampling_run(buf, len);

    return len;
}

void gaf_cc_stream_process_buf_deinit(void)
{
    gaf_cc_speech_heap_buf = NULL;
    g_capture_upsampling_buf = NULL;
}

static CC_CAPTURE_ALGO_FUNC_LIST_T cc_capture_algo_func_list =
{
    .capture_algo_run = gaf_cc_stream_capture_process_run,
    .capture_algo_open = gaf_cc_stream_capture_process_open,
    .capture_algo_close = gaf_cc_stream_capture_process_close,
    .capture_algo_buf_init = gaf_cc_stream_process_set_buf_alloc,
    .capture_algo_buf_deinit = gaf_cc_stream_process_buf_deinit,
};

void gaf_cc_capture_algo_func_register(void *_func_list)
{
    CC_CAPTURE_ALGO_FUNC_LIST_T **func_list = (CC_CAPTURE_ALGO_FUNC_LIST_T**)_func_list;
    *func_list = &cc_capture_algo_func_list;
}

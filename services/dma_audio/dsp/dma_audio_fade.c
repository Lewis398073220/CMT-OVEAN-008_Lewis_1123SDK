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
#include "string.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "dma_audio_public.h"
#include "dma_audio_def.h"

static uint16_t g_fade_in_skip_max_cnt = 0;
static uint32_t g_sample_bytes = 2;
static uint32_t g_frame_len = 0;
static uint32_t g_fade_in_skip_cnt = 0;
static uint32_t g_fade_in_cnt = 0;
static uint32_t g_fade_in_max_cnt = 0;
static uint32_t g_fade_out_cnt = 0;
static uint32_t g_fade_out_max_cnt = 0;

int32_t dma_audio_fade_init(const struct DAUD_STREAM_CONFIG_T *cfg)
{
    // TRACE(2, "[%s] bits: %d channel_num: %d sample_rate: %d", __func__, cfg->bits, cfg->channel_num, cfg->sample_rate);
    g_frame_len = (uint32_t)(cfg->sample_rate / 1000);
    g_sample_bytes = ((cfg->bits + 8) / 16) * sizeof(int16_t);
    g_fade_in_skip_max_cnt = g_frame_len * DAUD_STREAM_SKIP_FADE_MS;
    g_fade_in_skip_cnt = 0;
    g_fade_in_cnt = 0;
    g_fade_out_cnt = 0;
    g_fade_out_max_cnt = 0;

    return 0;
}

int32_t dma_audio_fade_deinit(void)
{
    g_fade_in_skip_cnt = 0;
    g_fade_in_cnt = 0;
    g_fade_out_cnt = 0;
    g_fade_out_max_cnt = 0;

    return 0;
}

int32_t dma_audio_set_fadein_time(uint32_t ms)
{
/* Solve the problem that fadeout always exists after turning off the macro DMA_AUDIO_APP_DYN_ON*/
    g_fade_out_max_cnt = 0;
    g_fade_out_cnt = 0;

    g_fade_in_skip_cnt = 0;
    g_fade_in_cnt = 0;
    g_fade_in_max_cnt = g_frame_len * ms;

    return 0;
}

int32_t dma_audio_set_fadeout_time(uint32_t ms)
{
    g_fade_out_cnt = 0;
    g_fade_out_max_cnt = g_frame_len * ms;

    return 0;
}

static int32_t dma_audio_fade_process_int16(uint32_t ch_num, uint8_t *buf, uint32_t len)
{
    uint32_t frame_len = len / sizeof(int16_t) / ch_num;

    if (g_fade_in_skip_cnt < g_fade_in_skip_max_cnt) {
        // TRACE(2, "[%s] skip cnt: %d, g_fade_in_max_cnt: %d", __func__, g_fade_in_skip_cnt, g_fade_in_max_cnt);
        g_fade_in_skip_cnt +=  frame_len;
        memset(buf, 0, len);
    } else if (g_fade_in_max_cnt) {
        int16_t *pcm_buf = (int16_t *)buf;
        // TRACE(2, "[%s] fadein cnt: %d frame_len: %d max_cnt: %d", __func__, g_fade_in_cnt, frame_len, g_fade_in_max_cnt);
        for (uint32_t i=0; i<frame_len; i++) {
            g_fade_in_cnt++;
            if (g_fade_in_cnt > g_fade_in_max_cnt) {
                g_fade_in_cnt = g_fade_in_max_cnt;
            }
            pcm_buf[ch_num * i] = (int16_t)(pcm_buf[ch_num * i] * (((float)g_fade_in_cnt) / g_fade_in_max_cnt));
            if (ch_num >= 2) {
                pcm_buf[ch_num * i + 1] = (int16_t)(pcm_buf[ch_num * i + 1] * (((float)g_fade_in_cnt) / g_fade_in_max_cnt));
            }
        }
        if (g_fade_in_cnt >= g_fade_in_max_cnt) {
            g_fade_in_cnt = 0;
            g_fade_in_max_cnt = 0;
        }
    } else if (g_fade_out_max_cnt) {
        int16_t *pcm_buf = (int16_t *)buf;
        // TRACE(2, "[%s] fadeout cnt: %d", __func__, g_fade_out_cnt);
        for (int16_t i=0; i<frame_len; i++) {
            g_fade_out_cnt++;
            if (g_fade_out_cnt > g_fade_out_max_cnt) {
                g_fade_out_cnt = g_fade_out_max_cnt;
            }
            pcm_buf[ch_num * i] = (int16_t)(pcm_buf[ch_num * i] * (((float)(g_fade_out_max_cnt - g_fade_out_cnt)) / g_fade_out_max_cnt));
            if (ch_num >= 2) {
                pcm_buf[ch_num * i + 1] = (int16_t)(pcm_buf[ch_num * i + 1] * (((float)(g_fade_out_max_cnt - g_fade_out_cnt)) / g_fade_out_max_cnt));
            }
        }
    }

    return 0;
}

static int32_t dma_audio_fade_process_int32(uint32_t ch_num, uint8_t *buf, uint32_t len)
{
    uint32_t frame_len = len / sizeof(int32_t) / ch_num;

    if (g_fade_in_skip_cnt < g_fade_in_skip_max_cnt) {
        // TRACE(2, "[%s] skip cnt: %d, g_fade_in_max_cnt: %d", __func__, g_fade_in_skip_cnt, g_fade_in_max_cnt);
        g_fade_in_skip_cnt +=  frame_len;
        memset(buf, 0, len);
    } else if (g_fade_in_max_cnt) {
        int32_t *pcm_buf = (int32_t *)buf;
        // TRACE(2, "[%s] fadein cnt: %d frame_len: %d max_cnt: %d", __func__, g_fade_in_cnt, frame_len, g_fade_in_max_cnt);
        for (uint32_t i=0; i<frame_len; i++) {
            g_fade_in_cnt++;
            if (g_fade_in_cnt > g_fade_in_max_cnt) {
                g_fade_in_cnt = g_fade_in_max_cnt;
            }
            pcm_buf[ch_num * i] = (int32_t)(pcm_buf[ch_num * i] * (((float)g_fade_in_cnt) / g_fade_in_max_cnt));
            if (ch_num >= 2) {
                pcm_buf[ch_num * i + 1] = (int32_t)(pcm_buf[ch_num * i + 1] * (((float)g_fade_in_cnt) / g_fade_in_max_cnt));
            }
        }
        if (g_fade_in_cnt >= g_fade_in_max_cnt) {
            g_fade_in_cnt = 0;
            g_fade_in_max_cnt = 0;
        }
    } else if (g_fade_out_max_cnt) {
        int32_t *pcm_buf = (int32_t *)buf;
        // TRACE(2, "[%s] fadeout cnt: %d", __func__, g_fade_out_cnt);
        for (uint32_t i=0; i<frame_len; i++) {
            g_fade_out_cnt++;
            if (g_fade_out_cnt > g_fade_out_max_cnt) {
                g_fade_out_cnt = g_fade_out_max_cnt;
            }
            pcm_buf[ch_num * i] = (int32_t)(pcm_buf[ch_num * i] * (((float)(g_fade_out_max_cnt - g_fade_out_cnt)) / g_fade_out_max_cnt));
            if (ch_num >= 2) {
                pcm_buf[ch_num * i + 1] = (int32_t)(pcm_buf[ch_num * i + 1] * (((float)(g_fade_out_max_cnt - g_fade_out_cnt)) / g_fade_out_max_cnt));
            }
        }
    }

    return 0;
}

int32_t dma_audio_fade_process(uint32_t ch_num, uint8_t *buf, uint32_t len)
{
    if (g_sample_bytes == sizeof(int16_t)) {
        dma_audio_fade_process_int16(ch_num, buf, len);
    } else {
        dma_audio_fade_process_int32(ch_num, buf, len);
    }

    return 0;
}
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
#include <math.h>
#include "signal_generator.h"

// DEFINE
#define _PI               3.14159265358979f

// Modify this to support ultra low sample rate. e.g. 5Hz
#define MULTI_BUF       (1)

// NOTE: Support sample rate: x.xk
#define PERIOD_CNT      (10)

// max sample rate: 48k
#define PCM_BUF_LEN     (48 * sizeof(int) * PERIOD_CNT * MULTI_BUF)


// global parameters
static uint32_t g_sample_bytes = 2;
static uint32_t g_channel_num = 1;

static uint32_t g_pcm_index = 0;
static uint32_t g_pcm_len = 0;
static uint8_t g_pcm_buf[PCM_BUF_LEN];

void signal_generator_init(SG_TYPE_T type, uint32_t sample_rate, uint32_t bits, uint32_t channel_num, float gain_dB)
{
    uint32_t period_samples = 0;
    uint32_t period_cnt = 1;
    int32_t  bits_max_val = 0;
    int16_t *g_pcm_buf_int16 = (int16_t *)g_pcm_buf;
    int32_t *g_pcm_buf_int32 = (int32_t *)g_pcm_buf;
    double gain = powf(10, gain_dB / 20);

    TRACE(5, "[%s] type=%d, %dHz, %dbits, %dch, gain(x1000)=%d.", __func__, type, sample_rate, bits, channel_num, (int32_t)(gain * 1000));
    ASSERT((bits == 16) || (bits == 24) || (bits == 32), "[%s] bits(%d) is invalid", __func__, bits);

    g_sample_bytes = ((bits + 8) / 16) * sizeof(int16_t);
    g_channel_num =  channel_num;
    g_pcm_index = 0;
    bits_max_val = ((0x00000001 << (bits-1)) - 1);

    if (type <= SG_TYPE_TONE_MAX) {
        ASSERT(type <= sample_rate / 2, "[%s] type > sample_rate / 2", __func__);
        for (period_cnt=1; period_cnt <= PERIOD_CNT; period_cnt++) {
            if ((type % period_cnt == 0) && (sample_rate % (type / period_cnt) == 0)) {
                period_samples = sample_rate / (type / period_cnt);
                break;
            }
        }
        ASSERT(period_cnt <= PERIOD_CNT, "[%s] period_cnt > PERIOD_CNT", __func__);
    }

    ASSERT(period_samples * g_sample_bytes <= PCM_BUF_LEN, "[%s] period_samples(%d) > PCM_BUF_LEN", __func__, period_samples);

    for (uint32_t i=0; i<period_samples; i++) {
        if (g_sample_bytes == sizeof(int16_t)) {
            g_pcm_buf_int16[i] = (int16_t)(sin(i * 2 * _PI * period_cnt / period_samples) * gain * bits_max_val);
        } else {
            g_pcm_buf_int32[i] = (int32_t)(sin(i * 2 * _PI * period_cnt  / period_samples) * gain * bits_max_val);
        }

    }

    g_pcm_len = period_samples;

    TRACE(3, "[%s] period_cnt = %d, g_pcm_len = %d", __func__, period_cnt, g_pcm_len);
}

void signal_generator_deinit(void)
{
    ;
}

void signal_generator_get_data(void *pcm_buf, uint32_t frame_len)
{
    ;
}

void signal_generator_loop_get_data(void *pcm_buf, uint32_t frame_len)
{
    int16_t *g_pcm_buf_int16 = (int16_t *)g_pcm_buf;
    int32_t *g_pcm_buf_int32 = (int32_t *)g_pcm_buf;

        for (uint32_t i = 0; i < frame_len; i++) {
            for (uint32_t ch = 0; ch < g_channel_num; ch++) {
                if (g_sample_bytes == sizeof(int16_t)) {
                    ((int16_t *)pcm_buf)[g_channel_num * i + ch] = g_pcm_buf_int16[g_pcm_index];
                } else {
                    ((int32_t *)pcm_buf)[g_channel_num * i + ch] = g_pcm_buf_int32[g_pcm_index];
                }
            }

            g_pcm_index++;
            g_pcm_index = g_pcm_index % g_pcm_len;
        }
}
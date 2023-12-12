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
#ifdef DMA_AUDIO_SW_RESAMPLE
#include "string.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_aud.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "dma_audio_resample.h"
#include "dma_audio.h"

#ifdef DMA_AUDIO_RESAMPLE_EX
static const float iir_resample_table_96k_to_48k[9] = {0.071510,
                                                       0.286042,
                                                       0.429063,
                                                       0.286042,
                                                       0.071511,
                                                      -0.325500,
                                                       0.519536,
                                                      -0.069598,
                                                       0.019730};

static const float iir_resample_table_96k_to_16k[9] = {0.000933,
                                                       0.003734,
                                                       0.005601,
                                                       0.003734,
                                                       0.000933,
                                                      -2.976844,
                                                       3.422310,
                                                      -1.786107,
                                                       0.355577};

static const float iir_resample_table_48k_to_16k[9] = {0.010209,
                                                       0.040838,
                                                       0.061257,
                                                       0.040838,
                                                       0.010209,
                                                      -1.968428,
                                                       1.735861,
                                                      -0.724471,
                                                       0.120390};

static const float iir_resample_table_96k_to_8k[9] = {0.000073,
                                                      0.000291,
                                                      0.000437,
                                                      0.000291,
                                                      0.000073,
                                                     -3.487308,
                                                      4.589291,
                                                     -2.698884,
                                                      0.598065};

static int32_t sampling_ssat(int32_t val, uint32_t sat)
{
    if ((sat >= 1U) && (sat <= 32U)) {
        const int32_t max = (int32_t)((1U << (sat - 1U)) - 1U);
        const int32_t min = -1 - max ;
        if (val > max) {
            return max;
        } else if (val < min) {
            return min;
        }
    }
    return val;
}

static void POSSIBLY_UNUSED _downsampling_filter(float *rs_coeffs,
                                                 float *rs_state,
                                                 uint8_t *in,
                                                 uint8_t *out,
                                                 int32_t pcm_len,
                                                 uint32_t bits,
                                                 uint32_t stride)
{
    float y1 = 0, y2 = 0, y3 = 0, y4 = 0;
    float x0 = 0, x1 = 0, x2 = 0, x3 = 0, x4 = 0;

    float b0, b1, b2, b3, b4, a1, a2, a3, a4;
    float acc;

    b0 = *rs_coeffs++;
    b1 = *rs_coeffs++;
    b2 = *rs_coeffs++;
    b3 = *rs_coeffs++;
    b4 = *rs_coeffs++;
    a1 = *rs_coeffs++;
    a2 = *rs_coeffs++;
    a3 = *rs_coeffs++;
    a4 = *rs_coeffs++;

    x1 = rs_state[0];
    x2 = rs_state[1];
    x3 = rs_state[2];
    x4 = rs_state[3];
    y1 = rs_state[4];
    y2 = rs_state[5];
    y3 = rs_state[6];
    y4 = rs_state[7];

    uint32_t sample;

    if (bits == 16){
        int16_t *in_buf = (int16_t *)in;
        int16_t *out_buf = (int16_t *)out;
        sample = pcm_len >> 3U;

        while (sample > 0U) {
            x0 = *in_buf;
            in_buf += stride;
            y4 = (b0 * x0) + (b1 * x1) + (b2 * x2) + (b3 * x3) + (b4 * x4) + (a1 * y1) + (a2 * y2) + (a3 * y3) + (a4 * y4);
            *out_buf = sampling_ssat((int32_t)y4, bits);
            out_buf += stride;

            x4 = *in_buf;
            in_buf += stride;
            y3 = (b0 * x4) + (b1 * x0) + (b2 * x1) + (b3 * x2) + (b4 * x3) + (a1 * y4) + (a2 * y1) + (a3 * y2) + (a4 * y3);
            *out_buf = sampling_ssat((int32_t)y3, bits);
            out_buf += stride;

            x3 = *in_buf;
            in_buf += stride;
            y2 = (b0 * x3) + (b1 * x4) + (b2 * x0) + (b3 * x1) + (b4 * x2) + (a1 * y3) + (a2 * y4) + (a3 * y1) + (a4 * y2);
            *out_buf = sampling_ssat((int32_t)y2, bits);
            out_buf += stride;

            x2 = *in_buf;
            in_buf += stride;
            y1 = (b0 * x2) + (b1 * x3) + (b2 * x4) + (b3 * x0) + (b4 * x1) + (a1 * y2) + (a2 * y3) + (a3 * y4) + (a4 * y1);
            *out_buf = sampling_ssat((int32_t)y1, bits);
            out_buf += stride;

            x1 = *in_buf;
            in_buf += stride;
            y4 = (b0 * x1) + (b1 * x2) + (b2 * x3) + (b3 * x4) + (b4 * x0) + (a1 * y1) + (a2 * y2) + (a3 * y3) + (a4 * y4);
            *out_buf = sampling_ssat((int32_t)y4, bits);
            out_buf += stride;

            x0 = *in_buf;
            in_buf += stride;
            y3 = (b0 * x0) + (b1 * x1) + (b2 * x2) + (b3 * x3) + (b4 * x4) + (a1 * y4) + (a2 * y1) + (a3 * y2) + (a4 * y3);
            *out_buf = sampling_ssat((int32_t)y3, bits);
            out_buf += stride;

            x4 = *in_buf;
            in_buf += stride;
            y2 = (b0 * x4) + (b1 * x0) + (b2 * x1) + (b3 * x2) + (b4 * x3) + (a1 * y3) + (a2 * y4) + (a3 * y1) + (a4 * y2);
            *out_buf = sampling_ssat((int32_t)y2, bits);
            out_buf += stride;

            x3 = *in_buf;
            in_buf += stride;
            y1 = (b0 * x3) + (b1 * x4) + (b2 * x0) + (b3 * x1) + (b4 * x2) + (a1 * y2) + (a2 * y3) + (a3 * y4) + (a4 * y1);
            *out_buf = sampling_ssat((int32_t)y1, bits);
            out_buf += stride;

            x2 = x4;
            x4 = x1;
            x1 = x3;
            x3 = x0;

            sample--;
        }
        sample = pcm_len & 0x7U;

        while (sample > 0U) {
            x0 = *in_buf;
            in_buf += stride;
            acc = (b0 * x0) + (b1 * x1) + (b2 * x2) + (b3 * x3) + (b4 * x4) + (a1 * y1) + (a2 * y2) + (a3 * y3) + (a4 * y4);
            *out_buf = sampling_ssat((int32_t)acc, bits);
            out_buf += stride;

            x4 = x3;
            x3 = x2;
            x2 = x1;
            x1 = x0;
            y4 = y3;
            y3 = y2;
            y2 = y1;
            y1 = acc;

            sample--;
        }
        *rs_state++ = x1;
        *rs_state++ = x2;
        *rs_state++ = x3;
        *rs_state++ = x4;
        *rs_state++ = y1;
        *rs_state++ = y2;
        *rs_state++ = y3;
        *rs_state++ = y4;
    } else if (bits == 24 || bits == 32){
        int32_t *in_buf = (int32_t *)in;
        int32_t *out_buf = (int32_t *)out;
        sample = pcm_len >> 3U;

        while (sample > 0U) {
            x0 = *in_buf;
            in_buf += stride;
            y4 = (b0 * x0) + (b1 * x1) + (b2 * x2) + (b3 * x3) + (b4 * x4) + (a1 * y1) + (a2 * y2) + (a3 * y3) + (a4 * y4);
            *out_buf = sampling_ssat((int32_t)y4, bits);
            out_buf += stride;

            x4 = *in_buf;
            in_buf += stride;
            y3 = (b0 * x4) + (b1 * x0) + (b2 * x1) + (b3 * x2) + (b4 * x3) + (a1 * y4) + (a2 * y1) + (a3 * y2) + (a4 * y3);
            *out_buf = sampling_ssat((int32_t)y3, bits);
            out_buf += stride;

            x3 = *in_buf;
            in_buf += stride;
            y2 = (b0 * x3) + (b1 * x4) + (b2 * x0) + (b3 * x1) + (b4 * x2) + (a1 * y3) + (a2 * y4) + (a3 * y1) + (a4 * y2);
            *out_buf = sampling_ssat((int32_t)y2, bits);
            out_buf += stride;

            x2 = *in_buf;
            in_buf += stride;
            y1 = (b0 * x2) + (b1 * x3) + (b2 * x4) + (b3 * x0) + (b4 * x1) + (a1 * y2) + (a2 * y3) + (a3 * y4) + (a4 * y1);
            *out_buf = sampling_ssat((int32_t)y1, bits);
            out_buf += stride;

            x1 = *in_buf;
            in_buf += stride;
            y4 = (b0 * x1) + (b1 * x2) + (b2 * x3) + (b3 * x4) + (b4 * x0) + (a1 * y1) + (a2 * y2) + (a3 * y3) + (a4 * y4);
            *out_buf = sampling_ssat((int32_t)y4, bits);
            out_buf += stride;

            x0 = *in_buf;
            in_buf += stride;
            y3 = (b0 * x0) + (b1 * x1) + (b2 * x2) + (b3 * x3) + (b4 * x4) + (a1 * y4) + (a2 * y1) + (a3 * y2) + (a4 * y3);
            *out_buf = sampling_ssat((int32_t)y3, bits);
            out_buf += stride;

            x4 = *in_buf;
            in_buf += stride;
            y2 = (b0 * x4) + (b1 * x0) + (b2 * x1) + (b3 * x2) + (b4 * x3) + (a1 * y3) + (a2 * y4) + (a3 * y1) + (a4 * y2);
            *out_buf = sampling_ssat((int32_t)y2, bits);
            out_buf += stride;

            x3 = *in_buf;
            in_buf += stride;
            y1 = (b0 * x3) + (b1 * x4) + (b2 * x0) + (b3 * x1) + (b4 * x2) + (a1 * y2) + (a2 * y3) + (a3 * y4) + (a4 * y1);
            *out_buf = sampling_ssat((int32_t)y1, bits);
            out_buf += stride;

            x2 = x4;
            x4 = x1;
            x1 = x3;
            x3 = x0;

            sample--;
        }
        sample = pcm_len & 0x7U;

        while (sample > 0U) {
            x0 = *in_buf;
            in_buf += stride;
            acc = (b0 * x0) + (b1 * x1) + (b2 * x2) + (b3 * x3) + (b4 * x4) + (a1 * y1) + (a2 * y2) + (a3 * y3) + (a4 * y4);
            *out_buf = sampling_ssat((int32_t)acc, bits);
            out_buf += stride;

            x4 = x3;
            x3 = x2;
            x2 = x1;
            x1 = x0;
            y4 = y3;
            y3 = y2;
            y2 = y1;
            y1 = acc;

            sample--;
        }
        *rs_state++ = x1;
        *rs_state++ = x2;
        *rs_state++ = x3;
        *rs_state++ = x4;
        *rs_state++ = y1;
        *rs_state++ = y2;
        *rs_state++ = y3;
        *rs_state++ = y4;
    }
}
#endif

void dma_audio_resample_init(struct DAUD_RESAMPLE_T *daud_resample)
{
    daud_resample->rs_buf = NULL;
    daud_resample->rs_buf_size = 0;
    daud_resample->rs_data_in_buf = NULL;
    daud_resample->rs_data_in_size = 0;
    daud_resample->rs_data_out_buf = NULL;
    daud_resample->rs_data_out_size = 0;
    daud_resample->rs_time_us = 0;
    daud_resample->rs_bits = 0;
    daud_resample->rs_chan_num = 0;
    daud_resample->ratio_step = 0;
    daud_resample->status.open = 0;
    daud_resample->status.enable = 0;
}

void dma_audio_resample_dump(struct DAUD_RESAMPLE_T *daud_resample)
{
    TRACE(1, "[%s]: daud_resample=%x", __func__, (int)daud_resample);
    TRACE(1, "rs_buf          =%x", (int)daud_resample->rs_buf);
    TRACE(1, "rs_buf_size     =%d", (int)daud_resample->rs_buf_size);
    TRACE(1, "rs_data_in_buf  =%x", (int)daud_resample->rs_data_in_buf);
    TRACE(1, "rs_data_in_size =%d", (int)daud_resample->rs_data_in_size);
    TRACE(1, "rs_data_out_buf =%x", (int)daud_resample->rs_data_out_buf);
    TRACE(1, "rs_data_out_size=%d", (int)daud_resample->rs_data_out_size);
    TRACE(1, "rs_bits         =%d", (int)daud_resample->rs_bits);
    TRACE(1, "rs_chan_num     =%d", (int)daud_resample->rs_chan_num);
    TRACE(1, "ratio_step      =%d", (int)daud_resample->ratio_step);
    TRACE(1, "open            =%d", (int)daud_resample->status.open);
    TRACE(1, "enable          =%d", (int)daud_resample->status.enable);
}

static uint32_t calc_data_in_buf_size(
    struct DAUD_STREAM_CONFIG_T *out_cfg,
    struct DAUD_STREAM_CONFIG_T *in_cfg)
{
    uint32_t in_samp_rate  = in_cfg->sample_rate;
    uint32_t in_chan_num   = in_cfg->channel_num;
    uint32_t in_data_size  = in_cfg->data_size;
    uint32_t in_samp_size  = daud_samp_bits_to_size(in_cfg->bits);
    uint32_t out_chan_num  = out_cfg->channel_num;
    uint32_t out_samp_size = daud_samp_bits_to_size(out_cfg->bits);
    uint32_t frm_ms;
    uint32_t buf_size;

    frm_ms = in_data_size / (in_samp_rate / 1000) / in_chan_num / in_samp_size;
    buf_size = (in_samp_rate / 1000) * out_chan_num * out_samp_size * frm_ms / 2; // div 2 for ping-pong
    return buf_size;
}

int dma_audio_resample_open(
    struct DAUD_RESAMPLE_T *daud_resample,
    struct DAUD_STREAM_CONFIG_T *out_cfg,
    struct DAUD_STREAM_CONFIG_T *in_cfg)
{
    uint8_t *rs_buf = NULL;
    uint32_t rs_size = 0;
    uint8_t *rs_data_in_buf = NULL;
    uint32_t rs_data_in_size = 0;
    uint8_t *rs_data_out_buf = NULL;
    uint32_t rs_data_out_size = 0;

    enum AUD_BITS_T bits;
    enum AUD_CHANNEL_NUM_T chan_num;
    enum AUD_SAMPRATE_T in_sample_rate;
    enum AUD_SAMPRATE_T out_sample_rate;
    int ratio_step;
    uint32_t lock;

    TRACE(1, "[%s]:", __func__);

    if (daud_resample->status.open) {
        ASSERT(false, "[%s]: has bean opened", __func__);
    }

    // set stream param
    bits            = out_cfg->bits;
    chan_num        = out_cfg->channel_num;
    out_sample_rate = out_cfg->sample_rate;
    in_sample_rate  = in_cfg->sample_rate;
    TRACE(1, "RSPL_STREAM_CFG: bits=%d,chan_num=%d,in_samprate=%d,out_samprate=%d",
        bits, chan_num, in_sample_rate, out_sample_rate);

    ratio_step = in_sample_rate / out_sample_rate;

#ifdef DMA_AUDIO_RESAMPLE_EX
    const float *coeffs;
    if (ratio_step == 2) {
        coeffs = iir_resample_table_96k_to_48k;
    } else if (ratio_step == 3) {
        coeffs = iir_resample_table_48k_to_16k;
    } else if (ratio_step == 6) {
        coeffs = iir_resample_table_96k_to_16k;
    } else if (ratio_step == 12) {
        coeffs = iir_resample_table_96k_to_8k;
    } else {
        ASSERT(false, "[%s]: Unsupported downsampling. sample_rate_in:%d, sample_rate_out:%d",
        __func__, in_sample_rate, out_sample_rate);
    }

    daud_resample->rs_coeffs = daud_heap_malloc(sizeof(float) * 9);
    daud_resample->rs_coeffs[0] = coeffs[0];
    daud_resample->rs_coeffs[1] = coeffs[1];
    daud_resample->rs_coeffs[2] = coeffs[2];
    daud_resample->rs_coeffs[3] = coeffs[3];
    daud_resample->rs_coeffs[4] = coeffs[4];
    daud_resample->rs_coeffs[5] = -coeffs[5];
    daud_resample->rs_coeffs[6] = -coeffs[6];
    daud_resample->rs_coeffs[7] = -coeffs[7];
    daud_resample->rs_coeffs[8] = -coeffs[8];

    for (int32_t ch = 0; ch < chan_num; ch++) {
        daud_resample->rs_state[ch] = daud_heap_malloc(sizeof(float) * 8);
    }
#endif

    // malloc resample data buffer
    rs_data_in_size = calc_data_in_buf_size(out_cfg, in_cfg);
    rs_data_in_buf = (uint8_t *)daud_heap_malloc(rs_data_in_size);
    ASSERT(rs_data_in_buf != NULL, "[%s]: malloc error, rs_data_in_buf is null", __func__);

    rs_data_out_size = rs_data_in_size / ratio_step;
    //rs_data_out_buf = (uint8_t *)daud_heap_malloc(rs_data_in_size);
    //ASSERT(rs_data_in_buf != NULL, "[%s]: malloc error, rs_data_out_buf is null", __func__);

    lock = int_lock();
    daud_resample->rs_buf           = rs_buf;
    daud_resample->rs_buf_size      = rs_size;
    daud_resample->rs_data_in_buf   = rs_data_in_buf;
    daud_resample->rs_data_in_size  = rs_data_in_size;
    daud_resample->rs_data_out_buf  = rs_data_out_buf;
    daud_resample->rs_data_out_size = rs_data_out_size;
    daud_resample->rs_bits          = bits;
    daud_resample->rs_chan_num      = chan_num;
    daud_resample->ratio_step       = ratio_step;
    daud_resample->status.open      = 1;
    int_unlock(lock);

    return 0;
}

int dma_audio_resample_enable(struct DAUD_RESAMPLE_T *daud_resample)
{
    if (!daud_resample->status.open) {
        ASSERT(false, "[%s]: not opened", __func__);
    }
    if (!daud_resample->status.enable) {
        daud_resample->status.enable = 1;
        daud_resample->rs_time_us = 0;
    }
    return 0;
}

static int dma_audio_extract_data(struct DAUD_RESAMPLE_T *daud_resample)
{
    uint32_t bits         = daud_resample->rs_bits;
    uint32_t chan_num     = daud_resample->rs_chan_num;
    uint8_t  *in_buf      = daud_resample->rs_data_in_buf;
    uint8_t  *in_buf_end  = daud_resample->rs_data_in_buf + daud_resample->rs_data_in_size;
    uint32_t in_buf_size  = daud_resample->rs_data_in_size;
    uint8_t  *out_buf     = daud_resample->rs_data_out_buf;
    uint8_t  *out_buf_end = daud_resample->rs_data_out_buf + daud_resample->rs_data_out_size;
    uint32_t out_buf_size = daud_resample->rs_data_out_size;
    int ratio_step        = daud_resample->ratio_step;

    ASSERT(in_buf_size == out_buf_size * ratio_step,
        "[%s]: Bad size: in_buf_size=%d, out_buf_size=%d, ratio_step=%d",
        __func__, in_buf_size, out_buf_size, ratio_step);

    if (bits == 16) {
        int16_t *src         = (int16_t *)in_buf;
        int16_t *src_end     = (int16_t *)in_buf_end;
        int16_t *dst         = (int16_t *)out_buf;
        int16_t *dst_end     = (int16_t *)out_buf_end;

        if (chan_num == 1) {
            while (dst < dst_end) {
                dst[0] = src[0];
                dst += 1;
                src += ratio_step;
            }
        } else if (chan_num == 2) {
            while (dst < dst_end) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst += 2;
                src += ratio_step * 2;
            }
        } else if (chan_num == 4) {
            while (dst < dst_end) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = src[3];
                dst += 4;
                src += ratio_step * 4;
            }
        }
        ASSERT(src == src_end, "[%s]: src buffer overflow: src=%x, src_end=%x",
            __func__, (int)src, (int)src_end);
    }
    else if (bits == 24 || bits == 32) {
        int32_t *src         = (int32_t *)in_buf;
        int32_t *src_end     = (int32_t *)in_buf_end;
        int32_t *dst         = (int32_t *)out_buf;
        int32_t *dst_end     = (int32_t *)out_buf_end;

        if (chan_num == 1) {
            while (dst < dst_end) {
                dst[0] = src[0];
                dst += 1;
                src += ratio_step;
            }
        } else if (chan_num == 2) {
            while (dst < dst_end) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst += 2;
                src += ratio_step * 2;
            }
        } else if (chan_num == 4) {
            while (dst < dst_end) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = src[3];
                dst += 4;
                src += ratio_step * 4;
            }
        }
        ASSERT(src == src_end, "[%s]: src buffer overflow: src=%x, src_end=%x",
            __func__, (int)src, (int)src_end);
    }
    else {
        ASSERT(false, "[%s]: Bad bits=%d", __func__, bits);
    }
    return 0;
}

int dma_audio_resample_run(struct DAUD_RESAMPLE_T *daud_resample)
{
    uint32_t time;

    time = hal_fast_sys_timer_get();

#if defined(DMA_AUDIO_RESAMPLE_EX)
    uint32_t bits         = daud_resample->rs_bits;
    uint32_t chan_num     = daud_resample->rs_chan_num;
    uint8_t  *in_buf      = daud_resample->rs_data_in_buf;
    uint32_t in_buf_size  = daud_resample->rs_data_in_size;
    float *rs_coeffs      = daud_resample->rs_coeffs;

    for (int ch = 0; ch < chan_num; ch++){
        float *rs_state = daud_resample->rs_state[ch];
        if (bits == 16){
            uint8_t *pcm_buf = in_buf + (ch * sizeof(int16_t));
            uint32_t pcm_len = in_buf_size / chan_num / sizeof(uint16_t);
            _downsampling_filter(rs_coeffs, rs_state, pcm_buf, pcm_buf, pcm_len, bits, chan_num);
        } else if (bits == 24 || bits == 32){
            uint8_t *pcm_buf = in_buf + (ch * sizeof(int32_t));
            uint32_t pcm_len = in_buf_size / chan_num / sizeof(uint32_t);
            _downsampling_filter(rs_coeffs, rs_state, pcm_buf, pcm_buf, pcm_len, bits, chan_num);
        }
    }
#endif

    dma_audio_extract_data(daud_resample);

    time = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time);
    daud_resample->rs_time_us = time;

    return 0;
}

int dma_audio_resample_disable(struct DAUD_RESAMPLE_T *daud_resample)
{
    if (!daud_resample->status.open) {
        ASSERT(false, "[%s]: not opened", __func__);
    }
    if (daud_resample->status.enable) {
        daud_resample->status.enable = 0;
    }
    return 0;
}

int dma_audio_resample_reset(struct DAUD_RESAMPLE_T *daud_resample)
{
    if (daud_resample->status.open) {
#ifdef DMA_AUDIO_RESAMPLE_EX

#endif
    }
    return 0;
}

int dma_audio_resample_close(struct DAUD_RESAMPLE_T *daud_resample)
{
    if (daud_resample->status.open) {
        if (daud_resample->status.enable) {
            dma_audio_resample_disable(daud_resample);
        }
#ifdef DMA_AUDIO_RESAMPLE_EX
        daud_heap_free(daud_resample->rs_coeffs);
        for (int32_t ch = 0; ch < daud_resample->rs_chan_num; ch++) {
            daud_heap_free(daud_resample->rs_state[ch]);
        }
#endif
        daud_heap_free(daud_resample->rs_buf);
        daud_heap_free(daud_resample->rs_data_in_buf);
//        daud_heap_free(daud_resample->rs_data_out_buf);
        daud_resample->rs_buf_size = 0;
        daud_resample->rs_data_in_size = 0;
        daud_resample->rs_data_out_size = 0;
        daud_resample->status.open = 0;
    }
    return 0;
}

#endif

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
#include "hal_trace.h"
#include "hal_timer.h"
#include "audio_down_mixer.h"
#include "audio_down_mixer_dsp.h"
#include "dma_audio.h"
// #define AUDIO_DOWN_MIXER_USE_TEST_DATA
// #define AUDIO_DOWN_MIXER_DEBUG
enum AUDIO_DOWN_MIXER_STATE_T {
    AUDIO_DOWN_MIXER_STATE_NULL = 0,
    AUDIO_DOWN_MIXER_STATE_INIT_DEINIT = (1<<0),
    AUDIO_DOWN_MIXER_STATE_START_STOP = (1<<1),
};

struct audio_down_mixe_hdlr_t {
    uint8_t state;
    AudioDownMixerState *adm_st;
    int32_t sample_rate;
    int32_t sample_bits;
    int32_t chan_num;
    int32_t frame_size;
    int32_t role;
    uint8_t *out_buf;
    uint32_t out_buf_size;
};

#define ADM_OUT_BUF_SIZE (ADM_SAMPLE_RATE / 1000 * 1 * ADM_OUT_CH_NUM * ADM_SAMPLE_SIZE)

static struct audio_down_mixe_hdlr_t adm_hdlr = {
    .state = AUDIO_DOWN_MIXER_STATE_NULL,
    .adm_st = NULL,
    .sample_rate = ADM_SAMPLE_RATE,
    .sample_bits = ADM_SAMPLE_BITS,
    .chan_num    = ADM_CH_NUM,
    .frame_size  = ADM_FRAME_SIZE,
    .role        = ADM_STEREO_CHANNEL_LEFT,
    .out_buf     = NULL,
    .out_buf_size= ADM_OUT_BUF_SIZE,
};


#ifdef AUDIO_DOWN_MIXER_USE_TEST_DATA
#define ADM_TEST_DATA_BUF_SIZE (ADM_SAMPLE_RATE / 1000 * 1 * ADM_CH_NUM * ADM_SAMPLE_SIZE)
static uint8_t adm_test_data_buf[ADM_TEST_DATA_BUF_SIZE];
static uint32_t adm_test_data_len;
static void adm_test_data_init(void)
{
    uint32_t i;
    int16_t *s = (int16_t *)adm_test_data_buf;
    uint32_t n = ADM_TEST_DATA_BUF_SIZE / 2;

    adm_test_data_len = ADM_TEST_DATA_BUF_SIZE;
    for (i = 0; i < n; i += 6) {
        s[0] = 0x1000;
        s[1] = 0x2000;
        s[2] = 0x3000;
        s[3] = 0x4000;
        s[4] = 0x5000;
        s[5] = 0x6000;
        s += 6;
    }
    TRACE(1, "[%s]: buf=%x, len=%d",
        __func__, (int)adm_test_data_buf, adm_test_data_len);
}
#endif

void donkey_memcpy(uint8_t *dst, uint8_t *src, uint32_t len)
{
#define BSIZE 4
    int32_t *s = (int32_t *)src;
    int32_t *d = (int32_t *)dst;
    uint32_t i, n, m;

    n = len / 4 / BSIZE;
    m = len / 4 % BSIZE;
    for (i = 0; i < n; i += BSIZE) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        s += BSIZE;
        d += BSIZE;
    }
    if (m > 0) {
        for (i = 0; i < m; i++) {
            *d++ = *s++;
        }
    }
}

static int32_t audio_down_mixer_dsp_start(void)
{
    struct audio_down_mixe_hdlr_t *h = &adm_hdlr;

    if (h->state != AUDIO_DOWN_MIXER_STATE_INIT_DEINIT) {
        TRACE(1, "[%s]: Bad state=%x", __func__, h->state);
        return -1;
    }
    h->state |= AUDIO_DOWN_MIXER_STATE_START_STOP;

    TRACE(1, "[%s]: done", __func__);

    return 0;
}

static int32_t audio_down_mixer_dsp_stop(void)
{
    struct audio_down_mixe_hdlr_t *h = &adm_hdlr;

    if (h->state != (AUDIO_DOWN_MIXER_STATE_INIT_DEINIT | AUDIO_DOWN_MIXER_STATE_START_STOP)) {
        TRACE(1, "[%s]: Bad state=%x", __func__, h->state);
        return -1;
    }
    h->state &= ~AUDIO_DOWN_MIXER_STATE_START_STOP;

    TRACE(1, "[%s]: done", __func__);

    return 0;
}

int32_t audio_down_mixer_dsp_init(void)
{
    TRACE(1,"[%s] start..",__func__);
    struct audio_down_mixe_hdlr_t *h = &adm_hdlr;

    h->out_buf = daud_heap_malloc(h->out_buf_size);
    TRACE(1, "[%s]: out_buf=%x, out_buf_size=%d",
        __func__, (int)h->out_buf, h->out_buf_size);
    ASSERT(h->out_buf != NULL, "[%s]: malloc out_buf failed", __func__);

#ifdef AUDIO_DOWN_MIXER_USE_TEST_DATA
    adm_test_data_init();
#endif
    h->adm_st = audio_down_mixer_create(h->sample_rate, h->frame_size, h->sample_bits, h->role);
    h->state = AUDIO_DOWN_MIXER_STATE_INIT_DEINIT;
    TRACE(1, "[%s]: init done", __func__);

    audio_down_mixer_dsp_start();

    return 0;
}

int32_t audio_down_mixer_dsp_deinit(void)
{
    TRACE(1, "[%s]: start..", __func__);
    struct audio_down_mixe_hdlr_t *h = &adm_hdlr;

    audio_down_mixer_dsp_stop();
    audio_down_mixer_destroy(h->adm_st);
    h->state = AUDIO_DOWN_MIXER_STATE_INIT_DEINIT;
    TRACE(1, "[%s]: deinit done", __func__);

    return 0;
}

int32_t audio_down_mixer_dsp_process(uint8_t *dst, uint32_t dst_len, uint8_t *src, uint32_t src_len)
{
    uint8_t *src_buf     = src;
    uint32_t src_buf_len = src_len;
    uint8_t *dst_buf     = dst;
    uint32_t dst_buf_len = dst_len;

    int16_t *in_buf, *out_buf;
    int32_t pcm_len, in_pcm_len, out_pcm_len, chan_num, out_buf_size;
    int32_t ret;
    struct audio_down_mixe_hdlr_t *h = &adm_hdlr;

    if (h->state != (AUDIO_DOWN_MIXER_STATE_INIT_DEINIT | AUDIO_DOWN_MIXER_STATE_START_STOP)) {
        return -1;
    }

#ifdef AUDIO_DOWN_MIXER_DEBUG
    uint32_t time;
    time = hal_fast_sys_timer_get();
#endif

#ifdef AUDIO_DOWN_MIXER_USE_TEST_DATA
    src_buf = adm_test_data_buf;
    src_buf_len = adm_test_data_len;
#endif

    in_buf  = (int16_t *)src_buf;
    out_buf = (int16_t *)(h->out_buf);
    out_buf_size = h->out_buf_size;
    ASSERT(out_buf != NULL, "[%s]: null out_buf", __func__);

    pcm_len = ADM_FRAME_SIZE * ADM_CH_NUM;
    chan_num = ADM_CH_NUM;
    in_pcm_len = src_buf_len / ADM_SAMPLE_SIZE;
    out_pcm_len = src_buf_len / ADM_SAMPLE_SIZE / (ADM_CH_NUM / ADM_OUT_CH_NUM);
    ASSERT(pcm_len == in_pcm_len, "[%s]: Bad src_buf_len=%d, in_pcm_len=%d, pcm_len=%d",
        __func__, src_buf_len, in_pcm_len, pcm_len);
    ASSERT((out_pcm_len * ADM_SAMPLE_SIZE) <= out_buf_size, "[%s]: Bad out_pcm_len=%d, out_buf_size=%d",
        __func__, out_pcm_len, out_buf_size);

    ret = audio_down_mixer_process(h->adm_st, in_buf, out_buf, pcm_len, chan_num);
    if (ret) {
        ASSERT(false, "[%s]: adm_process error, ret=%d", __func__, ret);
    }

    //ASSERT(dst_buf_len <= out_buf_size, "[%s][%d]: Bad dst_buf_len=%d, out_buf_size=%d",
    //   __func__, __LINE__, dst_buf_len, out_buf_size);

    donkey_memcpy((uint8_t *)dst_buf, (uint8_t *)out_buf, dst_buf_len);

#ifdef AUDIO_DOWN_MIXER_DEBUG
    time = hal_fast_sys_timer_get() - time;
    TRACE(1, "[DB_LIB]: cost=%d us, buf=%x, len=%d", FAST_TICKS_TO_US(time), (int)src_buf, src_buf_len);
#endif

    return 0;
}

static void audio_down_mixer_dsp_sw_init(bool init)
{
    if (init) {
        audio_down_mixer_dsp_init();
    } else {
        audio_down_mixer_dsp_deinit();
    }
}

int32_t audio_down_mixer_dsp_register_cb(bool en)
{
    if (en) {
        dma_audio_algo_setup_init_callback(audio_down_mixer_dsp_sw_init);
        dma_audio_algo_setup_algo_proc_callback(audio_down_mixer_dsp_process);
        dma_audio_algo_setup_config_callback(NULL);
    } else {
        dma_audio_algo_setup_init_callback(NULL);
        dma_audio_algo_setup_algo_proc_callback(NULL);
        dma_audio_algo_setup_config_callback(NULL);
    }

    return 0;
}


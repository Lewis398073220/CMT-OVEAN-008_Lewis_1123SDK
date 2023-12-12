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
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hal_cache.h"
#include "dma_audio.h"
#include "dma_audio_stream.h"
#include "dma_audio_stream_conf.h"
#include "aud_data_primitive.h"
#include "dma_audio_fade.h"
#ifdef DMA_AUDIO_ALGO
#include "dma_audio_algo.h"
#endif
#include "dma_audio_sync.h"

#define DMA_AUDIO_FADE

#ifdef DMA_AUDIO_DUMP
#include "audio_dump.h"
#define DMA_AUDIO_DUMP_CHANNEL_NUM 4
#endif

#ifdef DMA_AUDIO_DELAY_TEST
#define DELAY_TEST_MAX_PLAY_BUFF_SIZE (200*1024)
#define DELAY_TEST_MAX_CAP_BUFF_SIZE  (200*1024)
#endif

//#define CAPTURE_VERBOSE  1
//#define PLAYBACK_VERBOSE 1

//#define DMA_AUDIO_MIPS_STAT
//#define DMA_AUDIO_MIPS_STAT_IMM
//#define DMA_AUDIO_STREAM_DATA_COPY1

//#define DMA_AUDIO_BYPASS_SPK_HDLR
//#define DMA_AUDIO_BYPASS_MIC_HDLR

#ifdef DMA_AUDIO_SYNC_CLOCK
#if !defined(__AUDIO_RESAMPLE__) && !defined(DMA_AUDIO_USE_TDM_I2S)
#error "AUDIO_RESAMPLE should be enabled for DMA_AUDIO_SYNC_CLOCK !!"
#endif
#endif

#ifdef DMA_AUDIO_LOW_LATENCY
#if defined(CAPTURE_VERBOSE) || defined(PLAYBACK_VERBOSE)
#error "No any trace should be enabled for DMA_AUDIO_LOW_LATENCY !!"
#endif
#endif

#ifndef ALIGN4
#define ALIGN4 __attribute__((aligned(4)))
#endif

static POSSIBLY_UNUSED enum AUD_STREAM_USE_DEVICE_T g_cap_device = DMA_AUD_CAPT_DEV;
static POSSIBLY_UNUSED bool                   g_cap_data_24bit = false;
static POSSIBLY_UNUSED bool                   g_play_data_24bit = false;
static POSSIBLY_UNUSED enum AUD_SAMPRATE_T    g_cap_samp_rate = DMA_AUD_SAMP_RATE;
static POSSIBLY_UNUSED enum AUD_BITS_T        g_cap_samp_bits = DMA_AUD_SAMP_BITS;
static POSSIBLY_UNUSED enum AUD_CHANNEL_NUM_T g_cap_chan_num  = DMA_AUD_CHAN_NUM_CAP;
static POSSIBLY_UNUSED enum AUD_CHANNEL_MAP_T g_cap_chan_map  = DMA_AUD_CHAN_MAP_CAP;
static POSSIBLY_UNUSED uint32_t               g_cap_samp_size = DMA_AUD_SAMP_SIZE;
static POSSIBLY_UNUSED uint8_t                g_cap_chan_vol  = 17;
static POSSIBLY_UNUSED uint8_t               *g_cap_buf_ptr   = NULL;
static POSSIBLY_UNUSED uint32_t               g_cap_buf_size  = 0;
static POSSIBLY_UNUSED bool                   g_cap_irq_mode = true;

static POSSIBLY_UNUSED enum AUD_STREAM_USE_DEVICE_T g_play_device = DMA_AUD_PLAY_DEV;
static POSSIBLY_UNUSED enum AUD_SAMPRATE_T    g_play_samp_rate = DMA_AUD_SAMP_RATE;
static POSSIBLY_UNUSED enum AUD_BITS_T        g_play_samp_bits = DMA_AUD_SAMP_BITS;
static POSSIBLY_UNUSED enum AUD_CHANNEL_NUM_T g_play_chan_num  = DMA_AUD_CHAN_NUM_PLAY;
static POSSIBLY_UNUSED enum AUD_CHANNEL_MAP_T g_play_chan_map  = DMA_AUD_CHAN_MAP_PLAY;
static POSSIBLY_UNUSED uint32_t               g_play_samp_size = DMA_AUD_SAMP_SIZE;
static POSSIBLY_UNUSED uint8_t                g_play_chan_vol  = 17;
static POSSIBLY_UNUSED uint8_t               *g_play_buf_ptr   = NULL;
static POSSIBLY_UNUSED uint32_t               g_play_buf_size  = 0;
static POSSIBLY_UNUSED bool                   g_play_irq_mode = true;

enum {
    I2S_SW_TRIG_FLAG_NONE = 0,
    I2S_SW_TRIG_FLAG_OPEN_CLOSE = (1 << 0),
    I2S_SW_TRIG_FLAG_FINISH = (1 << 1),
};

struct i2s_sw_trig_ctl_t {
    enum AUD_STREAM_ID_T id;
    enum AUD_STREAM_T stream;
    enum AUD_STREAM_USE_DEVICE_T device;
    int flag;
};

static struct i2s_sw_trig_ctl_t i2s_sw_trig_ctl;

bool i2s_trig_dac_need_clear = false;
static bool dma_aud_stream_inited = false;
static bool dma_aud_app_req_clock = false;
static bool g_stream_on = false;

static DMA_AUD_MIC_DATA_HDLR_T mic_data_handler = NULL;
static DMA_AUD_SPK_DATA_HDLR_T spk_data_handler = NULL;

void cpu_whoami(void);

typedef void (*AUD_DATA_MEMCPY_CB_T)(uint8_t *dst, uint8_t *src, uint32_t len);

static AUD_DATA_MEMCPY_CB_T dma_aud_data_memcpy_cb = NULL;

#ifdef DMA_AUDIO_MIPS_STAT

#define MIPS_STAT_MAX_CNT         (100)
#define CAP_STREAM_MAX_USED_MIPS  (100)
#define PLAY_STREAM_MAX_USED_MIPS (100)
#define AUD_MIPS_STAT_QTY         (AUD_STREAM_NUM)

struct AUD_MIPS_STAT_T {
    int id;
    int samp_cnt;
    int per_samp_us;
    int total_us;
    int used_us;
    int idle_us;
    int used_p;
    int max_used_p;
    int cnt;
};

static struct AUD_MIPS_STAT_T g_mips[AUD_STREAM_NUM];

static void audio_stream_mips_stat_init(void)
{
    uint32_t samp_cnt;
    uint32_t per_samp_us;
    uint32_t total_us;
    struct AUD_MIPS_STAT_T *mips;

    DAUD_TRACE(0, "%s: start", __func__);

    samp_cnt = g_play_buf_size / g_play_chan_num / g_play_samp_size / 2;
    per_samp_us = 10000 / (g_play_samp_rate / 1000);
    per_samp_us = (per_samp_us + 9) / 10;
    total_us = per_samp_us * samp_cnt;

    mips = &g_mips[AUD_STREAM_PLAYBACK];
    mips->id = AUD_STREAM_PLAYBACK;
    mips->samp_cnt = (int)samp_cnt;
    mips->per_samp_us = (int)per_samp_us;
    mips->total_us = (int)total_us;
    mips->used_us = 0;
    mips->idle_us = 0;
    mips->used_p  = 0;
    mips->cnt = 0;
    mips->max_used_p = PLAY_STREAM_MAX_USED_MIPS;
    DAUD_TRACE(0, "mips stat: id=%d,samp_cnt=%d,samp_us=%d,total_us=%d",
        mips->id, mips->samp_cnt, mips->per_samp_us, mips->total_us);

    samp_cnt = g_cap_buf_size / g_cap_chan_num / g_cap_samp_size / 2;
    per_samp_us = 10000 / (g_cap_samp_rate / 1000);
    per_samp_us = (per_samp_us + 9) / 10;
    total_us = per_samp_us * samp_cnt;

    mips = &g_mips[AUD_STREAM_CAPTURE];
    mips->id = AUD_STREAM_CAPTURE;
    mips->samp_cnt = (int)samp_cnt;
    mips->per_samp_us = (int)per_samp_us;
    mips->total_us = (int)total_us;
    mips->used_us = 0;
    mips->idle_us = 0;
    mips->used_p  = 0;
    mips->cnt = 0;
    mips->max_used_p = CAP_STREAM_MAX_USED_MIPS;
    DAUD_TRACE(0, "mips stat: id=%d,samp_cnt=%d,samp_us=%d,total_us=%d",
        mips->id, mips->samp_cnt, mips->per_samp_us, mips->total_us);

    DAUD_TRACE(0, "init done");
}

static int audio_stream_mips_stat_calc(struct AUD_MIPS_STAT_T *mips)
{
    int error = 0;

    mips->idle_us = mips->total_us - mips->used_us;
    if (mips->idle_us < 0) {
        DAUD_TRACE(1, "MIPS WARN: idle<0, stream[%d]: used=%d,idle=%d,total=%d",
            mips->id,mips->used_us, mips->idle_us, mips->total_us);
        error |= 0x1;
    }
    mips->used_p = mips->used_us * 1000 / mips->total_us;
    mips->used_p = (mips->used_p + 9 ) / 10;
    if (mips->used_p >= mips->max_used_p) {
        DAUD_TRACE(1, "MIPS WARN: not enough, stream[%d]: used=%d,idle=%d,total=%d,used_p=%d,max_p=%d",
            mips->id,mips->used_us, mips->idle_us, mips->total_us, mips->used_p, mips->max_used_p);
        error |= 0x2;
    }
    mips->cnt++;
    if (mips->cnt >= MIPS_STAT_MAX_CNT) {
        mips->cnt = 0;
        DAUD_TRACE(1, "MIPS STAT: stream[%d]: used=%d,idle=%d,total=%d",
            mips->id,mips->used_us, mips->idle_us, mips->total_us);
    }
    return error;
}

static void audio_stream_mips_stat_set(enum AUD_STREAM_T stream, int used_us)
{
    struct AUD_MIPS_STAT_T *mips = &g_mips[stream];
    int error = 0;

    mips->used_us = used_us;
    error = audio_stream_mips_stat_calc(mips);
    if (error) {
        DAUD_TRACE(0, "====> CPU MIPS calc error: 0x%x", error);
    }
}

#endif /* DMA_AUDIO_MIPS_STAT */

static void dma_audio_data_format_info_print(void)
{
    DAUD_TRACE(0, "PLAYBACK STREAM:");
    DAUD_TRACE(0, "device   :    %d", g_play_device);
    DAUD_TRACE(0, "samp_rate:    %d", g_play_samp_rate);
    DAUD_TRACE(0, "samp_bits:    %d", g_play_samp_bits);
    DAUD_TRACE(0, "samp_size:    %d", g_play_samp_size);
    DAUD_TRACE(0, "chan_num :    %d", g_play_chan_num);
    DAUD_TRACE(0, "chan_map :    %d", g_play_chan_map);
    DAUD_TRACE(0, "buf_ptr  :    %x", (uint32_t)g_play_buf_ptr);
    DAUD_TRACE(0, "buf_size :    %d", g_play_buf_size);
    DAUD_TRACE(0, "max_buf_size: %d", PLAY_BUFF_MAX_SIZE);
    if (daud_stream_use_i2s_tdm_device(g_play_device)) {
        DAUD_TRACE(0, "align    :    %d", DMA_AUDIO_TDM_ALIGN);
        DAUD_TRACE(0, "fs_edge  :    %d", DMA_AUDIO_TDM_FS_EDGE);
        DAUD_TRACE(0, "fs_cycles:    %d", DMA_AUDIO_TDM_FS_CYCLES);
        DAUD_TRACE(0, "g_play_data_24bit  :    %d", g_play_data_24bit);
    }
    DAUD_TRACE(0, "-");

    DAUD_TRACE(0, "CAPTURE STREAM:");
    DAUD_TRACE(0, "device   :    %d", g_cap_device);
    DAUD_TRACE(0, "samp_rate:    %d", g_cap_samp_rate);
    DAUD_TRACE(0, "samp_bits:    %d", g_cap_samp_bits);
    DAUD_TRACE(0, "samp_size:    %d", g_cap_samp_size);
    DAUD_TRACE(0, "chan_num :    %d", g_cap_chan_num);
    DAUD_TRACE(0, "chan_map :    %d", g_cap_chan_map);
    DAUD_TRACE(0, "buf_ptr  :    %x", (uint32_t)g_cap_buf_ptr);
    DAUD_TRACE(0, "buf_size :    %d", g_cap_buf_size);
    DAUD_TRACE(0, "max_buf_size: %d", CAP_BUFF_MAX_SIZE);
    if (daud_stream_use_i2s_tdm_device(g_cap_device)) {
        DAUD_TRACE(0, "align    :    %d", DMA_AUDIO_TDM_ALIGN);
        DAUD_TRACE(0, "fs_edge  :    %d", DMA_AUDIO_TDM_FS_EDGE);
        DAUD_TRACE(0, "fs_cycles:    %d", DMA_AUDIO_TDM_FS_CYCLES);
        DAUD_TRACE(0, "g_cap_data_24bit  :    %d", g_cap_data_24bit);
    }

    DAUD_TRACE(0, "-");
}

static void aud_data_memcpy_samp_inc_check(void)
{
#ifdef AUD_DATA_MEMCPY_SAMP_INC
    uint32_t samp_inc = AUD_DATA_MEMCPY_SAMP_INC;
    uint32_t samp_cnt = g_cap_buf_size / g_cap_chan_num / g_cap_samp_size;

    DAUD_TRACE(0, "%s: samp_cnt=%d, samp_inc=%d", __func__, samp_cnt, samp_inc);
    ASSERT(samp_cnt >= samp_inc, "%s: samp_inc %d too large", __func__, samp_inc);
    if (samp_inc > 1) {
        ASSERT((samp_cnt%samp_inc) == 0, "%s: samp_inc %d cannot be divided by samp_cnt", __func__, samp_inc);
    }
#endif
}

static void dma_audio_stream_setup_memcpy_callback(void)
{
    enum AUD_BITS_T out_bits = g_play_samp_bits;
    enum AUD_CHANNEL_NUM_T out_chan_num = g_play_chan_num;
    enum AUD_BITS_T in_bits = g_cap_samp_bits;
    enum AUD_CHANNEL_NUM_T in_chan_num = g_cap_chan_num;

    aud_data_memcpy_samp_inc_check();

    DAUD_TRACE(0, "%s: start", __func__);
    if (out_bits == AUD_BITS_32) {
        if (out_chan_num == AUD_CHANNEL_NUM_2) {
            if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_stereo_from_i32_stereo;
                DAUD_TRACE(1, "memcpy_i32_stereo_from_i32_stereo");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_stereo_from_i32_mono;
                DAUD_TRACE(1, "memcpy_i32_stereo_from_i32_mono");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_3)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_stereo_from_i32_3ch;
                DAUD_TRACE(1, "memcpy_i32_stereo_from_i32_3ch");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_stereo_from_i32_4ch;
                DAUD_TRACE(1, "memcpy_i32_stereo_from_i32_4ch");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_h24_stereo_from_i32_l24_mono;
                DAUD_TRACE(1, "memcpy_i32_stereo_from_i24_mono");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                if (daud_stream_use_i2s_tdm_device(g_play_device) && g_play_data_24bit) {
                       dma_aud_data_memcpy_cb = aud_data_memcpy_i32_h24_stereo_from_i32_l24_stereo;
                    DAUD_TRACE(1, "memcpy_i32_h24_stereo_from_i32_l24_stereo");
                } else {
                    ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
                }
            } else {
                ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
            }
        } else if (out_chan_num == AUD_CHANNEL_NUM_1) {
            if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_mono_from_i32_stereo;
                DAUD_TRACE(1, "memcpy_i32_mono_from_i32_stereo");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_mono_from_i32_mono;
                DAUD_TRACE(1, "memcpy_i32_mono_from_i32_mono");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_3)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_mono_from_i32_3ch;
                DAUD_TRACE(1, "memcpy_i32_mono_from_i32_3ch");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_mono_from_i32_4ch;
                DAUD_TRACE(1, "memcpy_i32_mono_from_i32_4ch");
            } else {
                ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
            }
        } else if (out_chan_num == AUD_CHANNEL_NUM_8) {
            if (0) {
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_8ch_from_i32_4ch;
                DAUD_TRACE(1, "memcpy_i32_8ch_from_i32_4ch");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_8)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_8ch_from_i32_8ch;
                DAUD_TRACE(1, "memcpy_i32_8ch_from_i32_8ch");
            }
        } else {
            ASSERT(false, "[%d]: bad out_chan_num=%d", __LINE__, out_chan_num);
        }
    } else if (out_bits == AUD_BITS_24) {
        if (out_chan_num == AUD_CHANNEL_NUM_2) {
            if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_stereo_from_i32_l24_stereo;
                DAUD_TRACE(1, "memcpy_i32_l24_stereo_from_i32_l24_stereo");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_stereo_from_i32_l24_mono;
                DAUD_TRACE(1, "memcpy_i32_l24_stereo_from_i32_l24_mono");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_3)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_stereo_from_i32_l24_3ch;
                DAUD_TRACE(1, "memcpy_i32_l24_stereo_from_i32_l24_3ch");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_stereo_from_i32_l24_4ch;
                DAUD_TRACE(1, "memcpy_i32_l24_stereo_from_i32_l24_4ch");
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_8)) {
                if (daud_stream_use_i2s_tdm_device(g_cap_device) && g_cap_data_24bit) {
                    dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_stereo_from_i32_h24_8ch;
                    DAUD_TRACE(1, "memcpy_i32_l24_stereo_from_i32_h24_8ch");
                }
            } else if ((in_bits == AUD_BITS_32) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                if (daud_stream_use_i2s_tdm_device(g_cap_device) && g_cap_data_24bit) {
                    //i2s high 24bit data to codec low 24bit data
                    dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_stereo_from_i32_h24_2ch;
                    DAUD_TRACE(1, "memcpy_i32_l24_stereo_from_i32_h24_2ch");
                } else {
                    ASSERT(false, "[%d]: Bad in_chan_num=%d and in_bits=%d %d", __LINE__, in_chan_num, in_bits, g_cap_data_24bit);
                }
            } else {
                ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
            }
        } else if (out_chan_num == AUD_CHANNEL_NUM_1) {
            if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_mono_from_i32_l24_stereo;
                DAUD_TRACE(1, "memcpy_i32_l24_mono_from_i32_l24_stereo");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_mono_from_i32_l24_mono;
                DAUD_TRACE(1, "memcpy_i32_l24_mono_from_i32_l24_mono");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_3)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_mono_from_i32_l24_3ch;
                DAUD_TRACE(1, "memcpy_i32_l24_mono_from_i32_l24_3ch");
            } else if ((in_bits == AUD_BITS_24) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i32_l24_mono_from_i32_l24_4ch;
                DAUD_TRACE(1, "memcpy_i32_l24_mono_from_i32_l24_4ch");
            } else {
                ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
            }
        } else {
            ASSERT(false, "[%d]: bad out_chan_num=%d", __LINE__, out_chan_num);
        }
    } else if (out_bits == AUD_BITS_16) {
        if (out_chan_num == AUD_CHANNEL_NUM_2) {
            if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_stereo;
                DAUD_TRACE(1, "memcpy_i16_stereo_from_i16_stereo");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_mono;
                DAUD_TRACE(1, "memcpy_i16_stereo_from_i16_mono");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_3)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_3ch;
                DAUD_TRACE(1, "memcpy_i16_stereo_from_i16_3ch");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_4ch;
                DAUD_TRACE(1, "memcpy_i16_stereo_from_i16_4ch");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_8)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_8ch;
                DAUD_TRACE(1, "memcpy_i16_stereo_from_i16_8ch");
            } else {
                ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
            }
        } else if (out_chan_num == AUD_CHANNEL_NUM_1) {
            if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_mono_from_i16_stereo;
                DAUD_TRACE(1, "memcpy_i16_mono_from_i16_stereo");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_1)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_mono_from_i16_mono;
                DAUD_TRACE(1, "memcpy_i16_mono_from_i16_mono");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_3)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_mono_from_i16_3ch;
                DAUD_TRACE(1, "memcpy_i16_mono_from_i16_3ch");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_mono_from_i16_4ch;
                DAUD_TRACE(1, "memcpy_i16_mono_from_i16_4ch");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_8)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_mono_from_i16_8ch;
                DAUD_TRACE(1, "memcpy_i16_mono_from_i16_8ch");
            } else {
                ASSERT(false, "[%d]: bad in_chan_num=%d and in_bits=%d", __LINE__, in_chan_num, in_bits);
            }
        } else if (out_chan_num == AUD_CHANNEL_NUM_8) {
            if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_2)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_8ch_from_i16_2ch;
                DAUD_TRACE(1, "memcpy_i16_8ch_from_i16_2ch");
            } else if ((in_bits == AUD_BITS_16) && (in_chan_num == AUD_CHANNEL_NUM_4)) {
                dma_aud_data_memcpy_cb = aud_data_memcpy_i16_8ch_from_i16_4ch;
                DAUD_TRACE(1, "memcpy_i16_8ch_from_i16_4ch");
            }
        } else {
            ASSERT(false, "[%d]: bad out_chan_num=%d", __LINE__, out_chan_num);
        }
    }
    ASSERT(dma_aud_data_memcpy_cb != NULL, "%s: null callback", __func__);
    DAUD_TRACE(0, "done");
}

static uint32_t daud_play_data_handler(uint8_t *buf, uint32_t len);
static uint32_t daud_cap_data_handler(uint8_t *buf, uint32_t len);

void dma_audio_stream_get_config(enum AUD_STREAM_T stream, struct DAUD_STREAM_CONFIG_T *cfg)
{
    if (stream == AUD_STREAM_PLAYBACK) {
        if (daud_stream_use_i2s_tdm_device(g_play_device)) {
            cfg->sync_start = false;
            cfg->slot_cycles = g_play_samp_bits;
            cfg->align       = DMA_AUDIO_TDM_ALIGN;
            cfg->fs_edge     = DMA_AUDIO_TDM_FS_EDGE;
            cfg->fs_cycles   = DMA_AUDIO_TDM_FS_CYCLES;
            TRACE(1, "[%s]: I2S/TDM SEND: bits = %d", __func__, g_play_samp_bits);
        }
        cfg->irq_mode    = g_play_irq_mode;
        cfg->sample_rate = g_play_samp_rate;
        cfg->bits        = g_play_samp_bits;
        cfg->channel_num = g_play_chan_num;
        cfg->channel_map = g_play_chan_map;
        cfg->device      = g_play_device;
        cfg->vol         = g_play_chan_vol;
        cfg->handler     = daud_play_data_handler;
        cfg->io_path     = AUD_OUTPUT_PATH_SPEAKER;
        cfg->data_ptr    = g_play_buf_ptr;
        cfg->data_size   = g_play_buf_size;
    } else {
        // I2S RECV: 32bit, 24bit, 16bit
        if (daud_stream_use_i2s_tdm_device(g_cap_device)) {
            cfg->sync_start = false;
            cfg->slot_cycles = g_cap_samp_bits;
            cfg->align       = DMA_AUDIO_TDM_ALIGN;
            cfg->fs_edge     = DMA_AUDIO_TDM_FS_EDGE;
            cfg->fs_cycles   = DMA_AUDIO_TDM_FS_CYCLES;
            TRACE(1, "[%s]: I2S/TDM RECV: bits = %d", __func__, g_cap_samp_bits);
        }
        cfg->irq_mode    = g_cap_irq_mode;;
        cfg->sample_rate = g_cap_samp_rate;
        cfg->bits        = g_cap_samp_bits;
        cfg->channel_num = g_cap_chan_num;
        cfg->channel_map = g_cap_chan_map;
        cfg->device      = g_cap_device;
        cfg->vol         = g_cap_chan_vol;
        cfg->handler     = daud_cap_data_handler;
        cfg->io_path     = AUD_INPUT_PATH_MAINMIC;
        cfg->data_ptr    = g_cap_buf_ptr;
        cfg->data_size   = g_cap_buf_size;
    }
}

void dma_audio_stream_set_config(enum AUD_STREAM_T stream, struct DAUD_STREAM_CONFIG_T *cfg)
{
    if (stream == AUD_STREAM_PLAYBACK) {
        g_play_device = cfg->device;
        if (daud_stream_use_tdm_mst_device(g_play_device)
            || daud_stream_use_i2s_mst_device(g_play_device)) {
            if (cfg->bits == AUD_BITS_24) {
                g_play_data_24bit = true;
                g_play_samp_bits = AUD_BITS_32;
                g_play_samp_size = 4;
            } else {
                g_play_data_24bit = false;
                g_play_samp_bits  = cfg->bits;
                g_play_samp_size  = (cfg->bits == AUD_BITS_16) ? 2 : 4;
            }
        } else {
            g_play_samp_bits = cfg->bits;
            g_play_samp_size = (cfg->bits == AUD_BITS_16) ? 2 : 4;
        }
        g_play_irq_mode  = cfg->irq_mode;
        g_play_samp_rate = cfg->sample_rate;
        g_play_chan_num  = cfg->channel_num;
        g_play_chan_map  = cfg->channel_map;
        g_play_chan_vol  = cfg->vol;
        if ((cfg->data_size != g_play_buf_size) || (cfg->data_ptr == NULL)) {
            if (g_play_buf_ptr) {
                daud_heap_free(g_play_buf_ptr);
            }
            ASSERT(cfg->data_size != 0, "[%s]: zero cfg->data_size", __func__);
#ifdef DMA_AUDIO_DELAY_TEST
            cfg->data_ptr = daud_heap_malloc(DELAY_TEST_MAX_PLAY_BUFF_SIZE);
#else
            cfg->data_ptr = daud_heap_malloc(cfg->data_size);
#endif
            ASSERT(cfg->data_ptr != NULL, "[%s] malloc playback buffer failed", __func__);
            TRACE(0 ,"[%s]: playback buff: data_ptr=%x, data_size=%d",
                __func__, (int)(cfg->data_ptr), cfg->data_size);
        }
        g_play_buf_ptr   = cfg->data_ptr;
        g_play_buf_size  = cfg->data_size;
    } else if (stream == AUD_STREAM_CAPTURE) {
        g_cap_device = cfg->device;
        if (daud_stream_use_tdm_slv_device(g_cap_device)
            || daud_stream_use_i2s_slv_device(g_cap_device)) {
            if (cfg->bits == AUD_BITS_24) {
                g_cap_data_24bit = true;
                g_cap_samp_bits  = AUD_BITS_32;
                g_cap_samp_size  = 4;
            } else {
                g_cap_data_24bit = false;
                g_cap_samp_bits  = cfg->bits;
                g_cap_samp_size  = (cfg->bits == AUD_BITS_16) ? 2 : 4;
            }
        } else {
            g_cap_samp_bits = cfg->bits;
            g_cap_samp_size = (cfg->bits == AUD_BITS_16) ? 2 : 4;
        }
        g_cap_irq_mode  = cfg->irq_mode;
        g_cap_samp_rate = cfg->sample_rate;
        g_cap_chan_num  = cfg->channel_num;
        g_cap_chan_map  = cfg->channel_map;
        g_cap_chan_vol  = cfg->vol;
        if ((cfg->data_size != g_cap_buf_size) || (cfg->data_ptr == NULL)) {
            if (g_cap_buf_ptr) {
                daud_heap_free(g_cap_buf_ptr);
            }
            ASSERT(cfg->data_size != 0, "[%s]: zero cfg->data_size", __func__);
#ifdef DMA_AUDIO_DELAY_TEST
            cfg->data_ptr = daud_heap_malloc(DELAY_TEST_MAX_CAP_BUFF_SIZE);
#else
            cfg->data_ptr = daud_heap_malloc(cfg->data_size);
#endif
            ASSERT(cfg->data_ptr!= NULL, "[%s] malloc capture buffer failed",__func__);
            TRACE(0 ,"[%s]: capture buff: data_ptr=%x, data_size=%d",
                __func__, (int)(cfg->data_ptr), cfg->data_size);
        }
        g_cap_buf_ptr   = cfg->data_ptr;
        g_cap_buf_size  = cfg->data_size;
    }
    dma_audio_stream_setup_memcpy_callback();
#ifdef DMA_AUDIO_MIPS_STAT
    audio_stream_mips_stat_init();
#endif
    //TODO: if DMA_AUDIO_APP_DYN_ON = 0, check if dma audio app need to restart ?
}

static bool dma_audio_clear_i2s_trig_dac(void)
{
    bool success = false;
    uint32_t addr = 0x40300490;
    uint32_t val;

    val = *(volatile uint32_t *)addr;
    val &= ~(1 << 6);
    *(volatile uint32_t *)addr = val;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    val = *(volatile uint32_t *)addr;
    if ((val & (1 << 6)) == 0) {
        success = true;
    }
    TRACE(1, "[%s]: success=%d", __func__, success);
    return success;
}

void dma_audio_set_i2s_trig_dac_clear_flag(bool need_clear)
{
    TRACE(1, "[%s]: need_clear=%d", __func__, need_clear);
    i2s_trig_dac_need_clear = need_clear;
}

void dma_audio_i2s_sw_trig_init(void)
{
    struct i2s_sw_trig_ctl_t *ctl = &i2s_sw_trig_ctl;

    ctl->id = AUD_STREAM_ID_NUM;
    ctl->stream = AUD_STREAM_NUM;
    ctl->device = AUD_STREAM_USE_DEVICE_NULL;
    ctl->flag = I2S_SW_TRIG_FLAG_NONE;
    TRACE(1, "[%s]: done", __func__);
}

void dma_audio_i2s_sw_trig_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    enum AUD_STREAM_USE_DEVICE_T device)
{
    struct i2s_sw_trig_ctl_t *ctl = &i2s_sw_trig_ctl;

    ctl->id = id;
    ctl->stream = stream;
    ctl->device = device;
    ctl->flag |= I2S_SW_TRIG_FLAG_OPEN_CLOSE;
    TRACE(1, "[%s]: id=%d, stream=%d, device=%d", __func__, id, stream, device);
}

void dma_audio_i2s_sw_trig_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    enum AUD_STREAM_USE_DEVICE_T device)
{
    struct i2s_sw_trig_ctl_t *ctl = &i2s_sw_trig_ctl;

    ctl->id = AUD_STREAM_ID_NUM;
    ctl->stream = AUD_STREAM_NUM;
    ctl->device = AUD_STREAM_USE_DEVICE_NULL;
    ctl->flag &= ~(I2S_SW_TRIG_FLAG_OPEN_CLOSE | I2S_SW_TRIG_FLAG_FINISH);
    TRACE(1, "[%s]: id=%d, stream=%d, device=%d", __func__, id, stream, device);
}

int dma_audio_i2s_sw_trig_get_flag(int flag_mask)
{
    return i2s_sw_trig_ctl.flag & flag_mask;
}

void dma_audio_i2s_sw_trig_set_flag(int flag_mask)
{
    i2s_sw_trig_ctl.flag |= flag_mask;
}

void dma_audio_i2s_sw_trig_enable(void)
{
#define I2S_CLK_GEN_REG_OFFS 0x0C
    volatile uint32_t regaddr;
    volatile uint32_t regval = 0;
    struct i2s_sw_trig_ctl_t *ctl = &i2s_sw_trig_ctl;
    enum AUD_STREAM_USE_DEVICE_T device = ctl->device;
    POSSIBLY_UNUSED enum HAL_I2S_ID_T i2s_id;

    if (ctl->id == DAUD_STREAM_ID && ctl->stream == AUD_STREAM_PLAYBACK) {
        if ((device == AUD_STREAM_USE_TDM0_MASTER)
            || (device == AUD_STREAM_USE_I2S0_MASTER)
            || (device == AUD_STREAM_USE_TDM0_SLAVE)
            || (device == AUD_STREAM_USE_I2S0_SLAVE)) {
            i2s_id = HAL_I2S_ID_0;
            regaddr = I2S0_BASE + I2S_CLK_GEN_REG_OFFS;
        } else if ((device == AUD_STREAM_USE_TDM1_MASTER)
            || (device == AUD_STREAM_USE_I2S1_MASTER)
            || (device == AUD_STREAM_USE_TDM1_SLAVE)
            || (device == AUD_STREAM_USE_I2S1_SLAVE)) {
            i2s_id = HAL_I2S_ID_1;
            regaddr = I2S1_BASE + I2S_CLK_GEN_REG_OFFS;
        } else {
            ASSERT(false, "[%s]: Bad device=%d", __func__, device);
        }
        regval = *(volatile uint32_t *)regaddr;

        (*(volatile uint32_t *)regaddr) = 0x1;
        regval = *(volatile uint32_t *)regaddr;
        if (!regval) {
            (*(volatile uint32_t *)regaddr) = 1;
        }
        TRACE(1, "[%s]: I2S id=%d, reg[%x] = %x", __func__, i2s_id, regaddr, regval);
    }
}

void dma_audio_stream_irq_notify(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    if ((id == DAUD_STREAM_ID) && (stream == AUD_STREAM_CAPTURE)) {
        if (dma_audio_i2s_sw_trig_get_flag(I2S_SW_TRIG_FLAG_OPEN_CLOSE)) {
            if (!dma_audio_i2s_sw_trig_get_flag(I2S_SW_TRIG_FLAG_FINISH)) {
                dma_audio_i2s_sw_trig_enable();
                dma_audio_i2s_sw_trig_set_flag(I2S_SW_TRIG_FLAG_FINISH);
            }
        }
    }
}

POSSIBLY_UNUSED uint32_t mips_time[2];

#define DAUD_MIPS_CALC_START() {mips_time[0] = hal_fast_sys_timer_get();}
#define DAUD_MIPS_CALC_END()   {mips_time[1] = hal_fast_sys_timer_get();}
#define DAUD_MIPS_CALC_US()    (FAST_TICKS_TO_US(mips_time[1] - mips_time[0]))

static uint32_t daud_play_data_handler(uint8_t *buf, uint32_t len)
{
    POSSIBLY_UNUSED uint32_t cur_time = hal_fast_sys_timer_get();
    POSSIBLY_UNUSED uint32_t mips_a = 0;

    DAUD_MIPS_CALC_START();

    if (i2s_trig_dac_need_clear) {
        bool success = false;
        success = dma_audio_clear_i2s_trig_dac();
        if (success) {
            i2s_trig_dac_need_clear = false;
        }
    }

#ifdef DMA_AUDIO_SYNC_CLOCK
    dma_audio_stream_sync_callback(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
#endif

#ifndef DMA_AUDIO_BYPASS_SPK_HDLR
    if (spk_data_handler) {
        spk_data_handler(buf, len);
    }
#endif

    DAUD_MIPS_CALC_END();
    mips_a = DAUD_MIPS_CALC_US();
#ifdef DMA_AUDIO_MIPS_STAT_IMM
    DAUD_TRACE(0, "play data proc mips us: %d", mips_a);
#endif
    if (mips_a  > PLAY_MIPS_MAX_US) {
        DAUD_TRACE(0, "[%s]: WARNING: no enough MIPS: a(%d) > t(%d)", __func__, mips_a, PLAY_MIPS_MAX_US);
    }

#if defined(PLAYBACK_VERBOSE) && (PLAYBACK_VERBOSE > 0)
    DAUD_TRACE(5,"[%8u] PLAY: buf=%x, len=%4u",
        FAST_TICKS_TO_US(cur_time), (int)buf, len);
#endif

#ifdef DMA_AUDIO_MIPS_STAT
    cur_time = hal_fast_sys_timer_get() - cur_time;
    audio_stream_mips_stat_set(AUD_STREAM_PLAYBACK, FAST_TICKS_TO_US(cur_time));
#endif


#if defined(DMA_AUDIO_FADE) && (!defined(DMA_AUDIO_ALGO) && defined(DMA_AUDIO_NO_MEMCPY))
    dma_audio_fade_process(g_play_chan_num, buf, len);
#endif

#ifdef DMA_AUDIO_DUMP
    audio_dump_add_channel_data_from_multi_channels(2, buf, PLAY_MIPS_SAMP, 2, 0); //dump playback ch0
    audio_dump_add_channel_data_from_multi_channels(3, buf, PLAY_MIPS_SAMP, 2, 1); //dump playback ch1
    audio_dump_run();
#endif

    return 0;
}

static uint32_t daud_cap_data_handler(uint8_t *buf, uint32_t len)
{
    POSSIBLY_UNUSED uint32_t cur_time = hal_fast_sys_timer_get();
    POSSIBLY_UNUSED uint32_t mips[4] = {0};
    uint8_t *dst;

    int sw_trig = dma_audio_i2s_sw_trig_get_flag(I2S_SW_TRIG_FLAG_OPEN_CLOSE);

    if (!sw_trig) {
        if ((uint32_t)buf == (uint32_t)g_cap_buf_ptr) {
            dst = g_play_buf_ptr;
        } else {
            dst = g_play_buf_ptr + g_play_buf_size/2;
        }
    } else {
        if ((uint32_t)buf == (uint32_t)g_cap_buf_ptr) {
            dst = g_play_buf_ptr + g_play_buf_size/2;
        } else {
            dst = g_play_buf_ptr;
        }
    }
    dma_audio_stream_irq_notify(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);

#ifdef DMA_AUDIO_SYNC_CLOCK
    DAUD_MIPS_CALC_START();
    dma_audio_stream_sync_callback(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
    DAUD_MIPS_CALC_END();
    mips[0] = DAUD_MIPS_CALC_US();
#endif

#ifdef DMA_AUDIO_DUMP
    audio_dump_clear_up();
    audio_dump_add_channel_data_from_multi_channels(0, buf, CAP_MIPS_SAMP, 2, 0); //dump capture ch0
    audio_dump_add_channel_data_from_multi_channels(1, buf, CAP_MIPS_SAMP, 2, 1); //dump capture ch1
#endif

#ifndef DMA_AUDIO_BYPASS_MIC_HDLR
    DAUD_MIPS_CALC_START();
    if (mic_data_handler) {
        mic_data_handler(buf, len);
    }
    DAUD_MIPS_CALC_END();
    mips[1] = DAUD_MIPS_CALC_US();
#endif

#ifdef DMA_AUDIO_ALGO
    DAUD_MIPS_CALC_START();
    dma_audio_algo_process(dst, g_play_buf_size/2, buf, len);
    DAUD_MIPS_CALC_END();
    mips[3] = DAUD_MIPS_CALC_US();
#endif

#ifndef DMA_AUDIO_NO_MEMCPY
    DAUD_MIPS_CALC_START();
    if (dma_aud_data_memcpy_cb) {
        dma_aud_data_memcpy_cb(dst, buf, len);
    }
    DAUD_MIPS_CALC_END();
    mips[2] = DAUD_MIPS_CALC_US();
#endif

#if defined(DMA_AUDIO_FADE) && (defined(DMA_AUDIO_ALGO) || !defined(DMA_AUDIO_NO_MEMCPY))
    dma_audio_fade_process(g_play_chan_num, dst, g_play_buf_size/2);
#endif

#ifdef DMA_AUDIO_MIPS_STAT_IMM
    DAUD_TRACE(0, "cap proc mips=[%d %d %d %d]", mips[0], mips[1], mips[2], mips[3]);
#endif
    uint32_t e, t;
    e = FAST_TICKS_TO_US(hal_fast_sys_timer_get()-cur_time);
    t = CAP_MIPS_MAX_US;
    if (e > t) {
        DAUD_TRACE(0, "[%s]: WARNING: no enough MIPS: e(%d) > t(%d)",
            __func__, e, t);
    }

#if defined(CAPTURE_VERBOSE) && (CAPTURE_VERBOSE > 0)
    DAUD_TRACE(4,"[%8u] CAP: buf=%x, len=%4u",
        FAST_TICKS_TO_US(cur_time), (int)buf, len);
#endif

#ifdef DMA_AUDIO_MIPS_STAT
    cur_time = hal_fast_sys_timer_get() - cur_time;
    audio_stream_mips_stat_set(AUD_STREAM_CAPTURE, FAST_TICKS_TO_US(cur_time));
#endif

    return 0;
}

void dma_audio_stream_request_clock(void)
{
    if (!dma_aud_app_req_clock) {
        hal_sysfreq_req(DAUD_FREQ_USER, DAUD_FREQ_CLOCK);
        dma_aud_app_req_clock = true;
    }
}

void dma_audio_stream_release_clock(void)
{
    if (dma_aud_app_req_clock) {
        hal_sysfreq_req(DAUD_FREQ_USER, HAL_CMU_FREQ_32K);
        dma_aud_app_req_clock = false;
    }
}

void dma_audio_stream_init(void)
{
    uint32_t samp_num;

    TRACE(0, "[%s]", __func__);

    if (!dma_aud_stream_inited) {
        samp_num = PLAY_BUFF_SAMP_NUM;
        g_play_buf_size = samp_num * g_play_chan_num * g_play_samp_size;
        g_play_buf_ptr  = daud_heap_malloc(g_play_buf_size);
        TRACE(0, "playback: buf_ptr=%x, buf_size=%d", (int)g_play_buf_ptr, g_play_buf_size);

        samp_num = CAP_BUFF_SAMP_NUM;
        g_cap_buf_size = samp_num * g_cap_chan_num * g_cap_samp_size;
        g_cap_buf_ptr  = daud_heap_malloc(g_cap_buf_size);
        TRACE(0, "capture: buf_ptr=%x, buf_size=%d", (int)g_cap_buf_ptr, g_cap_buf_size);

        if (daud_stream_use_i2s_tdm_device(g_cap_device)) {
            if (g_cap_samp_bits == AUD_BITS_24) {
                g_cap_data_24bit = true;
                g_cap_samp_bits = AUD_BITS_32;
            } else {
                g_cap_data_24bit = false;
            }
        }
        if (daud_stream_use_i2s_tdm_device(g_play_device)) {
            if (g_play_samp_bits == AUD_BITS_24) {
                g_play_data_24bit = true;
                g_play_samp_bits = AUD_BITS_32;
            } else {
                g_play_data_24bit = false;
            }
        }
        dma_audio_i2s_sw_trig_init();
        dma_aud_stream_inited = true;
    }
}

void dma_audio_stream_deinit(void)
{
    TRACE(0, "[%s]", __func__);

    if (dma_aud_stream_inited) {
        if (g_play_buf_ptr) {
            daud_heap_free(g_play_buf_ptr);
            g_play_buf_ptr = NULL;
            g_play_buf_size = 0;
        }
        if (g_cap_buf_ptr) {
            daud_heap_free(g_cap_buf_ptr);
            g_cap_buf_ptr = NULL;
            g_cap_buf_size = 0;
        }
        dma_aud_stream_inited = false;
    }
}

void dma_audio_stream_on(bool start)
{
    int ret = 0;

    DAUD_TRACE(2, "[%s] start: %d", __func__, start);

    if (g_stream_on == start) {
        TRACE(2, "[%s] WARNING: Invalid operation: %d", __func__, start);
        return;
    }

    if (start) {
#ifndef CHIP_ROLE_CP
        dma_audio_stream_request_clock();
#endif

#ifdef DMA_AUDIO_DUMP
        audio_dump_init(CAP_MIPS_SAMP, sizeof(short), DMA_AUDIO_DUMP_CHANNEL_NUM);
#endif

        struct DAUD_STREAM_CONFIG_T stream_cfg;
#ifdef DMA_AUDIO_ALGO
        struct DAUD_STREAM_CONFIG_T stream_cfg_algo[AUD_STREAM_NUM];
#endif
#if defined(DMA_AUDIO_LOW_LATENCY)
        ASSERT(PLAY_BUFF_SAMP_NUM >= 8, "%s: inavlid PLAY_BUFF_SAMP_NUM %d", __func__, PLAY_BUFF_SAMP_NUM);
        ASSERT(CAP_BUFF_SAMP_NUM >= 8, "%s: inavlid CAP_BUFF_SAMP_NUM %d", __func__, CAP_BUFF_SAMP_NUM);
        ASSERT(DMA_AUD_SAMP_RATE <= AUD_SAMPRATE_384000, "%s, invalid DMA_AUD_SAMP_RATE %d", __func__, DMA_AUD_SAMP_RATE);
#endif

        ASSERT(g_play_buf_size > 0, "%s: Bad g_play_buf_size=%d", __func__, g_play_buf_size);
        ASSERT(g_cap_buf_size > 0, "%s: Bad g_cap_buf_size=%d", __func__, g_cap_buf_size);
        ASSERT(g_play_buf_ptr != NULL, "%s: null g_play_buf_ptr", __func__);
        ASSERT(g_cap_buf_ptr != NULL, "%s: cap g_play_buf_ptr", __func__);

#ifdef DMA_AUDIO_MIPS_STAT
        audio_stream_mips_stat_init();
#endif
        // config playback stream
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        dma_audio_stream_get_config(AUD_STREAM_PLAYBACK, &stream_cfg);
#ifdef DMA_AUDIO_ALGO
        stream_cfg_algo[AUD_STREAM_PLAYBACK] = stream_cfg;
#endif

        ret = daud_stream_open(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, &stream_cfg);
        ASSERT(ret==0, "[%s] daud_stream_open playback failed: %d", __func__, ret);

#ifdef DMA_AUDIO_SYNC_CLOCK
        struct AUD_STREAM_SYNC_CONFIG_T sync_cfg;

        sync_cfg.sample_rate = stream_cfg.sample_rate;
        sync_cfg.bits = stream_cfg.bits;
        sync_cfg.channel_num = stream_cfg.channel_num;
        sync_cfg.data_ptr = stream_cfg.data_ptr;
        sync_cfg.data_size = stream_cfg.data_size;
        sync_cfg.pair_id = DAUD_STREAM_ID;
        sync_cfg.pair_stream = AUD_STREAM_CAPTURE;
        dma_audio_stream_sync_open(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, &sync_cfg);
        dma_audio_stream_sync_start(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
#endif

        // config capture stream
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        dma_audio_stream_get_config(AUD_STREAM_CAPTURE, &stream_cfg);
#ifdef DMA_AUDIO_ALGO
        stream_cfg_algo[AUD_STREAM_CAPTURE] = stream_cfg;
#endif

        ret = daud_stream_open(DAUD_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
        ASSERT(ret==0, "[%s] daud_stream_open capture failed: %d", __func__, ret);

#ifdef DMA_AUDIO_SYNC_CLOCK
        sync_cfg.sample_rate = stream_cfg.sample_rate;
        sync_cfg.bits = stream_cfg.bits;
        sync_cfg.channel_num = stream_cfg.channel_num;
        sync_cfg.data_ptr = stream_cfg.data_ptr;
        sync_cfg.data_size = stream_cfg.data_size;
        sync_cfg.pair_id = DAUD_STREAM_ID;
        sync_cfg.pair_stream = AUD_STREAM_PLAYBACK;
        dma_audio_stream_sync_open(DAUD_STREAM_ID, AUD_STREAM_CAPTURE, &sync_cfg);
        if (daud_stream_use_i2s_tdm_device(g_cap_device)
            && daud_stream_use_i2s_tdm_device(g_play_device)) {
            dma_audio_stream_sync_sw_trig_mode(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, true);
        }
        dma_audio_stream_sync_start(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
#endif

        dma_audio_data_format_info_print();
        dma_audio_stream_setup_memcpy_callback();

#ifdef DMA_AUDIO_FADE
        dma_audio_fade_init(&stream_cfg);
#endif

#ifdef DMA_AUDIO_ALGO
        dma_audio_algo_init(&stream_cfg_algo[AUD_STREAM_CAPTURE], &stream_cfg_algo[AUD_STREAM_PLAYBACK]);
#endif

        ret = daud_stream_start(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        ASSERT(ret==0, "[%s] daud_stream_start playback failed: %d", __func__, ret);

        ret = daud_stream_start(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        ASSERT(ret==0, "[%s] daud_stream_start capture failed: %d", __func__, ret);

#ifndef CHIP_ROLE_CP
        dma_audio_stream_release_clock();
#endif
    } else {
#ifndef CHIP_ROLE_CP
        dma_audio_stream_request_clock();
#endif

        daud_stream_stop(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        daud_stream_stop(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);

        daud_stream_close(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        daud_stream_close(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);

#ifdef DMA_AUDIO_ALGO
        dma_audio_algo_deinit();
#endif

#ifdef DMA_AUDIO_FADE
        dma_audio_fade_deinit();
#endif

#ifdef DMA_AUDIO_SYNC_CLOCK
        dma_audio_stream_sync_stop(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        dma_audio_stream_sync_stop(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        dma_audio_stream_sync_close(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        dma_audio_stream_sync_close(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        if (daud_stream_use_i2s_tdm_device(g_cap_device)
            && daud_stream_use_i2s_tdm_device(g_play_device)) {
            dma_audio_stream_sync_sw_trig_mode(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, false);
        }
#endif

#ifndef CHIP_ROLE_CP
        dma_audio_stream_release_clock();
#endif
    }

    g_stream_on = start;
}

void dma_audio_stream_setup_mic_data_handler(DMA_AUD_MIC_DATA_HDLR_T handler)
{
    mic_data_handler = handler;
}

void dma_audio_stream_setup_spk_data_handler(DMA_AUD_SPK_DATA_HDLR_T handler)
{
    spk_data_handler = handler;
}

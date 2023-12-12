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
#include "dma_audio_stream.h"
#include "dma_audio_stream_conf.h"
#include "dma_audio_algo.h"
#include "dma_audio.h"
#include "audio_dump.h"
#include "stream_mcps.h"
#include "psap_sw_comm.h"

// #define PSAP_SW_AUDIO_DUMP
// #define PSAP_SW_MCPS

#define CHAR_BYTES          (1)
#define SHORT_BYTES         (2)
#define INT_BYTES           (4)
#define SHIFT_DEFAULT       (4)

#if DMA_AUD_SAMP_SIZE == SHORT_BYTES
typedef short   VOICE_PCM_T;
#elif DMA_AUD_SAMP_SIZE == INT_BYTES
typedef int     VOICE_PCM_T;
#else
#error "Invalid DMA_AUD_SAMP_SIZE!!!"
#endif

#ifdef PSAP_SW_MCPS
static const char *g_stream_mcps_name = "psap_sw_dsp";
#endif

static struct DAUD_STREAM_CONFIG_T g_play_stream_cfg;
static struct DAUD_STREAM_CONFIG_T g_cap_stream_cfg;

static uint32_t g_cap_channel_num = 0;
static uint32_t g_play_channel_num = 0;

static uint32_t g_cap_frame_len = 0;
static uint32_t g_play_frame_len = 0;

static uint32_t g_gain_left_shift = 0;

int32_t psap_sw_dsp_init(struct DAUD_STREAM_CONFIG_T *cap_cfg, struct DAUD_STREAM_CONFIG_T *play_cfg)
{
    ASSERT(cap_cfg->sample_rate == play_cfg->sample_rate, "[%s] sample_rate is diff: %d != %d",
        __func__, cap_cfg->sample_rate, play_cfg->sample_rate);

    ASSERT(cap_cfg->bits == play_cfg->bits, "[%s] bits is diff: %d != %d",
        __func__, cap_cfg->bits, play_cfg->bits);

    ASSERT(((cap_cfg->bits + 8) / 16) * sizeof(int16_t) == sizeof(VOICE_PCM_T), "[%s] bits is diff: %d, %d",
        __func__, cap_cfg->bits, sizeof(VOICE_PCM_T));

    g_cap_channel_num = cap_cfg->channel_num;
    g_cap_frame_len = cap_cfg->data_size / 2 / sizeof(VOICE_PCM_T) / g_cap_channel_num;

    g_play_channel_num = play_cfg->channel_num;
    g_play_frame_len = play_cfg->data_size / 2 / sizeof(VOICE_PCM_T) / g_play_channel_num;

    g_gain_left_shift = SHIFT_DEFAULT;

    ASSERT(g_cap_frame_len == g_play_frame_len, "[%s] frame_len is diff: %d != %d",
        __func__, g_cap_frame_len, g_play_frame_len);

    TRACE(0, "[%s] sample_rate: %d, bits: %d", __func__, cap_cfg->sample_rate, cap_cfg->bits);
    TRACE(0, "[%s] cap_frame_len: %d, play_frame_len: %d", __func__, g_cap_frame_len, g_play_frame_len);
    TRACE(0, "[%s] g_cap_channel_num: %d, g_play_channel_num: %d", __func__, g_cap_channel_num, g_play_channel_num);

#ifdef PSAP_SW_AUDIO_DUMP
    audio_dump_init(g_cap_frame_len, sizeof(short), 1);
#endif

#ifdef PSAP_SW_MCPS
    //96：This is the main frequency value, it needs to be changed to the actual main frequency value
    stream_mcps_start(g_stream_mcps_name, 96, g_cap_frame_len / (cap_cfg->sample_rate  / 1000), 3000);
#endif

    return 0;
}

int32_t psap_sw_dsp_deinit(void)
{
#ifdef PSAP_SW_AUDIO_DUMP
    audio_dump_deinit();
#endif

#ifdef PSAP_SW_MCPS
    stream_mcps_stop(g_stream_mcps_name);
#endif

    return 0;
}

static int32_t psap_sw_cmd_handler(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg, uint32_t len, bool set)
{
    ASSERT(len == sizeof(psap_sw_cmd_t), "[%s] thirdparty input len(%d) != cfg len", __func__, len);

    psap_sw_cmd_t cmd;

    memcpy(&cmd, (psap_sw_cmd_t *)cfg, sizeof(psap_sw_cmd_t));

    TRACE(2, "[%s] mode: %d", __func__, cmd.mode);

    if (cmd.mode == PSAP_SW_THIRD_CMD_GAIN) {
        TRACE(2, "[%s] gain :%d", __func__, cmd.gain);
        g_gain_left_shift = cmd.gain;
    } else {
        TRACE(2, "[%s] WARNING: mode(%d) is invalid", __func__, cmd.mode);
    }

    return 0;
}

int32_t psap_sw_dsp_process(uint8_t *play_buf, uint32_t play_len, uint8_t *cap_buf, uint32_t cap_len)
{
    VOICE_PCM_T *cap_pcm_buf = (VOICE_PCM_T *)cap_buf;
    VOICE_PCM_T *play_pcm_buf = (VOICE_PCM_T *)play_buf;

    ASSERT(cap_len == g_cap_frame_len * g_cap_channel_num * sizeof(VOICE_PCM_T), "[%s] cap_len is invalid: %d", __func__, cap_len);
    ASSERT(play_len == g_play_frame_len * g_play_channel_num * sizeof(VOICE_PCM_T), "[%s] play_len is invalid: %d", __func__, play_len);

#ifdef PSAP_SW_AUDIO_DUMP
    audio_dump_clear_up();
    audio_dump_add_channel_data_from_multi_channels_32bit_to_16bit(0, cap_pcm_buf, g_cap_frame_len, g_cap_channel_num, 0, 8);
    audio_dump_run();
#endif

#ifdef PSAP_SW_MCPS
    stream_mcps_run_pre(g_stream_mcps_name);
#endif

    if (g_play_channel_num == 1) {
        for(int i = 0; i < g_cap_frame_len; i++) {
            play_pcm_buf[g_play_channel_num * i + 0] = cap_pcm_buf[g_cap_channel_num * i + 0] << g_gain_left_shift;
        }
    } else if (g_play_channel_num == 2) {
        for(int i = 0; i < g_cap_frame_len; i++) {
            play_pcm_buf[g_play_channel_num * i + 0] = cap_pcm_buf[g_cap_channel_num * i + 0] << g_gain_left_shift;
            play_pcm_buf[g_play_channel_num * i + 1] = cap_pcm_buf[g_cap_channel_num * i + 0] << g_gain_left_shift;
        }
    }

#ifdef PSAP_SW_MCPS
    stream_mcps_run_post(g_stream_mcps_name);
#endif

    return 0;
}

static void psap_sw_sw_init(bool init)
{
    if (init) {
        dma_audio_stream_get_config(AUD_STREAM_PLAYBACK, &g_play_stream_cfg);
        dma_audio_stream_get_config(AUD_STREAM_CAPTURE, &g_cap_stream_cfg);
        psap_sw_dsp_init(&g_cap_stream_cfg, &g_play_stream_cfg);
    } else {
        psap_sw_dsp_deinit();
    }
}

int32_t psap_sw_dsp_register_cb(bool en)
{
    if (en) {
        dma_audio_algo_setup_init_callback(psap_sw_sw_init);
        dma_audio_algo_setup_config_callback(psap_sw_cmd_handler);
        dma_audio_algo_setup_algo_proc_callback(psap_sw_dsp_process);
    } else {
        dma_audio_algo_setup_init_callback(NULL);
        dma_audio_algo_setup_config_callback(NULL);
        dma_audio_algo_setup_algo_proc_callback(NULL);
    }

    return 0;
}
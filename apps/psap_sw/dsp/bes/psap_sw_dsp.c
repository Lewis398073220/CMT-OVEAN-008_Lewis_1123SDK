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
#include "psap_freq_psap.h"
#include "psap_sw_app_ctrl.h"
#include "psap_sw_comm.h"

#define CHAR_BYTES          (1)
#define SHORT_BYTES         (2)
#define INT_BYTES           (4)

#if DMA_AUD_SAMP_SIZE == SHORT_BYTES
typedef short   VOICE_PCM_T;
#elif DMA_AUD_SAMP_SIZE == INT_BYTES
typedef int     VOICE_PCM_T;
#else
#error "Invalid DMA_AUD_SAMP_SIZE!!!"
#endif

static VOICE_PCM_T **g_pcm_ff_mic = NULL;
static VOICE_PCM_T **g_pcm_talk_mic = NULL;
static VOICE_PCM_T **g_pcm_spk  = NULL;
static VOICE_PCM_T **g_pcm_ref_mic = NULL;

static bool g_psap_sw_dsp_opened = false;

static PsapFreqPsapState      *freq_psap_st = NULL;
extern PsapFreqPsapConfig     psap_freq_psap_cfg;

static uint32_t g_cap_channel_num = 0;
static uint32_t g_play_channel_num = 0;
static uint32_t g_cap_frame_len = 0;
static uint32_t g_play_frame_len = 0;

static struct DAUD_STREAM_CONFIG_T g_play_stream_cfg;
static struct DAUD_STREAM_CONFIG_T g_cap_stream_cfg;

// #define PSAP_FREQ_PSAP_96K_DUMP
#ifdef PSAP_FREQ_PSAP_96K_DUMP
#include "audio_dump.h"
typedef short       _DUMP_PCM_T;
int frame_len0 = 192;
#endif

extern int32_t psap_freq_set_total_gain(float *gain_l);
extern int32_t psap_wdrc_set_dynamic_gain(float *gain, uint32_t band_num);
extern int32_t psap_bf_control(bool enable);
extern int32_t psap_ns_set_denoise_level(int32_t level);

static int32_t psap_sw_cmd_handler(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg, uint32_t len, bool set)
{
    ASSERT(len == sizeof(psap_sw_cmd_t), "[%s] bes input len(%d) != cfg len", __func__, len);

    psap_sw_cmd_t cmd;

    memcpy(&cmd, (psap_sw_cmd_t *)cfg, sizeof(psap_sw_cmd_t));

    TRACE(2, "[%s] mode: %d", __func__, cmd.mode);

    if (cmd.mode == PSAP_SW_CMD_TOTAL_GAIN) {
        TRACE(5, "[%s] Total Gain(x100): [L] %d, [R] %d", __func__,
                (int32_t)(cmd.total_gain[0] *100), (int32_t)(cmd.total_gain[1] *100));
        psap_freq_set_total_gain(cmd.total_gain);
    } else if (cmd.mode == PSAP_SW_CMD_MODE_WDRC) {
        TRACE(5, "[%s] Gain(x100): [0] %d, [1] %d, [-2] %d, [-1] %d", __func__,
                (int32_t)(cmd.band_gain[0] * 100),
                (int32_t)(cmd.band_gain[1] * 100),
                (int32_t)(cmd.band_gain[PSAP_BAND_NUM-2] * 100),
                (int32_t)(cmd.band_gain[PSAP_BAND_NUM-1] * 100));
        psap_wdrc_set_dynamic_gain(cmd.band_gain, PSAP_BAND_NUM);
    } else if (cmd.mode == PSAP_SW_CMD_MODE_BF) {
        TRACE(2, "[%s] bf_enable: %d", __func__, cmd.bf_enable);
        psap_bf_control(cmd.bf_enable);
    } else if (cmd.mode == PSAP_SW_CMD_MODE_NS) {
        TRACE(2, "[%s] ns_level: %d", __func__, cmd.ns_level);
        psap_ns_set_denoise_level(cmd.ns_level);
    } else if (cmd.mode == PSAP_SW_CMD_MODE_EQ) {
        TRACE(3, "[%s] eq config: %p/%p", __func__, cmd.eq_config[0], cmd.eq_config[1]);
        psap_freq_psap_set_eq_config(freq_psap_st, cmd.eq_config[0], AUD_CHANNEL_MAP_CH0);
        psap_freq_psap_set_eq_config(freq_psap_st, cmd.eq_config[1], AUD_CHANNEL_MAP_CH1);
    } else if (cmd.mode == PSAP_SW_CMD_MODE_DEHOWLING) {
        TRACE(3, "[%s] dehowling config: %d", __func__, cmd.dehowling_config.onoff);
        psap_freq_set_dehowling_config(freq_psap_st, &cmd.dehowling_config);
    } else if (cmd.mode == PSAP_SW_CMD_MODE_REVERB) {
        TRACE(3, "[%s] reverb config", __func__);
        psap_freq_set_reverb_config(freq_psap_st, cmd.reverb_cfg); 
    } else if (cmd.mode == PSAP_SW_CMD_MODE_ALL) {
        TRACE(5, "[%s] ALL Gain(x100): [0] %d, [1] %d, [-2] %d, [-1] %d", __func__,
                (int32_t)(cmd.band_gain[0] * 100),
                (int32_t)(cmd.band_gain[1] * 100),
                (int32_t)(cmd.band_gain[PSAP_BAND_NUM-2] * 100),
                (int32_t)(cmd.band_gain[PSAP_BAND_NUM-1] * 100));
        TRACE(2, "[%s] ALL bf_enable: %d", __func__, cmd.bf_enable);
        TRACE(2, "[%s] ALL ns_level: %d", __func__, cmd.ns_level);
        psap_wdrc_set_dynamic_gain(cmd.band_gain, PSAP_BAND_NUM);
        psap_bf_control(cmd.bf_enable);
        psap_ns_set_denoise_level(cmd.ns_level);
    } else {
        TRACE(2, "[%s] WARNING: mode(%d) is invalid", __func__, cmd.mode);
    }

    return 0;
}

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

    ASSERT(g_cap_frame_len == g_play_frame_len, "[%s] frame_len is diff: %d != %d",
        __func__, g_cap_frame_len, g_play_frame_len);

    TRACE(0, "[%s] sample_rate: %d, bits: %d", __func__, cap_cfg->sample_rate, cap_cfg->bits);
    TRACE(0, "[%s] cap_frame_len: %d, play_frame_len: %d", __func__, g_cap_frame_len, g_play_frame_len);
    TRACE(0, "[%s] g_cap_channel_num: %d, g_play_channel_num: %d", __func__, g_cap_channel_num, g_play_channel_num);

    if(g_psap_sw_dsp_opened == true) {
        TRACE(0,"[%s] WARNING: PSAP SW DSP is opened", __func__);
        return 1;
    }

    g_pcm_ff_mic = (VOICE_PCM_T **)daud_heap_malloc(sizeof(VOICE_PCM_T*)*g_play_channel_num);
    g_pcm_talk_mic = (VOICE_PCM_T **)daud_heap_malloc(sizeof(VOICE_PCM_T*)*g_play_channel_num);
    g_pcm_spk = (VOICE_PCM_T **)daud_heap_malloc(sizeof(VOICE_PCM_T*)*g_play_channel_num);
    g_pcm_ref_mic = (VOICE_PCM_T **)daud_heap_malloc(sizeof(VOICE_PCM_T*)*g_play_channel_num);
    for (int i = 0;i < g_play_channel_num; i++){
        g_pcm_ff_mic[i] = (VOICE_PCM_T *)daud_heap_malloc(sizeof(VOICE_PCM_T)*g_cap_frame_len);
    }
    for (int i = 0;i < g_play_channel_num; i++){
        g_pcm_talk_mic[i] = (VOICE_PCM_T *)daud_heap_malloc(sizeof(VOICE_PCM_T)*g_cap_frame_len);
    }
    for (int i = 0;i < g_play_channel_num; i++){
        g_pcm_spk[i] = (VOICE_PCM_T *)daud_heap_malloc(sizeof(VOICE_PCM_T)*g_play_frame_len);
    }
    for (int i = 0;i < g_play_channel_num; i++){
        g_pcm_ref_mic[i] = (VOICE_PCM_T *)daud_heap_malloc(sizeof(VOICE_PCM_T)*g_play_frame_len);
    }

    freq_psap_st = psap_freq_psap_create(cap_cfg->sample_rate, cap_cfg->bits, g_play_frame_len, g_play_channel_num, &psap_freq_psap_cfg);

#ifdef PSAP_FREQ_PSAP_96K_DUMP
    audio_dump_init(frame_len0, sizeof(_DUMP_PCM_T), 1);
#endif

    g_psap_sw_dsp_opened = true;

    return 0;
}

int32_t psap_sw_dsp_deinit(void)
{
    for (int i = 0; i < g_play_channel_num; i++){
        daud_heap_free(g_pcm_ff_mic[i]);
    }
    daud_heap_free(g_pcm_ff_mic);

    for (int i = 0; i < g_play_channel_num; i++){
        daud_heap_free(g_pcm_talk_mic[i]);
    }
    daud_heap_free(g_pcm_talk_mic);

    for (int i = 0; i < g_play_channel_num; i++){
        daud_heap_free(g_pcm_spk[i]);
    }
    daud_heap_free(g_pcm_spk);

    for (int i = 0; i < g_play_channel_num; i++){
        daud_heap_free(g_pcm_ref_mic[i]);
    }
    daud_heap_free(g_pcm_ref_mic);

    psap_freq_psap_destroy(freq_psap_st);

#ifdef PSAP_FREQ_PSAP_96K_DUMP
    audio_dump_deinit();
#endif

    g_psap_sw_dsp_opened = false;

    TRACE(0, "[%s] g_cap_channel_num: %d, g_play_channel_num: %d", __func__, g_cap_channel_num, g_play_channel_num);
    return 0;
}

static void psap_sw_init(bool init)
{
    if (init) {
        dma_audio_stream_get_config(AUD_STREAM_CAPTURE, &g_cap_stream_cfg);
        dma_audio_stream_get_config(AUD_STREAM_PLAYBACK, &g_play_stream_cfg);
        psap_sw_dsp_init(&g_cap_stream_cfg, &g_play_stream_cfg);
    } else {
        psap_sw_dsp_deinit();
    }
}

int32_t psap_sw_dsp_process(uint8_t *play_buf, uint32_t play_len, uint8_t *cap_buf, uint32_t cap_len)
{

    VOICE_PCM_T *pcm_buf = (VOICE_PCM_T *)cap_buf;
    VOICE_PCM_T *dst_buf = (VOICE_PCM_T *)play_buf;
    VOICE_PCM_T **pcm_ff_mic = g_pcm_ff_mic;
    VOICE_PCM_T **pcm_talk_mic = g_pcm_talk_mic;
    VOICE_PCM_T **pcm_spk = g_pcm_spk;
    VOICE_PCM_T **pcm_ref_mic = g_pcm_ref_mic;

    if (g_play_channel_num == 1) { // tws
        for (int i = 0; i < g_play_frame_len; i++) {
            pcm_ff_mic[0][i] = pcm_buf[0 + g_cap_channel_num * i];
            pcm_talk_mic[0][i] = pcm_buf[1 + g_cap_channel_num * i];
            pcm_ref_mic[0][i] = pcm_buf[2 + g_cap_channel_num * i];
        }
    } else if (g_play_channel_num == 2){ // stereo
        for (int i = 0; i < g_play_frame_len; i++) {
            pcm_ff_mic[0][i] = pcm_buf[0 + g_cap_channel_num * i];
            pcm_ff_mic[1][i] = pcm_buf[1 + g_cap_channel_num * i];
            pcm_talk_mic[0][i] = pcm_buf[2 + g_cap_channel_num * i];
            pcm_talk_mic[1][i] = pcm_buf[3 + g_cap_channel_num * i];
        }
    }

    // uint32_t start_time = FAST_TICKS_TO_US(hal_fast_sys_timer_get());
    psap_freq_psap_process(freq_psap_st, (int32_t **)pcm_ff_mic, (int32_t **)pcm_talk_mic, (int32_t **)pcm_ref_mic, (int32_t **)pcm_spk, &psap_freq_psap_cfg);
    // TRACE(2,"psap time = %d us",FAST_TICKS_TO_US(hal_fast_sys_timer_get())-start_time);

    if (g_play_channel_num == 1) { // tws
        for (int i = 0; i < g_play_frame_len; i++) {
            dst_buf[0 + g_play_channel_num * i] = pcm_spk[0][i];
        }
    } else if (g_play_channel_num == 2){ // stereo
        for (int i = 0; i < g_play_frame_len; i++) {
            dst_buf[0 + g_play_channel_num * i] = pcm_spk[0][i];
            dst_buf[1 + g_play_channel_num * i] = pcm_spk[1][i];
        }
    }


#ifdef PSAP_FREQ_PSAP_96K_DUMP
    audio_dump_clear_up();

    audio_dump_add_channel_data_from_multi_channels_32bit_to_16bit(0, pcm_ff_mic[0], frame_len0, 1, 0, 8);

    audio_dump_run();
#endif

    return 0;
}

int32_t psap_sw_dsp_register_cb(bool en)
{
    if (en) {
        dma_audio_algo_setup_init_callback(psap_sw_init);
        dma_audio_algo_setup_config_callback(psap_sw_cmd_handler);
        dma_audio_algo_setup_algo_proc_callback(psap_sw_dsp_process);
    } else {
        dma_audio_algo_setup_init_callback(NULL);
        dma_audio_algo_setup_config_callback(NULL);
        dma_audio_algo_setup_algo_proc_callback(NULL);
    }

    return 0;
}
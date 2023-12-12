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
#ifdef BLE_I2S_AUDIO_SUPPORT
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "audioflinger.h"
#include "app_ble_i2s_audio_stream.h"

#ifdef AOB_MOBILE_ENABLED
#include "app_bt_func.h"
#include "plat_types.h"
#include "app_audio.h"
#include "app_ble_i2s_audio.h"
#include "hal_tdm.h"

#ifdef HEADSET_AS_TDM0_SLAVE
#define I2S_AUDIO_PLAYBACK_96K
#define I2S_AUDIO_24BIT
#endif

#ifdef I2S_AUDIO_32BIT
#define SAMPLE_SIZE_PLAYBACK            4
#define SAMPLE_SIZE_CAPTURE             4
#elif defined(I2S_AUDIO_24BIT)
#define SAMPLE_SIZE_PLAYBACK            4
#define SAMPLE_SIZE_CAPTURE             4
#else
#define SAMPLE_SIZE_PLAYBACK            2
#define SAMPLE_SIZE_CAPTURE             2
#endif

#define CHAN_NUM_PLAYBACK               2
#define CHAN_NUM_CAPTURE                2

#ifdef I2S_AUDIO_PLAYBACK_96K
#define SAMPLE_RATE_PLAYBACK            AUD_SAMPRATE_96000
#else
#define SAMPLE_RATE_PLAYBACK            AUD_SAMPRATE_48000
#endif

#define SAMPLE_RATE_CAPTURE             AUD_SAMPRATE_48000

#define SAMP_RATE_TO_FRAME_SIZE(n)      (((n) + (1000 - 1)) / 1000)
#define SAMPLE_FRAME_PLAYBACK           SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_PLAYBACK)
#define SAMPLE_FRAME_CAPTURE            SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_CAPTURE)

#define SAMP_TO_BYTE_PLAYBACK(n)        ((n) * SAMPLE_SIZE_PLAYBACK * CHAN_NUM_PLAYBACK)
#define SAMP_TO_BYTE_CAPTURE(n)         ((n) * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)

#define FRAME_SIZE_PLAYBACK             SAMP_TO_BYTE_PLAYBACK(SAMPLE_FRAME_PLAYBACK)
#define FRAME_SIZE_CAPTURE              SAMP_TO_BYTE_CAPTURE(SAMPLE_FRAME_CAPTURE)

#define PLAYBACK_FRAME_NUM              20
#define CAPTURE_FRAME_NUM               20

#define BUFF_ALIGN                      (4 * 4)
#define NON_EXP_ALIGN(val, exp)         (((val) + ((exp) - 1)) / (exp) * (exp))

#define I2S_AUDIO_PLAYBACK_BUFF_SIZE    NON_EXP_ALIGN(FRAME_SIZE_PLAYBACK * PLAYBACK_FRAME_NUM, BUFF_ALIGN)
#define I2S_AUDIO_CAPTURE_BUFF_SIZE     NON_EXP_ALIGN(FRAME_SIZE_CAPTURE * CAPTURE_FRAME_NUM, BUFF_ALIGN)

// #define ALIGNED4                        ALIGNED(4)
// static uint8_t ALIGNED4 i2s_in_buff[I2S_AUDIO_PLAYBACK_BUFF_SIZE] = {0};
// static uint8_t ALIGNED4 dac_out_buff[I2S_AUDIO_PLAYBACK_BUFF_SIZE] = {0};
// static uint8_t ALIGNED4 i2s_out_buff[I2S_AUDIO_CAPTURE_BUFF_SIZE] = {0};
// static uint8_t ALIGNED4 dac_in_buff[I2S_AUDIO_CAPTURE_BUFF_SIZE] = {0};

static uint8_t* i2s_in_buff = NULL;
static uint8_t* dac_out_buff = NULL;
static uint8_t* i2s_out_buff = NULL;
static uint8_t* dac_in_buff = NULL;

static enum AUD_SAMPRATE_T sample_rate_play = SAMPLE_RATE_PLAYBACK;
static enum AUD_SAMPRATE_T sample_rate_cap = SAMPLE_RATE_CAPTURE;

static const uint8_t sample_size_play = SAMPLE_SIZE_PLAYBACK;
static const uint8_t sample_size_cap = SAMPLE_SIZE_CAPTURE;

static uint8_t playback_vol = TGT_ADC_VOL_LEVEL_10;
static uint8_t capture_vol = TGT_ADC_VOL_LEVEL_10;

static uint8_t* pcm_queue_buff[AUD_STREAM_NUM] = {NULL};
static CQueue pcm_queue[AUD_STREAM_NUM];
static osMutexId pcmbuff_mutex_id[AUD_STREAM_NUM] = {NULL};
osMutexDef(pcmbuff_playback_mutex);
osMutexDef(pcmbuff_capture_mutex);

static bool lea_i2s_playback_streaming = false;
static bool lea_i2s_capture_streaming = false;

#if defined(DONGLE_AS_I2S0_MASTER)
static AUD_STREAM_USE_DEVICE_T i2s_device = AUD_STREAM_USE_I2S0_MASTER;
#elif defined(HEADSET_AS_TDM0_SLAVE)
static AUD_STREAM_USE_DEVICE_T i2s_device = AUD_STREAM_USE_TDM0_SLAVE;
#else
static AUD_STREAM_USE_DEVICE_T i2s_device = AUD_STREAM_USE_I2S0_SLAVE;
#endif

static struct I2S_AUDIO_SOURCE_EVENT_CALLBACK_T i2s_audio_source_cb_list;

void i2s_audio_source_config_init(const struct I2S_AUDIO_SOURCE_EVENT_CALLBACK_T *cb_list)
{
    memcpy(&i2s_audio_source_cb_list, cb_list, sizeof(i2s_audio_source_cb_list));
}

static enum AUD_BITS_T sample_size_to_enum(uint32_t size)
{
    if (size == 2) {
        return AUD_BITS_16;
    } else if (size == 4) {
        return AUD_BITS_24;
    } else {
        ASSERT(false, "%s: Invalid sample size: %u", __FUNCTION__, size);
    }

    return AUD_BITS_NULL;
}

static enum AUD_CHANNEL_NUM_T chan_num_to_enum(uint32_t num)
{
    return (enum AUD_CHANNEL_NUM_T)(AUD_CHANNEL_NUM_1 + (num - 1));
}

int app_ble_i2s_audio_pcmbuff_init(AUD_STREAM_T stream)
{
    uint32_t pcmbuff_size = 0;
    if (stream == AUD_STREAM_PLAYBACK) {
        if (pcmbuff_mutex_id[stream] == NULL)
            pcmbuff_mutex_id[stream] = osMutexCreate((osMutex(pcmbuff_playback_mutex)));
        pcmbuff_size = I2S_AUDIO_PLAYBACK_BUFF_SIZE;
    } else if (stream == AUD_STREAM_CAPTURE) {
        if (pcmbuff_mutex_id[stream] == NULL)
            pcmbuff_mutex_id[stream] = osMutexCreate((osMutex(pcmbuff_capture_mutex)));
        pcmbuff_size = I2S_AUDIO_CAPTURE_BUFF_SIZE;
    } else {
        ASSERT(0, "invalid aud stream: %d", stream);
        return -1;
    }

    ASSERT(pcmbuff_mutex_id[stream], "%s create os mutex failed.", __func__);

    if (lea_i2s_playback_streaming == false && lea_i2s_capture_streaming == false) {
        app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
        pcm_queue_buff[AUD_STREAM_PLAYBACK] = NULL;
        pcm_queue_buff[AUD_STREAM_CAPTURE] = NULL;
        i2s_in_buff = NULL; dac_out_buff = NULL;
        i2s_out_buff = NULL; dac_in_buff = NULL;
    }

    if (stream == AUD_STREAM_PLAYBACK) {
        if (pcm_queue_buff[stream] == NULL) app_audio_mempool_get_buff(&pcm_queue_buff[stream], pcmbuff_size * 2);
        if (i2s_in_buff  == NULL) app_audio_mempool_get_buff(&i2s_in_buff, pcmbuff_size);
        if (dac_out_buff == NULL) app_audio_mempool_get_buff(&dac_out_buff, pcmbuff_size);

        ASSERT(pcm_queue_buff[stream] && i2s_in_buff && dac_out_buff, "%s get mempool failed.", __func__);
    }else if(stream == AUD_STREAM_CAPTURE) {
        if (pcm_queue_buff[stream] == NULL) app_audio_mempool_get_buff(&pcm_queue_buff[stream], pcmbuff_size * 2);
        if (i2s_out_buff == NULL) app_audio_mempool_get_buff(&i2s_out_buff, pcmbuff_size);
        if (dac_in_buff  == NULL) app_audio_mempool_get_buff(&dac_in_buff, pcmbuff_size);

        ASSERT(pcm_queue_buff[stream] && i2s_out_buff && dac_in_buff, "%s get mempool failed.", __func__);
    }

    osMutexWait(pcmbuff_mutex_id[stream], osWaitForever);
    InitCQueue(&pcm_queue[stream], pcmbuff_size * 2, pcm_queue_buff[stream]);
    memset(pcm_queue_buff[stream], 0x00, pcmbuff_size * 2);
    osMutexRelease(pcmbuff_mutex_id[stream]);

    return 0;
}

int app_ble_i2s_audio_pcmbuff_space(AUD_STREAM_T stream)
{
    int len;

    osMutexWait(pcmbuff_mutex_id[stream], osWaitForever);
    len = AvailableOfCQueue(&pcm_queue[stream]);
    osMutexRelease(pcmbuff_mutex_id[stream]);

    return len;
}

int app_ble_i2s_audio_pcmbuff_length(AUD_STREAM_T stream)
{
    int len;

    osMutexWait(pcmbuff_mutex_id[stream], osWaitForever);
    len = LengthOfCQueue(&pcm_queue[stream]);
    osMutexRelease(pcmbuff_mutex_id[stream]);

    return len;
}

int app_ble_i2s_audio_pcmbuff_put(AUD_STREAM_T stream, uint8_t *pcm, uint16_t pcm_len)
{
    int status;

    osMutexWait(pcmbuff_mutex_id[stream], osWaitForever);
    status = EnCQueue(&pcm_queue[stream], pcm, pcm_len);
    osMutexRelease(pcmbuff_mutex_id[stream]);

    return status;
}

int app_ble_i2s_audio_pcmbuff_get(AUD_STREAM_T stream, uint8_t *pcm, uint16_t pcm_len)
{
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int status;

    osMutexWait(pcmbuff_mutex_id[stream], osWaitForever);
    status = PeekCQueue(&pcm_queue[stream], pcm_len, &e1, &len1, &e2, &len2);
    if (pcm_len == (len1 + len2)) {
        memcpy(pcm, e1, len1);
        memcpy(pcm + len1, e2, len2);
        DeCQueue(&pcm_queue[stream], 0, len1);
        DeCQueue(&pcm_queue[stream], 0, len2);
    }else {
        memset(pcm, 0x00, pcm_len);
        status = -1;
    }
    osMutexRelease(pcmbuff_mutex_id[stream]);

    return status;
}

static uint32_t lea_i2s_playback_data_handler_i2s_in(uint8_t *buf, uint32_t len)
{
    app_ble_i2s_audio_pcmbuff_put(AUD_STREAM_PLAYBACK, buf, len);

    return len;
}

static uint32_t lea_i2s_playback_data_handler_dac_out(uint8_t *buf, uint32_t len)
{
    app_ble_i2s_audio_pcmbuff_get(AUD_STREAM_PLAYBACK, (uint8_t *)buf, len);

    if (i2s_audio_source_cb_list.data_playback_cb) {
        i2s_audio_source_cb_list.data_playback_cb(buf, len);
    }

    return len;
}

static uint32_t lea_i2s_playback_data_handler_dac_in(uint8_t *buf, uint32_t len)
{
    if (i2s_audio_source_cb_list.data_capture_cb) {
        i2s_audio_source_cb_list.data_capture_cb(buf, len);
    }

    app_ble_i2s_audio_pcmbuff_put(AUD_STREAM_CAPTURE, buf, len);

    return len;
}

static uint32_t lea_i2s_playback_data_handler_i2s_out(uint8_t *buf, uint32_t len)
{
    app_ble_i2s_audio_pcmbuff_get(AUD_STREAM_CAPTURE, (uint8_t *)buf, len);

    return len;
}

void app_ble_audio_i2s_playback_stream_onoff(bool onoff)
{
    if (lea_i2s_playback_streaming == onoff)
        return;

    if (onoff) {

        app_ble_i2s_audio_pcmbuff_init(AUD_STREAM_PLAYBACK);

        struct AF_STREAM_CONFIG_T stream_cfg;
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = sample_size_to_enum(sample_size_play);
        stream_cfg.sample_rate = sample_rate_play;
        stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_PLAYBACK);
        stream_cfg.device = i2s_device;
        stream_cfg.handler = lea_i2s_playback_data_handler_i2s_in;
        stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
        stream_cfg.data_ptr = i2s_in_buff;
        stream_cfg.data_size = I2S_AUDIO_PLAYBACK_BUFF_SIZE;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);

        if (stream_cfg.device == AUD_STREAM_USE_TDM0_SLAVE) {
            stream_cfg.sync_start = false;
            if (stream_cfg.bits == AUD_BITS_24)
                stream_cfg.slot_cycles = AUD_BITS_32;
            else
                stream_cfg.slot_cycles = stream_cfg.bits;

            stream_cfg.fs_cycles = HAL_TDM_FS_CYCLES_1;
            stream_cfg.align = AUD_DATA_ALIGN_I2S;
            stream_cfg.fs_edge = AUD_FS_FIRST_EDGE_POS;
        }

        af_stream_open(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_CAPTURE, &stream_cfg);
        af_stream_start(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_CAPTURE);

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.bits = sample_size_to_enum(sample_size_play);
        stream_cfg.sample_rate = sample_rate_play;
        stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_PLAYBACK);
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = playback_vol;
        stream_cfg.handler = lea_i2s_playback_data_handler_dac_out;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.data_ptr = dac_out_buff;
        stream_cfg.data_size = I2S_AUDIO_PLAYBACK_BUFF_SIZE;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);

        af_stream_open(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK, &stream_cfg);

        if (i2s_audio_source_cb_list.data_prep_cb != NULL) {
            i2s_audio_source_cb_list.data_prep_cb(AUD_STREAM_PLAYBACK);
        }
        af_stream_start(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);

        lea_i2s_playback_streaming = true;

    }else {
        af_stream_stop(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_CAPTURE);
        af_stream_stop(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);

        af_stream_close(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_CAPTURE);
        af_stream_close(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);

        lea_i2s_playback_streaming = false;
    }
}

void app_ble_audio_i2s_capture_stream_onoff(bool onoff)
{
    if (lea_i2s_capture_streaming == onoff)
        return;

    if (onoff) {

        app_ble_i2s_audio_pcmbuff_init(AUD_STREAM_CAPTURE);

        struct AF_STREAM_CONFIG_T stream_cfg;
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = sample_size_to_enum(sample_size_play);
        stream_cfg.sample_rate = sample_rate_play;
        stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_CAPTURE);
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = capture_vol;
        stream_cfg.handler = lea_i2s_playback_data_handler_dac_in;
        stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
        stream_cfg.data_ptr = dac_in_buff;
        stream_cfg.data_size = I2S_AUDIO_CAPTURE_BUFF_SIZE;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);

        af_stream_open(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE, &stream_cfg);

        if (i2s_audio_source_cb_list.data_prep_cb != NULL) {
            i2s_audio_source_cb_list.data_prep_cb(AUD_STREAM_CAPTURE);
        }

        af_stream_start(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.bits = sample_size_to_enum(sample_size_cap);
        stream_cfg.sample_rate = sample_rate_cap;
        stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_CAPTURE);
        stream_cfg.device = i2s_device;
        stream_cfg.vol = capture_vol;
        stream_cfg.handler = lea_i2s_playback_data_handler_i2s_out;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.data_ptr = i2s_out_buff;
        stream_cfg.data_size = I2S_AUDIO_CAPTURE_BUFF_SIZE;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);

        af_stream_open(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_PLAYBACK);

        lea_i2s_capture_streaming = true;
    }else {
        af_stream_stop(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
        af_stream_stop(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_PLAYBACK);

        af_stream_close(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
        af_stream_close(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_PLAYBACK);

        lea_i2s_capture_streaming = false;
    }
}

void app_ble_audio_i2s_playback_stream_start(void)
{
#ifdef WIFI_ALWAYS_AS_SOURCE
    return;
#endif
    if (i2s_audio_source_cb_list.playback_start_cb != NULL) {
        i2s_audio_source_cb_list.playback_start_cb();
    }

    return app_ble_audio_i2s_playback_stream_onoff(true);
}

void app_ble_audio_i2s_playback_stream_stop(void)
{
#ifdef WIFI_ALWAYS_AS_SOURCE
    return;
#endif
    if (i2s_audio_source_cb_list.playback_stop_cb != NULL) {
        i2s_audio_source_cb_list.playback_stop_cb();
    }

    return app_ble_audio_i2s_playback_stream_onoff(false);
}

void app_ble_audio_i2s_capture_stream_start(void)
{
    if (i2s_audio_source_cb_list.capture_start_cb != NULL) {
        i2s_audio_source_cb_list.capture_start_cb();
    }

    return app_ble_audio_i2s_capture_stream_onoff(true);
}

void app_ble_audio_i2s_capture_stream_stop(void)
{
    if (i2s_audio_source_cb_list.capture_stop_cb != NULL) {
        i2s_audio_source_cb_list.capture_stop_cb();
    }

    return app_ble_audio_i2s_capture_stream_onoff(false);
}

void app_ble_audio_i2s_playback_stream_vol_control(uint32_t volume_level)
{
    struct AF_STREAM_CONFIG_T *cfg;
    uint8_t POSSIBLY_UNUSED old_vol;
    uint32_t ret;

    TRACE(1, "vol_control level:%d", volume_level);

    playback_vol = volume_level;
    ret = af_stream_get_cfg(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK, &cfg, true);
    if (ret == 0) {
        old_vol = cfg->vol;
        if (old_vol != volume_level) {
            cfg->vol = volume_level;
            af_stream_setup(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK, cfg);
        }
    }

    if (i2s_audio_source_cb_list.playback_vol_change_cb != NULL) {
        i2s_audio_source_cb_list.playback_vol_change_cb(volume_level);
    }
}

uint8_t app_ble_audio_i2s_playback_stream_vol_get(void)
{
    return playback_vol;
}
#endif

#ifdef DMA_AUDIO_APP
#include "dma_audio_host.h"
#include "dma_audio_def.h"
#include "dma_audio_cli.h"
#endif

#include "gaf_media_stream.h"
static bool low_latency_stream_started = false;
static bool resume_low_latency_stream = false;

static uint8_t low_latency_playback_vol = TGT_ADC_VOL_LEVEL_3;

void app_ble_audio_stream_start_pre_handler(void)
{
#ifdef WIFI_ALWAYS_AS_SOURCE
    return;
#endif
    TRACE(1, "enter %s", __func__);
    if (low_latency_stream_started == true)
    {
        app_ble_audio_low_latency_playback_stream_stop();
        resume_low_latency_stream = true;
    }
}

void app_ble_audio_stream_stop_post_handler(void)
{
#ifdef WIFI_ALWAYS_AS_SOURCE
    return;
#endif
    TRACE(1, "enter %s", __func__);
    if (resume_low_latency_stream == true)
    {
        app_ble_audio_low_latency_playback_stream_start();
    }
}

void app_ble_audio_low_latency_playback_stream_start(void)
{
    TRACE(1, "%s", __func__);
#ifdef DMA_AUDIO_APP
    if ((gaf_audio_is_playback_stream_on() == false) &&
#ifndef WIFI_ALWAYS_AS_SOURCE
        (gaf_audio_is_capture_stream_on() == false) &&
#endif
        (low_latency_stream_started == false)) {
        dma_audio_cli_key_on(DAUD_CLI_KEY_NORMAL);
        app_ble_audio_low_latency_playback_stream_vol_control(low_latency_playback_vol);
        low_latency_stream_started = true;
    }
#endif
}

void app_ble_audio_low_latency_playback_stream_stop(void)
{
    TRACE(1, "%s", __func__);
#ifdef DMA_AUDIO_APP
    if (low_latency_stream_started == true) {
        dma_audio_cli_key_off(DAUD_CLI_KEY_NORMAL);
        low_latency_stream_started = false;
    }
#endif
}

void app_ble_audio_low_latency_playback_stream_vol_control(uint32_t volume_level)
{
#ifdef DMA_AUDIO_APP
    struct AF_STREAM_CONFIG_T *cfg;
    uint8_t POSSIBLY_UNUSED old_vol;
    uint32_t ret;

    TRACE(1, "vol_control level:%d", volume_level);

    low_latency_playback_vol = volume_level;
    ret = af_stream_get_cfg(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, &cfg, true);
    if (ret == 0) {
        old_vol = cfg->vol;
        if (old_vol != volume_level) {
            cfg->vol = volume_level;
            af_stream_setup(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, cfg);
        }
    }
#endif
}

uint8_t app_ble_audio_low_latency_playback_stream_vol_get(void)
{
    return low_latency_playback_vol;
}
#endif
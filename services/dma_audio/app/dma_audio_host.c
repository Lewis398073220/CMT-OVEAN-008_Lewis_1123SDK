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
#ifdef DMA_AUDIO_APP
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "string.h"
#include "analog.h"
#include "hal_key.h"
#include "hal_key.h"
#include "hal_aud.h"
#include "hal_codec.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_i2s.h"
#include "hal_sysfreq.h"
#include "audioflinger.h"
#include "stream_dma_rpc.h"
#include "dma_audio_host.h"
#include "dma_audio_def.h"
#include "dma_audio_stream_conf.h"
#include "mcu_dsp_m55_app.h"
#if defined(CP_AS_SUBSYS)
#include "cp_subsys.h"
#elif defined(SENSOR_HUB)
#include "sensor_hub.h"
#else
#include "dsp_m55.h"
#endif
#ifndef NOAPP
#include "app_utils.h"
#endif

#ifndef CODEC_SW_SYNC
#error "CODEC_SW_SYNC should be defined !"
#endif

#define DAUD_HOST_DBG_SYNC_VERBOSE 1

#define WS_TRIGGER_CNT 16384

#define CODEC_ALIGN_MODE_PSAP (CODEC_ALIGN_MODE_DAC1 | CODEC_ALIGN_MODE_DAC2 | CODEC_ALIGN_MODE_DAC3)

static bool dma_aud_cur_state = false;
static bool dma_aud_inited = false;
static bool dma_aud_req_clock = false;
static enum AUD_STREAM_USE_DEVICE_T dma_aud_sync_device[AUD_STREAM_NUM] = {0};

static DMA_AUD_STATE_CB_T pre_state_switch_cb[DMA_AUD_CB_ID_QTY] = {NULL};
static DMA_AUD_STATE_CB_T post_state_switch_cb[DMA_AUD_CB_ID_QTY] = {NULL};

static uint32_t g_dma_stream_fade_ms = DAUD_STREAM_FADE_MS;

#define ASSERT_DMA_AUD_CB_ID(id) (ASSERT((id) < DMA_AUD_CB_ID_QTY, "%s: invalid id %d", __func__, id))

static uint8_t dma_aud_rpcif_user_map = 0;

static struct DAUD_STREAM_CONFIG_T dma_aud_cur_cfg[AUD_STREAM_NUM];

#ifdef RTOS
static osMutexId dma_aud_mutex_id = NULL;
osMutexDef(dma_aud_mutex);
#else
static int dma_aud_lock_flag = 0;
#endif

#ifdef CODEC_SW_SYNC
void hal_codec_sw_sync_low_latency_enable(uint32_t intv_us);
#endif

static void cpu_whoami(void)
{
    uint32_t cpuid = get_cpu_id();

#if defined(CHIP_BEST1600) || defined(CHIP_BEST1603)
    TRACE(1, "CPUID:%d, CORE:%s", cpuid, cpuid==0?"M55":"M33");
#else
    TRACE(1, "CPUID:%d", cpuid);
#endif
}

static void dma_audio_lock(void)
{
#ifdef RTOS
    ASSERT(dma_aud_mutex_id!=NULL, "%s: null ptr", __func__);
    osMutexWait(dma_aud_mutex_id, osWaitForever);
#else
    ASSERT(dma_aud_lock_flag==0, "%s: locked", __func__);
    dma_aud_lock_flag = 1;
#endif
}

static void dma_audio_unlock(void)
{
#ifdef RTOS
    ASSERT(dma_aud_mutex_id!=NULL, "%s: null ptr", __func__);
    osMutexRelease(dma_aud_mutex_id);
#else
    ASSERT(dma_aud_lock_flag==1, "%s: locked", __func__);
    dma_aud_lock_flag = 0;
#endif
}

unsigned int dma_audio_rx_data_handler(const void *data, unsigned int len)
{
    struct DMA_MSG_HDR_T *hdr = (struct DMA_MSG_HDR_T *)data;
    uint32_t req_len = sizeof(struct DMA_RPC_REQ_MSG_T);
    uint32_t rsp_len = sizeof(struct DMA_RPC_REPLY_MSG_T);

    if (((len != req_len) && (len != rsp_len)) || (hdr->id != DMA_MSG_ID_RPC)) {
        TRACE(1, "dma_audio_host: recv unkown msg, data=%x,len=%d", (int)data, len);
        return 0;
    }
    dma_rpcif_rx_handler(data, len);
    return len;
}

void dma_audio_tx_data_handler(const void *data, unsigned int len)
{
    struct DMA_MSG_HDR_T *hdr = (struct DMA_MSG_HDR_T *)data;
    uint32_t req_len = sizeof(struct DMA_RPC_REQ_MSG_T);
    uint32_t rsp_len = sizeof(struct DMA_RPC_REPLY_MSG_T);

    if (((len != req_len) && (len != rsp_len)) || (hdr->id != DMA_MSG_ID_RPC)) {
        TRACE(1, "dma_audio_host: send unknown msg done: data=%x,len=%d", (int)data, len);
        return;
    }
    dma_rpcif_tx_handler(data, len);
}

static void dma_audio_init_stream_cfg(void)
{
    struct DAUD_STREAM_CONFIG_T *cfg;

    TRACE(1, "[%s]: ", __func__);

    cfg = &dma_aud_cur_cfg[AUD_STREAM_PLAYBACK];
    cfg->sample_rate = DMA_AUD_SAMP_RATE;
    cfg->bits        = DMA_AUD_SAMP_BITS;
    cfg->channel_num = DMA_AUD_CHAN_NUM_PLAY;
    cfg->channel_map = DMA_AUD_CHAN_MAP_PLAY;
    cfg->device      = DMA_AUD_PLAY_DEV;
    cfg->io_path     = AUD_OUTPUT_PATH_SPEAKER;
    cfg->vol         = 17;
    cfg->align       = DMA_AUDIO_TDM_ALIGN;
    cfg->fs_edge     = DMA_AUDIO_TDM_FS_EDGE;
    cfg->fs_cycles   = DMA_AUDIO_TDM_FS_CYCLES;
    cfg->irq_mode    = true;
    cfg->data_ptr    = NULL;
    cfg->data_size   = PLAY_BUFF_SIZE;
    TRACE(1, "[PLAY_CFG]: rate=%d, bits=%d, chan_num=%d, chan_map=%d, " \
            "device=%d, data_size=%d, vol=%d",
        cfg->sample_rate, cfg->bits, cfg->channel_num, cfg->channel_map,
        cfg->device, cfg->data_size, cfg->vol);

    cfg = &dma_aud_cur_cfg[AUD_STREAM_CAPTURE];
    cfg->sample_rate = DMA_AUD_SAMP_RATE;
    cfg->bits        = DMA_AUD_SAMP_BITS;
    cfg->channel_num = DMA_AUD_CHAN_NUM_CAP;
    cfg->channel_map = DMA_AUD_CHAN_MAP_CAP;
    cfg->device      = DMA_AUD_CAPT_DEV;
    cfg->io_path     = AUD_INPUT_PATH_MAINMIC;
    cfg->vol         = 17;
    cfg->align       = DMA_AUDIO_TDM_ALIGN;
    cfg->fs_edge     = DMA_AUDIO_TDM_FS_EDGE;
    cfg->fs_cycles   = DMA_AUDIO_TDM_FS_CYCLES;
    cfg->irq_mode    = true;
    cfg->data_ptr    = NULL;
    cfg->data_size   = CAP_BUFF_SIZE;
    TRACE(1, "[CAP_CFG]: rate=%d, bits=%d, chan_num=%d, chan_map=%d, " \
        "device=%d, data_size=%d, vol=%d",
        cfg->sample_rate, cfg->bits, cfg->channel_num, cfg->channel_map,
        cfg->device, cfg->data_size, cfg->vol);
}

void dma_audio_init(void)
{
    TRACE(1, "%s:", __func__);

    if (dma_aud_inited) {
        return;
    }
#ifdef RTOS
    if (dma_aud_mutex_id == NULL) {
        dma_aud_mutex_id = osMutexCreate(osMutex(dma_aud_mutex));
    }
#endif
    dma_audio_lock();

#if defined(CP_AS_SUBSYS)
    dma_rpcif_setup_send_cmd_handler(cp_subsys_send);
#elif defined(SENSOR_HUB)
    dma_rpcif_setup_send_cmd_handler(sensor_hub_send);
#elif defined(DMA_RPC_SVR_CORE_HIFI)
    dma_rpcif_setup_send_cmd_handler(dsp_hifi_send);
#else
    dma_rpcif_setup_send_cmd_handler(dsp_m55_send);
#endif

    dma_audio_init_stream_cfg();

    cpu_whoami();

    dma_aud_cur_state = false;
    dma_aud_inited = true;

    dma_audio_unlock();
}

bool dma_audio_started(void)
{
    return dma_aud_cur_state;
}

void dma_audio_setup_pre_state_switch_callback(enum DMA_AUD_CB_ID_T id, DMA_AUD_STATE_CB_T callback)
{
    ASSERT_DMA_AUD_CB_ID(id);
    pre_state_switch_cb[id] = callback;
}

void dma_audio_setup_post_state_switch_callback(enum DMA_AUD_CB_ID_T id, DMA_AUD_STATE_CB_T callback)
{
    ASSERT_DMA_AUD_CB_ID(id);
    post_state_switch_cb[id] = callback;
}

static void dma_audio_run_pre_cb(bool state)
{
    uint32_t i;

    for (i = 0; i < DMA_AUD_CB_ID_QTY; i++) {
        if (pre_state_switch_cb[i]) {
            pre_state_switch_cb[i](state);
        }
    }
}

static void dma_audio_run_post_cb(bool state)
{
    uint32_t i;

    for (i = 0; i < DMA_AUD_CB_ID_QTY; i++) {
        if (post_state_switch_cb[i]) {
            post_state_switch_cb[i](state);
        }
    }
}

void dma_audio_request_clock(void)
{
    if (!dma_aud_req_clock) {
#ifdef NOAPP
        hal_sysfreq_req(DAUD_HOST_FREQ_USER, DAUD_HOST_FREQ_CLOCK);
#else
        app_sysfreq_req(DAUD_HOST_FREQ_USER, (enum APP_SYSFREQ_FREQ_T)DAUD_HOST_FREQ_CLOCK);
#endif
        dma_aud_req_clock = true;
    }
}

void dma_audio_release_clock(void)
{
    if (dma_aud_req_clock) {
#ifdef NOAPP
        hal_sysfreq_req(DAUD_HOST_FREQ_USER, HAL_CMU_FREQ_32K);
#else
        app_sysfreq_req(DAUD_HOST_FREQ_USER, APP_SYSFREQ_32K);
#endif
        dma_aud_req_clock = false;
    }
}

static bool is_tdm_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool tdm_device = false;

    if ((device == AUD_STREAM_USE_TDM0_SLAVE)
        || (device == AUD_STREAM_USE_TDM1_SLAVE)
        || (device == AUD_STREAM_USE_TDM0_MASTER)
        || (device == AUD_STREAM_USE_TDM1_MASTER)) {
        tdm_device = true;
    }
    return tdm_device;
}

static bool is_i2s_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool i2s_device = false;

    if ((device == AUD_STREAM_USE_I2S0_SLAVE)
        || (device == AUD_STREAM_USE_I2S1_SLAVE)
        || (device == AUD_STREAM_USE_I2S0_MASTER)
        || (device == AUD_STREAM_USE_I2S1_MASTER)) {
        i2s_device = true;
    }
    return i2s_device;
}

static bool is_i2s_tdm_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool i2s_tdm_device = false;

    if (is_i2s_device(device) || is_tdm_device(device)) {
        i2s_tdm_device = true;
    }
    return i2s_tdm_device;
}

static bool is_codec_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool codec_device = false;

    if ((device == AUD_STREAM_USE_INT_CODEC)
        || (device == AUD_STREAM_USE_INT_CODEC2)
        || (device == AUD_STREAM_USE_INT_CODEC3)) {
        codec_device = true;
    }
    return codec_device;
}

void hal_codec_trigger_by_ws_enable(void);
void hal_codec_trigger_by_ws_disable(void);
void hal_codec_timer_set_ws_trigger_cnt(uint32_t cnt);
void hal_codec_trigger_delay_cnt_by_ws(uint32_t cnt);
void hal_codec_set_event_trigger_callback(HAL_CODEC_EVENT_TRIGGER_CALLBACK callback);
int hal_codec_timer_trig_i2s_enable(enum HAL_CODEC_TIMER_TRIG_MODE_T mode, uint32_t ticks, bool periodic);
int hal_codec_timer_trig_i2s_disable(void);
void hal_i2s_ws_codec_enable(enum HAL_I2S_ID_T id);
void hal_i2s_ws_codec_disable(enum HAL_I2S_ID_T id);

WEAK void analog_aud_pll_tune_quiet(bool quiet) {}

#ifdef DMA_AUDIO_USE_TDM_I2S
#ifndef DMA_AUDIO_NO_CODEC_TIMER_IRQ
static void i2s_codec_sync_timer_callback(void)
{
    static uint32_t old_time = 0, old_codec_time = 0;
    static uint32_t irq_cnt = 0;

    POSSIBLY_UNUSED uint32_t time, codec_time, diff_time;

    irq_cnt++;

    time = hal_fast_sys_timer_get();
    if (old_time == 0) {
        old_time = time;
    }

    codec_time = hal_codec_timer_get();
    diff_time = codec_time - old_codec_time;
    old_codec_time = codec_time;

#if defined(DAUD_HOST_DBG_SYNC_VERBOSE) && (DAUD_HOST_DBG_SYNC_VERBOSE > 0)
    if ((irq_cnt % 500) == 0) {
        TRACE(0, "[SYNC_TIMER]: cnt=%d, period=%d, codec_time=%d, diff_time=%d",
            irq_cnt, FAST_TICKS_TO_US(time-old_time),codec_time,diff_time);
    }
#elif defined(DAUD_HOST_DBG_SYNC_VERBOSE) && (DAUD_HOST_DBG_SYNC_VERBOSE > 1)
    TRACE(0, "[SYNC_TIMER]: cnt=%d, period=%d, codec_time=%d, diff_time=%d",
        irq_cnt, FAST_TICKS_TO_US(time-old_time),codec_time,diff_time);
#endif

    old_time = time;
}
#endif

static uint32_t calc_tdm_ws_trigger_cnt(struct AF_STREAM_CONFIG_T *cfg)
{
    uint32_t trig_cnt = WS_TRIGGER_CNT;

    return trig_cnt;
}

enum HAL_I2S_ID_T i2s_device_to_id(enum AUD_STREAM_USE_DEVICE_T device)
{
    enum HAL_I2S_ID_T id = HAL_I2S_ID_QTY;

    if ((device == AUD_STREAM_USE_TDM0_SLAVE)
        || (device == AUD_STREAM_USE_I2S0_SLAVE)
        || (device == AUD_STREAM_USE_TDM0_MASTER)
        || (device == AUD_STREAM_USE_I2S0_MASTER)) {
        id = HAL_I2S_ID_0;
    } else if (device == AUD_STREAM_USE_TDM1_SLAVE
        || device == AUD_STREAM_USE_I2S1_SLAVE
        || device == AUD_STREAM_USE_TDM1_MASTER
        || device == AUD_STREAM_USE_I2S1_MASTER) {
        id = HAL_I2S_ID_1;
    } else {
        ASSERT(false, "[%s]: Bad device=%d", __func__, device);
    }
    return id;
}
#endif

static int dma_audio_i2s_trig_codec_enable(enum AUD_STREAM_T stream,
    struct AF_STREAM_CONFIG_T *cfg, bool enable)
{
#ifdef DMA_AUDIO_USE_TDM_I2S
    enum HAL_I2S_ID_T i2s_id = HAL_I2S_ID_0;
    enum HAL_I2S_ID_T i2s_id_out = HAL_I2S_ID_1;
    enum AUD_STREAM_T codec_stream = AUD_STREAM_PLAYBACK;
    enum AUD_STREAM_USE_DEVICE_T cap_device, play_device, codec_device;
    bool i2s_in_codec_out = false;
    bool codec_in_i2s_out = false;
    bool i2s_in_i2s_out   = false;

    codec_device = AUD_STREAM_USE_DEVICE_NULL;
    play_device = dma_aud_sync_device[AUD_STREAM_PLAYBACK];
    cap_device = dma_aud_sync_device[AUD_STREAM_CAPTURE];

    TRACE(1, "[%s]: stream=%d, enable=%d", __func__, stream, enable);

    if (is_codec_device(play_device) && is_i2s_tdm_device(cap_device)) {
        /* I2S CAPTURE, CODEC PLAYBACK */
        codec_stream = AUD_STREAM_PLAYBACK;
        codec_device = play_device;
        i2s_id = i2s_device_to_id(cap_device);
        i2s_in_codec_out = true;
        TRACE(1, "i2s_in_codec_out enabled");
    } else if (is_codec_device(cap_device) && is_i2s_tdm_device(play_device)) {
        /* CODEC CAPTURE, I2S PLAYBACK */
        codec_stream = AUD_STREAM_CAPTURE;
        codec_device = cap_device;
        i2s_id = i2s_device_to_id(play_device);
        codec_in_i2s_out = true;
        TRACE(1, "codec_in_i2s_out enabled");
    } else if (is_i2s_tdm_device(cap_device) && is_i2s_tdm_device(play_device)) {
        /* I2S CAPTURE, I2S PLAYBACK */
        i2s_id = i2s_device_to_id(cap_device);
        i2s_id_out = i2s_device_to_id(play_device);
        i2s_in_i2s_out = true;
        TRACE(1, "i2s_in_i2s_out enabled");
    } else {
        TRACE(1, "WARNING: [%s]: Bad cap_device=%d, play_device=%d", __func__, cap_device, play_device);
        return -1;
    }
    TRACE(1, "flag=[%d %d %d], device=[%d %d %d], i2s_id=[%d %d]",
        i2s_in_codec_out, codec_in_i2s_out, i2s_in_i2s_out,
        cap_device, play_device, codec_device,
        i2s_id, i2s_id_out);

    if (i2s_in_codec_out || codec_in_i2s_out) {
        if (enable) {
            uint32_t ws_trig_cnt = 0;

            if (stream == AUD_STREAM_CAPTURE) {
                ASSERT(cap_device == cfg->device, "[%s]: Bad cap_device=%d, cfg->device=%d",
                    __func__, cap_device, cfg->device);
            }

            // enable BT trigger CODEC DAC/ADC mode
            af_codec_sync_device_config(codec_device, codec_stream, AF_CODEC_SYNC_TYPE_BT, true);

            // select I2S WS trigger CODEC
            hal_codec_trigger_by_ws_enable();

            // enable I2S WS trigger CODEC
            hal_i2s_ws_codec_enable(i2s_id);

            /* setup CODEC TIMER param */
            // set trigger interval
            ws_trig_cnt = calc_tdm_ws_trigger_cnt(cfg);
            TRACE(1, "ws_trig_cnt=%d", ws_trig_cnt);
            hal_codec_timer_set_ws_trigger_cnt(ws_trig_cnt);

            hal_codec_trigger_delay_cnt_by_ws(1);
            // setup trigger callback
#ifndef DMA_AUDIO_NO_CODEC_TIMER_IRQ
            hal_codec_set_event_trigger_callback(i2s_codec_sync_timer_callback);
#endif
            if (codec_stream == AUD_STREAM_PLAYBACK) {
                // enable trigger DAC mode
                hal_codec_timer_trig_i2s_enable(HAL_CODEC_TIMER_TRIG_MODE_DAC, 0, false);
            } else {
                // enable trigger ADC mode
                hal_codec_timer_trig_i2s_enable(HAL_CODEC_TIMER_TRIG_MODE_ADC, 0, false);
            }
        } else {
            hal_codec_timer_trig_i2s_disable();
            hal_codec_set_event_trigger_callback(NULL);
            hal_codec_trigger_by_ws_disable();

            hal_i2s_ws_codec_disable(i2s_id);

            af_codec_sync_device_config(codec_device, codec_stream, AF_CODEC_SYNC_TYPE_BT, false);
        }
#ifdef I2S_SW_SYNC
    } else if (i2s_in_i2s_out) {
        if (enable) {
            analog_aud_pll_tune_quiet(true);
            dma_rpc_setup_tune_clock_handler(analog_aud_pll_tune);
            hal_i2s_enable_delay(i2s_id_out);
        } else {
            hal_i2s_disable_delay(i2s_id_out);
            dma_rpc_setup_tune_clock_handler(NULL);
            analog_aud_pll_tune_quiet(false);
        }
#endif
    }
#endif
    return 0;
}

static int dma_audio_device_sync_open(enum AUD_STREAM_T stream, struct AF_STREAM_CONFIG_T *cfg)
{
    enum AUD_STREAM_USE_DEVICE_T device = cfg->device;

    dma_aud_sync_device[stream] = device;

    if (stream == AUD_STREAM_PLAYBACK) {
        if (is_codec_device(device)) {
            if (device == AUD_STREAM_USE_INT_CODEC) {
                hal_codec_sw_sync_play_open();
            } else if (device == AUD_STREAM_USE_INT_CODEC2) {
                hal_codec_sw_sync_play2_open();
#ifdef CHIP_SUBSYS_BTH
            } else if (device == AUD_STREAM_USE_INT_CODEC3) {
                hal_codec_sw_sync_play3_open();
#endif
            } else {
                ASSERT(false, "[%s][%d]: Bad playback codec device=%d",
                    __func__, __LINE__, device);
            }
        } else if (is_i2s_tdm_device(device)) {
            /* I2S playback stream to trigger CODEC ADC */
            dma_audio_i2s_trig_codec_enable(stream, cfg, true);
        } else {
            ASSERT(false, "[%s]: Bad playback device=%d", __func__, device);
        }
        TRACE(1, "[%s]: playback device(%d)", __func__, device);
    } else {
        if (is_codec_device(device)) {
            if (device == AUD_STREAM_USE_INT_CODEC) {
                hal_codec_sw_sync_cap_open();
#ifdef CHIP_SUBSYS_BTH
            } else if (device == AUD_STREAM_USE_INT_CODEC2) {
                hal_codec_sw_sync_cap2_open();
#endif
            } else {
                ASSERT(false, "[%s][%d]: Bad capture codec device=%d",
                    __func__, __LINE__, device);
            }
        } else if (is_i2s_tdm_device(device)) {
            /* I2S capture stream to trigger CODEC DAC */
            dma_audio_i2s_trig_codec_enable(stream, cfg, true);
        } else {
            ASSERT(false, "[%s]: Bad capture device=%d", __func__, device);
        }
        TRACE(1, "[%s]: capture device(%d)", __func__, device);
    }
    return 0;
}

static int dma_audio_device_sync_start(void)
{
    enum AUD_STREAM_USE_DEVICE_T play_device, cap_device;

    play_device = dma_aud_sync_device[AUD_STREAM_PLAYBACK];
    cap_device = dma_aud_sync_device[AUD_STREAM_CAPTURE];

    TRACE(1, "[%s]:play_device=%d, cap_device=%d", __func__, play_device, cap_device);

    if (is_codec_device(play_device) && is_codec_device(cap_device)) {
        // CODEC ADC capture --> CODEC DAC play
        hal_codec_sw_sync_low_latency_enable(0);
    }
    else if (is_codec_device(play_device) && is_i2s_tdm_device(cap_device)) {
        // I2S recv --> CODEC DAC play
        if (play_device == AUD_STREAM_USE_INT_CODEC) {
            hal_codec_sw_sync_play_enable();
        } else if (play_device == AUD_STREAM_USE_INT_CODEC2) {
            hal_codec_sw_sync_play2_enable();
#ifdef CHIP_SUBSYS_BTH
        } else if (play_device == AUD_STREAM_USE_INT_CODEC3) {
            hal_codec_sw_sync_play3_enable();
#endif
        } else {
            ASSERT(false, "[%s]: Bad play_device=%d", __func__, play_device);
        }
    }
    else if (is_i2s_tdm_device(play_device) && is_codec_device(cap_device)) {
        // CODEC ADC capture  --> I2S send
        if (cap_device == AUD_STREAM_USE_INT_CODEC) {
            hal_codec_sw_sync_cap_enable();
#ifdef CHIP_SUBSYS_BTH
        } else if (cap_device == AUD_STREAM_USE_INT_CODEC2) {
            hal_codec_sw_sync_cap2_enable();
#endif
        } else {
            ASSERT(false, "[%s]: Bad cap_device=%d", __func__, cap_device);
        }
    }
    else if (is_i2s_tdm_device(play_device) && is_i2s_tdm_device(cap_device)) {
    }
    else {
        ASSERT(false ,"[%s]: Bad cap_device=%d, play_device=%d", __func__, cap_device, play_device);
    }
    return 0;
}

static int dma_audio_device_sync_stop(void)
{
    TRACE(1, "[%s]:", __func__);
    return 0;
}

static int dma_audio_device_sync_close(enum AUD_STREAM_T stream)
{
    enum AUD_STREAM_USE_DEVICE_T device = dma_aud_sync_device[stream];

    if (stream == AUD_STREAM_PLAYBACK) {
        if (is_codec_device(device)) {
            if (device == AUD_STREAM_USE_INT_CODEC) {
                hal_codec_sw_sync_play_close();
            } else if (device == AUD_STREAM_USE_INT_CODEC2) {
                hal_codec_sw_sync_play2_close();
#ifdef CHIP_SUBSYS_BTH
            } else if (device == AUD_STREAM_USE_INT_CODEC3) {
                hal_codec_sw_sync_play3_close();
#endif
            } else {
                ASSERT(false, "[%s][%d]: Bad playback codec device=%d",
                    __func__, __LINE__, device);
            }
        } else if (is_i2s_tdm_device(device)) {
            dma_audio_i2s_trig_codec_enable(stream, NULL, false);
        } else {
            ASSERT(false, "[%s]: Bad playback device=%d", __func__, device);
        }
        TRACE(1, "[%s]: playback device(%d)", __func__, device);
    } else {
        if (is_codec_device(device)) {
            if (device == AUD_STREAM_USE_INT_CODEC) {
                hal_codec_sw_sync_cap_close();
#ifdef CHIP_SUBSYS_BTH
            } else if (device == AUD_STREAM_USE_INT_CODEC2) {
                hal_codec_sw_sync_cap2_close();
#endif
            } else {
                ASSERT(false, "[%s][%d]: Bad capture codec device=%d",
                    __func__, __LINE__, device);
            }
        } else if (is_i2s_tdm_device(device)) {
            dma_audio_i2s_trig_codec_enable(stream, NULL, false);
        } else {
            ASSERT(false, "[%s]: Bad capture device=%d", __func__, device);
        }
        TRACE(1, "[%s]: capture device(%d)", __func__, device);
    }
    return 0;
}

#ifdef DMA_AUDIO_GPIO_TRIG_CODEC
static const struct HAL_IOMUX_PIN_FUNCTION_MAP codec_trig_iomux[] = {
    {HAL_IOMUX_PIN_P2_5, HAL_IOMUX_FUNC_GPIO,HAL_IOMUX_PIN_VOLTAGE_MEM,HAL_IOMUX_PIN_NOPULL},
};

void hal_codec_hw_sync_dac1_uh_disable(void);
void hal_codec_hw_sync_dac1_uh_enable(enum HAL_CODEC_SYNC_TYPE_T type);

void dma_audio_gpio_trigger_codec_open(void)
{
    hal_iomux_init(codec_trig_iomux, ARRAY_SIZE(codec_trig_iomux));
    hal_gpio_pin_set_dir(codec_trig_iomux[0].pin, HAL_GPIO_DIR_OUT, 1);
    hal_iomux_set_codec_gpio_trigger(codec_trig_iomux[0].pin, false);

    hal_codec_hw_sync_dac1_uh_enable(HAL_CODEC_SYNC_TYPE_GPIO);
}

void dma_audio_gpio_trigger_codec_close(void)
{
    hal_codec_hw_sync_dac1_uh_disable();
}

void dma_audio_gpio_trigger_codec_start(void)
{
    hal_gpio_pin_set(codec_trig_iomux[0].pin);
    hal_sys_timer_delay_us(5);
    hal_gpio_pin_clr(codec_trig_iomux[0].pin);
}
#endif /* DMA_AUDIO_GPIO_TRIG_CODEC */


#ifdef AUDIO_OUTPUT_MULTI_DAC_SW_MERGE
#define CODEC_REG_READ(addr)          (*(volatile uint32_t *)(addr))
#define CODEC_REG_WRITE(addr, val)    ((*(volatile uint32_t *)(addr)) = (val))

void dma_audio_echo_path_enable(bool enable)
{
    uint32_t addr, val;
    struct HAL_CODEC_ECHO_CHAN_CONFIG_T ec_cfg[2];

    memset(&ec_cfg[0], 0, sizeof(ec_cfg[0]));
    memset(&ec_cfg[1], 0, sizeof(ec_cfg[1]));

    TRACE(1, "[%s]: enable = %d", __func__, enable);

    if (enable) {
        ec_cfg[0].chan = AUD_CHANNEL_MAP_CH0;
        ec_cfg[0].rate = HAL_CODEC_ECHO_RATE_384K;
        ec_cfg[0].data = HAL_CODEC_ECHO_CHAN_DAC_SND_DATA;
        ec_cfg[0].enable = true;

        ec_cfg[1].chan = AUD_CHANNEL_MAP_CH1;
        ec_cfg[1].rate = HAL_CODEC_ECHO_RATE_384K;
        ec_cfg[1].data = HAL_CODEC_ECHO_CHAN_DAC_TRI_DATA;
        ec_cfg[1].enable = true;

        hal_codec_echo2_chan_config(&ec_cfg[0]);
        hal_codec_echo2_chan_config(&ec_cfg[1]);

        hal_codec_set_echo_ch(HAL_CODEC_ECHO_CHAN_DAC_TRI_DATA);
        hal_codec_set_echo_ch1(HAL_CODEC_ECHO_CHAN_DAC_TRI_DATA);

        addr = 0x403000AC;
        val = CODEC_REG_READ(addr);
        TRACE(1, "CODEC_REG: [%x] = %x", addr, val);
        addr = 0x40300780;
        val = CODEC_REG_READ(addr);
        TRACE(1, "CODEC_REG: [%x] = %x", addr, val);

    } else {

        ec_cfg[0].chan = AUD_CHANNEL_MAP_CH0;
        ec_cfg[0].rate = HAL_CODEC_ECHO_RATE_48K;
        ec_cfg[0].data = HAL_CODEC_ECHO_CHAN_DAC_DATA_OUT_L;
        ec_cfg[0].enable = false;

        ec_cfg[1].chan = AUD_CHANNEL_MAP_CH1;
        ec_cfg[1].rate = HAL_CODEC_ECHO_RATE_48K;
        ec_cfg[1].data = HAL_CODEC_ECHO_CHAN_DAC_DATA_OUT_L;
        ec_cfg[1].enable = false;

        hal_codec_set_echo_ch(HAL_CODEC_ECHO_CHAN_DAC_DATA_OUT_L);
        hal_codec_set_echo_ch1(HAL_CODEC_ECHO_CHAN_DAC_DATA_OUT_R);

        hal_codec_echo2_chan_config(&ec_cfg[0]);
        hal_codec_echo2_chan_config(&ec_cfg[1]);
    }
}
#endif

void dma_audio_rpcif_open(enum DMA_AUD_RPCIF_USER_T user)
{
    int set = 0;

    if (dma_aud_rpcif_user_map == 0) {
        set = 1;
    }
    dma_aud_rpcif_user_map |= (1 << user);

    if (set) {
        dma_rpcif_open(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        dma_rpcif_open(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
    }
}

void dma_audio_rpcif_close(enum DMA_AUD_RPCIF_USER_T user)
{
    int set = 0;

    dma_aud_rpcif_user_map &= ~(1 << user);
    if (dma_aud_rpcif_user_map == 0) {
        set = 1;
    }
    if (set) {
        dma_rpcif_close(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        dma_rpcif_close(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
    }
}

int32_t dma_audio_stream_set_fade_time(uint32_t ms)
{
    g_dma_stream_fade_ms = ms;

    return 0;
}

void dma_audio_on(bool on, struct DAUD_STREAM_CONFIG_T *cap_cfg, struct DAUD_STREAM_CONFIG_T *play_cfg)
{
    int ret = 0;

    ASSERT(dma_aud_inited == true, "[%s]: dma audio not inited", __func__);
    TRACE(1, "%s: on=%d", __func__, on);
    if (dma_aud_cur_state == on) {
        return;
    }

    dma_audio_lock();
    dma_audio_run_pre_cb(on);
    if (on) {
        struct AF_STREAM_CONFIG_T stream_cfg;
        struct DAUD_STREAM_CONFIG_T daud_cfg;

        dma_audio_request_clock();

        // config playback stream
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        memset(&daud_cfg, 0, sizeof(daud_cfg));

        dma_audio_rpcif_open(DMA_AUD_RPCIF_USER_APP);

        /*
         * Set stream cfg to SYS system.
         * The Main MCU do not need to allocated any buffer for dma audio because of the
         * buffer is always be allocated by SYS automatically.
         */
        if (play_cfg) {
            play_cfg->handler = NULL;
            play_cfg->data_ptr = NULL;
            dma_aud_cur_cfg[AUD_STREAM_PLAYBACK] = *play_cfg;
        }
        dma_rpc_set_stream_cfg(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, (void *)&dma_aud_cur_cfg[AUD_STREAM_PLAYBACK]);

        if (cap_cfg) {
            cap_cfg->handler = NULL;
            cap_cfg->data_ptr = NULL;
            dma_aud_cur_cfg[AUD_STREAM_CAPTURE] = *cap_cfg;
        }
        dma_rpc_set_stream_cfg(DAUD_STREAM_ID, AUD_STREAM_CAPTURE, (void *)&dma_aud_cur_cfg[AUD_STREAM_CAPTURE]);

        /*
         * Get new stream cfg from SYS system.
         * The data_ptr & data_size has been updated by SYS sub-system.
         */
        dma_rpc_get_stream_cfg(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, (void *)&daud_cfg);

        stream_cfg.sample_rate = daud_cfg.sample_rate;
        stream_cfg.bits        = daud_cfg.bits;
        stream_cfg.channel_num = daud_cfg.channel_num;
        stream_cfg.channel_map = daud_cfg.channel_map;
        stream_cfg.device      = daud_cfg.device;
        stream_cfg.io_path     = daud_cfg.io_path;
        stream_cfg.vol         = daud_cfg.vol;
        stream_cfg.handler     = daud_cfg.handler;
        stream_cfg.data_ptr    = daud_cfg.data_ptr;
        stream_cfg.data_size   = daud_cfg.data_size;
        stream_cfg.sync_start  = true;
        if (is_tdm_device(stream_cfg.device)) {
            stream_cfg.sync_start  = daud_cfg.sync_start;
            stream_cfg.slot_cycles = daud_cfg.slot_cycles;
            stream_cfg.align       = daud_cfg.align;
            stream_cfg.fs_edge     = daud_cfg.fs_edge;
            stream_cfg.fs_cycles   = daud_cfg.fs_cycles;
        } else if (is_i2s_device(stream_cfg.device)) {
            stream_cfg.sync_start  = daud_cfg.sync_start;
            stream_cfg.slot_cycles = daud_cfg.slot_cycles;
        }

        TRACE(1, "[dma_audio]: playback: rate=%d,bits=%d data_ptr=%x, data_size=%d",
            stream_cfg.sample_rate, stream_cfg.bits,
            (uint32_t)stream_cfg.data_ptr, stream_cfg.data_size);

        ret = af_stream_open(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, &stream_cfg);
        ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);

        ret = dma_audio_device_sync_open(AUD_STREAM_PLAYBACK, &stream_cfg);
        ASSERT(ret == 0, "sync_open playback failed: %d", ret);

        // config capture stream
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        memset(&daud_cfg, 0, sizeof(daud_cfg));

        dma_rpc_get_stream_cfg(DAUD_STREAM_ID, AUD_STREAM_CAPTURE, (void *)&daud_cfg);

        stream_cfg.sample_rate = daud_cfg.sample_rate;
        stream_cfg.bits        = daud_cfg.bits;
        stream_cfg.channel_num = daud_cfg.channel_num;
        stream_cfg.channel_map = daud_cfg.channel_map;
        stream_cfg.device      = daud_cfg.device;
        stream_cfg.io_path     = daud_cfg.io_path;
        stream_cfg.vol         = daud_cfg.vol;
        stream_cfg.handler     = daud_cfg.handler;
        stream_cfg.data_ptr    = daud_cfg.data_ptr;
        stream_cfg.data_size   = daud_cfg.data_size;
        stream_cfg.sync_start  = true;
        if (is_tdm_device(stream_cfg.device)) {
            stream_cfg.sync_start  = daud_cfg.sync_start;
            stream_cfg.slot_cycles = daud_cfg.slot_cycles;
            stream_cfg.align       = daud_cfg.align;
            stream_cfg.fs_edge     = daud_cfg.fs_edge;
            stream_cfg.fs_cycles   = daud_cfg.fs_cycles;
        } else if (is_i2s_device(stream_cfg.device)) {
            stream_cfg.sync_start  = daud_cfg.sync_start;
            stream_cfg.slot_cycles = daud_cfg.slot_cycles;
        }

        TRACE(1, "[dma_audio]: capture: rate=%d,bits=%d data_ptr=%x, data_size=%d",
            stream_cfg.sample_rate, stream_cfg.bits,
            (uint32_t)stream_cfg.data_ptr, stream_cfg.data_size);

        TRACE(1, "[dma_audio]: capture: ch_num=%d,ch_map=%d device=%x, io_path=%d",
            stream_cfg.channel_num, stream_cfg.channel_map,
            (uint32_t)stream_cfg.device, stream_cfg.io_path);

        ret = af_stream_open(DAUD_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
        ASSERT(ret == 0, "af_stream_open capture failed: %d", ret);

#ifdef AUDIO_OUTPUT_MULTI_DAC_SW_MERGE
        hal_codec_set_align_mode_status(CODEC_ALIGN_USER_PSAP, CODEC_ALIGN_MODE_PSAP, true);
        dma_audio_echo_path_enable(true);
#endif

#ifdef DMA_AUDIO_GPIO_TRIG_CODEC
        dma_audio_gpio_trigger_codec_open();
#endif
        ret = dma_audio_device_sync_open(AUD_STREAM_CAPTURE, &stream_cfg);
        ASSERT(ret == 0, "sync_open capture failed: %d", ret);

        ret = af_stream_start(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        ASSERT(ret == 0, "af_stream_start playback failed: %d", ret);

        ret = af_stream_start(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        ASSERT(ret == 0, "af_stream_start capture failed: %d", ret);
        TRACE(1, "====> dma audio on");

        dma_audio_device_sync_start();

#ifdef DMA_AUDIO_GPIO_TRIG_CODEC
        dma_audio_gpio_trigger_codec_start();
#endif
        if (g_dma_stream_fade_ms) {
            dma_rpc_stream_fadein(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, g_dma_stream_fade_ms);
        }
    } else {
        if (g_dma_stream_fade_ms) {
            dma_rpc_stream_fadeout(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK, g_dma_stream_fade_ms);
            //Reserve 10ms to avoid incomplete fadeout
            osDelay(g_dma_stream_fade_ms + 10);
        }

        af_stream_stop(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        af_stream_stop(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);

        dma_audio_device_sync_stop();

        dma_audio_device_sync_close(AUD_STREAM_CAPTURE);
        dma_audio_device_sync_close(AUD_STREAM_PLAYBACK);

#ifdef AUDIO_OUTPUT_MULTI_DAC_SW_MERGE
        dma_audio_echo_path_enable(false);
        hal_codec_set_align_mode_status(CODEC_ALIGN_USER_PSAP, CODEC_ALIGN_MODE_PSAP, false);
#endif

#ifdef DMA_AUDIO_GPIO_TRIG_CODEC
        dma_audio_gpio_trigger_codec_close();
#endif

        af_stream_close(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        af_stream_close(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);

        dma_audio_rpcif_close(DMA_AUD_RPCIF_USER_APP);

        dma_audio_release_clock();
        TRACE(1, "====> dma audio off");
    }
    dma_aud_cur_state = on;
    dma_audio_run_post_cb(on);
    dma_audio_unlock();
}
#endif /* DMA_AUDIO_APP */

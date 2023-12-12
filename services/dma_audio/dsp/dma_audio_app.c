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
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hal_cmu.h"
#include "dma_audio.h"
#include "dma_audio_app.h"
#include "dma_audio_stream.h"
#include "dma_audio_stream_conf.h"
#ifdef DMA_AUDIO_ALGO
#include "dma_audio_algo.h"
#endif
#ifdef AUDIO_DOWN_MIXER
#include "audio_down_mixer_dsp.h"
#endif
#ifdef PSAP_SW_APP
#include "psap_sw_dsp.h"
#endif
#include "audioflinger.h"
#include "dma_audio_stream_conf.h"

static volatile bool app_inited = false;
static volatile bool app_cur_state = false;
static volatile bool app_next_state = false;

#ifdef RTOS
static POSSIBLY_UNUSED osThreadId main_tid = NULL;
#endif

void dma_audio_set_i2s_trig_dac_clear_flag(bool need_clear);

void dma_audio_i2s_sw_trig_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    enum AUD_STREAM_USE_DEVICE_T device);

void dma_audio_i2s_sw_trig_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    enum AUD_STREAM_USE_DEVICE_T device);

void cpu_whoami(void)
{
    POSSIBLY_UNUSED uint32_t cpuid = get_cpu_id();
    DAUD_TRACE(1, "CPUID:%d, CORE:%s", cpuid, cpuid==0?"M55":"M33");
}

static POSSIBLY_UNUSED void app_dma_irq_handler(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    //DAUD_TRACE(1, "[DMA IRQ CLALBACK]: id=%d, stream=%d", id, stream);
//    dma_audio_stream_irq_notify(id, stream);
}

static POSSIBLY_UNUSED void app_event_handler(enum DAUD_EVT_T evt, uint32_t p0, uint32_t p1, uint32_t p2)
{
    DAUD_TRACE(1, "[app_event_handler]:evt=%d,param=[0x%x/0x%x/0x%x]", evt, p0, p1, p2);

#ifndef CHIP_ROLE_CP
        if ((evt == DAUD_EVT_STREAM_OPEN)
            || (evt == DAUD_EVT_DMA_GET_CHAN)
            || (evt == DAUD_EVT_STREAM_SET_CFG)
            || (evt == DAUD_EVT_STREAM_GET_CFG)) {
            dma_audio_stream_request_clock();
        } else if (evt == DAUD_EVT_STREAM_CLOSE
            || (evt == DAUD_EVT_DMA_STOP)) {
            dma_audio_stream_release_clock();
        }
#endif

    if (IS_DAUD_DMA_EVT(evt)) {
        switch (evt) {
        case DAUD_EVT_DMA_INIT:
            break;
        case DAUD_EVT_DMA_GET_CHAN:
            break;
        case DAUD_EVT_DMA_START:
            break;
        case DAUD_EVT_DMA_STOP:
            break;
        default:
            break;
        }
    }
    else if (IS_DAUD_STREAM_EVT(evt)) {
        enum AUD_STREAM_ID_T id  = (enum AUD_STREAM_ID_T)p0;
        enum AUD_STREAM_T stream = (enum AUD_STREAM_T)(p1 & 0x1);
        uint32_t stream_flag = p1 & (~0x1);
        bool flag_forward = !!(stream_flag & DAUD_STREAM_FLAG_FORWARD);
        struct AF_STREAM_CONFIG_T *stream_cfg = (struct AF_STREAM_CONFIG_T *)p2;

        switch (evt) {
        case DAUD_EVT_STREAM_OPEN:
#ifdef DMA_AUDIO_APP_DYN_ON
            dma_audio_stream_on(true);
#endif
            if (flag_forward) {
                struct DAUD_STREAM_CONFIG_T daud_cfg;
                daud_cfg.sample_rate = stream_cfg->sample_rate;
                daud_cfg.bits        = stream_cfg->bits;
                daud_cfg.channel_num = stream_cfg->channel_num;
                daud_cfg.channel_map = stream_cfg->channel_map;
                daud_cfg.device      = stream_cfg->device;
                daud_cfg.io_path     = stream_cfg->io_path;
                daud_cfg.vol         = stream_cfg->vol;
                daud_cfg.irq_mode    = false;
                daud_cfg.data_ptr    = stream_cfg->data_ptr;
                daud_cfg.data_size   = stream_cfg->data_size;
                daud_cfg.handler     = NULL;

                // TODO: remove this woraround for SCO mode
                if ((stream_cfg->channel_map == 0) && (stream_cfg->channel_num > 0)) {
                    if (stream_cfg->channel_num == 1) {
                        daud_cfg.channel_map = DMA_AUDIO_DEFAULT_ONE_CHAN_MAP;
                    } else if (stream_cfg->channel_num == 2) {
                        daud_cfg.channel_map = DMA_AUDIO_DEFAULT_TWO_CHAN_MAP;
                    }
                    TRACE(1, "[DAUD_EVT_STREAM_OPEN]: WARNING: reset channel_map=%x",daud_cfg.channel_map);
                }
                daud_forward_stream_open(id, stream, &daud_cfg);
            }
            break;
        case DAUD_EVT_STREAM_START:
        {
            enum AUD_STREAM_USE_DEVICE_T play_device, cap_device;
            bool i2s_trig_codec = false;
            bool i2s_sw_trig = false;

            cap_device = daud_stream_get_device(id, AUD_STREAM_CAPTURE);
            play_device = daud_stream_get_device(id, AUD_STREAM_PLAYBACK);

            if (daud_stream_use_codec_device(play_device)
                && daud_stream_use_i2s_tdm_device(cap_device)) {
                i2s_trig_codec = true; //I2S RX --> DAC PLAY --> SPK
            } else if (daud_stream_use_codec_device(cap_device)
                && daud_stream_use_i2s_tdm_device(play_device)) {
                i2s_trig_codec = true; //ADC CAP --> I2S TX --> PA
            } else if (daud_stream_use_i2s_tdm_device(cap_device)
                && daud_stream_use_i2s_tdm_device(play_device)) {
                i2s_sw_trig = true; //I2S RX --> I2S TX --> PA
            }
            if (i2s_sw_trig && (stream == AUD_STREAM_PLAYBACK)) {
                // playback stream is started with sw trigger mode
                dma_audio_i2s_sw_trig_open(id, stream, play_device);
            }
            if (i2s_trig_codec && (stream == AUD_STREAM_CAPTURE)) {
                // we set the flag when capture stream started
                dma_audio_set_i2s_trig_dac_clear_flag(true);
            }
            if (flag_forward) {
                daud_forward_stream_start(id, stream);
            }
        }
            break;
        case DAUD_EVT_STREAM_STOP:
        {
            enum AUD_STREAM_USE_DEVICE_T play_device, cap_device;
            bool i2s_trig_codec = false;
            bool i2s_sw_trig = false;

            cap_device = daud_stream_get_device(id, AUD_STREAM_CAPTURE);
            play_device = daud_stream_get_device(id, AUD_STREAM_PLAYBACK);

            if (daud_stream_use_codec_device(play_device)
                && daud_stream_use_i2s_tdm_device(cap_device)) {
                i2s_trig_codec = true; //I2S RX --> DAC PLAY --> SPK
            } else if (daud_stream_use_codec_device(cap_device)
                && daud_stream_use_i2s_tdm_device(play_device)) {
                i2s_trig_codec = true; //ADC CAP --> I2S TX --> PA
            } else if (daud_stream_use_i2s_tdm_device(cap_device)
                && daud_stream_use_i2s_tdm_device(play_device)) {
                i2s_sw_trig = true; //I2S RX --> I2S TX --> PA
            }
            if (i2s_sw_trig && (stream == AUD_STREAM_PLAYBACK)) {
                dma_audio_i2s_sw_trig_close(id, AUD_STREAM_PLAYBACK, play_device);
            }
            if (i2s_trig_codec && (stream == AUD_STREAM_CAPTURE)) {
                dma_audio_set_i2s_trig_dac_clear_flag(false);
            }
            if (flag_forward) {
                daud_forward_stream_stop(id, stream);
            }
        }
            break;
        case DAUD_EVT_STREAM_CLOSE:
            if (flag_forward) {
                daud_forward_stream_close(id, stream);
            }
#ifdef DMA_AUDIO_APP_DYN_ON
            if ((daud_client_stream_is_opened(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK) == false) &&
                (daud_client_stream_is_opened(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK) == false)) {
                dma_audio_stream_on(false);
            }
#endif
            break;
        case DAUD_EVT_STREAM_GET_CFG:
        case DAUD_EVT_STREAM_SET_CFG:
        case DAUD_EVT_STREAM_GET_ALGO_CFG:
        case DAUD_EVT_STREAM_SET_ALGO_CFG:
            break;
        default:
            break;
        }
    }
}

static POSSIBLY_UNUSED uint32_t app_mic_data_handler(uint8_t *buf, uint32_t len)
{
    uint32_t ret = 0;

    return ret;
}

void dma_audio_app_init(bool init)
{
    DAUD_TRACE(1, "%s:", __func__);

    if (init == app_inited) {
        return;
    }

    cpu_whoami();

    if (init) {
        daud_open();
        daud_set_dma_irq_notification(app_dma_irq_handler);
        daud_set_event_handler(DAUD_EVT_HDLR_ID_STREAM, app_event_handler);
        daud_set_event_handler(DAUD_EVT_HDLR_ID_DMA, app_event_handler);

        dma_audio_stream_init();
        dma_audio_stream_setup_mic_data_handler(app_mic_data_handler);

#ifdef PSAP_SW_APP
        psap_sw_dsp_register_cb(true);
#endif

#ifdef AUDIO_DOWN_MIXER
        audio_down_mixer_dsp_register_cb(true);
#endif

#ifdef DMA_AUDIO_APP_DYN_ON
        dma_rpcif_open(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        dma_rpcif_open(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
#endif

#ifdef CHIP_ROLE_CP
        dma_audio_stream_request_clock();
#endif

#ifdef SLEEP_TEST
        hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);
        hal_sysfreq_print_user_freq();
#endif
#ifdef RTOS
        if (main_tid == NULL) {
            main_tid = osThreadGetId();
            DAUD_TRACE(1, "main_tid=%x", (int)main_tid);
        }
#endif
    } else {
#ifdef DMA_AUDIO_APP_DYN_ON
        dma_rpcif_close(DAUD_STREAM_ID, AUD_STREAM_PLAYBACK);
        dma_rpcif_close(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
#endif

#ifdef AUDIO_DOWN_MIXER
        audio_down_mixer_dsp_register_cb(false);
#endif

#ifdef PSAP_SW_APP
        psap_sw_dsp_register_cb(false);
#endif

        dma_audio_stream_setup_mic_data_handler(NULL);
        dma_audio_stream_deinit();

        daud_set_event_handler(DAUD_EVT_HDLR_ID_DMA, NULL);
        daud_set_event_handler(DAUD_EVT_HDLR_ID_STREAM, NULL);
        daud_set_dma_irq_notification(NULL);
        daud_close();
    }
    app_inited = init;
}

static void dma_audio_app_run(void)
{
    bool next_state = app_next_state;

    if (next_state != app_cur_state) {
        ASSERT(app_inited==true, "%s: not inited", __func__);
        if (next_state) {
            dma_audio_stream_on(true);
        } else {
            dma_audio_stream_on(false);
        }
        app_cur_state = next_state;
    }
}

void dma_audio_app_on(bool on)
{
    app_next_state = on;
    dma_audio_app_run();
}

bool dma_audio_app_started(void)
{
    return app_cur_state;
}

void dma_audio_app_thread(void)
{
#ifndef RTOS
    void daud_thread(void const *argument);
    daud_thread(NULL);
#endif
}


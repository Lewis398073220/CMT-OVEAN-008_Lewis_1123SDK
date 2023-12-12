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
#include <stdbool.h>
#include "hal_trace.h"
#include "app_utils.h"
#include "tgt_hardware.h"
#include "hal_codec.h"

#include "psap_sw_app.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#ifdef DMA_RPC_SVR_CORE_M55
#include "mcu_dsp_m55_app.h"
#include "app_dsp_m55.h"
#endif
#ifdef DMA_RPC_SVR_CORE_HIFI
#include "mcu_dsp_hifi4_app.h"
#include "hal_timer.h"
#endif
#include "dma_audio_host.h"

#ifdef CP_AS_SUBSYS
/* CP core*/
#define PSAP_FRAME_MS           (3)

#define PSAP_PLAY_DEV           (AUD_STREAM_USE_INT_CODEC2)
#define PSAP_CAPT_DEV           (AUD_STREAM_USE_INT_CODEC)
#else
/* M55 core and HIFI core*/
#define PSAP_FRAME_MS           (2)

#define PSAP_PLAY_DEV           (AUD_STREAM_USE_INT_CODEC3)
#define PSAP_CAPT_DEV           (AUD_STREAM_USE_INT_CODEC2)
#endif

#define PSAP_SAMP_RATE          (96000)
#define PSAP_SAMP_BITS          (AUD_BITS_24)

/* ANC_TALK_MIC_CH_L | ANC_FF_MIC_CH_L | ANC_FB_MIC_CH_L | ANC_REF_MIC_CH */
#define PSAP_CAPT_CHAN_MAP      (ANC_TALK_MIC_CH_L | ANC_FF_MIC_CH_L)
/* AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 */
#define PSAP_PLAY_CHAN_MAP      (CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV)

#define PSAP_PLAY_VOL           (TGT_VOLUME_LEVEL_MAX)

#if PSAP_SAMP_RATE == 96000 && !defined(CP_AS_SUBSYS)
#define PSAP_SAMP_RATE_REAL     (AUD_SAMPRATE_101562)
#else
#define PSAP_SAMP_RATE_REAL     (PSAP_SAMP_RATE)
#endif

#define AF_PINGPONG             (2)

static bool g_psap_sw_app_status = false;

/* TODO: Need to support it */
void psap_sw_app_fadeout(uint32_t ms)
{
    // dma_rpc_stream_fadeout(DAUD_STREAM_ID, ms);
}

bool psap_sw_app_is_opened(void)
{
    return g_psap_sw_app_status;
}

static void psap_sw_stream_setup(void)
{
    TRACE(1, "[%s] ...", __func__);

#ifdef DMA_AUDIO_APP_DYN_ON
    struct DAUD_STREAM_CONFIG_T capture_cfg, playback_cfg;

    memset((void *)&capture_cfg, 0, sizeof(capture_cfg));
    capture_cfg.irq_mode    = true;
    capture_cfg.sample_rate = PSAP_SAMP_RATE_REAL;
    capture_cfg.bits        = PSAP_SAMP_BITS;
    capture_cfg.channel_map = PSAP_CAPT_CHAN_MAP;
    capture_cfg.channel_num = hal_codec_get_input_map_chan_num(PSAP_CAPT_CHAN_MAP);
    capture_cfg.vol         = 0; // This is invalid param.
    capture_cfg.device      = PSAP_CAPT_DEV;
    capture_cfg.io_path     = AUD_INPUT_PATH_MAINMIC;
    capture_cfg.data_ptr    = NULL;
    capture_cfg.data_size   = (uint32_t)(AF_PINGPONG *
                                ((capture_cfg.bits + 8) / 16) * 2 *
                                PSAP_SAMP_RATE * PSAP_FRAME_MS / 1000 *
                                capture_cfg.channel_num);

    memset((void *)&playback_cfg, 0, sizeof(playback_cfg));
    playback_cfg.irq_mode    = true;
    playback_cfg.sample_rate = PSAP_SAMP_RATE_REAL;
    playback_cfg.bits        = PSAP_SAMP_BITS;
    playback_cfg.channel_map = PSAP_PLAY_CHAN_MAP;
    playback_cfg.channel_num = hal_codec_get_input_map_chan_num(PSAP_PLAY_CHAN_MAP);
    playback_cfg.vol         = PSAP_PLAY_VOL;
    playback_cfg.device      = PSAP_PLAY_DEV;
    playback_cfg.io_path     = AUD_OUTPUT_PATH_SPEAKER;
    playback_cfg.data_ptr    = NULL;
    playback_cfg.data_size   = (uint32_t)(AF_PINGPONG *
                                ((playback_cfg.bits + 8) / 16) * 2 *
                                PSAP_SAMP_RATE * PSAP_FRAME_MS / 1000 *
                                playback_cfg.channel_num);

    dma_audio_on(true, &capture_cfg, &playback_cfg);
#else
    dma_audio_on(true, NULL, NULL);
#endif
}

int32_t psap_sw_app_open(void)
{
    TRACE(2, "[%s] g_psap_sw_app_status: %d", __func__, g_psap_sw_app_status);

#ifdef DMA_RPC_SVR_CORE_M55
    app_dsp_m55_init(APP_DSP_M55_USER_DMA_AUDIO);

    TRACE(1, "[%s] Wait ping finish ...", __func__);
    while(app_dsp_m55_is_running() == false);
    TRACE(1, "[%s] Wait done!", __func__);
    osDelay(100);   // TODO: Need to delete this osDelay
#elif defined(DMA_RPC_SVR_CORE_HIFI)
    app_dsp_hifi4_init(APP_DSP_HIFI4_USER_DMA_AUDIO);
    int32_t wait_cnt = 0;
    while(app_dsp_hifi4_is_running() == false) {
        hal_sys_timer_delay_us(10);
        if (wait_cnt++ > 300000) {    // 3s
            ASSERT(0, "%s: hifi4 is hung", __func__);
        }
    }
#endif

    if (g_psap_sw_app_status == false) {
#ifdef CP_AS_SUBSYS
        app_sysfreq_req(APP_SYSFREQ_USER_PSAP_SW, APP_SYSFREQ_208M);
#endif
        psap_sw_stream_setup();

        g_psap_sw_app_status = true;
    }

    return 0;
}

int32_t psap_sw_app_close(void)
{
    TRACE(2, "[%s] g_psap_sw_app_status: %d", __func__, g_psap_sw_app_status);

    if (g_psap_sw_app_status == true) {
        struct DAUD_STREAM_CONFIG_T *cap_cfg = NULL, *play_cfg = NULL;
        dma_audio_on(false, cap_cfg, play_cfg);
        g_psap_sw_app_status = false;
#ifdef CP_AS_SUBSYS
        app_sysfreq_req(APP_SYSFREQ_USER_PSAP_SW, APP_SYSFREQ_32K);
#endif
    }

#ifdef DMA_RPC_SVR_CORE_M55
    app_dsp_m55_deinit(APP_DSP_M55_USER_DMA_AUDIO);
#elif defined(DMA_RPC_SVR_CORE_HIFI)
    app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_DMA_AUDIO);
#endif

    return 0;
}

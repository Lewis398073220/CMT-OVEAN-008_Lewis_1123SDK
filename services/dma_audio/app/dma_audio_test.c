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
#ifdef DMA_AUDIO_TEST
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "string.h"
#include "hal_key.h"
#include "hal_aud.h"
#include "hal_codec.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "hal_sleep.h"
#include "audioflinger.h"
#include "stream_dma_rpc.h"
#include "dma_audio_def.h"
#include "dma_audio_host.h"
#include "dma_audio_cli.h"
#include "hwtimer_list.h"
#include "signal_generator.h"

#if defined(BTH_AS_MAIN_MCU)
#define LOW_POWER_FREQ HAL_CMU_FREQ_6M
#elif defined(CP_AS_SUBSYS)
#define LOW_POWER_FREQ HAL_CMU_FREQ_6P5M
#else
#define LOW_POWER_FREQ HAL_CMU_FREQ_26M
#endif

#define CAPTURE1_STREAM_ID              AUD_STREAM_ID_0
#define PLAYBACK1_STREAM_ID             AUD_STREAM_ID_0
#define PLAYBACK2_STREAM_ID             AUD_STREAM_ID_1

/* playback1 stream config */
#define PLAY1_SAMPLE_RATE               48000
#define PLAY1_SAMPLE_SIZE               4
#define PLAY1_PLAYBACK_CHAN             2
#define PLAY1_CAPTURE_CHAN              2
#define PLAYBACK1_FRAME_NUM             8

/* playback2 stream config */
#define PLAY2_SAMPLE_RATE               16000
#define PLAY2_SAMPLE_SIZE               4
#define PLAY2_PLAYBACK_CHAN             2
#define PLAY2_CAPTURE_CHAN              2
#define PLAYBACK2_FRAME_NUM             8

/* adda loop stream config */
#define ADDA_SAMPLE_RATE                16000
#define ADDA_SAMPLE_SIZE                4

#define ADDA_PLAYBACK_CHAN              2
#define ADDA_CAPTURE_CHAN               2

#define RATE_TO_SIZE(n)                 (((n) + (1000 - 1)) / 1000)

#define ADDA_PLAYBACK_FRAME_SIZE        (RATE_TO_SIZE(ADDA_SAMPLE_RATE) * ADDA_SAMPLE_SIZE * ADDA_PLAYBACK_CHAN)
#define ADDA_CAPTURE_FRAME_SIZE         (RATE_TO_SIZE(ADDA_SAMPLE_RATE) * ADDA_SAMPLE_SIZE * ADDA_CAPTURE_CHAN)
#define PLAYBACK1_FRAME_SIZE            (RATE_TO_SIZE(PLAY1_SAMPLE_RATE) * PLAY1_SAMPLE_SIZE * PLAY1_PLAYBACK_CHAN)
#define PLAYBACK2_FRAME_SIZE            (RATE_TO_SIZE(PLAY2_SAMPLE_RATE) * PLAY2_SAMPLE_SIZE * PLAY2_PLAYBACK_CHAN)

#define PLAYBACK_FRAME_NUM              8
#define CAPTURE_FRAME_NUM               15

#define ADDA_NON_EXP_ALIGN(val, exp)    (((val) + ((exp) - 1)) / (exp) * (exp))
#define BUFF_ALIGN                      (4 * 4)

#define ADDA_PLAYBACK_SIZE    ADDA_NON_EXP_ALIGN(ADDA_PLAYBACK_FRAME_SIZE * PLAYBACK_FRAME_NUM, BUFF_ALIGN)
#define ADDA_CAPTURE1_SIZE    ADDA_NON_EXP_ALIGN(ADDA_CAPTURE_FRAME_SIZE * CAPTURE_FRAME_NUM, BUFF_ALIGN)
#define PLAYBACK1_SIZE        ADDA_NON_EXP_ALIGN(PLAYBACK1_FRAME_SIZE * PLAYBACK1_FRAME_NUM, BUFF_ALIGN)
#define PLAYBACK2_SIZE        ADDA_NON_EXP_ALIGN(PLAYBACK2_FRAME_SIZE * PLAYBACK2_FRAME_NUM, BUFF_ALIGN)

#define AUD_STREAM_CAP1_DEVICE   AUD_STREAM_USE_INT_CODEC
#ifdef AUDIO_OUTPUT_SWAP_DAC_1_3
#define AUD_STREAM_PLAY1_DEVICE  AUD_STREAM_USE_INT_CODEC3
#else
#define AUD_STREAM_PLAY1_DEVICE  AUD_STREAM_USE_INT_CODEC
#endif
#define AUD_STREAM_PLAY2_DEVICE  AUD_STREAM_USE_INT_CODEC2

#define ALIGNED4                        ALIGNED(4)

static uint8_t ALIGNED4 adda_playback_buf[ADDA_PLAYBACK_SIZE];
static uint8_t ALIGNED4 g_playback2_buf[PLAYBACK2_SIZE];

static uint8_t ALIGNED4 adda_capture_buf[ADDA_CAPTURE1_SIZE];

static bool adda_loop_en = false;
static uint32_t cap_wpos = 0;
static uint32_t cap_rpos = 0;

#define SYSFREQ_USER_CAPTURE1  (1<<0)
#define SYSFREQ_USER_PLAYBACK1 (1<<1)
#define SYSFREQ_USER_PLAYBACK2 (1<<2)

static uint8_t sysfreq_user_map = 0;

static enum DMA_AUD_TRIG_MODE_T dma_aud_app_trig_mode = DMA_AUD_TRIG_MODE_PWRKEY;
static int dma_aud_key_phase = 0;
static int dma_aud_cmd = 0;
static volatile bool dma_aud_cmd_come = false;
#ifdef SLEEP_TEST
static volatile bool sys_sleep_allow = true;
#endif

#ifdef RTOS
static osThreadId self_tid = NULL;
static void dma_aud_timer_handler(void const *unused);
static osTimerId dma_aud_timer_id = NULL;
osTimerDef(dma_aud_timer, dma_aud_timer_handler);
#else
static void dma_aud_timer_handler(void *arguments);
static HWTIMER_ID dma_aud_timer_id = NULL;
#endif

static int dma_aud_timer_irq = 0;
static int dma_aud_timer_flag = 0;

static uint32_t cap_buff_frame_ms  = 0;
static uint32_t play_buff_frame_ms = 0;

#define DMA_AUD_TIMER_PERIOD (1000)

void dma_audio_cp_key_proc(void);
void dma_audio_bth_key_proc(void);

#ifdef RTOS
static void dma_aud_timer_handler(void const *unused)
#else
static void dma_aud_timer_handler(void *arguments)
#endif
{
    dma_aud_timer_irq++;
    if (dma_aud_timer_flag) {
        if (dma_aud_timer_irq == 5) {
            dma_aud_timer_irq = 0;
#ifdef BTH_AS_MAIN_MCU
            dma_audio_bth_key_proc();
#endif
        }
    }
    TRACE(1, "[%s]: irq=%d, period=%d", __func__, dma_aud_timer_irq, DMA_AUD_TIMER_PERIOD);

#ifndef RTOS
    hwtimer_start(dma_aud_timer_id, MS_TO_TICKS(DMA_AUD_TIMER_PERIOD));
#endif
}

void dma_audio_periodic_on_off_test(void)
{
    TRACE(1, "%s:", __func__);

#ifdef RTOS
    osStatus_t ret = 0;
    if (dma_aud_timer_id == NULL) {
        dma_aud_timer_id = osTimerCreate(osTimer(dma_aud_timer),osTimerPeriodic, NULL);
        ret = osTimerStart(dma_aud_timer_id, DMA_AUD_TIMER_PERIOD);
        ASSERT(ret == osOK, "%s: osTimerStart failed, ret=%d", __func__, ret);
    }
#else
    int ret = 0;
    if (dma_aud_timer_id == NULL) {
        dma_aud_timer_id  = hwtimer_alloc(dma_aud_timer_handler, 0);
        ret = hwtimer_start(dma_aud_timer_id, MS_TO_TICKS(DMA_AUD_TIMER_PERIOD));
        ASSERT(ret == E_HWTIMER_OK, "[%s]: hwtimer_start failed, ret=%d", __func__, ret);
    }
#endif
    dma_aud_timer_irq = 0;
    dma_aud_timer_flag = 1;
}

void dma_audio_get_stream_cfg(struct DAUD_STREAM_CONFIG_T **cap_cfg, struct DAUD_STREAM_CONFIG_T **play_cfg)
{
#define TST_SAMP_RATE (AUD_SAMPRATE_48000)
#define TST_SAMP_BITS (AUD_BITS_24)
#define TST_CHAN_NUM  (AUD_CHANNEL_NUM_2)
#define TST_CHAN_MAP  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define TST_CHAN_VOL  (17)
#define TST_DATA_SIZE (8*16)

    struct DAUD_STREAM_CONFIG_T capture_cfg, playback_cfg;

    memset((void *)&capture_cfg, 0, sizeof(capture_cfg));

    capture_cfg.sample_rate = TST_SAMP_RATE;
    capture_cfg.bits        = TST_SAMP_BITS;
    capture_cfg.channel_num = TST_CHAN_NUM;
    capture_cfg.channel_map = TST_CHAN_MAP;
    capture_cfg.vol         = TST_CHAN_VOL;
    capture_cfg.data_size   = TST_DATA_SIZE;
    capture_cfg.irq_mode    = true;
    capture_cfg.device      = AUD_STREAM_USE_INT_CODEC2;
    capture_cfg.io_path     = AUD_INPUT_PATH_MAINMIC;

    memset((void *)&playback_cfg, 0, sizeof(playback_cfg));

    playback_cfg.sample_rate = TST_SAMP_RATE;
    playback_cfg.bits        = TST_SAMP_BITS;
    playback_cfg.channel_num = TST_CHAN_NUM;
    playback_cfg.channel_map = TST_CHAN_MAP;
    playback_cfg.vol         = TST_CHAN_VOL;
    playback_cfg.data_size   = TST_DATA_SIZE;
    playback_cfg.irq_mode    = true;
    playback_cfg.device      = AUD_STREAM_USE_INT_CODEC3;
    playback_cfg.io_path     = AUD_OUTPUT_PATH_SPEAKER;

    *cap_cfg = &capture_cfg;
    *play_cfg = &playback_cfg;
}

static void stream_request_clock(uint32_t user)
{
    if (sysfreq_user_map == 0) {
        hal_sysfreq_req(HAL_SYSFREQ_USER_APP_2, HAL_CMU_FREQ_104M);
    }
    sysfreq_user_map |= user;
}

static void stream_release_clock(uint32_t user)
{
    sysfreq_user_map &= ~user;
    if (sysfreq_user_map == 0) {
        hal_sysfreq_req(HAL_SYSFREQ_USER_APP_2, HAL_CMU_FREQ_32K);
    }
}

static void dma_audio_sleep_proc(void)
{
#ifdef SLEEP_TEST
    if ((dma_aud_cmd == DMA_AUD_TEST_CMD_APP_ON)
        || (dma_aud_cmd == DMA_AUD_TEST_CMD_ADDA_LOOP_ON)
        || (dma_aud_cmd == DMA_AUD_TEST_CMD_CAPTURE1_ON)
        || (dma_aud_cmd == DMA_AUD_TEST_CMD_PLAYBACK1_ON)) {

        sys_sleep_allow = false;
    } else {
        sys_sleep_allow = true;
    }
#ifdef RTOS
    osSignalSet(self_tid, 0x1); //trigger main thread to exit osSignalWait()
#endif
    TRACE(1, "sys_sleep_allow=%d", sys_sleep_allow);
#endif
}

#define CASE_ENUM(e) case e: return "["#e"]"
static const char *cmd_to_str(enum DMA_AUD_TEST_CMD_T cmd)
{
    switch(cmd) {
    CASE_ENUM(DMA_AUD_TEST_CMD_APP_OFF);
    CASE_ENUM(DMA_AUD_TEST_CMD_APP_ON);
    CASE_ENUM(DMA_AUD_TEST_CMD_ADDA_LOOP_ON);
    CASE_ENUM(DMA_AUD_TEST_CMD_PLAYBACK1_ON);
    CASE_ENUM(DMA_AUD_TEST_CMD_PLAYBACK1_OFF);
    CASE_ENUM(DMA_AUD_TEST_CMD_PLAYBACK2_ON);
    CASE_ENUM(DMA_AUD_TEST_CMD_PLAYBACK2_OFF);
    CASE_ENUM(DMA_AUD_TEST_CMD_CAPTURE1_ON);
    CASE_ENUM(DMA_AUD_TEST_CMD_CAPTURE1_OFF);
    CASE_ENUM(DMA_AUD_TEST_CMD_ADDA_LOOP_OFF);
    CASE_ENUM(DMA_AUD_TEST_CMD_DELAY_TEST);
    CASE_ENUM(DMA_AUD_TEST_CMD_QTY);
    }
    return "";
}

POSSIBLY_UNUSED static void dma_audio_handle_key(int key_phase)
{
    bool dummy_key = false;

    if (dma_aud_cmd_come == true) {
        TRACE(1, "[%s] cmd queue is full", __func__);
        return;
    }
    switch (key_phase) {
    case 1:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_ON;
        break;
    case 2:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_OFF;
        break;
    case 3:
        dma_aud_cmd = DMA_AUD_TEST_CMD_ADDA_LOOP_ON;
        break;
    case 4:
        dma_aud_cmd = DMA_AUD_TEST_CMD_ADDA_LOOP_OFF;
        break;
    case 5:
        dma_aud_cmd = DMA_AUD_TEST_CMD_PLAYBACK1_ON;
        break;
    case 6:
        dma_aud_cmd = DMA_AUD_TEST_CMD_PLAYBACK1_OFF;
        break;
    case 10:
        dma_aud_cmd = DMA_AUD_TEST_CMD_DELAY_TEST;
        break;
    default:
        TRACE(1,"DUMMY_KEY");
        dummy_key = true;
        break;
    }
    TRACE(1,"dma_aud_cmd=%s", cmd_to_str(dma_aud_cmd));
    dma_audio_sleep_proc();
    if (!dummy_key) {
        dma_aud_cmd_come = true;
        TRACE(1, "[%s]: key_phase=%d, cmd=%d", __func__, dma_aud_key_phase, dma_aud_cmd);
    }
}

#ifdef DMA_AUDIO_COMPLEX_PSAP_SCO
static void dma_audio_handle_key2(int key_phase)
{
    bool dummy_key = false;

    if (dma_aud_cmd_come == true) {
        TRACE(1, "[%s] cmd queue is full", __func__);
        return;
    }
    switch (key_phase) {
    case 1:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_ON;
        break;
    case 2:
        dma_aud_cmd = DMA_AUD_TEST_CMD_ADDA_LOOP_ON;
        break;
    case 3:
        dma_aud_cmd = DMA_AUD_TEST_CMD_ADDA_LOOP_OFF;
        break;
    case 4:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_OFF;
        break;
    default:
        TRACE(1,"DUMMY_KEY");
        dummy_key = true;
        break;
    }
    TRACE(1,"dma_aud_cmd=%s", cmd_to_str(dma_aud_cmd));
    dma_audio_sleep_proc();
    if (!dummy_key) {
        dma_aud_cmd_come = true;
        TRACE(1, "[%s]: key_phase=%d, cmd=%d", __func__, dma_aud_key_phase, dma_aud_cmd);
    }
}
#endif /* DMA_AUDIO_COMPLEX_PSAP_SCO */

#ifdef DMA_AUDIO_COMPLEX_PSAP_A2DP
static void dma_audio_handle_key3(int key_phase)
{
    bool dummy_key = false;

    if (dma_aud_cmd_come == true) {
        TRACE(1, "[%s] cmd queue is full", __func__);
        return;
    }
    switch (key_phase) {
    case 1:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_ON;
        break;
    case 2:
        dma_aud_cmd = DMA_AUD_TEST_CMD_PLAYBACK1_ON;
        break;
    case 3:
        dma_aud_cmd = DMA_AUD_TEST_CMD_PLAYBACK1_OFF;
        break;
    case 4:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_OFF;
        break;
    default:
        TRACE(1,"DUMMY_KEY");
        dummy_key = true;
        break;
    }
    TRACE(1,"dma_aud_cmd=%s", cmd_to_str(dma_aud_cmd));
    dma_audio_sleep_proc();
    if (!dummy_key) {
        dma_aud_cmd_come = true;
        TRACE(1, "[%s]: key_phase=%d, cmd=%d", __func__, dma_aud_key_phase, dma_aud_cmd);
    }
}
#endif /* DMA_AUDIO_COMPLEX_PSAP_A2DP */

#ifdef DMA_AUDIO_COMPLEX_PSAP_ANC
static void dma_audio_handle_key4(int key_phase)
{
    bool dummy_key = false;

    if (dma_aud_cmd_come == true) {
        TRACE(1, "[%s] cmd queue is full", __func__);
        return;
    }
    switch (key_phase) {
    case 1:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_ON;
        break;
    case 2:
        dma_aud_cmd = DMA_AUD_TEST_CMD_CAPTURE1_ON;
        break;
    case 3:
        dma_aud_cmd = DMA_AUD_TEST_CMD_CAPTURE1_OFF;
        break;
    case 4:
        dma_aud_cmd = DMA_AUD_TEST_CMD_APP_OFF;
        break;
    default:
        TRACE(1,"DUMMY_KEY");
        dummy_key = true;
        break;
    }
    TRACE(1,"dma_aud_cmd=%s", cmd_to_str(dma_aud_cmd));
    dma_audio_sleep_proc();
    if (!dummy_key) {
        dma_aud_cmd_come = true;
        TRACE(1, "[%s]: key_phase=%d, cmd=%d", __func__, dma_aud_key_phase, dma_aud_cmd);
    }
}
#endif /* DMA_AUDIO_COMPLEX_PSAP_A2DP */

static enum AUD_BITS_T sample_size_to_enum(uint32_t size)
{
    if (size == 2) {
        return AUD_BITS_16;
    } else if (size == 4) {
        return AUD_BITS_24;
    } else {
        ASSERT(false, "%s: Invalid sample size: %u", __FUNCTION__, size);
    }

    return 0;
}

static enum AUD_CHANNEL_NUM_T chan_num_to_enum(uint32_t num)
{
    if (num == 2) {
        return AUD_CHANNEL_NUM_2;
    } else if (num == 1) {
        return AUD_CHANNEL_NUM_1;
    } else {
        ASSERT(false, "%s: Invalid channel num: %u", __FUNCTION__, num);
    }

    return 0;
}

uint32_t codec_play_test_signal_data(uint8_t *buf, uint32_t len, uint32_t samp_size, uint32_t chan_num)
{
    uint32_t frame_len;

    frame_len = len / samp_size / chan_num;
    signal_generator_loop_get_data((void *)buf, frame_len);

#if 0
    int i;
    int *pd = (int *)buf;
    for (i = 0; i < 4; i++) {
        TRACE(1, "%d %d %d %d %d %d %d %d",
            pd[0], pd[1], pd[2], pd[3],
            pd[4], pd[5], pd[6], pd[7]);

        pd += 8;
    }
#endif

    return 0;
}

static uint32_t codec_play1_data_handler(uint8_t *buf, uint32_t len)
{
    static uint32_t old_time = 0;
    uint32_t cur_time;

    cur_time = hal_sys_timer_get();
    if (old_time == 0) {
        old_time = cur_time;
    }

#ifdef PLAYBACK1_SIGNAL_TEST
    codec_play_test_signal_data(buf, len, PLAY1_SAMPLE_SIZE, PLAY1_PLAYBACK_CHAN);
    return 0;
#endif

    if (!adda_loop_en) {
        goto _exit;
    }

    int32_t *src;
    int32_t *dst;
    int32_t *src_end;
    int32_t *dst_end;
    uint32_t avail;

    if (cap_wpos >= cap_rpos) {
        avail = cap_wpos - cap_rpos;
    } else {
        avail = sizeof(adda_capture_buf) + cap_wpos - cap_rpos;
    }
    if (avail * ADDA_PLAYBACK_CHAN / ADDA_CAPTURE_CHAN >= len) {
        src = (int32_t *)(adda_capture_buf + cap_rpos);
        src_end = (int32_t *)(adda_capture_buf + sizeof(adda_capture_buf));
        dst = (int32_t *)buf;
        dst_end = (int32_t *)(buf + len);

        while (dst < dst_end) {
            *dst++ = *src++;
            if (src == src_end) {
                src = (int32_t *)adda_capture_buf;
            }
#if (ADDA_PLAYBACK_CHAN == 2) && (ADDA_CAPTURE_CHAN == 1)
            *dst = *(dst-1);
            dst++;
#endif
        }

        cap_rpos = (uint32_t)src - (uint32_t)adda_capture_buf;
    } else {
        memset(buf, 0, len);
    }

_exit:

#if 0
    static uint32_t cnt;

    if (++cnt >= 2000) {
        TRACE(0, "[%X] playback: cnt=%u", hal_sys_timer_get(), cnt);
        cnt = 0;
    }
#else
    TRACE(5,"[PLAY1][%d]: buf=%x, len=%4u, wpos=%d, rpos=%d", TICKS_TO_MS(cur_time-old_time), (int)buf, len, cap_wpos, cap_rpos);
#endif
    old_time = cur_time;
    return 0;
}

static uint32_t codec_cap1_data_handler(uint8_t *buf, uint32_t len)
{
    static uint32_t old_time = 0;

    uint32_t cur_time;

    cur_time = hal_sys_timer_get();
    if (old_time == 0) {
        old_time = cur_time;
    }

    if (adda_loop_en) {
        cap_wpos += len;
        if (cap_wpos >= sizeof(adda_capture_buf)) {
            cap_wpos = 0;
        }
    }

    TRACE(4,"[CAP1][%d]: buf=%x, len=%4u, wpos=%d, rpos=%d", TICKS_TO_MS(cur_time-old_time), (int)buf, len, cap_wpos, cap_rpos);
    old_time = cur_time;
    return 0;
}

int playback1_stream_open(bool open)
{
    int ret = 0;
    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE(1, "[%s]:open=%d", __func__, open);

    if (open) {
        stream_request_clock(SYSFREQ_USER_PLAYBACK1);

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits        = sample_size_to_enum(ADDA_SAMPLE_SIZE);
        stream_cfg.channel_num = chan_num_to_enum(ADDA_PLAYBACK_CHAN);
        stream_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
        stream_cfg.sample_rate = ADDA_SAMPLE_RATE;
        stream_cfg.device      = AUD_STREAM_PLAY1_DEVICE;
        stream_cfg.vol         = 16;;
        stream_cfg.handler     = codec_play1_data_handler;
        stream_cfg.io_path     = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.data_ptr    = adda_playback_buf;
        stream_cfg.data_size   = sizeof(adda_playback_buf);

        TRACE(3,"playback1 sample_rate=%d,bits=%d,data_ptr=%x,data_size=%d",
            stream_cfg.sample_rate, stream_cfg.bits,
            (int)stream_cfg.data_ptr, stream_cfg.data_size);

#ifdef PLAYBACK1_SIGNAL_TEST
        signal_generator_init(SG_TYPE_TONE_1KHz, stream_cfg.sample_rate,
            stream_cfg.bits, stream_cfg.channel_num, -20.0);
#endif

        ret = af_stream_open(PLAYBACK1_STREAM_ID, AUD_STREAM_PLAYBACK, &stream_cfg);
        ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);

    } else {
        af_stream_close(PLAYBACK1_STREAM_ID, AUD_STREAM_PLAYBACK);

        stream_release_clock(SYSFREQ_USER_PLAYBACK1);
    }
    return ret;
}

int playback1_stream_start(bool on)
{
    int ret = 0;

    TRACE(1, "[%s]:on=%d", __func__, on);

    if (on) {
        ret = af_stream_start(PLAYBACK1_STREAM_ID, AUD_STREAM_PLAYBACK);
        ASSERT(ret == 0, "af_stream_start playback failed: %d", ret);
    } else {
        af_stream_stop(PLAYBACK1_STREAM_ID, AUD_STREAM_PLAYBACK);
    }
    return ret;
}

static uint32_t codec_play2_data_handler(uint8_t *buf, uint32_t len)
{
    static uint32_t old_time = 0;

    uint32_t time = hal_sys_timer_get();

#ifdef PLAYBACK2_SIGNAL_TEST
    codec_play_test_signal_data(buf, len, PLAY2_SAMPLE_SIZE, PLAY2_PLAYBACK_CHAN);
    return 0;
#endif

    if (old_time == 0) {
        old_time = time;
    }
    TRACE(1, "[PLAY2][%d]: buf=%x, len=%d", TICKS_TO_MS(time - old_time), (int)buf, len);
    old_time = time;

    return 0;
}

int playback2_stream_open(bool open)
{
    int ret = 0;
    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE(1, "[%s]:open=%d", __func__, open);

    if (open) {
        stream_request_clock(SYSFREQ_USER_PLAYBACK2);

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits        = sample_size_to_enum(PLAY2_SAMPLE_SIZE);
        stream_cfg.channel_num = chan_num_to_enum(PLAY2_PLAYBACK_CHAN);
        if (stream_cfg.channel_num == 1) {
            stream_cfg.channel_map = AUD_CHANNEL_MAP_CH0;
        } else {
            stream_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
        }
        stream_cfg.sample_rate = PLAY2_SAMPLE_RATE;
        stream_cfg.device      = AUD_STREAM_PLAY2_DEVICE;
        stream_cfg.vol         = 16;
        stream_cfg.handler     = codec_play2_data_handler;
        stream_cfg.io_path     = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.data_ptr    = g_playback2_buf;
        stream_cfg.data_size   = sizeof(g_playback2_buf);

        TRACE(3,"playback2 sample_rate=%d,bits=%d,data_ptr=%x,data_size=%d",
            stream_cfg.sample_rate, stream_cfg.bits,
            (int)stream_cfg.data_ptr, stream_cfg.data_size);

#ifdef PLAYBACK2_SIGNAL_TEST
        signal_generator_init(SG_TYPE_TONE_1KHz, stream_cfg.sample_rate,
            stream_cfg.bits, stream_cfg.channel_num, -20.0);
#endif
        ret = af_stream_open(PLAYBACK2_STREAM_ID, AUD_STREAM_PLAYBACK, &stream_cfg);
        ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);

    } else {
        af_stream_close(PLAYBACK2_STREAM_ID, AUD_STREAM_PLAYBACK);

        stream_release_clock(SYSFREQ_USER_PLAYBACK2);
    }
    return ret;
}

int playback2_stream_start(bool on)
{
    int ret = 0;

    TRACE(1, "[%s]:on=%d", __func__, on);

    if (on) {
        ret = af_stream_start(PLAYBACK2_STREAM_ID, AUD_STREAM_PLAYBACK);
        ASSERT(ret == 0, "af_stream_start playback failed: %d", ret);
    } else {
        af_stream_stop(PLAYBACK2_STREAM_ID, AUD_STREAM_PLAYBACK);
    }
    return ret;
}

int capture1_stream_open(bool open)
{
    int ret = 0;
    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE(1, "[%s]:open=%d", __func__, open);

    if (open) {
        stream_request_clock(SYSFREQ_USER_CAPTURE1);

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits        = sample_size_to_enum(ADDA_SAMPLE_SIZE);
        stream_cfg.channel_num = chan_num_to_enum(ADDA_CAPTURE_CHAN);
        stream_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
        stream_cfg.sample_rate = ADDA_SAMPLE_RATE;
        stream_cfg.device      = AUD_STREAM_CAP1_DEVICE;
        stream_cfg.vol         = 7;
        stream_cfg.handler     = codec_cap1_data_handler;
        stream_cfg.io_path     = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.data_ptr    = adda_capture_buf;
        stream_cfg.data_size   = sizeof(adda_capture_buf);

        TRACE(3,"capture1 sample_rate=%d,bits=%d,data_ptr=%x,data_size=%d",
            stream_cfg.sample_rate, stream_cfg.bits,
            (int)stream_cfg.data_ptr, stream_cfg.data_size);

        ret = af_stream_open(CAPTURE1_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
        ASSERT(ret == 0, "af_stream_open catpure failed: %d", ret);

    } else {
        af_stream_close(CAPTURE1_STREAM_ID, AUD_STREAM_CAPTURE);

        stream_release_clock(SYSFREQ_USER_CAPTURE1);
    }

    return ret;
}

int capture1_stream_start(bool on)
{
    int ret = 0;

    TRACE(1, "[%s]:on=%d", __func__, on);

    if (on) {
        ret = af_stream_start(CAPTURE1_STREAM_ID, AUD_STREAM_CAPTURE);
        ASSERT(ret == 0, "af_stream_start catpure failed: %d", ret);
    } else {
        af_stream_stop(CAPTURE1_STREAM_ID, AUD_STREAM_CAPTURE);
    }
    return ret;
}

void dma_audio_delay_test(void)
{
    struct DAUD_STREAM_CONFIG_T capture_cfg, playback_cfg;
    uint32_t cap_data_size, play_data_size;

    if (cap_buff_frame_ms >= DMA_AUDIO_MAX_DELAY_MS) {
        cap_buff_frame_ms = 0;
    }
    if (play_buff_frame_ms >= DMA_AUDIO_MAX_DELAY_MS) {
        play_buff_frame_ms = 0;
    }
#if 1
    cap_buff_frame_ms  += 10;
    play_buff_frame_ms += 10;
#else
    cap_buff_frame_ms  = 1;
    play_buff_frame_ms = 1;
#endif

    memset((void *)&capture_cfg, 0, sizeof(capture_cfg));
    capture_cfg.irq_mode    = true;
    capture_cfg.sample_rate = AUD_SAMPRATE_96000;
    capture_cfg.bits        = AUD_BITS_24;
    capture_cfg.channel_num = AUD_CHANNEL_NUM_2;
    capture_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
    capture_cfg.vol         = 15;
    capture_cfg.device      = AUD_STREAM_USE_INT_CODEC2;
    capture_cfg.io_path     = AUD_INPUT_PATH_MAINMIC;
    capture_cfg.data_ptr    = NULL;

    cap_data_size = (capture_cfg.sample_rate + (1000 - 1)) / 1000;
    cap_data_size = cap_data_size * 4 * capture_cfg.channel_num * cap_buff_frame_ms;
    capture_cfg.data_size   = cap_data_size;
    TRACE(1, "[%s]: cap_data_size=%d, cap_buff_frame_ms=%d",
        __func__, cap_data_size, cap_buff_frame_ms);

    memset((void *)&playback_cfg, 0, sizeof(playback_cfg));
    playback_cfg.irq_mode    = true;
    playback_cfg.sample_rate = AUD_SAMPRATE_96000;
    playback_cfg.bits        = AUD_BITS_24;
    playback_cfg.channel_num = AUD_CHANNEL_NUM_2;
    playback_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
    playback_cfg.vol         = 15;
    playback_cfg.device      = AUD_STREAM_USE_INT_CODEC3;
    playback_cfg.io_path     = AUD_OUTPUT_PATH_SPEAKER;
    playback_cfg.data_ptr    = NULL;

    play_data_size = (playback_cfg.sample_rate + (1000 - 1)) / 1000;
    play_data_size = play_data_size * 4 * playback_cfg.channel_num * play_buff_frame_ms;
    playback_cfg.data_size   = play_data_size;
    TRACE(1, "[%s]: play_data_size=%d, play_buff_fram_ms=%d",
        __func__, play_data_size, play_buff_frame_ms);

    dma_audio_on(false, NULL, NULL);
    hal_sys_timer_delay(MS_TO_TICKS(100));

    dma_audio_on(true, &capture_cfg, &playback_cfg);
}

void dma_audio_cmd_handler(void)
{
    int cmd;

    if (!dma_aud_cmd_come) {
        return;
    }
    cmd = dma_aud_cmd;
    TRACE(1, "[%s]: cmd=%d", __func__, cmd);
    switch (cmd) {
    case DMA_AUD_TEST_CMD_DELAY_TEST:
        dma_audio_delay_test();
        break;
    case DMA_AUD_TEST_CMD_APP_OFF:
        dma_audio_on(false, NULL, NULL);
        break;
    case DMA_AUD_TEST_CMD_APP_ON:
        dma_audio_on(true, NULL, NULL);
#ifdef SLEEP_TEST
        dma_audio_release_clock();
        hal_sysfreq_req(DAUD_HOST_FREQ_USER, LOW_POWER_FREQ);
#endif
        break;
    case DMA_AUD_TEST_CMD_ADDA_LOOP_ON:
        playback1_stream_open(true);
        capture1_stream_open(true);
        playback1_stream_start(true);
        capture1_stream_start(true);
        cap_wpos = 0;
        cap_rpos = 0;
        adda_loop_en = true;
        break;
    case DMA_AUD_TEST_CMD_ADDA_LOOP_OFF:
        adda_loop_en = false;
        cap_wpos = 0;
        cap_rpos = 0;
        capture1_stream_start(false);
        playback1_stream_start(false);
        capture1_stream_open(false);
        playback1_stream_open(false);
        break;
    case DMA_AUD_TEST_CMD_CAPTURE1_ON:
        capture1_stream_open(true);
        capture1_stream_start(true);
        break;
    case DMA_AUD_TEST_CMD_CAPTURE1_OFF:
        capture1_stream_start(false);
        capture1_stream_open(false);
        break;
    case DMA_AUD_TEST_CMD_PLAYBACK1_ON:
        playback1_stream_open(true);
        playback1_stream_start(true);
        break;
    case DMA_AUD_TEST_CMD_PLAYBACK1_OFF:
        playback1_stream_start(false);
        playback1_stream_open(false);
        break;
    case DMA_AUD_TEST_CMD_PLAYBACK2_ON:
        playback2_stream_open(true);
        playback2_stream_start(true);
        break;
    case DMA_AUD_TEST_CMD_PLAYBACK2_OFF:
        playback2_stream_start(false);
        playback2_stream_open(false);
        break;
    default:
        break;
    }
    dma_aud_cmd_come = false;
#ifdef SLEEP_TEST
    sys_sleep_allow = true;
#endif
}

void dma_audio_cp_key_proc(void)
{
#if defined(DMA_AUDIO_COMPLEX_PSAP_SCO)
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 4) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key2(dma_aud_key_phase);
#elif defined(DMA_AUDIO_COMPLEX_PSAP_A2DP)
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 4) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key3(dma_aud_key_phase);
#elif defined(DMA_AUDIO_COMPLEX_PSAP_ANC)
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 4) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key4(dma_aud_key_phase);
#else
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 6) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key(dma_aud_key_phase);
#endif
}

void dma_audio_bth_key_proc(void)
{
#if defined(DMA_AUDIO_COMPLEX_PSAP_SCO)
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 4) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key2(dma_aud_key_phase);
#elif defined(DMA_AUDIO_COMPLEX_PSAP_A2DP)
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 4) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key3(dma_aud_key_phase);
#elif defined(DMA_AUDIO_COMPLEX_PSAP_ANC)
    dma_aud_key_phase++;
    if (dma_aud_key_phase > 4) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key4(dma_aud_key_phase);
#elif defined(DMA_AUDIO_DELAY_TEST)
    dma_aud_key_phase = 10;
    dma_audio_handle_key(dma_aud_key_phase);
#else
    dma_aud_key_phase++;
    if (dma_aud_key_phase >= 3) {
        dma_aud_key_phase = 1;
    }
    dma_audio_handle_key(dma_aud_key_phase);
#endif
}

int dma_audio_app_test_key_handler(uint32_t code, uint8_t event)
{
    if (event == HAL_KEY_EVENT_CLICK) {
        if (code == HAL_KEY_CODE_PWR) {
            if (dma_aud_app_trig_mode == DMA_AUD_TRIG_MODE_PWRKEY) {
#if defined(CP_AS_SUBSYS) || defined(SENSOR_HUB)
                dma_audio_cp_key_proc();
#endif
#ifdef BTH_AS_MAIN_MCU
                dma_audio_bth_key_proc();
#endif
            }
            else if (dma_aud_app_trig_mode == DMA_AUD_TRIG_MODE_TIMER) {
                dma_aud_timer_flag ^= 1;
                dma_aud_timer_irq = 0;
            }
        }
    }

    return 0;
}

void dma_audio_app_test(enum DMA_AUD_TRIG_MODE_T mode, bool open_af)
{
#define BTPCM_STREAM_ID AUD_STREAM_ID_1

    TRACE(1, "[%s]: mode=%d, open_af=%d", __func__, mode, open_af);

    // wait for CP boot
    osDelay(200);

    if (open_af) {
        af_open();
    }
    dma_aud_app_trig_mode = mode;

    dma_audio_init();

#if defined(CP_AS_SUBSYS) || defined(SENSOR_HUB)
    int i;
    for (i = 0; i < AUD_STREAM_ID_NUM; i++) {
        if (i == DAUD_STREAM_ID || i == BTPCM_STREAM_ID) {
            continue;
        }
        dma_rpcif_forward_stream_open(i, AUD_STREAM_CAPTURE);
        TRACE(0, "enable audio forward stream: id=%d, stream=%d", i, AUD_STREAM_CAPTURE);
    }
#endif

#ifdef SLEEP_TEST
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);
    hal_sysfreq_print_user_freq();
    TRACE(1, "[%s]: CPU freq: %u", __func__, hal_sys_timer_calc_cpu_freq(5, 0));
#endif

#ifdef RTOS
    self_tid = osThreadGetId();
    TRACE(1, "self_tid=%x", (int)self_tid);
#endif

    if (mode == DMA_AUD_TRIG_MODE_TIMER) {
        dma_audio_periodic_on_off_test();
    }

#ifdef DMA_AUDIO_INIT_ON
    ASSERT(mode == DMA_AUD_TRIG_MODE_PWRKEY, "[%s] Bad trigger mode %d", __func__, mode);
    dma_audio_app_test_key_handler(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_CLICK);
#endif

#if defined(DMA_AUDIO_COMPLEX_PSAP_SCO)
    TRACE(1, "[DMA_AUDIO_COMPLEX_PSAP_SCO] is enabled");
#elif defined(DMA_AUDIO_COMPLEX_PSAP_A2DP)
    TRACE(1, "[DMA_AUDIO_COMPLEX_PSAP_A2DP] is enabled");
#elif defined(DMA_AUDIO_COMPLEX_PSAP_ANC)
    TRACE(1, "[DMA_AUDIO_COMPLEX_PSAP_ANC] is enabled");
#elif defined(DMA_AUDIO_DELAY_TEST)
    TRACE(1, "[DMA_AUDIO_DELAY_TEST] is enabled");
#endif

    while (1) {

#ifdef SLEEP_TEST
        if (sys_sleep_allow) {
#if !defined(RTOS)
            hal_sleep_enter_sleep();
#else
            osSignalWait(0x0, osWaitForever);
#endif
        }
#endif /* SLEEP_TEST */
        dma_audio_cmd_handler();

#ifndef RTOS
        extern void af_thread(void const *argument);
        af_thread(NULL);
#endif
    }
}

void dma_audio_app_test_by_cmd(enum DMA_AUD_TEST_CMD_T cmd)
{
    if (dma_aud_cmd_come) {
        TRACE(1, "[%s]: dma_aud_cmd_come not clear", __func__);
    }
    dma_aud_cmd = cmd;
    dma_aud_cmd_come = true;
    dma_audio_cmd_handler();
}

#endif /* DMA_AUDIO_TEST */

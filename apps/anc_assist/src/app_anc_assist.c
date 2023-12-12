/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#include "app_utils.h"
#include "app_thread.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "anc_process.h"
#include "audioflinger.h"
#include "hwtimer_list.h"
#include "audio_dump.h"
#include "speech_cfg.h"
#include "anc_assist.h"
#include "app_anc_assist_core.h"
#include "anc_assist_comm.h"
#include "assist/anc_assist_defs.h"
#include "assist/anc_assist_utils.h"
#include "assist/anc_assist_anc.h"
#include "assist/anc_assist_mic.h"
#include "assist/anc_assist_tws_sync.h"
#include "assist/anc_assist_resample.h"
#include "assist_ultrasound.h"
#include "app_anc_assist_trigger.h"
#include "app_voice_assist_ultrasound.h"
#include "app_anc_assist_thirdparty.h"
#include "app_anc.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#if defined(ANC_ASSIST_VPU)
#include "speech_bone_sensor.h"
#endif

#ifdef ANC_ASSIST_PROCESS_THREAD
#include "assist/anc_assist_thread.h"
#endif

// #define ANC_ASSIST_AUDIO_DUMP_96K

// #define ANC_ASSIST_AUDIO_DUMP

// #define ANC_ASSIST_AUDIO_DUMP_32K

//#ifndef ANC_ASSIST_ON_DSP
//#define _LOOP_CNT_FIXED_MAX
//#endif

#define ANC_ASSIST_UPDATE_SYSFREQ

#if defined(ASSIST_LOW_RAM_MOD)
#define ALGO_SAMPLE_RATE    (8000)
#else
#define ALGO_SAMPLE_RATE    (16000)
#endif

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
#define _CHANNEL_NUM        (4)
#else
#define _CHANNEL_NUM        (8)
#endif
#if defined(ASSIST_LOW_RAM_MOD)
#define ALGO_FRAME_LEN      (60)
#else
#define ALGO_FRAME_LEN      (120)
#endif
#define ALGO_FRAME_MS       (1000.0 * ALGO_FRAME_LEN / ALGO_SAMPLE_RATE)

#ifdef ANC_ASSIST_SAMPLE_RATE
#define SAMPLE_RATE_MAX     (ANC_ASSIST_SAMPLE_RATE)
#elif defined (DUAL_MIC_RECORDING)
#define SAMPLE_RATE_MAX     (48000)
#elif defined (VOICE_ASSIST_WD_ENABLED)
#define SAMPLE_RATE_MAX     (96000)
#elif defined(VOICE_ASSIST_FF_FIR_LMS)
#define SAMPLE_RATE_MAX     (32000)
#elif defined (ASSIST_LOW_RAM_MOD)
#define SAMPLE_RATE_MAX     (16000)
#else
#define SAMPLE_RATE_MAX     (ALGO_SAMPLE_RATE)
#endif

#if ((SAMPLE_RATE_MAX != ALGO_SAMPLE_RATE) && !defined(ANC_ASSIST_ON_DSP))
#define ANC_ASSIST_RESAMPLE_ENABLE
#endif

#define _SAMPLE_BITS        (24)
#if defined (ASSIST_LOW_RAM_MOD)
#define _FRAME_LEN          (ALGO_FRAME_LEN)
#else
#define _FRAME_LEN          (ALGO_FRAME_LEN * SAMPLE_RATE_MAX / ALGO_SAMPLE_RATE)
#endif

#if defined(VOICE_ASSIST_WD_ENABLED) || defined(ANC_ASSIST_ON_DSP)
#define _LOOP_CNT           (2)
#else
#define _LOOP_CNT           (2)
#endif

#ifdef VOICE_ASSIST_WD_ENABLED
#define _PLAY_SAMPLE_RATE   (96000)
#else
#define _PLAY_SAMPLE_RATE   (48000)
#endif
#define _PLAY_CHANNEL_NUM   (1)
#define _PLAY_FRAME_LEN     (ALGO_FRAME_LEN * _PLAY_SAMPLE_RATE / ALGO_SAMPLE_RATE)

#if _SAMPLE_BITS != 24
#pragma message ("APP ANC ASSIST: Just support 24 bits!!!")
#endif

typedef enum {
    _ASSIST_MSG_CLOSE = 0,
    _ASSIST_MSG_OPEN,
    _ASSIST_MSG_CLOSE_SPK,
    _ASSIST_MSG_SET_MODE,
    _ASSIST_MSG_USER_SUSPEND,
    _ASSIST_MSG_USER_RESET,
    _ASSIST_MSG_USER_CTRL,
} anc_assist_cmd_t;

typedef struct {
    anc_assist_user_t user;
    anc_assist_cmd_t  cmd;
    osThreadId thread_id;
    uint32_t params[10];
} ANC_ASSIST_MESSAGE_T;

#define ANC_ASSIST_SIGNAL_SET_MODE         (0xFF)
// Capture stream
#define AF_PINGPONG					(2)
#define STREAM_CAPTURE_ID			AUD_STREAM_ID_0
#define CODEC_CAPTURE_BUF_SIZE    	(_FRAME_LEN * sizeof(_PCM_T) * _CHANNEL_NUM * AF_PINGPONG * _LOOP_CNT)
static uint8_t __attribute__((aligned(4))) codec_capture_buf[CODEC_CAPTURE_BUF_SIZE];

#define CAPTURE_BUF_LEN 			(_FRAME_LEN * _LOOP_CNT)
static float capture_buf[_CHANNEL_NUM][CAPTURE_BUF_LEN];
#if defined(ANC_ASSIST_VPU)
static float capture_vpu_buf[CAPTURE_BUF_LEN];
#endif

// Play stream
#if defined(ANC_ASSIST_USE_INT_CODEC) || !defined(ANC_ASSIST_PILOT_TONE_ALWAYS_ON)
#define STREAM_PLAY_ID				AUD_STREAM_ID_0
#define STREAM_PLAY_CODEC			AUD_STREAM_USE_INT_CODEC
#else
#define STREAM_PLAY_ID				AUD_STREAM_ID_3
#define STREAM_PLAY_CODEC			AUD_STREAM_USE_INT_CODEC2
#endif

#if defined(ANC_ASSIST_PLAYBACK)
#define CODEC_PLAY_BUF_SIZE    		(_PLAY_FRAME_LEN * sizeof(_PCM_T) * _PLAY_CHANNEL_NUM * AF_PINGPONG * _LOOP_CNT)
static uint8_t __attribute__((aligned(4))) codec_play_buf[CODEC_PLAY_BUF_SIZE];

#define PLAY_BUF_LEN 				(_PLAY_FRAME_LEN * _LOOP_CNT)
static float play_buf[PLAY_BUF_LEN];
static int32_t g_play_buf_size = CODEC_PLAY_BUF_SIZE;

static bool g_need_open_spk = true;
static bool g_spk_open_flag = false;

static uint32_t spk_cfg_old = 0;

#ifndef ANC_ASSIST_ALGO_ON_DSP
static int32_t g_play_sample_rate = _PLAY_SAMPLE_RATE;
#endif
#endif

static int32_t g_sample_rate = ALGO_SAMPLE_RATE;
static int32_t g_sample_bits = _SAMPLE_BITS;
static int32_t g_chan_num = _CHANNEL_NUM;
static int32_t g_frame_len = ALGO_FRAME_LEN;
static int32_t g_loop_cnt = _LOOP_CNT;
static int32_t g_capture_buf_size = CODEC_CAPTURE_BUF_SIZE;

static int32_t g_phone_call_sample_rate = ALGO_SAMPLE_RATE;

static enum APP_SYSFREQ_FREQ_T g_sys_freq = APP_SYSFREQ_32K;
static bool g_opened_flag = false;
static bool g_mode_switching_flag = false;
static anc_assist_mode_t g_anc_assist_mode = ANC_ASSIST_MODE_QTY;

static bool g_need_open_mic = true;
static bool g_mic_open_flag = false;
#if defined(ANC_ASSIST_VPU)
static bool g_need_open_vpu = false;
static bool g_vpu_open_flag = false;
#endif

static uint8_t g_ff_ch_num = MAX_FF_CHANNEL_NUM;
static float *g_ff_mic_buf[MAX_FF_CHANNEL_NUM] 		= {NULL};
static uint8_t g_fb_ch_num = MAX_FB_CHANNEL_NUM;
static float *g_fb_mic_buf[MAX_FB_CHANNEL_NUM] 		= {NULL};
static uint8_t g_talk_ch_num = MAX_TALK_CHANNEL_NUM;
static float *g_talk_mic_buf[MAX_TALK_CHANNEL_NUM] 	= {NULL};
static uint8_t g_ref_ch_num = MAX_REF_CHANNEL_NUM;
static float *g_ref_mic_buf[MAX_REF_CHANNEL_NUM] 	= {NULL};
#if defined(ANC_ASSIST_VPU)
static float *g_vpu_mic_buf 	= NULL;
#endif
static uint32_t mic_cfg_old = 0;
#if defined(ANC_ASSIST_VPU)
static uint32_t vpu_cfg_old = 0;
extern uint32_t anc_vpu_cfg;
#endif

#ifndef ANC_ASSIST_ALGO_ON_DSP
static AncAssistState *anc_assist_st = NULL;
#endif
static AncAssistRes anc_assist_res;
static bool g_anc_assist_algo_sync_reset_flag = false;
#if defined(ANC_ASSIST_ALGO_ON_DSP)
static AncAssistRes anc_assist_res_temp;
static uint32_t g_anc_assist_res_change_flag = 0;
#endif
static ThirdpartyAssistRes thirdparty_assist_res;
extern AncAssistConfig anc_assist_cfg;
extern ThirdpartyAssistConfig thirdparty_assist_cfg;

static uint32_t g_user_status = 0;
static anc_assist_user_callback_t g_user_callback[ANC_ASSIST_USER_QTY];
static anc_assist_user_result_callback_t g_user_result_callback[ANC_ASSIST_USER_QTY];

static void _open_mic(void);
static void _close_mic(void);
#if defined(ANC_ASSIST_VPU)
static void _open_vpu(void);
static void _close_vpu(void);
#endif
#if defined(ANC_ASSIST_PLAYBACK)
static void _open_spk(void);
static void _close_spk(void);
#endif

static int32_t _anc_assist_open_impl(AncAssistConfig *cfg, ThirdpartyAssistConfig *thirdparty_cfg);
static int32_t _anc_assist_close_impl(void);
static int32_t app_anc_assist_direct_set_mode(anc_assist_mode_t mode);

#ifdef ANC_ASSIST_UPDATE_SYSFREQ
static void _anc_assist_sysfreq_print(void);
static void _anc_assist_update_sysfreq(void);
static void _anc_assist_reset_sysfreq(void);
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP
typedef short		_DUMP_PCM_T;
static _DUMP_PCM_T audio_dump_buf[ALGO_FRAME_LEN];
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP_32K
typedef short		_DUMP_PCM_T;
static _DUMP_PCM_T audio_dump_buf[_FRAME_LEN];
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP_96K
typedef int16_t		_DUMP_PCM_T;
static _DUMP_PCM_T audio_dump_buf[_FRAME_LEN * _LOOP_CNT];
#endif

#define ANC_ASSIST_THREAD_STACK_SIZE   (1024 * 2)
static osThreadId anc_assist_thread_tid;
static void app_anc_assist_thread(void const *argument);
osThreadDef(app_anc_assist_thread, osPriorityAboveNormal, 1, ANC_ASSIST_THREAD_STACK_SIZE, "app_anc_assist");

#define ANC_ASSIST_MAILBOX_MAX (20)
static osMailQId anc_assist_mailbox = NULL;
osMailQDef (anc_assist_mailbox, ANC_ASSIST_MAILBOX_MAX, ANC_ASSIST_MESSAGE_T);

void get_anc_assist_cfg(int *frame_size, int *sample_rate){

    *frame_size = ALGO_FRAME_LEN;
    *sample_rate = ALGO_SAMPLE_RATE;
}


bool ultrasound_process_flag = true;
osMutexId _anc_assist_mutex_id = NULL;
osMutexDef(_anc_assist_mutex);

static void _anc_assist_create_lock(void)
{
    if (_anc_assist_mutex_id == NULL) {
        _anc_assist_mutex_id = osMutexCreate((osMutex(_anc_assist_mutex)));
    }
}

static void _anc_assist_lock(void)
{
    osMutexWait(_anc_assist_mutex_id, osWaitForever);
}

static void _anc_assist_unlock(void)
{
    osMutexRelease(_anc_assist_mutex_id);
}

int32_t app_anc_assist_set_capture_info(uint32_t frame_len)
{
    g_frame_len = frame_len;
    TRACE(0, "[%s]g_frame_len=%d ", __func__, g_frame_len);

    return 0;
}

static int32_t update_stream_cfg(AncAssistConfig *cfg, ThirdpartyAssistConfig *thirdparty_cfg)
{
    _anc_assist_lock();
    g_need_open_mic = true;
#if defined(ANC_ASSIST_PLAYBACK)
    g_need_open_spk = true;
#endif
    g_loop_cnt      = _LOOP_CNT;
    g_sample_rate   = SAMPLE_RATE_MAX;

    enum AUD_IO_PATH_T app_path = AUD_IO_PATH_NULL;

    if (g_anc_assist_mode == ANC_ASSIST_MODE_STANDALONE) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_STANDALONE", __func__);
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_PHONE_CALL) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_PHONE_CALL", __func__);
        g_need_open_mic = false;
#if defined(ANC_ASSIST_VPU)
        g_need_open_vpu = false;
#endif
#if defined(ANC_ASSIST_USE_INT_CODEC) && defined(ANC_ASSIST_PLAYBACK)
        g_need_open_spk = false;
#endif
        // g_loop_cnt = 2;
        //g_loop_cnt = 1;
        g_sample_rate = g_phone_call_sample_rate;
        app_path = AUD_INPUT_PATH_MAINMIC;
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_RECORD) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_RECORD", __func__);
        g_need_open_mic = false;
#if defined(ANC_ASSIST_VPU)
        g_need_open_vpu = false;
#endif
        // g_loop_cnt = 2;
        g_sample_rate = AUD_SAMPRATE_48000;
        app_path = AUD_INPUT_PATH_ASRMIC;
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_MUSIC", __func__);
        // g_loop_cnt = 3;
#if defined(ANC_ASSIST_USE_INT_CODEC) && defined(ANC_ASSIST_PLAYBACK)
        g_need_open_spk = false;
#endif
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_AAC) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_MUSIC_AAC", __func__);
#if defined(ANC_ASSIST_USE_INT_CODEC) && defined(ANC_ASSIST_PLAYBACK)
        g_need_open_spk = false;
#endif
        // g_loop_cnt = 3;
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_SBC) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_MUSIC_SBC", __func__);
#if defined(ANC_ASSIST_USE_INT_CODEC) && defined(ANC_ASSIST_PLAYBACK)
        g_need_open_spk = false;
#endif
        // g_loop_cnt = 2;
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_NONE) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_NONE", __func__);
        g_need_open_mic = false;
#if defined(ANC_ASSIST_VPU)
        g_need_open_vpu = false;
#endif
#if defined(ANC_ASSIST_PLAYBACK)
        g_need_open_spk = false;
#endif
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_LE_CALL) {
        TRACE(0, "[%s] ANC_ASSIST_MODE_LE_CALL", __func__);
        g_need_open_mic = false;
#if defined(ANC_ASSIST_VPU)
        g_need_open_vpu = false;
#endif
#ifdef ANC_ASSIST_USE_INT_CODEC
    g_need_open_spk = false;
#endif
        app_path = AUD_INPUT_PATH_MAINMIC;
        g_loop_cnt = 1;
    } else {
        ASSERT(0, "[%s] g_anc_assist_mode(%d) is invalid!", __func__, g_anc_assist_mode);
    }

#if defined(ANC_ASSIST_VPU)
    if (cfg->enable_list_cfg.vpu_anc_en) {
        g_need_open_vpu = true;
    } else {
        g_need_open_vpu = false;
    }
#endif
    if (app_path == AUD_IO_PATH_NULL) {
        anc_assist_mic_update_anc_cfg(cfg, thirdparty_cfg);
        anc_assist_mic_set_app_cfg(AUD_IO_PATH_NULL);
        anc_assist_mic_parser_index(AUD_INPUT_PATH_ANC_ASSIST);
        g_chan_num = anc_assist_mic_get_ch_num(AUD_INPUT_PATH_ANC_ASSIST);
    } else {
        anc_assist_mic_set_anc_cfg(AUD_INPUT_PATH_ANC_ASSIST);
        anc_assist_mic_set_app_cfg(app_path);
        anc_assist_mic_parser_index(app_path);
        g_chan_num = anc_assist_mic_get_ch_num(app_path);
    }

#if defined(ANC_ASSIST_PLAYBACK)
    if (anc_assist_spk_parser_anc_cfg(cfg, thirdparty_cfg) == 0) {
        g_need_open_spk = false;
    }
#endif

#if defined(_LOOP_CNT_FIXED_MAX)
    g_loop_cnt = _LOOP_CNT;
#endif

    if (g_anc_assist_mode != ANC_ASSIST_MODE_LE_CALL) {
        g_frame_len = ALGO_FRAME_LEN * (g_sample_rate / ALGO_SAMPLE_RATE);
    }

    g_capture_buf_size = g_frame_len * sizeof(_PCM_T) * g_chan_num * AF_PINGPONG * g_loop_cnt;
#if defined(ANC_ASSIST_PLAYBACK)
    g_play_buf_size = (CODEC_PLAY_BUF_SIZE / _LOOP_CNT) * g_loop_cnt;
#endif

#if defined(ANC_ASSIST_VPU)
    TRACE(0, "[%s] Need to open vpu: %d", __func__, g_need_open_vpu);
#endif
    TRACE(0, "[%s] Need to open MIC: %d", __func__, g_need_open_mic);
#if defined(ANC_ASSIST_PLAYBACK)
    TRACE(0, "[%s] Need to open SPK: %d", __func__, g_need_open_spk);
#endif
    TRACE(0, "[%s] fs: %d, chan_num: %d, loop_cnt: %d", __func__, g_sample_rate, g_chan_num, g_loop_cnt);
    _anc_assist_unlock();

    return 0;
}

#ifdef VOICE_ASSIST_WD_ENABLED
static void update_trigger_status(void)
{
    app_anc_assist_trigger_init();

    if (g_mic_open_flag) {
        af_stream_start(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);
    }

#if defined(ANC_ASSIST_PLAYBACK)
    if (g_spk_open_flag) {
        af_stream_start(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);
    }
#endif
}
#endif

static int32_t update_codec_status(void)
{
    // MIC
    uint32_t mic_cfg_new = anc_assist_mic_parser_anc_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
    // VPU
#if defined(ANC_ASSIST_VPU)
    uint32_t vpu_cfg_new = anc_assist_mic_parser_vpu_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
#endif
    // SPK
#if defined(ANC_ASSIST_PLAYBACK)
    uint32_t spk_cfg_new = anc_assist_spk_parser_anc_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);

    if (spk_cfg_old != spk_cfg_new) {
        if (g_spk_open_flag) {
            _close_spk();
        }
    }
#endif

    if (mic_cfg_old != mic_cfg_new) {
        if (g_mic_open_flag) {
            _close_mic();
        }
    }

#if defined(ANC_ASSIST_VPU)
    if (vpu_cfg_old != vpu_cfg_new) {
        if (g_vpu_open_flag) {
            _close_vpu();
        }
    }
#endif

#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_set_cfg_sync(anc_assist_st, &anc_assist_cfg);
#endif
    thirdparty_anc_assist_set_cfg(&thirdparty_assist_cfg);

#if defined(ANC_ASSIST_VPU)
    TRACE(0, "[%s] Need to open vpu: %d / %d !!!!!", __func__,vpu_cfg_old, vpu_cfg_new);
    if (vpu_cfg_old != vpu_cfg_new) {
        if (vpu_cfg_new) {
            _open_vpu();
        }
    }
#endif

    if (mic_cfg_old != mic_cfg_new) {
        if (mic_cfg_new) {
            _open_mic();
        }
    }

#if defined(ANC_ASSIST_PLAYBACK)
    if (spk_cfg_old != spk_cfg_new) {
        if (spk_cfg_new) {
            _open_spk();
        }
    }
    spk_cfg_old = spk_cfg_new;
#endif

    mic_cfg_old = mic_cfg_new;
#if defined(ANC_ASSIST_VPU)
    vpu_cfg_old = vpu_cfg_new;
#endif

#ifdef VOICE_ASSIST_WD_ENABLED
    if (mic_cfg_old != mic_cfg_new && spk_cfg_old != spk_cfg_new) {
        if((spk_cfg_new)||(mic_cfg_new))
            update_trigger_status();
    }
#endif


    return 0;
}

static int32_t update_algo_cfg(uint32_t user_status, AncAssistConfig *cfg, ThirdpartyAssistConfig *thirdparty_cfg)
{
    cfg->enable_list_cfg.ff_howling_en = false;
    cfg->enable_list_cfg.fb_howling_en = false;
    cfg->enable_list_cfg.noise_en = false;
    cfg->enable_list_cfg.noise_classify_en = false;
    cfg->enable_list_cfg.wind_en = false;
    cfg->enable_list_cfg.pilot_en = false;
    cfg->enable_list_cfg.pnc_en = false;
    cfg->enable_list_cfg.wsd_en = false;
    cfg->enable_list_cfg.extern_kws_en = false;
    cfg->enable_list_cfg.ultrasound_en = false;
    cfg->enable_list_cfg.extern_adaptive_eq_en = false;
    cfg->enable_list_cfg.extern_adj_eq_en = false;
    cfg->enable_list_cfg.prompt_en = false;
    cfg->enable_list_cfg.autp_eq_en = false;
    cfg->enable_list_cfg.fir_lms_en = false;
    cfg->enable_list_cfg.custom_leak_en = false;
    cfg->enable_list_cfg.optimal_tf_en = false;
    cfg->enable_list_cfg.fir_anc_open_leak_en = false;
    cfg->enable_list_cfg.vpu_anc_en = false;

    for (uint32_t i=0; i<ANC_ASSIST_USER_QTY; i++) {
        if (user_status & (0x1 << i)) {
            if (i ==  ANC_ASSIST_USER_ANC) {
                TRACE(0, "[%s] ANC_ASSIST_USER_ANC", __func__);
                cfg->enable_list_cfg.ff_howling_en = false;
                cfg->enable_list_cfg.fb_howling_en = false;
                cfg->enable_list_cfg.wind_en = true;
                thirdparty_cfg->en = true;
            } else if (i ==  ANC_ASSIST_USER_PSAP) {
                TRACE(0, "[%s] ANC_ASSIST_USER_PSAP", __func__);
                cfg->enable_list_cfg.ff_howling_en = true;
                cfg->enable_list_cfg.fb_howling_en = true;
                cfg->enable_list_cfg.wind_en = true;
            } else if (i ==  ANC_ASSIST_USER_PSAP_SW) {
                TRACE(0, "[%s] ANC_ASSIST_USER_PSAP_SW", __func__);
                cfg->enable_list_cfg.ff_howling_en = true;
                cfg->enable_list_cfg.fb_howling_en = true;
                cfg->enable_list_cfg.wind_en = true;
            } else if (i ==  ANC_ASSIST_USER_KWS) {
                TRACE(0, "[%s] ANC_ASSIST_USER_KWS", __func__);
                // cfg->wsd_en = true;
                cfg->enable_list_cfg.extern_kws_en = true;
            } else if (i ==  ANC_ASSIST_USER_ULTRASOUND) {
                TRACE(0, "[%s] ANC_ASSIST_USER_ULTRASOUND", __func__);
                cfg->enable_list_cfg.ultrasound_en = true;
            } else if (i ==  ANC_ASSIST_USER_WD) {
                TRACE(0, "[%s] ANC_ASSIST_USER_WD", __func__);
                cfg->enable_list_cfg.pilot_en = true;
                cfg->pilot_cfg.adaptive_anc_en = 0;
                cfg->pilot_cfg.wd_en = 1;
                cfg->pilot_cfg.custom_leak_detect_en = 0;
            } else if (i ==  ANC_ASSIST_USER_CUSTOM_LEAK_DETECT) {
                TRACE(0, "[%s] ANC_ASSIST_USER_CUSTOM_LEAK_DETECT", __func__);
                cfg->enable_list_cfg.custom_leak_en = true;
                cfg->enable_list_cfg.pilot_en = true;
                cfg->pilot_cfg.adaptive_anc_en = 0;
                cfg->pilot_cfg.wd_en = 0;
                cfg->pilot_cfg.custom_leak_detect_en = 1;
                // cfg->fb_howling_en = true;
            } else if(i == ANC_ASSIST_USER_ONESHOT_ADAPT_ANC){
                TRACE(0, "[%s] ANC_ASSIST_USER_ONESHOT_ADAPT_ANC", __func__);
                cfg->enable_list_cfg.ff_howling_en = true;
                cfg->enable_list_cfg.fb_howling_en = true;
                cfg->enable_list_cfg.wind_en = true;
            } else if (i == ANC_ASSIST_USER_PILOT_ANC) {
                TRACE(0, "[%s] ANC_ASSIST_USER_PILOT_ANC", __func__);
                cfg->enable_list_cfg.pilot_en = true;
                cfg->pilot_cfg.adaptive_anc_en = 1;
                cfg->pilot_cfg.playback_ms = 10000;
                cfg->pilot_cfg.wd_en = 0;
                cfg->pilot_cfg.custom_leak_detect_en = 0;
            } else if (i == ANC_ASSIST_USER_PROMPT_LEAK_DETECT) {
                TRACE(0, "[%s] ANC_ASSIST_USER_PROMPT_LEAK_DETECT", __func__);
                cfg->enable_list_cfg.prompt_en = true;
            } else if (i ==  ANC_ASSIST_USER_ADAPTIVE_EQ) {
                TRACE(0, "[%s] ANC_ASSIST_USER_ADAPTIVE_EQ", __func__);
                cfg->enable_list_cfg.extern_adaptive_eq_en = true;
            } else if(i == ANC_ASSIST_USER_NOISE_ADAPT_ANC){
                TRACE(0, "[%s] ANC_ASSIST_USER_NOISE_ADAPT_ANC", __func__);
                cfg->enable_list_cfg.noise_en = true;
            } else if (i ==  ANC_ASSIST_USER_ADJ_EQ) {
                TRACE(0, "[%s] ANC_ASSIST_USER_ADJ_EQ", __func__);
                cfg->enable_list_cfg.extern_adj_eq_en = true;
            } else if(i == ANC_ASSIST_USER_NOISE_CLASSIFY_ADAPT_ANC){
                TRACE(0, "[%s] ANC_ASSIST_USER_NOISE_CLASSIFY_ADAPT_ANC", __func__);
                cfg->enable_list_cfg.noise_classify_en = true;
            }else if (i ==  ANC_ASSIST_USER_AUTO_EQ_SELECTION) {
                TRACE(0, "[%s] ANC_ASSIST_USER_AUTO_EQ_SELECTION", __func__);
                cfg->enable_list_cfg.autp_eq_en = true;
            } else if (i ==  ANC_ASSIST_USER_FIR_LMS) {
                TRACE(0, "[%s] ANC_ASSIST_USER_FIR_LMS", __func__);
                cfg->enable_list_cfg.fir_lms_en = true;
            } else if (i == ANC_ASSIST_USER_WSD) {
                TRACE(0, "[%s] ANC_ASSIST_USER_WSD", __func__);
                cfg->enable_list_cfg.wsd_en = true;
            } else if (i == ANC_ASSIST_USER_PNC_ADAPT_ANC) {
                TRACE(0, "[%s] ANC_ASSIST_USER_PNC_ADAPT_ANC", __func__);
                cfg->enable_list_cfg.pnc_en = true;
            } else if (i == ANC_ASSIST_USER_OPTIMAL_TF_ANC) {
                TRACE(0, "[%s] ANC_ASSIST_USER_OPTIMAL_TF_ANC", __func__);
                cfg->enable_list_cfg.optimal_tf_en = true;
            } else if (i == ANC_ASSIST_USER_FIR_ANC_OPEN_LEAK) {
                TRACE(0, "[%s] ANC_ASSIST_USER_FIR_ANC_OPEN_LEAK", __func__);
                cfg->enable_list_cfg.fir_anc_open_leak_en = true;
            } else {
                ASSERT(0, "[%s] i(%d) is invalid!", __func__, i);
            }
        }
    }
#if defined(ASSIST_LOW_RAM_MOD)
    //If ASSIST LOW RAM MOD is ON, we should abandon some function.
    ASSERT((0==cfg->enable_list_cfg.ff_howling_en && 0==cfg->enable_list_cfg.fb_howling_en),
        "Do not support howling when ASSIST_LOW_RAM_MOD is ON!!!");
#endif
    return 0;
}

static int anc_assist_mailbox_put(ANC_ASSIST_MESSAGE_T *msg_src)
{
    osStatus status = osOK;

    ANC_ASSIST_MESSAGE_T *msg = (ANC_ASSIST_MESSAGE_T *)osMailAlloc(anc_assist_mailbox, 0);

    if (!msg) {
        osEvent evt;
        TRACE(0, "[ANC_ASSIST] osMailAlloc error dump");
        for (uint8_t i = 0; i < ANC_ASSIST_MAILBOX_MAX; i++)
        {
            evt = osMailGet(anc_assist_mailbox, 0);
            if (evt.status == osEventMail) {
                TRACE(5, "cnt: %d user: %d cmd: %d params: %d, %d",
                    i,
                    ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->user,
                    ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->cmd,
                    ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->params[0],
                    ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->params[1]);
            } else {
                TRACE(2, "cnt:%d %d", i, evt.status);
                break;
            }
        }
        TRACE(0, "osMailAlloc error dump end");
        ASSERT(0, "[ANC_ASSIST] osMailAlloc error");
    } else {
        memcpy(msg, msg_src, sizeof(ANC_ASSIST_MESSAGE_T));
        status = osMailPut(anc_assist_mailbox, msg);
        if (osOK != status) {
            TRACE(2, "[%s] WARNING: failed: %d", __func__, status);
        }
    }

    return (int)status;
}

static int anc_assist_mailbox_get(ANC_ASSIST_MESSAGE_T **msg)
{
    int ret = 0;
    osEvent evt;
    evt = osMailGet(anc_assist_mailbox, osWaitForever);

    if (evt.status == osEventMail) {
        *msg = (ANC_ASSIST_MESSAGE_T *)evt.value.p;
    } else {
        ret = -1;
    }

    return ret;
}

static int anc_assist_mailbox_free(ANC_ASSIST_MESSAGE_T *msg)
{
    osStatus status;

    status = osMailFree(anc_assist_mailbox, msg);
    if (osOK != status) {
        TRACE(2, "[%s] WARNING: failed: %d", __func__, status);
    }

    return (int)status;
}

#if defined(ANC_ASSIST_ALGO_ON_DSP)
static uint32_t g_anc_algo_user_status = 0;
static void anc_assist_m55_handler(uint32_t user, bool open_opt)
{
    uint32_t user_status_old = g_anc_algo_user_status;
    uint32_t user_status_new = 0;
    if (user > ANC_ASSIST_USER_ALGO_DSP) {
        TRACE(2, "[%s] user(%d) > ANC_ASSIST_USER_ALGO_DSP", __func__, user);
        if (open_opt) {
            app_anc_assist_core_open(user);
        } else {
            app_anc_assist_core_close(user);
        }
        return;
    }

    if (open_opt == true) {
        user_status_new = user_status_old | (0x1 << user);
    } else {
        user_status_new = user_status_old & ~(0x1 << user);
    }

    TRACE(3, "[%s] user: old = %d new = %d", __func__, user_status_old, user_status_new);
    if (user_status_old != user_status_new) {
        if (user_status_old == 0) {
            app_anc_assist_core_open(ANC_ASSIST_USER_ALGO_DSP);
            app_anc_assist_core_ctrl(ANC_ASSIST_USER_ALGO_DSP, ASSIST_CTRL_CFG_SYNC, (uint8_t *)&(anc_assist_cfg.enable_list_cfg),sizeof(assist_enable_list_cfg_t));
        } else if (user_status_new == 0) {
            app_anc_assist_core_close(ANC_ASSIST_USER_ALGO_DSP);
        }  else {
            app_anc_assist_core_ctrl(ANC_ASSIST_USER_ALGO_DSP, ASSIST_CTRL_CFG_SYNC, (uint8_t *)&(anc_assist_cfg.enable_list_cfg),sizeof(assist_enable_list_cfg_t));
        }
    }
    g_anc_algo_user_status = user_status_new;
}
#endif

static bool ultrasound_reset_needed = true;
static int app_anc_assist_thread_handler(ANC_ASSIST_MESSAGE_T *msg_body)
{
    uint32_t cmd = msg_body->cmd;
    anc_assist_user_t user = msg_body->user;

    TRACE(3, "[%s] user: %d, cmd :%d", __func__, user, cmd);

    mic_cfg_old = anc_assist_mic_parser_anc_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
#if defined(ANC_ASSIST_VPU)
    vpu_cfg_old = anc_assist_mic_parser_vpu_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
#endif
#if defined(ANC_ASSIST_PLAYBACK)
    spk_cfg_old = anc_assist_spk_parser_anc_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
#endif

    switch (cmd) {
        case _ASSIST_MSG_CLOSE:
        case _ASSIST_MSG_OPEN: {
            bool open_opt = cmd;    // _ASSIST_MSG_CLOSE: 0, _ASSIST_MSG_OPEN: 1
            uint32_t user_status_old = g_user_status;
            uint32_t user_status_new = 0;

            if (((g_user_status & (0x1 << user)) != 0) && (open_opt == true)) {
                TRACE(2, "[%s] user(%d) is opened", __func__, user);
                return 0;
            } else if (((g_user_status & (0x1 << user)) == 0) && (open_opt == false)) {
                TRACE(2, "[%s] user(%d) is closed", __func__, user);
                return 0;
            }

            if (open_opt == true) {
                user_status_new = user_status_old | (0x1 << user);
            } else {
                user_status_new = user_status_old & ~(0x1 << user);
            }

            TRACE(0, "[%s] opt: %d, old: 0x%x, new: 0x%x", __func__, open_opt, user_status_old, user_status_new);

            if (user_status_old != user_status_new) {
                update_algo_cfg(user_status_new, &anc_assist_cfg, &thirdparty_assist_cfg);

#if defined(ANC_ASSIST_ALGO_ON_DSP)
                anc_assist_m55_handler(user,open_opt);
#endif

                if (open_opt == true && user == ANC_ASSIST_USER_ULTRASOUND) {
                    ultrasound_reset_needed = true;
                }

#ifdef VOICE_ASSIST_WD_ENABLED
                if (open_opt == false && user == ANC_ASSIST_USER_WD) {
#ifndef ANC_ASSIST_ALGO_ON_DSP
                    anc_assist_pilot_set_play_fadeout(anc_assist_st);
#endif
                }
#endif

                if ((user_status_old == 0) && (user_status_new != 0)) {
                    // Open
                    update_stream_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
                    _anc_assist_open_impl(&anc_assist_cfg, &thirdparty_assist_cfg);
                } else if ((user_status_old != 0) && (user_status_new == 0)) {
                    // Close
                    _anc_assist_close_impl();
                } else {
                    // Update
                    update_stream_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
                    update_codec_status();
                }

                // Save parameters
                g_user_status = user_status_new;
            }
            break;
        }
        case _ASSIST_MSG_SET_MODE:
            app_anc_assist_direct_set_mode(msg_body->params[0]);
            osSignalSet(msg_body->thread_id, ANC_ASSIST_SIGNAL_SET_MODE);
            osThreadYield();
            break;
        case _ASSIST_MSG_USER_SUSPEND:
#ifdef ANC_ASSIST_ON_DSP
            app_anc_assist_core_suspend_users(msg_body->params[0], msg_body->params[1]);
#endif
            break;
        case _ASSIST_MSG_USER_RESET:
#ifdef ANC_ASSIST_ON_DSP
            app_anc_assist_core_reset(user);
#endif
            break;
        case _ASSIST_MSG_USER_CTRL: {
#ifdef ANC_ASSIST_ON_DSP
            uint32_t ctrl = msg_body->params[0];
            uint32_t len = msg_body->params[1];
            uint8_t *buf = NULL;
            if (len != 0) {
                buf = (uint8_t *)&msg_body->params[2];
            }
            app_anc_assist_core_ctrl(user, ctrl, buf, len);
#endif
            break;
        }
        default:
            ASSERT(0, "[%s] cmd is invalid: %d", __func__, cmd);
            break;
    }

    return 0;
}

static void app_anc_assist_thread(void const *argument)
{
    ANC_ASSIST_MESSAGE_T *msg_p = NULL;

    while(1) {
        if (!anc_assist_mailbox_get(&msg_p)) {
            app_anc_assist_thread_handler(msg_p);
            anc_assist_mailbox_free(msg_p);
        }
    }
}

int32_t app_anc_assist_init(void)
{
#ifdef ANC_ASSIST_ON_DSP
    if (MIC_INDEX_QTY > MAX_MIC_CHANNEL_NUMBER) {
        ASSERT(0, "[%s] MAX_MIC_CHANNEL_NUMBER need to enlarge", __func__);
    }
    app_anc_assist_core_init();
#endif
    osStatus status;
    g_opened_flag = false;
    g_mode_switching_flag = false;
    g_anc_assist_mode = ANC_ASSIST_MODE_STANDALONE;

    for (uint8_t i = 0; i < MAX_FF_CHANNEL_NUM; i++) {
        g_ff_mic_buf[i] = capture_buf[MIC_INDEX_FF + i];
    }
    for (uint8_t i = 0; i < MAX_FB_CHANNEL_NUM; i++) {
        g_fb_mic_buf[i] = capture_buf[MIC_INDEX_FB + i];
    }
    for (uint8_t i = 0; i < MAX_TALK_CHANNEL_NUM; i++) {
        g_talk_mic_buf[i] = capture_buf[MIC_INDEX_TALK + i];
    }
    for (uint8_t i = 0; i < MAX_REF_CHANNEL_NUM; i++) {
        g_ref_mic_buf[i] = capture_buf[MIC_INDEX_REF + i];
    }
#if defined(ANC_ASSIST_VPU)
    g_vpu_mic_buf = capture_vpu_buf;
#endif

    g_user_status = 0;
    for (uint32_t i = 0; i<ANC_ASSIST_USER_QTY; i++) {
        g_user_callback[i] = NULL;
    }

    _anc_assist_create_lock();
    anc_assist_mic_reset();

    if (anc_assist_mailbox != NULL) {
        TRACE(1, "[%s] Mailbox is not NULL. Cnt: %d", __func__, osMailGetCount(anc_assist_mailbox));
    // status = osMailDelete(anc_assist_mailbox);
    // ASSERT(status == osOK, "[%s] Can not create mailbox", __func__);
        anc_assist_mailbox = NULL;
    }

    anc_assist_mailbox = osMailCreate(osMailQ(anc_assist_mailbox), NULL);
    ASSERT(anc_assist_mailbox != NULL, "[%s] Can not create mailbox", __func__);


    if (anc_assist_thread_tid != NULL) {
        TRACE(1, "[%s] Thread is not NULL", __func__);
        status = osThreadTerminate(anc_assist_thread_tid);
        ASSERT(status == osOK, "[%s] Can not create thread. status: %d", __func__, status);
        anc_assist_thread_tid = NULL;
    }
    anc_assist_thread_tid = osThreadCreate(osThread(app_anc_assist_thread), NULL);
    ASSERT(anc_assist_thread_tid != NULL, "[%s] Can not create thread", __func__);

    return 0;
}

int32_t app_anc_assist_deinit(void)
{
    return 0;
}

int32_t app_anc_assist_register(anc_assist_user_t user, anc_assist_user_callback_t func)
{
    TRACE(0, "[%s] user: %d", __func__, user);

    g_user_callback[user] = func;

    return 0;
}

int32_t app_anc_assist_result_register(anc_assist_user_t user, anc_assist_user_result_callback_t func)
{
    TRACE(0, "[%s] user: %d", __func__, user);
    ASSERT(user < ANC_ASSIST_USER_QTY, "[%s] user (%d) not suppotted", __FUNCTION__, user);

    g_user_result_callback[user] = func;

    return 0;
}

anc_assist_user_result_callback_t app_anc_assist_get_result_register(anc_assist_user_t user)
{
    ASSERT(user < ANC_ASSIST_USER_QTY, "[%s] user (%d) not suppotted", __FUNCTION__, user);
    return g_user_result_callback[user];
}

// TODO: Currently, just used by sco. Can extend
uint32_t app_anc_assist_get_mic_ch_num(enum AUD_IO_PATH_T path)
{
    return anc_assist_mic_get_ch_num(path);
}

uint32_t app_anc_assist_get_mic_ch_map(enum AUD_IO_PATH_T path)
{
    return anc_assist_mic_get_cfg(path);
}

int32_t app_anc_assist_parser_app_mic_buf(void *buf, uint32_t *len)
{
    _anc_assist_lock();
    anc_assist_mic_parser_app_buf(buf, len, capture_buf, sizeof(capture_buf));
    _anc_assist_unlock();

    return 0;
}

static bool _need_switch_mode(anc_assist_mode_t old_mode, anc_assist_mode_t new_mode)
{
#ifndef ANC_ASSIST_USE_INT_CODEC
    if ((old_mode == ANC_ASSIST_MODE_STANDALONE)    &&
        (new_mode == ANC_ASSIST_MODE_MUSIC)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_MUSIC)         &&
        (new_mode == ANC_ASSIST_MODE_STANDALONE)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_STANDALONE)    &&
        (new_mode == ANC_ASSIST_MODE_MUSIC_AAC)     &&
        (g_loop_cnt == 3)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_STANDALONE)    &&
        (new_mode == ANC_ASSIST_MODE_MUSIC_SBC)     &&
        (g_loop_cnt == 2)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_MUSIC_AAC)     &&
        (new_mode == ANC_ASSIST_MODE_STANDALONE)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_MUSIC_SBC)     &&
        (new_mode == ANC_ASSIST_MODE_STANDALONE)) {
            return false;
    }
#endif

    return true;
}

static int32_t app_anc_assist_direct_set_mode(anc_assist_mode_t mode)
{
    TRACE(0, "[%s] %d --> %d", __func__, g_anc_assist_mode, mode);

    if (g_anc_assist_mode == mode) {
        TRACE(0, "[%s] WARNING: Same mode = %d", __func__, mode);
        return 1;
    }

    g_anc_assist_mode = mode;
    if (g_opened_flag) {
        TRACE(0, "[%s] ------ START ------", __func__);
        _anc_assist_close_impl();
        update_stream_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
        _anc_assist_open_impl(&anc_assist_cfg, &thirdparty_assist_cfg);
        TRACE(0, "[%s] ------ END ------", __func__);
    } else {
        update_stream_cfg(&anc_assist_cfg, &thirdparty_assist_cfg);
    }

    return 0;
}

int32_t app_anc_assist_set_playback_info(int32_t sample_rate)
{
#if defined(ANC_ASSIST_USE_INT_CODEC) && defined(ANC_ASSIST_PLAYBACK)
    g_play_sample_rate = (sample_rate == 0) ? _PLAY_SAMPLE_RATE : sample_rate;
#else
    TRACE(1, "[%s] No need to set playback info when use DAC2", __FUNCTION__);
#endif
    //phone call sample rate needs to be set if ASSIST use different sample rate with SCO
    g_phone_call_sample_rate = sample_rate;
    return 0;
}

int32_t app_anc_assist_is_runing(void)
{
    return g_opened_flag;
}

static void POSSIBLY_UNUSED _anc_assist_callback(anc_assist_notify_t msg, void *data, uint32_t value)
{
    switch (msg) {
        case ANC_ASSIST_NOTIFY_FREQ:
#ifdef ANC_ASSIST_UPDATE_SYSFREQ
            _anc_assist_update_sysfreq();
            _anc_assist_sysfreq_print();
#else
            TRACE(0, "[%s] TODO: Impl ANC_ASSIST_NOTIFY_FREQ", __func__);
#endif
            break;
        default:
            break;
    }
}

void app_anc_assist_algo_reset_impl(void)
{
    TRACE(1, "[%s] ...", __func__);

#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_algo_reset_impl(anc_assist_st, &anc_assist_cfg);
#endif
    anc_assist_tws_reset_curr_info();

    g_anc_assist_algo_sync_reset_flag = false;
}

int32_t app_anc_assist_algo_reset(void)
{
    TRACE(1, "[%s] ...", __func__);

    g_anc_assist_algo_sync_reset_flag = true;

    return 0;
}

static int32_t _anc_assist_open_impl(AncAssistConfig *cfg, ThirdpartyAssistConfig *thirdparty_cfg)
{
    TRACE(0, "[%s] ...", __func__);

    if (g_opened_flag == true) {
        TRACE(0, "[%s] WARNING: g_opened_flag is true", __func__);
        return 1;
    }

    // Temporary
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, HAL_CMU_FREQ_104M);

    _anc_assist_lock();
#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_st = anc_assist_create(ALGO_SAMPLE_RATE, g_sample_bits, _CHANNEL_NUM, ALGO_FRAME_LEN, cfg, _anc_assist_callback);
#if defined(ANC_ASSIST_PLAYBACK)
    TRACE(2, "[%s] playback stream sample rate = %d", __FUNCTION__, g_play_sample_rate);
    anc_assist_pilot_set_play_sample_rate(anc_assist_st, g_play_sample_rate);
#endif
#endif
    thirdparty_anc_assist_create(ALGO_SAMPLE_RATE, _CHANNEL_NUM, ALGO_FRAME_LEN, thirdparty_cfg);


#ifdef ANC_ASSIST_AUDIO_DUMP
    audio_dump_init(ALGO_FRAME_LEN, sizeof(_DUMP_PCM_T), 4);
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP_32K
    audio_dump_init(_FRAME_LEN, sizeof(_DUMP_PCM_T), 4);
#endif

#ifdef ANC_ASSIST_PROCESS_THREAD
    anc_assist_thread_open();
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP_96K
    // audio_dump_init(g_frame_len * g_loop_cnt, sizeof(_DUMP_PCM_T), 2);

    audio_dump_init(193, sizeof(_DUMP_PCM_T), 2);
#endif

#ifdef ANC_APP
    anc_assist_anc_init();
#endif

    anc_assist_tws_sync_init(ALGO_FRAME_MS);

#if defined(ANC_ASSIST_RESAMPLE_ENABLE)
    anc_assist_resample_init(g_sample_rate, g_frame_len, NULL, NULL);
#endif

    g_opened_flag = true;
    _anc_assist_unlock();

#if defined(ANC_ASSIST_VPU)
    _open_vpu();
#endif

    _open_mic();
#if defined(ANC_ASSIST_PLAYBACK)
    _open_spk();
#endif


#ifdef VOICE_ASSIST_WD_ENABLED
    update_trigger_status();
#endif

#ifdef ANC_ASSIST_UPDATE_SYSFREQ
    _anc_assist_update_sysfreq();
    _anc_assist_sysfreq_print();
#else
    g_sys_freq = APP_SYSFREQ_26M;
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif
#endif

    return 0;
}

static int32_t _anc_assist_close_impl(void)
{
    TRACE(0, "[%s] ...", __func__);

    if (g_opened_flag == false) {
        TRACE(0, "[%s] WARNING: g_opened_flag is false", __func__);
        return 1;
    }

    // Temporary
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, HAL_CMU_FREQ_104M);

#if defined(ANC_ASSIST_PLAYBACK)
    _close_spk();
#endif
#if defined(ANC_ASSIST_VPU)
    _close_vpu();
#endif
    _close_mic();

#ifdef VOICE_ASSIST_WD_ENABLED
    app_anc_assist_trigger_deinit();
#endif

    _anc_assist_lock();
    g_opened_flag = false;

#ifdef ANC_APP
    anc_assist_anc_deinit();
#endif

#if defined(ANC_ASSIST_RESAMPLE_ENABLE)
    anc_assist_resample_deinit(g_sample_rate);
#endif

#ifdef ANC_ASSIST_PROCESS_THREAD
    anc_assist_thread_close();
#endif

#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_destroy(anc_assist_st);
#endif
    thirdparty_anc_assist_destroy();
    _anc_assist_unlock();

    g_sys_freq = APP_SYSFREQ_32K;
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif

    return 0;
}

int32_t app_anc_assist_open(anc_assist_user_t user)
{
    ANC_ASSIST_MESSAGE_T msg;
    TRACE(0, "[%s] g_user: 0x%x, user: %d", __func__, g_user_status, user);

    memset(&msg, 0, sizeof(msg));
    msg.user = user;
    msg.cmd = _ASSIST_MSG_OPEN;
    anc_assist_mailbox_put(&msg);

    return 0;
}

int32_t app_anc_assist_close(anc_assist_user_t user)
{
    ANC_ASSIST_MESSAGE_T msg;
    TRACE(0, "[%s] g_user: 0x%x, user: %d", __func__, g_user_status, user);

    memset(&msg, 0, sizeof(msg));
    msg.user = user;
    msg.cmd = _ASSIST_MSG_CLOSE;
    anc_assist_mailbox_put(&msg);

    return 0;
}

int32_t app_anc_assist_reset(anc_assist_user_t user)
{
    ANC_ASSIST_MESSAGE_T msg;
    TRACE(0, "[%s] g_user: 0x%x, user: %d", __func__, g_user_status, user);

    memset(&msg, 0, sizeof(msg));
    msg.user = user;
    msg.cmd = _ASSIST_MSG_USER_RESET;
    anc_assist_mailbox_put(&msg);

    return 0;
}

int32_t app_anc_assist_ctrl(anc_assist_user_t user, uint32_t ctrl, uint8_t *buf, uint32_t len)
{
    ANC_ASSIST_MESSAGE_T msg;
    TRACE(0, "[%s] g_user: 0x%x, user: %d, ctrl: %d", __func__, g_user_status, user, ctrl);
    ASSERT(sizeof(msg.params) >= sizeof(ctrl) + sizeof(len) + len, "[%s] Invalid len: %d", __func__, len);

    memset(&msg, 0, sizeof(msg));
    msg.user = user;
    msg.cmd = _ASSIST_MSG_USER_CTRL;
    msg.params[0] = ctrl;
    msg.params[1] = len;
    if (len > 0) {
        memcpy(&msg.params[2], buf, len);
    }
    anc_assist_mailbox_put(&msg);

    return 0;
}

int32_t app_anc_assist_set_mode(anc_assist_mode_t mode)
{
    ANC_ASSIST_MESSAGE_T msg;
    osEvent evt;

    if (g_anc_assist_mode == mode) {
        TRACE(0, "[%s] WARNING: Same mode = %d", __func__, mode);
        return 1;
    }

    if (_need_switch_mode(g_anc_assist_mode, mode) == false) {
        g_anc_assist_mode = mode;
#ifdef ANC_ASSIST_UPDATE_SYSFREQ
        if (g_opened_flag) {
            _anc_assist_update_sysfreq();
            _anc_assist_sysfreq_print();
        }
#endif
        return 0;
    }

    TRACE(1, "[%s] Start", __FUNCTION__);
    g_mode_switching_flag = true;
    memset(&msg, 0, sizeof(msg));
    msg.user = ANC_ASSIST_USER_QTY;
    msg.cmd = _ASSIST_MSG_SET_MODE;
    msg.thread_id = osThreadGetId();
    msg.params[0] = mode;

    osSignalClear(osThreadGetId(), ANC_ASSIST_SIGNAL_SET_MODE);
    anc_assist_mailbox_put(&msg);
    osThreadSetPriority(anc_assist_thread_tid, osPriorityRealtime);

    evt = osSignalWait(ANC_ASSIST_SIGNAL_SET_MODE, 2000);
    if (evt.status == osEventSignal) {
        if (evt.value.signals & ANC_ASSIST_SIGNAL_SET_MODE) {
            TRACE(1, "[%s] OK", __FUNCTION__);
        }
    } else if (evt.status == osEventTimeout) {
        TRACE(1, "[%s] WARNING: Timeout", __FUNCTION__);
        app_anc_assist_direct_set_mode(mode);
    }

    osThreadSetPriority(anc_assist_thread_tid, osPriorityAboveNormal);
    g_mode_switching_flag = false;

    return 0;
}

bool ultrasound_changed = false;
static int32_t POSSIBLY_UNUSED _process_frame_ultrasound(float *fb_mic, float *ref, uint32_t frame_len)
{
    if (ultrasound_reset_needed) {
        assist_ultrasound_reset();
        ultrasound_reset_needed = false;
    }

    ultrasound_changed = assist_ultrasound_process(fb_mic, ref, frame_len);

    if ((g_user_callback[ANC_ASSIST_USER_ULTRASOUND] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_ULTRASOUND))) {
        uint32_t res = ultrasound_changed;
        g_user_callback[ANC_ASSIST_USER_ULTRASOUND](&res, 1, NULL);
    }

    return 0;
}

void app_anc_assist_reset_pilot_state(void){
#ifndef ANC_ASSIST_ALGO_ON_DSP
    if(anc_assist_st!= NULL){
        anc_assist_reset_pilot_state(anc_assist_st);
    }
#endif
}

void app_anc_assist_set_res(AncAssistRes *res)
{
#if defined(ANC_ASSIST_ALGO_ON_DSP)
    anc_assist_res_temp = *res;
    g_anc_assist_res_change_flag = 1;
#endif
}

int32_t POSSIBLY_UNUSED _process_frame(float **ff_mic, uint8_t ff_ch_num,
                                    float **fb_mic, uint8_t fb_ch_num,
                                    float **talk_mic, uint8_t talk_ch_num,
                                    float **ref, uint8_t ref_ch_num,
#if defined(ANC_ASSIST_VPU)
                                    float *vpu_mic,
#endif
                                    uint32_t frame_len)
{
#ifdef ANC_ASSIST_AUDIO_DUMP
    uint32_t dump_ch = 0;
    audio_dump_clear_up();

    // TODO: Use capture buf
    for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)ff_mic[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

#if defined(ANC_ASSIST_VPU)
    for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)vpu_mic[i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);
#else
    for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)fb_mic[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);
#endif
    for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)talk_mic[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

    for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)ref[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

    audio_dump_run();
#endif

    if (g_anc_assist_algo_sync_reset_flag) {
        app_anc_assist_algo_reset_impl();
    }

#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_res = anc_assist_process(anc_assist_st, ff_mic, ff_ch_num, fb_mic, fb_ch_num, talk_mic, talk_ch_num, ref, ref_ch_num, frame_len);
#endif
    thirdparty_assist_res = thirdparty_anc_assist_process(ff_mic, ff_ch_num, fb_mic, fb_ch_num, talk_mic, talk_ch_num, ref, ref_ch_num, frame_len);
#if defined(ANC_ASSIST_ALGO_ON_DSP)
    memcpy(&anc_assist_res, &anc_assist_res_temp, sizeof(anc_assist_res));
#endif

#if 0
    for (uint32_t i=0; i<ANC_ASSIST_USER_QTY; i++) {
        if ((g_user_callback[i] != NULL) && (g_user_status & (0x1 << i))) {
            g_user_callback[i](talk_mic, frame_len, NULL);
        }
    }
#else
    if ((g_user_callback[ANC_ASSIST_USER_WD] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_WD))) {
        uint32_t res[2 * MAX_FB_CHANNEL_NUM];
        for (uint8_t i = 0, j = 0; i < 2 * MAX_FB_CHANNEL_NUM; i += 2, j++) {
            res[i + 0] = anc_assist_res.wd_changed[j];
            res[i + 1] = anc_assist_res.wd_status[j];
        }
        g_user_callback[ANC_ASSIST_USER_WD](res, 2 * MAX_FB_CHANNEL_NUM, NULL);
    }

    if ((g_user_callback[ANC_ASSIST_USER_KWS] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_KWS))) {
        g_user_callback[ANC_ASSIST_USER_KWS](talk_mic[0], frame_len, NULL);
    }

    if ((g_user_callback[ANC_ASSIST_USER_CUSTOM_LEAK_DETECT] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_CUSTOM_LEAK_DETECT))) {
        float res[2];
        res[0] = anc_assist_res.custom_leak_detect_result;
        res[1] = (float)anc_assist_res.custom_leak_detect_status;
        g_user_callback[ANC_ASSIST_USER_CUSTOM_LEAK_DETECT](res, 2, NULL);
    }

#if (SAMPLE_RATE_MAX == 16000)
    if ((g_user_callback[ANC_ASSIST_USER_OPTIMAL_TF_ANC] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_OPTIMAL_TF_ANC))){
        float * input_data[3];
        input_data[0] = ff_mic[0];
        input_data[1] = fb_mic[0];
        input_data[2] = ref[0];
        g_user_callback[ANC_ASSIST_USER_OPTIMAL_TF_ANC](input_data, frame_len, &anc_assist_res);
    }

    if ((g_user_callback[ANC_ASSIST_USER_FIR_LMS] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_FIR_LMS))) {
        float * input_data[3];
        input_data[0] = ff_mic[0];
        input_data[1] = fb_mic[0];
        input_data[2] = ref[0];
        g_user_callback[ANC_ASSIST_USER_FIR_LMS](input_data, frame_len, &anc_assist_res);
    }
#endif

    if ((g_user_callback[ANC_ASSIST_USER_PROMPT_LEAK_DETECT] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_PROMPT_LEAK_DETECT))) {
        float res[4];
        res[0] = anc_assist_res.fb_energy_band1;
        res[1] = anc_assist_res.fb_energy_band2;
        res[2] = anc_assist_res.fb_energy_band3;
        res[3] = (float)anc_assist_res.prompt_leak_detect_status;
        g_user_callback[ANC_ASSIST_USER_PROMPT_LEAK_DETECT](res, 4, NULL);
    }

    if ((g_user_callback[ANC_ASSIST_USER_ONESHOT_ADAPT_ANC] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_ONESHOT_ADAPT_ANC))){
        float * input_data[2];
        input_data[0] = ff_mic[0];
        input_data[1] = fb_mic[0];
        g_user_callback[ANC_ASSIST_USER_ONESHOT_ADAPT_ANC](input_data, frame_len, &anc_assist_res);
    }

    if ((g_user_callback[ANC_ASSIST_USER_NOISE_ADAPT_ANC] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_NOISE_ADAPT_ANC))){
        uint32_t res[2];
        res[0] = anc_assist_res.noise_adapt_changed;
        res[1] = anc_assist_res.noise_status;
        if (res[0]) {
            g_user_callback[ANC_ASSIST_USER_NOISE_ADAPT_ANC](res, 2, NULL);
        }
    }

    if ((g_user_callback[ANC_ASSIST_USER_FIR_ANC_OPEN_LEAK] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_FIR_ANC_OPEN_LEAK))){
#if defined(ANC_ASSIST_ALGO_ON_DSP)
        if (g_anc_assist_res_change_flag == 1){
            uint32_t res[1];
            res[0] = anc_assist_res.fir_anc_wear_leak_changed;
            if (res[0]) {
                g_user_callback[ANC_ASSIST_USER_FIR_ANC_OPEN_LEAK](res, 1, NULL);
            }
            g_anc_assist_res_change_flag = 0;
        }
#endif
    }

    if ((g_user_callback[ANC_ASSIST_USER_NOISE_CLASSIFY_ADAPT_ANC] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_NOISE_CLASSIFY_ADAPT_ANC))){
        int res[4] = {0};
        for (uint8_t i = 0, j = 0; i < 2 * MAX_FB_CHANNEL_NUM; i += 2, j++) {
            res[i + 0] = anc_assist_res.noise_classify_adapt_changed[j];
            res[i + 1] = anc_assist_res.noise_classify_status[j];
        }
        if (res[0] || res[2]) {
            g_user_callback[ANC_ASSIST_USER_NOISE_CLASSIFY_ADAPT_ANC](res, 4, NULL);
        }
    }

    if ((g_user_callback[ANC_ASSIST_USER_PILOT_ANC] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_PILOT_ANC))){
        g_user_callback[ANC_ASSIST_USER_PILOT_ANC](NULL, frame_len, &anc_assist_res);
    }

    if ((g_user_callback[ANC_ASSIST_USER_ADAPTIVE_EQ] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_ADAPTIVE_EQ))) {
        g_user_callback[ANC_ASSIST_USER_ADAPTIVE_EQ](fb_mic, frame_len, ref[0]);
    }

    if ((g_user_callback[ANC_ASSIST_USER_WSD] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_WSD))) {
        uint32_t res;
        res = anc_assist_res.wsd_flag;
        g_user_callback[ANC_ASSIST_USER_WSD](&res, 1, NULL);
    }

    if ((g_user_callback[ANC_ASSIST_USER_PNC_ADAPT_ANC] != NULL) &&
        (g_user_status & (0x1 << ANC_ASSIST_USER_PNC_ADAPT_ANC))){
        uint32_t res[2];
        res[0] = anc_assist_res.curve_changed[0];
        res[1] = anc_assist_res.curve_index[0];
        if (res[0]) {
            g_user_callback[ANC_ASSIST_USER_PNC_ADAPT_ANC](res, 2, NULL);
        }
    }

#endif

#ifdef ANC_APP
#if defined(ANC_ASSIST_ALGO_ON_DSP)
    if (g_anc_assist_res_change_flag == 1) {
#endif
        if (any_of_u32(anc_assist_res.ff_gain_changed, ff_ch_num, ANC_ASSIST_ALGO_STATUS_CHANGED) ||
            any_of_u32(anc_assist_res.fb_gain_changed, fb_ch_num, ANC_ASSIST_ALGO_STATUS_CHANGED)) {
            anc_assist_anc_set_gain_coef(anc_assist_res.ff_gain_changed, anc_assist_res.ff_gain_id, anc_assist_res.ff_gain, ff_ch_num,anc_assist_res.fb_gain_changed, anc_assist_res.fb_gain_id, anc_assist_res.fb_gain, fb_ch_num);
        }

        // TODO: deal with stereo headphone
        if (anc_assist_res.curve_changed[0]) {
            anc_assist_anc_switch_curve(anc_assist_res.curve_id[0], anc_assist_res.curve_index[0]);
        }
#if defined(ANC_ASSIST_ALGO_ON_DSP)
        g_anc_assist_res_change_flag = 0;
    }
#endif
#endif

    anc_assist_tws_sync_heartbeat();
    return 0;
}

extern bool infrasound_fadeout_flag;

static int32_t app_anc_assist_process_impl(bool pcm_interval,void *buf, uint32_t len)
{
    _anc_assist_lock();
    uint32_t pcm_len = len / sizeof(_PCM_T);
    uint32_t frame_len = pcm_len / g_chan_num;
    void *pcm_buf = buf;

#ifndef ANC_ASSIST_PROCESS_THREAD
    uint32_t offset = 0;
    uint32_t loop_cnt = frame_len / ALGO_FRAME_LEN;
#endif

    if (g_opened_flag == false) {
        TRACE(1, "[%s] WARNING: g_opened_flag is false", __func__);
        _anc_assist_unlock();
        return -1;
    }

    if (g_mode_switching_flag == true) {
        TRACE(1, "[%s] mode is switching", __func__);
        _anc_assist_unlock();
        return -1;
    }

    if (frame_len != g_frame_len*g_loop_cnt) {
        TRACE(1, "[%s] WARNING: pcm_len=%d, frame_len=%d, g_chan_num=%d, g_frame_len=%d, g_loop_cnt=%d",
                        __func__, pcm_len, frame_len, g_chan_num, g_frame_len, g_loop_cnt);
        _anc_assist_unlock();
        return -2;
    }

    // TRACE(3, "[%s] len = %d, loop_cnt = %d", __func__, len, g_loop_cnt);
    if (g_anc_assist_mode == ANC_ASSIST_MODE_PHONE_CALL) {
        anc_assist_mic_parser_anc_buf(pcm_interval, AUD_INPUT_PATH_MAINMIC, (float *)capture_buf, CAPTURE_BUF_LEN, pcm_buf, pcm_len);
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_LE_CALL) {
        anc_assist_mic_parser_anc_buf(pcm_interval, AUD_INPUT_PATH_MAINMIC, (float *)capture_buf, CAPTURE_BUF_LEN, pcm_buf, pcm_len);
    } else {
        anc_assist_mic_parser_anc_buf(pcm_interval, AUD_INPUT_PATH_ANC_ASSIST, (float *)capture_buf, CAPTURE_BUF_LEN, pcm_buf, pcm_len);
    }

#ifdef ANC_ASSIST_AUDIO_DUMP_96K
    uint32_t dump_ch = 0;
    audio_dump_clear_up();
    int offseti = 105;

    for (uint32_t i = offseti,j = 0; i < offseti + 193; i++,j++) {

        audio_dump_buf[j] = (_PCM_T)g_fb_mic_buf[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, 193);

    for (uint32_t i=offseti, j=0; i<193+offseti; i++, j++) {
        audio_dump_buf[j] = (_PCM_T)g_ref_mic_buf[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, 193);

    audio_dump_run();
#endif

#if defined(ANC_ASSIST_VPU)
    if (g_vpu_open_flag) {
        int32_t vpu_frame_len = frame_len;
#if defined(VOICE_ASSIST_FF_FIR_LMS) && SAMPLE_RATE_MAX == 32000
        vpu_frame_len = frame_len / (SAMPLE_RATE_MAX / 16000);
#endif
        speech_bone_sensor_get_data(g_vpu_mic_buf, vpu_frame_len, 2, 24);
        int32_t *buf32 = (int32_t *)g_vpu_mic_buf;
        for (int32_t i = 0; i < vpu_frame_len; i++) {
            g_vpu_mic_buf[i] = (float)buf32[i];
        }

#if defined(VOICE_ASSIST_FF_FIR_LMS) && SAMPLE_RATE_MAX == 32000
        float input_vpu_data[_FRAME_LEN * 2] = {0};
        for (int i=0; i < vpu_frame_len/2; i++) {
            input_vpu_data[i] = g_vpu_mic_buf[i];
            input_vpu_data[i + _FRAME_LEN] = g_vpu_mic_buf[i + vpu_frame_len/2];
        }
        for (int i=0; i < vpu_frame_len*2; i++) {
            g_vpu_mic_buf[i] = input_vpu_data[i];
        }
#endif
    }
#endif

    if (g_sample_rate != ALGO_SAMPLE_RATE) {
        for (uint32_t cnt = 0, offset = 0; cnt < g_loop_cnt; cnt++, offset += _FRAME_LEN) {
#ifdef ANC_ASSIST_AUDIO_DUMP_32K
        uint32_t dump_ch = 0;
        audio_dump_clear_up();

        // TODO: Use capture buf
        for (uint32_t i=0; i<_FRAME_LEN; i++) {
            audio_dump_buf[i] = (_PCM_T) (g_ff_mic_buf[0] + offset)[i] >> 5;
        }
        audio_dump_add_channel_data(dump_ch++, audio_dump_buf, _FRAME_LEN);

        for (uint32_t i=0; i<_FRAME_LEN; i++) {
            audio_dump_buf[i] = (_PCM_T)(g_fb_mic_buf[0] + offset)[i] >> 5;
        }
        audio_dump_add_channel_data(dump_ch++, audio_dump_buf, _FRAME_LEN);

        for (uint32_t i=0; i<_FRAME_LEN; i++) {
            audio_dump_buf[i] = (_PCM_T)(g_talk_mic_buf[0] + offset)[i] >> 5;
        }
        audio_dump_add_channel_data(dump_ch++, audio_dump_buf, _FRAME_LEN);

        for (uint32_t i=0; i<_FRAME_LEN; i++) {
            audio_dump_buf[i] = (_PCM_T)(g_ref_mic_buf[0] + offset)[i] >> 5;
        }
        audio_dump_add_channel_data(dump_ch++, audio_dump_buf, _FRAME_LEN);

#endif
        if ((g_user_callback[ANC_ASSIST_USER_FIR_LMS] != NULL) &&
            (g_user_status & (0x1 << ANC_ASSIST_USER_FIR_LMS))) {
            float * input_data[5];
            input_data[0] = g_ff_mic_buf[0] + offset;
            input_data[1] = g_fb_mic_buf[0] + offset;
            input_data[2] = g_talk_mic_buf[0] + offset;
#if defined(ANC_ASSIST_VPU)
            input_data[3] = g_vpu_mic_buf + offset;
#else
            input_data[3] = NULL;
#endif
            input_data[4] = g_ref_mic_buf[0] + offset;
            g_user_callback[ANC_ASSIST_USER_FIR_LMS](input_data, _FRAME_LEN, &anc_assist_res);
        }

        if ((g_user_callback[ANC_ASSIST_USER_OPTIMAL_TF_ANC] != NULL) &&
            (g_user_status & (0x1 << ANC_ASSIST_USER_OPTIMAL_TF_ANC))){
            float * input_data[3];
            input_data[0] = g_ff_mic_buf[0] + offset;
            input_data[1] = g_fb_mic_buf[0] + offset;
            input_data[2] = g_ref_mic_buf[0] + offset;
            g_user_callback[ANC_ASSIST_USER_OPTIMAL_TF_ANC](input_data, _FRAME_LEN, &anc_assist_res);
        }
#ifdef ANC_ASSIST_AUDIO_DUMP_32K
        audio_dump_run();
#endif
        }
    }

#if defined(ANC_ASSIST_RESAMPLE_ENABLE)
    if (g_sample_rate != ALGO_SAMPLE_RATE) {
        ASSERT(g_sample_rate == SAMPLE_RATE_MAX, "[%s] g_sample_rate(%d) is invalid!", __func__, g_sample_rate);

#ifdef VOICE_ASSIST_WD_ENABLED
        _process_frame_ultrasound(g_fb_mic_buf[0], g_ref_mic_buf[0], frame_len);
#endif
        anc_assist_resample_process((float *)capture_buf, CAPTURE_BUF_LEN, frame_len);
#ifndef ANC_ASSIST_PROCESS_THREAD
        loop_cnt /= (g_sample_rate / ALGO_SAMPLE_RATE);
#endif
        frame_len /= (g_sample_rate / ALGO_SAMPLE_RATE);
    }
#endif
    _anc_assist_unlock();

#if defined(ANC_ASSIST_PROCESS_THREAD) || defined(ANC_ASSIST_ON_DSP)
    float *in_buf[MIC_INDEX_QTY] = {NULL};

    for (uint8_t i = 0; i < g_ff_ch_num; i++) {
        in_buf[MIC_INDEX_FF + i] = g_ff_mic_buf[i];
    }

    for (uint8_t i = 0; i < g_fb_ch_num ; i++) {
        in_buf[MIC_INDEX_FB + i] = g_fb_mic_buf[i];
    }

    for (uint8_t i = 0; i < g_talk_ch_num ; i++) {
        in_buf[MIC_INDEX_TALK + i] = g_talk_mic_buf[i];
    }

    for (uint8_t i = 0; i < g_ref_ch_num ; i++) {
        in_buf[MIC_INDEX_REF + i] = g_ref_mic_buf[i];
    }
#if defined(ANC_ASSIST_VPU)
    in_buf[MIC_INDEX_VPU] = g_vpu_mic_buf;
#endif
#endif

#ifdef ANC_ASSIST_ON_DSP
    app_anc_assist_core_process(in_buf, frame_len);
#endif
#ifdef ASSIST_RESULT_FIFO_BUF
    app_anc_assist_core_get_result_via_fifo();
#endif

#if defined(ANC_ASSIST_PROCESS_THREAD)
    anc_assist_thread_capture_process(in_buf, frame_len);
#else
    for (uint32_t cnt = 0; cnt < loop_cnt; cnt++) {
        float *ff_mic_buf[MAX_FF_CHANNEL_NUM];
        float *fb_mic_buf[MAX_FB_CHANNEL_NUM];
        float *talk_mic_buf[MAX_TALK_CHANNEL_NUM];
        float *ref_mic_buf[MAX_REF_CHANNEL_NUM];

        for (uint8_t i = 0; i < g_ff_ch_num; i++) {
            ff_mic_buf[i] = g_ff_mic_buf[i] + offset;
        }
        for (uint8_t i = 0; i < g_fb_ch_num; i++) {
            fb_mic_buf[i] = g_fb_mic_buf[i] + offset;
        }
        for (uint8_t i = 0; i < g_talk_ch_num; i++) {
            talk_mic_buf[i] = g_talk_mic_buf[i] + offset;
        }
        for (uint8_t i = 0; i < g_ref_ch_num; i++) {
            ref_mic_buf[i] = g_ref_mic_buf[i] + offset;
        }

        _process_frame(ff_mic_buf, g_ff_ch_num,
                    fb_mic_buf, g_fb_ch_num,
                    talk_mic_buf, g_talk_ch_num,
                    ref_mic_buf, g_ref_ch_num,
#if defined(ANC_ASSIST_VPU)
                    g_vpu_mic_buf 	+ offset,
#endif
                    ALGO_FRAME_LEN);
        offset += ALGO_FRAME_LEN;
    }
#endif

    return 0;
}

int32_t app_anc_assist_process(void *buf, uint32_t len)
{
    app_anc_assist_process_impl(false, buf, len);

    return 0;
}

int32_t app_anc_assist_process_interval(void *buf, uint32_t len)
{
    app_anc_assist_process_impl(true, buf, len);

    return 0;
}

static uint32_t codec_capture_stream_callback(uint8_t *buf, uint32_t len)
{
#if !defined(ANC_ASSIST_ON_DSP ) && !defined(ANC_ASSIST_PROCESS_THREAD) && defined(ANC_ASSIST_UPDATE_SYSFREQ)
    _anc_assist_update_sysfreq();
#endif

    app_anc_assist_process(buf, len);

#if !defined(ANC_ASSIST_ON_DSP ) && !defined(ANC_ASSIST_PROCESS_THREAD) && defined(ANC_ASSIST_UPDATE_SYSFREQ)
    _anc_assist_reset_sysfreq();
#endif

    return len;
}

#if defined(ANC_ASSIST_PLAYBACK)
#ifdef VOICE_ASSIST_WD_ENABLED
#define ULTRASOUND_VOL (DB2LIN(-40))
static const int16_t local_96kHz_pcm_data[] = {
    #include "pilot_oed_pcm.h"
};
#endif

static uint32_t codec_play_stream_callback(uint8_t *buf, uint32_t len)
{
    uint32_t pcm_len = len / sizeof(_PCM_T);
    uint32_t frame_len = pcm_len;
    _PCM_T *pcm_buf = (_PCM_T *)buf;

#ifdef VOICE_ASSIST_WD_ENABLED
    app_anc_assist_trigger_checker();
#endif

#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_pilot_get_play_data(anc_assist_st, play_buf, frame_len);
#endif

    for (int32_t i=0; i<frame_len; i++) {
        pcm_buf[i] = (_PCM_T)play_buf[i];
    }

#ifdef VOICE_ASSIST_WD_ENABLED
    for (int32_t i=0; i<ARRAY_SIZE(local_96kHz_pcm_data); i++) {
        pcm_buf[i] = speech_ssat_int24(pcm_buf[i] + (int32_t)(ULTRASOUND_VOL * (local_96kHz_pcm_data[i] << 8)));
    }
#endif

#if 0//def ANC_ASSIST_AUDIO_DUMP_96K
    audio_dump_clear_up();
    for (uint32_t i=0; i<frame_len; i++) {
        audio_dump_buf[i] = (_PCM_T)pcm_buf[i] >> 8;
    }
    audio_dump_add_channel_data(0, audio_dump_buf, frame_len);
    audio_dump_run();
#endif

    return len;
}

#if defined(ANC_ASSIST_USE_INT_CODEC)
static uint32_t codec_play_merge_pilot_data_q15(uint8_t *buf, uint32_t len, enum AUD_CHANNEL_NUM_T channel_num)
{
    uint32_t pcm_len = len / sizeof(int16_t);
    uint32_t remain_len = pcm_len;
    int16_t *pcm_buf = (int16_t *)buf;

    while (remain_len) {
        uint32_t block_len = MIN(PLAY_BUF_LEN, remain_len / channel_num);

#ifndef ANC_ASSIST_ALGO_ON_DSP
        anc_assist_pilot_get_play_data(anc_assist_st, play_buf, block_len);
#endif

        arm_scale_f32(play_buf, 1.f / (1 << 8), play_buf, block_len);

        if (channel_num == AUD_CHANNEL_NUM_1) {
            for (int32_t i = 0; i < block_len; i++) {
                pcm_buf[i] = speech_ssat_int16(pcm_buf[i]+ (int16_t)play_buf[i]);
            }
            pcm_buf += block_len;
        } else {
            for (int32_t i = 0, j = 0; j < block_len; i += 2, j++) {
                pcm_buf[i] = speech_ssat_int16(pcm_buf[i] + (int16_t)play_buf[j]);
                pcm_buf[i + 1] = speech_ssat_int16(pcm_buf[i + 1] + (int16_t)play_buf[j]);
            }
            pcm_buf += block_len * 2;
        }

        remain_len -= block_len * channel_num;
    }

    return len;
}

static uint32_t codec_play_merge_pilot_data_q23(uint8_t *buf, uint32_t len, enum AUD_CHANNEL_NUM_T channel_num)
{
    uint32_t pcm_len = len / sizeof(int32_t);
    uint32_t remain_len = pcm_len;
    int32_t *pcm_buf = (int32_t *)buf;

    while (remain_len) {
        uint32_t block_len = MIN(PLAY_BUF_LEN, remain_len / channel_num);
#ifndef ANC_ASSIST_ALGO_ON_DSP
        anc_assist_pilot_get_play_data(anc_assist_st, play_buf, block_len);
#endif

        if (channel_num == AUD_CHANNEL_NUM_1) {
            for (int32_t i=0; i<block_len; i++) {
                pcm_buf[i] = speech_ssat_int24(pcm_buf[i]+ (_PCM_T)play_buf[i]);
            }
            pcm_buf += block_len;
        } else {
            for (int32_t i = 0, j = 0; j < block_len; i += 2, j++) {
                pcm_buf[i] = speech_ssat_int24(pcm_buf[i] + (_PCM_T)play_buf[j]);
                pcm_buf[i + 1] = speech_ssat_int24(pcm_buf[i + 1] + (_PCM_T)play_buf[j]);
            }
            pcm_buf += block_len * 2;
        }

        remain_len -= block_len * channel_num;
    }

    return len;
}

uint32_t codec_play_merge_pilot_data(uint8_t *buf, uint32_t len, void *cfg)
{
    struct AF_STREAM_CONFIG_T *config = (struct AF_STREAM_CONFIG_T *)cfg;

    if (config->bits == AUD_BITS_16) {
        return codec_play_merge_pilot_data_q15(buf, len, config->channel_num);
    } else if (config->bits == AUD_BITS_24) {
        return codec_play_merge_pilot_data_q23(buf, len, config->channel_num);
    } else {
        ASSERT(0, "[%s] bits(%d) not supported", __FUNCTION__, config->bits);
    }
}
#endif
#endif

static void _open_mic(void)
{
    if (g_mic_open_flag == true) {
        TRACE(0, "[%s] WARNING: MIC is opened", __func__);
        return;
    }

    if (g_need_open_mic) {
        struct AF_STREAM_CONFIG_T stream_cfg;

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.channel_num 	= (enum AUD_CHANNEL_NUM_T)g_chan_num;
        stream_cfg.sample_rate 	= (enum AUD_SAMPRATE_T)g_sample_rate;
        stream_cfg.bits 		= (enum AUD_BITS_T)g_sample_bits;
        stream_cfg.vol 			= 12;
        stream_cfg.chan_sep_buf = false;
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path      = AUD_INPUT_PATH_ANC_ASSIST;
        stream_cfg.channel_map	= anc_assist_mic_get_cfg(AUD_INPUT_PATH_ANC_ASSIST);
        stream_cfg.handler      = codec_capture_stream_callback;
        stream_cfg.data_size    = g_capture_buf_size;
        stream_cfg.data_ptr     = codec_capture_buf;

        TRACE(0, "[%s] sample_rate:%d, data_size:%d", __func__, stream_cfg.sample_rate, stream_cfg.data_size);

        af_stream_open(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE, &stream_cfg);
#ifndef VOICE_ASSIST_WD_ENABLED
        af_stream_start(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);
#endif

        g_mic_open_flag = true;
    }
}

#if defined(ANC_ASSIST_VPU)
static void _open_vpu(void)
{
    if (g_vpu_open_flag == true) {
        TRACE(0, "[%s] WARNING: VPU is opened", __func__);
        return;
    }

    if (g_need_open_vpu) {
        int32_t sample_ratio = 1;
#if defined(VOICE_ASSIST_FF_FIR_LMS) && SAMPLE_RATE_MAX == 32000
        sample_ratio = SAMPLE_RATE_MAX / 16000;
#endif
        speech_bone_sensor_open(16000, g_frame_len * g_loop_cnt / sample_ratio); //bone sensor set 16k
        TRACE(0, "[%s]: frame_len %d...", __func__, g_frame_len * g_loop_cnt);
        speech_bone_sensor_start();
        g_vpu_open_flag = true;
    }
}
#endif

static void _close_mic(void)
{
    if (g_mic_open_flag == false) {
        TRACE(0, "[%s] WARNING: MIC is closed", __func__);
        return;
    }

    TRACE(0, "[%s] ...", __func__);
    af_stream_stop(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);
    af_stream_close(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);

    g_mic_open_flag = false;
}

#if defined(ANC_ASSIST_VPU)
static void _close_vpu(void)
{
    if (g_vpu_open_flag == false) {
        TRACE(0, "[%s] WARNING: vpu is closed", __func__);
        return;
    }
    speech_bone_sensor_stop();
    speech_bone_sensor_close();
    g_vpu_open_flag = false;
}
#endif

#if defined(ANC_ASSIST_PLAYBACK)
static void _open_spk(void)
{
    if (g_spk_open_flag == true) {
        TRACE(0, "[%s] WARNING: SPK is opened", __func__);
        return;
    }

    if (g_need_open_spk) {
        struct AF_STREAM_CONFIG_T stream_cfg;

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)AUD_CHANNEL_MAP_CH0;
        stream_cfg.sample_rate = _PLAY_SAMPLE_RATE;
        stream_cfg.bits = AUD_BITS_24;
        stream_cfg.vol = TGT_VOLUME_LEVEL_MAX;
        stream_cfg.chan_sep_buf = false;
        stream_cfg.device       = STREAM_PLAY_CODEC;
        stream_cfg.handler      = codec_play_stream_callback;
        stream_cfg.data_size    = g_play_buf_size;
        stream_cfg.data_ptr     = codec_play_buf;

        TRACE(0, "[%s] sample_rate:%d, data_size:%d", __func__, stream_cfg.sample_rate, stream_cfg.data_size);

        af_stream_open(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK, &stream_cfg);

    // add the pingpang
#ifdef VOICE_ASSIST_WD_ENABLED
        int32_t *pcm_buf = (int32_t *)codec_play_buf;
        for (int32_t i=0; i<ARRAY_SIZE(local_96kHz_pcm_data); i++) {
            pcm_buf[i] = (int32_t)(ULTRASOUND_VOL * (local_96kHz_pcm_data[i] << 8));
        }

        pcm_buf = (int32_t *)(codec_play_buf + g_play_buf_size / 2);
        for (int32_t i=0; i<ARRAY_SIZE(local_96kHz_pcm_data); i++) {
            pcm_buf[i] = (int32_t)(ULTRASOUND_VOL * (local_96kHz_pcm_data[i] << 8));
        }
#endif

#ifndef VOICE_ASSIST_WD_ENABLED
    af_stream_start(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);
#endif

        g_spk_open_flag = true;
    }
}

extern bool pilot_play_get_fadeout_state(void);
static void _close_spk(void)
{
    if (g_spk_open_flag == false) {
        TRACE(0, "[%s] WARNING: SPK is closed", __func__);
        return;
    }

#ifndef VOICE_ASSIST_WD_ENABLED
#ifndef ANC_ASSIST_ALGO_ON_DSP
    anc_assist_pilot_set_play_fadeout(anc_assist_st);
#endif
    osDelay(anc_assist_cfg.pilot_cfg.gain_smooth_ms + 300);     // 300: More time to fadeout
#else
    if (pilot_play_get_fadeout_state()) {
        osDelay(anc_assist_cfg.pilot_cfg.gain_smooth_ms + 300);
    } else {
        osDelay(10);
    }
#endif

    TRACE(0, "[%s] ...", __func__);
    af_stream_stop(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);
    af_stream_close(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);

    g_spk_open_flag = false;
}
#endif

#ifdef ANC_ASSIST_UPDATE_SYSFREQ

#define ANC_ASSIST_BASE_MIPS (3)

#define ULTRA_INFRASOUND_BASE_MIPS (20)     //need to be changed

static enum APP_SYSFREQ_FREQ_T _anc_assist_get_sysfreq(void)
{
    enum APP_SYSFREQ_FREQ_T freq = APP_SYSFREQ_32K;
    int32_t required_mips = ANC_ASSIST_BASE_MIPS;
#ifndef ANC_ASSIST_ALGO_ON_DSP
    required_mips += anc_assist_get_required_mips(anc_assist_st);
#endif
#ifdef VOICE_ASSIST_WD_ENABLED
    required_mips += ULTRA_INFRASOUND_BASE_MIPS;
#endif
    // TRACE(0, "[%s] Required mips: %dM", __func__,  required_mips);

    if (required_mips >= 96) {
        freq = APP_SYSFREQ_208M;
    } else if(required_mips >= 72) {
        freq = APP_SYSFREQ_104M;
    } else if (required_mips >= 48) {
        freq = APP_SYSFREQ_78M;
    } else if (required_mips >= 24) {
        freq = APP_SYSFREQ_52M;//BES intentional code. If define VOICE_ASSIST_WD_ENABLED, this statement is reachable
#if defined(ANC_ASSIST_ON_DSP) && !defined(ANC_ASSIST_ON_DSP_SENS) // FIXED ME : only 1600
    } else if (required_mips >= 15) {
        freq = APP_SYSFREQ_26M;
    } else if (required_mips >= 6) {
        freq = APP_SYSFREQ_15M;//BES intentional code. This statement is reachable.
#endif
    } else {
#if defined(ANC_ASSIST_ON_DSP) && !defined(ANC_ASSIST_ON_DSP_SENS) // FIXED ME : only 1600
        freq = APP_SYSFREQ_15M;
#else
        freq = APP_SYSFREQ_26M;
#endif
    }

    // NOTE: Optimize power consumption for special project
    if (g_anc_assist_mode == ANC_ASSIST_MODE_PHONE_CALL) {
#ifdef ANC_ASSIST_ON_DSP // FIXED ME : only 1600
        if (required_mips < 12) {
            freq = APP_SYSFREQ_32K;
        }
#endif
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_RECORD) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC) {
#ifdef ANC_ASSIST_ON_DSP // FIXED ME : only 1600
        if (required_mips <= ANC_ASSIST_BASE_MIPS) {
            freq = APP_SYSFREQ_32K;
        }
#endif
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_AAC) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }

    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_SBC) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }

    }

    return freq;
}

static void _anc_assist_sysfreq_print(void)
{
    int32_t required_mips = ANC_ASSIST_BASE_MIPS;
#ifndef ANC_ASSIST_ALGO_ON_DSP
    required_mips += anc_assist_get_required_mips(anc_assist_st);
#endif
#ifdef VOICE_ASSIST_WD_ENABLED
    required_mips += ULTRA_INFRASOUND_BASE_MIPS;
#endif

    TRACE(0, "[%s] Required mips: %dM", __func__,  required_mips);
    TRACE(0,"assist sys freq =%d ",g_sys_freq);
}

static void _anc_assist_update_sysfreq(void)
{
    g_sys_freq = _anc_assist_get_sysfreq();
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif
}

POSSIBLY_UNUSED static void _anc_assist_reset_sysfreq(void)
{
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, APP_SYSFREQ_32K);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, APP_SYSFREQ_32K, hal_sys_timer_calc_cpu_freq(5, 0));
#endif
}

#endif

int32_t app_anc_assist_get_prompt_anc_index(int32_t *anc_index,int channel_idx, float *band1, float *band2, float *band3)
{
#ifndef ANC_ASSIST_ALGO_ON_DSP
    ASSERT(anc_assist_st != NULL,"[%s] st is null",__func__);
    TRACE(0,"[%s] st is not null, set new one",__func__);
    anc_assist_get_prompt_anc_index(anc_assist_st,anc_index,channel_idx,band1,band2,band3);
#endif

    return 0;
}

int32_t app_anc_assist_users_suspend(bool en, uint32_t users)
{
    TRACE(3, "[%s] en: %d, users: 0x%x", __func__, en, users);

    ANC_ASSIST_MESSAGE_T msg;
    memset(&msg, 0, sizeof(msg));
    msg.user = ANC_ASSIST_USER_QTY;
    msg.cmd = _ASSIST_MSG_USER_SUSPEND;
    msg.params[0] = en;
    msg.params[1] = users;
    anc_assist_mailbox_put(&msg);

    return 0;
}

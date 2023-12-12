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
#include "stdio.h"
#include <stdlib.h>
#include "hal_trace.h"
#include "hal_aud.h"
#include "plat_types.h"
#include "string.h"
#include "audio_test_defs.h"
#include "audio_test_cmd.h"
#include "app_trace_rx.h"
#include "app_key.h"
#include "bluetooth_bt_api.h"
#include "app_media_player.h"
#include "anc_process.h"
#include "audio_player_adapter.h"
#include "math.h"
#if defined(ANC_APP)
#include "app_anc.h"
#include "app_anc_utils.h"
#endif

#ifdef DMA_AUDIO_APP
#include "dma_audio_cli.h"
#endif

#ifdef PSAP_SW_APP
#include "psap_sw_app.h"
extern void psap_sw_app_ctrl_test(const char *cmd);
#endif

#ifdef BLE_AUDIO_ENABLED
#include "bluetooth_ble_api.h"
#endif

#define AUDIO_TEST_LOG_I(str, ...)      TR_INFO(TR_MOD(TEST), "[AUDIO_TEST]" str, ##__VA_ARGS__)

/**
 * Usage:
 *  1. CMD Format: e.g. [audio_test,anc_switch]
 **/

typedef void (*func_handler_t)(const char *cmd);

typedef struct {
    const char *name;
    func_handler_t handler;
} audio_test_func_t;

extern void app_debug_tool_printf(const char *fmt, ...);

static void audio_test_anc_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] cmd len: %d", __func__, strlen(cmd) - 2);

#if defined(ANC_APP)
    if (strlen(cmd) > 2) {
        uint32_t mode = 0;
        sscanf(cmd, "%d", &mode);

        AUDIO_TEST_LOG_I("[%s] mode: %d", __func__, mode);
        ASSERT(mode < APP_ANC_MODE_QTY, "[%s] mode is invalid: %d", __func__, mode);
        app_anc_switch(mode);
    } else {
        app_anc_loop_switch();
    }
#endif
}

#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND_CP__) || defined(__VIRTUAL_SURROUND_STEREO__)
extern int32_t audio_process_stereo_surround_onoff(int32_t onoff);
int open_flag = 0;
int32_t stereo_surround_status = 0;
#include "app_bt_stream.h"
extern int32_t audio_process_stereo_set_yaw(float yaw);
extern int32_t audio_process_stereo_set_pitch(float pitch);
static float pitch_angle = 0;
static float yaw_angle = 0;
#endif

static void audio_test_virtual_surround_on(const char *cmd){
#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND_CP__) || defined(__VIRTUAL_SURROUND_STEREO__)
    open_flag = 1;
    // app_anc_switch(open_flag);
    stereo_surround_status = open_flag;
    audio_process_stereo_surround_onoff(open_flag);
#endif
}

static void audio_test_virtual_surround_off(const char *cmd){
#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND_CP__) || defined(__VIRTUAL_SURROUND_STEREO__)
    open_flag = 0;
    // app_anc_switch(open_flag);
    stereo_surround_status = open_flag;
    audio_process_stereo_surround_onoff(open_flag);
#endif
}

static void audio_test_virtual_surround_test_yaw(const char *cmd){
#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND_CP__) || defined(__VIRTUAL_SURROUND_STEREO__)
    yaw_angle += 10.0;
    if(yaw_angle > 180){
        yaw_angle = -180;
    }
    // TRACE(0,"!!!!!!!!!!!!!!!!!!!!! yaw_angle = %d",(int)(yaw_angle));
    audio_process_stereo_set_yaw(yaw_angle);
#endif
}

static void audio_test_virtual_surround_test_pitch(const char *cmd){
#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND_CP__) || defined(__VIRTUAL_SURROUND_STEREO__)
    
    pitch_angle += 10.0;
    if(pitch_angle > 80){
        pitch_angle = -80;
    }
    audio_process_stereo_set_pitch(pitch_angle);
#endif
}

static void audio_test_anc_gain_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);

#if defined(ANC_APP)
    static bool flag = false;
    if (flag) {
        app_anc_set_gain_f32(ANC_GAIN_USER_TUNING, PSAP_FEEDFORWARD, 1.0, 1.0);
    } else {
        app_anc_set_gain_f32(ANC_GAIN_USER_TUNING, PSAP_FEEDFORWARD, 0.0, 0.0);
    }

    flag = !flag;
#endif
}

extern int32_t audio_test_stream(void);
static void audio_test_stream_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);

#if defined(AUDIO_TEST_STREAM)
    audio_test_stream();
#endif
}

#if defined(ANC_ASSIST_ENABLED)
#include "app_anc_assist.h"
#endif
static void audio_test_anc_assist_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if defined(ANC_ASSIST_ENABLED)
    static bool flag = true;
    if (flag) {
        app_anc_assist_open(ANC_ASSIST_USER_ANC);
    } else {
        app_anc_assist_close(ANC_ASSIST_USER_ANC);
    }

    flag = !flag;
#endif
}

#if defined(ANC_ASSIST_ENABLED)
#include "app_voice_assist_ai_voice.h"
#include "audioflinger.h"
#endif

static void audio_test_kws_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if defined(ANC_ASSIST_ENABLED)

    struct AF_STREAM_CONFIG_T kws_stream_cfg;
    kws_stream_cfg.sample_rate    = AUD_SAMPRATE_16000;
    kws_stream_cfg.channel_num    = AUD_CHANNEL_NUM_1;
    kws_stream_cfg.bits           = AUD_BITS_16;
    kws_stream_cfg.device         = AUD_STREAM_USE_INT_CODEC;
    kws_stream_cfg.io_path        = AUD_INPUT_PATH_ASRMIC;
    kws_stream_cfg.vol            = 12;
    kws_stream_cfg.handler        = NULL;
    kws_stream_cfg.data_ptr       = NULL;
    kws_stream_cfg.data_size      = 0;
    TRACE(0,"111");
    static bool flag = true;
    if (flag) {
        // FIXME: Create a stream_cfg
        app_voice_assist_ai_voice_open(ASSIST_AI_VOICE_MODE_KWS, &kws_stream_cfg, NULL);
    } else {
        app_voice_assist_ai_voice_close();
    }

    flag = !flag;
#endif
}

#if defined(ANC_ASSIST_ENABLED)
int32_t app_voice_assist_wd_open(void);
int32_t app_voice_assist_wd_close(void);
#endif
static void audio_test_wd_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if defined(ANC_ASSIST_ENABLED)
    static bool flag = true;
    if (flag) {
        app_voice_assist_wd_open();
    } else {
        app_voice_assist_wd_close();
    }

    flag = !flag;
#endif
}

#if defined(ANC_ASSIST_ENABLED)
int32_t app_voice_assist_pnc_adapt_anc_open(void);
int32_t app_voice_assist_pnc_adapt_anc_close(void);
#endif

static void audio_test_pnc_adapt_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if defined(ANC_ASSIST_ENABLED)
    static bool flag = false;
    if (flag == false) {
        app_voice_assist_pnc_adapt_anc_open();
    } else {
        app_voice_assist_pnc_adapt_anc_close();
    }

    flag = !flag;
#endif
}

#if defined(ANC_ASSIST_ENABLED)
int32_t app_voice_assist_optimal_tf_anc_open(void);
int32_t app_voice_assist_optimal_tf_anc_close(void);
#endif

static void audio_test_optimal_tf_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if defined(ANC_ASSIST_ENABLED)
    app_voice_assist_optimal_tf_anc_open();
#endif
}

extern void switch_virtual_filt(uint32_t index);
static void audio_test_lms_vir(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    uint32_t ch = 0;
    sscanf(cmd, "%d", &ch);
    switch_virtual_filt(ch);
}

#ifdef VOICE_ASSIST_FF_FIR_LMS
#include "app_voice_assist_fir_lms.h"
#endif
static void audio_test_fir(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#ifdef VOICE_ASSIST_FF_FIR_LMS
    app_voice_assist_fir_lms_open();
#endif
}

extern void set_fir_lms_step(uint32_t step);
static void audio_test_set_lms_step(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    uint32_t ch = 0;
    sscanf(cmd, "%d", &ch);
#ifdef VOICE_ASSIST_FF_FIR_LMS
    set_fir_lms_step(ch);
#endif
}

WEAK void set_fir_cache(void){

};
static void audio_test_fir_cache(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    set_fir_cache();

}

// #include "anc_process.h"
extern int anc_print_cfg(enum ANC_TYPE_T anc_type);
extern int anc_set_gain(int32_t gain_ch_l, int32_t gain_ch_r,enum ANC_TYPE_T anc_type);
static void audio_test_set_anc_gain_ff(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    int32_t ch = 0;
    sscanf(cmd, "%d", &ch);
    anc_set_gain(ch, ch, ANC_FEEDFORWARD);
}
static void audio_test_set_anc_gain_fb(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    int32_t ch = 0;
    sscanf(cmd, "%d", &ch);
    anc_set_gain(ch, ch, ANC_FEEDBACK);
}
static void audio_test_set_anc_gain_tt(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    int32_t ch = 0;
    sscanf(cmd, "%d", &ch);
    anc_set_gain(ch, ch, ANC_TALKTHRU);
}

static struct_anc_fir_cfg fir_cfg = {
    .anc_fir_cfg_ff_l.fir_bypass_flag = 0,
    .anc_fir_cfg_ff_l.fir_len = 472,
    .anc_fir_cfg_ff_l.fir_coef[0] = 0.5 * 32767 * 256,
};

static void audio_test_set_fir_gain(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    int32_t gain = 0;
    sscanf(cmd, "%d", &gain);
    AUDIO_TEST_LOG_I("[%s] gain = %d", __func__, gain);
    fir_cfg.anc_fir_cfg_ff_l.fir_coef[0] = (int32_t)(powf(10.f, gain / 20.f) * 32767 * 256);

    anc_set_fir_cfg(&fir_cfg, ANC_FEEDFORWARD);
}

extern void set_leaky_gama(uint32_t gama);
static void audio_test_set_leaky_gama(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
    int32_t ch = 0;
    sscanf(cmd, "%d", &ch);
    set_leaky_gama(ch);
    // anc_print_cfg(ANC_FEEDBACK);
}

#if defined(SPEECH_BONE_SENSOR)
#include "speech_bone_sensor.h"
#endif
static void audio_test_bone_sensor_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if defined(SPEECH_BONE_SENSOR)
    static bool flag = true;
    if (flag) {
        speech_bone_sensor_open(16000, 120);
        speech_bone_sensor_start();
    } else {
        speech_bone_sensor_stop();
        speech_bone_sensor_close();
    }

    flag = !flag;
#endif
}

static void audio_test_prompt_switch(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#if 1//defined(ANC_ASSIST_ENABLED)
    int prompt_buf[6] = {14, 15, 16, 17, 18, 19};
    static int prompt_cnt = 0;

    AUD_ID_ENUM id_tmp = (AUD_ID_ENUM)prompt_buf[prompt_cnt];
    TRACE(1,"[%s],id = 0x%x",__func__,(int)id_tmp);
    audio_player_play_prompt(id_tmp, 0);

    prompt_cnt++;

    if(6<= prompt_cnt)
        prompt_cnt = 0;

#endif
}

extern int32_t bt_sco_chain_bypass_tx_algo(uint32_t sel_ch);
static void audio_test_bypass_tx_algo(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);

    uint32_t ch = 0;
    sscanf(cmd, "ch=%d", &ch);

    AUDIO_TEST_LOG_I("[%s] ch: %d", __func__, ch);
    bt_sco_chain_bypass_tx_algo(ch);
}

static void audio_test_input_param(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);

    int32_t val1 = 0;
    int32_t val2 = 0;
    sscanf(cmd, "val1=%d val2=%d", &val1, &val2);

    AUDIO_TEST_LOG_I("[%s] val1: %d, val2: %d", __func__, val1, val2);
}

static void audio_test_call_algo_cfg(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);

}

static void audio_test_hifi(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);

#if defined(AUDIO_TEST_HIFI)
    extern int32_t audio_test_hifi_tx_data(void);
    audio_test_hifi_tx_data();
#endif
}

static void audio_test_psap_sw(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);
#ifdef PSAP_SW_APP
    if(!strcmp(cmd, "on")){
        psap_sw_app_open();
    }else if(!strcmp(cmd, "off")){
        psap_sw_app_close();
    }else{
        AUDIO_TEST_LOG_I("invalid cmd");
    }
#else
    AUDIO_TEST_LOG_I("please define PSAP_SW_APP first!!!");
#endif
}

#ifdef PSAP_SW_APP
extern int32_t psap_sw_set_reverb(float *data);
#endif
static void audio_test_psap_reverb(const char *cmd)
{
    AUDIO_TEST_LOG_I(" %s...", __func__);
#ifdef PSAP_SW_APP
    uint32_t val[10];
    float data[10];
    sscanf(cmd, "%d, %d, %d, %d", &val[0], &val[1], &val[2], &val[3]);
    data[0] = (float)val[0];
    data[1] = (float)val[1];
    data[2] = (float)val[2];
    data[3] = (float)val[3];
    
    psap_sw_set_reverb(data);
#endif
}


/*
 * audio_test_dma_audio - The test command handler for DMA AUDIO APP.
 * The test command usage:
 *      audio_test:dma_audio on                  - enable dma audio app
 *      audio_test:dma_audio loop_on             - enable adda loop stream
 *      audio_test:dma_audio dac1_on             - enable dac1 stream
 *      audio_test:dma_audio dac2_on             - enable dac2 stream
 *      audio_test:dma_audio adc1_on             - enable adc1 stream

 *      audio_test:dma_audio off                 - disable dma audio app
 *      audio_test:dma_audio loop_off            - disable adda loop stream
 *      audio_test:dma_audio dac1_off            - disable dac1 stream
 *      audio_test:dma_audio dac2_off            - disable dac2 stream
 *      audio_test:dma_audio adc1_off            - disable dac2 stream
 *
 */
static void audio_test_dma_audio(const char *cmd)
{
    if (cmd == NULL) {
        AUDIO_TEST_LOG_I("null cmd");
        return;
    }
#ifdef DMA_AUDIO_APP
    AUDIO_TEST_LOG_I("[%s]: cmd: %s", __func__, cmd);

    if (!strcmp(cmd, "on")) {
        dma_audio_cli_key_on(DAUD_CLI_KEY_NORMAL);
    } else if (!strcmp(cmd, "off")) {
        dma_audio_cli_key_off(DAUD_CLI_KEY_NORMAL);
    } else if (!strcmp(cmd, "loop_on")) {
        dma_audio_cli_key_on(DAUD_CLI_KEY_TEST_LOOP);
    } else if (!strcmp(cmd, "loop_off")) {
        dma_audio_cli_key_off(DAUD_CLI_KEY_TEST_LOOP);
    } else if (!strcmp(cmd, "dac1_on")) {
        dma_audio_cli_key_on(DAUD_CLI_KEY_TEST_PLAY1);
    } else if (!strcmp(cmd, "dac1_off")) {
        dma_audio_cli_key_off(DAUD_CLI_KEY_TEST_PLAY1);
    } else if (!strcmp(cmd, "dac2_on")) {
        dma_audio_cli_key_on(DAUD_CLI_KEY_TEST_PLAY2);
    } else if (!strcmp(cmd, "dac2_off")) {
        dma_audio_cli_key_off(DAUD_CLI_KEY_TEST_PLAY2);
    } else if (!strcmp(cmd, "adc1_on")) {
        dma_audio_cli_key_on(DAUD_CLI_KEY_TEST_CAP1);
    } else if (!strcmp(cmd, "adc1_off")) {
        dma_audio_cli_key_off(DAUD_CLI_KEY_TEST_CAP1);
    } else {
        AUDIO_TEST_LOG_I("invalid cmd: %s", cmd);
        AUDIO_TEST_LOG_I("cmd usage: audio_test:dma_audio on");
        AUDIO_TEST_LOG_I("cmd usage: audio_test:dma_audio off");
    }
#else
    AUDIO_TEST_LOG_I("[%s]: dummy cmd: %s", __func__, cmd);
#endif

}

static void audio_test_psap_sw_ctrl(const char *cmd)
{
    AUDIO_TEST_LOG_I(" %s...", __func__);
#ifdef PSAP_SW_APP
    psap_sw_app_ctrl_test(cmd);
#endif
}

static void audio_test_switch_speech_algo_dsp_test(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);

#if defined(AUDIO_TEST_SPEECH_ALGO_DSP_TEST)
    extern int32_t audio_test_speech_algo_dsp_test(void);
    audio_test_speech_algo_dsp_test();
#endif
}

static void audio_test_switch_app_mcpp(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] %s", __func__, cmd);

#ifdef APP_MCPP_CLI
    extern int32_t audio_test_app_mcpp(const char *cmd);
    audio_test_app_mcpp(cmd);
#else
    AUDIO_TEST_LOG_I("please define APP_MCPP_CLI first!!!");
#endif
}

extern void set_adj_eq_state_audio(int val);

static void audio_test_switch_adj_eq(const char *cmd)
{
#ifdef AUDIO_ADJ_EQ
    static bool flag = true;
    if (flag) {
        set_adj_eq_state_audio(1);
        AUDIO_TEST_LOG_I("[%s]:open", __FUNCTION__);
        flag = 0;
    } else {
        set_adj_eq_state_audio(0);
        AUDIO_TEST_LOG_I("[%s]:close", __FUNCTION__);
        flag = 1;
    }
#endif
}

extern void audio_virtual_bass_switch(int val);
static void audio_test_virtual_bass_switch(const char *cmd)
{
#ifdef AUDIO_BASS_ENHANCER
    if (!strcmp(cmd, "on")) {
        audio_virtual_bass_switch(1);
        AUDIO_TEST_LOG_I("[%s]:open", __FUNCTION__);
    } else if (!strcmp(cmd, "off")) {
        audio_virtual_bass_switch(0);
        AUDIO_TEST_LOG_I("[%s]:close", __FUNCTION__);
    } else {
        AUDIO_TEST_LOG_I("invalid cmd: %s", cmd);
        AUDIO_TEST_LOG_I("cmd usage: audio_test:virtual_bass on");
        AUDIO_TEST_LOG_I("cmd usage: audio_test:virtual_bass off");
    }
#endif
}

#ifdef BLE_AUDIO_ENABLED
#ifdef AOB_MOBILE_ENABLED
static void aob_start_stream_for_audio_test_new(float frame_len, uint32_t playback_context, uint32_t playback_sample_rate, uint32_t capture_context, uint16_t capture_sample_rate)
{
    const bes_gaf_codec_id_t codec_id_lc3 = {{0x06}};

    bes_lea_ase_cfg_param_t ase_to_start[] =
    {
        {BES_BLE_GAF_SAMPLE_FREQ_16000, 40, BES_BLE_GAF_DIRECTION_SINK, &codec_id_lc3, BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT},
        {BES_BLE_GAF_SAMPLE_FREQ_16000, 40, BES_BLE_GAF_DIRECTION_SRC, &codec_id_lc3, BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT},
    };

    uint32_t sample_rate[BLE_AUDIO_CONNECTION_CNT];
    uint16_t context_type[BLE_AUDIO_CONNECTION_CNT];

    sample_rate[0] = playback_sample_rate;
    sample_rate[1] = capture_sample_rate;

    context_type[0] = (uint16_t)(1 << playback_context);
    context_type[1] = (uint16_t)(1 << capture_context);

    for (uint32_t i=0; i<BLE_AUDIO_CONNECTION_CNT; i++)
    {
        switch (sample_rate[i])
        {
            case (8000):
                ase_to_start[i].sample_rate = BES_BLE_GAF_SAMPLE_FREQ_8000;
            break;
            case (16000):
                ase_to_start[i].sample_rate = BES_BLE_GAF_SAMPLE_FREQ_16000;
            break;
            case (24000):
                ase_to_start[i].sample_rate = BES_BLE_GAF_SAMPLE_FREQ_24000;
            break;
            case (32000):
                ase_to_start[i].sample_rate = BES_BLE_GAF_SAMPLE_FREQ_32000;
            break;
            case (48000):
                ase_to_start[i].sample_rate = BES_BLE_GAF_SAMPLE_FREQ_48000;
            break;
            default:
            return;
        }

        ase_to_start[i].frame_octet = (uint16_t)(frame_len * sample_rate[i] / 1000 / 4);
        ase_to_start[i].context_type = context_type[i];
    }

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        bes_lea_mobile_stream_start(i, ase_to_start);
    }
}

static void aob_start_stream_for_audio_test(uint32_t playback_context, uint32_t playback_sample_rate, uint32_t capture_context, uint16_t capture_sample_rate)
{
    aob_start_stream_for_audio_test_new(10, playback_context, playback_sample_rate, capture_context, capture_sample_rate);
}
#endif
#endif

static void audio_test_le_stream_start(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);

#ifdef BLE_AUDIO_ENABLED
#ifdef AOB_MOBILE_ENABLED
    uint32_t playback_sample_rate = 0;
    uint32_t capture_sample_rate = 0;
    uint32_t playback_context = 0;
    uint32_t capture_context = 0;
    sscanf(cmd, "%d:%d;%d:%d", &playback_context, &playback_sample_rate, &capture_context, &capture_sample_rate);

    AUDIO_TEST_LOG_I("[%s] playback_sample_rate: %d, capture_sample_rate: %d, capture_context %d", __func__, playback_sample_rate, capture_sample_rate, capture_context);

    aob_start_stream_for_audio_test(playback_context, playback_sample_rate, capture_context, capture_sample_rate);
#endif
#endif
}

void audio_test_set_anc_FF_sample_rate_192K_test(const char *cmd)
{
    anc_set_sample_rate(ANC_FF_SAMPLE_RATE_192k);
}

#ifdef __AUDIO_DYNAMIC_BOOST__
extern int switch_cfg(void);
#endif

static void audio_test_dynamic_boost(const char *cmd)
{
    AUDIO_TEST_LOG_I("[%s] ...", __func__);
#ifdef __AUDIO_DYNAMIC_BOOST__
    switch_cfg();
#endif

}


const audio_test_func_t audio_test_func[]= {
    {"anc_switch",          audio_test_anc_switch},
    {"stream_switch",       audio_test_stream_switch},
    {"anc_assist_switch",   audio_test_anc_assist_switch},
    {"kws_switch",          audio_test_kws_switch},
    {"wd_switch",           audio_test_wd_switch},
    {"anc_gain_switch",     audio_test_anc_gain_switch},
    {"bone_sensor_switch",  audio_test_bone_sensor_switch},
    {"prompt_switch",       audio_test_prompt_switch},
    {"bypass_tx_algo",      audio_test_bypass_tx_algo},
    {"input_param",         audio_test_input_param},
    {"call_algo_cfg",       audio_test_call_algo_cfg},
    {"hifi",                audio_test_hifi},
    {"app_mcpp",            audio_test_switch_app_mcpp},
    {"speech_algo_dsp_test",audio_test_switch_speech_algo_dsp_test},
    {"adj_eq_switch",       audio_test_switch_adj_eq},
    {"psap_sw",             audio_test_psap_sw},
    {"psap_reverb",         audio_test_psap_reverb},
    {"dma_audio",           audio_test_dma_audio},
    {"le_stream_start",     audio_test_le_stream_start},
    {"psap_sw_ctrl",        audio_test_psap_sw_ctrl},
	{"virtual_surround_on",     audio_test_virtual_surround_on},
    {"virtual_surround_off",    audio_test_virtual_surround_off},
    {"virtual_surround_test_yaw",      audio_test_virtual_surround_test_yaw},
    {"virtual_surround_test_pitch",    audio_test_virtual_surround_test_pitch},
    {"Set_ANC_FF_sample_rate_192K",    audio_test_set_anc_FF_sample_rate_192K_test},
    {"dynamic_bass_boost",  audio_test_dynamic_boost},
    {"virtual_bass",        audio_test_virtual_bass_switch},
    {"pnc_adapt_switch",    audio_test_pnc_adapt_switch},
    {"optimal_tf_switch",   audio_test_optimal_tf_switch},
    {"lms_virt_switch",     audio_test_lms_vir},
    {"fir_open_switch",     audio_test_fir},
    {"set_lms_step_switch", audio_test_set_lms_step},
    {"fir_cache_switch",    audio_test_fir_cache},
    {"set_ff_gain_switch",  audio_test_set_anc_gain_ff},
    {"set_fb_gain_switch",  audio_test_set_anc_gain_fb},
    {"set_tt_gain_switch",  audio_test_set_anc_gain_tt},
    {"set_fir_gain_switch", audio_test_set_fir_gain},
    {"set_leaky_gama_switch", audio_test_set_leaky_gama},
};

static uint32_t audio_test_cmd_callback(uint8_t *buf, uint32_t len)
{
    uint32_t index = 0, i;
    char *cmd = (char *)buf;
    func_handler_t func_handler = NULL;

    //TRACE(1, "[%s]: cmd=%s, len=%d, strlen(cmd)=%d", __func__, cmd, len, strlen(cmd));

    // filter ' ' before function
    for (i = 0; i < strlen(cmd); i++) {
        if (cmd[i] != ' ') {
            cmd += i;
            break;
        }
    }
    //TRACE(1, "cmd=%s, strlen(cmd)=%d", cmd, strlen(cmd));

    // Separate function and value with ' '
    for (index = 0; index < strlen(cmd); index++) {
        if (cmd[index] == ' ' || cmd[index] == '\r' || cmd[index] == '\n') {
            break;
        }
    }

    for (i = 0; i < ARRAY_SIZE(audio_test_func); i++) {
        if (strncmp((char *)cmd, audio_test_func[i].name, index) == 0) {
            func_handler = audio_test_func[i].handler;
            break;
        }
    }

    // filter ' ' before value
    for (; index<strlen(cmd); index++) {
        if (cmd[index] != ' ') {
            break;
        }
    }

    if (strncmp(cmd + index, "bin ", strlen("bin ")) == 0) {
        index += strlen("bin ");
    }

    if (func_handler) {
        func_handler(cmd + index);
        AUDIO_TEST_LOG_I("[audio_test] cmd: OK!");
        TRACE(0, "[CMD] res : 0;");
        app_debug_tool_printf("cmd, Process %s", cmd);
    } else {
        AUDIO_TEST_LOG_I("[audio_test] cmd: Can not found cmd: %s", cmd);
        TRACE(0, "[CMD] res : 1; info : Can not found cmd(%s);", cmd);
        app_debug_tool_printf("cmd, Invalid %s", cmd);
    }

    return 0;
}

int32_t audio_test_cmd_init(void)
{
    app_trace_rx_register("audio_test", audio_test_cmd_callback);

#if defined(AUDIO_TEST_HIFI)
    extern int32_t audio_test_hifi_init(void);
    audio_test_hifi_init();
#endif

    return 0;
}

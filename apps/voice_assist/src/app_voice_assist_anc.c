/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#include "hal_trace.h"
#include "plat_types.h"
#include "anc_assist.h"
#include "app_anc_assist.h"

#ifdef ANC_ASSIST_ENABLED

extern void app_anc_assist_set_res(AncAssistRes *res);

#ifdef ANC_ASSIST_ON_DSP
static int32_t _assist_anc_dsp_result_callback(void *buf, uint32_t len, void *other, uint32_t sub_cmd);
#endif

int32_t app_voice_assist_anc_dsp_init(void)
{
#ifdef ANC_ASSIST_ON_DSP
    app_anc_assist_result_register(ANC_ASSIST_USER_ALGO_DSP, _assist_anc_dsp_result_callback);
#endif
    return 0;
}

#ifdef ANC_ASSIST_ON_DSP
static int32_t _assist_anc_dsp_result_callback(void *buf, uint32_t len, void *other, uint32_t sub_cmd)
{
   AncAssistRes *anc_assist_res = (AncAssistRes *)buf;
   app_anc_assist_set_res(anc_assist_res);
 
   return 0;  
}
#endif
#endif

#ifdef ANC_ALGO_DSP
#include <string.h>
#include "anc_assist_dsp.h"
#include "app_anc_assist.h"
#include "app_dsp_m55.h"
#include "app_anc_assist_thirdparty.h"

#define _SAMPLE_BITS        (24)
#ifndef FREEMAN_ENABLED_STERO
#define _CHANNEL_NUM    (4)
#else
#define _CHANNEL_NUM    (8)
#endif
#if defined(ASSIST_LOW_RAM_MOD)
#define ALGO_SAMPLE_RATE    (8000)
#define ALGO_FRAME_LEN      (60)
#else
#define ALGO_SAMPLE_RATE    (16000)
#define ALGO_FRAME_LEN      (120)
#endif

static uint8_t g_ff_ch_num = MAX_FF_CHANNEL_NUM;
static uint8_t g_fb_ch_num = MAX_FB_CHANNEL_NUM;
static AncAssistState *anc_assist_st;
static AncAssistRes anc_assist_res;
static int32_t g_sample_bits = _SAMPLE_BITS;
static anc_assist_mode_t g_sys_stream_mode = ANC_ASSIST_MODE_NONE;
extern AncAssistConfig anc_assist_cfg;

uint8_t is_sco_mode(void)
{
    return false;
}

uint8_t amgr_is_bluetooth_sco_on()
{
    return false;;
}

static int assist_algo_dsp_sync_cfg(uint8_t *cfg, uint32_t cfg_len)
{
    ASSERT(cfg_len == sizeof(assist_enable_list_cfg_t), "[%s] wrong cfg len %d != %d", __func__, cfg_len, sizeof(assist_enable_list_cfg_t));

    memcpy(&anc_assist_cfg.enable_list_cfg, cfg, cfg_len);

  //  TRACE(2, "[%s] anc mode: %d", __func__, anc_assist_cfg.enable_list_cfg.extern_adj_eq_en);

    // tmp for pilot
    if(anc_assist_cfg.enable_list_cfg.custom_leak_en == 1){
        anc_assist_cfg.pilot_cfg.custom_leak_detect_en = 1;
    } else {
        anc_assist_cfg.pilot_cfg.custom_leak_detect_en = 0;
    }
//    anc_assist_cfg.pilot_cfg.adaptive_anc_en = anc_assist_cfg.enable_list_cfg.adaptive_anc_en;
//    anc_assist_cfg.pilot_cfg.wd_en = anc_assist_cfg.enable_list_cfg.wd_en;
   

    if(anc_assist_st != NULL){
        anc_assist_set_cfg_sync(anc_assist_st, &anc_assist_cfg);
    } 
    return 0;
}


static int assist_algo_m55_custom_leak_ctrl(bool onoff)
{
    TRACE(2, "[%s] onoff: %d", __func__, onoff);
    anc_assist_set_custom_leak_working_status(onoff);

    return 0;
} 

int assist_algo_dsp_open()
{
    //TRACE(2, "[%s] %s", __func__, anc_assist_get_version());
    anc_assist_st = anc_assist_create(ALGO_SAMPLE_RATE, g_sample_bits, _CHANNEL_NUM, ALGO_FRAME_LEN, &anc_assist_cfg, NULL);

    return 0;
}

int assist_algo_dsp_close(void)
{
    TRACE(1, "[%s] ...", __func__);
    anc_assist_destroy(anc_assist_st);
    anc_assist_st = NULL;

    return 0;
}

int assist_algo_dsp_get_freq(void)
{
#if 0
    TRACE(4, "[%s] mode: %d, wind: %d, pilot: %d",
        __func__,
        g_sys_stream_mode,
        anc_assist_cfg.enable_list_cfg.wind_en,
        anc_assist_cfg.enable_list_cfg.pilot_en);
#endif

  /*  if ((g_sys_stream_mode == ANC_ASSIST_MODE_PHONE_CALL) &&
        (anc_assist_cfg.enable_list_cfg.wind_en == 0) &&
        (anc_assist_cfg.enable_list_cfg.pilot_en == 0)) {

        TRACE(2, "[%s] Opt MIPS for call", __func__);
        return 0;
    } else { */
        return 12;
  //  }
}

int assist_algo_dsp_set_mode(uint32_t mode)
{
    TRACE(2, "[%s] mode: %d", __func__, mode);
    g_sys_stream_mode = mode;
    return 0;
}

int assist_algo_dsp_process(process_frame_data_t *process_frame_data)
{
    int loop_cnt = 1;
    int offset = 0;

    ASSERT(((process_frame_data->frame_len % 120) == 0), "[%s] the 120 data_len is error ", __func__);
    loop_cnt = process_frame_data->frame_len / 120;
    for (int i = 0; i < loop_cnt; i++) {
        float *ff_mic_buf[MAX_FF_CHANNEL_NUM];
        float *fb_mic_buf[MAX_FB_CHANNEL_NUM];
        float *talk_mic_buf[MAX_TALK_CHANNEL_NUM];
        float *ref_mic_buf[MAX_REF_CHANNEL_NUM];

        for (uint8_t i = 0; i < process_frame_data->ff_ch_num; i++) {
            ff_mic_buf[i] = process_frame_data->ff_mic[i] + offset;
        }

        for (uint8_t i = 0; i < process_frame_data->fb_ch_num; i++) {
            fb_mic_buf[i] = process_frame_data->fb_mic[i] + offset;
        }

        for (uint8_t i = 0; i < process_frame_data->talk_ch_num; i++) {
            talk_mic_buf[i] = process_frame_data->talk_mic[i] + offset;
        }

        for (uint8_t i = 0; i < process_frame_data->ref_ch_num; i++) {
            ref_mic_buf[i] = process_frame_data->ref[i] + offset;
        }

        anc_assist_res = anc_assist_process(anc_assist_st, ff_mic_buf, process_frame_data->ff_ch_num,
                                            fb_mic_buf, process_frame_data->fb_ch_num, talk_mic_buf, process_frame_data->talk_ch_num,
                                            ref_mic_buf, process_frame_data->ref_ch_num, ALGO_FRAME_LEN);
        offset += ALGO_FRAME_LEN;
        if (any_of_u32(anc_assist_res.ff_gain_changed, g_ff_ch_num, ANC_ASSIST_ALGO_STATUS_CHANGED) ||
            any_of_u32(anc_assist_res.fb_gain_changed, g_fb_ch_num, ANC_ASSIST_ALGO_STATUS_CHANGED) || 
            anc_assist_res.custom_leak_detect_status > 0 ||
            anc_assist_res.curve_changed[0] ||
            anc_assist_res.fir_anc_wear_leak_changed > 0) {
            anc_assist_dsp_send_result_to_bth(ANC_ASSIST_USER_ALGO_DSP, (uint8_t *)&anc_assist_res, sizeof(anc_assist_res), 0);
        }
    }

    return 0;
}

int assist_algo_dsp_ctrl(uint32_t ctrl, uint8_t *ptr, uint32_t ptr_len)
{
    TRACE(3, "[%s] ctrl: %d, len: %d", __func__, ctrl, ptr_len);
    switch (ctrl) {
        case ASSIST_CTRL_CFG_SYNC:
            assist_algo_dsp_sync_cfg(ptr,ptr_len);
            break;

        case ASSIST_CTRL_CUSTOM_LEAK_CTRL_ON:
            assist_algo_m55_custom_leak_ctrl(true);
            break;

        case ASSIST_CTRL_CUSTOM_LEAK_CTRL_OFF:
            assist_algo_m55_custom_leak_ctrl(false);
            break;
        case ASSIST_CTRL_CUSTOM_LEAK_CTRL_RESET:
            // anc_assist_pilot_status_reset(anc_assist_st, &anc_assist_cfg);
            break;

        default:
            ASSERT(0, "[%s] ctrl (%d) is invalid", __func__, ctrl);
            break;
    }
    return 0;

}

int assist_algo_dsp_reset(void)
{
    TRACE(0, "[%s] assist_algo_dsp reset open...", __func__);
    // TODO ie. fix when switching standalone and phone

    // follow bth method
    anc_assist_destroy(anc_assist_st);
    anc_assist_st = NULL;
    anc_assist_st = anc_assist_create(ALGO_SAMPLE_RATE, g_sample_bits, _CHANNEL_NUM, ALGO_FRAME_LEN, &anc_assist_cfg, NULL);

    return 0;
}


app_voice_assist_dsp_t assist_algo_dsp = {
    .open       = assist_algo_dsp_open,
    .close      = assist_algo_dsp_close,
    .process    = assist_algo_dsp_process,
    .ctrl       = assist_algo_dsp_ctrl,
    .reset      = assist_algo_dsp_reset,
    .get_freq   = assist_algo_dsp_get_freq,
    .set_mode   = assist_algo_dsp_set_mode,
};

int32_t assist_algo_dsp_init(void)
{
    anc_assist_dsp_register(ANC_ASSIST_USER_ALGO_DSP, &assist_algo_dsp, ANC_ASSIST_USER_FS_16K);

    return 0;
}

#endif

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
#include "hal_trace.h"
#include "anc_assist.h"
#include "app_anc.h"
#include "app_anc_assist.h"
#include "app_voice_assist_pilot_anc.h"
#include "cmsis_os.h"
#include "app_utils.h"
#include "apps.h"
#include "app_anc_utils.h"

#define ANC_ASSIST_TIMING_PEROID_MS (1000 * 60)
extern uint8_t is_sco_mode (void);
extern bool assist_speak_to_chat_opened();
static void pilot_anc_handler(void const *param);
osTimerDef(pilot_anc, pilot_anc_handler);
static osTimerId pilot_anc_id = NULL;
static int32_t sync_cnt = 0;
static int32_t last_stop_flag = 0;
static bool pilot_anc_opened = false;
static bool pilot_anc_direct_close = false;

POSSIBLY_UNUSED static float leak_gain[4] = {
    1.f,       // 0dB
    1.12,      // 1dB
    1.41,      // 3dB
    1.0,      // 0dB
};

static int32_t _voice_assist_pilot_anc_callback(void *buf, uint32_t len, void *other);

static void app_voice_assist_pilot_anc_checker_start(void)
{
    pilot_anc_id = osTimerCreate(osTimer(pilot_anc), osTimerPeriodic, NULL);
}

// bool bt_media_cur_is_bt_stream_music(void);
// bool pilot_anc_boost_freq = false;
static int32_t app_voice_assist_pilot_anc_open_impl(void)
{
    // if (1)//(bt_media_cur_is_bt_stream_music() == true)
    // {
    //     app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, APP_SYSFREQ_52M);
    //     pilot_anc_boost_freq = true;
    // }
    pilot_anc_opened = true;
    app_anc_assist_open(ANC_ASSIST_USER_PILOT_ANC);

    sync_cnt = 0;
    last_stop_flag = 0;
    TRACE(0, "[%s] pilot anc open stream", __func__);
    return 0;
}

static int32_t app_voice_assist_pilot_anc_close_impl(void)
{
    app_anc_assist_close(ANC_ASSIST_USER_PILOT_ANC);
    pilot_anc_opened = false;
    TRACE(0, "[%s] pilot anc close stream", __func__);
    return 0;
}

static void pilot_anc_handler(void const *param)
{
    TRACE(0, "[%s]", __FUNCTION__);

    if (app_anc_work_status()
    && !is_sco_mode()
    )
    {
        app_voice_assist_pilot_anc_open_impl();
    }
}

bool is_pilot_anc_direct_close()
{
    return pilot_anc_direct_close;
}

void set_pilot_anc_direct_close(bool set)
{
    pilot_anc_direct_close = set;
}


bool is_pilot_anc_opened()
{
    return pilot_anc_opened;
}


int32_t app_voice_assist_pilot_anc_init(void)
{
    app_anc_assist_register(ANC_ASSIST_USER_PILOT_ANC, _voice_assist_pilot_anc_callback);
    app_voice_assist_pilot_anc_checker_start();

    return 0;
}

int32_t app_voice_assist_pilot_anc_open(void)
{
    if (pilot_anc_id != NULL) {
        TRACE(0,"[%s]...",__func__);
        osTimerStart(pilot_anc_id, ANC_ASSIST_TIMING_PEROID_MS);
    }else {
        TRACE(0,"[%s]WARNING:pilot_anc_id is NULL!!!!!!",__func__);
    }

    return 0;
}

int32_t app_voice_assist_pilot_anc_close(void)
{
    TRACE(0,"[%s]...",__func__);
    if (pilot_anc_opened) {
        app_voice_assist_pilot_anc_close_impl();
    }
    osTimerStop(pilot_anc_id);
    return 0;
}

static int32_t _voice_assist_pilot_anc_callback(void *buf, uint32_t len, void *other)
{
#if defined(FREEMAN_ENABLED_STERO)
    sync_cnt++;
    AncAssistRes *assist_res = (AncAssistRes *)other;
    // TRACE(0, "last_stop_flag %d, changed %d", last_stop_flag, assist_res->curve_changed[0]);

    if (pilot_anc_direct_close == true)
    {
        TRACE(1, "pilot anc direct close.");
        app_voice_assist_pilot_anc_close_impl();
        pilot_anc_direct_close = false;
        return 0;
    }

    // close pilot anc when pilot stops
    if (sync_cnt > 4 && last_stop_flag < 1001) {
        if (last_stop_flag == 1000 || (assist_res->curve_changed[0] == 1 && assist_res->curve_changed[1] == 1)) {
            TRACE(0,"!!!!!!!!!!!!!!!!!!!!!!!! current state_l is %d state_r is %d", assist_res->curve_index[0], assist_res->curve_index[1]);
            TRACE(0, "[%s] pilot finished", __FUNCTION__);
            app_voice_assist_pilot_anc_close_impl();
            last_stop_flag = 1000;
        }

        if (assist_res->curve_changed[0] == 1 && assist_res->curve_changed[1] == 1 && assist_res->curve_id[0] == ANC_ASSIST_ALGO_ID_PILOT) {
            //to set gain
            TRACE(0,"!!!!!!!!!!!!!!!!!!!!!!!! current state_l is %d state_r is %d,to set gain", assist_res->curve_index[0], assist_res->curve_index[0]);
            // app_anc_set_gain_f32(ANC_GAIN_USER_ANC_ASSIST, ANC_FEEDFORWARD, leak_gain[assist_res->curve_index[0]], leak_gain[assist_res->curve_index[0]]);
        }

        last_stop_flag++;
    }
#endif
    return 0;
}


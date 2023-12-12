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

#include "hal_trace.h"
#include "anc_assist.h"
#include "app_anc_assist.h"
#include "app_voice_assist_wsd.h"
#include "app_anc.h"
#include "cmsis_os.h"
#include "cmsis.h"

static int32_t _voice_assist_wsd_callback(void *buf, uint32_t len, void *other);

static short last_wsd_flag = 0;
static app_anc_mode_t last_anc_mode = APP_ANC_MODE_QTY;

static void voice_assist_wsd_timer_cb(void const *n);
osTimerDef(VOICE_ASSIST_WSD_TIMER, voice_assist_wsd_timer_cb);
osTimerId voice_assist_wsd_timer_id = NULL;

static void voice_assist_wsd_timer_cb(void const *n)
{
    // app_anc_switch(last_anc_mode);
    TRACE(1, "[%s]", __FUNCTION__);
}

int32_t app_voice_assist_wsd_init(void)
{
    app_anc_assist_register(ANC_ASSIST_USER_WSD, _voice_assist_wsd_callback);

    if (voice_assist_wsd_timer_id == NULL){
        voice_assist_wsd_timer_id = osTimerCreate(osTimer(VOICE_ASSIST_WSD_TIMER), osTimerOnce, NULL);
    }

    return 0;
}

int32_t app_voice_assist_wsd_open(void)
{
    app_anc_assist_open(ANC_ASSIST_USER_WSD);

    last_wsd_flag = 0;
    last_anc_mode = app_anc_get_curr_mode();

    return 0;
}

int32_t app_voice_assist_wsd_close(void)
{
    app_anc_assist_close(ANC_ASSIST_USER_WSD);

    return 0;
}

static int32_t _voice_assist_wsd_callback(void *buf, uint32_t len, void *other)
{
    uint32_t *res = (uint32_t *)buf;
    short wsd_flag = res[0];

    /*
     * when wsd 0 -> 1, switch to TT
     * when wsd 1 -> 0, start timer, when timer arrive, switch to ANC. if wsd change to 1 before timer, just stop timer
     */
    if (wsd_flag != last_wsd_flag) {
        last_wsd_flag = wsd_flag;
        TRACE(0, "[%s] Change to %d", __func__, wsd_flag);
        // if (wsd_flag == true) {
        //     if (app_anc_get_curr_mode() != APP_ANC_MODE2) {
        //         last_anc_mode = app_anc_get_curr_mode();
        //         // app_anc_switch(APP_ANC_MODE2);
        //     }
        //     osTimerStop(voice_assist_wsd_timer_id);
        // } else {
        //     osTimerStart(voice_assist_wsd_timer_id, 10*1000);
        // }
    }

    return 0;
}


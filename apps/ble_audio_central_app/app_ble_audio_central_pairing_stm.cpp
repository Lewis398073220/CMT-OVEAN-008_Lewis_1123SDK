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
#include "bluetooth_bt_api.h"
#include "bluetooth_ble_api.h"
#include "app_bt_func.h"
#include "ble_audio_dbg.h"
#include "ble_audio_core.h"
#include "ble_audio_mobile_info.h"
#include "app_ble_audio_central_stream_stm.h"
#include "bes_aob_api.h"
#include "bes_gap_api.h"
#include "list.h"
#include "app_ble_audio_central_pairing_stm.h"
#include "bt_drv_interface.h"
#include "nvrecord_ble.h"

#ifdef AOB_MOBILE_ENABLED
#define INVALID_LID         (0xFF)

static ble_audio_central_pairing_stm_t ble_audio_central_pairing_stm;
static ble_bdaddr_t ble_audio_dongle_peer_addr;

static void app_ble_audio_central_start_scan_reconn_alternate_handler(void const *param);
osTimerDef (BLE_AUDIO_CENTRAL_PAIRING_START, app_ble_audio_central_start_scan_reconn_alternate_handler);
static osTimerId app_ble_audio_central_alternate_start_reconn_scan_timer = NULL;
#define BLE_AUDIO_CENTRAL_START_ALTERNATE_RECONN_SCAN_TIME_MS          20000

static void app_ble_audio_central_scan_timer_over_handler(void const *param);
osTimerDef (BLE_AUDIO_CENTRAL_SCAN_NEW_HEADSET_START, app_ble_audio_central_scan_timer_over_handler);
static osTimerId app_ble_audio_central_alternate_start_scan_timer = NULL;
#define BLE_AUDIO_CENTRAL_START_ALTERNATE_SCAN_TIME_MS  (ble_audio_get_core_config()->ble_audio_central_start_alternate_scan_time_ms)

static void app_ble_audio_central_reconn_timer_over_handler(void const *param);
osTimerDef (BLE_AUDIO_CENTRAL_RECONN_OLD_HEADSET_START, app_ble_audio_central_reconn_timer_over_handler);
static osTimerId app_ble_audio_central_alternate_start_reconn_timer = NULL;
#define BLE_AUDIO_CENTRAL_CAPTURE_START_ALTERNATE_RECONN_TIME_MS (ble_audio_get_core_config()->ble_audio_central_start_alternate_reconn_time_ms)

static const char* ble_audio_central_pairing_event_to_string(int event)
{
    #define CASES(s) case s:return "["#s"]";

    switch(event)
    {
        CASES(BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START)
        CASES(BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START)
        CASES(BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_STOP)
        CASES(BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_STOP)
        CASES(BLE_AUDIO_CENTRAL_REQ_SCAN_CONNECT_ALTERNATE_HEADSET_START)
        CASES(START_EVT)
        CASES(ENTRY_EVT)
        CASES(EXIT_EVT)
    }
    return "[UNDEFINE EVENT]";
}

POSSIBLY_UNUSED static void app_ble_audio_central_start_pairing_handler(void const *param)
{
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START, 0, 0);
}

static void app_ble_audio_central_start_scan_reconn_alternate_handler(void const *param)
{
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_SCAN_CONNECT_ALTERNATE_HEADSET_START, 0, 0);
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START, 0, 0);
}

POSSIBLY_UNUSED static void gaf_ble_audio_central_scan_start(void)
{
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START, 0, 0);
}

POSSIBLY_UNUSED static void gaf_ble_audio_central_scan_stop(void)
{
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_STOP, 0, 0);
}

POSSIBLY_UNUSED static void app_ble_audio_central_start_reconn_handler(void)
{
        ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START, 0, 0);
        ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START, 0, 0);
}

void gaf_ble_audio_central_reconn_start(void)
{
    LOG_D("%s", __func__);
    app_ble_audio_central_start_reconn_handler();
    osTimerStart(app_ble_audio_central_alternate_start_reconn_scan_timer, BLE_AUDIO_CENTRAL_START_ALTERNATE_RECONN_SCAN_TIME_MS);
}

void gaf_ble_audio_central_reconn_stop(void)
{
    LOG_D("%s", __func__);
    osTimerStop(app_ble_audio_central_alternate_start_reconn_scan_timer);
}

static void app_ble_audio_central_scan_timer_over_handler(void const *param)
{
    LOG_D("%s", __func__);
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START, 0, 0);
}

static void app_ble_audio_central_start_scan_alternate_handler(void const *param)
{
    LOG_D("%s", __func__);
    osTimerStart(app_ble_audio_central_alternate_start_scan_timer, BLE_AUDIO_CENTRAL_START_ALTERNATE_SCAN_TIME_MS);
}

static void app_ble_audio_central_reconn_timer_over_handler(void const *param)
{
    LOG_D("%s", __func__);
    ble_audio_central_pairing_send_message(ble_audio_central_get_pairing_sm(), BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START, 0, 0);
}

static void app_ble_audio_central_start_reconn_alternate_handler(void const *param)
{
    LOG_D("%s", __func__);
    osTimerStart(app_ble_audio_central_alternate_start_reconn_timer, BLE_AUDIO_CENTRAL_CAPTURE_START_ALTERNATE_RECONN_TIME_MS);
}

Msg const* ble_audio_central_pairiing_sm_super_state(ble_audio_central_pairing_stm_t *me, Msg *msg)
{
    LOG_I("LEA_CENTRAL-PAIRING: SUPER state on event: %s", ble_audio_central_pairing_event_to_string(msg->evt));
    switch (msg->evt)
    {
        case START_EVT:
            STATE_START(me, &me->idle);
            return 0;
        case ENTRY_EVT:
            return 0;
        case EXIT_EVT:
            return 0;
    }

    return msg;
}

Msg const* ble_audio_central_pairing_sm_idle_state(ble_audio_central_pairing_stm_t *me, Msg *msg)
{
    LOG_I("LEA_CENTRAL-PAIRING: IDLE State on event: %s", ble_audio_central_pairing_event_to_string(msg->evt));
    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START:
            STATE_TRAN(me, &me->reconn_old_headset_start);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START:
             STATE_TRAN(me, &me->scan_new_headset_start);
        return 0;
        case EXIT_EVT:
            return 0;
    }

    return msg;
}

Msg const* ble_audio_central_scan_new_headset_start_state(ble_audio_central_pairing_stm_t *me, Msg *msg)
{
    LOG_I("LEA_CENTRAL-PAIRING: scan state on event: %s", ble_audio_central_pairing_event_to_string(msg->evt));

    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START:
        {
            bes_ble_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);
            bes_ble_scan_param_t scan_param = {0};

            scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
            scan_param.scanWindowMs   = 20;
            scan_param.scanIntervalMs   = 50;
            bes_ble_gap_start_scan(&scan_param);
            return 0;
        }
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_STOP:
            bes_ble_gap_stop_scan();
            gaf_ble_audio_central_reconn_stop();
            STATE_TRAN(me, &me->reconn_old_headset_start);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_STOP:
            return 0;
        case EXIT_EVT:
            return 0;
    }

    return msg;
}

Msg const* ble_audio_central_reconn_old_headset_start_state(ble_audio_central_pairing_stm_t *me, Msg *msg)
{
    LOG_I("LEA_CENTRAL-PAIRING: reconn state on event: %s", ble_audio_central_pairing_event_to_string(msg->evt));

    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START:
            bes_ble_mobile_start_connect();
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START:
            STATE_TRAN(me, &me->scan_new_headset_start);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_STOP:
            gaf_ble_audio_central_reconn_stop();
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_STOP:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_CONNECT_ALTERNATE_HEADSET_START:
            STATE_TRAN(me, &me->scan_reconn_alternate_start);
            return 0;
        case EXIT_EVT:
            return 0;
    }

    return msg;
}

Msg const* ble_audio_central_scan_reconn_alternate_start_state(ble_audio_central_pairing_stm_t *me, Msg *msg)
{
    LOG_I("LEA_CENTRAL-PAIRING: reconn scan state on event: %s", ble_audio_central_pairing_event_to_string(msg->evt));

    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START:
            ble_audio_mobile_conn_sm_send_event_by_conidx(0xff, EVT_BLE_RECONNECTING_CANCELLED, 0, 0);
            bes_ble_gap_stop_scan();
            memcpy(ble_audio_dongle_peer_addr.addr, nvrecord_find_arbitrary_peer_ble_device_address(), BTIF_BD_ADDR_SIZE);
            ble_audio_dongle_peer_addr.addr_type = APP_GAPM_GEN_RSLV_ADDR;
            bes_ble_audio_mobile_reconnect_dev(&ble_audio_dongle_peer_addr);
            app_ble_audio_central_start_reconn_alternate_handler(NULL);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START:
        {
            bes_ble_gap_cancel_connecting();
            bes_ble_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);
            bes_ble_scan_param_t scan_param = {0};

            scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
            scan_param.scanWindowMs = 20;
            scan_param.scanIntervalMs = 50;
            bes_ble_gap_start_scan(&scan_param);
            app_ble_audio_central_start_scan_alternate_handler(NULL);
            return 0;
        }
        case BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_STOP:
        {
            bes_ble_gap_stop_scan();
            gaf_ble_audio_central_reconn_stop();
            osTimerStop(app_ble_audio_central_alternate_start_scan_timer);
            osTimerStop(app_ble_audio_central_alternate_start_reconn_timer);
            STATE_TRAN(me, &me->reconn_old_headset_start);
            return 0;
        }
        case BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_STOP:
            return 0;
        case EXIT_EVT:
            return 0;
    }

    return msg;
}

int ble_audio_central_pairing_sm_on_event(uint32_t pairing_stm_ptr,uint32_t event,uint32_t param0,uint32_t param1)
{
    Msg message;
    ble_audio_central_pairing_stm_t *p_pairing_sm = NULL;

    message.evt = event;
    message.param0 = param0;
    message.param1 = param1;
    message.param2 = 0;

    p_pairing_sm = (ble_audio_central_pairing_stm_t*)pairing_stm_ptr;
    HsmOnEvent((Hsm *)p_pairing_sm, &message);

    return 0;
}

void ble_audio_central_pairing_send_message(ble_audio_central_pairing_stm_t *p_pairing_sm, BLE_AUDIO_CENTRAL_PAIRING_EVENT_E event, uint32_t param0, uint32_t param1)
{
    app_bt_call_func_in_bt_thread((uint32_t)p_pairing_sm,(uint32_t)event,param0,param1,(uint32_t)ble_audio_central_pairing_sm_on_event);
}

POSSIBLY_UNUSED ble_audio_central_pairing_stm_t* ble_audio_central_get_pairing_sm(void)
{
    ble_audio_central_pairing_stm_t* iter = &ble_audio_central_pairing_stm;

    return iter;

}

BLE_AUDIO_CENTRAL_PAIRING_STATE_E ble_audio_central_pairing_stm_get_cur_state(void)
{
    ble_audio_central_pairing_stm_t *p_pairing_sm = &ble_audio_central_pairing_stm;

    BLE_AUDIO_CENTRAL_PAIRING_STATE_E state = BLE_AUDIO_CENTRAL_PAIRING_STATE_UNKNOW;
    if (!p_pairing_sm->used) {
        return state;
    }

    if (p_pairing_sm->super.curr == &p_pairing_sm->idle)
    {
        state = BLE_AUDIO_CENTRAL_PAIRING_STATE_IDLE;
    } else if (p_pairing_sm->super.curr == &p_pairing_sm->scan_new_headset_start) {
        state = BLE_AUDIO_CENTRAL_PAIRING_STATE_SCAN_STARTED;
    } else if (p_pairing_sm->super.curr == &p_pairing_sm->reconn_old_headset_start) {
        state = BLE_AUDIO_CENTRAL_PAIRING_STATE_RECONN_STARTED;
    }

    return state;
}

void ble_audio_central_pairing_stm_startup(ble_audio_central_pairing_stm_t *ble_audio_central_pairing_sm)
{
    LOG_I("LEA_CENTRAL-STREAM: %s", __func__);
    ble_audio_central_pairing_stm.used = true;

    HsmCtor((Hsm *)ble_audio_central_pairing_sm, "stream_super", (EvtHndlr)ble_audio_central_pairiing_sm_super_state);
    AddState(&ble_audio_central_pairing_sm->idle, "idle",  &((Hsm *)ble_audio_central_pairing_sm)->top, (EvtHndlr)ble_audio_central_pairing_sm_idle_state);
    AddState(&ble_audio_central_pairing_sm->scan_new_headset_start, "scan_new_headset_start", &((Hsm *)ble_audio_central_pairing_sm)->top, (EvtHndlr)ble_audio_central_scan_new_headset_start_state);
    AddState(&ble_audio_central_pairing_sm->reconn_old_headset_start, "reconn_old_headset_start", &((Hsm *)ble_audio_central_pairing_sm)->top, (EvtHndlr)ble_audio_central_reconn_old_headset_start_state);
    AddState(&ble_audio_central_pairing_sm->scan_reconn_alternate_start, "scan_reconn_alternate_start", &((Hsm *)ble_audio_central_pairing_sm)->top, (EvtHndlr)ble_audio_central_scan_reconn_alternate_start_state);
    HsmOnStart((Hsm *)ble_audio_central_pairing_sm);
}

void ble_audio_central_pairing_stm_init(void)
{
    memset(&ble_audio_central_pairing_stm, 0xFF, sizeof(ble_audio_central_pairing_stm_t));
    ble_audio_central_pairing_stm.used = false;

    if (app_ble_audio_central_alternate_start_scan_timer == NULL) {
        app_ble_audio_central_alternate_start_scan_timer =
            osTimerCreate (osTimer(BLE_AUDIO_CENTRAL_SCAN_NEW_HEADSET_START), osTimerOnce, NULL);
    }

    if (app_ble_audio_central_alternate_start_reconn_timer == NULL) {
        app_ble_audio_central_alternate_start_reconn_timer =
            osTimerCreate (osTimer(BLE_AUDIO_CENTRAL_RECONN_OLD_HEADSET_START), osTimerOnce, NULL);
    }
}

void ble_audio_central_start_pairing_state_machine(void)
{
    if (app_ble_audio_central_alternate_start_reconn_scan_timer)
    {
        return;
    }

    ble_audio_central_pairing_stm_init();
    ble_audio_central_pairing_stm_t* ble_audio_central_pairing_stm_ptr = ble_audio_central_get_pairing_sm();
    if (NULL != ble_audio_central_pairing_stm_ptr) {
        ble_audio_central_pairing_stm_startup(ble_audio_central_pairing_stm_ptr);
    }

    if (app_ble_audio_central_alternate_start_reconn_scan_timer == NULL) {
        app_ble_audio_central_alternate_start_reconn_scan_timer =
            osTimerCreate (osTimer(BLE_AUDIO_CENTRAL_PAIRING_START), osTimerOnce, NULL);
    }

    gaf_ble_audio_central_reconn_start();
}
#endif //AOB_MOBILE_ENABLED


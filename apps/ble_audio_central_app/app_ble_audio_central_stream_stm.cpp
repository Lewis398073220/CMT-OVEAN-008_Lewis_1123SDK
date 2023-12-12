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
#ifdef AOB_MOBILE_ENABLED
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "ble_audio_dbg.h"
#include "ble_audio_core.h"
#include "ble_audio_mobile_info.h"
#include "app_ble_audio_central_stream_stm.h"
//#include "app_ble_usb_audio.h"
#include "aob_media_api.h"
#include "list.h"
#include "app_gaf_custom_api.h"

#define INVALID_LID         (0xFF)

static ble_audio_central_stream_stm_t ble_audio_central_stream_stm;

static const char* ble_audio_central_stream_sm_event_to_string(int event)
{
    #define CASES(s) case s:return "["#s"]";

    switch(event)
    {
        CASES(BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START)
        CASES(BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START)
        CASES(BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP)
        CASES(BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP)
        CASES(EVT_PLAYBACK_STREAM_STARTED)
        CASES(EVT_CAPTURE_STREAM_STARTED)
        CASES(EVT_PLAYBACK_STREAM_STOPPED)
        CASES(EVT_CAPTURE_STREAM_STOPPED)
        CASES(BLE_AUDIO_CENTRAL_REQ_REFRESH_PLAYBACK_STREAM)
        CASES(BLE_AUDIO_CENTRAL_REQ_REFRESH_CAPTURE_STREAM)
        CASES(START_EVT)
        CASES(ENTRY_EVT)
        CASES(EXIT_EVT)
    }
    return "[UNDEFINE EVENT]";
}

POSSIBLY_UNUSED static void ble_audio_capture_start_handler(uint8_t con_lid)
{
    LOG_I("%s", __func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
#ifdef GSBC_SUPPORT
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 48, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_GSBC,  BES_BLE_GAF_CONTEXT_TYPE_GAME_BIT},
#else
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  BES_BLE_GAF_CONTEXT_TYPE_GAME_BIT},
#endif
    };
    aob_media_mobile_start_stream(ase_to_start, con_lid, false);
}

static void ble_audio_playback_start_handler(uint8_t con_lid)
{
    LOG_I("%s", __func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
#ifdef GSBC_SUPPORT
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 119, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_GSBC,  BES_BLE_GAF_CONTEXT_TYPE_GAME_BIT},
#else
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  BES_BLE_GAF_CONTEXT_TYPE_GAME_BIT},
#endif
    };
    aob_media_mobile_start_stream(ase_to_start, con_lid, false);
}

void ble_audio_central_stream_resume_ble_audio(uint8_t con_lid)
{
    BLE_AUDIO_CENTRAL_STREAM_STATE_E stream_state = ble_audio_central_stream_stm_get_cur_state();
    LOG_I("%s stream_state=%d", __func__,stream_state);
    ble_audio_capture_start_handler(con_lid);
    ble_audio_playback_start_handler(con_lid);
}

Msg const* ble_audio_central_stream_sm_super_state(ble_audio_central_stream_stm_t *me, Msg *msg)
{
    LOG_I("LEAC-STREAM: SUPER state on event: %s", ble_audio_central_stream_sm_event_to_string(msg->evt));
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

static void ble_audio_central_stream_sm_refresh_stream(BLE_AUDIO_CENTRAL_STREAM_EVENT_E event)
{
    if (ble_audio_central_stream_stm.post_op_checker)
    {
        if (BLE_AUDIO_CENTRAL_REQ_REFRESH_PLAYBACK_STREAM == event)
        {
            ble_audio_central_stream_stm.post_op_checker(EVT_PLAYBACK_STREAM_STOPPED);
        }
        else if (BLE_AUDIO_CENTRAL_REQ_REFRESH_CAPTURE_STREAM == event)
        {
            ble_audio_central_stream_stm.post_op_checker(EVT_CAPTURE_STREAM_STOPPED);
        }
    }
}

Msg const* ble_audio_central_stream_sm_idle_state(ble_audio_central_stream_stm_t *me, Msg *msg)
{
    LOG_I("LEAC-STREAM: IDLE State on event: %s", ble_audio_central_stream_sm_event_to_string(msg->evt));
    uint32_t i = 0;
    uint8_t ase_lid = INVALID_LID;

    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START:
        {
            for (i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
            {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_QOS_CONFIGURED, APP_GAF_DIRECTION_SINK);
                if (ase_lid != INVALID_LID)
                {
                    ble_ase_stm_t *p_ase_sm = ble_audio_find_ase_sm_by_ase_lid(ase_lid);
                    if (p_ase_sm)
                    {
                        aob_media_mobile_enable_stream(ase_lid);
                    }
                }
                else
                {
                    LOG_I("LEAC-STREAM link(%d) ase is not ready", i);
                }
            }
        }
        return 0;
        case EVT_PLAYBACK_STREAM_STARTED:
            if (ble_audio_central_stream_stm.start_receiving_handler)
            {
                ble_audio_central_stream_stm.start_receiving_handler();
            }

            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
			STATE_TRAN(me, &me->playback_start);
        return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START:
        {
            for (i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_QOS_CONFIGURED, APP_GAF_DIRECTION_SRC);
                if (ase_lid != INVALID_LID) {
                    ble_ase_stm_t *p_ase_sm = ble_audio_find_ase_sm_by_ase_lid(ase_lid);
                    if (p_ase_sm)
                    {
                        aob_media_mobile_enable_stream(ase_lid);
                    }
                }
            }
        }
        return 0;
        case EVT_CAPTURE_STREAM_STARTED:
            if (ble_audio_central_stream_stm.start_transmission_handler)
            {
                ble_audio_central_stream_stm.start_transmission_handler();
            }

            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
            STATE_TRAN(me, &me->capture_start);
        return 0;

        case EVT_PLAYBACK_STREAM_STOPPED:
        case EVT_CAPTURE_STREAM_STOPPED:
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
        return 0;
        case BLE_AUDIO_CENTRAL_REQ_REFRESH_PLAYBACK_STREAM:
        case BLE_AUDIO_CENTRAL_REQ_REFRESH_CAPTURE_STREAM:
            ble_audio_central_stream_sm_refresh_stream((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
        return 0;
        case EXIT_EVT:
            return 0;
    }

    return msg;
}

Msg const* ble_audio_central_stream_sm_playback_start_state(ble_audio_central_stream_stm_t *me, Msg *msg)
{
    LOG_I("LEAC-STREAM: playback state on event: %s", ble_audio_central_stream_sm_event_to_string(msg->evt));
    uint8_t ase_lid = INVALID_LID;

    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case EVT_PLAYBACK_STREAM_STOPPED:
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
            STATE_TRAN(me, &me->idle);
            return 0;
        case EVT_CAPTURE_STREAM_STOPPED:
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
            STATE_TRAN(me, &me->idle);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP:
            for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, APP_GAF_DIRECTION_SINK);
                if (ase_lid != INVALID_LID)
                {
                    aob_media_mobile_disable_stream(ase_lid);
                }
            }
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START:
            {
                for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                    ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_QOS_CONFIGURED, APP_GAF_DIRECTION_SRC);
                    if (ase_lid != INVALID_LID) {
                        ble_ase_stm_t *p_ase_sm = ble_audio_find_ase_sm_by_ase_lid(ase_lid);
                        if (p_ase_sm) {
                            aob_media_mobile_enable_stream(ase_lid);
                        }
                    }
                }
            }
            return 0;
        case EVT_CAPTURE_STREAM_STARTED:
            if (ble_audio_central_stream_stm.start_transmission_handler)
            {
                ble_audio_central_stream_stm.start_transmission_handler();
            }

            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
            STATE_TRAN(me, &me->both_start);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_REFRESH_PLAYBACK_STREAM:
        case BLE_AUDIO_CENTRAL_REQ_REFRESH_CAPTURE_STREAM:
            ble_audio_central_stream_sm_refresh_stream((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            return 0;
        case EXIT_EVT:
            return 0;
        default:
            break;
    }

    return msg;
}

Msg const* ble_audio_central_stream_sm_capture_start_state(ble_audio_central_stream_stm_t *me, Msg *msg)
{
    LOG_I("LEAC-STREAM: CAPTURE state on event: %s", ble_audio_central_stream_sm_event_to_string(msg->evt));
    uint8_t ase_lid = INVALID_LID;

    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ble_audio_playback_start_handler(i);
            }
            return 0;
        case EVT_CAPTURE_STREAM_STOPPED:
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            }
            STATE_TRAN(me, &me->idle);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP:
        {
            for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, APP_GAF_DIRECTION_SRC);
                if (ase_lid != INVALID_LID) {
                    aob_media_mobile_disable_stream(ase_lid);
                }
            }
        }
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START:
        {
            for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_QOS_CONFIGURED, APP_GAF_DIRECTION_SINK);
                if (ase_lid != INVALID_LID) {
                    ble_ase_stm_t *p_ase_sm = ble_audio_find_ase_sm_by_ase_lid(ase_lid);
                    if (p_ase_sm) {
                        aob_media_mobile_enable_stream(ase_lid);
                    }
                }
            }
        }
        return 0;
        case EVT_PLAYBACK_STREAM_STARTED:
            if (ble_audio_central_stream_stm.start_receiving_handler)
            {
                ble_audio_central_stream_stm.start_receiving_handler();
            }
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker(EVT_PLAYBACK_STREAM_STARTED);
            }
            STATE_TRAN(me, &me->both_start);
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_REFRESH_PLAYBACK_STREAM:
        case BLE_AUDIO_CENTRAL_REQ_REFRESH_CAPTURE_STREAM:
            ble_audio_central_stream_sm_refresh_stream((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)msg->evt);
            return 0;
        case EXIT_EVT:
            return 0;
        default:
            break;
    }

    return msg;
}

Msg const* ble_audio_central_stream_sm_both_start_state(ble_audio_central_stream_stm_t *me, Msg *msg)
{
    LOG_I("LEAC-STREAM: BOTH stream state on event: %s", ble_audio_central_stream_sm_event_to_string(msg->evt));
    uint8_t ase_lid = INVALID_LID;
    switch (msg->evt)
    {
        case START_EVT:
            return 0;
        case ENTRY_EVT:
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP:
        {
            for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, APP_GAF_DIRECTION_SINK);
                if (ase_lid != INVALID_LID) {
                    aob_media_mobile_disable_stream(ase_lid);
                }
            }
        }
            return 0;
        case BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP:
        {
            for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
                ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, APP_GAF_DIRECTION_SRC);
                if (ase_lid != INVALID_LID) {
                    aob_media_mobile_disable_stream(ase_lid);
                }
            }
        }
            return 0;
        case EVT_PLAYBACK_STREAM_STOPPED:
        {
            STATE_TRAN(me, &me->capture_start);
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)(msg->evt));
            }
            return 0;
        }
        case EVT_CAPTURE_STREAM_STOPPED:
        {
            STATE_TRAN(me, &me->playback_start);
            if (ble_audio_central_stream_stm.post_op_checker)
            {
                ble_audio_central_stream_stm.post_op_checker((BLE_AUDIO_CENTRAL_STREAM_EVENT_E)(msg->evt));
            }
            return 0;
        }
        case EXIT_EVT:
            return 0;
        default:
            break;
    }

    return msg;
}

int ble_audio_central_stream_sm_on_event(uint32_t stream_stm_ptr,uint32_t event,uint32_t param0,uint32_t param1)
{
    Msg message;
    ble_audio_central_stream_stm_t *p_stream_sm = NULL;

    message.evt = event;
    message.param0 = param0;
    message.param1 = param1;
    message.param2 = 0;

    p_stream_sm = (ble_audio_central_stream_stm_t*)stream_stm_ptr;
    HsmOnEvent((Hsm *)p_stream_sm, &message);

    return 0;
}

void ble_audio_central_stream_send_message(ble_audio_central_stream_stm_t *p_stream_sm, BLE_AUDIO_CENTRAL_STREAM_EVENT_E event, uint32_t param0, uint32_t param1)
{
    app_bt_call_func_in_bt_thread((uint32_t)p_stream_sm,(uint32_t)event,param0,param1,(uint32_t)ble_audio_central_stream_sm_on_event);
}

ble_audio_central_stream_stm_t* ble_audio_central_get_stream_sm(void)
{
    ble_audio_central_stream_stm_t* iter = &ble_audio_central_stream_stm;

    return iter;
}

BLE_AUDIO_CENTRAL_STREAM_STATE_E ble_audio_central_stream_stm_get_cur_state(void)
{
    ble_audio_central_stream_stm_t *p_stream_sm = &ble_audio_central_stream_stm;

    BLE_AUDIO_CENTRAL_STREAM_STATE_E state = BLE_AUDIO_CENTRAL_STREAM_STATE_UNKNOW;
    if (!p_stream_sm->used) {
        return state;
    }

    if (p_stream_sm->super.curr == &p_stream_sm->idle)
    {
        state = BLE_AUDIO_CENTRAL_STREAM_STATE_IDLE;
    } else if (p_stream_sm->super.curr == &p_stream_sm->playback_start) {
        state = BLE_AUDIO_CENTRAL_STREAM_STATE_PLAYBACK_STARTED;
    } else if (p_stream_sm->super.curr == &p_stream_sm->capture_start) {
        state = BLE_AUDIO_CENTRAL_STREAM_STATE_CAPTURE_STARTED;
    } else if (p_stream_sm->super.curr == &p_stream_sm->both_start) {
        state = BLE_AUDIO_CENTRAL_STREAM_STATE_BOTH_STARTED;
    }

    return state;
}

void ble_audio_central_stream_stm_startup(ble_audio_central_stream_stm_t *ble_audio_central_stream_sm)
{
    LOG_I("LEAC-STREAM: %s", __func__);
    ble_audio_central_stream_stm.used = true;

    HsmCtor((Hsm *)ble_audio_central_stream_sm, "stream_super", (EvtHndlr)ble_audio_central_stream_sm_super_state);
    AddState(&ble_audio_central_stream_sm->idle, "idle",  &((Hsm *)ble_audio_central_stream_sm)->top, (EvtHndlr)ble_audio_central_stream_sm_idle_state);
    AddState(&ble_audio_central_stream_sm->playback_start, "playback_start", &((Hsm *)ble_audio_central_stream_sm)->top, (EvtHndlr)ble_audio_central_stream_sm_playback_start_state);
    AddState(&ble_audio_central_stream_sm->capture_start, "capture_start", &((Hsm *)ble_audio_central_stream_sm)->top, (EvtHndlr)ble_audio_central_stream_sm_capture_start_state);
    AddState(&ble_audio_central_stream_sm->both_start, "both_start", &((Hsm *)ble_audio_central_stream_sm)->top, (EvtHndlr)ble_audio_central_stream_sm_both_start_state);

    HsmOnStart((Hsm *)ble_audio_central_stream_sm);
}

void ble_audio_central_stream_start_receiving_handler_register(ble_audio_central_stream_start_receiving_func cb)
{
    ble_audio_central_stream_stm.start_receiving_handler = cb;
}

void ble_audio_central_stream_start_transmission_handler_register(ble_audio_central_stream_start_transmission_func cb)
{
    ble_audio_central_stream_stm.start_transmission_handler = cb;
}

void ble_audio_central_stream_post_operation_check_cb_register(ble_audio_central_stream_stm_post_op_check_func cb)
{
    ble_audio_central_stream_stm.post_op_checker = cb;
}

void ble_audio_central_stream_post_cis_discon_operation_check_cb_register(ble_audio_central_stream_stm_post_cis_discon_op_check_func cb)
{
    ble_audio_central_stream_stm.post_cis_discon_op_checker = cb;
}

void ble_audio_central_stream_post_cis_discon_operation_check(void)
{
    if (ble_audio_central_stream_stm.post_cis_discon_op_checker)
    {
        ble_audio_central_stream_stm.post_cis_discon_op_checker();
    }
}

void ble_audio_central_stream_stm_init(void)
{
    memset(&ble_audio_central_stream_stm, 0xFF, sizeof(ble_audio_central_stream_stm_t));
    ble_audio_central_stream_stm.used = false;
    ble_audio_central_stream_stm.post_op_checker = NULL;
    ble_audio_central_stream_stm.post_cis_discon_op_checker = NULL;
}

#endif //AOB_MOBILE_ENABLED


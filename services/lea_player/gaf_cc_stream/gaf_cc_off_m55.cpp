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

#ifdef GAF_CODEC_CROSS_CORE
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "plat_types.h"
#include "app_gaf_dbg.h"
#include "gaf_cc_api.h"
#include "gaf_cc_off_m55.h"
#include "app_dsp_m55.h"
#include "mcu_dsp_m55_app.h"
#include "cc_stream_common.h"

/************************private macro defination***************************/

/************************private struct defination****************************/

/************************private variable defination************************/

/**********************private function declaration*************************/
static void gaf_off_m55_capture_init_transmit(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_capture_init_wait_rsp_timeout_handler(uint8_t*, uint16_t);
static void gaf_off_m55_capture_init_rsp_handler(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_capture_deinit_transmit(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_capture_deinit_wait_rsp_timeout_handler(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_capture_retrigger_handler(uint8_t* param, uint16_t len);
static void gaf_off_m55_capture_deinit_rsp_handler(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_capture_data_notify_transmit(uint8_t* ptr, uint16_t len);

static void gaf_off_m55_playback_init_transmit(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_playback_deinit_transmit(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_playback_deinit_wait_rsp_timeout_handler(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_playback_deinit_rsp_handler(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_playback_retrigger_handler(uint8_t* param, uint16_t len);
static void gaf_off_m55_playback_init_rsp_handler(uint8_t* ptr, uint16_t len);
static void gaf_off_m55_playback_data_notify_transmit(uint8_t* ptr, uint16_t len);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_ENCODE_INIT_WAITING_RSP,
                            "CC_GAF_INIT_WAITING_RSP",
                            gaf_off_m55_capture_init_transmit,
                            NULL,
                            5000,
                            gaf_off_m55_capture_init_wait_rsp_timeout_handler,
                            gaf_off_m55_capture_init_rsp_handler,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_ENCODE_DEINIT_WAITING_RSP,
                            "CC_GAF_ENCODE_DEINIT_WAITING_RSP",
                            gaf_off_m55_capture_deinit_transmit,
                            NULL,
                            1000,
                            gaf_off_m55_capture_deinit_wait_rsp_timeout_handler,
                            gaf_off_m55_capture_deinit_rsp_handler,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_RETRIGGER_REQ_NO_RSP,
                            "CC_GAF_RETRIIGER_REQ_NO_RSP",
                            NULL,
                            gaf_off_m55_capture_retrigger_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_ENCODE_DATA_NOTIFY,
                            gaf_off_m55_capture_data_notify_transmit,
                            NULL);



M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_WAITING_RSP,
                            gaf_off_m55_playback_init_transmit,
                            NULL);

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_RSP_TO_CORE,
                            NULL,
                            gaf_off_m55_playback_init_rsp_handler);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_DECODE_DEINIT_WAITING_RSP,
                            "CC_GAF_DECODE_DEINIT_WAITING_RSP",
                            gaf_off_m55_playback_deinit_transmit,
                            NULL,
                            1000,
                            gaf_off_m55_playback_deinit_wait_rsp_timeout_handler,
                            gaf_off_m55_playback_deinit_rsp_handler,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_DECODE_RETRIGGER_REQ_NO_RSP,
                            "CC_GAF_RETRIGGER_REQ_NO_RSP",
                            NULL,
                            gaf_off_m55_playback_retrigger_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_DECODE_DATA_NOTIFY,
                            gaf_off_m55_playback_data_notify_transmit,
                            NULL);

/****************************function defination****************************/

static void gaf_off_m55_capture_feed_init_req(void *initReq)
{
    CC_CAPTURE_INIT_REQ_T *req = (CC_CAPTURE_INIT_REQ_T*)initReq;
    if (req->isBis) {
        app_dsp_m55_init(APP_DSP_M55_USER_BIS);
    }
    else {
        app_dsp_m55_init(APP_DSP_M55_USER_AUDIO_ENCODER);
    }
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_GAF_ENCODE_INIT_WAITING_RSP,
                    (uint8_t*)initReq, sizeof(CC_CAPTURE_INIT_REQ_T));
}

static void gaf_off_m55_capture_init_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_GAF_ENCODE_INIT_WAITING_RSP, ptr, len);
}

static void gaf_off_m55_capture_init_wait_rsp_timeout_handler(uint8_t*, uint16_t)
{
    ASSERT(false, "wait for gaf init rsp from off mcu core time out!");
}

static void gaf_off_m55_capture_init_rsp_handler(uint8_t* ptr, uint16_t len)
{
    gaf_cc_capture_init_rsp(ptr);
}

static void gaf_off_m55_capture_deinit_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_GAF_ENCODE_DEINIT_WAITING_RSP, ptr, len);
}

static void gaf_off_m55_capture_deinit_handler(CC_CAPTURE_DEINIT_RSP_T *deinitRsp)
{
    LOG_D("%s", __func__);
    if (deinitRsp->closeEncoderCore)
    {
        if (deinitRsp->isBis) {
            app_dsp_m55_deinit(APP_DSP_M55_USER_BIS);
        }
        else {
            app_dsp_m55_deinit(APP_DSP_M55_USER_AUDIO_ENCODER);
        }
    }
}

static void gaf_off_m55_capture_deinit_wait_rsp_timeout_handler(uint8_t* ptr, uint16_t len)
{
    CC_CAPTURE_DEINIT_RSP_T *deinitRsp = (CC_CAPTURE_DEINIT_RSP_T*)ptr;
    LOG_I("BTH get gaf_encoder_deinit_rsp from m55 timeout");
    gaf_off_m55_capture_deinit_handler(deinitRsp);
}

static void gaf_off_m55_capture_deinit_rsp_handler(uint8_t* ptr, uint16_t len)
{
    CC_CAPTURE_DEINIT_RSP_T *deinitRsp = (CC_CAPTURE_DEINIT_RSP_T*)ptr;
    /// bth received m55_deinit_rsp,and to close m55
    LOG_I("BTH gets gaf_encoder_deinit_rsp from m55");
    gaf_off_m55_capture_deinit_handler(deinitRsp);
}

static void gaf_off_m55_capture_retrigger_handler(uint8_t* param, uint16_t len)
{
    // To do: gaf audio media retrigger api
}

static void gaf_off_m55_capture_feed_deinit_req(void *deinitReq)
{
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_GAF_ENCODE_DEINIT_WAITING_RSP,
            (uint8_t*)deinitReq, sizeof(CC_CAPTURE_DEINIT_REQ_T));
}

static void gaf_off_m55_capture_data_notify(void)
{
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_INSTANT_CMD_GAF_ENCODE_DATA_NOTIFY, NULL, 0);
}

static void gaf_off_m55_capture_data_notify_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(CROSS_CORE_INSTANT_CMD_GAF_ENCODE_DATA_NOTIFY, ptr, len);
}

static void gaf_off_m55_playback_feed_init_req(void *initReq)
{
    CC_PLAYBACK_INIT_REQ_T *req = (CC_PLAYBACK_INIT_REQ_T*)initReq;
    if (req->isBis) {
        app_dsp_m55_init(APP_DSP_M55_USER_BIS);
    }
    else {
        app_dsp_m55_init(APP_DSP_M55_USER_AUDIO_DECODER);
    }
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_WAITING_RSP,
                    (uint8_t*)initReq, sizeof(CC_PLAYBACK_INIT_REQ_T));
}

static void gaf_off_m55_playback_init_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_WAITING_RSP, ptr, len);
}

static void gaf_off_m55_playback_init_rsp_handler(uint8_t* ptr, uint16_t len)
{
    gaf_cc_playback_init_rsp(ptr);
}

static void gaf_off_m55_playback_deinit_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_GAF_DECODE_DEINIT_WAITING_RSP, ptr, len);
}

static void gaf_off_m55_playback_data_notify_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(CROSS_CORE_INSTANT_CMD_GAF_DECODE_DATA_NOTIFY, ptr, len);
}

static void gaf_off_m55_playback_deinit_handler(CC_PLAYBACK_DEINIT_RSP_T *deinitRsp)
{
    LOG_D("%s", __func__);
    if (deinitRsp->closeDecoderCore)
    {
        if (deinitRsp->isBis) {
            app_dsp_m55_deinit(APP_DSP_M55_USER_BIS);
        }
        else {
            app_dsp_m55_deinit(APP_DSP_M55_USER_AUDIO_DECODER);
        }
    }
}

static void gaf_off_m55_playback_deinit_wait_rsp_timeout_handler(uint8_t* ptr, uint16_t len)
{
    CC_PLAYBACK_DEINIT_RSP_T *deinitRsp = (CC_PLAYBACK_DEINIT_RSP_T*)ptr;
    LOG_I("BTH wait for gaf_deinit_rsp from m55 time out!");
    gaf_off_m55_playback_deinit_handler(deinitRsp);
}

static void gaf_off_m55_playback_deinit_rsp_handler(uint8_t* ptr, uint16_t len)
{
    CC_PLAYBACK_DEINIT_RSP_T *deinitRsp = (CC_PLAYBACK_DEINIT_RSP_T*)ptr;
    LOG_I("BTH gets gaf_decoder_deinit_rsp from m55");
    gaf_off_m55_playback_deinit_handler(deinitRsp);
}

static void gaf_off_m55_playback_retrigger_handler(uint8_t* param, uint16_t len)
{
    // To do: retrigger audio media
}

static void gaf_off_m55_playback_feed_deinit_req(void *deinitReq)
{
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_GAF_DECODE_DEINIT_WAITING_RSP,
                    (uint8_t*)deinitReq, sizeof(CC_PLAYBACK_DEINIT_REQ_T));
}

static void gaf_off_m55_playback_data_notify(void)
{
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_INSTANT_CMD_GAF_DECODE_DATA_NOTIFY, NULL, 0);
}

static ENCODER_CORE_FUNC_LIST_T gaf_off_m55_capture_func_list =
{
    .encoder_core_init = gaf_off_m55_capture_feed_init_req,
    .encoder_core_deinit = gaf_off_m55_capture_feed_deinit_req,
    .encoder_core_notify = gaf_off_m55_capture_data_notify,
};

static DECODER_CORE_FUNC_LIST_T gaf_off_m55_playback_func_list =
{
    .decoder_core_int = gaf_off_m55_playback_feed_init_req,
    .decoder_core_deinit = gaf_off_m55_playback_feed_deinit_req,
    .decoder_core_notify = gaf_off_m55_playback_data_notify,
};

void gaf_off_m55_capture_update_func_list(void *_func_list)
{
    ENCODER_CORE_FUNC_LIST_T **func_list = (ENCODER_CORE_FUNC_LIST_T**)_func_list;
    *func_list = &gaf_off_m55_capture_func_list;
}

void gaf_off_m55_playback_update_func_list(void *_func_list)
{
    DECODER_CORE_FUNC_LIST_T **func_list = (DECODER_CORE_FUNC_LIST_T**)_func_list;
    *func_list = &gaf_off_m55_playback_func_list;
}

#endif

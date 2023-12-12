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
#include "plat_types.h"
#include "heap_api.h"
#include "app_utils.h"
#include "app_dsp_m55.h"
#include "app_gaf_dbg.h"
#include "gaf_m55_stream.h"
#include "cc_stream_common.h"

/************************private macro defination***************************/

/************************private type defination****************************/
static uint32_t playback_instance_bit_map;
static uint32_t capture_instance_bit_map;

/************************private variable defination************************/

/**********************private function declaration*************************/
static void gaf_m55_playback_init_req_handler(uint8_t* ptr, uint16_t len);
static void gaf_m55_playback_deinit_req_handler(uint8_t* ptr, uint16_t len);
static void gaf_m55_playback_retrigger_req_transmit(uint8_t* ptr, uint16_t len);
static void gaf_m55_playback_init_rsp_transmit(uint8_t* ptr, uint16_t len);
static void gaf_m55_playback_data_notify_handler(uint8_t *ptr, uint16_t len);

static void gaf_m55_capture_init_req_handler(uint8_t* ptr, uint16_t len);
static void gaf_m55_capture_deinit_req_handler(uint8_t* ptr, uint16_t len);
static void gaf_m55_capture_retrigger_req_transmit(uint8_t* ptr, uint16_t len);
static void gaf_m55_capture_data_notify_handler(uint8_t *ptr, uint16_t len);


M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_WAITING_RSP,
                            NULL,
                            gaf_m55_playback_init_req_handler);

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_RSP_TO_CORE,
                            gaf_m55_playback_init_rsp_transmit,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_DECODE_DEINIT_WAITING_RSP,
                            "CC_GAF_DECODE_DEINIT_NO_RSP",
                            NULL,
                            gaf_m55_playback_deinit_req_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_DECODE_RETRIGGER_REQ_NO_RSP,
                            "CC_GAF_RETRIGGER_REQ_NO_RSP",
                            gaf_m55_playback_retrigger_req_transmit,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_DECODE_DATA_NOTIFY,
                            NULL,
                            gaf_m55_playback_data_notify_handler);


M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_ENCODE_INIT_WAITING_RSP,
                            "CC_GAF_ENCODE_INIT_WAITING_RSP",
                            NULL,
                            gaf_m55_capture_init_req_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_ENCODE_DEINIT_WAITING_RSP,
                            "CC_GAF_ENCODE_DEINIT_NO_RSP",
                            NULL,
                            gaf_m55_capture_deinit_req_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(CROSS_CORE_TASK_CMD_GAF_RETRIGGER_REQ_NO_RSP,
                            "CC_GAF_RETRIGGLE_REQ_NO_RSP",
                            gaf_m55_capture_retrigger_req_transmit,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(CROSS_CORE_INSTANT_CMD_GAF_ENCODE_DATA_NOTIFY,
                            NULL,
                            gaf_m55_capture_data_notify_handler);


/****************************function defination****************************/
static void gaf_m55_playback_init_rsp(void *ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_cmd(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_RSP_TO_CORE, (uint8_t*)ptr, len);
}

static void gaf_m55_playback_init_rsp_transmit(uint8_t *ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_RSP_TO_CORE, ptr, len);
}

static void gaf_m55_playback_deinit_rsp(uint8_t *rsp, uint16_t len)
{
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_GAF_DECODE_DEINIT_WAITING_RSP, rsp, len);
}

static void gaf_m55_playback_deinit_req_handler(uint8_t* ptr, uint16_t len)
{
    ASSERT(sizeof(CC_PLAYBACK_DEINIT_REQ_T) == len,
        "%s len:%d deinitReqSize:%d", __func__, len, sizeof(CC_PLAYBACK_DEINIT_REQ_T));
    cc_playback_deinit_req_handler(ptr);
}

static void gaf_m55_playback_data_notify_handler(uint8_t *ptr, uint16_t len)
{
    if (0 == playback_instance_bit_map)
    {
        LOG_I("m55 playback has already been stopped.:%s", __func__);
        return;
    }
    cc_stream_data_notify_handler();
}

static void gaf_m55_playback_retrigger_req_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_GAF_DECODE_RETRIGGER_REQ_NO_RSP, ptr, len);
}

POSSIBLY_UNUSED static void gaf_m55_playback_feed_retrigger_req(void)
{
    app_dsp_m55_bridge_send_cmd(
                CROSS_CORE_TASK_CMD_GAF_DECODE_RETRIGGER_REQ_NO_RSP, \
                NULL, 0);
}

static void gaf_m55_capture_init_rsp(void *ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_GAF_ENCODE_INIT_WAITING_RSP, (uint8_t*)ptr, len);
}

static void gaf_m55_capture_deinit_rsp(uint8_t *rsp, uint16_t len)
{
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_GAF_ENCODE_DEINIT_WAITING_RSP, rsp, len);
}

static void gaf_m55_capture_deinit_req_handler(uint8_t* ptr, uint16_t len)
{
    ASSERT(sizeof(CC_CAPTURE_DEINIT_REQ_T) == len,
            "%s len:%d deinitReqSize:%d", __func__, len, sizeof(CC_CAPTURE_DEINIT_REQ_T));
    cc_capture_deinit_req_handler(ptr);
}

static void gaf_m55_capture_retrigger_req_transmit(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_GAF_RETRIGGER_REQ_NO_RSP, ptr, len);
}

static void gaf_m55_capture_data_notify_handler(uint8_t *ptr, uint16_t len)
{
    if (0 == capture_instance_bit_map)
    {
        LOG_I("m55 capture has already been stopped.:%s", __func__);
        return;
    }
    cc_stream_data_notify_handler();
}

static void* gaf_m55_buffer_alloc(void *pool, uint32_t size)
{
    uint8_t *buf = NULL;
    ASSERT((int)size < off_bth_syspool_free_size(),
        "size:%d m55 free size:%d", size, off_bth_syspool_free_size());
    off_bth_syspool_get_buff(&buf, size);
    ASSERT(buf, "size:%d free size:%d", size, off_bth_syspool_free_size());
    return buf;
}

static void gaf_m55_playback_syspool_cb(void)
{
    cc_playback_buf_deinit(M55_CORE);
}

static void gaf_m55_capture_syspool_cb(void)
{
    cc_capture_buf_deinit(M55_CORE);
}

static void gaf_m55_playback_init_cb(uint8_t instance_handle)
{
    if (0 == playback_instance_bit_map)
    {
        off_bth_syspool_init(SYSPOOL_USER_GAF_DECODE, gaf_m55_playback_syspool_cb);
    }
    playback_instance_bit_map |= 1<<instance_handle;
}

static bool gaf_m55_playback_deinit_cb(uint8_t instance_handle)
{
    playback_instance_bit_map &= ~(1<<instance_handle);

    if (0 == playback_instance_bit_map)
    {
        off_bth_syspool_deinit(SYSPOOL_USER_GAF_DECODE);
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_32K);
        return true;
    }
    return false;
}

static void gaf_m55_capture_init_cb(uint8_t instance_handle)
{
    if (0 == capture_instance_bit_map)
    {
        off_bth_syspool_init(SYSPOOL_USER_GAF_ENCODE, gaf_m55_capture_syspool_cb);
    }
    capture_instance_bit_map |= 1<<instance_handle;
}

static bool gaf_m55_capture_deinit_cb(uint8_t instance_handle)
{
    capture_instance_bit_map &= ~(1<<instance_handle);

    if (0 == capture_instance_bit_map)
    {
        off_bth_syspool_deinit(SYSPOOL_USER_GAF_ENCODE);
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);
        return true;
    }
    return false;
}

const CC_CORE_PLAYBACK_FUNC_LIST_T m55_playback_func_list =
{
    .buf_alloc = gaf_m55_buffer_alloc,
    .playback_init = gaf_m55_playback_init_cb,
    .playback_init_rsp = gaf_m55_playback_init_rsp,
    .playback_deinit = gaf_m55_playback_deinit_cb,
    .playback_deinit_rsp = gaf_m55_playback_deinit_rsp,
};

const CC_CORE_CAPTURE_FUNC_LIST_T m55_capture_func_list =
{
    .buf_alloc = gaf_m55_buffer_alloc,
    .capture_init = gaf_m55_capture_init_cb,
    .capture_init_rsp = gaf_m55_capture_init_rsp,
    .capture_deinit = gaf_m55_capture_deinit_cb,
    .capture_deinit_rsp = gaf_m55_capture_deinit_rsp,
};

static void gaf_m55_playback_init_req_handler(uint8_t *ptr, uint16_t len)
{
    ASSERT(sizeof(CC_PLAYBACK_INIT_REQ_T) == len,
        "%s len:%d initReqSize:%d", __func__, len, sizeof(CC_PLAYBACK_INIT_REQ_T));

    CORE_PLAYBACK_INIT_INFO_T initInfo;
    initInfo.initReq = ptr;
    initInfo.core_type = M55_CORE;
    initInfo.playback_func_list = &m55_playback_func_list;
    cc_playback_init_req_handler(&initInfo);
}

static void gaf_m55_capture_init_req_handler(uint8_t* ptr, uint16_t len)
{
    ASSERT(sizeof(CC_CAPTURE_INIT_REQ_T) == len,
        "%s len:%d initReqSize:%d", __func__, len, sizeof(CC_CAPTURE_INIT_REQ_T));

    CORE_CAPTURE_INIT_INFO_T initInfo;
    initInfo.initReq = ptr;
    initInfo.core_type = M55_CORE;
    initInfo.capture_func_list = &m55_capture_func_list;
    cc_capture_init_req_handler(&initInfo);
}

void gaf_m55_stream_init(void)
{
    cc_stream_init(M55_CORE);
}
#endif
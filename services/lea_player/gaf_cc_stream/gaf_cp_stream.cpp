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
#ifdef AOB_CODEC_CP
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "plat_types.h"
#include "app_gaf_dbg.h"
#include "hal_location.h"
#include "heap_api.h"
#include "cp_accel.h"
#include "norflash_api.h"
#include "gaf_cp_stream.h"
#include "gaf_cc_api.h"
#include "cc_stream_common.h"

/************************private macro defination***************************/

/************************private type defination****************************/
static uint32_t cp_playback_instance_bit_map = 0;
static uint32_t cp_capture_instance_bit_map = 0;

/************************private variable defination************************/

/**********************private function declaration*************************/

static unsigned int cp_process_main(uint8_t event);

static struct cp_task_desc TASK_DESC_AOB_CODEC = {
    CP_ACCEL_STATE_CLOSED, cp_process_main, NULL, NULL, NULL};
/****************************function defination****************************/

CP_TEXT_SRAM_LOC
static unsigned int cp_process_main(uint8_t event)
{
    if (CP_EVENT_AOB_CODEC_PROCESSING != event)
    {
        return 0;
    }

    cc_stream_process();

    return 0;
}

static void gaf_cp_buf_init(void)
{
    if (0 == cp_playback_instance_bit_map && 0 == cp_capture_instance_bit_map)
    {
        cp_pool_init();
        LOG_I("%s cp pool total size:%d", __func__, cp_pool_total_size());
    }
}

static void gaf_cp_buf_deinit(void)
{
    if (0 == cp_playback_instance_bit_map && 0 == cp_capture_instance_bit_map)
    {
        cc_playback_buf_deinit(CP_CORE);
        cc_capture_buf_deinit(CP_CORE);
        LOG_I("%s cp pool total size:%d", __func__, cp_pool_total_size());
    }
}

static void gaf_cp_stop_process(void)
{
    if (0 == cp_playback_instance_bit_map && 0 == cp_capture_instance_bit_map)
    {
        LOG_I("%s", __func__);
        cp_accel_close(CP_TASK_AOB_CODEC);
    }
}

static void gaf_cp_start_process(void)
{
    if (0 == cp_playback_instance_bit_map && 0 == cp_capture_instance_bit_map)
    {
        norflash_api_flush_disable(NORFLASH_API_USER_CP,(uint32_t)cp_accel_init_done);
        cp_accel_open(CP_TASK_AOB_CODEC, &TASK_DESC_AOB_CODEC);
        while (cp_accel_init_done() == false)
        {
            LOG_D("[%s] Delay...", __func__);
            osDelay(1);
        }
        norflash_api_flush_enable(NORFLASH_API_USER_CP);
        LOG_I("%s", __func__);
    }
}

static void gaf_cp_playback_init_rsp(void *rsp, uint16_t len)
{
    gaf_cc_playback_init_rsp(rsp);
}

static void gaf_cp_capture_init_rsp(void *rsp, uint16_t len)
{
    gaf_cc_capture_init_rsp(rsp);
}

static void gaf_cp_playback_init_cb(uint8_t instance_handle)
{
    cp_playback_instance_bit_map |= 1<<instance_handle;
}

static bool gaf_cp_playback_deinit_cb(uint8_t instance_handle)
{
    cp_playback_instance_bit_map &= ~(1<<instance_handle);
    if (0 == cp_playback_instance_bit_map)
    {
        gaf_cp_stop_process();
        gaf_cp_buf_deinit();
        return true;
    }
    return false;
}

static bool gaf_cp_capture_deinit_cb(uint8_t instance_handle)
{
    cp_capture_instance_bit_map &= ~(1<<instance_handle);
    if (0 == cp_capture_instance_bit_map)
    {
        gaf_cp_stop_process();
        gaf_cp_buf_deinit();
        return true;
    }
    return false;
}

static void gaf_cp_capture_init_cb(uint8_t instance_handle)
{
    cp_capture_instance_bit_map |= 1<<instance_handle;
}

static void* gaf_cp_buffer_alloc(void *pool, uint32_t size)
{
    uint8_t *buf = NULL;
    ASSERT((int)size < cp_pool_free_size(), "size:%d cp free size:%d", size, cp_pool_free_size());
    cp_pool_get_buff(&buf, size);
    ASSERT(buf, "size:%d free size:%d", size, cp_pool_free_size());
    return buf;
}

const CC_CORE_PLAYBACK_FUNC_LIST_T cp_playback_func_list =
{
    .buf_alloc = gaf_cp_buffer_alloc,
    .playback_init = gaf_cp_playback_init_cb,
    .playback_init_rsp = gaf_cp_playback_init_rsp,
    .playback_deinit = gaf_cp_playback_deinit_cb,
    .playback_deinit_rsp = NULL,
};

const CC_CORE_CAPTURE_FUNC_LIST_T cp_capture_func_list =
{
    .buf_alloc = gaf_cp_buffer_alloc,
    .capture_init = gaf_cp_capture_init_cb,
    .capture_init_rsp = gaf_cp_capture_init_rsp,
    .capture_deinit = gaf_cp_capture_deinit_cb,
    .capture_deinit_rsp = NULL,
};

static void gaf_cp_playback_init_handler(void *initReq)
{
    cc_stream_boost_freq();
    gaf_cp_start_process();
    gaf_cp_stream_init();
    gaf_cp_buf_init();
    CORE_PLAYBACK_INIT_INFO_T initInfo;
    initInfo.initReq = (uint8_t*)initReq;
    initInfo.core_type = CP_CORE;
    initInfo.playback_func_list = &cp_playback_func_list;
    cc_playback_init(&initInfo);
}

static void gaf_cp_capture_init_handler(void *initReq)
{
    cc_stream_boost_freq();
    gaf_cp_start_process();
    gaf_cp_stream_init();
    gaf_cp_buf_init();
    CORE_CAPTURE_INIT_INFO_T initInfo;
    initInfo.initReq = (uint8_t*)initReq;
    initInfo.core_type = CP_CORE;
    initInfo.capture_func_list = &cp_capture_func_list;
    cc_capture_init(&initInfo);
}

static void gaf_cp_playback_deinit_handler(void *deinitReq)
{
    cc_playback_deinit(deinitReq);
}

static void gaf_cp_capture_deinit_handler(void *deinitReq)
{
    cc_capture_deinit(deinitReq);
}

static void gaf_cp_data_notify_handler(void)
{
    cp_accel_send_event_mcu2cp(CP_BUILD_ID(CP_TASK_AOB_CODEC, CP_EVENT_AOB_CODEC_PROCESSING));
}

static ENCODER_CORE_FUNC_LIST_T gaf_cp_capture_func_list =
{
    .encoder_core_init = gaf_cp_capture_init_handler,
    .encoder_core_deinit = gaf_cp_capture_deinit_handler,
    .encoder_core_notify = gaf_cp_data_notify_handler,
};

static DECODER_CORE_FUNC_LIST_T gaf_cp_playback_func_list =
{
    .decoder_core_int = gaf_cp_playback_init_handler,
    .decoder_core_deinit = gaf_cp_playback_deinit_handler,
    .decoder_core_notify = gaf_cp_data_notify_handler,
};

void gaf_cp_capture_update_func_list(void *_func_list)
{
    ENCODER_CORE_FUNC_LIST_T **func_list = (ENCODER_CORE_FUNC_LIST_T**)_func_list;
    *func_list = &gaf_cp_capture_func_list;
}

void gaf_cp_playback_update_func_list(void *_func_list)
{
    DECODER_CORE_FUNC_LIST_T **func_list = (DECODER_CORE_FUNC_LIST_T**)_func_list;
    *func_list = &gaf_cp_playback_func_list;
}

void gaf_cp_stream_init(void)
{
    if (0 == cp_playback_instance_bit_map && 0 == cp_capture_instance_bit_map)
    {
        cc_stream_init(CP_CORE);
    }
}

#endif

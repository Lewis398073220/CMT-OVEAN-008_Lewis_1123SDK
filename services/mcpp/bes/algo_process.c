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
#include "algo_process.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "app_mcpp_comm.h"

const algo_process_stream_t *load_capture_algo_process(int32_t user)
{
    TRACE(0, "[%s]:user=%d", __func__, user);
    const algo_process_stream_t *algo_ptr = NULL;

    switch (user)
    {
    case APP_MCPP_USER_CALL:
    case APP_MCPP_USER_TEST_01:
    case APP_MCPP_USER_TEST_02:
        algo_ptr = &algo_process_call_capture;
        break;
    case APP_MCPP_USER_AUDIO:
        ASSERT(false, "[%s]:audio does not support capture", __func__);
        break;
    default:
        ASSERT(false, "[%s]:user error!", __func__);
        break;
    }

    return algo_ptr;
}

const algo_process_stream_t *load_playback_algo_process(int32_t user)
{
    TRACE(0, "[%s]:user=%d", __func__, user);
    const algo_process_stream_t *algo_ptr = NULL;

    switch (user)
    {
    case APP_MCPP_USER_CALL:
    case APP_MCPP_USER_TEST_01:
    case APP_MCPP_USER_TEST_02:
        algo_ptr = &algo_process_call_playback;
        break;
    case APP_MCPP_USER_AUDIO:
        algo_ptr = &algo_process_audio_playback;
        break;
    default:
        ASSERT(false, "[%s]:user error!", __func__);
        break;
    }

    return algo_ptr;
}

uint32_t get_algo_process_heap_buffer_size(int32_t user)
{
    uint32_t heap_buf_size = 0;

    /**
     * If multiple users may exist at the same time when MCPP is used,
     * Please set the buffer size to the sum of multiple users buffer sizes.
    */
    switch (user)
    {
    case APP_MCPP_USER_CALL:
    case APP_MCPP_USER_TEST_01:
    case APP_MCPP_USER_TEST_02:
        heap_buf_size = ALGO_PROCESS_CALL_POOL_BUF_SIZE;
        break;
    case APP_MCPP_USER_AUDIO:
        heap_buf_size = ALGO_PROCESS_AUDIO_POOL_BUF_SIZE;
        break;
    default:
        break;
    }

    TRACE(0, "[%s] user:%d, heap_size:%d", __func__, user, heap_buf_size);
    return heap_buf_size;

}
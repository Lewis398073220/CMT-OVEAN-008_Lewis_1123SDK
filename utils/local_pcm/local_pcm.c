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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "local_pcm.h"
#include "hal_trace.h"

static uint32_t g_pcm_buf_len = 0;
static uint32_t g_pcm_buf_index = 0;
static const uint8_t *g_pcm_buf = NULL;

int32_t local_pcm_init(const uint8_t *buf, uint32_t size)
{
    g_pcm_buf = buf;
    g_pcm_buf_len = size;
    g_pcm_buf_index = 0;

    return 0;
}

int32_t local_pcm_get_data(uint8_t *buf, uint32_t size)
{
    if (g_pcm_buf_index + size <= g_pcm_buf_len) {
        memcpy(buf, g_pcm_buf + g_pcm_buf_index, size);
        g_pcm_buf_index += size;
    } else {
        memset(buf, 0, size);
        g_pcm_buf_index = g_pcm_buf_len;
    }

    return 0;
}

int32_t local_pcm_loop_get_data(uint8_t *buf, uint32_t size)
{
    ASSERT(!(size > g_pcm_buf_len), "[%s] Please enter valid audio data", __func__);
    if (g_pcm_buf_index + size <= g_pcm_buf_len) {
        memcpy(buf, g_pcm_buf + g_pcm_buf_index, size);
        g_pcm_buf_index += size;
    } else {
        uint32_t remain_size = g_pcm_buf_len - g_pcm_buf_index;
        uint32_t restart_size = size - remain_size;

        memcpy(buf, g_pcm_buf + g_pcm_buf_index, remain_size);
        memcpy(buf + remain_size, g_pcm_buf, restart_size);
        g_pcm_buf_index = restart_size;
    }

    return 0;
}
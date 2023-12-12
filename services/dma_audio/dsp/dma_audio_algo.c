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
#include "string.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hal_cache.h"
#include "dma_audio_stream.h"
#include "dma_audio_stream_conf.h"
#include "dma_audio_algo.h"
#include "dma_audio.h"

static DAUD_ALGO_INIT_CALLBACK_T daud_algo_init_cb = NULL;

static DAUD_ALGO_CFG_CALLBACK_T daud_algo_cfg_cb = NULL;

static DAUD_ALGO_PROC_CALLBACK_T daud_algo_proc_cb = NULL;

static bool initialed = false;

static int algo_cfg_update_notify(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    void *cfg, uint32_t len, bool set)
{
    TRACE(0, "[%s]: id=%d, stream=%d, cfg=%x, set=%d", __func__, id, stream, (int)cfg, set);

    if (daud_algo_cfg_cb) {
        daud_algo_cfg_cb(id, stream, cfg, len, set);
    }

    return 0;
}

void dma_audio_algo_setup_init_callback(DAUD_ALGO_INIT_CALLBACK_T callback)
{
    daud_algo_init_cb = callback;
}

void dma_audio_algo_setup_config_callback(DAUD_ALGO_CFG_CALLBACK_T callback)
{
    daud_algo_cfg_cb = callback;
}

void dma_audio_algo_setup_algo_proc_callback(DAUD_ALGO_PROC_CALLBACK_T callback)
{
    daud_algo_proc_cb = callback;
}

void dma_audio_algo_init(struct DAUD_STREAM_CONFIG_T *cap_cfg, struct DAUD_STREAM_CONFIG_T *play_cfg)
{
    TRACE(0, "[%s] initialed ?= %d", __func__,initialed);

    if(!initialed){
        daud_set_algo_config_notify(algo_cfg_update_notify);

        if (daud_algo_init_cb) {
            daud_algo_init_cb(true);
        }

        initialed = true;
    }
}

void dma_audio_algo_deinit(void)
{
    TRACE(0, "[%s] initialed ?= %d", __func__,initialed);

    if(initialed){
        if (daud_algo_init_cb) {
            daud_algo_init_cb(false);
        }

        initialed = false;
    }
}

int dma_audio_algo_process(uint8_t *dst, uint32_t dst_len, uint8_t *src, uint32_t src_len)
{
    int ret = 0;

    if (daud_algo_proc_cb) {
        daud_algo_proc_cb(dst, dst_len, src, src_len);
    }

    return ret;
}

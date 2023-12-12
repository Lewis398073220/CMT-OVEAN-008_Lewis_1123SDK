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
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_trace.h"
#include "mcu_dsp_hifi4_app.h"
#include "dsp_loader.h"

#if defined(APP_RPC_ENABLE)
#include "app_rpc_api.h"
#if defined(APP_RPC_BTH_DSP_EN) && defined(BTH_AS_MAIN_MCU)
#include "rpc_bth_dsp.h"
#define RPC_BTH_DSP_ENABLE
#elif defined(APP_RPC_M55_DSP_EN)&& !defined(BTH_AS_MAIN_MCU)
#include "rpc_m55_dsp.h"
#define RPC_M55_DSP_ENABLE
#endif
#endif

static uint32_t app_dsp_hifi4_init_bitmap = 0;
static bool dsp_hifi4_is_running = false;
static app_dsp_hifi4_init_done_handler_t init_done_handler = NULL;

static const osMutexAttr_t mcu_dsp_hifi4_mutexAttr = {
    .name = "mcu_dsp_hifi4_mutex",
    .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
    .cb_mem = NULL,
    .cb_size = 0U,
};
static osMutexId_t mcu_dsp_hifi4_mutex_id = NULL;

void app_dsp_hifi4_register_init_done_handler(app_dsp_hifi4_init_done_handler_t Handler)
{
    init_done_handler = Handler;
}

static POSSIBLY_UNUSED void app_dsp_hifi4_init_done_callback(void)
{
    TRACE(0, "[%s]", __func__);

    if (init_done_handler) {
        init_done_handler();
    }
}

bool app_dsp_hifi4_is_running(void)
{
#if defined(RPC_BTH_DSP_ENABLE)
    return rpc_bth_dsp_ctx_inited();
#elif defined(RPC_M55_DSP_ENABLE)
    return rpc_m55_dsp_ctx_inited();
#else
    return false;
#endif
}

void app_dsp_hifi4_first_init(void)
{
    if (mcu_dsp_hifi4_mutex_id == NULL) {
        mcu_dsp_hifi4_mutex_id = osMutexNew(&mcu_dsp_hifi4_mutexAttr);
    }
}

int app_dsp_hifi4_init(APP_DSP_HIFI4_USER_E user)
{
    TRACE(3, "[%s] user:%d bitmap:0x%x", __func__, user, app_dsp_hifi4_init_bitmap);

    osMutexAcquire(mcu_dsp_hifi4_mutex_id, osWaitForever);
    if ((!dsp_hifi4_is_running) && (0 == app_dsp_hifi4_init_bitmap)) {

        dsp_hifi4_is_running = true;

#if defined(RPC_BTH_DSP_ENABLE)
        app_dsp_hifi_register_init_done_callback(app_dsp_hifi4_init_done_callback);
        app_rpc_ctx_init(APP_RPC_CORE_BTH_DSP);
#elif defined(RPC_M55_DSP_ENABLE)
        app_dsp_m55_register_init_done_callback(app_dsp_hifi4_init_done_callback);
        app_rpc_ctx_init(APP_RPC_CORE_DSP_M55);
#endif

        dsp_open();

#if defined(RPC_BTH_DSP_ENABLE)
        app_rpc_core_open(APP_RPC_CORE_BTH_DSP);
#elif defined(RPC_M55_DSP_ENABLE)
        app_rpc_core_open(APP_RPC_CORE_DSP_M55);
#endif
    }

    app_dsp_hifi4_init_bitmap |= (1 << user);
    osMutexRelease(mcu_dsp_hifi4_mutex_id);

    return 0;
}

int app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_E user)
{
    TRACE(3, "[%s] user:%d bitmap:0x%x", __func__, user, app_dsp_hifi4_init_bitmap);

    osMutexAcquire(mcu_dsp_hifi4_mutex_id, osWaitForever);
    app_dsp_hifi4_init_bitmap &= ~(1 << user);
    if ((dsp_hifi4_is_running) && (0 == app_dsp_hifi4_init_bitmap)) {
#if defined(RPC_BTH_DSP_ENABLE)
        app_dsp_hifi_register_init_done_callback(NULL);
        app_rpc_ctx_deinit(APP_RPC_CORE_BTH_DSP);
#elif defined(RPC_M55_DSP_ENABLE)
        app_dsp_m55_register_init_done_callback(NULL);
        app_rpc_ctx_deinit(APP_RPC_CORE_DSP_M55);
#endif
        init_done_handler = NULL;
        dsp_close();
        dsp_hifi4_is_running = false;
    }
    osMutexRelease(mcu_dsp_hifi4_mutex_id);
    return 0;
}
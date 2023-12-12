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
#include "cmsis_os.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "app_rpc_api.h"
#include "mcpp_cmd.h"
#include "app_mcpp_comm.h"
#include "app_mcpp_cfg.h"
#if defined(APP_MCPP_M55_HIFI_ENABLE)
#include "mcu_dsp_hifi4_app.h"
#endif

APP_MCPP_CORE_T g_mcpp_core = APP_MCPP_CORE_M55;

extern const MCPP_CMD_CFG_T *g_mcpp_cmd_cfg[APP_MCPP_CORE_QTY];

#ifdef APP_MCPP_BTH_M55_ENABLE
#include "app_dsp_m55.h"
/*****************************BTH to M55*****************************/
static const MCPP_CMD_CFG_T g_mcpp_cmd_bth_cfg = {
    .core        = APP_RPC_CORE_BTH_M55,

    .rsp_cmd     = MCU_DSP_M55_TASK_CMD_MCPP_RSP,
    .no_rsp_cmd  = MCU_DSP_M55_TASK_CMD_MCPP_NO_RSP,
};
#endif

#ifdef APP_MCPP_M55_HIFI_ENABLE
#include "rpc_m55_dsp.h"
/*****************************M55 to HIFI*****************************/
static const MCPP_CMD_CFG_T g_mcpp_cmd_hifi_cfg = {
    .core        = APP_RPC_CORE_DSP_M55,

    .rsp_cmd     = M55_DSP_TASK_CMD_MCPP_RSP,
    .no_rsp_cmd  = M55_DSP_TASK_CMD_MCPP_NO_RSP,
};
#endif

void mcpp_load_core_cmd_cfg(void)
{
#ifdef APP_MCPP_BTH_M55_ENABLE
    g_mcpp_cmd_cfg[APP_MCPP_CORE_BTH] = &g_mcpp_cmd_bth_cfg;
#endif
#ifdef APP_MCPP_M55_HIFI_ENABLE
    g_mcpp_cmd_cfg[APP_MCPP_CORE_HIFI] = &g_mcpp_cmd_hifi_cfg;
#endif
}

#ifdef APP_MCPP_M55_HIFI_ENABLE
void app_mcpp_task_hifi_process_done(void)
{
    TRACE(0, "[%s]", __func__);
}

static void mcpp_srv_core_hifi_init(void)
{
    app_dsp_hifi4_init(APP_DSP_HIFI4_USER_MCPP);
}

static void mcpp_srv_core_hifi_deinit(void)
{
    app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_MCPP);
}

static bool mcpp_srv_core_hifi_inited(void)
{
    return app_dsp_hifi4_is_running();
}
#endif

bool server_core_is_opened(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_M55_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        return mcpp_srv_core_hifi_inited();
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return false;
}

bool server_core_is_running(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_BTH_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        return false;
    } else
#endif
    {
        return server_core_is_opened(server);
    }
}

int32_t mcpp_core_open(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_M55_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        mcpp_srv_core_hifi_init();
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return 0;
}

int32_t mcpp_core_close(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_M55_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        mcpp_srv_core_hifi_deinit();
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return 0;
}

bool mcpp_target_core_is_baremetal(APP_MCPP_CORE_T server)
{
    return false;
}

int32_t mcpp_core_baremetal_send_data(APP_MCPP_CORE_T server, void *data, uint32_t len)
{
    return 0;
}
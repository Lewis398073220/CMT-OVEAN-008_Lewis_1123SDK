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

#if defined(APP_MCPP_BTH_M55_ENABLE)
#include "mcu_dsp_m55_app.h"
#endif
#if defined(APP_MCPP_BTH_HIFI_ENABLE)
#include "mcu_dsp_hifi4_app.h"
#include "hal_sys2bth.h"
#endif
#if defined(APP_MCPP_BTH_SENS_ENABLE)
#include "mcu_sensor_hub_app.h"
#endif
#if defined(APP_MCPP_BTH_CP_SUBSYS_ENABLE)
#include "cp_subsys.h"
#endif

APP_MCPP_CORE_T g_mcpp_core = APP_MCPP_CORE_BTH;

extern const MCPP_CMD_CFG_T *g_mcpp_cmd_cfg[APP_MCPP_CORE_QTY];

#ifdef APP_MCPP_BTH_M55_ENABLE
#include "app_dsp_m55.h"
/*****************************BTH to M55*****************************/
static const MCPP_CMD_CFG_T g_mcpp_cmd_m55_cfg = {
    .core        = APP_RPC_CORE_BTH_M55,

    .rsp_cmd     = MCU_DSP_M55_TASK_CMD_MCPP_RSP,
    .no_rsp_cmd  = MCU_DSP_M55_TASK_CMD_MCPP_NO_RSP,
};
#endif

#ifdef APP_MCPP_BTH_HIFI_ENABLE
#include "rpc_bth_dsp.h"
/*****************************BTH to HIFI*****************************/
static const MCPP_CMD_CFG_T g_mcpp_cmd_hifi_cfg = {
    .core        = APP_RPC_CORE_BTH_DSP,

    .rsp_cmd     = BTH_DSP_TASK_CMD_MCPP_RSP,
    .no_rsp_cmd  = BTH_DSP_TASK_CMD_MCPP_NO_RSP,
};
#endif

#ifdef APP_MCPP_BTH_SENS_ENABLE
#include "app_sensor_hub.h"
/*****************************BTH to SENS*****************************/
static const MCPP_CMD_CFG_T g_mcpp_cmd_sens_cfg = {
    .core        = APP_RPC_CORE_MCU_SENSOR,

    .rsp_cmd     = BTH_SENS_TASK_CMD_MCPP_RSP,
    .no_rsp_cmd  = BTH_SENS_TASK_CMD_MCPP_NO_RSP,
};
#endif

void mcpp_load_core_cmd_cfg(void)
{
#ifdef APP_MCPP_BTH_M55_ENABLE
    g_mcpp_cmd_cfg[APP_MCPP_CORE_M55] = &g_mcpp_cmd_m55_cfg;
#endif
#ifdef APP_MCPP_BTH_HIFI_ENABLE
    g_mcpp_cmd_cfg[APP_MCPP_CORE_HIFI] = &g_mcpp_cmd_hifi_cfg;
#endif
#ifdef APP_MCPP_BTH_SENS_ENABLE
    g_mcpp_cmd_cfg[APP_MCPP_CORE_SENS] = &g_mcpp_cmd_sens_cfg;
#endif
}

#ifdef APP_MCPP_BTH_M55_ENABLE
static void mcpp_src_core_m55_init(void)
{
    app_dsp_m55_init(APP_DSP_M55_USER_CALL);
}

static void mcpp_srv_core_m55_deinit(void)
{
    app_dsp_m55_deinit(APP_DSP_M55_USER_CALL);
}
#endif

#ifdef APP_MCPP_BTH_HIFI_ENABLE
#ifdef APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
#if defined(DSP_HIFI4_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
#include "rx_dsp_hifi4_trc.h"
#endif

#ifdef RMT_TRC_IN_MSG_CHAN
#define RMT_TRC_CHAN_ID                     HAL_SYS2BTH_ID_1
#else
#define RMT_TRC_CHAN_ID                     HAL_SYS2BTH_ID_0
#endif

static unsigned int mcpp_bth_dsp_rx_data_handler(const void *data, unsigned int len)
{
#if defined(DSP_HIFI4_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
    if (dsp_hifi4_trace_rx_handler(data, len)) {
        return len;
    }
#endif

    if (mcpp_check_rpc_cmd(len)) {
        mcpp_baremetal_received_data_handler(data, len);
        return len;
    }

    return len;
}

static void mcpp_bth_dsp_tx_data_done_handler(const void* data, unsigned int len)
{
    if (mcpp_check_rpc_cmd(len)) {
        mcpp_baremetal_send_data_done_handler(data, len);
    }
}

static int mcpp_bth_dsp_tx_data(const void *data, uint32_t len)
{
    return hal_sys2bth_send(RMT_TRC_CHAN_ID, data, len);
}
#else
void app_mcpp_task_hifi_process_done(void)
{
    TRACE(0, "[%s]", __func__);
}
#endif

static void mcpp_srv_core_hifi_init(void)
{
    app_dsp_hifi4_init(APP_DSP_HIFI4_USER_MCPP);

#ifdef APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
    int ret = 0;
    ret = hal_sys2bth_open(RMT_TRC_CHAN_ID, mcpp_bth_dsp_rx_data_handler, mcpp_bth_dsp_tx_data_done_handler, false);
    ASSERT(ret == 0, "hal_sys2bth_opened failed: %d", ret);
    ret = hal_sys2bth_start_recv(RMT_TRC_CHAN_ID);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);
#endif
}

static void mcpp_srv_core_hifi_deinit(void)
{
#ifdef APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
    hal_sys2bth_stop_recv(RMT_TRC_CHAN_ID);
    hal_sys2bth_close(RMT_TRC_CHAN_ID);
#endif

    app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_MCPP);
}

static bool mcpp_srv_core_hifi_inited(void)
{
#ifdef APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
    return hal_sys2bth_opened(RMT_TRC_CHAN_ID);
#else
    return app_dsp_hifi4_is_running();
#endif
}
#endif

#ifdef APP_MCPP_BTH_SENS_ENABLE
static void mcpp_srv_core_sensor_hub_init(void)
{
    app_sensor_hub_init();
}

static void mcpp_srv_core_sensor_hub_deinit(void)
{
    app_sensor_hub_deinit();
}
#endif

#ifdef APP_MCPP_BTH_CP_SUBSYS_ENABLE
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
static unsigned int mcpp_bth_cp_subsys_rx_data_handler(const void *data, unsigned int len)
{
    if (mcpp_check_rpc_cmd(len)) {
        mcpp_baremetal_received_data_handler(data, len);
        return len;
    }

    return len;
}

static void mcpp_bth_cp_subsys_tx_data_done_handler(const void* data, unsigned int len)
{
    if (mcpp_check_rpc_cmd(len)) {
        mcpp_baremetal_send_data_done_handler(data, len);
    }
}

static int mcpp_bth_cp_subsys_tx_data(const void *data, uint32_t len)
{
    return cp_subsys_send(data, len);
}
#endif

static void mcpp_srv_core_cp_subsys_init(void)
{
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    int ret = 0;
    ret = cp_subsys_open(mcpp_bth_cp_subsys_rx_data_handler, mcpp_bth_cp_subsys_tx_data_done_handler);
    ASSERT(ret == 0, "[%s]: cp_subsys_open failed, ret=%d", __func__, ret);
#endif
}

static void mcpp_srv_core_cp_subsys_deinit(void)
{
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    cp_subsys_close();
#endif
}

static bool mcpp_srv_core_cp_subsys_inited(void)
{
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    return cp_subsys_opened();
#endif
}
#endif

int32_t mcpp_core_open(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_BTH_M55_ENABLE
    if (server == APP_MCPP_CORE_M55){
        mcpp_src_core_m55_init();
    } else
#endif
#ifdef APP_MCPP_BTH_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        mcpp_srv_core_hifi_init();
    } else
#endif
#ifdef APP_MCPP_BTH_SENS_ENABLE
    if (server == APP_MCPP_CORE_SENS) {
        mcpp_srv_core_sensor_hub_init();
    } else
#endif
#ifdef APP_MCPP_BTH_CP_SUBSYS_ENABLE
    if (server == APP_MCPP_CORE_CP_SUBSYS) {
        mcpp_srv_core_cp_subsys_init();
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return 0;
}

bool server_core_is_opened(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_BTH_M55_ENABLE
    if (server == APP_MCPP_CORE_M55){
        return app_dsp_m55_is_running();
    } else
#endif
#ifdef APP_MCPP_BTH_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        return mcpp_srv_core_hifi_inited();
    } else
#endif
#ifdef APP_MCPP_BTH_SENS_ENABLE
    if (server == APP_MCPP_CORE_SENS) {
        return app_sensor_hub_is_inited();
    } else
#endif
#ifdef APP_MCPP_BTH_CP_SUBSYS_ENABLE
    if (server == APP_MCPP_CORE_CP_SUBSYS) {
        return mcpp_srv_core_cp_subsys_inited();
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return false;
}

bool server_core_is_running(APP_MCPP_CORE_T server)
{
    // Because of M55 and HIFI4's registration init mechanism,
    // Always return false to ensure that app_dsp_m55_init is called
#ifdef APP_MCPP_BTH_M55_ENABLE
    if (server == APP_MCPP_CORE_M55){
        return false;
    } else
#endif
#ifdef APP_MCPP_BTH_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        return false;
    } else
#endif
    {
        return server_core_is_opened(server);
    }
}

int32_t mcpp_core_close(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_BTH_M55_ENABLE
    if (server == APP_MCPP_CORE_M55){
        mcpp_srv_core_m55_deinit();
    } else
#endif
#ifdef APP_MCPP_BTH_HIFI_ENABLE
    if (server == APP_MCPP_CORE_HIFI) {
        mcpp_srv_core_hifi_deinit();
    } else
#endif
#ifdef APP_MCPP_BTH_SENS_ENABLE
    if (server == APP_MCPP_CORE_SENS) {
        mcpp_srv_core_sensor_hub_deinit();
    } else
#endif
#ifdef APP_MCPP_BTH_CP_SUBSYS_ENABLE
    if (server == APP_MCPP_CORE_CP_SUBSYS) {
        mcpp_srv_core_cp_subsys_deinit();
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return 0;
}

bool mcpp_target_core_is_baremetal(APP_MCPP_CORE_T server)
{
#ifdef APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
    if (server == APP_MCPP_CORE_HIFI){
        return true;
    } else
#endif
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    if (server == APP_MCPP_CORE_CP_SUBSYS){
        return true;
    } else
#endif
    {
        return false;
    }
}


int32_t mcpp_core_baremetal_send_data(APP_MCPP_CORE_T server, void *data, uint32_t len)
{
#ifdef APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
    if (server == APP_MCPP_CORE_HIFI){
        mcpp_bth_dsp_tx_data(data, len);
    } else
#endif
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    if (server == APP_MCPP_CORE_CP_SUBSYS){
        mcpp_bth_cp_subsys_tx_data(data, len);
    } else
#endif
    {
        ASSERT(0, "[%s]:dsp_server:%d is error", __func__, server);
    }
    return 0;
}
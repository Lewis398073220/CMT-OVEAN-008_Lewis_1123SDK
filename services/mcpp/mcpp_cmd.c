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
#include "plat_types.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "app_rpc_api.h"
#include "mcpp_cmd.h"
#include "mcpp_core.h"
#include "app_mcpp_cfg.h"

#define MCPP_CMD_SIGNAL_TIMEROUT_MS (1000)
#define MCPP_baremetal_DEFAULT_WAIT_RSP_TIMEOUT_MS (500)

const MCPP_CMD_CFG_T *g_mcpp_cmd_cfg[APP_MCPP_CORE_QTY] = {NULL};
static MCPP_RPC_T *g_baremetal_rpc_ptr = NULL;
mcpp_rpc_cmd_handler_t mcpp_rsp_done_handler = NULL;
mcpp_rpc_cmd_handler_t cmd_received_client_handler = NULL;
mcpp_rpc_cmd_handler_t cmd_received_server_handler = NULL;

const MCPP_CMD_CFG_T *mcpp_get_cmd_cfg(APP_MCPP_CORE_T core)
{
    const MCPP_CMD_CFG_T *cmd_cfg = g_mcpp_cmd_cfg[core];
    ASSERT(cmd_cfg != NULL, "[%s]:core:%d error, g_mcpp_core is %d", __func__, core, g_mcpp_core);
    return cmd_cfg;
}

static POSSIBLY_UNUSED void mcpp_cmd_wait_rsp_timeout(uint8_t* ptr, uint16_t len)
{
    MCPP_LOG_I("Warning %s", __func__);
}

static POSSIBLY_UNUSED void mcpp_cmd_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    if (mcpp_rsp_done_handler) {
        mcpp_rsp_done_handler(ptr, len);
    }
}

void mcpp_rpc_cmd_rsp_received_cb_register(mcpp_rpc_cmd_handler_t handler)
{
    mcpp_rsp_done_handler = handler;
}

static POSSIBLY_UNUSED int32_t mcpp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    if (cmd_received_client_handler) {
        return cmd_received_client_handler(ptr, len);
    }

    if (cmd_received_server_handler) {
        return cmd_received_server_handler(ptr, len);
    }

    MCPP_LOG_I("[%s]: cmd_received_handler is NULL!", __func__);
    return -1;
}

void mcpp_cmd_received_client_cb_register(mcpp_rpc_cmd_handler_t handler)
{
    cmd_received_client_handler = handler;
}

void mcpp_cmd_received_server_cb_register(mcpp_rpc_cmd_handler_t handler)
{
    cmd_received_server_handler = handler;
}

bool mcpp_check_rpc_cmd(uint16_t len)
{
    if (len == sizeof(MCPP_RPC_T)) {
        return true;
    } else {
        return false;
    }
}

static void mcpp_baremetal_set_cmd_null(MCPP_RPC_CMD_T *cmd)
{
    *cmd = MCPP_RPC_CMD_NULL;
}

static void mcpp_baremetal_send_data(uint8_t *data, uint16_t len, APP_MCPP_CORE_T tar_core, bool rsp_flag)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_core_baremetal_send_data(tar_core, data, len);
    int32_t wait_cnt = 0;

    while (rsp_flag) {
        MCPP_RPC_T *rpc_ptr = (MCPP_RPC_T *)data;
        if (rpc_ptr->cmd == MCPP_RPC_CMD_NULL) {
            mcpp_cmd_rsp_received_handler(NULL, 0);
            break;
        } else {
            hal_sys_timer_delay_us(10);
            if (wait_cnt++ > MCPP_baremetal_DEFAULT_WAIT_RSP_TIMEOUT_MS * 100) {    // 500ms
                ASSERT(0, "%s: playback server is hung", __func__);
            }
        }
    }
}

void mcpp_baremetal_send_data_done_handler(const void *data, uint32_t len)
{
    MCPP_LOG_D("[%s]", __func__);
}

uint32_t mcpp_baremetal_received_data_handler(const void *data, uint32_t len)
{
    MCPP_LOG_D("[%s]", __func__);
    ASSERT(len == sizeof(MCPP_RPC_T), "[%s] len(%d) != %d", __func__, len, sizeof(MCPP_RPC_T));

    g_baremetal_rpc_ptr = (MCPP_RPC_T *)data;
    int32_t ret = mcpp_cmd_received_handler((uint8_t *)g_baremetal_rpc_ptr, len);

    if (ret == 0) {
        mcpp_baremetal_set_cmd_null(&g_baremetal_rpc_ptr->cmd);
    } else if (ret == -1) {
        ASSERT(false, "%s cmd is error", __func__);
    }

    return len;
}

#ifdef RTOS
#include "cmsis_os.h"
osSemaphoreDef(mcpp_cmd_sema);
osSemaphoreId mcpp_cmd_sema_id = NULL;

static int32_t mcpp_cmd_sema_init(void)
{
    if (mcpp_cmd_sema_id == NULL) {
        mcpp_cmd_sema_id = osSemaphoreCreate(osSemaphore(mcpp_cmd_sema), 0);
    }
    ASSERT(mcpp_cmd_sema_id, "create mcpp_cmd_sema_id fail!");
    return 0;
}

static int32_t mcpp_cmd_sema_deinit(void)
{
    int32_t ret = 0;
    if(mcpp_cmd_sema_id) {
        ret = osSemaphoreDelete(mcpp_cmd_sema_id);
        mcpp_cmd_sema_id = NULL;
    }
    return ret;
}

static int32_t mcpp_cmd_sema_wait_rsp_task(uint32_t timeout)
{
    int32_t ret = 0;
    if(mcpp_cmd_sema_id) {
        ret = osSemaphoreAcquire(mcpp_cmd_sema_id, timeout);
    }
    return ret;
}

static int32_t mcpp_cmd_task_process_done(uint8_t *ptr, uint16_t len)
{
    if(mcpp_cmd_sema_id) {
        osSemaphoreRelease(mcpp_cmd_sema_id);
    }
    return 0;
}

int32_t mcpp_cmd_init(void)
{
    mcpp_cmd_sema_init();
    mcpp_rpc_cmd_rsp_received_cb_register(mcpp_cmd_task_process_done);
    mcpp_load_core_cmd_cfg();
    return 0;
}

int32_t mcpp_cmd_deinit(void)
{
    int32_t ret = 0;
    ret = mcpp_cmd_sema_deinit();
    mcpp_rpc_cmd_rsp_received_cb_register(NULL);
    return ret;
}

static POSSIBLY_UNUSED void mcpp_rsp_cmd_transmit_handler(uint8_t* ptr, uint16_t len)
{
    MCPP_LOG_D("[%s]", __func__);
    ASSERT(ptr != NULL, "[%s]:ptr is NULL", __func__);
    const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(((MCPP_RPC_T *)ptr)->core_server);
    app_rpc_send_data_waiting_rsp(cmd_cfg->core, cmd_cfg->rsp_cmd, ptr, len);
}

static POSSIBLY_UNUSED void mcpp_no_rsp_cmd_transmit_handler(uint8_t* ptr, uint16_t len)
{
    MCPP_LOG_D("[%s]", __func__);
    ASSERT(ptr != NULL, "[%s]:ptr is NULL", __func__);
    MCPP_RPC_T *rpc_ptr = (MCPP_RPC_T *)ptr;
    if (rpc_ptr->core_client == g_mcpp_core) {
        const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(rpc_ptr->core_server);
        app_rpc_send_data_no_rsp(cmd_cfg->core, cmd_cfg->no_rsp_cmd, ptr, len);
    } else if (rpc_ptr->core_server == g_mcpp_core) {
        const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(rpc_ptr->core_client);
        app_rpc_send_data_no_rsp(cmd_cfg->core, cmd_cfg->no_rsp_cmd, ptr, len);
    } else {
        ASSERT(false, "[%s]:core_client and core_server is wrong!", __func__);
    }
}

static POSSIBLY_UNUSED void mcpp_rsp_cmd_received_handler(uint8_t *data, uint16_t len)
{
    MCPP_LOG_D("[%s]", __func__);
    ASSERT(len == sizeof(MCPP_RPC_T), "[%s] len(%d) != %d", __func__, len, sizeof(MCPP_RPC_T));

    int32_t ret = mcpp_cmd_received_handler(data, len);

    if (ret == 0) {
        MCPP_RPC_T *rpc_ptr = (MCPP_RPC_T *)data;
        const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(rpc_ptr->core_client);
        app_rpc_send_cmd_rsp(cmd_cfg->core, cmd_cfg->rsp_cmd, data, len);
    } else if (ret == -1) {
        ASSERT(false, "%s cmd is error", __func__);
    }
}

static POSSIBLY_UNUSED void mcpp_no_rsp_cmd_received_handler(uint8_t *data, uint16_t len)
{
    MCPP_LOG_D("[%s]", __func__);
    ASSERT(len == sizeof(MCPP_RPC_T), "[%s] len(%d) != %d", __func__, len, sizeof(MCPP_RPC_T));

    int32_t ret = mcpp_cmd_received_handler(data, len);

    if (ret == -1) {
        ASSERT(false, "%s cmd is error", __func__);
    }
}

void mcpp_send_rsp_cmd(uint8_t *data, uint16_t len, APP_MCPP_CORE_T target_core)
{
    MCPP_RPC_T *rpc_ptr = (MCPP_RPC_T *)data;

    if (mcpp_target_core_is_baremetal(target_core)) {
        mcpp_baremetal_send_data(data, len, target_core, true);
    } else {
        const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(target_core);
        app_rpc_send_cmd(cmd_cfg->core, cmd_cfg->rsp_cmd, data, len);
    }

    int32_t res = mcpp_cmd_sema_wait_rsp_task(MCPP_CMD_SIGNAL_TIMEROUT_MS);
    ASSERT(res == osOK, "[%s] cmd:%d, res:%d", __func__, rpc_ptr->cmd, res);
}

void mcpp_send_no_rsp_cmd(uint8_t *data, uint16_t len, APP_MCPP_CORE_T target_core)
{
    if (target_core == g_mcpp_core) {
        mcpp_no_rsp_cmd_received_handler(data, len);
        return;
    }

    if (mcpp_target_core_is_baremetal(target_core)) {
        mcpp_baremetal_send_data(data, len, target_core, false);
    } else {
        const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(target_core);
        app_rpc_send_cmd(cmd_cfg->core, cmd_cfg->no_rsp_cmd, data, len);
    }
}

void mcpp_cmd_send_empty_rsp(APP_MCPP_CORE_T core_client)
{
    MCPP_LOG_D("[%s]", __func__);
    const MCPP_CMD_CFG_T *cmd_cfg = mcpp_get_cmd_cfg(core_client);
    app_rpc_send_cmd_rsp(cmd_cfg->core, cmd_cfg->rsp_cmd, NULL, 0);
}

#ifdef APP_MCPP_BTH_M55_ENABLE
#include "app_dsp_m55.h"
/*****************************BTH to M55*****************************/
M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_MCPP_RSP,
                                    "MCU_DSP_M55_TASK_CMD_MCPP_RSP",
                                    mcpp_rsp_cmd_transmit_handler,
                                    mcpp_rsp_cmd_received_handler,
                                    RPC_CMD_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                    mcpp_cmd_wait_rsp_timeout,
                                    mcpp_cmd_rsp_received_handler,
                                    NULL);

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_MCPP_NO_RSP,
                                    "MCU_DSP_M55_TASK_CMD_MCPP_NO_RSP",
                                    mcpp_no_rsp_cmd_transmit_handler,
                                    mcpp_no_rsp_cmd_received_handler,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL);
#endif

#ifdef APP_MCPP_BTH_HIFI_ENABLE
#include "rpc_bth_dsp.h"
/*****************************BTH to HIFI*****************************/
RPC_BTH_DSP_TASK_CMD_TO_ADD(BTH_DSP_TASK_CMD_MCPP_RSP,
                            "BTH_DSP_TASK_CMD_MCPP_RSP",
                            mcpp_rsp_cmd_transmit_handler,
                            mcpp_rsp_cmd_received_handler,
                            RPC_CMD_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                            mcpp_cmd_wait_rsp_timeout,
                            mcpp_cmd_rsp_received_handler,
                            NULL);

RPC_BTH_DSP_TASK_CMD_TO_ADD(BTH_DSP_TASK_CMD_MCPP_NO_RSP,
                            "BTH_DSP_TASK_CMD_MCPP_NO_RSP",
                            mcpp_no_rsp_cmd_transmit_handler,
                            mcpp_no_rsp_cmd_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);
#endif

#ifdef APP_MCPP_M55_HIFI_ENABLE
#include "rpc_m55_dsp.h"
/*****************************M55 to HIFI*****************************/
RPC_M55_DSP_TASK_CMD_TO_ADD(M55_DSP_TASK_CMD_MCPP_RSP,
                            "M55_DSP_TASK_CMD_MCPP_RSP",
                            mcpp_rsp_cmd_transmit_handler,
                            mcpp_rsp_cmd_received_handler,
                            RPC_CMD_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                            mcpp_cmd_wait_rsp_timeout,
                            mcpp_cmd_rsp_received_handler,
                            NULL);

RPC_M55_DSP_TASK_CMD_TO_ADD(M55_DSP_TASK_CMD_MCPP_NO_RSP,
                            "M55_DSP_TASK_CMD_MCPP_NO_RSP",
                            mcpp_no_rsp_cmd_transmit_handler,
                            mcpp_no_rsp_cmd_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);
#endif

#ifdef APP_MCPP_BTH_SENS_ENABLE
#include "app_sensor_hub.h"
/*****************************BTH to M55*****************************/
CORE_BRIDGE_TASK_COMMAND_TO_ADD(BTH_SENS_TASK_CMD_MCPP_RSP,
                                "BTH_SENS_TASK_CMD_MCPP_RSP",
                                mcpp_rsp_cmd_transmit_handler,
                                mcpp_rsp_cmd_received_handler,
                                RPC_CMD_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                mcpp_cmd_wait_rsp_timeout,
                                mcpp_cmd_rsp_received_handler,
                                NULL);

CORE_BRIDGE_TASK_COMMAND_TO_ADD(BTH_SENS_TASK_CMD_MCPP_NO_RSP,
                                "BTH_SENS_TASK_CMD_MCPP_NO_RSP",
                                mcpp_no_rsp_cmd_transmit_handler,
                                mcpp_no_rsp_cmd_received_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);
#endif
#else
void mcpp_send_rsp_cmd(uint8_t *data, uint16_t len, APP_MCPP_CORE_T target_core)
{
    if (mcpp_target_core_is_baremetal(target_core)) {
        mcpp_baremetal_send_data(data, len, target_core, true);
    }
}

void mcpp_send_no_rsp_cmd(uint8_t *data, uint16_t len, APP_MCPP_CORE_T target_core)
{
    if (mcpp_target_core_is_baremetal(target_core)) {
        mcpp_baremetal_send_data(data, len, target_core, false);
    }
}

void mcpp_cmd_send_empty_rsp(APP_MCPP_CORE_T core_client)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_baremetal_set_cmd_null(&g_baremetal_rpc_ptr->cmd);
}
#endif
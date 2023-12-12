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
#include "hal_location.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "speech_cfg.h"
#include "math.h"
#include "norflash_api.h"
#include "app_rpc_api.h"
#include "mcpp_client.h"
#include "mcpp_cmd.h"
#include "mcpp_core.h"
#include "app_mcpp_cfg.h"

#define MCPP_CLI_SIGNAL_TIMEROUT_MS (1000)

static const osMutexAttr_t MCPP_Cli_MutexAttr = {
    .name = "MCPP_CLIENT_MUTEX",
    .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
    .cb_mem = NULL,
    .cb_size = 0U,
};
static osMutexId_t mcpp_cli_mutex_id = NULL;

void mcpp_cli_lock_init(void)
{
    if (mcpp_cli_mutex_id == NULL) {
        mcpp_cli_mutex_id = osMutexNew(&MCPP_Cli_MutexAttr);
        ASSERT(mcpp_cli_mutex_id, "[%s]:mutex init error", __func__);
    }
}

static void mcpp_cli_lock(void)
{
    osMutexAcquire(mcpp_cli_mutex_id, osWaitForever);
}

static void mcpp_cli_unlock(void)
{
    osMutexRelease(mcpp_cli_mutex_id);
}

typedef struct {
    bool inited_status;
    MCPP_RPC_T rpc_cfg;
    MCPP_CLI_INIT_CFG_T init_cfg;
} _MCPP_CLI_CFG_T;

static int32_t core_register_number[APP_MCPP_CORE_QTY] = {0};
static _MCPP_CLI_CFG_T g_mcpp_cli_cfg[APP_MCPP_USER_QTY] = {0};
static mcpp_cli_result_process_handler_t mcpp_cli_result_handler = NULL;

static _MCPP_CLI_CFG_T *mcpp_cli_get_cfg(APP_MCPP_USER_T user)
{
    return &g_mcpp_cli_cfg[user];
}

static MCPP_CLI_INIT_CFG_T *mcpp_cli_get_init_cfg(APP_MCPP_USER_T user)
{
    _MCPP_CLI_CFG_T *cli_cfg = mcpp_cli_get_cfg(user);
    return &cli_cfg->init_cfg;
}

static MCPP_RPC_T *mcpp_cli_get_rpc_cfg(APP_MCPP_USER_T user)
{
    _MCPP_CLI_CFG_T *cli_cfg = mcpp_cli_get_cfg(user);
    return  &cli_cfg->rpc_cfg;
}

static int32_t mcpp_cli_get_inited_user_num(void)
{
    int32_t user_num = 0;
    for (int32_t i = 0; i < APP_MCPP_USER_QTY; i++) {
        if (mcpp_cli_get_cfg(i)->inited_status) {
            user_num += 1;
        }
    }
    return user_num;
}

static void mcpp_cli_set_core_register_to_zero(void)
{
    for (int32_t i = 0; i < APP_MCPP_CORE_QTY; i++) {
        core_register_number[i] = 0;
    }
}

static void mcpp_cli_result_process_handler(APP_MCPP_USER_T user, uint8_t* ptr, uint16_t len)
{
    if (mcpp_cli_result_handler) {
        mcpp_cli_result_handler(user, ptr, len);
    } else {
        MCPP_LOG_I("[%s]: mcpp_cli_result_handler is NULL!", __func__);
    }
}

void mcpp_cli_result_process_cb_register(mcpp_cli_result_process_handler_t handler)
{
    mcpp_cli_result_handler = handler;
}

static int32_t mcpp_cli_received_cmd_handler(uint8_t *ptr, uint16_t ptr_len)
{
    MCPP_LOG_D("%s...", __func__);
    MCPP_RPC_T *rpc_ptr = (MCPP_RPC_T *)ptr;
    switch (rpc_ptr->cmd)
    {
    case MCPP_RPC_CMD_RETURN_RESULT:
        mcpp_cli_result_process_handler(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    default:
        break;
    }
    return 1;
}

int32_t mcpp_cli_init(APP_MCPP_USER_T user, MCPP_CLI_INIT_CFG_T *cfg)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    _MCPP_CLI_CFG_T *cli_cfg = mcpp_cli_get_cfg(user);

    mcpp_cli_lock();
    if (cli_cfg->inited_status) {
        if ((cli_cfg->init_cfg.capture_enable == cfg->capture_enable) &&
        (cli_cfg->init_cfg.playback_enable == cfg->playback_enable) &&
        (cli_cfg->init_cfg.capture_server == cfg->capture_server) &&
        (cli_cfg->init_cfg.playback_server == cfg->playback_server))
        {
            MCPP_LOG_I("[%s]:mcpp client was successfully initialized!", __func__);
            mcpp_cli_unlock();
            return 0;
        } else {
            ASSERT(0, "[%s] Error: The initialization parameters are inconsistent.", __func__);
        }
    } else {
        cli_cfg->init_cfg = *cfg;
        cli_cfg->rpc_cfg.user = user;
    }

    MCPP_LOG_I("[%s]:There is the info of mcpp init!", __func__);
    MCPP_LOG_I("user:             %d", user);
    MCPP_LOG_I("capture_enable:   %d", cfg->capture_enable);
    MCPP_LOG_I("playback_enable:  %d", cfg->playback_enable);
    MCPP_LOG_I("capture_server:   %d", cfg->capture_server);
    MCPP_LOG_I("playback_server:  %d", cfg->playback_server);

    if (mcpp_cli_get_inited_user_num() == 0) {
        mcpp_cmd_init();
        mcpp_cmd_received_server_cb_register(mcpp_cli_received_cmd_handler);
        mcpp_cli_set_core_register_to_zero();
    }

    if (cfg->capture_enable) {
        if (cfg->capture_server != g_mcpp_core) {
            if (core_register_number[cfg->capture_server] == 0) {
                if (server_core_is_running(cfg->capture_server) == false) {
                    mcpp_core_open(cfg->capture_server);
                } else {
                    core_register_number[cfg->capture_server] = 1;
                }
            }
            core_register_number[cfg->capture_server] += 1;
        }
    }

    if (cfg->playback_enable) {
        if (cfg->playback_server != g_mcpp_core) {
            if (core_register_number[cfg->playback_server] == 0) {
                if (server_core_is_running(cfg->playback_server) == false) {
                    mcpp_core_open(cfg->playback_server);
                } else {
                    core_register_number[cfg->playback_server] = 1;
                }
            }
            core_register_number[cfg->playback_server] += 1;
        }
    }

    cli_cfg->inited_status = true;
    mcpp_cli_unlock();
    return 0;
}

int32_t mcpp_cli_wait_server_core_inited(APP_MCPP_USER_T user)
{
    int32_t wait_cnt = 0;
    _MCPP_CLI_CFG_T *cli_cfg = mcpp_cli_get_cfg(user);

    if (cli_cfg->inited_status == false){
        ASSERT(0, "[%s]:mcpp client was not successfully initialized!", __func__);
    }

    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    if (init_cfg->capture_enable) {
        if (init_cfg->capture_server != g_mcpp_core) {
            while (server_core_is_opened(init_cfg->capture_server) == false) {
                hal_sys_timer_delay_us(10);
                if (wait_cnt++ > 300000) {    // 3s
                    ASSERT(0, "%s: capture server is hung", __func__);
                }
            }
        }
    }

    if (init_cfg->playback_enable) {
        if (init_cfg->playback_server != g_mcpp_core) {
            while (server_core_is_opened(init_cfg->playback_server) == false) {
                hal_sys_timer_delay_us(10);
                if (wait_cnt++ > 300000) {    // 3s
                    ASSERT(0, "%s: playback server is hung", __func__);
                }
            }
        }
    }

    return 0;
}

int32_t mcpp_cli_deinit(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]... The user:%d!", __func__, user);
    mcpp_cli_lock();
    _MCPP_CLI_CFG_T *cli_cfg = mcpp_cli_get_cfg(user);

    if (cli_cfg->inited_status == false){
        TRACE(0, "[%s]:not succeed init", __func__);
        mcpp_cli_unlock();
        return -1;
    }

    cli_cfg->inited_status = false;

    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    if (init_cfg->capture_enable && init_cfg->capture_server != g_mcpp_core) {
        core_register_number[init_cfg->capture_server] -= 1;
        if (core_register_number[init_cfg->capture_server] == 0) {
            mcpp_core_close(init_cfg->capture_server);
        }
    }
    if (init_cfg->playback_enable && init_cfg->playback_server != g_mcpp_core) {
        core_register_number[init_cfg->playback_server] -= 1;
        if (core_register_number[init_cfg->playback_server] == 0) {
            mcpp_core_close(init_cfg->playback_server);
        }
    }

    if (mcpp_cli_get_inited_user_num() == 0) {
        mcpp_cmd_received_server_cb_register(NULL);
        mcpp_cmd_deinit();
        mcpp_cli_set_core_register_to_zero();
    }
    mcpp_cli_unlock();
    return 0;
}

int32_t mcpp_cli_stream_capture_open(APP_MCPP_USER_T user, void *info, uint16_t info_len)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_CAPTURE_OPEN;
    rpc_cfg->ptr = info;
    rpc_cfg->ptr_len = info_len;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->capture_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->capture_server);

    return 0;
}

int32_t mcpp_cli_stream_playback_open(APP_MCPP_USER_T user, void *info, uint16_t info_len)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_PLAYBACK_OPEN;
    rpc_cfg->ptr = info;
    rpc_cfg->ptr_len = info_len;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->playback_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->playback_server);

    return 0;
}

int32_t mcpp_cli_stream_capture_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_CAPTURE_CLOSE;
    rpc_cfg->ptr = NULL;
    rpc_cfg->ptr_len = 0;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->capture_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->capture_server);

    return 0;
}

int32_t mcpp_cli_stream_playback_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_PLAYBACK_CLOSE;
    rpc_cfg->ptr = NULL;
    rpc_cfg->ptr_len = 0;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->playback_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->playback_server);

    return 0;
}

int32_t mcpp_cli_send_capture_trigger(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg)
{
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_CAPTURE_PROCESS;
    rpc_cfg->ptr = pcm_cfg;
    rpc_cfg->ptr_len = sizeof(APP_MCPP_CAP_PCM_T);
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->capture_server;
    mcpp_send_no_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->capture_server);

    return 0;
}

int32_t mcpp_cli_send_playback_trigger(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg)
{
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_PLAYBACK_PROCESS;
    rpc_cfg->ptr = pcm_cfg;
    rpc_cfg->ptr_len = sizeof(APP_MCPP_CAP_PCM_T);
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->playback_server;
    mcpp_send_no_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->playback_server);

    return 0;
}

int32_t mcpp_cli_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len)
{
    MCPP_LOG_D("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_CAPTURE_SET_CFG;
    rpc_cfg->ptr = cfg;
    rpc_cfg->ptr_len = cfg_len;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->capture_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->capture_server);

    return 0;
}

int32_t mcpp_cli_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len)
{
    MCPP_LOG_D("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_PLAYBACK_SET_CFG;
    rpc_cfg->ptr = cfg;
    rpc_cfg->ptr_len = cfg_len;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->playback_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->playback_server);

    return 0;
}

int32_t mcpp_cli_capture_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len)
{
    MCPP_LOG_D("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_CAPTURE_CTRL;
    rpc_cfg->ptr = ctl_ptr;
    rpc_cfg->ptr_len = ptr_len;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->capture_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->capture_server);
    return 0;
}

int32_t mcpp_cli_playback_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len)
{
    MCPP_LOG_D("[%s]:The user:%d!", __func__, user);
    MCPP_CLI_INIT_CFG_T *init_cfg = mcpp_cli_get_init_cfg(user);
    MCPP_RPC_T *rpc_cfg = mcpp_cli_get_rpc_cfg(user);

    rpc_cfg->cmd = MCPP_RPC_CMD_PLAYBACK_CTRL;
    rpc_cfg->ptr = ctl_ptr;
    rpc_cfg->ptr_len = ptr_len;
    rpc_cfg->core_client = g_mcpp_core;
    rpc_cfg->core_server = init_cfg->playback_server;
    mcpp_send_rsp_cmd((uint8_t *)rpc_cfg, sizeof(MCPP_RPC_T), init_cfg->playback_server);
    return 0;
}
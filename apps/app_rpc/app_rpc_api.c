/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"
#include "hal_mcu2sens.h"
#include "hal_sys2bth.h"
#include "app_rpc_api.h"
#include "rpc_rx_thread.h"

#ifdef APP_RPC_BTH_DSP_EN
#include "rpc_bth_dsp.h"
#endif

#ifdef APP_RPC_BTH_M55_EN
#include "app_dsp_m55.h"
#endif

#ifdef APP_RPC_M55_DSP_EN
#include "rpc_m55_dsp.h"
#endif

#ifdef APP_RPC_MCU_SENSOR_EN
#include "app_sensor_hub.h"
#endif

static bth_ready_ping_handle_cb bth_ping_timeout_ind_cb = NULL;
static bth_ready_ping_handle_cb bth_ping_rsp_ind_cb = NULL;
static bth_ready_ping_handle_cb bth_ping_rcv_ind_cb = NULL;

static bth_ready_ping_handle_cb bth_ready_ind_send_timeout_cb = NULL;
static bth_ready_ping_handle_cb bth_ready_ind_recv_resp_cb = NULL;


WEAK void app_bth_server_ready_notify(uint8_t ready_flag)
{

}

int32_t app_rpc_send_data_no_rsp(enum APP_RPC_CORE_T core, uint16_t cmdcode, uint8_t *p_buff, uint16_t length)
{
    int32_t ret = RPC_RES_FAILD;
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
            app_dsp_m55_bridge_send_data_without_waiting_rsp(cmdcode, p_buff, length);
            ret = RPC_RES_SUCCESS;
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            ret = rpc_bth_dsp_send_data_no_rsp(cmdcode, p_buff, length);
            break;
#endif

        case APP_RPC_CORE_MCU_SENSOR:
#ifdef APP_RPC_MCU_SENSOR_EN
            app_core_bridge_send_data_without_waiting_rsp(cmdcode, p_buff, length);
            ret = RPC_RES_SUCCESS;
            break;
#endif
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            ret = rpc_m55_dsp_send_data_no_rsp(cmdcode, p_buff, length);
            break;
#endif
        default:
            break;
    }
    return ret;
}

int32_t app_rpc_send_data_waiting_rsp(enum APP_RPC_CORE_T core, uint16_t cmdcode, uint8_t *p_buff, uint16_t length)
{
    int32_t ret = RPC_RES_FAILD;
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
            app_dsp_m55_bridge_send_data_with_waiting_rsp(cmdcode, p_buff, length);
            ret = RPC_RES_SUCCESS;
#endif
            break;
#ifdef APP_RPC_BTH_DSP_EN
        case APP_RPC_CORE_BTH_DSP:
            ret = rpc_bth_dsp_send_data_wait_rsp(cmdcode, p_buff, length);
            break;
#endif

        case APP_RPC_CORE_MCU_SENSOR:
#ifdef APP_RPC_MCU_SENSOR_EN
            app_core_bridge_send_data_with_waiting_rsp(cmdcode, p_buff, length);
            ret = RPC_RES_SUCCESS;
            break;
#endif
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            ret = rpc_m55_dsp_send_data_wait_rsp(cmdcode, p_buff, length);
            break;
#endif
        default:
            break;
    }
    return ret;
}

int32_t app_rpc_send_cmd(enum APP_RPC_CORE_T core, uint16_t cmd_code, uint8_t *p_buff, uint16_t length)
{
    int32_t ret = RPC_RES_FAILD;
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
            ret = app_dsp_m55_bridge_send_cmd(cmd_code, p_buff, length);
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            ret = rpc_bth_dsp_send_cmd(cmd_code, p_buff, length);
            break;
#endif

        case APP_RPC_CORE_MCU_SENSOR:
#ifdef APP_RPC_MCU_SENSOR_EN
            ret = app_core_bridge_send_cmd(cmd_code, p_buff, length);
            break;
#endif
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            ret = rpc_m55_dsp_send_cmd(cmd_code, p_buff, length);
            break;
#endif
        default:
            break;
    }

   return ret;
}

int32_t app_rpc_send_cmd_rsp(enum APP_RPC_CORE_T core, uint16_t rsp_code, uint8_t *p_buff, uint16_t length)
{
    int ret = RPC_RES_FAILD;
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
            ret = app_dsp_m55_bridge_send_rsp(rsp_code, p_buff, length);
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            ret = rpc_bth_dsp_send_cmd_rsp(rsp_code, p_buff, length);
#endif
            break;

        case APP_RPC_CORE_MCU_SENSOR:
#ifdef APP_RPC_MCU_SENSOR_EN
            ret = app_core_bridge_send_rsp(rsp_code, p_buff, length);
            break;
#endif
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            ret = rpc_m55_dsp_send_cmd_rsp(rsp_code, p_buff, length);
            break;
#endif
        default:
            break;
    }

   return ret;
}

void app_rpc_ctx_init(enum APP_RPC_CORE_T core)
{
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
            app_dsp_m55_bridge_init();
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            rpc_bth_dsp_ctx_init();
            rpc_rx_thread_init();
#endif
            break;
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            rpc_m55_dsp_ctx_init();
            rpc_rx_thread_init();
#endif
            break;
        default:
            break;
    }
}

void app_rpc_ctx_deinit(enum APP_RPC_CORE_T core)
{
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
            // deinit
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            rpc_bth_dsp_ctx_deinit();
#endif
            break;
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            rpc_m55_dsp_ctx_deinit();
#endif
            break;
        default:
            break;
    }
}

void app_rpc_core_open(enum APP_RPC_CORE_T core)
{
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            bth_dsp_core_open();
#endif
            break;
        case APP_RPC_CORE_DSP_M55:
#ifdef APP_RPC_M55_DSP_EN
            m55_dsp_core_open();
#endif
            break;
        default:
            break;
    }
}

void app_rpc_rx_handler_process(enum APP_RPC_CORE_T core, const void *data, unsigned int len)
{
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            bth_dsp_data_received(data, len);
#endif
            break;
        case APP_RPC_CORE_DSP_M55:
            break;
        default:
            break;
    }
}

void app_rpc_tx_handler_process(enum APP_RPC_CORE_T core, const void *data, unsigned int len)
{
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
#ifdef APP_RPC_BTH_M55_EN
#endif
            break;
        case APP_RPC_CORE_BTH_DSP:
#ifdef APP_RPC_BTH_DSP_EN
            bth_dsp_data_tx_done(data, len);
#endif
            break;
        case APP_RPC_CORE_DSP_M55:
            break;
        default:
            break;
    }
}

void app_rpc_ping_cmd_send_handle(enum APP_RPC_CORE_T core)
{
    TRACE(0, "sys ping core %d start", core);
    switch (core)
    {
        case APP_RPC_CORE_BTH_M55:
        {
#ifdef APP_RPC_BTH_M55_EN
#endif
        }
        break;
        case APP_RPC_CORE_BTH_DSP:
        {
#ifdef APP_RPC_BTH_DSP_EN
            rpc_bth_dsp_send_cmd(BTH_CORE_ALIVE_POLL_CMD, NULL, 0);
#endif
        }
        break;
        case APP_RPC_CORE_MCU_SENSOR:
        {
#ifdef APP_RPC_MCU_SENSOR_EN
            app_core_bridge_send_cmd(BTH_CORE_ALIVE_POLL_CMD, NULL, 0);
#endif
        }
        break;
        default:
        break;
    }
}

void app_rpc_ping_handle_cb_regist(bth_ready_ping_handle_cb tmo_cb, bth_ready_ping_handle_cb rsp_cb)
{
    bth_ping_timeout_ind_cb = tmo_cb;
    bth_ping_rsp_ind_cb = rsp_cb;
}

void app_rpc_ping_recv_handle_cb_regist(bth_ready_ping_handle_cb rcv_cb)
{
    bth_ping_rcv_ind_cb = rcv_cb;
}

POSSIBLY_UNUSED static void bth_dsp_cmd_ping_transmit_handler(uint8_t *p_buff, uint16_t length)
{
#ifdef APP_RPC_BTH_DSP_EN
    rpc_bth_dsp_send_data_wait_rsp(BTH_CORE_ALIVE_POLL_CMD, p_buff, length);
#endif

#ifdef APP_RPC_MCU_SENSOR_EN
    app_core_bridge_send_data_with_waiting_rsp(BTH_CORE_ALIVE_POLL_CMD, p_buff, length);
#endif
}

POSSIBLY_UNUSED static void bth_dsp_cmd_ping_receive_handler(uint8_t *p_buff, uint16_t length)
{
    uint8_t status = 0;
    if (bth_ping_rcv_ind_cb) {
        status = bth_ping_rcv_ind_cb(true);
    }
    //TRACE(1, "bth ping cmd recv status:%d", status);
    POSSIBLY_UNUSED uint8_t buffer[1];
    buffer[0] = status;
#ifdef APP_RPC_BTH_DSP_EN
    rpc_bth_dsp_send_cmd_rsp(BTH_CORE_ALIVE_POLL_CMD, buffer, 1);
#endif
#ifdef APP_RPC_MCU_SENSOR_EN
    app_core_bridge_send_rsp(BTH_CORE_ALIVE_POLL_CMD, buffer, 1);
#endif
}

POSSIBLY_UNUSED static void bth_dsp_cmd_ping_timeout_handler(uint8_t* ptr, uint16_t len)
{
    uint8_t status = 0;
    if (ptr) {
        status = ptr[0];
    }
    if (bth_ping_timeout_ind_cb) {
        bth_ping_timeout_ind_cb(status);
    }
}

POSSIBLY_UNUSED static void bth_dsp_cmd_ping_response_handler(uint8_t* ptr, uint16_t len)
{
    uint8_t status = 0;
    if (ptr) {
        status = ptr[0];
    }
    if (bth_ping_rsp_ind_cb) {
        bth_ping_rsp_ind_cb(status);
    }
}

void app_rpc_bth_ready_ind_send_handler_cb_regist(bth_ready_ping_handle_cb tmo_cb,
                                                                        bth_ready_ping_handle_cb rsp_cb)
{
    bth_ready_ind_send_timeout_cb = tmo_cb;
    bth_ready_ind_recv_resp_cb = rsp_cb;
}

POSSIBLY_UNUSED static void bth_stack_ready_ind_cmd_trans_handler(uint8_t *p_buff, uint16_t length)
{
#ifdef APP_RPC_BTH_DSP_EN
    rpc_bth_dsp_send_data_wait_rsp(BTH_TASK_CMD_STACK_READY_IND, p_buff, length);
#endif

#ifdef APP_RPC_MCU_SENSOR_EN
    app_core_bridge_send_data_with_waiting_rsp(BTH_TASK_CMD_STACK_READY_IND, p_buff, length);
#endif
}

POSSIBLY_UNUSED static void bth_stack_ready_ind_cmd_timeout_handler(uint8_t* ptr, uint16_t len)
{
    if (bth_ready_ind_send_timeout_cb) {
        bth_ready_ind_send_timeout_cb(false);
    }
}

POSSIBLY_UNUSED static void bth_stack_ready_ind_cmd_resp_handler(uint8_t* ptr, uint16_t len)
{
    if (bth_ready_ind_recv_resp_cb) {
        bth_ready_ind_recv_resp_cb(true);
    }
}

POSSIBLY_UNUSED static void bth_stack_ready_ind_cmd_recv_handler(uint8_t *p_buff, uint16_t length)
{
    app_bth_server_ready_notify(true);

#ifdef APP_RPC_BTH_DSP_EN
    rpc_bth_dsp_send_cmd_rsp(BTH_TASK_CMD_STACK_READY_IND, NULL, 0);
#endif
#ifdef APP_RPC_MCU_SENSOR_EN
    app_core_bridge_send_rsp(BTH_TASK_CMD_STACK_READY_IND, NULL, 0);
#endif
}

#ifdef APP_RPC_BTH_DSP_EN
RPC_BTH_DSP_TASK_CMD_TO_ADD(BTH_CORE_ALIVE_POLL_CMD,
                            "bth core polling cmd",
                            bth_dsp_cmd_ping_transmit_handler,
                            bth_dsp_cmd_ping_receive_handler,
                            500,
                            bth_dsp_cmd_ping_timeout_handler,
                            bth_dsp_cmd_ping_response_handler,
                            NULL);
RPC_BTH_DSP_TASK_CMD_TO_ADD(BTH_TASK_CMD_STACK_READY_IND,
                            "bthost stack ready ind ",
                            bth_stack_ready_ind_cmd_trans_handler,
                            bth_stack_ready_ind_cmd_recv_handler,
                            1000,
                            bth_stack_ready_ind_cmd_timeout_handler,
                            bth_stack_ready_ind_cmd_resp_handler,
                            NULL);
#endif

#ifdef APP_RPC_MCU_SENSOR_EN
CORE_BRIDGE_TASK_COMMAND_TO_ADD(BTH_CORE_ALIVE_POLL_CMD,
                                "bth core polling cmd",
                                bth_dsp_cmd_ping_transmit_handler,
                                bth_dsp_cmd_ping_receive_handler,
                                500,
                                bth_dsp_cmd_ping_timeout_handler,
                                bth_dsp_cmd_ping_response_handler,
                                NULL);
CORE_BRIDGE_TASK_COMMAND_TO_ADD(BTH_TASK_CMD_STACK_READY_IND,
                                "bthost stack ready ind ",
                                bth_stack_ready_ind_cmd_trans_handler,
                                bth_stack_ready_ind_cmd_recv_handler,
                                1000,
                                bth_stack_ready_ind_cmd_timeout_handler,
                                bth_stack_ready_ind_cmd_resp_handler,
                                NULL);
#endif

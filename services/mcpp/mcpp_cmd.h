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
#ifndef __MCPP_CMD_H__
#define __MCPP_CMD_H__

#include "app_mcpp_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t    core;

    uint16_t    rsp_cmd;
    uint16_t    no_rsp_cmd;
} MCPP_CMD_CFG_T;

typedef enum {
    MCPP_RPC_CMD_NULL,
    // rsp cmd
    MCPP_RPC_CMD_CAPTURE_OPEN,
    MCPP_RPC_CMD_CAPTURE_CLOSE,
    MCPP_RPC_CMD_CAPTURE_SET_CFG,
    MCPP_RPC_CMD_CAPTURE_CTRL,

    MCPP_RPC_CMD_PLAYBACK_OPEN,
    MCPP_RPC_CMD_PLAYBACK_CLOSE,
    MCPP_RPC_CMD_PLAYBACK_SET_CFG,
    MCPP_RPC_CMD_PLAYBACK_CTRL,
    //no rsp cmd
    MCPP_RPC_CMD_CAPTURE_PROCESS,
    MCPP_RPC_CMD_PLAYBACK_PROCESS,

    MCPP_RPC_CMD_RETURN_RESULT,
} MCPP_RPC_CMD_T;

typedef struct {
    MCPP_RPC_CMD_T   cmd;
    APP_MCPP_USER_T  user;
    APP_MCPP_CORE_T  core_client;
    APP_MCPP_CORE_T  core_server;
    void            *ptr;
    uint16_t          ptr_len;
} MCPP_RPC_T;

typedef int32_t(*mcpp_rpc_cmd_handler_t)(uint8_t *ptr, uint16_t len);
void mcpp_rpc_cmd_rsp_received_cb_register(mcpp_rpc_cmd_handler_t handler);
void mcpp_cmd_received_client_cb_register(mcpp_rpc_cmd_handler_t handler);
void mcpp_cmd_received_server_cb_register(mcpp_rpc_cmd_handler_t handler);

int32_t mcpp_cmd_init(void);
int32_t mcpp_cmd_deinit(void);
bool mcpp_check_rpc_cmd(uint16_t len);

void mcpp_send_rsp_cmd(uint8_t *param, uint16_t length, APP_MCPP_CORE_T target_core);
void mcpp_send_no_rsp_cmd(uint8_t *param, uint16_t length, APP_MCPP_CORE_T target_core);
void mcpp_cmd_send_empty_rsp(APP_MCPP_CORE_T core_client);

void mcpp_baremetal_send_data_done_handler(const void *data, uint32_t len);
uint32_t mcpp_baremetal_received_data_handler(const void *data, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif
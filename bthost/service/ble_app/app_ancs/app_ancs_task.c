/**
 * @file app_ancs_task.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-12-31
 *
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

/*****************************header include********************************/
#include "rwip_task.h"
#include "ke_task.h"
#include "ke_msg.h"
#include "gatt.h"
#include "prf_utils.h"
#include "anc_common.h"
#include "ancc_task.h"
#include "ancs_task.h"
#include "app_ancs_task.h"
#include "app.h"

/*********************external function declearation************************/

/************************private macro defination***************************/

/************************private type defination****************************/

/************************extern function declearation***********************/

/**********************private function declearation************************/
static int _read_evt_handler(ke_msg_id_t const msgid,
                             struct anc_srv_rd_evt *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id);


static int _write_evt_handler(ke_msg_id_t const msgid,
                              struct anc_srv_wr_evt *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id);

/************************private variable defination************************/
const struct ke_msg_handler app_ancs_msg_handler_list[] =
{
    {ANCS_READ_EVT,         (ke_msg_func_t)_read_evt_handler},
    {ANCS_WRITE_EVT,        (ke_msg_func_t)_write_evt_handler},
};

const struct app_subtask_handlers app_ancs_table_handler ={
    &app_ancs_msg_handler_list[0],
    (sizeof(app_ancs_msg_handler_list)/sizeof(struct ke_msg_handler)),
};

/****************************function defination****************************/
static int _read_evt_handler(ke_msg_id_t const msgid,
                             struct anc_srv_rd_evt *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id)
{
    /// ask client to send read command
    anc_cli_rd_cmd_t *cmd = KE_MSG_ALLOC(ANCC_READ_CMD,
                                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCC), param->conidx),
                                         KE_BUILD_ID(TASK_APP, param->conidx),
                                         anc_cli_rd_cmd);

    cmd->token = param->token;
    cmd->user_lid = param->user_lid;
    cmd->conidx = param->conidx;
    cmd->hdl = param->hdl;
    ke_msg_send(cmd);

    return KE_MSG_CONSUMED;
}

static int _write_evt_handler(ke_msg_id_t const msgid,
                              struct anc_srv_wr_evt *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    /// ask client to write peer's ATT
    gatt_cli_write_cmd_t *cmd =
        (gatt_cli_write_cmd_t *)KE_MSG_ALLOC_DYN(ANCC_WRITE_CMD,
                                                 KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCC), param->conidx),
                                                 KE_BUILD_ID(TASK_APP, param->conidx),
                                                 gatt_cli_write_cmd,
                                                 param->value_length);

    cmd->cmd_code = GATT_CLI_WRITE;
    cmd->dummy = param->token;
    cmd->user_lid = param->user_lid;
    cmd->conidx = param->conidx;
    cmd->write_type = GATT_WRITE;
    cmd->hdl = param->hdl;
    cmd->offset = param->offset;
    cmd->value_length = param->value_length;
    memcpy(cmd->value, param->value, cmd->value_length);
    ke_msg_send(cmd);

    return KE_MSG_CONSUMED;
}

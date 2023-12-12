/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#include "stdint.h"
#include "ke_msg.h"
#include "ke_task.h"
#include "app_gatt.h"
#include "gatt_msg.h"
#include "app.h"

#include "co_utils.h"

#define APP_BLE_GATT_USER_MAX   10
#define APP_BLE_GATT_MTU_DEFLT  23
#define APP_BLE_GATT_MTU_MAX    23

typedef struct app_ble_gatt_env
{
   uint8_t  ser_user_lid;
   uint8_t  cli_user_lid;
   uint8_t  prio_level;
   uint16_t pref_mtu[BLE_CONNECTION_MAX];
}app_ble_gatt_env_t;

static app_ble_gatt_env_t gatt_env = { 0 };

app_ble_gatt_event_cb user_event_cb[APP_BLE_GATT_USER_MAX] = {NULL};

static int app_ble_gatt_cmp_evt_handle(ke_msg_id_t const msgid,
                                                 gatt_cmp_evt_t *param,
                                                 ke_task_id_t const dest_id,
                                                 ke_task_id_t const src_id)
{
    gatt_proc_cmp_evt_t *proc_cmp_param = (gatt_proc_cmp_evt_t *)param;
    app_ble_gatt_event_param_u event_para = {0};
    switch(param->cmd_code){
    case GATT_USER_REGISTER:
    {
        if(param->dummy == APP_BLE_GATT_ROLE_CLIENT)
            gatt_env.cli_user_lid = param->user_lid;
        else if(param->dummy == APP_BLE_GATT_ROLE_SERVER)
            gatt_env.ser_user_lid = param->user_lid;
        else
            TRACE(0, "[%s][%d]ERROR: not find role, role = %d", __FUNCTION__, __LINE__, param->dummy);
    } return (KE_MSG_CONSUMED);
    case GATT_USER_UNREGISTER:
    {
        if(param->dummy == APP_BLE_GATT_ROLE_CLIENT)
            gatt_env.cli_user_lid = 0;
        else if(param->dummy == APP_BLE_GATT_ROLE_SERVER)
            gatt_env.ser_user_lid = 0;
        else
            TRACE(0, "[%s][%d]ERROR: not find role, role = %d", __FUNCTION__, __LINE__, param->dummy);
    } return (KE_MSG_CONSUMED);
    case GATT_DB_SVC_ADD:
    {
        gatt_db_svc_add_cmp_evt_t* add_param = (gatt_db_svc_add_cmp_evt_t*)param;
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_ADD;
        event_para.cmp.param.start_hdl = add_param->start_hdl;
    }break;
    case GATT_DB_SVC_REMOVE:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_REMOVE;
    }break;
    case GATT_DB_SVC_CTRL:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_CTRL;
    }break;
    case GATT_DB_HASH_GET:
    {
        gatt_db_hash_get_cmp_evt_t *ctrl = (gatt_db_hash_get_cmp_evt_t*)param;
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_GET_HASH;
        memcpy(event_para.cmp.param.hash, ctrl->hash, 16);
    }break;
    case GATT_SRV_EVENT_RELIABLE_SEND:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_SEND_RELIABLE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_SRV_EVENT_SEND:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_SEND;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_SRV_EVENT_MTP_SEND:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_SEND_MTP;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_SRV_EVENT_MTP_CANCEL:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_SVC_SEND_MTP_CANCLE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_DISCOVER_SVC:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_DISCOVER;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_DISCOVER_INC_SVC:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_DISCOVER_INC;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_DISCOVER_CHAR:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_DISCOVER_CHAR;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_DISCOVER_DESC:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_DISCOVER_DESC;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_DISCOVER_CANCEL:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_DISCOVER_CANCEL;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_READ:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_READ;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_READ_BY_UUID:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_TREA_BY_UUID;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_READ_MULTIPLE:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_TREA_MULTIPLE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_WRITE_RELIABLE:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_WRITE_RELIABLE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_WRITE:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_WEITE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_WRITE_EXE:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_WEITE_EXE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_EVENT_REGISTER:
    case GATT_CLI_EVENT_UNREGISTER:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_RECEIVE_CTRL;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    case GATT_CLI_MTU_UPDATE:
    {
        event_para.cmp.operation = APP_BLE_GATT_OP_CLI_MTU_UPDATE;
        event_para.cmp.param.conidx = proc_cmp_param->conidx;
    }break;
    default:
        break;
    }

    event_para.cmp.status = param->status;

    if(user_event_cb[param->dummy])
    {
        user_event_cb[param->dummy](APP_BLE_GATT_CMP_EVENT, &event_para);
    }

    return (KE_MSG_CONSUMED);
}
static int app_ble_gatt_ind_evt_handler(ke_msg_id_t const msgid,
                                                   gatt_unknown_msg_ind_t *param,
                                                   ke_task_id_t const dest_id,
                                                   ke_task_id_t const src_id)
{
    uint8_t gatt_user_id = APP_BLE_GATT_SUER_ID_INVALID;
    APP_BLE_GATT_EVENT_TYPE_E req_type = APP_BLE_GATT_EVENT_AMX;
    app_ble_gatt_event_param_u event_para = {0};
    switch(param->ind_code){
    case GATT_CLI_SVC:
    {
        gatt_cli_svc_ind_t* dis_svc = (gatt_cli_svc_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_DIS_SVC_EVENT;
        gatt_user_id = dis_svc->dummy;
        event_para.cli_dis_svc.conidx    = dis_svc->conidx;
        event_para.cli_dis_svc.hdl       = dis_svc->hdl;
        event_para.cli_dis_svc.disc_info = dis_svc->disc_info;
        event_para.cli_dis_svc.nb_att    = dis_svc->nb_att;
        event_para.cli_dis_svc.atts      = (app_ble_gatt_svc_att_t *)dis_svc->atts;
    }break;
    case GATT_CLI_SVC_INFO:
    {
        gatt_cli_svc_info_ind_t* dis_svc_info = (gatt_cli_svc_info_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_DIS_SVC_INFO_EVENT;
        gatt_user_id = dis_svc_info->dummy;
        event_para.cli_dis_svc_info.conidx    = dis_svc_info->conidx;
        event_para.cli_dis_svc_info.start_hdl = dis_svc_info->start_hdl;
        event_para.cli_dis_svc_info.end_hdl   = dis_svc_info->end_hdl;
        event_para.cli_dis_svc_info.uuid_type = dis_svc_info->uuid_type;
        memcpy(event_para.cli_dis_svc_info.uuid, dis_svc_info->uuid, 16);
    }break;
    case GATT_CLI_INC_SVC:
    {
        gatt_cli_inc_svc_ind_t* dis_svc_inc = (gatt_cli_inc_svc_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_DIS_INC_SVC_EVENT;
        gatt_user_id = dis_svc_inc->dummy;
        event_para.cli_dis_svc_inc.conidx    = dis_svc_inc->conidx;
        event_para.cli_dis_svc_inc.start_hdl = dis_svc_inc->start_hdl;
        event_para.cli_dis_svc_inc.end_hdl   = dis_svc_inc->end_hdl;
        event_para.cli_dis_svc_inc.uuid_type = dis_svc_inc->uuid_type;
        memcpy(event_para.cli_dis_svc_inc.uuid, dis_svc_inc->uuid, 16);
    }break;
    case GATT_CLI_CHAR:
    {
        gatt_cli_char_ind_t* dis_char = (gatt_cli_char_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_DIS_CHAR_EVENT;
        gatt_user_id = dis_char->dummy;
        event_para.cli_dis_char.conidx    = dis_char->conidx;
        event_para.cli_dis_char.char_hdl  = dis_char->char_hdl;
        event_para.cli_dis_char.val_hdl   = dis_char->val_hdl;
        event_para.cli_dis_char.prop      = dis_char->prop;
        event_para.cli_dis_char.uuid_type = dis_char->uuid_type;
        memcpy(event_para.cli_dis_char.uuid, dis_char->uuid, 16);
    }break;
    case GATT_CLI_DESC:
    {
        gatt_cli_desc_ind_t* disc_desc = (gatt_cli_desc_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_DIS_DESC_EVENT;
        gatt_user_id = disc_desc->dummy;
        event_para.cli_dis_desc.conidx    = disc_desc->conidx;
        event_para.cli_dis_desc.desc_hdl  = disc_desc->desc_hdl;
        event_para.cli_dis_desc.uuid_type = disc_desc->uuid_type;
        memcpy(event_para.cli_dis_desc.uuid, disc_desc->uuid, 16);
    }break;
    case GATT_CLI_ATT_VAL:
    {
        gatt_cli_att_val_ind_t* read_resp = (gatt_cli_att_val_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_READ_RESP_EVENT;
        gatt_user_id = read_resp->dummy;
        event_para.cli_read_resp.conidx       = read_resp->conidx;
        event_para.cli_read_resp.hdl          = read_resp->hdl;
        event_para.cli_read_resp.offset       = read_resp->offset;
        event_para.cli_read_resp.value_length = read_resp->value_length;
        event_para.cli_read_resp.value        = read_resp->value;
    }break;
    case GATT_CLI_SVC_CHANGED:
    {
        gatt_cli_svc_changed_ind_t* svc_change = (gatt_cli_svc_changed_ind_t*)param;
        req_type = APP_BLE_GATT_CLI_SVC_CHANGED_EVENT;
        gatt_user_id = svc_change->dummy;
        event_para.cli_svc_change.conidx       = svc_change->conidx;
        event_para.cli_svc_change.out_of_sync  = svc_change->out_of_sync;
        event_para.cli_svc_change.start_hdl    = svc_change->start_hdl;
        event_para.cli_svc_change.end_hdl      = svc_change->end_hdl;
    }break;
    default:
        break;
    }

    if(user_event_cb[param->dummy])
    {
        user_event_cb[gatt_user_id](req_type, &event_para);
    }

    return (KE_MSG_CONSUMED);
}
static int app_ble_gatt_req_ind_evt_handler(ke_msg_id_t const msgid,
                             gatt_srv_att_info_get_req_ind_t *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id)
{
    uint8_t gatt_user_id = APP_BLE_GATT_SUER_ID_INVALID;
    APP_BLE_GATT_EVENT_TYPE_E req_type = APP_BLE_GATT_EVENT_AMX;
    app_ble_gatt_event_param_u event_para = {0};
    TRACE(0, "[%s][%d]req_code=%x, token=%x", __FUNCTION__, __LINE__, param->req_ind_code, param->token);
    switch(param->req_ind_code) {
    case GATT_SRV_ATT_READ_GET:
    {
        gatt_srv_att_read_get_req_ind_t *read_req = (gatt_srv_att_read_get_req_ind_t *)param;
        req_type = APP_BLE_GATT_SRV_READ_REQ_EVENT;
        event_para.svc_read_req.token      = read_req->token;
        event_para.svc_read_req.conidx     = read_req->conidx;
        event_para.svc_read_req.hdl        = read_req->hdl;
        event_para.svc_read_req.offset     = read_req->offset;
        event_para.svc_read_req.max_length = read_req->max_length;
    }break;
    case GATT_SRV_ATT_EVENT_GET:
    {
        gatt_srv_att_event_get_req_ind_t *rel_get_req = (gatt_srv_att_event_get_req_ind_t *)param;
        req_type = APP_BLE_GATT_SRV_VAL_GET_REQ_EVENT;
        gatt_user_id = rel_get_req->dummy;
        event_para.svc_val_get_req.token      = rel_get_req->token;
        event_para.svc_val_get_req.conidx     = rel_get_req->conidx;
        event_para.svc_val_get_req.hdl        = rel_get_req->hdl;
        event_para.svc_val_get_req.max_length = rel_get_req->max_length;
    }break;
    case GATT_SRV_ATT_INFO_GET:
    {
        gatt_srv_att_info_get_req_ind_t *info_get_req = (gatt_srv_att_info_get_req_ind_t *)param;
        req_type = APP_BLE_GATT_SRV_GET_VAL_LEN_REQ_EVENT;
        event_para.svc_get_val_len_req.token      = info_get_req->token;
        event_para.svc_get_val_len_req.conidx     = info_get_req->conidx;
        event_para.svc_get_val_len_req.hdl        = info_get_req->hdl;
    }break;
    case GATT_SRV_ATT_VAL_SET:
    {
        gatt_srv_att_val_set_req_ind_t *write_req = (gatt_srv_att_val_set_req_ind_t *)param;
        req_type = APP_BLE_GATT_SRV_VAL_WRITE_REQ_EVENT;
        event_para.svc_write_req.token        = write_req->token;
        event_para.svc_write_req.conidx       = write_req->conidx;
        event_para.svc_write_req.hdl          = write_req->hdl;
        event_para.svc_write_req.offset       = write_req->offset;
        event_para.svc_write_req.value_length = write_req->value_length;
        event_para.svc_write_req.value        = write_req->value;
    }break;
    case GATT_CLI_ATT_VAL_GET:
    {
        gatt_cli_att_val_get_req_ind_t * val_get = (gatt_cli_att_val_get_req_ind_t *)param;
        req_type = APP_BLE_GATT_CLI_VAL_GET_REQ_EVENT;
        gatt_user_id = val_get->dummy;
        event_para.cli_get_val_req.token        = val_get->token;
        event_para.cli_get_val_req.conidx       = val_get->conidx;
        event_para.cli_get_val_req.hdl          = val_get->hdl;
        event_para.cli_get_val_req.offset       = val_get->offset;
        event_para.cli_get_val_req.max_length   = val_get->max_length;
    }break;
    case GATT_CLI_ATT_EVENT:
    {
        gatt_cli_att_event_req_ind_t * event_req = (gatt_cli_att_event_req_ind_t *)param;
        req_type = APP_BLE_GATT_CLI_EVENT_REQ_EVENT;
        event_para.cli_event_req.token        = event_req->token;
        event_para.cli_event_req.conidx       = event_req->conidx;
        event_para.cli_event_req.evt_type     = event_req->evt_type;
        event_para.cli_event_req.complete     = event_req->complete;
        event_para.cli_event_req.hdl          = event_req->hdl;
        event_para.cli_event_req.value_length = event_req->value_length;
        event_para.cli_event_req.value        = event_req->value;
    }break;
    default:
        break;
    }

    if((gatt_user_id != APP_BLE_GATT_SUER_ID_INVALID) && (user_event_cb[gatt_user_id]))
    {
        user_event_cb[gatt_user_id](req_type, &event_para);
    }
    else
    {
        for(int i = 0; i < APP_BLE_GATT_USER_MAX; i++){
            if(user_event_cb[i]){
                user_event_cb[i](req_type, &event_para);
            }
        }
    }

    return (KE_MSG_CONSUMED);
}
static int app_ble_gatt_dflt_handler(ke_msg_id_t const msgid,
                                              void *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    TRACE(0, "[%s][%d]: %x", __FUNCTION__, __LINE__, msgid);
    return (KE_MSG_CONSUMED);
}

/// Default State handlers definition
const struct ke_msg_handler app_gatt_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER, (ke_msg_func_t)app_ble_gatt_dflt_handler},
    {GATT_CMP_EVT,           (ke_msg_func_t)app_ble_gatt_cmp_evt_handle},
    {GATT_IND,               (ke_msg_func_t)app_ble_gatt_ind_evt_handler},
    {GATT_REQ_IND,           (ke_msg_func_t)app_ble_gatt_req_ind_evt_handler},
};
const struct app_subtask_handlers app_gatt_handler = {app_gatt_msg_handler_list, ARRAY_LEN(app_gatt_msg_handler_list)};

void app_ble_gatt_reg(APP_BLE_GATT_USER_ROLE_E role)
{
    gatt_user_register_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD, TASK_GATT, TASK_APP,
                                                 gatt_user_register_cmd);
    cmd->cmd_code   = GATT_USER_REGISTER;
    cmd->pref_mtu   = APP_BLE_GATT_MTU_MAX;
    cmd->prio_level = gatt_env.prio_level;
    cmd->dummy      = role;
    cmd->role       = role;
    ke_msg_send(cmd);

    for(int i=0; i<BLE_CONNECTION_MAX; i++)
    {
        gatt_env.pref_mtu[i] = APP_BLE_GATT_MTU_DEFLT;
    }
}
void app_ble_gatt_unreg(APP_BLE_GATT_USER_ROLE_E role)
{
     /// role unreg
     gatt_user_unregister_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                  TASK_GATT,
                                                  TASK_APP,
                                                  gatt_user_unregister_cmd);
     cmd->cmd_code = GATT_USER_UNREGISTER;
     if(role == APP_BLE_GATT_ROLE_CLIENT)
     {
         cmd->dummy    = gatt_env.cli_user_lid;
         cmd->user_lid = gatt_env.cli_user_lid;
     }
     else if(APP_BLE_GATT_ROLE_SERVER)
     {
         cmd->dummy    = gatt_env.ser_user_lid;
         cmd->user_lid = gatt_env.ser_user_lid;
     }
     else
     {
        TRACE(0, "[%s][%d]ERROR: not find role, role = %d", __FUNCTION__, __LINE__, role);
        return;
     }
     ke_msg_send(cmd);
}
uint8_t app_ble_gatt_user_reg(app_ble_gatt_event_cb cb)
{
    for(int i = 0; i < APP_BLE_GATT_USER_MAX; i++)
    {
        if(user_event_cb[i] == NULL)
        {
            user_event_cb[i] = cb;
            return i;
        }
    }
    return APP_BLE_GATT_SUER_ID_INVALID;
}
void app_ble_gatt_user_unreg(uint8_t gatt_user_id)
{
    if(gatt_user_id < APP_BLE_GATT_USER_MAX)
    {
        user_event_cb[gatt_user_id] = NULL;
    }
}
/*************************************GATT SERVICE FUNC****************************************/
void app_ble_gatt_svc_add(uint8_t gatt_user_id, app_ble_gatt_add_server_t *svc_info)
{
    /// server att add
    gatt_db_svc_add_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_db_svc_add_cmd,
                                                 sizeof(gatt_att_desc_t)*svc_info->nb_att);
    cmd->cmd_code    = GATT_DB_SVC_ADD;
    cmd->dummy       = gatt_user_id;
    cmd->user_lid    = gatt_env.ser_user_lid;
    cmd->info        = svc_info->info;
    cmd->start_hdl   = 0;
    cmd->nb_att_rsvd = svc_info->nb_att;
    cmd->nb_att      = svc_info->nb_att;
    switch(GETF(cmd->info, GATT_SVC_UUID_TYPE))
    {
        case GATT_UUID_16:   {memcpy(cmd->uuid, svc_info->p_uuid, GATT_UUID_16_LEN);}   break;
        case GATT_UUID_32:   {memcpy(cmd->uuid, svc_info->p_uuid, GATT_UUID_32_LEN);}   break;
        case GATT_UUID_128:  {memcpy(cmd->uuid, svc_info->p_uuid, GATT_UUID_128_LEN);}   break;
    }
    memcpy(cmd->atts, svc_info->p_atts, sizeof(gatt_att_desc_t) * svc_info->nb_att);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_remove(uint8_t gatt_user_id, uint16_t          start_hdl)
{
    gatt_db_svc_remove_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_db_svc_remove_cmd);
    cmd->cmd_code = GATT_DB_SVC_REMOVE;
    cmd->dummy = gatt_user_id;
    cmd->user_lid = gatt_env.ser_user_lid;
    cmd->start_hdl = start_hdl;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_ctrl(uint8_t gatt_user_id, uint8_t enable, uint8_t visible, uint16_t start_hdl)
{
    gatt_db_svc_ctrl_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_db_svc_ctrl_cmd);
    cmd->cmd_code = GATT_DB_SVC_CTRL;
    cmd->dummy = gatt_user_id;
    cmd->user_lid = gatt_env.ser_user_lid;
    cmd->start_hdl = start_hdl;
    cmd->enable    = enable;
    cmd->visible   = visible;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_get_hash(uint8_t gatt_user_id)
{
    gatt_db_hash_get_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_db_hash_get_cmd);
    cmd->cmd_code  = GATT_DB_HASH_GET;
    cmd->dummy     = gatt_user_id;
    cmd->user_lid  = gatt_env.ser_user_lid;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_send_reliable(uint8_t gatt_user_id, app_ble_gatt_srv_send_reliable_t *param)
{
    gatt_srv_event_reliable_send_cmd_t *cmd =    KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_srv_event_reliable_send_cmd,
                                                 sizeof(app_ble_gatt_att_t) * param->nb_att);
    cmd->cmd_code  = GATT_SRV_EVENT_RELIABLE_SEND;
    cmd->dummy     = gatt_user_id;
    cmd->user_lid  = gatt_env.ser_user_lid;
    cmd->conidx    = param->conidx;
    cmd->evt_type  = param->evt_type;
    cmd->nb_att    = param->nb_att;
    memcpy(cmd->atts, param->atts, sizeof(gatt_att_t) * param->nb_att);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_send(uint8_t gatt_user_id, app_ble_gatt_srv_send_t *param)
{
    gatt_srv_event_send_cmd_t *cmd =    KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_srv_event_send_cmd,
                                                 param->value_length);
    cmd->cmd_code     = GATT_SRV_EVENT_SEND;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.ser_user_lid;
    cmd->conidx       = param->conidx;
    cmd->evt_type     = param->evt_type;
    cmd->hdl          = param->hdl;
    cmd->value_length = param->value_length;
    memcpy(cmd->value, param->value, cmd->value_length);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_send_mtp(uint8_t gatt_user_id, app_ble_gatt_srv_send_mtp_t *param)
{
    gatt_srv_event_mtp_send_cmd_t *cmd =    KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_srv_event_mtp_send_cmd,
                                                 param->value_length);
    cmd->cmd_code     = GATT_SRV_EVENT_MTP_SEND;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.ser_user_lid;
    cmd->conidx_bf    = param->conidx_bf;
    cmd->evt_type     = param->evt_type;
    cmd->hdl          = param->hdl;
    cmd->value_length = param->value_length;
    memcpy(cmd->value, param->value, cmd->value_length);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_send_mtp_cancel(uint8_t gatt_user_id)
{
    gatt_srv_event_mtp_cancel_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_srv_event_mtp_cancel_cmd);
    cmd->cmd_code     = GATT_SRV_EVENT_MTP_CANCEL;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.ser_user_lid;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_read_resp(app_ble_gatt_svc_val_basic_resp_t *param)
{
    gatt_srv_att_read_get_cfm_t *cmd = KE_MSG_ALLOC_DYN(GATT_CFM,
                                                        TASK_GATT,
                                                        TASK_APP,
                                                        gatt_srv_att_read_get_cfm,
                                                        param->value_length);
    cmd->req_ind_code     = GATT_SRV_ATT_READ_GET;
    cmd->token            = param->token;
    cmd->user_lid         = gatt_env.ser_user_lid;
    cmd->conidx           = param->conidx;
    cmd->status           = param->status;
    cmd->att_length       = param->att_length;
    cmd->value_length     = param->value_length;
    if(param->value_length){
        memcpy(cmd->value, param->value, param->value_length);
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_val_get_resp(app_ble_gatt_svc_val_basic_resp_t *param)
{
    gatt_srv_att_event_get_cfm_t *cmd = KE_MSG_ALLOC_DYN(GATT_CFM,
                                                        TASK_GATT,
                                                        TASK_APP,
                                                        gatt_srv_att_event_get_cfm,
                                                        param->value_length);
    cmd->req_ind_code     = GATT_SRV_ATT_EVENT_GET;
    cmd->token            = param->token;
    cmd->user_lid         = gatt_env.ser_user_lid;
    cmd->conidx           = param->conidx;
    cmd->status           = param->status;
    cmd->att_length       = param->att_length;
    cmd->value_length     = param->value_length;
    if(param->value_length){
        memcpy(cmd->value, param->value, param->value_length);
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_get_val_len_resp(app_ble_gatt_srv_get_val_len_resp_t *param)
{
    gatt_srv_att_info_get_cfm_t *cmd = KE_MSG_ALLOC(GATT_CFM,
                                                    TASK_GATT,
                                                    TASK_APP,
                                                    gatt_srv_att_info_get_cfm);
    cmd->req_ind_code     = GATT_SRV_ATT_INFO_GET;
    cmd->token            = param->token;
    cmd->user_lid         = gatt_env.ser_user_lid;
    cmd->conidx           = param->conidx;
    cmd->status           = param->status;
    cmd->att_length       = param->att_length;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_svc_write_resp(app_ble_gatt_srv_write_resp_t *param)
{
    gatt_srv_att_val_set_cfm_t *cmd = KE_MSG_ALLOC(GATT_CFM,
                                                   TASK_GATT,
                                                   TASK_APP,
                                                   gatt_srv_att_val_set_cfm);
    cmd->req_ind_code     = GATT_SRV_ATT_VAL_SET;
    cmd->token            = param->token;
    cmd->user_lid         = gatt_env.ser_user_lid;
    cmd->conidx           = param->conidx;
    cmd->status           = param->status;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
/*************************************GATT CLIENT FUNC****************************************/
void app_ble_gatt_cli_discover(uint8_t gatt_user_id, app_ble_gatt_cli_discover_t *param)
{
    gatt_cli_discover_svc_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_discover_svc_cmd);
    cmd->cmd_code     = GATT_CLI_DISCOVER_SVC;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->disc_type    = param->disc_type;
    cmd->full         = param->full;
    cmd->start_hdl    = param->start_hdl;
    cmd->end_hdl      = param->end_hdl;
    if((param->disc_type == APP_BLE_GATT_DISCOVER_SVC_PRIMARY_BY_UUID) ||
        (param->disc_type == APP_BLE_GATT_DISCOVER_SVC_SECONDARY_BY_UUID))
    {
        cmd->uuid_type    = param->uuid_type;
        memcpy(cmd->uuid, param->uuid, GATT_UUID_128_LEN);
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_discover_included(uint8_t gatt_user_id, app_ble_gatt_cli_discover_general_t *param)
{
    gatt_cli_discover_inc_svc_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_discover_inc_svc_cmd);
    cmd->cmd_code     = GATT_CLI_DISCOVER_INC_SVC;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->start_hdl    = param->start_hdl;
    cmd->end_hdl      = param->end_hdl;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_discover_char(uint8_t gatt_user_id, app_ble_gatt_cli_discover_char_t *param)
{
    gatt_cli_discover_char_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_discover_char_cmd);
    cmd->cmd_code     = GATT_CLI_DISCOVER_CHAR;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->disc_type    = param->disc_type;
    cmd->start_hdl    = param->start_hdl;
    cmd->end_hdl      = param->end_hdl;
    if(param->disc_type == APP_BLE_GATT_DISCOVER_CHAR_BY_UUID)
    {
        cmd->uuid_type    = param->uuid_type;
        memcpy(cmd->uuid, param->uuid, GATT_UUID_128_LEN);
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_discover_desc(uint8_t gatt_user_id, app_ble_gatt_cli_discover_general_t *param)
{
    gatt_cli_discover_desc_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_discover_desc_cmd);
    cmd->cmd_code     = GATT_CLI_DISCOVER_DESC;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->start_hdl    = param->start_hdl;
    cmd->end_hdl      = param->end_hdl;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_discover_cancel(uint8_t gatt_user_id, uint8_t conidx)
{
    gatt_cli_discover_cancel_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_discover_cancel_cmd);
    cmd->cmd_code     = GATT_CLI_DISCOVER_CANCEL;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = conidx;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_read(uint8_t gatt_user_id, app_ble_gatt_cli_read_t *param)
{
    gatt_cli_read_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_cli_read_cmd);
    cmd->cmd_code     = GATT_CLI_READ;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->hdl          = param->hdl;
    cmd->offset       = param->offset;
    cmd->length       = param->length;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_read_by_uuid(uint8_t gatt_user_id, app_ble_gatt_cli_read_by_uuid_t *param)
{
    gatt_cli_read_by_uuid_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_read_by_uuid_cmd);
    cmd->cmd_code     = GATT_CLI_READ_BY_UUID;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->start_hdl    = param->start_hdl;
    cmd->end_hdl      = param->end_hdl;
    cmd->uuid_type    = param->uuid_type;
    memcpy(cmd->uuid, param->uuid, GATT_UUID_128_LEN);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_read_multiple(uint8_t gatt_user_id, app_ble_gatt_cli_read_multiple_t *param)
{
    gatt_cli_read_multiple_cmd_t *cmd =    KE_MSG_ALLOC_DYN(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_read_multiple_cmd,
                                                             sizeof(app_ble_gatt_att_t) * param->nb_att);
    cmd->cmd_code     = GATT_CLI_READ_MULTIPLE;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->nb_att       = param->nb_att;
    memcpy(cmd->atts, param->atts, sizeof(app_ble_gatt_att_t) * param->nb_att);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_write_reliable(uint8_t gatt_user_id, app_ble_gatt_cli_write_reliable_t *param)
{
    gatt_cli_write_reliable_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_write_reliable_cmd);
    cmd->cmd_code     = GATT_CLI_WRITE_RELIABLE;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->write_type   = param->write_type;
    cmd->write_mode   = param->write_mode;
    cmd->hdl          = param->hdl;
    cmd->offset       = param->offset;
    cmd->length       = param->length;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_write(uint8_t gatt_user_id, app_ble_gatt_cli_write_t *param)
{
    gatt_cli_write_cmd_t *cmd =    KE_MSG_ALLOC_DYN(GATT_CMD,
                                                    TASK_GATT,
                                                    TASK_APP,
                                                    gatt_cli_write_cmd,
                                                    param->value_length);
    cmd->cmd_code     = GATT_CLI_WRITE;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->write_type   = param->write_type;
    cmd->hdl          = param->hdl;
    cmd->offset       = param->offset;
    cmd->value_length = param->value_length;
    if(param->value_length){
        memcpy(cmd->value, param->value, param->value_length);
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_write_exe(uint8_t gatt_user_id, uint8_t conidx, uint8_t execute)
{
    gatt_cli_write_exe_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                             TASK_GATT,
                                                             TASK_APP,
                                                             gatt_cli_write_exe_cmd);
    cmd->cmd_code     = GATT_CLI_WRITE_EXE;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = conidx;
    cmd->execute      = execute;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_receive_ctrl(uint8_t gatt_user_id, app_ble_gatt_cli_rec_ctrl_t *param)
{
    gatt_cli_event_register_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                      TASK_GATT,
                                                      TASK_APP,
                                                      gatt_cli_event_register_cmd);
    if(param->en){
        cmd->cmd_code     = GATT_CLI_EVENT_REGISTER;
    }
    else {
        cmd->cmd_code     = GATT_CLI_EVENT_UNREGISTER;
    }
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = param->conidx;
    cmd->start_hdl    = param->start_hdl;
    cmd->end_hdl      = param->end_hdl;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_mtu_update(uint8_t gatt_user_id, uint8_t conidx)
{
    gatt_cli_mtu_update_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                     TASK_GATT,
                                                     TASK_APP,
                                                     gatt_cli_mtu_update_cmd);
    cmd->cmd_code     = GATT_CLI_MTU_UPDATE;
    cmd->dummy        = gatt_user_id;
    cmd->user_lid     = gatt_env.cli_user_lid;
    cmd->conidx       = conidx;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_get_val_resp(app_ble_gatt_cli_get_val_resp_t *param)
{
    gatt_cli_att_val_get_cfm_t *cmd = KE_MSG_ALLOC_DYN(GATT_CFM,
                                                   TASK_GATT,
                                                   TASK_APP,
                                                   gatt_cli_att_val_get_cfm,
                                                   param->value_length);
    cmd->req_ind_code     = GATT_CLI_ATT_VAL_GET;
    cmd->token            = param->token;
    cmd->user_lid         = gatt_env.cli_user_lid;
    cmd->conidx           = param->conidx;
    cmd->status           = param->status;
    cmd->value_length     = param->value_length;
    if(param->value_length){
        memcpy(cmd->value, param->value, param->value_length);
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}
void app_ble_gatt_cli_event_resp(app_ble_gatt_cli_event_resp_t *param)
{
    gatt_cli_att_event_cfm_t *cmd = KE_MSG_ALLOC(GATT_CFM,
                                                 TASK_GATT,
                                                 TASK_APP,
                                                 gatt_cli_att_event_cfm);
    cmd->req_ind_code     = GATT_CLI_ATT_EVENT;
    cmd->token            = param->token;
    cmd->user_lid         = gatt_env.cli_user_lid;
    cmd->conidx           = param->conidx;
    cmd->status           = param->status;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void app_ble_gatt_set_mtu(uint8_t conidx, uint16_t mtu)
{
    if(conidx < BLE_CONNECTION_MAX)
    {
        gatt_env.pref_mtu[conidx] = mtu;
    }
}

void app_ble_gatt_get_mtu(uint8_t conidx, uint16_t *mtu)
{
    if(conidx < BLE_CONNECTION_MAX)
    {
        *mtu = gatt_env.pref_mtu[conidx];
    }
    else
    {
        TRACE(0, "[%s][%d][ERROR]: conidx=%d, max=%d", __FUNCTION__, __LINE__, conidx, BLE_CONNECTION_MAX);
        *mtu = 0xFFFF;
    }
}

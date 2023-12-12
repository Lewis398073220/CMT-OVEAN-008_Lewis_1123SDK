/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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


/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
*/
#include "rwip_config.h"

#if (BLE_TILE)
#include "ke_msg.h"
#include "app.h"
#include "app_tile.h"
#include "tile_gatt_server.h"

#ifndef BLE_CONNECTION_MAX
#define BLE_CONNECTION_MAX (1)
#endif

static app_ble_tile_event_cb user_callback = NULL;

static int app_tile_channel_connected_handler(ke_msg_id_t const msgid,
                                                            struct tile_gatt_connection_event_t *param,
                                                            ke_task_id_t const dest_id,
                                                            ke_task_id_t const src_id)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type = APP_BLE_TILE_CH_CONN_EVENT;
        param_info.conidx     = param->conidx;
        param_info.data.channel_conn.hdl = param->handle;
        param_info.data.channel_conn.status = param->status;
        user_callback(&param_info);
    }
    return KE_MSG_CONSUMED;
}

static int app_tile_channel_disconnected_handler(ke_msg_id_t const msgid,
                                                 struct tile_gatt_connection_event_t *param,
                                                 ke_task_id_t const dest_id,
                                                 ke_task_id_t const src_id)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type = APP_BLE_TILE_CH_DISCONN_EVENT;
        param_info.conidx     = param->conidx;
        param_info.data.channel_conn.hdl = param->handle;
        param_info.data.channel_conn.status = param->status;
        user_callback(&param_info);
    }
    return KE_MSG_CONSUMED;
}

static int app_tile_tx_data_sent_done_handler(ke_msg_id_t const msgid,
                                         struct tile_gatt_tx_complete_ind_t *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type  = APP_BLE_TILE_CH_DISCONN_EVENT;
        param_info.conidx      = param->conidx;
        param_info.data.result = param->success;
        user_callback(&param_info);
    }
    return KE_MSG_CONSUMED;
}

static int app_tile_rx_received_handler(ke_msg_id_t const msgid,
                                        struct tile_gatt_rx_data_ind_t *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type  = APP_BLE_TILE_CH_DISCONN_EVENT;
        param_info.conidx      = param->conidx;
        param_info.data.receive.data_len = param->length;
        param_info.data.receive.data     = param->data;
        user_callback(&param_info);
    }
    return KE_MSG_CONSUMED;
}

static int app_tile_msg_handler(ke_msg_id_t const msgid, void const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id) {
    TRACE(1,"App Default Message Handler: id=%d", msgid);
    return KE_MSG_CONSUMED;
}

const struct ke_msg_handler app_tile_msg_handler_list[] = {
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_tile_msg_handler},

    {TILE_CHANNEL_CONNECTED_IND,      (ke_msg_func_t)app_tile_channel_connected_handler},
    {TILE_CHANNEL_DISCONNECTED_IND,   (ke_msg_func_t)app_tile_channel_disconnected_handler},
    {TILE_TOA_TX_DATA_SENT_DONE_IND,  (ke_msg_func_t)app_tile_tx_data_sent_done_handler},
    {TILE_TOA_RECEIVED_IND,           (ke_msg_func_t)app_tile_rx_received_handler},
};

const struct app_subtask_handlers app_tile_table_handler = {
    &app_tile_msg_handler_list[0], (sizeof(app_tile_msg_handler_list)/sizeof(struct ke_msg_handler)),
};

void app_tile_connected_evt_handler(uint8_t conidx, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type = APP_BLE_TILE_CONN_EVENT;
        param_info.conidx     = conidx;
        param_info.data.ble_conn_param.interval = interval;
        param_info.data.ble_conn_param.latency  = latency;
        param_info.data.ble_conn_param.conn_sup_timeout  = timeout;
        user_callback(&param_info);
    }
}

void app_tile_disconnected_evt_handler(uint8_t conidx)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type = APP_BLE_TILE_DISCONN_EVENT;
        param_info.conidx     = conidx;
        user_callback(&param_info);
    }
}


void app_tile_ble_conn_parameter_updated(uint8_t conidx, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    app_ble_tile_event_param_t param_info = {0};

    if (user_callback)
    {
        param_info.event_type = APP_BLE_TILE_CONN_UPDATE_EVENT;
        param_info.conidx     = conidx;
        param_info.data.ble_conn_param.interval = interval;
        param_info.data.ble_conn_param.latency  = latency;
        param_info.data.ble_conn_param.conn_sup_timeout  = timeout;
        user_callback(&param_info);
    }
}

void app_ble_tile_add_svc(void)
{
    TRACE(0,"Registering GATT Service");
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                TASK_GAPM, 
                                                TASK_APP,
                                                gapm_profile_task_add_cmd,
                                                0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX>1
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
#else
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
#endif
    req->prf_api_id = TASK_ID_TILE;
    req->user_prio = 0;
    req->start_hdl = 0;

    ke_msg_send(req);
}

void  app_tile_send_ntf(uint8_t conidx, uint8_t* ptrData, uint32_t length)
{
    struct ble_tile_sent_ntf_t *cmd = KE_MSG_ALLOC_DYN(TILE_TOA_TX_DATA_SENT_CMD,
                                                      prf_get_task_from_id(TASK_ID_CUSTOMIZE),
                                                      TASK_APP,
                                                      ble_tile_sent_ntf_t,
                                                      length);

    cmd->conidx   = conidx;
    cmd->data_len = length;
    if(length){
        memcpy(cmd->data, ptrData, length);
    }

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void app_tile_event_cb_reg(app_ble_tile_event_cb cb)
{
    if(!user_callback)
    {
        user_callback = cb;
    }
}


#endif

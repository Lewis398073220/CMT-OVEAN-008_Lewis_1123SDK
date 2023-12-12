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

/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"              // SW configuration

#ifdef CFG_AI_VOICE
#include "ke_msg.h"
#include "gatt.h"
#include "app.h"
#include "app_ai.h"
#include "ai.h"
#include "gapm_msg.h"

app_ble_ai_event_cb event_callback = NULL;

#if (BLE_CUSTOMIZE_VOICE | BLE_DUAL_MIC_REC_VOICE)
void app_ai_ble_add_ai(void)
{
    TRACE(0, "%s %d", __func__, TASK_ID_AI);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);; //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_AI;

    // Send the message
    ke_msg_send(req);
}
#endif

#if (BLE_APP_AMA_VOICE)
void app_ai_ble_add_ama(void)
{
    TRACE(0, "%s %d", __func__, TASK_ID_AMA);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_AMA;
    // Send the message
    ke_msg_send(req);
}
#endif

#if (BLE_APP_SMART_VOICE)
void app_ai_ble_add_smartvoice(void)
{
    TRACE(0, "%s %d", __func__, TASK_ID_SMART);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_SMART;
    // Send the message
    ke_msg_send(req);
}
#endif

#if (BLE_APP_DMA_VOICE)
void app_ai_ble_add_dma(void)
{
    TRACE(0, "%s %d", __func__, TASK_ID_DMA);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_DMA;
    // Send the message
    ke_msg_send(req);
}
#endif

#if (BLE_APP_GMA_VOICE)
void app_ai_ble_add_gma(void)
{
    TRACE(0, "%s %d", __func__, TASK_ID_GMA);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_GMA;
    // Send the message
    ke_msg_send(req);

}
#endif

#if (BLE_APP_TENCENT_VOICE)
void app_ai_ble_add_tencent(void)
{
    TRACE(0, "%s %d", __func__, TASK_ID_TENCENT);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_TENCENT;
    // Send the message
    ke_msg_send(req);
}
#endif

void app_ai_data_send(app_ble_ai_data_send_param_t *param)
{
    ai_data_send_cfm_t *cmd = NULL;
    ke_msg_id_t  msg_id = 0;
    ke_task_id_t dest_id = 0;

    switch(param->ai_type){
    case APP_BLE_AI_SPEC_AMA:
        msg_id = AMA_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_AMA);
        break;
    case APP_BLE_AI_SPEC_DMA:
        msg_id = DMA_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_DMA);
        break;
    case APP_BLE_AI_SPEC_GMA:
        msg_id = GMA_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_GMA);
        break;
    case APP_BLE_AI_SPEC_SMART:
        msg_id = SMART_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_SMART);
        break;
    case APP_BLE_AI_SPEC_TENCENT:
        msg_id = TENCENT_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_TENCENT);
        break;
    case APP_BLE_AI_SPEC_RECORDING:
        msg_id = RECORDING_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_AI);
        break;
    case APP_BLE_AI_SPEC_COMMON:
        msg_id = CUSTOMIZE_DATA_DEND_CFM;
        dest_id = prf_get_task_from_id(TASK_ID_CUSTOMIZE);
        break;
    default:
        TRACE(0, "%s[ERROR]: not find ai type=%d", __func__, param->ai_type);
        return;
    }

    cmd = KE_MSG_ALLOC_DYN(msg_id, dest_id, TASK_APP, ai_data_send_cfm, param->data_len);

    cmd->conidx = param->conidx;
    cmd->gatt_event_type = param->gatt_event_type;
    cmd->data_type       = param->data_type;
    cmd->data_len        = param->data_len;
    if(param->data_len){
        memcpy(cmd->data, param->data, param->data_len);
    }

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void app_ai_event_reg(app_ble_ai_event_cb cb)
{
    event_callback = cb;
}

void app_ai_ble_mtu_exchanged_handler(uint8_t conidx ,uint16_t mtu)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t param;

        param.ai_type      = APP_BLE_AI_SPEC_MAX;
        param.event_type   = APP_BLE_AI_MTU_CHANGE;
        param.conidx       = conidx;
        param.data.mtu     = mtu;
        event_callback(&param);
    }
}

void app_ai_ble_disconnected_evt_handler(uint8_t conidx)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t param;

        param.ai_type    = APP_BLE_AI_SPEC_MAX;
        param.event_type = APP_BLE_AI_DISCONN;
        param.conidx     = conidx;
        event_callback(&param);
    }
}

static int app_ai_add_svc_done_handler(ke_msg_id_t const msgid,
                                                  struct ai_add_svc_ind *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type                    = param->ai_type;
        event_param.event_type                 = APP_BLE_AI_SVC_ADD_DONE;
        event_param.data.svc_add_done.star_hdl = param->start_hdl;
        event_param.data.svc_add_done.att_num  = param->att_num;
        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}


static int app_ai_connected_evt_handler(ke_msg_id_t const msgid,
                                                   struct ai_event_ind *param,
                                                   ke_task_id_t const dest_id,
                                                   ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type    = param->ai_type;
        event_param.conidx     = param->conidx;
        event_param.event_type = APP_BLE_AI_CONN;
        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ai_disconnected_evt_handler(ke_msg_id_t const msgid,
                                                        struct ai_event_ind *param,
                                                        ke_task_id_t const dest_id,
                                                        ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type    = param->ai_type;
        event_param.conidx     = param->conidx;
        event_param.event_type = APP_BLE_AI_DISCONN;
        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);

}

static int app_ai_tx_done_ind_handler(ke_msg_id_t const msgid,
                                                struct ai_event_ind *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type    = param->ai_type;
        event_param.conidx     = param->conidx;
        event_param.event_type = APP_BLE_AI_TX_DONE_EVENT;
        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ai_cmd_receive_ind_handler(ke_msg_id_t const msgid,
                                                      struct ai_event_ind *param,
                                                      ke_task_id_t const dest_id,
                                                      ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type    = param->ai_type;
        event_param.conidx     = param->conidx;
        event_param.event_type = APP_BLE_AI_RECEIVED_EVENT;
        event_param.data.received.data_type = APP_BLE_AI_CMD;
        event_param.data.received.data_len  = param->data_len;
        event_param.data.received.data      = param->data;
        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ai_data_receive_ind_handler(ke_msg_id_t const msgid,
                                                       struct ai_event_ind *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type    = param->ai_type;
        event_param.conidx     = param->conidx;
        event_param.event_type = APP_BLE_AI_RECEIVED_EVENT;
        event_param.data.received.data_type = APP_BLE_AI_DATA;
        event_param.data.received.data_len  = param->data_len;
        event_param.data.received.data      = param->data;
        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ai_cmd_change_ccc_ind_handler(ke_msg_id_t const msgid,
                                                          struct ai_change_ccc_ind *param,
                                                          ke_task_id_t const dest_id,
                                                          ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type    = param->ai_type;
        event_param.conidx     = param->conidx;
        event_param.event_type = APP_BLE_AI_CHANGE_CCC_EVENT;
        event_param.data.change_ccc.data_type    = APP_BLE_AI_CMD;
        event_param.data.change_ccc.ntf_ind_flag = param->ntf_ind_flag;

        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ai_data_change_ccc_ind_handler(ke_msg_id_t const msgid,
                                                            struct ai_change_ccc_ind  *param,
                                                            ke_task_id_t const dest_id,
                                                            ke_task_id_t const src_id)
{
    if(event_callback)
    {
        app_ble_ai_event_param_t event_param = {0};

        event_param.ai_type = param->ai_type;
        event_param.conidx  = param->conidx;
        event_param.event_type = APP_BLE_AI_CHANGE_CCC_EVENT;
        event_param.data.change_ccc.data_type    = APP_BLE_AI_DATA;
        event_param.data.change_ccc.ntf_ind_flag = param->ntf_ind_flag;

        event_callback(&event_param);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ai_dft_msg_handler(ke_msg_id_t const msgid,
                                    struct ai_event_ind *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}

const struct ke_msg_handler app_ai_msg_handler_list[] = {
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,      (ke_msg_func_t)app_ai_dft_msg_handler},
    {PRF_AI_SVC_ADD_DONE_IND,     (ke_msg_func_t)app_ai_add_svc_done_handler},
    {PRF_AI_CONNECT_IND,          (ke_msg_func_t)app_ai_connected_evt_handler},
    {PRF_AI_DISCONNECT_IND,       (ke_msg_func_t)app_ai_disconnected_evt_handler},
    {PRF_AI_TX_DONE_IND,          (ke_msg_func_t)app_ai_tx_done_ind_handler},
    {PRF_AI_CMD_RECEIVED_IND,     (ke_msg_func_t)app_ai_cmd_receive_ind_handler},
    {PRF_AI_DATA_RECEIVED_IND,    (ke_msg_func_t)app_ai_data_receive_ind_handler},
    {PRF_AI_CMD_CHANGGE_CCC_IND,  (ke_msg_func_t)app_ai_cmd_change_ccc_ind_handler},
    {PRF_AI_DATA_CHANGGE_CCC_IND, (ke_msg_func_t)app_ai_data_change_ccc_ind_handler},
};

const struct app_subtask_handlers app_ai_table_handler = {
    &app_ai_msg_handler_list[0],
    (sizeof(app_ai_msg_handler_list)/sizeof(struct ke_msg_handler)),
};

#endif

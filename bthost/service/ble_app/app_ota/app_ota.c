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
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_OTA)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gapm_msg.h"
#include "app_ota.h"                  // OTA Application Definitions
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"
#include "ota.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
static app_ota_event_callback ota_event_cb = NULL;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ota_event_reg(app_ota_event_callback cb)
{
    if(!ota_event_cb)
    {
        ota_event_cb = cb;
    }
}

void app_ota_event_unreg()
{
    ota_event_cb = NULL;
}

void app_ota_add_ota(void)
{
    BLE_APP_DBG("app_ota_add_ota");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM, TASK_APP,
                         gapm_profile_task_add_cmd, 0);

    /// Fill message
    req->operation  = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl    = SVC_SEC_LVL(NOT_ENC) | SVC_UUID(128);
    req->user_prio  = 0;
    req->prf_api_id = TASK_ID_OTA;
    req->app_task   = TASK_APP;
    req->start_hdl  = 0;

    /// Send the message
    ke_msg_send(req);
}

void app_ota_send_rx_cfm(uint8_t conidx)
{
    ota_send_rx_cfm(conidx);
}

bool app_ota_send_notification(uint8_t conidx, uint8_t* ptrData, uint32_t length)
{
    return ota_send_ind_ntf_generic(true, conidx, ptrData, length);
}

bool app_ota_send_indication(uint8_t conidx, uint8_t* ptrData, uint32_t length)
{
    return ota_send_ind_ntf_generic(false, conidx, ptrData, length);
}

void app_ota_disconnected_evt_handler(uint8_t conidx)
{
    if(ota_event_cb)
    {
        app_ota_event_param_t param_info = {0};

        param_info.event_type = APP_OTA_DIS_CONN;
        param_info.conidx     = conidx;
        ota_event_cb(&param_info);
    }
}

void app_ota_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu)
{
    if(ota_event_cb)
    {
        TRACE(0,"[%s] %d %d",__func__, conidx, mtu);
        app_ota_event_param_t param_info = {0};

        param_info.event_type = APP_OTA_MTU_UPDATE;
        param_info.conidx     = conidx;
        param_info.param.mtu  = mtu;
        TRACE(0,"[%s] %d %d",__func__, param_info.conidx, param_info.param.mtu);

        ota_event_cb(&param_info);
    }
}

static int app_ota_ccc_changed_handler(ke_msg_id_t const msgid,
                                                  struct ble_ota_tx_notif_config_t *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    if(ota_event_cb)
    {
        app_ota_event_param_t param_info = {0};

        param_info.event_type   = APP_OTA_CCC_CHANGED;
        param_info.conidx       = param->conidx;
        param_info.param.ntf_en = param->ntfIndEnableFlag;

        ota_event_cb(&param_info);
    }
    return (KE_MSG_CONSUMED);
}

static int app_ota_tx_data_sent_handler(ke_msg_id_t const msgid,
                                                   struct ble_ota_tx_sent_ind_t *param,
                                                   ke_task_id_t const dest_id,
                                                   ke_task_id_t const src_id)
{
    if(ota_event_cb)
    {
        app_ota_event_param_t param_info = {0};

        param_info.event_type   = APP_OTA_SEND_DONE;
        param_info.conidx       = param->conidx;
        param_info.param.status = param->status;

        ota_event_cb(&param_info);
    }

    return (KE_MSG_CONSUMED);
}

void app_ota_data_receive_data(uint8_t conidx, uint8_t *data, uint16_t data_len)
{
    if(ota_event_cb)
    {
        app_ota_event_param_t param_info = {0};

        param_info.event_type   = APP_OTA_RECEVICE_DATA;
        param_info.conidx       = conidx;
        param_info.param.receive_data.data_len = data_len;
        param_info.param.receive_data.data     = data;

        ota_event_cb(&param_info);
    }
}

static int app_ota_data_received_handler(ke_msg_id_t const msgid,
                                                     struct ble_ota_rx_data_ind_t *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id)
{
    app_ota_data_receive_data(param->conidx, param->data, param->length);
    return (KE_MSG_CONSUMED);
}

static int app_ota_default_msg_handler(ke_msg_id_t const msgid,
                                                  void const *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    // Do nothing
    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_ota_msg_handler_list[] =
{
    // Note: first message is last message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)app_ota_default_msg_handler},

    {OTA_CCC_CHANGED,           (ke_msg_func_t)app_ota_ccc_changed_handler},
    {OTA_TX_DATA_SENT,          (ke_msg_func_t)app_ota_tx_data_sent_handler},
    {OTA_DATA_RECEIVED,         (ke_msg_func_t)app_ota_data_received_handler},
};

const struct app_subtask_handlers app_ota_table_handler =
    {app_ota_msg_handler_list, ARRAY_SIZE(app_ota_msg_handler_list)};

#endif //BLE_APP_OTA

/// @} APP

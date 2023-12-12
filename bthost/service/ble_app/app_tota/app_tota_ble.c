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

#if (BLE_APP_TOTA)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "string.h"
#include "gapm_msg.h"
#include "arch.h"                    // Platform Definitions
#include "co_bt.h"
#include "prf.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "app_tota_ble.h"                  // TOTA Application Definitions
#include "tota_ble.h"
#if(BLE_APP_OTA_OVER_TOTA)
#include "ota_control.h"
#endif

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static app_tota_event_callback app_tota_event_cb = NULL;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_tota_event_reg(app_tota_event_callback cb)
{
    if(!app_tota_event_cb)
    {
        app_tota_event_cb = cb;
    }
}

void app_tota_event_unreg(void)
{
    app_tota_event_cb = NULL;
}

void app_tota_add_tota(void)
{
    TRACE(0,"app_tota_add_tota, %d",TASK_ID_TOTA);
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM, TASK_APP,
                         gapm_profile_task_add_cmd, 0);

    /// Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
    req->user_prio = 0;
    req->prf_api_id = TASK_ID_TOTA;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    /// Send the message
    ke_msg_send(req);
}

bool app_tota_send_notification(uint8_t conidx, uint8_t* ptrData, uint32_t length)
{
    TRACE(1,"[%s] tota ntf",__func__);
    return tota_send_ind_ntf_generic(true, conidx, ptrData, length);
}

bool app_tota_send_indication(uint8_t conidx, uint8_t* ptrData, uint32_t length)
{
    TRACE(1,"[%s] tota ind",__func__);
    return tota_send_ind_ntf_generic(false, conidx, ptrData, length);
}

void app_tota_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu)
{
    if(app_tota_event_cb)
    {
        app_tota_event_param_t param_info;

        param_info.event_type = APP_TOTA_MTU_UPDATE;
        param_info.conidx = conidx;
        param_info.param.mtu    = mtu;
        app_tota_event_cb(&param_info);
    }
}

void app_tota_disconnected_evt_handler(uint8_t conidx)
{
    if(app_tota_event_cb)
    {
        app_tota_event_param_t param_info;

        param_info.event_type = APP_TOTA_DIS_CONN_EVENT;
        param_info.conidx = conidx;
        app_tota_event_cb(&param_info);
    }
}

static int app_tota_ccc_changed_handler(ke_msg_id_t const msgid,
                                                   struct ble_tota_tx_notif_config_t *param,
                                                   ke_task_id_t const dest_id,
                                                   ke_task_id_t const src_id)
{
    if(app_tota_event_cb)
    {
        uint8_t conidx = KE_IDX_GET(src_id);
        app_tota_event_param_t param_info;

        param_info.event_type   = APP_TOTA_CCC_CHANGED;
        param_info.conidx = conidx;

        param_info.param.ntf_en = param->isNotificationEnabled;
        app_tota_event_cb(&param_info);
    }
    return (KE_MSG_CONSUMED);
}

static int app_tota_tx_data_sent_handler(ke_msg_id_t const msgid,
                                                     struct ble_tota_tx_sent_ind_t *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id)
{
    if(app_tota_event_cb)
    {
        app_tota_event_param_t param_info;

        param_info.event_type   = APP_TOTA_SEND_DONE;
        param_info.param.status = param->status;
        app_tota_event_cb(&param_info);
    }
    return (KE_MSG_CONSUMED);
}

static int app_tota_data_received_handler(ke_msg_id_t const msgid,
                                                      struct ble_tota_rx_data_ind_t *param,
                                                      ke_task_id_t const dest_id,
                                                      ke_task_id_t const src_id)
{
    if(app_tota_event_cb)
    {
        app_tota_event_param_t param_info;

        param_info.event_type   = APP_TOTA_RECEVICE_DATA;
        param_info.param.receive_data.data_len = param->length;
        param_info.param.receive_data.data     = param->data;
        app_tota_event_cb(&param_info);
    }
    return (KE_MSG_CONSUMED);
}


static int app_tota_msg_default_handler(ke_msg_id_t const msgid,
                                                   void const *param,
                                                   ke_task_id_t const dest_id,
                                                   ke_task_id_t const src_id)
{
    // Do nothing
    TRACE(1,"[%s]TOTA",__func__);
    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_tota_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)app_tota_msg_default_handler},

    {TOTA_CCC_CHANGED,          (ke_msg_func_t)app_tota_ccc_changed_handler},
    {TOTA_TX_DATA_SENT,         (ke_msg_func_t)app_tota_tx_data_sent_handler},
    {TOTA_DATA_RECEIVED,        (ke_msg_func_t)app_tota_data_received_handler},
};

const struct app_subtask_handlers app_tota_table_handler =
    {app_tota_msg_handler_list, ARRAY_SIZE(app_tota_msg_handler_list)};

#endif //BLE_APP_TOTA

/// @} APP

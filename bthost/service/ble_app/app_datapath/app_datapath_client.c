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

#include "rwip_config.h"     // SW configuration
#include "app_ble_cmd_handler.h"

#if (CFG_APP_DATAPATH_CLIENT)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gapc_msg.h"
#include "gapm_msg.h"
#include "app.h"
#include "app_datapath_client.h"                  // Data Path Application Definitions
#include "app_task.h"                // application task definitions
#include "datapathpc_task.h"
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf.h"
#include "string.h"
#include "app_tota_custom.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// health thermometer application environment structure
struct app_datapath_client_env_tag app_datapath_client_env =
{
    BLE_INVALID_CONNECTION_INDEX,
};

static app_datapath_client_tx_done_t tx_done_callback = NULL;
static app_datapath_client_data_received_callback_func_t rx_done_callback = NULL;
static app_datapath_client_disconnected_done_t disconnected_done_callback = NULL;
static app_datapath_client_connected_done_t connected_done_callback = NULL;
static app_datapath_client_mtuexchanged_done_t mtuexchanged_done_callback = NULL;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_datapath_client_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu)
{
    if (NULL != mtuexchanged_done_callback)
    {
        mtuexchanged_done_callback(conidx, mtu);
    }
}

void app_datapath_client_connected_evt_handler(uint8_t conidx)
{
    TRACE(0,"app datapath client connected.");
    app_datapath_client_env.connectionIndex = conidx;

    if (NULL != connected_done_callback)
    {
        connected_done_callback(conidx);
    }
}

void app_datapath_client_disconnected_evt_handler(uint8_t conidx)
{
    if (conidx == app_datapath_client_env.connectionIndex)
    {
        TRACE(0,"app datapath client dis-connected.");
        app_datapath_client_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;

        tx_done_callback = NULL;
    }
    if (NULL != disconnected_done_callback)
    {
        disconnected_done_callback(conidx);
    }
}

void app_datapath_client_init(void)
{
    // Reset the environment
    app_datapath_client_env.connectionIndex =  BLE_INVALID_CONNECTION_INDEX;
}

void app_datapath_add_datapathpc(void)
{
    TRACE(1, "app_datapath_add_datapathpc %d", TASK_ID_DATAPATHPC);
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, 0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
    req->user_prio = 0;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    req->prf_api_id = TASK_ID_DATAPATHPC;

    // Send the message
    ke_msg_send(req);
}

void app_datapath_client_send_data_via_write_command(uint8_t* ptrData, uint32_t length)
{
    struct ble_datapath_send_data_req_t * req = KE_MSG_ALLOC_DYN(DATAPATHPC_SEND_DATA_VIA_WRITE_COMMAND,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPC),
                                                TASK_APP,
                                                ble_datapath_send_data_req_t,
                                                length);
    req->connecionIndex = 0;
    req->length = length;
    memcpy(req->value, ptrData, length);
    ke_msg_send(req);
}

void app_datapath_client_send_data_via_write_request(uint8_t* ptrData, uint32_t length)
{
    struct ble_datapath_send_data_req_t * req = KE_MSG_ALLOC_DYN(DATAPATHPC_SEND_DATA_VIA_WRITE_REQUEST,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPC),
                                                TASK_APP,
                                                ble_datapath_send_data_req_t,
                                                length);
    req->connecionIndex = app_datapath_client_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);
    ke_msg_send(req);
}

void app_datapath_client_discover(uint8_t conidx)
{
    TRACE(1, "%s", __func__);
    struct ble_datapath_discover_t * req = KE_MSG_ALLOC_DYN(DATAPATHPC_DISCOVER,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPC),
                                                TASK_APP,
                                                ble_datapath_discover_t,
                                                PRF_SVC_DESC_CLI_CFG_LEN);
    req->connecionIndex = conidx;
    req->length = PRF_SVC_DESC_CLI_CFG_LEN;
    ke_msg_send(req);
}

void app_datapath_client_control_notification(uint8_t conidx, bool isEnable)
{
    TRACE(1, "%s", __func__);
    struct ble_datapath_control_notification_t * req = KE_MSG_ALLOC_DYN(DATAPATHPC_CONTROL_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPC),
                                                TASK_APP,
                                                ble_datapath_control_notification_t,
                                                PRF_SVC_DESC_CLI_CFG_LEN);
    req->connecionIndex = conidx;
    req->length = PRF_SVC_DESC_CLI_CFG_LEN;
    uint16_t ccc_val = isEnable;
    memcpy(req->value, (const void *)&ccc_val, req->length);
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Handles health thermometer timer
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_datapath_client_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing

    return (KE_MSG_CONSUMED);
}

static int app_datapath_client_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_datapath_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    if (NULL != tx_done_callback)
    {
        tx_done_callback();
    }

    return (KE_MSG_CONSUMED);
}

static int app_datapath_client_rx_data_received_handler(ke_msg_id_t const msgid,
                              struct ble_datapath_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{

    TRACE(2,"%s length %d", __func__, param->length);
    //DUMP8("%02x ", (param->data+i), len);
    BLE_custom_command_receive_data(param->data, param->length);

    if (NULL != rx_done_callback)
    {
        rx_done_callback(param->data, param->length);
    }

    return (KE_MSG_CONSUMED);
}

void app_datapath_client_register_tx_done(app_datapath_client_tx_done_t callback)
{
    tx_done_callback = callback;
}

void app_datapath_client_register_rx_done(app_datapath_client_data_received_callback_func_t callback)
{
    rx_done_callback = callback;
}

void app_datapath_client_register_disconnected_done(app_datapath_client_disconnected_done_t callback)
{
    disconnected_done_callback = callback;
}

void app_datapath_client_register_connected_done(app_datapath_client_connected_done_t callback)
{
    connected_done_callback = callback;
}

void app_datapath_client_register_mtu_exchanged_done(app_datapath_client_mtuexchanged_done_t callback)
{
    mtuexchanged_done_callback = callback;
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_datapath_client_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_datapath_client_msg_handler},

    {DATAPATHPC_TX_DATA_SENT,       (ke_msg_func_t)app_datapath_client_tx_data_sent_handler},
    {DATAPATHPC_RX_DATA_RECEIVED,   (ke_msg_func_t)app_datapath_client_rx_data_received_handler},
    {DATAPATHPC_NOTIFICATION_RECEIVED, (ke_msg_func_t)app_datapath_client_rx_data_received_handler},
};

const struct app_subtask_handlers app_datapath_client_table_handler =
    {&app_datapath_client_msg_handler_list[0], (sizeof(app_datapath_client_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //BLE_APP_DATAPATH_CLIENT

/// @} APP

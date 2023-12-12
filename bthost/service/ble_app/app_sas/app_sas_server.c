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
#if (BLE_APP_SAS_SERVER)
/*
 * INCLUDE FILES
 ****************************************************************************************
*/
#include "app_sas_server.h"                 // SAS Application Module Definitions
#include "sas_task.h"
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"               // Profile common types definition
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include <string.h>

#ifdef ANC_APP
#include "app_anc.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Switching Ambient Serive Environment Structure
app_sas_env_tag_t app_sas_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_sas_server_init(void)
{
    // Reset the environment
    memset(&app_sas_env, 0, sizeof(app_sas_env_tag_t));

    app_sas_env.ambient_status = AMBIENT_SERVICE_OFF;
    app_sas_env.conidx = INVALID_CONNECTION_INDEX;
#ifdef ANC_APP
    app_anc_switch(APP_ANC_MODE_OFF);
#endif
}

void app_sas_add_sass(void)
{
    TRACE(1, "app_sas_add_sass %d", TASK_ID_SAS);
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, 0);
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
    req->user_prio = 0;
    req->start_hdl = 0;
    req->app_task = TASK_APP;
    req->prf_api_id = TASK_ID_SAS;

    // Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Handles of SAS Server
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_sas_server_msg_handler(ke_msg_id_t const msgid,
                                                      void const *param,
                                                      ke_task_id_t const dest_id,
                                                      ke_task_id_t const src_id)
{
    // Do nothing
    return (KE_MSG_CONSUMED);
}

void app_sas_server_update_ambient_status(AMBIENT_SERVICE_STATUS status)
{
    app_sas_env.ambient_status = status;
    TRACE(0, "ambient status changed to %d", status);
#ifdef ANC_APP    
    if (AMBIENT_SERVICE_ON == status)
    {
        app_anc_switch(APP_ANC_MODE1);
    }
    else
    {
        app_anc_switch(APP_ANC_MODE_OFF);
    }
#endif
    app_sas_server_inform_ambient_status();
}

void app_sas_server_inform_ambient_status(void)
{
    if ((INVALID_CONNECTION_INDEX != app_sas_env.conidx) && app_sas_env.isNotificationEnabled)
    {
        struct ble_sas_send_ntf_req_t * req = KE_MSG_ALLOC_DYN(SAS_SEND_NOTIFICATION,
                                                              prf_get_task_from_id(TASK_ID_SAS),
                                                              TASK_APP,
                                                              ble_sas_send_ntf_req_t,
                                                              2);

        req->connecionIndex = app_sas_env.conidx;
        req->audio_status   = app_sas_env.ambient_status;
        ke_msg_send(req);
    }
    else
    {
        TRACE(0, "sas conidx 0x%x ccc val %d", app_sas_env.conidx, app_sas_env.isNotificationEnabled);
    }
}

static int app_sas_server_ccc_changed_handler(ke_msg_id_t const msgid,
                                            struct ble_sas_notif_config_t *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    app_sas_env.isNotificationEnabled = param->isNotificationEnabled;

    if (app_sas_env.isNotificationEnabled)
    {
        // the app datapath server is connected when receiving the first enable CCC request
        if (BLE_INVALID_CONNECTION_INDEX == app_sas_env.conidx)
        {
            uint8_t conidx = KE_IDX_GET(src_id);
            if (app_sas_env.connected_cb)
            {
                app_sas_env.connected_cb(conidx);
            }
            app_sas_env.conidx = conidx;
            // fo test purpose
            // app_sas_server_inform_ambient_status();
        }
    }

    return (KE_MSG_CONSUMED);
}

static int app_sas_server_data_received_handler(ke_msg_id_t const msgid,
                  struct ble_sas_data_rx_ind_t *param,
                  ke_task_id_t const dest_id,
                  ke_task_id_t const src_id)
{
    app_sas_server_update_ambient_status(param->audio_status);
    if (NULL != app_sas_env.data_recieved)
    {
        app_sas_env.data_recieved(param->audio_status);
    }

    return (KE_MSG_CONSUMED);
}

void app_sas_server_register_data_received_cb(app_sas_server_data_received_cb_func_t callback)
{
    app_sas_env.data_recieved = callback;
}

void app_sas_server_register_disconnected_cb(app_sas_server_disconnected_cb_func_t callback)
{
    app_sas_env.disconnected_cb = callback;
}

void app_sas_server_register_connected_cb(app_sas_server_connected_cb_func_t callback)
{
    app_sas_env.connected_cb = callback;
}

void app_sas_server_disconnected_evt_handler(uint8_t conidx)
{
    if (conidx == app_sas_env.conidx)
    {
        TRACE(0,"app sas server dis-connected.");
        app_sas_env.conidx = INVALID_CONNECTION_INDEX;
        app_sas_env.isNotificationEnabled = false;
    }

    if (NULL != app_sas_env.disconnected_cb)
    {
        app_sas_env.disconnected_cb(conidx);
    }
}

/// Default State handlers definition
const struct ke_msg_handler app_sas_server_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_sas_server_msg_handler                           },

    {SAS_CCC_CHANGED,               (ke_msg_func_t)app_sas_server_ccc_changed_handler                   },

    {SAS_WRITE_CHAR_WITHOUT_RSP,    (ke_msg_func_t)app_sas_server_data_received_handler                  },
};

const struct app_subtask_handlers app_sas_server_table_handler =
    {&app_sas_server_msg_handler_list[0], (sizeof(app_sas_server_msg_handler_list)/sizeof(struct ke_msg_handler))};


#endif // (BLE_APP_SAS_SERVER)

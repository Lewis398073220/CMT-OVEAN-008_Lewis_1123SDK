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
#if (BLE_APP_AHP_SERVER)
/*
 * INCLUDE FILES
 ****************************************************************************************
*/
#include "app_ahp_server.h"           // AHP Application Module Definitions
#include "ahp_ahs.h"
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"               // Profile common types definition
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include <string.h>
#include "gapm_msg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Advanced Headphone Serive Environment Structure
app_ahps_env_tag_t app_ahps_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ahps_server_init(void)
{
    // Reset the environment
    memset(&app_ahps_env, 0, sizeof(app_ahps_env_tag_t));

    app_ahps_env.ahps_brc_status = AHP_SERVER_BRC_OFF;
    app_ahps_env.ahps_htc_status = AHP_SERVER_HTC_OFF;
    app_ahps_env.conidx = INVALID_CONNECTION_INDEX;
    app_ahps_env.isBRCNotificationEnabled = false;
    app_ahps_env.isHTCNotificationEnabled = false;
}

void app_ahps_add_ahs(void)
{
    TRACE(1, "app_ahps_add_ahs %d", TASK_ID_AHPS);
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, 0);
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
    req->user_prio = 0;
    req->start_hdl = 0;
    req->app_task = TASK_APP;
    req->prf_api_id = TASK_ID_AHPS;

    // Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Handles of AHP Server
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_ahp_server_msg_handler(ke_msg_id_t const msgid,
                                                      void const *param,
                                                      ke_task_id_t const dest_id,
                                                      ke_task_id_t const src_id)
{
    // Do nothing
    return (KE_MSG_CONSUMED);
}

void app_ahp_server_update_brc_status(ADVANCED_HEADPHONE_SERVICE_BRC_STATUS status)
{
    TRACE(0, "ARC status changed to %d", status);
    app_ahps_env.ahps_brc_status = status;
    app_ahp_server_inform_brc_status();
}

void app_ahp_server_inform_brc_status(void)
{
    if ((INVALID_CONNECTION_INDEX != app_ahps_env.conidx) && app_ahps_env.isBRCNotificationEnabled)
    {
        struct ble_ahps_brc_send_ntf_req_t * req = KE_MSG_ALLOC_DYN(AHPS_BRC_SEND_NOTIFICATION,
                                                              prf_get_task_from_id(TASK_ID_AHPS),
                                                              TASK_APP,
                                                              ble_ahps_brc_send_ntf_req_t,
                                                              2);

        req->connecionIndex = app_ahps_env.conidx;
        req->brc_status     = app_ahps_env.ahps_brc_status;
        ke_msg_send(req);
    }
    else
    {
        TRACE(0, "ahps conidx 0x%x ccc val %d", app_ahps_env.conidx, app_ahps_env.isBRCNotificationEnabled);
    }
}

void app_ahp_server_update_htc_status(ADVANCED_HEADPHONE_SERVICE_HTC_STATUS status)
{
    TRACE(0, "HTC status changed to %d", status);
    app_ahps_env.ahps_htc_status = status;
    app_ahp_server_inform_htc_status();
}

void app_ahp_server_inform_htc_status(void)
{
    if ((INVALID_CONNECTION_INDEX != app_ahps_env.conidx) && app_ahps_env.isHTCNotificationEnabled)
    {
        struct ble_ahps_htc_send_ntf_req_t * req = KE_MSG_ALLOC_DYN(AHPS_HTC_SEND_NOTIFICATION,
                                                              prf_get_task_from_id(TASK_ID_AHPS),
                                                              TASK_APP,
                                                              ble_ahps_htc_send_ntf_req_t,
                                                              2);

        req->connecionIndex = app_ahps_env.conidx;
        req->htc_status     = app_ahps_env.ahps_htc_status;
        ke_msg_send(req);
    }
    else
    {
        TRACE(0, "ahps conidx 0x%x ccc val %d", app_ahps_env.conidx, app_ahps_env.isHTCNotificationEnabled);
    }
}

static int app_ahp_server_brc_ccc_changed_handler(ke_msg_id_t const msgid,
                                            struct ble_ahps_brc_notif_config_t *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    app_ahps_env.isBRCNotificationEnabled = param->is_BRCNotifEnabled;

    if (app_ahps_env.isBRCNotificationEnabled)
    {
        // the app ahp server is connected when receiving the first enable CCC request
        if (BLE_INVALID_CONNECTION_INDEX == app_ahps_env.conidx)
        {
            uint8_t conidx = KE_IDX_GET(src_id);
            if (app_ahps_env.connected_cb)
            {
                app_ahps_env.connected_cb(conidx);
            }
            app_ahps_env.conidx = conidx;
        }
    }

    return (KE_MSG_CONSUMED);
}

static int app_ahp_server_htc_ccc_changed_handler(ke_msg_id_t const msgid,
                                            struct ble_ahps_htc_notif_config_t *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    app_ahps_env.isHTCNotificationEnabled = param->is_HTCNotifEnabled;

    if (app_ahps_env.isHTCNotificationEnabled)
    {
        // the app ahp server is connected when receiving the first enable CCC request
        if (BLE_INVALID_CONNECTION_INDEX == app_ahps_env.conidx)
        {
            uint8_t conidx = KE_IDX_GET(src_id);
            if (app_ahps_env.connected_cb)
            {
                app_ahps_env.connected_cb(conidx);
            }
            app_ahps_env.conidx = conidx;
        }
    }

    return (KE_MSG_CONSUMED);
}

static int app_ahp_server_brc_data_received_handler(ke_msg_id_t const msgid,
                  struct ble_ahps_brc_data_rx_ind_t *param,
                  ke_task_id_t const dest_id,
                  ke_task_id_t const src_id)
{
    app_ahp_server_update_brc_status(param->BRC_status);

    if (NULL != app_ahps_env.data_recieved)
    {
        app_ahps_env.data_recieved(param->BRC_status);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ahp_server_htc_data_received_handler(ke_msg_id_t const msgid,
                struct ble_ahps_htc_data_rx_ind_t *param,
                ke_task_id_t const dest_id,
                ke_task_id_t const src_id)
{
    app_ahp_server_update_htc_status(param->HTC_status);

    if (NULL != app_ahps_env.data_recieved)
    {
      app_ahps_env.data_recieved(param->HTC_status);
    }

    return (KE_MSG_CONSUMED);
}

void app_ahps_server_register_data_received_cb(app_ahps_server_data_received_cb_func_t callback)
{
    app_ahps_env.data_recieved = callback;
}

void app_ahps_server_register_disconnected_cb(app_ahps_server_disconnected_cb_func_t callback)
{
    app_ahps_env.disconnected_cb = callback;
}

void app_ahps_server_register_connected_cb(app_ahps_server_connected_cb_func_t callback)
{
    app_ahps_env.connected_cb = callback;
}

void app_ahp_server_disconnected_evt_handler(uint8_t conidx)
{
    if (conidx == app_ahps_env.conidx)
    {
        TRACE(0,"app ahps server dis-connected.");
        app_ahps_env.conidx = INVALID_CONNECTION_INDEX;
        app_ahps_env.isBRCNotificationEnabled = false;
        app_ahps_env.isHTCNotificationEnabled = false;
    }

    if (NULL != app_ahps_env.disconnected_cb)
    {
        app_ahps_env.disconnected_cb(conidx);
    }
}

/// Default State handlers definition
const struct ke_msg_handler app_ahp_server_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,          (ke_msg_func_t)app_ahp_server_msg_handler                         },

    {AHPS_BRC_CCC_CHANGED,            (ke_msg_func_t)app_ahp_server_brc_ccc_changed_handler             },

    {AHPS_HTC_CCC_CHANGED,            (ke_msg_func_t)app_ahp_server_htc_ccc_changed_handler             },

    {AHPS_BRC_WRITE_CHAR_WITHOUT_RSP, (ke_msg_func_t)app_ahp_server_brc_data_received_handler           },

    {AHPS_HTC_WRITE_CHAR_WITHOUT_RSP, (ke_msg_func_t)app_ahp_server_htc_data_received_handler           },
};

const struct app_subtask_handlers app_ahp_server_table_handler =
    {&app_ahp_server_msg_handler_list[0], (sizeof(app_ahp_server_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif // (BLE_APP_AHP_SERVER)

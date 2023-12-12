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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_APP_PRESENT)
#if (BLE_AMS_CLIENT)

#include "app_amsc.h"                  // Health Thermometer Application Definitions
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "amsc_task.h"               // health thermometer functions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions

#include "co_math.h"
#include "ke_timer.h"
#include "gapm_msg.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

void app_amsc_add_amsc(void)
{
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_profile_task_add_cmd, 0);

    /// Fill message
    req->operation  = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl    = (uint8_t)(SVC_SEC_LVL(AUTH) | ATT_UUID(128));
    req->user_prio  = 0;
    req->prf_api_id = TASK_ID_AMSC;
    req->start_hdl  = 0;

    /// Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Initialize application and enable AMSC profile.
 *
 ****************************************************************************************
 */
void app_amsc_enable(uint8_t conidx)
{
    TRACE(0,"app_amsc_enable");
    /// Allocate the message
    struct amsc_enable_req *req =
        KE_MSG_ALLOC(AMSC_ENABLE_REQ,
                     KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSC), conidx),
                     TASK_APP,
                     amsc_enable_req);

    /// Fill in the parameter structure
    req->conidx = conidx;

    /// Send the message
    ke_msg_send(req);
}

void app_amsc_read(uint8_t conidx, uint16_t hdl)
{
    /// ask client to send read command
    ams_cli_rd_cmd_t *cmd = KE_MSG_ALLOC(AMSC_READ_CMD,
                                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSC), conidx),
                                         KE_BUILD_ID(TASK_APP, conidx),
                                         ams_cli_rd_cmd);

    cmd->conidx = conidx;
    cmd->hdl = hdl;
    ke_msg_send(cmd);
}
void app_amsc_write(uint8_t conidx, uint16_t hdl, uint16_t value_length, uint8_t *value)
{
    if (NULL == value)
    {
        return ;
    }
    /// ask client to send write command
    ams_cli_wr_cmd_t *cmd = (ams_cli_wr_cmd_t *)ke_msg_alloc(AMSC_WRITE_CMD,
                                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSC), conidx),
                                         KE_BUILD_ID(TASK_APP, conidx),
                                         sizeof(ams_cli_wr_cmd_t) + value_length);

    memset(cmd, 0, sizeof(ams_cli_wr_cmd_t));
    cmd->conidx = conidx;
    cmd->hdl = hdl;
    cmd->value_length = value_length;
    if (value_length > 0)
    {
        memcpy(cmd->value, value, value_length);
    }
    ke_msg_send(cmd);
}

void app_amsp_add_svc(void)
{
    TRACE(0, "Registering AMS Proxy GATT Service");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM,
                         TASK_APP,
                         gapm_profile_task_add_cmd,
                         0);

    req->operation  = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl    = (uint8_t)(SVC_SEC_LVL(NO_AUTH) | SVC_UUID(128)); //!< is this service level OK?
    req->user_prio  = 0;
    req->prf_api_id = TASK_ID_AMSP;
    req->start_hdl  = 0;

    ke_msg_send(req);
}

#endif //BLE_AMS_CLIENT
#endif //BLE_APP_PRESENT

/// @} APP

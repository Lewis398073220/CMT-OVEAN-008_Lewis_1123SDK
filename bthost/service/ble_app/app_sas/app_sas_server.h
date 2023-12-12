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
 * @addtogroup APP_SAS
 * @{
 ****************************************************************************************
 */
#ifndef __APP_SAS_SERVER_H__
#define __APP_SAS_SERVER_H__

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration
#if (BLE_APP_SAS_SERVER)
/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */
/// SAS Application Module Environment Structure
typedef enum
{
    AMBIENT_SERVICE_OFF = 0,
    AMBIENT_SERVICE_ON  = 1
} AMBIENT_SERVICE_STATUS;

typedef void(*app_sas_server_data_received_cb_func_t)(AMBIENT_SERVICE_STATUS status);

typedef void(*app_sas_server_connected_cb_func_t)(uint8_t conidx);

typedef void(*app_sas_server_disconnected_cb_func_t)(uint8_t conidx);

typedef struct
{
    /// is_notification
    bool isNotificationEnabled;
    /// Connection handle
    uint8_t conidx;
    /// Current ambeient status
    AMBIENT_SERVICE_STATUS ambient_status;

    // called when ambient service characteristic is written
    app_sas_server_data_received_cb_func_t data_recieved;

    // called when ccc is written
    app_sas_server_connected_cb_func_t connected_cb;

    // called when the cooresponding connection is disconnected
    app_sas_server_disconnected_cb_func_t disconnected_cb;

} app_sas_env_tag_t;

#define INVALID_CONNECTION_INDEX 0xFF

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// Table of message handlers
extern const struct app_subtask_handlers app_sas_server_table_handler;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_sas_server_register_data_received_cb(app_sas_server_data_received_cb_func_t callback);
void app_sas_server_register_connected_cb(app_sas_server_connected_cb_func_t callback);
void app_sas_server_register_disconnected_cb(app_sas_server_disconnected_cb_func_t callback);
void app_sas_server_update_ambient_status(AMBIENT_SERVICE_STATUS status);
void app_sas_server_inform_ambient_status(void);
void app_sas_server_disconnected_evt_handler(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Initialize SAS Server Application
 ****************************************************************************************
 */
void app_sas_server_init(void);
void app_sas_add_sass(void);

#endif /// (BLE_APP_SAS_SERVER)
#endif /// __APP_SAS_SERVER_H__
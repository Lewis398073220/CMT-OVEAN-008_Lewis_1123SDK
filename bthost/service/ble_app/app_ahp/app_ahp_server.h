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
 * @addtogroup APP_AHP
 * @{
 ****************************************************************************************
 */
#ifndef __APP_AHP_SERVER_H__
#define __APP_AHP_SERVER_H__

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration
#if (BLE_APP_AHP_SERVER)
/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */

/// AHPS BRC Application Module Environment Structure
typedef enum
{
    AHP_SERVER_BRC_OFF = 0,
    AHP_SERVER_BRC_ON  = 1
} ADVANCED_HEADPHONE_SERVICE_BRC_STATUS;

/// AHPS HTC Application Module Environment Structure
typedef enum
{
    AHP_SERVER_HTC_OFF = 0,
    AHP_SERVER_HTC_ON  = 1
} ADVANCED_HEADPHONE_SERVICE_HTC_STATUS;

typedef void(*app_ahps_server_data_received_cb_func_t)(uint8_t status);

typedef void(*app_ahps_server_connected_cb_func_t)(uint8_t conidx);

typedef void(*app_ahps_server_disconnected_cb_func_t)(uint8_t conidx);

typedef struct
{
    /// is_BRC_notification
    bool isBRCNotificationEnabled;
    /// is_HTC_notification
    bool isHTCNotificationEnabled;
    /// Connection handle
    uint8_t conidx;

    /// Current ARC status
    ADVANCED_HEADPHONE_SERVICE_BRC_STATUS ahps_brc_status;

    /// Current HTC status
    ADVANCED_HEADPHONE_SERVICE_HTC_STATUS ahps_htc_status;

    // called when AHP Server characteristic is written
    app_ahps_server_data_received_cb_func_t data_recieved;

    // called when ccc is written
    app_ahps_server_connected_cb_func_t connected_cb;

    // called when the cooresponding connection is disconnected
    app_ahps_server_disconnected_cb_func_t disconnected_cb;

} app_ahps_env_tag_t;

#define INVALID_CONNECTION_INDEX 0xFF

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// Table of message handlers
extern const struct app_subtask_handlers app_ahp_server_table_handler;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ahps_server_register_data_received_cb(app_ahps_server_data_received_cb_func_t callback);
void app_ahps_server_register_disconnected_cb(app_ahps_server_disconnected_cb_func_t callback);
void app_ahps_server_register_connected_cb(app_ahps_server_connected_cb_func_t callback);
void app_ahp_server_update_brc_status(ADVANCED_HEADPHONE_SERVICE_BRC_STATUS status);
void app_ahp_server_update_htc_status(ADVANCED_HEADPHONE_SERVICE_HTC_STATUS status);
void app_ahp_server_inform_brc_status(void);
void app_ahp_server_inform_htc_status(void);
void app_ahp_server_disconnected_evt_handler(uint8_t conidx);
/**
 ****************************************************************************************
 * @brief Initialize AHP Server Application
 ****************************************************************************************
 */
void app_ahps_server_init(void);
void app_ahps_add_ahs(void);
#endif /// (BLE_APP_AHP_SERVER)
#endif /// __APP_AHP_SERVER_H__


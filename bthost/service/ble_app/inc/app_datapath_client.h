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
#ifndef APP_DATAPATH_CLIENT_H_
#define APP_DATAPATH_CLIENT_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief DataPath Client Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (CFG_APP_DATAPATH_CLIENT)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"

#define BLE_INVALID_CONNECTION_INDEX    0xFF

#define HIGH_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS        20
#define HIGH_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS        30
#define HIGH_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS  2000
#define HIGH_SPEED_BLE_CONNECTION_SLAVELATENCY              0



#define LOW_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS         400
#define LOW_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS         500
#define LOW_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS   2000
#define LOW_SPEED_BLE_CONNECTION_SLAVELATENCY               0

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


extern struct app_env_tag app_env;

/// health thermometer application environment structure
struct app_datapath_client_env_tag
{
    uint8_t connectionIndex;
};

typedef void(*app_datapath_client_tx_done_t)(void);
typedef void(*app_datapath_client_data_received_callback_func_t)(uint8_t *p_buff, uint16_t bufLength);
typedef void(*app_datapath_client_disconnected_done_t)(uint8_t conidx);
typedef void(*app_datapath_client_connected_done_t)(uint8_t conidx);
typedef void(*app_datapath_client_mtuexchanged_done_t)(uint8_t conidx, uint16_t mtu);

typedef void(*app_datapath_client_activity_stopped_t)(void);

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Health Thermomter Application environment
extern struct app_datapath_client_env_tag app_datapath_client_env;

/// Table of message handlers
extern const struct app_subtask_handlers app_datapath_client_table_handler;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize DataPath client Application
 ****************************************************************************************
 */
void app_datapath_client_init(void);

/**
 ****************************************************************************************
 * @brief Add a DataPath CLIENT instance in the DB
 ****************************************************************************************
 */
void app_datapath_client_register_tx_done(app_datapath_client_tx_done_t callback);

void app_datapath_client_register_rx_done(app_datapath_client_data_received_callback_func_t callback);

void app_datapath_client_register_disconnected_done(app_datapath_client_disconnected_done_t callback);

void app_datapath_client_register_connected_done(app_datapath_client_connected_done_t callback);

void app_datapath_client_register_mtu_exchanged_done(app_datapath_client_mtuexchanged_done_t callback);

void app_datapath_add_datapathpc(void);

void app_datapath_client_connected_evt_handler(uint8_t conidx);

void app_datapath_client_disconnected_evt_handler(uint8_t conidx);

void app_datapath_client_send_data_via_write_command(uint8_t* ptrData, uint32_t length);

void app_datapath_client_send_data_via_write_request(uint8_t* ptrData, uint32_t length);

void app_datapath_client_control_notification(uint8_t conidx,bool isEnable);

void app_datapath_client_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu);

void app_datapath_client_discover(uint8_t conidx);

#ifdef __cplusplus
}
#endif

#endif //(CFG_APP_DATAPATH_CLIENT)

/// @} APP

#endif // APP_DATAPATH_CLIENT_H_

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


#ifndef APP_OTA_H_
#define APP_OTA_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief OTA Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_OTA)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"
#include "app_ble_mode_switch.h"
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */
typedef enum {
    APP_OTA_CCC_CHANGED = 0,
    APP_OTA_DIS_CONN,
    APP_OTA_RECEVICE_DATA,
    APP_OTA_MTU_UPDATE,
    APP_OTA_SEND_DONE,
} APP_OTA_EVENT_TYPE_E;

typedef struct {
    /// tota event type, see@APP_OTA_EVENT_TYPE_E
    uint8_t event_type;
    uint8_t conidx;
    uint16_t connhdl;
    union {
       /// APP_OTA_CCC_CHANGED
       uint8_t ntf_en;
       // APP_OTA_MTU_UPDATE
       uint16_t mtu;
       // APP_OTA_SEND_DONE
       uint8_t status;
       // APP_OTA_RECEVICE_DATA
       struct {
           uint16_t data_len;
           uint8_t *data;
       } receive_data;
    } param;
} app_ota_event_param_t;

typedef void(*app_ota_event_callback)(app_ota_event_param_t *param);

/// Table of message handlers
extern const struct app_subtask_handlers app_ota_table_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize DataPath Server Application
 ****************************************************************************************
 */

void app_ota_add_ota(void);

void app_ota_event_reg(app_ota_event_callback cb);

void app_ota_event_unreg(void);

void app_ota_send_rx_cfm(uint8_t conidx);

bool app_ota_send_notification(uint8_t conidx, uint8_t* ptrData, uint32_t length);

bool app_ota_send_indication(uint8_t conidx, uint8_t* ptrData, uint32_t length);

void app_ota_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu);

void app_ota_disconnected_evt_handler(uint8_t conidx);

void app_ota_data_receive_data(uint8_t conidx, uint8_t *data, uint16_t data_len);

#ifdef __cplusplus
    }
#endif


#endif //(BLE_APP_OTA)

/// @} APP

#endif // APP_OTA_H_

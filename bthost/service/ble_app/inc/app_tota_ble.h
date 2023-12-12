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


#ifndef APP_TOTA_H_
#define APP_TOTA_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief TOTA Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_TOTA)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_TOTA_CCC_CHANGED = 0,
    APP_TOTA_DIS_CONN_EVENT,
    APP_TOTA_RECEVICE_DATA,
    APP_TOTA_MTU_UPDATE,
    APP_TOTA_SEND_DONE,
} APP_TOTA_EVENT_TYPE_E;

typedef struct {
    /// tota event type, see@APP_TOTA_EVENT_TYPE_E
    uint8_t event_type;
    uint8_t conidx;
    uint16_t connhdl;
    union {
       /// APP_TOTA_CCC_CHANGED
       uint8_t ntf_en;
       // APP_TOTA_MTU_UPDATE
       uint16_t mtu;
       // APP_TOTA_SEND_DONE
       uint8_t status;
       // APP_TOTA_RECEVICE_DATA
       struct {
           uint16_t data_len;
           uint8_t *data;
       } receive_data;
    } param;
} app_tota_event_param_t;

typedef void(*app_tota_event_callback)(app_tota_event_param_t *param);

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Table of message handlers
extern const struct app_subtask_handlers app_tota_table_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize DataPath Server Application
 ****************************************************************************************
 */
void app_tota_add_tota(void);

void app_tota_event_reg(app_tota_event_callback cb);

void app_tota_event_unreg(void);

bool app_tota_send_notification(uint8_t conidx, uint8_t* ptrData, uint32_t length);

bool app_tota_send_indication(uint8_t conidx, uint8_t* ptrData, uint32_t length);

void app_tota_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu);

void app_tota_disconnected_evt_handler(uint8_t conidx);
#ifdef __cplusplus
    }
#endif


#endif //(BLE_APP_TOTA)

/// @} APP

#endif // APP_TOTA_H_
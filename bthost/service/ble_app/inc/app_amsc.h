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


#ifndef APP_AMSC_H_
#define APP_AMSC_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief AMSC Application entry point.
 *
 * @{
 ****************************************************************************************
 */


#if BLE_AMS_CLIENT
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * AMSC Application Functions
 *
 ****************************************************************************************
 */
 typedef struct {
    /// Connection index
    uint8_t conidx;
    /// Attribute handle
    uint16_t hdl;
    /// Value length
    uint16_t len;
    /// Value to transmit
    uint8_t *data;
}app_amsc_srv_param;

void app_amsc_add_amsc(void);

/**
 ****************************************************************************************
 * @brief Inialize application and enable AMSC profile.
 *
 ****************************************************************************************
 */
void app_amsc_enable(uint8_t conidx);
void app_amsc_read(uint8_t conidx, uint16_t hdl);
void app_amsc_write(uint8_t conidx, uint16_t hdl, uint16_t value_length, uint8_t *value);

/**
 ****************************************************************************************
 *
 * AMSP Application Functions
 *
 ****************************************************************************************
 */
void app_amsp_add_svc(void);

#endif //BLE_AMS_CLIENT

/// @} APP

#endif // APP_AMSC_H_

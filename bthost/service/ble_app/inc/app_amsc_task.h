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


#include "rwip_config.h"

#ifndef APP_AMSC_TASK_H_
#define APP_AMSC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief 
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#if BLE_AMS_CLIENT

#include "amsc_task.h"
typedef enum app_amsc_event
{
    APP_AMSC_EVENT_BOND_SUCCESS = 0,
    APP_AMSC_EVENT_BOND_FAILED,
    APP_AMSC_EVENT_ENABLE_RSP,
    APP_AMSC_EVENT_WRITE_RSP,
    APP_AMSC_EVENT_INDICATION,
    APP_AMSC_EVENT_ATT_VAL,
} app_amsc_event_t;

typedef struct app_amsc_event_param
{
    uint8_t conidx;
    union
    {
        struct
        {
            bt_bdaddr_t *addr;
        } bond_success;
        struct
        {
            bt_bdaddr_t *addr;
        } bond_failed;
        struct
        {
            bt_bdaddr_t *addr;
            bool enable;
            uint16_t remote_command_char;
            uint16_t entity_update_char;
            uint16_t entity_attribute_char;
        } enable_rsp;
        struct
        {
            uint16_t hdl;
        } write_rsp;
        struct
        {
            uint16_t hdl;
            const uint8_t *value;
            uint16_t value_len;
        } indication;
        struct
        {
            uint16_t hdl;
            const uint8_t *value;
            uint16_t value_len;
        } att_val;
    } p;
} app_amsc_event_param_t;
/// Table of message handlers
extern const struct app_subtask_handlers app_amsc_table_handler;

#endif //BLE_AMS_CLIENT

/// @} APP

#endif // APP_AMSC_TASK_H_

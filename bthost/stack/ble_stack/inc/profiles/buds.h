/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
 
#ifndef _BUDS_H_
#define _BUDS_H_

/**
 ****************************************************************************************
 * @addtogroup BUDS Profile
 * @ingroup BUDS
 * @brief Buds Profile
 *
 * Buds Profile provides functionalities to upper layer module
 * application. The device using this profile takes the role of Buds Server.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Send data to peer device via notifications  (from APP)
 *  - Receive data from peer device via write no response (from APP)
 *
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BUDS)
#include "prf_types.h"
#include "prf.h"
#include "buds_task.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define BUDS_MAX_LEN            (509)    // consider the extended data length

/*
 * MACROS
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */
/// Possible states of the BUDS task
enum
{
    /// Idle state
    BUDS_IDLE,
    /// Connected state
    BUDS_BUSY,

    /// Number of defined states.
    BUDS_STATE_MAX,
};

///Attributes State Machine
typedef enum
{
    BUDS_IDX_SVC,

    BUDS_IDX_TX_CHAR,
    BUDS_IDX_TX_VAL,
    BUDS_IDX_TX_NTF_CFG,

    BUDS_IDX_RX_CHAR,
    BUDS_IDX_RX_VAL,

    BUDS_IDX_NB,
} buds_idx_e;

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
typedef void (*buds_att_evt_callback_t)(buds_idx_e, uint8_t*, uint16_t);

/// Buds Profile Server environment variable
typedef struct buds_env
{
    prf_hdr_t   hdr;
    /// Service Start Handle
    uint16_t    shdl;
    /// flag to mark whether notification or indication is enabled
    uint16_t    ntfIndEnableFlag[BLE_CONNECTION_MAX];
    /// State of different task instances
    ke_state_t  state;
    /// GATT User local index
    uint8_t     srv_user_lid;
    uint8_t     cli_user_lid;
}buds_env_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve buds service profile interface
 *
 * @return buds service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* buds_prf_itf_get(void);


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void buds_task_init(struct ke_task_desc *task_desc, void *p_env);

void app_add_buds(void);

bool buds_send_indication(uint8_t conidx, const uint8_t* ptrData, uint32_t length);

bool buds_send_notification(uint8_t conidx, const uint8_t* ptrData, uint32_t length);

bool buds_send_data_via_write_command(uint8_t conidx, uint8_t* ptrData, uint32_t length);

bool buds_send_data_via_write_request(uint8_t conidx, uint8_t* ptrData, uint32_t length);

void buds_control_ind_ntf(uint8_t conidx, bool isEnable);

void buds_register_cli_event(uint8_t conidx);

void buds_data_received_register(buds_att_evt_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* #if (BLE_BUDS) */

/// @} BUDS

#endif /* _BUDS_H_ */


/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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

#ifndef _DATAPATHPC_H_
#define _DATAPATHPC_H_

/**
 ****************************************************************************************
 * @addtogroup DATAPATHPC Datapath Profile client
 * @ingroup DATAPATHP
 * @brief Datapath Profile client
 *
 * Datapath Profile client provides functionalities to upper layer module
 * application. The device using this profile takes the role of Datapath client.
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
#include "ke_task.h"
#include "ke_msg.h"

#if (CFG_APP_DATAPATH_CLIENT)
#include "prf_types.h"
#include "prf.h"
#include "datapathpc_task.h"
#include "prf_utils.h"


#define BLE_MAXIMUM_CHARACTERISTIC_DESCRIPTION    32

static const char custom_tx_desc[] = "Data Path TX Data";
static const char custom_rx_desc[] = "Data Path RX Data";



#define datapathps_uuid_128_content       {0x12, 0x34, 0x56, 0x78, 0x90, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01 }

static const uint8_t DATAPATHPS_UUID_128[GATT_UUID_128_LEN] = datapathps_uuid_128_content;

/*
 * DEFINES
 ****************************************************************************************
 */

#define DATAPATHPC_MAX_LEN            (509)    // consider the extended data length

#define DATAPATHPC_MANDATORY_MASK             (0x0F)
#define DATAPATHPC_BODY_SENSOR_LOC_MASK       (0x30)
#define DATAPATHPC_HR_CTNL_PT_MASK            (0xC0)

#define HRP_PRF_CFG_PERFORMED_OK        (0x80)

/*
 * MACROS
 ****************************************************************************************
 */

#define DATAPATHPC_IS_SUPPORTED(features, mask) ((features & mask) == mask)


/*
 * DEFINES
 ****************************************************************************************
 */
/// Possible states of the DATAPATHPC task
enum
{
    /// Idle state
    DATAPATHPC_IDLE,
    /// Connected state
    DATAPATHPC_BUSY,

    /// Number of defined states.
    DATAPATHPC_STATE_MAX,
};

///Attributes State Machine
enum
{
    DATAPATHPC_IDX_SVC,

    DATAPATHPC_IDX_TX_CHAR,
    DATAPATHPC_IDX_TX_VAL,
    DATAPATHPC_IDX_TX_NTF_CFG,
    DATAPATHPC_IDX_TX_DESC,

    DATAPATHPC_IDX_RX_CHAR,
    DATAPATHPC_IDX_RX_VAL,

    DATAPATHPC_IDX_RX_DESC,

    DATAPATHPC_IDX_NB,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


///Structure containing the characteristics handles, value handles and descriptors
typedef struct datapathpc_content
{
    /// service info
    prf_svc_t           svc;

} datapathpc_content_t;

/// Environment variable for each Connections
typedef struct datapathpc_cnx_env
{
    /// Peer database discovered handle mapping
    datapathpc_content_t       datapath;
    /// counter used to check service uniqueness
    uint8_t             nb_svc;
    /// Client is in discovering state
    bool                discover;
} datapathpc_cnx_env_t;

/// Datapath Profile client environment variable
typedef struct datapathpc_env
{
    prf_hdr_t hdr;
    /// Environment variable pointer for each connections
    datapathpc_cnx_env_t*      p_env[BLE_CONNECTION_MAX];
    /// State of different task instances
    ke_state_t  state;
    /// GATT User local index
    uint8_t     user_lid;
}datapathpc_env_t;


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
 * @brief Retrieve HRP service profile interface
 *
 * @return HRP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* datapathpc_prf_itf_get(void);


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
void datapathpc_task_init(struct ke_task_desc *task_desc, void *p_env);


uint16_t datapathpc_discover(uint8_t conidx, uint8_t con_type, const datapathpc_content_t* p_datapath);

const gatt_cli_cb_t* datapathpc_task_get_cli_cbs(void);
#endif /* #if (BLE_DATAPATH_CLIENT) */

/// @} DATAPATHPC

#endif /* _DATAPATHPC_H_ */


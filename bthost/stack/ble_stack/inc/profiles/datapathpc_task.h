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

#ifndef _DATAPATHPC_TASK_H_
#define _DATAPATHPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DATAPATHPCTASK Task
 * @ingroup DATAPATHPC
 * @brief Heart Rate Profile Task.
 *
 * The DATAPATHPCTASK is responsible for handling the messages coming in and out of the
 * @ref DATAPATHPC collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "rwip_task.h" // Task definitions
#include "datapath_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Messages for Data Path client Profile
enum datapathpc_msg_id
{
    DATAPATHPC_TX_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_DATAPATHPC),

    DATAPATHPC_TX_DATA_SENT,

    DATAPATHPC_RX_DATA_RECEIVED,

    DATAPATHPC_NOTIFICATION_RECEIVED,

    DATAPATHPC_SEND_DATA_VIA_WRITE_COMMAND,

    DATAPATHPC_SEND_DATA_VIA_WRITE_REQUEST,

    DATAPATHPC_CONTROL_NOTIFICATION,

    DATAPATHPC_DISCOVER,

};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */



/// @} DATAPATHPCTASK

#endif /* _DATAPATHPC_TASK_H_ */

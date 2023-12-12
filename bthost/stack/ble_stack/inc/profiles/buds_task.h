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
 
#ifndef _BUDS_TASK_H_
#define _BUDS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BUDSTASK Task
 * @ingroup BUDS
 * @brief Buds Profile Task.
 *
 * The BUDSTASK is responsible for handling the messages coming in and out of the
 * @ref BUDS collector block of the BLE Host.
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

/*
 * DEFINES
 ****************************************************************************************
 */
/// Messages for Data Path Server Profile 
enum buds_msg_id
{
    BUDS_CONTROL_NOTIFICATION = TASK_FIRST_MSG(TASK_ID_BUDS),
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// @} BUDSTASK

#endif /* _BUDS_TASK_H_ */

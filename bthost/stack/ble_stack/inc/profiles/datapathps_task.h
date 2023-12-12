#ifndef _DATAPATHPS_TASK_H_
#define _DATAPATHPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DATAPATHPSTASK Task
 * @ingroup DATAPATHPS
 * @brief Heart Rate Profile Task.
 *
 * The DATAPATHPSTASK is responsible for handling the messages coming in and out of the
 * @ref DATAPATHPS collector block of the BLE Host.
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
/// Messages for Data Path Server Profile
enum datapathps_msg_id
{
    DATAPATHPS_TX_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_DATAPATHPS),

    DATAPATHPS_TX_DATA_SENT,

    DATAPATHPS_RX_DATA_RECEIVED,

    DATAPATHPS_SEND_DATA_VIA_NOTIFICATION,

    DATAPATHPS_SEND_DATA_VIA_INDICATION,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// @} DATAPATHPSTASK

#endif /* _DATAPATHPS_TASK_H_ */

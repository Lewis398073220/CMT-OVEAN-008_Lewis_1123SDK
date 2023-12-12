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
#ifndef _DATAPATH_COMMON_H_
#define _DATAPATH_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup DATAPATH_COMMON Datapath Common
 * @ingroup DATAPATH_COMMON
 * @brief Datapath Common declarations
 *
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
#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"

struct ble_datapath_tx_notif_config_t
{
    bool         isNotificationEnabled;
};

struct ble_datapath_rx_data_ind_t
{
    uint16_t    length;
    uint8_t        data[0];
};

struct ble_datapath_tx_sent_ind_t
{
    uint8_t     status;
};

struct ble_datapath_send_data_req_t
{
    uint8_t     connecionIndex;
    uint32_t     length;
    uint8_t      value[__ARRAY_EMPTY];
};

struct ble_datapath_control_notification_t
{
    uint8_t     connecionIndex;
    uint32_t     length;
    uint8_t      value[__ARRAY_EMPTY];
};

struct ble_datapath_discover_t
{
    uint8_t     connecionIndex;
    uint32_t     length;
    uint8_t      value[__ARRAY_EMPTY];
};

/// @} DATAPATH_COMMON

#endif /* _DATAPATH_COMMON_H_ */


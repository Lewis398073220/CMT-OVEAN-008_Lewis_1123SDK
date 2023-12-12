/**
 * @file app_gaf_common.h
 * @author BES AI team
 * @version 0.1
 * @date 2021-06-22
 *
 * @copyright Copyright (c) 2015-2021 BES Technic.
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
 */
/**
 ****************************************************************************************
 * @addtogroup APP_GAF
 * @{
 ****************************************************************************************
 */
/**
 * NOTE: This header file defines the common used module within GAF_core layer
 */

#ifndef __APP_GAF_COMMON_H__
#define __APP_GAF_COMMON_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include <stdint.h>
#include <stdbool.h>       // standard boolean definitions

#include "compiler.h"
/******************************macro defination*****************************/

#define APP_CO_BIT(pos) (1UL<<(pos))

/// Length of GAP Broadcast ID
#define APP_GAP_BCAST_ID_LEN    (3)
/// Length of GAP broadcast Key
#define APP_GAP_KEY_LEN         (16)
/// Length of Device Address
#define APP_GAP_BD_ADDR_LEN     (6)
/// Default Preffered MTU
#define APP_GAF_DFT_PREF_MTU    (128)
/// Length of Codec ID value
#define APP_GAF_CODEC_ID_LEN    (5)

/******************************type defination******************************/

/// ASE Direction
typedef enum app_gaf_direction
{
    /// Sink direction
    APP_GAF_DIRECTION_SINK = 0,
    /// Source direction
    APP_GAF_DIRECTION_SRC,

    APP_GAF_DIRECTION_MAX,
} app_gaf_direction_t;

/// Direction requirements bit field
enum app_gaf_direction_bf
{
    /// Required for sink direction
    APP_GAF_DIRECTION_BF_SINK_POS = 0,
    APP_GAF_DIRECTION_BF_SINK_BIT = 0x01,
    /// Required for source direction
    APP_GAF_DIRECTION_BF_SRC_POS = 1,
    APP_GAF_DIRECTION_BF_SRC_BIT = 0x02,
    /// Required for both directions
    APP_GAF_DIRECTION_BF_BOTH = APP_GAF_DIRECTION_BF_SRC_BIT + APP_GAF_DIRECTION_BF_SINK_BIT,
};


/******************************macro defination*****************************/

/****************************function declaration***************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_GAF_COMMON_H__ */

/// @} APP_GAF

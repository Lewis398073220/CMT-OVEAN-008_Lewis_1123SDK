
/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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

#ifndef _BLE_COMM_CONFIG_H_
#define _BLE_COMM_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdio.h>
#include "rwip_config.h"

/******************************************************************************************/
/* -------------------------   KERNEL SETUP          -------------------------------------*/
/******************************************************************************************/

/**
 * Size of non-retention heap
 *
 * This heap can be used to split the RAM into 2 parts:
 *    - an always-on part that can handle a certain number of links
 *    - a secondary memory that could be powered-off when not used, and retained only when used
 *
 * With such mechanism, the previous heaps need to be reduced so that they can contain all required data
 * in a light scenario (few connections, few profiles). Then the non-retention heap is sized in order to
 * cover the worst case scenario (max connections, max profiles, etc ...)
 *
 * The current size show what is already known as not needing to be retained during deep sleep.
 */

/// Heap header size is 12 bytes
#define RWIP_HEAP_HEADER                  (12 / sizeof(uint32_t))

/// ceil(len/sizeof(uint32_t)) + RWIP_HEAP_HEADER
#define RWIP_CALC_HEAP_LEN(len)           ((((len) + (sizeof(uint32_t) - 1)) / sizeof(uint32_t)) + RWIP_HEAP_HEADER)

/// compute size of the heap block in bytes
#define RWIP_CALC_HEAP_LEN_IN_BYTES(len)  (RWIP_CALC_HEAP_LEN(len) * sizeof(uint32_t))

/// environment heap array size
#define RWIP_HEAP_ENV_SIZE                (640 + BLE_CONNECTION_MAX * 480 + BES_BLE_ACTIVITY_MAX * 196)

/// default Size of Attribute database heap
#if (HL_LE_CENTRAL || HL_LE_PERIPHERAL)
#if BLE_AUDIO_ENABLED
    /// TODO: may not need so much ble memory, Kernel Message Heap Size
#ifdef BLE_AUDIO_IS_SUPPORT_CENTRAL_ROLE
    #define RWIP_HEAP_PROFILE_SIZE        3*1024
    #define RWIP_HEAP_MSG_SIZE            (7*2048 + 256 * BLE_CONNECTION_MAX)
#else
    #define RWIP_HEAP_PROFILE_SIZE        1*1024
    #define RWIP_HEAP_MSG_SIZE            (5*2048 + 256 * BLE_CONNECTION_MAX)
#endif
#elif __GATT_OVER_BR_EDR__
    #define RWIP_HEAP_PROFILE_SIZE        3*1024
    #define RWIP_HEAP_MSG_SIZE            (3*2048 + 256 * BLE_CONNECTION_MAX)
#else
    #define RWIP_HEAP_PROFILE_SIZE        3*1024
    #define RWIP_HEAP_MSG_SIZE            (2048 + 256 * BLE_CONNECTION_MAX)
#endif /// BLE_AUDIO_ENABLED
#endif /// HL_LE_CENTRAL || HL_LE_PERIPHERAL

/// default Size of non-retention heap
#define RWIP_HEAP_NON_RET_SIZE            0

/// IP heap structure
typedef struct {
    /// env heap size variable
    uint32_t rwip_env_heap_size;

    /// db heap size variable
    uint32_t rwip_profile_heap_size;

    /// msg heap size variable
    uint32_t rwip_msg_heap_size;

    /// non_ret heap size variable
    uint32_t rwip_non_ret_heap_size;

    /// env heap size variable point
    uint32_t *rwip_env_heap_p;

    /// db heap size variable point
    uint32_t *rwip_profile_heap_p;

    /// msg heap size variable
    uint32_t *rwip_msg_heap_p;

    /// non_ret heap size variable
    uint32_t *rwip_non_ret_heap_p;
}rwip_heap_config_t;

typedef struct besble_cfg{
    bool vol_control_by_ux;
    // Add more configuration here
} __attribute ((packed)) besble_cfg_t;

void ble_comm_config_rw_heap(rwip_heap_config_t* pConfig);
void ble_common_config_init(void);
besble_cfg_t *ble_common_get_cfg(void);

#endif /* _BLE_COMM_CONFIG_H_ */

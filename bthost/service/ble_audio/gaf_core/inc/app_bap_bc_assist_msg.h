/**
 ****************************************************************************************
 *
 * @file app_bap_bc_assist_msg.h
 *
 * @brief BLE Audio Broadcast Assistant
 *
 * Copyright 2015-2019 BES.
 *
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
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP_ACC
 
 * @{
 ****************************************************************************************
 */

#ifndef APP_BAP_ASSIST_MSG_H_
#define APP_BAP_ASSIST_MSG_H_
#if BLE_AUDIO_ENABLED
#include "app_bap_bc_scan_msg.h"

/// Values for PA Sync field in Broadcast Audio Scan Control Point characteristic value
enum app_bap_bc_pa_sync
{
    /// Do not synchronize to PA
    APP_BAP_BC_PA_SYNC_NO_SYNC = 0,
    /// Synchronize to PA, no PAST on client
    APP_BAP_BC_PA_SYNC_SYNC_NO_PAST,
    /// Synchronize to PA, PAST on client
    APP_BAP_BC_PA_SYNC_SYNC_PAST,

    APP_BAP_BC_PA_SYNC_MAX
};

/// Broadcast Sources information
typedef struct app_bc_srcs_info
{
    /// Connection local index
    uint8_t con_lid;
    /// Broadcast source number of specific delegator
    uint8_t nb_srcs;
    /// true: Assistant is scanning for delegator, else NOT
    bool is_deleg_scan;
} app_bc_srcs_info_t;

/// Content of Broadcast Assistant environment
typedef struct app_bap_bc_assist_env
{
    /// Preferred Mtu
    uint8_t preferred_mtu;
    /// Broadcast Sources Information
    app_bc_srcs_info_t bc_srcs[BLE_CONNECTION_MAX];
}app_bap_bc_assist_env_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APP_BLE_BIS_ASSIST_ENABLE

uint32_t app_bap_bc_assist_cmp_evt_handler(void const *param);
uint32_t app_bap_bc_assist_rsp_handler(void const *param);
uint32_t app_bap_bc_assist_req_ind_handler(void const *param);
uint32_t app_bap_bc_assist_ind_handler(void const *param);

void app_bap_bc_assist_init(void);
void app_bap_bc_assist_info_init(void);
void app_bap_bc_assist_start(uint8_t con_lid);
void app_bap_bc_assist_update_scan(uint8_t scan_state);
bool app_bap_bc_assist_is_deleg_scan(void);
void app_bap_bc_set_assist_is_deleg_scan(uint8_t con_lid, bool is_deleg_scan);
void app_bap_bc_assist_get_cfg_cmd(uint8_t con_lid, uint8_t src_lid);
void app_bap_bc_assist_set_cfg_cmd(uint8_t con_lid, uint8_t src_lid, uint8_t enable);
#endif

#ifdef __cplusplus
}
#endif

#endif
#endif // APP_BAP_ASSIST_MSG_H_

/// @} APP_BAP_ASSIST_MSG_H_

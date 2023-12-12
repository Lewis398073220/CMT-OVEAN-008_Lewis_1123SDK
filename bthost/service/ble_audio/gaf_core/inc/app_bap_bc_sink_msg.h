/**
 ****************************************************************************************
 *
 * @file app_bap_bc_sink_msg.h
 *
 * @brief BLE Audio Broadcast Sink
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
 * @addtogroup APP_BAP
 * @{
 ****************************************************************************************
 */

#ifndef APP_BAP_BC_SINK_MSG_H_
#define APP_BAP_BC_SINK_MSG_H_
#if BLE_AUDIO_ENABLED
#include "app_gaf.h"
#include "app_gaf_define.h"

#define APP_BAP_DFT_BC_SINK_DP_ID                  0
#define APP_BAP_DFT_BC_SINK_CTL_DELAY_US           0x0102
#define APP_BAP_DFT_BC_SINK_TIMEOUT_10MS           100
#define APP_BAP_DFT_BC_SINK_MSE                    5

/// Broadcast source group state
enum app_bap_bc_sink_state
{
    /// Broadcast group in idle state
    APP_BAP_BC_SINK_STATE_IDLE,
    /// Broadcast group in ENABLE state
    APP_BAP_BC_SINK_STATE_ENABLED,
    /// Broadcast group in stream state
    APP_BAP_BC_SINK_STATE_STREAMING,
};

typedef struct
{
    /// Group local index
    uint8_t   sink_state;
} app_bap_bc_sink_env_t;


#ifdef __cplusplus
extern "C" {
#endif

uint32_t app_bap_bc_sink_cmp_evt_handler(void const *param);
uint32_t app_bap_bc_sink_ind_handler(void const *param);
uint32_t app_bap_bc_sink_req_ind_handler(void const *param);
uint8_t app_bap_bc_sink_get_stream_pos(uint32_t stream_pos_bf);
uint8_t app_bap_bc_sink_get_stream_lid(uint32_t stream_pos_bf);
app_bap_bc_sink_env_t *app_bap_bc_sink_get_env();

void app_bap_bc_sink_init(void);

#ifdef __cplusplus
}
#endif


#endif
#endif // APP_BAP_BC_SINK_MSG_H_

/// @} APP_BAP

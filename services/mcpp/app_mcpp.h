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
#ifndef __APP_MCPP_API_H__
#define __APP_MCPP_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "app_mcpp_comm.h"

/**
 * Multi-Core PCM Process(MCPP) interface.
*/
int32_t app_mcpp_open(APP_MCPP_USER_T user, APP_MCPP_CFG_T *cfg);
int32_t app_mcpp_close(APP_MCPP_USER_T user);
int32_t app_mcpp_capture_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg);
int32_t app_mcpp_playback_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg);

/**
 * It is usually used to set algorithm config at one time.
*/
int32_t app_mcpp_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);
int32_t app_mcpp_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);

/**
 * It is usually used to set algorithm parameters or to return algorithm parameters.
 * If you want to return results asynchronously,
 * you need to register the callback function in advance.
 * Note: When the following two conditions are met, only synchronous returns are supported.
 * 1. core_client == core_server,
 * 2. frame_len == algo_frame_len.
*/
void app_mcpp_capture_async_ctl_cb_register(APP_MCPP_USER_T user, app_mcpp_async_handler_t handler);
void app_mcpp_playback_async_ctl_cb_register(APP_MCPP_USER_T user, app_mcpp_async_handler_t handler);

int32_t app_mcpp_capture_algo_ctl(APP_MCPP_USER_T user, int32_t ctl, void *ptr, uint16_t ptr_len, bool sync_flag);
int32_t app_mcpp_playback_algo_ctl(APP_MCPP_USER_T user, int32_t ctl, void *ptr, uint16_t ptr_len, bool sync_flag);

uint32_t app_mcpp_get_cap_channel_map(bool ref_flag, bool ref2_flag, bool vpu_flag);

#ifdef __cplusplus
}
#endif
#endif
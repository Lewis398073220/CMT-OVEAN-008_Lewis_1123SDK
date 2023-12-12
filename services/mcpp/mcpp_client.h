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
#ifndef __MCPP_CLIENT_H__
#define __MCPP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "app_mcpp_comm.h"

typedef struct {
    bool capture_enable;
    bool playback_enable;
    APP_MCPP_CORE_T capture_server;
    APP_MCPP_CORE_T playback_server;
} MCPP_CLI_INIT_CFG_T;

void mcpp_cli_lock_init(void);

typedef void(*mcpp_cli_result_process_handler_t)(APP_MCPP_USER_T user, uint8_t* ptr, uint16_t len);
void mcpp_cli_result_process_cb_register(mcpp_cli_result_process_handler_t handler);

int32_t mcpp_cli_init(APP_MCPP_USER_T user, MCPP_CLI_INIT_CFG_T *init_cfg);
int32_t mcpp_cli_wait_server_core_inited(APP_MCPP_USER_T user);
int32_t mcpp_cli_deinit(APP_MCPP_USER_T user);

int32_t mcpp_cli_stream_capture_open(APP_MCPP_USER_T user, void *info, uint16_t info_len);
int32_t mcpp_cli_stream_playback_open(APP_MCPP_USER_T user, void *info, uint16_t info_len);
int32_t mcpp_cli_stream_capture_close(APP_MCPP_USER_T user);
int32_t mcpp_cli_stream_playback_close(APP_MCPP_USER_T user);
int32_t mcpp_cli_send_capture_trigger(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg);
int32_t mcpp_cli_send_playback_trigger(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg);
int32_t mcpp_cli_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);
int32_t mcpp_cli_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);
int32_t mcpp_cli_capture_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len);
int32_t mcpp_cli_playback_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len);

#ifdef __cplusplus
}
#endif
#endif
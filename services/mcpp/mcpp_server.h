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
#ifndef __MCPP_SERVER_H__
#define __MCPP_SERVER_H__

#include <plat_types.h>
#include "app_mcpp_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

WEAK void mcpp_srv_stream_capture_open(APP_MCPP_USER_T user, void *info, uint16_t info_len);
WEAK void mcpp_srv_stream_playback_open(APP_MCPP_USER_T user, void *info, uint16_t info_len);

WEAK void mcpp_srv_stream_capture_close(APP_MCPP_USER_T user);
WEAK void mcpp_srv_stream_playback_close(APP_MCPP_USER_T user);

WEAK void mcpp_srv_capture_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg);
WEAK void mcpp_srv_playback_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg);

WEAK int32_t mcpp_srv_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);
WEAK int32_t mcpp_srv_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);

WEAK int32_t mcpp_srv_capture_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len);
WEAK int32_t mcpp_srv_playback_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len);

#ifdef __cplusplus
}
#endif

#endif

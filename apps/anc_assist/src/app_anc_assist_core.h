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
#ifndef __APP_ANC_ASSIST_CORE_H__
#define __APP_ANC_ASSIST_CORE_H__

#include "app_anc_assist.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_anc_assist_core_init(void);
void app_anc_assist_core_deinit(void);
void app_anc_assist_core_open( anc_assist_user_t user);
void app_anc_assist_core_close(anc_assist_user_t user);
void app_anc_assist_core_reset(anc_assist_user_t user);
void app_anc_assist_core_ctrl(anc_assist_user_t user, uint32_t ctrl, uint8_t *buf, uint32_t len);
void app_anc_assist_core_process(float *in_buf[],  uint32_t frame_len);
void app_anc_assist_core_get_result_via_fifo(void);

uint32_t app_anc_assist_core_get_status(void);
void app_anc_assist_core_suspend_users(bool en, uint32_t users);
bool app_anc_assist_core_user_is_suspend(anc_assist_user_t user);

#ifdef __cplusplus
}
#endif

#endif
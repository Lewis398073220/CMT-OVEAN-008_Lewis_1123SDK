/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#ifndef __APP_VOICE_ASSIST_FIR_LMS_H__
#define __APP_VOICE_ASSIST_FIR_LMS_H__

#define FIR_ANC_SYS_FREQ_LOG

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t app_voice_assist_fir_lms_init(void);
int32_t app_voice_assist_fir_lms_open(void);
int32_t app_voice_assist_fir_lms_reset(void);
int32_t app_voice_assist_fir_lms_close(void);
void app_voice_assist_fir_lms_update_Sz(float **Sz, uint32_t len);
void set_fir_cache(void);
void set_mc_fir_cache(void);

#ifdef __cplusplus
}
#endif

#endif
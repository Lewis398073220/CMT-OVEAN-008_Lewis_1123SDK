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
#ifndef __MCU_DSP_M55_H__
#define __MCU_DSP_M55_H__

typedef enum  {
    APP_DSP_M55_USER_INIT               = 0,
    APP_DSP_M55_USER_A2DP               = 1,
    APP_DSP_M55_USER_CALL               = 2,
    APP_DSP_M55_USER_VOICE_ASSIST       = 3,
    APP_DSP_M55_USER_DMA_AUDIO          = 4,
    APP_DSP_M55_USER_AUDIO_TEST         = 5,
    APP_DSP_M55_USER_AUDIO_DECODER      = 6,
    APP_DSP_M55_USER_AUDIO_ENCODER      = 7,
    APP_DSP_M55_USER_BIS                = 8,
    APP_DSP_M55_USER_QTY
} APP_DSP_M55_USER_E;

#ifdef __cplusplus
extern "C" {
#endif
void app_dsp_m55_first_init(void);
void app_dsp_m55_init(APP_DSP_M55_USER_E user);
void app_dsp_m55_deinit(APP_DSP_M55_USER_E user);
void app_dsp_m55_force_deinit(void);
bool app_dsp_m55_is_running(void);


#ifdef __cplusplus
}
#endif

#endif


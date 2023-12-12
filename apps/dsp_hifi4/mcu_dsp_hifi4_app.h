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
#ifndef __MCU_DSP_HIFI4_APP_H__
#define __MCU_DSP_HIFI4_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum  {
    APP_DSP_HIFI4_USER_INIT               = 0,
    APP_DSP_HIFI4_USER_MCPP               = 1,
    APP_DSP_HIFI4_USER_DMA_AUDIO          = 2,
    APP_DSP_HIFI4_USER_GAF_DSP            = 3,
    APP_DSP_HIFI4_USER_WALKIE_TALK        = 4,
    APP_DSP_HIFI4_USER_VOICE_ASSIST       = 5,
    APP_DSP_HIFI4_USER_QTY
} APP_DSP_HIFI4_USER_E;

typedef void (*app_dsp_hifi4_init_done_handler_t)(void);
void app_dsp_hifi4_register_init_done_handler(app_dsp_hifi4_init_done_handler_t Handler);

void app_dsp_hifi4_first_init(void);
int app_dsp_hifi4_init(APP_DSP_HIFI4_USER_E user);
int app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_E user);
bool app_dsp_hifi4_is_running(void);

#ifdef __cplusplus
}
#endif

#endif
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
#ifndef __APP_VOICE_ASSIST_DSP_H__
#define __APP_VOICE_ASSIST_DSP_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float **ff_mic;
    uint8_t ff_ch_num;
    float **fb_mic;
    uint8_t fb_ch_num;
    float **talk_mic;
    uint8_t talk_ch_num;
    float *vpu_mic;
    float **ref;
    uint8_t ref_ch_num;
    uint32_t frame_len;
} process_frame_data_t;

typedef int (*app_voice_assist_dsp_init_t)(void);
typedef int (*app_voice_assist_dsp_deinit_t)(void);
typedef int (*app_voice_assist_dsp_reset_t)(void);
typedef int (*app_voice_assist_dsp_get_freq_t)(void);
typedef int (*app_voice_assist_dsp_ctrl_t)(uint32_t ctrl, uint8_t *ptr, uint32_t ptr_len);
typedef int (*app_voice_assist_dsp_process_t)(process_frame_data_t *process_frame_data);
typedef int (*app_voice_assist_dsp_set_mode_t)(uint32_t mode);

typedef struct {
    app_voice_assist_dsp_init_t         open;
    app_voice_assist_dsp_deinit_t       close;
    app_voice_assist_dsp_reset_t        reset;
    app_voice_assist_dsp_ctrl_t         ctrl;
    app_voice_assist_dsp_process_t      process;
    app_voice_assist_dsp_get_freq_t     get_freq;
    app_voice_assist_dsp_set_mode_t     set_mode;
} app_voice_assist_dsp_t;

int32_t anc_assist_dsp_init();

#ifdef ANC_ALGO_DSP
int32_t assist_algo_dsp_init(void);
#endif

#ifdef FIR_ADAPT_ANC_M55
int32_t assist_fir_lms_dsp_init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
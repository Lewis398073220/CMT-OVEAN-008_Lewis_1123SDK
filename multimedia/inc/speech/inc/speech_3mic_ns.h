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
#ifndef __SPEECH_3MIC_NS_H__
#define __SPEECH_3MIC_NS_H__

#include <stdint.h>
#include "speech_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t     bypass;
    int32_t     wnr_enable;
    float       wind_thd;
    int32_t     wnd_pwr_thd;
    float       wind_gamma;
    int32_t     state_trans_frame_thd;

    int32_t     af_enable;
    float       filter_gamma;
    int32_t     vad_bin_start1;
    int32_t     vad_bin_end1;
    int32_t     vad_bin_start2;
    int32_t     vad_bin_end2;
    int32_t     vad_bin_start3;
    int32_t     vad_bin_end3;
    int32_t     vad_bin_start4;
    int32_t     vad_bin_end4;
    float       coef1_thd;
    float       coef2_thd;
    float       coef3_thd;
    float       coef4_thd;

    int32_t     calib_enable;
    int32_t     calib_delay;
    float*      filter;
    int32_t     filter_len;

    int32_t     low_ram_enable;
    int32_t     low_mips_enable;

    int32_t     echo_supp_enable;
    int32_t     ref_delay;

    int32_t     post_supp_enable;
    float       post_denoise_db;
    float       dnn_denoise_db;
    float       inner_denoise_db;
    int32_t     spectral_smooth_enable;

    // crossover
    int32_t     blend_en;

	// filter
	float       *ff_fb_coeff;
	int32_t     ff_fb_coeff_len;

    int32_t     comp_num;
    int32_t     comp_freq[MAX_COMP_NUM];
    int32_t     comp_gaindB[MAX_COMP_NUM];
} Speech3MicNsConfig;

struct Speech3MicNsState_;

typedef struct Speech3MicNsState_ Speech3MicNsState;

Speech3MicNsState *speech_3mic_ns_create(int32_t sample_rate, int32_t frame_size, const Speech3MicNsConfig *cfg);

int32_t speech_3mic_ns_destroy(Speech3MicNsState *st);

int32_t speech_3mic_ns_set_config(Speech3MicNsState *st, const Speech3MicNsConfig *cfg);

void speech_3mic_ns_preprocess(Speech3MicNsState *st, int16_t *pcm_buf, int16_t *ref_buf, int32_t pcm_len, int16_t *out_buf);

int32_t speech_3mic_ns_process(Speech3MicNsState *st, short *pcm_buf, short *ref_buf, int32_t pcm_len, short *out_buf);

float speech_3mic_get_required_mips(Speech3MicNsState *st);

void speech_3mic_ns_set_ns_state(Speech3MicNsState *st, void *ns);

void speech_3mic_ns_update_snr(Speech3MicNsState *st, float snr);

enum WIND_STATE_E speech_3mic_ns_get_wind_state(Speech3MicNsState *st);

#ifdef __cplusplus
}
#endif

#endif

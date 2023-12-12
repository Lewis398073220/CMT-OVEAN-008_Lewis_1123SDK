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


#ifndef __ANC_FF_FIR_LMS_H__
#define __ANC_FF_FIR_LMS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdint.h"
#include "iirfilt.h"
#include "custom_allocator.h"
#include "anc_process.h"

// #define WEAR_LEAK_STATUS_CHANGED_DETECT
#define LOCAL_FIR_LEN (472)

#define FIR_SAMPLE_RATE (32000)

#if FIR_SAMPLE_RATE == 48000
#define FIR_BLOCK_SIZE (360)
#elif FIR_SAMPLE_RATE == 32000
#define FIR_BLOCK_SIZE (240)
#else
#define FIR_BLOCK_SIZE (120)
#endif

typedef struct {
    enum IIR_BIQUARD_TYPE type;
    float       gain;
    int         freq;
    float       q;
    int         sample_rate;
}FIR_LMS_FILTER_CFG_T;

typedef struct {
    int32_t max_cnt;
    int32_t period_cnt;
    uint32_t ff_filt_num;

    int32_t mc_max_cnt;
    int32_t mc_period_cnt;

    uint32_t mc_filt_cnt;
    uint32_t ff_filt_cnt;
    uint32_t fb_filt_cnt;
    uint32_t fb2emic_filt_cnt;

    float mc_filt_ref_gain;
    float *ff_filt_ref_gain;
    float fb_filt_ref_gain;
    float fb2emic_filt_ref_gain;

    FIR_LMS_FILTER_CFG_T  *mc_filt_cfg;
    FIR_LMS_FILTER_CFG_T  *ff_filt_cfg;
    FIR_LMS_FILTER_CFG_T  *fb_filt_cfg;
    FIR_LMS_FILTER_CFG_T  *fb2emic_filt_cfg;

} ANC_FF_FIR_LMS_CFG_T;

typedef struct ANCFFFirLmsSt_ ANCFFFirLmsSt;

typedef int (*anc_set_fir_cfg_handler_t)(struct_anc_fir_cfg *cfg, enum ANC_TYPE_T cmd);
uint32_t anc_ff_fir_lms_register_anc_set_fir_cfg_handler(anc_set_fir_cfg_handler_t func);
ANCFFFirLmsSt * anc_ff_fir_lms_create(int32_t sample_rate, int32_t frame_len, ANC_FF_FIR_LMS_CFG_T * cfg, custom_allocator *allocator);
void anc_ff_fir_lms_destroy(ANCFFFirLmsSt* st);
int32_t anc_ff_fir_lms_reset(ANCFFFirLmsSt* st,int32_t status);

int32_t anc_ff_fir_lms_process(ANCFFFirLmsSt* st,float *ff, float *fb, float *talk, float *vpu, float *ref, int frame_len);
int32_t *fir_lms_coeff_cache(ANCFFFirLmsSt* st);
int32_t *get_local_fir_cache(ANCFFFirLmsSt* st);
int32_t *get_local_fir_cache2(ANCFFFirLmsSt* st);

int32_t anc_mc_fir_lms_reset(ANCFFFirLmsSt* st,int32_t status);
int32_t anc_mc_fir_lms_process(ANCFFFirLmsSt* st,float *ref, float *fb, int frame_len);
int32_t *mc_fir_lms_coeff_cache(ANCFFFirLmsSt* st);
int32_t *get_mc_local_fir_cache(ANCFFFirLmsSt* st);

void switch_virtual_filt(uint32_t index);

#if defined(WEAR_LEAK_STATUS_CHANGED_DETECT)
int32_t fir_anc_online_leak_detect_process(float * ff_mic, float * fb_mic, int frame_len);
#endif

#ifdef __cplusplus
}
#endif

#endif
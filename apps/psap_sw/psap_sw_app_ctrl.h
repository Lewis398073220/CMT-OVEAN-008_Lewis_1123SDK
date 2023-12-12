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
#ifndef __PSAP_SW_APP_CTRL_H__
#define __PSAP_SW_APP_CTRL_H__

#include "psap_sw_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BEAMFORMING_QTY             (2)
#define NOISE_REDUCTION_QTY         (5)

#ifdef PSAP_SW_THIRDPARTY
//Adding third-party functions, the following is only an example
int32_t psap_sw_app_set_gain(int32_t gain);
#else
int32_t psap_sw_set_total_gain(float *total_gain);
int32_t psap_sw_set_band_gain(float *band_gain);
int32_t psap_sw_set_beamforming(uint32_t enable);
int32_t psap_sw_set_noise_reduction(uint32_t level);
int32_t psap_sw_set_all_cfg(float *total_gain, float *band_gain, uint32_t bf_enable, uint32_t ns_level);
int32_t psap_sw_set_eq(psap_freq_eq_cfg_t *eq_l, psap_freq_eq_cfg_t *eq_r);
int32_t psap_sw_set_dehowling(psap_freq_dehowling_cfg *dehowling);
int32_t psap_sw_set_reverb(float *data);
#endif

#ifdef __cplusplus
}
#endif

#endif

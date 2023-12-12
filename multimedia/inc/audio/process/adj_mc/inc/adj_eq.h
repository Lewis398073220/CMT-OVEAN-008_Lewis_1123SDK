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

#ifndef __ADJ_EQ_H__
#define __ADJ_EQ_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "echo_canceller.h"

void adj_eq_init(uint32_t algo_frame_len, uint32_t a2dp_frame_len, uint32_t algo_sample_rate, uint32_t a2dp_sample_rate);
void adj_eq_deinit(void);
int adj_eq_run_mono(uint8_t *buf, int len, enum AUD_BITS_T sample_bit, uint32_t g_adj_eq);
uint32_t adj_eq_filter_estimate(int16_t *fb_ptr, int16_t *ref_ptr, uint32_t pcm_len);
void adj_eq_reset(void);

#ifdef __cplusplus
}
#endif

#endif

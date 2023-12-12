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
#ifndef __SIGNAL_GENERATOR_H__
#define __SIGNAL_GENERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum  {
    SG_TYPE_TONE_100Hz = 100,
    SG_TYPE_TONE_500Hz = 500,
    SG_TYPE_TONE_1KHz = 1000,
    SG_TYPE_TONE_MAX = 96000,
    // TODO: Add more non-tone signal. e.g. White Noise
} SG_TYPE_T;

void signal_generator_init(SG_TYPE_T type, uint32_t sample_rate, uint32_t bits, uint32_t channel_num, float gain_dB);
void signal_generator_deinit(void);
void signal_generator_loop_get_data(void *pcm_buf, uint32_t frame_len);

#ifdef __cplusplus
}
#endif

#endif
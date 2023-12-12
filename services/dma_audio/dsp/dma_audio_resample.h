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
#ifndef __DMA_AUDIO_RESAMPLE_H__
#define __DMA_AUDIO_RESAMPLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dma_audio_public.h"

#define MAX_CHANNEL_NUMBER (6)

struct DAUD_RESAMPLE_STATUS_T {
    uint8_t open   : 1;
    uint8_t enable : 1;
    uint8_t rsvd   : 6;
};

struct DAUD_RESAMPLE_T {
    uint8_t *rs_buf;
    uint32_t rs_buf_size;
    uint8_t *rs_data_in_buf;
    uint32_t rs_data_in_size;
    uint8_t *rs_data_out_buf;
    uint32_t rs_data_out_size;
    uint32_t rs_time_us;
    uint32_t rs_bits;
    uint32_t rs_chan_num;
    int ratio_step;
    float *rs_coeffs;
    float *rs_state[MAX_CHANNEL_NUMBER];
    struct DAUD_RESAMPLE_STATUS_T status;
};

void dma_audio_resample_init(struct DAUD_RESAMPLE_T *daud_resample);

void dma_audio_resample_dump(struct DAUD_RESAMPLE_T *daud_resample);

int dma_audio_resample_open(
    struct DAUD_RESAMPLE_T *daud_resample,
    struct DAUD_STREAM_CONFIG_T *out_cfg,
    struct DAUD_STREAM_CONFIG_T *in_cfg);

int dma_audio_resample_run(struct DAUD_RESAMPLE_T *daud_resample);

int dma_audio_resample_enable(struct DAUD_RESAMPLE_T *daud_resample);

int dma_audio_resample_disable(struct DAUD_RESAMPLE_T *daud_resample);

int dma_audio_resample_reset(struct DAUD_RESAMPLE_T *daud_resample);

int dma_audio_resample_close(struct DAUD_RESAMPLE_T *daud_resample);

#ifdef __cplusplus
}
#endif

#endif

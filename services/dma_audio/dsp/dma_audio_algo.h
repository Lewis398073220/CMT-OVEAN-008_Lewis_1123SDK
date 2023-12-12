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
#ifndef __DMA_AUDIO_ALGO_H__
#define __DMA_AUDIO_ALGO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_aud.h"

typedef void (*DAUD_ALGO_INIT_CALLBACK_T)(bool init);

typedef int (*DAUD_ALGO_CFG_CALLBACK_T)(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    void *cfg, uint32_t len, bool set);

typedef int (*DAUD_ALGO_PROC_CALLBACK_T)(uint8_t *dst, uint32_t dst_len, uint8_t *src, uint32_t src_len);

void dma_audio_algo_setup_init_callback(DAUD_ALGO_INIT_CALLBACK_T callback);

void dma_audio_algo_setup_config_callback(DAUD_ALGO_CFG_CALLBACK_T callback);

void dma_audio_algo_setup_algo_proc_callback(DAUD_ALGO_PROC_CALLBACK_T callback);

void dma_audio_algo_init(struct DAUD_STREAM_CONFIG_T *cap_cfg, struct DAUD_STREAM_CONFIG_T *play_cfg);

void dma_audio_algo_deinit(void);

int dma_audio_algo_process(uint8_t *dst, uint32_t dst_len, uint8_t *src, uint32_t src_len);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_AUDIO_ALGO_H__ */



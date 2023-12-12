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
#ifndef __DMA_AUDIO_HOST_H__
#define __DMA_AUDIO_HOST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "dma_audio_public.h"
#include "dma_audio_test.h"

enum DMA_AUD_RPCIF_USER_T {
    DMA_AUD_RPCIF_USER_APP,
    DMA_AUD_RPCIF_USER_TEST,
};

enum DMA_AUD_CB_ID_T {
    DMA_AUD_CB_ID_0,
    DMA_AUD_CB_ID_1,

    DMA_AUD_CB_ID_QTY
};

typedef void (*DMA_AUD_STATE_CB_T)(bool state);

void dma_audio_init(void);

void dma_audio_rpcif_open(enum DMA_AUD_RPCIF_USER_T user);

void dma_audio_rpcif_close(enum DMA_AUD_RPCIF_USER_T user);

void dma_audio_on(bool on, struct DAUD_STREAM_CONFIG_T *cap_cfg, struct DAUD_STREAM_CONFIG_T *play_cfg);

bool dma_audio_started(void);

void dma_audio_request_clock(void);

void dma_audio_release_clock(void);

void dma_audio_setup_pre_state_switch_callback(enum DMA_AUD_CB_ID_T id, DMA_AUD_STATE_CB_T callback);

void dma_audio_setup_post_state_switch_callback(enum DMA_AUD_CB_ID_T id, DMA_AUD_STATE_CB_T callback);

unsigned int dma_audio_rx_data_handler(const void *data, unsigned int len);

void dma_audio_tx_data_handler(const void *data, unsigned int len);

int32_t dma_audio_stream_set_fade_time(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif

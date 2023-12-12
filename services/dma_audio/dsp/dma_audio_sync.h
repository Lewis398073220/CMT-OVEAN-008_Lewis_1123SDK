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
#ifndef __DMA_AUDIO_SYNC_H__
#define __DMA_AUDIO_SYNC_H__
#include "hal_aud.h"

struct AUD_STREAM_SYNC_CONFIG_T {
    uint32_t sample_rate;
    uint32_t bits;
    uint32_t channel_num;
    uint8_t *data_ptr;
    uint32_t data_size;
    enum AUD_STREAM_ID_T pair_id;
    enum AUD_STREAM_T pair_stream;
};

int dma_audio_stream_sync_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct AUD_STREAM_SYNC_CONFIG_T *cfg);

int dma_audio_stream_sync_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

int dma_audio_stream_sync_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

int dma_audio_stream_sync_reset(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

int dma_audio_stream_sync_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

uint32_t dma_audio_stream_sync_callback(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

void dma_audio_stream_sync_sw_trig_mode(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, bool enable);

#endif /* __DMA_AUDIO_SYNC_H__ */

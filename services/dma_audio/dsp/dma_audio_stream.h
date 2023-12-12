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
#ifndef __DMA_AUDIO_STREAM_H__
#define __DMA_AUDIO_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dma_audio_public.h"

typedef uint32_t (*DMA_AUD_MIC_DATA_HDLR_T)(uint8_t *buf, uint32_t len);
typedef uint32_t (*DMA_AUD_SPK_DATA_HDLR_T)(uint8_t *buf, uint32_t len);

void dma_audio_stream_init(void);

void dma_audio_stream_deinit(void);

void dma_audio_stream_on(bool on);

void dma_audio_stream_request_clock(void);

void dma_audio_stream_release_clock(void);

void dma_audio_stream_setup_mic_data_handler(DMA_AUD_MIC_DATA_HDLR_T handler);

void dma_audio_stream_setup_spk_data_handler(DMA_AUD_SPK_DATA_HDLR_T handler);

void dma_audio_stream_set_config(enum AUD_STREAM_T stream, struct DAUD_STREAM_CONFIG_T *cfg);

void dma_audio_stream_get_config(enum AUD_STREAM_T stream, struct DAUD_STREAM_CONFIG_T *cfg);

void dma_audio_stream_irq_notify(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_AUDIO_APP_H__ */

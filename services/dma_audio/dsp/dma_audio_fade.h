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
#ifndef _DMA_AUDIO_FADE_H__
#define _DMA_AUDIO_FADE_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t dma_audio_fade_init(const struct DAUD_STREAM_CONFIG_T *cfg);

int32_t dma_audio_fade_deinit(void);

int32_t dma_audio_set_fadein_time(uint32_t ms);

int32_t dma_audio_set_fadeout_time(uint32_t ms);

int32_t dma_audio_fade_process(uint32_t ch_num, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
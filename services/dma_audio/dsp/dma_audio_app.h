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
#ifndef __DMA_AUDIO_APP_H__
#define __DMA_AUDIO_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

void dma_audio_app_init(bool init);

void dma_audio_app_on(bool on);

void dma_audio_app_thread(void);

bool dma_audio_app_started(void);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_AUDIO_APP_H__ */

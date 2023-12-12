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
#ifndef __DMA_AUDIO_PUBLIC_H__
#define __DMA_AUDIO_PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_aud.h"

typedef uint32_t (*DAUD_STREAM_HANDLER_T)(uint8_t *buf, uint32_t len);

/* dma audio stream config */
struct DAUD_STREAM_CONFIG_T {
    enum AUD_SAMPRATE_T sample_rate;
    enum AUD_BITS_T bits;
    enum AUD_CHANNEL_NUM_T channel_num;
    enum AUD_CHANNEL_MAP_T channel_map;
    enum AUD_STREAM_USE_DEVICE_T device;
    enum AUD_IO_PATH_T io_path;
    enum AUD_DATA_ALIGN_T align;
    enum AUD_FS_FIRST_EDGE_T fs_edge;
    uint16_t fs_cycles;
    uint8_t slot_cycles;
    bool sync_start;
    uint32_t vol;
    bool irq_mode;
    uint8_t *data_ptr;
    uint32_t data_size;
    DAUD_STREAM_HANDLER_T handler;
};

#ifdef __cplusplus
}
#endif

#endif


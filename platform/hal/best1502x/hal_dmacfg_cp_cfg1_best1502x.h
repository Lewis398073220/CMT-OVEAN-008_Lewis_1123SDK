/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#ifndef __HAL_DMACFG_CP_CFG1_BEST1502X_H__
#define __HAL_DMACFG_CP_CFG1_BEST1502X_H__

#include "plat_addr_map.h"

#define AUDMA_PERIPH_NUM                        16

#define AUDMA_CHAN_NUM                          8

#define AUDMA_CHAN_START                        (0)

static const uint32_t audma_fifo_addr[AUDMA_PERIPH_NUM] = {
    CODEC_BASE + 0x01C,     // CODEC RX
    0,//CODEC_BASE + 0x01C,     // CODEC TX
    0,//BTPCM_BASE + 0x1C0,     // BTPCM RX
    0,//BTPCM_BASE + 0x1C8,     // BTPCM TX
    I2S0_BASE + 0x200,      // I2S0 RX
    I2S0_BASE + 0x240,      // I2S0 TX
    0,                      // FIR RX
    0,                      // FIR TX
    0,//SPDIF0_BASE + 0x1C0,    // SPDIF0 RX
    0,//SPDIF0_BASE + 0x1C8,    // SPDIF0 TX
    0,//CODEC_BASE + 0x03C,     // IIR RX
    CODEC_BASE + 0x03C,     // CODEC TX2
    0,//BTDUMP_BASE + 0x34,     // BTDUMP
    0,//CODEC_BASE + 0x038,     // MC RX
    0,//I2S1_BASE + 0x200,      // I2S1 RX
    0,//I2S1_BASE + 0x240,      // I2S1 TX
};

static const enum HAL_DMA_PERIPH_T audma_fifo_periph[AUDMA_PERIPH_NUM] = {
    HAL_AUDMA_CODEC_RX,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_AUDMA_I2S0_RX,
    HAL_AUDMA_I2S0_TX,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_AUDMA_CODEC_TX2,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
    HAL_DMA_PERIPH_NULL,
};

#endif


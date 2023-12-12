/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef __HAL_DMACFG_SYS_CFG1_BEST1502X_H__
#define __HAL_DMACFG_SYS_CFG1_BEST1502X_H__

#include "plat_addr_map.h"

#define AUDMA_PERIPH_NUM                        16

#define AUDMA_CHAN_NUM                          8

#define AUDMA_CHAN_START                        (0)

static const uint32_t audma_fifo_addr[AUDMA_PERIPH_NUM] = {
    I2S0_BASE + 0x200,      // I2S0 RX
    I2S0_BASE + 0x240,      // I2S0 TX
    I2C0_BASE + 0x010,      // I2C0 RX
    I2C0_BASE + 0x010,      // I2C0 TX
#ifdef CP_SUBSYS_ADC1_DMA_ENABLE
    SPI_BASE + 0x008,       // SPI RX
#else
    CODEC_BASE + 0x01C,     // CODEC RX
#endif
    CODEC_BASE + 0x03C,     // CODEC TX2
    CODEC_BASE + 0x01C,     // CODEC TX
    BTDUMP_BASE + 0x34,     // BTDUMP
    UART0_BASE + 0x000,     // UART0 RX
    UART0_BASE + 0x000,     // UART0 TX
    UART1_BASE + 0x000,     // UART1 RX
    UART1_BASE + 0x000,     // UART1 TX
    I2C1_BASE + 0x010,      // I2C1 RX
    I2C1_BASE + 0x010,      // I2C1 TX
    BTPCM_BASE + 0x1C0,     // BTPCM RX
    BTPCM_BASE + 0x1C8,     // BTPCM TX
};

static const enum HAL_DMA_PERIPH_T audma_fifo_periph[AUDMA_PERIPH_NUM] = {
    HAL_AUDMA_I2S0_RX,
    HAL_AUDMA_I2S0_TX,
    HAL_GPDMA_I2C0_RX,
    HAL_GPDMA_I2C0_TX,
#ifdef CP_SUBSYS_ADC1_DMA_ENABLE
    HAL_GPDMA_SPI_RX,
#else
    HAL_AUDMA_CODEC_RX,
#endif
    HAL_AUDMA_CODEC_TX2,
    HAL_AUDMA_CODEC_TX,
    HAL_AUDMA_BTDUMP,
    HAL_GPDMA_UART0_RX,
    HAL_GPDMA_UART0_TX,
    HAL_GPDMA_UART1_RX,
    HAL_GPDMA_UART1_TX,
    HAL_GPDMA_I2C1_RX,
    HAL_GPDMA_I2C1_TX,
    HAL_AUDMA_BTPCM_RX,
    HAL_AUDMA_BTPCM_TX,
};

#endif


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
#ifndef __HAL_DMACFG_SENS_BEST1502X_H__
#define __HAL_DMACFG_SENS_BEST1502X_H__

#define AUDMA_PERIPH_NUM                        16

#define AUDMA_CHAN_NUM                          8

#define AUDMA_CHAN_START                        (0)

static const uint32_t audma_fifo_addr[AUDMA_PERIPH_NUM] = {
    I2C0_BASE + 0x010,      // I2C0 RX
    I2C0_BASE + 0x010,      // I2C0 TX
    I2C1_BASE + 0x010,      // I2C1 RX
    I2C1_BASE + 0x010,      // I2C1 TX
    I2C2_BASE + 0x010,      // I2C2 RX
    I2C2_BASE + 0x010,      // I2C2 TX
    SPI_BASE + 0x008,       // SPI RX
    SPI_BASE + 0x008,       // SPI TX
    SPILCD_BASE + 0x008,    // SPILCD RX
    SPILCD_BASE + 0x008,    // SPILCD TX
#ifdef SENS_I2S_DMA_ENABLE
    I2S0_BASE + 0x200,      // I2S0 RX
    I2S0_BASE + 0x240,      // I2S0 TX
#else
    I2C3_BASE + 0x010,      // I2C3 RX
    I2C3_BASE + 0x010,      // I2C3 TX
#endif
    SENS_VAD_BASE + 0x01C,  // VAD RX
#ifdef SENS_CAP_SENS_DMA_ENABLE
    SENS_CAP_SENSOR_BASE + 0x06C, // CAP_SENS RX
#else
    UART0_BASE + 0x000,     // UART0 TX
#endif
    UART1_BASE + 0x000,     // UART1 RX
    UART1_BASE + 0x000,     // UART1 TX
};

static const enum HAL_DMA_PERIPH_T audma_fifo_periph[AUDMA_PERIPH_NUM] = {
    HAL_GPDMA_I2C0_RX,
    HAL_GPDMA_I2C0_TX,
    HAL_GPDMA_I2C1_RX,
    HAL_GPDMA_I2C1_TX,
    HAL_GPDMA_I2C2_RX,
    HAL_GPDMA_I2C2_TX,
    HAL_GPDMA_SPI_RX,
    HAL_GPDMA_SPI_TX,
    HAL_GPDMA_SPILCD_RX,
    HAL_GPDMA_SPILCD_TX,
#ifdef SENS_I2S_DMA_ENABLE
    HAL_AUDMA_I2S0_RX,
    HAL_AUDMA_I2S0_TX,
#else
    HAL_GPDMA_I2C3_RX,
    HAL_GPDMA_I2C3_TX,
#endif
    HAL_AUDMA_CODEC_RX,
#ifdef SENS_CAP_SENS_DMA_ENABLE
    HAL_AUDMA_CAP_SENS,
#else
    HAL_GPDMA_UART0_TX,
#endif
    HAL_GPDMA_UART1_RX,
    HAL_GPDMA_UART1_TX,
};

#endif


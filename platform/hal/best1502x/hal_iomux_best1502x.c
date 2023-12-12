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
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_iomux)
#include "cmsis.h"
#include "hal_chipid.h"
#include "hal_gpio.h"
#include "hal_iomux.h"
#include "hal_location.h"
#include "hal_memsc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "pmu.h"
#include "tgt_hardware.h"

#define MEMSC_ID_IOMUX                      HAL_MEMSC_AON_ID_0

#define UART2_VOLTAGE_SEL                   HAL_IOMUX_PIN_VOLTAGE_MEM

#ifdef I2S0_VOLTAGE_VMEM
#define I2S0_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2S0_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef I2S1_VOLTAGE_VMEM
#define I2S1_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2S1_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SPDIF0_VOLTAGE_VMEM
#define SPDIF0_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SPDIF0_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef DIGMIC_VOLTAGE_VMEM
#define DIGMIC_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define DIGMIC_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SPI_VOLTAGE_VMEM
#define SPI_VOLTAGE_SEL                     HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SPI_VOLTAGE_SEL                     HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SPILCD_VOLTAGE_VMEM
#define SPILCD_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SPILCD_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef I2C0_VOLTAGE_VMEM
#define I2C0_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2C0_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef I2C1_VOLTAGE_VMEM
#define I2C1_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2C1_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef I2C2_VOLTAGE_VMEM
#define I2C2_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2C2_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef I2C3_VOLTAGE_VMEM
#define I2C3_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2C3_VOLTAGE_SEL                    HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef CLKOUT_VOLTAGE_VMEM
#define CLKOUT_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define CLKOUT_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SDMMC0_VOLTAGE_VMEM
#define SDMMC0_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SDMMC0_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SDMMC0_PULLUP_ENABLE
#define SDMMC0_PULLUP_SEL                   HAL_IOMUX_PIN_PULLUP_ENABLE
#else
#define SDMMC0_PULLUP_SEL                   HAL_IOMUX_PIN_NOPULL
#endif

#ifdef SDMMC0_CLEAR_WITH_NOPULL
#define SDMMC0_CLEAR_RES_SEL                HAL_IOMUX_PIN_NOPULL
#else
#define SDMMC0_CLEAR_RES_SEL                HAL_IOMUX_PIN_PULLDOWN_ENABLE
#endif

#ifdef SDMMC1_VOLTAGE_VMEM
#define SDMMC1_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SDMMC1_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SDMMC1_PULLUP_ENABLE
#define SDMMC1_PULLUP_SEL                   HAL_IOMUX_PIN_PULLUP_ENABLE
#else
#define SDMMC1_PULLUP_SEL                   HAL_IOMUX_PIN_NOPULL
#endif

#ifdef SDMMC1_CLEAR_WITH_NOPULL
#define SDMMC1_CLEAR_RES_SEL                HAL_IOMUX_PIN_NOPULL
#else
#define SDMMC1_CLEAR_RES_SEL                HAL_IOMUX_PIN_PULLDOWN_ENABLE
#endif

#ifndef SDMMC0_IOMUX_INDEX
#define SDMMC0_IOMUX_INDEX                  0
#endif

#ifndef SDMMC1_IOMUX_INDEX
#define SDMMC1_IOMUX_INDEX                  0
#endif

#ifndef I2S0_IOMUX_INDEX
#define I2S0_IOMUX_INDEX                    0
#endif

#ifndef I2S1_IOMUX_INDEX
#define I2S1_IOMUX_INDEX                    0
#endif

#ifndef I2S_MCLK_IOMUX_INDEX
#define I2S_MCLK_IOMUX_INDEX                0
#endif

#ifndef SPDIF0_IOMUX_INDEX
#define SPDIF0_IOMUX_INDEX                  0
#endif

#ifndef DIG_MIC2_CK_IOMUX_INDEX
#define DIG_MIC2_CK_IOMUX_INDEX             0
#endif

#ifndef DIG_MIC3_CK_IOMUX_INDEX
#define DIG_MIC3_CK_IOMUX_INDEX             0
#endif

#ifndef DIG_MIC_CK_IOMUX_PIN
#define DIG_MIC_CK_IOMUX_PIN                0
#endif

#ifndef DIG_MIC_D0_IOMUX_PIN
#define DIG_MIC_D0_IOMUX_PIN                1
#endif

#ifndef DIG_MIC_D1_IOMUX_PIN
#define DIG_MIC_D1_IOMUX_PIN                2
#endif

#ifndef DIG_MIC_D2_IOMUX_PIN
#define DIG_MIC_D2_IOMUX_PIN                3
#endif

#ifndef SPI_IOMUX_INDEX
#define SPI_IOMUX_INDEX                     0
#endif

#ifndef SPI_IOMUX_DI0_INDEX
#define SPI_IOMUX_DI0_INDEX                 0
#endif

#ifndef SPILCD_IOMUX_INDEX
#define SPILCD_IOMUX_INDEX                  0
#endif

#ifndef I2C0_IOMUX_INDEX
#define I2C0_IOMUX_INDEX                    0
#endif

#ifndef I2C1_IOMUX_INDEX
#define I2C1_IOMUX_INDEX                    0
#endif

#ifndef I2C2_IOMUX_INDEX
#define I2C2_IOMUX_INDEX                    0
#endif

#ifndef I2C3_IOMUX_INDEX
#define I2C3_IOMUX_INDEX                    0
#endif

#ifndef CLKOUT_IOMUX_INDEX
#define CLKOUT_IOMUX_INDEX                  0
#endif

#ifndef JTAG_IOMUX_INDEX
#define JTAG_IOMUX_INDEX                    0
#endif

#ifndef RXON_IOMUX_INDEX
#define RXON_IOMUX_INDEX                    0
#endif

#ifndef TXON_IOMUX_INDEX
#define TXON_IOMUX_INDEX                    0
#endif

#ifndef PWM0_IOMUX_INDEX
#define PWM0_IOMUX_INDEX                    30
#endif

#ifndef PWM1_IOMUX_INDEX
#define PWM1_IOMUX_INDEX                    31
#endif

#ifndef PWM2_IOMUX_INDEX
#define PWM2_IOMUX_INDEX                    22
#endif

#ifndef PWM3_IOMUX_INDEX
#define PWM3_IOMUX_INDEX                    33
#endif

#ifndef PWM4_IOMUX_INDEX
#define PWM4_IOMUX_INDEX                    34
#endif

#ifndef PWM5_IOMUX_INDEX
#define PWM5_IOMUX_INDEX                    35
#endif

#ifndef PWM6_IOMUX_INDEX
#define PWM6_IOMUX_INDEX                    36
#endif

#ifndef PWM7_IOMUX_INDEX
#define PWM7_IOMUX_INDEX                    37
#endif

#define IOMUX_FUNC_VAL_GPIO                 0

#define IOMUX_ALT_FUNC_NUM                  7

// Other func values: 0 -> gpio, 5 -> bt, 6 -> sdmmc, 7 -> ana_test
static const uint8_t index_to_func_val[IOMUX_ALT_FUNC_NUM] = { 1, 2, 3, 4, 5, 6, 7};

static const enum HAL_IOMUX_FUNCTION_T pin_func_map[HAL_IOMUX_PIN_NUM][IOMUX_ALT_FUNC_NUM] = {
    // P0_0
    { HAL_IOMUX_FUNC_SENS_I2S0_SCK, HAL_IOMUX_FUNC_MCU_I2S0_SCK, HAL_IOMUX_FUNC_MCU_I2C_M0_SCL, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_1
    { HAL_IOMUX_FUNC_SENS_I2S0_WS, HAL_IOMUX_FUNC_MCU_I2S0_WS, HAL_IOMUX_FUNC_MCU_I2C_M0_SDA, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_2
    { HAL_IOMUX_FUNC_SENS_I2S0_SDI0, HAL_IOMUX_FUNC_MCU_I2S0_SDI0, HAL_IOMUX_FUNC_MCU_I2C_M1_SCL, HAL_IOMUX_FUNC_PDM1_D, HAL_IOMUX_FUNC_SENS_UART0_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_3
    { HAL_IOMUX_FUNC_SENS_I2S0_SDO, HAL_IOMUX_FUNC_MCU_I2S0_SDO, HAL_IOMUX_FUNC_MCU_I2C_M1_SDA, HAL_IOMUX_FUNC_PDM2_D, HAL_IOMUX_FUNC_SENS_UART0_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_4
    { HAL_IOMUX_FUNC_SENS_I2C_M0_SCL, HAL_IOMUX_FUNC_MCU_I2C_M0_SCL, HAL_IOMUX_FUNC_SPDIF0_DI, HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_5
    { HAL_IOMUX_FUNC_SENS_I2C_M0_SDA, HAL_IOMUX_FUNC_MCU_I2C_M0_SDA, HAL_IOMUX_FUNC_SPDIF0_DO, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_6
    { HAL_IOMUX_FUNC_SENS_I2C_M1_SCL, HAL_IOMUX_FUNC_MCU_I2S1_SCK, HAL_IOMUX_FUNC_CLK_REQ_IN, HAL_IOMUX_FUNC_SENS_UART0_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P0_7
    { HAL_IOMUX_FUNC_SENS_I2C_M1_SDA, HAL_IOMUX_FUNC_MCU_I2S1_WS, HAL_IOMUX_FUNC_CLK_REQ_OUT, HAL_IOMUX_FUNC_SENS_UART0_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P1_0
    { HAL_IOMUX_FUNC_SENS_UART1_RX, HAL_IOMUX_FUNC_MCU_I2C_M1_SCL, HAL_IOMUX_FUNC_MCU_UART2_RX, HAL_IOMUX_FUNC_BT_UART_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_UART1_RX},
    // P1_1
    { HAL_IOMUX_FUNC_SENS_UART1_TX, HAL_IOMUX_FUNC_MCU_I2C_M1_SDA, HAL_IOMUX_FUNC_MCU_UART2_TX, HAL_IOMUX_FUNC_BT_UART_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_UART1_TX},
    // P1_2
    { HAL_IOMUX_FUNC_SENS_UART1_CTS, HAL_IOMUX_FUNC_MCU_I2S1_SDI0, HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_QSPI_NAND_CLK, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_SDMMC1_CLK, HAL_IOMUX_FUNC_MCU_UART1_CTS},
    // P1_3
    { HAL_IOMUX_FUNC_SENS_UART1_RTS, HAL_IOMUX_FUNC_MCU_I2S1_SDO, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_FUNC_QSPI_NAND_CS, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_SDMMC1_CMD, HAL_IOMUX_FUNC_MCU_UART1_RTS},
    // P1_4
    { HAL_IOMUX_FUNC_SENS_SPI_CLK, HAL_IOMUX_FUNC_MCU_SPI_CLK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D0, HAL_IOMUX_FUNC_PDM1_CK, HAL_IOMUX_FUNC_SDMMC1_DATA0, HAL_IOMUX_FUNC_NONE},
    // P1_5
    { HAL_IOMUX_FUNC_SENS_SPI_CS0, HAL_IOMUX_FUNC_MCU_SPI_CS0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D1, HAL_IOMUX_FUNC_PDM1_D, HAL_IOMUX_FUNC_SDMMC1_DATA1, HAL_IOMUX_FUNC_NONE},
    // P1_6
    { HAL_IOMUX_FUNC_SENS_SPI_DI0, HAL_IOMUX_FUNC_MCU_SPI_DI0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D2, HAL_IOMUX_FUNC_PDM2_CK, HAL_IOMUX_FUNC_SDMMC1_DATA2, HAL_IOMUX_FUNC_NONE},
    // P1_7
    { HAL_IOMUX_FUNC_SENS_SPI_DIO, HAL_IOMUX_FUNC_MCU_SPI_DIO, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D3, HAL_IOMUX_FUNC_PDM2_D, HAL_IOMUX_FUNC_SDMMC1_DATA3, HAL_IOMUX_FUNC_NONE},
    // P2_0
    { HAL_IOMUX_FUNC_SENS_I2C_M2_SCL, HAL_IOMUX_FUNC_MCU_UART1_RX, HAL_IOMUX_FUNC_MCU_UART1_CTS, HAL_IOMUX_FUNC_BT_UART_CTS, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_1
    { HAL_IOMUX_FUNC_SENS_I2C_M2_SDA, HAL_IOMUX_FUNC_MCU_UART1_TX, HAL_IOMUX_FUNC_MCU_UART1_RTS, HAL_IOMUX_FUNC_BT_UART_RTS, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_2
    { HAL_IOMUX_FUNC_SENS_UART0_RX, HAL_IOMUX_FUNC_MCU_UART0_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_BT_UART_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_3
    { HAL_IOMUX_FUNC_SENS_UART0_TX, HAL_IOMUX_FUNC_MCU_UART0_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_BT_UART_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_4
    { HAL_IOMUX_FUNC_SENS_SPILCD_CLK, HAL_IOMUX_FUNC_MCU_SPI_CLK, HAL_IOMUX_FUNC_PCM_CLK, HAL_IOMUX_FUNC_PDM2_CK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_5
    { HAL_IOMUX_FUNC_SENS_SPILCD_CS0, HAL_IOMUX_FUNC_MCU_SPI_CS0, HAL_IOMUX_FUNC_PCM_FSYNC, HAL_IOMUX_FUNC_PDM2_D, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_6
    { HAL_IOMUX_FUNC_SENS_SPILCD_DI0, HAL_IOMUX_FUNC_MCU_SPI_DI0, HAL_IOMUX_FUNC_PCM_DI, HAL_IOMUX_FUNC_PDM1_CK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P2_7
    { HAL_IOMUX_FUNC_SENS_SPILCD_DIO, HAL_IOMUX_FUNC_MCU_SPI_DIO, HAL_IOMUX_FUNC_PCM_DO, HAL_IOMUX_FUNC_PDM1_D, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P3_0
    { HAL_IOMUX_FUNC_SENS_SPI_CLK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PDM2_CK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_FUNC_NONE},
    // P3_1
    { HAL_IOMUX_FUNC_SENS_SPI_CS0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PDM2_D, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_FUNC_NONE},
    // P3_2
    { HAL_IOMUX_FUNC_SENS_SPI_DI0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_FUNC_NONE},
    // P3_3
    { HAL_IOMUX_FUNC_SENS_SPI_DIO, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_FUNC_NONE},
    // P3_4
    { HAL_IOMUX_FUNC_SENS_I2C_M2_SCL, HAL_IOMUX_FUNC_MCU_I2C_M0_SDA, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_FUNC_NONE},
    // P3_5
    { HAL_IOMUX_FUNC_SENS_I2C_M2_SDA, HAL_IOMUX_FUNC_MCU_I2C_M0_SCL, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_FUNC_NONE},
    // P3_6
    { HAL_IOMUX_FUNC_SENS_I2C_M3_SCL, HAL_IOMUX_FUNC_MCU_SPI_CS1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PDM1_CK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_FUNC_SDMMC1_CLK},
    // P3_7
    { HAL_IOMUX_FUNC_SENS_I2C_M3_SDA, HAL_IOMUX_FUNC_MCU_SPI_DI1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PDM1_D, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_FUNC_SDMMC1_CMD},
    // P4_0
    { HAL_IOMUX_FUNC_MCU_I2S0_SCK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC1_DATA0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_CLK},
    // P4_1
    { HAL_IOMUX_FUNC_MCU_I2S0_WS, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC1_DATA1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_CS},
    // P4_2
    { HAL_IOMUX_FUNC_MCU_I2S0_SDI0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC1_DATA2, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D0},
    // P4_3
    { HAL_IOMUX_FUNC_MCU_I2S0_SDO, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC1_DATA3, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D1},
    // P4_4
    { HAL_IOMUX_FUNC_MCU_I2C_M2_SCL, HAL_IOMUX_FUNC_MCU_I2S1_SCK, HAL_IOMUX_FUNC_SDMMC1_DATA4, HAL_IOMUX_FUNC_PCM_CLK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D2},
    // P4_5
    { HAL_IOMUX_FUNC_MCU_I2C_M2_SDA, HAL_IOMUX_FUNC_MCU_I2S1_WS, HAL_IOMUX_FUNC_SDMMC1_DATA5, HAL_IOMUX_FUNC_PCM_FSYNC, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPI_NAND_D3},
    // P4_6
    { HAL_IOMUX_FUNC_MCU_I2C_M3_SCL, HAL_IOMUX_FUNC_MCU_I2S1_SDI0, HAL_IOMUX_FUNC_SDMMC1_DATA6, HAL_IOMUX_FUNC_PCM_DI, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P4_7
    { HAL_IOMUX_FUNC_MCU_I2C_M3_SDA, HAL_IOMUX_FUNC_MCU_I2S1_SDO, HAL_IOMUX_FUNC_SDMMC1_DATA7, HAL_IOMUX_FUNC_PCM_DO, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_0
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPILCD_CLK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_1
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPILCD_CS0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_2
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPILCD_DIO0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_3
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPILCD_DIO1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_4
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPILCD_DIO2, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_5
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_QSPILCD_DIO3, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_6
    { HAL_IOMUX_FUNC_MCU_UART2_RX, HAL_IOMUX_FUNC_MCU_I2C_M2_SCL, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P5_7
    { HAL_IOMUX_FUNC_MCU_UART2_TX, HAL_IOMUX_FUNC_MCU_I2C_M2_SDA, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P6_0
    { HAL_IOMUX_FUNC_MCU_UART1_CTS, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC0_CLK, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P6_1
    { HAL_IOMUX_FUNC_MCU_UART1_RTS, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC0_CMD, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P6_2
    { HAL_IOMUX_FUNC_MCU_UART1_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_SPI_CS1, HAL_IOMUX_FUNC_NONE},
    // P6_3
    { HAL_IOMUX_FUNC_MCU_UART1_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_SPI_DI1, HAL_IOMUX_FUNC_NONE},
    // P6_4
    { HAL_IOMUX_FUNC_MCU_I2S0_SCK, HAL_IOMUX_FUNC_MCU_UART2_RX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_SPI_CLK, HAL_IOMUX_FUNC_NONE},
    // P6_5
    { HAL_IOMUX_FUNC_MCU_I2S0_WS, HAL_IOMUX_FUNC_MCU_UART2_TX, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_SPI_CS0, HAL_IOMUX_FUNC_NONE},
    // P6_6
    { HAL_IOMUX_FUNC_MCU_I2S0_SDI0, HAL_IOMUX_FUNC_MCU_UART1_CTS, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_FUNC_MCU_I2C_M3_SCL, HAL_IOMUX_FUNC_MCU_SPI_DI0, HAL_IOMUX_FUNC_NONE},
    // P6_7
    { HAL_IOMUX_FUNC_MCU_I2S0_SDO, HAL_IOMUX_FUNC_MCU_UART1_RTS, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_FUNC_MCU_I2C_M3_SDA, HAL_IOMUX_FUNC_MCU_SPI_DIO, HAL_IOMUX_FUNC_NONE},
    // P7_0
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC0_DATA0, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_1
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC0_DATA1, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_2
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC0_DATA2, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_3
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_SDMMC0_DATA3, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_4
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_UART2_RX, HAL_IOMUX_FUNC_SDMMC0_DATA4, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_5
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_UART2_TX, HAL_IOMUX_FUNC_SDMMC0_DATA5, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_6
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_UART1_CTS, HAL_IOMUX_FUNC_SDMMC0_DATA6, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P7_7
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_MCU_UART1_RTS, HAL_IOMUX_FUNC_SDMMC0_DATA7, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_0
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_1
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_2
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_3
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_4
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_5
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_6
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P8_7
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_0
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_1
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_2
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_3
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_4
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_5
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_6
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
    // P9_7
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE},
};

static struct IOMUX_T * const iomux = (struct IOMUX_T *)IOMUX_BASE;

#ifdef ANC_PROD_TEST
#define OPT_TYPE
#else
#define OPT_TYPE                        const
#endif

static OPT_TYPE enum HAL_IOMUX_PIN_T digmic_ck_pin = DIG_MIC_CK_IOMUX_PIN;

static OPT_TYPE enum HAL_IOMUX_PIN_T digmic_d0_pin = DIG_MIC_D0_IOMUX_PIN;
static OPT_TYPE enum HAL_IOMUX_PIN_T digmic_d1_pin = DIG_MIC_D1_IOMUX_PIN;
static OPT_TYPE enum HAL_IOMUX_PIN_T digmic_d2_pin = DIG_MIC_D2_IOMUX_PIN;

#if (I2C0_IOMUX_INDEX == 22)
#define I2C_SEL                         (IOMUX_I2C1_M_SEL_GPIO | IOMUX_I2C2_M_SEL_GPIO | IOMUX_I2C3_M_SEL_GPIO)
#else
#define I2C_SEL                         (IOMUX_I2C0_M_SEL_GPIO | IOMUX_I2C1_M_SEL_GPIO | IOMUX_I2C2_M_SEL_GPIO | IOMUX_I2C3_M_SEL_GPIO)
#endif

__STATIC_FORCEINLINE
uint32_t iomux_lock(void)
{
#if defined(ROM_BUILD) || defined(PROGRAMMER)
    return 0;
#else
    uint32_t lock;

    lock = int_lock();
    while (hal_memsc_aon_lock(MEMSC_ID_IOMUX) == 0);

    return lock;
#endif
}

__STATIC_FORCEINLINE
void iomux_unlock(uint32_t lock)
{
#if defined(ROM_BUILD) || defined(PROGRAMMER)
    return;
#else
    hal_memsc_aon_unlock(MEMSC_ID_IOMUX);
    int_unlock(lock);
#endif
}

void hal_iomux_set_default_config(void)
{
    uint32_t i;
    volatile uint32_t *reg_sel;
    volatile uint32_t *reg_pu;
    volatile uint32_t *reg_pd;
    uint32_t sel_idx;
    uint32_t pu_idx;
    uint32_t pd_idx;

#if !defined(MCU_SPI_SLAVE) && !defined(CODEC_APP)
    // Clear spi slave pin mux
    iomux->REG_008 = ~0UL;
    // Disable spi slave irq
    iomux->REG_0C4 = 0;
#endif

    // Set all unused GPIOs to pull-down by default
    for (i = 0; i < HAL_IOMUX_PIN_NUM; i++) {
        if (i < HAL_GPIO_PIN_P4_0) {
            reg_sel = &iomux->REG_004 + i / 8;
            reg_pu = &iomux->REG_02C;
            reg_pd = &iomux->REG_030;
            sel_idx = (i % 8) * 4;
            pu_idx = i;
            pd_idx = i;
        } else {
            reg_sel = &iomux->REG_07C + (i - HAL_GPIO_PIN_P4_0) / 8;
            sel_idx = ((i - HAL_GPIO_PIN_P4_0) % 8) * 4;
            if (i < HAL_GPIO_PIN_P8_0) {
                reg_pu = &iomux->REG_094;
                reg_pd = &iomux->REG_098;
                pu_idx = i - HAL_GPIO_PIN_P4_0;
                pd_idx = i - HAL_GPIO_PIN_P4_0;
            } else {
                reg_pu = &iomux->REG_09C;
                reg_pd = &iomux->REG_09C;
                pu_idx = i - HAL_GPIO_PIN_P8_0;
                pd_idx = i - HAL_GPIO_PIN_P8_0 + 16;
            }
        }
        if (((*reg_sel & (0xF << sel_idx)) >> sel_idx) == 0xF) {
            *reg_pu &= ~(1 << pu_idx);
            *reg_pd |= (1 << pd_idx);
        }
    }
#ifdef FLASH_QSPI_ENABLE
    hal_iomux_set_flash_qspi();
#endif
}

uint32_t hal_iomux_check(const struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; ++i) {
    }
    return 0;
}

uint32_t hal_iomux_init(const struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count)
{
    uint32_t i;
    uint32_t ret;
    uint32_t lock;

    ret = 0;

    lock = iomux_lock();

    for (i = 0; i < count; ++i) {
        ret = hal_iomux_set_function(map[i].pin, map[i].function, HAL_IOMUX_OP_CLEAN_OTHER_FUNC_BIT);
        if (ret) {
            ret = (i << 8) + 1;
            goto _exit;
        }
        ret = hal_iomux_set_io_voltage_domains(map[i].pin, map[i].volt);
        if (ret) {
            ret = (i << 8) + 2;
            goto _exit;
        }
        ret = hal_iomux_set_io_pull_select(map[i].pin, map[i].pull_sel);
        if (ret) {
            ret = (i << 8) + 3;
            goto _exit;
        }
    }

_exit:
    iomux_unlock(lock);

    return ret;
}

#ifdef ANC_PROD_TEST
void hal_iomux_set_dig_mic_clock_pin(enum HAL_IOMUX_PIN_T pin)
{
    digmic_ck_pin = pin;
}
void hal_iomux_set_dig_mic_data0_pin(enum HAL_IOMUX_PIN_T pin)
{
    digmic_d0_pin = pin;
}

void hal_iomux_set_dig_mic_data1_pin(enum HAL_IOMUX_PIN_T pin)
{
    digmic_d1_pin = pin;
}

void hal_iomux_set_dig_mic_data2_pin(enum HAL_IOMUX_PIN_T pin)
{
    digmic_d2_pin = pin;
}
#endif

uint32_t hal_iomux_set_function(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_FUNCTION_T func, enum HAL_IOMUX_OP_TYPE_T type)
{
    int i;
    uint8_t val;
    __IO uint32_t *reg;
    uint32_t shift;

    if (pin >= HAL_IOMUX_PIN_LED_NUM) {
        return 1;
    }
    if (func >= HAL_IOMUX_FUNC_END) {
        return 2;
    }

    if (pin == HAL_IOMUX_PIN_P2_2 || pin == HAL_IOMUX_PIN_P2_3) {
        if (func ==  HAL_IOMUX_FUNC_I2C_SCL || func == HAL_IOMUX_FUNC_I2C_SDA) {
            // Enable analog I2C slave
            iomux->REG_050 = (iomux->REG_050 & ~IOMUX_GPIO_I2C_MODE) | I2C_SEL;
            // Set mcu GPIO func
            iomux->REG_00C = (iomux->REG_00C & ~(IOMUX_GPIO_P22_SEL_MASK | IOMUX_GPIO_P23_SEL_MASK)) |
                IOMUX_GPIO_P22_SEL(IOMUX_FUNC_VAL_GPIO) | IOMUX_GPIO_P23_SEL(IOMUX_FUNC_VAL_GPIO);
            return 0;
        } else {
            iomux->REG_050 = (iomux->REG_050 | IOMUX_GPIO_I2C_MODE) | I2C_SEL;
            // Continue to set the alt func
        }
    } else if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
#if !defined(CHIP_SUBSYS_SENS) && !defined(CHIP_ROLE_CP)
        if (func == HAL_IOMUX_FUNC_GPIO) {
            pmu_led_uart_disable(pin);
            hal_iomux_clear_pmu_uart();
        }
#endif
        ASSERT(func == HAL_IOMUX_FUNC_GPIO, "Bad func=%d for IOMUX pin=%d", func, pin);
        return 0;
    }

    if (pin >= ARRAY_SIZE(pin_func_map)) {
        return 0;
    }

    if (func == HAL_IOMUX_FUNC_GPIO) {
        val = IOMUX_FUNC_VAL_GPIO;
    } else {
        for (i = 0; i < IOMUX_ALT_FUNC_NUM; i++) {
            if (pin_func_map[pin][i] == func) {
                break;
            }
        }

        if (i == IOMUX_ALT_FUNC_NUM) {
            return 3;
        }
        val = index_to_func_val[i];
    }

    if (pin < HAL_IOMUX_PIN_P4_0) {
        reg = &iomux->REG_004 + pin / 8;
    } else {
        reg = &iomux->REG_07C + (pin - HAL_IOMUX_PIN_P4_0) / 8;
    }
    shift = (pin % 8) * 4;

    *reg = (*reg & ~(0xF << shift)) | (val << shift);

    return 0;
}

enum HAL_IOMUX_FUNCTION_T hal_iomux_get_function(enum HAL_IOMUX_PIN_T pin)
{
    return HAL_IOMUX_FUNC_NONE;
}

uint32_t hal_iomux_set_io_voltage_domains(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    if (pin >= HAL_IOMUX_PIN_LED_NUM) {
        return 1;
    }

#if !defined(CHIP_SUBSYS_SENS) && !defined(CHIP_ROLE_CP)
    if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        pmu_led_set_voltage_domains(pin, volt);
    }
#endif

    return 0;
}

uint32_t hal_iomux_set_io_pull_select(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_PULL_SELECT_T pull_sel)
{
    volatile uint32_t *reg_pu;
    volatile uint32_t *reg_pd;
    uint32_t pu_idx;
    uint32_t pd_idx;

    if (pin >= HAL_IOMUX_PIN_LED_NUM) {
        return 1;
    }

    if (pin < HAL_IOMUX_PIN_LED1) {
        if (pin < HAL_IOMUX_PIN_P4_0) {
            reg_pu = &iomux->REG_02C;
            reg_pd = &iomux->REG_030;
            pu_idx = pin;
            pd_idx = pin;
        } else if (pin < HAL_IOMUX_PIN_P8_0) {
            reg_pu = &iomux->REG_094;
            reg_pd = &iomux->REG_098;
            pu_idx = pin - HAL_IOMUX_PIN_P4_0;
            pd_idx = pin - HAL_IOMUX_PIN_P4_0;
        } else {
            reg_pu = &iomux->REG_09C;
            reg_pd = &iomux->REG_09C;
            pu_idx = pin - HAL_IOMUX_PIN_P8_0;
            pd_idx = pin - HAL_IOMUX_PIN_P8_0 + 16;
        }
        *reg_pu &= ~(1 << pu_idx);
        *reg_pd &= ~(1 << pd_idx);
        if (pull_sel == HAL_IOMUX_PIN_PULLUP_ENABLE) {
            *reg_pu |= (1 << pu_idx);
        } else if (pull_sel == HAL_IOMUX_PIN_PULLDOWN_ENABLE) {
            *reg_pd |= (1 << pd_idx);
        }
#if !defined(CHIP_SUBSYS_SENS) && !defined(CHIP_ROLE_CP)
    } else if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        pmu_led_set_pull_select(pin, pull_sel);
#endif
    }

    return 0;
}

uint32_t hal_iomux_set_io_drv(enum HAL_IOMUX_PIN_T pin, uint32_t val)
{
    if (pin >= HAL_IOMUX_PIN_NUM) {
        return 1;
    }
    if (val > 3) {
        return 2;
    }
    if (pin < HAL_IOMUX_PIN_P2_0) {
        iomux->REG_074 = (iomux->REG_074 & ~(IOMUX_GPIO_P0_DRV0_SEL_MASK << 2 * (pin - HAL_IOMUX_PIN_P0_0))) |
            (IOMUX_GPIO_P0_DRV0_SEL(val) << 2 * (pin - HAL_IOMUX_PIN_P0_0));
    } else if (pin < HAL_IOMUX_PIN_P4_0) {
        iomux->REG_078 = (iomux->REG_078 & ~(IOMUX_GPIO_P0_DRV0_SEL_MASK << 2 * (pin - HAL_IOMUX_PIN_P2_0))) |
            (IOMUX_GPIO_P0_DRV0_SEL(val) << 2 * (pin - HAL_IOMUX_PIN_P2_0));
    } else if (pin < HAL_IOMUX_PIN_P6_0) {
        iomux->REG_0A0 = (iomux->REG_0A0 & ~(IOMUX_GPIO_P0_DRV0_SEL_MASK << 2 * (pin - HAL_IOMUX_PIN_P4_0))) |
            (IOMUX_GPIO_P0_DRV0_SEL(val) << 2 * (pin - HAL_IOMUX_PIN_P4_0));
    } else if (pin < HAL_IOMUX_PIN_P8_0) {
        iomux->REG_0A4 = (iomux->REG_0A4 & ~(IOMUX_GPIO_P0_DRV0_SEL_MASK << 2 * (pin - HAL_IOMUX_PIN_P6_0))) |
            (IOMUX_GPIO_P0_DRV0_SEL(val) << 2 * (pin - HAL_IOMUX_PIN_P6_0));
    } else {
        iomux->REG_0A8 = (iomux->REG_0A8 & ~(IOMUX_GPIO_P0_DRV0_SEL_MASK << 2 * (pin - HAL_IOMUX_PIN_P8_0))) |
            (IOMUX_GPIO_P0_DRV0_SEL(val) << 2 * (pin - HAL_IOMUX_PIN_P8_0));
    }

    return 0;
}

void hal_iomux_set_uart0_voltage(enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
}

void hal_iomux_set_uart1_voltage(enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
}

bool hal_iomux_uart0_connected(void)
{
    uint32_t reg_050, reg_00c, reg_02c, reg_030;
    uint32_t mask;
    int val;

    // Save current iomux settings
    reg_050 = iomux->REG_050;
    reg_00c = iomux->REG_00C;
    reg_02c = iomux->REG_02C;
    reg_030 = iomux->REG_030;

    // Disable analog I2C slave & master
    iomux->REG_050 = (iomux->REG_050 | (IOMUX_GPIO_I2C_MODE | IOMUX_I2C0_M_SEL_GPIO)) | I2C_SEL;
    // Set uart0-rx as gpio
    iomux->REG_00C = SET_BITFIELD(iomux->REG_00C, IOMUX_GPIO_P22_SEL, IOMUX_FUNC_VAL_GPIO);

    mask = (1 << HAL_IOMUX_PIN_P2_2);
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Setup pulldown
    iomux->REG_030 |= mask;

    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)HAL_IOMUX_PIN_P2_2, HAL_GPIO_DIR_IN, 0);

    hal_sys_timer_delay(MS_TO_TICKS(2));

    val = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)HAL_IOMUX_PIN_P2_2);

    // Restore iomux settings
    iomux->REG_030 = reg_030;
    iomux->REG_02C = reg_02c;
    iomux->REG_00C = reg_00c;
    iomux->REG_050 = (iomux->REG_050 | reg_050) | I2C_SEL;

    hal_sys_timer_delay(MS_TO_TICKS(2));

    return !!val;
}

bool hal_iomux_uart1_connected(void)
{
    uint32_t reg_00c, reg_02c, reg_030;
    uint32_t mask;
    int val;

    // Save current iomux settings
    reg_00c = iomux->REG_00C;
    reg_02c = iomux->REG_02C;
    reg_030 = iomux->REG_030;

    // Set uart1-rx as gpio
    iomux->REG_00C = SET_BITFIELD(iomux->REG_00C, IOMUX_GPIO_P20_SEL, IOMUX_FUNC_VAL_GPIO);

    mask = (1 << HAL_IOMUX_PIN_P2_0);
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Setup pulldown
    iomux->REG_030 |= mask;

    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)HAL_IOMUX_PIN_P2_0, HAL_GPIO_DIR_IN, 0);

    hal_sys_timer_delay(MS_TO_TICKS(2));

    val = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)HAL_IOMUX_PIN_P2_0);

    // Restore iomux settings
    iomux->REG_030 = reg_030;
    iomux->REG_02C = reg_02c;
    iomux->REG_00C = reg_00c;

    hal_sys_timer_delay(MS_TO_TICKS(2));

    return !!val;
}

static void hal_iomux_set_uart0_common(uint8_t func)
{
    uint32_t mask;
    uint32_t lock;

    lock = iomux_lock();

    // Disable analog I2C slave & master
    iomux->REG_050 = (iomux->REG_050 | (IOMUX_GPIO_I2C_MODE | IOMUX_I2C0_M_SEL_GPIO)) | I2C_SEL;
    // Set uart0 func
    iomux->REG_00C = (iomux->REG_00C & ~(IOMUX_GPIO_P22_SEL_MASK | IOMUX_GPIO_P23_SEL_MASK)) |
        IOMUX_GPIO_P22_SEL(func) | IOMUX_GPIO_P23_SEL(func);

    mask = (1 << HAL_IOMUX_PIN_P2_2) | (1 << HAL_IOMUX_PIN_P2_3);
    // Setup pullup
    iomux->REG_02C |= (1 << HAL_IOMUX_PIN_P2_2);
    iomux->REG_02C &= ~(1 << HAL_IOMUX_PIN_P2_3);
    // Clear pulldown
    iomux->REG_030 &= ~mask;

    iomux_unlock(lock);
}

void hal_iomux_set_mcu_uart0(void)
{
    hal_iomux_set_uart0_common(2);
}

void hal_iomux_set_sens_uart0(void)
{
    hal_iomux_set_uart0_common(1);
}

void hal_iomux_set_uart0(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_uart0();
#else
    hal_iomux_set_mcu_uart0();
#endif
}

void hal_iomux_set_mcu_uart1(void)
{
    uint32_t mask;
    uint32_t lock;

    lock = iomux_lock();

    // Set uart1 func
#if (UART1_IOMUX_INDEX == 62)
    iomux->REG_084 = (iomux->REG_084 & ~(IOMUX_GPIO_P62_SEL_MASK | IOMUX_GPIO_P63_SEL_MASK)) |
        IOMUX_GPIO_P62_SEL(1) | IOMUX_GPIO_P63_SEL(1);

    mask = (1 << (HAL_IOMUX_PIN_P6_2 - HAL_IOMUX_PIN_P4_0)) | (1 << (HAL_IOMUX_PIN_P6_3 - HAL_IOMUX_PIN_P4_0));
    // Setup pullup
    iomux->REG_094 |= (1 << (HAL_IOMUX_PIN_P6_2 - HAL_IOMUX_PIN_P4_0));
    iomux->REG_094 &= ~(1 << (HAL_IOMUX_PIN_P6_3 - HAL_IOMUX_PIN_P4_0));
    // Clear pulldown
    iomux->REG_098 &= ~mask;
#else
    iomux->REG_00C = (iomux->REG_00C & ~(IOMUX_GPIO_P20_SEL_MASK | IOMUX_GPIO_P21_SEL_MASK)) |
        IOMUX_GPIO_P20_SEL(2) | IOMUX_GPIO_P21_SEL(2);

    mask = (1 << HAL_IOMUX_PIN_P2_0) | (1 << HAL_IOMUX_PIN_P2_1);
    // Setup pullup
    iomux->REG_02C |= (1 << HAL_IOMUX_PIN_P2_0);
    iomux->REG_02C &= ~(1 << HAL_IOMUX_PIN_P2_1);
    // Clear pulldown
    iomux->REG_030 &= ~mask;
#endif

    iomux_unlock(lock);
}

void hal_iomux_set_sens_uart1(void)
{
    uint32_t mask;
    uint32_t lock;

    lock = iomux_lock();

    // Set uart1 func
    iomux->REG_008 = (iomux->REG_008 & ~(IOMUX_GPIO_P10_SEL_MASK | IOMUX_GPIO_P11_SEL_MASK)) |
        IOMUX_GPIO_P10_SEL(1) | IOMUX_GPIO_P11_SEL(1);

    mask = (1 << HAL_IOMUX_PIN_P1_0) | (1 << HAL_IOMUX_PIN_P1_1);
    // Setup pullup
    iomux->REG_02C |= (1 << HAL_IOMUX_PIN_P1_0);
    iomux->REG_02C &= ~(1 << HAL_IOMUX_PIN_P1_1);
    // Clear pulldown
    iomux->REG_030 &= ~mask;

    iomux_unlock(lock);
}

void hal_iomux_set_uart1(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_uart1();
#else
    hal_iomux_set_mcu_uart1();
#endif
}

void hal_iomux_set_mcu_uart2(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP iomux_uart[] = {
#if (UART2_IOMUX_INDEX == 10)
        {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_MCU_UART2_RX, UART2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_MCU_UART2_TX, UART2_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P6_4, HAL_IOMUX_FUNC_MCU_UART2_RX, UART2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P6_5, HAL_IOMUX_FUNC_MCU_UART2_TX, UART2_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };

    hal_iomux_init(iomux_uart, ARRAY_SIZE(iomux_uart));
}

void hal_iomux_set_uart2(void)
{
    hal_iomux_set_mcu_uart2();
}

void hal_iomux_set_analog_i2c(void)
{
#if !(defined(JTAG_ENABLE) && (JTAG_IOMUX_INDEX == 22))
    uint32_t mask;
    uint32_t lock;

    lock = iomux_lock();

    // Disable analog I2C master
    iomux->REG_050 = (iomux->REG_050 | IOMUX_I2C0_M_SEL_GPIO) | I2C_SEL;
    // Set mcu GPIO func
    iomux->REG_00C = (iomux->REG_00C & ~(IOMUX_GPIO_P22_SEL_MASK | IOMUX_GPIO_P23_SEL_MASK)) |
        IOMUX_GPIO_P22_SEL(IOMUX_FUNC_VAL_GPIO) | IOMUX_GPIO_P23_SEL(IOMUX_FUNC_VAL_GPIO);
    // Enable analog I2C slave
    iomux->REG_050 &= (iomux->REG_050 & ~IOMUX_GPIO_I2C_MODE) | I2C_SEL;

    mask = (1 << HAL_IOMUX_PIN_P2_2) | (1 << HAL_IOMUX_PIN_P2_3);
    // Setup pullup
    iomux->REG_02C |= mask;
    // Clear pulldown
    iomux->REG_030 &= ~mask;

    iomux_unlock(lock);
#endif
}

void hal_iomux_set_jtag(void)
{
    uint32_t mask;
    uint32_t lock;

#if (JTAG_IOMUX_INDEX == 22)
    hal_iomux_set_uart0_common(3);

    lock = iomux_lock();

    mask = (1 << HAL_IOMUX_PIN_P2_2) | (1 << HAL_IOMUX_PIN_P2_3);
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Clear pulldown
    iomux->REG_030 &= ~mask;

    iomux_unlock(lock);
#else
    uint32_t val;

    lock = iomux_lock();

    // SWCLK/TCK, SWDIO/TMS
    mask = IOMUX_GPIO_P31_SEL_MASK | IOMUX_GPIO_P32_SEL_MASK;
    val = IOMUX_GPIO_P31_SEL(4) | IOMUX_GPIO_P32_SEL(4);

    // TDI, TDO
#ifdef JTAG_TDI_TDO_PIN
    mask |= IOMUX_GPIO_P33_SEL_MASK | IOMUX_GPIO_P34_SEL_MASK;
    val |= IOMUX_GPIO_P33_SEL(4) | IOMUX_GPIO_P34_SEL(4);
#endif
    iomux->REG_010 = (iomux->REG_010 & ~mask) | val;

    // RESET
#if defined(JTAG_RESET_PIN) || defined(JTAG_TDI_TDO_PIN)
    iomux->REG_010 = (iomux->REG_010 & ~(IOMUX_GPIO_P30_SEL_MASK)) | IOMUX_GPIO_P30_SEL(4);
#endif

    mask = (1 << HAL_IOMUX_PIN_P3_1) | (1 << HAL_IOMUX_PIN_P3_2);
#ifdef JTAG_TDI_TDO_PIN
    mask |= (1 << HAL_IOMUX_PIN_P3_3) | (1 << HAL_IOMUX_PIN_P3_4);
#endif
#if defined(JTAG_RESET_PIN) || defined(JTAG_TDI_TDO_PIN)
    mask |= (1 << HAL_IOMUX_PIN_P3_0);
#endif
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Clear pulldown
    iomux->REG_030 &= ~mask;

    iomux_unlock(lock);
#endif
}

enum HAL_IOMUX_ISPI_ACCESS_T hal_iomux_ispi_access_enable(enum HAL_IOMUX_ISPI_ACCESS_T access)
{
    uint32_t v;

    v = iomux->REG_044;
    iomux->REG_044 |= access;

    return v;
}

enum HAL_IOMUX_ISPI_ACCESS_T hal_iomux_ispi_access_disable(enum HAL_IOMUX_ISPI_ACCESS_T access)
{
    uint32_t v;

    v = iomux->REG_044;
    iomux->REG_044 &= ~access;

    return v;
}

void hal_iomux_ispi_access_init(void)
{
    // Disable bt spi access ana/pmu interface
    hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_BT_ANA | HAL_IOMUX_ISPI_BT_PMU);
}

void hal_iomux_set_mcu_i2s0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2s[] = {
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_MCU_I2S0_WS,   I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_MCU_I2S0_SCK,  I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_MCU_I2S0_SDI0, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_MCU_I2S0_SDO,  I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };

    hal_iomux_init(pinmux_i2s, ARRAY_SIZE(pinmux_i2s));
}

void hal_iomux_set_sens_i2s0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2s[] = {
#if (I2S0_IOMUX_INDEX == 25)
        {HAL_IOMUX_PIN_P2_5, HAL_IOMUX_FUNC_SENS_I2S0_WS,   I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_4, HAL_IOMUX_FUNC_SENS_I2S0_SCK,  I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_6, HAL_IOMUX_FUNC_SENS_I2S0_SDI0, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_7, HAL_IOMUX_FUNC_SENS_I2S0_SDO,  I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_SENS_I2S0_WS,   I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_SENS_I2S0_SCK,  I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_SENS_I2S0_SDI0, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_SENS_I2S0_SDO,  I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };

    hal_iomux_init(pinmux_i2s, ARRAY_SIZE(pinmux_i2s));
}

void hal_iomux_set_i2s0(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_i2s0();
#else
    hal_iomux_set_mcu_i2s0();
#endif
}

void hal_iomux_set_i2s1(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2s[] = {
#if (I2S1_IOMUX_INDEX == 44)
        {HAL_IOMUX_PIN_P4_4, HAL_IOMUX_FUNC_MCU_I2S1_SCK,  I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P4_5, HAL_IOMUX_FUNC_MCU_I2S1_WS,   I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P4_6, HAL_IOMUX_FUNC_MCU_I2S1_SDI0, I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P4_7, HAL_IOMUX_FUNC_MCU_I2S1_SDO,  I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P0_6, HAL_IOMUX_FUNC_MCU_I2S1_SCK,  I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_7, HAL_IOMUX_FUNC_MCU_I2S1_WS,   I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_MCU_I2S1_SDI0, I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_MCU_I2S1_SDO,  I2S1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };

    hal_iomux_init(pinmux_i2s, ARRAY_SIZE(pinmux_i2s));
}

void hal_iomux_set_i2s0_mclk(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux[] = {
#if (I2S_MCLK_IOMUX_INDEX == 4)
        {HAL_IOMUX_PIN_P0_4, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#elif (I2S_MCLK_IOMUX_INDEX == 16)
        {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#elif (I2S_MCLK_IOMUX_INDEX == 33)
        {HAL_IOMUX_PIN_P3_3, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#elif (I2S_MCLK_IOMUX_INDEX == 54)
        {HAL_IOMUX_PIN_P5_4, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#elif (I2S_MCLK_IOMUX_INDEX == 62)
        {HAL_IOMUX_PIN_P6_2, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#elif (I2S_MCLK_IOMUX_INDEX == 74)
        {HAL_IOMUX_PIN_P7_4, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_I2S_MCLK, I2S0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };

    hal_iomux_init(pinmux, ARRAY_SIZE(pinmux));
}

void hal_iomux_set_spdif0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spdif[] = {
#if (SPDIF0_IOMUX_INDEX == 54)
        {HAL_IOMUX_PIN_P5_4, HAL_IOMUX_FUNC_SPDIF0_DI, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P5_5, HAL_IOMUX_FUNC_SPDIF0_DO, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P0_4, HAL_IOMUX_FUNC_SPDIF0_DI, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P0_5, HAL_IOMUX_FUNC_SPDIF0_DO, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };

    hal_iomux_init(pinmux_spdif, ARRAY_SIZE(pinmux_spdif));
}

void hal_iomux_set_spdif1(void)
{
}

void hal_iomux_set_dig_mic(uint32_t map)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic_clk[] = {
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_PDM0_CK, DIGMIC_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic0[] = {
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_PDM0_D,  DIGMIC_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic1[] = {
        {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_PDM1_D,  DIGMIC_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic2[] = {
        {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_PDM2_D,  DIGMIC_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
    uint32_t clk_map = 0;

    if (digmic_ck_pin == HAL_IOMUX_PIN_P0_0 || digmic_ck_pin == HAL_IOMUX_PIN_P1_2 ||
            digmic_ck_pin == HAL_IOMUX_PIN_P5_6 || digmic_ck_pin == HAL_IOMUX_PIN_P6_6) {
        pinmux_digitalmic_clk[0].pin = digmic_ck_pin;
        pinmux_digitalmic_clk[0].function = HAL_IOMUX_FUNC_PDM0_CK;
        clk_map = (1 << 0);
    } else if (digmic_ck_pin == HAL_IOMUX_PIN_P1_4 || digmic_ck_pin == HAL_IOMUX_PIN_P2_6 || digmic_ck_pin == HAL_IOMUX_PIN_P3_6) {
        pinmux_digitalmic_clk[0].pin = digmic_ck_pin;
        pinmux_digitalmic_clk[0].function = HAL_IOMUX_FUNC_PDM1_CK;
        clk_map = (1 << 1);
    } else if (digmic_ck_pin == HAL_IOMUX_PIN_P1_6 || digmic_ck_pin == HAL_IOMUX_PIN_P2_4 || digmic_ck_pin == HAL_IOMUX_PIN_P3_0) {
        pinmux_digitalmic_clk[0].pin = digmic_ck_pin;
        pinmux_digitalmic_clk[0].function = HAL_IOMUX_FUNC_PDM2_CK;
    }

    if (digmic_d0_pin == HAL_IOMUX_PIN_P0_1 || digmic_d0_pin == HAL_IOMUX_PIN_P1_3 ||
            digmic_d0_pin == HAL_IOMUX_PIN_P5_7 || digmic_d0_pin == HAL_IOMUX_PIN_P6_7) {
        pinmux_digitalmic0[0].pin = digmic_d0_pin;
    }

    if (digmic_d1_pin == HAL_IOMUX_PIN_P0_2 || digmic_d1_pin == HAL_IOMUX_PIN_P1_5 ||
            digmic_d1_pin == HAL_IOMUX_PIN_P2_7 || digmic_d1_pin == HAL_IOMUX_PIN_P3_7) {
        pinmux_digitalmic1[0].pin = digmic_d1_pin;
    }

    if (digmic_d2_pin == HAL_IOMUX_PIN_P0_3 || digmic_d2_pin == HAL_IOMUX_PIN_P1_7 ||
            digmic_d2_pin == HAL_IOMUX_PIN_P2_5 || digmic_d2_pin == HAL_IOMUX_PIN_P3_1 ) {
        pinmux_digitalmic2[0].pin = digmic_d2_pin;
    }

    if ((map & 0xF) == 0) {
        pinmux_digitalmic_clk[0].function = HAL_IOMUX_FUNC_GPIO;
    }
    hal_iomux_init(pinmux_digitalmic_clk, ARRAY_SIZE(pinmux_digitalmic_clk));
    if (map & (1 << 0)) {
        hal_iomux_init(pinmux_digitalmic0, ARRAY_SIZE(pinmux_digitalmic0));
    }
    if (map & (1 << 1)) {
        hal_iomux_init(pinmux_digitalmic1, ARRAY_SIZE(pinmux_digitalmic1));
    }
    if (map & (1 << 2)) {
        hal_iomux_init(pinmux_digitalmic2, ARRAY_SIZE(pinmux_digitalmic2));
    }

#ifdef CHIP_SUBSYS_SENS
    hal_cmu_sens_pdm_clock_out(clk_map);
#else
    hal_cmu_mcu_pdm_clock_out(clk_map);
#endif
}

void hal_iomux_set_mcu_spi(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spi_3wire[] = {
#if (SPI_IOMUX_INDEX == 24)
        {HAL_IOMUX_PIN_P2_4, HAL_IOMUX_FUNC_MCU_SPI_CLK, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_5, HAL_IOMUX_FUNC_MCU_SPI_CS0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_7, HAL_IOMUX_FUNC_MCU_SPI_DIO, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_MCU_SPI_CLK, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_MCU_SPI_CS0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_MCU_SPI_DIO, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
#ifdef SPI_IOMUX_CS1_INDEX
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_MCU_SPI_CS1,  SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
#ifdef SPI_IOMUX_CS2_INDEX
        {HAL_IOMUX_PIN_P6_1, HAL_IOMUX_FUNC_MCU_SPI_CS2,  SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
#ifdef SPI_IOMUX_CS3_INDEX
        {HAL_IOMUX_PIN_P5_6, HAL_IOMUX_FUNC_MCU_SPI_CS3,  SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };
#ifdef SPI_IOMUX_4WIRE
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spi_4wire[1] = {
#if (SPI_IOMUX_DI0_INDEX == 26)
        {HAL_IOMUX_PIN_P2_6, HAL_IOMUX_FUNC_MCU_SPI_DI0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_MCU_SPI_DI0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
#ifdef SPI_IOMUX_DI1_INDEX
        {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_MCU_SPI_DI1, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
#ifdef SPI_IOMUX_DI2_INDEX
        {HAL_IOMUX_PIN_P6_0, HAL_IOMUX_FUNC_MCU_SPI_DI2, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
#ifdef SPI_IOMUX_DI3_INDEX
        {HAL_IOMUX_PIN_P5_7, HAL_IOMUX_FUNC_MCU_SPI_DI3, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };
#endif

    hal_iomux_init(pinmux_spi_3wire, ARRAY_SIZE(pinmux_spi_3wire));
#ifdef SPI_IOMUX_4WIRE
    hal_iomux_init(pinmux_spi_4wire, ARRAY_SIZE(pinmux_spi_4wire));
#endif
}

void hal_iomux_set_sens_spi(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spi_3wire[] = {
#if (SPI_IOMUX_INDEX == 30)
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_SENS_SPI_CLK, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_SENS_SPI_CS0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P3_3, HAL_IOMUX_FUNC_SENS_SPI_DIO, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_SENS_SPI_CLK, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_SENS_SPI_CS0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_SENS_SPI_DIO, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };
#ifdef SPI_IOMUX_4WIRE
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spi_4wire[1] = {
#if (SPI_IOMUX_INDEX == 30)
        {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_SENS_SPI_DI0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_SENS_SPI_DI0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };
#endif

    hal_iomux_init(pinmux_spi_3wire, ARRAY_SIZE(pinmux_spi_3wire));
#ifdef SPI_IOMUX_4WIRE
    hal_iomux_init(pinmux_spi_4wire, ARRAY_SIZE(pinmux_spi_4wire));
#endif
}

void hal_iomux_set_spi(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_spi();
#else
    hal_iomux_set_mcu_spi();
#endif
}

void hal_iomux_set_sens_spilcd(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spilcd_3wire[] = {
        {HAL_IOMUX_PIN_P2_4, HAL_IOMUX_FUNC_SENS_SPILCD_CLK, SPILCD_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_5, HAL_IOMUX_FUNC_SENS_SPILCD_CS0, SPILCD_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P2_7, HAL_IOMUX_FUNC_SENS_SPILCD_DIO, SPILCD_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
#ifdef SPILCD_IOMUX_4WIRE
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spilcd_4wire[] = {
        {HAL_IOMUX_PIN_P2_6, HAL_IOMUX_FUNC_SENS_SPILCD_DI0, SPILCD_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
#endif

    hal_iomux_init(pinmux_spilcd_3wire, ARRAY_SIZE(pinmux_spilcd_3wire));
#ifdef SPILCD_IOMUX_4WIRE
    hal_iomux_init(pinmux_spilcd_4wire, ARRAY_SIZE(pinmux_spilcd_4wire));
#endif
}

void hal_iomux_set_spilcd(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_spilcd();
#endif
}

void hal_iomux_set_mcu_i2c0(void)
{
#if (I2C0_IOMUX_INDEX == 22)
    hal_iomux_set_analog_i2c();
    // IOMUX_GPIO_I2C_MODE should be kept in disabled state
    iomux->REG_050 = (iomux->REG_050 & ~IOMUX_I2C0_M_SEL_GPIO) | I2C_SEL;
#elif (I2C0_IOMUX_INDEX == 35)
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P3_5, HAL_IOMUX_FUNC_MCU_I2C_M0_SCL, I2C0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P3_4, HAL_IOMUX_FUNC_MCU_I2C_M0_SDA, I2C0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
#else
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P0_4, HAL_IOMUX_FUNC_MCU_I2C_M0_SCL, I2C0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P0_5, HAL_IOMUX_FUNC_MCU_I2C_M0_SDA, I2C0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
#endif
}

void hal_iomux_set_sens_i2c0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P0_4, HAL_IOMUX_FUNC_SENS_I2C_M0_SCL, I2C0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P0_5, HAL_IOMUX_FUNC_SENS_I2C_M0_SDA, I2C0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
}

void hal_iomux_set_i2c0(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_i2c0();
#else
    hal_iomux_set_mcu_i2c0();
#endif
}

void hal_iomux_set_mcu_i2c1(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
#if (I2C1_IOMUX_INDEX == 56)
        {HAL_IOMUX_PIN_P5_6, HAL_IOMUX_FUNC_MCU_I2C_M1_SCL, I2C1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P5_7, HAL_IOMUX_FUNC_MCU_I2C_M1_SDA, I2C1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
#else
        {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_MCU_I2C_M1_SCL, I2C1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_MCU_I2C_M1_SDA, I2C1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
#endif
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
    iomux->REG_050 |= I2C_SEL;
}

void hal_iomux_set_sens_i2c1(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P0_6, HAL_IOMUX_FUNC_SENS_I2C_M1_SCL, I2C1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P0_7, HAL_IOMUX_FUNC_SENS_I2C_M1_SDA, I2C1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
}

void hal_iomux_set_i2c1(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_i2c1();
#else
    hal_iomux_set_mcu_i2c1();
#endif
}

void hal_iomux_set_mcu_i2c2(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
#if (I2C2_IOMUX_INDEX == 56)
        {HAL_IOMUX_PIN_P5_6, HAL_IOMUX_FUNC_MCU_I2C_M2_SCL, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P5_7, HAL_IOMUX_FUNC_MCU_I2C_M2_SDA, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
#else
        {HAL_IOMUX_PIN_P4_4, HAL_IOMUX_FUNC_MCU_I2C_M2_SCL, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P4_5, HAL_IOMUX_FUNC_MCU_I2C_M2_SDA, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
#endif
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
    iomux->REG_050 |= I2C_SEL;
}

void hal_iomux_set_sens_i2c2(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
#if (I2C2_IOMUX_INDEX == 34)
        {HAL_IOMUX_PIN_P3_4, HAL_IOMUX_FUNC_SENS_I2C_M2_SCL, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P3_5, HAL_IOMUX_FUNC_SENS_I2C_M2_SDA, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
#else
        {HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_SENS_I2C_M2_SCL, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_SENS_I2C_M2_SDA, I2C2_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
#endif
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
}

void hal_iomux_set_i2c2(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_i2c2();
#else
    hal_iomux_set_mcu_i2c2();
#endif
}

void hal_iomux_set_mcu_i2c3(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P4_6, HAL_IOMUX_FUNC_MCU_I2C_M3_SCL, I2C3_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P4_7, HAL_IOMUX_FUNC_MCU_I2C_M3_SDA, I2C3_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
    iomux->REG_050 |= I2C_SEL;
}

void hal_iomux_set_sens_i2c3(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P3_6, HAL_IOMUX_FUNC_SENS_I2C_M3_SCL, I2C3_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P3_7, HAL_IOMUX_FUNC_SENS_I2C_M3_SDA, I2C3_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_iomux_init(pinmux_i2c, ARRAY_SIZE(pinmux_i2c));
}

void hal_iomux_set_i2c3(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_set_sens_i2c3();
#else
    hal_iomux_set_mcu_i2c3();
#endif
}

void hal_iomux_set_pwm0(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM0_IOMUX_INDEX == 30)
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM0_IOMUX_INDEX == 60)
        {HAL_IOMUX_PIN_P6_0, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM0_IOMUX_INDEX == 70)
        {HAL_IOMUX_PIN_P7_0, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM0_IOMUX_INDEX == 80)
        {HAL_IOMUX_PIN_P8_0, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM0_IOMUX_INDEX == 90)
        {HAL_IOMUX_PIN_P9_0, HAL_IOMUX_FUNC_PWM0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm1(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM1_IOMUX_INDEX == 31)
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM1_IOMUX_INDEX == 61)
        {HAL_IOMUX_PIN_P6_1, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM1_IOMUX_INDEX == 71)
        {HAL_IOMUX_PIN_P7_1, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM1_IOMUX_INDEX == 81)
        {HAL_IOMUX_PIN_P8_1, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM1_IOMUX_INDEX == 91)
        {HAL_IOMUX_PIN_P9_1, HAL_IOMUX_FUNC_PWM1, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm2(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM2_IOMUX_INDEX == 32)
        {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM2_IOMUX_INDEX == 62)
        {HAL_IOMUX_PIN_P6_2, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM2_IOMUX_INDEX == 72)
        {HAL_IOMUX_PIN_P7_2, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM2_IOMUX_INDEX == 82)
        {HAL_IOMUX_PIN_P8_2, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM2_IOMUX_INDEX == 92)
        {HAL_IOMUX_PIN_P9_2, HAL_IOMUX_FUNC_PWM2, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm3(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM3_IOMUX_INDEX == 33)
        {HAL_IOMUX_PIN_P3_3, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM3_IOMUX_INDEX == 63)
        {HAL_IOMUX_PIN_P6_3, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM3_IOMUX_INDEX == 73)
        {HAL_IOMUX_PIN_P7_3, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM3_IOMUX_INDEX == 83)
        {HAL_IOMUX_PIN_P8_3, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM3_IOMUX_INDEX == 93)
        {HAL_IOMUX_PIN_P9_3, HAL_IOMUX_FUNC_PWM3, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm4(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM4_IOMUX_INDEX == 34)
        {HAL_IOMUX_PIN_P3_4, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM4_IOMUX_INDEX == 64)
        {HAL_IOMUX_PIN_P6_4, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM4_IOMUX_INDEX == 74)
        {HAL_IOMUX_PIN_P7_4, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM4_IOMUX_INDEX == 84)
        {HAL_IOMUX_PIN_P8_4, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM4_IOMUX_INDEX == 94)
        {HAL_IOMUX_PIN_P9_4, HAL_IOMUX_FUNC_PWM4, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm5(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM5_IOMUX_INDEX == 35)
        {HAL_IOMUX_PIN_P3_5, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM5_IOMUX_INDEX == 65)
        {HAL_IOMUX_PIN_P6_5, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM5_IOMUX_INDEX == 75)
        {HAL_IOMUX_PIN_P7_5, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM5_IOMUX_INDEX == 85)
        {HAL_IOMUX_PIN_P8_5, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM5_IOMUX_INDEX == 95)
        {HAL_IOMUX_PIN_P9_5, HAL_IOMUX_FUNC_PWM5, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm6(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM6_IOMUX_INDEX == 36)
        {HAL_IOMUX_PIN_P3_6, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM6_IOMUX_INDEX == 66)
        {HAL_IOMUX_PIN_P6_6, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM6_IOMUX_INDEX == 76)
        {HAL_IOMUX_PIN_P7_6, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM6_IOMUX_INDEX == 86)
        {HAL_IOMUX_PIN_P8_6, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM6_IOMUX_INDEX == 96)
        {HAL_IOMUX_PIN_P9_6, HAL_IOMUX_FUNC_PWM6, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_pwm7(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_pwm[] = {
#if (PWM7_IOMUX_INDEX == 37)
        {HAL_IOMUX_PIN_P3_7, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM7_IOMUX_INDEX == 67)
        {HAL_IOMUX_PIN_P6_7, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM7_IOMUX_INDEX == 77)
        {HAL_IOMUX_PIN_P7_7, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM7_IOMUX_INDEX == 87)
        {HAL_IOMUX_PIN_P8_7, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#elif (PWM7_IOMUX_INDEX == 97)
        {HAL_IOMUX_PIN_P9_7, HAL_IOMUX_FUNC_PWM7, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
#endif
    };
    hal_iomux_init(pinmux_pwm, ARRAY_SIZE(pinmux_pwm));
}

void hal_iomux_set_clock_out(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_clkout[] = {
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_CLK_OUT, CLKOUT_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };

    hal_iomux_init(pinmux_clkout, ARRAY_SIZE(pinmux_clkout));
}

void hal_iomux_set_mcu_clock_out(void)
{
}

void hal_iomux_set_bt_clock_out(void)
{
}

///if use the tports for bt/wifi coex  we can't use tports any more
#ifdef BT_WIFI_COEX_P12
void hal_iomux_set_bt_tport(void)
{
    uint32_t lock;

    lock = iomux_lock();


    //P1_0 ~ P1_1,
    iomux->REG_008 = (iomux->REG_008 & ~(IOMUX_GPIO_P12_SEL_MASK)) |
    IOMUX_GPIO_P12_SEL(8);

    // ANA TEST DIR
    iomux->REG_014 = 0x400;
    // ANA TEST SEL
    iomux->REG_018 = IOMUX_ANA_TEST_SEL(5);

    iomux_unlock(lock);
}

void hal_iomux_clear_bt_tport(void)
{
    uint32_t lock;

    static const struct HAL_IOMUX_PIN_FUNCTION_MAP tport_iomux[] = {
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
    }

    hal_iomux_init(tport_iomux, ARRAY_SIZE(tport_iomux));

    lock = iomux_lock();
    // ANA TEST DIR
    iomux->REG_014 = 0x0;
    // ANA TEST SEL
    iomux->REG_018 = IOMUX_ANA_TEST_SEL(0);

    iomux_unlock(lock);
}

#else
void hal_iomux_set_bt_tport(void)
{
    uint32_t lock;

    lock = iomux_lock();

    // P0_0 ~ P0_3,
    iomux->REG_004 = (iomux->REG_004 & ~(IOMUX_GPIO_P00_SEL_MASK | IOMUX_GPIO_P01_SEL_MASK | IOMUX_GPIO_P02_SEL_MASK | IOMUX_GPIO_P03_SEL_MASK)) |
    IOMUX_GPIO_P00_SEL(8) | IOMUX_GPIO_P01_SEL(8) | IOMUX_GPIO_P02_SEL(8) |IOMUX_GPIO_P03_SEL(8);

#ifdef TPORTS_KEY_COEXIST
    //P1_0 ~ P1_1,
    iomux->REG_008 = (iomux->REG_008 & ~(IOMUX_GPIO_P10_SEL_MASK | IOMUX_GPIO_P11_SEL_MASK)) |
    IOMUX_GPIO_P10_SEL(8) |IOMUX_GPIO_P11_SEL(8);
#else
    //P1_0 ~ P1_3,
    iomux->REG_008 = (iomux->REG_008 & ~(IOMUX_GPIO_P10_SEL_MASK | IOMUX_GPIO_P11_SEL_MASK | IOMUX_GPIO_P12_SEL_MASK | IOMUX_GPIO_P13_SEL_MASK)) |
    IOMUX_GPIO_P10_SEL(8) | IOMUX_GPIO_P11_SEL(8) | IOMUX_GPIO_P12_SEL(8) |IOMUX_GPIO_P13_SEL(8);
#endif

    // ANA TEST DIR
    iomux->REG_014 = 0xf0f;
    // ANA TEST SEL
    iomux->REG_018 = IOMUX_ANA_TEST_SEL(5);

    iomux_unlock(lock);
}

void hal_iomux_clear_bt_tport(void)
{
    uint32_t lock;

    // P0_0 ~ P0_3,
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP tport_iomux[] = {
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
#ifdef TPORTS_KEY_COEXIST
    //P1_0 ~ P1_1,
        {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
#else
    //P1_0 ~ P1_3,
        {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
#endif
    };

    hal_iomux_init(tport_iomux, ARRAY_SIZE(tport_iomux));

    lock = iomux_lock();
    // ANA TEST DIR
    iomux->REG_014 = 0x0;
    // ANA TEST SEL
    iomux->REG_018 = IOMUX_ANA_TEST_SEL(0);

    iomux_unlock(lock);
}
#endif

void hal_iomux_set_bt_active_out(void)
{
    uint32_t lock;

    lock = iomux_lock();

    //P62 switch to btc ctrl bt active signal
    iomux->REG_084 = (iomux->REG_084 & ~(IOMUX_GPIO_P62_SEL_MASK)) | IOMUX_GPIO_P62_SEL(3);

    iomux_unlock(lock);
}

void hal_iomux_set_pta_out(void)
{
    uint32_t lock;

    lock = iomux_lock();

    iomux->REG_080 = (iomux->REG_080 & ~(IOMUX_GPIO_P50_SEL_MASK | IOMUX_GPIO_P51_SEL_MASK | IOMUX_GPIO_P52_SEL_MASK )) |
    IOMUX_GPIO_P50_SEL(3) | IOMUX_GPIO_P51_SEL(3) | IOMUX_GPIO_P52_SEL(3);

    iomux_unlock(lock);
}

void hal_iomux_set_bt_rf_sw(int rx_on, int tx_on)
{
    uint32_t lock;

    lock = iomux_lock();

    if (rx_on) {
#if (RXON_IOMUX_INDEX == 6)
        iomux->REG_004 = SET_BITFIELD(iomux->REG_004, IOMUX_GPIO_P06_SEL, 5);
#elif (RXON_IOMUX_INDEX == 10)
        iomux->REG_008 = SET_BITFIELD(iomux->REG_008, IOMUX_GPIO_P10_SEL, 6);
#elif (RXON_IOMUX_INDEX == 16)
        iomux->REG_008 = SET_BITFIELD(iomux->REG_008, IOMUX_GPIO_P16_SEL, 3);
#else
#endif
    }

    if (tx_on) {
#if (TXON_IOMUX_INDEX == 7)
        iomux->REG_004 = SET_BITFIELD(iomux->REG_004, IOMUX_GPIO_P07_SEL, 5);
#elif (TXON_IOMUX_INDEX == 11)
        iomux->REG_008 = SET_BITFIELD(iomux->REG_008, IOMUX_GPIO_P11_SEL, 6);
#elif (TXON_IOMUX_INDEX == 17)
        iomux->REG_008 = SET_BITFIELD(iomux->REG_008, IOMUX_GPIO_P17_SEL, 3);
#else
#endif
    }

    iomux_unlock(lock);
}

static void _hal_iomux_config_spi_slave(uint32_t val)
{
    uint32_t lock;
    uint32_t reg_val;

    lock = iomux_lock();

    iomux->REG_008 = (iomux->REG_008 & ~(IOMUX_GPIO_P14_SEL_MASK | IOMUX_GPIO_P15_SEL_MASK |
        IOMUX_GPIO_P16_SEL_MASK | IOMUX_GPIO_P17_SEL_MASK)) | IOMUX_GPIO_P14_SEL(val) | IOMUX_GPIO_P15_SEL(val) |
        IOMUX_GPIO_P16_SEL(val) | IOMUX_GPIO_P17_SEL(val);

    if (val == IOMUX_FUNC_VAL_GPIO) {
        reg_val = 0;
    } else {
        reg_val = IOMUX_SPI_INTR_RESETN | IOMUX_SPI_INTR_MASK;
    }

    iomux->REG_0C4 = reg_val;

    iomux_unlock(lock);
}

void hal_iomux_set_spi_slave(void)
{
    _hal_iomux_config_spi_slave(7);
}

void hal_iomux_clear_spi_slave(void)
{
    _hal_iomux_config_spi_slave(IOMUX_FUNC_VAL_GPIO);
}

int WEAK hal_pwrkey_set_irq(enum HAL_PWRKEY_IRQ_T type)
{
    return 0;
}

bool hal_pwrkey_pressed(void)
{
    uint32_t v = iomux->REG_040;
    return !!(v & IOMUX_PWR_KEY_VAL);
}

bool hal_pwrkey_startup_pressed(void)
{
    return hal_pwrkey_pressed();
}

enum HAL_PWRKEY_IRQ_T WEAK hal_pwrkey_get_irq_state(void)
{
    enum HAL_PWRKEY_IRQ_T state = HAL_PWRKEY_IRQ_NONE;

    return state;
}

const struct HAL_IOMUX_PIN_FUNCTION_MAP iomux_tport[] = {
/*    {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},*/
    {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
};

int hal_iomux_tportopen(void)
{
    int i;

    for (i=0;i<sizeof(iomux_tport)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP);i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&iomux_tport[i], 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)iomux_tport[i].pin, HAL_GPIO_DIR_OUT, 0);
    }
    return 0;
}

int hal_iomux_tportset(int port)
{
    hal_gpio_pin_set((enum HAL_GPIO_PIN_T)iomux_tport[port].pin);
    return 0;
}

int hal_iomux_tportclr(int port)
{
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)iomux_tport[port].pin);
    return 0;
}

void hal_iomux_set_codec_gpio_trigger(enum HAL_IOMUX_PIN_T pin, bool polarity)
{
    iomux->REG_064 = SET_BITFIELD(iomux->REG_064, IOMUX_CFG_CODEC_TRIG_SEL, pin);
    if (polarity) {
        iomux->REG_064 &= ~IOMUX_CFG_CODEC_TRIG_POL;
    } else {
        iomux->REG_064 |= IOMUX_CFG_CODEC_TRIG_POL;
    }
}

void hal_iomux_set_sdmmc0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux[] = {
        {HAL_IOMUX_PIN_P6_0, HAL_IOMUX_FUNC_SDMMC0_CLK,   SDMMC0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},//No pull-up required
        {HAL_IOMUX_PIN_P6_1, HAL_IOMUX_FUNC_SDMMC0_CMD,   SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_0, HAL_IOMUX_FUNC_SDMMC0_DATA0, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_1, HAL_IOMUX_FUNC_SDMMC0_DATA1, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_2, HAL_IOMUX_FUNC_SDMMC0_DATA2, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_3, HAL_IOMUX_FUNC_SDMMC0_DATA3, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
#ifdef SDMMC0_IOMUX_8WIRE
        {HAL_IOMUX_PIN_P7_4, HAL_IOMUX_FUNC_SDMMC0_DATA4, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_5, HAL_IOMUX_FUNC_SDMMC0_DATA5, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_6, HAL_IOMUX_FUNC_SDMMC0_DATA6, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
        {HAL_IOMUX_PIN_P7_7, HAL_IOMUX_FUNC_SDMMC0_DATA7, SDMMC0_VOLTAGE_SEL, SDMMC0_PULLUP_SEL},
#endif
    };

#ifdef SDMMC0_IO_DRV
    for (uint8_t i = 0; i < sizeof(pinmux) / sizeof(pinmux[0]); i++) {
        hal_iomux_set_io_drv(pinmux[i].pin, SDMMC0_IO_DRV);
    }
#endif

    hal_iomux_init(pinmux, ARRAY_SIZE(pinmux));
}

void hal_iomux_clear_sdmmc0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux[] = {
        {HAL_IOMUX_PIN_P6_0, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P6_1, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_0, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_1, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_2, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_3, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
#ifdef SDMMC0_IOMUX_8WIRE
        {HAL_IOMUX_PIN_P7_4, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_5, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_6, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P7_7, HAL_IOMUX_FUNC_AS_GPIO, SDMMC0_VOLTAGE_SEL, SDMMC0_CLEAR_RES_SEL},
#endif
    };

    hal_iomux_init(pinmux, ARRAY_SIZE(pinmux));
}

void hal_iomux_set_sdmmc0_dt_n_out_group(int enable)
{
}

void hal_iomux_set_sdmmc1(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux[] = {
#if (SDMMC1_IOMUX_INDEX == 12)
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_SDMMC1_CLK,   SDMMC1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},//No pull-up required
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_SDMMC1_CMD,   SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_0, HAL_IOMUX_FUNC_SDMMC1_DATA0, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_1, HAL_IOMUX_FUNC_SDMMC1_DATA1, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_2, HAL_IOMUX_FUNC_SDMMC1_DATA2, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_3, HAL_IOMUX_FUNC_SDMMC1_DATA3, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_4, HAL_IOMUX_FUNC_SDMMC1_DATA4, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_5, HAL_IOMUX_FUNC_SDMMC1_DATA5, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_6, HAL_IOMUX_FUNC_SDMMC1_DATA6, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_7, HAL_IOMUX_FUNC_SDMMC1_DATA7, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
#elif (SDMMC1_IOMUX_INDEX == 36)
        {HAL_IOMUX_PIN_P3_6, HAL_IOMUX_FUNC_SDMMC1_CLK,   SDMMC1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},//No pull-up required
        {HAL_IOMUX_PIN_P3_7, HAL_IOMUX_FUNC_SDMMC1_CMD,   SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_0, HAL_IOMUX_FUNC_SDMMC1_DATA0, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_1, HAL_IOMUX_FUNC_SDMMC1_DATA1, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_2, HAL_IOMUX_FUNC_SDMMC1_DATA2, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_3, HAL_IOMUX_FUNC_SDMMC1_DATA3, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_4, HAL_IOMUX_FUNC_SDMMC1_DATA4, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_5, HAL_IOMUX_FUNC_SDMMC1_DATA5, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_6, HAL_IOMUX_FUNC_SDMMC1_DATA6, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P4_7, HAL_IOMUX_FUNC_SDMMC1_DATA7, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
#else
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_SDMMC1_CLK,   SDMMC1_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},//No pull-up required
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_SDMMC1_CMD,   SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_SDMMC1_DATA0, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_SDMMC1_DATA1, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_SDMMC1_DATA2, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
        {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_SDMMC1_DATA3, SDMMC1_VOLTAGE_SEL, SDMMC1_PULLUP_SEL},
#endif
    };

#ifdef SDMMC1_IO_DRV
    for (uint8_t i = 0; i < sizeof(pinmux) / sizeof(pinmux[0]); i++) {
        hal_iomux_set_io_drv(pinmux[i].pin, SDMMC1_IO_DRV);
    }
#endif

    hal_iomux_init(pinmux, ARRAY_SIZE(pinmux));
}

void hal_iomux_clear_sdmmc1(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux[] = {
#if (SDMMC1_IOMUX_INDEX == 12)
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_0, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_1, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_2, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_3, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_4, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_5, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_6, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_7, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
#elif (SDMMC1_IOMUX_INDEX == 36)
        {HAL_IOMUX_PIN_P3_6, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P3_7, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_0, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_1, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_2, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_3, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_4, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_5, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_6, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P4_7, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
#else
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
        {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_AS_GPIO, SDMMC1_VOLTAGE_SEL, SDMMC1_CLEAR_RES_SEL},
#endif
    };

    hal_iomux_init(pinmux, ARRAY_SIZE(pinmux));
}

void hal_iomux_set_pmu_uart(uint32_t uart)
{
    iomux->REG_048 &= ~IOMUX_DR_PMU_UART_OENB;
    iomux->REG_048 = SET_BITFIELD(iomux->REG_048, IOMUX_SEL_PMU_UART, uart) | IOMUX_EN_PMU_UART;
}

void hal_iomux_clear_pmu_uart(void)
{
    iomux->REG_048 &= ~IOMUX_EN_PMU_UART;
}

void hal_iomux_single_wire_pmu_uart_rx(uint32_t uart)
{
    pmu_led_uart_enable(HAL_IOMUX_PIN_LED2);
    hal_iomux_set_pmu_uart(uart);
}

void hal_iomux_single_wire_pmu_uart_tx(uint32_t uart)
{
}

void hal_iomux_single_wire_sens_uart_rx(uint32_t uart)
{
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart[] = {
        {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_SENS_UART1_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    iomux->REG_0AC &= ~IOMUX_UART1_SEN_HALFN;

    hal_iomux_init(pinmux_uart, ARRAY_SIZE(pinmux_uart));
}

void hal_iomux_single_wire_sens_uart_tx(uint32_t uart)
{
}

void hal_iomux_single_wire_uart_rx(uint32_t uart)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_single_wire_sens_uart_rx(uart);
#else
    hal_iomux_single_wire_pmu_uart_rx(uart);
#endif
}

void hal_iomux_single_wire_uart_tx(uint32_t uart)
{
#ifdef CHIP_SUBSYS_SENS
    hal_iomux_single_wire_sens_uart_tx(uart);
#else
    hal_iomux_single_wire_pmu_uart_tx(uart);
#endif
}

void hal_iomux_set_flash_qspi(void)
{
    uint32_t mask;

#if (NAND_IOMUX_INDEX == 40)
    iomux->REG_07C = (iomux->REG_07C & ~(IOMUX_GPIO_P40_SEL_MASK | IOMUX_GPIO_P41_SEL_MASK |\
                       IOMUX_GPIO_P42_SEL_MASK | IOMUX_GPIO_P43_SEL_MASK | IOMUX_GPIO_P44_SEL_MASK |\
                       IOMUX_GPIO_P45_SEL_MASK)) |
                     IOMUX_GPIO_P40_SEL(7) | IOMUX_GPIO_P41_SEL(7) | IOMUX_GPIO_P42_SEL(7) |
                     IOMUX_GPIO_P43_SEL(7) | IOMUX_GPIO_P44_SEL(7) | IOMUX_GPIO_P45_SEL(7);

    mask = ((1 << (HAL_IOMUX_PIN_P4_0 - HAL_IOMUX_PIN_P4_0)) | (1 << (HAL_IOMUX_PIN_P4_1 - HAL_IOMUX_PIN_P4_0)) | (1 << (HAL_IOMUX_PIN_P4_2 - HAL_IOMUX_PIN_P4_0)) |
           (1 << (HAL_IOMUX_PIN_P4_3 - HAL_IOMUX_PIN_P4_0)) | (1 << (HAL_IOMUX_PIN_P4_4 - HAL_IOMUX_PIN_P4_0)) | (1 << (HAL_IOMUX_PIN_P4_5 - HAL_IOMUX_PIN_P4_0)));

    // Clear pullup
    iomux->REG_094 &= ~mask;

    // Clear pulldown
    iomux->REG_098 &= ~mask;

#else
    iomux->REG_008 = (iomux->REG_008 & ~(IOMUX_GPIO_P12_SEL_MASK | IOMUX_GPIO_P13_SEL_MASK |\
                       IOMUX_GPIO_P14_SEL_MASK | IOMUX_GPIO_P15_SEL_MASK | IOMUX_GPIO_P16_SEL_MASK |\
                       IOMUX_GPIO_P17_SEL_MASK)) |
                     IOMUX_GPIO_P12_SEL(4) | IOMUX_GPIO_P13_SEL(4) | IOMUX_GPIO_P14_SEL(4) |
                     IOMUX_GPIO_P15_SEL(4) | IOMUX_GPIO_P16_SEL(4) | IOMUX_GPIO_P17_SEL(4);

    mask = ((1 << HAL_IOMUX_PIN_P1_2) | (1 << HAL_IOMUX_PIN_P1_3) | (1 << HAL_IOMUX_PIN_P1_4) |
           (1 << HAL_IOMUX_PIN_P1_5) | (1 << HAL_IOMUX_PIN_P1_6) | (1 << HAL_IOMUX_PIN_P1_7));

    // Clear pullup
    iomux->REG_02C &= ~mask;

    // Clear pulldown
    iomux->REG_030 &= ~mask;

#endif

    // Set oenb
    iomux->REG_0AC |= (IOMUX_QSPI_NAND_WM3_0 | IOMUX_QSPI_NAND_WM3_1 | IOMUX_QSPI_NAND_WM3_2 | IOMUX_QSPI_NAND_WM3_3);

}

void hal_iomux_set_dsi_te(void)
{
    uint32_t mask;
    uint32_t lock;

    lock = iomux_lock();

    iomux->REG_08C = (iomux->REG_08C & ~(IOMUX_GPIO_P81_SEL_MASK)) |
        IOMUX_GPIO_P81_SEL(7);

    // Clear pullup
    mask = (1 << (HAL_IOMUX_PIN_P8_1 - HAL_IOMUX_PIN_P8_0));
    iomux->REG_09C &= ~mask;

    // Clear pulldown
    mask = (1 << ((HAL_IOMUX_PIN_P8_1 - HAL_IOMUX_PIN_P8_0) + IOMUX_R_GPIO_P8_PD_SHIFT));
    iomux->REG_09C &= ~mask;

    iomux_unlock(lock);
}

enum HAL_IOMUX_PIN_T hal_iomux_get_dsi_te_pin(void)
{
    return HAL_IOMUX_PIN_P8_1;
}

enum HAL_IOMUX_PIN_T hal_iomux_get_uart_rx_pin(void)
{
#if (DEBUG_PORT == 1)
    return HAL_IOMUX_PIN_P2_2;
#elif (DEBUG_PORT == 2)
    return HAL_IOMUX_PIN_P2_0;
#elif (DEBUG_PORT == 3)
    return HAL_IOMUX_PIN_P6_4;
#else
    return HAL_IOMUX_PIN_LED_NUM;
#endif
}

void hal_iomux_set_qspilcd0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux[] =
    {
        {HAL_GPIO_PIN_P5_0, HAL_IOMUX_FUNC_QSPILCD_CLK, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
        {HAL_GPIO_PIN_P5_1, HAL_IOMUX_FUNC_QSPILCD_CS0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
        {HAL_GPIO_PIN_P5_2, HAL_IOMUX_FUNC_QSPILCD_DIO0, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
        {HAL_GPIO_PIN_P5_3, HAL_IOMUX_FUNC_QSPILCD_DIO1, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
        {HAL_GPIO_PIN_P5_4, HAL_IOMUX_FUNC_QSPILCD_DIO2, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
        {HAL_GPIO_PIN_P5_5, HAL_IOMUX_FUNC_QSPILCD_DIO3, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},

    };
    uint32_t lock;

    hal_iomux_init(pinmux, ARRAY_SIZE(pinmux));

    lock = iomux_lock();
    //set qspi_lcdc_wm3, enable QSPI_IO0 config as input mode in read stage
    iomux->REG_0AC |= IOMUX_QSPI_LCDC_WM3;

    iomux->REG_0A0 = 0x0fff0000;

    iomux_unlock(lock);
}

void hal_iomux_clock_request_in(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pin_req_in[] = {
        {HAL_IOMUX_PIN_P0_6, HAL_IOMUX_FUNC_CLK_REQ_IN,  HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
    };
    hal_iomux_init(pin_req_in, ARRAY_SIZE(pin_req_in));

    iomux->REG_050 |= IOMUX_CFG_EN_CLK_REQIN;
}

void hal_iomux_clock_request_out(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pin_req_out[] = {
        {HAL_IOMUX_PIN_P0_7, HAL_IOMUX_FUNC_CLK_REQ_OUT,  HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
    };
    hal_iomux_init(pin_req_out, ARRAY_SIZE(pin_req_out));

    iomux->REG_050 |= IOMUX_CFG_EN_CLK_REQOUT;
}

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
#ifndef __HAL_SENSCMU_BEST1502X_H__
#define __HAL_SENSCMU_BEST1502X_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_CMU_DEFAULT_CRYSTAL_FREQ        24000000
#define HAL_CMU_VALID_CRYSTAL_FREQ          { 24000000, 26000000, }

enum HAL_CMU_MOD_ID_T {
    // HCLK/HRST
    HAL_CMU_MOD_H_MCU,          // 0
    HAL_CMU_MOD_H_RAM0,         // 1
    HAL_CMU_MOD_H_RAM1,         // 2
    HAL_CMU_MOD_H_RAM2,         // 3
    HAL_CMU_MOD_H_RAM3,         // 4
    HAL_CMU_MOD_H_RAM4,         // 5
    HAL_CMU_MOD_H_AHB0,         // 6
    HAL_CMU_MOD_H_AHB1,         // 7
    HAL_CMU_MOD_H_AH2H_BT,      // 8
    HAL_CMU_MOD_H_ADMA,         // 9
    HAL_CMU_MOD_H_CODEC,        // 10
    HAL_CMU_MOD_H_SENSOR_ENG0,   // 11
    HAL_CMU_MOD_H_TEP,          // 12
    HAL_CMU_MOD_H_AVS,          // 13
    HAL_CMU_MOD_H_A2A,          // 14
    HAL_CMU_MOD_H_PSPY,         // 15
    HAL_CMU_MOD_H_VAD,          // 16
    HAL_CMU_MOD_H_RAM5,         // 17
    HAL_CMU_MOD_H_RAM6,	        // 18
    HAL_CMU_MOD_H_CP0,          // 19
    HAL_CMU_MOD_H_CP1,	        // 20
    HAL_CMU_MOD_H_CP, 		    // 21
    HAL_CMU_MOD_H_CPUDUMP,		// 22
    HAL_CMU_MOD_H_REAL_FLASH,	// 23
    // PCLK/PRST
    HAL_CMU_MOD_P_CMU,          // 24
    HAL_CMU_MOD_P_WDT,          // 25
    HAL_CMU_MOD_P_TIMER0,       // 26
    HAL_CMU_MOD_P_TIMER1,       // 27
    HAL_CMU_MOD_P_TIMER2,       // 28
    HAL_CMU_MOD_P_I2C0,         // 29
    HAL_CMU_MOD_P_I2C1,         // 30
    HAL_CMU_MOD_P_SPI,          // 31
    HAL_CMU_MOD_P_SLCD,         // 32
    HAL_CMU_MOD_P_SPI_ITN,      // 33
    HAL_CMU_MOD_P_UART0,        // 34
    HAL_CMU_MOD_P_UART1,        // 35
    HAL_CMU_MOD_P_PCM,          // 36
    HAL_CMU_MOD_P_I2S0,         // 37
    HAL_CMU_MOD_P_I2C2,         // 38
    HAL_CMU_MOD_P_I2C3,         // 39
    HAL_CMU_MOD_P_CAP,          // 40
    HAL_CMU_MOD_P_PSPY,         // 41
    // OCLK/ORST
    HAL_CMU_MOD_O_SLEEP,        // 42
    HAL_CMU_MOD_O_WDT,          // 43
    HAL_CMU_MOD_O_TIMER0,       // 44
    HAL_CMU_MOD_O_TIMER1,       // 45
    HAL_CMU_MOD_O_TIMER2,       // 46
    HAL_CMU_MOD_O_I2C0,         // 47
    HAL_CMU_MOD_O_I2C1,         // 48
    HAL_CMU_MOD_O_SPI,          // 49
    HAL_CMU_MOD_O_SLCD,         // 50
    HAL_CMU_MOD_O_SPI_ITN,      // 51
    HAL_CMU_MOD_O_UART0,        // 52
    HAL_CMU_MOD_O_UART1,        // 53
    HAL_CMU_MOD_O_PCM,          // 54
    HAL_CMU_MOD_O_I2S0,         // 55
    HAL_CMU_MOD_O_I2C2,         // 56
    HAL_CMU_MOD_O_I2C3,         // 57
    HAL_CMU_MOD_O_CAP0,         // 58
    HAL_CMU_MOD_O_CAP1,         // 59
    HAL_CMU_MOD_O_CAP2,         // 60
    HAL_CMU_MOD_O_ADC_FREE,     // 61
    HAL_CMU_MOD_O_ADC_CH0,      // 62
    HAL_CMU_MOD_O_ADC_CH1,      // 63
    HAL_CMU_MOD_O_ADC_ANA,      // 64
    HAL_CMU_MOD_O_SADC_ANA,     // 65
    HAL_CMU_MOD_O_REAL_FLASH, 	// 66
    // AON ACLK/ARST
    HAL_CMU_AON_A_CMU,          // 67
    HAL_CMU_AON_A_GPIO0,        // 68
    HAL_CMU_AON_A_GPIO0_INT,    // 69
    HAL_CMU_AON_A_WDT,          // 70
    HAL_CMU_AON_A_PWM,          // 71
    HAL_CMU_AON_A_TIMER0,       // 72
    HAL_CMU_AON_A_PSC,          // 73
    HAL_CMU_AON_A_IOMUX,        // 74
    HAL_CMU_AON_A_APBC,         // 75
    HAL_CMU_AON_A_H2H_MCU,      // 76
    HAL_CMU_AON_A_I2C_SLV,      // 77
    HAL_CMU_AON_A_TZC,          // 78
    HAL_CMU_AON_A_GPIO1,        // 79
    HAL_CMU_AON_A_GPIO1_INT,    // 80
    HAL_CMU_AON_A_GPIO2,        // 81
    HAL_CMU_AON_A_GPIO2_INT,    // 82
    HAL_CMU_AON_A_TIMER1,       // 83
    HAL_CMU_AON_A_PWM1,         // 84
    // AON OCLK/ORST
    HAL_CMU_AON_O_WDT,          // 85
    HAL_CMU_AON_O_TIMER0,       // 86
    HAL_CMU_AON_O_GPIO0,        // 87
    HAL_CMU_AON_O_PWM0,         // 88
    HAL_CMU_AON_O_PWM1,         // 89
    HAL_CMU_AON_O_PWM2,         // 90
    HAL_CMU_AON_O_PWM3,         // 91
    HAL_CMU_AON_O_IOMUX,        // 92
    HAL_CMU_AON_O_GPIO1,        // 93
    HAL_CMU_AON_O_BTAON,        // 94
    HAL_CMU_AON_O_PSC,          // 95
    HAL_CMU_AON_O_GPIO2,        // 96
    HAL_CMU_AON_O_TIMER1,       // 97
    HAL_CMU_AON_O_PWM4,		    // 98
    HAL_CMU_AON_O_PWM5,		    // 99
    HAL_CMU_AON_O_PWM6,		    // 100
    HAL_CMU_AON_O_PWM7,         // 101
    // AON SUBSYS
    HAL_CMU_AON_MCU,            // 102
    HAL_CMU_AON_CODEC,          // 103
    HAL_CMU_AON_SENS,           // 104
    HAL_CMU_AON_BT,             // 105
    HAL_CMU_AON_MCUCPU,         // 106
    HAL_CMU_AON_RESERVED5,      // 107
    HAL_CMU_AON_BTCPU,          // 108
    HAL_CMU_AON_GLOBAL,         // 109

    HAL_CMU_MOD_QTY,

    HAL_CMU_MOD_GLOBAL = HAL_CMU_AON_GLOBAL,

    HAL_CMU_MOD_P_PWM = HAL_CMU_AON_A_PWM,
    HAL_CMU_MOD_P_PWM1 = HAL_CMU_AON_A_PWM1,
    HAL_CMU_MOD_O_PWM0 = HAL_CMU_AON_O_PWM0,
    HAL_CMU_MOD_O_PWM1 = HAL_CMU_AON_O_PWM1,
    HAL_CMU_MOD_O_PWM2 = HAL_CMU_AON_O_PWM2,
    HAL_CMU_MOD_O_PWM3 = HAL_CMU_AON_O_PWM3,
    HAL_CMU_MOD_O_PWM4 = HAL_CMU_AON_O_PWM4,
    HAL_CMU_MOD_O_PWM5 = HAL_CMU_AON_O_PWM5,
    HAL_CMU_MOD_O_PWM6 = HAL_CMU_AON_O_PWM6,
    HAL_CMU_MOD_O_PWM7 = HAL_CMU_AON_O_PWM7,
};

enum HAL_CMU_CLOCK_OUT_ID_T {
    HAL_CMU_CLOCK_OUT_AON_32K           = 0x00,
    HAL_CMU_CLOCK_OUT_AON_OSC           = 0x01,
    HAL_CMU_CLOCK_OUT_AON_OSCX2         = 0x02,
    HAL_CMU_CLOCK_OUT_AON_DIG_OSCX2     = 0x03,
    HAL_CMU_CLOCK_OUT_AON_DIG_OSCX4     = 0x04,
    HAL_CMU_CLOCK_OUT_AON_SYS           = 0x05,
    HAL_CMU_CLOCK_OUT_AON_SYS_2         = 0x06,
    HAL_CMU_CLOCK_OUT_AON_DCDC          = 0x07,

    HAL_CMU_CLOCK_OUT_BT_NONE           = 0x40,
    HAL_CMU_CLOCK_OUT_BT_32K            = 0x41,
    HAL_CMU_CLOCK_OUT_BT_SYS            = 0x42,
    HAL_CMU_CLOCK_OUT_BT_48M            = 0x43,
    HAL_CMU_CLOCK_OUT_BT_12M            = 0x44,
    HAL_CMU_CLOCK_OUT_BT_ADC            = 0x45,
    HAL_CMU_CLOCK_OUT_BT_ADCD3          = 0x46,
    HAL_CMU_CLOCK_OUT_BT_DAC            = 0x47,
    HAL_CMU_CLOCK_OUT_BT_DACD2          = 0x48,
    HAL_CMU_CLOCK_OUT_BT_DACD4          = 0x49,
    HAL_CMU_CLOCK_OUT_BT_DACD8          = 0x50,

    HAL_CMU_CLOCK_OUT_MCU_32K           = 0x60,
    HAL_CMU_CLOCK_OUT_MCU_SYS           = 0x61,
    HAL_CMU_CLOCK_OUT_MCU_FLASH0        = 0x62,
    HAL_CMU_CLOCK_OUT_MCU_USB           = 0x63,
    HAL_CMU_CLOCK_OUT_MCU_PCLK          = 0x64,
    HAL_CMU_CLOCK_OUT_MCU_I2S0          = 0x65,
    HAL_CMU_CLOCK_OUT_MCU_PCM           = 0x66,
    HAL_CMU_CLOCK_OUT_MCU_SPDIF0        = 0x67,
    HAL_CMU_CLOCK_OUT_MCU_SDMMC         = 0x68,
    HAL_CMU_CLOCK_OUT_MCU_SPI2          = 0x69,
    HAL_CMU_CLOCK_OUT_MCU_SPI1          = 0x6A,
    HAL_CMU_CLOCK_OUT_MCU_FLASH1        = 0x6B,
    HAL_CMU_CLOCK_OUT_MCU_I2S1          = 0x6C,
    HAL_CMU_CLOCK_OUT_MCU_PIX           = 0x6D,
    HAL_CMU_CLOCK_OUT_MCU_DSI           = 0x6E,

    HAL_CMU_CLOCK_OUT_CODEC_ADC_ANA     = 0x80,
    HAL_CMU_CLOCK_OUT_CODEC_CODEC       = 0x81,
    HAL_CMU_CLOCK_OUT_CODEC_ANC_IIR     = 0x82,
    HAL_CMU_CLOCK_OUT_CODEC_RS_DAC      = 0x83,
    HAL_CMU_CLOCK_OUT_CODEC_RS_ADC      = 0x84,
    HAL_CMU_CLOCK_OUT_CODEC_FIR         = 0x85,
    HAL_CMU_CLOCK_OUT_CODEC_PSAP        = 0x86,
    HAL_CMU_CLOCK_OUT_CODEC_EQ_IIR      = 0x87,
    HAL_CMU_CLOCK_OUT_CODEC_HCLK        = 0x88,

    HAL_CMU_CLOCK_OUT_SENS_32K          = 0xA0,
    HAL_CMU_CLOCK_OUT_SENS_SYS          = 0xA1,
    HAL_CMU_CLOCK_OUT_SENS_PCLK         = 0xA2,
    HAL_CMU_CLOCK_OUT_SENS_PCM          = 0xA3,
    HAL_CMU_CLOCK_OUT_SENS_SPI2         = 0xA4,
    HAL_CMU_CLOCK_OUT_SENS_SPI0         = 0xA5,
    HAL_CMU_CLOCK_OUT_SENS_SPI1         = 0xA6,
    HAL_CMU_CLOCK_OUT_SENS_I2C          = 0xA7,
    HAL_CMU_CLOCK_OUT_SENS_UART0        = 0xA8,
    HAL_CMU_CLOCK_OUT_SENS_JTAG         = 0xA9,
    HAL_CMU_CLOCK_OUT_SENS_CAP          = 0xAA,
    HAL_CMU_CLOCK_OUT_SENS_ADC_ANA      = 0xAB,
    HAL_CMU_CLOCK_OUT_SENS_SAR_ADC_ANA  = 0xAC,
    HAL_CMU_CLOCK_OUT_SENS_CODEC        = 0xAD,
    HAL_CMU_CLOCK_OUT_SENS_CODEC_HCLK   = 0xAE,
    HAL_CMU_CLOCK_OUT_SENS_I2S          = 0xAF,
};

enum HAL_CMU_I2S_MCLK_ID_T {
    HAL_CMU_I2S_MCLK_PLLI2S0            = 0x00,
    HAL_CMU_I2S_MCLK_PLLPCM             = 0x01,
};

enum HAL_PWM_ID_T {
    HAL_PWM_ID_0,
    HAL_PWM_ID_1,
    HAL_PWM_ID_2,
    HAL_PWM_ID_3,
    HAL_PWM1_ID_0,
    HAL_PWM1_ID_1,
    HAL_PWM1_ID_2,
    HAL_PWM1_ID_3,

    HAL_PWM_ID_QTY
};
#define HAL_PWM_ID_T                        HAL_PWM_ID_T

enum HAL_I2S_ID_T {
    HAL_I2S_ID_0 = 0,
    HAL_I2S_ID_1,

    HAL_I2S_ID_QTY,
};
#define HAL_I2S_ID_T                        HAL_I2S_ID_T

enum HAL_CMU_ANC_CLK_USER_T {
    HAL_CMU_ANC_CLK_USER_ANC,

    HAL_CMU_ANC_CLK_USER_QTY
};

enum HAL_CMU_FREQ_T {
    HAL_CMU_FREQ_32K,
    HAL_CMU_FREQ_6P5M,
    HAL_CMU_FREQ_13M,
    HAL_CMU_FREQ_26M,
    HAL_CMU_FREQ_52M,
    HAL_CMU_FREQ_78M,
    HAL_CMU_FREQ_104M,
    HAL_CMU_FREQ_208M,

    HAL_CMU_FREQ_QTY
};
#define HAL_CMU_FREQ_T                      HAL_CMU_FREQ_T

enum HAL_CMU_PLL_T {
    HAL_CMU_PLL_BB = 0,
    HAL_CMU_PLL_AUD = HAL_CMU_PLL_BB,

    HAL_CMU_PLL_QTY
};
#define HAL_CMU_PLL_T                       HAL_CMU_PLL_T

enum HAL_CMU_PLL_USER_T {
    HAL_CMU_PLL_USER_SYS,
    HAL_CMU_PLL_USER_AUD,

    HAL_CMU_PLL_USER_QTY,
    HAL_CMU_PLL_USER_ALL = HAL_CMU_PLL_USER_QTY,
};
#define HAL_CMU_PLL_USER_T                  HAL_CMU_PLL_USER_T

int hal_cmu_fast_timer_offline(void);

enum HAL_CMU_PLL_T hal_cmu_get_audio_pll(void);

void hal_cmu_codec_high_speed_enable(void);

void hal_cmu_codec_high_speed_disable(void);

void hal_cmu_anc_enable(enum HAL_CMU_ANC_CLK_USER_T user);

void hal_cmu_anc_disable(enum HAL_CMU_ANC_CLK_USER_T user);

int hal_cmu_anc_get_status(enum HAL_CMU_ANC_CLK_USER_T user);

void hal_cmu_mcu_pdm_clock_out(uint32_t clk_map);

void hal_cmu_sens_pdm_clock_out(uint32_t clk_map);

uint32_t hal_cmu_get_aon_chip_id(void);

uint32_t hal_cmu_get_aon_revision_id(void);

void hal_cmu_beco_enable(void);

void hal_cmu_beco_disable(void);

void hal_cmu_cp_beco_enable(void);

void hal_cmu_cp_beco_disable(void);

void hal_cmu_cp_enable(uint32_t sp, uint32_t entry);

void hal_cmu_cp_disable(void);

uint32_t hal_cmu_cp_get_entry_addr(void);

int hal_cmu_sys_is_using_audpll(void);

void hal_cmu_cap_sens_clock_enable(void);

void hal_cmu_cap_sens_clock_disable(void);

#ifdef __cplusplus
}
#endif

#endif


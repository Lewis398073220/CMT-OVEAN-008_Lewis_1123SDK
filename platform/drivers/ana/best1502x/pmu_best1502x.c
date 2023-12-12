/***************************************************************************pmu_charger_out_poweron_init
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
#include "pmu.h"
#include CHIP_SPECIFIC_HDR(reg_pmu)
#include CHIP_SPECIFIC_HDR(rf_xtal)
#include "analog.h"
#include "cmsis.h"
#include "cmsis_nvic.h"
#include "hal_aud.h"
#include "hal_bootmode.h"
#include "hal_cache.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "patch.h"
#include "tgt_hardware.h"
#include "usbphy.h"

#if !defined(DIG_DCDC_MODE) && !defined(PMU_LDO_VCORE_CALIB)
#define PMU_LDO_VCORE_CALIB
#endif

#ifdef PSRAM_ENABLE
#define PMU_OPEN_BSS_LOC                BOOT_BSS_LOC
#else
#define PMU_OPEN_BSS_LOC
#endif

#ifdef PMU_IRQ_UNIFIED
#define PMU_IRQ_HDLR_PARAM              uint16_t irq_status
#else
#define PMU_IRQ_HDLR_PARAM              void
#endif

//#define PMU_FIRST_ACIN_IRQ_DISABLE

// Only sensor cpu use RC clock.
// Sensor cpu never touch PSRAM during work with RC clock.
// Fixed RC mode VCORE setting by the following macros:
// PMU_VCORE_RC_MODE_FIXED_VDIG PMU_VCORE_RC_MODE_FIXED_DCDC_DIG PMU_VCORE_RC_MODE_FIXED_VDIG_L.
//#define PMU_VCORE_RC_MODE_FIXED

#define ana_read(reg,val)               hal_analogif_reg_read(ANA_REG(reg),val)
#define ana_write(reg,val)              hal_analogif_reg_write(ANA_REG(reg),val)
#define rf_read(reg,val)                hal_analogif_reg_read(RF_REG(reg),val)
#define rf_write(reg,val)               hal_analogif_reg_write(RF_REG(reg),val)

// LDO soft start interval is about 1000 us
#define PMU_LDO_PU_STABLE_TIME_US       1800
#define PMU_LDO_PULLDOWN_STABLE_TIME_US 1000
#define PMU_DCDC_PU_STABLE_TIME_US      100
#define PMU_VANA_STABLE_TIME_US         10
#define PMU_VCORE_STABLE_TIME_US        10
#define PMU_BIG_BG_STABLE_TIME_US       200

// External 32.768KHz stable time(ms)
#define PMU_EXT_32K_STABLE_TIME         300
#define PMU_EXT_32K_CLK_OUT_STABLE_TIME 1
#define PMU_CLK_SWITCH_STABLE_TIME      1
#define PMU_EXT_32K_CAPBIT_DEFAULT      0x1E0

#define PMU_DCDC_ANA_1_8V               0x9F
#define PMU_DCDC_ANA_1_7V               0x91
#define PMU_DCDC_ANA_1_6V               0x84
#define PMU_DCDC_ANA_1_5V               0x77
#define PMU_DCDC_ANA_1_4V               0x69
#define PMU_DCDC_ANA_1_35V              0x63
#define PMU_DCDC_ANA_1_3V               0x5C
#define PMU_DCDC_ANA_1_2V               0x4F
#define PMU_DCDC_ANA_1_1V               0x41
#define PMU_DCDC_ANA_1_0V               0x34

#define PMU_DCDC_ANA_SLEEP_1_3V         PMU_DCDC_ANA_1_3V
#define PMU_DCDC_ANA_SLEEP_1_2V         PMU_DCDC_ANA_1_2V
#define PMU_DCDC_ANA_SLEEP_1_1V         PMU_DCDC_ANA_1_1V
#define PMU_DCDC_ANA_SLEEP_1_0V         PMU_DCDC_ANA_1_0V

#define PMU_DCDC_DIG_1_1V               0xD4
#define PMU_DCDC_DIG_1_05V              0xC7
#define PMU_DCDC_DIG_1_0V               0xBA
#define PMU_DCDC_DIG_0_95V              0xAC
#define PMU_DCDC_DIG_0_9V               0x9F
#define PMU_DCDC_DIG_0_85V              0x92
#define PMU_DCDC_DIG_0_84V              0x8F
#define PMU_DCDC_DIG_0_83V              0x8C
#define PMU_DCDC_DIG_0_82V              0x8A
#define PMU_DCDC_DIG_0_8V               0x84
#define PMU_DCDC_DIG_0_75V              0x77
#define PMU_DCDC_DIG_0_74V              0x74
#define PMU_DCDC_DIG_0_73V              0x71
#define PMU_DCDC_DIG_0_72V              0x6F
#define PMU_DCDC_DIG_0_71V              0x6C
#define PMU_DCDC_DIG_0_7V               0x6A
#define PMU_DCDC_DIG_0_6V               0x50
#define PMU_DCDC_DIG_50MV               (PMU_DCDC_DIG_0_8V - PMU_DCDC_DIG_0_75V)

#define PMU_DCDC_HPPA_1_9V              0xAC
#define PMU_DCDC_HPPA_1_8V              0x9F
#define PMU_DCDC_HPPA_1_75V             0x98
#define PMU_DCDC_HPPA_1_7V              0x91
#define PMU_DCDC_HPPA_1_65V             0x8B
#define PMU_DCDC_HPPA_1_6V              0x84

#define PMU_VDIG_1_35V                  0xF
#define PMU_VDIG_1_3V                   0xE
#define PMU_VDIG_1_25V                  0xD
#define PMU_VDIG_1_2V                   0xC
#define PMU_VDIG_1_15V                  0xB
#define PMU_VDIG_1_1V                   0xA
#define PMU_VDIG_1_05V                  0x9
#define PMU_VDIG_1_0V                   0x8
#define PMU_VDIG_0_95V                  0x7
#define PMU_VDIG_0_9V                   0x6
#define PMU_VDIG_0_85V                  0x5
#define PMU_VDIG_0_8V                   0x4
#define PMU_VDIG_0_75V                  0x3
#define PMU_VDIG_0_7V                   0x2
#define PMU_VDIG_0_65V                  0x1
#define PMU_VDIG_0_6V                   0x0
#define PMU_VDIG_MAX                    PMU_VDIG_1_2V
#define PMU_VDIG_DEFAULT                0x4 /* 0.8V */

#define PMU_VDIG_L_1_1V                 0xFF
#define PMU_VDIG_L_1_05V                0xFF
#define PMU_VDIG_L_1_0V                 0xFF
#define PMU_VDIG_L_0_95V                0xF0
#define PMU_VDIG_L_0_9V                 0xDE
#define PMU_VDIG_L_0_85V                0xCC
#define PMU_VDIG_L_0_8V                 0xBA
#define PMU_VDIG_L_0_75V                0xA8
#define PMU_VDIG_L_0_7V                 0x96
#define PMU_VDIG_L_0_65V                0x84
#define PMU_VDIG_L_0_6V                 0x72
#define PMU_VDIG_L_0_55V                0x60
#define PMU_VDIG_L_0_5V                 0x4E
#define PMU_VDIG_L_0_45V                0x3C

#define PMU_LDO_SENSOR_3_3V             0x2A
#define PMU_LDO_SENSOR_3_2V             0x28
#define PMU_LDO_SENSOR_3_1V             0x26
#define PMU_LDO_SENSOR_3_0V             0x24
#define PMU_LDO_SENSOR_2_9V             0x22
#define PMU_LDO_SENSOR_2_8V             0x20
#define PMU_LDO_SENSOR_2_7V             0x1E
#define PMU_LDO_SENSOR_2_6V             0x1C
#define PMU_LDO_SENSOR_2_5V             0x1A
#define PMU_LDO_SENSOR_2_4V             0x18
#define PMU_LDO_SENSOR_2_3V             0x16
#define PMU_LDO_SENSOR_2_2V             0x14
#define PMU_LDO_SENSOR_2_1V             0x12
#define PMU_LDO_SENSOR_2_0V             0x10
#define PMU_LDO_SENSOR_1_9V             0xE
#define PMU_LDO_SENSOR_1_8V             0xC
#define PMU_LDO_SENSOR_1_7V             0xA
#define PMU_LDO_SENSOR_1_6V             0x8

#define PMU_LDO_PA_MAX                  PMU_LDO_PA_3_0V
#define PMU_LDO_PA_3_0V                 0x9
#define PMU_LDO_PA_2_9V                 0x8
#define PMU_LDO_PA_2_8V                 0x7
#define PMU_LDO_PA_2_7V                 0x6
#define PMU_LDO_PA_2_6V                 0x5
#define PMU_LDO_PA_2_5V                 0x4
#define PMU_LDO_PA_2_4V                 0x3
#define PMU_LDO_PA_2_3V                 0x2
#define PMU_LDO_PA_2_2V                 0x1
#define PMU_LDO_PA_2_1V                 0x0

#define PMU_LDO_GP_3_3V                 0x2A
#define PMU_LDO_GP_3_2V                 0x28
#define PMU_LDO_GP_3_1V                 0x26
#define PMU_LDO_GP_3_0V                 0x24
#define PMU_LDO_GP_2_9V                 0x22
#define PMU_LDO_GP_2_8V                 0x20
#define PMU_LDO_GP_2_7V                 0x1E
#define PMU_LDO_GP_2_6V                 0x1C
#define PMU_LDO_GP_2_5V                 0x1A
#define PMU_LDO_GP_2_4V                 0x18
#define PMU_LDO_GP_2_3V                 0x16
#define PMU_LDO_GP_2_2V                 0x14
#define PMU_LDO_GP_2_1V                 0x12
#define PMU_LDO_GP_2_0V                 0x10
#define PMU_LDO_GP_1_9V                 0xE
#define PMU_LDO_GP_1_8V                 0xC
#define PMU_LDO_GP_1_7V                 0xA

#define PMU_VMIC_2_8V                   0x33
#define PMU_VMIC_2_7V                   0x30
#define PMU_VMIC_2_6V                   0x2D
#define PMU_VMIC_1_6V                   0x13

#define PMU_VMIC_RES_3_3V               0xF
#define PMU_VMIC_RES_2_8V               0xA

#define PMU_CODEC_1_9V                  0x7
#define PMU_CODEC_1_8V                  0x6
#define PMU_CODEC_1_7V                  0x5
#define PMU_CODEC_1_6V                  0x4

#define PMU_USB_3_3V                    0xC
#define PMU_USB_3_2V                    0xB
#define PMU_USB_3_1V                    0xA
#define PMU_USB_3_0V                    0x9
#define PMU_USB_2_9V                    0x8
#define PMU_USB_2_8V                    0x7
#define PMU_USB_2_7V                    0x6
#define PMU_USB_2_6V                    0x5
#define PMU_USB_2_5V                    0x4
#define PMU_USB_2_4V                    0x3

#define MAX_BUCK_VANA_BIT_VAL                       (BUCK_VANA_BIT_NORMAL_MASK >> BUCK_VANA_BIT_NORMAL_SHIFT)
#define MAX_BUCK_VCORE_BIT_VAL                      (BUCK_VCORE_BIT_NORMAL_MASK >> BUCK_VCORE_BIT_NORMAL_SHIFT)
#define MAX_BUCK_VHPPA_BIT_VAL                      (REG_BUCK_HPPA_VBIT_NORMAL_MASK >> REG_BUCK_HPPA_VBIT_NORMAL_SHIFT)

#define INTR_MSKED_CHARGE_OUT                       INTR_MSKED_CHARGE(1 << 1)
#define INTR_MSKED_CHARGE_IN                        INTR_MSKED_CHARGE(1 << 0)

// PMU_EFUSE_PAGE_DCCALIB2_L_LP 0xA
#define PMU_EFUSE_SAR_ADC_VREF_CAL_SHIFT            0
#define PMU_EFUSE_SAR_ADC_VREF_CAL_MASK             (0x1F << PMU_EFUSE_SAR_ADC_VREF_CAL_SHIFT)
#define PMU_EFUSE_SAR_ADC_VREF_CAL(n)               BITFIELD_VAL(PMU_EFUSE_SAR_ADC_VREF_CAL, n)
#define PMU_EFUSE_SAR_ADC_VREF_CAL_SIGN             (1 << 5)
#define PMU_EFUSE_SAR_ADC_VREF_CALIBED              (1 << 6)
#define PMU_EFUSE_SAR_ADC_VREF_CAL_LP_SHIFT         7
#define PMU_EFUSE_SAR_ADC_VREF_CAL_LP_MASK          (0x1F << PMU_EFUSE_SAR_ADC_VREF_CAL_LP_SHIFT)
#define PMU_EFUSE_SAR_ADC_VREF_CAL_LP(n)            BITFIELD_VAL(PMU_EFUSE_SAR_ADC_VREF_CAL_LP, n)
#define PMU_EFUSE_SAR_ADC_VREF_CAL_LP_SIGN          (1 << 12)
#define PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_OLD_SHIFT    13
#define PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_OLD_MASK     (0x3 << PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_OLD_SHIFT)
#define PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_OLD(n)       BITFIELD_VAL(PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_OLD, n)

#define PMU_SAR_ADC_VREF_VREG_DEFAULT               0x22

// PMU_EFUSE_PAGE_BUCK_CFG_CAL 0x1D
#define PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_SHIFT        3
#define PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_MASK         (0x7 << PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_SHIFT)
#define PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL(n)           BITFIELD_VAL(PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL, n)
#define PMU_EFUSE_BUCK_ANA_CFG_TYPE_SHIFT           0
#define PMU_EFUSE_BUCK_ANA_CFG_TYPE_MASK            (0x7 << PMU_EFUSE_BUCK_ANA_CFG_TYPE_SHIFT)
#define PMU_EFUSE_BUCK_ANA_CFG_TYPE(n)              BITFIELD_VAL(PMU_EFUSE_BUCK_ANA_CFG_TYPE, n)

// RF_REG_F5
#define REG_BT_BBPLL_DIVN_CODEC_SHIFT               0
#define REG_BT_BBPLL_DIVN_CODEC_MASK                (0x1F << REG_BT_BBPLL_DIVN_CODEC_SHIFT)
#define REG_BT_BBPLL_DIVN_CODEC(n)                  BITFIELD_VAL(REG_BT_BBPLL_DIVN_CODEC, n)
#define REG_RXCALI_REVERSE                          (1 << 5)
#define REG_SIG_LOSS_SHIFT                          6
#define REG_SIG_LOSS_MASK                           (0xF << REG_SIG_LOSS_SHIFT)
#define REG_SIG_LOSS(n)                             BITFIELD_VAL(REG_SIG_LOSS, n)
#define REG_BT_IPTAT_ISEL_SHIFT                     10
#define REG_BT_IPTAT_ISEL_MASK                      (0x3F << REG_BT_IPTAT_ISEL_SHIFT)
#define REG_BT_IPTAT_ISEL(n)                        BITFIELD_VAL(REG_BT_IPTAT_ISEL, n)

// REG_111
#define REG_LP_BBPLL_RSTN_DR                        (1 << 0)
#define REG_LP_BBPLL_RSTN                           (1 << 1)
#define REG_LP_BBPLL_CLK_FBC_EDGE                   (1 << 2)
#define REG_LP_BBPLL_INT_DEC_SEL_SHIFT              3
#define REG_LP_BBPLL_INT_DEC_SEL_MASK               (0x7 << REG_LP_BBPLL_INT_DEC_SEL_SHIFT)
#define REG_LP_BBPLL_INT_DEC_SEL(n)                 BITFIELD_VAL(REG_LP_BBPLL_INT_DEC_SEL, n)
#define REG_LP_BBPLL_DITHER_BYPASS                  (1 << 6)
#define REG_LP_BBPLL_PRESCALER_DEL_SEL_SHIFT        7
#define REG_LP_BBPLL_PRESCALER_DEL_SEL_MASK         (0xF << REG_LP_BBPLL_PRESCALER_DEL_SEL_SHIFT)
#define REG_LP_BBPLL_PRESCALER_DEL_SEL(n)           BITFIELD_VAL(REG_LP_BBPLL_PRESCALER_DEL_SEL, n)
#define REG_LP_BBPLL_FREQ_EN_COMB                   (1 << 11)
#define REG_LP_BBPLL_TRI_STEP_SEL_SHIFT             12
#define REG_LP_BBPLL_TRI_STEP_SEL_MASK              (0x3 << REG_LP_BBPLL_TRI_STEP_SEL_SHIFT)
#define REG_LP_BBPLL_TRI_STEP_SEL(n)                BITFIELD_VAL(REG_LP_BBPLL_TRI_STEP_SEL, n)
#define REG_LP_BBPLL_TRI_FREQ_SEL_SHIFT             14
#define REG_LP_BBPLL_TRI_FREQ_SEL_MASK              (0x3 << REG_LP_BBPLL_TRI_FREQ_SEL_SHIFT)
#define REG_LP_BBPLL_TRI_FREQ_SEL(n)                BITFIELD_VAL(REG_LP_BBPLL_TRI_FREQ_SEL, n)

// REG_113
#define REG_LP_BBPLL_SDM_FREQWORD_15_0_SHIFT        0
#define REG_LP_BBPLL_SDM_FREQWORD_15_0_MASK         (0xFFFF << REG_LP_BBPLL_SDM_FREQWORD_15_0_SHIFT)
#define REG_LP_BBPLL_SDM_FREQWORD_15_0(n)           BITFIELD_VAL(REG_LP_BBPLL_SDM_FREQWORD_15_0, n)

// REG_114
#define REG_LP_BBPLL_SDM_FREQWORD_31_16_SHIFT       0
#define REG_LP_BBPLL_SDM_FREQWORD_31_16_MASK        (0xFFFF << REG_LP_BBPLL_SDM_FREQWORD_31_16_SHIFT)
#define REG_LP_BBPLL_SDM_FREQWORD_31_16(n)          BITFIELD_VAL(REG_LP_BBPLL_SDM_FREQWORD_31_16, n)

// REG_115
#define REG_LP_BBPLL_SDM_FREQWORD_34_32_SHIFT       0
#define REG_LP_BBPLL_SDM_FREQWORD_34_32_MASK        (0x7 << REG_LP_BBPLL_SDM_FREQWORD_34_32_SHIFT)
#define REG_LP_BBPLL_SDM_FREQWORD_34_32(n)          BITFIELD_VAL(REG_LP_BBPLL_SDM_FREQWORD_34_32, n)

// REG_120
#define REG_LP_BBPLL_VCO_SWRC_SHIFT                 3
#define REG_LP_BBPLL_VCO_SWRC_MASK                  (0x3 << REG_LP_BBPLL_VCO_SWRC_SHIFT)
#define REG_LP_BBPLL_VCO_SWRC(n)                    BITFIELD_VAL(REG_LP_BBPLL_VCO_SWRC, n)
#define REG_LP_BBPLL_VCO_SPD_SHIFT                  5
#define REG_LP_BBPLL_VCO_SPD_MASK                   (0x7 << REG_LP_BBPLL_VCO_SPD_SHIFT)
#define REG_LP_BBPLL_VCO_SPD(n)                     BITFIELD_VAL(REG_LP_BBPLL_VCO_SPD, n)
#define REG_LP_BBPLL_DIG_SWRC_SHIFT                 11
#define REG_LP_BBPLL_DIG_SWRC_MASK                  (0x3 << REG_LP_BBPLL_DIG_SWRC_SHIFT)
#define REG_LP_BBPLL_DIG_SWRC(n)                    BITFIELD_VAL(REG_LP_BBPLL_DIG_SWRC, n)

// REG_122
#define REG_LP_BBPLL_LK_RSTB                        (1 << 0)
#define REG_LP_BBPLL_CLKGEN_RST                     (1 << 1)
#define REG_LP_BBPLL_EN_CLK_USB                     (1 << 2)
#define REG_LP_BBPLL_EN_CLK_CODEC                   (1 << 3)
#define REG_LP_BBPLL_DIVN_AD_SHIFT                  4
#define REG_LP_BBPLL_DIVN_AD_MASK                   (0x1F << REG_LP_BBPLL_DIVN_AD_SHIFT)
#define REG_LP_BBPLL_DIVN_AD(n)                     BITFIELD_VAL(REG_LP_BBPLL_DIVN_AD_SHIFT, n)
#define REG_LP_BBPLL_DIG_CLK_FREQ_SEL               (1 << 9)
#define REG_LP_BBPLL_EN_CLK_DIG                     (1 << 10)
#define REG_LP_BBPLL_EN_CLK_BBADC                   (1 << 11)
#define REG_LP_BBPLL_CLKGEN_RST_DR                  (1 << 12)
#define REG_BT_LNA_LG_RIN_DR                        (1 << 13)
#define REG_BT_LNA_LG_RIN_SHIFT                     14
#define REG_BT_LNA_LG_RIN_MASK                      (0x3 << REG_BT_LNA_LG_RIN_SHIFT)
#define REG_BT_LNA_LG_RIN(n)                        BITFIELD_VAL(REG_BT_LNA_LG_RIN_SHIFT, n)

// ANALOG REG_16D
#define REG_ANALOG_PU_OSC                           (1 << 6)

// ANALOG REG_1AD
#define REG_ANALOG_CLK_SEL_DSI                      (1 << 0)
#define REG_ANALOG_EN_CLK_DSI                       (1 << 1)

#ifdef PMU_CLK_USE_EXT_CRYSTAL
#define PMU_CONFIG_SYSTICK_HZ_NOMINAL               (16000)
#define PMU_MAX_CALIB_SYSTICK_HZ                    (PMU_CONFIG_SYSTICK_HZ_NOMINAL * 2)
#define PMU_MIN_CALIB_TICKS                         (30 * (PMU_CONFIG_SYSTICK_HZ_NOMINAL / 1000))
#define PMU_MAX_CALIB_TICKS                         (30 * PMU_CONFIG_SYSTICK_HZ_NOMINAL)
#ifdef FAST_TIMER_COMPENSATE
#define PMU_FAST_TICK_RATIO_NUM                     3
#endif
#endif

#define PMU_LDO_VCODEC_SETTING_DEFAULT              0xB4C6
#define PMU_LDO_VANA_SETTING_DEFAULT                0x2D11
#define PMU_LDO_VCORE_SETTING_DEFAULT               0x2D44

#define PMU_VCORE_RC_MODE_FIXED_VDIG                PMU_VDIG_0_75V
#define PMU_VCORE_RC_MODE_FIXED_DCDC_DIG            PMU_DCDC_DIG_0_75V
#define PMU_VCORE_RC_MODE_FIXED_VDIG_L              PMU_VDIG_L_0_7V

#define PMU_EFUSE_TEMPSENSOR_DEFAULT                25000

enum RF_ANA_REG_T {
    ANA_REG_60                  = 0x60,

    RF_REG_00                   = 0x00,
    RF_REG_F5                   = 0xF5,
    RF_REG_111                  = 0x111,
    RF_REG_113                  = 0x113,
    RF_REG_114                  = 0x114,
    RF_REG_115                  = 0x115,
    RF_REG_11F                  = 0X11F,
    RF_REG_120                  = 0x120,
    RF_REG_122                  = 0x122,
    RF_REG_1F4                  = 0x1F4,
};

enum PMU_VCORE_REQ_T {
    PMU_VCORE_FLASH_WRITE_ENABLED   = (1 << 0),
    PMU_VCORE_FLASH_FREQ_HIGH       = (1 << 1),
    PMU_VCORE_PSRAM_FREQ_HIGH       = (1 << 2),
    PMU_VCORE_PSRAM_FREQ_MEDIUM     = (1 << 3),
    PMU_VCORE_PSRAM_FREQ_MEDIUM_LOW = (1 << 4),
    PMU_VCORE_USB_HS_ENABLED        = (1 << 5),
    PMU_VCORE_SYS_FREQ_MEDIUM_LOW   = (1 << 6),
    PMU_VCORE_SYS_FREQ_MEDIUM       = (1 << 7),
    PMU_VCORE_SYS_FREQ_HIGH         = (1 << 8),
    PMU_VCORE_RS_FREQ_HIGH          = (1 << 9),
    PMU_VCORE_RS_ADC_FREQ_HIGH      = (1 << 10),
    PMU_VCORE_IIR_FREQ_HIGH         = (1 << 11),
    PMU_VCORE_IIR_EQ_FREQ_HIGH      = (1 << 12),
    PMU_VCORE_FIR_FREQ_HIGH         = (1 << 13),
    PMU_VCORE_PMU_RESET             = (1 << 14),
};

enum PMU_VUSB_REQ_T {
    PMU_VUSB_REQ_INIT               = (1 << 0),
    PMU_VUSB_REQ_USB                = (1 << 1),
    PMU_VUSB_REQ_LBRT               = (1 << 2),
};

union BOOT_SETTINGS_T {
    struct {
        unsigned short usb_dld_dis      :1;
        unsigned short uart_dld_en      :1;
        unsigned short uart_trace_en    :1;
        unsigned short pll_dis          :1;
        unsigned short uart_baud_div2   :1;
        unsigned short sec_freq_div2    :1;
        unsigned short crystal_freq     :2;
        unsigned short timeout_div      :2;
        unsigned short uart_connected   :1;
        unsigned short uart_1p8v        :1;
        unsigned short chksum           :4;
    };
    unsigned short reg;
};

enum PMU_MODULE_T {
    PMU_ANA,
    PMU_DIG, // VCORE-M LDO
    PMU_USB,
    PMU_CODEC,
    PMU_GP,
    PMU_SENSOR,
    PMU_PA,
    PMU_CORE_L, // VCORE-L

    PMU_MODULE_QTY,
};

struct PMU_MODULE_CFG_T {
    unsigned short manual_bit;
    unsigned short ldo_en;
    unsigned short lp_en_dr;
    unsigned short lp_en;
    unsigned short dsleep_mode;
    unsigned short dsleep_v;
    unsigned short dsleep_v_shift;
    unsigned short normal_v;
    unsigned short normal_v_shift;
    unsigned short rc_v;
    unsigned short rc_v_shift;
};

#define PMU_MOD_CFG_VAL(m)              { \
    REG_PU_LDO_V##m##_DR, REG_PU_LDO_V##m##_REG, \
    LP_EN_V##m##_LDO_DR, LP_EN_V##m##_LDO_REG, \
    REG_PU_LDO_V##m##_DSLEEP, \
    LDO_V##m##_VBIT_DSLEEP_MASK, LDO_V##m##_VBIT_DSLEEP_SHIFT,\
    LDO_V##m##_VBIT_NORMAL_MASK, LDO_V##m##_VBIT_NORMAL_SHIFT,\
    LDO_V##m##_VBIT_RC_MASK,     LDO_V##m##_VBIT_RC_SHIFT }

static const struct PMU_MODULE_CFG_T pmu_module_cfg[] = {
    PMU_MOD_CFG_VAL(ANA),
    PMU_MOD_CFG_VAL(DIG),
    PMU_MOD_CFG_VAL(USB),
    PMU_MOD_CFG_VAL(CODEC),
    PMU_MOD_CFG_VAL(GPLDO),
    PMU_MOD_CFG_VAL(SENSOR),
    PMU_MOD_CFG_VAL(PA),
    PMU_MOD_CFG_VAL(CORE_L),
};

#define OPT_TYPE                        const

static OPT_TYPE uint8_t ana_act_dcdc =
    PMU_DCDC_ANA_1_3V;

static OPT_TYPE POSSIBLY_UNUSED uint16_t vcodec_mv = (uint16_t)(VCODEC_VOLT * 1000);
static OPT_TYPE POSSIBLY_UNUSED uint16_t vhppa_mv = (uint16_t)(VHPPA_VOLT * 1000);

static enum PMU_POWER_MODE_T BOOT_DATA_LOC pmu_power_mode = PMU_POWER_MODE_NONE;

static enum PMU_VCORE_REQ_T BOOT_BSS_LOC pmu_vcore_req;

PMU_OPEN_BSS_LOC
static enum PMU_VUSB_REQ_T pmu_vusb_req;

static PMU_CHARGER_IRQ_HANDLER_T charger_irq_handler;

#ifdef PMU_IRQ_UNIFIED
static bool gpio_irq_en[2];
static HAL_GPIO_PIN_IRQ_HANDLER gpio_irq_handler[2];

static PMU_WDT_IRQ_HANDLER_T wdt_irq_handler;

static PMU_IRQ_UNIFIED_HANDLER_T pmu_irq_hdlrs[PMU_IRQ_TYPE_QTY];
#endif

static const uint8_t ana_lp_dcdc = ana_act_dcdc;

// Move all the data/bss invovled in pmu_open() to .sram_data/.sram_bss,
// so that pmu_open() can be called at the end of BootInit(),
// for data/bss is initialized after BootInit().
static const uint8_t dig_lp_ldo = PMU_VDIG_0_6V;
static const uint8_t dig_lp_dcdc = PMU_DCDC_DIG_0_6V;
#ifdef SPLIT_CORE_M_CORE_L
static const uint8_t dig_lp_logic = PMU_VDIG_L_0_55V;
#endif

static uint8_t BOOT_DATA_LOC bbpll_codec_div = 32;

static uint32_t BOOT_BSS_LOC pmu_metal_id;

static uint16_t wdt_irq_timer;
static uint16_t wdt_reset_timer;

static uint16_t SRAM_BSS_DEF(dcdc_ramp_map);

static uint16_t ntc_raw = 0;
static int ntc_temperature = 0;
static bool ntc_irq_busy = false;
static PMU_NTC_IRQ_HANDLER_T pmu_ntc_cb;

#if defined(MCU_HIGH_PERFORMANCE_MODE)
static const uint16_t high_perf_freq_mhz =
#if defined(MTEST_ENABLED) && defined(MTEST_CLK_MHZ)
    MTEST_CLK_MHZ;
#else
    320;
#endif
PMU_OPEN_BSS_LOC
static bool high_perf_on;
#endif

#ifdef PMU_FORCE_LP_MODE
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_BATTER_LV = PMU_EFUSE_PAGE_BATTER_LV_LP;
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_BATTER_HV = PMU_EFUSE_PAGE_BATTER_HV_LP;
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_GPADC_LV  = PMU_EFUSE_PAGE_GPADC_LV_LP;
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_GPADC_HV  = PMU_EFUSE_PAGE_GPADC_HV_LP;
#else
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_BATTER_LV = PMU_EFUSE_PAGE_BATTER_LV_ACT;
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_BATTER_HV = PMU_EFUSE_PAGE_BATTER_HV_ACT;
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_GPADC_LV  = PMU_EFUSE_PAGE_GPADC_LV_ACT;
const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_GPADC_HV  = PMU_EFUSE_PAGE_GPADC_HV_ACT;
#endif

#ifdef PMU_FORCE_LP_MODE
static enum PMU_BIG_BANDGAP_USER_T big_bandgap_user_map;
static void pmu_bandgap_cfg_reset(void);
#endif

static enum PMU_BOOT_CAUSE_T BOOT_BSS_LOC pmu_boot_reason = PMU_BOOT_CAUSE_NULL;

static bool BOOT_BSS_LOC pmu_boot_first_pwr_up = false;

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#ifndef HPPA_LDO_ON
static void pmu_hppa_dcdc_to_ldo(void);
#endif
#endif

#ifdef PMU_CLK_USE_EXT_CRYSTAL
#define HAL_BUS_WAKE_LOCK_USER_XTAL_CALIB   HAL_BUS_WAKE_LOCK_USER_30

enum XTAL_32K_CALIB_STATE_T {
    XTAL_32K_CALIB_STATE_NULL,
    XTAL_32K_CALIB_STATE_CALIB,
    XTAL_32K_CALIB_STATE_SET,
    XTAL_32K_CALIB_STATE_ROLLBACK,
};

static enum XTAL_32K_CALIB_STATE_T xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
static uint32_t xtal_32k_calib_time;
static float xtal_32k_calib_prev_hz;
static uint16_t xtal_32k_calib_prev_capbit;
static bool xtal_32k_calib_prev_valid = false;
static const uint32_t xtal_32k_calib_intvl_ms = 200;
static const uint32_t xtal_32k_calib_stable_ms = 300;

static uint32_t BOOT_BSS_LOC pmu_ext_crystal_init_stime;
static uint8_t pmu_ext_crystal_clkout_usr_map;

static void pmu_external_crystal_init(void);
static void pmu_external_crystal_clkout_after_pwroff(void);
#ifdef RTC_ENABLE
static void pmu_rtc_clk_div_reload(void);
static float pmu_sys_tick_hz = PMU_CONFIG_SYSTICK_HZ_NOMINAL;
static uint32_t pmu_slow_val;
static uint32_t pmu_fast_val;
#endif
#endif

#if defined(_AUTO_TEST_)
static bool at_skip_shutdown = false;

void pmu_at_skip_shutdown(bool enable)
{
    at_skip_shutdown = enable;
}
#endif

#ifdef RTC_ENABLE
struct PMU_RTC_CTX_T {
    bool enabled;
    bool alarm_set;
    uint32_t alarm_val;
};

static struct PMU_RTC_CTX_T BOOT_BSS_LOC rtc_ctx;

static PMU_RTC_IRQ_HANDLER_T rtc_irq_handler;

static void BOOT_TEXT_SRAM_LOC pmu_rtc_save_context(void)
{
    if (pmu_rtc_enabled()) {
        rtc_ctx.enabled = true;
        if (pmu_rtc_alarm_status_set()) {
            rtc_ctx.alarm_set = true;
            rtc_ctx.alarm_val = pmu_rtc_get_alarm();
        }
    } else {
        rtc_ctx.enabled = false;
    }
}

static void pmu_rtc_restore_context(void)
{
    uint32_t rtc_val;

    if (pmu_rtc_enabled()) {
        pmu_rtc_enable();
        if (rtc_ctx.alarm_set) {
            rtc_val = pmu_rtc_get();
            if (rtc_val - rtc_ctx.alarm_val <= 1 || rtc_ctx.alarm_val - rtc_val < 5) {
                rtc_ctx.alarm_val = rtc_val + 5;
            }
            pmu_rtc_set_alarm(rtc_ctx.alarm_val);
        }
    }
}
#endif

#ifdef PMU_LDO_VCORE_CALIB
union LDO_DIG_COMP_T {
    struct LDO_DIG_COMP_FIELD_T {
        uint16_t dig_l_v:   5; //vcore0p6:    bit[4:0]: 0 ~ 31
        uint16_t dig_l_f:   1; //bit[5]:      1: negative, 0: positive
        uint16_t dig_l_lp_v:5; //vcore0p6_lp: bit[10:6]: 0 ~ 31
        uint16_t dig_l_lp_f:1; //bit[11]:     1: negative, 0: positive
        uint16_t reserved:  3; //bit[14:12]:  reserved
        uint16_t calib_flag:1; //bit[15]:     flag
    } f;
    uint16_t v;
};

static int8_t PMU_OPEN_BSS_LOC pmu_ldo_dig_l_comp = 0;
static int8_t PMU_OPEN_BSS_LOC pmu_ldo_lp_dig_l_comp = 0;

void pmu_get_ldo_dig_calib_value(void)
{
    union LDO_DIG_COMP_T lg;

    pmu_get_efuse(PMU_EFUSE_PAGE_RESERVED_15, &lg.v);
    if (lg.f.calib_flag) {
        if (lg.f.dig_l_f) {
            pmu_ldo_dig_l_comp = -(int8_t)(lg.f.dig_l_v);
        } else {
            pmu_ldo_dig_l_comp =  (int8_t)(lg.f.dig_l_v);
        }
        if (lg.f.dig_l_lp_f) {
            pmu_ldo_lp_dig_l_comp = -(int8_t)(lg.f.dig_l_lp_v);
        } else {
            pmu_ldo_lp_dig_l_comp =  (int8_t)(lg.f.dig_l_lp_v);
        }

#ifdef FORCE_BIG_BANDGAP
        pmu_ldo_lp_dig_l_comp  = pmu_ldo_dig_l_comp;
#elif defined(PMU_FORCE_LP_MODE)
        pmu_ldo_dig_l_comp  = pmu_ldo_lp_dig_l_comp;
#endif

        TRACE(0, "%s", __func__);
        TRACE(0, "normal VCORE-L=%d", pmu_ldo_dig_l_comp);
        TRACE(0, "lowpwr VCORE-L=%d", pmu_ldo_lp_dig_l_comp);
    } else {
        TRACE(0, "%s No calibration.", __func__);
    }

}
#endif

#ifdef PMU_DCDC_CALIB
union VOLT_COMP_T {
    struct VOLT_COMP_FIELD_T {
        uint16_t dcdc1_v: 5; //bit[4:0]: 0 ~ 31
        uint16_t dcdc1_f: 1; //bit[5]  : 1: negative, 0: positive;
        uint16_t dcdc2_v: 4; //bit[9:6]: 0 ~ 15
        uint16_t dcdc2_f: 1; //bit[10] :
        uint16_t dcdc3_v: 4; //bit[14:11]: 0 ~ 15
        uint16_t dcdc3_f: 1; //bit[15]
    } f;
    uint16_t v;
};

static int8_t PMU_OPEN_BSS_LOC pmu_dcdc_dig_comp = 0;
static int8_t PMU_OPEN_BSS_LOC pmu_dcdc_ana_comp = 0;
static int8_t PMU_OPEN_BSS_LOC pmu_dcdc_hppa_comp = 0;

static int8_t PMU_OPEN_BSS_LOC pmu_dig_lp_comp  = 0;
static int8_t PMU_OPEN_BSS_LOC pmu_ana_lp_comp  = 0;
static int8_t PMU_OPEN_BSS_LOC pmu_hppa_lp_comp = 0;

static POSSIBLY_UNUSED void pmu_get_dcdc_calib_value(void)
{
    union VOLT_COMP_T cv;

    pmu_get_efuse(PMU_EFUSE_PAGE_RESERVED_7, &cv.v);
    if (cv.f.dcdc1_f) { //digital
        pmu_dcdc_dig_comp = -(int8_t)(cv.f.dcdc1_v);
    } else {
        pmu_dcdc_dig_comp = (int8_t)(cv.f.dcdc1_v);
    }
    if (cv.f.dcdc2_f) { //ana
        pmu_dcdc_ana_comp = -(int8_t)(cv.f.dcdc2_v);
    } else {
        pmu_dcdc_ana_comp = (int8_t)(cv.f.dcdc2_v);
    }
    if (cv.f.dcdc3_f) { //hppa
        pmu_dcdc_hppa_comp = -(int8_t)(cv.f.dcdc3_v);
    } else {
        pmu_dcdc_hppa_comp = (int8_t)(cv.f.dcdc3_v);
    }

    pmu_get_efuse(PMU_EFUSE_PAGE_DCCALIB2_L, &cv.v);
    if (cv.f.dcdc1_f) { //digital
        pmu_dig_lp_comp = -(int8_t)(cv.f.dcdc1_v);
    } else {
        pmu_dig_lp_comp = (int8_t)(cv.f.dcdc1_v);
    }
    if (cv.f.dcdc2_f) { //ana
        pmu_ana_lp_comp = -(int8_t)(cv.f.dcdc2_v);
    } else {
        pmu_ana_lp_comp = (int8_t)(cv.f.dcdc2_v);
    }
    if (cv.f.dcdc3_f) { //hppa
        pmu_hppa_lp_comp = -(int8_t)(cv.f.dcdc3_v);
    } else {
        pmu_hppa_lp_comp = (int8_t)(cv.f.dcdc3_v);
    }

#ifdef FORCE_BIG_BANDGAP
    pmu_dig_lp_comp  = pmu_dcdc_dig_comp;
    pmu_ana_lp_comp  = pmu_dcdc_ana_comp;
    pmu_hppa_lp_comp = pmu_dcdc_hppa_comp;
#elif defined(PMU_FORCE_LP_MODE)
    pmu_dcdc_dig_comp  = pmu_dig_lp_comp;
    pmu_dcdc_ana_comp  = pmu_ana_lp_comp;
    pmu_dcdc_hppa_comp = pmu_hppa_lp_comp;
#endif

    TRACE(0, "%s", __func__);
    TRACE(0, "normal VCORE-M=%d VANA=%d VCODEC=%d", pmu_dcdc_dig_comp, pmu_dcdc_ana_comp, pmu_dcdc_hppa_comp);
    TRACE(0, "lowpwr VCORE-M=%d VANA=%d VCODEC=%d", pmu_dig_lp_comp, pmu_ana_lp_comp, pmu_hppa_lp_comp);
}

static POSSIBLY_UNUSED unsigned short pmu_reg_val_add(unsigned short val, int delta, unsigned short max)
{
    int result = val + delta;

    if (result > max) {
        result = max;
    } else if (result < 0) {
        result = 0;
    }

    return (unsigned short)result;
}
#endif

#ifdef PMU_CLK_USE_EXT_CRYSTAL
// Reset PMU power mode after hal wdt.
static void BOOT_TEXT_FLASH_LOC pmu_power_mode_sw_reset(void)
{
    uint16_t val;
    bool dcdc_on = false;

    pmu_read(PMU_REG_BIAS_CFG, &val);
    // lp bandgap
    if (!(val & BG_VBG_SEL_REG)) {
        val |= PU_BIAS_LDO_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR | BG_NOTCH_EN_DR | BG_VBG_SEL_DR |
            PU_BIAS_LDO_REG | BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG | BG_NOTCH_EN_REG;
        pmu_write(PMU_REG_BIAS_CFG, val);
        hal_sys_timer_delay_us(PMU_BIG_BG_STABLE_TIME_US);

        val |= BG_VBG_SEL_REG;
        pmu_write(PMU_REG_BIAS_CFG, val);
    }

    pmu_read(PMU_REG_POWER_STATUS, &val);
    if (val & DIG_PU_DCDC_HPPA) {
        pmu_write(PMU_REG_CODEC_CFG, PMU_LDO_VCODEC_SETTING_DEFAULT);
        hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);
        // Disable dcdc3
        pmu_write(PMU_REG_DCDC_HPPA_EN, PU_DCDC_HPPA_DR);
    }

    if (val & DIG_PU_DCDC_VANA) {
        dcdc_on = true;
        pmu_write(PMU_REG_ANA_CFG, PMU_LDO_VANA_SETTING_DEFAULT);
        hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);
    }

    if (val & DIG_PU_DCDC_VCORE) {
        dcdc_on = true;
        pmu_write(PMU_REG_DIG_CFG, PMU_LDO_VCORE_SETTING_DEFAULT);
        hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);
        pmu_write(PMU_REG_DCDC_DIG_VOLT, PMU_DCDC_DIG_0_8V);
    }

    if (dcdc_on) {
        // Disable dcdc1 dcdc2
        val = PU_DCDC_ANA_DR | PU_DCDC_DIG_DR;
        pmu_write(PMU_REG_DCDC_EN, val);
    }
}
#endif

uint32_t BOOT_TEXT_FLASH_LOC read_hw_metal_id(void)
{
    uint16_t val;
    uint32_t metal_id;

    pmu_read(PMU_REG_METAL_ID, &val);
    pmu_metal_id = GET_BITFIELD(val, REVID);

    metal_id = hal_cmu_get_aon_revision_id();

    return metal_id;
}

static void BOOT_TEXT_FLASH_LOC pmu_boot_cause_init(void)
{
    uint16_t val = 0;
    union HAL_HW_BOOTMODE_T hw_bm;

    hw_bm = hal_hw_bootmode_get();

    if (hw_bm.watchdog) {
        pmu_boot_reason = PMU_BOOT_CAUSE_DIG_WDT;
    } else if (hw_bm.global) {
        pmu_boot_reason = PMU_BOOT_CAUSE_DIG_REBOOT;
    } else {
        pmu_read(PMU_REG_POWER_STATUS, &val);
        val = GET_BITFIELD(val, PMU_LDO_ON_SOURCE);
        if (val == 1) {
            pmu_boot_reason = PMU_BOOT_CAUSE_POWER_KEY;
        } else if (val == 2) {
            pmu_boot_reason = PMU_BOOT_CAUSE_RTC;
        } else if (val == 3) {
            pmu_boot_reason = PMU_BOOT_CAUSE_AC_IN;
        } else if (val == 4) {
            pmu_boot_reason = PMU_BOOT_CAUSE_AC_OUT;
        } else {
            pmu_boot_reason = PMU_BOOT_CAUSE_NULL;
        }
    }

    pmu_read(PMU_REG_NOT_RESET_61, &val);
    if (val & REG_NOT_RESET_PMU_WDT_REBOOT) {
        pmu_boot_reason = PMU_BOOT_CAUSE_PMU_WDT;
    }
    if ((val & REG_NOT_RESET_CHIP_PWR_ON) == 0) {
        pmu_boot_first_pwr_up = true;
    }
#if !(defined(OTA_BOOT_IMAGE) || defined(ARM_CMSE))
    val |= REG_NOT_RESET_CHIP_PWR_ON;
    val &= ~PMU_BOOT_CAUSE_PMU_WDT;
    pmu_write(PMU_REG_NOT_RESET_61, val);
#endif
}

enum PMU_BOOT_CAUSE_T pmu_boot_cause_get(void)
{
    return pmu_boot_reason;
}

bool pmu_boot_first_power_up(void)
{
    return pmu_boot_first_pwr_up;
}

void BOOT_TEXT_FLASH_LOC pmu_boot_init(void)
{
    uint16_t val;

#if !defined(PROGRAMMER) && defined(EFUSE_READ_WORKAROUND)
    if ((hal_sw_bootmode_get() & HAL_SW_BOOTMODE_FORCE_USB_DLD) == 0 &&
            hal_iomux_uart0_connected()) {
        hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD);
        patch_open(0);
        patch_code_enable_id(0, 0x20e94, 0x22012201);
        patch_code_enable_id(1, 0x20e9c, 0xbd10b002);
        hal_cmu_reset_pulse(HAL_CMU_MOD_H_MCU);
    }
#endif

    pmu_boot_cause_init();

#ifdef RTC_ENABLE
    // RTC will be restored in pmu_open()
    pmu_rtc_save_context();
#endif

#ifdef __WATCHER_DOG_RESET__
    pmu_wdt_save_context();
#endif

#if !defined(FPGA) && !defined(PROGRAMMER)
    pmu_charger_save_context();
#endif

#ifdef PMU_CLK_USE_EXT_CRYSTAL
    // Switch to lpo clock before reg reset if pmu clock used ext 32k.
    uint16_t val;

    pmu_read(PMU_REG_MIC_BIAS_C, &val);
    if (!!(val & CLK_32K_SEL_1)) {
        val &= ~CLK_32K_SEL_1;
        pmu_write(PMU_REG_MIC_BIAS_C, val);
        hal_sys_timer_delay(MS_TO_TICKS(PMU_CLK_SWITCH_STABLE_TIME));
    }

    pmu_power_mode_sw_reset();
#endif

    // Reset PMU (to recover from a possible insane state, e.g., ESD reset)
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(US_TO_TICKS(500));

    pmu_read(PMU_REG_CHARGER_CFG, &val);
    val |= REG_CHARGE_INTR_EN;
    pmu_write(PMU_REG_CHARGER_CFG, val);

#ifdef PMU_CLK_USE_EXT_CRYSTAL
    pmu_external_crystal_init();
#endif

#ifdef __WATCHER_DOG_RESET__
    pmu_wdt_restore_context();
#else
    pmu_wdt_stop();
#endif

    pmu_rf_ana_init();
}

static unsigned int NOINLINE BOOT_TEXT_SRAM_LOC pmu_count_zeros(unsigned int val, unsigned int bits)
{
    int cnt = 0;
    int i;

    for (i = 0; i < bits; i++) {
        if ((val & (1 << i)) == 0) {
            cnt++;
        }
    }

    return cnt;
}

void BOOT_TEXT_FLASH_LOC pmu_dsipll_freq_reg_set(uint16_t low, uint16_t high, uint16_t high2)
{
    int ret;
    uint16_t val;

    ret = analog_write(0x1A0, low);
    if (ret) {
        return;
    }

    ret = analog_write(0x1A1, high);
    if (ret) {
        return;
    }

    ret = analog_write(0x1A2, high2);
    if (ret) {
        return;
    }

    ret = analog_read(0x1A8, &val);
    if (ret) {
        return;
    }

    val |= (1 << 13);
    val &= ~(1 << 14);
    analog_write(0x1A8, val);

    val |= (1 << 14);
    analog_write(0x1A8, val);

    // Delay at least for 7us
    hal_sys_timer_delay(US_TO_TICKS(100));
}

void BOOT_TEXT_FLASH_LOC dsipll_freq_pll_config(uint32_t freq)
{
    uint64_t PLL_cfg_val;
    uint16_t v[3];
    uint32_t crystal = hal_cmu_get_crystal_freq();

    PLL_cfg_val = ((uint64_t)(1 << 28) * freq + crystal / 2) / crystal;
    v[0] = PLL_cfg_val & 0xFFFF;
    v[1] = (PLL_cfg_val >> 16) & 0xFFFF;
    v[2] = (PLL_cfg_val >> 32) & 0xFFFF;

    pmu_dsipll_freq_reg_set(v[0], v[1], v[2]);
}

void BOOT_TEXT_FLASH_LOC pmu_pll_freq_reg_set(uint16_t low, uint16_t high, uint16_t high2)
{
    int ret;
    uint16_t val;

    ret = rf_write(RF_REG_113, low);
    if (ret) {
        return;
    }

    ret = rf_write(RF_REG_114, high);
    if (ret) {
        return;
    }

    ret = rf_write(RF_REG_115, high2);
    if (ret) {
        return;
    }

    ret = rf_read(RF_REG_111, &val);
    if (ret) {
        return;
    }
    val |= REG_LP_BBPLL_FREQ_EN_COMB;
    ret = rf_write(RF_REG_111, val);
    if (ret) {
        return;
    }

    // Delay at least for 7us
    hal_sys_timer_delay(US_TO_TICKS(100));
}

void BOOT_TEXT_FLASH_LOC bbpll_freq_pll_config(uint32_t freq)
{
    uint64_t PLL_cfg_val;
    uint16_t v[3];
    uint32_t crystal = hal_cmu_get_crystal_freq();

    PLL_cfg_val = ((uint64_t)(1 << 28) * freq + crystal / 2) / crystal;
    v[0] = PLL_cfg_val & 0xFFFF;
    v[1] = (PLL_cfg_val >> 16) & 0xFFFF;
    v[2] = (PLL_cfg_val >> 32) & 0xFFFF;

    pmu_pll_freq_reg_set(v[0], v[1], v[2]);
}

void bbpll_codec_clock_enable(bool en)
{
    uint16_t val;

    rf_read(RF_REG_122, &val);
    if (en) {
        val |= REG_LP_BBPLL_EN_CLK_CODEC;
    } else {
        val &= ~(REG_LP_BBPLL_EN_CLK_CODEC);
    }
    rf_write(RF_REG_122, val);
}

void bbpll_pll_update(uint64_t pll_cfg_val, bool calib)
{
    uint16_t v[3];
    uint16_t val;
    //bool sys_is_bbpll = false;
    //bool flash_is_bbpll = false;
    //uint32_t lock = 0;

    v[0] = pll_cfg_val & 0xFFFF;
    v[1] = (pll_cfg_val >> 16) & 0xFFFF;
    v[2] = (pll_cfg_val >> 32) & 0xFFFF;

    rf_write(RF_REG_113, v[0]);
    rf_write(RF_REG_114, v[1]);
    rf_read(RF_REG_115, &val);
    val = SET_BITFIELD(val, REG_LP_BBPLL_SDM_FREQWORD_34_32, v[2]);
    rf_write(RF_REG_115, val);

    rf_read(RF_REG_111, &val);
    val |= REG_LP_BBPLL_FREQ_EN_COMB;
    rf_write(RF_REG_111, val);
}

void bbpll_set_codec_div(uint32_t div)
{
    uint16_t val;

    rf_read(RF_REG_122, &val);
    val = SET_BITFIELD(val, REG_LP_BBPLL_DIVN_AD, div);
    rf_write(RF_REG_122, val);
}

void bbpll_set_spd(uint32_t spd)
{
    uint16_t val;

    rf_read(RF_REG_120, &val);
    val = SET_BITFIELD(val, REG_LP_BBPLL_VCO_SPD, spd);
    rf_write(RF_REG_120, val);
}

void bbpll_auto_calib_spd(uint32_t codec_freq, uint32_t div)
{
    const uint32_t sys_freq_div = 4;
    const uint32_t f_med = codec_freq * div / sys_freq_div
#ifndef AUD_PLL_DOUBLE
        / 2
#endif
        ;
    const uint32_t calc_ms = 50;
    const float deviation = (float)f_med / hal_cmu_get_crystal_freq() * (1.0f / CONFIG_FAST_SYSTICK_HZ) / (calc_ms / 1000.0f);
    const uint32_t f_min = f_med - (uint32_t)(f_med * deviation * 1.6f);
    const uint32_t f_max = f_med + (uint32_t)(f_med * deviation * 1.2f);
    uint32_t f;
    uint32_t k = 0;
    uint32_t valid_cnt = 0;
    uint32_t valid_start = 0;
    uint32_t valid_end = 0;
    uint32_t cur_cnt = 0;
    uint32_t sel;
    uint32_t lock;

    lock = int_lock();

    const enum HAL_CMU_FREQ_T org_freq = hal_sysfreq_get_hw_freq();
    // Switch to 52M to avoid using PLL
    hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);
    // Enable PLL
#ifdef ULTRA_LOW_POWER
    hal_cmu_low_freq_mode_disable(org_freq, HAL_CMU_FREQ_208M);
#endif
    // Inc vcore voltage
    pmu_sys_freq_config(HAL_CMU_FREQ_208M);

    analog_aud_freq_pll_config(codec_freq, div);
    TRACE(0,"codec_freq = %d, div = %d", codec_freq, div);
    TRACE(0,"f_med = %d, deviation = %f", f_med, (double)deviation);
    hal_cmu_force_sys_pll_div(sys_freq_div);

    for (k = 0; k < 8; k++) {
        // Switch to 52M to avoid using PLL
        hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);

        bbpll_set_spd(k);

        // Switch to PLL freq
        hal_cmu_sys_set_freq(HAL_CMU_FREQ_208M);

        f = hal_sys_timer_calc_cpu_freq(calc_ms, 1);
        if (f_min < f && f < f_max) {
            cur_cnt++;
        } else {
            if (cur_cnt) {
                if (cur_cnt > valid_cnt) {
                    valid_cnt = cur_cnt;
                    valid_end = k - 1;
                }
                cur_cnt = 0;
            }
        }
        TRACE(0, "[%c%2u] f=%u min=%u max=%u", cur_cnt ? '+' : '-', k, f, f_min, f_max);
    }

    if (cur_cnt) {
        if (cur_cnt > valid_cnt) {
            valid_cnt = cur_cnt;
            valid_end = k - 1;
        }
    }
    if (valid_cnt) {
        valid_start = valid_end + 1 - valid_cnt;
        sel = (valid_start + valid_end) / 2;
        TRACE(0, "Valid spd=[%2u, %2u] cnt=%2u sel=%2u/0x%02X", valid_start, valid_end, valid_cnt, sel, sel);
    } else {
        sel = 0x0;
        TRACE(0, "BAD: no valid spd");
    }
    bbpll_set_spd(sel);
    // Restore the sys freq
    hal_cmu_force_sys_pll_div(0);
    hal_cmu_sys_set_freq(org_freq);
    // Restore PLL status
#ifdef ULTRA_LOW_POWER
    hal_cmu_low_freq_mode_enable(HAL_CMU_FREQ_208M, org_freq);
#endif
    // Restore vcore voltage
    pmu_sys_freq_config(org_freq);

    int_unlock(lock);
}

void BOOT_TEXT_FLASH_LOC pmu_rf_ana_init(void)
{
    int ret;
    uint16_t val;
    union BOOT_SETTINGS_T boot;

    ret = pmu_get_efuse(PMU_EFUSE_PAGE_BOOT, &boot.reg);
    if (ret) {
        boot.reg = 0;
    } else {
        if (pmu_count_zeros(boot.reg, 12) != boot.chksum) {
            boot.reg = 0;
        }
    }
    hal_cmu_set_crystal_freq_index(boot.crystal_freq);
    // Update ISPI cfg
    ret = hal_analogif_open();
    if (ret) {
        SAFE_PROGRAM_STOP();
    }

#if !defined(FPGA) && !defined(PROGRAMMER) && !defined(MCU_HIGH_PERFORMANCE_MODE)
    if (hal_cmu_get_crystal_freq() != hal_cmu_get_default_crystal_freq()) {
        // Update bbpll freq after resetting RF and getting crystal freq
        bbpll_freq_pll_config(384000000);
    }
#endif

#if defined(PMU_FULL_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    // Reset RF
    rf_write(RF_REG_00, 0xCAFE);
    rf_write(RF_REG_00, 0x5FEE);
    hal_sys_timer_delay(US_TO_TICKS(500));

    // Reset ANA
    ana_write(ANA_REG_60, 0xCAFE);
    ana_write(ANA_REG_60, 0x5FEE);
    hal_sys_timer_delay(US_TO_TICKS(500));
#endif

    //enhance xtal drv
    rf_write(0x9D,0x8C6C);
    rf_write(0x9c, 0x1817);

    // Enable 26M doubler (52M)
    rf_write(0xB5, 0x8000);
#ifdef ANA_26M_X4_ENABLE
    rf_write(0x15D, 0x0053);
#endif

    // Power up bbpll and audpll clk buf
    // Clk buf bits:
    // 0-rfpll 1-bt_dac 2-codec_resample&audpll_prechr 3-bbpll 4-audpll 5-usbhspll 6-lbrt 7-dig
    rf_read(0xC4, &val);
    val &= ~0xFF;
    val |= (1 << 2) | (1 << 3) | (1 << 7);
    rf_write(0xC4, val);

    // Cfg bbpll
    rf_write(0x9F, 0xC22F);
    rf_write(0xA0, 0x2788);
    // Close bbpll
    rf_write(0xDF, 0x0001);

    // Clear reg_bt_tst_buf_sel_in/out to avoid impacting P00-P03 and P30-P33 pins
    rf_write(0xA2, 0x01C2);

    // Init pll dividers
    rf_read(RF_REG_F5, &val);
    val = SET_BITFIELD(val, REG_BT_BBPLL_DIVN_CODEC, bbpll_codec_div);
    rf_write(RF_REG_F5, val);

#if defined(BBPLL_MHZ) && (BBPLL_MHZ == 312)
    rf_write(RF_REG_113, 0x0000);
    rf_write(RF_REG_114, 0xd000);
    rf_write(RF_REG_115, 0x0000);
#elif defined(BBPLL_MHZ) && (BBPLL_MHZ == 384)
    rf_write(RF_REG_113, 0x0000);
    rf_write(RF_REG_114, 0x0000);
    rf_write(RF_REG_115, 0x0001);
#endif

    // disable clk gate for all
    analog_read(0x16D, &val);
    val |= REG_ANALOG_PU_OSC;
    analog_write(0x16D, val);

#ifdef DSI_CLK_ENABLE
    bool dsi_div_en = true;
#ifdef DSIPLL_MHZ
#if (DSIPLL_MHZ == 500)
    // 500M
    dsi_div_en = false;
    analog_write(0x1A0, 0x5555);
    analog_write(0x1A1, 0x4d55);
    // analog_write(0x1A2, 0x0001);
    analog_write(0x1A6, 0x20F9);
#elif (DSIPLL_MHZ == 480)
    // 480M
    dsi_div_en = false;
    analog_write(0x1A0, 0x0000);
    analog_write(0x1A1, 0x4000);
    analog_write(0x1A2, 0x0001);
#elif (DSIPLL_MHZ == 400)
    // 393M
    dsi_div_en = false;
    // analog_write(0x1A0, 0xAAAB);
    // analog_write(0x1A1, 0x0AAA);
    // analog_write(0x1A2, 0x0001);
#elif (DSIPLL_MHZ == 384)
    // 384M
    dsi_div_en = false;
    analog_write(0x1A0, 0x0);
    analog_write(0x1A1, 0x0);
    analog_write(0x1A2, 0x0001);
#elif (DSIPLL_MHZ == 320)
    // 320M
    dsi_div_en = false;
    analog_write(0x1A0, 0x5555);
    analog_write(0x1A1, 0xD555);
    analog_write(0x1A2, 0x0000);
#elif (DSIPLL_MHZ == 200)
    // 197M
    // analog_write(0x1A0, 0xAAAB);
    // analog_write(0x1A1, 0x0AAA);
    // analog_write(0x1A2, 0x0001);
#else
#error "Bad DSIPLL_MHZ"
#endif
#endif /* DSIPLL_MHZ */
    analog_read(0x1AD, &val);
    val |= REG_ANALOG_EN_CLK_DSI;
    if (dsi_div_en) {
        val |= REG_ANALOG_CLK_SEL_DSI;
    } else {
        val &= ~REG_ANALOG_CLK_SEL_DSI;
    }
    analog_write(0x1AD, val);
#endif /* DSI_CLK_ENABLE */

    rf_read(RF_REG_120, &val);
    val |= REG_LP_BBPLL_DIG_SWRC(3) | REG_LP_BBPLL_VCO_SWRC(3);
    rf_write(RF_REG_120, val);

    rf_read(RF_REG_111, &val);
    val = SET_BITFIELD(val, REG_LP_BBPLL_INT_DEC_SEL, 0);
    rf_write(RF_REG_111, val);
}

void BOOT_TEXT_FLASH_LOC dsipll_div2_enable(bool is_div)
{
    uint16_t val;
    analog_read(0x1AD, &val);
    if(is_div){
        val |= REG_ANALOG_CLK_SEL_DSI;
    } else {
        val &= ~REG_ANALOG_CLK_SEL_DSI;
    }
    analog_write(0x1AD, val);
}

void BOOT_TEXT_SRAM_LOC pmu_pll_div_reset_set(enum HAL_CMU_PLL_T pll)
{
    uint32_t lock;
    uint16_t val;

    lock = int_lock();
    if (pll == HAL_CMU_PLL_AUD) {
        rf_read(RF_REG_F5, &val);
        val |= REG_BT_BBPLL_DIVN_CODEC_MASK;
        rf_write(RF_REG_F5, val);
    }
    int_unlock(lock);
}

void BOOT_TEXT_SRAM_LOC pmu_pll_div_reset_clear(enum HAL_CMU_PLL_T pll)
{
    uint32_t lock;
    uint16_t val;

    lock = int_lock();
    if (pll == HAL_CMU_PLL_AUD) {
        rf_read(RF_REG_F5, &val);
        val = SET_BITFIELD(val, REG_BT_BBPLL_DIVN_CODEC, bbpll_codec_div);
        rf_write(RF_REG_F5, val);
    }
    int_unlock(lock);
}

void pmu_pll_div_set(enum HAL_CMU_PLL_T pll, enum PMU_PLL_DIV_TYPE_T type, uint32_t div)
{
    uint32_t lock;
    uint16_t val;

    if (type != PMU_PLL_DIV_CODEC) {
        return;
    }

    lock = int_lock();
    if (pll == HAL_CMU_PLL_AUD) {
        if (div != bbpll_codec_div) {
            bbpll_codec_div = div;
            rf_read(RF_REG_F5, &val);
            val |= REG_BT_BBPLL_DIVN_CODEC_MASK;
            rf_write(RF_REG_F5, val);
            if (div != (REG_BT_BBPLL_DIVN_CODEC_MASK >> REG_BT_BBPLL_DIVN_CODEC_SHIFT)) {
                val = SET_BITFIELD(val, REG_BT_BBPLL_DIVN_CODEC, bbpll_codec_div);
                rf_write(RF_REG_F5, val);
            }
        }
    }
    int_unlock(lock);
}

int pmu_get_security_value(union SECURITY_VALUE_T *val)
{
    int ret;

    val->reg = 0;

    ret = pmu_get_efuse(PMU_EFUSE_PAGE_SECURITY, &val->reg);
    if (ret) {
        // Error
        return ret;
    }

    return 0;
}

enum HAL_FLASH_ID_T pmu_get_boot_flash_ctrl_id(void)
{
#ifdef FLASH1_CTRL_BASE
    union SECURITY_VALUE_T security;

    pmu_get_security_value(&security);

    return (security.root.flash_id) ? HAL_FLASH_ID_1 : HAL_FLASH_ID_0;
#else
    return HAL_FLASH_ID_0;
#endif
}

int BOOT_TEXT_SRAM_LOC pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse)
{
    int ret;
    unsigned short val;
    unsigned short tmp[2];

#ifdef EFUSE_READ_WORKAROUND
    *efuse = 0;
    return 0;
#endif

    //hal_cmu_pmu_fast_clock_enable();

    // Enable CLK_EN
    val = REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Enable TURN_ON
    val |= REG_EFUSE_TURN_ON;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Sel efuse page
    if (page > 0xF) {
        val |= REG_EFUSE_SEL;
    } else {
        val &= ~REG_EFUSE_SEL;
    }
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Write Address
#ifdef PMU_EFUSE_NO_REDUNDANCY
    val |= REG_EFUSE_ADDRESS(page / 2);
#else
    val |= REG_EFUSE_ADDRESS(page); //redundancy
#endif
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Set Strobe Trigger = 1
    val |= REG_EFUSE_STROBE_TRIGGER;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // set Strobe Trigger = 0
    val &= ~REG_EFUSE_STROBE_TRIGGER;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Read Efuse High 16 bits
    ret = pmu_read(PMU_REG_EFUSE_DATA_LOW, &tmp[0]);
    if (ret) {
        goto _exit;
    }

    // Read Efuse Low 16 bits
    ret = pmu_read(PMU_REG_EFUSE_DATA_HIGH, &tmp[1]);
    if (ret) {
        goto _exit;
    }
#ifdef PMU_EFUSE_NO_REDUNDANCY
    *efuse = tmp[page % 2];
#else
    *efuse = (tmp[0] | tmp[1]); //redundancy
#endif

    _exit:
    // Disable TURN_ON
    val &= ~(REG_EFUSE_TURN_ON | REG_EFUSE_ADDRESS_MASK);
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);

    // Disable CLK_EN
    val &= ~REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);

    //hal_cmu_pmu_fast_clock_disable();

    return ret;
}

static void pmu_sys_ctrl(bool shutdown)
{
    uint16_t val;
    uint32_t lock = int_lock();

    PMU_INFO_TRACE_IMM(0, "Start pmu %s", shutdown ? "shutdown" : "reboot");

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    // Default vcore might not be high enough to support high performance mode
    pmu_high_performance_mode_enable(false);
#endif
    hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);
    pmu_vcore_req |= PMU_VCORE_PMU_RESET;

#ifdef SPLIT_CORE_M_CORE_L
    pmu_read(PMU_REG_CORE_L_CFG2, &val);
    val |= REG_BYPASS_VCORE_L;
    pmu_write(PMU_REG_CORE_L_CFG2, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));
#endif

#ifndef HPPA_LDO_ON
    pmu_hppa_dcdc_to_ldo();
#endif
    pmu_mode_change(PMU_POWER_MODE_LDO);
    hal_sys_timer_delay(MS_TO_TICKS(1));
#endif

#ifdef RTC_ENABLE
    pmu_rtc_save_context();
#endif

#ifdef PMU_FORCE_LP_MODE
    pmu_bandgap_cfg_reset();
    hal_sys_timer_delay(MS_TO_TICKS(1));

    pmu_read(PMU_REG_CODEC_CFG, &val);
    val &= ~LP_EN_VCODEC_LDO_DR;
    pmu_write(PMU_REG_CODEC_CFG, val);

    pmu_read(PMU_REG_ANA_CFG, &val);
    val &= ~LP_EN_VANA_LDO_DR;
    pmu_write(PMU_REG_ANA_CFG, val);

    pmu_read(PMU_REG_DIG_CFG, &val);
    val &= ~LP_EN_VDIG_LDO_DR;
    pmu_write(PMU_REG_DIG_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));
#endif

#ifdef PMU_CLK_USE_EXT_CRYSTAL
    // Switch to lpo clock
    pmu_read(PMU_REG_MIC_BIAS_C, &val);
    val &= ~CLK_32K_SEL_1;
    pmu_write(PMU_REG_MIC_BIAS_C, val);
    hal_sys_timer_delay(MS_TO_TICKS(PMU_CLK_SWITCH_STABLE_TIME));
#endif

    // Reset PMU
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(US_TO_TICKS(500));

    pmu_read(PMU_REG_CHARGER_CFG, &val);
    val |= REG_CHARGE_INTR_EN;
    pmu_write(PMU_REG_CHARGER_CFG, val);

#ifdef PMU_CLK_USE_EXT_CRYSTAL
    if (shutdown && hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2) {
        pmu_external_crystal_enable();
        if (pmu_ext_crystal_clkout_usr_map) {
            pmu_external_crystal_clkout_after_pwroff();
        }
    }
#endif

#ifdef RTC_ENABLE
    pmu_rtc_restore_context();
#ifdef PMU_CLK_USE_EXT_CRYSTAL
    if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_2 && shutdown) {
        pmu_rtc_clk_div_reload();
    }
#endif
#endif

    if (shutdown) {
#if defined(_AUTO_TEST_)
        if (at_skip_shutdown) {
            hal_cmu_sys_reboot();
            return;
        }
#endif

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
        if (
#if defined(RTC_ENABLE)
            pmu_rtc_enabled() ||
#endif
#if defined(CHG_OUT_PWRON)
            // ac_on will en lpo
            true ||
#endif
#ifdef PMU_CLK_USE_EXT_CRYSTAL
            pmu_ext_crystal_clkout_usr_map != 0 ||
#endif
            false) {
            pmu_wdt_stop();
        } else {
            pmu_wdt_config(3*1000, 3*1000);
            pmu_wdt_start();
        }
#endif

        pmu_read(PMU_REG_USB_CFG_3F, &val);
#ifdef CHG_OUT_PWRON
        val |= AC_OUT_LDO_ON_EN;
#else
        val &= ~AC_OUT_LDO_ON_EN;
#endif
        pmu_write(PMU_REG_USB_CFG_3F, val);

        pmu_read(PMU_REG_VRTC_CFG_EC, &val);
        val = SET_BITFIELD(val, REG_VRTC_PWR_DOWN, 0x2);
        pmu_write(PMU_REG_VRTC_CFG_EC, val);

        // Power off
        pmu_read(PMU_REG_POWER_OFF,&val);
        val |= SOFT_POWER_OFF;
        for (int i = 0; i < 100; i++) {
            pmu_write(PMU_REG_POWER_OFF,val);
            hal_sys_timer_delay(MS_TO_TICKS(5));
        }

        hal_sys_timer_delay(MS_TO_TICKS(50));

        //can't reach here
        PMU_INFO_TRACE_IMM(0, "\nError: pmu shutdown failed!\n");
        hal_sys_timer_delay(MS_TO_TICKS(5));
    } else {
#if defined(PMU_FULL_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
        // CAUTION:
        // 1) Never reset RF because system or flash might be using X2/X4, which are off by default
        // 2) Never reset RF/ANA because system or flash might be using PLL, and the reset might cause clock glitch
        // TODO:
        // Restore BBPLL settings in RF
#endif
    }

    hal_cmu_sys_reboot();

    int_unlock(lock);
}

void pmu_shutdown(void)
{
    pmu_sys_ctrl(true);
}

void pmu_reboot(void)
{
    pmu_sys_ctrl(false);
}

static inline uint16_t pmu_get_module_addr(enum PMU_MODULE_T module)
{
    if (module == PMU_USB) {
        return PMU_REG_USB_CFG;
    } else if (module == PMU_CODEC) {
        return PMU_REG_CODEC_CFG;
    } else if (module == PMU_GP){
        return PMU_REG_GP_CFG;
    } else if (module == PMU_SENSOR) {
        return PMU_REG_SENSOR_CFG;
    } else if (module == PMU_PA) {
        return PMU_REG_PA_CFG;
    } else if (module == PMU_CORE_L){
        return PMU_REG_CORE_L_CFG;
    } else {
        return module + PMU_REG_MODULE_START;
    }
}

static inline uint16_t pmu_get_module_rc_volt_addr(enum PMU_MODULE_T module)
{
    if (module == PMU_ANA || module == PMU_DIG || module == PMU_CODEC) {
        return PMU_REG_MAIN_LDO_VOLT_RC;
    } else if (module == PMU_USB || module == PMU_PA) {
        return PMU_REG_USB_PA_VOLT_RC;
    } else if (module == PMU_CORE_L) {
        return PMU_REG_CORE_L_CFG;
    } else {
        // PMU_GP, PMU_SENSOR
        return PMU_REG_SENSOR_GP_VOLT_RC;
    }
}

void pmu_module_config(enum PMU_MODULE_T module,unsigned short is_manual,unsigned short ldo_on,unsigned short lp_mode,unsigned short dpmode)
{
    unsigned short val;
    unsigned short module_address;
    const struct PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    module_address = pmu_get_module_addr(module);

    pmu_read(module_address, &val);
    if (is_manual) {
        val |= module_cfg_p->manual_bit;
    } else {
        val &= ~module_cfg_p->manual_bit;
    }
    if (ldo_on) {
        val |= module_cfg_p->ldo_en;
    } else {
        val &= ~module_cfg_p->ldo_en;
    }

#ifdef PMU_FORCE_LP_MODE
    if (pmu_power_mode != PMU_POWER_MODE_NONE) {
        val |= module_cfg_p->lp_en_dr;
        if (ldo_on) {
            val |= module_cfg_p->lp_en;
        } else {
            val &= ~module_cfg_p->lp_en;
        }
    }
#else
    if (lp_mode) {
        val &= ~module_cfg_p->lp_en_dr;
    } else {
        val = (val & ~module_cfg_p->lp_en) | module_cfg_p->lp_en_dr;
    }
#endif

    if (dpmode) {
        val |= module_cfg_p->dsleep_mode;
    } else {
        val &= ~module_cfg_p->dsleep_mode;
    }
    pmu_write(module_address, val);
}

#ifdef PMU_FORCE_LP_MODE
void pmu_module_force_lp_config(void)
{
    uint16_t val;
    uint16_t module_address;
    const struct PMU_MODULE_CFG_T *module_cfg_p;
    enum PMU_MODULE_T module;
    bool ldo_on;

    for (module = 0; module < PMU_MODULE_QTY; module++) {
        module_cfg_p = &pmu_module_cfg[module];
        module_address = pmu_get_module_addr(module);

        pmu_read(module_address, &val);
        ldo_on = !!(val & module_cfg_p->ldo_en);
        val |= module_cfg_p->lp_en_dr;
        if (ldo_on) {
            val |= module_cfg_p->lp_en;
        } else {
            val &= ~module_cfg_p->lp_en;
        }
        pmu_write(module_address, val);
    }
}
#endif

void pmu_module_set_volt(unsigned char module, unsigned short sleep_v,unsigned short normal_v)
{
    unsigned short val;
    unsigned short module_address;
    unsigned short module_rc_volt_address;
    unsigned short rc_v = normal_v;
    const struct PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    if (module == PMU_PA) {
        normal_v = normal_v > PMU_LDO_PA_MAX ? PMU_LDO_PA_MAX : normal_v;
        sleep_v = sleep_v > PMU_LDO_PA_MAX ? PMU_LDO_PA_MAX : sleep_v;
    }

#ifdef PMU_LDO_VCORE_CALIB
    if (module == PMU_CORE_L) {
        normal_v = pmu_reg_val_add(normal_v, pmu_ldo_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
        sleep_v = pmu_reg_val_add(sleep_v, pmu_ldo_lp_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
#if !defined(PMU_FORCE_LP_MODE) && !defined(FORCE_BIG_BANDGAP)
        rc_v = pmu_reg_val_add(rc_v, pmu_ldo_lp_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
#else
        rc_v = normal_v;
#endif
    }
#endif

#ifdef PMU_VCORE_RC_MODE_FIXED
    // Fixed in pmu_vcore_rc_mode_init().
    if (module != PMU_DIG && module != PMU_CORE_L)
#endif
    {
        module_rc_volt_address = pmu_get_module_rc_volt_addr(module);

        pmu_read(module_rc_volt_address, &val);
        val &= ~module_cfg_p->rc_v;
        val |= (rc_v << module_cfg_p->rc_v_shift) & module_cfg_p->rc_v;
        pmu_write(module_rc_volt_address, val);
    }

    if (module == PMU_GP) {
        pmu_read(PMU_REG_GP_CFG, &val);
        val &= ~module_cfg_p->normal_v;
        val |= (normal_v << module_cfg_p->normal_v_shift) & module_cfg_p->normal_v;
        pmu_write(PMU_REG_GP_CFG, val);
        pmu_read(PMU_REG_GP_SENS_VOLT_SLEEP, &val);
        val &= ~module_cfg_p->dsleep_v;
        val |= (sleep_v << module_cfg_p->dsleep_v_shift) & module_cfg_p->dsleep_v;
        pmu_write(PMU_REG_GP_SENS_VOLT_SLEEP, val);

        return;
    } else if (module == PMU_SENSOR) {
        pmu_read(PMU_REG_SENSOR_CFG, &val);
        val &= ~module_cfg_p->normal_v;
        val |= (normal_v << module_cfg_p->normal_v_shift) & module_cfg_p->normal_v;
        pmu_write(PMU_REG_SENSOR_CFG, val);
        pmu_read(PMU_REG_GP_SENS_VOLT_SLEEP, &val);
        val &= ~module_cfg_p->dsleep_v;
        val |= (sleep_v << module_cfg_p->dsleep_v_shift) & module_cfg_p->dsleep_v;
        pmu_write(PMU_REG_GP_SENS_VOLT_SLEEP, val);

        return;
    } else if (module == PMU_CORE_L) {
        module_address = PMU_REG_CORE_L_VBIT;
    } else {
        module_address = pmu_get_module_addr(module);
    }

    pmu_read(module_address, &val);
    val &= ~module_cfg_p->normal_v;
    val |= (normal_v << module_cfg_p->normal_v_shift) & module_cfg_p->normal_v;
    val &= ~module_cfg_p->dsleep_v;
    val |= (sleep_v << module_cfg_p->dsleep_v_shift) & module_cfg_p->dsleep_v;
    pmu_write(module_address, val);
}

int pmu_module_get_volt(unsigned char module, unsigned short *sleep_vp,unsigned short *normal_vp)
{
    unsigned short val;
    unsigned short module_address;
    const struct PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    if (module == PMU_GP) {
        pmu_read(PMU_REG_GP_CFG, &val);
        if (normal_vp) {
            *normal_vp = (val & module_cfg_p->normal_v) >> module_cfg_p->normal_v_shift;
        }
        pmu_read(PMU_REG_GP_SENS_VOLT_SLEEP, &val);
        if (sleep_vp) {
            *sleep_vp = (val & module_cfg_p->dsleep_v) >> module_cfg_p->dsleep_v_shift;
        }

        return 0;
    } else if (module == PMU_SENSOR) {
        pmu_read(PMU_REG_SENSOR_CFG, &val);
        if (normal_vp) {
            *normal_vp = (val & module_cfg_p->normal_v) >> module_cfg_p->normal_v_shift;
        }
        pmu_read(PMU_REG_GP_SENS_VOLT_SLEEP, &val);
        if (sleep_vp) {
            *sleep_vp = (val & module_cfg_p->dsleep_v) >> module_cfg_p->dsleep_v_shift;
        }

        return 0;
    } else if (module == PMU_CORE_L) {
        module_address = PMU_REG_CORE_L_VBIT;
    } else {
        module_address = pmu_get_module_addr(module);
    }

    pmu_read(module_address, &val);
    if (normal_vp) {
        *normal_vp = (val & module_cfg_p->normal_v) >> module_cfg_p->normal_v_shift;
    }
    if (sleep_vp) {
        *sleep_vp = (val & module_cfg_p->dsleep_v) >> module_cfg_p->dsleep_v_shift;
    }

#ifdef PMU_LDO_VCORE_CALIB
    if (module == PMU_CORE_L) {
        if (normal_vp) {
            *normal_vp = pmu_reg_val_add(*normal_vp, -pmu_ldo_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
        }
        if (sleep_vp) {
            *sleep_vp = pmu_reg_val_add(*sleep_vp, -pmu_ldo_lp_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
        }
    }
#endif

    return 0;
}

static void pmu_module_ramp_volt(unsigned char module, unsigned short sleep_v, unsigned short normal_v)
{
    uint16_t old_normal_v;
    uint16_t old_sleep_v;

    pmu_module_get_volt(module, &old_sleep_v, &old_normal_v);

    if (old_normal_v < normal_v) {
        while (old_normal_v < normal_v) {
            if (old_normal_v + 2 < normal_v) {
                old_normal_v += 2;
            } else {
                old_normal_v = normal_v;
            }
            pmu_module_set_volt(module, sleep_v, old_normal_v);
        }
#ifndef DIG_DCDC_MODE
    } else if (old_normal_v > normal_v && module == PMU_DIG) {
        while (old_normal_v > normal_v) {
            if (old_normal_v > normal_v + 2) {
                old_normal_v -= 2;
            } else {
                old_normal_v = normal_v;
            }
            pmu_module_set_volt(module, sleep_v, old_normal_v);
            hal_sys_timer_delay_us(100);
        }
#endif
    } else if (old_normal_v != normal_v || old_sleep_v != sleep_v) {
        pmu_module_set_volt(module, sleep_v, normal_v);
    }
}

static void pmu_dcdc_ana_get_volt(unsigned short *normal_vp, unsigned short *dsleep_vp)
{
    unsigned short val;

    pmu_read(PMU_REG_DCDC_ANA_VOLT, &val);
    if (normal_vp) {
        *normal_vp = GET_BITFIELD(val, BUCK_VANA_BIT_NORMAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = GET_BITFIELD(val, BUCK_VANA_BIT_DSLEEP);
    }

#ifdef PMU_DCDC_CALIB
    if (normal_vp) {
        *normal_vp = pmu_reg_val_add(*normal_vp, -pmu_dcdc_ana_comp, MAX_BUCK_VANA_BIT_VAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = pmu_reg_val_add(*dsleep_vp, -pmu_ana_lp_comp, MAX_BUCK_VANA_BIT_VAL);
    }
#endif
}

static void pmu_dcdc_ana_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    unsigned short val;
    unsigned short rc_v = normal_v;

#ifdef PMU_DCDC_CALIB
    normal_v = pmu_reg_val_add(normal_v, pmu_dcdc_ana_comp, MAX_BUCK_VANA_BIT_VAL);
    dsleep_v = pmu_reg_val_add(dsleep_v, pmu_ana_lp_comp, MAX_BUCK_VANA_BIT_VAL);
#if !defined(PMU_FORCE_LP_MODE) && !defined(FORCE_BIG_BANDGAP)
    rc_v = pmu_reg_val_add(rc_v, pmu_ana_lp_comp, MAX_BUCK_VANA_BIT_VAL);
#else
    rc_v = normal_v;
#endif
#endif

    pmu_read(PMU_REG_DCDC_ANA_VOLT, &val);
    val = SET_BITFIELD(val, BUCK_VANA_BIT_DSLEEP, dsleep_v);
    val = SET_BITFIELD(val, BUCK_VANA_BIT_NORMAL, normal_v);
    pmu_write(PMU_REG_DCDC_ANA_VOLT, val);

    pmu_read(PMU_REG_DIG_ANA_VOLT_RC, &val);
    val = SET_BITFIELD(val, BUCK_VANA_VBIT_RC, rc_v);
    pmu_write(PMU_REG_DIG_ANA_VOLT_RC, val);

    if (normal_v > dsleep_v || rc_v > dsleep_v) {
        dcdc_ramp_map |= REG_BUCK_VANA_RAMP_EN;
    } else {
        dcdc_ramp_map &= ~REG_BUCK_VANA_RAMP_EN;
    }
}

static void pmu_ana_set_volt(enum PMU_POWER_MODE_T mode)
{
    uint16_t old_act_dcdc;
    uint16_t old_lp_dcdc;
    uint16_t new_act_dcdc;

    new_act_dcdc = ana_act_dcdc;

    if (mode == PMU_POWER_MODE_ANA_DCDC || mode == PMU_POWER_MODE_DIG_DCDC) {
        pmu_dcdc_ana_get_volt(&old_act_dcdc, &old_lp_dcdc);
        if (old_act_dcdc < new_act_dcdc) {
            while (old_act_dcdc++ < new_act_dcdc) {
                pmu_dcdc_ana_set_volt(old_act_dcdc, ana_lp_dcdc);
            }
            hal_sys_timer_delay_us(PMU_VANA_STABLE_TIME_US);
        } else if (old_act_dcdc != new_act_dcdc || old_lp_dcdc != ana_lp_dcdc) {
            pmu_dcdc_ana_set_volt(new_act_dcdc, ana_lp_dcdc);
        }
    } else {
        pmu_dcdc_ana_set_volt(new_act_dcdc, ana_lp_dcdc);
    }
}

static void pmu_dcdc_dig_get_volt(unsigned short *normal_vp, unsigned short *dsleep_vp)
{
    unsigned short val;

    pmu_read(PMU_REG_DCDC_DIG_VOLT, &val);
    if (normal_vp) {
        *normal_vp = GET_BITFIELD(val, BUCK_VCORE_BIT_NORMAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = GET_BITFIELD(val, BUCK_VCORE_BIT_DSLEEP);
    }

#ifdef PMU_DCDC_CALIB
    if (normal_vp) {
        *normal_vp = pmu_reg_val_add(*normal_vp, -pmu_dcdc_dig_comp, MAX_BUCK_VCORE_BIT_VAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = pmu_reg_val_add(*dsleep_vp, -pmu_dig_lp_comp, MAX_BUCK_VCORE_BIT_VAL);
    }
#endif
}

static void pmu_dcdc_dig_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    unsigned short val;
    unsigned short rc_v =
#ifdef PMU_VCORE_RC_MODE_FIXED
        PMU_VCORE_RC_MODE_FIXED_DCDC_DIG;
#else
        normal_v;
#endif

#ifdef PMU_DCDC_CALIB
    normal_v = pmu_reg_val_add(normal_v, pmu_dcdc_dig_comp, MAX_BUCK_VCORE_BIT_VAL);
    dsleep_v = pmu_reg_val_add(dsleep_v, pmu_dig_lp_comp, MAX_BUCK_VCORE_BIT_VAL);
#if !defined(PMU_FORCE_LP_MODE) && !defined(FORCE_BIG_BANDGAP)
    rc_v = pmu_reg_val_add(rc_v, pmu_dig_lp_comp, MAX_BUCK_VCORE_BIT_VAL);
#else
    rc_v = normal_v;
#endif
#endif

    pmu_read(PMU_REG_DCDC_DIG_VOLT, &val);
    val = SET_BITFIELD(val, BUCK_VCORE_BIT_DSLEEP, dsleep_v);
    val = SET_BITFIELD(val, BUCK_VCORE_BIT_NORMAL, normal_v);
    pmu_write(PMU_REG_DCDC_DIG_VOLT, val);

#ifndef PMU_VCORE_RC_MODE_FIXED
    // Fixed in pmu_vcore_rc_mode_init().
    pmu_read(PMU_REG_DIG_ANA_VOLT_RC, &val);
    val = SET_BITFIELD(val, BUCK_VCORE_VBIT_RC, rc_v);
    pmu_write(PMU_REG_DIG_ANA_VOLT_RC, val);
#endif

    if (normal_v > dsleep_v || rc_v > dsleep_v) {
        dcdc_ramp_map |= REG_BUCK_VCORE_RAMP_EN;
    } else {
        dcdc_ramp_map &= ~REG_BUCK_VCORE_RAMP_EN;
    }
}

#ifdef MTEST_ENABLED
bool pmu_dig_set_volt_value(uint16_t dcdc_volt_mv, uint16_t logic_volt_mv, uint16_t ldo_volt_mv)
{
    const static uint16_t dcdc_volt_list[][2] = {
        {1100, PMU_DCDC_DIG_1_1V },
        {1050, PMU_DCDC_DIG_1_05V},
        {1000, PMU_DCDC_DIG_1_0V },
        {950,  PMU_DCDC_DIG_0_95V},
        {900,  PMU_DCDC_DIG_0_9V },
        {850,  PMU_DCDC_DIG_0_85V},
        {840,  PMU_DCDC_DIG_0_84V},
        {830,  PMU_DCDC_DIG_0_83V},
        {820,  PMU_DCDC_DIG_0_82V},
        {800,  PMU_DCDC_DIG_0_8V },
        {750,  PMU_DCDC_DIG_0_75V},
        {740,  PMU_DCDC_DIG_0_74V},
        {730,  PMU_DCDC_DIG_0_73V},
        {700,  PMU_DCDC_DIG_0_7V },
    };

    uint16_t dcdc_volt;
    uint32_t i, find = 0;
    bool success = false;

    for (i = 0; i < ARRAY_SIZE(dcdc_volt_list); i++) {
        if (dcdc_volt_mv == dcdc_volt_list[i][0]) {
            dcdc_volt = dcdc_volt_list[i][1];
            find |= 0x1;
            break;
        }
    }
    if (find == 0x1) {
        pmu_dcdc_dig_set_volt(dcdc_volt, dig_lp_dcdc);
//    pmu_module_set_volt(PMU_BAT2DIG, dig_lp_ldo, ldo_volt);
        success = true;
    }
    return success;
}
#endif /* MTEST_ENABLED */

static void BOOT_TEXT_SRAM_LOC pmu_dig_get_target_volt(uint16_t *ldo, uint16_t *dcdc, uint16_t *logic)
{
    uint16_t ldo_volt;
    uint16_t dcdc_volt;
    uint16_t logic_volt;

    if (0) {
    } else if (pmu_vcore_req & PMU_VCORE_PMU_RESET) {
        ldo_volt = PMU_VDIG_DEFAULT;
        dcdc_volt = PMU_DCDC_DIG_0_8V - PMU_DCDC_DIG_50MV;
        logic_volt = PMU_VDIG_L_0_7V;
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    } else if (pmu_vcore_req & (PMU_VCORE_SYS_FREQ_HIGH)) {
        if (high_perf_freq_mhz <= 260) {
            ldo_volt = PMU_VDIG_1_05V;
            dcdc_volt = PMU_DCDC_DIG_1_05V;
            logic_volt = PMU_VDIG_L_1_0V;
        } else if (high_perf_freq_mhz <= 300) {
            ldo_volt = PMU_VDIG_1_1V;
            dcdc_volt = PMU_DCDC_DIG_1_1V;
            logic_volt = PMU_VDIG_L_1_05V;
        } else {
            ldo_volt = PMU_VDIG_1_2V;
            dcdc_volt = PMU_DCDC_DIG_1_1V;
            logic_volt = PMU_VDIG_L_1_05V;
        }
#endif
    } else if (pmu_vcore_req & PMU_VCORE_PSRAM_FREQ_HIGH) {
        ldo_volt = PMU_VDIG_0_9V;
        dcdc_volt = PMU_DCDC_DIG_0_9V;
        logic_volt = PMU_VDIG_L_0_8V;
    } else if (pmu_vcore_req & (PMU_VCORE_RS_FREQ_HIGH | PMU_VCORE_SYS_FREQ_MEDIUM)) {
        ldo_volt = PMU_VDIG_0_85V;
        dcdc_volt = PMU_DCDC_DIG_0_85V;
        logic_volt = PMU_VDIG_L_0_8V;
    } else if (pmu_vcore_req & PMU_VCORE_PSRAM_FREQ_MEDIUM) {
        ldo_volt = PMU_VDIG_0_8V;
        dcdc_volt = PMU_DCDC_DIG_0_8V;
        logic_volt = PMU_VDIG_L_0_7V;
    } else if (pmu_vcore_req & (PMU_VCORE_FLASH_FREQ_HIGH | PMU_VCORE_FLASH_WRITE_ENABLED | PMU_VCORE_PSRAM_FREQ_MEDIUM_LOW)) {
        ldo_volt = PMU_VDIG_0_75V;
        dcdc_volt = PMU_DCDC_DIG_0_75V;
        logic_volt = PMU_VDIG_L_0_7V;
    } else {
        // Common cases
        ldo_volt = PMU_VDIG_0_75V;
        dcdc_volt = PMU_DCDC_DIG_0_75V;
        logic_volt = PMU_VDIG_L_0_7V;
    }

#if (defined(PROGRAMMER) || defined(__BES_OTA_MODE__)) && !defined(PMU_FULL_INIT)
    // Try to keep the same vcore voltage as ROM (hardware default)
    if (ldo_volt < PMU_VDIG_0_9V) {
        ldo_volt = PMU_VDIG_0_9V;
    }
    if (dcdc_volt < PMU_DCDC_DIG_0_9V) {
        dcdc_volt = PMU_DCDC_DIG_0_9V;
    }
    if (logic_volt < PMU_VDIG_L_0_9V) {
        logic_volt = PMU_VDIG_L_0_9V;
    }
#endif

#ifdef DIG_DCDC_MODE
    if (ldo_volt < PMU_VDIG_DEFAULT) {
        ldo_volt = PMU_VDIG_DEFAULT;
    }
#endif

    if (ldo) {
        *ldo = ldo_volt;
    }
    if (dcdc) {
        *dcdc = dcdc_volt;
    }
    if (logic) {
        *logic = logic_volt;
    }
}

static void pmu_dig_set_volt(enum PMU_POWER_MODE_T mode)
{
    uint32_t lock;
    uint16_t dcdc_volt, old_act_dcdc, old_lp_dcdc;
    uint16_t ldo_volt, old_act_ldo, old_lp_ldo;
    uint16_t logic_volt;
    bool volt_inc = false;

    lock = int_lock();

    if (mode == PMU_POWER_MODE_NONE) {
        mode = pmu_power_mode;
    }

    pmu_dig_get_target_volt(&ldo_volt, &dcdc_volt, &logic_volt);

    pmu_module_get_volt(PMU_DIG, &old_lp_ldo, &old_act_ldo);
    pmu_dcdc_dig_get_volt(&old_act_dcdc, &old_lp_dcdc);

    if (mode == PMU_POWER_MODE_DIG_DCDC) {
        if (old_act_dcdc < dcdc_volt) {
            volt_inc = true;
            while (old_act_dcdc++ < dcdc_volt) {
                pmu_dcdc_dig_set_volt(old_act_dcdc, dig_lp_dcdc);
            }
        } else if (old_act_dcdc != dcdc_volt || old_lp_dcdc != dig_lp_dcdc) {
#ifdef SPLIT_CORE_M_CORE_L
            pmu_module_set_volt(PMU_CORE_L, dig_lp_logic, logic_volt);
#endif
            pmu_dcdc_dig_set_volt(dcdc_volt, dig_lp_dcdc);
        }
        // Update the voltage of the other mode
        pmu_module_set_volt(PMU_DIG, dig_lp_ldo, ldo_volt);
    } else {
        if (old_act_ldo < ldo_volt) {
            volt_inc = true;
        }
#ifdef SPLIT_CORE_M_CORE_L
        if (!volt_inc) {
            pmu_module_set_volt(PMU_CORE_L, dig_lp_logic, logic_volt);
        }
#endif
        pmu_module_ramp_volt(PMU_DIG, dig_lp_ldo, ldo_volt);
        // Update the voltage of the other mode
        pmu_dcdc_dig_set_volt(dcdc_volt, dig_lp_dcdc);
    }

    if (volt_inc) {
#ifdef SPLIT_CORE_M_CORE_L
        // VCORE-L looks smoother without using ramp.
        pmu_module_set_volt(PMU_CORE_L, dig_lp_logic, logic_volt);
#endif
        hal_sys_timer_delay_us(PMU_VCORE_STABLE_TIME_US);
    }

    int_unlock(lock);
}

static void pmu_ana_reserved_en(bool en)
{
    uint16_t val;

    pmu_read(PMU_REG_RESERVED_ANA, &val);
    if (en) {
        val |= RESERVED_ANA_EN;
    } else {
        val &= ~RESERVED_ANA_EN;
    }
    pmu_write(PMU_REG_RESERVED_ANA, val);
}

static void pmu_ldo_mode_en(void)
{
    unsigned short val;

    // Enable vana ldo
    pmu_module_config(PMU_ANA,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    // Enable vcore ldo
    pmu_module_config(PMU_DIG,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

    hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);

    // The load under ana_ldo should be turned on after ana_ldo enabled.
    pmu_ana_reserved_en(true);

    // Disable vana dcdc and vcore dcdc
    pmu_read(PMU_REG_DCDC_EN, &val);
    val = val & TEST_MODE_MASK;
    pmu_write(PMU_REG_DCDC_EN, val);
}

static void pmu_dcdc_ana_mode_en(void)
{
    unsigned short val;

    if (pmu_power_mode == PMU_POWER_MODE_DIG_DCDC) {
        // Enable vcore ldo
        pmu_module_config(PMU_DIG,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

        hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);

        // Disable vcore dcdc
        pmu_read(PMU_REG_DCDC_EN, &val);
        val &= ~(PU_DCDC_DIG_DR | PU_DCDC_DIG_REG);
        pmu_write(PMU_REG_DCDC_EN, val);
    } else {
        pmu_read(PMU_REG_DCDC_CFG_63, &val);
#ifdef PMU_DCDC_PWM
        val &= ~REG_BUCK_ANA_BURST_MODE_NORMAL;
#else
        val |= REG_BUCK_ANA_BURST_MODE_NORMAL;
#endif
        pmu_write(PMU_REG_DCDC_CFG_63, val);

        // Enable vana dcdc
        pmu_read(PMU_REG_DCDC_EN, &val);
        val |= DCDC_ANA_LP_EN_DSLEEP | PU_DCDC_ANA_DSLEEP;
        val |= PU_DCDC_ANA_DR | PU_DCDC_ANA_REG;
        pmu_write(PMU_REG_DCDC_EN, val);

        hal_sys_timer_delay_us(PMU_DCDC_PU_STABLE_TIME_US);

        // Disable vana ldo
        pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);

        // The load under ana_ldo should be turned off after ana_ldo disabled.
        pmu_ana_reserved_en(false);
    }
}

static void pmu_dcdc_dual_mode_en(void)
{
    unsigned short val;

    pmu_read(PMU_REG_DCDC_CFG_63, &val);
#ifdef PMU_DCDC_PWM
    val &= ~REG_BUCK_ANA_BURST_MODE_NORMAL;
#else
    val |= REG_BUCK_ANA_BURST_MODE_NORMAL;
#endif
    pmu_write(PMU_REG_DCDC_CFG_63, val);

    pmu_read(PMU_REG_DCDC_CORE_CFG_65, &val);
#ifdef PMU_DCDC_PWM
    val &= ~REG_BUCK_VCORE_BURST_MODE_NORMAL;
#else
    val |= REG_BUCK_VCORE_BURST_MODE_NORMAL;
#endif
    pmu_write(PMU_REG_DCDC_CORE_CFG_65, val);

    // Enable vana dcdc and vcore dcdc
    pmu_read(PMU_REG_DCDC_EN, &val);
    val |= PU_DCDC_ANA_DR | PU_DCDC_ANA_REG;
    val |= PU_DCDC_DIG_DR | PU_DCDC_DIG_REG;
    val |= DCDC_ANA_LP_EN_DSLEEP | PU_DCDC_ANA_DSLEEP;
    val |= DCDC_DIG_LP_EN_DSLEEP | PU_DCDC_DIG_DSLEEP;
    pmu_write(PMU_REG_DCDC_EN, val);

    hal_sys_timer_delay_us(PMU_DCDC_PU_STABLE_TIME_US);

    // Disable vana ldo
    pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    // Disable vcore ldo
    pmu_module_config(PMU_DIG,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);

    // The load under ana_ldo should be turned off after ana_ldo disabled.
    pmu_ana_reserved_en(false);
}

void pmu_mode_change(enum PMU_POWER_MODE_T mode)
{
    uint32_t lock;

    if (pmu_power_mode == mode || mode == PMU_POWER_MODE_NONE) {
        return;
    }

    lock = int_lock();

    pmu_ana_set_volt(pmu_power_mode);
    pmu_dig_set_volt(pmu_power_mode);

    if (mode == PMU_POWER_MODE_ANA_DCDC) {
        pmu_dcdc_ana_mode_en();
    } else if (mode == PMU_POWER_MODE_DIG_DCDC) {
        pmu_dcdc_dual_mode_en();
    } else if (mode == PMU_POWER_MODE_LDO) {
        pmu_ldo_mode_en();
    }

    pmu_power_mode = mode;

    int_unlock(lock);
}

void pmu_sleep_en(unsigned char sleep_en)
{
    unsigned short val;

    pmu_read(PMU_REG_SLEEP_CFG, &val);
    if(sleep_en) {
        val |= SLEEP_ALLOW;
    } else {
        val &= ~SLEEP_ALLOW;
    }
    pmu_write(PMU_REG_SLEEP_CFG, val);

    pmu_read(PMU_REG_RC_SLEEP_CFG, &val);
    if (sleep_en) {
        val |= REG_RC_SLEEP_ALLOW;
    } else {
        val &= ~REG_RC_SLEEP_ALLOW;
    }
    pmu_write(PMU_REG_RC_SLEEP_CFG, val);
}

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#ifndef HPPA_LDO_ON
static uint32_t pmu_dcdc_hppa_mv_to_val(uint16_t mv)
{
    uint32_t val;

    if (mv == 1600) {
        val = PMU_DCDC_HPPA_1_6V;
    } else if (mv == 1650) {
        val = PMU_DCDC_HPPA_1_65V;
    } else if (mv == 1700) {
        val = PMU_DCDC_HPPA_1_7V;
    } else if (mv == 1750) {
        val = PMU_DCDC_HPPA_1_75V;
    } else if (mv == 1900) {
        val = PMU_DCDC_HPPA_1_9V;
    } else {
        val = PMU_DCDC_HPPA_1_8V;
    }

    return val;
}

POSSIBLY_UNUSED
static void pmu_dcdc_hppa_get_volt(unsigned short *normal_vp, unsigned short *dsleep_vp)
{
    unsigned short val;

    pmu_read(PMU_REG_DCDC_HPPA_VOLT, &val);
    if (normal_vp) {
        *normal_vp = GET_BITFIELD(val, REG_BUCK_HPPA_VBIT_NORMAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = GET_BITFIELD(val, REG_BUCK_HPPA_VBIT_DSLEEP);
    }

#ifdef PMU_DCDC_CALIB
    if (normal_vp) {
        *normal_vp = pmu_reg_val_add(*normal_vp, -pmu_dcdc_hppa_comp, MAX_BUCK_VHPPA_BIT_VAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = pmu_reg_val_add(*dsleep_vp, -pmu_hppa_lp_comp, MAX_BUCK_VHPPA_BIT_VAL);
    }
#endif
}

static void pmu_dcdc_hppa_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    unsigned short val;
    unsigned short rc_v = normal_v;

#ifdef PMU_DCDC_CALIB
    normal_v = pmu_reg_val_add(normal_v, pmu_dcdc_hppa_comp, MAX_BUCK_VHPPA_BIT_VAL);
    dsleep_v = pmu_reg_val_add(dsleep_v, pmu_hppa_lp_comp, MAX_BUCK_VHPPA_BIT_VAL);
#if !defined(PMU_FORCE_LP_MODE) && !defined(FORCE_BIG_BANDGAP)
    rc_v = pmu_reg_val_add(rc_v, pmu_hppa_lp_comp, MAX_BUCK_VHPPA_BIT_VAL);
#else
    rc_v = normal_v;
#endif
#endif

    pmu_read(PMU_REG_DCDC_HPPA_VOLT, &val);
    val = SET_BITFIELD(val, REG_BUCK_HPPA_VBIT_DSLEEP, dsleep_v);
    val = SET_BITFIELD(val, REG_BUCK_HPPA_VBIT_NORMAL, normal_v);
    pmu_write(PMU_REG_DCDC_HPPA_VOLT, val);

    pmu_read(PMU_REG_HPPA_VOLT_RC, &val);
    val = SET_BITFIELD(val, BUCK_HPPA_VBIT_RC, rc_v);
    pmu_write(PMU_REG_HPPA_VOLT_RC, val);

    if (normal_v > dsleep_v || rc_v > dsleep_v) {
        dcdc_ramp_map |= REG_BUCK_HPPA_RAMP_EN;
    } else {
        dcdc_ramp_map &= ~REG_BUCK_HPPA_RAMP_EN;
    }
}

static void pmu_dcdc_hppa_en(int enable)
{
    uint16_t val;

    pmu_read(PMU_REG_DCDC_HPPA_EN, &val);
    if (enable) {
        val |= DCDC_HPPA_LP_EN_DSLEEP | PU_DCDC_HPPA_DR| PU_DCDC_HPPA_REG | PU_DCDC_HPPA_DSLEEP | REG_BUCK_HPPA_BURST_MODE_DSLEEP;
#ifdef PMU_DCDC_PWM
        val &= ~REG_BUCK_HPPA_BURST_MODE_NORMAL;
#else
        val |= REG_BUCK_HPPA_BURST_MODE_NORMAL;
#endif
    } else {
        val &= ~(DCDC_HPPA_LP_EN_DSLEEP | PU_DCDC_HPPA_DR| PU_DCDC_HPPA_REG | PU_DCDC_HPPA_DSLEEP);
    }
    pmu_write(PMU_REG_DCDC_HPPA_EN, val);
}

static void pmu_hppa_dcdc_to_ldo(void)
{
    pmu_module_set_volt(PMU_CODEC, PMU_CODEC_1_8V, PMU_CODEC_1_8V);
    pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_OFF,PMU_DSLEEP_MODE_OFF);
    hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);
    pmu_dcdc_hppa_en(false);
}
#endif

int pmu_codec_volt_ramp_up(void)
{
    return 0;
}

int pmu_codec_volt_ramp_down(void)
{
    return 0;
}

static void BOOT_TEXT_FLASH_LOC pmu_dig_init_volt(void)
{
    uint16_t ldo_volt;
    uint16_t val;

    pmu_dig_get_target_volt(&ldo_volt, NULL, NULL);

    pmu_read(PMU_REG_DIG_CFG, &val);
    if (GET_BITFIELD(val, LDO_DIG_VBIT_NORMAL) < ldo_volt) {
        val = SET_BITFIELD(val, LDO_DIG_VBIT_NORMAL, ldo_volt);
        pmu_write(PMU_REG_DIG_CFG, val);
    }
}

// Target voltage 1.8V
static void pmu_set_sar_adc_vref(void)
{
    int16_t val_vreg_sel_comp = 0;
    uint16_t efuse_val;
    uint16_t val_vreg_sel_base = PMU_SAR_ADC_VREF_VREG_DEFAULT;
    uint16_t val;

    pmu_get_efuse(PMU_EFUSE_PAGE_DCCALIB2_L_LP, &efuse_val);
    if (!!(efuse_val & PMU_EFUSE_SAR_ADC_VREF_CALIBED)) {
#ifdef PMU_FORCE_LP_MODE
        val_vreg_sel_comp = GET_BITFIELD(efuse_val, PMU_EFUSE_SAR_ADC_VREF_CAL_LP);
        if (!!(efuse_val & PMU_EFUSE_SAR_ADC_VREF_CAL_LP_SIGN)) {
            val_vreg_sel_comp = -val_vreg_sel_comp;
        }
#else
        val_vreg_sel_comp = GET_BITFIELD(efuse_val, PMU_EFUSE_SAR_ADC_VREF_CAL);
        if (!!(efuse_val & PMU_EFUSE_SAR_ADC_VREF_CAL_SIGN)) {
            val_vreg_sel_comp = -val_vreg_sel_comp;
        }
#endif
        TRACE(0, "%s Calibration done! offset=%d", __func__, val_vreg_sel_comp);

        pmu_read(PMU_REG_SAR_VREF_CFG_E6, &val);
        val_vreg_sel_base += val_vreg_sel_comp;
        val = SET_BITFIELD(val, REG_SAR_VREG_SEL, val_vreg_sel_base);
        pmu_write(PMU_REG_SAR_VREF_CFG_E6, val);
    } else {
        TRACE(0, "%s No calibration!", __func__);
    }
}

#ifdef PMU_VCORE_RC_MODE_FIXED
// Don't need ramp because MCU don't use RC clock.
static void pmu_vcore_rc_mode_init(void)
{
    uint16_t rc_v;
    uint16_t val;

    // VCORE-M-DCDC-RC
    rc_v = PMU_VCORE_RC_MODE_FIXED_DCDC_DIG;

#ifdef PMU_DCDC_CALIB
#if !defined(PMU_FORCE_LP_MODE) && !defined(FORCE_BIG_BANDGAP)
    rc_v = pmu_reg_val_add(rc_v, pmu_dig_lp_comp, MAX_BUCK_VCORE_BIT_VAL);
#else
    rc_v = pmu_reg_val_add(rc_v, pmu_dcdc_dig_comp, MAX_BUCK_VCORE_BIT_VAL);
#endif
#endif

    pmu_read(PMU_REG_DIG_ANA_VOLT_RC, &val);
    val = SET_BITFIELD(val, BUCK_VCORE_VBIT_RC, rc_v);
    pmu_write(PMU_REG_DIG_ANA_VOLT_RC, val);

    // VCORE-M-LDO-RC
    pmu_read(PMU_REG_MAIN_LDO_VOLT_RC, &val);
    val = SET_BITFIELD(val, LDO_DIG_VBIT_RC, PMU_VCORE_RC_MODE_FIXED_VDIG);
    pmu_write(PMU_REG_MAIN_LDO_VOLT_RC, val);

#ifdef SPLIT_CORE_M_CORE_L
    // VCORE-L-RC
    rc_v = PMU_VCORE_RC_MODE_FIXED_VDIG_L;

#ifdef PMU_LDO_VCORE_CALIB
#if !defined(PMU_FORCE_LP_MODE) && !defined(FORCE_BIG_BANDGAP)
    rc_v = pmu_reg_val_add(rc_v, pmu_ldo_lp_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
#else
    rc_v = pmu_reg_val_add(rc_v, pmu_ldo_dig_l_comp, MAX_LDO_VCORE_L_VBIT);
#endif
#endif

    pmu_read(PMU_REG_CORE_L_CFG, &val);
    val = SET_BITFIELD(val, LDO_VCORE_L_VBIT_RC, rc_v);
    pmu_write(PMU_REG_CORE_L_CFG, val);
#endif
}
#endif
#endif

static void pmu_sar_adc_vref_sw_pu(bool pu)
{
    uint16_t val;

    pmu_read(PMU_REG_SAR_VREF_CFG_E6, &val);
    if (pu) {
        val &= ~REG_SAR_PD_VREF_ADC_SW;
    } else {
        val |= REG_SAR_PD_VREF_ADC_SW;
    }
    pmu_write(PMU_REG_SAR_VREF_CFG_E6, val);
}

#ifdef PMU_CLK_LED1_INPUT_32K
static void pmu_led1_use_external_32k(void)
{
    uint16_t val;

    // sel vmem voltage
    pmu_read(PMU_REG_LED_CFG_IO1, &val);
    val = SET_BITFIELD(val, REG_LED_IO1_SEL, 0x1);
    pmu_write(PMU_REG_LED_CFG_IO1, val);

    pmu_led_set_direction(HAL_GPIO_PIN_LED1, HAL_GPIO_DIR_IN);

    pmu_read(PMU_REG_POWER_KEY_CFG, &val);
    val |= CLK_32K_SEL_0;
    pmu_write(PMU_REG_POWER_KEY_CFG, val);

    pmu_read(PMU_REG_MIC_BIAS_C, &val);
    val |= CLK_32K_SEL_1;
    pmu_write(PMU_REG_MIC_BIAS_C, val);
}
#endif

POSSIBLY_UNUSED static BOOT_BSS_LOC bool pmu_opened = false;

int BOOT_TEXT_FLASH_LOC pmu_open(void)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))

    uint16_t val;
    enum PMU_POWER_MODE_T mode;

    if (pmu_opened)
        return 0;

    pmu_opened = true;

    TRACE(0, "%s pmu_metal_id:%d chip_metal_id:%d boot_cause:0x%X first_pwron:%d",
            __func__, pmu_metal_id, hal_get_chip_metal_id() , pmu_boot_cause_get(), pmu_boot_first_power_up());

    ASSERT(hal_get_chip_metal_id() < 2, "chip_metal_id=%d can not be used in earbuds", hal_get_chip_metal_id());

    ASSERT(vcodec_mv == 1650 || vcodec_mv == 1700 || vcodec_mv == 1750 || vcodec_mv == 1800 || vcodec_mv == 1900,
        "Invalid vcodec cfg: vcodec_mv=%u", vcodec_mv);
    ASSERT(vhppa_mv == 1650 || vhppa_mv == 1700 || vhppa_mv == 1750 || vhppa_mv == 1800 || vhppa_mv == 1900,
        "Invalid vhppa cfg: vhppa_mv=%u", vhppa_mv);
    TRACE(0, "%s vcodec_mv:%d vhppa_mv:%d", __func__, vcodec_mv, vhppa_mv);

    // Disable and clear all PMU irqs by default
    pmu_write(PMU_REG_INT_MASK, 0);
    pmu_write(PMU_REG_INT_EN, 0);
    // PMU irqs cannot be cleared by PMU soft reset
    pmu_read(PMU_REG_CHARGER_STATUS, &val);
    pmu_write(PMU_REG_CHARGER_STATUS, val);
    pmu_read(PMU_REG_INT_STATUS, &val);
    pmu_write(PMU_REG_INT_CLR, val);

    // Allow PMU to sleep when power key is pressed
    pmu_read(PMU_REG_POWER_KEY_CFG, &val);
    val &= ~POWERKEY_WAKEUP_OSC_EN;
    pmu_write(PMU_REG_POWER_KEY_CFG, val);

    // Increase big bandgap startup time (stable time)
    pmu_read(PMU_REG_CRYSTAL_CFG, &val);
    val = SET_BITFIELD(val, REG_VCORE_SSTIME_MODE, 2);
    pmu_write(PMU_REG_CRYSTAL_CFG, val);

#ifndef PMU_FORCE_LP_MODE
    pmu_read(PMU_REG_BIAS_CFG, &val);
#ifdef FORCE_BIG_BANDGAP
    val |= PU_BIAS_LDO_DR | BG_VBG_SEL_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR;
#else
    // Allow low power bandgap
    val &= ~BG_VBG_SEL_DR;
#endif
    pmu_write(PMU_REG_BIAS_CFG, val);
#endif

    pmu_read(PMU_REG_SAR_VREF_CFG_E6, &val);
    val = SET_BITFIELD(val, REG_VBUCK_RAMP_STEP, 0x4);
    pmu_write(PMU_REG_SAR_VREF_CFG_E6, val);

    // Vhppa cap bits
    pmu_read(PMU_REG_DCDC_HPPA_CAP, &val);
    val = SET_BITFIELD(val, REG_BUCK_HPPA_CAP_BIT, 0xE);
    pmu_write(PMU_REG_DCDC_HPPA_CAP, val);

    // Disable DCDC sync
    pmu_read(PMU_REG_DCDC_DIG_SYNC, &val);
    val |= REG_BUCK_VCORE_SYNC_DISABLE;
    pmu_write(PMU_REG_DCDC_DIG_SYNC, val);

    // Enable GPADC
    pmu_read(PMU_REG_SAR_EN, &val);
    val |= REG_SAR_BUF_EN;
    pmu_write(PMU_REG_SAR_EN, val);

    // Disable sar vref output
    pmu_sar_adc_vref_sw_pu(false);
    pmu_set_sar_adc_vref();

#ifdef PMU_IRQ_UNIFIED
    pmu_read(PMU_REG_WDT_CFG, &val);
    val |= POWERON_DETECT_EN | MERGE_INTR;
    pmu_write(PMU_REG_WDT_CFG, val);
#endif

#ifdef PMU_DCDC_CALIB
    pmu_get_dcdc_calib_value();
#endif

#ifdef PMU_LDO_VCORE_CALIB
    pmu_get_ldo_dig_calib_value();
#endif

#ifndef NO_SLEEP
    pmu_sleep_en(1);  //enable sleep
#endif

    uint16_t val_burst_threshold_cal = 0;
    uint16_t val_is_gain_normal_cal = 0;
    bool val_buck_ana_cfg_part2_update = false;

    pmu_get_efuse(PMU_EFUSE_PAGE_BUCK_CFG_CAL, &val);
    val_is_gain_normal_cal = GET_BITFIELD(val, PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL);
    val = GET_BITFIELD(val, PMU_EFUSE_BUCK_ANA_CFG_TYPE);

    if (val == 0) {
        pmu_get_efuse(PMU_EFUSE_PAGE_DCCALIB2_L_LP, &val);
        val = GET_BITFIELD(val, PMU_EFUSE_BUCK_ANA_IS_GAIN_CAL_OLD);
        if (val == 0x1) {
            val_is_gain_normal_cal = 0x2;
        } else if (val == 0x2) {
            val_is_gain_normal_cal = 0x1;
        } else {
            val_is_gain_normal_cal = 0x0;
        }
    } else if (val == 1) {
        val_burst_threshold_cal = 0x1C;
    } else if (val == 2) {
        val_burst_threshold_cal = 0x1D;
    } else if (val == 3) {
        val_burst_threshold_cal = 0x1C;
        val_buck_ana_cfg_part2_update = true;
    } else if (val == 4) {
        val_burst_threshold_cal = 0x1F;
    } else if (val == 5) {
        val_burst_threshold_cal = 0x1D;
        val_buck_ana_cfg_part2_update = true;
    } else if (val == 6) {
        val_burst_threshold_cal = 0x1F;
        val_buck_ana_cfg_part2_update = true;
    }

    pmu_read(PMU_REG_DCDC_DIG_SYNC, &val);
    val = SET_BITFIELD(val, REG_BUCK_VCORE_INTERNAL_FREQUENCY, 0x3);
    pmu_write(PMU_REG_DCDC_DIG_SYNC, val);

    pmu_read(PMU_REG_BUCK_ANA_FREQ_CFG, &val);
    val = SET_BITFIELD(val, REG_BUCK_ANA_INTERNAL_FREQUENCY, 0x3);
    if (val_is_gain_normal_cal) {
        val = SET_BITFIELD(val, REG_BUCK_ANA_IS_GAIN_NORMAL, val_is_gain_normal_cal);
    } else {
        if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2) {
            val = SET_BITFIELD(val, REG_BUCK_ANA_IS_GAIN_NORMAL, 0x2);
        }
    }
    pmu_write(PMU_REG_BUCK_ANA_FREQ_CFG, val);

    pmu_read(PMU_REG_BUCK_HPPA_FREQ_CFG, &val);
    val = SET_BITFIELD(val, REG_BUCK_HPPA_INTERNAL_FREQUENCY, 0x3);
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2) {
        val = SET_BITFIELD(val, REG_BUCK_HPPA_IS_GAIN_NORMAL, 0x2);
    }
#ifdef PMU_RELAX_BURST_THRESHOLD
    val = SET_BITFIELD(val, REG_BUCK_HPPA_BURST_THRESHOLD, 0x1C);
#endif
    pmu_write(PMU_REG_BUCK_HPPA_FREQ_CFG, val);

    // Reduce DCDC ripple
    pmu_read(PMU_REG_BUCK_HPPA_CFG_C0, &val);
    val |= REG_BUCK_HPPA_IX2_ERR;
    pmu_write(PMU_REG_BUCK_HPPA_CFG_C0, val);

    pmu_read(PMU_REG_BUCK_ANA_CFG_C4, &val);
    val |= REG_BUCK_ANA_IX2_ERR;
    if (val_burst_threshold_cal)
    {
        val = SET_BITFIELD(val, REG_BUCK_ANA_BURST_THRESHOLD, val_burst_threshold_cal);
    }
#ifdef PMU_RELAX_BURST_THRESHOLD
    else
    {
        val = SET_BITFIELD(val, REG_BUCK_ANA_BURST_THRESHOLD, 0x1C);
    }
#endif
    pmu_write(PMU_REG_BUCK_ANA_CFG_C4, val);

    pmu_read(PMU_REG_BUCK_CORE_CFG_C7, &val);
    val |= REG_BUCK_VCORE_IX2_ERR;
#ifdef PMU_RELAX_BURST_THRESHOLD
    val = SET_BITFIELD(val, REG_BUCK_VCORE_BURST_THRESHOLD, 0x1C);
#endif
    pmu_write(PMU_REG_BUCK_CORE_CFG_C7, val);

    // DCDC configratuion
    pmu_read(PMU_REG_DCDC_HPPA_CFG_C2, &val);
    val = SET_BITFIELD(val, REG_BUCK_HPPA_HYSTERESIS_BIT, 0x3);
    pmu_write(PMU_REG_DCDC_HPPA_CFG_C2, val);

    pmu_read(PMU_REG_BUCK_ANA_CFG_C5, &val);
    if (val_buck_ana_cfg_part2_update) {
        val |= REG_BUCK_ANA_BURST_THRESHOLD_PART2;
    }
    val = SET_BITFIELD(val, REG_BUCK_ANA_HYSTERESIS_BIT, 0x1);
    val = SET_BITFIELD(val, REG_BUCK_ANA_COUNTER_SEL, 0x0);
    pmu_write(PMU_REG_BUCK_ANA_CFG_C5, val);

    pmu_read(PMU_REG_BUCK_CORE_CFG_C8, &val);
    val = SET_BITFIELD(val, REG_BUCK_VCORE_HYSTERESIS_BIT, 0x7);
    pmu_write(PMU_REG_BUCK_CORE_CFG_C8, val);

    pmu_read(PMU_REG_DCDC_CFG_63, &val);
    val = SET_BITFIELD(val, REG_BUCK_VCORE_IS_GAIN_NORMAL, 0x2);
    val = SET_BITFIELD(val, REG_BUCK_VCORE_IS_GAIN_DSLEEP, 0x4);
    pmu_write(PMU_REG_DCDC_CFG_63, val);

    pmu_read(PMU_REG_USB_PA_VOLT_RC, &val);
    val = SET_BITFIELD(val, REG_BUCK_VCORE_IS_GAIN_RC, 0x2);
    pmu_write(PMU_REG_USB_PA_VOLT_RC, val);

#ifndef HPPA_LDO_ON
    // enable vcodec dcdc
    val = pmu_dcdc_hppa_mv_to_val(vhppa_mv);
    pmu_dcdc_hppa_set_volt(val, val);
    pmu_dcdc_hppa_en(true);
    hal_sys_timer_delay_us(PMU_DCDC_PU_STABLE_TIME_US);
    //disable vcodec ldo
    pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
#endif

#ifndef PROGRAMMER
#ifdef VUSB_ON
    pmu_vusb_req |= PMU_VUSB_REQ_INIT;
#endif
    // Disable usbphy power, and vusb if possible
    pmu_usb_config(PMU_USB_CONFIG_TYPE_NONE);
#ifdef VUSB_ON
    pmu_vusb_req &= ~PMU_VUSB_REQ_INIT;
#endif
#endif

    // Disable LDO_USB, LDO_GP, LDO_SENSOR as default.
    pmu_module_config(PMU_USB,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    pmu_module_config(PMU_GP,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    pmu_module_config(PMU_SENSOR,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    pmu_module_config(PMU_PA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);

    pmu_codec_mic_bias_set_volt(AUD_VMIC_MAP_VMIC1 | AUD_VMIC_MAP_VMIC2, 2200);

#ifdef DIG_DCDC_MODE
    mode = PMU_POWER_MODE_DIG_DCDC;
#elif defined(ANA_DCDC_MODE)
    mode = PMU_POWER_MODE_ANA_DCDC;
#else // LDO_MODE
    mode = PMU_POWER_MODE_LDO;
#endif

    pmu_mode_change(mode);

#ifdef PMU_VCORE_RC_MODE_FIXED
    pmu_vcore_rc_mode_init();
#endif

#ifndef PSRAM_ENABLE
    pmu_read(PMU_REG_SW_1P8_PSRAM, &val);
    val &= ~REG_EN_SW1P8_PSRAM;
    pmu_write(PMU_REG_SW_1P8_PSRAM, val);
#endif

#ifdef SPLIT_CORE_M_CORE_L
    pmu_module_config(PMU_CORE_L,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);

    pmu_read(PMU_REG_CORE_L_CFG2, &val);
    val &= ~REG_BYPASS_VCORE_L;
    pmu_write(PMU_REG_CORE_L_CFG2, val);
    hal_sys_timer_delay(MS_TO_TICKS(5));
#endif

#ifdef PMU_FORCE_LP_MODE
    // Reduce LDO voltage ripple
    pmu_read(PMU_REG_AVDD_EN, &val);
    val = SET_BITFIELD(val, REG_LP_BIAS_SEL_LDO, 0);
    pmu_write(PMU_REG_AVDD_EN, val);

    // Force LP mode
    pmu_module_force_lp_config();
    pmu_codec_mic_bias_lowpower_mode(AUD_VMIC_MAP_VMIC1 | AUD_VMIC_MAP_VMIC2, true);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    // Switch to little bandgap
    pmu_read(PMU_REG_BIAS_CFG, &val);
    val = (val & ~BG_VBG_SEL_REG) | BG_VBG_SEL_DR;
    pmu_write(PMU_REG_BIAS_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    if (big_bandgap_user_map == 0) {
        // Disable big bandgap
        val = (val & ~(PU_BIAS_LDO_REG | BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG | BG_NOTCH_EN_REG)) |
            PU_BIAS_LDO_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR | BG_NOTCH_EN_DR;
        pmu_write(PMU_REG_BIAS_CFG, val);
    }
#endif

#ifdef PMU_CLK_USE_EXT_CRYSTAL
    pmu_external_crystal_enable();
#endif

#ifdef RTC_ENABLE
    pmu_rtc_restore_context();
#endif

#if defined(MCU_HIGH_PERFORMANCE_MODE)
    pmu_high_performance_mode_enable(true);
#endif
    pmu_read(PMU_REG_PU_MEM_DR, &val);
    val &= ~(REG_PU_DISP_MEM_DR |
             REG_PU_SHARE_MEM9_DR |
             REG_PU_SHARE_MEM8_DR |
             REG_PU_SHARE_MEM7_DR |
             REG_PU_SHARE_MEM6_DR |
             REG_PU_SHARE_MEM5_DR |
             REG_PU_SHARE_MEM4_DR |
             REG_PU_SENS_MEM_DR   |
             REG_PU_CODEC_MEM_DR  |
             REG_PU_BT_MEM_DR     |
             REG_PU_MCU_MEM_DR);
    pmu_write(PMU_REG_PU_MEM_DR, val);

#endif // PMU_INIT || (!FPGA && !PROGRAMMER)

#ifdef BT_RCOSC_CAL
    rf_xtal_rcosc_cal();
#endif

#ifdef PMU_VIO_3V3_ENABLE
    pmu_vio_3v3(true);
#endif

#ifdef PMU_CLK_LED1_INPUT_32K
    pmu_led1_use_external_32k();
#endif

    return 0;
}

#ifdef PMU_FORCE_LP_MODE
static void pmu_bandgap_cfg_reset(void)
{
    uint16_t val;

    pmu_read(PMU_REG_BIAS_CFG, &val);
    val |= PU_BIAS_LDO_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR | BG_NOTCH_EN_DR | BG_VBG_SEL_DR |
        PU_BIAS_LDO_REG | BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG | BG_NOTCH_EN_REG;
    pmu_write(PMU_REG_BIAS_CFG, val);
    hal_sys_timer_delay_us(PMU_BIG_BG_STABLE_TIME_US);

    val |= BG_VBG_SEL_REG;
    pmu_write(PMU_REG_BIAS_CFG, val);
}
#endif

void pmu_big_bandgap_enable(enum PMU_BIG_BANDGAP_USER_T user, int enable)
{
#ifdef PMU_FORCE_LP_MODE
    uint16_t val;
    bool update = false;

    if (enable) {
        if (big_bandgap_user_map == 0) {
            update = true;
        }
        big_bandgap_user_map |= user;
    } else {
        if (big_bandgap_user_map & user) {
            big_bandgap_user_map &= ~user;
            if (big_bandgap_user_map == 0) {
                update = true;
            }
        }
    }

    if (!update) {
        return;
    }

    pmu_read(PMU_REG_BIAS_CFG, &val);
    val |= BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR;
    if (enable) {
        val |= BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG;
    } else {
        val &= ~(BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG);
    }
    pmu_write(PMU_REG_BIAS_CFG, val);
    if (enable) {
        hal_sys_timer_delay_us(PMU_BIG_BG_STABLE_TIME_US);
    }
#endif
}

void pmu_sleep(void)
{
    uint16_t val;

    if (dcdc_ramp_map) {
        // Enable DCDC ramp
        pmu_read(PMU_REG_DCDC_RAMP_EN, &val);
        val |= dcdc_ramp_map;
        pmu_write(PMU_REG_DCDC_RAMP_EN, val);
    }

    pmu_read(PMU_REG_BUCK_ANA_CFG_C5, &val);
    val = SET_BITFIELD(val, REG_BUCK_ANA_HYSTERESIS_BIT, 0x3);
    val = SET_BITFIELD(val, REG_BUCK_ANA_COUNTER_SEL, 0x1);
    pmu_write(PMU_REG_BUCK_ANA_CFG_C5, val);
}

void pmu_wakeup(void)
{
    uint16_t val;

    if (dcdc_ramp_map) {
        // Disable DCDC ramp
        pmu_read(PMU_REG_DCDC_RAMP_EN, &val);
        val &= ~dcdc_ramp_map;
        pmu_write(PMU_REG_DCDC_RAMP_EN, val);
    }

    pmu_read(PMU_REG_BUCK_ANA_CFG_C5, &val);
    val = SET_BITFIELD(val, REG_BUCK_ANA_HYSTERESIS_BIT, 0x1);
    val = SET_BITFIELD(val, REG_BUCK_ANA_COUNTER_SEL, 0x0);
    pmu_write(PMU_REG_BUCK_ANA_CFG_C5, val);
}

void pmu_codec_mic_bias_set_volt(uint32_t map, uint32_t mv)
{
    uint16_t val;
    int i;
    enum PMU_REG_T bias_reg;
    uint8_t volt = 0;
    uint8_t res = 0;
    uint8_t step = 0;

    step = 40;

    if (mv <= 1600) {
        volt = PMU_VMIC_1_6V;
    } else {
        volt = PMU_VMIC_1_6V + (mv - 1600) / step;
        if (volt > PMU_VMIC_2_8V) {
            volt = PMU_VMIC_2_8V;
        }
    }

    if (vcodec_mv < 1800) {
        volt += 2;
    }

    res = PMU_VMIC_RES_2_8V;

    pmu_read(PMU_REG_MIC_BIAS_A, &val);
    val = (val & ~(REG_MIC_LDO_RES_MASK)) | REG_MIC_LDO_RES(res);
    pmu_write(PMU_REG_MIC_BIAS_A, val);

    for (i = 0; i < MAX_VMIC_CH_NUM; i++) {
        if ((map & (AUD_VMIC_MAP_VMIC1 << i)) == 0) {
            continue;
        }
        bias_reg = PMU_REG_MIC_BIAS_A + i;
        pmu_read(bias_reg, &val);
        val = (val & ~(REG_MIC_BIASA_VSEL_MASK)) | REG_MIC_BIASA_VSEL(volt);
        pmu_write(bias_reg, val);
    }
}

void pmu_codec_mic_bias_enable(uint32_t map, int enable)
{
    int i;
    enum PMU_REG_T bias_reg;
    uint16_t val;
    uint16_t val_temp = 0;
    static bool reg_mic_ldo_en = false;
    static int need_ldo_on_cnt = 0;

    for (i = 0; i < MAX_VMIC_CH_NUM; i++) {
        if ((map & (AUD_VMIC_MAP_VMIC1 << i)) == 0) {
            continue;
        }
        bias_reg = PMU_REG_MIC_BIAS_A + i;
        pmu_read(bias_reg, &val);
        if (enable) {
            val |= REG_MIC_BIASA_EN;
#ifdef PMU_VMIC_LPF_ON
            val |= REG_MIC_BIASA_ENLPF;
#endif
            need_ldo_on_cnt++;
        } else {
            val &= ~REG_MIC_BIASA_EN;
#ifdef PMU_VMIC_LPF_ON
            val &= ~REG_MIC_BIASA_ENLPF;
#endif
            need_ldo_on_cnt--;
        }
        pmu_write(bias_reg, val);
    }

    if (!reg_mic_ldo_en && need_ldo_on_cnt) {
        pmu_read(PMU_REG_MIC_BIAS_B, &val);
        val |= REG_MIC_LDO_EN;
        pmu_write(PMU_REG_MIC_BIAS_B, val);
        reg_mic_ldo_en = true;
    } else if (need_ldo_on_cnt == 0) {
        pmu_read(PMU_REG_MIC_BIAS_B, &val);
        val &= ~REG_MIC_LDO_EN;
        pmu_write(PMU_REG_MIC_BIAS_B, val);
        reg_mic_ldo_en = false;
    }

    if (!enable) {
        pmu_read(PMU_REG_MIC_PULL_DOWN, &val);
        val_temp = val;
        if (map & AUD_VMIC_MAP_VMIC1) {
            val_temp |= REG_MIC_BIASA_PULLDOWN;
        }
        if (map & AUD_VMIC_MAP_VMIC2) {
            val_temp |= REG_MIC_BIASB_PULLDOWN;
        }
        pmu_write(PMU_REG_MIC_PULL_DOWN, val_temp);
        hal_sys_timer_delay_us(PMU_LDO_PULLDOWN_STABLE_TIME_US);
        // need clear pull_down
        pmu_write(PMU_REG_MIC_PULL_DOWN, val);
    }
}

void pmu_codec_mic_bias_lowpower_mode(uint32_t map, int enable)
{
    uint16_t val;

    if (map & (AUD_VMIC_MAP_VMIC1 | AUD_VMIC_MAP_VMIC2)) {
        pmu_read(PMU_REG_MIC_PULL_DOWN, &val);
        if (enable) {
            val |= REG_MIC_LP_ENABLE;
        } else {
            val &= ~REG_MIC_LP_ENABLE;
        }
        pmu_write(PMU_REG_MIC_PULL_DOWN, val);
    }
}

SRAM_TEXT_LOC void pmu_flash_write_config(void)
{
#ifdef FLASH_WRITE_AT_HIGH_VCORE
    uint32_t lock;

    if (pmu_vcore_req & PMU_VCORE_FLASH_WRITE_ENABLED) {
        return;
    }

    lock = int_lock();
    pmu_vcore_req |= PMU_VCORE_FLASH_WRITE_ENABLED;
    int_unlock(lock);

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
#endif
}

SRAM_TEXT_LOC void pmu_flash_read_config(void)
{
#ifdef FLASH_WRITE_AT_HIGH_VCORE
    uint32_t lock;

    if ((pmu_vcore_req & PMU_VCORE_FLASH_WRITE_ENABLED) == 0) {
        return;
    }

    lock = int_lock();
    pmu_vcore_req &= ~PMU_VCORE_FLASH_WRITE_ENABLED;
    int_unlock(lock);

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
#endif
}

void BOOT_TEXT_FLASH_LOC pmu_flash_freq_config(uint32_t freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    uint32_t lock;

    lock = int_lock();
    if (freq > 52000000) {
        // The real max freq is 120M
        //pmu_vcore_req |= PMU_VCORE_FLASH_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_FLASH_FREQ_HIGH;
    }
    int_unlock(lock);

    if (pmu_power_mode == PMU_POWER_MODE_NONE) {
        // PMU not init yet
        pmu_dig_init_volt();
        return;
    }

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
#endif
}

void BOOT_TEXT_FLASH_LOC pmu_psram_freq_config(uint32_t freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    uint32_t lock;

    lock = int_lock();
    pmu_vcore_req &= ~(PMU_VCORE_PSRAM_FREQ_HIGH | PMU_VCORE_PSRAM_FREQ_MEDIUM | PMU_VCORE_PSRAM_FREQ_MEDIUM_LOW);
    if (freq >= 200000000) {
        pmu_vcore_req |= PMU_VCORE_PSRAM_FREQ_HIGH;
    } else if (freq >= 160000000) {
        pmu_vcore_req |= PMU_VCORE_PSRAM_FREQ_MEDIUM;
    } else if (freq >= 100000000) {
        pmu_vcore_req |= PMU_VCORE_PSRAM_FREQ_MEDIUM_LOW;
    }
    int_unlock(lock);

    if (pmu_power_mode == PMU_POWER_MODE_NONE) {
        // PMU not init yet
        pmu_dig_init_volt();
    } else {
        pmu_dig_set_volt(PMU_POWER_MODE_NONE);
    }

#if !defined(DSI_CLK_ENABLE) || defined(PSRAM_DYN_FREQ_CONFIG)
    lock = int_lock();
    if (freq > (150 * 1000000)) {
        dsipll_freq_pll_config(freq * 2);
        dsipll_div2_enable(0);
    } else {
        dsipll_freq_pll_config(freq * 4);
        dsipll_div2_enable(1);
    }
    int_unlock(lock);
#endif
#endif
}

void pmu_anc_config(int enable)
{
}

void pmu_fir_high_speed_config(int enable)
{
}

void pmu_fir_freq_config(uint32_t freq)
{
    uint32_t lock;

    lock = int_lock();
    if (freq >= 96000000) {
        pmu_vcore_req |= PMU_VCORE_FIR_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_FIR_FREQ_HIGH;
    }
    int_unlock(lock);

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
}

void pmu_iir_freq_config(uint32_t freq)
{
}

void pmu_iir_eq_freq_config(uint32_t freq)
{
    uint32_t lock;

    lock = int_lock();
    if (freq >= 96000000) {
        pmu_vcore_req |= PMU_VCORE_IIR_EQ_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_IIR_EQ_FREQ_HIGH;
    }
    int_unlock(lock);

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
}

void pmu_rs_freq_config(uint32_t freq)
{
    uint32_t lock;

    lock = int_lock();
    if (freq >= 60000000) {
        pmu_vcore_req |= PMU_VCORE_RS_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_RS_FREQ_HIGH;
    }
    int_unlock(lock);

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
}

void pmu_rs_adc_freq_config(uint32_t freq)
{
    uint32_t lock;

    lock = int_lock();
    if (freq >= 96000000) {
        pmu_vcore_req |= PMU_VCORE_RS_ADC_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_RS_ADC_FREQ_HIGH;
    }
    int_unlock(lock);

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
}


void BOOT_TEXT_SRAM_LOC pmu_sys_freq_config(enum HAL_CMU_FREQ_T freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#if defined(MCU_HIGH_PERFORMANCE_MODE) || defined(ULTRA_LOW_POWER) || !defined(OSC_26M_X4_AUD2BB)
    uint32_t lock;
    enum PMU_VCORE_REQ_T old_req;
    bool update = false;

    lock = int_lock();
    old_req = pmu_vcore_req;
    pmu_vcore_req &= ~(PMU_VCORE_SYS_FREQ_HIGH | PMU_VCORE_SYS_FREQ_MEDIUM);
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    if (freq > HAL_CMU_FREQ_104M) {
        if (high_perf_on) {
            // The real freq is 350M
            pmu_vcore_req |= PMU_VCORE_SYS_FREQ_HIGH;
        } else {
            pmu_vcore_req |= PMU_VCORE_SYS_FREQ_MEDIUM;
        }
    } else {
#ifndef OSC_26M_X4_AUD2BB
        if (freq == HAL_CMU_FREQ_104M) {
            // The real freq is 200M
            pmu_vcore_req |= PMU_VCORE_SYS_FREQ_MEDIUM;
        }
#endif
    }
#else
    if (freq > HAL_CMU_FREQ_104M) {
        pmu_vcore_req |= PMU_VCORE_SYS_FREQ_MEDIUM;
    }
#endif
    if (old_req != pmu_vcore_req) {
        update = true;
    }
    int_unlock(lock);

    if (!update) {
        // Nothing changes
        return;
    }

    if (pmu_power_mode == PMU_POWER_MODE_NONE) {
        // PMU not init yet
        pmu_dig_init_volt();
        return;
    }

    pmu_dig_set_volt(PMU_POWER_MODE_NONE);
#endif
#endif
}

void pmu_high_performance_mode_enable(bool enable)
{
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    uint16_t val;

    if (high_perf_on == enable) {
        return;
    }
    high_perf_on = enable;

    if (!enable) {
        if (high_perf_freq_mhz > 300) {
            // Switch to 52M to avoid using PLL
            hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);
            // Restore the default div
            rf_read(0x122, &val);
            val |= (1 << 9); // div=2 (bit9=1)
            rf_write(0x122, val);
            // Restore the sys freq
            hal_cmu_sys_set_freq(hal_sysfreq_get_hw_freq());
        }
        // Restore the default PLL freq (384M)
        bbpll_freq_pll_config(192 * 1000000 * 2);
    }

    pmu_sys_freq_config(hal_sysfreq_get_hw_freq());

    if (enable) {
        uint32_t pll_freq;

        // Change freq first, and then change divider.
        // Otherwise there will be an instant very high freq sent to digital domain.

        if (high_perf_freq_mhz <= 300) {
            pll_freq = high_perf_freq_mhz * 1000000 * 2;
        } else {
            pll_freq = high_perf_freq_mhz * 1000000;
        }
        bbpll_freq_pll_config(pll_freq);

        if (high_perf_freq_mhz > 300) {
            // Switch to 52M to avoid using PLL
            hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);
            rf_read(0x122, &val);
            val &= ~(1 << 9); // div=1 (bit9=0)
            rf_write(0x122, val);
            // Restore the sys freq
            hal_cmu_sys_set_freq(hal_sysfreq_get_hw_freq());
        }
    }
#endif
}

void pmu_usb_config(enum PMU_USB_CONFIG_TYPE_T type)
{
    uint32_t lock;
    bool update;
    unsigned short ldo_on, deep_sleep_on;

    update = false;

    if (type == PMU_USB_CONFIG_TYPE_NONE) {
        usbphy_ldo_config(false);

        ldo_on = PMU_LDO_OFF;
        deep_sleep_on = PMU_DSLEEP_MODE_OFF;
    } else {
        ldo_on = PMU_LDO_ON;
        deep_sleep_on = PMU_DSLEEP_MODE_ON;
    }

    lock = int_lock();

    if (type == PMU_USB_CONFIG_TYPE_NONE) {
        pmu_vusb_req &= ~PMU_VUSB_REQ_USB;
        if (pmu_vusb_req == 0) {
            update = true;
        }
    } else {
        if (pmu_vusb_req == 0) {
            update = true;
        }
        pmu_vusb_req |= PMU_VUSB_REQ_USB;
    }

    if (update) {
        pmu_module_config(PMU_USB, PMU_MANUAL_MODE, ldo_on, PMU_LP_MODE_ON, deep_sleep_on);
        }

#ifdef USB_HIGH_SPEED
    if (type == PMU_USB_CONFIG_TYPE_NONE) {
        pmu_vcore_req &= ~PMU_VCORE_USB_HS_ENABLED;
    } else {
        pmu_vcore_req |= PMU_VCORE_USB_HS_ENABLED;
    }
#endif

    int_unlock(lock);

    if (pmu_power_mode != PMU_POWER_MODE_NONE) {
        // PMU has been inited
        pmu_dig_set_volt(PMU_POWER_MODE_NONE);
    }

    if (type != PMU_USB_CONFIG_TYPE_NONE) {
        usbphy_ldo_config(true);
    }
}

#if !defined(FPGA) && !defined(PROGRAMMER)
struct PMU_CHG_CTX_T {
    uint16_t pmu_chg_status;
};

struct PMU_CHG_CTX_T BOOT_BSS_LOC pmu_chg_ctx;

void BOOT_TEXT_SRAM_LOC pmu_charger_save_context(void)
{
    pmu_read(PMU_REG_CHARGER_STATUS, &pmu_chg_ctx.pmu_chg_status);
}

enum PMU_POWER_ON_CAUSE_T pmu_charger_poweron_status(void)
{
    enum PMU_POWER_ON_CAUSE_T pmu_power_on_cause = PMU_POWER_ON_CAUSE_NONE;

    if (pmu_chg_ctx.pmu_chg_status & AC_ON_DET_OUT){
        pmu_power_on_cause = PMU_POWER_ON_CAUSE_CHARGER_ACOFF;
    }else if (pmu_chg_ctx.pmu_chg_status & AC_ON){
        pmu_power_on_cause = PMU_POWER_ON_CAUSE_CHARGER_ACON;
    }
    return pmu_power_on_cause;
}
#endif

void pmu_charger_init(void)
{
    unsigned short readval_cfg;
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_CFG, &readval_cfg);
    readval_cfg &= ~(REG_CHARGE_OUT_INTR_MSK | REG_CHARGE_IN_INTR_MSK |
        REG_AC_ON_OUT_EN | REG_AC_ON_IN_EN);
    pmu_write(PMU_REG_CHARGER_CFG ,readval_cfg);
    int_unlock(lock);

    hal_sys_timer_delay(MS_TO_TICKS(1));

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_CFG, &readval_cfg);
    readval_cfg |= REG_AC_ON_OUT_EN | REG_AC_ON_IN_EN;
    readval_cfg = SET_BITFIELD(readval_cfg, REG_AC_ON_DB_VALUE, 8);
    pmu_write(PMU_REG_CHARGER_CFG ,readval_cfg);
    int_unlock(lock);
}

static void pmu_charger_irq_handler(PMU_IRQ_HDLR_PARAM)
{
    enum PMU_CHARGER_STATUS_T status = PMU_CHARGER_UNKNOWN;
    unsigned short readval;

#ifdef PMU_IRQ_UNIFIED
    readval = irq_status;
#else
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_STATUS, &readval);
    pmu_write(PMU_REG_CHARGER_STATUS, readval);
    int_unlock(lock);
#endif
    PMU_DEBUG_TRACE(3,"%s REG_%02X=0x%04X", __func__, PMU_REG_CHARGER_STATUS, readval);

    if ((readval & (INTR_MSKED_CHARGE_IN | INTR_MSKED_CHARGE_OUT)) == 0){
        PMU_DEBUG_TRACE(1,"%s SKIP", __func__);
        return;
    } else if ((readval & (INTR_MSKED_CHARGE_IN | INTR_MSKED_CHARGE_OUT)) ==
            (INTR_MSKED_CHARGE_IN | INTR_MSKED_CHARGE_OUT)) {
        PMU_DEBUG_TRACE(1,"%s DITHERING", __func__);
        hal_sys_timer_delay(2);
    } else {
        PMU_DEBUG_TRACE(1,"%s NORMAL", __func__);
    }

    status = pmu_charger_get_status();

    if (charger_irq_handler) {
        charger_irq_handler(status);
    }
}

void pmu_charger_set_irq_handler(PMU_CHARGER_IRQ_HANDLER_T handler)
{
    uint32_t lock;
    uint16_t val;

    charger_irq_handler = handler;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_CFG, &val);
    if (handler) {
        val |= REG_CHARGE_IN_INTR_MSK | REG_CHARGE_OUT_INTR_MSK;
#ifdef PMU_FIRST_ACIN_IRQ_DISABLE
        val &= ~REG_AC_ON_IN_EN;
#endif
    } else {
        val &= ~(REG_CHARGE_IN_INTR_MSK | REG_CHARGE_OUT_INTR_MSK);
    }
    pmu_write(PMU_REG_CHARGER_CFG, val);
#ifdef PMU_FIRST_ACIN_IRQ_DISABLE
    val = GET_BITFIELD(val, REG_AC_ON_DB_VALUE);
    // 3 lpo cycles as default and add 1 lpo cycle margin.
    val = val + 3 + 1;
    hal_sys_timer_delay_us(val * 32);
#endif

#ifdef PMU_IRQ_UNIFIED
   pmu_set_irq_unified_handler(PMU_IRQ_TYPE_CHARGER, handler ? pmu_charger_irq_handler : NULL);
#else
    if (handler) {
        NVIC_SetVector(CHARGER_IRQn, (uint32_t)pmu_charger_irq_handler);
        NVIC_SetPriority(CHARGER_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_ClearPendingIRQ(CHARGER_IRQn);
        NVIC_EnableIRQ(CHARGER_IRQn);
    } else {
        NVIC_DisableIRQ(CHARGER_IRQn);
    }
#endif

#ifdef PMU_FIRST_ACIN_IRQ_DISABLE
    if (handler) {
        pmu_read(PMU_REG_CHARGER_CFG, &val);
        val |= REG_AC_ON_IN_EN;
        pmu_write(PMU_REG_CHARGER_CFG, val);
    }
#endif
    int_unlock(lock);
}

void pmu_charger_plugin_config(void)
{
}

void pmu_charger_plugout_config(void)
{
}

enum PMU_CHARGER_STATUS_T pmu_charger_get_status(void)
{
    unsigned short readval;
    enum PMU_CHARGER_STATUS_T status;

    pmu_read(PMU_REG_CHARGER_STATUS, &readval);
    if (readval & AC_ON)
        status = PMU_CHARGER_PLUGIN;
    else
        status = PMU_CHARGER_PLUGOUT;

    return status;
}

#ifdef RTC_ENABLE
void pmu_rtc_enable(void)
{
    uint16_t readval;
    uint32_t lock;

#ifdef SIMU
    // Set RTC counter to 1KHz
    pmu_write(PMU_REG_RTC_DIV_1HZ, 32 - 2);
#else
    // Set RTC counter to 1Hz
    pmu_write(PMU_REG_RTC_DIV_1HZ, CONFIG_SYSTICK_HZ * 2 - 2);
#endif

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);
    readval |= RTC_POWER_ON_EN | PU_LPO_DR | PU_LPO_REG;
    pmu_write(PMU_REG_POWER_KEY_CFG, readval);

    pmu_read(PMU_REG_NOT_RESET_61, &readval);
    readval |= REG_NOT_RESET_RTC_ENABLED;
    pmu_write(PMU_REG_NOT_RESET_61, readval);
    int_unlock(lock);
}

void pmu_rtc_disable(void)
{
    uint16_t readval;
    uint32_t lock;

    pmu_rtc_clear_alarm();

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);
    readval &= ~(RTC_POWER_ON_EN | PU_LPO_DR);
    pmu_write(PMU_REG_POWER_KEY_CFG, readval);

    pmu_read(PMU_REG_NOT_RESET_61, &readval);
    readval &= ~REG_NOT_RESET_RTC_ENABLED;
    pmu_write(PMU_REG_NOT_RESET_61, readval);
    int_unlock(lock);
}

int BOOT_TEXT_SRAM_LOC pmu_rtc_enabled(void)
{
    uint16_t readval;

    pmu_read(PMU_REG_NOT_RESET_61, &readval);

    return !!(readval & REG_NOT_RESET_RTC_ENABLED);
}

void pmu_rtc_set(uint32_t seconds)
{
    uint16_t high, low;

    // Need 3 seconds to load a new value
    seconds += 3;

    high = seconds >> 16;
    low = seconds & 0xFFFF;

    pmu_write(PMU_REG_RTC_LOAD_LOW, low);
    pmu_write(PMU_REG_RTC_LOAD_HIGH, high);
}

uint32_t pmu_rtc_get(void)
{
    uint16_t high, low, high2;

    pmu_read(PMU_REG_RTC_VAL_HIGH, &high);
    pmu_read(PMU_REG_RTC_VAL_LOW, &low);
    // Handle counter wrap
    pmu_read(PMU_REG_RTC_VAL_HIGH, &high2);
    if (high != high2) {
        high = high2;
        pmu_read(PMU_REG_RTC_VAL_LOW, &low);
    }

    return (high << 16) | low;
}

void pmu_rtc_set_alarm(uint32_t seconds)
{
    uint16_t readval;
    uint16_t high, low;
    uint32_t lock;

    // Need 1 second to raise the interrupt
    if (seconds > 0) {
        seconds -= 1;
    }

    high = seconds >> 16;
    low = seconds & 0xFFFF;

    pmu_write(PMU_REG_INT_CLR, RTC_INT_CLR_1);

    pmu_write(PMU_REG_RTC_MATCH1_LOW, low);
    pmu_write(PMU_REG_RTC_MATCH1_HIGH, high);

    lock = int_lock();
    pmu_read(PMU_REG_INT_EN, &readval);
    readval |= RTC_INT_EN_1;
    pmu_write(PMU_REG_INT_EN, readval);
    int_unlock(lock);
}

uint32_t BOOT_TEXT_SRAM_LOC pmu_rtc_get_alarm(void)
{
    uint16_t high, low;

    pmu_read(PMU_REG_RTC_MATCH1_LOW, &low);
    pmu_read(PMU_REG_RTC_MATCH1_HIGH, &high);

    // Compensate the alarm offset
    return (uint32_t)((high << 16) | low) + 1;
}

void pmu_rtc_clear_alarm(void)
{
    uint16_t readval;
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_INT_EN, &readval);
    readval &= ~RTC_INT_EN_1;
    pmu_write(PMU_REG_INT_EN, readval);
    int_unlock(lock);

    pmu_write(PMU_REG_INT_CLR, RTC_INT_CLR_1);
}

int BOOT_TEXT_SRAM_LOC pmu_rtc_alarm_status_set(void)
{
    uint16_t readval;

    pmu_read(PMU_REG_INT_EN, &readval);

    return !!(readval & RTC_INT_EN_1);
}

int pmu_rtc_alarm_alerted()
{
    uint16_t readval;

    pmu_read(PMU_REG_INT_STATUS, &readval);

    return !!(readval & RTC_INT_1);
}

static void pmu_rtc_irq_handler(PMU_IRQ_HDLR_PARAM)
{
    uint32_t seconds;
    bool alerted;

#ifdef PMU_IRQ_UNIFIED
    alerted = !!(irq_status & RTC_INT_1);
#else
    alerted = pmu_rtc_alarm_alerted();
#endif

    if (alerted) {
        pmu_rtc_clear_alarm();

        if (rtc_irq_handler) {
            seconds = pmu_rtc_get();
            rtc_irq_handler(seconds);
        }
    }
}

void pmu_rtc_set_irq_handler(PMU_RTC_IRQ_HANDLER_T handler)
{
    uint16_t readval;
    uint32_t lock;

    rtc_irq_handler = handler;

    lock = int_lock();
    pmu_read(PMU_REG_INT_MASK, &readval);
    if (handler) {
        readval |= RTC_INT1_MSK;
    } else {
        readval &= ~RTC_INT1_MSK;
    }
    pmu_write(PMU_REG_INT_MASK, readval);

#ifdef PMU_IRQ_UNIFIED
    pmu_set_irq_unified_handler(PMU_IRQ_TYPE_RTC, handler ? pmu_rtc_irq_handler : NULL);
#else
    if (handler) {
        NVIC_SetVector(RTC_IRQn, (uint32_t)pmu_rtc_irq_handler);
        NVIC_SetPriority(RTC_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_ClearPendingIRQ(RTC_IRQn);
        NVIC_EnableIRQ(RTC_IRQn);
    } else {
        NVIC_DisableIRQ(RTC_IRQn);
    }
#endif
    int_unlock(lock);
}

int pmu_rtc_clk_div_set(uint16_t freq)
{
    if (freq < 2) {
        return -1;
    }

    freq -= 2;

    pmu_write(PMU_REG_RTC_DIV_1HZ, freq);

    return 0;
}
#endif

#ifdef PMU_IRQ_UNIFIED
static void pmu_general_irq_handler(void)
{
    uint32_t lock;
    uint16_t val;
    bool pwrkey, charger, gpadc, rtc, gpio, wdt;

    pwrkey = false;
    charger = false;
    gpadc = false;
    rtc = false;
    gpio = false;
    wdt = false;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_STATUS, &val);
    if (val & (POWER_ON_RELEASE | POWER_ON_PRESS)) {
        pwrkey = true;
    }
    if (val & (INTR_MSKED_CHARGE_IN | INTR_MSKED_CHARGE_OUT)) {
        charger = true;
    }
    if (pwrkey || charger) {
        pmu_write(PMU_REG_CHARGER_STATUS, val);
    }
    int_unlock(lock);

    if (pwrkey) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_PWRKEY]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_PWRKEY](val);
        }
    }
    if (charger) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_CHARGER]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_CHARGER](val);
        }
    }

    lock = int_lock();
    pmu_read(PMU_REG_INT_MSKED_STATUS, &val);
    if (val & (KEY_ERR1_INTR_MSKED | KEY_ERR0_INTR_MSKED | KEY_PRESS_INTR_MSKED | KEY_RELEASE_INTR_MSKED |
            SAMPLE_DONE_INTR_MSKED | CHAN_DATA_INTR_MSKED_MASK)) {
        gpadc = true;
    }
    if (val & (RTC_INT1_MSKED | RTC_INT0_MSKED)) {
        rtc = true;
    }
    if (gpadc || rtc) {
        pmu_write(PMU_REG_INT_CLR, val);
    }
    int_unlock(lock);

    if (gpadc) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_GPADC]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_GPADC](val);
        }
    }
    if (rtc) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_RTC]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_RTC](val);
        }
    }

    lock = int_lock();
    pmu_read(PMU_REG_LED_IO_IN, &val);
    if (val & (PMU_GPIO_INTR_MSKED1 | PMU_GPIO_INTR_MSKED2)) {
        gpio = true;
    }
    if (val & WDT_INTR_MSKED) {
        wdt = true;
    }
    if (gpio) {
        pmu_write(PMU_REG_LED_IO_IN, val);
    }
    if (wdt) {
        // No need to clear
    }
    int_unlock(lock);

    if (gpio) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_GPIO]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_GPIO](val);
        }
    }
    if (wdt) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_WDT]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_WDT](val);
        }
    }
}

int pmu_set_irq_unified_handler(enum PMU_IRQ_TYPE_T type, PMU_IRQ_UNIFIED_HANDLER_T hdlr)
{
    bool update;
    uint32_t lock;
    int i;

    if (type >= PMU_IRQ_TYPE_QTY) {
        return 1;
    }

    enum PMU_REG_T reg;
    uint16_t val;
    uint16_t mask;

    if (type == PMU_IRQ_TYPE_GPADC) {
        reg = PMU_REG_USB_CFG2;
        mask = GPADC_INTR_MERGED_MSK;
    } else if (type == PMU_IRQ_TYPE_RTC) {
        reg = PMU_REG_USB_CFG2;
        mask = RTC_INTR_TMP_MERGED_MSK;
    } else if (type == PMU_IRQ_TYPE_CHARGER) {
        reg = PMU_REG_USB_CFG2;
        mask = CHARGE_INTR_MERGED_MSK;
    } else if (type == PMU_IRQ_TYPE_GPIO) {
        reg = PMU_REG_USB_CFG3;
        mask = PMU_GPIO_INTR_MSKED1_MERGED_MSK | PMU_GPIO_INTR_MSKED2_MERGED_MSK;
    } else if (type == PMU_IRQ_TYPE_WDT) {
        reg = PMU_REG_USB_CFG3;
        mask = WDT_INTR_MSKED_MERGED_MSK;
    } else if (type == PMU_IRQ_TYPE_PWRKEY) {
        reg = PMU_REG_USB_CFG3;
        mask = POWER_ON_INTR_MERGED_MSK;
    } else {
        return 2;
    }

    update = false;

    lock = int_lock();

    for (i = 0; i < PMU_IRQ_TYPE_QTY; i++) {
        if (pmu_irq_hdlrs[i]) {
            break;
        }
    }

    pmu_irq_hdlrs[type] = hdlr;

    pmu_read(reg, &val);
    if (hdlr) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    pmu_write(reg, val);

    if (hdlr) {
        update = (i >= PMU_IRQ_TYPE_QTY);
    } else {
        if (i == type) {
            for (; i < PMU_IRQ_TYPE_QTY; i++) {
                if (pmu_irq_hdlrs[i]) {
                    break;
                }
            }
            update = (i >= PMU_IRQ_TYPE_QTY);
        }
    }

    if (update) {
        if (hdlr) {
            NVIC_SetVector(RTC_IRQn, (uint32_t)pmu_general_irq_handler);
            NVIC_SetPriority(RTC_IRQn, IRQ_PRIORITY_NORMAL);
            NVIC_ClearPendingIRQ(RTC_IRQn);
            NVIC_EnableIRQ(RTC_IRQn);
        } else {
            NVIC_DisableIRQ(RTC_IRQn);
        }
    }

    int_unlock(lock);

    return 0;
}
#endif

enum HAL_PWRKEY_IRQ_T pmu_pwrkey_irq_value_to_state(uint16_t irq_status)
{
    enum HAL_PWRKEY_IRQ_T state = HAL_PWRKEY_IRQ_NONE;

    if (irq_status & POWER_ON_PRESS) {
        state |= HAL_PWRKEY_IRQ_FALLING_EDGE;
    }

    if (irq_status & POWER_ON_RELEASE) {
        state |= HAL_PWRKEY_IRQ_RISING_EDGE;
    }

    return state;
}

int pmu_debug_config_ana(uint16_t volt)
{
    return 0;
}

int pmu_debug_config_codec(uint16_t volt)
{
    return 0;
}

int pmu_debug_config_vcrystal(bool on)
{
    return 0;
}

int pmu_debug_config_audio_output(bool diff)
{
    return 0;
}

void pmu_debug_reliability_test(int stage)
{
    uint16_t volt;

    if (stage == 0) {
        volt = PMU_DCDC_ANA_1_2V;
    } else {
        volt = PMU_DCDC_ANA_1_3V;
    }
    pmu_dcdc_ana_set_volt(volt, ana_lp_dcdc);
}

void pmu_led_set_hiz(enum HAL_GPIO_PIN_T pin)
{
    uint16_t val;
    uint32_t lock;

    if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
        lock = int_lock();
        if (pin == HAL_GPIO_PIN_LED1) {
            pmu_read(PMU_REG_LED_CFG_IO1, &val);
            val = (val | REG_LED_IO1_PU | REG_LED_IO1_OENB_PRE) & ~(REG_LED_IO1_PUEN | REG_LED_IO1_PDEN | REG_LED_IO1_RX_EN);
            pmu_write(PMU_REG_LED_CFG_IO1, val);
        } else {
            pmu_read(PMU_REG_LED_CFG_IO2, &val);
            val = (val | REG_LED_IO2_PU | REG_LED_IO2_OENB_PRE) & ~(REG_LED_IO2_PUEN | REG_LED_IO2_PDEN | REG_LED_IO2_RX_EN);
            pmu_write(PMU_REG_LED_CFG_IO2, val);
        }
        int_unlock(lock);
    }
}

void pmu_led_set_direction(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_DIR_T dir)
{
    uint16_t val;
    uint32_t lock;

    if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
        lock = int_lock();
        if (pin == HAL_GPIO_PIN_LED1) {
            pmu_read(PMU_REG_LED_CFG_IO1, &val);
            val |= REG_LED_IO1_PU;
            if (dir == HAL_GPIO_DIR_IN) {
                val |= REG_LED_IO1_OENB_PRE | REG_LED_IO1_RX_EN;
            } else {
                val &= ~(REG_LED_IO1_OENB_PRE | REG_LED_IO1_RX_EN);
            }
            pmu_write(PMU_REG_LED_CFG_IO1, val);
        } else {
            pmu_read(PMU_REG_LED_CFG_IO2, &val);
            val |= REG_LED_IO2_PU;
            if (dir == HAL_GPIO_DIR_IN) {
                val |= REG_LED_IO2_OENB_PRE | REG_LED_IO2_RX_EN;
            } else {
                val &= ~(REG_LED_IO2_OENB_PRE | REG_LED_IO2_RX_EN);
            }
            pmu_write(PMU_REG_LED_CFG_IO2, val);
        }
        int_unlock(lock);
    }
}

enum HAL_GPIO_DIR_T pmu_led_get_direction(enum HAL_GPIO_PIN_T pin)
{
    uint16_t val;

    if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
        if (pin == HAL_GPIO_PIN_LED1) {
            pmu_read(PMU_REG_LED_CFG_IO1, &val);
            return (val & REG_LED_IO1_OENB_PRE) ? HAL_GPIO_DIR_IN : HAL_GPIO_DIR_OUT;
        } else {
            pmu_read(PMU_REG_LED_CFG_IO2, &val);
            return (val & REG_LED_IO2_OENB_PRE) ? HAL_GPIO_DIR_IN : HAL_GPIO_DIR_OUT;
        }
    } else {
        return HAL_GPIO_DIR_IN;
    }
}

void pmu_led_set_voltage_domains(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    enum PMU_LED_VOLT_T {
        PMU_LED_VOLT_VBAT,
        PMU_LED_VOLT_VMEM,
        PMU_LED_VOLT_VIO,
    };
    enum PMU_LED_VOLT_T sel;
    uint16_t val;
    uint32_t lock;

    if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        if (volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
            sel = PMU_LED_VOLT_VIO;
        } else if (volt == HAL_IOMUX_PIN_VOLTAGE_MEM) {
            sel = PMU_LED_VOLT_VMEM;
        } else {
            sel = PMU_LED_VOLT_VBAT;
        }

        lock = int_lock();
        if (pin == HAL_IOMUX_PIN_LED1) {
            pmu_read(PMU_REG_LED_CFG_IO1, &val);
            val = SET_BITFIELD(val, REG_LED_IO1_SEL, sel);
            pmu_write(PMU_REG_LED_CFG_IO1, val);
        } else {
            pmu_read(PMU_REG_LED_CFG_IO2, &val);
            val = SET_BITFIELD(val, REG_LED_IO2_SEL, sel);
            pmu_write(PMU_REG_LED_CFG_IO2, val);
        }
        int_unlock(lock);
    }
}

void pmu_led_set_pull_select(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_PULL_SELECT_T pull_sel)
{
    uint16_t val;
    uint32_t lock;

    if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        lock = int_lock();
        if (pin == HAL_IOMUX_PIN_LED1) {
            pmu_read(PMU_REG_LED_CFG_IO1, &val);
            val &= ~(REG_LED_IO1_PDEN | REG_LED_IO1_PUEN);
            if (pull_sel == HAL_IOMUX_PIN_PULLUP_ENABLE) {
                val |= REG_LED_IO1_PUEN;
            } else if (pull_sel == HAL_IOMUX_PIN_PULLDOWN_ENABLE) {
                val |= REG_LED_IO1_PDEN;
            }
            pmu_write(PMU_REG_LED_CFG_IO1, val);
        } else {
            pmu_read(PMU_REG_LED_CFG_IO2, &val);
            val &= ~(REG_LED_IO2_PDEN | REG_LED_IO2_PUEN);
            if (pull_sel == HAL_IOMUX_PIN_PULLUP_ENABLE) {
                val |= REG_LED_IO2_PUEN;
            } else if (pull_sel == HAL_IOMUX_PIN_PULLDOWN_ENABLE) {
                val |= REG_LED_IO2_PDEN;
            }
            pmu_write(PMU_REG_LED_CFG_IO2, val);
        }
        int_unlock(lock);
    }
}

void pmu_led_set_value(enum HAL_GPIO_PIN_T pin, int data)
{
    uint32_t lock;
    uint16_t val;

    if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
        lock = int_lock();
        if (pin == HAL_GPIO_PIN_LED1) {
#ifdef PMU_LED_VIA_PWM
            pmu_write(PMU_REG_PWM2_TOGGLE, 0xFFFF);
            pmu_read(PMU_REG_PWM2_BR_EN, &val);
            val &= ~REG_PWM2_BR_EN;
            val |= PWM_SELECT_EN;
            if (data) {
                val &= ~PWM_SELECT_INV;
            } else {
                val |= PWM_SELECT_INV;
            }
            pmu_write(PMU_REG_PWM2_BR_EN, val);
#else
            pmu_read(PMU_REG_PWM2_EN, &val);
            if (data) {
                val |= REG_LED0_OUT;
            } else {
                val &= ~REG_LED0_OUT;
            }
            pmu_write(PMU_REG_PWM2_EN, val);
#endif
        }else {
#ifdef PMU_LED_VIA_PWM
            pmu_write(PMU_REG_PWMB_TOGGLE, 0xFFFF);
            pmu_read(PMU_REG_PWMB_BR_EN, &val);
            val &= ~REG_PWMB_BR_EN;
            val |= PWMB_SELECT_EN;
            if (data) {
                val &= ~PWMB_SELECT_INV;
            } else {
                val |= PWMB_SELECT_INV;
            }
            pmu_write(PMU_REG_PWMB_BR_EN, val);
#else
            pmu_read(PMU_REG_PWMB_EN, &val);
            if (data) {
                val |= REG_LEDB_OUT;
            } else {
                val &= ~REG_LEDB_OUT;
            }
            pmu_write(PMU_REG_PWMB_EN, val);
#endif
        }
        int_unlock(lock);
    }
}

int pmu_led_get_value(enum HAL_GPIO_PIN_T pin)
{
    uint16_t val;
    int data = 0;

    if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
        pmu_read(PMU_REG_LED_IO_IN, &val);
        if (pin == HAL_GPIO_PIN_LED1) {
            data = LED_IO1_IN_DB;
        } else {
            data = LED_IO2_IN_DB;
        }
        data &= val;
    }

    return !!data;
}

void pmu_led_uart_enable(enum HAL_IOMUX_PIN_T pin)
{
    uint16_t val;
    if (pin == HAL_IOMUX_PIN_LED1) {
        pmu_read(PMU_REG_UART1_CFG, &val);
        val &= ~(REG_UART_LEDA_SEL | REG_PMU_UART_DR1);
        val |=  REG_GPIO_I_SEL;
        pmu_write(PMU_REG_UART1_CFG, val);

        pmu_read(PMU_REG_UART2_CFG, &val);
        val |= REG_PMU_UART_DR2;
        pmu_write(PMU_REG_UART2_CFG, val);

        pmu_read(PMU_REG_LED_CFG_IO1, &val);
        val |= REG_LED_IO1_PUEN;
        pmu_write(PMU_REG_LED_CFG_IO1, val);
    } else {
        pmu_read(PMU_REG_UART1_CFG, &val);
        val &= ~ REG_GPIO_I_SEL;
        val |= REG_PMU_UART_DR1;
        pmu_write(PMU_REG_UART1_CFG, val);

        pmu_read(PMU_REG_UART2_CFG, &val);
        val &= ~(REG_UART_LEDB_SEL | REG_PMU_UART_DR2);
        pmu_write(PMU_REG_UART2_CFG, val);

        pmu_read(PMU_REG_LED_CFG_IO2, &val);
        val |= REG_LED_IO2_PUEN;
        pmu_write(PMU_REG_LED_CFG_IO2, val);
    }
}

void pmu_led_uart_disable(enum HAL_IOMUX_PIN_T pin)
{
    uint16_t val;
    if (pin == HAL_IOMUX_PIN_LED1) {
        pmu_read(PMU_REG_UART1_CFG, &val);
        val |= (REG_UART_LEDA_SEL | REG_PMU_UART_DR1);
        val &=  ~REG_GPIO_I_SEL;
        pmu_write(PMU_REG_UART1_CFG, val);

        pmu_read(PMU_REG_UART2_CFG, &val);
        val &= ~REG_PMU_UART_DR2;
        pmu_write(PMU_REG_UART2_CFG, val);

        pmu_read(PMU_REG_LED_CFG_IO1, &val);
        val &= ~REG_LED_IO1_PUEN;
        pmu_write(PMU_REG_LED_CFG_IO1, val);
    } else {
        pmu_read(PMU_REG_UART1_CFG, &val);
        val |= REG_GPIO_I_SEL;
        val &= ~REG_PMU_UART_DR1;
        pmu_write(PMU_REG_UART1_CFG, val);

        pmu_read(PMU_REG_UART2_CFG, &val);
        val |= (REG_UART_LEDB_SEL | REG_PMU_UART_DR2);
        pmu_write(PMU_REG_UART2_CFG, val);

        pmu_read(PMU_REG_LED_CFG_IO2, &val);
        val &= ~REG_LED_IO2_PUEN;
        pmu_write(PMU_REG_LED_CFG_IO2, val);
    }
}

void pmu_led_breathing_enable(enum HAL_IOMUX_PIN_T pin, const struct PMU_LED_BR_CFG_T *cfg)
{
    uint32_t st1;
    uint32_t st2;
    uint32_t subcnt_data;
    uint8_t tg;
    uint16_t val;
    uint32_t lock;
    if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        st1 = MS_TO_TICKS(cfg->off_time_ms);
        if (st1 > 0xFFFF) {
            st1 = 0xFFFF;
        }
        st2 = MS_TO_TICKS(cfg->on_time_ms);
        if (st2 > 0xFFFF) {
            st2 = 0xFFFF;
        }
        subcnt_data = MS_TO_TICKS(cfg->fade_time_ms);
        subcnt_data = integer_sqrt_nearest(subcnt_data);
        if (subcnt_data > (SUBCNT_DATA2_MASK >> SUBCNT_DATA2_SHIFT)) {
            subcnt_data = (SUBCNT_DATA2_MASK >> SUBCNT_DATA2_SHIFT);
        }
        // TODO: Keep compatible with digital PWM module (can be removed after 2500)
        if (subcnt_data > 0xFE) {
            subcnt_data = 0xFE;
        }
        tg = 1;

        pmu_led_set_direction((enum HAL_GPIO_PIN_T)pin, HAL_GPIO_DIR_OUT);
        pmu_led_set_pull_select(pin, HAL_IOMUX_PIN_NOPULL);
        pmu_led_set_voltage_domains(pin, HAL_IOMUX_PIN_VOLTAGE_VBAT);

        lock = int_lock();
        if (pin == HAL_IOMUX_PIN_LED1) {
            pmu_write(PMU_REG_PWM2_TOGGLE, st2);
            pmu_write(PMU_REG_PWM2_ST1, st1);
            val = SUBCNT_DATA2(subcnt_data) | TG_SUBCNT_D2_ST(tg);
            pmu_write(PMU_REG_PWM2_EN, val);
            pmu_read(PMU_REG_PWM2_BR_EN, &val);
            val = (val & ~REG_CLK_PWM_DIV_MASK) | REG_CLK_PWM_DIV(0) |
                REG_PWM_CLK_EN | REG_PWM2_BR_EN | PWM_SELECT_EN;
            pmu_write(PMU_REG_PWM2_BR_EN, val);
        } else {
            pmu_write(PMU_REG_PWMB_TOGGLE, st2);
            pmu_write(PMU_REG_PWMB_ST1, st1);
            val = SUBCNT_DATAB(subcnt_data) | TG_SUBCNT_DB_ST(tg);
            pmu_write(PMU_REG_PWMB_EN, val);
            pmu_read(PMU_REG_PWMB_BR_EN, &val);
            val = (val & ~REG_CLK_PWMB_DIV_MASK) | REG_CLK_PWMB_DIV(0) |
                REG_PWMB_CLK_EN | REG_PWMB_BR_EN | PWMB_SELECT_EN;
            pmu_write(PMU_REG_PWMB_BR_EN, val);
        }
        int_unlock(lock);
    }
}

void pmu_led_breathing_disable(enum HAL_IOMUX_PIN_T pin)
{
    uint16_t val;
    uint32_t lock;
    if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        lock = int_lock();
        if (pin == HAL_IOMUX_PIN_LED1) {
            pmu_read(PMU_REG_PWM2_BR_EN, &val);
            val &= ~(REG_PWM_CLK_EN | REG_PWM2_BR_EN | PWM_SELECT_EN);
            pmu_write(PMU_REG_PWM2_BR_EN, val);
        } else {
            pmu_read(PMU_REG_PWMB_BR_EN, &val);
            val &= ~(REG_PWMB_CLK_EN | REG_PWMB_BR_EN | PWMB_SELECT_EN);
            pmu_write(PMU_REG_PWMB_BR_EN, val);
        }
        int_unlock(lock);
        pmu_led_set_direction((enum HAL_GPIO_PIN_T)pin, HAL_GPIO_DIR_IN);
        pmu_led_set_pull_select(pin, HAL_IOMUX_PIN_PULLUP_ENABLE);
    }
}

#ifdef PMU_IRQ_UNIFIED
static void pmu_gpio_irq_handler(uint16_t irq_status)
{
    if (irq_status & PMU_GPIO_INTR_MSKED1) {
        if (gpio_irq_handler[0]) {
            gpio_irq_handler[0](HAL_GPIO_PIN_LED1);
        }
    }
    if (irq_status & PMU_GPIO_INTR_MSKED2) {
        if (gpio_irq_handler[1]) {
            gpio_irq_handler[1](HAL_GPIO_PIN_LED2);
        }
    }
}

uint8_t pmu_gpio_setup_irq(enum HAL_GPIO_PIN_T pin, const struct HAL_GPIO_IRQ_CFG_T *cfg)
{
    uint32_t lock;
    uint16_t val;
    bool old_en;

    if (pin != HAL_GPIO_PIN_LED1 && pin != HAL_GPIO_PIN_LED2) {
        return 1;
    }

    lock = int_lock();

    old_en = (gpio_irq_en[0] || gpio_irq_en[1]);

    if (pin == HAL_GPIO_PIN_LED1) {
        gpio_irq_en[0] = cfg->irq_enable;
        gpio_irq_handler[0] = cfg->irq_handler;

        if (cfg->irq_enable) {
        }

        pmu_read(PMU_REG_UART1_CFG, &val);
        if (cfg->irq_enable) {
            val |= REG_PMU_GPIO_INTR_MSK1 | REG_PMU_GPIO_INTR_EN1;
            if (cfg->irq_debounce) {
                val &= ~PMU_DB_BYPASS1;
            } else {
                val |= PMU_DB_BYPASS1;
            }
        } else {
            val &= ~(REG_PMU_GPIO_INTR_MSK1 | REG_PMU_GPIO_INTR_EN1);
        }
        pmu_write(PMU_REG_UART1_CFG, val);
    } else {
        gpio_irq_en[1] = cfg->irq_enable;
        gpio_irq_handler[1] = cfg->irq_handler;

        pmu_read(PMU_REG_UART2_CFG, &val);
        if (cfg->irq_enable) {
            val |= REG_PMU_GPIO_INTR_MSK2 | REG_PMU_GPIO_INTR_EN2;
            if (cfg->irq_debounce) {
                val &= ~PMU_DB_BYPASS2;
            } else {
                val |= PMU_DB_BYPASS2;
            }
        } else {
            val &= ~(REG_PMU_GPIO_INTR_MSK1 | REG_PMU_GPIO_INTR_EN1);
        }
        pmu_write(PMU_REG_UART2_CFG, val);
    }

    if (cfg->irq_enable) {
        uint16_t type;
        uint16_t pol;

        type = (pin == HAL_GPIO_PIN_LED1) ? REG_EDGE_INTR_SEL1 : REG_EDGE_INTR_SEL2;
        pol = (pin == HAL_GPIO_PIN_LED1) ? REG_POS_INTR_SEL1 : REG_POS_INTR_SEL2;
        pmu_read(PMU_REG_MIC_BIAS_C, &val);
        if (cfg->irq_type == HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE) {
            val |= type;
            if (cfg->irq_polarity == HAL_GPIO_IRQ_POLARITY_LOW_FALLING) {
                val &= ~pol;
            } else {
                val |= pol;
            }
        } else {
            val &= ~type;
        }
        pmu_write(PMU_REG_MIC_BIAS_C, val);

        if (cfg->irq_type != HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE) {
            if (pin == HAL_GPIO_PIN_LED1) {
                pmu_read(PMU_REG_WDT_INT_CFG, &val);
                if (cfg->irq_polarity == HAL_GPIO_IRQ_POLARITY_LOW_FALLING) {
                    val |= REG_LOW_LEVEL_INTR_SEL1;
                } else {
                    val &= ~REG_LOW_LEVEL_INTR_SEL1;
                }
                pmu_write(PMU_REG_WDT_INT_CFG, val);
            } else {
                pmu_read(PMU_REG_UART2_CFG, &val);
                if (cfg->irq_polarity == HAL_GPIO_IRQ_POLARITY_LOW_FALLING) {
                    val |= REG_LOW_LEVEL_INTR_SEL2;
                } else {
                    val &= ~REG_LOW_LEVEL_INTR_SEL2;
                }
                pmu_write(PMU_REG_UART2_CFG, val);
            }
        }
    }

    if (old_en != cfg->irq_enable) {
        pmu_set_irq_unified_handler(PMU_IRQ_TYPE_GPIO, cfg->irq_enable ? pmu_gpio_irq_handler : NULL);
    }

    int_unlock(lock);

    return 0;
}

static void pmu_wdt_irq_handler(uint16_t irq_status)
{
    if (wdt_irq_handler) {
        wdt_irq_handler();
    }
}

void pmu_wdt_set_irq_handler(PMU_WDT_IRQ_HANDLER_T handler)
{
    uint32_t lock;
    uint16_t val;

    lock = int_lock();

    val = REG_WDT_INTR_CLR;
    pmu_write(PMU_REG_LED_IO_IN, val);

    wdt_irq_handler = handler;

    pmu_read(PMU_REG_WDT_INT_CFG, &val);
    if (handler) {
        val |= REG_WDT_INTR_EN | REG_WDT_INTR_MSK;
    } else {
        val &= ~(REG_WDT_INTR_EN | REG_WDT_INTR_MSK);
    }
    pmu_write(PMU_REG_WDT_INT_CFG, val);

    pmu_set_irq_unified_handler(PMU_IRQ_TYPE_WDT, handler ? pmu_wdt_irq_handler : NULL);

    int_unlock(lock);
}
#else
uint8_t pmu_gpio_setup_irq(enum HAL_GPIO_PIN_T pin, const struct HAL_GPIO_IRQ_CFG_T *cfg)
{
    ASSERT(false, "PMU_IRQ_UNIFIED must defined to use PMU GPIO IRQ");
    return 1;
}

void pmu_wdt_set_irq_handler(PMU_WDT_IRQ_HANDLER_T handler)
{
}
#endif

#ifdef __WATCHER_DOG_RESET__
struct PMU_WDT_CTX_T {
    bool enabled;
    uint16_t wdt_irq_timer;
    uint16_t wdt_reset_timer;
    uint16_t wdt_cfg;
};

static struct PMU_WDT_CTX_T BOOT_BSS_LOC wdt_ctx;

void BOOT_TEXT_SRAM_LOC pmu_wdt_save_context(void)
{
    uint16_t wdt_cfg = 0, wdt_timer = 0;
    pmu_read(PMU_REG_WDT_CFG, &wdt_cfg);
    if (wdt_cfg & (REG_WDT_RESET_EN | REG_WDT_EN)){
        wdt_ctx.enabled = true;
        wdt_ctx.wdt_cfg = wdt_cfg;
        pmu_read(PMU_REG_WDT_IRQ_TIMER, &wdt_timer);
        wdt_ctx.wdt_irq_timer = wdt_timer;
        pmu_read(PMU_REG_WDT_RESET_TIMER, &wdt_timer);
        wdt_ctx.wdt_reset_timer = wdt_timer;
    }
}

void BOOT_TEXT_SRAM_LOC pmu_wdt_restore_context(void)
{
    if (wdt_ctx.enabled) {
        pmu_write(PMU_REG_WDT_IRQ_TIMER, wdt_ctx.wdt_irq_timer);
        pmu_write(PMU_REG_WDT_RESET_TIMER, wdt_ctx.wdt_reset_timer);
        pmu_write(PMU_REG_WDT_CFG, wdt_ctx.wdt_cfg);
    }
}
#endif

int pmu_wdt_config(uint32_t irq_ms, uint32_t reset_ms)
{
#ifdef PMU_CLK_USE_EXT_CRYSTAL
    return 2;
#endif

    if (irq_ms > 0xFFFF) {
        return 1;
    }
    if (reset_ms > 0xFFFF) {
        return 1;
    }
    wdt_irq_timer = irq_ms;
    wdt_reset_timer = reset_ms;

    pmu_write(PMU_REG_WDT_IRQ_TIMER, wdt_irq_timer);
    pmu_write(PMU_REG_WDT_RESET_TIMER, wdt_reset_timer);

    return 0;
}

void pmu_wdt_start(void)
{
    uint16_t val;

    if (wdt_irq_timer == 0 && wdt_reset_timer == 0) {
        return;
    }

    pmu_read(PMU_REG_WDT_CFG, &val);
    val |= (REG_WDT_RESET_EN | REG_WDT_EN);
    pmu_write(PMU_REG_WDT_CFG, val);
}

#ifndef __WATCHER_DOG_RESET__
BOOT_TEXT_SRAM_LOC
#endif
void pmu_wdt_stop(void)
{
    uint16_t val;

    pmu_read(PMU_REG_WDT_CFG, &val);
    val &= ~(REG_WDT_RESET_EN | REG_WDT_EN);
    pmu_write(PMU_REG_WDT_CFG, val);

    val = REG_WDT_INTR_CLR;
    pmu_write(PMU_REG_LED_IO_IN, val);
}

void pmu_wdt_feed(void)
{
    if (wdt_irq_timer == 0 && wdt_reset_timer == 0) {
        return;
    }

    pmu_write(PMU_REG_WDT_IRQ_TIMER, wdt_irq_timer);
    pmu_write(PMU_REG_WDT_RESET_TIMER, wdt_reset_timer);
}

#ifdef PMU_CLK_USE_EXT_CRYSTAL
int pmu_wdt_reboot(void)
{
    uint16_t val;
    uint16_t val2;
    const uint16_t wdt_time_ms = 4;

    pmu_read(PMU_REG_MIC_BIAS_C, &val);
    val &= ~CLK_32K_SEL_1;
    pmu_write(PMU_REG_MIC_BIAS_C, val);
    hal_sys_timer_delay(MS_TO_TICKS(PMU_CLK_SWITCH_STABLE_TIME));

    pmu_read(PMU_REG_MIC_BIAS_C, &val);
    if (val & CLK_32K_SEL_1) {
        // Switch to lpo fail!
        return -1;
    }

    // Save boot cause reason
    pmu_read(PMU_REG_NOT_RESET_61, &val2);
    val2 |= REG_NOT_RESET_PMU_WDT_REBOOT;
    pmu_write(PMU_REG_NOT_RESET_61, val2);

    pmu_write(PMU_REG_WDT_IRQ_TIMER, wdt_time_ms / 2);
    pmu_write(PMU_REG_WDT_RESET_TIMER, wdt_time_ms / 2);

    // Clear intr by default
    val = REG_WDT_INTR_CLR;
    pmu_write(PMU_REG_LED_IO_IN, val);

    pmu_read(PMU_REG_WDT_INT_CFG, &val);
    val |= REG_WDT_INTR_EN | REG_WDT_INTR_MSK;
    pmu_write(PMU_REG_WDT_INT_CFG, val);

    pmu_read(PMU_REG_WDT_CFG, &val);
    val |= (REG_WDT_RESET_EN | REG_WDT_EN);
    pmu_write(PMU_REG_WDT_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(wdt_time_ms << 1));

    // Can't reach here
    val2 &= ~REG_NOT_RESET_PMU_WDT_REBOOT;
    pmu_write(PMU_REG_NOT_RESET_61, val2);
    PMU_INFO_TRACE_IMM(0, "\nError: pmu wdt reboot failed!\n");

    return 0;
}
#endif

void pmu_ntc_capture_enable(void)
{
    uint16_t val;

    pmu_read(PMU_REG_AVDD_EN, &val);
    val |= REG_PU_AVDD25_ANA;
    pmu_write(PMU_REG_AVDD_EN, val);

    pmu_read(PMU_REG_SAR_CLK_CFG, &val);
    val |= REG_SAR_INPUT_BUF_EN;
    pmu_write(PMU_REG_SAR_CLK_CFG, val);

    pmu_read(PMU_REG_SAR_PU_CFG, &val);
    val |= REG_SAR_PU_VREF_IN;
    pmu_write(PMU_REG_SAR_PU_CFG, val);

    pmu_read(PMU_REG_IPTAT_CORE_I_CFG, &val);
    val |= DIG_IPTAT_CORE_EN;
    pmu_write(PMU_REG_IPTAT_CORE_I_CFG, val);
}

void pmu_ntc_capture_disable(void)
{
    uint16_t val;

    pmu_read(PMU_REG_AVDD_EN, &val);
    val &= ~REG_PU_AVDD25_ANA;
    pmu_write(PMU_REG_AVDD_EN, val);

    pmu_read(PMU_REG_SAR_CLK_CFG, &val);
    val &= ~REG_SAR_INPUT_BUF_EN;
    pmu_write(PMU_REG_SAR_CLK_CFG, val);

    pmu_read(PMU_REG_SAR_PU_CFG, &val);
    val &= ~REG_SAR_PU_VREF_IN;
    pmu_write(PMU_REG_SAR_PU_CFG, val);

    pmu_read(PMU_REG_IPTAT_CORE_I_CFG, &val);
    val &= ~DIG_IPTAT_CORE_EN;
    pmu_write(PMU_REG_IPTAT_CORE_I_CFG, val);
}

void pmu_bt_reconn(bool en)
{
    uint16_t pmu_val = 0;

    pmu_read(0x38, &pmu_val);
    //TRACE(1,"origin: PMU_38=0x%x", pmu_val);
    pmu_val &= 0xfff0;      //clear [3:0]

    if(en){
        pmu_val |= 0xf;
    }
    pmu_write(0x38, pmu_val);

    //pmu_read(0x38, &pmu_val);
    //TRACE(1,"final: PMU_38=0x%x", pmu_val);
}

void pmu_ldo_vpa_enable(bool en)
{
    uint32_t lock;

    lock = int_lock();
    if (en) {
        pmu_module_set_volt(PMU_PA, PMU_LDO_PA_3_0V, PMU_LDO_PA_3_0V);
        pmu_module_config(PMU_PA,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
        hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);
    } else {
        pmu_module_config(PMU_PA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    }
    int_unlock(lock);
}

void pmu_vio_3v3(bool en)
{
    uint32_t lock;

    lock = int_lock();
    if (en) {
        pmu_module_config(PMU_SENSOR,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
        pmu_module_ramp_volt(PMU_SENSOR, PMU_LDO_SENSOR_3_3V, PMU_LDO_SENSOR_3_3V);
    } else {
        pmu_module_config(PMU_SENSOR,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    }
    int_unlock(lock);
}

void pmu_sar_adc_vref_enable(void)
{
    uint16_t val;

#ifdef PMU_FORCE_LP_MODE
    pmu_big_bandgap_enable(PMU_BIG_BANDGAP_USER_SAR_VREF_OUTPUT, true);
#endif

    // force enable
    pmu_read(PMU_REG_AVDD_EN, &val);
    val |= REG_PU_AVDD25_ANA;
    pmu_write(PMU_REG_AVDD_EN, val);

    pmu_read(PMU_REG_SAR_PU_CFG, &val);
    val |= REG_SAR_PU_VREF_REG | REG_SAR_PU_VREF_DR | REG_SAR_PU_REG | REG_SAR_PU_DR;
    pmu_write(PMU_REG_SAR_PU_CFG, val);

    // sar vref output
    pmu_sar_adc_vref_sw_pu(true);
}

void pmu_sar_adc_vref_disable(void)
{
    uint16_t val;

#ifdef PMU_FORCE_LP_MODE
    pmu_big_bandgap_enable(PMU_BIG_BANDGAP_USER_SAR_VREF_OUTPUT, false);
#endif

    pmu_sar_adc_vref_sw_pu(false);

    pmu_read(PMU_REG_SAR_PU_CFG, &val);
    val &= ~(REG_SAR_PU_VREF_REG | REG_SAR_PU_VREF_DR | REG_SAR_PU_REG | REG_SAR_PU_DR);
    pmu_write(PMU_REG_SAR_PU_CFG, val);

    pmu_read(PMU_REG_AVDD_EN, &val);
    val &= ~REG_PU_AVDD25_ANA;
    pmu_write(PMU_REG_AVDD_EN, val);
}

static int pmu_vsys_div_ctrl(int enable)
{
    uint32_t lock;
    uint16_t val;

    lock = int_lock();
    pmu_read(PMU_REG_PU_VSYS_DIV, &val);
    if (enable) {
        val |= REG_PU_VSYS_DIV;
    } else {
        val &= ~REG_PU_VSYS_DIV;
    }
    pmu_write(PMU_REG_PU_VSYS_DIV, val);
    int_unlock(lock);

    return 0;
}

static int pmu_vbat_div_ctrl(int enable)
{
    uint32_t lock;
    uint16_t val;

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &val);
    if (enable) {
        val |= REG_PU_VBAT_DIV;
    } else {
        val &= ~REG_PU_VBAT_DIV;
    }
    pmu_write(PMU_REG_POWER_KEY_CFG, val);
    int_unlock(lock);

    return 0;
}

int pmu_gpadc_div_ctrl(enum HAL_GPADC_CHAN_T channel, int enable)
{
    int ret = 0;

    if (channel == HAL_GPADC_CHAN_BATTERY) {
        ret = pmu_vsys_div_ctrl(enable);
    } else if (channel == HAL_GPADC_CHAN_6) {
        ret = pmu_vbat_div_ctrl(enable);
    }

    return ret;
}

#ifdef PMU_CLK_USE_EXT_CRYSTAL
static void pmu_external_crystal_capbit_set(uint32_t capbit)
{
    uint16_t val;

    pmu_read(PMU_REG_PU_VSYS_DIV, &val);
    val = SET_BITFIELD(val, REG_XO_32K_CAPBIT, capbit);
    pmu_write(PMU_REG_PU_VSYS_DIV, val);
}

static void pmu_external_crystal_capbit_get(uint32_t *p_capbit)
{
    uint16_t val;

    pmu_read(PMU_REG_PU_VSYS_DIV, &val);
    *p_capbit = GET_BITFIELD(val, REG_XO_32K_CAPBIT);
}

static void BOOT_TEXT_FLASH_LOC pmu_external_crystal_init(void)
{
    uint16_t val;

    pmu_external_crystal_capbit_set(PMU_EXT_32K_CAPBIT_DEFAULT);

    // power on external crystal.
    pmu_read(PMU_REG_XO_32K_CFG, &val);
    val = SET_BITFIELD(val, REG_XO_32K_LDO_PRECHARGE_VALUE, 0x1);
    val = SET_BITFIELD(val, REG_XO_32K_ISEL_VALUE, 0x10);
    pmu_write(PMU_REG_XO_32K_CFG, val);

    pmu_read(PMU_REG_XO_32K_ISEL_RAMP, &val);
    val = SET_BITFIELD(val, REG_XO_32K_ISEL_RAMP_START_DLY, 0x100);
    pmu_write(PMU_REG_XO_32K_ISEL_RAMP, val);

    pmu_read(PMU_REG_XO_32K_CFG, &val);
    val |= REG_XO_32K_PU_LDO | REG_XO_32K_PU_LDO_DR | REG_XO_32K_ISEL_DR;
    pmu_write(PMU_REG_XO_32K_CFG, val);

    pmu_ext_crystal_init_stime = hal_sys_timer_get();
}

// Don't support
static void pmu_external_crystal_clkout_after_pwroff(void)
{
}

void pmu_external_crystal_enable(void)
{
    uint16_t val;

    if (pmu_ext_crystal_init_stime == 0) {
        // Shutdown
        pmu_external_crystal_init();
        hal_sys_timer_delay(MS_TO_TICKS(PMU_EXT_32K_STABLE_TIME));
    } else {
        // Power on or reboot
        while (hal_sys_timer_get() - pmu_ext_crystal_init_stime < MS_TO_TICKS(PMU_EXT_32K_STABLE_TIME));
        pmu_ext_crystal_init_stime = 0;
    }

    // Pmu clock slecet to external crystal.
    pmu_read(PMU_REG_MIC_BIAS_C, &val);
    val |= CLK_32K_SEL_1;
    pmu_write(PMU_REG_MIC_BIAS_C, val);
    hal_sys_timer_delay(MS_TO_TICKS(PMU_CLK_SWITCH_STABLE_TIME));

#ifdef RTC_ENABLE
    if (pmu_rtc_enabled()) {
#ifdef SIMU
        val = 32;
#else
        val = CONFIG_SYSTICK_HZ << 1;
#endif
        pmu_rtc_clk_div_set(val);
    }
#endif
}

void pmu_external_crystal_clkout_enable(enum PMU_EXT_CRYSTAL_CLKOUT_PIN_T pin)
{
    uint16_t val;

    if (pin == PMU_EXT_CRYSTAL_CLKOUT_PIN_0) {
        pmu_read(PMU_REG_PU_VSYS_DIV, &val);
        val |= REG_CLK_32K_EXT1_EN;
        pmu_write(PMU_REG_PU_VSYS_DIV, val);
    } else if (pin == PMU_EXT_CRYSTAL_CLKOUT_PIN_1) {
        pmu_read(PMU_REG_PU_VSYS_DIV, &val);
        val |= REG_CLK_32K_EXT2_EN;
        pmu_write(PMU_REG_PU_VSYS_DIV, val);
    } else {
        pmu_read(PMU_REG_PU_VSYS_DIV, &val);
        val |= REG_CLK_32K_EXT3_EN;
        pmu_write(PMU_REG_PU_VSYS_DIV, val);
    }
    pmu_ext_crystal_clkout_usr_map |= pin;
    hal_sys_timer_delay(MS_TO_TICKS(PMU_EXT_32K_CLK_OUT_STABLE_TIME));
}

void pmu_external_crystal_clkout_disable(enum PMU_EXT_CRYSTAL_CLKOUT_PIN_T pin)
{
    uint16_t val;

    if (pin == PMU_EXT_CRYSTAL_CLKOUT_PIN_0) {
        pmu_read(PMU_REG_PU_VSYS_DIV, &val);
        val &= ~REG_CLK_32K_EXT1_EN;
        pmu_write(PMU_REG_PU_VSYS_DIV, val);
    } else if (pin == PMU_EXT_CRYSTAL_CLKOUT_PIN_1) {
        pmu_read(PMU_REG_PU_VSYS_DIV, &val);
        val &= ~REG_CLK_32K_EXT2_EN;
        pmu_write(PMU_REG_PU_VSYS_DIV, val);
    } else {
        pmu_read(PMU_REG_PU_VSYS_DIV, &val);
        val &= ~REG_CLK_32K_EXT3_EN;
        pmu_write(PMU_REG_PU_VSYS_DIV, val);
    }
    pmu_ext_crystal_clkout_usr_map &= ~pin;
}

int pmu_external_crystal_calib(uint32_t *p_wait_ms)
{
    uint32_t time;
    int ret;
    float hz;
    uint32_t capbit;

    TRACE(0, "%s: cur_state=%d", __func__, xtal_32k_calib_state);

    *p_wait_ms = 0;
    ret = -100;

    if (xtal_32k_calib_state == XTAL_32K_CALIB_STATE_NULL) {
_null_start:
        xtal_32k_calib_prev_valid = false;
_calib_start:
        hal_sys_timer_calib_start();
        hal_bus_wake_lock(HAL_BUS_WAKE_LOCK_USER_XTAL_CALIB);
        xtal_32k_calib_state = XTAL_32K_CALIB_STATE_CALIB;
        xtal_32k_calib_time = hal_sys_timer_get();
        *p_wait_ms = xtal_32k_calib_intvl_ms;
        ret = 1;
        goto _exit;
    } else if (xtal_32k_calib_state == XTAL_32K_CALIB_STATE_CALIB) {
        time = hal_sys_timer_get();
        time = TICKS_TO_MS(time - xtal_32k_calib_time);
        if (time < xtal_32k_calib_intvl_ms) {
            *p_wait_ms = xtal_32k_calib_intvl_ms - time;
            ret = 2;
            goto _exit;
        }
        ret = hal_sys_timer_calib_end();
        hal_bus_wake_unlock(HAL_BUS_WAKE_LOCK_USER_XTAL_CALIB);
        if (ret) {
            TRACE(0, "%s: calib_end failed with %d", __func__, ret);
            xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
            ret = -1;
            goto _exit;
        }
        hz = hal_sys_timer_systick_hz_float();
        pmu_external_crystal_capbit_get(&capbit);
        if (xtal_32k_calib_prev_valid) {
            TRACE(0, "%s: hz=%u.%02u capbit=0x%X prev=%u.%02u prev_capbit=0x%X",
                __func__,
                (unsigned)hz, ((unsigned)(hz * 100) - ((unsigned)hz * 100)),
                capbit,
                (unsigned)xtal_32k_calib_prev_hz, ((unsigned)(xtal_32k_calib_prev_hz * 100) - ((unsigned)xtal_32k_calib_prev_hz * 100)),
                xtal_32k_calib_prev_capbit);
        } else {
            TRACE(0, "%s: hz=%u.%02u", __func__, (unsigned)hz, ((unsigned)(hz * 100) - ((unsigned)hz * 100)));
        }
        if (ABS(hz - CONFIG_SYSTICK_HZ_NOMINAL) < 0.1) {
            // Calib result OK
            TRACE(0, "%s: Calib result OK", __func__);
            xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
            ret = 0;
            goto _exit;
        } else if (xtal_32k_calib_prev_valid) {
            // If no better result
            if (hz < CONFIG_SYSTICK_HZ_NOMINAL && CONFIG_SYSTICK_HZ_NOMINAL < xtal_32k_calib_prev_hz) {
                if (CONFIG_SYSTICK_HZ_NOMINAL - hz <= xtal_32k_calib_prev_hz - CONFIG_SYSTICK_HZ_NOMINAL) {
                    TRACE(0, "%s: Calib no better result: Select cur", __func__);
                    xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
                    ret = 0;
                    goto _exit;
                } else {
                    TRACE(0, "%s: Calib no better result: Select prev", __func__);
                    pmu_external_crystal_capbit_set(xtal_32k_calib_prev_capbit);
                    xtal_32k_calib_state = XTAL_32K_CALIB_STATE_ROLLBACK;
                    xtal_32k_calib_time = hal_sys_timer_get();
                    ret = 0;
                    goto _exit;
                }
            } else if (xtal_32k_calib_prev_hz < CONFIG_SYSTICK_HZ_NOMINAL && CONFIG_SYSTICK_HZ_NOMINAL < hz) {
                if (hz - CONFIG_SYSTICK_HZ_NOMINAL <= CONFIG_SYSTICK_HZ_NOMINAL - xtal_32k_calib_prev_hz) {
                    TRACE(0, "%s: Calib no better result: Select cur", __func__);
                    xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
                    ret = 0;
                    goto _exit;
                } else {
                    TRACE(0, "%s: Calib no better result: Select prev", __func__);
                    pmu_external_crystal_capbit_set(xtal_32k_calib_prev_capbit);
                    xtal_32k_calib_state = XTAL_32K_CALIB_STATE_ROLLBACK;
                    xtal_32k_calib_time = hal_sys_timer_get();
                    ret = 0;
                    goto _exit;
                }

            }
        }
        // Save as previous values
        xtal_32k_calib_prev_hz = hz;
        xtal_32k_calib_prev_capbit = capbit;
        xtal_32k_calib_prev_valid = true;
        if (hz > CONFIG_SYSTICK_HZ_NOMINAL) {
            if (capbit >= (REG_XO_32K_CAPBIT_MASK >> REG_XO_32K_CAPBIT_SHIFT)) {
                TRACE(0, "%s: capbit too large: 0x%X", __func__, capbit);
                xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
                ret = -2;
                goto _exit;
            }
            capbit += 0x10;
            if (capbit > (REG_XO_32K_CAPBIT_MASK >> REG_XO_32K_CAPBIT_SHIFT)) {
                capbit = (REG_XO_32K_CAPBIT_MASK >> REG_XO_32K_CAPBIT_SHIFT);
            }
        } else {
            if (capbit <= 1) {
                TRACE(0, "%s: capbit too small: 0x%X", __func__, capbit);
                xtal_32k_calib_state = XTAL_32K_CALIB_STATE_NULL;
                ret = -3;
                goto _exit;
            }
            if (capbit > 0x10) {
                capbit -= 0x10;
            } else {
                capbit = 1;
            }
        }
        TRACE(0, "%s: Set capbit=0x%X", __func__, capbit);
        pmu_external_crystal_capbit_set(capbit);
        xtal_32k_calib_state = XTAL_32K_CALIB_STATE_SET;
        xtal_32k_calib_time = hal_sys_timer_get();
        *p_wait_ms = xtal_32k_calib_stable_ms;
        ret = 3;
        goto _exit;
    } else if (xtal_32k_calib_state == XTAL_32K_CALIB_STATE_SET) {
        time = hal_sys_timer_get();
        time = TICKS_TO_MS(time - xtal_32k_calib_time);
        if (time < xtal_32k_calib_stable_ms) {
            *p_wait_ms = xtal_32k_calib_stable_ms - time;
            ret = 4;
            goto _exit;
        }
        goto _calib_start;
    } else if (xtal_32k_calib_state == XTAL_32K_CALIB_STATE_ROLLBACK) {
        time = hal_sys_timer_get();
        time = TICKS_TO_MS(time - xtal_32k_calib_time);
        if (time < xtal_32k_calib_stable_ms) {
            *p_wait_ms = xtal_32k_calib_stable_ms - time;
            ret = 5;
            goto _exit;
        }
        goto _null_start;
    } else {
        ASSERT(false, "%s: Bad xtal_32k_calib_state=%d", __func__, xtal_32k_calib_state);
    }

_exit:
    TRACE(0, "%s: ret=%d next_state=%d", __func__, ret, xtal_32k_calib_state);
    return ret;
}
#endif

static void pmu_ntc_irq_handler(uint16_t raw, uint16_t volt)
{
    pmu_ntc_capture_disable();

    ntc_temperature = pmu_volt2temperature(volt);
    ntc_raw = raw;

    if (pmu_ntc_cb) {
        pmu_ntc_cb(ntc_raw, ntc_temperature);
    }

    ntc_irq_busy = false;
}

int pmu_ntc_capture_start(PMU_NTC_IRQ_HANDLER_T cb)
{
    int nRet = 0;
    uint32_t lock = 0;

    lock = int_lock();
    if (ntc_irq_busy) {
        nRet = -1;
        goto exit;
    }
    pmu_ntc_cb = cb;
    ntc_irq_busy = true;
    pmu_ntc_capture_enable();
    hal_gpadc_open(HAL_GPADC_CHAN_0, HAL_GPADC_ATP_ONESHOT, pmu_ntc_irq_handler);
exit:
    int_unlock(lock);

    return nRet;
}

int pmu_volt2temperature(const uint16_t volt)
{
    const int Kvtherm_T = 373;
    static uint16_t ntc_tmep_ref = 0;
    static uint16_t ntc_tmep_ref2volt = 0;
    int temp_centigrade = 0;

    if (ntc_tmep_ref == 0) {
        ntc_tmep_ref = pmu_ntc_temperature_reference_get();
        ntc_tmep_ref2volt = hal_gpadc_adc2volt(ntc_tmep_ref);
    }

    temp_centigrade = (int)(((volt - ntc_tmep_ref2volt) * 100 + Kvtherm_T / 2) / Kvtherm_T + 25);

    return temp_centigrade;
}

uint16_t pmu_ntc_temperature_reference_get(void)
{
    static uint16_t efuse_temperature_raw = 0;

    if (!efuse_temperature_raw) {
        uint16_t efuse_val;

#ifdef PMU_FORCE_LP_MODE
        pmu_get_efuse(PMU_EFUSE_PAGE_TEMP_SENSOR_LP, &efuse_val);
#else
        pmu_get_efuse(PMU_EFUSE_PAGE_TEMP_SENSOR, &efuse_val);
#endif
        if (efuse_val) {
            efuse_temperature_raw = efuse_val;
        } else {
            efuse_temperature_raw = PMU_EFUSE_TEMPSENSOR_DEFAULT;
        }
        TRACE(0, "%s efuse_val=0x%x efuse_temperature_raw=%d", __func__, efuse_val, efuse_temperature_raw);
    }

    return efuse_temperature_raw;
}

#ifdef PMU_REG_DUMP
void pmu_reg_dump(void)
{
    uint16_t val;
    uint16_t pmu_reg_dmup[] = {
        0xC1, 0x64, 0xCA, 0x14, 0x0E, 0x07, 0x08, 0x68, 0x4A, 0x13,
        0xC3, 0x15, 0x64, 0xBF
    };

    TRACE(0, "pmu_reg_dump");
    for (uint16_t i = 0; i < sizeof(pmu_reg_dmup) / sizeof(pmu_reg_dmup[0]); i++) {
        pmu_read(pmu_reg_dmup[i], &val);
        TRACE(0, "REG/VAL:0x%x/0x%x", pmu_reg_dmup[i], val);
        hal_sys_timer_delay_us(1);
    }
}
#endif

#if defined(PMU_CLK_USE_EXT_CRYSTAL) && defined(RTC_ENABLE)
// Workaround for old chips can not use ext 32k crystal after power off.
static void BOOT_TEXT_SRAM_LOC pmu_sys_timer_calib_start(void)
{
    uint32_t lock;
    uint32_t slow;
    uint32_t fast;

    lock = int_lock();
    slow = hal_sys_timer_get();
    while (hal_sys_timer_get() == slow);
    __ISB();
    fast = hal_fast_sys_timer_get();
    int_unlock(lock);

    pmu_slow_val = slow + 1;
    pmu_fast_val = fast;
}

static int BOOT_TEXT_SRAM_LOC pmu_sys_timer_calib_end(void)
{
    uint32_t lock;
    uint32_t slow;
    uint32_t fast;
    uint32_t slow_diff;

    lock = int_lock();
    slow = hal_sys_timer_get();
    while (hal_sys_timer_get() == slow);
    __ISB();
    fast = hal_fast_sys_timer_get();
    int_unlock(lock);

    slow += 1;
    slow_diff = slow - pmu_slow_val;

    // Avoid computation error
    if (slow_diff < PMU_MIN_CALIB_TICKS) {
        return 1;
    }
    // Avoid fast tick overflow
    if (slow_diff > PMU_MAX_CALIB_TICKS) {
        return 2;
    }

    pmu_sys_tick_hz = (float)CONFIG_FAST_SYSTICK_HZ / (fast - pmu_fast_val) * slow_diff;

    if (pmu_sys_tick_hz > PMU_MAX_CALIB_SYSTICK_HZ) {
        pmu_sys_tick_hz = PMU_MAX_CALIB_SYSTICK_HZ;
    }

    return 0;
}

static void BOOT_TEXT_SRAM_LOC pmu_sys_timer_calib(void)
{
    pmu_sys_timer_calib_start();
    hal_sys_timer_delay(PMU_MIN_CALIB_TICKS);
    pmu_sys_timer_calib_end();
}

static uint32_t pmu_sys_timer_systick_hz(void)
{
    return (uint32_t)(pmu_sys_tick_hz + 0.5);
}

static void pmu_rtc_clk_div_reload(void)
{
    uint32_t lpo_freq;

    pmu_sys_timer_calib();
    lpo_freq = pmu_sys_timer_systick_hz() << 1;

    pmu_write(PMU_REG_RTC_DIV_1HZ, lpo_freq - 2);
}
#endif

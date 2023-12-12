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
#ifndef __PMU_BEST1502X_H__
#define __PMU_BEST1502X_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_cmu.h"

#define ANA_REG(r)                          (((r) & 0xFFF) | 0x1000)
#define RF_REG(r)                           (((r) & 0xFFF) | 0x2000)
#define PMU_REG(r)                          (((r) & 0xFFF) | 0x3000)
#define USBPHY_REG(r)                       (((r) & 0xFFF) | 0x5000)

#define ISPI_PMU_REG(reg)                   (((reg) & 0xFFF) | 0x3000)

#define MAX_VMIC_CH_NUM                     2

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY         = 0,
    PMU_EFUSE_PAGE_BOOT             = 1,
    PMU_EFUSE_PAGE_FEATURE          = 2,
    PMU_EFUSE_PAGE_BATTER_LV_ACT    = 3,

    PMU_EFUSE_PAGE_BATTER_HV_ACT    = 4,
    PMU_EFUSE_PAGE_SW_CFG           = 5,
    PMU_EFUSE_PAGE_PROD_TEST        = 6,
    PMU_EFUSE_PAGE_RESERVED_7       = 7,

    PMU_EFUSE_PAGE_BT_POWER         = 8,
    PMU_EFUSE_PAGE_DCCALIB2_L       = 9,
    PMU_EFUSE_PAGE_DCCALIB2_L_LP    = 10,
    PMU_EFUSE_PAGE_DCCALIB_L        = 11,

    PMU_EFUSE_PAGE_DCCALIB_L_LP     = 12,
    PMU_EFUSE_PAGE_RESERVED_13      = 13,
    PMU_EFUSE_PAGE_MODEL            = 14,
    PMU_EFUSE_PAGE_RESERVED_15      = 15,
    PMU_EFUSE_PAGE_GPADC_LV_ACT     = 0x10,
    PMU_EFUSE_PAGE_GPADC_HV_ACT     = 0x12,
    PMU_EFUSE_PAGE_BATTER_LV_LP     = 0x15,
    PMU_EFUSE_PAGE_BATTER_HV_LP     = 0x16,
    PMU_EFUSE_PAGE_GPADC_LV_LP      = 0x17,
    PMU_EFUSE_PAGE_GPADC_HV_LP      = 0x19,
    PMU_EFUSE_PAGE_RESERVED_26      = 0x1A,
    PMU_EFUSE_PAGE_TEMP_SENSOR      = 0x1B,
    PMU_EFUSE_PAGE_TEMP_SENSOR_LP   = 0x1C,
    PMU_EFUSE_PAGE_BUCK_CFG_CAL     = 0x1D,
};

enum PMU_IRQ_TYPE_T {
    PMU_IRQ_TYPE_GPADC,
    PMU_IRQ_TYPE_RTC,
    PMU_IRQ_TYPE_CHARGER,
    PMU_IRQ_TYPE_GPIO,
    PMU_IRQ_TYPE_WDT,
    PMU_IRQ_TYPE_PWRKEY,

    PMU_IRQ_TYPE_QTY
};

enum PMU_PLL_DIV_TYPE_T {
    PMU_PLL_DIV_DIG,
    PMU_PLL_DIV_CODEC,
};

enum PMU_BIG_BANDGAP_USER_T {
    PMU_BIG_BANDGAP_USER_GPADC              = (1 << 0),
    PMU_BIG_BANDGAP_USER_SAR_VREF_OUTPUT    = (1 << 1),
};

enum PMU_EXT_CRYSTAL_CLKOUT_PIN_T {
    PMU_EXT_CRYSTAL_CLKOUT_PIN_0            = (1 << 0),
    PMU_EXT_CRYSTAL_CLKOUT_PIN_1            = (1 << 1),
    PMU_EXT_CRYSTAL_CLKOUT_PIN_2            = (1 << 2),
};

union SECURITY_VALUE_T {
    struct {
        unsigned short security_en      :1;
        unsigned short mode             :1;
        unsigned short sig_type         :1;
        unsigned short reserved         :1;
        unsigned short key_id           :2;
        unsigned short vendor_id        :5;
        unsigned short flash_id         :1;
        unsigned short chksum           :4;
    } root;
    struct {
        unsigned short security_en      :1;
        unsigned short mode             :1;
        unsigned short sig_type         :1;
        unsigned short skip_romkey      :1;
        unsigned short key_id           :2;
        unsigned short reg_base         :2;
        unsigned short reg_size         :2;
        unsigned short reg_offset       :1;
        unsigned short flash_id         :1;
        unsigned short chksum           :4;
    } otp;
    unsigned short reg;
};
#define SECURITY_VALUE_T                    SECURITY_VALUE_T

extern const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_BATTER_LV;
extern const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_BATTER_HV;
extern const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_GPADC_LV;
extern const enum PMU_EFUSE_PAGE_T PMU_EFUSE_PAGE_GPADC_HV;

enum HAL_FLASH_ID_T pmu_get_boot_flash_ctrl_id(void);

uint8_t pmu_gpio_setup_irq(enum HAL_GPIO_PIN_T pin, const struct HAL_GPIO_IRQ_CFG_T *cfg);

void pmu_codec_mic_bias_set_volt(uint32_t map, uint32_t mv);

void pmu_codec_mic_bias_enable(uint32_t map, int enable);

void pmu_codec_mic_bias_lowpower_mode(uint32_t map, int enable);

int pmu_codec_volt_ramp_up(void);

int pmu_codec_volt_ramp_down(void);

void pmu_pll_div_reset_set(enum HAL_CMU_PLL_T pll);

void pmu_pll_div_reset_clear(enum HAL_CMU_PLL_T pll);

void pmu_pll_div_set(enum HAL_CMU_PLL_T pll, enum PMU_PLL_DIV_TYPE_T type, uint32_t div);

void pmu_pll_freq_reg_set(uint16_t low, uint16_t high, uint16_t high2);

void pmu_led_set_hiz(enum HAL_GPIO_PIN_T pin);

void pmu_led_uart_enable(enum HAL_IOMUX_PIN_T pin);

void pmu_led_uart_disable(enum HAL_IOMUX_PIN_T pin);

void pmu_rf_ana_init(void);

void pmu_big_bandgap_enable(enum PMU_BIG_BANDGAP_USER_T user, int enable);

void pmu_bt_reconn(bool en);

void pmu_ldo_vpa_enable(bool en);

bool pmu_dig_set_volt_value(uint16_t dcdc_volt_mv, uint16_t logic_volt_mv, uint16_t ldo_volt_mv);

void bbpll_pll_update(uint64_t pll_cfg_val, bool calib);

void bbpll_set_codec_div(uint32_t div);

void bbpll_codec_clock_enable(bool en);

void pmu_external_crystal_enable(void);

void pmu_external_crystal_clkout_enable(enum PMU_EXT_CRYSTAL_CLKOUT_PIN_T pin);

void pmu_external_crystal_clkout_disable(enum PMU_EXT_CRYSTAL_CLKOUT_PIN_T pin);

int pmu_external_crystal_calib(uint32_t *p_wait_ms);

void bbpll_auto_calib_spd(uint32_t codec_freq, uint32_t div);

void pmu_vio_3v3(bool en);

typedef void (*PMU_NTC_IRQ_HANDLER_T)(uint16_t raw_val, int temperature);

int pmu_volt2temperature(const uint16_t volt);

int pmu_ntc_capture_start(PMU_NTC_IRQ_HANDLER_T cb);

uint16_t pmu_ntc_temperature_reference_get(void);

void pmu_sar_adc_vref_enable(void);

void pmu_sar_adc_vref_disable(void);

int pmu_rtc_clk_div_set(uint16_t freq);

bool pmu_boot_first_power_up(void);

int pmu_wdt_reboot(void);

void dsipll_freq_pll_config(uint32_t freq);

void dsipll_div2_enable(bool is_div);

#ifdef __cplusplus
}
#endif

#endif


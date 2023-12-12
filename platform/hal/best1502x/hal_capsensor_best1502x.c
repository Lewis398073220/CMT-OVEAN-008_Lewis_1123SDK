/***************************************************************************
 *
 * Copyright 2021-2022 BES.
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

#if defined(CAP_SENSOR_BASE) && defined(CAPSENSOR_ENABLE)

#include "cmsis.h"
#include "hal_capsensor_best1502x.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "reg_capsensor_best1502x.h"

#define CAL_TIME_SEL            64         /*000 => 64*/
#define CRYSTAL_FREQ            (hal_cmu_get_crystal_freq()/1000000)       /*24M*/
#define CAPSENSOR_CH_SELECT     8         /*0-7--->1-8ch*/

struct CAPSENSOR_T* capsensor_base = (struct CAPSENSOR_T*)CAP_SENSOR_BASE;
void capsensor_setup_cfg(struct CAPSENSOR_CFG_T * cap_cfg) __attribute__((alias("hal_capsensor_setup")));
void capsensor_get_raw_data(struct capsensor_sample_data *sample, int num) __attribute__((alias("hal_capsensor_get_raw_data")));

void hal_capsensor_setup(struct CAPSENSOR_CFG_T * cap_cfg)
{
    uint16_t sam_fs;
    uint8_t ch_map[CAPSENSOR_CH_SELECT]={0};

    hal_cmu_cap_sens_clock_enable();

    sam_fs = 32000/cap_cfg->samp_fs;
    capsensor_base->REG_000 = SET_BITFIELD(capsensor_base->REG_000, CAP_SENS_REG_STG_NUM, cap_cfg->ch_num);
    capsensor_base->REG_000 = SET_BITFIELD(capsensor_base->REG_000, CAP_SENS_REG_INTVL1_CONV_NUM, 1);
    capsensor_base->REG_000 = SET_BITFIELD(capsensor_base->REG_000, CAP_SENS_REG_INTVL1, 0x20);
    capsensor_base->REG_000 = SET_BITFIELD(capsensor_base->REG_000, CAP_SENS_REG_INTVL2, sam_fs);

    for(uint8_t i=0,j=0; i<8; i++) {
        if(cap_cfg->ch_map & (1<<i)) {
            ch_map[j++] = i;
            if(j > cap_cfg->ch_num)
                break;
        }
    }

    capsensor_base->REG_004 = SET_BITFIELD(capsensor_base->REG_004, CAP_SENS_REG_CFG_STG0, ch_map[0] | cap_cfg->conversion_num);
    capsensor_base->REG_004 = SET_BITFIELD(capsensor_base->REG_004, CAP_SENS_REG_CFG_STG1, ch_map[1] | cap_cfg->conversion_num);
    capsensor_base->REG_004 = SET_BITFIELD(capsensor_base->REG_004, CAP_SENS_REG_CFG_STG2, ch_map[2] | cap_cfg->conversion_num);
    capsensor_base->REG_004 = SET_BITFIELD(capsensor_base->REG_004, CAP_SENS_REG_CFG_STG3, ch_map[3] | cap_cfg->conversion_num);

    capsensor_base->REG_008 = SET_BITFIELD(capsensor_base->REG_008, CAP_SENS_REG_CFG_STG4, ch_map[4] | cap_cfg->conversion_num);
    capsensor_base->REG_008 = SET_BITFIELD(capsensor_base->REG_008, CAP_SENS_REG_CFG_STG5, ch_map[5] | cap_cfg->conversion_num);
    capsensor_base->REG_008 = SET_BITFIELD(capsensor_base->REG_008, CAP_SENS_REG_CFG_STG6, ch_map[6] | cap_cfg->conversion_num);
    capsensor_base->REG_008 = SET_BITFIELD(capsensor_base->REG_008, CAP_SENS_REG_CFG_STG7, ch_map[7] | cap_cfg->conversion_num);

    capsensor_base->REG_01C = SET_BITFIELD(capsensor_base->REG_01C, CAP_SENS_REG_TCH_ACTIVE_NOISE_NUM, 0x3);
    capsensor_base->REG_01C = SET_BITFIELD(capsensor_base->REG_01C, CAP_SENS_REG_TCH_ACTIVE_CAP_NUM,   0x3);

    capsensor_base->REG_020 = SET_BITFIELD(capsensor_base->REG_020, CAP_SENS_REG_CAP_THR_CHAN1, 0x550);
    capsensor_base->REG_020 = SET_BITFIELD(capsensor_base->REG_020, CAP_SENS_REG_CAP_THR_CHAN0, 0x550);

    capsensor_base->REG_024 = SET_BITFIELD(capsensor_base->REG_024, CAP_SENS_REG_CAP_THR_CHAN3, 0x400);
    capsensor_base->REG_024 = SET_BITFIELD(capsensor_base->REG_024, CAP_SENS_REG_CAP_THR_CHAN2, 0x3B0);

    capsensor_base->REG_030 = SET_BITFIELD(capsensor_base->REG_030, CAP_SENS_REG_CAP_THR_DIFF1, 0x1000);
    capsensor_base->REG_030 = SET_BITFIELD(capsensor_base->REG_030, CAP_SENS_REG_CAP_THR_DIFF0, 0x2000);

    capsensor_base->REG_048 = SET_BITFIELD(capsensor_base->REG_048, CAP_SENS_REG_CHANN_SEL_DIFF3, 0);
    capsensor_base->REG_048 = SET_BITFIELD(capsensor_base->REG_048, CAP_SENS_REG_CHANN_SEL_DIFF2, 0);
    capsensor_base->REG_048 = SET_BITFIELD(capsensor_base->REG_048, CAP_SENS_REG_CHANN_SEL_DIFF1, 0x1A);
    capsensor_base->REG_048 = SET_BITFIELD(capsensor_base->REG_048, CAP_SENS_REG_CHANN_SEL_DIFF0, 0x8);

    capsensor_base->REG_04C = SET_BITFIELD(capsensor_base->REG_04C, CAP_SENS_REG_CHANN_SEL_DBL_CLICK3, 0x3);
    capsensor_base->REG_04C = SET_BITFIELD(capsensor_base->REG_04C, CAP_SENS_REG_CHANN_SEL_DBL_CLICK2, 0x2);
    capsensor_base->REG_04C = SET_BITFIELD(capsensor_base->REG_04C, CAP_SENS_REG_CHANN_SEL_DBL_CLICK1, 0x1);
    capsensor_base->REG_04C = SET_BITFIELD(capsensor_base->REG_04C, CAP_SENS_REG_CHANN_SEL_DBL_CLICK0, 0);

    capsensor_base->REG_050 = SET_BITFIELD(capsensor_base->REG_050, CAP_SENS_REG_CHANN_DBL_CLICK_INTVL_MIN, 0x8);
    capsensor_base->REG_050 = SET_BITFIELD(capsensor_base->REG_050, CAP_SENS_REG_CHANN_DBL_CLICK_INTVL_MAX, 0x2000);

    capsensor_base->REG_058 = SET_BITFIELD(capsensor_base->REG_058, CAP_SENS_REG_CONV_EN_INITIAL_DLY, 0x10);

    capsensor_base->REG_05C = SET_BITFIELD(capsensor_base->REG_05C, CAP_SENS_REG_CONV_WAIT, 0xF);
    capsensor_base->REG_05C = SET_BITFIELD(capsensor_base->REG_05C, CAP_SENS_REG_CDC_CORE_RST_CNT, 0x5);
    capsensor_base->REG_05C = SET_BITFIELD(capsensor_base->REG_05C, CAP_SENS_REG_CDC_CORE_PWUP_WAIT, 0x5);
    capsensor_base->REG_05C = SET_BITFIELD(capsensor_base->REG_05C, CAP_SENS_REG_VREF_PRECHG_CNT, 0x80);

    capsensor_base->REG_060 = SET_BITFIELD(capsensor_base->REG_060, CAP_SENS_REG_BIAS_ISEL_0, 4);
    capsensor_base->REG_060 = SET_BITFIELD(capsensor_base->REG_060, CAP_SENS_REG_BIAS_ISEL_1, 4);
    capsensor_base->REG_060 = SET_BITFIELD(capsensor_base->REG_060, CAP_SENS_REG_LDO_VSEL,    4);
    capsensor_base->REG_060 = SET_BITFIELD(capsensor_base->REG_060, CAP_SENS_REG_CREF_SEL,    4);

    capsensor_base->REG_068 |= CAP_SENS_REG_SOFT_MODE;
    capsensor_base->REG_068 &= ~CAP_SENS_REG_CAP_SENSOR_EN;
    hal_sys_timer_delay(MS_TO_TICKS(10));
    capsensor_base->REG_068 |= CAP_SENS_REG_CAP_SENSOR_EN;
    hal_sys_timer_delay(MS_TO_TICKS(10));
    capsensor_base->REG_068 |= CAP_SENS_REG_DMA_EN;

    capsensor_base->REG_080 &= ~CAP_SENS_REG_SAR_TEST_LOCK;
    capsensor_base->REG_080 |= CAP_SENS_REG_SAR_TEST_LOCK;
}

static int hal_capsensor_calib_wait_done(void)
{
    uint8_t count = 100;
    capsensor_base->REG_08C &= ~(CAP_SENS_REG_RCOSC_CAL_RESETN | CAP_SENS_REG_RCOSC_CAL_START);
    hal_sys_timer_delay(US_TO_TICKS(1));
    capsensor_base->REG_08C |= CAP_SENS_REG_RCOSC_CAL_RESETN;
    hal_sys_timer_delay(US_TO_TICKS(1));
    capsensor_base->REG_08C |= CAP_SENS_REG_RCOSC_CAL_START;

    do {
        hal_sys_timer_delay_us(50);
        if(capsensor_base->REG_090 & (1<<15)) {
            return 0;
        }
    } while(count--);
    return -1;
}


/* formula : freq = (CAL_TIME_SEL*CRYSTAL_FREQ)/rcosc_cal_cnt */
int hal_capsensor_clk_calib(void)
{
    uint16_t cal_cnt1,cal_cnt2;
    float f_val1,f_val2;
    float k,b,ftrim;
    int ret = 0;
    uint16_t int_num;

    capsensor_base->REG_054 |= CAP_SENS_REG_BIAS_PWUP_DR|CAP_SENS_REG_BIAS_PWUP_0|CAP_SENS_REG_BIAS_PWUP_1|CAP_SENS_REG_VREF_PWUP_DR|CAP_SENS_REG_VREF_PWUP_0|CAP_SENS_REG_VREF_PWUP_1|CAP_SENS_REG_LDO_PWUP_DR|
        CAP_SENS_REG_LDO_PWUP_0|CAP_SENS_REG_LDO_PWUP_1|CAP_SENS_REG_RCOSC_PWUP_DR|CAP_SENS_REG_RCOSC_PWUP_0|CAP_SENS_REG_RCOSC_PWUP_1;

    ret = hal_capsensor_calib_wait_done();
    if (ret) {
        return ret;
    }

    cal_cnt1 = capsensor_base->REG_090&0x7FFF;
    f_val1 = (CAL_TIME_SEL*CRYSTAL_FREQ)*1.0f/cal_cnt1;

    capsensor_base->REG_064 = SET_BITFIELD(capsensor_base->REG_064, CAP_SENS_REG_RCOSC_FRE_TRIM, 0x40);
    ret = hal_capsensor_calib_wait_done();
    if (ret) {
        return ret;
    }
    cal_cnt2 = capsensor_base->REG_090&0x7FFF;
    f_val2 = (CAL_TIME_SEL*CRYSTAL_FREQ)*1.0f/cal_cnt2;

    /*f_val1 = k*ftrim + b*/
    k = (f_val1-f_val2)/(0x80-0x40);
    b = f_val1 - k*0x80;

    /*1.5(M) = k*ftrim + b*/
    ftrim = (1.5-b)/k;
    int_num = (int)ftrim;
    capsensor_base->REG_064 = SET_BITFIELD(capsensor_base->REG_064, CAP_SENS_REG_RCOSC_FRE_TRIM, int_num);
    ret = hal_capsensor_calib_wait_done();
    if (ret) {
        return ret;
    }

    return ret;
}

void hal_capsensor_irq_enable(void)
{
    capsensor_base->REG_074 = (1 << 24);
}

void hal_capsensor_irq_disable(void)
{
    capsensor_base->REG_078 = (1 << 24);
}

void hal_capsensor_get_raw_data(struct capsensor_sample_data *sample, int num)
{
    uint32_t value;
    for (uint8_t i = 0; i < num; i++) {
        value = capsensor_base->REG_06C;
        sample[i].sdm = value & 0xfff;
        sample[i].ch = (value & 0x7000) >> 12;
    }
}

#endif // CAP_SENSOR_BASE

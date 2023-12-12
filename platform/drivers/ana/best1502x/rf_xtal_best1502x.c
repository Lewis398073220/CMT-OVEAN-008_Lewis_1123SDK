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
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include CHIP_SPECIFIC_HDR(reg_rf)

#define rf_read(reg, val)                 hal_analogif_reg_read(RF_REG(reg), val)
#define rf_write(reg, val)                hal_analogif_reg_write(RF_REG(reg), val)
#define XTAL_OSC_GOAL                     (0X200) //You can choose 0X100(256)  0X200(512)  0X400(1024)   0X800(2048)
#define RCOSC_CAL_MSB                     (0x07)  //reg_xtal_rcosc_fre_trim[7:0] is not same as 1600
#define GOAL_COMPENSAT_VAL                (0x7)   //the value worked out by experiments[0x6 - 0xA]

//rf_xtal_rcosc_cal is done!
void rf_xtal_startup_enable()
{
    uint16_t val = 0;
    rf_read(RF_REG_10F, &val);
    val |= REG_XTAL_STARTUP_RELEASE_EN;
    rf_write(RF_REG_10F, val);

    rf_read(RF_REG_105, &val);
    val = SET_BITFIELD(val, REG_XTAL_STARTUP_SEL_DRV, 3)|SET_BITFIELD(val, REG_XTAL_STARTUP_CNT_1ST, 0x18);
    rf_write(RF_REG_105, val);

    rf_read(RF_REG_106, &val);
    val = SET_BITFIELD(val, REG_XTAL_STARTUP_CNT_HOLD, 0xF)|SET_BITFIELD(val, REG_XTAL_STARTUP_RC, 0xC);
    rf_write(RF_REG_106, val);

    rf_read(RF_REG_108, &val);
    val = SET_BITFIELD(val, REG_XTAL_STARTUP_INJ_PERIOD, 0x10);
    rf_write(RF_REG_108, val);

    rf_read(RF_REG_104, &val);
    val &= ~REG_XTAL_RCOSC_CAL_EN;
    rf_write(RF_REG_104, val);
    rf_read(RF_REG_104, &val);
    val |= REG_XTAL_STARTUP_EN;
    rf_write(RF_REG_104, val);
    TRACE(2, "rf_xtal_rcosc_cal_startup_enable is done !!!");
}

uint16_t rf_xtal_rcosc_cal_cnt(uint16_t fre_trim_val)
{
    uint16_t val = 0;

    rf_read(RF_REG_104, &val);
    val &= ~REG_XTAL_STARTUP_EN;
    rf_write(RF_REG_104, val);
    rf_read(RF_REG_104, &val);
    val |= REG_XTAL_RCOSC_CAL_EN;
    rf_write(RF_REG_104, val);
    rf_read(RF_REG_104, &val);
    val |= REG_XTAL_LDO_MODE_SEL;
    rf_write(RF_REG_104, val);

    rf_read(RF_REG_107, &val);
    val = SET_BITFIELD(val, REG_XTAL_RCOSC_FRE_TRIM, fre_trim_val);
    rf_write(RF_REG_107, val);
    hal_sys_timer_delay(US_TO_TICKS(5));
    rf_read(RF_REG_101, &val);
    val &= ~REG_BT_RCOSC_CAL_RESETN;
    rf_write(RF_REG_101, val);
    hal_sys_timer_delay(US_TO_TICKS(5));
    rf_read(RF_REG_101, &val);
    val |= REG_BT_RCOSC_CAL_RESETN;
    rf_write(RF_REG_101, val);
    hal_sys_timer_delay(US_TO_TICKS(5));
    rf_read(RF_REG_101, &val);
    val &= ~REG_BT_RCOSC_CAL_START;
    rf_write(RF_REG_101, val);
    hal_sys_timer_delay(US_TO_TICKS(5));

    rf_read(RF_REG_101, &val);
    val |= REG_BT_RCOSC_CAL_START;
    rf_write(RF_REG_101, val);

    hal_sys_timer_delay(MS_TO_TICKS(1));
    rf_read(RF_REG_117, &val);

    return val;
}

void rf_xtal_rcosc_cal()
{
    uint16_t left_val = 1 << (RCOSC_CAL_MSB + 1);//[7:0]
    uint16_t right_val = 0;
    uint16_t bt_rcosc_cal_done = 0;
    uint8_t cycle_cnt = 0;
    int delta_margin = 2;
    uint16_t rcosc_fre_trim;
    uint16_t REG_17_cnt;
    uint16_t rcosc_cal_cnt=0;
    uint16_t val = 0;
    uint16_t xtal_osc_goal = XTAL_OSC_GOAL;
    uint8_t fre_final_integer = 0;
    uint8_t fre_final_decimal = 0;
    uint8_t fre_final_temp = 0;

    if (XTAL_OSC_GOAL == 0x100) //GOAL is 256 bit[10:8] 010
    {
        rf_read(RF_REG_101, &val);
        val = SET_BITFIELD(val, REG_BT_RCOSC_CAL_TIME_SEL, 0x2);
        rf_write(RF_REG_101, val);
        xtal_osc_goal -= (GOAL_COMPENSAT_VAL);
    }
    if (XTAL_OSC_GOAL == 0x200) //GOAL is 512 bit[10:8] 011
    {
        rf_read(RF_REG_101, &val);
        val = SET_BITFIELD(val, REG_BT_RCOSC_CAL_TIME_SEL, 0x3);
        rf_write(RF_REG_101, val);
        xtal_osc_goal -= (2 * GOAL_COMPENSAT_VAL);
    }
    if (XTAL_OSC_GOAL == 0x400) //GOAL is 1024 bit[10:8] 100
    {
        rf_read(RF_REG_101, &val);
        val = SET_BITFIELD(val, REG_BT_RCOSC_CAL_TIME_SEL, 0x4);
        rf_write(RF_REG_101, val);
        xtal_osc_goal -= (4 * GOAL_COMPENSAT_VAL);
    }
    if (XTAL_OSC_GOAL == 0x800) //GOAL is 2048 bit[10:8] 101
    {
        rf_read(RF_REG_101, &val);
        val = SET_BITFIELD(val, REG_BT_RCOSC_CAL_TIME_SEL, 0x5);
        rf_write(RF_REG_101, val);
        xtal_osc_goal -= (8 * GOAL_COMPENSAT_VAL);
    }
    rf_read(RF_REG_10A, &val);
    val = SET_BITFIELD(val, REG_XTAL_RCOSC_MODE, 0x2);
    val = SET_BITFIELD(val, REG_XTAL_BUF_RC_SSC_OTHER, 0xF);
    rf_write(RF_REG_10A, val);
    do
    {
        if(left_val < right_val)
        {
            TRACE(2,"rf_xtal_rcosc_cal is failed!!,the calculate border is error!!!");
        }
        rcosc_fre_trim = ((left_val + right_val) >> 1);
        REG_17_cnt = rf_xtal_rcosc_cal_cnt(rcosc_fre_trim);
        rcosc_cal_cnt =  (REG_17_cnt & (0x7fff));
        bt_rcosc_cal_done = REG_17_cnt & (1 << 15);
        if(bt_rcosc_cal_done)
        {
            delta_margin = xtal_osc_goal - rcosc_cal_cnt;
            if(rcosc_cal_cnt > xtal_osc_goal)
            {
                right_val = ((left_val + right_val) >> 1);
            }else{
                left_val = ((left_val + right_val) >> 1);
            }
            if((left_val - right_val) == 1)
            {
                rcosc_fre_trim = left_val;
                break;
            }
        }else{
            TRACE(2,"rf_xtal_rcosc_cal is failed!!,bt_rcosc_cal_done not be 1,rcosc_fre_trim is %d!!!",rcosc_fre_trim);
        }
        cycle_cnt++;
        if(cycle_cnt > 50)
        {
            TRACE(2,"rf_xtal_rcosc_cal is failed!!,can not match the goal !!!");
            break;
        }
    }while(ABS(delta_margin)> 1);

    if((ABS(delta_margin) < 2) || ((left_val - right_val) == 1))
    {
        REG_17_cnt = rf_xtal_rcosc_cal_cnt(rcosc_fre_trim);
        rcosc_cal_cnt =  (REG_17_cnt & (0x7fff));
        fre_final_temp = (240 * XTAL_OSC_GOAL / rcosc_cal_cnt);
        fre_final_integer = fre_final_temp / 10;
        fre_final_decimal = fre_final_temp - fre_final_integer * 10;
        TRACE(2,"rf_xtal_rcosc_cal is done! fre_final=%d.%dMHz,Goal=%d,delta_margin=%d,rf_0x07=%d,rf_0x17=%d.",fre_final_integer,fre_final_decimal,xtal_osc_goal,delta_margin,rcosc_fre_trim,rcosc_cal_cnt);
        rf_xtal_startup_enable();
    }
}



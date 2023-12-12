/***************************************************************************
 *
 * Copyright 2022-2023 BES.
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
#include "cmsis.h"
#include "capsensor_driver.h"
#include "hal_trace.h"
#include "cmsis_os.h"
#include CHIP_SPECIFIC_HDR(hal_senscmu)
#include "tgt_hardware_capsensor.h"
#ifdef CHIP_SUBSYS_SENS
#include CHIP_SPECIFIC_HDR(capsensor_sens_driver)
#else
#include CHIP_SPECIFIC_HDR(capsensor_driver)
#endif

static struct CAPSENSOR_CFG_T capsensor_cfg = {0};

//touch config
TouchConfig cap_touch_config=
{
    .cap_keydown_th                 = CAP_KEYDOWN_TH,
    .cap_keyup_th                   = CAP_KEYUP_TH,
#ifdef CAPSENSOR_SLIDE
    .slide_channel_0                = WORKING_CH0,
    .slide_channel_1                = WORKING_CH1,
    .slide_channel_2                = WORKING_CH2,
#else
    .touch_channel                  = WORKING_CH4,
#endif
    .cap_key_click_max_duration     = CAP_KEY_CLICK_MAX_DURATION,// 350m
    .cap_key_click_min_duration     = CAP_KEY_CLICK_MIN_DURATION,// 50ms
    .cap_key_double_interval        = CAP_KEY_DOUBLE_INTERVAL ,  // double key interval
    .cap_key_triple_interval        = CAP_KEY_TRIPLE_INTERVAL ,  // triple key interval
    .cap_key_ultra_interval         = CAP_KEY_ULTRA_INTERVAL  ,  // ultra  key interval
    .cap_key_rampage_interval       = CAP_KEY_RAMPAGE_INTERVAL,  // rampage key interval
#ifdef CAPSENSOR_SLIDE
    .cap_key_slide_up_interval      = CAP_KEY_SLIDE_UP_INTERVAL, // slide up key interval
#endif
    .cap_key_long_interval          = CAP_KEY_LONG_INTERVAL    , // 1000ms
    .cap_key_longlong_interval      = CAP_KEY_LONGLONG_INTERVAL, // 5000ms
    .cap_dc_reset_interval          = CAP_DC_RESET_INTERVAL    , // 20000ms
};

//Wear confign
WearConfig cap_wear_config=
{
    .wear_detect_channel_0             = WORKING_CH1,
    .wear_detect_channel_1             = WORKING_CH2,
    .wear_reference_channel_0          = WORKING_CH0,
    .wear_reference_channel_1          = WORKING_CH3,
    .cap_wear_threshold_high1          = CAP_WEAR_THRESHOLD_HIGH1,
    .cap_wear_threshold_high2          = CAP_WEAR_THRESHOLD_HIGH2,
    .cap_wear_threshold_low1           = CAP_WEAR_THRESHOLD_LOW1,
    .cap_wear_threshold_low2           = CAP_WEAR_THRESHOLD_LOW2,
    .cap_wear_twopad_threshold1        = CAP_WEAR_TWOPAD_THRESHOLD1,
    .cap_wear_twopad_state_win1        = CAP_WEAR_TWOPAD_STATE_WIN1,
    .cap_wear_ear_on_state_win         = CAP_WEAR_EAR_ON_STATE_WIN,
    .cap_wear_ear_off_state_win        = CAP_WEAR_EAR_OFF_STATE_WIN,
    .cap_wear_offset0_update_threshold = CAP_WEAR_OFFSET0_UPDATE_THRESHOLD,
    .cap_wear_offset1_update_threshold = CAP_WEAR_OFFSET1_UPDATE_THRESHOLD,
    .cap_wear_jump_threshold1          = CAP_WEAR_JUMP_THRESHOLD1,
    .cap_wear_jump_threshold2          = CAP_WEAR_JUMP_THRESHOLD2,
    .cap_wear_on_delay_count           = CAP_WEAR_ON_DELAY_COUNT,
    .cap_wear_on_delay_mse_th          = CAP_WEAR_ON_DELAY_MSE_TH,
};

/***************************************************************************
 * @brief capsensor driver start function
 *
 ***************************************************************************/
void capsensor_drv_start(void)
{
    /*cap sensor config set*/
    capsensor_cfg.ch_num = capsensor_ch_num_get() - 1;     //0-7 numbes: 1-8 ch_num
    capsensor_cfg.conversion_num = capsensor_conversion_num_get(); //rep numbers
    capsensor_cfg.samp_fs = capsensor_samp_fs_get();   //cap samp fs
    capsensor_cfg.ch_map = capsensor_ch_map_get();     //cap_channel_set

    capsensor_gpio_init();
    capsensor_setup_cfg(&capsensor_cfg);
    capsensor_sar_baseline_init();

#if (CHIP_CAPSENSOR_VER < 1)
    capsensor_clk_calib();
#endif
}

/***************************************************************************
 * @brief read capsesnor fifo data function
 *
 * @param sample  pointer of data
 ***************************************************************************/
void read_capsensor_fifo(struct capsensor_sample_data *sample)
{
    capsensor_get_raw_data(sample, capsensor_ch_num_get()*CAP_REPNUM);
}

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
#ifndef __TGT_HARDWARE_CAPSENSOR__
#define __TGT_HARDWARE_CAPSENSOR__

#ifdef __cplusplus
extern "C" {
#endif

#if (CHIP_CAPSENSOR_VER < 1)
#ifdef CAPSENSOR_SLIDE
#define CAP_SAMP_FS          50  /* hz */
#else
#define CAP_SAMP_FS          20  /* hz */
#endif
#else
#ifdef CAPSENSOR_SLIDE
#define CAP_SAMP_FS          30  /* ms */
#else
#define CAP_SAMP_FS          50  /* ms */
#endif
#endif

#define CAP_CHNUM            5   /* channel numbers, CAP_USED_TOUCH_NUM+CAP_USED_WEAR_NUM */
#if (CHIP_CAPSENSOR_VER < 1)
#define CAP_REPNUM           2   /* rep numbers */
#else
#define CAP_REPNUM           1   /* rep numbers */
#define CAP_CLK_DIV          20  /* clk_samp_fs = clk/(CAP_CLK_DIV+2): 591K = 13M/(CAP_CLK_DIV + 2) */
#endif

#ifdef CAPSENSOR_SLIDE
#define CAP_USED_TOUCH_NUM   3      /* slide_num */
#else
#define CAP_USED_TOUCH_NUM   1      /* touch_num */
#endif
#define CAP_USED_WEAR_NUM    4      /* wear_num */
#define CAP_DETECT_WEAR_NUM  2      /* detect_num */

/*customers can be modified parameters*/
// touch key
#if (CHIP_CAPSENSOR_VER < 1)
#define CAP_KEYDOWN_TH              (-36)   // key down
#define CAP_KEYUP_TH                (-28)   // key up
#else
#define CAP_KEYDOWN_TH              (-8000) // key down
#define CAP_KEYUP_TH                (-6000) // key up
#endif
#define CAP_KEY_CLICK_MAX_DURATION  (350)   // 350m
#define CAP_KEY_CLICK_MIN_DURATION  (50)    // 50ms
#define CAP_KEY_DOUBLE_INTERVAL     (400)   // double key interval
#define CAP_KEY_TRIPLE_INTERVAL     (400)   // triple key interval
#define CAP_KEY_ULTRA_INTERVAL      (400)   // ultra  key interval
#define CAP_KEY_RAMPAGE_INTERVAL    (400)   // rampage key interval
#define CAP_KEY_SLIDE_UP_INTERVAL   (5)     // slide up key interval
#define CAP_KEY_LONG_INTERVAL       (1000)  // 1000ms
#define CAP_KEY_LONGLONG_INTERVAL   (5000)  // 5000ms
#define CAP_DC_RESET_INTERVAL       (20000) // 20000ms
#define CAP_SECOND_CLICK_TH_FACTOR  (0.7)
// end touch key

// wear detect
#if (CHIP_CAPSENSOR_VER < 1)
#define CAP_WEAR_THRESHOLD_HIGH1            100
#define CAP_WEAR_THRESHOLD_HIGH2            150
#define CAP_WEAR_THRESHOLD_LOW1             70
#define CAP_WEAR_THRESHOLD_LOW2             120
#define CAP_WEAR_TWOPAD_THRESHOLD1          160
#define CAP_WEAR_OFFSET0_UPDATE_THRESHOLD   5
#define CAP_WEAR_OFFSET1_UPDATE_THRESHOLD   5
#define CAP_WEAR_JUMP_THRESHOLD1            30
#define CAP_WEAR_JUMP_THRESHOLD2            30
#define CAP_WEAR_ON_DELAY_MSE_TH            4
#else
#define CAP_WEAR_THRESHOLD_HIGH1            -8000
#define CAP_WEAR_THRESHOLD_HIGH2            -12000
#define CAP_WEAR_THRESHOLD_LOW1             -6000
#define CAP_WEAR_THRESHOLD_LOW2             -10000
#define CAP_WEAR_TWOPAD_THRESHOLD1          12000
#define CAP_WEAR_OFFSET0_UPDATE_THRESHOLD   600
#define CAP_WEAR_OFFSET1_UPDATE_THRESHOLD   600
#define CAP_WEAR_JUMP_THRESHOLD1            5000
#define CAP_WEAR_JUMP_THRESHOLD2            5000
#define CAP_WEAR_ON_DELAY_MSE_TH            200
#endif
#define CAP_WEAR_ON_DELAY_COUNT     50
#define CAP_WEAR_TWOPAD_STATE_WIN1  12
#define CAP_WEAR_EAR_ON_STATE_WIN   15
#define CAP_WEAR_EAR_OFF_STATE_WIN  12
// end wear detect
/*customers can be modified parameters*/

#define CAP_CH0             (1 << 0)
#define CAP_CH1             (1 << 1)
#define CAP_CH2             (1 << 2)
#define CAP_CH3             (1 << 3)
#define CAP_CH4             (1 << 4)
#define CAP_CH5             (1 << 5)
#define CAP_CH6             (1 << 6)
#define CAP_CH7             (1 << 7)

#ifdef __cplusplus
}
#endif

#endif

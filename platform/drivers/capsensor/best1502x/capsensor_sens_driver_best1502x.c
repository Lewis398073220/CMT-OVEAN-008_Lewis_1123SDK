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
#ifdef CHIP_SUBSYS_SENS
#include "capsensor_sens_driver_best1502x.h"
#include "hal_trace.h"
#include "hal_capsensor_best1502x.h"
#include "tgt_hardware_capsensor.h"

/***************************************************************************
 * @brief get capsensor rep number value for register
 *
 * @param rep_number rep number value
 * @return uint8_t ret: rep number value for register
 ***************************************************************************/
static uint8_t cap_reg_repnum_get(uint8_t rep_number)
{
    uint8_t ret = 0;

    while (rep_number && !(rep_number & 1)) {
        rep_number >>= 1;
        ret++;
    }

    ret <<= 3;
    return ret;
}

uint8_t capsensor_ch_num_get(void)
{
    return CAP_CHNUM;
}

uint8_t capsensor_conversion_num_get(void)
{
    return cap_reg_repnum_get(CAP_REPNUM);
}

uint16_t capsensor_samp_fs_get(void)
{
    return CAP_SAMP_FS;
}

uint8_t capsensor_ch_map_get(void)
{
    return (CAP_CH3 | CAP_CH4 | CAP_CH5 | CAP_CH6 | CAP_CH7);
}

void capsensor_sar_baseline_init(void)
{
}

void capsensor_clk_calib(void)
{
    int ret = 0;
    ret = hal_capsensor_clk_calib();
    TRACE(0, "%s ret:%d exit calib", __func__, ret);
}

#endif

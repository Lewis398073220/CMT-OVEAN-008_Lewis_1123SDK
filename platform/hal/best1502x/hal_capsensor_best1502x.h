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
#ifndef __HAL_CAPSENSOR_BEST1502X_H__
#define __HAL_CAPSENSOR_BEST1502X_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "capsensor_driver.h"

void capsensor_setup_cfg(struct CAPSENSOR_CFG_T * cap_cfg);
void hal_capsensor_irq_enable(void);
void hal_capsensor_irq_disable(void);

void capsensor_get_raw_data(struct capsensor_sample_data *sample, int num);
int hal_capsensor_clk_calib(void);

#ifdef __cplusplus
}
#endif

#endif

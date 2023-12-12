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
#ifndef __RF_XTAL_BEST1502X_H__
#define __RF_XTAL_BEST1502X_H__

#ifdef __cplusplus
extern "C" {
#endif

void rf_xtal_rcosc_cal();

void rf_xtal_startup_enable();

uint16_t rf_xtal_rcosc_cal_cnt(uint16_t fre_trim_val);

#ifdef __cplusplus
}
#endif

#endif
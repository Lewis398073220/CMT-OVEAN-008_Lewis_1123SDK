/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#ifndef __PSAP_SW_COMM_H__
#define __PSAP_SW_COMM_H__

#include "psap_freq_psap.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef PSAP_SW_THIRDPARTY
typedef enum {
    //Adding third-party modes, the following is only an example
    PSAP_SW_THIRD_CMD_GAIN,
    PSAP_SW_CMD_MODE_QTY
} psap_sw_cmd_mode_t;

typedef struct {
    psap_sw_cmd_mode_t mode;
    //Adding third-party variables, the following is only an example
    int32_t gain;
} psap_sw_cmd_t;
#else
#ifndef PSAP_BAND_NUM
#define PSAP_BAND_NUM   (17)
#endif

typedef enum {
    PSAP_SW_CMD_TOTAL_GAIN,
    PSAP_SW_CMD_MODE_WDRC,
    PSAP_SW_CMD_MODE_BF,
    PSAP_SW_CMD_MODE_NS,
    PSAP_SW_CMD_MODE_EQ,
    PSAP_SW_CMD_MODE_DEHOWLING,
    PSAP_SW_CMD_MODE_REVERB,
    PSAP_SW_CMD_MODE_ALL,
    PSAP_SW_CMD_MODE_QTY
} psap_sw_cmd_mode_t;

typedef struct {
    psap_sw_cmd_mode_t mode;
    float total_gain[2];
    float band_gain[PSAP_BAND_NUM];
    uint32_t bf_enable;
    uint32_t ns_level;
    psap_freq_eq_cfg_t *eq_config[2];
    psap_freq_dehowling_cfg dehowling_config;
    psap_freq_reverberator_cfg_t *reverb_cfg;
} psap_sw_cmd_t;
#endif

#ifdef __cplusplus
}
#endif
#endif

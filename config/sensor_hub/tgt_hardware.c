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
#include "tgt_hardware.h"
#include "analog.h"

#ifndef VMIC_MAP_CFG
#define VMIC_MAP_CFG                        AUD_VMIC_MAP_VMIC5
#endif

#ifndef MAINMIC_MAP_CFG
#define MAINMIC_MAP_CFG                     AUD_CHANNEL_MAP_CH4
#endif

#ifdef VAD_USE_SAR_ADC
#ifndef VADMIC_MAP_CFG
#define VADMIC_MAP_CFG                      AUD_CHANNEL_MAP_CH5
#endif
#else /* !VAD_USE_SAR_ADC */
#ifndef VADMIC_MAP_CFG
#define VADMIC_MAP_CFG                      AUD_CHANNEL_MAP_CH4
#endif
#endif

#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV   (MAINMIC_MAP_CFG | VMIC_MAP_CFG)
#define CFG_HW_AUD_INPUT_PATH_VADMIC_DEV    (VADMIC_MAP_CFG | VMIC_MAP_CFG)

const struct AUD_IO_PATH_CFG_T cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM] = {
    { AUD_INPUT_PATH_MAINMIC, CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV, },
    { AUD_INPUT_PATH_VADMIC,  CFG_HW_AUD_INPUT_PATH_VADMIC_DEV,  },
};


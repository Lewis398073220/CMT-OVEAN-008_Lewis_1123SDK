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
#ifndef __TGT_HARDWARE__
#define __TGT_HARDWARE__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_aud.h"

// audio codec
#define CFG_HW_AUD_INPUT_PATH_NUM           3
extern const struct AUD_IO_PATH_CFG_T cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM];

#ifdef VAD_USE_SAR_ADC
#define CODEC_SADC_VOL (15)
#else
#define CODEC_SADC_VOL (7)
#endif

#define CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)

extern const struct CODEC_DAC_VOL_T codec_dac_vol[TGT_VOLUME_LEVEL_QTY];

#ifdef __cplusplus
}
#endif

#endif

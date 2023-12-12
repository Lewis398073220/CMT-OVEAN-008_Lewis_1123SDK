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
#ifndef __AUDIO_PROCESS_EQ_H__
#define __AUDIO_PROCESS_EQ_H__

#include "stdint.h"

typedef enum {
    AUDIO_EQ_TYPE_SW_IIR = 0,
    AUDIO_EQ_TYPE_HW_FIR,
    AUDIO_EQ_TYPE_HW_DAC_IIR,
    AUDIO_EQ_TYPE_HW_IIR,
    AUDIO_EQ_TYPE_QTY
} AUDIO_EQ_TYPE_T;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t audio_process_eq_get_index(AUDIO_EQ_TYPE_T type);
int32_t audio_process_eq_set_index(AUDIO_EQ_TYPE_T type, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif

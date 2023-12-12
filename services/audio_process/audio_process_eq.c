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
#include "hal_trace.h"
#include "tgt_hardware.h"
#include "audio_process_eq.h"

static uint8_t g_eq_index[AUDIO_EQ_TYPE_QTY] = {0, 0, 0, 0};

uint32_t audio_process_eq_get_index(AUDIO_EQ_TYPE_T type)
{
    return g_eq_index[type];
}

int32_t audio_process_eq_set_index(AUDIO_EQ_TYPE_T type, uint32_t index)
{
    ASSERT(type < AUDIO_EQ_TYPE_QTY, "[%s] type is invalid: %d", __func__, type);

#ifdef __SW_IIR_EQ_PROCESS__
    if (index >= EQ_SW_IIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_FIR_EQ_PROCESS__
    if (index >= EQ_HW_FIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_DAC_IIR_EQ_PROCESS__
    if (index >= EQ_HW_DAC_IIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_IIR_EQ_PROCESS__
    if (index >= EQ_HW_IIR_LIST_NUM)
        return -1;
#endif

    g_eq_index[type] = index;

    return index;
}

// TODO: Will move all audio EQ function to here.

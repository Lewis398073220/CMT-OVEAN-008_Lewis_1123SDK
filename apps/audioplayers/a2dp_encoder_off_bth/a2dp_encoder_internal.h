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
#ifndef __A2DP_ENCODER_INTERNAL_H__
#define __A2DP_ENCODER_INTERNAL_H__

#include "list.h"
#include "hal_trace.h"

#define TRACE_A2DP_ENCODER_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
#define TRACE_A2DP_ENCODER_I(str, ...) TR_INFO(TR_MOD(AUDFLG),     "[AUD][ENCODER]" str, ##__VA_ARGS__)
#define TRACE_A2DP_ENCODER_W(str, ...) TR_WARN(TR_MOD(AUDFLG),     "[AUD][ENCODER][WARN]" str, ##__VA_ARGS__)
#define TRACE_A2DP_ENCODER_E(str, ...) TR_ERROR(TR_MOD(AUDFLG),    "[AUD][ENCODER][ERR]" str, ##__VA_ARGS__)

#define ASSERT_A2DP_ENCODER(cond, str, ...) ASSERT(cond, str, ##__VA_ARGS__)


// #define TEXT_A2DP_LOC_A(n, l)               __attribute__((section(#n "." #l)))
// #define TEXT_A2DP_LOC(n, l)                 TEXT_A2DP_LOC_A(n, l)

// #define TEXT_SBC_LOC                        TEXT_A2DP_LOC(.overlay_a2dp_sbc, __LINE__)
// #define TEXT_AAC_LOC                        TEXT_A2DP_LOC(.overlay_a2dp_aac, __LINE__)
// #define TEXT_SSC_LOC                        TEXT_A2DP_LOC(.overlay_a2dp_ssc, __LINE__)
// #define TEXT_LDAC_LOC                       TEXT_A2DP_LOC(.overlay_a2dp_ldac, __LINE__)
// #define TEXT_LHDC_LOC                       TEXT_A2DP_LOC(.overlay_a2dp_lhdcv3, __LINE__)
// #define TEXT_LHDCV5_LOC                     TEXT_A2DP_LOC(.overlay_a2dp_lhdcv5, __LINE__)

#define A2DP_ENCODER_NO_ERROR              (0)
#define A2DP_ENCODER_ENCODE_ERROR          (-1)
#define A2DP_ENCODER_BUSY_ERROR            (-2)
#define A2DP_ENCODER_MEMORY_ERROR          (-3)
#define A2DP_ENCODER_MTU_LIMTER_ERROR      (-4)
#define A2DP_ENCODER_CACHE_UNDERFLOW_ERROR (-5)
//#define A2DP_ENCODER_SYNC_ERROR            (-6)
#define A2DP_ENCODER_CACHE_UNDERFLOW_ERROR_SYS_BUSY     (-7)
#define A2DP_ENCODER_NON_CONSECTIVE_SEQ    (-8)
#define A2DP_ENCODER_NOT_SUPPORT           (-128)

#endif//__A2DPPLAY_H__


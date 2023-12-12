/***************************************************************************
 *
 * Copyright 2020-2025 BES.
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
#ifndef __GSOUND_SBC_H__
#define __GSOUND_SBC_H__
#include "cqueue.h"
#include "voice_sbc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSOUND_SBC_FRAME_HEADER_LEN (4)

#define GSOUND_SBC_CHANNEL_COUNT (1)
#define GSOUND_SBC_CHANNEL_MODE  (SBC_CHANNEL_MODE_MONO)
// bitpool vs block size:
/*
block size = 4 + (4 * nrof_subbands * nrof_channels ) / 8 + round_up(nrof_blocks * nrof_channels * bitpool / 8)

block size = 4 + (4 * 8 * 1) / 8 + round_up(16 * 1 * bitpool / 8)

block size = 56
*/
// 10 - 28: 3.5KB/s (suggested to be used for android ble)
// 12 - 32: 4KB/s 	(suggested to be used for ios ble)
// 24 - 56: 7KB/s	(suggested to be used for android RFComm)

#define GSOUND_SBC_BIT_POOL   (24)
#define GSOUND_SBC_BLOCK_SIZE (56)

#define GSOUND_SBC_SIZE_PER_SAMPLE (2)  // 16 bits, 1 channel
#define GSOUND_SBC_SAMPLE_RATE     (SBC_SAMPLERATE_16K)
#define GSOUND_SBC_NUM_BLOCKS      (16)
#define GSOUND_SBC_NUM_SUB_BANDS   (8)
#define GSOUND_SBC_MSBC_FLAG       (0)
#define GSOUND_SBC_ALLOC_METHOD    (SBC_ALLOC_METHOD_LOUDNESS)

#define GSOUND_SBC_SAMPLE_RATE_VALUE  (16000)
#define GSOUND_SBC_FRAME_PERIOD_IN_MS (20)
#define GSOUND_SBC_HANDLE_INTERVAL_MS (48)

#define GSOUND_SBC_PCM_DATA_SIZE_PER_HANDLE \
    (((GSOUND_SBC_HANDLE_INTERVAL_MS * GSOUND_SBC_SAMPLE_RATE_VALUE) / 1000) * GSOUND_SBC_SIZE_PER_SAMPLE)

#ifdef __cplusplus
}
#endif

#endif  // __GSOUND_SBC_H__

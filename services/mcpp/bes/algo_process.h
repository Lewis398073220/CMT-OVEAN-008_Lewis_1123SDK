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
#ifndef __ALGO_PROCESS_H__
#define __ALGO_PROCESS_H__

#include "plat_types.h"
#include "app_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALGO_PROCESS_CALL_POOL_BUF_SIZE          (1024 * 50)
#define ALGO_PROCESS_AUDIO_POOL_BUF_SIZE         (1024 * 30)

/**
 * Please calculate ALGO_PROCESS_HEAP_BUF_SIZE as the maximum buffer size of all users.
*/
#define ALGO_PROCESS_HEAP_BUF_SIZE \
    MAX(ALGO_PROCESS_CALL_POOL_BUF_SIZE, ALGO_PROCESS_AUDIO_POOL_BUF_SIZE)

#define MCPP_SERVER_RPC_BUF_SIZE                 (1024 * 50)

/**
 *  1. If SYSFREQ_VALUE_FOR_CALC_MCPS is not 0, just use this vaule to calculate MCPS.
 *     So make sure the SYSFREQ_VALUE_FOR_CALC_MCPS is correctly.
 *  2. If SYSFREQ_VALUE_FOR_CALC_MCPS is 0, stream_mcps module will get sys_freq in real time.
 *     This will cause pop noise.
 */
#define CALC_MIPS_CNT_THD_MS                     (3000)
#define SYSFREQ_VALUE_FOR_CALC_MCPS              (96)

#define MCPP_SRV_SYSFREQ                         APP_SYSFREQ_104M

typedef enum {
    PCM_CHANNEL_INDEX_MIC1 = 0,
    PCM_CHANNEL_INDEX_MIC2,

    // Indexs 0 to 7 are used to index mic.
    PCM_CHANNEL_INDEX_REF = 8,
    PCM_CHANNEL_INDEX_REF2,
    PCM_CHANNEL_INDEX_VPU,

    PCM_CHANNEL_INDEX_QTY
} PCM_CHANNEL_INDEX_T;

typedef struct {
    uint32_t sample_rate;
    uint32_t sample_bytes;
    uint32_t frame_len;
    uint32_t mic_num;
    uint32_t channel_map;
    uint32_t delay;
    uint32_t mode;
} algo_process_cfg_t;

typedef int32_t (*algo_process_open_t)(algo_process_cfg_t *cfg);
typedef int32_t (*algo_process_close_t)(void);
typedef int32_t (*algo_process_set_cfg_t)(uint8_t *cfg, uint32_t len);
typedef int32_t (*algo_process_ctl_t)(uint32_t ctl, uint8_t *ptr, uint32_t ptr_len);
typedef int32_t (*algo_process_process_t)(uint8_t *in[PCM_CHANNEL_INDEX_QTY], uint8_t *out, uint32_t frame_len);

typedef struct {
    algo_process_open_t      open;
    algo_process_close_t     close;
    algo_process_set_cfg_t   set_cfg;
    algo_process_ctl_t       ctl;
    algo_process_process_t   process;
} algo_process_stream_t;

extern const algo_process_stream_t algo_process_call_capture;
extern const algo_process_stream_t algo_process_call_playback;

extern const algo_process_stream_t algo_process_audio_playback;

const algo_process_stream_t *load_capture_algo_process(int32_t user);
const algo_process_stream_t *load_playback_algo_process(int32_t user);
uint32_t get_algo_process_heap_buffer_size(int32_t user);

#ifdef __cplusplus
}
#endif

#endif

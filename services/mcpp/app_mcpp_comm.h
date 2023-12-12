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
#ifndef __APP_MCPP_COMM_H__
#define __APP_MCPP_COMM_H__

#include <stdint.h>
#include "kfifo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PCM_CHANNEL_MAP_CAP_REF                 (1 << 8)
#define PCM_CHANNEL_MAP_CAP_REF2                (1 << 9)
#define PCM_CHANNEL_MAP_CAP_VPU                 (1 << 10)

typedef enum {
    APP_MCPP_CORE_BTH,
    APP_MCPP_CORE_M55,
    APP_MCPP_CORE_HIFI,
    APP_MCPP_CORE_SENS,
    APP_MCPP_CORE_CP_SUBSYS,

    APP_MCPP_CORE_XXXX,

    APP_MCPP_CORE_QTY
} APP_MCPP_CORE_T;

typedef enum {
    APP_MCPP_USER_CALL,
    APP_MCPP_USER_AUDIO,

    APP_MCPP_USER_TEST_01,
    APP_MCPP_USER_TEST_02,

    APP_MCPP_USER_QTY,
} APP_MCPP_USER_T;

typedef struct {
    struct kfifo pcm_fifo;
    uint8_t *pcm_fifo_mem;
    struct kfifo ref_fifo;
    uint8_t *ref_fifo_mem;
    struct kfifo ref2_fifo;
    uint8_t *ref2_fifo_mem;
    struct kfifo vpu_fifo;
    uint8_t *vpu_fifo_mem;
    struct kfifo out_fifo;
    uint8_t *out_fifo_mem;
} capture_fifo_ctx_t;

typedef struct {
    struct kfifo pcm_fifo;
    uint8_t *pcm_fifo_mem;
    struct kfifo out_fifo;
    uint8_t *out_fifo_mem;
} playback_fifo_ctx_t;

typedef struct {
    int     sample_rate;            // Necessary.
    int     sample_bytes;           // Necessary.
    int     frame_len;              // Not necessary, IO frame len
    int     algo_frame_len;         // Necessary, algo process frame len
    int     channel_num;            // Indicates PCM in buffer channel numbers
    /**
     * Normally, if frame_len == algo_frame_len, set delay = frame_len, or delay = 0.
     * If frame_len != algo_frame_len, set delay = 2 * frame_len.
     */
    int     delay;
    /**
     * Not necessary. Depend on your algo and cap or play stream.
     * if capture stream:
     * params[0] represents the mode which is used to indicate codec type;
     * params[1] represents the channel map, it is necessary.
    */
    int     params[3];
    bool    stream_enable;          // Necessary, enable or disable stream algo process.
    APP_MCPP_CORE_T core_server;    // Necessary
} APP_MCPP_STREAM_CFG_T;

typedef struct {
    APP_MCPP_STREAM_CFG_T capture;
    APP_MCPP_STREAM_CFG_T playback;
} APP_MCPP_CFG_T;

typedef struct {
    void    *in;
    void    *ref;
    void    *ref2;
    void    *vpu;
    void    *out;
    int     frame_len;
} APP_MCPP_CAP_PCM_T;

typedef struct {
    void    *in;
    void    *out;
    int     frame_len;
} APP_MCPP_PLAY_PCM_T;

typedef void(*app_mcpp_async_handler_t)(unsigned char *ptr, short ptr_len);

#ifdef __cplusplus
}
#endif

#endif

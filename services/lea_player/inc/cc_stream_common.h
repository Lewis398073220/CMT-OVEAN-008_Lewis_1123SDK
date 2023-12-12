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

#ifndef __CC_STREAM_COMMON_H__
#define __CC_STREAM_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************header include********************************/
#include "cfifo.h"
#include "gaf_codec_common.h"
/******************************macro defination*****************************/
#ifdef LEA_FOUR_CIS_ENABLED
#define PLAYBACK_INSTANCE_MAX    (4)
#else
#define PLAYBACK_INSTANCE_MAX    (2)
#endif
#define CAPTURE_INSTANCE_MAX     (1)
/******************************type defination******************************/
typedef void* (*CC_HEAP_ALLOC)(void *pool, uint32_t size);
typedef void (*CC_PLAYBACK_INIT)(uint8_t instance_handle);
typedef void (*CC_PLAYBACK_INIT_RSP)(void *iniRsp, uint16_t len);
typedef bool (*CC_PLAYBACK_DEINIT)(uint8_t instance_handle);
typedef void (*CC_PLAYBACK_DEINIT_RSP)(uint8_t *rsp, uint16_t len);
typedef void (*CC_CAPTURE_INIT)(uint8_t instance_handle);
typedef void (*CC_CAPTURE_INIT_RSP)(void *initRsp, uint16_t len);
typedef bool (*CC_CAPTURE_DEINIT)(uint8_t instance_handle);
typedef void (*CC_CAPTURE_DEINIT_RSP)(uint8_t *rsp, uint16_t len);

typedef uint32_t (*CC_CAPTURE_ALGO_RUN)(uint8_t *buf, uint32_t len);
typedef int32_t (*CC_CAPTURE_ALGO_OPEN)(void *algo_cfg, void *encoder_info);
typedef int32_t (*CC_CAPTURE_ALGO_CLOSE)(void);
typedef int32_t (*CC_CAPTURE_ALGO_BUF_INIT)(void *buf_alloc);
typedef void (*CC_CAPTURE_ALGO_BUF_DEINIT)(void);

typedef struct
{
    CC_HEAP_ALLOC buf_alloc;
    CC_PLAYBACK_INIT playback_init;
    CC_PLAYBACK_INIT_RSP playback_init_rsp;
    CC_PLAYBACK_DEINIT playback_deinit;
    CC_PLAYBACK_DEINIT_RSP playback_deinit_rsp;
} CC_CORE_PLAYBACK_FUNC_LIST_T;

typedef struct
{
    CC_HEAP_ALLOC buf_alloc;
    CC_CAPTURE_INIT capture_init;
    CC_CAPTURE_INIT_RSP capture_init_rsp;
    CC_CAPTURE_DEINIT capture_deinit;
    CC_CAPTURE_DEINIT_RSP capture_deinit_rsp;
} CC_CORE_CAPTURE_FUNC_LIST_T;

typedef struct
{
    CC_CAPTURE_ALGO_RUN         capture_algo_run;
    CC_CAPTURE_ALGO_OPEN        capture_algo_open;
    CC_CAPTURE_ALGO_CLOSE       capture_algo_close;
    CC_CAPTURE_ALGO_BUF_INIT    capture_algo_buf_init;
    CC_CAPTURE_ALGO_BUF_DEINIT  capture_algo_buf_deinit;
} CC_CAPTURE_ALGO_FUNC_LIST_T;

typedef struct
{
    uint8_t *initReq;
    uint32_t core_type;
    const CC_CORE_PLAYBACK_FUNC_LIST_T *playback_func_list;
} CORE_PLAYBACK_INIT_INFO_T;

typedef struct
{
    uint8_t *initReq;
    uint32_t core_type;
    const CC_CORE_CAPTURE_FUNC_LIST_T *capture_func_list;
} CORE_CAPTURE_INIT_INFO_T;

typedef enum
{
    CP_CORE = 0,
    M55_CORE = 1,
    HIFI_CORE = 2,
    M33_CORE = 3,
    CORE_TYPE_MAX,
} CORE_TYPE_E;

typedef enum
{
    INSTANCE_IDLE = 0,
    INSTANCE_INITIALIZING = 1,
    INSTANCE_INITIALIZED = 2,
    INSTANCE_STREAMING = 3,
} INSTANCE_STATUS;

typedef struct
{
    uint32_t time_stamp;
    uint16_t seq_nb;
    uint32_t data_len;
    uint8_t *data;
    bool isPLC;
} CC_PLAYBACK_DATA_T;

typedef struct
{
    uint32_t time_stamp;
    uint16_t data_len;
    uint8_t *data;
    uint16_t frame_size;
} CC_CAPTURE_DATA_T;

typedef struct
{
    CODEC_INFO_T decoder_info;
    uint8_t codec_type;
    bool isBis;
    void *ptr;
    void *ptr2;
    // cfifo *frame_fifo;
    // cfifo *pcm_fifo;
} __attribute__ ((__packed__))CC_PLAYBACK_INIT_REQ_T;

typedef struct
{
    cfifo *frame_fifo;
    cfifo *pcm_fifo;
    bool *isDecoding;
    uint8_t instance_handle;
    bool isBis;
    void *ptr;
    void *ptr2;
} __attribute__ ((__packed__))CC_PLAYBACK_INIT_RSP_T;

typedef struct
{
    uint8_t instance_handle;
    bool isRetrigger;
    bool isBis;
} __attribute__ ((__packed__))CC_PLAYBACK_DEINIT_REQ_T;

typedef struct
{
    bool closeDecoderCore;
    bool isBis;
} __attribute__ ((__packed__))CC_PLAYBACK_DEINIT_RSP_T;

typedef struct
{
    uint32_t    bypass;
    uint32_t    frame_len;
    uint32_t    algo_frame_len;
    uint32_t    sample_rate;
    uint32_t    channel_num;
    uint32_t    bits;
} __attribute__ ((__packed__))TX_ALGO_CFG_T;

typedef struct
{
    TX_ALGO_CFG_T tx_algo_cfg;
    CODEC_INFO_T encoder_info;
    uint8_t codec_type;
    uint16_t pcm_size;
    bool isBis;
    bool algoProcess;
    void *ptr;
    void *ptr2;
    // cfifo *pcm_fifo;
    // cfifo *frame_fifo;
} __attribute__ ((__packed__))CC_CAPTURE_INIT_REQ_T;

typedef struct
{
    cfifo *pcm_fifo;
    cfifo *vpu_fifo;
    cfifo *ref1_fifo;
    cfifo *ref2_fifo;
    cfifo *frame_fifo;
    bool *isEncoding;
    uint8_t instance_handle;
    bool isBis;
    void *ptr;
    void *ptr2;
} __attribute__ ((__packed__))CC_CAPTURE_INIT_RSP_T;

typedef struct
{
    uint8_t instance_handle;
    bool isRetrigger;
    bool isBis;
} __attribute__ ((__packed__))CC_CAPTURE_DEINIT_REQ_T;

typedef struct
{
    bool closeEncoderCore;
    bool isBis;
} __attribute__ ((__packed__))CC_CAPTURE_DEINIT_RSP_T;

typedef struct
{
    CODEC_INFO_T decoder_info;
    uint32_t pcmSize;
    uint32_t frameSize;
}CC_PLAYBACK_INFO_T;

typedef struct
{
    CODEC_INFO_T encoder_info;
    uint32_t pcmSize;
    uint32_t frameSize;
}CC_CAPTURE_INFO_T;

typedef struct
{
    uint16_t last_seq;

    cfifo *frame_fifo;
    uint8_t *frame_buf;
    cfifo *pcm_fifo;
    uint8_t *pcm_buf;
    uint8_t *frame_cache;
    uint8_t *pcm_cache;
}CC_PLAYBACK_CONTEXT_T;

typedef struct
{
    cfifo *pcm_fifo;
    uint8_t *pcm_buf;
    cfifo *frame_fifo;
    uint8_t *frame_buf;
    uint8_t *frame_cache;
    uint8_t *pcm_cache;
}CC_CAPTURE_CONTEXT_T;

typedef struct
{
    CC_PLAYBACK_INFO_T playback_info;
    CC_PLAYBACK_CONTEXT_T playback_context;
    GAF_DECODER_FUNC_LIST_T *dec_func_list;
    const CC_CORE_PLAYBACK_FUNC_LIST_T *core_playback_func_list;
    CORE_TYPE_E core;
    bool buffer_initialized;
    INSTANCE_STATUS status;
} CC_PLAYBACK_INSTANCE;

typedef struct
{
    CC_CAPTURE_INFO_T capture_info;
    CC_CAPTURE_CONTEXT_T capture_context;
    GAF_ENCODER_FUNC_LIST_T *enc_func_list;
    const CC_CORE_CAPTURE_FUNC_LIST_T *core_capture_func_list;
    CC_CAPTURE_ALGO_FUNC_LIST_T *capture_algo_func_list;
    CORE_TYPE_E core;
    bool buffer_initialized;
    INSTANCE_STATUS status;
} CC_CAPTURE_INSTANCE;

/****************************function declaration***************************/
void cc_stream_boost_freq(void);

void cc_stream_process(void);

void cc_playback_init(CORE_PLAYBACK_INIT_INFO_T *initInfo);

void cc_capture_init(CORE_CAPTURE_INIT_INFO_T *initInfo);

void cc_playback_deinit(void *_deinitReq);

void cc_capture_deinit(void *_deinitReq);

void cc_playback_buf_deinit(CORE_TYPE_E core_type);

void cc_capture_buf_deinit(CORE_TYPE_E core_type);

void cc_stream_data_notify_handler(void);

void cc_playback_init_req_handler(CORE_PLAYBACK_INIT_INFO_T *initInfo);

void cc_capture_init_req_handler(CORE_CAPTURE_INIT_INFO_T *initInfo);

void cc_playback_deinit_req_handler(uint8_t *deinitReq);

void cc_capture_deinit_req_handler(uint8_t *deinitReq);

void cc_stream_init(uint32_t core_type);

#ifdef __cplusplus
}
#endif

#endif /// __CC_STREAM_COMMON_H__

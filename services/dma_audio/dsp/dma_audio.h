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
#ifndef __DMA_AUDIO_H__
#define __DMA_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_aud.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "stream_dma_rpc.h"
#include "dma_audio_def.h"
#include "dma_audio_public.h"
#include "dma_audio_algo.h"

//#define DAUD_DEBUG 1
//#define DAUD_WARNING

#ifdef DAUD_DEBUG
#ifndef DAUD_TRACE
#define DAUD_TRACE(attr, str, ...)      TR_INFO(attr, str, ##__VA_ARGS__)
#endif
#else
#ifndef DAUD_TRACE
#define DAUD_TRACE(...)                 do { } while(0)
#endif
#endif /* DAUD_DEBUG */

#ifdef DAUD_WARNING
#ifndef DAUD_WARN
#define DAUD_WARN(attr, str, ...)       TR_INFO(attr, str, ##__VA_ARGS__)
#endif
#else
#ifndef DAUD_WARN
#define DAUD_WARN(...)                  do { } while(0)
#endif
#endif

enum DAUD_RESULT_T {
    DAUD_RES_SUCCESS = 0,
    DAUD_RES_FAILED = 1
};

enum DAUD_USER_T {
    DAUD_USER_AI,
    DAUD_USER_THIRDPART,
    DAUD_USER_AUDIO,
    DAUD_USER_PSAP,

    DAUD_USER_QTY
};

enum DAUD_EVT_HDLR_ID_T {
    DAUD_EVT_HDLR_ID_DMA,
    DAUD_EVT_HDLR_ID_STREAM,

    DAUD_EVT_HDLR_ID_QTY
};

enum DAUD_EVT_T {
    /* DMA event */
    DAUD_EVT_DMA_INIT             = DMA_RPC_EVT_DMA_INIT,
    DAUD_EVT_DMA_START            = DMA_RPC_EVT_DMA_START,
    DAUD_EVT_DMA_STOP             = DMA_RPC_EVT_DMA_STOP,
    DAUD_EVT_DMA_GET_CHAN         = DMA_RPC_EVT_DMA_GET_CHAN,
    /* STREAM event */
    DAUD_EVT_STREAM_FADEIN        = DMA_RPC_EVT_STREAM_FADEIN,
    DAUD_EVT_STREAM_FADEOUT       = DMA_RPC_EVT_STREAM_FADEOUT,
    DAUD_EVT_STREAM_OPEN          = DMA_RPC_EVT_STREAM_OPEN,
    DAUD_EVT_STREAM_START         = DMA_RPC_EVT_STREAM_START,
    DAUD_EVT_STREAM_STOP          = DMA_RPC_EVT_STREAM_STOP,
    DAUD_EVT_STREAM_CLOSE         = DMA_RPC_EVT_STREAM_CLOSE,
    DAUD_EVT_STREAM_GET_CFG       = DMA_RPC_EVT_STREAM_GET_CFG,
    DAUD_EVT_STREAM_SET_CFG       = DMA_RPC_EVT_STREAM_SET_CFG,
    DAUD_EVT_STREAM_GET_ALGO_CFG  = DMA_RPC_EVT_STREAM_GET_ALGO_CFG,
    DAUD_EVT_STREAM_SET_ALGO_CFG  = DMA_RPC_EVT_STREAM_SET_ALGO_CFG,
};

#define IS_DAUD_STREAM_EVT(evt) ( \
    (evt == DAUD_EVT_STREAM_OPEN) || \
    (evt == DAUD_EVT_STREAM_START) || \
    (evt == DAUD_EVT_STREAM_STOP) || \
    (evt == DAUD_EVT_STREAM_CLOSE) || \
    (evt == DAUD_EVT_STREAM_GET_CFG) || \
    (evt == DAUD_EVT_STREAM_SET_CFG) || \
    (evt == DAUD_EVT_STREAM_GET_ALGO_CFG) || \
    (evt == DAUD_EVT_STREAM_GET_ALGO_CFG) \
    )

#define IS_DAUD_DMA_EVT(evt) ( \
    (evt == DAUD_EVT_DMA_INIT) || \
    (evt == DAUD_EVT_DMA_START) || \
    (evt == DAUD_EVT_DMA_STOP) || \
    (evt == DAUD_EVT_DMA_GET_CHAN) \
	)

#define DAUD_STREAM_FLAG_FORWARD 0x2

/*
 * DAUD_EVT_HDLR_T
 * for dma event handler:
 *     param0: dma channel number;
 *     param1: zero;
 *
 * for stream event handler:
 *     param0: stream id AUD_STREAM_ID_{0,1,2}
 *     param1: stream type AUD_STREAM_PLAYBACK or AUD_STREAM_CAPTURE
 */
typedef void (*DAUD_EVT_HDLR_T)(enum DAUD_EVT_T evt, uint32_t p0, uint32_t p1, uint32_t p2);

typedef void (*DAUD_IRQ_NOTIFICATION_T)(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

typedef int (*DAUD_ALGO_CFG_NOTIFY_T)(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    void *cfg, uint32_t len, bool set);

uint32_t daud_open(void);
uint32_t daud_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    const struct DAUD_STREAM_CONFIG_T *cfg);
uint32_t daud_stream_setup(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    const struct DAUD_STREAM_CONFIG_T *cfg);
uint32_t daud_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
uint32_t daud_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
uint32_t daud_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
void daud_close(void);

uint32_t daud_forward_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct DAUD_STREAM_CONFIG_T *cfg);
uint32_t daud_forward_stream_setup(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct DAUD_STREAM_CONFIG_T *cfg);
uint32_t daud_forward_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
uint32_t daud_forward_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
uint32_t daud_forward_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

bool daud_client_stream_is_opened(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
bool daud_client_stream_is_started(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

int daud_get_priority(void);
int daud_get_default_priority(void);
void daud_set_priority(enum DAUD_USER_T user, int priority);
void daud_reset_priority(void);

void daud_set_dma_irq_notification(DAUD_IRQ_NOTIFICATION_T notif);
void daud_set_event_handler(enum DAUD_EVT_HDLR_ID_T id, DAUD_EVT_HDLR_T handler);
void daud_set_algo_config_notify(DAUD_ALGO_CFG_NOTIFY_T notify);

void *daud_heap_malloc(uint32_t size);
void daud_heap_free(void *ptr);

uint32_t daud_samp_bits_to_size(uint32_t bits);

uint32_t daud_stream_get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

enum AUD_STREAM_USE_DEVICE_T daud_stream_get_device(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);
bool daud_stream_use_tdm_slv_device(enum AUD_STREAM_USE_DEVICE_T device);
bool daud_stream_use_tdm_mst_device(enum AUD_STREAM_USE_DEVICE_T device);
bool daud_stream_use_i2s_slv_device(enum AUD_STREAM_USE_DEVICE_T device);
bool daud_stream_use_i2s_mst_device(enum AUD_STREAM_USE_DEVICE_T device);
bool daud_stream_use_i2s_tdm_device(enum AUD_STREAM_USE_DEVICE_T device);
bool daud_stream_use_codec_device(enum AUD_STREAM_USE_DEVICE_T device);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_AUDIO_APP_H__ */


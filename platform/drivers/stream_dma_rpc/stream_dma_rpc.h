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
#ifndef __STREAM_DMA_RPC_H__
#define __STREAM_DMA_RPC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_dma.h"
#include "hal_aud.h"

#define DMA_VIR_CHAN_ID_INI_VALUE 0x4
#define DMA_VIR_CHAN_ID_END_VALUE 0xF

#define VIRTUAL_ATTR_MASK     (0x8000)
#define VIRTUAL_RPC_ID(id)    ((id) | VIRTUAL_ATTR_MASK)
#define IS_VIRTUAL_RPC_ID(id) ((id) & VIRTUAL_ATTR_MASK)
#define RPC_ID_VIR_ATTR(id)   ((id) & VIRTUAL_ATTR_MASK)
#define RPC_ID_VAL_ATTR(id)   ((id) & (~VIRTUAL_ATTR_MASK))

enum DMA_RPC_CMD_ID_T {
    DMA_RPC_CMD_DMA_SG_START,
    DMA_RPC_CMD_DMA_SG_2D_START,
    DMA_RPC_CMD_DMA_INIT_DESC,
    DMA_RPC_CMD_DMA_GET_CHAN,
    DMA_RPC_CMD_DMA_GET_BASE_ADDR,
    DMA_RPC_CMD_DMA_STOP,
    DMA_RPC_CMD_DMA_FREE_CHAN,
    DMA_RPC_CMD_DMA_TC_IRQ_ENABLE,
    DMA_RPC_CMD_DMA_TC_IRQ_DISABLE,
    DMA_RPC_CMD_SYNC_DATA,
    DMA_RPC_CMD_STREAM_FADEIN,
    DMA_RPC_CMD_STREAM_FADEOUT,
    DMA_RPC_CMD_STREAM_OPEN,
    DMA_RPC_CMD_STREAM_CLOSE,
    DMA_RPC_CMD_STREAM_START,
    DMA_RPC_CMD_STREAM_STOP,
    DMA_RPC_CMD_STREAM_GET_CFG,
    DMA_RPC_CMD_STREAM_SET_CFG,
    DMA_RPC_CMD_STREAM_GET_ALGO_CFG,
    DMA_RPC_CMD_STREAM_SET_ALGO_CFG,
    DMA_RPC_CMD_STREAM_TRANSFER,
    DMA_RPC_CMD_TUNE_CLOCK,

    DMA_RPC_CMD_ID_QTY
};

enum DMA_RPC_EVT_T {
    DMA_RPC_EVT_DMA_START,
    DMA_RPC_EVT_DMA_STOP,
    DMA_RPC_EVT_DMA_INIT,
    DMA_RPC_EVT_DMA_GET_CHAN,
    DMA_RPC_EVT_SYNC_CTL_DATA,
    DMA_RPC_EVT_STREAM_FADEIN,
    DMA_RPC_EVT_STREAM_FADEOUT,
    DMA_RPC_EVT_STREAM_OPEN,
    DMA_RPC_EVT_STREAM_CLOSE,
    DMA_RPC_EVT_STREAM_START,
    DMA_RPC_EVT_STREAM_STOP,
    DMA_RPC_EVT_STREAM_GET_CFG,
    DMA_RPC_EVT_STREAM_SET_CFG,
    DMA_RPC_EVT_STREAM_GET_ALGO_CFG,
    DMA_RPC_EVT_STREAM_SET_ALGO_CFG,

    DMA_RPC_EVT_QTY
};

enum DMA_RPC_IRQ_HDLR_ID_T {
    DMA_RPC_IRQ_HDLR_ID_0,
    DMA_RPC_IRQ_HDLR_ID_1,

    DMA_RPC_IRQ_HDLR_QTY
};

typedef void (*DMA_RPC_EVT_HANDLER_T)(enum DMA_RPC_EVT_T evt, void *param);

typedef void (*DMA_RPC_PRE_CMD_HANDLER_T)(enum DMA_RPC_CMD_ID_T cmd,
    uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3);

typedef void (*DMA_RPC_POST_CMD_HANDLER_T)(enum DMA_RPC_CMD_ID_T cmd, uint32_t ret);

typedef void (*DMA_RPC_ERR_CMD_HANDLER_T)(enum DMA_RPC_CMD_ID_T cmd, uint32_t ret);

typedef int (*DMA_RPC_SEND_CMD_HANDLER_T)(const void *data, unsigned int len);

typedef void (*DMA_RPC_TUNE_CLOCK_HANDLER_T)(float ratio);

enum DMA_MSG_ID_T {
    DMA_MSG_ID_RPC = 0xDA, // RPC CMD
    DMA_MSG_ID_QTY
};

struct DMA_MSG_HDR_T {
    uint16_t id            : 8;
    uint16_t reply         : 1;
    uint16_t sync_flag     : 1;
    uint16_t wait_sync_flag: 1;
    uint16_t rsv           : 5;
};

#define DMA_RPC_REQ_MSG_PARAM_NUM 8
struct DMA_RPC_REQ_MSG_T {
    struct DMA_MSG_HDR_T hdr;
    uint16_t rpc_id;
    uint32_t param[DMA_RPC_REQ_MSG_PARAM_NUM];
};

struct DMA_RPC_REPLY_MSG_T {
    struct DMA_MSG_HDR_T hdr;
    uint16_t rpc_id;
    uint32_t ret;
};

struct DMA_RPC_ERR_TAG_T {
    struct DMA_MSG_HDR_T hdr;
    uint16_t rpc_id;
    int ret;
};

#define DMA_RPC_ERR_TAG_NUM 5

struct DMA_RPC_ERR_LIST_T {
    struct DMA_RPC_ERR_TAG_T tag[DMA_RPC_ERR_TAG_NUM];
    int idx;
};

struct DMA_RPC_STREAM_PARAM_T {
    enum AUD_STREAM_ID_T id;
    enum AUD_STREAM_T stream;
    uint8_t dma_chan;
    uint32_t dma_addr;
};

/* DMA RPC SERVER APIs */
void dma_rpc_setup_irq_handler(enum DMA_RPC_IRQ_HDLR_ID_T id, HAL_DMA_IRQ_HANDLER_T handler);

void dma_rpc_setup_evt_handler(DMA_RPC_EVT_HANDLER_T handler);

uint8_t dma_rpc_get_dma_hw_chan(enum HAL_DMA_PERIPH_T periph);

void dma_rpc_stream_data_transfer(struct DMA_RPC_STREAM_PARAM_T *param);

void dma_rpc_tune_clock(float ratio);

/* DMA RPC CLIENT APIs */
enum HAL_DMA_RET_T dma_rpc_sg_2d_start(const struct HAL_DMA_DESC_T *desc,
                                       const struct HAL_DMA_CH_CFG_T *cfg,
                                       const struct HAL_DMA_2D_CFG_T *src_2d,
                                       const struct HAL_DMA_2D_CFG_T *dst_2d);

enum HAL_DMA_RET_T dma_rpc_sg_start(const struct HAL_DMA_DESC_T *desc,
                                    const struct HAL_DMA_CH_CFG_T *cfg);

enum HAL_DMA_RET_T dma_rpc_init_desc(struct HAL_DMA_DESC_T *desc,
                                     const struct HAL_DMA_CH_CFG_T *cfg,
                                     const struct HAL_DMA_DESC_T *next,
                                     int tc_irq);

uint8_t dma_rpc_get_chan(enum HAL_DMA_PERIPH_T periph, enum HAL_DMA_GET_CHAN_T policy);

uint32_t dma_rpc_get_base_addr(uint8_t ch);

void dma_rpc_stop(uint8_t ch);

void dma_rpc_free_chan(uint8_t ch);

void dma_rpc_tc_irq_enable(uint8_t ch);

void dma_rpc_tc_irq_disable(uint8_t ch);

void dma_rpc_sync_data(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *param, uint32_t len);

uint32_t dma_rpc_stream_fadein(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, uint32_t ms);

uint32_t dma_rpc_stream_fadeout(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, uint32_t ms);

uint32_t dma_rpc_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg);

uint32_t dma_rpc_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

uint32_t dma_rpc_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

uint32_t dma_rpc_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

uint32_t dma_rpc_get_stream_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg);

uint32_t dma_rpc_set_stream_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg);

uint32_t dma_rpc_get_algo_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg);

uint32_t dma_rpc_set_algo_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg, uint32_t len);

void dma_rpc_setup_pre_cmd_handler(DMA_RPC_PRE_CMD_HANDLER_T handler);

void dma_rpc_setup_post_cmd_handler(DMA_RPC_POST_CMD_HANDLER_T handler);

void dma_rpc_setup_error_cmd_handler(DMA_RPC_ERR_CMD_HANDLER_T handler);

void dma_rpc_setup_tune_clock_handler(DMA_RPC_TUNE_CLOCK_HANDLER_T handler);

uint32_t dma_rpc_get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

/* DMA RPC PUBLIC APIs */
unsigned int dma_rpcif_rx_handler(const void *data, unsigned int len);

void dma_rpcif_tx_handler(const void *data, unsigned int len);

void dma_rpcif_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

void dma_rpcif_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

bool dma_rpcif_enabled(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

void dma_rpcif_setup_send_cmd_handler(DMA_RPC_SEND_CMD_HANDLER_T handler);

bool dma_rpcif_forward_enabled(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

void dma_rpcif_forward_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

void dma_rpcif_forward_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

#ifdef __cplusplus
}
#endif

#endif /* __STREAM_DMA_RPC_H__ */

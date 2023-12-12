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
#if defined(DMA_RPC_CLI) || defined(DMA_RPC_SVR)
#include "string.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_dma.h"
#include "stream_dma_rpc.h"
#include "dsp_m55.h"

//#define DMA_RPC_INFO

#ifdef DMA_RPC_DEBUG
#define RPC_TRACE TRACE
#else
#define RPC_TRACE(...) do{}while(0)
#endif

#ifdef DMA_RPC_INFO
#define RPC_INFO TRACE
#else
#define RPC_INFO(...) do{}while(0)
#endif
#define RPC_WARN TRACE
#define RPC_ERROR TRACE

#ifdef DMA_RPC_CLI
static bool rpccli_opened = false;
static bool rpccli_stream_map[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];
static struct DMA_RPC_REQ_MSG_T dma_req_msg;
static struct DMA_RPC_REPLY_MSG_T  dma_ack_msg;
static bool rpc_ack_done = false;
static DMA_RPC_PRE_CMD_HANDLER_T dma_rpc_pre_cmd_handler = NULL;
static DMA_RPC_POST_CMD_HANDLER_T dma_rpc_post_cmd_handler = NULL;
static DMA_RPC_ERR_CMD_HANDLER_T dma_rpc_err_cmd_handler = NULL;
static DMA_RPC_TUNE_CLOCK_HANDLER_T dma_rpc_tune_clock_handler = NULL;
#endif

#ifdef DMA_RPC_SVR
static bool rpcsvr_opened = false;
static bool rpcsvr_stream_map[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];
static struct DMA_RPC_REPLY_MSG_T dma_ack_msg2;
static struct DMA_RPC_REQ_MSG_T dma_req_msg2;
static DMA_RPC_EVT_HANDLER_T dma_evt_handler = NULL;
static HAL_DMA_IRQ_HANDLER_T dma_irq_hdlr[DMA_RPC_IRQ_HDLR_QTY];
static HAL_DMA_IRQ_HANDLER_T ptr_dma_irq_handler = NULL;
static int dma_rpcsvr_msg_handler(struct DMA_RPC_REQ_MSG_T *msg);

static bool dma_sync_data_done = true;
#endif

#ifdef DMA_RPC_STREAM_API
struct dma_rpc_stream_cntl_t {
    bool forward;
    uint32_t dma_addr;
};

static enum AUD_STREAM_ID_T dma_vir_id;
static enum AUD_STREAM_T dma_vir_stream;
static struct dma_rpc_stream_cntl_t dma_rpc_stream_cntl[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];
#endif

static DMA_RPC_SEND_CMD_HANDLER_T rpcif_send_cmd_handler = NULL;
static DSP_M55_RX_IRQ_HANDLER rpcif_rx_handler = NULL;
static DSP_M55_TX_IRQ_HANDLER rpcif_tx_handler = NULL;

#if (DMA_RPC_ERR_TAG_NUM > 0)
static struct DMA_RPC_ERR_LIST_T dma_rpc_errs;

static POSSIBLY_UNUSED void dma_rpc_err_list_init(void)
{
    memset((void *)&dma_rpc_errs, 0, sizeof(struct DMA_RPC_ERR_LIST_T));
}

static POSSIBLY_UNUSED void dma_rpc_err_print(int thresh)
{
    int i;
    struct DMA_RPC_ERR_LIST_T *err = &dma_rpc_errs;

    if (err->idx < thresh) {
        return;
    }
    for (i = 0; i < err->idx; i++) {
        RPC_ERROR(1, "tag[%d]:hdr[%d,%d],rpc_id=%d,ret=%d",
            i, err->tag[i].hdr.id,err->tag[i].hdr.reply,
            err->tag[i].rpc_id, err->tag[i].ret);
    }
}

static POSSIBLY_UNUSED int dma_rpc_err_add(struct DMA_MSG_HDR_T *hdr, uint16_t rpc_id, int ret)
{
    struct DMA_RPC_ERR_LIST_T *err = &dma_rpc_errs;

    if (err->idx >= DMA_RPC_ERR_TAG_NUM) {
        RPC_ERROR(1, "[dma_rpc_err_add]: too many errs %d", err->idx);
        return 1;
    }
    err->tag[err->idx].hdr.id = hdr->id;
    err->tag[err->idx].hdr.reply = hdr->reply;
    err->tag[err->idx].rpc_id = rpc_id;
    err->tag[err->idx].ret = ret;
    err->idx++;
    return 0;
}
#endif

#define CASE_ENUM(e) case e: return "["#e"]"
static const char *get_cmd_string_name(uint16_t id)
{
    switch (id) {
    CASE_ENUM(DMA_RPC_CMD_DMA_SG_START);
    CASE_ENUM(DMA_RPC_CMD_DMA_SG_2D_START);
    CASE_ENUM(DMA_RPC_CMD_DMA_INIT_DESC);
    CASE_ENUM(DMA_RPC_CMD_DMA_GET_CHAN);
    CASE_ENUM(DMA_RPC_CMD_DMA_GET_BASE_ADDR);
    CASE_ENUM(DMA_RPC_CMD_DMA_STOP);
    CASE_ENUM(DMA_RPC_CMD_DMA_FREE_CHAN);
    CASE_ENUM(DMA_RPC_CMD_SYNC_DATA);
    CASE_ENUM(DMA_RPC_CMD_STREAM_OPEN);
    CASE_ENUM(DMA_RPC_CMD_STREAM_CLOSE);
    CASE_ENUM(DMA_RPC_CMD_STREAM_START);
    CASE_ENUM(DMA_RPC_CMD_STREAM_STOP);
    CASE_ENUM(DMA_RPC_CMD_STREAM_GET_CFG);
    CASE_ENUM(DMA_RPC_CMD_STREAM_SET_CFG);
    CASE_ENUM(DMA_RPC_CMD_STREAM_GET_ALGO_CFG);
    CASE_ENUM(DMA_RPC_CMD_STREAM_SET_ALGO_CFG);
    CASE_ENUM(DMA_RPC_CMD_STREAM_TRANSFER);
    }
    return " ";
}

static int dma_rpcif_send(const void *data, unsigned int len)
{
    if (rpcif_send_cmd_handler) {
        return rpcif_send_cmd_handler(data, len);
    }
    return -1;
}

#ifdef DMA_RPC_STREAM_API
POSSIBLY_UNUSED static bool dma_rpc_stream_get_forward_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s]: Bad touched stream id %d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s]: Bad touched stream %d", __func__, stream);
    return dma_rpc_stream_cntl[id][stream].forward;
}

POSSIBLY_UNUSED static void dma_rpc_stream_set_forward_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, bool forward)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s]: Bad touched stream id %d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s]: Bad touched stream %d", __func__, stream);
    dma_rpc_stream_cntl[id][stream].forward = forward;
}

POSSIBLY_UNUSED static void dma_rpc_stream_set_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, uint32_t dma_addr)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s]: Bad touched stream id %d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s]: Bad touched stream %d", __func__, stream);
    dma_rpc_stream_cntl[id][stream].dma_addr = dma_addr;
}

POSSIBLY_UNUSED static uint32_t dma_rpc_stream_get_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s]: Bad touched stream id %d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s]: Bad touched stream %d", __func__, stream);

    return dma_rpc_stream_cntl[id][stream].dma_addr;
}

POSSIBLY_UNUSED static void dma_rpc_stream_touch(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    dma_vir_id = id;
    dma_vir_stream = stream;
}
#endif

#ifdef DMA_RPC_CLI
static void dma_rpccli_tx_handler(const void *data, unsigned int len)
{
    POSSIBLY_UNUSED struct DMA_RPC_REQ_MSG_T *msg = (struct DMA_RPC_REQ_MSG_T *)data;

    RPC_TRACE(1, "[RPCCLI_TX][HDLR]: [%d],param[0x%x/%d]",msg->rpc_id,(int)data, len);
}

static unsigned int dma_rpccli_rx_handler(const void *data, unsigned int len)
{
    struct DMA_MSG_HDR_T *hdr = (struct DMA_MSG_HDR_T *)data;

    if (len <= sizeof(*hdr)) {
        RPC_ERROR(1, "[RPCCLI_RX][HDLR]: len %d, is not equal hdr len %d", len, sizeof(*hdr));
        ASSERT(false, "%s: bad msg len=%d", __func__, len);
    }

    if (hdr->reply) {
        struct DMA_RPC_REPLY_MSG_T *msg = (struct DMA_RPC_REPLY_MSG_T *)data;

        if (len != sizeof(struct DMA_RPC_REPLY_MSG_T)) {
            RPC_ERROR(1, "[RPCCLI_RX][HDLR]: len %d, is not equal msg len %d",
                len, sizeof(struct DMA_RPC_REPLY_MSG_T));
        }
        ASSERT(len == sizeof(struct DMA_RPC_REPLY_MSG_T), "[%s]: error msg: data=%x,len=%d",
            __func__, (int)data, len);

        if (msg->hdr.id == dma_ack_msg.hdr.id) {
            if (msg->rpc_id == dma_ack_msg.rpc_id) {
                dma_ack_msg.ret = msg->ret;
                RPC_TRACE(1, "[RPCCLI_RX][HDLR]: [%d],RET=0x%x",msg->rpc_id,msg->ret);
                rpc_ack_done = true;
            }
        }
    } else {
        struct DMA_RPC_REQ_MSG_T *msg = (struct DMA_RPC_REQ_MSG_T *)data;
        ASSERT(len == sizeof(struct DMA_RPC_REQ_MSG_T), "[%s]: error req msg: data=%x,len=%d",
            __func__, (int)data, len);

        if (msg->rpc_id == DMA_RPC_CMD_STREAM_TRANSFER) {
            enum AUD_STREAM_ID_T id;
            enum AUD_STREAM_T stream;
            uint8_t dma_chan;
            uint32_t dma_addr;
            uint8_t lock;
            HAL_DMA_IRQ_HANDLER_T handler;

            id           = msg->param[0];
            stream       = msg->param[1];
            dma_chan     = msg->param[2];
            dma_addr     = msg->param[3];
            handler      = (HAL_DMA_IRQ_HANDLER_T)msg->param[4];

            RPC_TRACE(1, "[RPCCLI_RX][DMA_RPC_CMD_STREAM_TRANSFER]:handler=%x, param=%d/%d/%d/%x",
                (uint32_t)handler, id, stream, dma_chan, dma_addr);

            lock = int_lock();
            if (hdr->wait_sync_flag) {
                msg->param[0] = 0;
                hdr->sync_flag = 1;
            }
            int_unlock(lock);

#ifdef DMA_RPC_STREAM_API
            dma_rpc_stream_set_dma_addr(id, stream, dma_addr);
#else
            (void)id;
            (void)stream;
            (void)dma_addr;
#endif
            if (handler) {
                handler(dma_chan, 0, 0, NULL);
            }
        } else if (msg->rpc_id == DMA_RPC_CMD_TUNE_CLOCK) {
            float ratio = *(float *)&(msg->param[0]);
            if (dma_rpc_tune_clock_handler) {
                dma_rpc_tune_clock_handler(ratio);
            }
        }
    }
    return len;
}

uint32_t dma_rpc_get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t dma_addr = 0;

#ifdef DMA_RPC_STREAM_API
    if (dma_rpc_stream_get_forward_status(id, stream)) {
        dma_addr = dma_rpc_stream_get_dma_addr(id, stream);
    }
#endif

    return dma_addr;
}

#else /* DMA_RPC_SVR */
static void dma_rpcsvr_tx_handler(const void *data, unsigned int len)
{
    RPC_TRACE(1, "[RPCSVR_TX][HDLR]: param[0x%x/%d]", (int)data, len);
}

static unsigned int dma_rpcsvr_rx_handler(const void *data, unsigned int len)
{
    int ret = 0;
    struct DMA_MSG_HDR_T *hdr = (struct DMA_MSG_HDR_T *)data;
    uint32_t lock;

    if (len <= sizeof(*hdr)) {
        ASSERT(false, "%s: bad msg len=%d", __func__, len);
    }
    if (hdr->reply) {
        ASSERT(false, "%s: invalid msg", __func__);
    } else {
        if (hdr->id == DMA_MSG_ID_RPC) {
            struct DMA_RPC_REQ_MSG_T *msg = (struct DMA_RPC_REQ_MSG_T *)data;
            struct DMA_MSG_HDR_T *hdr = &msg->hdr;

            ret = dma_rpcsvr_msg_handler(msg);

            lock = int_lock();

            if (!hdr->wait_sync_flag) {
                dma_ack_msg2.hdr.id    = msg->hdr.id;
                dma_ack_msg2.hdr.reply = 1;
                dma_ack_msg2.rpc_id    = msg->rpc_id;
                dma_ack_msg2.ret       = ret;
            } else {
                msg->param[0] = ret;
                hdr->sync_flag = 1;
            }

            int_unlock(lock);

            if (!hdr->wait_sync_flag) {
                ret = dma_rpcif_send((uint8_t *)&dma_ack_msg2, sizeof(dma_ack_msg2));
                if (ret) {
                    RPC_WARN(1, "[RPCSVR_RX][WARNING]:send ack failed %d", ret);
#if (DMA_RPC_ERR_TAG_NUM > 0)
                       ret = dma_rpc_err_add(&dma_ack_msg2.hdr, dma_ack_msg2.rpc_id, ret);
#endif
                }
            }
        }
    }
    RPC_TRACE(1, "[RPCSVR_RX][HDLR]: param[0x%x/%d]", (int)data, len);
    return len;
}
#endif

/* DMA RPC CLIENT */
#ifdef DMA_RPC_CLI
static int dma_rpc_prepare_ack(uint16_t id, uint16_t rpc_id)
{
    dma_ack_msg.hdr.id = id;
    dma_ack_msg.rpc_id = rpc_id;
    rpc_ack_done = false;

    return 0;
}

static int dma_rpc_wait_ack(struct DMA_RPC_REQ_MSG_T *msg)
{
#define RPC_WAIT_DLY  (5)
#define RPC_WAIT_TIME (2000*100)
#define RPC_WAIT_CNT  (RPC_WAIT_TIME / RPC_WAIT_DLY)

    static volatile int wait_cnt = RPC_WAIT_CNT;
    int err =  0;
    uint16_t hdr_id = msg->hdr.id;
    uint16_t rpc_id = msg->rpc_id;
    uint16_t wait_sync_flag = msg->hdr.wait_sync_flag;
    volatile uint16_t sync_flag = 0;

    wait_cnt = RPC_WAIT_CNT;
    if (!wait_sync_flag) {
        while (!rpc_ack_done) {
            if (wait_cnt <= 0) {
                err = 1;
                goto _exit;
            }
            hal_sys_timer_delay(US_TO_TICKS(RPC_WAIT_DLY));
            wait_cnt--;
        }
    } else {
        do {
            hal_sys_timer_delay(US_TO_TICKS(RPC_WAIT_DLY));
            if (wait_cnt > 0) {
                wait_cnt--;
            } else {
                err = 2;
                goto _exit;
            }
            sync_flag = msg->hdr.sync_flag;
        } while (sync_flag == 0);
        if (hdr_id != dma_ack_msg.hdr.id) {
            err = 3;
        }
        if (rpc_id != dma_ack_msg.rpc_id) {
            err = 4;
        }
        if (!err) {
            dma_ack_msg.ret = msg->param[0];
            rpc_ack_done = true;
        }
    }

_exit:
    if (err) {
        RPC_WARN(1, "%s: wait ack timeout, err=%d", __func__, err);
    }
    return err;
}

uint32_t dma_rpc_send_common(uint16_t rpc_id, uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3)
{
    uint32_t idx = 0;
    int r = 0, rpcif_ret = 0;
    POSSIBLY_UNUSED const char *cs = get_cmd_string_name(rpc_id);
    uint32_t lock;

    RPC_INFO(1, "[RPCCLI_TX][CMD]: %s[%d],param[%x/%x/%x/%x]", cs,rpc_id,p0,p1,p2,p3);
#if (DMA_RPC_ERR_TAG_NUM > 0)
    dma_rpc_err_print(DMA_RPC_ERR_TAG_NUM);
#endif
    if (dma_rpc_pre_cmd_handler) {
        dma_rpc_pre_cmd_handler(rpc_id, p0, p1, p2, p3);
    }
    lock = int_lock();
    dma_req_msg.hdr.id       = DMA_MSG_ID_RPC;
    dma_req_msg.hdr.reply    = 0;
    dma_req_msg.hdr.sync_flag = 0;
#ifdef DMA_RPC_SYNC_FLAG_ENABLE
    dma_req_msg.hdr.wait_sync_flag = 1;
#else
    dma_req_msg.hdr.wait_sync_flag = 0;
#endif
    dma_req_msg.rpc_id       = rpc_id;
    dma_req_msg.param[idx++] = p0;
    dma_req_msg.param[idx++] = p1;
    dma_req_msg.param[idx++] = p2;
    dma_req_msg.param[idx++] = p3;

    dma_rpc_prepare_ack(dma_req_msg.hdr.id, dma_req_msg.rpc_id);
    int_unlock(lock);

    rpcif_ret = dma_rpcif_send((uint8_t *)&dma_req_msg, sizeof(dma_req_msg));
    ASSERT(rpcif_ret==0, "[%s]: send cmd failed, rpcif_ret=%d", __func__, rpcif_ret);

    r = dma_rpc_wait_ack(&dma_req_msg);
    ASSERT(r==0, "%s: r=%d", __func__, r);
    r = dma_ack_msg.ret;

    if (rpcif_ret) {
        if (dma_rpc_err_cmd_handler) {
            dma_rpc_err_cmd_handler(rpc_id, rpcif_ret);
        }
#if (DMA_RPC_ERR_TAG_NUM > 0)
        dma_rpc_err_add(&dma_req_msg.hdr, rpc_id, rpcif_ret);
#endif
        RPC_INFO(1, "[RPCCLI_TX][CMD]: %s[%d],rpcif err: %d", cs,rpc_id,rpcif_ret);
    }
    if (dma_rpc_post_cmd_handler) {
        dma_rpc_post_cmd_handler(dma_ack_msg.rpc_id, dma_ack_msg.ret);
    }
    RPC_INFO(1, "[RPCCLI_TX][CMD]: %s[%d],RET=0x%x", cs,rpc_id,r);
    return r;
}

static uint16_t rpc_id_remap(uint16_t rpc_id)
{
    uint16_t new_rpc_id = rpc_id;

#ifdef DMA_RPC_STREAM_API
    bool forward = dma_rpc_stream_get_forward_status(dma_vir_id, dma_vir_stream);
    if (forward) {
        new_rpc_id = VIRTUAL_RPC_ID(rpc_id);
    }
#endif
    return new_rpc_id;
}

enum HAL_DMA_RET_T dma_rpc_sg_2d_start(const struct HAL_DMA_DESC_T *desc,
                                       const struct HAL_DMA_CH_CFG_T *cfg,
                                       const struct HAL_DMA_2D_CFG_T *src_2d,
                                       const struct HAL_DMA_2D_CFG_T *dst_2d)
{
    return (enum HAL_DMA_RET_T)dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_SG_2D_START),
        (uint32_t)desc, (uint32_t)cfg, (uint32_t)src_2d, (uint32_t)dst_2d);
}

enum HAL_DMA_RET_T dma_rpc_sg_start(const struct HAL_DMA_DESC_T *desc, const struct HAL_DMA_CH_CFG_T *cfg)
{
    return (enum HAL_DMA_RET_T)dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_SG_START),
        (uint32_t)desc, (uint32_t)cfg, 0, 0);
}

enum HAL_DMA_RET_T dma_rpc_init_desc(struct HAL_DMA_DESC_T *desc,
                                     const struct HAL_DMA_CH_CFG_T *cfg,
                                     const struct HAL_DMA_DESC_T *next,
                                     int tc_irq)
{
    return (enum HAL_DMA_RET_T)dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_INIT_DESC),
        (uint32_t)desc, (uint32_t)cfg, (uint32_t)next, (uint32_t)tc_irq);
}

uint8_t dma_rpc_get_chan(enum HAL_DMA_PERIPH_T periph, enum HAL_DMA_GET_CHAN_T policy)
{
    return (uint8_t)dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_GET_CHAN),
        (uint32_t)periph, (uint32_t)policy, 0, 0);
}

uint32_t dma_rpc_get_base_addr(uint8_t ch)
{
    return (uint32_t)dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_GET_BASE_ADDR),
        (uint32_t)ch, 0, 0, 0);
}

void dma_rpc_stop(uint8_t ch)
{
    dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_STOP), (uint32_t)ch, 0, 0, 0);
}

void dma_rpc_free_chan(uint8_t ch)
{
    dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_FREE_CHAN), (uint32_t)ch, 0, 0, 0);
}

void dma_rpc_tc_irq_enable(uint8_t ch)
{
    dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_TC_IRQ_ENABLE), (uint32_t)ch, 0, 0, 0);
}

void dma_rpc_tc_irq_disable(uint8_t ch)
{
    dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_DMA_TC_IRQ_DISABLE), (uint32_t)ch, 0, 0, 0);
}

void dma_rpc_sync_data(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *param, uint32_t len)
{
    dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_SYNC_DATA), id, stream, (uint32_t)param, len);
}

uint32_t dma_rpc_stream_fadein(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, uint32_t ms)
{
    return dma_rpc_send_common(DMA_RPC_CMD_STREAM_FADEIN, id, stream, ms, 0);
}

uint32_t dma_rpc_stream_fadeout(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, uint32_t ms)
{
    return dma_rpc_send_common(DMA_RPC_CMD_STREAM_FADEOUT, id, stream, ms, 0);
}

uint32_t dma_rpc_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg)
{
#ifdef DMA_RPC_STREAM_API
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_OPEN), id, stream, (uint32_t)cfg, 0);
#else
    return 0;
#endif
}

uint32_t dma_rpc_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_CLOSE), id, stream, 0, 0);
#else
    return 0;
#endif
}

uint32_t dma_rpc_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_START), id, stream, 0, 0);
#else
    return 0;
#endif
}

uint32_t dma_rpc_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_STOP), id, stream, 0, 0);
#else
    return 0;
#endif
}

uint32_t dma_rpc_get_stream_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg)
{
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_GET_CFG), id, stream, (uint32_t)cfg, 0);
}

uint32_t dma_rpc_set_stream_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg)
{
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_SET_CFG), id, stream, (uint32_t)cfg, 0);
}

uint32_t dma_rpc_get_algo_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg)
{
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_GET_ALGO_CFG), id, stream, (uint32_t)cfg, 0);
}

uint32_t dma_rpc_set_algo_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, void *cfg, uint32_t len)
{
    return dma_rpc_send_common(rpc_id_remap(DMA_RPC_CMD_STREAM_SET_ALGO_CFG), id, stream, (uint32_t)cfg, len);
}

void dma_rpc_setup_pre_cmd_handler(DMA_RPC_PRE_CMD_HANDLER_T handler)
{
    dma_rpc_pre_cmd_handler = handler;
}

void dma_rpc_setup_post_cmd_handler(DMA_RPC_POST_CMD_HANDLER_T handler)
{
    dma_rpc_post_cmd_handler = handler;
}

void dma_rpc_setup_error_cmd_handler(DMA_RPC_ERR_CMD_HANDLER_T handler)
{
    dma_rpc_err_cmd_handler = handler;
}

void dma_rpc_setup_tune_clock_handler(DMA_RPC_TUNE_CLOCK_HANDLER_T handler)
{
    dma_rpc_tune_clock_handler = handler;
}

#else /* DMA_RPC_SVR */
static void dma_hw_irq_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error,
    struct HAL_DMA_DESC_T *lli)
{
    int i;

    //RPC_TRACE(1, "[dma_hw_irq]: chan=%d,remain_tsize=%d,error=%d",chan,remain_tsize,error);
    if (!dma_sync_data_done) {
        RPC_TRACE(1, "[dma_hw_irq] warning: Not sync dma data yet!");
        return;
    }
    for (i = 0; i < DMA_RPC_IRQ_HDLR_QTY; i++) {
        if (dma_irq_hdlr[i]) {
            dma_irq_hdlr[i](chan, remain_tsize, error, lli);
        }
    }
}

static void dma_override_dma_irq_handler(struct HAL_DMA_CH_CFG_T *cfg)
{
    if ((uint32_t)(cfg->handler) != (uint32_t)dma_hw_irq_handler) {
        RPC_TRACE(1, "override dma irq handler: %x -> %x", (int)(cfg->handler), (int)dma_hw_irq_handler);
        ptr_dma_irq_handler = cfg->handler;
        cfg->handler = dma_hw_irq_handler;
    }
}

static void dma_send_event(enum DMA_RPC_EVT_T evt, void *param)
{
    if (dma_evt_handler) {
        dma_evt_handler(evt, param);
    }
}

enum DMA_CHAN_STATUS_T {
    DMA_CHAN_STATUS_NULL = 0,
    DMA_CHAN_STATUS_OPEN_CLOSE  = (1 << 0),
    DMA_CHAN_STATUS_ALLOC_FREE  = (1 << 1),
    DMA_CHAN_STATUS_DESC_INIT   = (1 << 2),
    DMA_CHAN_STATUS_START_STOP  = (1 << 3),
    DMA_CHAN_STATUS_TC_IRQ_ENABLE_DISABLE  = (1 << 4),

};

struct DMA_CHAN_T {
    uint8_t status;
    uint8_t hw_chan;
    uint8_t vir_chan;
    uint8_t semaphore_alloc;
    uint8_t semaphore_start;
    uint8_t semaphore_desc_init;
    enum HAL_DMA_PERIPH_T periph;
};

#define DMA_CHAN_LIST_SIZE 3
static struct DMA_CHAN_T dma_chan_list[DMA_CHAN_LIST_SIZE];
static uint8_t dma_chan_seed = DMA_VIR_CHAN_ID_INI_VALUE;

static int dma_rpc_dma_chan_open(void)
{
    uint32_t i;
    struct DMA_CHAN_T *list = dma_chan_list;
    uint32_t num = ARRAY_SIZE(dma_chan_list);

    RPC_TRACE(1, "[%s]: list=%x, num=%d" ,__func__, (int)list, num);

    for (i = 0; i < num; i++) {
        if (list[i].status == DMA_CHAN_STATUS_NULL) {
            list[i].status = DMA_CHAN_STATUS_OPEN_CLOSE;
            list[i].hw_chan = HAL_DMA_CHAN_NONE;
            list[i].vir_chan = HAL_DMA_CHAN_NONE;
            list[i].semaphore_alloc = 0;
            list[i].semaphore_start = 0;
            list[i].semaphore_desc_init = 0;
            list[i].periph = HAL_DMA_PERIPH_NULL;
            RPC_TRACE(1, "init list[%d] done", i);
        }
    }
    dma_chan_seed = DMA_VIR_CHAN_ID_INI_VALUE;
    return 0;
}

static int dma_rpc_dma_chan_close(void)
{
    uint32_t i;
    struct DMA_CHAN_T *list = dma_chan_list;
    uint32_t num = ARRAY_SIZE(dma_chan_list);

    RPC_TRACE(1, "[%s]: list=%x, num=%d" ,__func__, (int)list, num);

    for (i = 0; i < num; i++) {
        list[i].status = DMA_CHAN_STATUS_NULL;
        list[i].hw_chan = HAL_DMA_CHAN_NONE;
        list[i].vir_chan = HAL_DMA_CHAN_NONE;
        list[i].semaphore_alloc = 0;
        list[i].semaphore_start = 0;
        list[i].semaphore_desc_init = 0;
        list[i].periph = HAL_DMA_PERIPH_NULL;
        RPC_TRACE(1, "deinit list[%d] done", i);
    }
    return 0;
}

struct DMA_CHAN_T *dma_rpc_dma_chan_get_avail_by_periph(enum HAL_DMA_PERIPH_T periph)
{
    uint32_t i;
    struct DMA_CHAN_T *list = dma_chan_list;
    uint32_t num = ARRAY_SIZE(dma_chan_list);

    for (i = 0; i < num; i++) {
        if (list[i].periph == periph) {
            return &list[i];
        }
    }
    return NULL;
}

struct DMA_CHAN_T *dma_rpc_dma_chan_get_avail_by_chan(uint8_t chan)
{
    uint32_t i;
    struct DMA_CHAN_T *list = dma_chan_list;
    uint32_t num = ARRAY_SIZE(dma_chan_list);

    for (i = 0; i < num; i++) {
        if ((chan == list[i].hw_chan) || (chan == list[i].vir_chan)) {
            return &list[i];
        }
    }
    return NULL;
}

struct DMA_CHAN_T *dma_rpc_dma_chan_get_empty(void)
{
    uint32_t i;
    struct DMA_CHAN_T *list = dma_chan_list;
    uint32_t num = ARRAY_SIZE(dma_chan_list);

    for (i = 0; i < num; i++) {
        if (list[i].periph == HAL_DMA_PERIPH_NULL) {
            return &list[i];
        }
    }
    return NULL;
}

#define DMA_VIRTUAL_CHAN(chan) ((1 << 7) | chan)
#define DMA_HW_CHAN(chan)      ((~(1 << 7)) & chan)
static uint8_t dma_rpc_dma_chan_virtual_chan(uint8_t hw_chan)
{
    uint8_t vir_chan = DMA_VIRTUAL_CHAN(hw_chan);

    return vir_chan;
}

static uint8_t dma_rpc_dma_chan_alloc(enum HAL_DMA_PERIPH_T periph, enum HAL_DMA_GET_CHAN_T policy)
{
    struct DMA_CHAN_T *p;
    uint8_t chan = HAL_DMA_CHAN_NONE;

    p = dma_rpc_dma_chan_get_avail_by_periph(periph);
    if (p == NULL) {
        // if get nothing from list, then alloc it, record it;
        chan = hal_audma_get_chan(periph, policy);
        ASSERT(chan != HAL_DMA_CHAN_NONE, "[%s]: get chan failed", __func__);
        p = dma_rpc_dma_chan_get_empty();
        ASSERT(p != NULL, "[%s]: get empty dma chan failed", __func__);
        p->periph = periph;
        p->hw_chan = chan;
        if (p->semaphore_alloc == 0) {
            p->status |= DMA_CHAN_STATUS_ALLOC_FREE;
        }
        p->semaphore_alloc++;

        RPC_TRACE(1, "[%s]: alloc hw chan=%d, periph=%d", __func__, chan, periph);
    } else {
        // if channel has been allocated, use random number as ch;
        if (p->vir_chan == HAL_DMA_CHAN_NONE) {
            chan = dma_rpc_dma_chan_virtual_chan(p->hw_chan);
            p->periph = periph;
            p->vir_chan = chan;
            if (p->semaphore_alloc == 0) {
                p->status |= DMA_CHAN_STATUS_ALLOC_FREE;
            }
            p->semaphore_alloc++;
        } else {
            ASSERT(false, "[%s]: alloc virtual dma chan failed", __func__);
        }
        RPC_TRACE(1, "[%s]: alloc vir chan=%d, periph=%d", __func__, chan, periph);
    }
    return chan;
}

static int dma_rpc_dma_chan_free(uint8_t chan)
{
    int ret = HAL_DMA_OK;
    struct DMA_CHAN_T *p;

    RPC_TRACE(1, "[%s]: chan=%d" ,__func__, chan);

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    if (p->status & DMA_CHAN_STATUS_ALLOC_FREE) {
        if (p->vir_chan == chan) {
            p->vir_chan = HAL_DMA_CHAN_NONE;
        }  else {
            p->hw_chan = HAL_DMA_CHAN_NONE;
        }
        if (p->semaphore_alloc > 0) {
            p->semaphore_alloc--;
        }
        if (p->semaphore_alloc == 0) {
            hal_dma_free_chan(chan);
            p->periph = HAL_DMA_PERIPH_NULL;
            p->status &= ~DMA_CHAN_STATUS_ALLOC_FREE;
            RPC_TRACE(1, "[%s]: free chan(%d) done", __func__, chan);
        }
    }

    return ret;
}

static int dma_rpc_dma_chan_init_desc(struct HAL_DMA_DESC_T *desc,
    const struct HAL_DMA_CH_CFG_T *cfg,
    const struct HAL_DMA_DESC_T *next,
    int tc_irq)
{
    int ret = HAL_DMA_OK;
    uint8_t chan = cfg->ch;
    struct DMA_CHAN_T *p;

    RPC_TRACE(1, "[%s]: chan=%d", __func__, chan);

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    if (p->hw_chan == chan) {
        ret = hal_dma_init_desc(desc, cfg, next, tc_irq);
        p->status |= DMA_CHAN_STATUS_DESC_INIT;
        p->semaphore_desc_init = 1;
        RPC_TRACE(1, "[%s]: init desc for hw chan=%d", __func__, chan);
    } else {
        p->semaphore_desc_init = 1;
        RPC_TRACE(1, "[%s]: desc init for vir chan=%d", __func__, chan);
    }
    return ret;
}

static int dma_rpc_dma_chan_deinit_desc(uint8_t chan)
{
    int ret = HAL_DMA_OK;
    struct DMA_CHAN_T *p;

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    if (p->semaphore_desc_init > 0) {
        p->semaphore_desc_init--;
    }
    if (p->semaphore_desc_init == 0) {
        p->status &= ~DMA_CHAN_STATUS_DESC_INIT;
    }
    RPC_TRACE(1, "[%s]: chan=%d", __func__, chan);
    return ret;
}

static int dma_rpc_dma_tc_irq_enable(uint8_t chan)
{
    int ret = HAL_DMA_OK;
    struct DMA_CHAN_T *p;

    RPC_TRACE(1, "[%s]: chan=%d" ,__func__, chan);

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    if (!(p->status & DMA_CHAN_STATUS_TC_IRQ_ENABLE_DISABLE)) {
        if (p->vir_chan == chan) {
            RPC_TRACE(1, "[%s]: enable tc irq for vir chan %d", __func__, chan);
        }  else {
            RPC_TRACE(1, "[%s]: enable tc irq for hw chan %d", __func__, chan);
        }
        hal_dma_tc_irq_enable(p->hw_chan);
        p->status |= DMA_CHAN_STATUS_TC_IRQ_ENABLE_DISABLE;
    }
    return ret;
}

static int dma_rpc_dma_tc_irq_disable(uint8_t chan)
{
    int ret = HAL_DMA_OK;
    struct DMA_CHAN_T *p;

    RPC_TRACE(1, "[%s]: chan=%d" ,__func__, chan);

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    if (p->status & DMA_CHAN_STATUS_TC_IRQ_ENABLE_DISABLE) {
        if (p->vir_chan == chan) {
            RPC_TRACE(1, "[%s]: disable tc irq for vir chan %d", __func__, chan);
        }  else {
            RPC_TRACE(1, "[%s]: disable tc irq for hw chan %d", __func__, chan);
        }
        hal_dma_tc_irq_disable(p->hw_chan);
        p->status &= ~DMA_CHAN_STATUS_TC_IRQ_ENABLE_DISABLE;
    }
    return ret;
}

static int dma_rpc_dma_chan_start(const struct HAL_DMA_DESC_T *desc,
    const struct HAL_DMA_CH_CFG_T *cfg,
    const struct HAL_DMA_2D_CFG_T *src_2d,
    const struct HAL_DMA_2D_CFG_T *dst_2d, bool flag_2d)
{
    int ret = HAL_DMA_OK;
    uint8_t chan = cfg->ch;
    struct DMA_CHAN_T *p;

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr, ch=%d", __func__, chan);
    if (chan == p->hw_chan) {
        if (p->semaphore_start == 0) {
            if (flag_2d) {
                ret = hal_dma_sg_start(desc, cfg);
            } else {
                ret = hal_dma_sg_2d_start(desc, cfg, src_2d, dst_2d);
            }
            p->status |= DMA_CHAN_STATUS_START_STOP;
            RPC_TRACE(1, "[%s]: start dma for hw chan %d", __func__, chan);
        }
        p->semaphore_start++;
    } else {
        p->semaphore_start++;
        RPC_TRACE(1, "[%s]: start dma for vir chan %d", __func__, chan);
    }
    return ret;
}

static int dma_rpc_dma_chan_stop(uint8_t chan)
{
    int ret = HAL_DMA_OK;
    struct DMA_CHAN_T *p;

    RPC_TRACE(1, "[%s]: chan=%d", __func__, chan);

    p = dma_rpc_dma_chan_get_avail_by_chan(chan);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    if (p->status & DMA_CHAN_STATUS_START_STOP) {
        if (p->semaphore_start > 0) {
            p->semaphore_start--;
        }
        if (p->semaphore_start == 0) {
            hal_dma_stop(chan);
            p->status &= ~DMA_CHAN_STATUS_START_STOP;
            RPC_TRACE(1, "[%s]: stop dma for hw chan %d", __func__, chan);
        }
    }
    ret = dma_rpc_dma_chan_deinit_desc(chan);
    return ret;
}

uint8_t dma_rpc_get_dma_hw_chan(enum HAL_DMA_PERIPH_T periph)
{
    uint8_t chan = HAL_DMA_CHAN_NONE;
    struct DMA_CHAN_T *p;

    p = dma_rpc_dma_chan_get_avail_by_periph(periph);
    ASSERT(p != NULL, "[%s]: null ptr", __func__);
    chan = p->hw_chan;
    RPC_TRACE(1, "[%s]: periph=%d, chan=%d", __func__, periph, chan);
    return chan;
}

void dma_rpc_stream_data_transfer(struct DMA_RPC_STREAM_PARAM_T *p)
{
    uint32_t lock = 0;
    uint32_t idx = 0;
    int rpcif_ret = 0;

    lock = int_lock();
    dma_req_msg2.hdr.id        = DMA_MSG_ID_RPC;
    dma_req_msg2.hdr.reply     = 0;
    dma_req_msg2.hdr.sync_flag = 0;
#ifdef DMA_RPC_SYNC_FLAG_ENABLE
    dma_req_msg2.hdr.wait_sync_flag = 1;
#else
    dma_req_msg2.hdr.wait_sync_flag = 0;
#endif
    dma_req_msg2.rpc_id       = DMA_RPC_CMD_STREAM_TRANSFER;
    dma_req_msg2.param[idx++] = p->id;
    dma_req_msg2.param[idx++] = p->stream;
    dma_req_msg2.param[idx++] = p->dma_chan;
    dma_req_msg2.param[idx++] = p->dma_addr;
    dma_req_msg2.param[idx++] = (uint32_t)ptr_dma_irq_handler;
    int_unlock(lock);

    rpcif_ret = dma_rpcif_send((uint8_t *)&dma_req_msg2, sizeof(dma_req_msg2));
    ASSERT(rpcif_ret == 0, "[%s]: send error, ret=%d", __func__, rpcif_ret);
}

void dma_rpc_tune_clock(float ratio)
{
    uint32_t lock = 0;
    uint32_t idx = 0;
    int rpcif_ret = 0;

    lock = int_lock();
    dma_req_msg2.hdr.id        = DMA_MSG_ID_RPC;
    dma_req_msg2.hdr.reply     = 0;
    dma_req_msg2.hdr.sync_flag = 0;
#ifdef DMA_RPC_SYNC_FLAG_ENABLE
    dma_req_msg2.hdr.wait_sync_flag = 1;
#else
    dma_req_msg2.hdr.wait_sync_flag = 0;
#endif
    dma_req_msg2.rpc_id       = DMA_RPC_CMD_TUNE_CLOCK;
    dma_req_msg2.param[idx++] = *((uint32_t *)&ratio);
    int_unlock(lock);

    rpcif_ret = dma_rpcif_send((uint8_t *)&dma_req_msg2, sizeof(dma_req_msg2));
    ASSERT(rpcif_ret == 0, "[%s]: send error, ret=%d", __func__, rpcif_ret);
}

static int dma_rpcsvr_msg_handler(struct DMA_RPC_REQ_MSG_T *msg)
{
    int ret = HAL_DMA_OK;
    uint16_t rpc_id = RPC_ID_VAL_ATTR(msg->rpc_id);
    POSSIBLY_UNUSED const char *cs = get_cmd_string_name(rpc_id);

    RPC_TRACE(1, "[RPCSVR][MSG]: %s[%d]", cs, rpc_id);

    if (msg->hdr.id == DMA_MSG_ID_RPC) {
        uint32_t *param = msg->param;

        switch (rpc_id) {
        case DMA_RPC_CMD_DMA_SG_START:
        {
            const struct HAL_DMA_DESC_T *desc = (const struct HAL_DMA_DESC_T *)param[0];
            const struct HAL_DMA_CH_CFG_T *cfg = (const struct HAL_DMA_CH_CFG_T *)param[1];

            dma_override_dma_irq_handler((struct HAL_DMA_CH_CFG_T *)cfg);
            ret = dma_rpc_dma_chan_start(desc, cfg, NULL, NULL, false);
            dma_send_event(DMA_RPC_EVT_DMA_START, (void *)msg);
        }
            break;
        case DMA_RPC_CMD_DMA_SG_2D_START:
        {
            const struct HAL_DMA_DESC_T *desc = (const struct HAL_DMA_DESC_T *)param[0];
            const struct HAL_DMA_CH_CFG_T *cfg = (const struct HAL_DMA_CH_CFG_T *)param[1];
            const struct HAL_DMA_2D_CFG_T *src_2d = (const struct HAL_DMA_2D_CFG_T *)param[2];
            const struct HAL_DMA_2D_CFG_T *dst_2d = (const struct HAL_DMA_2D_CFG_T *)param[3];

            dma_override_dma_irq_handler((struct HAL_DMA_CH_CFG_T *)cfg);
            ret = dma_rpc_dma_chan_start(desc, cfg, src_2d, dst_2d, true);
            dma_send_event(DMA_RPC_EVT_DMA_START, (void *)msg);
        }
            break;
        case DMA_RPC_CMD_DMA_INIT_DESC:
        {
            struct HAL_DMA_DESC_T *desc = (struct HAL_DMA_DESC_T *)param[0];
            const struct HAL_DMA_CH_CFG_T *cfg = (const struct HAL_DMA_CH_CFG_T *)param[1];
            const struct HAL_DMA_DESC_T *next = (const struct HAL_DMA_DESC_T *)param[2];
            int tc_irq = (int)param[3];

            dma_override_dma_irq_handler((struct HAL_DMA_CH_CFG_T *)cfg);
            ret = dma_rpc_dma_chan_init_desc(desc, cfg, next, tc_irq);
            dma_send_event(DMA_RPC_EVT_DMA_INIT, (void *)msg);
        }
            break;
        case DMA_RPC_CMD_DMA_GET_CHAN:
        {
            enum HAL_DMA_PERIPH_T periph = (enum HAL_DMA_PERIPH_T)param[0];
            enum HAL_DMA_PERIPH_T policy = (enum HAL_DMA_PERIPH_T)param[1];
            ret = dma_rpc_dma_chan_alloc(periph, policy);
            //msg->param[0] = ret;
            dma_send_event(DMA_RPC_EVT_DMA_GET_CHAN, (void *)msg);
        }
            break;
        case DMA_RPC_CMD_DMA_GET_BASE_ADDR:
        {
            uint8_t ch = (uint8_t)param[0];
            ret = hal_audma_get_base_addr(DMA_HW_CHAN(ch));
        }
            break;
        case DMA_RPC_CMD_DMA_STOP:
        {
            uint8_t ch = (uint8_t)param[0];
            ret = dma_rpc_dma_chan_stop(ch);
            dma_send_event(DMA_RPC_EVT_DMA_STOP, (void *)msg);
        }
            break;
        case DMA_RPC_CMD_DMA_FREE_CHAN:
        {
            uint8_t ch = (uint8_t)param[0];
            ret = dma_rpc_dma_chan_free(ch);
        }
            break;
        case DMA_RPC_CMD_DMA_TC_IRQ_ENABLE:
        {
            uint8_t ch = (uint8_t)param[0];
            ret = dma_rpc_dma_tc_irq_enable(ch);
        }
            break;
        case DMA_RPC_CMD_DMA_TC_IRQ_DISABLE:
        {
            uint8_t ch = (uint8_t)param[0];
            ret = dma_rpc_dma_tc_irq_disable(ch);
        }
            break;
        case DMA_RPC_CMD_SYNC_DATA:
            dma_sync_data_done = true;
            dma_send_event(DMA_RPC_EVT_SYNC_CTL_DATA, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_FADEIN:
            dma_send_event(DMA_RPC_EVT_STREAM_FADEIN, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_FADEOUT:
            dma_send_event(DMA_RPC_EVT_STREAM_FADEOUT, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_OPEN:
            dma_send_event(DMA_RPC_EVT_STREAM_OPEN, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_CLOSE:
            dma_send_event(DMA_RPC_EVT_STREAM_CLOSE, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_START:
            dma_send_event(DMA_RPC_EVT_STREAM_START, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_STOP:
            dma_send_event(DMA_RPC_EVT_STREAM_STOP, (void *)msg);
            break;
        case DMA_RPC_CMD_STREAM_GET_CFG:
            dma_send_event(DMA_RPC_EVT_STREAM_GET_CFG, (void *)msg);
            ret = msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1];
            break;
        case DMA_RPC_CMD_STREAM_SET_CFG:
            dma_send_event(DMA_RPC_EVT_STREAM_SET_CFG, (void *)msg);
            ret = msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1];
            break;
        case DMA_RPC_CMD_STREAM_GET_ALGO_CFG:
            dma_send_event(DMA_RPC_EVT_STREAM_GET_ALGO_CFG, (void *)msg);
            ret = msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1];
            break;
        case DMA_RPC_CMD_STREAM_SET_ALGO_CFG:
            dma_send_event(DMA_RPC_EVT_STREAM_SET_ALGO_CFG, (void *)msg);
            ret = msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1];
            break;
        default:
            break;
        }
    }
    RPC_TRACE(1, "[RPCSVR][MSG]: %s[%d],RET=0x%x", cs, rpc_id, ret);
    return ret;
}

void dma_rpc_setup_irq_handler(enum DMA_RPC_IRQ_HDLR_ID_T id, HAL_DMA_IRQ_HANDLER_T handler)
{
    dma_irq_hdlr[id] = handler;
}

void dma_rpc_setup_evt_handler(DMA_RPC_EVT_HANDLER_T handler)
{
    dma_evt_handler = handler;
}

#endif

void dma_rpcif_tx_handler(const void *data, unsigned int len)
{
    if (rpcif_tx_handler) {
        rpcif_tx_handler(data, len);
    }
}

unsigned int dma_rpcif_rx_handler(const void *data, unsigned int len)
{
    uint32_t ret = 0;

    if (rpcif_rx_handler) {
        ret = rpcif_rx_handler(data, len);
    }
    return ret;
}

bool dma_rpcif_enabled(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    dma_rpc_stream_touch(id, stream);
#endif

#ifdef DMA_RPC_CLI
    return rpccli_stream_map[id][stream];
#else
    return rpcsvr_stream_map[id][stream];
#endif
}

bool dma_rpcif_forward_enabled(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    return dma_rpc_stream_get_forward_status(id, stream);
#else
    return false;
#endif
}

void dma_rpcif_forward_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    dma_rpc_stream_set_forward_status(id, stream, true);
    dma_rpcif_open(id, stream);
#endif
}

void dma_rpcif_forward_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_STREAM_API
    dma_rpc_stream_set_forward_status(id, stream, false);
    dma_rpcif_close(id, stream);
#endif
}

void dma_rpcif_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef DMA_RPC_CLI
    rpccli_stream_map[id][stream] = true;
    if (!rpccli_opened) {
        rpcif_tx_handler = dma_rpccli_tx_handler;
        rpcif_rx_handler = dma_rpccli_rx_handler;
        rpccli_opened = true;
#if (DMA_RPC_ERR_TAG_NUM > 0)
        dma_rpc_err_list_init();
#endif
    }
    RPC_TRACE(1, "dma_req_msg=%x,size=%d", (int)&dma_req_msg,sizeof(dma_req_msg));
    RPC_TRACE(1, "dma_ack_msg=%x,size=%d", (int)&dma_ack_msg,sizeof(dma_ack_msg));
    RPC_INFO(1, "[DMA_RPCCLI][OPEN]: id=%d, stream=%d", id, stream);
#else
    rpcsvr_stream_map[id][stream] = true;
    if (!rpcsvr_opened) {
        rpcif_tx_handler = dma_rpcsvr_tx_handler;
        rpcif_rx_handler = dma_rpcsvr_rx_handler;
        rpcsvr_opened = true;
#if (DMA_RPC_ERR_TAG_NUM > 0)
        dma_rpc_err_list_init();
#endif
        dma_rpc_dma_chan_open();
    }
    RPC_TRACE(1, "dma_ack_msg2=%x,size=%d", (int)&dma_ack_msg2, sizeof(dma_ack_msg2));
    RPC_INFO(1, "[DMA_RPCSVR][OPEN]: id=%d, stream=%d", id, stream);
#endif
}

void dma_rpcif_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    bool opened = false;

#ifdef DMA_RPC_CLI
    rpccli_stream_map[id][stream] = false;
    for (uint8_t i = 0; i < AUD_STREAM_ID_NUM; i++) {
        for (uint8_t j = 0; j < AUD_STREAM_NUM; j++) {
            if (rpccli_stream_map[i][j] == true) {
                opened = true;
                break;
            }
        }
    }
    if (!opened) {
        rpcif_tx_handler = NULL;
        rpcif_rx_handler = NULL;
        rpccli_opened = false;
    }
    RPC_INFO(1, "[DMA_RPCCLI][CLOSE]: id=%d, stream=%d", id, stream);
#else
    rpcsvr_stream_map[id][stream] = false;
    for (uint8_t i = 0; i < AUD_STREAM_ID_NUM; i++) {
        for (uint8_t j = 0; j < AUD_STREAM_NUM; j++) {
            if (rpcsvr_stream_map[i][j] == true) {
                opened = true;
                break;
            }
        }
    }
    if (!opened) {
        rpcif_tx_handler = NULL;
        rpcif_rx_handler = NULL;
        rpcsvr_opened = false;
        dma_sync_data_done = false;
        dma_rpc_dma_chan_close();
    }
    RPC_INFO(1, "[DMA_RPCSVR][CLOSE]: id=%d, stream=%d", id, stream);
#endif
}

void dma_rpcif_setup_send_cmd_handler(DMA_RPC_SEND_CMD_HANDLER_T handler)
{
    rpcif_send_cmd_handler = handler;
}

#endif

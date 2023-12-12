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
#ifdef APP_MCPP_SRV
#include "cmsis.h"
#include "plat_types.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "app_utils.h"
#include "app_rpc_api.h"
#include "string.h"
#include "hal_sys2bth.h"
#include "hal_location.h"
#include "speech_memory.h"
#include "algo_process.h"
#include "app_mcpp_cfg.h"
#include "mcpp_server.h"
#include "mcpp_cmd.h"
#include "mcpp_core.h"

#define MCPP_SRV_MSG_CAPTURE_DATA_COME    (0x0)
#define MCPP_SRV_MSG_PLAYBACK_DATA_COME   (0x1)
#define MCPP_SRV_MSG_CAPTURE_OPEN         (0x2)
#define MCPP_SRV_MSG_CAPTURE_CLOSE        (0x3)
#define MCPP_SRV_MSG_PLAYBACK_OPEN        (0x4)
#define MCPP_SRV_MSG_PLAYBACK_CLOSE       (0x5)

#define PCM_FIFO_FRAME_NUM                (2)

#ifndef MCPP_SRV_SYSFREQ
#define MCPP_SRV_SYSFREQ                   APP_SYSFREQ_104M
#endif

typedef struct {
    bool                 opened_status;
    bool                 single_thread_cap_status;
    bool                 single_thread_play_status;
    APP_MCPP_CFG_T       mcpp_cfg;
    APP_MCPP_USER_T      user;
    APP_MCPP_CORE_T      core_client;
    capture_fifo_ctx_t  *cap_fifo;
    playback_fifo_ctx_t *play_fifo;
    app_mcpp_async_handler_t capture_callback;
    app_mcpp_async_handler_t playback_callback;
} APP_MCPP_CTX;

typedef struct {
    void   *cfg;
    uint16_t cfg_len;
} MCPP_SET_CFG_T;

typedef struct {
    void   *ptr;
    uint16_t ptr_len;
    int32_t ctl;
    bool    sync_flag;
} MCPP_ALGO_CTL_T;

typedef enum {
    MCPP_STREAM_CAPTURE,
    MCPP_STREAM_PLAYBACK,
} MCPP_STREAM_FLAG_T;

typedef struct {
    void   *ptr;
    uint16_t ptr_len;
    MCPP_STREAM_FLAG_T stream_flag;
} MCPP_RES_TER_T;

typedef struct {
    algo_process_cfg_t capture_cfg;
    algo_process_cfg_t playback_cfg;
    capture_fifo_ctx_t capture_fifo;
    playback_fifo_ctx_t playback_fifo;

    uint8_t *process_cap_pcm_buf;
    uint8_t *deinterlaved_cap_pcm_buf;
    uint8_t *process_ref_buf;
    uint8_t *process_ref2_buf;
    uint8_t *process_vpu_buf;
    uint8_t *process_play_pcm_buf;
    uint8_t *deinterlaved_play_pcm_buf;

    bool capture_enable;
    bool playback_enable;
    bool capture_opened;
    bool playback_opened;
    bool capture_close_flag;
    bool playback_close_flag;
    bool single_thread_cap_status;
    bool single_thread_play_status;

    MCPP_SET_CFG_T cap_cfg;
    MCPP_SET_CFG_T play_cfg;
    MCPP_ALGO_CTL_T cap_ctl;
    MCPP_ALGO_CTL_T play_ctl;
    MCPP_RES_TER_T cap_res_ret;
    MCPP_RES_TER_T play_res_ret;
    MCPP_RPC_T srv_rpc;
    const algo_process_stream_t *algo_capture_ptr;
    const algo_process_stream_t *algo_playback_ptr;

    int32_t  core_server;
    int32_t  core_client;
    APP_MCPP_USER_T user;
} mcpp_srv_ctx_t;

static uint32_t g_server_register_number = 0;
static mcpp_srv_ctx_t mcpp_srv_ctx[APP_MCPP_USER_QTY] = {0};
static uint8_t *pcm_process_heap_buf = NULL;

#ifndef ALGO_PROCESS_CALL_POOL_BUF_SIZE
#define ALGO_PROCESS_CALL_POOL_BUF_SIZE      (1024 * 50)
#endif

#ifndef ALGO_PROCESS_AUDIO_POOL_BUF_SIZE
#define ALGO_PROCESS_AUDIO_POOL_BUF_SIZE     (1024 * 30)
#endif

#ifndef MCPP_SERVER_RPC_BUF_SIZE
#define MCPP_SERVER_RPC_BUF_SIZE             (1024 * 50)
#endif

#ifndef ALGO_PROCESS_HEAP_BUF_SIZE
#define ALGO_PROCESS_HEAP_BUF_SIZE \
    MAX(ALGO_PROCESS_CALL_POOL_BUF_SIZE, ALGO_PROCESS_AUDIO_POOL_BUF_SIZE)
#endif

#ifdef PCM_PROCESS_HEAP_USE_STATIC_BUFF
#define PCM_PROCESS_STATIC_HEAP_BUF_SIZE     (ALGO_PROCESS_HEAP_BUF_SIZE + MCPP_SERVER_RPC_BUF_SIZE)
__attribute__((aligned(8)))
static uint8_t pcm_process_heap_buf_impl[PCM_PROCESS_STATIC_HEAP_BUF_SIZE] = {0};
#else
#include "heap_api.h"
#endif

static mcpp_srv_ctx_t *mcpp_srv_get_ctx(APP_MCPP_USER_T user)
{
    return &mcpp_srv_ctx[user];
}

static algo_process_cfg_t *mcpp_srv_get_capture_cfg(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    return &ctx->capture_cfg;
}

static algo_process_cfg_t *mcpp_srv_get_playback_cfg(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    return &ctx->playback_cfg;
}

static capture_fifo_ctx_t *mcpp_srv_get_capture_fifo_ctx(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    return &ctx->capture_fifo;
}

static playback_fifo_ctx_t *mcpp_srv_get_playback_fifo_ctx(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    return &ctx->playback_fifo;
}

static int32_t _nextpow2(int32_t N)
{
    int32_t i = 1;
    while (1) {
        i <<= 1;
        if (i >= N)
            return i;
    }
}

static bool POSSIBLY_UNUSED mcpp_srv_core_is_opened(void)
{
    for (int32_t user = 0; user < APP_MCPP_USER_QTY; user++) {
        mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
        if (ctx->capture_opened || ctx->playback_opened) {
            return true;
        }
    }
    return false;
}

int32_t mcpp_srv_received_cmd_handler(uint8_t *ptr, uint16_t len)
{
    MCPP_RPC_T *rpc_ptr = (MCPP_RPC_T *)ptr;
    MCPP_LOG_I("[%s]: The user is %d, cmd is %d", __func__, rpc_ptr->user, rpc_ptr->cmd);
    int32_t ret = 0;

    switch (rpc_ptr->cmd)
    {
    case MCPP_RPC_CMD_CAPTURE_OPEN:
        ASSERT(rpc_ptr->ptr_len == sizeof(APP_MCPP_CTX),\
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(APP_MCPP_CTX));
        mcpp_srv_stream_capture_open(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    case MCPP_RPC_CMD_PLAYBACK_OPEN:
        ASSERT(rpc_ptr->ptr_len == sizeof(APP_MCPP_CTX),\
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(APP_MCPP_CTX));
        mcpp_srv_stream_playback_open(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    case MCPP_RPC_CMD_CAPTURE_CLOSE:
        ASSERT(rpc_ptr->ptr_len == 0, "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, 0);
        mcpp_srv_stream_capture_close(rpc_ptr->user);
        ret = 1;
        break;
    case MCPP_RPC_CMD_PLAYBACK_CLOSE:
        ASSERT(rpc_ptr->ptr_len == 0, "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, 0);
        mcpp_srv_stream_playback_close(rpc_ptr->user);
        ret = 1;
        break;
    case MCPP_RPC_CMD_CAPTURE_SET_CFG:
        ASSERT(rpc_ptr->ptr_len == sizeof(MCPP_SET_CFG_T),
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(MCPP_SET_CFG_T));
        mcpp_srv_capture_set_algo_cfg(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    case MCPP_RPC_CMD_PLAYBACK_SET_CFG:
        ASSERT(rpc_ptr->ptr_len == sizeof(MCPP_SET_CFG_T),
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(MCPP_SET_CFG_T));
        mcpp_srv_playback_set_algo_cfg(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    case MCPP_RPC_CMD_CAPTURE_CTRL:
        ASSERT(rpc_ptr->ptr_len == sizeof(MCPP_ALGO_CTL_T),
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(MCPP_ALGO_CTL_T));
        mcpp_srv_capture_algo_ctl(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    case MCPP_RPC_CMD_PLAYBACK_CTRL:
        ASSERT(rpc_ptr->ptr_len == sizeof(MCPP_ALGO_CTL_T),
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(MCPP_ALGO_CTL_T));
        mcpp_srv_playback_algo_ctl(rpc_ptr->user, rpc_ptr->ptr, rpc_ptr->ptr_len);
        break;
    case MCPP_RPC_CMD_CAPTURE_PROCESS:
        ASSERT(rpc_ptr->ptr_len == sizeof(APP_MCPP_CAP_PCM_T),
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(APP_MCPP_CAP_PCM_T));
        mcpp_srv_capture_process(rpc_ptr->user, (APP_MCPP_CAP_PCM_T *)rpc_ptr->ptr);
        ret = 1;
        break;
    case MCPP_RPC_CMD_PLAYBACK_PROCESS:
        ASSERT(rpc_ptr->ptr_len == sizeof(APP_MCPP_PLAY_PCM_T),
            "[%s] len(%d) != %d", __func__, rpc_ptr->ptr_len, sizeof(APP_MCPP_PLAY_PCM_T));
        mcpp_srv_playback_process(rpc_ptr->user, (APP_MCPP_PLAY_PCM_T *)rpc_ptr->ptr);
        ret = 1;
        break;
    default:
        MCPP_LOG_I("[%s]:cmd:%d is wrong!", __func__, rpc_ptr->cmd);
        ret = -1;
        break;
    }

    return ret;
}

void mcpp_capture_ctl_cmd_send_handler_done(APP_MCPP_USER_T user, void *ptr, uint16_t ptr_len)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ctx->cap_res_ret.ptr = ptr;
    ctx->cap_res_ret.ptr_len = ptr_len;
    ctx->cap_res_ret.stream_flag = MCPP_STREAM_CAPTURE;

    ctx->srv_rpc.cmd = MCPP_RPC_CMD_RETURN_RESULT;
    ctx->srv_rpc.ptr = &ctx->cap_res_ret;
    ctx->srv_rpc.ptr_len = sizeof(MCPP_RES_TER_T);
    ctx->srv_rpc.user = ctx->user;
    ctx->srv_rpc.core_client = ctx->core_client;
    ctx->srv_rpc.core_server = ctx->core_server;
    mcpp_send_no_rsp_cmd((uint8_t *)&ctx->srv_rpc, sizeof(MCPP_RPC_T), ctx->core_client);
}

void mcpp_playback_ctl_cmd_send_handler_done(APP_MCPP_USER_T user, void *ptr, uint16_t ptr_len)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ctx->play_res_ret.ptr = ptr;
    ctx->play_res_ret.ptr_len = ptr_len;
    ctx->play_res_ret.stream_flag = MCPP_STREAM_PLAYBACK;

    ctx->srv_rpc.cmd = MCPP_RPC_CMD_RETURN_RESULT;
    ctx->srv_rpc.ptr = &ctx->play_res_ret;
    ctx->srv_rpc.ptr_len = sizeof(MCPP_RES_TER_T);
    ctx->srv_rpc.user = ctx->user;
    ctx->srv_rpc.core_client = ctx->core_client;
    ctx->srv_rpc.core_server = ctx->core_server;
    mcpp_send_no_rsp_cmd((uint8_t *)&ctx->srv_rpc, sizeof(MCPP_RPC_T), ctx->core_client);
}

static void pcm_fifo_cap_init(capture_fifo_ctx_t *ctx, algo_process_cfg_t *cfg)
{
    MCPP_LOG_D("[%s]", __func__);
    uint32_t pcm_fifo_len = _nextpow2(cfg->frame_len * cfg->mic_num * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);
    uint32_t out_fifo_len = _nextpow2(cfg->frame_len * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);
    ctx->pcm_fifo_mem = speech_calloc(1,pcm_fifo_len);
    ctx->out_fifo_mem = speech_calloc(1,out_fifo_len);

    // memset(ctx->pcm_fifo_mem, 0, pcm_fifo_len);
    kfifo_init(&ctx->pcm_fifo, ctx->pcm_fifo_mem, pcm_fifo_len);
    kfifo_init(&ctx->out_fifo, ctx->out_fifo_mem, out_fifo_len);

    uint32_t init_out_fifo_len = cfg->frame_len;
    if (cfg->delay) {
        ASSERT(cfg->delay < pcm_fifo_len, "[%s] delay(%d) is bigger than pcm_fifo_len", __func__, cfg->delay);
        ASSERT(cfg->delay < out_fifo_len, "[%s] delay(%d) is bigger than out_fifo_len", __func__, cfg->delay);
        init_out_fifo_len = cfg->delay;
    }
    MCPP_LOG_I("[%s] Pust %d pcm into out FIFO", __func__, init_out_fifo_len);
    kfifo_put(&ctx->out_fifo, (uint8_t *)ctx->pcm_fifo_mem, init_out_fifo_len * cfg->sample_bytes);

    if (cfg->channel_map & PCM_CHANNEL_MAP_CAP_REF) {
        uint32_t ref_fifo_len = _nextpow2(cfg->frame_len * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);
        ctx->ref_fifo_mem = speech_calloc(1,ref_fifo_len);
        kfifo_init(&ctx->ref_fifo, ctx->ref_fifo_mem, ref_fifo_len);
    }
    if (cfg->channel_map & PCM_CHANNEL_MAP_CAP_REF2) {
        uint32_t ref2_fifo_len = _nextpow2(cfg->frame_len * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);
        ctx->ref2_fifo_mem = speech_calloc(1,ref2_fifo_len);
        kfifo_init(&ctx->ref2_fifo, ctx->ref2_fifo_mem, ref2_fifo_len);
    }
    if (cfg->channel_map & PCM_CHANNEL_MAP_CAP_VPU) {
        uint32_t vpu_fifo_len = _nextpow2(cfg->frame_len * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);
        ctx->vpu_fifo_mem = speech_calloc(1,vpu_fifo_len);
        kfifo_init(&ctx->vpu_fifo, ctx->vpu_fifo_mem, vpu_fifo_len);
    }
}

static void pcm_fifo_cap_deinit(capture_fifo_ctx_t *ctx)
{
    MCPP_LOG_D("[%s]", __func__);
    if (ctx->vpu_fifo_mem) {
        speech_free(ctx->vpu_fifo_mem);
    }
    if (ctx->ref2_fifo_mem) {
        speech_free(ctx->ref2_fifo_mem);
    }
    if (ctx->ref_fifo_mem) {
        speech_free(ctx->ref_fifo_mem);
    }
    speech_free(ctx->out_fifo_mem);
    speech_free(ctx->pcm_fifo_mem);
}

static void pcm_fifo_play_init(playback_fifo_ctx_t *ctx, algo_process_cfg_t *cfg)
{
    MCPP_LOG_D("[%s]", __func__);
    uint32_t pcm_fifo_len = _nextpow2(cfg->frame_len * cfg->mic_num * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);
    uint32_t out_fifo_len = _nextpow2(cfg->frame_len * cfg->sample_bytes * PCM_FIFO_FRAME_NUM);

    ctx->pcm_fifo_mem = speech_calloc(1,pcm_fifo_len);
    ctx->out_fifo_mem = speech_calloc(1,out_fifo_len);

    kfifo_init(&ctx->pcm_fifo, ctx->pcm_fifo_mem, pcm_fifo_len);
    kfifo_init(&ctx->out_fifo, ctx->out_fifo_mem, out_fifo_len);

    uint32_t init_out_fifo_len = cfg->frame_len;
    if (cfg->delay) {
        ASSERT(cfg->delay < pcm_fifo_len, "[%s] delay(%d) is bigger than pcm_fifo_len", __func__, cfg->delay);
        ASSERT(cfg->delay < out_fifo_len, "[%s] delay(%d) is bigger than out_fifo_len", __func__, cfg->delay);
        init_out_fifo_len = cfg->delay;
    }
    MCPP_LOG_I("[%s] Pust %d pcm into out FIFO", __func__, init_out_fifo_len);
    kfifo_put(&ctx->out_fifo, (uint8_t *)ctx->pcm_fifo_mem, init_out_fifo_len * cfg->sample_bytes);
}

static void pcm_fifo_play_deinit(playback_fifo_ctx_t *ctx)
{
    MCPP_LOG_D("[%s]", __func__);
    speech_free(ctx->out_fifo_mem);
    speech_free(ctx->pcm_fifo_mem);
}

static void pcm_process_pre_init(APP_MCPP_USER_T user)
{
    MCPP_LOG_D("[%s]", __func__);
#if defined(APP_MCPP_CLI_M55)
    app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, MCPP_SRV_SYSFREQ);
#elif defined(APP_MCPP_CLI_SENS)
    hal_sysfreq_req(HAL_SYSFREQ_USER_DSP, HAL_CMU_FREQ_104M);
#endif

#ifdef PCM_PROCESS_HEAP_USE_STATIC_BUFF
    pcm_process_heap_buf = pcm_process_heap_buf_impl;
    speech_heap_init(pcm_process_heap_buf, PCM_PROCESS_STATIC_HEAP_BUF_SIZE);
#else
    uint32_t heap_buf_size = 0;
    heap_buf_size = get_algo_process_heap_buffer_size(user) + MCPP_SERVER_RPC_BUF_SIZE;
    off_bth_syspool_init(SYSPOOL_USER_SPEECH, NULL);
    off_bth_syspool_get_buff(&pcm_process_heap_buf, heap_buf_size);
    speech_heap_init(pcm_process_heap_buf, heap_buf_size);
#endif
}

static void pcm_process_cap_init(APP_MCPP_USER_T user, algo_process_cfg_t *cfg)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);

    if (g_server_register_number == 0 && ctx->core_client != ctx->core_server){
        pcm_process_pre_init(user);
    }

    ctx->algo_capture_ptr->open(cfg);

    pcm_fifo_cap_init(&ctx->capture_fifo, cfg);

    ctx->process_cap_pcm_buf = speech_calloc(cfg->frame_len * cfg->mic_num, cfg->sample_bytes);
    ctx->deinterlaved_cap_pcm_buf = speech_calloc(cfg->frame_len * cfg->mic_num, cfg->sample_bytes);
    if (!ctx->process_cap_pcm_buf || !ctx->deinterlaved_cap_pcm_buf) {
        ASSERT(false, "[%s]: speech_calloc buf is empty!", __func__);
    }
    if (cfg->channel_map & PCM_CHANNEL_MAP_CAP_REF) {
        ctx->process_ref_buf = speech_calloc(cfg->frame_len, cfg->sample_bytes);
    }
    if (cfg->channel_map & PCM_CHANNEL_MAP_CAP_REF2) {
        ctx->process_ref2_buf = speech_calloc(cfg->frame_len, cfg->sample_bytes);
    }
    if (cfg->channel_map & PCM_CHANNEL_MAP_CAP_VPU) {
        ctx->process_vpu_buf = speech_calloc(cfg->frame_len, cfg->sample_bytes);
    }

    g_server_register_number += 1;
}

static void pcm_process_play_init(APP_MCPP_USER_T user, algo_process_cfg_t *cfg)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);

    if (g_server_register_number == 0 && ctx->core_client != ctx->core_server){
        pcm_process_pre_init(user);
    }

    ctx->algo_playback_ptr->open(cfg);

    pcm_fifo_play_init(&ctx->playback_fifo, cfg);

    ctx->process_play_pcm_buf = speech_calloc(cfg->frame_len * cfg->mic_num, cfg->sample_bytes);
    ctx->deinterlaved_play_pcm_buf = speech_calloc(cfg->frame_len * cfg->mic_num, cfg->sample_bytes);
    if (!ctx->process_play_pcm_buf || !ctx->deinterlaved_play_pcm_buf) {
        ASSERT(false, "[%s]: speech_calloc buf is empty!", __func__);
    }

    g_server_register_number += 1;
}

static void pcm_process_post_deinit(void)
{
    MCPP_LOG_D("[%s]", __func__);
    uint32_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(4, "[%s] total=%d, used=%d, max_used=%d.", __func__, total, used, max_used);
    // ASSERT(used == 0, "[%s] used != 0", __func__);
#ifndef PCM_PROCESS_HEAP_USE_STATIC_BUFF
    off_bth_syspool_deinit(SYSPOOL_USER_SPEECH);
#endif

#if defined(APP_MCPP_CLI_M55)
    app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, APP_SYSFREQ_32K);
#elif defined(APP_MCPP_CLI_SENS)
    hal_sysfreq_req(HAL_SYSFREQ_USER_DSP, HAL_CMU_FREQ_32K);
#endif
}

static void pcm_process_cap_deinit(APP_MCPP_USER_T user)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);

    pcm_fifo_cap_deinit(&ctx->capture_fifo);

    if (ctx->process_vpu_buf) {
        speech_free(ctx->process_vpu_buf);
    }
    if (ctx->process_ref2_buf) {
        speech_free(ctx->process_ref2_buf);
    }
    if (ctx->process_ref_buf) {
        speech_free(ctx->process_ref_buf);
    }

    speech_free(ctx->deinterlaved_cap_pcm_buf);
    speech_free(ctx->process_cap_pcm_buf);

    ctx->algo_capture_ptr->close();

    g_server_register_number -= 1;

    if (g_server_register_number == 0 && ctx->core_client != ctx->core_server){
        pcm_process_post_deinit();
    }
}

static void pcm_process_play_deinit(APP_MCPP_USER_T user)
{
    MCPP_LOG_D("[%s]", __func__);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);

    pcm_fifo_play_deinit(&ctx->playback_fifo);

    speech_free(ctx->process_play_pcm_buf);
    speech_free(ctx->deinterlaved_play_pcm_buf);

    ctx->algo_playback_ptr->close();

    g_server_register_number -= 1;

    if (g_server_register_number == 0 && ctx->core_client != ctx->core_server){
        pcm_process_post_deinit();
    }
}

static bool pcm_capture_fifo_is_ready(APP_MCPP_USER_T user)
{
    capture_fifo_ctx_t *fifo_ctx = mcpp_srv_get_capture_fifo_ctx(user);
    algo_process_cfg_t *cfg = mcpp_srv_get_capture_cfg(user);
    if (kfifo_len(&fifo_ctx->pcm_fifo) >= cfg->frame_len * cfg->mic_num * cfg->sample_bytes &&
        kfifo_get_free_space(&fifo_ctx->out_fifo) >= cfg->frame_len * cfg->sample_bytes) {
        return true;
    } else {
        return false;
    }
}

static bool pcm_playback_fifo_is_ready(APP_MCPP_USER_T user)
{
    playback_fifo_ctx_t *fifo_ctx = mcpp_srv_get_playback_fifo_ctx(user);
    algo_process_cfg_t *cfg = mcpp_srv_get_playback_cfg(user);
    if (kfifo_len(&fifo_ctx->pcm_fifo) >= cfg->frame_len * cfg->mic_num * cfg->sample_bytes &&
        kfifo_get_free_space(&fifo_ctx->out_fifo) >= cfg->frame_len * cfg->sample_bytes) {
        return true;
    } else {
        return false;
    }
}

static void mcpp_srv_pcm_capture_set_cfg(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (ctx->cap_cfg.cfg == NULL) {
        return;
    }

    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    ctx->algo_capture_ptr->set_cfg(ctx->cap_cfg.cfg, ctx->cap_cfg.cfg_len);
    speech_free(ctx->cap_cfg.cfg);
    memset(&ctx->cap_cfg, 0, sizeof(MCPP_SET_CFG_T));
}

static void mcpp_srv_pcm_capture_ctl(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (ctx->cap_ctl.ptr == NULL) {
        return;
    }

    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    MCPP_ALGO_CTL_T *algo_ctl = &ctx->cap_ctl;
    ctx->algo_capture_ptr->ctl(algo_ctl->ctl, algo_ctl->ptr, algo_ctl->ptr_len);
    mcpp_capture_ctl_cmd_send_handler_done(user, algo_ctl->ptr, algo_ctl->ptr_len);
    memset(&ctx->cap_ctl, 0, sizeof(MCPP_ALGO_CTL_T));
}

static void mcpp_srv_pcm_capture_process(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    capture_fifo_ctx_t *capture = mcpp_srv_get_capture_fifo_ctx(user);
    algo_process_cfg_t *cfg = mcpp_srv_get_capture_cfg(user);

    mcpp_srv_pcm_capture_set_cfg(user);
    mcpp_srv_pcm_capture_ctl(user);

    if (kfifo_len(&capture->pcm_fifo) < cfg->frame_len * cfg->mic_num * cfg->sample_bytes) {
        return;
    }

    kfifo_get(&capture->pcm_fifo, ctx->process_cap_pcm_buf, cfg->frame_len * cfg->mic_num * cfg->sample_bytes);

    if (ctx->process_ref_buf) {
        kfifo_get(&capture->ref_fifo, ctx->process_ref_buf, cfg->frame_len * cfg->sample_bytes);
    }
    if (ctx->process_ref2_buf) {
        kfifo_get(&capture->ref2_fifo, ctx->process_ref2_buf, cfg->frame_len * cfg->sample_bytes);
    }
    if (ctx->process_vpu_buf) {
        kfifo_get(&capture->vpu_fifo, ctx->process_vpu_buf, cfg->frame_len * cfg->sample_bytes);
    }

    if (cfg->sample_bytes == sizeof(int16_t)) {
        int16_t *process_pcm_buf = (int16_t *)ctx->process_cap_pcm_buf;
        int16_t *deinterlaved_cap_pcm_buf = (int16_t *)ctx->deinterlaved_cap_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_cap_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    } else if (cfg->sample_bytes == sizeof(int32_t)) {
        int32_t *process_pcm_buf = (int32_t *)ctx->process_cap_pcm_buf;
        int32_t *deinterlaved_cap_pcm_buf = (int32_t *)ctx->deinterlaved_cap_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_cap_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    }

    uint8_t *pcm_buf[PCM_CHANNEL_INDEX_QTY] = {NULL};

    for (uint32_t i = 0; i < cfg->mic_num; i++) {
        pcm_buf[i] = &ctx->deinterlaved_cap_pcm_buf[i * cfg->frame_len * cfg->sample_bytes];
    }

    pcm_buf[PCM_CHANNEL_INDEX_REF] = ctx->process_ref_buf;
    pcm_buf[PCM_CHANNEL_INDEX_REF2] = ctx->process_ref2_buf;
    pcm_buf[PCM_CHANNEL_INDEX_VPU] = ctx->process_vpu_buf;

    ctx->algo_capture_ptr->process(pcm_buf, ctx->process_cap_pcm_buf, cfg->frame_len);

    kfifo_put(&capture->out_fifo, ctx->process_cap_pcm_buf, cfg->frame_len * cfg->sample_bytes);
}

static void mcpp_srv_pcm_playback_set_cfg(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (ctx->play_cfg.cfg == NULL) {
        return;
    }

    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    ctx->algo_playback_ptr->set_cfg(ctx->play_cfg.cfg, ctx->play_cfg.cfg_len);
    speech_free(ctx->play_cfg.cfg);
    memset(&ctx->play_cfg, 0, sizeof(MCPP_SET_CFG_T));
}

static void mcpp_srv_pcm_playback_ctl(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (ctx->play_ctl.ptr == NULL) {
        return;
    }

    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    MCPP_ALGO_CTL_T *algo_ctl = &ctx->play_ctl;
    ctx->algo_playback_ptr->ctl(algo_ctl->ctl, algo_ctl->ptr, algo_ctl->ptr_len);
    mcpp_playback_ctl_cmd_send_handler_done(user, algo_ctl->ptr, algo_ctl->ptr_len);
    memset(&ctx->play_ctl, 0, sizeof(MCPP_ALGO_CTL_T));

}

static void mcpp_srv_pcm_playback_process(APP_MCPP_USER_T user)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    playback_fifo_ctx_t *playback = mcpp_srv_get_playback_fifo_ctx(user);
    algo_process_cfg_t *cfg = mcpp_srv_get_playback_cfg(user);

    mcpp_srv_pcm_playback_set_cfg(user);
    mcpp_srv_pcm_playback_ctl(user);

    if (kfifo_len(&playback->pcm_fifo) < cfg->frame_len * cfg->mic_num * cfg->sample_bytes) {
        return;
    }

    kfifo_get(&playback->pcm_fifo, ctx->process_play_pcm_buf, cfg->frame_len * cfg->mic_num * cfg->sample_bytes);

    if (cfg->sample_bytes == sizeof(int16_t)) {
        int16_t *process_pcm_buf = (int16_t *)ctx->process_play_pcm_buf;
        int16_t *deinterlaved_play_pcm_buf = (int16_t *)ctx->deinterlaved_play_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_play_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    } else if (cfg->sample_bytes == sizeof(int32_t)) {
        int32_t *process_pcm_buf = (int32_t *)ctx->process_play_pcm_buf;
        int32_t *deinterlaved_play_pcm_buf = (int32_t *)ctx->deinterlaved_play_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_play_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    }

    uint8_t *pcm_buf[PCM_CHANNEL_INDEX_QTY] = {NULL};

    for (uint32_t i = 0; i < cfg->mic_num; i++) {
        pcm_buf[i] = &ctx->deinterlaved_play_pcm_buf[i * cfg->frame_len * cfg->sample_bytes];
    }

    ctx->algo_playback_ptr->process(pcm_buf, ctx->process_play_pcm_buf, cfg->frame_len);

    kfifo_put(&playback->out_fifo, ctx->process_play_pcm_buf, cfg->frame_len * cfg->sample_bytes);
}

static void mcpp_srv_process(void)
{
    for (int32_t user = 0; user < APP_MCPP_USER_QTY; user++) {
        mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
        if (ctx->capture_opened == true && pcm_capture_fifo_is_ready(user)){
            mcpp_srv_pcm_capture_process(user);
        }
        if (ctx->playback_opened == true && pcm_playback_fifo_is_ready(user)){
            mcpp_srv_pcm_playback_process(user);
        }
    }
}

static void mcpp_srv_thread_capture_close(void)
{
    for (int32_t user = 0; user < APP_MCPP_USER_QTY; user++) {
        mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
        if (ctx->capture_close_flag) {
            pcm_process_cap_deinit(user);
            ctx->capture_close_flag = false;
            if (ctx->core_server != ctx->core_client) {
                mcpp_cmd_send_empty_rsp(ctx->core_client);
            }
        }
    }
}

static void mcpp_srv_thread_playback_close(void)
{
    for (int32_t user = 0; user < APP_MCPP_USER_QTY; user++) {
        mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
        if (ctx->playback_close_flag) {
            pcm_process_play_deinit(user);
            ctx->playback_close_flag = false;
            if (ctx->core_server != ctx->core_client) {
                mcpp_cmd_send_empty_rsp(ctx->core_client);
            }
        }
    }
}

#ifdef RTOS
#include "cmsis_os.h"
static SRAM_DATA_LOC osThreadId mcpp_srv_thread_tid;

static void mcpp_srv_thread(void const *argument);
osThreadDef(mcpp_srv_thread, osPriorityAboveNormal, 1, 3 * 1024, "mcpp_srv");

SRAM_TEXT_LOC void mcpp_srv_thread(void const *argument)
{
    osEvent evt;
    uint32_t signals = 0;

    while(1) {
#ifdef MCPP_TRIGGER_DSP_PROCESS
        evt = osSignalWait(0x0, osWaitForever);
#else
        evt = osSignalWait(0x0, 1);
#endif
        signals = evt.value.signals;
        // TRACE(3,"[%s] status = %x, signals = %d, 01", __func__, evt.status, evt.value.signals);

        if (evt.status == osEventSignal) {
            if (signals & (0x1 << MCPP_SRV_MSG_CAPTURE_CLOSE)) {
                mcpp_srv_thread_capture_close();
                osThreadSetPriority(mcpp_srv_thread_tid, osPriorityAboveNormal);
            }
            if (signals & (0x1 << MCPP_SRV_MSG_PLAYBACK_CLOSE)) {
                mcpp_srv_thread_playback_close();
                osThreadSetPriority(mcpp_srv_thread_tid, osPriorityAboveNormal);
            }
#ifdef MCPP_TRIGGER_DSP_PROCESS
            if ((signals & (0x1 << SPEECH_MSG_CAPTURE_DATA_COME)) || (signals & (0x1 << SPEECH_MSG_PLAYBACK_DATA_COME))) {
                mcpp_srv_process();
            }
#endif
        } else {
#ifdef MCPP_TRIGGER_DSP_PROCESS
            TRACE(2, "[%s] ERROR: evt.status = %d", __func__, evt.status);
            continue;
#else
            mcpp_srv_process();
#endif
        }
    }
}

SRAM_TEXT_LOC void mcpp_srv_open(void)
{
    mcpp_load_core_cmd_cfg();
    mcpp_cmd_received_client_cb_register(mcpp_srv_received_cmd_handler);
    mcpp_srv_thread_tid = osThreadCreate(osThread(mcpp_srv_thread), NULL);
    osSignalSet(mcpp_srv_thread_tid, 0x0);
}

static void mcpp_srv_single_thread_core_cap_open(APP_MCPP_USER_T user, algo_process_cfg_t *cfg)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ctx->algo_capture_ptr->open(cfg);
    ctx->deinterlaved_cap_pcm_buf = speech_calloc(cfg->frame_len * cfg->mic_num, cfg->sample_bytes);
}

static void mcpp_srv_single_thread_core_cap_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ctx->algo_capture_ptr->close();
    speech_free(ctx->deinterlaved_cap_pcm_buf);
    ctx->single_thread_cap_status = false;
}

static void mcpp_srv_single_thread_core_cap_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    algo_process_cfg_t *cfg = mcpp_srv_get_capture_cfg(user);

    if (cfg->sample_bytes == sizeof(int16_t)) {
        int16_t *process_pcm_buf = (int16_t *)pcm_cfg->in;
        int16_t *deinterlaved_cap_pcm_buf = (int16_t *)ctx->deinterlaved_cap_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_cap_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    } else if (cfg->sample_bytes == sizeof(int32_t)) {
        int32_t *process_pcm_buf = (int32_t *)pcm_cfg->in;
        int32_t *deinterlaved_cap_pcm_buf = (int32_t *)ctx->deinterlaved_cap_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_cap_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    }
    uint8_t *pcm_buf[PCM_CHANNEL_INDEX_QTY] = {NULL};

    for (uint32_t i = 0; i < cfg->mic_num; i++) {
        pcm_buf[i] = &ctx->deinterlaved_cap_pcm_buf[i * cfg->frame_len * cfg->sample_bytes];
    }

    pcm_buf[PCM_CHANNEL_INDEX_REF] = pcm_cfg->ref;
    pcm_buf[PCM_CHANNEL_INDEX_REF2] = pcm_cfg->ref2;
    pcm_buf[PCM_CHANNEL_INDEX_VPU] = pcm_cfg->vpu;

    ctx->algo_capture_ptr->process(pcm_buf, pcm_cfg->out, cfg->frame_len);
}

static void mcpp_srv_single_thread_core_play_open(APP_MCPP_USER_T user, algo_process_cfg_t *cfg)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ctx->algo_playback_ptr->open(cfg);
    ctx->deinterlaved_play_pcm_buf = speech_calloc(cfg->frame_len * cfg->mic_num, cfg->sample_bytes);
}

static void mcpp_srv_single_thread_core_play_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ctx->algo_playback_ptr->close();
    speech_free(ctx->deinterlaved_play_pcm_buf);
    ctx->single_thread_play_status = false;
}

static void mcpp_srv_single_thread_core_play_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    algo_process_cfg_t *cfg = mcpp_srv_get_playback_cfg(user);

    if (cfg->sample_bytes == sizeof(int16_t)) {
        int16_t *process_pcm_buf = (int16_t *)pcm_cfg->in;
        int16_t *deinterlaved_play_pcm_buf = (int16_t *)ctx->deinterlaved_play_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_play_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    } else if (cfg->sample_bytes == sizeof(int32_t)) {
        int32_t *process_pcm_buf = (int32_t *)pcm_cfg->in;
        int32_t *deinterlaved_play_pcm_buf = (int32_t *)ctx->deinterlaved_play_pcm_buf;
        for (uint32_t ch = 0; ch < cfg->mic_num; ch++) {
            for (uint32_t i=0; i<cfg->frame_len; i++) {
                deinterlaved_play_pcm_buf[ch * cfg->frame_len + i] = process_pcm_buf[cfg->mic_num * i + ch];
            }
        }
    }
    uint8_t *pcm_buf[PCM_CHANNEL_INDEX_QTY] = {NULL};

    for (uint32_t i = 0; i < cfg->mic_num; i++) {
        pcm_buf[i] = &ctx->deinterlaved_play_pcm_buf[i * cfg->frame_len * cfg->sample_bytes];
    }

    ctx->algo_playback_ptr->process(pcm_buf, pcm_cfg->out, cfg->frame_len);
}

WEAK void mcpp_srv_stream_capture_open(APP_MCPP_USER_T user, void *info, uint16_t info_len)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    APP_MCPP_CTX *info_ctx = (APP_MCPP_CTX *)info;
    APP_MCPP_STREAM_CFG_T *capture_info = &info_ctx->mcpp_cfg.capture;

    ctx->capture_enable = capture_info->stream_enable;
    ctx->core_server = capture_info->core_server;
    ctx->core_client = info_ctx->core_client;
    ctx->user = info_ctx->user;

    // ASSERT(capture_info->core_server == g_mcpp_core);

    if (ctx->core_server == ctx->core_client && capture_info->frame_len == capture_info->algo_frame_len) {
        ctx->single_thread_cap_status = true;
    }

    algo_process_cfg_t *cfg = mcpp_srv_get_capture_cfg(user);
    MCPP_LOG_I("[%s]:There is the info of capture opening!", __func__);
    MCPP_LOG_I("sample_rate:       %d", capture_info->sample_rate);
    MCPP_LOG_I("sample_bytes:      %d", capture_info->sample_bytes);
    MCPP_LOG_I("algo_frame_len:    %d", capture_info->algo_frame_len);
    MCPP_LOG_I("in_channel_num:    %d", capture_info->channel_num);
    MCPP_LOG_I("cap_channel_map: 0x%x", capture_info->params[1]);
    MCPP_LOG_I("core_server:       %d", capture_info->core_server);
    MCPP_LOG_I("core_client:       %d", info_ctx->core_client);
    MCPP_LOG_I("user:              %d", info_ctx->user);

    // TODO: bth use algo_process_cfg_t directly
    cfg->sample_rate = capture_info->sample_rate;
    cfg->sample_bytes = capture_info->sample_bytes;
    cfg->frame_len = capture_info->algo_frame_len;
    cfg->mic_num = capture_info->channel_num;
    cfg->mode = capture_info->params[0];
    cfg->delay = capture_info->delay;
    cfg->channel_map = capture_info->params[1];

    ctx->algo_capture_ptr = load_capture_algo_process(ctx->user);

    if (ctx->single_thread_cap_status) {
        mcpp_srv_single_thread_core_cap_open(user, cfg);
    } else {
        pcm_process_cap_init(user, cfg);
        if (ctx->capture_enable){
            info_ctx->cap_fifo = &ctx->capture_fifo;
            MCPP_LOG_I("[%s] fifo_capture: %p", __func__, info_ctx->cap_fifo);
            MCPP_LOG_I("[%s] fifo_capture size: %d", __func__, mcpp_srv_get_capture_fifo_ctx(user)->pcm_fifo.size);
        }
        ctx->capture_opened = true;
        osSignalSet(mcpp_srv_thread_tid, 1 << MCPP_SRV_MSG_CAPTURE_OPEN);
    }
}

WEAK void mcpp_srv_stream_playback_open(APP_MCPP_USER_T user, void *info, uint16_t info_len)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    APP_MCPP_CTX *info_ctx = (APP_MCPP_CTX *)info;
    APP_MCPP_STREAM_CFG_T *playback_info = &info_ctx->mcpp_cfg.playback;

    ctx->playback_enable = playback_info->stream_enable;
    ctx->core_server = playback_info->core_server;
    ctx->core_client = info_ctx->core_client;
    ctx->user = info_ctx->user;

    // ASSERT(playback_info->core_server == g_mcpp_core);

    if (ctx->core_server == ctx->core_client && playback_info->frame_len == playback_info->algo_frame_len) {
        ctx->single_thread_play_status = true;
    }

    algo_process_cfg_t *cfg = mcpp_srv_get_playback_cfg(user);
    MCPP_LOG_I("[%s]:There is the info of playback opening!", __func__);
    MCPP_LOG_I("sample_rate:       %d", playback_info->sample_rate);
    MCPP_LOG_I("sample_bytes:      %d", playback_info->sample_bytes);
    MCPP_LOG_I("algo_frame_len:    %d", playback_info->algo_frame_len);
    MCPP_LOG_I("in_channel_num:    %d", playback_info->channel_num);
    MCPP_LOG_I("core_server:       %d", playback_info->core_server);
    MCPP_LOG_I("core_client:       %d", ctx->core_client);
    MCPP_LOG_I("user:              %d", ctx->user);

    // TODO: bth use algo_process_cfg_t directly
    cfg->sample_rate = playback_info->sample_rate;
    cfg->sample_bytes = playback_info->sample_bytes;
    cfg->frame_len = playback_info->algo_frame_len;
    cfg->mic_num = playback_info->channel_num;
    cfg->delay = playback_info->delay;

    ctx->algo_playback_ptr = load_playback_algo_process(ctx->user);

    if (ctx->single_thread_play_status) {
        mcpp_srv_single_thread_core_play_open(user, cfg);
    } else {
        pcm_process_play_init(user, cfg);
        if (ctx->playback_enable){
            info_ctx->play_fifo = &ctx->playback_fifo;
            MCPP_LOG_I("[%s] fifo_playback: %p", __func__, info_ctx->play_fifo);
            MCPP_LOG_I("[%s] fifo_playback size: %d", __func__, mcpp_srv_get_playback_fifo_ctx(user)->pcm_fifo.size);
        }
        ctx->playback_opened = true;
        osSignalSet(mcpp_srv_thread_tid, 1 << MCPP_SRV_MSG_PLAYBACK_OPEN);
    }
}

WEAK void mcpp_srv_stream_capture_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);

    if (ctx->single_thread_cap_status) {
        mcpp_srv_single_thread_core_cap_close(user);
    } else {
        ASSERT(ctx->capture_opened == true, "[%s]:stream not open", __func__);
        ctx->capture_opened = false;
        ctx->capture_close_flag = true;
        osThreadSetPriority(mcpp_srv_thread_tid, osPriorityRealtime);
        osSignalSet(mcpp_srv_thread_tid, 1 << MCPP_SRV_MSG_CAPTURE_CLOSE);
    }
}

WEAK void mcpp_srv_stream_playback_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);

    if (ctx->single_thread_play_status) {
        mcpp_srv_single_thread_core_play_close(user);
    } else {
        ASSERT(ctx->playback_opened == true, "[%s]:stream not open", __func__);
        ctx->playback_opened = false;
        ctx->playback_close_flag = true;
        osThreadSetPriority(mcpp_srv_thread_tid, osPriorityRealtime);
        osSignalSet(mcpp_srv_thread_tid, 1 << MCPP_SRV_MSG_PLAYBACK_CLOSE);
    }
}

WEAK void mcpp_srv_capture_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (ctx->single_thread_cap_status) {
        mcpp_srv_single_thread_core_cap_process(user, pcm_cfg);
    } else {
        osSignalSet(mcpp_srv_thread_tid, 1 << MCPP_SRV_MSG_CAPTURE_DATA_COME);
    }
}

WEAK void mcpp_srv_playback_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (ctx->single_thread_play_status) {
        mcpp_srv_single_thread_core_play_process(user, pcm_cfg);
    } else {
        osSignalSet(mcpp_srv_thread_tid, 1 << MCPP_SRV_MSG_PLAYBACK_DATA_COME);
    }
}

#else
#include "hal_sleep.h"
#define MCPP_SERVER_CPU_WAKE_USER              (HAL_CPU_WAKE_LOCK_USER_19)
static int32_t g_mcpp_server_main_status = 0;
static bool g_mcpp_server_baremetal_inited = false;

WEAK void mcpp_srv_stream_capture_open(APP_MCPP_USER_T user, void *info, uint16_t info_len)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    APP_MCPP_CTX *info_ctx = (APP_MCPP_CTX *)info;
    APP_MCPP_STREAM_CFG_T *capture_info = &info_ctx->mcpp_cfg.capture;

    ctx->capture_enable = capture_info->stream_enable;
    ctx->core_server = capture_info->core_server;
    ctx->core_client = info_ctx->core_client;
    ctx->user = info_ctx->user;

    algo_process_cfg_t *cfg = mcpp_srv_get_capture_cfg(user);
    MCPP_LOG_I("[%s]:There is the info of capture opening!", __func__);
    MCPP_LOG_I("sample_rate:       %d", capture_info->sample_rate);
    MCPP_LOG_I("sample_bytes:      %d", capture_info->sample_bytes);
    MCPP_LOG_I("algo_frame_len:    %d", capture_info->algo_frame_len);
    MCPP_LOG_I("in_channel_num:    %d", capture_info->channel_num);
    MCPP_LOG_I("cap_channel_map: 0x%x", capture_info->params[1]);
    MCPP_LOG_I("core_server:       %d", capture_info->core_server);
    MCPP_LOG_I("core_client:       %d", info_ctx->core_client);
    MCPP_LOG_I("user:              %d", info_ctx->user);

    // TODO: bth use algo_process_cfg_t directly
    cfg->sample_rate = capture_info->sample_rate;
    cfg->sample_bytes = capture_info->sample_bytes;
    cfg->frame_len = capture_info->algo_frame_len;
    cfg->mic_num = capture_info->channel_num;
    cfg->mode = capture_info->params[0];
    cfg->delay = capture_info->delay;
    cfg->channel_map = capture_info->params[1];

    ctx->algo_capture_ptr = load_capture_algo_process(ctx->user);
    pcm_process_cap_init(user, cfg);
    if (ctx->capture_enable){
        info_ctx->cap_fifo = &ctx->capture_fifo;
        MCPP_LOG_I("[%s] fifo_capture: %p", __func__, info_ctx->cap_fifo);
        MCPP_LOG_I("[%s] fifo_capture size: %d", __func__, mcpp_srv_get_capture_fifo_ctx(user)->pcm_fifo.size);
    }
    ctx->capture_opened = true;
}

WEAK void mcpp_srv_stream_playback_open(APP_MCPP_USER_T user, void *info, uint16_t info_len)
{
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    APP_MCPP_CTX *info_ctx = (APP_MCPP_CTX *)info;
    APP_MCPP_STREAM_CFG_T *playback_info = &info_ctx->mcpp_cfg.playback;

    ctx->playback_enable = playback_info->stream_enable;
    ctx->core_server = playback_info->core_server;
    ctx->core_client = info_ctx->core_client;
    ctx->user = info_ctx->user;

    algo_process_cfg_t *cfg = mcpp_srv_get_playback_cfg(user);
    MCPP_LOG_I("[%s]:There is the info of playback opening!", __func__);
    MCPP_LOG_I("sample_rate:       %d", playback_info->sample_rate);
    MCPP_LOG_I("sample_bytes:      %d", playback_info->sample_bytes);
    MCPP_LOG_I("algo_frame_len:    %d", playback_info->algo_frame_len);
    MCPP_LOG_I("in_channel_num:    %d", playback_info->channel_num);
    MCPP_LOG_I("core_server:       %d", playback_info->core_server);
    MCPP_LOG_I("core_client:       %d", ctx->core_client);
    MCPP_LOG_I("user:              %d", ctx->user);

    // TODO: bth use algo_process_cfg_t directly
    cfg->sample_rate = playback_info->sample_rate;
    cfg->sample_bytes = playback_info->sample_bytes;
    cfg->frame_len = playback_info->algo_frame_len;
    cfg->mic_num = playback_info->channel_num;
    cfg->delay = playback_info->delay;

    ctx->algo_playback_ptr = load_playback_algo_process(ctx->user);
    pcm_process_play_init(user, cfg);
    if (ctx->playback_enable){
        info_ctx->play_fifo = &ctx->playback_fifo;
        MCPP_LOG_I("[%s] fifo_playback: %p", __func__, info_ctx->play_fifo);
        MCPP_LOG_I("[%s] fifo_playback size: %d", __func__, mcpp_srv_get_playback_fifo_ctx(user)->pcm_fifo.size);
    }
    ctx->playback_opened = true;
}

WEAK void mcpp_srv_stream_capture_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ASSERT(ctx->capture_opened == true, "[%s]:stream not open", __func__);
    ctx->capture_opened = false;
    ctx->capture_close_flag = true;
    g_mcpp_server_main_status |= (1 << MCPP_SRV_MSG_CAPTURE_CLOSE);

}

WEAK void mcpp_srv_stream_playback_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    ASSERT(ctx->playback_opened == true, "[%s]:stream not open", __func__);
    ctx->playback_opened = false;
    ctx->playback_close_flag = true;
    g_mcpp_server_main_status |= (1 << MCPP_SRV_MSG_PLAYBACK_CLOSE);

}

WEAK void mcpp_srv_capture_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg)
{
    g_mcpp_server_main_status |= (1 << MCPP_SRV_MSG_CAPTURE_DATA_COME);

}

WEAK void mcpp_srv_playback_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg)
{
    g_mcpp_server_main_status |= (1 << MCPP_SRV_MSG_PLAYBACK_DATA_COME);
}

void mcpp_server_baremetal_main(void)
{
    if (!g_mcpp_server_baremetal_inited) {
        mcpp_cmd_received_client_cb_register(mcpp_srv_received_cmd_handler);
        g_mcpp_server_baremetal_inited = true;
    }

    if (mcpp_srv_core_is_opened()) {
        hal_cpu_wake_lock(MCPP_SERVER_CPU_WAKE_USER);
    }

    if (g_mcpp_server_main_status & (1 << MCPP_SRV_MSG_CAPTURE_CLOSE)) {
        mcpp_srv_thread_capture_close();
        g_mcpp_server_main_status &= ~(1 << MCPP_SRV_MSG_CAPTURE_CLOSE);
    } else if (g_mcpp_server_main_status & (1 << MCPP_SRV_MSG_PLAYBACK_CLOSE)) {
        mcpp_srv_thread_playback_close();
        g_mcpp_server_main_status &= ~(1 << MCPP_SRV_MSG_PLAYBACK_CLOSE);
    } else {
        mcpp_srv_process();
    }

    if (!mcpp_srv_core_is_opened()) {
        hal_cpu_wake_unlock(MCPP_SERVER_CPU_WAKE_USER);
    }
}
#endif

WEAK int32_t mcpp_srv_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (!(ctx->single_thread_cap_status) && !(ctx->capture_opened)) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        return -1;
    }

    MCPP_SET_CFG_T *algo_cfg = (MCPP_SET_CFG_T *)cfg;
    if (ctx->single_thread_cap_status) {
        ctx->algo_capture_ptr->set_cfg(algo_cfg->cfg, algo_cfg->cfg_len);
    } else {
        if (ctx->cap_cfg.cfg == NULL) {
            ctx->cap_cfg.cfg = speech_calloc(1,algo_cfg->cfg_len);
        }
        memcpy(ctx->cap_cfg.cfg, algo_cfg->cfg, algo_cfg->cfg_len);
        ctx->cap_cfg.cfg_len = algo_cfg->cfg_len;
    }
    return 0;
}

WEAK int32_t mcpp_srv_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (!(ctx->single_thread_play_status) && !(ctx->playback_opened)) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        return -1;
    }

    MCPP_SET_CFG_T *algo_cfg = (MCPP_SET_CFG_T *)cfg;
    if (ctx->single_thread_play_status) {
        ctx->algo_playback_ptr->set_cfg(algo_cfg->cfg, algo_cfg->cfg_len);
    } else {
        if (ctx->play_cfg.cfg == NULL) {
            ctx->play_cfg.cfg = speech_calloc(1,algo_cfg->cfg_len);
        }
        memcpy(ctx->play_cfg.cfg, algo_cfg->cfg, algo_cfg->cfg_len);
        ctx->play_cfg.cfg_len = algo_cfg->cfg_len;
    }
    return 0;
}

WEAK int32_t mcpp_srv_capture_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (!(ctx->single_thread_cap_status) && !(ctx->capture_opened)) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        return -1;
    }

    MCPP_ALGO_CTL_T *ctl_cfg = (MCPP_ALGO_CTL_T *)ctl_ptr;
    if (ctx->single_thread_cap_status || ctl_cfg->sync_flag) {
        ctx->algo_capture_ptr->ctl(ctl_cfg->ctl, ctl_cfg->ptr, ctl_cfg->ptr_len);
    } else {
        ctx->cap_ctl.ctl = ctl_cfg->ctl;
        ctx->cap_ctl.ptr = ctl_cfg->ptr;
        ctx->cap_ctl.ptr_len = ctl_cfg->ptr_len;
        ctx->cap_ctl.sync_flag = ctl_cfg->sync_flag;
    }
    return 0;
}

WEAK int32_t mcpp_srv_playback_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    mcpp_srv_ctx_t *ctx = mcpp_srv_get_ctx(user);
    if (!(ctx->single_thread_play_status) && !(ctx->playback_opened)) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        return -1;
    }

    MCPP_ALGO_CTL_T *ctl_cfg = (MCPP_ALGO_CTL_T *)ctl_ptr;
    if (ctx->single_thread_play_status || ctl_cfg->sync_flag) {
        ctx->algo_playback_ptr->ctl(ctl_cfg->ctl, ctl_cfg->ptr, ctl_cfg->ptr_len);
    } else {
        ctx->play_ctl.ctl = ctl_cfg->ctl;
        ctx->play_ctl.ptr = ctl_cfg->ptr;
        ctx->play_ctl.ptr_len = ctl_cfg->ptr_len;
        ctx->play_ctl.sync_flag = ctl_cfg->sync_flag;
    }
    return 0;
}

#else
#include "mcpp_server.h"

WEAK void mcpp_srv_stream_capture_open(APP_MCPP_USER_T user, void *info, uint16_t info_len);
WEAK void mcpp_srv_stream_playback_open(APP_MCPP_USER_T user, void *info, uint16_t info_len);
WEAK void mcpp_srv_stream_capture_close(APP_MCPP_USER_T user);
WEAK void mcpp_srv_stream_playback_close(APP_MCPP_USER_T user);
WEAK void mcpp_srv_capture_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg);
WEAK void mcpp_srv_playback_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg);
WEAK int32_t mcpp_srv_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);
WEAK int32_t mcpp_srv_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len);
WEAK int32_t mcpp_srv_capture_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len);
WEAK int32_t mcpp_srv_playback_algo_ctl(APP_MCPP_USER_T user, void *ctl_ptr, uint16_t ptr_len);

#endif
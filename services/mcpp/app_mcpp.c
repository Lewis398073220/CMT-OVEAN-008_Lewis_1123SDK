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
#include "hal_location.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "speech_cfg.h"
#include "math.h"
#include "norflash_api.h"
#include "app_rpc_api.h"
#include "app_mcpp.h"
#include "app_mcpp_cfg.h"
#include "mcpp_server.h"
#include "mcpp_client.h"
#include "mcpp_core.h"
#include "mcpp_cmd.h"

#define GET_OUT_FIFO_DATA_WAIT_TIME_US       (10)
#define GET_OUT_FIFO_DATA_WAIT_CNT           (2)

static const osMutexAttr_t APP_MCPP_MutexAttr = {
    .name = "APP_MCPP_MUTEX",
    .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
    .cb_mem = NULL,
    .cb_size = 0U,
};
static osMutexId_t g_app_mcpp_mutex_id = NULL;

void app_mcpp_lock_init(void)
{
    if (g_app_mcpp_mutex_id == NULL) {
        g_app_mcpp_mutex_id = osMutexNew(&APP_MCPP_MutexAttr);
        ASSERT(g_app_mcpp_mutex_id, "[%s]:mutex init error", __func__);
    }
}

static void app_mcpp_lock(void)
{
    osMutexAcquire(g_app_mcpp_mutex_id, osWaitForever);
}

static void app_mcpp_unlock(void)
{
    osMutexRelease(g_app_mcpp_mutex_id);
}

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

static APP_MCPP_CTX g_mcpp_ctx[APP_MCPP_USER_QTY] = {0};

static APP_MCPP_CTX *get_mcpp_ctx(APP_MCPP_USER_T user)
{
    return &g_mcpp_ctx[user];
}

static APP_MCPP_CFG_T *get_mcpp_cfg(APP_MCPP_USER_T user)
{
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    return &mcpp_ctx->mcpp_cfg;
}

static capture_fifo_ctx_t *get_capture_fifo_ctx(APP_MCPP_USER_T user)
{
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    return mcpp_ctx->cap_fifo;
}

static playback_fifo_ctx_t *get_playback_fifo_ctx(APP_MCPP_USER_T user)
{
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    return mcpp_ctx->play_fifo;
}

static APP_MCPP_STREAM_CFG_T *get_capture_cfg(APP_MCPP_USER_T user)
{
    APP_MCPP_CFG_T *mcpp_cfg = get_mcpp_cfg(user);
    return &mcpp_cfg->capture;
}

static APP_MCPP_STREAM_CFG_T *get_playback_cfg(APP_MCPP_USER_T user)
{
    APP_MCPP_CFG_T *mcpp_cfg = get_mcpp_cfg(user);
    return &mcpp_cfg->playback;
}

static void app_mcpp_result_process(APP_MCPP_USER_T user, uint8_t* ptr, uint16_t len)
{
    ASSERT(len == sizeof(MCPP_RES_TER_T), "[%s] len(%d) != %d", __func__, len, sizeof(MCPP_RES_TER_T));
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    MCPP_RES_TER_T *res_ret = (MCPP_RES_TER_T *)ptr;
    switch (res_ret->stream_flag)
    {
    case MCPP_STREAM_CAPTURE:
        mcpp_ctx->capture_callback(res_ret->ptr, res_ret->ptr_len);
        break;
    case MCPP_STREAM_PLAYBACK:
        mcpp_ctx->playback_callback(res_ret->ptr, res_ret->ptr_len);
        break;
    default:
        break;
    }
}

void app_mcpp_init(void)
{
    app_mcpp_lock_init();
    mcpp_cli_lock_init();
}

int32_t app_mcpp_open(APP_MCPP_USER_T user, APP_MCPP_CFG_T *cfg)
{
    MCPP_LOG_I("%s begin, user:%d", __func__, user);
    ASSERT(user < APP_MCPP_USER_QTY, "[%s]:The user:%d error!", __func__, user);

    app_mcpp_lock();
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (mcpp_ctx->opened_status) {
        ASSERT(false, "[%s]:The user:%d has been opened", __func__, user);
    }

    memset(mcpp_ctx, 0, sizeof(APP_MCPP_CTX));
    mcpp_ctx->cap_fifo = NULL;
    mcpp_ctx->play_fifo = NULL;
    mcpp_ctx->user = user;
    mcpp_ctx->mcpp_cfg = *cfg;
    mcpp_ctx->core_client = g_mcpp_core;

    MCPP_CLI_INIT_CFG_T init_cfg = {0};
    init_cfg.capture_enable = mcpp_ctx->mcpp_cfg.capture.stream_enable;
    init_cfg.playback_enable = mcpp_ctx->mcpp_cfg.playback.stream_enable;
    init_cfg.capture_server = mcpp_ctx->mcpp_cfg.capture.core_server;
    init_cfg.playback_server = mcpp_ctx->mcpp_cfg.playback.core_server;
    mcpp_cli_init(user, &init_cfg);
    mcpp_cli_result_process_cb_register(app_mcpp_result_process);
    mcpp_cli_wait_server_core_inited(user);

    APP_MCPP_STREAM_CFG_T *capture = get_capture_cfg(user);
    if (capture->stream_enable){
        if (capture->algo_frame_len == 0) {
            capture->algo_frame_len = capture->frame_len;
        }
        if (capture->core_server == g_mcpp_core){
            mcpp_srv_stream_capture_open(user, mcpp_ctx, sizeof(APP_MCPP_CTX));
            if (capture->frame_len == capture->algo_frame_len) {
                mcpp_ctx->single_thread_cap_status = true;
            } else {
                mcpp_ctx->single_thread_cap_status = false;
            }
        }else{
            mcpp_cli_stream_capture_open(user, mcpp_ctx, sizeof(APP_MCPP_CTX));
        }
    }

    APP_MCPP_STREAM_CFG_T *playback = get_playback_cfg(user);
    if (playback->stream_enable){
        if (playback->algo_frame_len == 0) {
            playback->algo_frame_len = playback->frame_len;
        }
        if (playback->channel_num > 1 && playback->channel_num != 0) {
            MCPP_LOG_I("[%s]:set playback channel num to 1, algo_frame_len to increase!", __func__);
            playback->algo_frame_len *= playback->channel_num;
            playback->channel_num = 1;
        }
        if (playback->core_server == g_mcpp_core){
            mcpp_srv_stream_playback_open(user, mcpp_ctx, sizeof(APP_MCPP_CTX));
            if (playback->frame_len == playback->algo_frame_len) {
                mcpp_ctx->single_thread_play_status = true;
            } else {
                mcpp_ctx->single_thread_play_status = false;
            }
        }else{
            mcpp_cli_stream_playback_open(user, mcpp_ctx, sizeof(APP_MCPP_CTX));
        }
    }

    if (mcpp_ctx->cap_fifo) {
        MCPP_LOG_I("[%s] fifo_capture addr: %p, size: %d", __func__,
                    mcpp_ctx->cap_fifo, mcpp_ctx->cap_fifo->pcm_fifo.size);
    }
    if (mcpp_ctx->play_fifo) {
        MCPP_LOG_I("[%s] fifo_playback addr: %p, size: %d", __func__,
                    mcpp_ctx->play_fifo, mcpp_ctx->play_fifo->pcm_fifo.size);
    }

    MCPP_LOG_I("%s end, user:%d", __func__, user);
    mcpp_ctx->opened_status = true;
    app_mcpp_unlock();
    return 0;
}

uint32_t app_mcpp_get_cap_channel_map(bool ref_flag, bool ref2_flag, bool vpu_flag)
{
    MCPP_LOG_I("[%s] ref:%d, ref2:%d, vpu:%d", __func__, ref_flag, ref2_flag, vpu_flag);
    uint32_t channel_map = 0;
    if (ref_flag){
        channel_map |= PCM_CHANNEL_MAP_CAP_REF;
    }
    if (ref2_flag){
        channel_map |= PCM_CHANNEL_MAP_CAP_REF2;
    }
    if (vpu_flag){
        channel_map |= PCM_CHANNEL_MAP_CAP_VPU;
    }
    return channel_map;
}

int32_t app_mcpp_close(APP_MCPP_USER_T user)
{
    MCPP_LOG_I("%s begin, user:%d", __func__, user);
    ASSERT(user < APP_MCPP_USER_QTY, "[%s]:The user:%d error!", __func__, user);

    app_mcpp_lock();
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        ASSERT(false, "[%s]:The user:%d has been closed", __func__, user);
    }

    mcpp_ctx->opened_status = false;

    APP_MCPP_CFG_T *cfg = get_mcpp_cfg(user);

    if (cfg->playback.stream_enable){
        if (cfg->playback.core_server == g_mcpp_core){
            mcpp_srv_stream_playback_close(user);
            mcpp_ctx->single_thread_play_status = false;
        }else{
            mcpp_cli_stream_playback_close(user);
        }
    }

    if (cfg->capture.stream_enable){
        if (cfg->capture.core_server == g_mcpp_core){
            mcpp_srv_stream_capture_close(user);
            mcpp_ctx->single_thread_cap_status = false;
        }else{
            mcpp_cli_stream_capture_close(user);
        }
    }

    mcpp_cli_deinit(user);
    memset(mcpp_ctx, 0, sizeof(APP_MCPP_CTX));
    MCPP_LOG_I("%s end, user:%d", __func__, user);
    app_mcpp_unlock();

    return 0;
}

int32_t app_mcpp_capture_process(APP_MCPP_USER_T user, APP_MCPP_CAP_PCM_T *pcm_cfg)
{
    ASSERT(user < APP_MCPP_USER_QTY, "[%s]:The user:%d error!", __func__, user);

    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        ASSERT(false, "[%s]:The user:%d has been closed", __func__, user);
    }

    if (mcpp_ctx->single_thread_cap_status){
        mcpp_srv_capture_process(user, pcm_cfg);
    } else {

        APP_MCPP_STREAM_CFG_T *cfg = get_capture_cfg(user);
        capture_fifo_ctx_t *ctx = get_capture_fifo_ctx(user);
        ASSERT(ctx != NULL, "[%s] capture fifo ctx is NULL", __func__);

        int32_t pcm_data_size = pcm_cfg->frame_len * cfg->channel_num * cfg->sample_bytes;
        int32_t data_size = pcm_cfg->frame_len * cfg->sample_bytes;

        if (kfifo_get_free_space(&ctx->pcm_fifo) >= pcm_data_size) {
            if (pcm_cfg->ref) {
                kfifo_put(&ctx->ref_fifo, pcm_cfg->ref, data_size);
            }
            if (pcm_cfg->ref2) {
                kfifo_put(&ctx->ref2_fifo, pcm_cfg->ref2, data_size);
            }
            if (pcm_cfg->vpu) {
                kfifo_put(&ctx->vpu_fifo, pcm_cfg->vpu, data_size);
            }
            kfifo_put(&ctx->pcm_fifo, pcm_cfg->in, pcm_data_size);
        } else {
            MCPP_LOG_I("[%s] Input buffer is overflow", __func__);
        }
#ifdef MCPP_TRIGGER_DSP_PROCESS
        mcpp_cli_send_capture_trigger(pcm_cfg);
#endif
        int32_t cnt = 0;
        int32_t out_size = pcm_cfg->frame_len * cfg->sample_bytes;
        while (true) {
            if (kfifo_len(&ctx->out_fifo) >= out_size) {
                kfifo_get(&ctx->out_fifo, pcm_cfg->out, out_size);
                break;
            } else {
                if (cnt <= GET_OUT_FIFO_DATA_WAIT_CNT) {
                    MCPP_LOG_I("[%s] Output buffer is underflow, wait cnt:%d.", __func__, cnt);
                    hal_sys_timer_delay_us(GET_OUT_FIFO_DATA_WAIT_TIME_US);
                    cnt ++;
                } else {
                    MCPP_LOG_I("[%s] Output buffer is underflow, out set to zero!", __func__);
                    memset(pcm_cfg->out, 0, out_size);
                    break;
                }
            }
        }
    }

    return 0;
}

int32_t app_mcpp_playback_process(APP_MCPP_USER_T user, APP_MCPP_PLAY_PCM_T *pcm_cfg)
{
    ASSERT(user < APP_MCPP_USER_QTY, "[%s]:The user:%d error!", __func__, user);

    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        ASSERT(false, "[%s]:The user:%d has been closed", __func__, user);
    }

    if (mcpp_ctx->single_thread_play_status){
        mcpp_srv_playback_process(user, pcm_cfg);
    } else {
        APP_MCPP_STREAM_CFG_T *cfg = get_playback_cfg(user);
        playback_fifo_ctx_t *ctx = get_playback_fifo_ctx(user);
        ASSERT(ctx != NULL, "[%s] playback fifo ctx is NULL", __func__);

        int32_t pcm_data_size = pcm_cfg->frame_len * cfg->channel_num * cfg->sample_bytes;

        if (kfifo_get_free_space(&ctx->pcm_fifo) >= pcm_data_size) {
            kfifo_put(&ctx->pcm_fifo, pcm_cfg->in, pcm_data_size);
        } else {
            MCPP_LOG_I("[%s] Input buffer is overflow", __func__);
        }
#ifdef MCPP_TRIGGER_DSP_PROCESS
        mcpp_cli_send_playback_trigger(pcm_cfg);
#endif
        int32_t cnt = 0;
        int32_t out_size = pcm_cfg->frame_len * cfg->sample_bytes;
        while (true) {
            if (kfifo_len(&ctx->out_fifo) >= out_size) {
                kfifo_get(&ctx->out_fifo, pcm_cfg->out, out_size);
                break;
            } else {
                if (cnt <= GET_OUT_FIFO_DATA_WAIT_CNT) {
                    MCPP_LOG_I("[%s] Output buffer is underflow, wait cnt:%d.", __func__, cnt);
                    hal_sys_timer_delay_us(GET_OUT_FIFO_DATA_WAIT_TIME_US);
                    cnt ++;
                } else {
                    MCPP_LOG_I("[%s] Output buffer is underflow, out set to zero!", __func__);
                    memset(pcm_cfg->out, 0, out_size);
                    break;
                }
            }
        }
    }
    return 0;
}

int32_t app_mcpp_capture_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    app_mcpp_lock();
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        app_mcpp_unlock();
        return -1;
    }
    ASSERT(cfg != NULL, "[%s]:cfg addr is NULL!", __func__);

    MCPP_SET_CFG_T algo_cfg = {0};
    algo_cfg.cfg = cfg;
    algo_cfg.cfg_len = cfg_len;

    int32_t ret = -1;
    if (mcpp_ctx->mcpp_cfg.capture.stream_enable){
        if (mcpp_ctx->mcpp_cfg.capture.core_server == g_mcpp_core){
            ret = mcpp_srv_capture_set_algo_cfg(user, &algo_cfg, sizeof(MCPP_SET_CFG_T));
        }else{
            ret = mcpp_cli_capture_set_algo_cfg(user, &algo_cfg, sizeof(MCPP_SET_CFG_T));
        }
    } else {
        MCPP_LOG_I("[%s] capture stream disabled!", __func__);
    }
    app_mcpp_unlock();
    return ret;
}

int32_t app_mcpp_playback_set_algo_cfg(APP_MCPP_USER_T user, void *cfg, uint16_t cfg_len)
{
    MCPP_LOG_I("[%s]:The user:%d!", __func__, user);
    app_mcpp_lock();
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        app_mcpp_unlock();
        return -1;
    }
    ASSERT(cfg != NULL, "[%s]:cfg addr is NULL!", __func__);

    MCPP_SET_CFG_T algo_cfg = {0};
    algo_cfg.cfg = cfg;
    algo_cfg.cfg_len = cfg_len;

    int32_t ret = -1;
    if (mcpp_ctx->mcpp_cfg.playback.stream_enable){
        if (mcpp_ctx->mcpp_cfg.playback.core_server == g_mcpp_core){
            ret = mcpp_srv_playback_set_algo_cfg(user, &algo_cfg, sizeof(MCPP_SET_CFG_T));
        }else{
            ret = mcpp_cli_playback_set_algo_cfg(user, &algo_cfg, sizeof(MCPP_SET_CFG_T));
        }
    } else {
        MCPP_LOG_I("[%s] playback stream disabled!", __func__);
    }
    app_mcpp_unlock();
    return ret;
}

int32_t app_mcpp_capture_algo_ctl(APP_MCPP_USER_T user, int32_t ctl, void *ptr, uint16_t ptr_len, bool sync_flag)
{
    MCPP_LOG_I("[%s]:The user:%d, sync_flag:%d", __func__, user, sync_flag);
    app_mcpp_lock();
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        app_mcpp_unlock();
        return -1;
    }
    // ASSERT(ptr != NULL, "[%s]:ptr addr is NULL!", __func__);

    MCPP_ALGO_CTL_T algo_ctl = {0};
    algo_ctl.ctl = ctl;
    algo_ctl.ptr = ptr;
    algo_ctl.ptr_len = ptr_len;
    algo_ctl.sync_flag = sync_flag;

    int32_t ret = -1;
    if (mcpp_ctx->mcpp_cfg.capture.stream_enable){
        if (mcpp_ctx->mcpp_cfg.capture.core_server == g_mcpp_core){
            ret = mcpp_srv_capture_algo_ctl(user, &algo_ctl, sizeof(MCPP_ALGO_CTL_T));
        }else{
            ret = mcpp_cli_capture_algo_ctl(user, &algo_ctl, sizeof(MCPP_ALGO_CTL_T));
        }
    } else {
        MCPP_LOG_I("[%s] capture stream disabled!", __func__);
    }
    app_mcpp_unlock();
    return ret;
}

int32_t app_mcpp_playback_algo_ctl(APP_MCPP_USER_T user, int32_t ctl, void *ptr, uint16_t ptr_len, bool sync_flag)
{
    MCPP_LOG_I("[%s]:The user:%d, sync_flag:%d", __func__, user, sync_flag);
    app_mcpp_lock();
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    if (!mcpp_ctx->opened_status) {
        MCPP_LOG_I("[%s]:The user:%d is not opened", __func__, user);
        app_mcpp_unlock();
        return -1;
    }
    // ASSERT(ptr != NULL, "[%s]:ptr addr is NULL!", __func__);

    MCPP_ALGO_CTL_T algo_ctl = {0};
    algo_ctl.ctl = ctl;
    algo_ctl.ptr = ptr;
    algo_ctl.ptr_len = ptr_len;
    algo_ctl.sync_flag = sync_flag;

    int32_t ret = -1;
    if (mcpp_ctx->mcpp_cfg.playback.stream_enable){
        if (mcpp_ctx->mcpp_cfg.playback.core_server == g_mcpp_core){
            ret = mcpp_srv_playback_algo_ctl(user, &algo_ctl, sizeof(MCPP_ALGO_CTL_T));
        }else{
            ret = mcpp_cli_playback_algo_ctl(user, &algo_ctl, sizeof(MCPP_ALGO_CTL_T));
        }
    } else {
        MCPP_LOG_I("[%s] playback stream disabled!", __func__);
    }
    app_mcpp_unlock();
    return ret;
}

void app_mcpp_capture_async_ctl_cb_register(APP_MCPP_USER_T user, app_mcpp_async_handler_t handler)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    mcpp_ctx->capture_callback = handler;
}

void app_mcpp_playback_async_ctl_cb_register(APP_MCPP_USER_T user, app_mcpp_async_handler_t handler)
{
    MCPP_LOG_I("[%s]: The user is %d", __func__, user);
    APP_MCPP_CTX *mcpp_ctx = get_mcpp_ctx(user);
    mcpp_ctx->playback_callback = handler;
}
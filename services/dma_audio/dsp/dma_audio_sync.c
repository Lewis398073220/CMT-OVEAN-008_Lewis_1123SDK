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
#include "string.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hal_cache.h"
#include "dma_audio.h"
#include "dma_audio_stream.h"
#include "dma_audio_stream_conf.h"
#include "dma_audio_sync.h"

//#define DMA_AUDIO_TUNE_CLOCK_BYPASS
//#define DMA_AUDIO_SYNC_DEBUG_VERBOSE 2

#define DMA_AUDIO_SYNC_USE_PID

#define FLOAT_I_X3(v)       ((int)((v) * 1000))
#define FLOAT_I_VAL(v)      ((int)(v))
#define FLOAT_P_VAL(v)      ((int)((v) * 1000 - ((int)(v)) * 1000))
#define FLOAT_TO_PPB_INT(f) ((int)((f) * 1000 * 1000 * 1000))

#define INTV_TIME_NUM        (5)
#define MAX_ERR_INTV_SAMP    (5)
#define MIN_ERR_INTV_SAMP    (-4-1)

#define SAMP_DIFF_NUM        (5)
#ifdef DMA_AUDIO_LOW_LATENCY
#define HI_SAMP_DIFF         (30.0f)
#define LO_SAMP_DIFF         (30.0f)
#else
#define HI_SAMP_DIFF         (80.0f)
#define LO_SAMP_DIFF         (80.0f)
#endif

#define SYNC_COEF_SUM_MAX_CNT (20)
#define DELTA_TIME_FACTOR     (40000.0f)

enum CODEC_RSPL_RATE_ID_T {
    CODEC_RSPL_RATE_ID_1,
    CODEC_RSPL_RATE_ID_2,
    CODEC_RSPL_RATE_ID_3,
};

#if defined(DMA_AUDIO_USE_DAC3)
#define CODEC_TUNE_RSPL_RATE_ID                             (CODEC_RSPL_RATE_ID_3)
#elif defined(DMA_AUDIO_USE_DAC2)
#define CODEC_TUNE_RSPL_RATE_ID                             (CODEC_RSPL_RATE_ID_2)
#elif defined(DMA_AUDIO_USE_DAC1)
#define CODEC_TUNE_RSPL_RATE_ID                             (CODEC_RSPL_RATE_ID_1)
#else
#if defined(CHIP_BEST1600) || defined(CHIP_BEST1603)
#define CODEC_TUNE_RSPL_RATE_ID                             (CODEC_RSPL_RATE_ID_3)
#elif defined(CHIP_BEST1502X)
#define CODEC_TUNE_RSPL_RATE_ID                             (CODEC_RSPL_RATE_ID_1)
#else
#error "Not support CHIP_BESTXXXX !!!"
#endif
#endif

#if defined(CHIP_BEST1600) || defined(CHIP_BEST1603)
#define CODEC_HW_ADDR                                       (0x40300000)
#define CODEC_CODEC_DAC1_UPDATE_REG_OFFS                    (0x0E4)
#define CODEC_CODEC_DAC1_RSPL_REG_OFFS                      (0x0EC)
#define CODEC_CODEC_DAC2_UPDATE_REG_OFFS                    (0x7C0)
#define CODEC_CODEC_DAC2_RSPL_REG_OFFS                      (0x7C8)
#define CODEC_CODEC_DAC3_UPDATE_REG_OFFS                    (0x7C0)
#define CODEC_CODEC_DAC3_RSPL_REG_OFFS                      (0x7CC)
#define CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE               (1 << 8)
#define CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE_SND           (1 << 4)
#define CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE_TRI           (1 << 9)
#elif defined(CHIP_BEST1502X)
#define CODEC_HW_ADDR                                       (0x40300000)
#define CODEC_CODEC_DAC1_UPDATE_REG_OFFS                    (0x0E4)
#define CODEC_CODEC_DAC1_RSPL_REG_OFFS                      (0x0EC)
#define CODEC_CODEC_DAC2_UPDATE_REG_OFFS                    (0x0E4)
#define CODEC_CODEC_DAC2_RSPL_REG_OFFS                      (0x0F4)
#define CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE               (1 << 8)
#define CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE_SND           (1 << 22)
#endif

enum AUD_STREAM_SYNC_STATUS_T {
    AUD_STREAM_SYNC_STATUS_OPEN_CLOSE  = (1 << 0),
    AUD_STREAM_SYNC_STATUS_START_STOP  = (1 << 1),
    AUD_STREAM_SYNC_STATUS_PAUSE_RESUME = (1 << 2),
};

enum SYNC_COEF_STATE_T {
    SYNC_COEF_STATE_IDLE,
    SYNC_COEF_STATE_SUM,
    SYNC_COEF_STATE_TUNE,
};

struct SYNC_COEF_CNTL_T {
    int sync_state;
    int sum_err_cnt;
    float sum_err_diff;

    float Ts_factor;
    float Ts;
    float Ti;
    float Td;
    float Kp;
    float Ki;
    float Kd;
    float e[3];
    float du;
};

struct AUD_STREAM_SYNC_CNTL_T {
    uint8_t status;

    uint32_t samp_size;
    uint32_t samp_cnt;
    uint32_t samp_ns;
    uint32_t frame_ms;

    enum CODEC_RSPL_RATE_ID_T rspl_id;

    int32_t avg_intv_us;
    int32_t exp_intv_us;
    int32_t err_intv_us;
    int32_t max_err_intv_us;
    int32_t min_err_intv_us;
    uint32_t intv_index;
    uint32_t intv_time_num;
    uint32_t calc_intv_cnt;
    uint32_t calc_intv_max;
    uint32_t intv_time[INTV_TIME_NUM];
    uint32_t local_time;

    float avg_samp_diff;
    float exp_samp_diff;
    float err_samp_diff;
    float hi_samp_diff;
    float lo_samp_diff;
    int32_t samp_diff_idx;
    int32_t samp_diff_num;
    uint32_t calc_diff_cnt;
    uint32_t calc_diff_max;
    uint32_t exp_set_cnt;
    uint32_t sync_irq_cnt;
    uint32_t last_sync_time;
    float sync_clock_ratio;
    int32_t samp_diff_val[SAMP_DIFF_NUM];
};

struct AUD_STREAM_SYNC_T {
    struct AUD_STREAM_SYNC_CNTL_T cntl;
    struct AUD_STREAM_SYNC_CONFIG_T cfg;
    struct SYNC_COEF_CNTL_T coef_cntl;
};

static struct AUD_STREAM_SYNC_T g_aud_stream_sync[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];

static bool dma_irq_sw_trig_mode[AUD_STREAM_ID_NUM][AUD_STREAM_NUM] = {false};

void dma_audio_stream_sync_sw_trig_mode(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, bool enable)
{
    dma_irq_sw_trig_mode[id][stream] = enable;
    TRACE(1, "[%s]: id=%d, stream=%d, enable=%d", __func__, id, stream, enable);
}

static struct AUD_STREAM_SYNC_T *get_stream_sync(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s]: Bad id=%d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s]: Bad stream=%d", __func__, stream);

    return &g_aud_stream_sync[id][stream];
}

uint32_t sync_timer_tick_get(void)
{
    uint32_t time;

    time = hal_fast_sys_timer_get();

    return time;
}

uint32_t sync_timer_ticks_to_ms(uint32_t ticks)
{
    uint32_t ms;

    ms = FAST_TICKS_TO_MS(ticks);
    return ms;
}

uint32_t sync_timer_ticks_to_us(uint32_t ticks)
{
    uint32_t us;

    us = FAST_TICKS_TO_US(ticks);
    return us;
}

void sync_timer_delay_us(uint32_t us)
{
    hal_sys_timer_delay_us(us);
}

static void sync_coef_init(struct SYNC_COEF_CNTL_T *cntl)
{
    float Kp, Ki, Kd;
    float Td, Ti, Ts, Ts_factor;

    cntl->sync_state = SYNC_COEF_STATE_IDLE;
    cntl->sum_err_cnt = 0;
    cntl->sum_err_diff = 0;

#ifdef __AUDIO_RESAMPLE__
    Ts_factor = 20;
    //Ts_factor = 100;
#if 0
    Ts = 5;
    Ti = 100;
    Td = 1 / 1000;

    Kp = 0.6;
    Ki = Ts / Ti; //0.05
    Kd = Td / Ts; //0.0002

#else
    Ts = 5;
    Ti = 50;
    Td = 1 / 1000;

    Kp = 0.6;
    Ki = Ts / Ti; //0.01
    Kd = Td / Ts; //0.0002
#endif
#else
    Ts_factor = 10;

    Ts = 5;
    Ti = 10;
    Td = 1 / 10;

    Kp = 0.8;
    Ki = Ts / Ti; //0.05
    Kd = Td / Ts; //0.0002
#endif

    cntl->Ts_factor = Ts_factor;
    cntl->Ts = Ts;
    cntl->Ti = Ti;
    cntl->Td = Td;
    cntl->Kp = Kp;
    cntl->Ki = Ki;
    cntl->Kd = Kd;
    cntl->e[0] = 0;
    cntl->e[1] = 0;
    cntl->e[2] = 0;
    cntl->du = 0;
}

POSSIBLY_UNUSED static bool sync_coef_calc_PID(struct AUD_STREAM_SYNC_T *sync, float *ptr_err_diff)
{
    struct SYNC_COEF_CNTL_T *cntl = &sync->coef_cntl;
    float *e = cntl->e;
    float err = *ptr_err_diff;
    float du;
    float Kp, Ki, Kd;

    e[2] = e[1];
    e[1] = e[0];
    e[0] = err;

    Kp = cntl->Kp;
    Ki = cntl->Ki;
    Kd = cntl->Kd;

    du = Kp * (e[0] - e[1]) + Kp * Ki * e[0] + Kp * Kd * (e[0] - 2 * e[1] + e[2]);
    cntl->du = du;

    *ptr_err_diff = du;

    return true;
}

POSSIBLY_UNUSED static bool sync_coef_calc_slow_P(struct AUD_STREAM_SYNC_T *sync, float *ptr_err_diff)
{
    struct SYNC_COEF_CNTL_T *cntl = &sync->coef_cntl;

    int next_state = cntl->sync_state;
    int exit = 1;
    bool tune = false;
    float cur_err_diff = *ptr_err_diff;

    do {
        switch (cntl->sync_state) {
        case SYNC_COEF_STATE_IDLE:
            if (!tune) {
                cntl->sum_err_cnt = 0;
                cntl->sum_err_diff = 0;
                next_state = SYNC_COEF_STATE_SUM;
            }
            break;
        case SYNC_COEF_STATE_SUM:
            cntl->sum_err_diff += cur_err_diff;
            cntl->sum_err_cnt++;
            if (cntl->sum_err_cnt == 1) {
                sync->cntl.last_sync_time = sync_timer_tick_get();
            }
            if (cntl->sum_err_cnt >= SYNC_COEF_SUM_MAX_CNT) {
                next_state = SYNC_COEF_STATE_TUNE;
            } else {
                next_state = SYNC_COEF_STATE_SUM;
            }
            break;
        case SYNC_COEF_STATE_TUNE:
            *ptr_err_diff = cntl->sum_err_diff;
            tune = true;
            next_state = SYNC_COEF_STATE_IDLE;
            break;
        default:
            exit = 1;
            next_state = SYNC_COEF_STATE_IDLE;
            cntl->sync_state = SYNC_COEF_STATE_IDLE;
            cntl->sum_err_cnt = 0;
            cntl->sum_err_diff = 0;
            break;
        }
        if (cntl->sync_state != next_state) {
            cntl->sync_state = next_state;
            exit = 0;
        } else {
            exit = 1;
        }
    } while (!exit);

#if 0//defined(DMA_AUDIO_SYNC_DEBUG_VERBOSE) && (DMA_AUDIO_SYNC_DEBUG_VERBOSE > 0)
    TRACE(1, "state[%d],cnt=%d,X3(cur_err_diff)=%d,X3(sum_err_diff)=%d",
        cntl->sync_state, cntl->sum_err_cnt,
        FLOAT_I_X3(cur_err_diff), FLOAT_I_X3(cntl->sum_err_diff));
#endif

    return tune;
}

static void stream_sync_reset(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    static uint32_t rst_time = 0;

    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    struct AUD_STREAM_SYNC_CNTL_T *cntl = &sync->cntl;
    uint32_t cur_time;

    cur_time = sync_timer_tick_get();
    if (sync_timer_ticks_to_ms(cur_time - rst_time) < 10) {
        return;
    }

    TRACE(1, "stream_sync_rest: id=%d, stream=%d", id, stream);

    // clear interval coef
    cntl->avg_intv_us = 0;
    cntl->err_intv_us = 0;
    cntl->calc_intv_cnt = 0;
    cntl->intv_index = 0;
    cntl->local_time = sync_timer_tick_get();

    // clear samp diff coef
    cntl->avg_samp_diff = 0;
    cntl->exp_samp_diff = 0;
    cntl->err_samp_diff = 0;
    cntl->samp_diff_idx = 0;
    cntl->calc_diff_cnt = 0;
    cntl->exp_set_cnt = 1;
    cntl->sync_irq_cnt = 0;
    cntl->last_sync_time = 0;
    cntl->sync_clock_ratio = 1.0f;

    // reset sync coef
    sync_coef_init(&sync->coef_cntl);

    rst_time = cur_time;
}

static void check_stream_resume_state(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    struct AUD_STREAM_SYNC_CNTL_T *cntl = &sync->cntl;

    if ((cntl->status & AUD_STREAM_SYNC_STATUS_PAUSE_RESUME) != 0) {
        stream_sync_reset(id, stream);
        cntl->status &= ~AUD_STREAM_SYNC_STATUS_PAUSE_RESUME;
        TRACE(1, "WARN2: stream resume: id=%d, stream=%d", id ,stream);
    }
}

static void check_stream_pause_state(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    struct AUD_STREAM_SYNC_CNTL_T *cntl = &sync->cntl;
    uint32_t cur_time, local_time;
    uint32_t diff_time;

    if ((cntl->status & AUD_STREAM_SYNC_STATUS_PAUSE_RESUME) == 0) {
        cur_time = sync_timer_tick_get();
        local_time = cntl->local_time;
        if (local_time == 0) {
            TRACE(1, "[%s]: skip check, id=%d, stream=%d ", __func__, id, stream);
            return;
        }
        diff_time = sync_timer_ticks_to_ms(cur_time - local_time);
        if (diff_time > 100) {
            local_time = sync_timer_ticks_to_ms(local_time);
            cur_time = sync_timer_ticks_to_us(cur_time);
            TRACE(1, "WARN2: stream paused: id=%d, stream=%d, local_time=%d us,cur_time=%d us",
                id ,stream, local_time, cur_time);
            cntl->status |= AUD_STREAM_SYNC_STATUS_PAUSE_RESUME;//set 1 for stream pause
        }
    }
}

int dma_audio_stream_sync_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct AUD_STREAM_SYNC_CONFIG_T *cfg)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    uint32_t samp_size, samp_cnt, frm_ms, samp_ns;
    int32_t unit_time;

    if (sync->cntl.status == AUD_STREAM_SYNC_STATUS_OPEN_CLOSE) {
        TRACE(1, "[%s]: Bad status: %d", __func__, sync->cntl.status);
        return -1;
    }

    ASSERT(cfg->pair_stream != stream, "[%s]: Bad pair_stream=%d", __func__, cfg->pair_stream);
    ASSERT(cfg->data_size != 0, "[%s]: zero data_size", __func__);

    if (cfg->bits == 24) {
        samp_size = 4;
    } else {
        samp_size = cfg->bits / 8;
    }
    samp_cnt = cfg->data_size / (samp_size * cfg->channel_num);
    frm_ms   = samp_cnt * 1000 / cfg->sample_rate;
    samp_ns  = (1000 * 1000) / (cfg->sample_rate / 1000);
    TRACE(1, "samp_size=%d,chan_num=%d,samp_cnt=%d,frm_ms=%d,samp_ns=%d",
        samp_size, cfg->channel_num, samp_cnt, frm_ms, samp_ns);

    unit_time = samp_ns;

    sync->cfg = *cfg;
    sync->cntl.samp_size = samp_size;
    sync->cntl.samp_cnt = samp_cnt;
    sync->cntl.samp_ns = samp_ns;
    sync->cntl.frame_ms = frm_ms;

    sync->cntl.rspl_id = CODEC_TUNE_RSPL_RATE_ID;

    sync->cntl.avg_intv_us = 0;
    sync->cntl.exp_intv_us = frm_ms * 1000 / 2; //div 2 is for ping pong buffer
    sync->cntl.err_intv_us = 0;
    sync->cntl.max_err_intv_us = MAX_ERR_INTV_SAMP * unit_time / 1000;
    sync->cntl.min_err_intv_us = MIN_ERR_INTV_SAMP * unit_time / 1000;
    sync->cntl.calc_intv_cnt = 0;
    sync->cntl.calc_intv_max = 1000 / (frm_ms * INTV_TIME_NUM / 2);
    sync->cntl.intv_index = 0;
    sync->cntl.local_time = 0;
    sync->cntl.intv_time_num = INTV_TIME_NUM;
    TRACE(1, "exp_intv_us=%d, err_intv_us range: [%d, %d]",
        sync->cntl.exp_intv_us, sync->cntl.min_err_intv_us, sync->cntl.max_err_intv_us);

    sync->cntl.avg_samp_diff = 0;
    sync->cntl.exp_samp_diff = 0;
    sync->cntl.err_samp_diff = 0;
    sync->cntl.hi_samp_diff  = HI_SAMP_DIFF;
    sync->cntl.lo_samp_diff  = LO_SAMP_DIFF;
    sync->cntl.samp_diff_idx = 0;
    sync->cntl.samp_diff_num = SAMP_DIFF_NUM;
    sync->cntl.calc_diff_cnt = 0;
    sync->cntl.calc_diff_max = 1000 / (frm_ms * SAMP_DIFF_NUM / 2);
    sync->cntl.exp_set_cnt   = 1;
    sync->cntl.sync_irq_cnt  = 0;
    sync->cntl.last_sync_time = 0;
    sync->cntl.sync_clock_ratio = 1.0f;
    TRACE(1, "hi_samp_diff=%d, lo_samp_diff=%d, samp_diff_num=%d",
        (int)(sync->cntl.hi_samp_diff),
        (int)(sync->cntl.lo_samp_diff),
        (int)(sync->cntl.samp_diff_num));

    sync_coef_init(&sync->coef_cntl);

    sync->cntl.status |= AUD_STREAM_SYNC_STATUS_OPEN_CLOSE;
    TRACE(1, "[%s]: done, id=%d, stream=%d", __func__, id, stream);
    return 0;
}

int dma_audio_stream_sync_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);

    if (sync->cntl.status != AUD_STREAM_SYNC_STATUS_OPEN_CLOSE) {
        TRACE(1, "[%s]: Bad status: %d", __func__, sync->cntl.status);
        return -1;
    }

    sync->cntl.status |= AUD_STREAM_SYNC_STATUS_START_STOP;
    TRACE(1, "[%s]: done, id=%d, stream=%d", __func__, id, stream);
    return 0;
}

int dma_audio_stream_sync_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);

    if (sync->cntl.status != (AUD_STREAM_SYNC_STATUS_OPEN_CLOSE
                        | AUD_STREAM_SYNC_STATUS_START_STOP)) {
        TRACE(1, "[%s]: Bad status: %d", __func__, sync->cntl.status);
        return -1;
    }

    sync->cntl.status &= ~AUD_STREAM_SYNC_STATUS_START_STOP;
    TRACE(1, "[%s]: done, id=%d, stream=%d", __func__, id, stream);
    return 0;
}

int dma_audio_stream_sync_reset(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    stream_sync_reset(id, stream);
    TRACE(1, "[%s]: done, id=%d, stream=%d", __func__, id, stream);
    return 0;
}

int dma_audio_stream_sync_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);

    if (sync->cntl.status != AUD_STREAM_SYNC_STATUS_OPEN_CLOSE) {
        TRACE(1, "[%s]: Bad status: %d", __func__, sync->cntl.status);
        return -1;
    }

    sync->cntl.status &= ~AUD_STREAM_SYNC_STATUS_OPEN_CLOSE;
    TRACE(1, "[%s]: done, id=%d, stream=%d", __func__, id, stream);
    return 0;
}

static void calc_stream_interval(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t cur_time;
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    struct AUD_STREAM_SYNC_CNTL_T *cntl = &sync->cntl;

    cur_time = sync_timer_tick_get();

    cntl->local_time = cur_time;

    if (cntl->intv_time_num != INTV_TIME_NUM) {
        ASSERT(false, "[%s]: Bad intv_time_num=%d", __func__, cntl->intv_time_num);
    }
    if (cntl->intv_index <= cntl->intv_time_num - 1) {
        cntl->intv_time[cntl->intv_time_num - cntl->intv_index - 1] = cur_time;
        cntl->intv_index++;
    }
    if (cntl->intv_index >= cntl->intv_time_num) {
        uint32_t st = 0, i, dt, avg_time;
        uint32_t num = cntl->intv_time_num;
        for (i = 0; i < num - 1; i++) {
            dt = cntl->intv_time[i] - cntl->intv_time[i+1];
            st += dt;
        }
        avg_time = st / (num - 1);
        cntl->avg_intv_us = FAST_TICKS_TO_US(avg_time);
        cntl->err_intv_us = cntl->avg_intv_us - cntl->exp_intv_us;
        cntl->calc_intv_cnt++;

#if defined(DMA_AUDIO_SYNC_DEBUG_VERBOSE) && (DMA_AUDIO_SYNC_DEBUG_VERBOSE > 2)
        if (cntl->calc_intv_cnt >= cntl->calc_intv_max) {
            cntl->calc_intv_cnt = 0;
            if (stream == AUD_STREAM_CAPTURE) {
                TRACE(1, "[SYNC_CAP ]: exp_intv=%d, avg_intv=%d, err_intv=%d",
                cntl->exp_intv_us, cntl->avg_intv_us, cntl->err_intv_us);
            } else {
                TRACE(1, "[SYNC_PLAY]: exp_intv=%d, avg_intv=%d, err_intv=%d",
                cntl->exp_intv_us, cntl->avg_intv_us, cntl->err_intv_us);
            }
        }
#endif

        if ((cntl->err_intv_us < cntl->min_err_intv_us)
            || (cntl->err_intv_us > cntl->max_err_intv_us)) {

            TRACE(1, "SYNC_WARN1: stream(%d) err_intv_us=%d out of range: [%d, %d]",
            stream, cntl->err_intv_us, cntl->min_err_intv_us, cntl->max_err_intv_us);
        }
        cntl->intv_index = 0;
    }
}

#ifdef __AUDIO_RESAMPLE__
static uint32_t resample_phase_float_to_value(float phase)
{
    if (phase >= 4.0) {
        return (uint32_t)-1;
    } else {
        // Phase format: 2.30
        return (uint32_t)(phase * (1 << 30));
    }
}

static void dma_audio_tune_dac_rspl_rate(enum CODEC_RSPL_RATE_ID_T id, float ratio)
{
    uint32_t val = 0, update_msk = 0, update_val = 0;
    uint32_t update_reg = 0, phase_reg = 0;

    switch (id) {
    case CODEC_RSPL_RATE_ID_1:
        update_msk = CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        update_reg = CODEC_HW_ADDR + CODEC_CODEC_DAC1_UPDATE_REG_OFFS;
        phase_reg  = CODEC_HW_ADDR + CODEC_CODEC_DAC1_RSPL_REG_OFFS;
        break;
    case CODEC_RSPL_RATE_ID_2:
        update_msk = CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE_SND;
        update_reg = CODEC_HW_ADDR + CODEC_CODEC_DAC2_UPDATE_REG_OFFS;
        phase_reg  = CODEC_HW_ADDR + CODEC_CODEC_DAC2_RSPL_REG_OFFS;
        break;
#if defined(CHIP_BEST1600) || defined(CHIP_BEST1603)
    case CODEC_RSPL_RATE_ID_3:
        update_msk = CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE_TRI;
        update_reg = CODEC_HW_ADDR + CODEC_CODEC_DAC3_UPDATE_REG_OFFS;
        phase_reg  = CODEC_HW_ADDR + CODEC_CODEC_DAC3_RSPL_REG_OFFS;
        break;
#endif
    default:
        ASSERT(false, "[%s]: Bad id=%d", __func__, id);
        break;
    }
    update_val = *(volatile uint32_t *)update_reg;
    if (update_val & update_msk) {
        update_val &= ~update_msk;
        *(volatile uint32_t *)update_reg = update_val;
        sync_timer_delay_us(2);

        val = resample_phase_float_to_value(1.024);//24.576/24=1.024
        val = (int)(val * ratio);
        *(volatile uint32_t *)phase_reg = val;

        sync_timer_delay_us(2);
        update_val |= update_msk;
        *(volatile uint32_t *)update_reg = update_val;
    }
}
#else /* ! __AUDIO_RESAMPLE__ */
static void dma_audio_tune_aud_pll_rate(enum CODEC_RSPL_RATE_ID_T id, float ratio)
{
    dma_rpc_tune_clock(ratio);
}
#endif

static int dma_audio_stream_tune_clock(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, float err_diff)
{
    uint32_t sync_time, last_sync_time;
    int samp_rate;
    float delta_ratio, delta_period;
    float delta_time, delta_time_factor;
    float sync_clock_ratio;
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    bool tune;

#ifdef DMA_AUDIO_TUNE_CLOCK_BYPASS
    return 0;
#endif

#ifdef DMA_AUDIO_SYNC_USE_PID
    tune = sync_coef_calc_PID(sync, &err_diff);
#else
    tune = sync_coef_calc_slow_P(sync, &err_diff);
#endif
    if (!tune) {
        return 0;
    }

    sync_clock_ratio = sync->cntl.sync_clock_ratio;
    samp_rate = sync->cfg.sample_rate;

    /*
     * caculate sync_clock_ratio for clock tuning:
     *
     * (1) delta_time = sync_time - last_sync_time;
     * (2) delta_period = delta_time * delta_time_factor;
     * (3) delta_ratio = err_samp_diff / samp_rate / delta_period;
     * (4) sync_clock_ratio = sync_clock_ratio - delta_ratio;
     */
    sync_time = sync_timer_tick_get();
    last_sync_time = sync->cntl.last_sync_time;
#ifdef DMA_AUDIO_SYNC_USE_PID
    delta_time        = sync->coef_cntl.Ts;
    delta_time_factor = sync->coef_cntl.Ts_factor;
#else
    delta_time        = sync_timer_ticks_to_ms(sync_time - last_sync_time);
    delta_time_factor = DELTA_TIME_FACTOR;
#endif
    last_sync_time = sync_time;

    delta_period = (float)delta_time * delta_time_factor;
    delta_ratio = ((err_diff * 1000) / (float)samp_rate) / delta_period;

#ifdef __AUDIO_RESAMPLE__
    sync_clock_ratio = sync_clock_ratio - delta_ratio;
#else
    sync_clock_ratio = -delta_ratio;
#endif

    sync->cntl.sync_clock_ratio = sync_clock_ratio;
    sync->cntl.last_sync_time = last_sync_time;

#if defined(DMA_AUDIO_SYNC_DEBUG_VERBOSE) && (DMA_AUDIO_SYNC_DEBUG_VERBOSE > 2)
    TRACE(1, "SYNC_COEF:X3(err_diff)=%d,dta_period=%d,PPB(dta_ratio)=%d,PPB(clk_ratio)=%d",
        FLOAT_I_X3(err_diff),FLOAT_I_VAL(delta_period),
        FLOAT_TO_PPB_INT(delta_ratio), FLOAT_TO_PPB_INT(sync_clock_ratio));
#endif

#ifdef __AUDIO_RESAMPLE__
        dma_audio_tune_dac_rspl_rate(sync->cntl.rspl_id, sync_clock_ratio);
#else
        dma_audio_tune_aud_pll_rate(sync->cntl.rspl_id, sync_clock_ratio);
#endif

    return 0;
}

static void calc_stream_samp_diff(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    struct AUD_STREAM_SYNC_CNTL_T *cntl = &sync->cntl;
    int i;
    int dma_cur_pos[2], dma_pos[2], dma_end_pos[2], dma_mid_pos[2];

    enum AUD_STREAM_ID_T pair_id = sync->cfg.pair_id;
    enum AUD_STREAM_T pair_stream = sync->cfg.pair_stream;

    for (i = 0; i < AUD_STREAM_NUM; i++) {
        uint32_t dma_addr, buf_addr, buf_size;

        if (i == pair_stream) {
            sync = get_stream_sync(pair_id, pair_stream);
            dma_addr = daud_stream_get_cur_dma_addr(pair_id, pair_stream);
        } else {
            sync = get_stream_sync(id, stream);
            dma_addr = daud_stream_get_cur_dma_addr(id, stream);
        }
        cntl = &sync->cntl;
        buf_addr = (uint32_t)(sync->cfg.data_ptr);
        buf_size = sync->cfg.data_size;
        if ((dma_addr >= buf_addr) && (dma_addr <= buf_addr + buf_size)) {
            dma_pos[i] = (int)(dma_addr - buf_addr);
        } else {
            dma_pos[i] = 0;
        }
        if (dma_pos[i] == buf_size) {
            dma_pos[i] = 0;
        }
        dma_cur_pos[i] = dma_pos[i] / (cntl->samp_size * sync->cfg.channel_num);
        dma_end_pos[i] = buf_size / (cntl->samp_size * sync->cfg.channel_num) - 1;
        dma_mid_pos[i] = buf_size / (cntl->samp_size * sync->cfg.channel_num) / 2 - 1;
    }

    sync = get_stream_sync(id, stream);
    cntl = &sync->cntl;

    int play_cur_pos, play_end_pos, play_mid_pos;
    int cap_cur_pos, cap_end_pos, cap_mid_pos;
    int samp_diff;
    bool odd_irq_pos = false;

    //cur_pos range: [0, mid_pos], [mid_pos+1, end_pos]
    if ((cntl->samp_diff_num % 2) != 0) {
        odd_irq_pos = true;
    } else {
        odd_irq_pos = false;
    }
    play_cur_pos = dma_cur_pos[AUD_STREAM_PLAYBACK];
    play_end_pos = dma_end_pos[AUD_STREAM_PLAYBACK];
    play_mid_pos = dma_mid_pos[AUD_STREAM_PLAYBACK];

    cap_cur_pos = dma_cur_pos[AUD_STREAM_CAPTURE];
    cap_end_pos = dma_end_pos[AUD_STREAM_CAPTURE];
    cap_mid_pos = dma_mid_pos[AUD_STREAM_CAPTURE];
    (void)cap_mid_pos;
    (void)cap_end_pos;

    if (odd_irq_pos) {
        if (dma_irq_sw_trig_mode[id][AUD_STREAM_PLAYBACK]) {
            if (play_cur_pos > play_mid_pos) {
                play_cur_pos -= (play_mid_pos + 1);
                cap_cur_pos += (cap_mid_pos + 1);
            }
        }
        if ((play_cur_pos > play_mid_pos) && (cap_cur_pos <= cap_mid_pos)) {
            samp_diff = play_end_pos - play_cur_pos + cap_cur_pos;
        } else {
            samp_diff = play_cur_pos - cap_cur_pos;
        }
    } else {
        if ((play_cur_pos > play_mid_pos) && (cap_cur_pos >= cap_mid_pos)) {
            samp_diff = play_end_pos - play_cur_pos + cap_cur_pos;
        } else {
            samp_diff = play_cur_pos - cap_cur_pos;
        }
    }

    cntl->sync_irq_cnt++;
    if (cntl->sync_irq_cnt == 1) {
        cntl->last_sync_time = sync_timer_tick_get();
    }
    if (cntl->samp_diff_idx <= cntl->samp_diff_num - 1) {
        cntl->samp_diff_val[cntl->samp_diff_num - cntl->samp_diff_idx - 1] = samp_diff;
        cntl->samp_diff_idx++;
    }
    if (cntl->samp_diff_idx >= cntl->samp_diff_num) {
        int n = cntl->samp_diff_num;
        float sum = 0, avg = 0;
        float hi_thd, lo_thd;
        int err_diff;

        for (i = 0; i < n; i++) {
            sum += (float)cntl->samp_diff_val[i];
        }
        avg = sum / n;

        cntl->avg_samp_diff = avg;
        if (cntl->exp_set_cnt > 0) {
            cntl->exp_samp_diff = cntl->avg_samp_diff;
            cntl->exp_set_cnt--;
            TRACE(1, "set[%d]: exp_samp_diff=%d.%d", cntl->exp_set_cnt,
            FLOAT_I_VAL(cntl->exp_samp_diff), FLOAT_P_VAL(cntl->exp_samp_diff));
        }
        cntl->err_samp_diff = cntl->avg_samp_diff - cntl->exp_samp_diff;

        hi_thd = cntl->exp_samp_diff + cntl->hi_samp_diff;
        lo_thd = cntl->exp_samp_diff - cntl->lo_samp_diff;

        if (avg > hi_thd || avg < lo_thd) {
            TRACE(1, "WARN3: avg_diff=%d.%d, out of range: [%d.%d, %d.%d]",
            FLOAT_I_VAL(cntl->avg_samp_diff),FLOAT_P_VAL(cntl->avg_samp_diff),
            FLOAT_I_VAL(lo_thd),FLOAT_P_VAL(lo_thd),
            FLOAT_I_VAL(hi_thd),FLOAT_P_VAL(hi_thd));
            stream_sync_reset(id, stream);
        }

#if defined(DMA_AUDIO_SYNC_DEBUG_VERBOSE) && (DMA_AUDIO_SYNC_DEBUG_VERBOSE > 1)
        TRACE(1, "pos=[%d %d], samp_pos=[%d %d], samp_diff=%d",
            dma_pos[0], dma_pos[1],
            dma_cur_pos[0], dma_cur_pos[1], samp_diff);

        TRACE(1, "[%d %d %d %d %d], avg=%d.%d, exp=%d.%d, err=%d.%d",
            cntl->samp_diff_val[0],
            cntl->samp_diff_val[1],
            cntl->samp_diff_val[2],
            cntl->samp_diff_val[3],
            cntl->samp_diff_val[4],
            FLOAT_I_VAL(cntl->avg_samp_diff),FLOAT_P_VAL(cntl->avg_samp_diff),
            FLOAT_I_VAL(cntl->exp_samp_diff),FLOAT_P_VAL(cntl->exp_samp_diff),
            FLOAT_I_VAL(cntl->err_samp_diff),FLOAT_P_VAL(cntl->err_samp_diff));
#endif

        cntl->calc_diff_cnt++;
#if defined(DMA_AUDIO_SYNC_DEBUG_VERBOSE) && (DMA_AUDIO_SYNC_DEBUG_VERBOSE > 0)
        if (cntl->calc_diff_cnt >= cntl->calc_diff_max) {
            cntl->calc_diff_cnt = 0;
            TRACE(1, "[SYNC_CAP ]: exp_diff=%d.%d, avg_diff=%d.%d, err_diff=%d.%d",
                FLOAT_I_VAL(cntl->exp_samp_diff), FLOAT_P_VAL(cntl->exp_samp_diff),
                FLOAT_I_VAL(cntl->avg_samp_diff), FLOAT_P_VAL(cntl->avg_samp_diff),
                FLOAT_I_VAL(cntl->err_samp_diff), FLOAT_P_VAL(cntl->err_samp_diff));
        }
#endif
        cntl->samp_diff_idx = 0;

        err_diff = (int)(cntl->err_samp_diff * 100);
        if (err_diff != 0) {
#if defined(DMA_AUDIO_SYNC_DEBUG_VERBOSE) && (DMA_AUDIO_SYNC_DEBUG_VERBOSE > 2)
            if (err_diff < 0) {
                TRACE(1, "playback tune fast");
            } else {
                TRACE(1, "playback tune slow");
            }
#endif
            dma_audio_stream_tune_clock(id, stream, cntl->err_samp_diff);
        }
    }
}

static uint32_t capture_stream_sync_callback(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    uint8_t mask = AUD_STREAM_SYNC_STATUS_OPEN_CLOSE | AUD_STREAM_SYNC_STATUS_START_STOP;

    if ((sync->cntl.status & mask) != mask) {
        TRACE(1, "[%s]: Bad status: %d", __func__, sync->cntl.status);
        return 1;
    }

    check_stream_resume_state(id, stream);

    calc_stream_interval(id, stream);

    calc_stream_samp_diff(id, stream);

    return 0;
}

static uint32_t playback_stream_sync_callback(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct AUD_STREAM_SYNC_T *sync = get_stream_sync(id, stream);
    uint8_t mask = AUD_STREAM_SYNC_STATUS_OPEN_CLOSE | AUD_STREAM_SYNC_STATUS_START_STOP;

    if ((sync->cntl.status & mask) != mask) {
        TRACE(1, "[%s]: Bad status: %d", __func__, sync->cntl.status);
        return 1;
    }

    calc_stream_interval(id, stream);

    check_stream_pause_state(sync->cfg.pair_id, sync->cfg.pair_stream);
    return 0;
}

uint32_t dma_audio_stream_sync_callback(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    int ret;

    if (stream == AUD_STREAM_PLAYBACK) {
        ret = playback_stream_sync_callback(id, stream);
    } else {
        ret = capture_stream_sync_callback(id, stream);
    }
    return ret;
}


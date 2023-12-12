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
#include "plat_types.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "cmsis.h"
#include "string.h"
#include "hal_aud.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "hal_sleep.h"
#include "stream_dma_rpc.h"
#if defined(CHIP_ROLE_CP)
#include "cp_subsys.h"
#include "cp_subsys_core.h"
#elif defined(CHIP_SUBSYS_SENS)
#include "sensor_hub.h"
#include "sensor_hub_core.h"
#else
#include "dsp_m55_core.h"
#include "dsp_m55.h"
#endif
#include "audioflinger.h"
#include "dma_audio.h"
#include "dma_audio_stream.h"
#include "dma_audio_resample.h"
#include "aud_data_primitive.h"
#include "dma_audio_fade.h"
#ifdef DAUD_USE_HEAP
#include "heap_api.h"
#endif

#ifndef DAUD_DEBUG_VERBOSE
#define DAUD_DEBUG_VERBOSE 0
#endif

#define AF_STACK_SIZE                   (1024*4)
#define DAUD_CPU_WAKE_USER              (HAL_CPU_WAKE_LOCK_USER_18)

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP)
#define DYNAMIC_AUDIO_BUFFER_COUNT
#endif

#ifdef DYNAMIC_AUDIO_BUFFER_COUNT
#define MAX_AUDIO_BUFFER_COUNT          (16)
#define MIN_AUDIO_BUFFER_COUNT          (2)
#define AUDIO_BUFFER_COUNT              (role->dma_desc_cnt)
#else
#define MAX_AUDIO_BUFFER_COUNT          (4)
#define AUDIO_BUFFER_COUNT              (MAX_AUDIO_BUFFER_COUNT)
#if (AUDIO_BUFFER_COUNT & 0x1)
#error "AUDIO_BUFFER_COUNT must be an even number"
#endif
#endif

#define DAUD_DEBUG_TRACE()              DAUD_TRACE(1,"%s:", __func__)
#define DAUD_DEBUG_TRACE2(id, stream)   DAUD_TRACE(1,"%s:id=%d,stream=%d", __func__, id, stream)
#define DAUD_DEBUG_DONE()               DAUD_TRACE(1,"%s: done", __func__)

enum AF_STATUS_T {
    AF_STATUS_NULL = 0x00,
    AF_STATUS_OPEN_CLOSE = 0x01,
    AF_STATUS_STREAM_OPEN_CLOSE = 0x02,
    AF_STATUS_STREAM_START_STOP = 0x04,
    AF_STATUS_STREAM_PAUSE_RESTART = 0x08,
    AF_STATUS_MASK = 0x0F,
};

struct af_stream_ctl_t {
    enum AF_PP_T pp_index;      //pingpong operate
    uint8_t pp_cnt;             //use to count the lost signals
    uint8_t status;             //status machine
    enum AUD_STREAM_USE_DEVICE_T use_device;

    uint32_t hdlr_intvl_ticks;
    uint32_t prev_hdlr_time;
    bool first_hdlr_proc;
    bool intvl_check_en;
};

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP)
#define DYNAMIC_AUDIO_BUFFER_COUNT
#endif

struct af_stream_cfg_t {
    //used inside
    struct af_stream_ctl_t ctl;

    //dma buf parameters, RAM can be alloced in different way
    uint8_t *dma_buf_ptr;
    uint32_t dma_buf_size;

    //store stream cfg parameters
    struct AF_STREAM_CONFIG_T cfg;

    //dma cfg parameters
#ifdef DYNAMIC_AUDIO_BUFFER_COUNT
    uint8_t dma_desc_cnt;
#endif
    struct HAL_DMA_DESC_T dma_desc[MAX_AUDIO_BUFFER_COUNT];
    struct HAL_DMA_CH_CFG_T dma_cfg;
    uint32_t dma_base;

    //callback function
    AF_STREAM_HANDLER_T handler;
};

enum DAUD_OP_FLAG_T {
    DAUD_OP_FLAG_NONE = 0,
    DAUD_OP_FLAG_UPDATE_SAMPRATE = (1<<0),
    DAUD_OP_FLAG_UPDATE_BITS     = (1<<1),
    DAUD_OP_FLAG_UPDATE_CHAN_NUM = (1<<2),
    DAUD_OP_FLAG_UPDATE_CHAN_MAP = (1<<3),
    DAUD_OP_FLAG_SW_RESAMPLE     = (1<<4),
};

struct daud_cli_stream_state_t {
    uint8_t open  : 1;
    uint8_t start : 1;
    uint8_t rsvd  : 6;
};

typedef void (*daud_stream_data_memcpy_cb_t)(uint8_t *dst, uint8_t *src, uint32_t len);

struct daud_stream_ctl_t {
    bool forward;
    uint8_t dma_hw_chan;
    uint8_t dma_vir_chan;
    uint16_t op_flag;
    uint32_t frm_seq_max_num;
    uint32_t frm_seq;
    uint32_t dst_pp_index;
    uint8_t *dst_buf_ptr;
    uint8_t *dst_buf_end;
    uint32_t dst_buf_size;
    uint32_t dst_dsize;
    uint8_t *dma_buf_ptr;
    uint32_t dma_buf_size;
    struct daud_cli_stream_state_t cli_state;
    daud_stream_data_memcpy_cb_t memcpy_cb;
};

struct daud_stream_cfg_t {
    uint8_t status;             //status machine
    struct daud_stream_ctl_t ctl;
    struct DAUD_STREAM_CONFIG_T cfg;
    struct DAUD_RESAMPLE_T resample;
};

#define ALIGNED4 ALIGNED(4)

static struct daud_stream_cfg_t daud_stream_cfg[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];
static struct af_stream_cfg_t daud_af_stream[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];
static DAUD_ALGO_CFG_NOTIFY_T daud_algo_cfg_notify = NULL;
static DAUD_EVT_HDLR_T daud_evt_hdlr[DAUD_EVT_HDLR_ID_QTY];

#ifdef DAUD_USE_HEAP
#ifndef DAUD_HEAP_SIZE
#define DAUD_HEAP_SIZE (12*1024)
#endif
static heap_handle_t daud_heap_handle = NULL;
static ALIGNED4 uint8_t daud_heap_buff[DAUD_HEAP_SIZE];
#endif

static DAUD_IRQ_NOTIFICATION_T daud_irq_notif;

/* for software resample rate config */
struct DAUD_SW_RSPL_SAMPLE_RATE_T {
    uint32_t in_sample_rate;
    uint32_t out_sample_rate;
};

static struct DAUD_SW_RSPL_SAMPLE_RATE_T daud_sw_resample_sr[] = {
    {AUD_SAMPRATE_16000, AUD_SAMPRATE_16000},
    {AUD_SAMPRATE_48000, AUD_SAMPRATE_16000},
    {AUD_SAMPRATE_48000, AUD_SAMPRATE_48000},
    {AUD_SAMPRATE_96000, AUD_SAMPRATE_48000},
    {AUD_SAMPRATE_96000, AUD_SAMPRATE_16000},
    {AUD_SAMPRATE_96000, AUD_SAMPRATE_8000},
    {AUD_SAMPRATE_48000, AUD_SAMPRATE_8000},
};

#ifdef RTOS
static osThreadId daud_thread_tid = NULL;

static void daud_thread(void const *argument);
osThreadDef(daud_thread, osPriorityAboveNormal, 1, AF_STACK_SIZE, "dma_audio");
static int daud_default_priority;

osMutexId daud_mutex_id = NULL;
osMutexDef(daud_mutex);

#else
static volatile uint32_t daud_flag_open;
static volatile uint32_t daud_flag_signal;
static volatile uint32_t daud_flag_lock;

static void daud_set_flag(volatile uint32_t *flag, uint32_t set)
{
    uint32_t lock;

    lock = int_lock();
    *flag |= set;
    int_unlock(lock);
}

static void daud_clear_flag(volatile uint32_t *flag, uint32_t clear)
{
    uint32_t lock;

    lock = int_lock();
    *flag &= ~clear;
    int_unlock(lock);
}

static bool daud_flag_active(volatile uint32_t *flag, uint32_t bits)
{
    return !!(*flag & bits);
}
#endif

uint32_t daud_samp_bits_to_size(uint32_t bits)
{
    uint32_t samp_size = 0;

    if (bits == 16) {
        samp_size = 2;
    } else if (bits == 24) {
        samp_size = 4;
    } else if (bits == 32) {
        samp_size = 4;
    } else if (bits == 8) {
        samp_size = 1;
    } else {
        ASSERT(false, "[%s]: bits=%d, samp_size=%d", __func__, bits, samp_size);
    }
    return samp_size;
}

bool daud_stream_use_tdm_slv_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool success = false;

    if ((device == AUD_STREAM_USE_TDM0_SLAVE)
        || (device == AUD_STREAM_USE_TDM1_SLAVE)) {
        success = true;
    }
    return success;
}

bool daud_stream_use_tdm_mst_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool success = false;

    if ((device == AUD_STREAM_USE_TDM0_MASTER)
        || (device == AUD_STREAM_USE_TDM1_MASTER)) {
        success = true;
    }
    return success;
}

bool daud_stream_use_i2s_slv_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool success = false;

    if ((device == AUD_STREAM_USE_I2S0_SLAVE)
        || (device == AUD_STREAM_USE_I2S1_SLAVE)) {
        success = true;
    }
    return success;
}

bool daud_stream_use_i2s_mst_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool success = false;

    if ((device == AUD_STREAM_USE_I2S0_MASTER)
        || (device == AUD_STREAM_USE_I2S1_MASTER)) {
        success = true;
    }
    return success;
}

bool daud_stream_use_i2s_tdm_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool success = false;

    if(daud_stream_use_tdm_slv_device(device)
        || daud_stream_use_tdm_mst_device(device)
        || daud_stream_use_i2s_slv_device(device)
        || daud_stream_use_i2s_mst_device(device)) {
        success = true;
    }
    return success;
}

bool daud_stream_use_codec_device(enum AUD_STREAM_USE_DEVICE_T device)
{
    bool success = false;

    if ((device == AUD_STREAM_USE_INT_CODEC)
        || (device == AUD_STREAM_USE_INT_CODEC2)
        || (device == AUD_STREAM_USE_INT_CODEC3)) {
        success = true;
    }
    return success;
}



static void daud_unlock_thread(void)
{
    void * POSSIBLY_UNUSED lr = __builtin_return_address(0);
#ifdef RTOS
    osMutexRelease(daud_mutex_id);
#else
    static void * POSSIBLY_UNUSED unlocked_lr;
    ASSERT(daud_flag_lock == 1, "audioflinger not locked before (lastly unlocked by %p). LR=%p", (void *)unlocked_lr, (void *)lr);
    daud_flag_lock = 0;
    unlocked_lr = lr;
#endif
}

static void daud_lock_thread(void)
{
    void * POSSIBLY_UNUSED lr = __builtin_return_address(0);
#ifdef RTOS
    osMutexWait(daud_mutex_id, osWaitForever);
#else
    static void * POSSIBLY_UNUSED locked_lr;
    ASSERT(daud_flag_lock == 0, "audioflinger has been locked by %p. LR=%p", (void *)locked_lr, (void *)lr);
    daud_flag_lock = 1;
    locked_lr = lr;
#endif
}

struct af_stream_cfg_t *daud_get_af_stream_role(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s] Bad id=%d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s] Bad stream=%d", __func__, stream);

    return &daud_af_stream[id][stream];
}

struct daud_stream_cfg_t *daud_get_stream_role(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    ASSERT(id < AUD_STREAM_ID_NUM, "[%s] Bad id=%d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s] Bad stream=%d", __func__, stream);

    return &daud_stream_cfg[id][stream];
}

uint8_t daud_get_stream_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    return daud_role->status;
}

void daud_set_dma_irq_notification(AF_IRQ_NOTIFICATION_T notif)
{
    daud_irq_notif = notif;
}

enum AUD_STREAM_USE_DEVICE_T daud_stream_get_device(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *role = daud_get_stream_role(id, stream);
    return role->cfg.device;
}

#ifdef RTOS
static int daud_priority[DAUD_USER_QTY];
void daud_set_priority(enum DAUD_USER_T user, int priority)
{
    uint8_t i = 0;
    int max_priority = 0;

    daud_priority[user] = priority;

    for (i = 0; i < DAUD_USER_QTY; i++) {
        if (max_priority < daud_priority[i]) {
            max_priority = daud_priority[i];
        }
    }
    if (max_priority != daud_get_priority()) {
        osThreadSetPriority(daud_thread_tid, max_priority);
    }
}

int daud_get_priority(void)
{
    return (int)osThreadGetPriority(daud_thread_tid);
}

int daud_get_default_priority(void)
{
    return daud_default_priority;
}

void daud_reset_priority(void)
{
    osThreadSetPriority(daud_thread_tid, daud_default_priority);
}

#endif

static void daud_set_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, enum AF_STATUS_T status)
{
    struct daud_stream_cfg_t *role = NULL;

    role = daud_get_stream_role(id, stream);

    role->status |= status;
}

static void daud_clear_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, enum AF_STATUS_T status)
{
    struct daud_stream_cfg_t *role = NULL;

    role = daud_get_stream_role(id, stream);

    role->status &= ~status;
}

static uint32_t _get_cur_dma_virtual_src_addr(const struct af_stream_cfg_t *role)
{
    uint32_t remain;
    uint32_t src;
    uint32_t desc_cnt;
    uint32_t desc_size;
    uint8_t idx;
    uint32_t offset;
    uint32_t addr_offset;

    hal_dma_get_cur_src_remain_and_addr(role->dma_cfg.ch, &remain, &src);
    if (role->dma_cfg.src_width == HAL_DMA_WIDTH_HALFWORD) {
        remain *= 2;
    } else if (role->dma_cfg.src_width == HAL_DMA_WIDTH_WORD) {
        remain *= 4;
    }
    if (src < (uint32_t)role->dma_buf_ptr || src >= ((uint32_t)role->dma_buf_ptr + role->dma_buf_size)) {
        return 0;
    }
    desc_cnt = AUDIO_BUFFER_COUNT;
    if (desc_cnt == 0) {
        return 0;
    }
    desc_size = role->dma_buf_size / desc_cnt;
    if (desc_size < remain) {
        return 0;
    }
    offset = desc_size - remain;
    idx = (src - (uint32_t)role->dma_buf_ptr) / desc_size;
    addr_offset = (src - (uint32_t)role->dma_buf_ptr) % desc_size;
    if (offset > addr_offset) {
        if (idx) {
            idx--;
        } else {
            idx = desc_cnt - 1;
        }
    }

    return ((uint32_t)role->dma_buf_ptr + desc_size * idx + offset);
}

static uint32_t _get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, bool virtual)
{
    struct af_stream_cfg_t *role;
    int i;
    uint32_t addr = 0;

    role = daud_get_af_stream_role(id, stream);

    //check stream is start and not stop
    if (role->ctl.status == (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE | AF_STATUS_STREAM_START_STOP)) {
        if (role->dma_cfg.ch != HAL_DMA_CHAN_NONE) {
            for (i = 0; i < 2; i++) {
                if (stream == AUD_STREAM_PLAYBACK) {
                    if (virtual) {
                        addr = _get_cur_dma_virtual_src_addr(role);
                    } else {
                        addr = hal_audma_get_cur_src_addr(role->dma_cfg.ch);
                    }
                } else {
                    addr = hal_audma_get_cur_dst_addr(role->dma_cfg.ch);
                }
                if (addr) {
                    break;
                }
                // Previous link list item was just finished. Read current DMA address again.
            }
        } else {
            DAUD_WARN(1, "[gcda]:bad dma ch=%x,id=%d,stream=%d", role->dma_cfg.ch,id,stream);
        }
    } else {
        DAUD_WARN(1, "[gcda]:bad ctl.status=%x,id=%d,stream=%d", role->ctl.status,id,stream);
    }

    return addr;
}

uint32_t daud_stream_get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    return _get_cur_dma_addr(id, stream, false);
}

POSSIBLY_UNUSED static uint32_t daud_trans_stream_get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    int i;
    uint32_t addr = 0;
    uint32_t mask;
    uint8_t dma_chan;
    struct daud_stream_cfg_t *daud_role;

    daud_role = daud_get_stream_role(id, stream);
    dma_chan = daud_role->ctl.dma_hw_chan;

    //check stream is start and not stop
    mask = AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE | AF_STATUS_STREAM_START_STOP;
    if (daud_role->status == mask) {
        if (dma_chan != HAL_DMA_CHAN_NONE) {
            for (i = 0; i < 2; i++) {
                if (stream == AUD_STREAM_PLAYBACK) {
                    addr = hal_audma_get_cur_src_addr(dma_chan);
                } else {
                    addr = hal_audma_get_cur_dst_addr(dma_chan);
                }
                if (addr) {
                    break;
                }
                // Previous link list item was just finished. Read current DMA address again.
            }
        } else {
            DAUD_WARN(1, "[daud_gcda]:bad dma_ch=%x,id=%d,stream=%d", dma_chan,id,stream);
        }
    } else {
        DAUD_WARN(1, "[daud_gcda]:bad status=%x,id=%d,stream=%d", daud_role->status,id,stream);
    }

    return addr;
}

static void duad_stream_data_handler(struct af_stream_cfg_t *role,
    enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint8_t *buf;
    uint32_t len;
    uint32_t hw_pos, dma_addr;
    uint32_t pp_index = PP_PING;

    dma_addr = daud_stream_get_cur_dma_addr(id, stream);
    if (dma_addr == 0) {
        hw_pos = 0;
    } else {
        hw_pos = dma_addr - (uint32_t)(role->dma_buf_ptr);
    }
    if (hw_pos < role->dma_buf_size / 2) {
        pp_index = PP_PANG;
    } else if(hw_pos < role->dma_buf_size) {
        pp_index = PP_PING;
    }
    if (pp_index == PP_PING) {
        buf = role->dma_buf_ptr;
    } else {
        buf = role->dma_buf_ptr + role->dma_buf_size / 2;
    }
    len = role->dma_buf_size / 2;

#if defined(DAUD_DEBUG) && (DAUD_DEBUG > 1)
    TRACE(1, "dma_addr=%x, dma_buf_ptr=%x, hw_pos=%d, handler=%x, len=%d",
        dma_addr, (int)role->dma_buf_ptr, hw_pos, (int)role->handler, len);
#endif
    if (role->handler) {
        role->handler(buf, len);
    }
}

static void daud_client_stream_init_state(struct daud_cli_stream_state_t *cli_state)
{
    if (cli_state) {
        cli_state->open = 0;
        cli_state->start = 0;
    }
}

static void daud_client_stream_update_state(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    enum DMA_RPC_EVT_T evt)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct daud_cli_stream_state_t *cli_state = &daud_role->ctl.cli_state;

    if (0) {
    } else if (evt == DMA_RPC_EVT_STREAM_OPEN) {
        cli_state->open = 1;
    } else if (evt == DMA_RPC_EVT_STREAM_CLOSE) {
        cli_state->open = 0;
    } else if (evt == DMA_RPC_EVT_STREAM_START) {
        cli_state->start = 1;
    } else if (evt == DMA_RPC_EVT_STREAM_STOP) {
        cli_state->start = 0;
    }
}

bool daud_client_stream_is_opened(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct daud_cli_stream_state_t *cli_state = &daud_role->ctl.cli_state;

    return !!cli_state->open;
}

bool daud_client_stream_is_started(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct daud_cli_stream_state_t *cli_state = &daud_role->ctl.cli_state;

    return !!cli_state->start;
}

#ifdef DAUD_USE_HEAP
void daud_heap_init(void)
{
    uint8_t *buf = daud_heap_buff;
    uint32_t len = sizeof(daud_heap_buff);

    DAUD_TRACE(1, "[%s]: buf=%x, len=%d B", __func__, (int)buf, len);

    daud_heap_handle = heap_register((void *)buf, (size_t)len);
    ASSERT(daud_heap_handle != NULL, "[%s]: heap_register failed", __func__);
}

void *daud_heap_malloc(uint32_t size)
{
    void *ptr = NULL;

    ptr = heap_malloc(daud_heap_handle, (size_t)size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    DAUD_TRACE(1, "[%s]: ptr=%x, size=%d", __func__, (int)ptr, size);

    return ptr;
}

void daud_heap_free(void *ptr)
{
    DAUD_TRACE(1, "[%s]: ptr=%x", __func__, (int)ptr);
    if (ptr) {
        heap_free(daud_heap_handle, ptr);
    }
}
#endif

#ifdef DMA_AUDIO_SW_RESAMPLE
void daud_sw_resample_init(struct DAUD_RESAMPLE_T *daud_resample)
{
    dma_audio_resample_init(daud_resample);
}

int daud_sw_resample_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct DAUD_STREAM_CONFIG_T *out_cfg, struct DAUD_STREAM_CONFIG_T *in_cfg)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_RESAMPLE_T *daud_resample = &daud_role->resample;
    int ret;

    DAUD_DEBUG_TRACE2(id, stream);
    ret = dma_audio_resample_open(daud_resample, out_cfg, in_cfg);
    ASSERT(ret==0, "[%s]: failed, ret=%d", __func__, ret);
    return 0;
}

void daud_sw_resample_dump(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_RESAMPLE_T *daud_resample = &daud_role->resample;

    DAUD_DEBUG_TRACE2(id, stream);
    dma_audio_resample_dump(daud_resample);
}

int daud_sw_resample_enable(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_RESAMPLE_T *daud_resample = &daud_role->resample;
    int ret;

    DAUD_DEBUG_TRACE2(id, stream);
    ret = dma_audio_resample_enable(daud_resample);
    ASSERT(ret==0, "[%s]: failed, ret=%d", __func__, ret);
    return 0;
}

int daud_sw_resample_disable(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_RESAMPLE_T *daud_resample = &daud_role->resample;
    int ret;

    DAUD_DEBUG_TRACE2(id, stream);
    ret = dma_audio_resample_disable(daud_resample);
    ASSERT(ret==0, "[%s]: failed, ret=%d", __func__, ret);
    return 0;
}

int daud_sw_resample_reset(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_RESAMPLE_T *daud_resample = &daud_role->resample;
    int ret;

    DAUD_DEBUG_TRACE2(id, stream);
    ret = dma_audio_resample_reset(daud_resample);
    ASSERT(ret==0, "[%s]: failed, ret=%d", __func__, ret);
    return 0;
}

int daud_sw_resample_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_RESAMPLE_T *daud_resample = &daud_role->resample;
    int ret;

    DAUD_DEBUG_TRACE2(id, stream);
    ret = dma_audio_resample_close(daud_resample);
    ASSERT(ret==0, "[%s]: failed, ret=%d", __func__, ret);
    return 0;
}

int daud_sw_resample_run(struct DAUD_RESAMPLE_T *daud_resample, uint8_t *out_buf, uint32_t out_size)
{
    daud_resample->rs_data_out_buf = out_buf;
//    daud_resample->rs_data_out_size = out_size;
    return dma_audio_resample_run(daud_resample);
}
#endif /* DMA_AUDIO_SW_RESAMPLE */

static void daud_data_fast_memcpy(uint8_t *dst, uint8_t *src, uint32_t len)
{
    uint32_t *pd = (uint32_t *)dst;
    uint32_t *ps = (uint32_t *)src;
    uint32_t bsize = len / 4;
    uint32_t remain = len % 4;
    uint32_t i;

    if (bsize > 0) {
        for (i = 0; i < bsize; i += 8) {
            pd[0] = ps[0];
            pd[1] = ps[1];
            pd[2] = ps[2];
            pd[3] = ps[3];
            pd[4] = ps[4];
            pd[5] = ps[5];
            pd[6] = ps[6];
            pd[7] = ps[7];
            pd += 8;
            ps += 8;
        }
    }
    if (remain > 0) {
        uint8_t *d = (uint8_t *)pd;
        uint8_t *s = (uint8_t *)ps;
        for (i = 0; i < remain; i++) {
            *d++ = *s++;
        }
    }
}

void daud_setup_memcpy_callback(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct DAUD_STREAM_CONFIG_T *out_cfg, struct DAUD_STREAM_CONFIG_T *in_cfg)
{
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    daud_stream_data_memcpy_cb_t memcpy_cb = NULL;
    enum AUD_BITS_T in_bits, out_bits;
    enum AUD_CHANNEL_NUM_T in_chan_num, out_chan_num;

    in_bits      = in_cfg->bits;
    in_chan_num  = in_cfg->channel_num;
    out_bits     = out_cfg->bits;
    out_chan_num = out_cfg->channel_num;

    DAUD_TRACE(1, "[%s]: id=%d, stream=%d", __func__, id, stream);
    DAUD_TRACE(1, "in_stream : bits=%d, chan_num=%d", in_bits, in_chan_num);
    DAUD_TRACE(1, "out_stream: bits=%d, chan_num=%d", out_bits, out_chan_num);

    if (0) {
    } else if ((in_bits == 32) && (in_chan_num == 2)) {
        if (0) {
        } else if ((out_bits == 24) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i24_2ch_from_i32_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i24_2ch_from_i32_2ch");
        } else if ((out_bits == 24) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i24_1ch_from_i32_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i24_1ch_from_i32_2ch");
        } else if ((out_bits == 16) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i16_2ch_from_i32_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_2ch_from_i32_2ch");
        } else if ((out_bits == 16) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i16_1ch_from_i32_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_1ch_from_i32_2ch");
        }
    } else if ((in_bits == 32) && (in_chan_num == 4)) {
        if (0) {
        } else if ((out_bits == 32) && (out_chan_num == 8)) {
            memcpy_cb = aud_data_memcpy_i32_8ch_from_i32_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i32_8ch_from_i32_4ch");
        }
    } else if ((in_bits == 24) && (in_chan_num == 2)) {
        if (0) {
        } else if ((out_bits == 24) && (out_chan_num == 2)) {
            memcpy_cb = daud_data_fast_memcpy;
            DAUD_TRACE(1, "memcpy_cb = daud_data_fast_memcpy");
        } else if ((out_bits == 24) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i24_1ch_from_i32_l24_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i24_1ch_from_i32_l24_2ch");
        } else if ((out_bits == 16) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i16_2ch_from_i32_l24_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_2ch_from_i32_l24_2ch");
        } else if ((out_bits == 16) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i16_1ch_from_i32_l24_2ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_1ch_from_i32_l24_2ch");
        }
    } else if ((in_bits == 24) && (in_chan_num == 4)) {
        if (0) {
        } else if ((out_bits == 24) && (out_chan_num == 4)) {
            memcpy_cb = daud_data_fast_memcpy;
        } else if ((out_bits == 24) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i24_2ch_from_i32_l24_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i24_2ch_from_i32_l24_4ch");
        } else if ((out_bits == 24) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i24_1ch_from_i32_l24_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i24_1ch_from_i32_l24_4ch");
        } else if ((out_bits == 16) && (out_chan_num == 4)) {
            memcpy_cb = aud_data_memcpy_i16_4ch_from_i32_l24_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_4ch_from_i32_l24_4ch");
        } else if ((out_bits == 16) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i16_2ch_from_i32_l24_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_2ch_from_i32_l24_4ch");
        } else if ((out_bits == 16) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i16_1ch_from_i32_l24_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_1ch_from_i32_l24_4ch");
        }
    } else if ((in_bits == 16) && (in_chan_num == 1)) {
        if (0) {
        } else if ((out_bits == 16) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i16_mono_from_i16_mono;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_mono_from_i16_mono");
        }
    } else if ((in_bits == 16) && (in_chan_num == 2)) {
        if (0) {
        } else if ((out_bits == 16) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_stereo;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_stereo");
        } else if ((out_bits == 16) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i16_mono_from_i16_stereo;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_mono_from_i16_stereo");
        }
    } else if ((in_bits == 16) && (in_chan_num == 4)) {
        if (0) {
        } else if ((out_bits == 16) && (out_chan_num == 2)) {
            memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_stereo_from_i16_4ch");
        } else if ((out_bits == 16) && (out_chan_num == 1)) {
            memcpy_cb = aud_data_memcpy_i16_mono_from_i16_4ch;
            DAUD_TRACE(1, "memcpy_cb = aud_data_memcpy_i16_mono_from_i16_4ch");
        }
    } else {
        ASSERT(false, "[%s]: Bad in_bits=%d, in_chan_num=%d", __func__, in_bits, in_chan_num);
    }

    ASSERT(memcpy_cb != NULL, "[%s]: null ptr", __func__);
    daud_role->ctl.memcpy_cb = memcpy_cb;
}

static void daud_transfer_data_handler(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    static struct DMA_RPC_STREAM_PARAM_T stream_param;

    struct af_stream_cfg_t *af_role = NULL;
    struct daud_stream_cfg_t *daud_role = NULL;
    uint32_t seq, max_num;
    uint32_t dma_addr;
    uint8_t dma_chan;
    uint32_t trans_dma_addr;
    uint8_t trans_dma_chan;
    bool notify = false;

    af_role = daud_get_af_stream_role(id, stream);
    daud_role = daud_get_stream_role(id, stream);
    dma_chan = af_role->dma_cfg.ch;
    dma_addr = daud_trans_stream_get_cur_dma_addr(id, stream);
    trans_dma_chan = dma_chan;
    trans_dma_addr = dma_addr;

    daud_role->ctl.frm_seq++;
    seq = daud_role->ctl.frm_seq;
    max_num = daud_role->ctl.frm_seq_max_num;
    ASSERT(seq <= max_num, "[%s]: Bad seq=%d, max_num=%d", __func__, seq, max_num);
    if (seq == max_num) {
        daud_role->ctl.frm_seq = 0;
        notify = true;
    }
    // copy data to dst buffer for forward stream when max_num > 1
    if (max_num > 1) {
        uint8_t *buf;
        uint32_t len;
        uint32_t pp_index = PP_PING;
        uint32_t dst_pp_index = daud_role->ctl.dst_pp_index;
        uint32_t hw_pos;
        daud_stream_data_memcpy_cb_t memcpy_cb = daud_role->ctl.memcpy_cb;

        if (dma_addr == 0) {
            hw_pos = 0;
        } else {
            hw_pos = dma_addr - (uint32_t)(daud_role->ctl.dma_buf_ptr);
        }
        if (hw_pos < daud_role->ctl.dma_buf_size / 2) {
            pp_index = PP_PANG;
        } else if (hw_pos < daud_role->ctl.dma_buf_size) {
            pp_index = PP_PING;
        }
        if (pp_index == PP_PING) {
            buf = daud_role->ctl.dma_buf_ptr;
        } else {
            buf = daud_role->ctl.dma_buf_ptr + daud_role->ctl.dma_buf_size / 2;
        }
        len = daud_role->ctl.dma_buf_size / 2;

        uint32_t dst_dsize     = daud_role->ctl.dst_dsize;
        uint8_t *dst_buf_start = daud_role->ctl.dst_buf_ptr;
        uint32_t dst_buf_size  = daud_role->ctl.dst_buf_size;
        uint8_t *dst_buf_end   = daud_role->ctl.dst_buf_end;
        uint8_t *dst_buf       = 0;
        uint32_t dst_offs;

        if (daud_role->ctl.op_flag & DAUD_OP_FLAG_SW_RESAMPLE) {
            dst_dsize = daud_role->resample.rs_data_out_size;
        } else {
            dst_dsize = daud_role->ctl.dst_dsize;
        }
        dst_offs = dst_dsize * (seq - 1);
        if (dst_pp_index == PP_PING) {
            dst_buf = dst_buf_start + dst_offs;
        } else {
            dst_buf = dst_buf_start + dst_buf_size / 2 + dst_offs;
        }
#if defined(DAUD_DEBUG_VERBOSE) && (DAUD_DEBUG_VERBOSE > 1)
        TRACE(1, "seq[%d]: notify=%d,dst_buf=%x,dst_offs=%d,buf=%x,len=%d,hw_pos=%d",
            seq, notify, (int)dst_buf, dst_offs, (int)buf, len, hw_pos);
#endif
        if (dst_buf >= dst_buf_end || dst_buf < dst_buf_start) {
            ASSERT(false, "[%s][%d]: Bad dst_buf=%x,start=%x,end=%x",
                __func__, __LINE__, (int)dst_buf, (int)dst_buf_start, (int)dst_buf_end);
        }

        ASSERT(memcpy_cb != NULL, "[%s][%d]: memcpy_cb is null ptr", __func__, __LINE__);
#ifdef DMA_AUDIO_SW_RESAMPLE
        // resample src data
        if (daud_role->ctl.op_flag & DAUD_OP_FLAG_SW_RESAMPLE) {
            uint8_t *in_buf  = daud_role->resample.rs_data_in_buf;
            uint32_t in_size = daud_role->resample.rs_data_in_size;
            uint8_t *out_buf = dst_buf;
            uint32_t out_size = dst_dsize;

            ASSERT(in_buf != NULL, "[%s][%d]: in_buf is null", __func__, __LINE__);
            ASSERT(in_size <= len, "[%s][%d]: in_size error: in_size=%d, len=%d",
                    __func__, __LINE__, in_size, len);

            // pre-process data before resample
            memcpy_cb(in_buf, buf, len);

            // sw resample
            daud_sw_resample_run(&daud_role->resample, out_buf, out_size);
        } else
#endif
        {
            // copy data to dst buffer
            memcpy_cb(dst_buf, buf, len);
        }

        if (notify) {
            // set dma position and channel
            uint32_t pp_addr  = (uint32_t)dst_buf_start;
            uint32_t pp_offs  = hw_pos % len;

            if (dst_pp_index == PP_PING) {
                pp_addr = (uint32_t)(dst_buf_start + dst_buf_size / 2);
            } else {
                pp_addr = (uint32_t)dst_buf_start;
            }
            trans_dma_addr = pp_addr + pp_offs;
            if (trans_dma_addr >= (uint32_t)dst_buf_end) {
                trans_dma_addr = (uint32_t)dst_buf_start;
            }

#if defined(DAUD_DEBUG_VERBOSE) && (DAUD_DEBUG_VERBOSE > 0)
            TRACE(1, "trans_dma_addr=%x,pp_index[%d]:pp_addr=%x,pp_offs=%x",
                trans_dma_addr, dst_pp_index, pp_addr, pp_offs);
#endif

            if (trans_dma_addr >= (uint32_t)dst_buf_end
                || trans_dma_addr < (uint32_t)dst_buf_start) {
                ASSERT(false, "[%s]: Bad trans_dma_addr=%x,start=%x,end=%x",
                    __func__, (int)trans_dma_addr, (int)dst_buf_start, (int)dst_buf_end);
            }
            trans_dma_chan = dma_chan;
            daud_role->ctl.dst_pp_index ^= 1;
        }
    }
    else if (max_num == 1) {
        // no need to copy data when max_num=1
        trans_dma_chan = dma_chan;
        trans_dma_addr = dma_addr;
    }
    if (notify) {
        stream_param.id = id;
        stream_param.stream = stream;
        stream_param.dma_chan = trans_dma_chan;
        stream_param.dma_addr = trans_dma_addr;
        dma_rpc_stream_data_transfer(&stream_param);
    }
}

static void daud_thread_stream_handler(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct af_stream_cfg_t *af_role;
    struct daud_stream_cfg_t *daud_role;

    daud_lock_thread();

    af_role = daud_get_af_stream_role(id, stream);
    daud_role = daud_get_stream_role(id, stream);

    if (daud_role->ctl.forward) {
        //transfer stream
        daud_transfer_data_handler(id, stream);
    } else {
        //process local stream
        duad_stream_data_handler(af_role, id, stream);
    }

    daud_unlock_thread();
}

static void daud_send_signal(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
#ifdef RTOS
    osSignalSet(daud_thread_tid, 0x01 << (id * 2 + stream));
#else
    daud_set_flag(&daud_flag_signal, 0x01 << (id * 2 + stream));
    hal_cpu_wake_lock(DAUD_CPU_WAKE_USER);
#endif
}

static void daud_dma_irq_handler(uint8_t chan, uint32_t remain_tsize,
    uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    struct af_stream_cfg_t *af_role = NULL;
    struct daud_stream_cfg_t *daud_role = NULL;
    bool irq_mode = false;
    bool forward = false;
    uint8_t id, stream;
    uint32_t mask;

#if 0
    TRACE(1, "DMA IRQ HDLR: chan=%d, remain_tsize=%d, error=%d, lli=%x",
        chan, remain_tsize, error, (int)lli);
#endif

    mask = AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE | AF_STATUS_STREAM_START_STOP;
    for (id = 0; id < AUD_STREAM_ID_NUM; id++) {
        for (stream = 0; stream < AUD_STREAM_NUM; stream++) {
            daud_role = daud_get_stream_role(id, stream);
            if (daud_role->status != mask) {
                continue;
            }
            forward = daud_role->ctl.forward;
            irq_mode = daud_role->cfg.irq_mode;
            af_role = daud_get_af_stream_role(id, stream);

            // process forward stream
            if (forward && (daud_role->ctl.dma_hw_chan == chan)) {
                if (irq_mode) {
                    daud_transfer_data_handler(id, stream);
                } else {
                    daud_send_signal(id, stream);
                }
            }
            // process daud stream
            if (af_role->dma_cfg.ch == chan) {
                af_role->ctl.pp_cnt++;
                if (daud_irq_notif) {
                    daud_irq_notif(id, stream);
                }
                if (irq_mode) {
                    duad_stream_data_handler(af_role, id, stream);
                } else {
                    daud_send_signal(id, stream);
                }
                return;
            }
        }
    }
    DAUD_WARN(0, "[%s] ERROR: channel id=%d", __func__, chan);
    //ASSERT(0, "[%s] ERROR: channel id=%d", __func__, chan);
}

static void daud_dump_stream_cfg(int id, int stream, struct daud_stream_cfg_t *cfg)
{
    TRACE(1, "[daud_stream_cfg][%d][%d]: cfg=%x", id, stream, (int)cfg);
    TRACE(1, "sample_rate       =%d", cfg->cfg.sample_rate);
    TRACE(1, "bits              =%d", cfg->cfg.bits);
    TRACE(1, "channel_map       =%x", cfg->cfg.channel_map);
    TRACE(1, "channel_num       =%d", cfg->cfg.channel_num);
    TRACE(1, "device            =%d", cfg->cfg.device);
    TRACE(1, "vol               =%d", cfg->cfg.vol);
    TRACE(1, "io_path           =%d", cfg->cfg.io_path);
    TRACE(1, "data_ptr          =%x", (int)cfg->cfg.data_ptr);
    TRACE(1, "data_size         =%d", cfg->cfg.data_size);
    TRACE(1, "data_handler      =%x", (int)cfg->cfg.handler);
    TRACE(1, "irq_mode          =%d", cfg->cfg.irq_mode);
    TRACE(1, "forward           =%d", cfg->ctl.forward);
    TRACE(1, "cli_open          =%d", cfg->ctl.cli_state.open);
    TRACE(1, "cli_start         =%d", cfg->ctl.cli_state.start);
    TRACE(1, "op_flag           =%x", cfg->ctl.op_flag);
    TRACE(1, "dma_hw_chan       =%d", cfg->ctl.dma_hw_chan);
    TRACE(1, "dma_vir_chan      =%d", cfg->ctl.dma_vir_chan);
    TRACE(1, "frm_seq_max_num   =%d", cfg->ctl.frm_seq_max_num);
    TRACE(1, "frm_seq           =%d", cfg->ctl.frm_seq);
    TRACE(1, "dst_buf_ptr       =%x", (int)(cfg->ctl.dst_buf_ptr));
    TRACE(1, "dst_buf_end  =%x", (int)(cfg->ctl.dst_buf_end));
    TRACE(1, "dst_buf_size      =%d", (int)(cfg->ctl.dst_buf_size));
    TRACE(1, "dma_buf_ptr       =%x", (int)(cfg->ctl.dma_buf_ptr));
    TRACE(1, "dma_buf_size      =%d", (int)(cfg->ctl.dma_buf_size));
    TRACE(1, "------------end");
}

static void daud_dump_af_stream_cfg(int id, int stream, struct af_stream_cfg_t *cfg)
{
    TRACE(1, "[af_stream_cfg][%d][%d]: cfg=%x", id, stream, (int)cfg);
    TRACE(1, "sample_rate         =%d", cfg->cfg.sample_rate);
    TRACE(1, "bits                =%d", cfg->cfg.bits);
    TRACE(1, "channel_map         =%x", cfg->cfg.channel_map);
    TRACE(1, "channel_num         =%d", cfg->cfg.channel_num);
    TRACE(1, "device              =%d", cfg->cfg.device);
    TRACE(1, "vol                 =%d", cfg->cfg.vol);
    TRACE(1, "io_path             =%d", cfg->cfg.io_path);
    TRACE(1, "data_ptr            =%x", (int)cfg->cfg.data_ptr);
    TRACE(1, "data_size           =%d", cfg->cfg.data_size);
    TRACE(1, "data_handler        =%x", (int)cfg->cfg.handler);
    TRACE(1, "handler             =%x", (int)cfg->handler);
    TRACE(1, "dma_base:           =%x", cfg->dma_base);
    TRACE(1, "dma_buf_ptr         =%x", (int)cfg->dma_buf_ptr);
    TRACE(1, "dma_buf_size        =%d", cfg->dma_buf_size);
    TRACE(1, "dma_cfg: ch         =%d", cfg->dma_cfg.ch);
    TRACE(1, "dma_cfg: src        =%x", cfg->dma_cfg.src);
    TRACE(1, "dma_cfg: src_periph =%d", cfg->dma_cfg.src_periph);
    TRACE(1, "dma_cfg: src_tsize  =%d", cfg->dma_cfg.src_tsize);
    TRACE(1, "dma_cfg: src_width  =%d", cfg->dma_cfg.src_width);
    TRACE(1, "dma_cfg: src_bsize  =%d", cfg->dma_cfg.src_bsize);
    TRACE(1, "dma_cfg: dst        =%x", cfg->dma_cfg.dst);
    TRACE(1, "dma_cfg: dst_periph =%d", cfg->dma_cfg.dst_periph);
    TRACE(1, "dma_cfg: dst_width  =%d", cfg->dma_cfg.dst_width);
    TRACE(1, "dma_cfg: dst_bsize  =%d", cfg->dma_cfg.dst_bsize);
    TRACE(1, "------------end");
}

static void dma_sync_ctl_data(void *param)
{
    struct DMA_RPC_REQ_MSG_T *msg = (struct DMA_RPC_REQ_MSG_T *)param;
    enum AUD_STREAM_ID_T id;
    enum AUD_STREAM_T stream;
    struct af_stream_cfg_t *af_role, *dst;
    uint32_t role_size = 0;
    uint32_t af_cfg_size = sizeof(struct af_stream_cfg_t);
    uint32_t lock;

    ASSERT(msg != NULL, "[%s]: null msg ptr", __func__);

    id        = (enum AUD_STREAM_ID_T)(msg->param[0]);
    stream    = (enum AUD_STREAM_T)(msg->param[1]);
    af_role   = (struct af_stream_cfg_t *)(msg->param[2]);
    role_size = msg->param[3];

    DAUD_TRACE(1, "[%s]: id=%d,stream=%d,af_role=%x,role_size=%d", __func__,
        id, stream, (int)af_role, role_size);

    ASSERT(id < AUD_STREAM_ID_NUM, "[%s]: Bad stream id=%d", __func__, id);
    ASSERT(stream < AUD_STREAM_NUM, "[%s]: Bad stream=%d", __func__, stream);
    ASSERT(af_role != NULL, "[%s]: null af role ptr", __func__);
    ASSERT(role_size == af_cfg_size, "[%s]: Bad role_size=%d, af_cfg_size=%d",__func__, role_size, af_cfg_size);

    dst = &daud_af_stream[id][stream];
    lock = int_lock();
    memcpy((void *)dst, (void *)af_role, role_size);
    int_unlock(lock);

    daud_dump_af_stream_cfg(id, stream, dst);

    // set forward stream dma channel
    uint16_t virtual = RPC_ID_VIR_ATTR(msg->rpc_id);
    bool forward = !!virtual;
    if (forward) {
        uint8_t hw_chan;
        uint8_t vir_chan = af_role->dma_cfg.ch;
        enum HAL_DMA_PERIPH_T periph;
        struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);

        ASSERT(daud_role->ctl.forward == true, "[%s]: forward status error, id=%d, stream=%d",
            __func__, id, stream);

        if (stream == AUD_STREAM_PLAYBACK) {
            periph = af_role->dma_cfg.dst_periph;
        } else {
            periph = af_role->dma_cfg.src_periph;
        }
        hw_chan = dma_rpc_get_dma_hw_chan(periph);
        ASSERT(hw_chan != HAL_DMA_CHAN_NONE, "[%s]: Bad dma hw chan=%x, id=%d, stream=%d",
            __func__, hw_chan, id, stream);

        lock = int_lock();
        daud_role->ctl.dma_hw_chan = hw_chan;
        daud_role->ctl.dma_vir_chan = vir_chan;
        int_unlock(lock);

        daud_dump_stream_cfg(id, stream, daud_role);
    }
}

static void daud_dma_event_handler(enum DAUD_EVT_T evt, uint8_t chan)
{
    if (daud_evt_hdlr[DAUD_EVT_HDLR_ID_DMA]) {
        daud_evt_hdlr[DAUD_EVT_HDLR_ID_DMA](evt, (uint32_t)chan, 0, 0);
    }
}

static void daud_stream_event_handler(enum DAUD_EVT_T evt, enum AUD_STREAM_ID_T id,
    enum AUD_STREAM_T stream, void *cfg, bool forward)
{
    uint32_t flag = 0;

    if (forward) {
        flag |= DAUD_STREAM_FLAG_FORWARD;
    }
    if (daud_evt_hdlr[DAUD_EVT_HDLR_ID_STREAM]) {
        daud_evt_hdlr[DAUD_EVT_HDLR_ID_STREAM](evt, (uint32_t)id, (uint32_t)stream | flag, (uint32_t)cfg);
    }
}

void daud_set_event_handler(enum DAUD_EVT_HDLR_ID_T id, DAUD_EVT_HDLR_T handler)
{
    ASSERT(id < DAUD_EVT_HDLR_ID_QTY, "%s: inavlid id=%d", __func__, id);
    daud_evt_hdlr[id] = handler;
}

void daud_set_algo_config_notify(DAUD_ALGO_CFG_NOTIFY_T notify)
{
    daud_algo_cfg_notify = notify;
}

static void daud_dma_rpc_event_handler(enum DMA_RPC_EVT_T evt, void *param)
{
    struct DMA_RPC_REQ_MSG_T *msg = (struct DMA_RPC_REQ_MSG_T *)param;
    POSSIBLY_UNUSED uint16_t virtual = RPC_ID_VIR_ATTR(msg->rpc_id);
    POSSIBLY_UNUSED uint16_t rpc_id = RPC_ID_VAL_ATTR(msg->rpc_id);
    bool forward = !!virtual;

    DAUD_TRACE(1, "%s: evt=%d, rpc_id=%d, forward=0x%x", __func__, evt, rpc_id, forward);

    switch (evt) {
    case DMA_RPC_EVT_SYNC_CTL_DATA:
        dma_sync_ctl_data(param);
        break;
    case DMA_RPC_EVT_DMA_INIT:
    case DMA_RPC_EVT_DMA_START:
    {
        const struct HAL_DMA_CH_CFG_T *cfg = (const struct HAL_DMA_CH_CFG_T *)msg->param[1];
        daud_dma_event_handler((enum DAUD_EVT_T)evt, cfg->ch);
    }
        break;
    case DMA_RPC_EVT_DMA_STOP:
    case DMA_RPC_EVT_DMA_GET_CHAN:
    {
        uint8_t ch = msg->param[0];
        daud_dma_event_handler((enum DAUD_EVT_T)evt, ch);
    }
        break;
    case DMA_RPC_EVT_STREAM_FADEIN:
    {
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];
        dma_audio_set_fadein_time(msg->param[2]);
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, NULL, forward);
    }
        break;
    case DMA_RPC_EVT_STREAM_FADEOUT:
    {
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];
        dma_audio_set_fadeout_time(msg->param[2]);
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, NULL, forward);
    }
        break;
    case DMA_RPC_EVT_STREAM_OPEN:
    {
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];
        struct AF_STREAM_CONFIG_T *cfg = (struct AF_STREAM_CONFIG_T *)msg->param[2];

        daud_client_stream_update_state(id ,stream, evt);
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, cfg, forward);
    }
        break;
    case DMA_RPC_EVT_STREAM_CLOSE:
    case DMA_RPC_EVT_STREAM_START:
    case DMA_RPC_EVT_STREAM_STOP:
    {
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];

        daud_client_stream_update_state(id ,stream, evt);
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, NULL, forward);
    }
        break;
    case DMA_RPC_EVT_STREAM_GET_CFG:
    {
        int err = 0;
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];
        struct DAUD_STREAM_CONFIG_T *dst_cfg = (struct DAUD_STREAM_CONFIG_T *)(msg->param[2]);

        if (dst_cfg) {
            dma_audio_stream_get_config(stream, dst_cfg);
        } else {
            err = 1;
        }
        msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1] = err;
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, NULL, forward);
    }
        break;
    case DMA_RPC_EVT_STREAM_SET_CFG:
    {
        int err = 0;
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];
        struct DAUD_STREAM_CONFIG_T *src_cfg = (struct DAUD_STREAM_CONFIG_T *)(msg->param[2]);

        if (src_cfg) {
            ASSERT(src_cfg->data_size != 0, "[%s] data_size is 0", __func__);
            dma_audio_stream_set_config(stream, src_cfg);
        } else {
            err = 1;
        }
        msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1] = err;
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, NULL, forward);
    }
        break;
    case DMA_RPC_EVT_STREAM_GET_ALGO_CFG:
    case DMA_RPC_EVT_STREAM_SET_ALGO_CFG:
    {
        int err = 0;
        enum AUD_STREAM_ID_T id  = msg->param[0];
        enum AUD_STREAM_T stream = msg->param[1];

        if (daud_algo_cfg_notify) {
            err = daud_algo_cfg_notify(id, stream, (void *)msg->param[2], msg->param[3], true);
        }
        msg->param[DMA_RPC_REQ_MSG_PARAM_NUM-1] = err;
        daud_stream_event_handler((enum DAUD_EVT_T)evt, id, stream, NULL, forward);
    }
        break;
    default:
        break;
    }
}

static unsigned int daud_rpcif_rx_handler(const void *data, unsigned int len)
{
    struct DMA_MSG_HDR_T *hdr = (struct DMA_MSG_HDR_T *)data;
    uint32_t req_len = sizeof(struct DMA_RPC_REQ_MSG_T);
    uint32_t rsp_len = sizeof(struct DMA_RPC_REPLY_MSG_T);

    if (((len != req_len) && (len != rsp_len)) || (hdr->id != DMA_MSG_ID_RPC)) {
#if defined(DAUD_DEBUG) && (DAUD_DEBUG > 1)
        TRACE(1, "[%s]: recv unknown msg,%x/%d", __func__,(int)data,len);
#endif
        return 0;
    }
    dma_rpcif_rx_handler(data, len);
    return len;
}

static void daud_rpcif_tx_handler(const void *data, unsigned int len)
{
    struct DMA_MSG_HDR_T *hdr = (struct DMA_MSG_HDR_T *)data;
    uint32_t req_len = sizeof(struct DMA_RPC_REQ_MSG_T);
    uint32_t rsp_len = sizeof(struct DMA_RPC_REPLY_MSG_T);

    if (((len != req_len) && (len != rsp_len)) || (hdr->id != DMA_MSG_ID_RPC)) {
#if defined(DAUD_DEBUG) && (DAUD_DEBUG > 1)
        TRACE(1, "[%s]: send unknown msg done,%x/%d", __func__,(int)data,len);
#endif
        return;
    }
    dma_rpcif_tx_handler(data, len);
}

static void daud_if_init_rpc_handler(void)
{
    DMA_RPC_SEND_CMD_HANDLER_T send_handler = NULL;
    enum CORE_IRQ_HDLR_ID_T core_irq_hdlr_id = CORE_IRQ_HDLR_PRIO_HIGH;

    // core irq handler layer
#if defined(CHIP_ROLE_CP)
    // CP
    cp_subsys_core_register_rx_irq_handler(core_irq_hdlr_id, daud_rpcif_rx_handler);
    cp_subsys_core_register_tx_done_irq_handler(core_irq_hdlr_id, daud_rpcif_tx_handler);
    send_handler = cp_subsys_send;
#elif defined(CHIP_SUBSYS_SENS)
    // sensor hub
    sensor_hub_core_register_rx_irq_handler(core_irq_hdlr_id, daud_rpcif_rx_handler);
    sensor_hub_core_register_tx_done_irq_handler(core_irq_hdlr_id, daud_rpcif_tx_handler);
    send_handler = sensor_hub_send;
#else
    // M55
    dsp_m55_core_register_rx_irq_handler(core_irq_hdlr_id, daud_rpcif_rx_handler);
    dsp_m55_core_register_tx_done_irq_handler(core_irq_hdlr_id, daud_rpcif_tx_handler);
    send_handler = dsp_m55_send;
#endif

    // dma rpc stream driver layer
    dma_rpcif_setup_send_cmd_handler(send_handler);
    dma_rpc_setup_evt_handler(daud_dma_rpc_event_handler);
    dma_rpc_setup_irq_handler(DMA_RPC_IRQ_HDLR_ID_0, daud_dma_irq_handler);
}

uint32_t daud_open(void)
{
    DAUD_DEBUG_TRACE();

#ifdef RTOS
    if (daud_mutex_id == NULL) {
        daud_mutex_id = osMutexCreate((osMutex(daud_mutex)));
    }
#endif

    daud_lock_thread();

    struct af_stream_cfg_t *af_role = NULL;
    struct daud_stream_cfg_t *daud_role;

    //initial parameters
    for(uint8_t id=0; id< AUD_STREAM_ID_NUM; id++)
    {
        for(uint8_t stream=0; stream < AUD_STREAM_NUM; stream++)
        {
            // init af stream role
            af_role = daud_get_af_stream_role((enum AUD_STREAM_ID_T)id, (enum AUD_STREAM_T)stream);
            if(af_role->ctl.status == AF_STATUS_NULL) {
                af_role->dma_buf_ptr    = NULL;
                af_role->dma_buf_size   = 0;
                af_role->ctl.status     = AF_STATUS_OPEN_CLOSE;
                af_role->ctl.use_device = AUD_STREAM_USE_DEVICE_NULL;
                af_role->dma_cfg.ch     = HAL_DMA_CHAN_NONE;
            } else {
                ASSERT(0, "[%s] ERROR af status: id = %d, stream = %d", __func__, id, stream);
            }
            // init dma audio stream role
            daud_role = daud_get_stream_role(id, stream);
            if (daud_role->status == AF_STATUS_NULL) {
                daud_role->status = AF_STATUS_OPEN_CLOSE;
                daud_role->cfg.irq_mode = false;
                daud_role->ctl.forward = false;
                daud_role->ctl.dma_hw_chan = HAL_DMA_CHAN_NONE;
                daud_role->ctl.dma_vir_chan = HAL_DMA_CHAN_NONE;
                daud_role->ctl.op_flag = DAUD_OP_FLAG_NONE;
                daud_role->ctl.frm_seq_max_num = 0;
                daud_role->ctl.frm_seq = 0;
                daud_role->ctl.dst_pp_index = 0;
                daud_role->ctl.dst_buf_ptr = NULL;
                daud_role->ctl.dst_buf_end = NULL;
                daud_role->ctl.dst_buf_size = 0;
                daud_role->ctl.dst_dsize = 0;
                daud_role->ctl.dma_buf_ptr = NULL;
                daud_role->ctl.dma_buf_size = 0;
                daud_client_stream_init_state(&daud_role->ctl.cli_state);
#ifdef DMA_AUDIO_SW_RESAMPLE
                daud_sw_resample_init(&daud_role->resample);
#endif
            } else {
                ASSERT(0, "[%s] ERROR daud status: id = %d, stream = %d", __func__, id, stream);
            }
        }
    }

#ifdef RTOS
    if (daud_thread_tid == NULL) {
        daud_thread_tid = osThreadCreate(osThread(daud_thread), NULL);
        daud_default_priority = daud_get_priority();
        osSignalSet(daud_thread_tid, 0x0);
    }
#endif

#ifdef DAUD_USE_HEAP
    daud_heap_init();
#endif

    daud_if_init_rpc_handler();

    daud_unlock_thread();

    DAUD_DEBUG_DONE();

    return DAUD_RES_SUCCESS;
}

uint32_t daud_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    const struct DAUD_STREAM_CONFIG_T *cfg)
{
    uint32_t ret = DAUD_RES_FAILED;
    struct daud_stream_cfg_t *role = daud_get_stream_role(id, stream);

    DAUD_DEBUG_TRACE();

    daud_lock_thread();

    if (role->status != AF_STATUS_OPEN_CLOSE) {
        TRACE(1, "[%s]: ERROR: status=%d", __func__, role->status);
        goto _exit;
    }

    role->cfg = *cfg;
    role->ctl.dma_buf_ptr = cfg->data_ptr;
    role->ctl.dma_buf_size = cfg->data_size;

#ifndef DMA_AUDIO_APP_DYN_ON
    dma_rpcif_open(id, stream);
#endif
    daud_set_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);
#ifndef RTOS
    daud_clear_flag(&daud_flag_signal, 1<<(id*2+stream));
    daud_set_flag(&daud_flag_open, 1<<(id*2+stream));
#endif

    DAUD_DEBUG_DONE();
    ret = DAUD_RES_SUCCESS;

_exit:
    daud_unlock_thread();

    return ret;
}

uint32_t daud_stream_setup(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    const struct DAUD_STREAM_CONFIG_T *cfg)
{
    return 0;
}

uint32_t daud_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t ret = DAUD_RES_FAILED;
    struct daud_stream_cfg_t *role = daud_get_stream_role(id, stream);

    DAUD_DEBUG_TRACE();

    daud_lock_thread();
    if (role->status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) {
        TRACE(1, "[%s] ERROR: status=%x", __func__, role->status);
        goto _exit;
    }

#ifndef RTOS
    daud_clear_flag(&daud_flag_signal, 1<<(id*2+stream));
#endif
    daud_set_status(id, stream, AF_STATUS_STREAM_START_STOP);

    DAUD_DEBUG_DONE();
    ret = DAUD_RES_SUCCESS;

_exit:
    daud_unlock_thread();
    return ret;
}

uint32_t daud_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t ret = DAUD_RES_FAILED;
    struct daud_stream_cfg_t *role = daud_get_stream_role(id, stream);

    DAUD_DEBUG_TRACE();

    daud_lock_thread();

    if (role->status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE | AF_STATUS_STREAM_START_STOP)) {
        TRACE(1, "[%s] ERROR: status=%x", __func__, role->status);
        goto _exit;
    }

#ifndef RTOS
    daud_clear_flag(&daud_flag_signal, 1<<(id*2+stream));
#endif
    daud_clear_status(id, stream, AF_STATUS_STREAM_START_STOP);

    DAUD_DEBUG_DONE();
    ret = DAUD_RES_SUCCESS;

_exit:
    daud_unlock_thread();
    return ret;
}

uint32_t daud_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t ret = DAUD_RES_FAILED;
    struct daud_stream_cfg_t *role = daud_get_stream_role(id, stream);

    DAUD_DEBUG_TRACE();

    daud_lock_thread();

    if (role->status != (AF_STATUS_OPEN_CLOSE|AF_STATUS_STREAM_OPEN_CLOSE)) {
        TRACE(1, "[%s]: ERROR: status=%d", __func__, role->status);
        goto _exit;
    }

#ifndef DMA_AUDIO_APP_DYN_ON
    dma_rpcif_close(id, stream);
#endif
    daud_clear_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);

#ifndef RTOS
    daud_clear_flag(&daud_flag_open, 1<<(id*2+stream));
    if (daud_flag_open == 0) {
        hal_cpu_wake_unlock(DAUD_CPU_WAKE_USER);
    }
#endif

    DAUD_DEBUG_DONE();
    ret = DAUD_RES_SUCCESS;

_exit:
    daud_unlock_thread();

    return ret;
}

uint32_t daud_forward_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct DAUD_STREAM_CONFIG_T *cfg)
{
    uint32_t ret = DAUD_RES_SUCCESS;
    struct daud_stream_cfg_t *daud_role = daud_get_stream_role(id, stream);
    struct DAUD_STREAM_CONFIG_T *daud_cfg = cfg;
    bool complex_forward = false;
    uint32_t i;
    uint32_t lock;

    DAUD_DEBUG_TRACE2(id, stream);

    daud_lock_thread();

    if (daud_role->status != AF_STATUS_OPEN_CLOSE) {
        TRACE(1, "[%s]: ERROR: status=%d", __func__, daud_role->status);
        goto _exit;
    }

    if (daud_client_stream_is_opened(DAUD_STREAM_ID, AUD_STREAM_CAPTURE)
        && daud_client_stream_is_started(DAUD_STREAM_ID, AUD_STREAM_CAPTURE)) {
        struct daud_stream_cfg_t *local_role = daud_get_stream_role(DAUD_STREAM_ID, AUD_STREAM_CAPTURE);
        enum AUD_STREAM_USE_DEVICE_T local_device;

        local_device = local_role->cfg.device;
        if (daud_stream_use_codec_device(local_device)) {
            complex_forward = true;
        }
    }

    if (complex_forward) {
        uint16_t op_flag = DAUD_OP_FLAG_NONE;
        int sample_rate_idx = -1;
        enum AUD_BITS_T bits, cur_bits;
        enum AUD_SAMPRATE_T sample_rate, cur_sample_rate;
        enum AUD_CHANNEL_NUM_T chan_num, cur_chan_num;
        enum AUD_CHANNEL_MAP_T chan_map, cur_chan_map;
        struct DAUD_STREAM_CONFIG_T cur_stream_cfg;
        uint32_t map = 0;

        TRACE(1, "COMPLEX FORWARD CONFIG");
        memset(&cur_stream_cfg, 0, sizeof(struct DAUD_STREAM_CONFIG_T));

        dma_audio_stream_get_config(stream, &cur_stream_cfg);

        cur_sample_rate = cur_stream_cfg.sample_rate;
        cur_bits        = cur_stream_cfg.bits;
        cur_chan_num    = cur_stream_cfg.channel_num;
        cur_chan_map    = cur_stream_cfg.channel_map;
        TRACE(1, "[CUR_CFG]: samp_rate=%d, bits=%d, chan_num=%d, chan_map=%x",
            cur_sample_rate, cur_bits, cur_chan_num, cur_chan_map);

        sample_rate = daud_cfg->sample_rate;
        bits        = daud_cfg->bits;
        chan_num    = daud_cfg->channel_num;
        chan_map    = daud_cfg->channel_map;
        TRACE(1, "[NEW_CFG]: samp_rate=%d, bits=%d, chan_num=%d, chan_map=%x",
            sample_rate, bits, chan_num, chan_map);

        if (chan_num < cur_chan_num) {
            map = (chan_map ^ cur_chan_map) & chan_map;
            ASSERT(map==0, "[%s]: Invalid chan_map(%x), cur_chan_map(%x), map(%x)",
                __func__, chan_num, cur_chan_map, map);
        } else if (chan_num == cur_chan_num) {
            map = chan_map ^ cur_chan_map;
            ASSERT(map==0, "[%s]: Invalid chan_map(0x%x) != cur_chan_map(0x%x)",
                __func__, chan_num, cur_chan_map);
        } else {
            ASSERT(false, "[%s]: Invalid chan_num(%d) > cur_chan_num(%d)",
                __func__, chan_num, cur_chan_num);
        }

        for (i = 0; i < ARRAY_SIZE(daud_sw_resample_sr); i++) {
            if ((sample_rate == daud_sw_resample_sr[i].out_sample_rate)
                && (cur_sample_rate == daud_sw_resample_sr[i].in_sample_rate)) {
                sample_rate_idx = i;
                break;
            }
        }
        ASSERT(sample_rate_idx >= 0, "[%s]: Bad sample_rate=%d, cur_sample_rate=%d",
            __func__, sample_rate, cur_sample_rate);

        if (bits != cur_bits) {
            op_flag |= DAUD_OP_FLAG_UPDATE_BITS;
        }
        if (chan_num != cur_chan_num) {
            op_flag |= DAUD_OP_FLAG_UPDATE_CHAN_NUM;
        }
        if (chan_map != cur_chan_map) {
            op_flag |= DAUD_OP_FLAG_UPDATE_CHAN_MAP;
        }
        if (sample_rate != cur_sample_rate) {
            op_flag |= (DAUD_OP_FLAG_UPDATE_SAMPRATE | DAUD_OP_FLAG_SW_RESAMPLE);
#ifndef DMA_AUDIO_SW_RESAMPLE
            ASSERT(false, "DMA_AUDIO_SW_RESAMPLE should be defined !");
#endif
        }

        daud_setup_memcpy_callback(id, stream, daud_cfg, &cur_stream_cfg);

#ifdef DMA_AUDIO_SW_RESAMPLE
        if (op_flag & DAUD_OP_FLAG_SW_RESAMPLE) {
            daud_sw_resample_open(id, stream, daud_cfg, &cur_stream_cfg);
            daud_sw_resample_enable(id, stream);
            daud_sw_resample_dump(id, stream);
        }
#endif

        uint32_t samp_size, data_size, samp_cnt, frm_ms;
        uint32_t cur_samp_size, cur_data_size, cur_samp_cnt, cur_frm_ms;
        uint32_t remain, seq_num, dst_dsize, dma_dsize;

        cur_samp_size = daud_samp_bits_to_size(cur_bits);
        cur_data_size = cur_stream_cfg.data_size;
        cur_samp_cnt  = cur_data_size / (cur_samp_size * cur_chan_num);
        cur_frm_ms    = cur_samp_cnt * 1000 / cur_sample_rate;
        TRACE(1, "[CUR_CFG]: data_size=%d, samp_size=%d, samp_cnt=%d, frm_ms=%d",
            cur_data_size, cur_samp_size, cur_samp_cnt, cur_frm_ms);

        samp_size = daud_samp_bits_to_size(bits);
        data_size = daud_cfg->data_size;
        samp_cnt  = data_size / (samp_size * chan_num);
        frm_ms    = samp_cnt * 1000 / sample_rate;
        TRACE(1, "[NEW_CFG]: data_size=%d, samp_size=%d, samp_cnt=%d, frm_ms=%d",
            data_size, samp_size, samp_cnt, frm_ms);

        ASSERT(cur_frm_ms <= frm_ms,"[%s]: Bad frm_ms=%d, should less cur_frm_ms=%d", __func__,
            frm_ms, cur_frm_ms);

        remain = frm_ms % cur_frm_ms;
        ASSERT(remain == 0, "[%s]: remain is not zero, remain=%d", __func__, remain);

        seq_num = frm_ms / cur_frm_ms;
        dma_dsize = cur_data_size / 2;
        dst_dsize = dma_dsize / ((cur_chan_num * cur_samp_size) / (chan_num * samp_size));
        TRACE(1, "op_flag=%x, seq_num=%d, dst_dsize=%d, sample_rate_idx=%d",
            op_flag, seq_num, dst_dsize, sample_rate_idx);

        lock = int_lock();

        daud_role->cfg                  = cur_stream_cfg;
        daud_role->ctl.forward          = true;
        daud_role->ctl.op_flag          = op_flag;
        daud_role->ctl.dma_hw_chan      = HAL_DMA_CHAN_NONE;
        daud_role->ctl.dma_vir_chan     = HAL_DMA_CHAN_NONE;
        daud_role->ctl.frm_seq_max_num  = seq_num;
        daud_role->ctl.frm_seq          = 0;
        daud_role->ctl.dst_pp_index     = 0;
        daud_role->ctl.dst_buf_ptr      = daud_cfg->data_ptr;
        daud_role->ctl.dst_buf_end      = daud_cfg->data_ptr + daud_cfg->data_size;
        daud_role->ctl.dst_buf_size     = daud_cfg->data_size;
        daud_role->ctl.dst_dsize        = dst_dsize;
        daud_role->ctl.dma_buf_ptr      = cur_stream_cfg.data_ptr;
        daud_role->ctl.dma_buf_size     = cur_stream_cfg.data_size;

        int_unlock(lock);
    } else {
        TRACE(1, "SIMPLE FORWARD CONFIG");
        lock = int_lock();

        daud_role->cfg                  = *daud_cfg;
        daud_role->ctl.forward          = true;
        daud_role->ctl.op_flag          = DAUD_OP_FLAG_NONE;
        daud_role->ctl.dma_hw_chan      = HAL_DMA_CHAN_NONE;
        daud_role->ctl.dma_vir_chan     = HAL_DMA_CHAN_NONE;
        daud_role->ctl.frm_seq_max_num  = 1;
        daud_role->ctl.frm_seq          = 0;
        daud_role->ctl.dst_pp_index     = 0;
        daud_role->ctl.dst_buf_ptr      = daud_cfg->data_ptr;
        daud_role->ctl.dst_buf_end      = daud_cfg->data_ptr + daud_cfg->data_size;
        daud_role->ctl.dst_buf_size     = daud_cfg->data_size;
        daud_role->ctl.dma_buf_ptr      = daud_cfg->data_ptr;
        daud_role->ctl.dma_buf_size     = daud_cfg->data_size;

        int_unlock(lock);
    }

    dma_rpcif_forward_stream_open(id, stream);
    daud_set_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);
#ifndef RTOS
    daud_clear_flag(&daud_flag_signal, 1<<(id*2+stream));
    daud_set_flag(&daud_flag_open, 1<<(id*2+stream));
#endif

    DAUD_DEBUG_DONE();
    ret = DAUD_RES_SUCCESS;

_exit:
    daud_unlock_thread();

    return ret;
}

uint32_t daud_forward_stream_setup(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
    struct DAUD_STREAM_CONFIG_T *cfg)
{
    uint32_t ret = DAUD_RES_SUCCESS;

    DAUD_DEBUG_TRACE2(id, stream);

    ret = daud_stream_setup(id, stream, cfg);
    return ret;
}

uint32_t daud_forward_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t ret = DAUD_RES_SUCCESS;

    DAUD_DEBUG_TRACE2(id, stream);

    ret = daud_stream_start(id, stream);
    return ret;
}

uint32_t daud_forward_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t ret = DAUD_RES_SUCCESS;

    DAUD_DEBUG_TRACE2(id, stream);

    ret = daud_stream_stop(id, stream);
    return ret;
}

uint32_t daud_forward_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t ret = DAUD_RES_FAILED;
    struct daud_stream_cfg_t *role = daud_get_stream_role(id, stream);

    DAUD_DEBUG_TRACE2(id, stream);

    daud_lock_thread();

    if (role->status != (AF_STATUS_OPEN_CLOSE|AF_STATUS_STREAM_OPEN_CLOSE)) {
        TRACE(1, "[%s]: ERROR: status=%d", __func__, role->status);
        goto _exit;
    }

#ifdef DMA_AUDIO_SW_RESAMPLE
    if (role->ctl.op_flag & DAUD_OP_FLAG_SW_RESAMPLE) {
        daud_sw_resample_disable(id, stream);
        daud_sw_resample_close(id, stream);
    }
#endif

    role->ctl.forward = false;
    role->ctl.op_flag = DAUD_OP_FLAG_NONE;
    role->ctl.frm_seq = 0;
    role->ctl.frm_seq_max_num = 0;
    dma_rpcif_forward_stream_close(id, stream);
    daud_clear_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);

#ifndef RTOS
    daud_clear_flag(&daud_flag_open, 1<<(id*2+stream));
    if (daud_flag_open == 0) {
        hal_cpu_wake_unlock(DAUD_CPU_WAKE_USER);
    }
#endif

    DAUD_DEBUG_DONE();
    ret = DAUD_RES_SUCCESS;

_exit:
    daud_unlock_thread();

    return ret;
}

#ifdef RTOS
static void daud_thread(void const *argument)
{
    osEvent evt;
    uint32_t signals = 0;
    enum AUD_STREAM_T stream;
    enum AUD_STREAM_ID_T id;

    while (1)
    {
        evt = osSignalWait(0, osWaitForever);
        signals = evt.value.signals;
        if (evt.status == osEventSignal) {
            for (uint8_t i = 0; i < AUD_STREAM_ID_NUM * AUD_STREAM_NUM; i++) {
                if (signals & (1<<i)) {
                    id = (enum AUD_STREAM_ID_T)(i>>1);
                    stream = (enum AUD_STREAM_T)(i & 1);
                    daud_thread_stream_handler(id, stream);
                }
            }
        } else {
            TRACE(2, "[%s] ERROR: evt.status=%d", __func__, evt.status);
            continue;
        }
    }
}
#else
void daud_thread(void const *argument)
{
    uint32_t lock;
    uint32_t i;
    enum AUD_STREAM_ID_T id;
    enum AUD_STREAM_T stream;

    if (!daud_flag_open) {
        return;
    }
    if (!daud_flag_active(&daud_flag_signal, 0xFF)) {
        return;
    }
    for (i = 0; i < AUD_STREAM_ID_NUM * AUD_STREAM_NUM; i++) {
        if (!daud_flag_active(&daud_flag_signal, 1<<i)) {
            continue;
        }
        daud_clear_flag(&daud_flag_signal, 1<<i);
        id = (enum AUD_STREAM_ID_T)(i>>1);
        stream = (enum AUD_STREAM_T)(i&1);
        daud_thread_stream_handler(id, stream);
    }

    lock = int_lock();
    if (daud_flag_signal == 0) {
        hal_cpu_wake_unlock(DAUD_CPU_WAKE_USER);
    }
    int_unlock(lock);
}
#endif
void daud_close(void)
{
    DAUD_TRACE(1, "%s: done", __func__);
}

/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "string.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "heap_api.h"
#include "app_utils.h"
#include "speech_utils.h"
#include "anc_assist_dsp.h"
#include "anc_assist_mic.h"
#include "anc_assist_resample.h"
#include "anc_assist_comm.h"
#include "app_rpc_api.h"

#if defined(_ASSIST_THREAD_DEBUG)
#define ASSIST_THREAD_LOG_D(str, ...)       TR_DEBUG(TR_MOD(AUD), "[ASSIST_THREAD]" str, ##__VA_ARGS__)
#else
#define ASSIST_THREAD_LOG_D(str, ...)
#endif

#ifndef ANC_ASSIST_SAMPLE_RATE
#define ANC_ASSIST_SAMPLE_RATE 16000
#endif

#if (ANC_ASSIST_SAMPLE_RATE == 16000)
#define BUF_SIZE_TIMES      (1)
#elif (ANC_ASSIST_SAMPLE_RATE == 32000)
#define BUF_SIZE_TIMES      (2)
#define HEAP_BUFF_SIZE      (1024 * 96)
#include "ext_heap.h"
#elif (ANC_ASSIST_SAMPLE_RATE == 48000)
#define BUF_SIZE_TIMES      (4)
#define HEAP_BUFF_SIZE      (1024 * 144)
#include "ext_heap.h"
#endif

#define ANC_ASSIST_16K_FRAME_LEN    (480) // input 4x7.5ms data, output 30ms
#define ANC_ASSIST_ALGO_FRAME_LEN   (ANC_ASSIST_16K_FRAME_LEN * ANC_ASSIST_SAMPLE_RATE / 16000)
#define ANC_ASSIST_ALGO_FRAME_BYTES (ANC_ASSIST_ALGO_FRAME_LEN * sizeof(float))
#define ASSIST_QUEUE_FRAME_NUM      (8)
#define ASSIST_QUEUE_BUF_PCM_NUM    (128 * ASSIST_QUEUE_FRAME_NUM * BUF_SIZE_TIMES)  // 2^N

//#define ANC_ASSIST_AUDIO_DUMP_32K_OR_48K
//#define ANC_ASSIST_AUDIO_DUMP_16K

#ifdef ANC_ASSIST_AUDIO_DUMP_32K_OR_48K
#define AUDIO_DUMP_LEN  (ANC_ASSIST_ALGO_FRAME_LEN)
#elif defined (ANC_ASSIST_AUDIO_DUMP_16K)
#define AUDIO_DUMP_LEN  (ANC_ASSIST_16K_FRAME_LEN)
#endif

#if defined(ANC_ASSIST_AUDIO_DUMP_32K_OR_48K) || defined(ANC_ASSIST_AUDIO_DUMP_16K)
#include "audio_dump.h"
typedef short      _DUMP_PCM_T;
static _DUMP_PCM_T audio_dump_buf[AUDIO_DUMP_LEN];
#endif

typedef struct {
    anc_assist_user_t user;
    app_anc_assist_core_cmd_t cmd;
    uint32_t ctrl;
    uint32_t ctrl_buf_len;
    uint8_t *ctrl_buf;
} ANC_ASSIST_MESSAGE_T;

#ifdef ASSIST_RESULT_FIFO_BUF
#if ASSIST_RESULT_FIFO_BUF_LEN % 1024 !=0
#error "ASSIST_RESULT_FIFO_BUF_LEN % 1024 should be 0"
#endif
static uint8_t g_res_fifo_buf[ASSIST_RESULT_FIFO_BUF_LEN];
static uint8_t *g_res_fifo_buf_ptr = NULL;
#endif

static float g_in_queue_buf[MIC_INDEX_QTY][ASSIST_QUEUE_BUF_PCM_NUM];
static float g_process_buf[MIC_INDEX_QTY][ANC_ASSIST_ALGO_FRAME_LEN];
static float *assist_in_queue_buf[MIC_INDEX_QTY] = {NULL};
static float *assist_process_buf[MIC_INDEX_QTY] = {NULL};
static uint32_t g_in_queue_len = 0;
static uint8_t g_ff_ch_num = MAX_FF_CHANNEL_NUM;
static uint8_t g_fb_ch_num = MAX_FB_CHANNEL_NUM;
static uint8_t g_talk_ch_num = MAX_TALK_CHANNEL_NUM;
static uint8_t g_ref_ch_num  = MAX_REF_CHANNEL_NUM;
static uint32_t g_user_status = 0;
static uint32_t g_user_status_16k = 0;
static uint32_t g_user_status_32k = 0;
static uint32_t g_user_status_48k = 0;
static app_voice_assist_dsp_t *g_voice_assist_dsp[ANC_ASSIST_USER_QTY] = {NULL};
static anc_assist_user_fs_t g_voice_assist_user_fs[ANC_ASSIST_USER_QTY] = {0};
static anc_assist_fifo_t g_fifo;

// Thread
//TODO:Will remove this MACRO
#ifdef FIR_ADAPT_ANC_M55
#define ANC_ASSIST_THREAD_STACK_SIZE    (1024 * 4)
#else
#define ANC_ASSIST_THREAD_STACK_SIZE    (1024 * 2)
#endif
static osThreadId anc_assist_thread_tid;
static void anc_assist_thread(void const *argument);
osThreadDef(anc_assist_thread, osPriorityNormal, 1, ANC_ASSIST_THREAD_STACK_SIZE, "anc_assist");

// mailbox
#define ANC_ASSIST_MAIL_MAX                 (30)
#ifdef ANC_ASSIST_TRIGGER_DSP_PROCESS
#define ANC_ASSIST_PROCESS_MAIL_THD         (3)
#else
#define ANC_ASSIST_PROCESS_MAIL_THD         (0)
#endif
static osMailQId anc_assist_mailbox = NULL;
osMailQDef (anc_assist_mailbox, ANC_ASSIST_MAIL_MAX, ANC_ASSIST_MESSAGE_T);

// Mailbox Heap
static heap_handle_t anc_assist_mailbox_heap;
static uint8_t anc_assist_mailbox_heap_pool[1024];

static uint32_t anc_assist_mailbox_cnt(void);

int32_t anc_assist_dsp_register(anc_assist_user_t user, app_voice_assist_dsp_t* anc_assist_dsp, anc_assist_user_fs_t user_fs)
{
    TRACE(0, "[%s] user: %d", __func__, user);
    g_voice_assist_dsp[user] = anc_assist_dsp;
    g_voice_assist_user_fs[user] = user_fs;

    return 0;
}

int32_t anc_assist_dsp_send_result_to_bth_via_fifo(uint8_t user, uint8_t* res_data, uint32_t len, uint8_t cmd)
{
#ifdef ASSIST_RESULT_FIFO_BUF
    app_anc_assist_quick_fifo_head header;
    header.head = 0x55;
    header.user = user;
    header.cmd = cmd;
    header.data_len = len;

    // TRACE(0, "[%s]:free_space = %d (sizeof(app_anc_assist_quick_fifo_head) + len = %d", __func__, kfifo_get_free_space(&(g_fifo.result_fifo)), (sizeof(app_anc_assist_quick_fifo_head) + len));
    if(kfifo_get_free_space(&(g_fifo.result_fifo)) >= (sizeof(app_anc_assist_quick_fifo_head) + len)) {
        kfifo_put(&(g_fifo.result_fifo), (uint8_t *)&header, sizeof(app_anc_assist_quick_fifo_head));
        kfifo_put(&(g_fifo.result_fifo), res_data, len);
    } else {
        TRACE(0,"[%s]: Result fifo buffer is overflow!", __func__);
    }
#else
    ASSERT(0, "[%s] Need to impl this function", __func__);
#endif
    return 0;
}

int32_t anc_assist_dsp_send_result_to_bth(anc_assist_user_t user, uint8_t *buf, uint32_t len,  uint32_t cmd)
{
    app_anc_assist_core_to_bth_data_t tx_cmd;
    uint32_t send_len = sizeof(app_anc_assist_core_to_bth_data_t) - sizeof(tx_cmd.buf) + len;

    memset(&tx_cmd, 0, sizeof(tx_cmd));
    tx_cmd.user = user;
    tx_cmd.sub_cmd = cmd;
    tx_cmd.len = len;
    if (len > 0) {
        ASSERT(len < sizeof(tx_cmd.buf), "buf is too long");
        memcpy(tx_cmd.buf, buf, len);
    }

    app_rpc_send_cmd(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, (uint8_t *)&tx_cmd, send_len);

    TRACE(3, "[%s] user: %d, cmd: %d OK", __func__, user, cmd);
    return 0;
}

static void anc_assist_frequency_update(void)
{
    uint8_t i = 0;
    uint32_t freq = 0;
    uint32_t status = 0;
    enum APP_SYSFREQ_FREQ_T total_freq;

    status = g_user_status;

    while(status) {
        if ((g_voice_assist_dsp[i] != NULL) && ((g_user_status & (0x1 << i)) != 0)) {
            if (g_voice_assist_dsp[i]->get_freq != NULL) {
                freq += g_voice_assist_dsp[i]->get_freq();
            } else {
                TRACE(2, "[%s] WARNING: user(%d) don't register get_freq func", __func__, i);
            }
        }
        status &= ~(0x1 << i) ;
        i++;
    }

#if (ANC_ASSIST_SAMPLE_RATE != 16000)
    freq += 3;
#endif

    if (freq >= 168) {
        total_freq = HAL_CMU_FREQ_208M; // 192
    } else if (freq >= 144) {
        total_freq = HAL_CMU_FREQ_208M; // 168
    } else if (freq >= 120) {
        total_freq = HAL_CMU_FREQ_208M; //144
    } else if (freq >= 96) {
        total_freq = HAL_CMU_FREQ_208M; //120
    } else if(freq >= 72) {
        total_freq = APP_SYSFREQ_104M; //96
    } else if (freq >= 48) {
        total_freq = APP_SYSFREQ_78M; //72
    } else if (freq >= 24) {
        total_freq = APP_SYSFREQ_52M; //48
    } else if (freq >= 15) {
        total_freq = APP_SYSFREQ_26M; //24
    }
#if !defined(ANC_ASSIST_ON_DSP_SENS)
    else if (freq >= 6) {
        total_freq = APP_SYSFREQ_15M; //15
    }
#endif
    else {
        total_freq = HAL_CMU_FREQ_32K;
    }

    TRACE(3, "[%s] Need freq: %d; Set freq: %d", __func__, freq, total_freq);

    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, total_freq);
}

#if defined(ANC_ASSIST_AUDIO_DUMP_32K_OR_48K) || defined(ANC_ASSIST_AUDIO_DUMP_16K)
static void anc_assist_audio_dump(float **ff_mic, float **fb_mic, float **talk_mic, float **ref)
{
    uint32_t dump_ch = 0;
    audio_dump_clear_up();

    // TODO: Use capture buf
    for (uint32_t i=0; i<AUDIO_DUMP_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)ff_mic[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, AUDIO_DUMP_LEN);

#if defined(ANC_ASSIST_VPU)
    for (uint32_t i=0; i<AUDIO_DUMP_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)vpu_mic[i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, AUDIO_DUMP_LEN);
#else
    for (uint32_t i=0; i<AUDIO_DUMP_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)fb_mic[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, AUDIO_DUMP_LEN);
#endif
    for (uint32_t i=0; i<AUDIO_DUMP_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)talk_mic[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, AUDIO_DUMP_LEN);

    for (uint32_t i=0; i<AUDIO_DUMP_LEN; i++) {
        audio_dump_buf[i] = (_PCM_T)ref[0][i] >> 8;
    }
    audio_dump_add_channel_data(dump_ch++, audio_dump_buf, AUDIO_DUMP_LEN);

    audio_dump_run();
}
#endif

static int32_t user_fs_process_frame(uint32_t user_status, process_frame_data_t process_frame_date)
{
#ifdef ANC_ASSIST_AUDIO_DUMP_32K_OR_48K
    if (((g_user_status_48k == user_status) || (g_user_status_32k == user_status)) && (user_status != 0)) {
        anc_assist_audio_dump(process_frame_date.ff_mic, process_frame_date.fb_mic, process_frame_date.talk_mic, process_frame_date.ref);
    }
#elif defined (ANC_ASSIST_AUDIO_DUMP_16K)
    if ((g_user_status_16k == user_status) && (user_status != 0)) {
        anc_assist_audio_dump(process_frame_date.ff_mic, process_frame_date.fb_mic, process_frame_date.talk_mic, process_frame_date.ref);
    }
#endif
    int i = 0;
    uint32_t status = user_status;
    int ret = 0;
    while(status) {
        if ((g_voice_assist_dsp[i] != NULL) && ((user_status & (0x1 << i)) != 0)) {
            if (g_voice_assist_dsp[i]->process != NULL) {
                ret = g_voice_assist_dsp[i]->process(&process_frame_date);
            } else {
                TRACE(2, "[%s] WARNING: user(%d) don't register process func", __func__, i);
            }
        }
        status &= ~(0x1 << i) ;
        i++;
    }
    return ret;
}

static int32_t _process_frame(float **ff_mic, uint8_t ff_ch_num,
                       float **fb_mic, uint8_t fb_ch_num,
                       float **talk_mic, uint8_t talk_ch_num,
                       float **ref, uint8_t ref_ch_num,
#if defined(ANC_ASSIST_VPU)
                       float *vpu_mic,
#endif
                       uint32_t frame_len)
{
    int ret = 0;
    process_frame_data_t process_frame_date;

    process_frame_date.ff_mic = ff_mic;
    process_frame_date.ff_ch_num = ff_ch_num;
    process_frame_date.fb_mic = fb_mic;
    process_frame_date.fb_ch_num = fb_ch_num;
    process_frame_date.talk_mic = talk_mic;
    process_frame_date.talk_ch_num = talk_ch_num;
#if defined(ANC_ASSIST_VPU)
    process_frame_date.vpu_mic = vpu_mic;
#endif
    process_frame_date.ref = ref;
    process_frame_date.ref_ch_num = ref_ch_num;
    process_frame_date.frame_len = frame_len;

    ret = user_fs_process_frame(g_user_status_48k, process_frame_date);
    ret = user_fs_process_frame(g_user_status_32k, process_frame_date); // 32 AND 48 olny one can exist
#if (ANC_ASSIST_SAMPLE_RATE != 16000)
    if (g_user_status_16k != 0) {
        anc_assist_resample_process((float *)g_process_buf, ANC_ASSIST_ALGO_FRAME_LEN, ANC_ASSIST_ALGO_FRAME_LEN);
    }
#endif
    ret = user_fs_process_frame(g_user_status_16k, process_frame_date);

    return ret;
}

static int32_t user_reset(anc_assist_user_t user)
{
    if ((g_voice_assist_dsp[user] != NULL) && ((g_user_status & (0x1 << user)) != 0)) {
        if (g_voice_assist_dsp[user]->reset != NULL) {
            g_voice_assist_dsp[user]->reset();
        } else {
            TRACE(2, "[%s] WARNING: user(%d) don't register reset func", __func__, user);
        }
    }

    return 0;
}

static int32_t user_ctrl(anc_assist_user_t user, uint32_t ctrl, uint8_t *ptr, uint32_t ptr_len)
{
    if ((g_voice_assist_dsp[user] != NULL) && ((g_user_status & (0x1 << user)) != 0)) {
        if (g_voice_assist_dsp[user]->ctrl != NULL) {
            g_voice_assist_dsp[user]->ctrl(ctrl, ptr, ptr_len);
        } else {
            TRACE(2, "[%s] WARNING: user(%d) don't register ctrl func", __func__, user);
        }
        anc_assist_frequency_update();
    }

    return 0;
}

static int32_t all_users_set_mode(uint32_t mode)
{
    for (uint32_t user=0; user<ANC_ASSIST_USER_QTY; user++) {
        if (g_voice_assist_dsp[user] != NULL) {
            if (g_voice_assist_dsp[user]->set_mode != NULL) {
                g_voice_assist_dsp[user]->set_mode(mode);
            }
        }
    }
    anc_assist_frequency_update();

    return 0;
}

static int32_t user_open(anc_assist_user_t user)
{
    if ((g_voice_assist_dsp[user] != NULL) && ((g_user_status & (0x1 << user)) == 0)) {
        if (g_voice_assist_dsp[user]->open != NULL) {
            g_voice_assist_dsp[user]->open();
        } else {
            TRACE(2, "[%s] WARNING: user(%d) don't register open func", __func__, user);
        }

        g_user_status = g_user_status | (0x1 << user);

        if (g_voice_assist_user_fs[user] == ANC_ASSIST_USER_FS_16K) {
            g_user_status_16k = g_user_status_16k | (0x1 << user);
        } else if (g_voice_assist_user_fs[user] == ANC_ASSIST_USER_FS_32K) {
            g_user_status_32k = g_user_status_32k | (0x1 << user);
        } else if (g_voice_assist_user_fs[user] == ANC_ASSIST_USER_FS_48K) {
            g_user_status_48k = g_user_status_48k | (0x1 << user);
        }
        anc_assist_frequency_update();
    }

    return 0;
}

static int32_t user_close(anc_assist_user_t user)
{
    if ((g_voice_assist_dsp[user] != NULL) && ((g_user_status & (0x1 << user)) != 0)) {
        if (g_voice_assist_dsp[user]->close != NULL) {
            g_voice_assist_dsp[user]->close();
        } else {
            TRACE(2, "[%s] WARNING: user(%d) don't register close func", __func__, user);
        }
        g_user_status = g_user_status & ~(0x1 << user);
        if (g_voice_assist_user_fs[user] == ANC_ASSIST_USER_FS_16K) {
            g_user_status_16k = g_user_status_16k & ~(0x1 << user);
        } else if (g_voice_assist_user_fs[user] == ANC_ASSIST_USER_FS_32K) {
            g_user_status_32k = g_user_status_32k & ~(0x1 << user);
        } else if (g_voice_assist_user_fs[user] == ANC_ASSIST_USER_FS_48K) {
            g_user_status_48k = g_user_status_48k & ~(0x1 << user);
        }
        anc_assist_frequency_update();
    }

    return 0;
}

static void anc_assist_mailbox_heap_init(void)
{
    anc_assist_mailbox_heap =
        heap_register(anc_assist_mailbox_heap_pool,
        sizeof(anc_assist_mailbox_heap_pool));
}

static void *anc_assist_mailbox_heap_malloc(uint32_t size)
{
    void *ptr = NULL;
    if (size){
        ptr = heap_malloc(anc_assist_mailbox_heap, size);
        ASSERT(ptr, "%s size:%d, mailboxCnt:%d", __func__, size, anc_assist_mailbox_cnt());
    }
    return ptr;
}

static void anc_assist_mailbox_heap_free(void *rmem)
{
    if (rmem){
        heap_free(anc_assist_mailbox_heap, rmem);
    }
}

static uint32_t anc_assist_mailbox_cnt(void)
{
    return osMailGetCount(anc_assist_mailbox);
}

static int anc_assist_mailbox_put(ANC_ASSIST_MESSAGE_T *msg_src)
{
    osStatus status = osOK;

    ANC_ASSIST_MESSAGE_T *msg = NULL;
    msg = (ANC_ASSIST_MESSAGE_T *)osMailAlloc(anc_assist_mailbox, 0);

    if (!msg) {
        osEvent evt;
        TRACE(0, "[ANC_ASSIST] osMailAlloc error dump");
        for (uint8_t i = 0; i < ANC_ASSIST_MAIL_MAX; i++)
        {
            evt = osMailGet(anc_assist_mailbox, 0);
            if (evt.status == osEventMail) {
                TRACE(4, "cnt:%d user:%d param:%d/%p",
                      i,
                      ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->user,
                      ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->cmd,
                      ((ANC_ASSIST_MESSAGE_T *)(evt.value.p))->ctrl_buf);
            } else {
                TRACE(2, "cnt:%d %d", i, evt.status);
                break;
            }
        }
        TRACE(0, "osMailAlloc error dump end");
        ASSERT(0, "[ANC_ASSIST] osMailAlloc error");
    } else {
        msg->user = msg_src->user;
        msg->cmd = msg_src->cmd;
        msg->ctrl_buf = msg_src->ctrl_buf;
        msg->ctrl = msg_src->ctrl;
        msg->ctrl_buf_len = msg_src->ctrl_buf_len;

        status = osMailPut(anc_assist_mailbox, msg);
        if (osOK != status) {
            TRACE(2, "[%s] WARNING: failed: %d", __func__, status);
        }
    }

    return (int)status;
}

static int anc_assist_mailbox_get(ANC_ASSIST_MESSAGE_T **msg)
{
    int ret = 0;
    osEvent evt;
    evt = osMailGet(anc_assist_mailbox, osWaitForever);

    if (evt.status == osEventMail) {
        *msg = (ANC_ASSIST_MESSAGE_T *)evt.value.p;
    } else {
        ret = -1;
    }

    return ret;
}

static int anc_assist_mailbox_free(ANC_ASSIST_MESSAGE_T *msg)
{
    osStatus status;

    anc_assist_mailbox_heap_free(msg->ctrl_buf);
    status = osMailFree(anc_assist_mailbox, msg);
    if (osOK != status) {
        TRACE(2, "[%s] WARNING: failed: %d", __func__, status);
    }

    return (int)status;
}

static bool anc_assist_data_is_ready(void)
{
    if (kfifo_len(&(g_fifo.talk_mic_fifo[g_talk_ch_num - 1])) >= ANC_ASSIST_ALGO_FRAME_BYTES) {
        return true;
    } else {
        return false;
    }
}

static void anc_assist_data_clear(void)
{
    uint32_t len = kfifo_len(&(g_fifo.ff_mic_fifo[0]));    // Must check ff_mic_fifo
    uint32_t loop_cnt = len / ANC_ASSIST_ALGO_FRAME_BYTES;

    for (uint32_t i=0; i<loop_cnt; i++) {
        for (uint8_t i = 0; i < g_ff_ch_num; i++) {
            kfifo_get(&(g_fifo.ff_mic_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_FF + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
        for (uint8_t i = 0; i < g_fb_ch_num; i++) {
            kfifo_get(&(g_fifo.fb_mic_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_FB + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
        for (uint8_t i = 0; i < g_talk_ch_num; i++) {
            kfifo_get(&(g_fifo.talk_mic_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_TALK + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
        for (uint8_t i = 0; i < g_ref_ch_num; i++) {
            kfifo_get(&(g_fifo.ref_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_REF + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
#if defined(ANC_ASSIST_VPU)
        kfifo_get(&(g_fifo.vpu_fifo), (uint8_t *)assist_process_buf[MIC_INDEX_VPU], ANC_ASSIST_ALGO_FRAME_BYTES);
#endif
    }

    if (len % ANC_ASSIST_ALGO_FRAME_BYTES > 0) {
        for (uint8_t i = 0; i < g_ff_ch_num; i++) {
            kfifo_get(&(g_fifo.ff_mic_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_FF + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
        for (uint8_t i = 0; i < g_fb_ch_num; i++) {
            kfifo_get(&(g_fifo.fb_mic_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_FB + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
        for (uint8_t i = 0; i < g_talk_ch_num; i++) {
            kfifo_get(&(g_fifo.talk_mic_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_TALK + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
        for (uint8_t i = 0; i < g_ref_ch_num; i++) {
            kfifo_get(&(g_fifo.ref_fifo[i]), (uint8_t *)assist_process_buf[MIC_INDEX_REF + i], ANC_ASSIST_ALGO_FRAME_BYTES);
        }
#if defined(ANC_ASSIST_VPU)
        kfifo_get(&(g_fifo.vpu_fifo), (uint8_t *)assist_process_buf[MIC_INDEX_VPU], ANC_ASSIST_ALGO_FRAME_BYTES);
#endif
    }
}

static void anc_assist_kfifo_init(anc_assist_fifo_t *fifo)
{
    for (uint8_t i = 0; i < g_ff_ch_num; i++) {
        fifo->ff_mic_fifo_mem[i] = (uint8_t *)assist_in_queue_buf[MIC_INDEX_FF + i];
    }
    for (uint8_t i = 0; i < g_fb_ch_num; i++) {
        fifo->fb_mic_fifo_mem[i] = (uint8_t *)assist_in_queue_buf[MIC_INDEX_FB + i];
    }
    for (uint8_t i = 0; i < g_talk_ch_num; i++) {
        fifo->talk_mic_fifo_mem[i] = (uint8_t *)assist_in_queue_buf[MIC_INDEX_TALK + i];
    }
    for (uint8_t i = 0; i < g_ref_ch_num; i++) {
        fifo->ref_fifo_mem[i] = (uint8_t *)assist_in_queue_buf[MIC_INDEX_REF + i];
    }
#ifdef ASSIST_RESULT_FIFO_BUF
    fifo->result_fifo_mem = (uint8_t *)g_res_fifo_buf_ptr;
#endif
#if defined(ANC_ASSIST_VPU)
    fifo->vpu_fifo_mem = (uint8_t *)assist_in_queue_buf[MIC_INDEX_VPU];
#endif

    for (uint8_t i = 0; i < g_ff_ch_num; i++) {
        kfifo_init(&fifo->ff_mic_fifo[i], fifo->ff_mic_fifo_mem[i], ASSIST_QUEUE_BUF_PCM_NUM * sizeof(float));
    }
    for (uint8_t i = 0; i < g_fb_ch_num; i++) {
        kfifo_init(&fifo->fb_mic_fifo[i], fifo->fb_mic_fifo_mem[i], ASSIST_QUEUE_BUF_PCM_NUM * sizeof(float));
    }
    for (uint8_t i = 0; i < g_talk_ch_num; i++) {
        kfifo_init(&fifo->talk_mic_fifo[i], fifo->talk_mic_fifo_mem[i], ASSIST_QUEUE_BUF_PCM_NUM * sizeof(float));
    }
    for (uint8_t i = 0; i < g_ref_ch_num; i++) {
        kfifo_init(&fifo->ref_fifo[i], fifo->ref_fifo_mem[i], ASSIST_QUEUE_BUF_PCM_NUM * sizeof(float));
    }
#ifdef ASSIST_RESULT_FIFO_BUF
    kfifo_init(&fifo->result_fifo, fifo->result_fifo_mem, ASSIST_RESULT_FIFO_BUF_LEN);
#endif
#if defined(ANC_ASSIST_VPU)
    kfifo_init(&fifo->vpu_fifo, fifo->vpu_fifo_mem, ASSIST_QUEUE_BUF_PCM_NUM * sizeof(float));
#endif

    TRACE(3, "[%s] fifo addr: %p, size: %d", __func__,  fifo, g_fifo.talk_mic_fifo[0].size);
}

static void anc_assist_thread(void const *argument)
{
    ANC_ASSIST_MESSAGE_T *msg_p = NULL;
    float *ff_mic_buf[MAX_FF_CHANNEL_NUM];
    float *fb_mic_buf[MAX_FB_CHANNEL_NUM];
    float *talk_mic_buf[MAX_TALK_CHANNEL_NUM];
    float *ref_buf[MAX_REF_CHANNEL_NUM];

    while(1) {
        if (!anc_assist_mailbox_get(&msg_p)) {
            if (msg_p->cmd == APP_ANC_ASSIST_CORE_OPEN) {
                user_open(msg_p->user);
                app_anc_assist_core_to_bth_rsp_t rsp;
                memset(&rsp, 0, sizeof(rsp));
                rsp.cmd = APP_ANC_ASSIST_CORE_OPEN;
                rsp.user = msg_p->user;
                rsp.fifo_ptr = &g_fifo;
                app_rpc_send_cmd_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, (uint8_t *)&rsp, sizeof(rsp));
            } else if (msg_p->cmd == APP_ANC_ASSIST_CORE_CLOSE) {
                user_close(msg_p->user);
                if (g_user_status == 0) {
                    anc_assist_data_clear();
                }
                app_rpc_send_cmd_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, NULL, 0);
            } else if (msg_p->cmd == APP_ANC_ASSIST_CORE_RESET) {
                user_reset(msg_p->user);
                app_rpc_send_cmd_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, NULL, 0);
            } else if (msg_p->cmd == APP_ANC_ASSIST_CORE_CTRL) {
                user_ctrl(msg_p->user, msg_p->ctrl,msg_p->ctrl_buf, msg_p->ctrl_buf_len);
                app_rpc_send_cmd_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, NULL, 0);
            } else if (msg_p->cmd == APP_ANC_ASSIST_CORE_SET_MODE) {
                all_users_set_mode(msg_p->ctrl);
            }
            anc_assist_mailbox_free(msg_p);

            while (g_user_status != 0) {
                if (anc_assist_data_is_ready()) {
                    ASSIST_THREAD_LOG_D("Process run start ...");

                    for (uint8_t i = 0; i < g_ff_ch_num; i++) {
                        ff_mic_buf[i] = assist_process_buf[i + MIC_INDEX_FF];
                    }
                    for (uint8_t i = 0; i < g_fb_ch_num; i++) {
                        fb_mic_buf[i] = assist_process_buf[i + MIC_INDEX_FB];
                    }
                    for (uint8_t i = 0; i < g_talk_ch_num; i++) {
                        talk_mic_buf[i] = assist_process_buf[i + MIC_INDEX_TALK];
                    }
                    for (uint8_t i = 0; i < g_ref_ch_num; i++) {
                        ref_buf[i] = assist_process_buf[i + MIC_INDEX_REF];
                    }

                    for (uint8_t i = 0; i < g_ff_ch_num; i++) {
                        kfifo_get(&(g_fifo.ff_mic_fifo[i]), (uint8_t *)ff_mic_buf[i], ANC_ASSIST_ALGO_FRAME_BYTES);
                    }
                    for (uint8_t i = 0; i < g_fb_ch_num; i++) {
                        kfifo_get(&(g_fifo.fb_mic_fifo[i]), (uint8_t *)fb_mic_buf[i], ANC_ASSIST_ALGO_FRAME_BYTES);
                    }
                    for (uint8_t i = 0; i < g_talk_ch_num; i++) {
                        kfifo_get(&(g_fifo.talk_mic_fifo[i]), (uint8_t *)talk_mic_buf[i], ANC_ASSIST_ALGO_FRAME_BYTES);
                    }
                    for (uint8_t i = 0; i < g_ref_ch_num; i++) {
                        kfifo_get(&(g_fifo.ref_fifo[i]), (uint8_t *)ref_buf[i], ANC_ASSIST_ALGO_FRAME_BYTES);
                    }
#if defined(ANC_ASSIST_VPU)
                    kfifo_get(&(g_fifo.vpu_fifo), (uint8_t *)assist_process_buf[MIC_INDEX_VPU], ANC_ASSIST_ALGO_FRAME_BYTES);
#endif
                    _process_frame(ff_mic_buf, g_ff_ch_num,
                                    fb_mic_buf, g_fb_ch_num,
                                    talk_mic_buf, g_talk_ch_num,
                                    ref_buf, g_ref_ch_num,
#if defined(ANC_ASSIST_VPU)
                                    assist_process_buf[MIC_INDEX_VPU],
#endif
                                    ANC_ASSIST_ALGO_FRAME_LEN);

                                    ASSIST_THREAD_LOG_D("Process run end ...");
                    if (anc_assist_mailbox_cnt() > ANC_ASSIST_PROCESS_MAIL_THD) {
                        break;
                    }
                } else {
#ifdef ANC_ASSIST_TRIGGER_DSP_PROCESS
                    break;
#else
                    if (anc_assist_mailbox_cnt() > ANC_ASSIST_PROCESS_MAIL_THD) {
                        break;
                    } else {
                        osDelay(3);
                    }
#endif
                }
            }
        }
    }
}

int32_t anc_assist_dsp_open(void)
{
    TRACE(1, "[%s] START", __func__);

    g_user_status = 0;
    g_in_queue_len = 0;

    for(uint32_t ch = 0; ch < MIC_INDEX_QTY; ch++) {
        assist_in_queue_buf[ch] = g_in_queue_buf[ch];
        assist_process_buf[ch] = g_process_buf[ch];
    }
#ifdef ASSIST_RESULT_FIFO_BUF
    g_res_fifo_buf_ptr = g_res_fifo_buf;
#endif
    anc_assist_kfifo_init(&g_fifo);

#if (ANC_ASSIST_SAMPLE_RATE != 16000)
    ext_heap_init();
    anc_assist_resample_init(ANC_ASSIST_SAMPLE_RATE, ANC_ASSIST_ALGO_FRAME_LEN, ext_calloc, ext_free);
#endif

#if defined(ANC_ASSIST_AUDIO_DUMP_32K_OR_48K) || defined(ANC_ASSIST_AUDIO_DUMP_16K)
    audio_dump_init(AUDIO_DUMP_LEN, sizeof(_DUMP_PCM_T), 4);
#endif

    anc_assist_mailbox_heap_init();

    anc_assist_mailbox = osMailCreate(osMailQ(anc_assist_mailbox), NULL);
    ASSERT(anc_assist_mailbox != NULL, "[%s] Can not create mailbox", __func__);

    anc_assist_thread_tid = osThreadCreate(osThread(anc_assist_thread), NULL);
    ASSERT(anc_assist_thread_tid != NULL, "[%s] Can not create thread", __func__);

    osSignalSet(anc_assist_thread_tid, 0x0);

    return 0;
}

int32_t anc_assist_dsp_close(void)
{
    TRACE(1, "[%s] ...", __func__);

    //all functions will not be called, Because anc_assist_dsp_open will be called once upon startup
#if (ANC_ASSIST_SAMPLE_RATE != 16000)
    anc_assist_resample_deinit(ANC_ASSIST_SAMPLE_RATE);
    ext_heap_deinit();
#endif

    return 0;
}

int32_t anc_assist_dsp_capture_process(float *in_buf[], uint32_t frame_len)
{
    ANC_ASSIST_MESSAGE_T msg;

    if (anc_assist_mailbox_cnt() < ANC_ASSIST_PROCESS_MAIL_THD) {
        memset(&msg, 0, sizeof(msg));
        msg.user = ANC_ASSIST_USER_QTY;
        msg.cmd = APP_ANC_ASSIST_CORE_PROCESS;
        anc_assist_mailbox_put(&msg);
    }

    return 0;
}

int32_t anc_assist_dsp_capture_cmd(uint8_t *buf, uint32_t len)
{
    ANC_ASSIST_MESSAGE_T msg;
    app_anc_assist_core_to_dsp_data_t *rx_cmd = (app_anc_assist_core_to_dsp_data_t *)buf;

    TRACE(4, "[%s] user: %d, cmd: %d, status: 0x%x", __func__, rx_cmd->user, rx_cmd->cmd, g_user_status);

    memset(&msg, 0, sizeof(msg));
    msg.user = rx_cmd->user;
    msg.cmd = rx_cmd->cmd;
    msg.ctrl = rx_cmd->ctrl;
    msg.ctrl_buf_len = rx_cmd->ctrl_buf_len;
    if (msg.ctrl_buf_len > 0) {
        msg.ctrl_buf = (uint8_t *)anc_assist_mailbox_heap_malloc(msg.ctrl_buf_len);
        memcpy(msg.ctrl_buf, rx_cmd->ctrl_buf, msg.ctrl_buf_len);
    } else {
        msg.ctrl_buf = NULL;
    }
    anc_assist_mailbox_put(&msg);

    return 0;
}

int32_t anc_assist_dsp_capture_set_mode(uint32_t mode)
{
    app_anc_assist_core_to_dsp_data_t rx_cmd;
    memset(&rx_cmd, 0, sizeof(rx_cmd));
    rx_cmd.cmd = APP_ANC_ASSIST_CORE_SET_MODE;
    rx_cmd.ctrl = mode;
    anc_assist_dsp_capture_cmd((uint8_t *)&rx_cmd, sizeof(rx_cmd));

    return 0;
}

int32_t anc_assist_dsp_thread_loop_test(void)
{
    uint32_t cnt = 0;

    while (1) {
        app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, APP_SYSFREQ_26M);
        anc_assist_dsp_open();
        hal_sys_timer_delay_us(13000);

        extern void rtx_show_memory_stats(void);
        rtx_show_memory_stats();

        TRACE(2, "[%s] cnt: %d", __func__, cnt++);
    }

    return 0;
}
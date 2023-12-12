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
#include "app_anc_assist_core.h"
#include "assist/anc_assist_mic.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "string.h"
#ifdef ANC_ASSIST_ON_DSP_M55
#include "mcu_dsp_m55_app.h"
#endif
#ifdef ANC_ASSIST_ON_DSP_HIFI
#include "mcu_dsp_hifi4_app.h"
#endif
#ifdef ANC_ASSIST_ON_DSP_SENS
#include "mcu_sensor_hub_app.h"
#endif
#ifdef ANC_ASSIST_ON_DSP
#include "anc_assist_comm.h"
#include "app_rpc_api.h"
//#include "app_overlay.h"
#endif
#ifdef RTOS
#include "cmsis_os.h"
#endif

// #define ANC_ASSIST_CORE_MUTEX

#ifdef ANC_ASSIST_ON_DSP
static uint32_t g_core_user_status = 0;
static uint32_t g_fifo_total_cnt = 0;
static uint32_t g_fifo_overflow_cnt = 0;
static uint8_t g_ff_ch_num = MAX_FF_CHANNEL_NUM;
static uint8_t g_fb_ch_num = MAX_FB_CHANNEL_NUM;
static uint8_t g_talk_ch_num = MAX_TALK_CHANNEL_NUM;
static uint8_t g_ref_ch_num  = MAX_REF_CHANNEL_NUM;

#define APP_ANC_ASSIST_TIMEROUT_MS (2000)

static anc_assist_fifo_t *g_assist_fifo = NULL;

osSemaphoreDef(app_anc_assist_sema);
osSemaphoreId app_anc_assist_sema_id = NULL;

#ifdef ANC_ASSIST_CORE_MUTEX
osMutexId _anc_assist_core_mutex_id = NULL;
osMutexDef(_anc_assist_core_mutex);
#endif

#define COMM_ANC_ASSIST_WAIT_RSP_TIMEOUT_MS (500)
typedef void(*app_dsp_anc_assist_rsp_received_handler_t)(unsigned char * ptr, short len);
app_dsp_anc_assist_rsp_received_handler_t g_anc_assist_rsp_done_handler = NULL;

static void comm_bth_anc_assist_send_cmd_to_dsp_handler(uint8_t* ptr, uint16_t len)
{
    app_rpc_send_data_waiting_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, ptr, len);
}

static void comm_bth_anc_assist_receive_data_from_dsp_handler(uint8_t* ptr, uint16_t len)
{
#ifdef ANC_APP
    app_anc_assist_core_to_bth_data_t *receive_data = (app_anc_assist_core_to_bth_data_t *)ptr;
    //TRACE(2, "BTH<----M55 assist_user:%d len:%d", receive_data->user, receive_data->len);
    if (app_anc_assist_get_result_register(receive_data->user) != NULL) {
        app_anc_assist_get_result_register(receive_data->user)(receive_data->buf, receive_data->len,
                                                                NULL, receive_data->sub_cmd);
    }
#endif
}

static void comm_bth_anc_assist_tx_done_handler(uint16_t cmdCode,
    uint8_t* ptr, uint16_t len)
{

}

static void comm_bth_anc_assist_wait_rsp_timeout(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "Warning %s", __func__);
}

static void comm_bth_anc_assist_received_rsp_handler(uint8_t* ptr, uint16_t len)
{
    if (g_anc_assist_rsp_done_handler) {
        g_anc_assist_rsp_done_handler(ptr, len);
    }
}

static void app_dsp_core_anc_assist_rsp_received_cb_register(app_dsp_anc_assist_rsp_received_handler_t handler)
{
    g_anc_assist_rsp_done_handler = handler;
}

RPC_ANC_ASSIST_TASK_CMD_TO_ADD(TASK_ANC_ASSIST_CMD_RSP,
                                "ANC Assist CMD",
                                comm_bth_anc_assist_send_cmd_to_dsp_handler,
                                comm_bth_anc_assist_receive_data_from_dsp_handler,
                                COMM_ANC_ASSIST_WAIT_RSP_TIMEOUT_MS,
                                comm_bth_anc_assist_wait_rsp_timeout,
                                comm_bth_anc_assist_received_rsp_handler,
                                comm_bth_anc_assist_tx_done_handler);

static void comm_bth_anc_send_data_to_dsp_handler(uint8_t* ptr, uint16_t len)
{
    app_rpc_send_data_no_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_DATA_NO_RSP, ptr, len);
}

RPC_ANC_ASSIST_TASK_CMD_TO_ADD(TASK_ANC_ASSIST_DATA_NO_RSP,
                                "ANC Assist Data",
                                comm_bth_anc_send_data_to_dsp_handler,
                                NULL,
                                0,
                                NULL,
                                NULL,
                                NULL);

static void _anc_assist_core_create_lock(void)
{
#ifdef ANC_ASSIST_CORE_MUTEX
    if (_anc_assist_core_mutex_id == NULL) {
        _anc_assist_core_mutex_id = osMutexCreate((osMutex(_anc_assist_core_mutex)));
    }
#endif
}

static void _anc_assist_core_lock(void)
{
#ifdef ANC_ASSIST_CORE_MUTEX
    osMutexWait(_anc_assist_core_mutex_id, osWaitForever);
#endif
}

static void _anc_assist_core_unlock(void)
{
#ifdef ANC_ASSIST_CORE_MUTEX
    osMutexRelease(_anc_assist_core_mutex_id);
#endif
}

static int app_anc_assist_sema_init(void)
{
    if (app_anc_assist_sema_id == NULL) {
        app_anc_assist_sema_id = osSemaphoreCreate(osSemaphore(app_anc_assist_sema), 0);
    }
    ASSERT(app_anc_assist_sema_id, "create app_anc_assist_sema_id fail!");

    return 0;
}

static int app_anc_assist_sema_deinit(void)
{
    int ret;
    if(app_anc_assist_sema_id) {
        ret = osSemaphoreDelete(app_anc_assist_sema_id);
        app_anc_assist_sema_id = NULL;
    }

    return ret;
}

static int app_anc_assist_sema_wait(uint32_t timeout)
{
    int ret = 0;
    if(app_anc_assist_sema_id) {
        ret = osSemaphoreAcquire(app_anc_assist_sema_id, timeout);
    }

    return ret;
}

static void app_anc_assist_sema_process_done(unsigned char *ptr, short len)
{
    if ((ptr != NULL) && (len != 0)) {
        ASSERT(len == sizeof(app_anc_assist_core_to_bth_rsp_t), "[%s] Invalid len: %d", __func__, len);
        app_anc_assist_core_to_bth_rsp_t *rsp = (app_anc_assist_core_to_bth_rsp_t *)ptr;
        TRACE(3, "[%s] user: %d, cmd: %d", __func__, rsp->user, rsp->cmd);
        if (rsp->cmd == APP_ANC_ASSIST_CORE_OPEN) {
            g_assist_fifo = rsp->fifo_ptr;
            TRACE(3, "[%s] fifo addr: %p, size: %d", __func__, g_assist_fifo, g_assist_fifo->talk_mic_fifo[0].size);
        }
    }

    if(app_anc_assist_sema_id) {
        osSemaphoreRelease(app_anc_assist_sema_id);
    }
}

void app_anc_assist_core_get_result_via_fifo(void)
{
#ifdef ASSIST_RESULT_FIFO_BUF
    if (g_core_user_status == 0) {
        return;
    }

    static uint8_t g_result_process_buf[ASSIST_RESULT_FIFO_BUF_LEN / 2];
    app_anc_assist_quick_fifo_head result_fifo_head;

    if(kfifo_len(&(g_assist_fifo->result_fifo)) >= sizeof(app_anc_assist_quick_fifo_head)) {
        kfifo_peek_to_buf(&(g_assist_fifo->result_fifo), (uint8_t *)&result_fifo_head, sizeof(app_anc_assist_quick_fifo_head));
        // TRACE(0, "[%s]:header_flag = %d user = %d len = %d", __func__, result_fifo_head.head, result_fifo_head.user, result_fifo_head.data_len);

        if (result_fifo_head.head != 0x55) {
            ASSERT(0, "%s worng header_flag ", __func__);
        } else {
            if(kfifo_len(&(g_assist_fifo->result_fifo)) >= (sizeof(app_anc_assist_quick_fifo_head) + result_fifo_head.data_len)) {

                kfifo_get(&(g_assist_fifo->result_fifo), (uint8_t *)&result_fifo_head, sizeof(app_anc_assist_quick_fifo_head));
                kfifo_get(&(g_assist_fifo->result_fifo), (uint8_t *)&g_result_process_buf, result_fifo_head.data_len);

                if (app_anc_assist_get_result_register(result_fifo_head.user) != NULL) {
                    app_anc_assist_get_result_register(result_fifo_head.user)(g_result_process_buf, result_fifo_head.data_len,
                                                                            NULL, result_fifo_head.cmd);
                } else {
                    ASSERT(0,"[%s]:result callback is NULL!",__func__);
                }
            }
        }
    }
#else
    ASSERT(0, "[%s] Need to impl this function", __func__);
#endif
}

static int app_anc_assist_rpc_init(void)
{
    app_anc_assist_sema_init();
    app_dsp_core_anc_assist_rsp_received_cb_register(app_anc_assist_sema_process_done);

    return 0;
}


static int app_anc_assist_rpc_deinit(void)
{
    app_dsp_core_anc_assist_rsp_received_cb_register(NULL);
    app_anc_assist_sema_deinit();

    return 0;
}

/* If talk_len != other_len happen always, it's better to clear fifo on M55*/
static void check_fifo_pcm_len(void)
{
    uint32_t talk_len = 0;
    uint32_t other_len = 0;

    if (g_fifo_total_cnt % 400 == 0) {
        talk_len = kfifo_get_free_space(&g_assist_fifo->talk_mic_fifo[g_talk_ch_num - 1]);

        for (uint8_t i = 0; i < g_ff_ch_num; i++) {
            other_len = kfifo_get_free_space(&g_assist_fifo->ff_mic_fifo[i]);
            if (talk_len != other_len) {
            TRACE(3, "[%s] WARNING: ff_mic_fifo %d != %d; i = %d", __func__, talk_len, other_len, i);
            }
        }

        for (uint8_t i = 0; i < g_fb_ch_num; i++) {
            other_len = kfifo_get_free_space(&g_assist_fifo->fb_mic_fifo[i]);
            if (talk_len != other_len) {
            TRACE(3, "[%s] WARNING: fb_mic_fifo %d != %d; i = %d", __func__, talk_len, other_len, i);
            }
        }

        for (uint8_t i = 0; i < g_talk_ch_num; i++) {
            other_len = kfifo_get_free_space(&g_assist_fifo->talk_mic_fifo[i]);
            if (talk_len != other_len) {
            TRACE(3, "[%s] WARNING: talk_mic_fifo %d != %d; i = %d", __func__, talk_len, other_len, i);
            }
        }

        for (uint8_t i = 0; i < g_ref_ch_num; i++) {
            other_len = kfifo_get_free_space(&g_assist_fifo->ref_fifo[i]);
            if (talk_len != other_len) {
            TRACE(3, "[%s] WARNING: ref_fifo %d != %d; i = %d", __func__, talk_len, other_len, i);
            }
        }
#ifdef ANC_ASSIST_VPU
        other_len = kfifo_get_free_space(&g_assist_fifo->vpu_fifo);
        if (talk_len != other_len) {
            TRACE(3, "[%s] WARNING: vpu_fifo %d != %d", __func__, talk_len, other_len);
        }
#endif
    }
}

void app_anc_assist_process_put_data(float *in_buf[], uint32_t frame_len)
{
    if (g_assist_fifo == NULL) {
        TRACE(1, "[%s] g_assist_fifo is NULL", __func__);
        return;
    }

    uint32_t len = frame_len * sizeof(float);
    if (kfifo_get_free_space(&g_assist_fifo->talk_mic_fifo[g_talk_ch_num - 1]) >= len) {

        for (uint8_t i = 0; i < g_ff_ch_num; i++) {
            kfifo_put(&g_assist_fifo->ff_mic_fifo[i], (uint8_t *)in_buf[MIC_INDEX_FF + i], len);
        }
        for (uint8_t i = 0; i < g_fb_ch_num; i++) {
            kfifo_put(&g_assist_fifo->fb_mic_fifo[i], (uint8_t *)in_buf[MIC_INDEX_FB + i], len);
        }
        for (uint8_t i = 0; i < g_ref_ch_num; i++) {
            kfifo_put(&g_assist_fifo->ref_fifo[i], (uint8_t *)in_buf[MIC_INDEX_REF + i], len);
        }
#ifdef ANC_ASSIST_VPU
        kfifo_put(&g_assist_fifo->vpu_fifo, (uint8_t *)in_buf[MIC_INDEX_VPU], len);
#endif
        // BTH put talk fifo in the end, M55 use this fifo to check length
        for (uint8_t i = 0; i < g_talk_ch_num; i++) {
            kfifo_put(&g_assist_fifo->talk_mic_fifo[i], (uint8_t *)in_buf[MIC_INDEX_TALK + i], len);
        }
    } else {
        TRACE(0, "[%s] Input buffer is overflow", __FUNCTION__);
        g_fifo_overflow_cnt++;
    }
    g_fifo_total_cnt++;

    check_fifo_pcm_len();
}

void app_anc_assist_send_data_to_dsp(float *in_buf[], uint32_t frame_len)
{
    app_anc_assist_process_put_data(in_buf, frame_len);
#ifdef ANC_ASSIST_TRIGGER_DSP_PROCESS
    app_rpc_send_cmd(ANC_ASSIST_CORE, TASK_ANC_ASSIST_DATA_NO_RSP, NULL, 0);
#endif
}

void app_anc_assist_send_cmd_to_dsp(anc_assist_user_t user, app_anc_assist_core_cmd_t cmd)
{
    app_anc_assist_core_to_dsp_data_t tx_cmd;
    uint32_t send_len = sizeof(app_anc_assist_core_to_dsp_data_t) - sizeof(tx_cmd.ctrl_buf);

    memset(&tx_cmd, 0, sizeof(tx_cmd));
    tx_cmd.user = user;
    tx_cmd.cmd = cmd;

    app_rpc_send_cmd(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, (uint8_t *)&tx_cmd, send_len);
    int res = app_anc_assist_sema_wait(APP_ANC_ASSIST_TIMEROUT_MS);
    if (res != osOK) {
        TRACE(2,"%s err = %d",__func__,res);
    }

    TRACE(3, "[%s] user: %d, cmd: %d. OK", __func__, user, cmd);
}

void app_anc_assist_send_ctrl_to_dsp(anc_assist_user_t user, app_anc_assist_core_cmd_t cmd, uint32_t ctrl, uint8_t *buf, uint32_t len)
{
    app_anc_assist_core_to_dsp_data_t tx_cmd;
    uint32_t send_len = sizeof(app_anc_assist_core_to_dsp_data_t) - sizeof(tx_cmd.ctrl_buf) + len;

    memset(&tx_cmd, 0, sizeof(tx_cmd));
    tx_cmd.user = user;
    tx_cmd.cmd = cmd;
    tx_cmd.ctrl = ctrl;
    tx_cmd.ctrl_buf_len = len;
    if (len > 0) {
        memcpy(tx_cmd.ctrl_buf, buf, len);
    }

    app_rpc_send_cmd(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, (uint8_t *)&tx_cmd, send_len);
    int res = app_anc_assist_sema_wait(APP_ANC_ASSIST_TIMEROUT_MS);
    if (res != osOK) {
        TRACE(2,"%s err = %d",__func__,res);
    }

    TRACE(4, "[%s] user: %d, cmd: %d, ctrl: %d. OK", __func__, user, cmd, ctrl);
}


uint32_t app_anc_assist_core_get_status(void)
{
    return g_core_user_status;
}

static int app_anc_assist_dsp_wait_inited(void)
{
    uint32_t wait_cnt = 0;
#if defined(ANC_ASSIST_ON_DSP_M55)
    while (app_dsp_m55_is_running() == false)
#elif defined(ANC_ASSIST_ON_DSP_HIFI)
    while (app_dsp_hifi4_is_running() == false)
#elif defined(ANC_ASSIST_ON_DSP_SENS)
    // FIXME: It's a temporary method
    hal_sys_timer_delay_us(50000);
    while (app_sensor_hub_is_inited() == false)
#endif
    {
        hal_sys_timer_delay_us(10);

        if (wait_cnt++ > 300000) {      // 3s
            ASSERT(0, "%s dsp is hung", __func__);
        }
    }
    return 0;
}

void app_anc_assist_core_open(anc_assist_user_t user)
{
    TRACE(4, "[%s] user: %d, status: 0x%x", __func__, user, g_core_user_status);

    _anc_assist_core_lock();
    if ((g_core_user_status & (0x1 << user)) != 0) {
        TRACE(2, "[%s] WARNING: user(%d) has been opend", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (app_anc_assist_core_user_is_suspend(user)) {
        TRACE(2, "[%s] WARNING: user(%d) is suspending", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (ANC_ASSIST_CORE < APP_RPC_CORE_QTY) {
        if (g_core_user_status == 0) {
            g_fifo_total_cnt = 0;
            g_fifo_overflow_cnt = 0;
            app_anc_assist_rpc_init();
#if defined(ANC_ASSIST_ON_DSP_M55)
            app_dsp_m55_init(APP_DSP_M55_USER_VOICE_ASSIST);
#endif
#if defined(ANC_ASSIST_ON_DSP_HIFI)
            app_dsp_hifi4_init(APP_DSP_HIFI4_USER_VOICE_ASSIST);
#endif
#if defined(ANC_ASSIST_ON_DSP_SENS)
            app_sensor_hub_init();
#endif
        }

        app_anc_assist_dsp_wait_inited();
#if defined(ANC_ASSIST_ON_DSP_HIFI)
#ifdef BTH_AS_MAIN_MCU
        hal_cmu_axi_freq_req(HAL_CMU_AXI_FREQ_USER_DSP, HAL_CMU_AXI_FREQ_96M);
#endif
#endif
 
        // Check overlay case
        if (user > ANC_ASSIST_USER_ALGO_DSP) {
         //   app_second_overlay_subsys_select(APP_OVERLAY_M55, APP_SECOND_OVERLAY_SUBSYS_NON_CALL);
        }

        app_anc_assist_send_cmd_to_dsp(user, APP_ANC_ASSIST_CORE_OPEN);
        g_core_user_status = g_core_user_status | (0x1 << user);
    } else {
        ASSERT(0, "[%s] core (%d) not supported", __FUNCTION__, ANC_ASSIST_CORE);
    }
    _anc_assist_core_unlock();
}

void app_anc_assist_core_close(anc_assist_user_t user)
{
    TRACE(4, "[%s], user: %d, status: 0x%x", __func__, user, g_core_user_status);

    _anc_assist_core_lock();
    if ((g_core_user_status & (0x1 << user)) == 0) {
        TRACE(2, "[%s] WARNING: user(%d) has been closed", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (app_anc_assist_core_user_is_suspend(user)) {
        TRACE(2, "[%s] WARNING: user(%d) is suspending", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (ANC_ASSIST_CORE < APP_RPC_CORE_QTY) {
        app_anc_assist_send_cmd_to_dsp(user, APP_ANC_ASSIST_CORE_CLOSE);
        g_core_user_status = g_core_user_status & ~(0x1 << user);
        if (g_core_user_status == 0) {
#if defined(ANC_ASSIST_ON_DSP_M55)
            app_dsp_m55_deinit(APP_DSP_M55_USER_VOICE_ASSIST);
#elif defined(ANC_ASSIST_ON_DSP_HIFI)
#ifdef BTH_AS_MAIN_MCU
            hal_cmu_axi_freq_req(HAL_CMU_AXI_FREQ_USER_DSP, HAL_CMU_AXI_FREQ_32K);
#endif
            app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_VOICE_ASSIST);
#elif defined(ANC_ASSIST_ON_DSP_SENS)
            app_sensor_hub_deinit();
#endif
            app_anc_assist_rpc_deinit();
            TRACE(3, "[%s] total_cnt: %d, overflow_cnt: %d", __func__, g_fifo_total_cnt, g_fifo_overflow_cnt);
        }
    } else {
        ASSERT(0, "[%s] core (%d) not supported", __FUNCTION__, ANC_ASSIST_CORE);
    }
    _anc_assist_core_unlock();
}

void app_anc_assist_core_reset(anc_assist_user_t user)
{
    TRACE(4, "[%s] user: %d, status: 0x%x", __func__, user, g_core_user_status);

    _anc_assist_core_lock();

    if ((g_core_user_status & (0x1 << user)) == 0) {
        TRACE(2, "[%s] WARNING: user(%d) has been closed, can not reset", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (app_anc_assist_core_user_is_suspend(user)) {
        TRACE(2, "[%s] WARNING: user(%d) is suspending", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (ANC_ASSIST_CORE < APP_RPC_CORE_QTY) {
        app_anc_assist_send_cmd_to_dsp(user, APP_ANC_ASSIST_CORE_RESET);
    } else {
        ASSERT(0, "[%s] core (%d) not supported", __FUNCTION__, ANC_ASSIST_CORE);
    }
    _anc_assist_core_unlock();
}

void app_anc_assist_core_ctrl(anc_assist_user_t user, uint32_t ctrl, uint8_t *buf, uint32_t len)
{
    TRACE(5, "[%s] user: %d, status: 0x%x, ctrl: %d", __func__, user, g_core_user_status, ctrl);

    _anc_assist_core_lock();

    if ((g_core_user_status & (0x1 << user)) == 0) {
        TRACE(2, "[%s] WARNING: user(%d) has been closed, can not set ctrl", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (app_anc_assist_core_user_is_suspend(user)) {
        TRACE(2, "[%s] WARNING: user(%d) is suspending", __func__, user);
        _anc_assist_core_unlock();
        return;
    }

    if (ANC_ASSIST_CORE < APP_RPC_CORE_QTY) {
        app_anc_assist_send_ctrl_to_dsp(user, APP_ANC_ASSIST_CORE_CTRL, ctrl, buf, len);
    } else {
        ASSERT(0, "[%s] core (%d) not supported", __FUNCTION__, ANC_ASSIST_CORE);
    }
    _anc_assist_core_unlock();
}

void app_anc_assist_core_process(float *in_buf[],  uint32_t frame_len)
{
    if (g_core_user_status == 0) {
        return;
    }

    if (ANC_ASSIST_CORE < APP_RPC_CORE_QTY) {
        app_anc_assist_send_data_to_dsp(in_buf, frame_len);
    } else {
        ASSERT(0, "[%s] core (%d) not supported", __FUNCTION__, ANC_ASSIST_CORE);
    }
}

static uint32_t g_suspend_users = 0;

static bool g_core_suspend_users_flag = false;
static uint32_t g_core_suspend_users_cache_status = 0;

bool app_anc_assist_core_user_is_suspend(anc_assist_user_t user)
{
    return (g_core_suspend_users_flag && (g_suspend_users & (0x1 << user)));
}

void app_anc_assist_core_suspend_users(bool en, uint32_t users)
{
    uint32_t user = 0;
    uint32_t suspend_users_tab = 0;
    TRACE(3, "[%s] en: %d, status: 0x%x", __func__, en, g_core_user_status);

    if (g_core_suspend_users_flag == en) {
        TRACE(2, "[%s] WARNING: Invalid suspend: %d", __func__, en);
        return;
    }

    if (en) {
        g_core_suspend_users_cache_status = 0;

        user = 0;
        suspend_users_tab = users;
        while (suspend_users_tab) {
            if (users & g_core_user_status & (0x1 << user)) {
                app_anc_assist_core_close(user);
                g_core_suspend_users_cache_status |= (0x1 << user);
            }
            suspend_users_tab &= ~(0x1 << user);
            user++;
        }

        g_core_suspend_users_flag = true;
    } else {
        g_core_suspend_users_flag = false;

        user = 0;
        suspend_users_tab = g_suspend_users;
        while (suspend_users_tab) {
            if (g_core_suspend_users_cache_status & (0x1 << user)) {
                if (g_core_user_status & (0x1 << user)) {
                    TRACE(2, "[%s] WARNING: Invalid user(%d) anti-suspend", __func__, user);
                } else {
                    app_anc_assist_core_open(user);
                }
            }
            suspend_users_tab &= ~(0x1 << user);
            user++;
        }

        g_core_suspend_users_cache_status = 0;
    }
    g_suspend_users = users;
}

void app_anc_assist_core_init(void)
{
    _anc_assist_core_create_lock();
}

void app_anc_assist_core_deinit(void)
{
    ;
}
#endif

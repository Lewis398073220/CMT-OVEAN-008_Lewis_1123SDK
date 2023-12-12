/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#include "kfifo.h"
#include "cmsis_os.h"
#include "app_utils.h"
#include "app_tota.h"
#include "app_spp_tota.h"
#include "app_tota_common.h"
#include "app_tota_cmd_code.h"
#include "app_tota_audio_dump.h"
#include "app_tota_cmd_handler.h"

#define AUDIO_DUMP_SELF_FORMAT_HEAD_SIZE    (2)
#define AUDIO_DUMP_SELF_FORMAT_FRAME_INDEX  (4)
#define AUDIO_DUMP_SELF_FORMAT_TOTAL_SIZE   (AUDIO_DUMP_SELF_FORMAT_HEAD_SIZE + AUDIO_DUMP_SELF_FORMAT_FRAME_INDEX)

static bool g_stream_started = false;
static uint32_t g_data_frame_index = 0;

#define FIFO_BUF_SIZE       (1024 * 2)  // NOTE: the size must be 2^N
static uint8_t g_data_fifo_buf[FIFO_BUF_SIZE];
static struct kfifo g_data_fifo;


#define THREAD_SIGNAL_PUT_DATA  (0x01)
#define THREAD_STACK_SIZE       (1024 * 4)
static void tota_audio_dump_thread(void const *argument);
osThreadId tota_audio_dump_thread_tid;
osThreadDef(tota_audio_dump_thread, osPriorityHigh, 1, THREAD_STACK_SIZE, "TOTA_AUDIO_DUMP_THREAD");

#if defined(AUDIO_DEBUG) && !defined(AUDIO_DEBUG_VIA_SPP)
extern "C" int audio_test_stream_start(bool on);
#endif

#ifdef AUDIO_DEBUG_VIA_SPP
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
static char g_bt_debug_tx_buf[128];

extern "C" void app_debug_tool_printf(const char *fmt, ...)
{
    va_list arg;
    char *buf_ptr = g_bt_debug_tx_buf;

    memset(g_bt_debug_tx_buf, 0, sizeof(g_bt_debug_tx_buf));
    memcpy(buf_ptr, "[trace]", strlen("[trace]"));
    buf_ptr += strlen("[trace]");

    va_start(arg, fmt);
    vsprintf(buf_ptr, fmt, arg);
    va_end(arg);

    strcat(buf_ptr, "\r");

    ASSERT(strlen(g_bt_debug_tx_buf) < sizeof(g_bt_debug_tx_buf),
        "[%s] Invalid length: %d", __func__, strlen(g_bt_debug_tx_buf));

    TRACE(2, "[%s] %s", __func__, g_bt_debug_tx_buf);
    app_spp_tota_send_via_spp_direct((uint8_t *)g_bt_debug_tx_buf, strlen(g_bt_debug_tx_buf));
}
#else
static void POSSIBLY_UNUSED _audio_dump_self_pack(uint8_t *buf, uint32_t len)
{
    ASSERT(buf != NULL, "[%s] buf is NULL", __func__);
    ASSERT(len >= AUDIO_DUMP_SELF_FORMAT_TOTAL_SIZE, "[%s] len(%d) is too small", __func__, len);

    *(uint16_t *)buf = OP_TOTA_AUDIO_DUMP_START;
    buf += AUDIO_DUMP_SELF_FORMAT_HEAD_SIZE;
    *(uint32_t *)buf = g_data_frame_index++;
}
#endif

static void tota_audio_dump_thread(void const *argument)
{
    osEvent evt;
    uint32_t signals = 0;
    uint8_t buf[MAX_SPP_PACKET_SIZE];
#ifdef AUDIO_DEBUG_VIA_SPP
    uint32_t data_len = MAX_SPP_PACKET_SIZE;
#else
    uint32_t data_len = MAX_SPP_PACKET_SIZE - TOTA_PACKET_VERIFY_SIZE - AUDIO_DUMP_SELF_FORMAT_TOTAL_SIZE;
#endif

    TOTA_LOG_DBG(0, "[%s] Start ...", __func__);
    while (1) {
        evt = osSignalWait(0x0, osWaitForever);
        signals = evt.value.signals;

        if (evt.status == osEventSignal) {
            if (signals & THREAD_SIGNAL_PUT_DATA) {
                while(kfifo_len(&g_data_fifo) >= data_len) {
#ifdef AUDIO_DEBUG_VIA_SPP
                    kfifo_get(&g_data_fifo, buf, data_len);
                    app_spp_tota_send_via_spp_direct(buf, data_len);
#else
                    _audio_dump_self_pack(buf, AUDIO_DUMP_SELF_FORMAT_TOTAL_SIZE);  // Will be changed by app_tota_send, so need to set it again.
                    kfifo_get(&g_data_fifo, buf + AUDIO_DUMP_SELF_FORMAT_TOTAL_SIZE, data_len);
                    app_tota_send(buf, data_len + AUDIO_DUMP_SELF_FORMAT_TOTAL_SIZE, OP_TOTA_NONE);
#endif
                }
            } else {
                TRACE(2, "[%s] ERROR: signals = 0x%x", __func__, signals);
            }
        } else {
           TRACE(2, "[%s] ERROR: evt.status = %d", __func__, evt.status);
        }
    }
}

#include "app_ibrt_if.h"
static void _tota_audio_dump_exit_bt_sniff(void)
{
    TOTA_LOG_MSG(1, "[%s] ...", __func__);

    uint8_t* mobileAddr = app_ibrt_if_get_bt_local_address();
    app_tws_ibrt_exit_sniff_with_tws();
    app_ibrt_if_exit_sniff_with_mobile(mobileAddr);
    app_ibrt_if_prevent_sniff_set(mobileAddr, AVRCP_STATUS_CHANING);
}

/* ------------------------------ Register event ------------------------------ */
static void _tota_audio_dump_connected(void)
{
    TOTA_LOG_DBG(1, "[%s] ...", __func__);
}

static void _tota_audio_dump_disconnected(void)
{
    TOTA_LOG_DBG(1, "[%s] ...", __func__);
}

static const app_tota_callback_func_t s_func = {
    _tota_audio_dump_connected,
    _tota_audio_dump_disconnected,
    NULL,
    NULL,
};

/* ------------------------------ TOTA CMD ------------------------------ */
int32_t app_tota_audio_dump_start(void)
{
    TOTA_LOG_DBG(1, "[%s] ...", __func__);

    app_sysfreq_req(APP_SYSFREQ_USER_TOTA, APP_SYSFREQ_104M);
    kfifo_init(&g_data_fifo, g_data_fifo_buf, FIFO_BUF_SIZE);

    g_data_frame_index = 0;
    g_stream_started = true;

#if defined(AUDIO_DEBUG) && !defined(AUDIO_DEBUG_VIA_SPP)
    audio_test_stream_start(true);
#endif

    return 0;
}

int32_t app_tota_audio_dump_stop(void)
{
    TOTA_LOG_DBG(1, "[%s] ...", __func__);

#if defined(AUDIO_DEBUG) && !defined(AUDIO_DEBUG_VIA_SPP)
    audio_test_stream_start(false);
#endif

    g_stream_started = false;
    app_sysfreq_req(APP_SYSFREQ_USER_TOTA, APP_SYSFREQ_32K);

    return 0;
}

static void _audio_dump_control(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    switch (funcCode) {
    case OP_TOTA_AUDIO_DUMP_START:
        TOTA_LOG_DBG(0, "tota_audio_dump start");
        _tota_audio_dump_exit_bt_sniff();
        app_tota_audio_dump_start();
        break;

    case OP_TOTA_AUDIO_DUMP_STOP:
        TOTA_LOG_DBG(0, "tota_audio_dump stop");
        app_tota_audio_dump_stop();
        break;

    case OP_TOTA_AUDIO_DUMP_CONTROL:
        TOTA_LOG_DBG(0, "tota_audio_dump contorl");
        // TODO: Can get or set info
        app_tota_send_rsp(funcCode, TOTA_NO_ERROR, (uint8_t *)".pcm", sizeof(".pcm"));
        break;

    default:
        break;
    }
}

TOTA_COMMAND_TO_ADD(OP_TOTA_AUDIO_DUMP_START, _audio_dump_control, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_AUDIO_DUMP_STOP, _audio_dump_control, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_AUDIO_DUMP_CONTROL, _audio_dump_control, false, 0, NULL );

/* ------------------------------ API ------------------------------ */
int32_t app_tota_audio_dump_init(void)
{
    TOTA_LOG_DBG(1, "[%s] ...", __func__);

    g_stream_started = false;
    g_data_frame_index = 0;
    tota_audio_dump_thread_tid = osThreadCreate(osThread(tota_audio_dump_thread), NULL);
    app_tota_callback_module_register(APP_TOTA_AUDIO_DUMP, &s_func);

    return 0;
}

int32_t app_tota_audio_dump_send(uint8_t *buf, uint32_t len)
{
    if (g_stream_started == false) {
        TOTA_LOG_DBG(0, "[%s] WARNING: Stream is not started", __func__);
        return -1;
    }

    if (kfifo_get_free_space(&g_data_fifo) >= len) {
        kfifo_put(&g_data_fifo, buf, len);
        osSignalSet(tota_audio_dump_thread_tid, THREAD_SIGNAL_PUT_DATA);
    } else {
        TRACE(0, "[%s] Input buffer is overflow. Try to increase FIFO_BUF_SIZE or decrease dump data", __func__);
        return -2;
    }

    return 0;
}

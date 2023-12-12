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
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "dsp_m55.h"
#include "dsp_m55_core.h"
#include "dsp_m55_comp_test.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#if defined(SUBSYS_FLASH_BOOT) && !defined(NO_FLASH_INIT)
#include "hal_norflash.h"
#endif
#include "hal_sys2bth.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "main_entry.h"
#include "dsp_m55_trc_to_mcu.h"
#include "dsp_m55_core_app.h"
#include "heap_api.h"
#if defined(DSP_M55_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
#include "rmt_trace_client.h"
#endif

#ifdef DMA_AUDIO_APP
#include "dma_audio_app.h"
#endif
#include "app_utils.h"

#ifdef ANC_ASSIST_DSP
#include "anc_assist_dsp.h"
#endif

static uint32_t g_cpu_freq = 96 * 1000000;

uint32_t get_cpu_freq_MHz(void)
{
    return (g_cpu_freq / 1000000);
}

int32_t set_cpu_freq_MHz(uint32_t freq)
{
    TRACE(2, "[%s] freq: %d", __func__, freq);
    g_cpu_freq = freq;

    return 0;
}

static dsp_m55_core_rx_irq_handler_t core_rx_handler[CORE_IRQ_HDLR_ID_QTY]={0,};
static dsp_m55_core_tx_done_irq_handler_t core_tx_handler[CORE_IRQ_HDLR_ID_QTY]={0,};

void dsp_m55_core_register_rx_irq_handler(enum CORE_IRQ_HDLR_ID_T id, dsp_m55_core_rx_irq_handler_t irqHandler)
{
    ASSERT(id<CORE_IRQ_HDLR_ID_QTY, "%s: invalid id %d", __func__, id);
    core_rx_handler[id] = irqHandler;
}

void dsp_m55_core_register_tx_done_irq_handler(enum CORE_IRQ_HDLR_ID_T id, dsp_m55_core_tx_done_irq_handler_t irqHandler)
{
    ASSERT(id<CORE_IRQ_HDLR_ID_QTY, "%s: invalid id %d", __func__, id);
    core_tx_handler[id] = irqHandler;
}

#ifdef NOAPP
static const uint8_t handshake_msg[] = "Hi";
#endif

#if defined(SLEEP_TEST) && !defined(NO_TIMER)
static HWTIMER_ID wakeup_timer;
static uint32_t wakeup_cnt;
static const uint32_t timer_ms = 7;

static void timer_handler(void *param)
{
    dsp_m55_send(&wakeup_cnt, sizeof(wakeup_cnt));
    wakeup_cnt++;
    hwtimer_start(wakeup_timer, MS_TO_TICKS(timer_ms));
}
#endif

static unsigned int rx_handler(const void *data, unsigned int len)
{
    uint32_t i = CORE_IRQ_HDLR_PRIO_LOW;
    uint32_t ret = len;

#if defined(DSP_M55_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
    if (rmt_trace_client_msg_handler(data, len)) {
        return len;
    }
#endif

#ifdef DMA_AUDIO_APP
    for (i = 0; i < CORE_IRQ_HDLR_ID_QTY; i++) {
        if (core_rx_handler[i]) {
            ret = core_rx_handler[i](data, len);
            if (ret == len) {
                break;
            }
        }
    }
#else
    if (core_rx_handler[i]) {
        ret = core_rx_handler[i](data, len);
    }
#endif
    return ret;
}

static void tx_handler(const void *data, unsigned int len)
{
    uint32_t i = CORE_IRQ_HDLR_PRIO_LOW;

#if defined(DSP_M55_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
    if (rmt_trace_check_trace(data, len)) {
        return;
    }
#endif

#ifdef DMA_AUDIO_APP
    for (i = 0; i < CORE_IRQ_HDLR_ID_QTY; i++)
#endif
    {
        if (core_tx_handler[i]) {
            core_tx_handler[i](data, len);
        }
    }
}

int dsp_m55_send_seq(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_sys2bth_send_seq(HAL_SYS2BTH_ID_0, data, len, seq);
}

int dsp_m55_send(const void *data, unsigned int len)
{
    return hal_sys2bth_send(HAL_SYS2BTH_ID_0, data, len);
}

int dsp_m55_tx_active(unsigned int seq)
{
    return hal_sys2bth_tx_active(HAL_SYS2BTH_ID_0, seq);
}

//#define TEST_FREQ_CURRENT
#ifdef TEST_FREQ_CURRENT
#include "string.h"

#define MAT_M 16
#define MAT_N 16
#define MAT_K 32

static uint16_t A[MAT_M*MAT_K];
static uint16_t B[MAT_K*MAT_N];
static uint32_t Cn[MAT_M*MAT_N];
//static uint32_t Cb[MAT_M*MAT_N];

void naive_mmult_aTb(const uint16_t *A, const uint16_t *B, uint32_t *C, int M, int N, int K)
{
    int i,j,p;
    uint32_t dot_p;

    for (i = 0; i < M; i++) {
      for (j = 0; j < N; j++) {
        // Calculate the dot product between A.column and B.column:
        dot_p = 0;
        for (p = 0; p < K; p++) {
           dot_p += A[i+p*M] * B[p*N+j]; // A[M][K] * B[K][N]
        }
        C[i*N+j] = dot_p;
      }
    }
}

void matrix_test(void)
{
    memset(A, 0, sizeof(A));
    memset(B, 0, sizeof(B));
    // 0 0...    4500 4501 4502 ...         0    0    0 ...
    // 1 0... x     0    0    0 ...    = 4500 4501 4502 ...
    // 2 0...                            9000 ...
    for(int i=0; i< 16; i++) { A[i] = i; B[i] = i+4500; }

    for (int k = 0; k < 1000; k++) {
        naive_mmult_aTb(A, B, Cn, MAT_M, MAT_N, MAT_K);
    }
}
#endif

// #define TEST_SINGLE_WIRE_COM
#ifdef TEST_SINGLE_WIRE_COM
void communication_init(void);
int communication_send_buf(uint8_t * buf, uint8_t len);
uint8_t test_buf[]={1,2,3,4,5,6,7,8,9};
#endif

int MAIN_ENTRY(void)
{
    int ret;

    hwtimer_init();
#if defined(DMA_APPLIED_ON_M55)
    hal_dma_open();
#endif

#ifdef DSP_M55_TRC_TO_MCU
    dsp_m55_trace_to_mcu();
#endif

#ifdef DEBUG
    enum HAL_TRACE_TRANSPORT_T transport;

#ifdef DSP_M55_TRC_TO_MCU
    transport = HAL_TRACE_TRANSPORT_NULL;
#elif (DEBUG_PORT == 1)
    hal_iomux_set_uart0();

    transport = HAL_TRACE_TRANSPORT_UART0;
#elif (DEBUG_PORT == 2)
    hal_iomux_set_uart1();

    transport = HAL_TRACE_TRANSPORT_UART1;
#else
    transport = HAL_TRACE_TRANSPORT_QTY;
#endif
    hal_trace_open(transport);
#endif

    hal_sleep_start_stats(10000, 10000);

    ret = hal_sys2bth_open(HAL_SYS2BTH_ID_0, rx_handler, tx_handler, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(HAL_SYS2BTH_ID_0);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);

#ifdef ANC_ASSIST_DSP
    anc_assist_dsp_init();
    anc_assist_dsp_open();
#endif

#if defined(SUBSYS_FLASH_BOOT) && !defined(NO_FLASH_INIT)
    hal_norflash_show_id_state(HAL_FLASH_ID_0, true);
#endif

#ifdef DMA_AUDIO_APP
    dma_audio_app_init(true);
#ifndef DMA_AUDIO_APP_DYN_ON
    dma_audio_app_on(true);
#endif
#endif

#ifdef NOAPP
    ret = hal_sys2bth_send(HAL_SYS2BTH_ID_0, handshake_msg, (sizeof(handshake_msg) - 1));
    ASSERT(ret == 0, "hal_sys2bth_send failed: %d", ret);
#else
    dsp_m55_core_app_init();
#endif

#ifdef SMF_RPC_M55
   extern void smf_test_entry(); smf_test_entry();
#endif

#ifdef MCU2DSP_M55_MSG_TEST
    mcu2dsp_m55_msg_test();
#endif

#if defined(SLEEP_TEST) && !defined(NO_TIMER)
    wakeup_timer = hwtimer_alloc(timer_handler, NULL);
    ASSERT(wakeup_timer, "Failed to alloc wakeup_timer");

    ret = hwtimer_start(wakeup_timer, MS_TO_TICKS(timer_ms));
#endif

#if defined(SLEEP_TEST) || !defined(NOAPP)
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);
#else
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);
#endif
    hal_sysfreq_print_user_freq();

#ifdef TEST_SINGLE_WIRE_COM
    communication_init();
#endif

#ifdef APP_MCPP_SRV
    extern void mcpp_srv_open(void);
    mcpp_srv_open();
#endif

#ifdef TEST_FREQ_CURRENT
    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_26M);
    while (1) {
        matrix_test();
    }
#endif

    while (1) {
#ifdef DMA_AUDIO_APP
        dma_audio_app_thread();
#endif /* DMA_AUDIO_APP */

#ifdef TEST_SINGLE_WIRE_COM
        communication_send_buf(test_buf, sizeof(test_buf));
        osSignalWait(0x0, 5000);
#endif /* TEST_SINGLE_WIRE_COM */

#ifdef SLEEP_TEST
#ifdef RTOS
        osSignalWait(0x0, osWaitForever);
#else
        hal_sleep_enter_sleep();
#endif
#else /* !SLEEP_TEST */
        osDelay(1000);
#endif /* SLEEP_TEST */

    }

    return 0;
}

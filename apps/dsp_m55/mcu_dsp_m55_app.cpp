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
#if !(defined(CHIP_ROLE_CP) || defined(CHIP_SUBSYS_SENS) || (defined(CHIP_SUBSYS_BTH) ^ defined(BTH_AS_MAIN_MCU)))
#include <string.h>
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "pmu.h"
#include "dsp_m55.h"
#include "dsp_m55_core.h"
#include "app_dsp_m55.h"
#include "mcu_dsp_m55_app.h"
#include "app_overlay.h"
#include "mpu.h"
#ifdef DMA_AUDIO_APP
#include "dma_audio_host.h"
#include "stream_dma_rpc.h"
#endif


static uint32_t app_dsp_m55_init_bitmap = 0;
static bool g_m55_core_runing = false;
static bool dsp_m55_is_running = false;

static dsp_m55_core_rx_irq_handler_t core_rx_handler[CORE_IRQ_HDLR_ID_QTY];
static dsp_m55_core_tx_done_irq_handler_t core_tx_handler[CORE_IRQ_HDLR_ID_QTY];
static osMutexId mcu_dsp_m55_mutex_id = NULL;
osMutexDef(mcu_dsp_m55_mutex);

static void app_dsp_m55_ping_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "mcu gets ping from dsp m55 core, time %d ms.", GET_CURRENT_MS());

    if ((!g_m55_core_runing) && (dsp_m55_is_running == true)) {
        g_m55_core_runing = true;
        app_dsp_m55_notify_tx_thread_status(true);
    }
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_PING,
                                "ping mcu",
                                NULL,
                                app_dsp_m55_ping_received_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);

static void app_dsp_m55_core_set_freq_received_handler(uint8_t* ptr, uint16_t len)
{
#if defined(BTH_AS_MAIN_MCU) && defined(BTH_USE_SYS_PERIPH)
    enum HAL_CMU_AXI_FREQ_T freq = (enum HAL_CMU_AXI_FREQ_T)*ptr;
    hal_cmu_axi_freq_req(HAL_CMU_AXI_FREQ_USER_DSP, freq);
#endif
}

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_DSP_M55_INSTANT_CMD_AXI_SYSFREQ_REQ,
                                NULL,
                                app_dsp_m55_core_set_freq_received_handler);

#ifdef DMA_AUDIO_APP
#define DMA_AUD_RSP_CMD(cmd) (0x8000|(cmd))
void app_dma_audio_check_conflict_cmd(void)
{
    uint32_t i;
    uint16_t cmd_code[] = {DMA_MSG_ID_RPC, DMA_AUD_RSP_CMD(DMA_MSG_ID_RPC)};
    app_dsp_m55_bridge_instant_cmd_instance_t *inst_i = NULL;
    app_dsp_m55_bridge_task_cmd_instance_t *inst_t = NULL;

    for (i = 0; i < ARRAY_SIZE(cmd_code); i++) {
        inst_i = app_dsp_m55_bridge_find_instant_cmd_entry(cmd_code[i]);
        ASSERT(inst_i==NULL, "%s: inst cmd 0x%x conflict", __func__, cmd_code[i]);
        inst_t = app_dsp_m55_bridge_find_task_cmd_entry(cmd_code[i]);
        ASSERT(inst_t==NULL, "%s: task cmd 0x%x conflict", __func__, cmd_code[i]);
    }
}

static bool app_dma_audio_cmd_filter(const void *data, unsigned int len)
{
    struct DMA_MSG_HDR_T *msg = (struct DMA_MSG_HDR_T *)data;

    if (len < sizeof(struct DMA_MSG_HDR_T)) {
        return false;
    }
    if (msg->id == DMA_MSG_ID_RPC) {
        return true;
    }
    return false;
}

#endif /* DMA_AUDIO_APP */

static void app_dsp_m55_register_rx_irq_handler(enum CORE_IRQ_HDLR_ID_T id,
    dsp_m55_core_rx_irq_handler_t handler)
{
    ASSERT(id<CORE_IRQ_HDLR_ID_QTY, "%s: invalid id %d", __func__, id);
    core_rx_handler[id] = handler;
}

static void app_dsp_m55_register_tx_irq_handler(enum CORE_IRQ_HDLR_ID_T id,
    dsp_m55_core_tx_done_irq_handler_t handler)
{
    ASSERT(id<CORE_IRQ_HDLR_ID_QTY, "%s: invalid id %d", __func__, id);
    core_tx_handler[id] = handler;
}

static void app_dsp_m55_init_handler(void)
{
#ifdef DMA_AUDIO_APP
    app_dma_audio_check_conflict_cmd();
    app_dsp_m55_register_rx_irq_handler(CORE_IRQ_HDLR_PRIO_HIGH, dma_audio_rx_data_handler);
    app_dsp_m55_register_tx_irq_handler(CORE_IRQ_HDLR_PRIO_HIGH, dma_audio_tx_data_handler);
#endif
    app_dsp_m55_register_rx_irq_handler(CORE_IRQ_HDLR_PRIO_LOW, app_dsp_m55_bridge_data_received);
    app_dsp_m55_register_tx_irq_handler(CORE_IRQ_HDLR_PRIO_LOW, app_dsp_m55_bridge_data_tx_done);
}

static void app_dsp_m55_init_module(void)
{
#ifdef DMA_AUDIO_APP
    dma_audio_init();
#endif

#ifdef SINGLE_WIRE_UART_PMU_ENABLE
    pmu_uart_enable();
#endif
}

static unsigned int dsp_m55_rx_handler(const void *data, unsigned int len)
{
    uint32_t i = CORE_IRQ_HDLR_PRIO_LOW;
    uint32_t ret = len;

#ifdef DMA_AUDIO_APP
    if (app_dma_audio_cmd_filter(data, len)) {
        i = CORE_IRQ_HDLR_PRIO_HIGH;
    }
#endif
    if (core_rx_handler[i]) {
        ret = core_rx_handler[i](data, len);
    }
    return ret;
}

static void dsp_m55_tx_handler(const void *data, unsigned int len)
{
    uint32_t i = CORE_IRQ_HDLR_PRIO_LOW;

#ifdef DMA_AUDIO_APP
    if (app_dma_audio_cmd_filter(data, len)) {
        i = CORE_IRQ_HDLR_PRIO_HIGH;
    }
#endif
    if (core_tx_handler[i]) {
        core_tx_handler[i](data, len);
    }
}


// mcu to initialize M55 module

#define DSP_M55_DELAY_CLOSE_TIME_IN_MS  (5000)
static void dsp_m55_delay_close_timer_handler(void const *param);
osTimerDef(DSP_M55_DELAY_CLOSE_TIMER, dsp_m55_delay_close_timer_handler);
static osTimerId dsp_m55_delay_close_timer_id = NULL;
static void dsp_m55_delay_close_timer_handler(void const *param)
{
    app_dsp_m55_bridge_trigger_turn_off();
}

void app_dsp_m55_delay_close_timer_start(void)
{
    if (NULL == dsp_m55_delay_close_timer_id) {
        dsp_m55_delay_close_timer_id = osTimerCreate(osTimer(DSP_M55_DELAY_CLOSE_TIMER), osTimerOnce, NULL);
    }

    osTimerStop(dsp_m55_delay_close_timer_id);
    osTimerStart(dsp_m55_delay_close_timer_id, DSP_M55_DELAY_CLOSE_TIME_IN_MS);
}

void app_dsp_m55_delay_close_timer_stop(void)
{
    if (dsp_m55_delay_close_timer_id) {
        osTimerStop(dsp_m55_delay_close_timer_id);
    }
}

static int dsp_m55_ram_access_enable(bool enable)
{
    int ret;

    if (enable) {
        ret = mpu_set(M55_ITCM_BASE, M55_ITCM_SIZE, MPU_ATTR_READ_WRITE, MEM_ATTR_INT_SRAM);
        if (ret) {
            TRACE(0, "mpu_set:M55 ITCM failed ret=%d", ret);
        }
        ret = mpu_set(M55_DTCM_BASE, M55_DTCM_SIZE, MPU_ATTR_READ_WRITE, MEM_ATTR_INT_SRAM);
        if (ret) {
            TRACE(0, "mpu_set:M55 DTCM failed ret=%d", ret);
        }
    } else {
        ret= mpu_clear(M55_ITCM_BASE, M55_ITCM_SIZE);
        if (ret) {
            TRACE(0, "mpu_clear:M55 ITCM failed ret=%d", ret);
        }
        ret = mpu_clear(M55_DTCM_BASE, M55_DTCM_SIZE);
        if (ret) {
            TRACE(0, "mpu_clear:M55 DTCM failed ret=%d", ret);
        }
    }

    return ret;
}

// mcu to initialize sensor hub module
void app_dsp_m55_init(APP_DSP_M55_USER_E user)
{
    TRACE(3, "[%s] user:%d bitmap:0x%x", __func__, user, app_dsp_m55_init_bitmap);

    osMutexWait(mcu_dsp_m55_mutex_id, osWaitForever);
    app_dsp_m55_delay_close_timer_stop();
    if ((!dsp_m55_is_running) && (0 == app_dsp_m55_init_bitmap)) {

        dsp_m55_is_running = true;

        app_dsp_m55_bridge_init();

        app_dsp_m55_init_handler();

        g_m55_core_runing = false;

#if defined(CHIP_SUBSYS_BTH) && defined(BTH_AS_MAIN_MCU) && defined(BTH_USE_SYS_PERIPH)
        // init dsp_m55 freq machine state, dsp m55 will automatically set freq to 24MHz.
        hal_cmu_axi_freq_req(HAL_CMU_AXI_FREQ_USER_DSP, HAL_CMU_AXI_FREQ_24M);
#endif

        //enable access DSP_M55's ram for BTH
        dsp_m55_ram_access_enable(true);

        int ret = dsp_m55_open(dsp_m55_rx_handler, dsp_m55_tx_handler);

        ASSERT(ret == 0, "dsp_m55_open failed: %d", ret);
        osDelay(5);

        app_dsp_m55_init_module();
    }

    app_dsp_m55_init_bitmap |= (1 << user);
    osMutexRelease(mcu_dsp_m55_mutex_id);
}

void dsp_m55_core_deinit(void)
{
    osMutexWait(mcu_dsp_m55_mutex_id, osWaitForever);

    if (!dsp_m55_is_running)
    {
        osMutexRelease(mcu_dsp_m55_mutex_id);
        return;
    }

    if (app_dsp_m55_init_bitmap) {
        osMutexRelease(mcu_dsp_m55_mutex_id);
        return;
    }

    dsp_m55_is_running = false;
    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_INSTANT_CMD_DISABLE_M55, NULL, 0);

    // wait core bridge thread send finished
    volatile uint32_t timeout_cnt = 20;
    uint32_t dly_list[] = {5,10,15,20,25};
    uint32_t txcnt = 0;
    do {
        txcnt = app_dsp_m55_bridge_get_tx_mailbox_cnt();
        if (txcnt > 0) {
            if (timeout_cnt == 0) {
                TRACE(2, "[%s]:WARNING: %d tx mailbox is not empty yet.", __func__, txcnt);
                break;
            }
            timeout_cnt--;
            osDelay(dly_list[timeout_cnt%5]);
        }
    } while (txcnt > 0);

    // clear & reset status for core bridge
    g_m55_core_runing = false;
    app_dsp_m55_notify_tx_thread_status(false);
    app_dsp_m55_bridge_deinit();

    // close low level data route
    dsp_m55_close();

    //BTH cannot access DSP_M55 ram after dsp_m55 was closed.
    dsp_m55_ram_access_enable(false);

#if defined(CHIP_SUBSYS_BTH) && defined(BTH_AS_MAIN_MCU) && defined(BTH_USE_SYS_PERIPH)
    // dsp_m55 close should reset dsp_m55 freq machine status as 32KHz.
    hal_cmu_axi_freq_req(HAL_CMU_AXI_FREQ_USER_DSP, HAL_CMU_AXI_FREQ_32K);
#endif

    osDelay(2);

    app_overlay_subsys_unloadall(APP_OVERLAY_M55);

    osMutexRelease(mcu_dsp_m55_mutex_id);
}

void app_dsp_m55_deinit(APP_DSP_M55_USER_E user)
{
    TRACE(3, "[%s] user:%d bitmap:0x%x", __func__, user, app_dsp_m55_init_bitmap);

    osMutexWait(mcu_dsp_m55_mutex_id, osWaitForever);
    uint32_t formerBitmap = app_dsp_m55_init_bitmap;
    app_dsp_m55_init_bitmap &= ~(1 << user);
    if ((formerBitmap > 0) && (0 == app_dsp_m55_init_bitmap)) {
        app_dsp_m55_delay_close_timer_start();
    }
    osMutexRelease(mcu_dsp_m55_mutex_id);
}

void app_dsp_m55_force_deinit(void)
{
    osMutexWait(mcu_dsp_m55_mutex_id, osWaitForever);
    app_dsp_m55_init_bitmap = APP_DSP_M55_USER_INIT;
    dsp_m55_core_deinit();
    osMutexRelease(mcu_dsp_m55_mutex_id);
}

bool app_dsp_m55_is_running(void)
{
    return g_m55_core_runing;
}

void app_dsp_m55_first_init(void)
{
    if (mcu_dsp_m55_mutex_id == NULL) {
        mcu_dsp_m55_mutex_id = osMutexCreate(osMutex(mcu_dsp_m55_mutex));
    }
}

static void app_dsp_m55_core_transmit_disable_m55_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(MCU_DSP_M55_INSTANT_CMD_DISABLE_M55, ptr, len);
}

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_DSP_M55_INSTANT_CMD_DISABLE_M55,
                                app_dsp_m55_core_transmit_disable_m55_handler,
                                NULL);
#endif

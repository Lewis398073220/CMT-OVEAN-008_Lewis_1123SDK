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
#ifndef NOAPP
#include "string.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hwtimer_list.h"
#include "mcu_dsp_m55_app.h"
#include "app_thread.h"
#include "dsp_m55_msg.h"
#include "dsp_m55_core.h"
#include "dsp_m55_core_app.h"
#include "app_dsp_m55.h"
#ifdef A2DP_DECODER_CROSS_CORE
#include "a2dp_decoder_cc_off_bth.h"
#endif
#ifdef GAF_CODEC_CROSS_CORE
#include "gaf_m55_stream.h"
#endif

/*
 * M55 CORE PING MSG
 ********************************************************
 */

static void app_dsp_m55_core_transmit_ping_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(MCU_DSP_M55_TASK_CMD_PING, ptr, len);
}

static void app_dsp_m55_ping_bth(void)
{
    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_TASK_CMD_PING, NULL, 0);
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_PING,
                                "ping mcu",
                                app_dsp_m55_core_transmit_ping_handler,
                                NULL,
                                0,
                                NULL,
                                NULL,
                                NULL);
/*
 * CORE CRASH DUMP MSG
 ********************************************************
 */

static void app_dsp_m55_core_transmit_set_freq_req_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(MCU_DSP_M55_INSTANT_CMD_AXI_SYSFREQ_REQ, ptr, len);
}

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_DSP_M55_INSTANT_CMD_AXI_SYSFREQ_REQ,
                                app_dsp_m55_core_transmit_set_freq_req_handler,
                                NULL);

static void app_dsp_m55_core_receive_disable_m55_handler(uint8_t* ptr, uint16_t len)
{
    // Avoid other task to run when the BTH need to disable M55
    POSSIBLY_UNUSED uint32_t lock = int_lock();
    while(1) {
        __WFI();
    }
}

M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_DSP_M55_INSTANT_CMD_DISABLE_M55,
                                NULL,
                                app_dsp_m55_core_receive_disable_m55_handler);

#define APP_DSP_M55_PING_MCU_TIMER_PERIOD_MS    (5000)

static void dsp_m55_ping_mcu_timer_handler(void const *param);
osTimerDef(dsp_m55_ping_mcu_timer, dsp_m55_ping_mcu_timer_handler);
static osTimerId dsp_m55_ping_mcu_timer_id = NULL;

static void dsp_m55_ping_mcu_timer_handler(void const *param)
{
    app_dsp_m55_ping_bth();
}

static void app_dsp_m55_timer_init(void)
{
    TRACE(0, "%s:", __func__);

    if (dsp_m55_ping_mcu_timer_id == NULL) {
        dsp_m55_ping_mcu_timer_id = osTimerCreate(osTimer(dsp_m55_ping_mcu_timer), osTimerPeriodic, NULL);
    }
    if (dsp_m55_ping_mcu_timer_id != NULL) {
        osTimerStart(dsp_m55_ping_mcu_timer_id, APP_DSP_M55_PING_MCU_TIMER_PERIOD_MS);
    }
}

void dsp_m55_core_deinit(void)
{
    // stub, do nothing
}

void dsp_m55_core_app_init(void)
{
    app_dsp_m55_bridge_init();

    dsp_m55_core_register_rx_irq_handler(CORE_IRQ_HDLR_PRIO_LOW, app_dsp_m55_bridge_data_received);

    dsp_m55_core_register_tx_done_irq_handler(CORE_IRQ_HDLR_PRIO_LOW, app_dsp_m55_bridge_data_tx_done);

    app_dsp_m55_notify_tx_thread_status(true);
#ifdef A2DP_DECODER_CROSS_CORE
    a2dp_decoder_cc_off_bth_init();
#endif
#ifdef GAF_CODEC_CROSS_CORE
    gaf_m55_stream_init();
#endif
    app_dsp_m55_ping_bth();

    app_dsp_m55_timer_init();
}
#endif

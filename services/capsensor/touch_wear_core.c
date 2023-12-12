/***************************************************************************
 * Copyright 2022-2023 BES.
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
 ***************************************************************************/
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

#include "hal_trace.h"
#include "hal_timer.h"
#include "touch_wear_core.h"
#include "capsensor_driver.h"
#include "capsensor_algorithm.h"
#ifdef CAPSENSOR_SPP_SERVER
#include "capsensor_debug_server.h"
#endif
#if (CHIP_CAPSENSOR_VER < 1)
#include CHIP_SPECIFIC_HDR(hal_capsensor)
#endif
#ifdef CHIP_SUBSYS_SENS
#include "app_sensor_hub.h"
#endif
#include "capsensor_factory_cal.h"

#if (CHIP_CAPSENSOR_VER > 1)
#include "pmu.h"
#endif

// Global variables
static void capsensor_thread(const void *arg);
osThreadDef(capsensor_thread, osPriorityHigh, 1, (1024 * 2), "capsensor_thread");
static osThreadId capsensor_thread_id = NULL;

static struct capsensor_sample_data capsense_sample_buff[CAP_CHNUM*CAP_REPNUM]={0};
static capsensor_click_event_func_type capsensor_click_event_cb = NULL;

static void capsensor_timer_handler(void const *param);
osTimerDef(capsensor_timer, capsensor_timer_handler);
static osTimerId capsensor_timer_id = NULL;

/***************************************************************************
 * @brief cap sensor timer interrupt handler
 *
 ***************************************************************************/
static void capsensor_timer_handler(void const *param)
{
    read_capsensor_fifo(capsense_sample_buff);
    osSignalSet(capsensor_thread_id, 0x0001);
}

/***************************************************************************
 * @brief cap sensor timer start
 *
 ***************************************************************************/
void capsensor_timer_start(void)
{
    TRACE(0, "%s", __func__);

    capsensor_timer_id = osTimerCreate(osTimer(capsensor_timer), osTimerPeriodic, NULL);
    if (capsensor_timer_id != NULL) {
        osTimerStart(capsensor_timer_id, CAP_SAMP_FS);
    }
}

/***************************************************************************
 * @brief touch or wear event callback function
 *
 * @param p function pointer
 * @return int
 ***************************************************************************/
int register_capsensor_click_event_callback(capsensor_click_event_func_type p)
{
    int8_t ret = 0;

    if (p == NULL) {
        ret = -1;
        return ret;
    }

    capsensor_click_event_cb = p;
    return ret;
}

#ifdef CHIP_SUBSYS_SENS
/* no response command start */
/***************************************************************************
 * @brief
 *
 * @param ptr : message pointer.
 * @param len : message lengths.
 ***************************************************************************/
static void app_sensor_hub_core_transmit_touch_no_rsp_cmd_handler(uint8_t* ptr, uint16_t len)
{
    app_core_bridge_send_data_without_waiting_rsp(MCU_SENSOR_HUB_TASK_CMD_TOUCH_REQ_NO_RSP, ptr, len);
}


/***************************************************************************
 * @brief
 *
 * @param ptr : message pointer.
 * @param len : message lengths.
 ***************************************************************************/
static void app_sensor_hub_core_touch_no_rsp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    CAPSENSOR_TRACE(0, "Get touch no rsp command from mcu:");
    DUMP8("%02x ", ptr, len);
}

/***************************************************************************
 * @brief
 *
 * @param cmdCode
 * @param ptr : message pointer.
 * @param len : message lengths.
 ***************************************************************************/
static void app_sensor_hub_core_touch_no_rsp_cmd_tx_done_handler(uint16_t cmdCode,
    uint8_t* ptr, uint16_t len)
{
    CAPSENSOR_TRACE(0, "cmdCode 0x%x tx done", cmdCode);
}

/***************************************************************************
 * @brief
 *
 * @param ptr : message pointer.
 * @param len : message lengths.
 ***************************************************************************/
void app_sensor_hub_core_send_touch_req_no_rsp(uint8_t *ptr, uint16_t len)
{
    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_TOUCH_REQ_NO_RSP,
        ptr, len);
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_TOUCH_REQ_NO_RSP,
                                "touch no rsp req to mcu",
                                app_sensor_hub_core_transmit_touch_no_rsp_cmd_handler,
                                app_sensor_hub_core_touch_no_rsp_cmd_received_handler,
                                0,
                                NULL,
                                NULL,
                                app_sensor_hub_core_touch_no_rsp_cmd_tx_done_handler);
/* no response command end */

/***************************************************************************
 * @brief An instant message request from the sensor hub to the MCU.
 *
 * @param ptr : message pointer.
 * @param len : message lengths.
 ***************************************************************************/
static void app_sensor_hub_core_transmit_touch_instant_req_handler(uint8_t* ptr, uint16_t len)
{
    app_core_bridge_send_instant_cmd_data(MCU_SENSOR_HUB_INSTANT_CMD_TOUCH_REQ,
        ptr, len);
}


/***************************************************************************
 * @brief An instant message request from the MCU to the sensor hub.
 *
 * @param ptr : message pointer.
 * @param len : message lengths.
 ***************************************************************************/
static void app_sensor_hub_core_touch_instant_req_handler(uint8_t* ptr, uint16_t len)
{
    // for test purpose, we add log print here.
    // but as instant cmd handler will be directly called in intersys irq context,
    // for realistic use, should never do log print
    CAPSENSOR_TRACE(0, "Get demo instant req command from mcu:");
    DUMP8("%02x ", ptr, len);
}

CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_SENSOR_HUB_INSTANT_CMD_TOUCH_REQ,
                                app_sensor_hub_core_transmit_touch_instant_req_handler,
                                app_sensor_hub_core_touch_instant_req_handler);
#endif

/***************************************************************************
 * @brief capsensor_median_filtering
 *
 * @param ptr :capsensor_sample_data.
 ***************************************************************************/
void capsensor_median_filtering(struct capsensor_sample_data * data)
{
    static uint32_t sdm_array[CAP_CHNUM*3] = {0};

    if (sdm_array[0] == 0) {
        for (uint8_t i = 0; i < CAP_CHNUM; i++) {
            sdm_array[i] = data[i].sdm;
        }
    } else if (sdm_array[CAP_CHNUM] == 0) {
        for (uint8_t i = 0; i < CAP_CHNUM; i++) {
            sdm_array[i + CAP_CHNUM] = data[i].sdm;
        }
    } else {
        for (uint8_t i = 0; i < CAP_CHNUM; i++) {
            sdm_array[i + CAP_CHNUM*2] = data[i].sdm;
        }
        for (uint8_t i = 0; i < CAP_CHNUM; i++) {
            if ((sdm_array[i] > sdm_array[i + CAP_CHNUM]) && (sdm_array[i] > sdm_array[i + CAP_CHNUM*2])) {
                if (sdm_array[i + CAP_CHNUM] >= sdm_array[i + CAP_CHNUM*2]) {
                    data[i].sdm = sdm_array[i + CAP_CHNUM];
                } else {
                    data[i].sdm = sdm_array[i + CAP_CHNUM*2];
                }
            } else if ((sdm_array[i] < sdm_array[i + CAP_CHNUM]) && (sdm_array[i + CAP_CHNUM] > sdm_array[i + CAP_CHNUM*2])) {
                if (sdm_array[i] >= sdm_array[i + CAP_CHNUM*2]) {
                    data[i].sdm = sdm_array[i];
                } else {
                    data[i].sdm = sdm_array[i + CAP_CHNUM*2];
                }
            } else if((sdm_array[i] < sdm_array[i + CAP_CHNUM*2]) && (sdm_array[i + CAP_CHNUM] < sdm_array[i + CAP_CHNUM*2])) {
                if (sdm_array[i] >= sdm_array[i + CAP_CHNUM]) {
                    data[i].sdm = sdm_array[i];
                } else {
                    data[i].sdm = sdm_array[i + CAP_CHNUM];
                }
            }
        }
        for (uint8_t i = 0; i < CAP_CHNUM; i++) {
            sdm_array[i] = sdm_array[i + CAP_CHNUM];
            sdm_array[i + CAP_CHNUM] = sdm_array[i + CAP_CHNUM*2];
        }
    }
}

/***************************************************************************
 * @brief cap sensor interrupt handler
 *
 ***************************************************************************/
void cap_sensor_irq_handler(void)
{
#if (CHIP_CAPSENSOR_VER < 1)
    hal_capsensor_irq_disable();
#endif
    read_capsensor_fifo(capsense_sample_buff);

    osSignalSet(capsensor_thread_id, 0x0001);
}

/***************************************************************************
 * @brief cap sensor interrupt init function
 *
 ***************************************************************************/
void  cap_sensor_irqinit(void)
{
#if (CHIP_CAPSENSOR_VER < 1)
#ifdef CHIP_SUBSYS_SENS
    NVIC_SetVector(CAP_SENSOR_IRQn, (uint32_t)cap_sensor_irq_handler);
    NVIC_SetPriority(CAP_SENSOR_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(CAP_SENSOR_IRQn);
    NVIC_EnableIRQ(CAP_SENSOR_IRQn);
    hal_capsensor_irq_enable();
#else
    NVIC_SetVector(SENS_IRQn, (uint32_t)cap_sensor_irq_handler);
    NVIC_SetPriority(SENS_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(SENS_IRQn);
    NVIC_EnableIRQ(SENS_IRQn);
    hal_capsensor_irq_enable();
#endif
#endif
#if (CHIP_CAPSENSOR_VER > 1)
#ifdef CHIP_SUBSYS_SENS
    capsensor_timer_start();
#else
    int ret = -1;
    ret = pmu_capsensor_set_irq_handler(cap_sensor_irq_handler);
    if (ret) {
        TRACE(1, "pmu_capsensor_set_irq_handler failed:%d\n", ret);
    }
#endif
#endif
}

/***************************************************************************
 * @brief cap sensor process thread
 *
 * @param arg
 ***************************************************************************/
static void capsensor_thread(const void *arg)
{
#if defined(CAPSENSOR_TOUCH) || defined(CAPSENSOR_WEAR)
    uint32_t chan_data_sum[CAP_CHNUM] = {0};
#endif

#ifdef CAPSENSOR_SPP_SERVER
    uint8_t touch_event = 0;
#endif

#if defined(CAPSENSOR_TOUCH)
    uint32_t timer_start[CAP_USED_TOUCH_NUM] = {0};
    uint32_t timer_stop[CAP_USED_TOUCH_NUM]  = {0};
#endif

    capsensor_drv_start();       /* init cap sensor regesiter. */

#if (CHIP_CAPSENSOR_VER == 1)
    capsensor_timer_start();    /* polling cap sensor . */
#else
    cap_sensor_irqinit();       /* init cap sensor irq */
#endif

#if defined(CAPSENSOR_TOUCH)
    capsensor_touch_config_init();
#endif
#if defined(CAPSENSOR_WEAR)
    capsensor_wear_config_init();
#endif
    CAPSENSOR_TRACE(0, "sensor MCU freq=%d", hal_sys_timer_calc_cpu_freq(5,0));     /* trace sensor mcu freq */

    while(1) {
        osEvent evt;
#if defined(CAPSENSOR_TOUCH) || defined(CAPSENSOR_WEAR)
        CAP_KEY_STATUS status;
#endif
        evt = osSignalWait(0, osWaitForever);
        if (evt.status == osEventSignal) {
            if (evt.value.signals == 0x01) {
#ifdef CAPSENSOR_MEDIAN_FILTERING
                capsensor_median_filtering(capsense_sample_buff);
#endif
#if defined(CAPSENSOR_TOUCH)
                status = capsensor_touch_process(capsense_sample_buff, CAP_CHNUM, chan_data_sum, (CAP_CHNUM * CAP_REPNUM), timer_start, timer_stop);
#ifdef CAPSENSOR_SPP_SERVER
                touch_event = status.event;
#endif
                if (status.event != CAP_KEY_EVENT_NONE) {
#ifdef CHIP_SUBSYS_SENS
                    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_TOUCH_REQ_NO_RSP, (uint8_t *)&status, sizeof(CAP_KEY_STATUS));
#else
                    if (capsensor_click_event_cb != NULL) {
                        capsensor_click_event_cb(status.event);
                    }
#endif
                }
#endif
#if defined(CAPSENSOR_WEAR)
                static uint8_t wear_event = 0;
                status = capsensor_wear_process(capsense_sample_buff, CAP_CHNUM, chan_data_sum, (CAP_CHNUM * CAP_REPNUM));
                if (status.event != CAP_KEY_EVENT_NONE && wear_event != status.event) {
                    wear_event = status.event;
#ifdef CHIP_SUBSYS_SENS
                    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_TOUCH_REQ_NO_RSP, (uint8_t *)&status, sizeof(CAP_KEY_STATUS));
#else
                    if (capsensor_click_event_cb != NULL) {
                        capsensor_click_event_cb(status.event);
                    }
#endif
                }
#endif
#ifdef CAPSENSOR_SPP_SERVER
                capsensor_check_mem_data(capsense_sample_buff, (CAP_CHNUM * CAP_REPNUM), touch_event);
#endif
#ifdef CAPSENSOR_FAC_CALCULATE
                capsensor_factory_calculate(capsense_sample_buff, (CAP_CHNUM * CAP_REPNUM));
#endif
            }
        }
    }
}

/***************************************************************************
 * @brief init snesorhub touch key functions.
 *
 ***************************************************************************/
void cap_sensor_core_thread_init(void)
{
    if (capsensor_thread_id == NULL) {
        capsensor_thread_id = osThreadCreate(osThread(capsensor_thread), NULL);
        CAPSENSOR_TRACE(1, "%s capsensor_thread successed !!!", __func__);
    }
}

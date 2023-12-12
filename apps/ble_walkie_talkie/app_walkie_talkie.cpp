/**
 *
 * @copyright Copyright (c) 2015-2022 BES Technic.
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
 */

#include "cmsis_os.h"
#include "hal_trace.h"
#include "factory_section.h"

#include "app_walkie_talkie.h"
#include "app_walkie_talkie_full_duplex.h"
#include "walkie_talkie_dbg.h"
#include "walkie_talkie_ble_gap.h"
#include "walkie_talkie_network_addr.h"
#include "walkie_talkie_build_network.h"
#include "walkie_talkie_test.h"

#ifdef WT_GAP_UNIT_TEST
#include "app_walkie_talkie_test.h"
#include "walkie_talkie_unit_test.h"
#endif

/*
 *  MVP framework,App_walkie_talkie as the Presenter
 *                Watch UI as the View layer
 *                walkie talkie service as the Mode
*/
#define WT_APP_EVENT_MAX_COUNT          5
#if defined(CHIP_BEST1502X)
#define WT_APP_STACK_SIZE               1536
#else
#define WT_APP_STACK_SIZE               1536
#endif
static void wt_app_thread(const void* arg);
osThreadDef(wt_app_thread, osPriorityNormal,1,WT_APP_STACK_SIZE, "wt_app_thread");

osMailQDef(wt_app_event_mbox, WT_APP_EVENT_MAX_COUNT,wt_app_recv_event_t);

static osMailQId wt_app_event_mbox = NULL;
static uint8_t wt_app_evt_mailbox_cnt = 0;
static osThreadId wt_app_thread_id;

static int wt_app_mailbox_init(void)
{
    if (wt_app_event_mbox)
    {
        return 0;
    }
    wt_app_event_mbox = osMailCreate(osMailQ(wt_app_event_mbox), NULL);
    if (wt_app_event_mbox == NULL)
    {
        TRACE(0, "create wt decoder mailbox error");
        return -1;
    }
    wt_app_evt_mailbox_cnt = 0;

    return 0;
}

void wt_app_thread_init()
{
    wt_app_mailbox_init();

    wt_app_thread_id = osThreadCreate(osThread(wt_app_thread), NULL);
    ASSERT(wt_app_thread_id,"wt app thread create fail");
}

void wt_app_thread_deinit()
{
    if (wt_app_thread_id)
    {
        osThreadTerminate(wt_app_thread_id);
        wt_app_thread_id = NULL;
    }
}

static int wt_app_evt_mailbox_get(wt_app_recv_event_t** msg_p)
{
    osEvent evt;
    evt = osMailGet(wt_app_event_mbox, osWaitForever);
    if (evt.status == osEventMail)
    {
        *msg_p = (wt_app_recv_event_t *)evt.value.p;
        return 0;
    }
    return -1;
}

static int wt_app_evt_mailbox_free(wt_app_recv_event_t* msg_p)
{
    osStatus status;

    status = osMailFree(wt_app_event_mbox, msg_p);
    if (osOK == status)
    {
        wt_app_evt_mailbox_cnt--;
    }

    return (int)status;
}

static void wt_app_thread(const void* arg)
{
    while (true)
    {
        wt_app_recv_event_t *pkt = NULL;
        if (wt_app_evt_mailbox_get(&pkt) == -1)
        {
            continue;
        }
        app_walkie_talkie_full_dup_event_handler(pkt->event_id,pkt->device_id,pkt->addr);
        wt_app_evt_mailbox_free(pkt);
    }
}

int wt_app_notify_app_event(WALKIE_TAKIE_SRV_EVENT_T event,uint8_t device_id,uint8_t *addr)
{
    wt_app_recv_event_t* wt_app_event = NULL;
    osStatus status = (osStatus)osErrorValue;
    wt_app_event = (wt_app_recv_event_t*)osMailCAlloc(wt_app_event_mbox, 0);
    if (wt_app_event == NULL)
    {
        return (int)status;
    }

    wt_app_event->event_id  = event;
    wt_app_event->device_id = device_id;
    wt_app_event->addr      = addr;
    status = osMailPut(wt_app_event_mbox, wt_app_event);
    if (osOK == status)
    {
        wt_app_evt_mailbox_cnt++;
    }
    else
    {
        TRACE(1,"wt decoder osMailPut fail,status=%x",status);
    }
    return (int)status;
}

static void app_walkie_talkie_ready_to_send_data_cb()
{
    LOG_D("W-T-APP:ready_to_send_data_cb");
}

static void app_walkie_talkie_find_device_cb(uint8_t device_id, uint8_t* device_addrss)
{
    LOG_D("W-T-APP:find_device(%d),address:",device_id);
    if(device_addrss != NULL)
    {
        DUMP8("%02x ",device_addrss, 6);
        //media_PlayAudio(AUD_ID_BT_CONNECTED, 0);
    }
}

static void app_walkie_talkie_device_loss_cb(uint8_t device_id,uint8_t* device_addrss)
{
    LOG_D("W-T-APP:device(%d),address loss",device_id);
    if(device_addrss != NULL)
    {
        DUMP8("%02x ",device_addrss, 6);
        //media_PlayAudio(AUD_ID_BT_DIS_CONNECT, 0);
    }
}

static void app_walkie_talkie_device_is_talking(uint8_t device_id,uint8_t* device_addrss)
{
    LOG_D("W-T-APP:device(%d) is talking",device_id);
    if(device_addrss != NULL)
    {
        DUMP8("%02x ",device_addrss, 6);
    }
}

static void app_wakie_talkie_device_end_talking(uint8_t device_id,uint8_t* device_addrss)
{
    LOG_D("W-T-APP:device(%d) talking end",device_id);
    if(device_addrss != NULL)
    {
        DUMP8("%02x ",device_addrss, 6);
    }
}

static void app_wt_allow_speaking()
{
    LOG_D("W-T-APP:%s",__func__);
}

static void app_wt_not_allow_speaking()
{
    LOG_D("W-T-APP:%s",__func__);
}

static const walkie_full_dup_event_cb walkie_talkie_full_dup_event_cbs = {
    .wt_ready_to_send_data = app_walkie_talkie_ready_to_send_data_cb,
    .wt_find_device = app_walkie_talkie_find_device_cb,
    .wt_device_loss = app_walkie_talkie_device_loss_cb,
    .wt_device_is_stalking = app_walkie_talkie_device_is_talking,
    .wt_device_end_stalking = app_wakie_talkie_device_end_talking,
    .wt_allow_speaking      = app_wt_allow_speaking,
    .wt_not_allow_speaking  = app_wt_not_allow_speaking,
};

void app_walkie_talkie_init()
{
    uint8_t* local_ble_addr = factory_section_get_ble_address();
    LOG_D("W-T:Local Address:");
    ASSERT(local_ble_addr,"Local Address Err!");
    DUMP8("%02x ",local_ble_addr, 6);
    wt_app_thread_init();
    wt_test_uart_cmd_init();
    wakie_talke_register_event_call_back(wt_app_notify_app_event);
    app_walkie_talkie_full_dup_init(NULL,0);

    app_wt_full_dup_reg_state_changed_callback(&walkie_talkie_full_dup_event_cbs);

#ifdef WT_AUTO_BUILD_NET
#if (WT_GAP_UNIT_TEST == 0)
    wt_network_rebuild(WT_NW_TYPE_MULTIPLE, wt_auto_build_net_addr,
        sizeof(wt_auto_build_net_addr), wt_auto_build_net_addr);
#endif
#endif

#if WT_GAP_UNIT_TEST
    // app_wt_uart_cmd_register();
#endif
}

void app_walkie_talkie_exit()
{
    app_wt_full_dup_start_exit();
}



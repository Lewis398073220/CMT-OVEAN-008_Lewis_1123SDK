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
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "plat_types.h"
#include "usb_audio_app.h"
#include "ble_audio_dbg.h"
#include "bt_drv_interface.h"
#include "gaf_media_common.h"
#include "app_usb_hw_timer.h"
#include "hwtimer_list.h"

/************************private macro defination***************************/
#define HW_TIMER_STREAM_THREAD_STACK_SIZE       (1024*10)
#define HW_TIMER_PORCESS_PLAYBACK               (1 << 0)
#define HW_TIMER_PORCESS_CAPTURE                (1 << 1)

/************************private strcuture defination****************************/

/************************private variable defination************************/
static osThreadId hw_timer_stream_processor_thread_id = NULL;

static HWTIMER_ID gaf_usb_audio_playback_timer_id = NULL;
static HWTIMER_ID gaf_usb_audio_capture_timer_id = NULL;
static void *playbackStreamEnv = NULL;
static void *captureStreamEnv = NULL;

/**********************private function declaration*************************/

static void hw_timer_stream_processor_thread(const void *arg);

osThreadDef(hw_timer_stream_processor_thread, osPriorityHigh, 1,
            (HW_TIMER_STREAM_THREAD_STACK_SIZE), "hw_timer_stream_processor_thread");

/****************************function defination****************************/

static uint32_t gaf_usb_capture_get_trigger_offset(void *_pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)_pStreamEnv;

    uint32_t current_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);

    int32_t gapUs = (int32_t)(latest_iso_bt_time - current_bt_time);

    while (gapUs < 1000)
    {
        latest_iso_bt_time += pStreamEnv->stream_info.captureInfo.isoIntervalUs;
        gapUs = (int32_t)(latest_iso_bt_time - current_bt_time);
    }

    if (gapUs <= (int32_t)(pStreamEnv->stream_info.captureInfo.isoIntervalUs / 2) ||
        gapUs >= (int32_t)(pStreamEnv->stream_info.captureInfo.isoIntervalUs * 1.5))
    {
        return pStreamEnv->stream_info.captureInfo.isoIntervalUs;
    }
    return gapUs;
}

static uint32_t gaf_usb_playback_get_trigger_offset(void *_pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)_pStreamEnv;

    uint32_t current_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    uint32_t latest_iso_bt_time = gaf_media_common_get_latest_rx_iso_evt_timestamp(pStreamEnv);
    int32_t gapUs = (int32_t)(latest_iso_bt_time - current_bt_time);

    while (gapUs < 1000)
    {
        latest_iso_bt_time += pStreamEnv->stream_info.playbackInfo.isoIntervalUs;
        gapUs = (int32_t)(latest_iso_bt_time - current_bt_time);
    }

    if (gapUs <= (int32_t)(pStreamEnv->stream_info.playbackInfo.isoIntervalUs / 2) ||
        gapUs >= (int32_t)(pStreamEnv->stream_info.playbackInfo.isoIntervalUs * 1.5))
    {
        return pStreamEnv->stream_info.playbackInfo.isoIntervalUs;
    }
    return gapUs;
}

static void gaf_usb_playback_hw_timer_handler(void *_pStreamEnv)
{
    osSignalSet(hw_timer_stream_processor_thread_id, HW_TIMER_PORCESS_PLAYBACK);
}

static void gaf_usb_capture_hw_timer_handler(void *_pStreamEnv)
{
    osSignalSet(hw_timer_stream_processor_thread_id, HW_TIMER_PORCESS_CAPTURE);
}

void gaf_usb_playback_hw_timer_start(void *_pStreamEnv, uint32_t trig_tick)
{
    if (NULL == _pStreamEnv)
    {
         LOG_E("%s, pStreamEnv is NULL", __func__);
         return;
    }

    if (NULL == gaf_usb_audio_playback_timer_id)
    {
        gaf_usb_audio_playback_timer_id =
            hwtimer_alloc(gaf_usb_playback_hw_timer_handler, _pStreamEnv);
    }
    playbackStreamEnv = _pStreamEnv;

    uint32_t current_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    LOG_I("hwtimer playback trigger_tick:%u curr_time:%u", trig_tick, current_bt_time);
    hwtimer_update_then_start(gaf_usb_audio_playback_timer_id,
        gaf_usb_playback_hw_timer_handler, _pStreamEnv, US_TO_TICKS(trig_tick - current_bt_time));
}

void gaf_usb_capture_hw_timer_start(void *_pStreamEnv, uint32_t trig_tick)
{
    if (NULL == _pStreamEnv)
    {
         LOG_E("%s please check param", __func__);
         return;
    }

    if (NULL == gaf_usb_audio_capture_timer_id)
    {
        gaf_usb_audio_capture_timer_id =
            hwtimer_alloc(gaf_usb_capture_hw_timer_handler, _pStreamEnv);
    }
    captureStreamEnv = _pStreamEnv;

    uint32_t current_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    LOG_I("hwtimer capture trigger_tick:%u curr_time:%u", trig_tick, current_bt_time);
    hwtimer_update_then_start(gaf_usb_audio_capture_timer_id,
        gaf_usb_capture_hw_timer_handler, _pStreamEnv, US_TO_TICKS(trig_tick - current_bt_time));
}

void gaf_usb_hw_timer_stop(uint32_t stream)
{
    if (AUD_STREAM_PLAYBACK == stream)
    {
        if (gaf_usb_audio_capture_timer_id)
        {
            hwtimer_stop(gaf_usb_audio_capture_timer_id);
        }
    }
    else if (AUD_STREAM_CAPTURE == stream)
    {
        if (gaf_usb_audio_playback_timer_id)
        {
            hwtimer_stop(gaf_usb_audio_playback_timer_id);
        }
    }
}

static void hw_timer_stream_processor_thread(const void *arg)
{
    while(1)
    {
        osEvent evt;
        evt = osSignalWait(0x0, osWaitForever);

        if (osEventSignal == evt.status)
        {
            if (evt.value.signals & HW_TIMER_PORCESS_PLAYBACK)
            {
                if (gaf_stream_is_playback_stream_iso_created(playbackStreamEnv)) {
                    uint32_t trigger_offset = gaf_usb_playback_get_trigger_offset(playbackStreamEnv);
                    hwtimer_start(gaf_usb_audio_playback_timer_id, US_TO_TICKS(trigger_offset));
                    send_usb_audio_buffer(AUD_STREAM_CAPTURE);
                }
                else {
                    TRACE(0, "playback iso stream stopped");
                }
            }
            if (evt.value.signals & HW_TIMER_PORCESS_CAPTURE)
            {
                if (gaf_stream_is_capture_stream_iso_created(captureStreamEnv)) {
                    uint32_t trigger_offset = gaf_usb_capture_get_trigger_offset(captureStreamEnv);
                    hwtimer_start(gaf_usb_audio_capture_timer_id, US_TO_TICKS(trigger_offset));
                    send_usb_audio_buffer(AUD_STREAM_PLAYBACK);
                }
                else {
                    TRACE(0, "capture iso stream stopped");
                }
            }
        }
    }
}

void gaf_usb_hw_timer_init(void)
{
    if (NULL == hw_timer_stream_processor_thread_id)
    {
        hw_timer_stream_processor_thread_id =
            osThreadCreate(osThread(hw_timer_stream_processor_thread), NULL);
    }
}
#endif

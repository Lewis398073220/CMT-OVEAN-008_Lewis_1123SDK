/**
 * @file aob_ux_stm.cpp
 * @author BES AI team
 * @version 0.1
 * @date 2020-08-31
 *
 * @copyright Copyright (c) 2015-2021 BES Technic.
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

/*****************************header include********************************/
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_aud.h"
#include "bt_drv_reg_op.h"
#include "audioflinger.h"
#include "bt_drv_interface.h"
#include "app_trace_rx.h"
#include "bluetooth_bt_api.h"
#include "besbt_string.h"
#include "app_bt_func.h"
#include "plat_types.h"
#include "heap_api.h"
#include "app_audio.h"
#include "app_utils.h"

#include "gaf_media_common.h"
#include "gaf_media_sync.h"
#include "gaf_stream_dbg.h"
#include "gaf_codec_lc3.h"
#include "rwble_config.h"
#ifndef BLE_STACK_NEW_DESIGN
#include "app_ble_tws_sync.h"
#else
#include "ble_tws.h"
#endif
#include "gaf_cc_api.h"

#include "bes_aob_api.h"

/*********************external function declaration*************************/
extern "C" uint32_t btdrv_reg_op_cig_anchor_timestamp(uint8_t link_id);

/************************private macro defination***************************/
#define GAF_STREAM_HEAP_DEBUG 0
#define GAF_STREAM_TRIGGER_TIMEROUT 3000
#define GAF_STREAM_MAX_RETRIGGER_TIMES 2
#define GAF_STREAM_INVALID_CLOCK_CNT 0xFFFFFFFF
#define GAF_STREAM_INVALID_US_SINCE_LAST_ANCHOR_POINT 0xFFFF

#define GAF_AUDIO_USE_ONE_STREAM_ONLY_ENABLED     (true)

#if defined(AOB_LOW_LATENCY_MODE) || (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
#define GAF_MAX_WAIT_FRAME_CNT      (1)
#define GAF_MAX_WAIT_PCM_CNT        (1)
#else
#define GAF_MAX_WAIT_FRAME_CNT      (3)
#define GAF_MAX_WAIT_PCM_CNT        (3)
#endif
#define GAF_WAIT_FRAME_MS           (2)
#define GAF_WAIT_PCM_MS             (2)
#define TIME_CALCULATE_REVERSAL_THRESHOLD   0x7F000000

/************************private type defination****************************/

/**********************private function declaration*************************/

/************************private variable defination************************/
static GAF_AUDIO_STREAM_RETRIGGER gaf_audio_stream_retrigger_cb = NULL;
static GAF_AUDIO_STREAM_RETRIGGER gaf_mobile_audio_stream_retrigger_cb = NULL;
static GAF_AUDIO_STREAM_ENV_T *gaf_audio_running_stream_ref[GAF_MAXIMUM_CONNECTION_COUNT] = {NULL};
static uint8_t max_playback_retrigger_times = 0;
static uint8_t max_capture_retrigger_times = 0;
static bool gaf_is_doing_prefill = false;

/// PresDelay in us set by custom
static uint32_t gaf_stream_custom_presdelay_us = 0;

static void playback_trigger_supervisor_timer_cb(void const *param);
osTimerDef(GAF_STREAM_PLAYBACK_TRIGGER_TIMEOUT, playback_trigger_supervisor_timer_cb);

static void capture_trigger_supervisor_timer_cb(void const *param);
osTimerDef(GAF_STREAM_CAPTURE_TRIGGER_TIMEOUT, capture_trigger_supervisor_timer_cb);

static heap_handle_t codec_data_heap;

/**********************************GAF CUSTOM******************************/
static GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T gaf_stream_common_custom_data_func_table[GAF_STREAM_USER_CASE_MAX] = {{NULL}};

/****************************function defination****************************/
void gaf_stream_register_retrigger_callback(GAF_AUDIO_STREAM_RETRIGGER retrigger_cb)
{
    gaf_audio_stream_retrigger_cb = retrigger_cb;
}

void gaf_mobile_stream_register_retrigger_callback(GAF_AUDIO_STREAM_RETRIGGER retrigger_cb)
{
    gaf_mobile_audio_stream_retrigger_cb = retrigger_cb;
}

static void gaf_stream_retrigger_handler(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t streamType)
{
    if (pStreamEnv->stream_info.is_mobile && gaf_mobile_audio_stream_retrigger_cb)
    {
        gaf_mobile_audio_stream_retrigger_cb(pStreamEnv, streamType);
    }
    else if (gaf_audio_stream_retrigger_cb)
    {
        gaf_audio_stream_retrigger_cb(pStreamEnv, streamType);
    }
}

static void playback_trigger_supervisor_timer_cb(void const *param)
{
    LOG_I("%s, gaf stream trigger timeout!", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)param;

    if (pStreamEnv)
    {
        pStreamEnv->stream_context.playback_retrigger_onprocess = true;
        app_bt_call_func_in_bt_thread((uint32_t)pStreamEnv, GAF_AUDIO_STREAM_TYPE_PLAYBACK, 0, 0,
            (uint32_t)gaf_stream_retrigger_handler);
    }
}

static void capture_trigger_supervisor_timer_cb(void const *param)
{
    LOG_I("%s, gaf stream trigger timeout!", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)param;

    if (pStreamEnv)
    {
        pStreamEnv->stream_context.capture_retrigger_onprocess = true;
        app_bt_call_func_in_bt_thread((uint32_t)pStreamEnv, GAF_AUDIO_STREAM_TYPE_CAPTURE, 0, 0,
            (uint32_t)gaf_stream_retrigger_handler);
    }
}

uint8_t gaf_media_common_get_ase_chan_lid_from_iso_channel(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                                           uint8_t direction, uint8_t iso_channel)
{
    GAF_AUDIO_STREAM_COMMON_INFO_T *pCommonInfo = NULL;
    if (direction == BES_BLE_GAF_DIRECTION_SINK)
    {
        pCommonInfo = &pStreamEnv->stream_info.playbackInfo;
    }
    else
    {
        pCommonInfo = &pStreamEnv->stream_info.captureInfo;
    }

    for (uint8_t chn_lid = 0; chn_lid < GAF_AUDIO_ASE_TOTAL_COUNT; chn_lid++)
    {
        if (pCommonInfo->aseChInfo[chn_lid].iso_channel_hdl == iso_channel)
        {
            return chn_lid;
        }
    }

    return GAF_AUDIO_ASE_TOTAL_COUNT;
}

uint32_t gaf_media_common_get_latest_tx_iso_evt_timestamp(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    if (pStreamEnv->stream_info.is_bis)
    {
        for (uint8_t i = 0; i < 2; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.captureInfo.bisChInfo[i].iso_channel_hdl)
            {
                return btdrv_reg_op_big_anchor_timestamp(
                                    pStreamEnv->stream_info.captureInfo.bisChInfo[i].iso_channel_hdl);
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl)
            {
                return btdrv_reg_op_cig_anchor_timestamp(
                                    pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl);
            }
        }
    }
    LOG_W("%s err!!!", __func__);
    return 0;
}

uint32_t gaf_media_common_get_latest_rx_iso_evt_timestamp(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    if (pStreamEnv->stream_info.is_bis)
    {
        for (uint8_t i = 0; i < 2; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.bisChInfo[i].iso_channel_hdl)
            {
                return btdrv_reg_op_big_anchor_timestamp(
                                    pStreamEnv->stream_info.playbackInfo.bisChInfo[i].iso_channel_hdl);
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl)
            {
                return btdrv_reg_op_cig_anchor_timestamp(
                                    pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl);
            }
        }
    }
    LOG_W("%s err!!!", __func__);
    return 0;
}

int gaf_stream_playback_trigger_checker_start(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    if (max_playback_retrigger_times > GAF_STREAM_MAX_RETRIGGER_TIMES) {
        max_playback_retrigger_times = 0;
        return -1;
    }

    if (pStreamEnv->stream_context.playback_trigger_supervisor_timer_id)
    {
        LOG_I("%s", __func__);
        pStreamEnv->stream_context.playback_retrigger_onprocess = false;
        osTimerStart(pStreamEnv->stream_context.playback_trigger_supervisor_timer_id,
            GAF_STREAM_TRIGGER_TIMEROUT);
        max_playback_retrigger_times++;
    }
    return 0;
}

int gaf_stream_playback_trigger_checker_stop(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    if (pStreamEnv->stream_context.playback_trigger_supervisor_timer_id != NULL)
    {
        pStreamEnv->stream_context.playback_retrigger_onprocess = false;
        osTimerStop(pStreamEnv->stream_context.playback_trigger_supervisor_timer_id);
    }
    return 0;
}

void gaf_stream_reset_retrigger_counter(void)
{
    max_capture_retrigger_times = 0;
    max_playback_retrigger_times = 0;
}

int gaf_stream_capture_trigger_checker_start(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    if (max_capture_retrigger_times > GAF_STREAM_MAX_RETRIGGER_TIMES) {
        max_capture_retrigger_times = 0;
        return -1;
    }

    if (pStreamEnv->stream_context.capture_trigger_supervisor_timer_id)
    {
        LOG_I("%s", __func__);
        pStreamEnv->stream_context.capture_retrigger_onprocess = false;
        osTimerStart(pStreamEnv->stream_context.capture_trigger_supervisor_timer_id, GAF_STREAM_TRIGGER_TIMEROUT);
        max_capture_retrigger_times++;
    }
    return 0;
}

int gaf_stream_capture_trigger_checker_stop(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    if (pStreamEnv->stream_context.capture_trigger_supervisor_timer_id != NULL)
    {
        pStreamEnv->stream_context.capture_retrigger_onprocess = false;
        osTimerStop(pStreamEnv->stream_context.capture_trigger_supervisor_timer_id);
    }
    return 0;
}

void gaf_stream_heap_init(void *begin_addr, uint32_t size)
{
    codec_data_heap = heap_register(begin_addr,size);
}

void *gaf_stream_heap_malloc(uint32_t size)
{
    uint32_t lock = int_lock();

    size = ((size >> 2) + 1) << 2;
    void *ptr = heap_malloc(codec_data_heap, size);
    ASSERT(ptr, "%s size:%d", __func__, size);
    memset(ptr, 0, size);
#if GAF_STREAM_HEAP_DEBUG
    LOG_I("[%s] ptr=%p size=%u user=%p left/mini left=%u/%u",
        __func__, ptr, size, __builtin_return_address(0),
        heap_free_size(codec_data_heap),
        heap_minimum_free_size(codec_data_heap));
#endif

    int_unlock(lock);
    return ptr;
}

void *gaf_stream_heap_cmalloc(uint32_t size)
{
    uint32_t lock = int_lock();
    void *ptr = heap_malloc(codec_data_heap, size);
    ASSERT(ptr, "%s size:%d", __func__, size);
    memset(ptr, 0, size);
#if GAF_STREAM_HEAP_DEBUG
    LOG_I("[%s] ptr=%p size=%u user=%p left/mini left=%u/%u",
        __func__, ptr, size, __builtin_return_address(0),
        heap_free_size(codec_data_heap),
        heap_minimum_free_size(codec_data_heap));
#endif
    int_unlock(lock);
    return ptr;
}

void *gaf_stream_heap_realloc(void *rmem, uint32_t newsize)
{
    uint32_t lock = int_lock();

    void *ptr = heap_realloc(codec_data_heap, rmem, newsize);
    ASSERT(ptr, "%s rmem:%p size:%d", __func__, rmem,newsize);
#if GAF_STREAM_HEAP_DEBUG
    LOG_I("[%s] ptr=%p/%p size=%u user=%p left/mini left=%u/%u",
        __func__, rmem, ptr, newsize, __builtin_return_address(0),
        heap_free_size(codec_data_heap),
        heap_minimum_free_size(codec_data_heap));
#endif
    int_unlock(lock);
    return ptr;
}

void gaf_stream_heap_free(void *rmem)
{
#if GAF_STREAM_HEAP_DEBUG
    LOG_I("[%s] ptr=%p user=%p", __func__, rmem, __builtin_return_address(0));
#endif
    ASSERT(rmem, "%s rmem:%p", __func__, rmem);

    uint32_t lock = int_lock();
    heap_free(codec_data_heap,rmem);
    int_unlock(lock);
}

void gaf_stream_data_free(void *packet)
{
    ASSERT(packet, "%s packet = %p", __func__, packet);

    gaf_media_data_t *decoder_frame_p = (gaf_media_data_t *)packet;
    if (decoder_frame_p->origin_buffer)
    {
        gaf_stream_heap_free(decoder_frame_p->origin_buffer);
        decoder_frame_p->origin_buffer = NULL;
    }
    gaf_stream_heap_free(decoder_frame_p);
}

void *gaf_stream_data_frame_malloc(uint32_t packet_len)

{
    gaf_media_data_t *decoder_frame_p = NULL;
    uint8_t *buffer = NULL;

    if (packet_len)
    {
        buffer = (uint8_t *)gaf_stream_heap_malloc(packet_len);
    }

    decoder_frame_p = (gaf_media_data_t *)gaf_stream_heap_malloc(sizeof(gaf_media_data_t));
    decoder_frame_p->origin_buffer = decoder_frame_p->sdu_data = buffer;
    decoder_frame_p->data_len = packet_len;
    return (void *)decoder_frame_p;
}

int gaf_playback_status_mutex_lock(void *mutex)
{
    osMutexWait((osMutexId)mutex, osWaitForever);
    return 0;
}

int gaf_playback_status_mutex_unlock(void *mutex)
{
    osMutexRelease((osMutexId)mutex);
    return 0;
}

uint32_t gaf_stream_common_sample_freq_parse(uint8_t sample_freq)
{
    switch (sample_freq)
    {
        case BES_BLE_GAF_SAMPLE_FREQ_8000:
            return AUD_SAMPRATE_8000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_16000:
            return AUD_SAMPRATE_16000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_22050:
            return AUD_SAMPRATE_22050;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_24000:
            return AUD_SAMPRATE_24000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_32000:
            return AUD_SAMPRATE_32000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_44100:
            return AUD_SAMPRATE_44100;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_48000:
            return AUD_SAMPRATE_48000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_88200:
            return AUD_SAMPRATE_88200;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_96000:
            return AUD_SAMPRATE_96000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_176400:
            return AUD_SAMPRATE_176400;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_192000:
            return AUD_SAMPRATE_192000;
        break;
        case BES_BLE_GAF_SAMPLE_FREQ_384000:
            return AUD_SAMPRATE_384000;
        break;
        default:
            LOG_I("%s unsupported freq %d", __func__, sample_freq);
        break;
    }

    return 0;
}

float gaf_stream_common_frame_duration_parse(uint8_t frame_duration)
{
    LOG_I("%s  duration %d", __func__, frame_duration);

#if defined (AOB_LOW_LATENCY_MODE) &&!defined (HID_ULL_ENABLE)
    return 5;
#endif
    switch (frame_duration)
    {
#if defined(LC3PLUS_SUPPORT) || defined(HID_ULL_ENABLE)
        case BES_BLE_GAF_BAP_FRAME_DURATION_2_5MS:
            return 2.5;
        break;
        case BES_BLE_GAF_BAP_FRAME_DURATION_5MS:
            return 5;
        break;
#endif
        case BES_BLE_GAF_BAP_FRAME_DURATION_7_5MS:
            return 7.5;
        break;
        case BES_BLE_GAF_BAP_FRAME_DURATION_10MS:
            return 10;
        break;
        default:
            LOG_I("%s unsupported duration %d", __func__, frame_duration);
            ASSERT(0, "%s, wrong Divided ", __func__);
        break;
    }

    return 0;
}

static const char * const gaf_playback_stream_str[] =
{
    "IDLE",
    "INITIALING",
    "BUF_INITIALIZED",
    "INITIALIZED",
    "START_TRIGGERING",
    "TRIGGERED",
};

void gaf_stream_common_update_playback_stream_state(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
    GAF_PLAYBACK_STREAM_STATE_E newState)
{
    if (newState != pStreamEnv->stream_context.playback_stream_state)
    {
        LOG_I("gaf playback stream state changes from %s to %s",
            gaf_playback_stream_str[pStreamEnv->stream_context.playback_stream_state],
            gaf_playback_stream_str[newState]);
        pStreamEnv->stream_context.playback_stream_state = newState;
    }

#ifndef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    if (GAF_PLAYBACK_STREAM_START_TRIGGERING == pStreamEnv->stream_context.playback_stream_state)
    {
        if (pStreamEnv->stream_context.playback_trigger_supervisor_timer_id == NULL)
        {
            pStreamEnv->stream_context.playback_trigger_supervisor_timer_id =
                osTimerCreate(osTimer(GAF_STREAM_PLAYBACK_TRIGGER_TIMEOUT), osTimerOnce, pStreamEnv);
        }
        gaf_stream_playback_trigger_checker_start(pStreamEnv);
    }
    else if (GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED == pStreamEnv->stream_context.playback_stream_state
        || GAF_PLAYBACK_STREAM_IDLE == pStreamEnv->stream_context.playback_stream_state)
    {
        gaf_stream_playback_trigger_checker_stop(pStreamEnv);
    }
#endif
}

static const char * const gaf_capture_stream_str[] =
{
    "IDLE",
    "INITIALING",
    "BUF_INITIALIZED",
    "INITIALIZED",
    "START_TRIGGERING",
    "TRIGGERED",
};

void gaf_stream_common_update_capture_stream_state(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
    GAF_CAPTURE_STREAM_STATE_E newState)
{
    if (newState != pStreamEnv->stream_context.capture_stream_state)
    {
        LOG_I("gaf capture stream state changes from %s to %s",
            gaf_capture_stream_str[pStreamEnv->stream_context.capture_stream_state],
            gaf_capture_stream_str[newState]);
        pStreamEnv->stream_context.capture_stream_state = newState;
    }

#ifndef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    if (GAF_CAPTURE_STREAM_INITIALIZED == pStreamEnv->stream_context.capture_stream_state)
    {
        if (pStreamEnv->stream_context.capture_trigger_supervisor_timer_id == NULL)
        {
            pStreamEnv->stream_context.capture_trigger_supervisor_timer_id =
                osTimerCreate(osTimer(GAF_STREAM_CAPTURE_TRIGGER_TIMEOUT), osTimerOnce, pStreamEnv);
        }
        gaf_stream_capture_trigger_checker_start(pStreamEnv);
    }
    else if (GAF_CAPTURE_STREAM_STREAMING_TRIGGERED == pStreamEnv->stream_context.capture_stream_state
        || GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state)
    {
        gaf_stream_capture_trigger_checker_stop(pStreamEnv);
    }
#endif
}

const char* gaf_stream_common_get_capture_stream_state(GAF_CAPTURE_STREAM_STATE_E capture_stream_state)
{
    return gaf_capture_stream_str[capture_stream_state];
}

void gaf_stream_common_set_playback_trigger_time_generic(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
    uint8_t dstStreamType, uint32_t tg_tick)
{
    pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs = tg_tick;
    pStreamEnv->stream_context.playbackTriggerStartTicks = tg_tick;

    // TODO: Already configured
    af_codec_sync_device_config(AUD_STREAM_USE_INT_CODEC, (enum AUD_STREAM_T)dstStreamType,
        AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_device_config(AUD_STREAM_USE_INT_CODEC, (enum AUD_STREAM_T)dstStreamType,
        AF_CODEC_SYNC_TYPE_BT, true);

    btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);
    bt_syn_ble_set_tg_ticks(tg_tick, pStreamEnv->stream_context.playbackTriggerChannel);

    LOG_I("[AOB TRIG] set playback trigger tg_tick:%u", tg_tick);

    gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_START_TRIGGERING);
}

void gaf_stream_common_set_playback_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t tg_tick)
{
    gaf_stream_common_set_playback_trigger_time_generic(pStreamEnv, AUD_STREAM_PLAYBACK, tg_tick);
}

void gaf_stream_common_set_capture_trigger_info(
        GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t tg_tick)
{
    pStreamEnv->stream_context.captureTriggerStartTicks = tg_tick;
    pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs = tg_tick;
    pStreamEnv->stream_context.lastCaptureDmaIrqTimeUsInTriggerPoint = tg_tick;

    pStreamEnv->stream_context.captureAverageDmaChunkIntervalUs =
        pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs;
    pStreamEnv->stream_context.latestCaptureSeqNum = 0;

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    pStreamEnv->stream_context.isUpStreamingStarted = false;
    if (pStreamEnv->stream_info.captureInfo.bnS2M >= 3)
    {
        pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming =
            pStreamEnv->stream_info.captureInfo.bnS2M;
    }
    else if (pStreamEnv->stream_info.captureInfo.bnS2M >= 1)
    {
        pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming =
            pStreamEnv->stream_info.captureInfo.bnS2M + 2;
    }
    else
    {
        pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming = 4;
    }
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming = 2;
#endif
    pStreamEnv->stream_context.usStorePcmToFetchFrame =
        (pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming - 1) * \
            pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs;
#else
    pStreamEnv->stream_context.isUpStreamingStarted = true;
    pStreamEnv->stream_context.usStorePcmToFetchFrame = 0;
    pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming = 0;
#endif
}

void gaf_stream_common_set_capture_trigger_time_generic(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
    uint8_t srcStreamType, uint32_t tg_tick)
{
    if ((GAF_CAPTURE_STREAM_START_TRIGGERING ==
        pStreamEnv->stream_context.capture_stream_state) ||
        (GAF_CAPTURE_STREAM_INITIALIZED ==
        pStreamEnv->stream_context.capture_stream_state))
    {
        gaf_stream_common_set_capture_trigger_info(pStreamEnv, tg_tick);

        // TODO: Already configured
        af_codec_sync_device_config(AUD_STREAM_USE_INT_CODEC, (enum AUD_STREAM_T)srcStreamType,
            AF_CODEC_SYNC_TYPE_BT, false);
        af_codec_sync_device_config(AUD_STREAM_USE_INT_CODEC, (enum AUD_STREAM_T)srcStreamType,
            AF_CODEC_SYNC_TYPE_BT, true);

        btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

        bt_syn_ble_set_tg_ticks(tg_tick, pStreamEnv->stream_context.captureTriggerChannel);

        LOG_I("[AOB TRIG] set capture trigger tg_tick:%u", tg_tick);

        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_START_TRIGGERING);
    }
    else
    {
        LOG_I("[AOB TRIG] cannot set trigger time when capture state is %s",
            gaf_capture_stream_str[pStreamEnv->stream_context.capture_stream_state]);
    }
}

void gaf_stream_common_set_capture_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t tg_tick)
{
    gaf_stream_common_set_capture_trigger_time_generic(pStreamEnv, AUD_STREAM_CAPTURE, tg_tick);
}

static void gaf_stream_common_updated_expeceted_playback_seq_num(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                                    uint8_t list_idx, uint32_t dmaIrqHappeningTimeUs)
{
    LOG_D("%s start", __func__);
    uint32_t expectedDmaIrqHappeningTimeUs =
        pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs +
        pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;

    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(expectedDmaIrqHappeningTimeUs, dmaIrqHappeningTimeUs);
    int32_t gapUs_abs = GAF_AUDIO_ABS(gapUs);
    LOG_D("playback, irqTime:%u-%u, gapUs:%d, Abs:%u",
        expectedDmaIrqHappeningTimeUs, dmaIrqHappeningTimeUs,
        gapUs, gapUs_abs);

    pStreamEnv->stream_context.lastestPlaybackSeqNum[list_idx]++;

    if (gapUs_abs > (int32_t)pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs/2)
    {
        do
        {
            pStreamEnv->stream_context.lastestPlaybackSeqNum[list_idx]++;
            gapUs_abs = GAF_AUDIO_CLK_32_BIT_DIFF(gapUs_abs,
                (int32_t)pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs);
            gapUs_abs = GAF_AUDIO_ABS(gapUs_abs);
            LOG_I("[%d] updated gapus %d, irqTime:%u-%u", list_idx, gapUs_abs,
                expectedDmaIrqHappeningTimeUs, dmaIrqHappeningTimeUs);
        } while (gapUs_abs >= (int32_t)pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs/2);
    } else if (gapUs_abs > (int32_t)pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs) {
        LOG_W("playback gapUs exceed the dmaChunkIntervalUs %d, %u->%u", gapUs_abs,
            dmaIrqHappeningTimeUs, expectedDmaIrqHappeningTimeUs);
    }

    LOG_D(" %s Playback %u %u 0x%x", __func__,dmaIrqHappeningTimeUs, expectedDmaIrqHappeningTimeUs,
        pStreamEnv->stream_context.lastestPlaybackSeqNum[list_idx]);

    LOG_D("%s end", __func__);
}

void gaf_stream_common_updated_expeceted_playback_seq_and_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                                    uint8_t list_idx, uint32_t dmaIrqHappeningTimeUs)
{
    gaf_stream_common_updated_expeceted_playback_seq_num(pStreamEnv, list_idx, dmaIrqHappeningTimeUs);
    /// Update expected timestamp for next time dma irq
    pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs = dmaIrqHappeningTimeUs;
}

void gaf_bis_stream_common_update_multi_channel_expect_seq_and_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs)
{
    uint8_t bis_idx = 0;
    for (bis_idx = 0; bis_idx < 2; bis_idx++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.bisChInfo[bis_idx].iso_channel_hdl)
        {
            gaf_stream_common_updated_expeceted_playback_seq_num(pStreamEnv, bis_idx, dmaIrqHappeningTimeUs);
        }
    }
    /// Update expected timestamp for next time dma irq
    pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs = dmaIrqHappeningTimeUs;
}

#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
void gaf_stream_common_update_multi_channel_expect_seq_and_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs)
{
    uint8_t ase_idx = 0;
    for (ase_idx = 0; ase_idx < (GAF_AUDIO_ASE_TOTAL_COUNT - 1); ase_idx++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.aseChInfo[ase_idx].iso_channel_hdl)
        {
            gaf_stream_common_updated_expeceted_playback_seq_num(pStreamEnv, ase_idx, dmaIrqHappeningTimeUs);
        }
    }
    /// Update expected timestamp for next time dma irq
    pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs = dmaIrqHappeningTimeUs;
}
#endif

static int32_t gaf_stream_common_update_average_dma_chunk_interval(int32_t y, int32_t x)
{
    if (y){
        y = ((GAF_AUDIO_ALPHA_PRAMS_1*y)+x)/GAF_AUDIO_ALPHA_PRAMS_2;
    }else{
        y = x;
    }
    return y;
}

void gaf_stream_common_capture_timestamp_checker_generic(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs, uint8_t StreamType)
{
    uint32_t latest_iso_bt_time = 0;
    int32_t usSinceLatestAnchorPoint = 0;

    latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
    usSinceLatestAnchorPoint = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, latest_iso_bt_time);

    uint32_t expectedDmaIrqHappeningTimeUs =
        pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs +
        (uint32_t)pStreamEnv->stream_context.captureAverageDmaChunkIntervalUs;

    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, expectedDmaIrqHappeningTimeUs);

    LOG_D("timestamp_checker,dma irq ts: %d - anchor point: %d", dmaIrqHappeningTimeUs, latest_iso_bt_time);
    int32_t gapUs_abs = GAF_AUDIO_ABS(gapUs);

    pStreamEnv->stream_context.latestCaptureSeqNum++;

    if (gapUs_abs < (int32_t)pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2)
    {
        int32_t dmaChunkInterval = GAF_AUDIO_CLK_32_BIT_DIFF(
            dmaIrqHappeningTimeUs,
            pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs);
        dmaChunkInterval = GAF_AUDIO_ABS(dmaChunkInterval);

        pStreamEnv->stream_context.captureAverageDmaChunkIntervalUs =
            gaf_stream_common_update_average_dma_chunk_interval(
            pStreamEnv->stream_context.captureAverageDmaChunkIntervalUs, dmaChunkInterval);

        int32_t intervalGap;

        intervalGap = pStreamEnv->stream_context.usSinceLatestAnchorPoint - usSinceLatestAnchorPoint;

        if (GAF_MEDIA_PID_ABS(intervalGap) < pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2)
        {
            pStreamEnv->stream_context.lastCaptureDmaIrqTimeUsInTriggerPoint = dmaIrqHappeningTimeUs;
        }

        LOG_D("capture pid %d - %d us diff %d us", usSinceLatestAnchorPoint,
            pStreamEnv->stream_context.usSinceLatestAnchorPoint,
            intervalGap);

        LOG_D("capture anch %d dma irq %d gap %d - %d", latest_iso_bt_time,
            dmaIrqHappeningTimeUs, pStreamEnv->stream_context.usSinceLatestAnchorPoint,
            intervalGap);
        if (AUD_STREAM_CAPTURE == StreamType) {
            // tune the capture codec clock
            gaf_media_pid_adjust(AUD_STREAM_CAPTURE, &(pStreamEnv->stream_context.capture_pid_env),
                intervalGap);
        }
        else if (AUD_STREAM_PLAYBACK == StreamType) {
            gaf_media_pid_adjust(AUD_STREAM_PLAYBACK, &(pStreamEnv->stream_context.playback_pid_env),
                intervalGap);
        }
    }
    else
    {
        LOG_I("DMA happening interval is longer than chunk interval!");
        LOG_I("last dma irq %u us, this dma irq %u us, diff %d us",
            pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs,
            dmaIrqHappeningTimeUs,
            gapUs_abs);
        do
        {
            pStreamEnv->stream_context.latestCaptureSeqNum++;
            gapUs_abs = GAF_AUDIO_CLK_32_BIT_DIFF(gapUs_abs,
                (int32_t)pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs);
            gapUs_abs = GAF_AUDIO_ABS(gapUs_abs);
        } while (gapUs_abs >= (int32_t)pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2);
    }

    if ((!pStreamEnv->stream_context.isUpStreamingStarted) &&
        (pStreamEnv->stream_context.latestCaptureSeqNum >=
        pStreamEnv->stream_context.capturedSeqNumToStartUpStreaming))
    {
        pStreamEnv->stream_context.isUpStreamingStarted = true;
        LOG_I("Cached seq num %d start up streaming", pStreamEnv->stream_context.latestCaptureSeqNum-1);
    }
    LOG_D("Capture dmaIrqTimeUs %u expectedDmaIrqTimeUs %u expected seq 0x%x",dmaIrqHappeningTimeUs,
        expectedDmaIrqHappeningTimeUs, pStreamEnv->stream_context.latestCaptureSeqNum);

    pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs = dmaIrqHappeningTimeUs;
}

void gaf_stream_common_capture_timestamp_checker(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs)
{
    gaf_stream_common_capture_timestamp_checker_generic(pStreamEnv, dmaIrqHappeningTimeUs,
        AUD_STREAM_CAPTURE);
}

void gaf_stream_common_playback_timestamp_checker(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs)
{
    gaf_stream_common_capture_timestamp_checker_generic(pStreamEnv, dmaIrqHappeningTimeUs,AUD_STREAM_PLAYBACK);
}

static uint32_t gaf_stream_common_get_packet_status(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t _dmaIrqHappeningTimeUs,
    CC_PLAYBACK_DATA_T* pReadData, uint32_t sink_play_delay, bool pidAdjust)
{
    uint32_t isPacketValid = 0;
    int32_t bt_time_diff = 0;
    uint32_t time_stamp = pReadData->time_stamp;

    bt_time_diff = (int32_t)(_dmaIrqHappeningTimeUs - pReadData->time_stamp - \
            pStreamEnv->stream_info.playbackInfo.presDelayUs);

    LOG_D("bt_diff_time:%d time_stamp:%u dmairqtime:%u preDelayus:%u", bt_time_diff,
            pReadData->time_stamp, _dmaIrqHappeningTimeUs, pStreamEnv->stream_info.playbackInfo.presDelayUs);

    if (ABS(bt_time_diff) < pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs/2)
    {
        isPacketValid = GAF_TIME_STAMP_CHECK_MATCH_EXPECED;
        LOG_D("received packet's playback time is match with dmairqtime.");
    }
    else
    {
        if (bt_time_diff < 0)
        {
            isPacketValid = GAF_TIME_STAMP_CHECK_LATER_THAN_EXPECED;
            LOG_D("received packet's playtime hasn't arrived.");
        }
        else
        {
            isPacketValid = GAF_TIME_STAMP_CHECK_EARLIER_THAN_EXPECED;
            LOG_D("received packet's playtime has passed.");
        }
    }

    if (pStreamEnv->stream_info.is_bis &&
        ABS(bt_time_diff) > TIME_CALCULATE_REVERSAL_THRESHOLD)
    {
        uint64_t revlersal_expected_time = 1;
        uint64_t revlersal_current_time = 1;
        time_stamp += sink_play_delay;
        _dmaIrqHappeningTimeUs += pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;
        if (_dmaIrqHappeningTimeUs > time_stamp)
        {
            revlersal_expected_time <<= 32;
            revlersal_expected_time += time_stamp;
            revlersal_current_time = _dmaIrqHappeningTimeUs;
            bt_time_diff = revlersal_current_time - revlersal_expected_time;
        }
        else
        {
            revlersal_current_time <<= 32;
            revlersal_expected_time = time_stamp;
            revlersal_current_time += _dmaIrqHappeningTimeUs;
            bt_time_diff = revlersal_current_time - revlersal_expected_time;
        }
    }
    LOG_D("bt_time_diff:%d", bt_time_diff);

    if (isPacketValid == GAF_TIME_STAMP_CHECK_MATCH_EXPECED &&
        ABS(bt_time_diff) < GAF_AUDIO_MAX_DIFF_BT_TIME && pidAdjust)
    {
        gaf_media_pid_adjust(AUD_STREAM_PLAYBACK, &(pStreamEnv->stream_context.playback_pid_env), bt_time_diff);
    }
    return isPacketValid;
}

bool gaf_stream_common_get_packet(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
    CC_PLAYBACK_DATA_T *out_frame, uint8_t instance_handle,
    uint32_t dmaIrqHappeningTimeUs, uint32_t sink_play_delay, bool pidAdjust)
{
    uint32_t isPacketValid = GAF_TIME_STAMP_CHECK_MATCH_EXPECED;
    cfifo *fifo = pStreamEnv->stream_context.playback_frame_fifo[instance_handle];
    uint32_t headerLen = sizeof(CC_PLAYBACK_DATA_T);
    uint32_t packet_size = pStreamEnv->stream_info.playbackInfo.codec_info.frame_size * \
                    pStreamEnv->stream_info.playbackInfo.codec_info.num_channels;
    uint16_t expectedSeq = pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX];
    uint8_t *data = out_frame->data;

peek_again:
    do {
        if (cfifo_len(fifo) < packet_size + headerLen)
        {
            LOG_W("playback frame fifo is empty.");
            out_frame->isPLC = true;
            break;
        }

        cfifo_peek_to_buf(fifo, (uint8_t *)out_frame, headerLen);

        if (!gaf_stream_get_prefill_status())
        {
            isPacketValid = gaf_stream_common_get_packet_status(pStreamEnv,
                    dmaIrqHappeningTimeUs, out_frame, sink_play_delay, pidAdjust);
        }

        LOG_D("%s pkt_seq:%d expected_seq:%d", __func__, out_frame->seq_nb, expectedSeq);
        if (expectedSeq != out_frame->seq_nb)
        {
            LOG_D("get pkt seq mismatch, seq: %d-%d", out_frame->seq_nb, expectedSeq);
        }

        if (GAF_TIME_STAMP_CHECK_MATCH_EXPECED == isPacketValid)
        {
            cfifo_pop(fifo, NULL, headerLen);
            cfifo_pop(fifo, data, packet_size);
        }
        else
        {
            LOG_W("TIME check:%d", isPacketValid);
            if (GAF_TIME_STAMP_CHECK_EARLIER_THAN_EXPECED == isPacketValid)
            {
                cfifo_pop(fifo, NULL, headerLen + packet_size);
                pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]++;
                expectedSeq = pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX];
                goto peek_again;
            }
        }
    } while(0);

    out_frame->data_len = packet_size;
    out_frame->data = data;
    pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX] = out_frame->seq_nb;

    if (isPacketValid != GAF_TIME_STAMP_CHECK_MATCH_EXPECED)
    {
        LOG_I("Hit PLC event!");
        out_frame->isPLC = true;
        return false;
    }
    return true;
}

bool gaf_bis_stream_common_get_combined_packet_from_multi_channels(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, CC_PLAYBACK_DATA_T *out_frame, uint32_t dmaIrqHappeningTimeUs, uint32_t sink_play_delay)
{
    uint8_t instance_handle = 0;
    bool ret_l = false;
    bool ret_r = false;
    uint8_t *data = out_frame->data;
    uint32_t frame_size = pStreamEnv->stream_info.playbackInfo.codec_info.frame_size;

    for (uint8_t iso_chan_lid = 0; iso_chan_lid < 2; iso_chan_lid++)
    {
        /// Do not handle invalid iso channel
        if (pStreamEnv->stream_info.playbackInfo.bisChInfo[iso_chan_lid].iso_channel_hdl ==
            GAF_AUDIO_INVALID_ISO_CHANNEL)
        {
            continue;
        }

        if (instance_handle > 1) {
            LOG_W("%s instance_handle can't large than 1", __func__);
            instance_handle = 1;
        }

        if (bes_ble_bap_capa_get_location_bf(BES_BLE_GAF_DIRECTION_SINK) &
                (BES_BLE_LOC_SIDE_LEFT | BES_BLE_LOC_FRONT_LEFT))
        {
            out_frame->data = data;
            ret_l = gaf_stream_common_get_packet(pStreamEnv,
                    out_frame, instance_handle, dmaIrqHappeningTimeUs, sink_play_delay, true);
        }
        else
        {
            out_frame->data = data + frame_size;
            ret_r = gaf_stream_common_get_packet(pStreamEnv,
                    out_frame, instance_handle, dmaIrqHappeningTimeUs, sink_play_delay, false);
        }

        instance_handle++;
    }

    out_frame->data = data;
    out_frame->data_len = frame_size * 2;

    if (!ret_l || !ret_r)
    {
        out_frame->isPLC = true;
        return false;
    }
    return true;
}

#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
bool gaf_stream_common_get_combined_packet_from_multi_channels(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, CC_PLAYBACK_DATA_T *out_frame, uint32_t dmaIrqHappeningTimeUs)
{
    uint8_t instance_handle = 0;
    bool ret_l = false;
    bool ret_r = false;
    uint8_t *data = out_frame->data;
    uint32_t frame_size = pStreamEnv->stream_info.playbackInfo.codec_info.frame_size;

    for (uint32_t index = 0; index < GAF_AUDIO_ASE_TOTAL_COUNT; index++)
    {
        /// Do not handle invalid iso channel
        if (pStreamEnv->stream_info.playbackInfo.aseChInfo[index].iso_channel_hdl ==
            GAF_AUDIO_INVALID_ISO_CHANNEL)
        {
            continue;
        }

        if (instance_handle > 1) {
            LOG_W("%s instance_handle can't larger than 1", __func__);
            instance_handle = 1;
        }

        if (pStreamEnv->stream_info.playbackInfo.aseChInfo[index].allocation_bf &
                (BES_BLE_LOC_SIDE_LEFT | BES_BLE_LOC_FRONT_LEFT))
        {
            out_frame->data = data;
            ret_l = gaf_stream_common_get_packet(pStreamEnv, out_frame,
                instance_handle, dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, true);
        }
        else
        {
            out_frame->data = data + frame_size;
            ret_r = gaf_stream_common_get_packet(pStreamEnv, out_frame,
                instance_handle, dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, false);
        }

        instance_handle++;
    }

    out_frame->data = data;
    out_frame->data_len = frame_size * 2;

    if (!ret_l || !ret_r)
    {
        out_frame->isPLC = true;
        return false;
    }
    return true;
}
#endif

static void gaf_stream_pre_fill_handler(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    LOG_I("start of pre fill");
    app_sysfreq_req(APP_SYSFREQ_USER_STREAM_BOOST, APP_SYSFREQ_208M);
    af_pre_fill_handler(id, stream, PP_PANG);
    app_sysfreq_req(APP_SYSFREQ_USER_STREAM_BOOST, APP_SYSFREQ_32K);
    LOG_I("end of pre fill");
}

#define GAF_PACKET_LOST_TESTx
#ifdef GAF_PACKET_LOST_TEST
static uint16_t cnt = 100;
static uint16_t limit = 300;
#endif
void gaf_stream_common_store_packet(GAF_AUDIO_STREAM_ENV_T *pStreamEnv,
                            CC_PLAYBACK_DATA_T *frame, CODEC_CORE_INFO_T *coreInfo)
{
#ifdef GAF_PACKET_LOST_TEST
    cnt+=1;
    if (cnt % limit == 0 || cnt%limit == 1 || cnt%limit == 2 || cnt%limit == 3 || cnt%limit == 4){
        TRACE(1, "INPUT FILL ERR!!!:%d", frame->seq_nb);
        return NULL;
    }
#endif

    cfifo *frame_fifo = pStreamEnv->stream_context.playback_frame_fifo[coreInfo->instance_handle];
    POSSIBLY_UNUSED cfifo *pcm_fifo = pStreamEnv->stream_context.playback_pcm_fifo[coreInfo->instance_handle];
    uint32_t header_len = sizeof(CC_PLAYBACK_DATA_T);
    uint32_t frame_size = pStreamEnv->stream_info.playbackInfo.codec_info.frame_size * \
                        pStreamEnv->stream_info.playbackInfo.codec_info.num_channels + header_len;
    POSSIBLY_UNUSED uint32_t pcm_size = pStreamEnv->stream_info.playbackInfo.codec_info.pcm_size + header_len;

    if (cfifo_free_space(frame_fifo) < frame_size)
    {
        LOG_I("instance:%d frame fifo overflow, fifo_len:%d frame size:%d",
                coreInfo->instance_handle, cfifo_len(frame_fifo), frame_size);
        goto exit;
    }

    cfifo_put(frame_fifo, (uint8_t*)frame, header_len);
    cfifo_put(frame_fifo, frame->data, frame_size - header_len);

exit:
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (!(*pStreamEnv->stream_context.isDecoding[coreInfo->instance_handle]) &&
        cfifo_free_space(pcm_fifo) > pcm_size)
    {
        gaf_cc_playback_data_notify(coreInfo->core_type);
    }
#endif
    if (gaf_stream_get_prefill_status())
    {
#ifdef ADVANCE_FILL_ENABLED
        pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]++;
        gaf_stream_pre_fill_handler(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
        gaf_stream_set_prefill_status(false);
    }
}

void gaf_stream_common_register_func_list(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
    GAF_AUDIO_FUNC_LIST_T* funcList)
{
    pStreamEnv->func_list = funcList;
}

static const char * const gaf_stream_codec_type_str[] =
{
    "LC3",
    "LC3plus",
    "ull"
    "Unknown codec type",
};

const char* gaf_stream_common_print_code_type(uint8_t codec_id)
{
    switch (codec_id)
    {
        case BES_BLE_GAF_CODEC_TYPE_LC3:
            return gaf_stream_codec_type_str[0];
        case BES_BLE_GAF_CODEC_TYPE_VENDOR:
            return gaf_stream_codec_type_str[1];
        case BES_BLE_GAF_CODEC_TYPE_ULL:
            return gaf_stream_codec_type_str[2];
        default :
            return gaf_stream_codec_type_str[3];
    }
}

static const char * const gaf_stream_context_type_str[] =
{
    "Conversational",
    "Media",
    "AI",
    "Live",
    "Game",
    "RingTone",
    "Instructional",
    "Notification",
    "Alerts",
    "Emergency alarm",
    "Unknown context",
};

const char* gaf_stream_common_print_context(uint16_t context_bf)
{
    switch (context_bf)
    {
        case BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT:
            return gaf_stream_context_type_str[0];
        case BES_BLE_GAF_CONTEXT_TYPE_MEDIA_BIT:
            return gaf_stream_context_type_str[1];
        case BES_BLE_GAF_CONTEXT_TYPE_MAN_MACHINE_BIT:
            return gaf_stream_context_type_str[2];
        case BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT:
            return gaf_stream_context_type_str[3];
        case BES_BLE_GAF_CONTEXT_TYPE_GAME_BIT:
            return gaf_stream_context_type_str[4];
        case BES_BLE_GAF_CONTEXT_TYPE_RINGTONE_BIT:
            return gaf_stream_context_type_str[5];
        case BES_BLE_GAF_CONTEXT_TYPE_INSTRUCTIONAL_BIT:
            return gaf_stream_context_type_str[6];
        case BES_BLE_GAF_CONTEXT_TYPE_ATTENTION_SEEKING_BIT:
            return gaf_stream_context_type_str[7];
        case BES_BLE_GAF_CONTEXT_TYPE_IMMEDIATE_ALERT_BIT:
            return gaf_stream_context_type_str[8];
        case BES_BLE_GAF_CONTEXT_TYPE_EMERGENCY_ALERT_BIT:
            return gaf_stream_context_type_str[9];
        default:
            return gaf_stream_context_type_str[10];//BES intentional code.The size of gaf_stream_context_type_str is 10
    }
}

void gaf_stream_common_clr_trigger(uint8_t triChannel)
{
    btdrv_syn_clr_trigger(triChannel);
#if defined(IBRT)
    app_ble_tws_sync_release_trigger_channel(triChannel);
#endif
}

#if defined(IBRT)
static void gaf_stream_common_trigger_sync_capture(
     GAF_AUDIO_STREAM_CONTEXT_TYPE_E streamContext, uint32_t master_clk_cnt,
     uint16_t master_bit_cnt, int32_t usSinceLatestAnchorPoint, uint32_t triggertimeUs)
{
    AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T streamStatus;
    streamStatus.streamContext = streamContext;
    streamStatus.master_clk_cnt = master_clk_cnt;
    streamStatus.master_bit_cnt = master_bit_cnt;
    streamStatus.usSinceLatestAnchorPoint = usSinceLatestAnchorPoint;
    streamStatus.triggertimeUs = triggertimeUs;

    LOG_I("gaf sync capture info:");
    LOG_I("context %d usGap %d triggerUs %d",
        streamContext, usSinceLatestAnchorPoint, triggertimeUs);

    app_ble_tws_sync_send_cmd(BLE_TWS_SYNC_REQ_TRIGGER_SYNC_CAPTURE, 1,\
                              (uint8_t*)&streamStatus, sizeof(streamStatus));

}
#endif

static void gaf_stream_common_rsp_sync_capture_req(uint16_t req_seq,
     GAF_AUDIO_STREAM_CONTEXT_TYPE_E streamContext, uint32_t master_clk_cnt,
     uint16_t master_bit_cnt, int32_t usSinceLatestAnchorPoint, uint32_t triggertimeUs)
{
    AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T streamStatus;
    streamStatus.streamContext = streamContext;
    streamStatus.master_clk_cnt = master_clk_cnt;
    streamStatus.master_bit_cnt = master_bit_cnt;
    streamStatus.usSinceLatestAnchorPoint = usSinceLatestAnchorPoint;
    streamStatus.triggertimeUs = triggertimeUs;
    app_ble_tws_sync_send_rsp(BLE_TWS_SYNC_REQ_TRIGGER_SYNC_CAPTURE, req_seq,
                              (uint8_t*)&streamStatus, sizeof(streamStatus));
}

void gaf_stream_register_running_stream_ref(uint8_t con_lid, GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
#if defined (GAF_AUDIO_USE_ONE_STREAM_ONLY_ENABLED)
    gaf_audio_running_stream_ref[0] = pStreamEnv;
#else
    gaf_audio_running_stream_ref[con_lid] = pStreamEnv;
#endif
}

static void aob_tws_capture_stream_set_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t trigger_time_us)
{
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    LOG_I("set trigger time %d current time %d", trigger_time_us, current_bt_time);
    int32_t gapUs = 0;
    // check how long since now to the trigger time
    while (gapUs < GAF_AUDIO_CAPTURE_TRIGGER_GUARD_TIME_US)
    {
        trigger_time_us += pStreamEnv->stream_info.captureInfo.isoIntervalUs;
        gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time, trigger_time_us);
    }
    LOG_I("coordinated trigger time %d", trigger_time_us);
    gaf_stream_common_set_capture_trigger_time(pStreamEnv, trigger_time_us);
}

static void aob_tws_capture_stream_trigger_time_request_handler(uint16_t req_seq, AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T* ptrPeerStreamStatus, GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint32_t trigger_time_master_clk_cnt = 0;
    uint16_t trigger_time_master_bit_cnt = 0;
    uint32_t trigger_time_us = 0;
    int32_t usSinceLatestAnchorPoint = GAF_STREAM_INVALID_US_SINCE_LAST_ANCHOR_POINT;
    uint32_t responsed_trigger_time_us = 0;

    APP_BLE_TWS_SYNC_ROLE_E ble_tws_role =  app_ble_tws_get_tws_local_role();

    // if stream is already triggred, share the local start trigger time to peer device
    if (GAF_CAPTURE_STREAM_STREAMING_TRIGGERED ==
        pStreamEnv->stream_context.capture_stream_state)
    {
        trigger_time_us = pStreamEnv->stream_context.lastCaptureDmaIrqTimeUsInTriggerPoint-
                        pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs;
        usSinceLatestAnchorPoint = pStreamEnv->stream_context.usSinceLatestAnchorPoint;
        LOG_I("last trigger time %u usSinceLatestAnchorPoint %u",
            trigger_time_us, usSinceLatestAnchorPoint);
    }
    else
    {
        if (ble_tws_role == APP_BLE_TWS_SLAVE)
        {
            // let primary just use its trigger time
            trigger_time_master_clk_cnt = GAF_STREAM_INVALID_CLOCK_CNT;
            responsed_trigger_time_us = ptrPeerStreamStatus->triggertimeUs;

            if ((GAF_CAPTURE_STREAM_START_TRIGGERING ==
                pStreamEnv->stream_context.capture_stream_state) &&
                (0 != ptrPeerStreamStatus->master_clk_cnt))
            {
                uint32_t master_trigger_time_us =
                    app_ble_tws_sync_get_slave_time_from_master_time(
                        ptrPeerStreamStatus->master_clk_cnt, ptrPeerStreamStatus->master_bit_cnt);
                aob_tws_capture_stream_set_trigger_time(pStreamEnv, master_trigger_time_us);
            }
        }
        else
        {
            if (GAF_CAPTURE_STREAM_START_TRIGGERING ==
                pStreamEnv->stream_context.capture_stream_state)
            {
                trigger_time_us = pStreamEnv->stream_context.captureTriggerStartTicks;
            }
            else
            {
                trigger_time_master_clk_cnt = GAF_STREAM_INVALID_CLOCK_CNT;
                responsed_trigger_time_us = ptrPeerStreamStatus->triggertimeUs;
            }
        }
    }

    if (trigger_time_master_clk_cnt != GAF_STREAM_INVALID_CLOCK_CNT)
    {
        if (ble_tws_role == APP_BLE_TWS_SLAVE)
        {
            app_ble_tws_sync_get_master_time_from_slave_time(trigger_time_us,
                &trigger_time_master_clk_cnt, &trigger_time_master_bit_cnt);
        }
        else
        {
            btdrv_reg_op_bts_to_bt_time(trigger_time_us,
                &trigger_time_master_clk_cnt, &trigger_time_master_bit_cnt);
        }
    }

    gaf_stream_common_rsp_sync_capture_req(
        req_seq,
        pStreamEnv->stream_info.contextType,
        trigger_time_master_clk_cnt,
        trigger_time_master_bit_cnt,
        usSinceLatestAnchorPoint, responsed_trigger_time_us);
}

static GAF_AUDIO_STREAM_ENV_T* aob_tws_sync_capture_get_stream_env(AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T* pStreamStatus)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = NULL;

#if !defined (GAF_AUDIO_USE_ONE_STREAM_ONLY_ENABLED)
    ASSERT(0, "add more function to get stream you want");
#endif

    if (gaf_audio_running_stream_ref[0] != NULL)
    {
        pStreamEnv = gaf_audio_running_stream_ref[0];
    }
    else
    {
        LOG_I("Local stream cb don't register.");
        return NULL;
    }

    if (NULL == pStreamEnv)
    {
        LOG_I("Local stream don't find.");
        return NULL;
    }

    if (GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state)
    {
        LOG_I("Local stream not configured yet.");
        return NULL;
    }

    return pStreamEnv;

}


void aob_tws_sync_capture_trigger_handler(uint16_t req_seq, uint8_t* data, uint16_t len)
{
    AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T* pStreamStatus = (AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T *)data;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = aob_tws_sync_capture_get_stream_env(pStreamStatus);

    if (NULL == pStreamEnv)
    {
        // response peer device to let it just do its local trigger
        gaf_stream_common_rsp_sync_capture_req(req_seq, (GAF_AUDIO_STREAM_CONTEXT_TYPE_E)pStreamStatus->streamContext,
            GAF_STREAM_INVALID_CLOCK_CNT, 0, 0, pStreamStatus->triggertimeUs);
    }
    else
    {
        aob_tws_capture_stream_trigger_time_request_handler(req_seq, pStreamStatus, pStreamEnv);
    }
}

void aob_tws_sync_capture_trigger_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T* pStreamStatus = (AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T *)p_buff;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = aob_tws_sync_capture_get_stream_env(pStreamStatus);
    if (NULL != pStreamEnv)
    {
        aob_tws_capture_stream_set_trigger_time(pStreamEnv, pStreamStatus->triggertimeUs);
    }
}

void aob_tws_sync_capture_trigger_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T* ptrPeerStreamStatus = (AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T *)p_buff;

    uint32_t trigger_time_us = 0;
    LOG_I("local clk cnt %u master_clk_cnt %u master_bit_cnt %u usSinceLatestAnchorPoint %d",
        btdrv_syn_get_curr_ticks(),
        ptrPeerStreamStatus->master_clk_cnt,
        ptrPeerStreamStatus->master_bit_cnt,
        ptrPeerStreamStatus->usSinceLatestAnchorPoint);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = aob_tws_sync_capture_get_stream_env(ptrPeerStreamStatus);

    if (ptrPeerStreamStatus->master_clk_cnt == GAF_STREAM_INVALID_CLOCK_CNT)
    {
        if (NULL != pStreamEnv)
        {
            aob_tws_capture_stream_set_trigger_time(pStreamEnv, ptrPeerStreamStatus->triggertimeUs);
        }
        return;
    }

    if (app_ble_tws_get_tws_local_role() == APP_BLE_TWS_SLAVE)
    {
        trigger_time_us = app_ble_tws_sync_get_slave_time_from_master_time(
            ptrPeerStreamStatus->master_clk_cnt,
            ptrPeerStreamStatus->master_bit_cnt);
    }
    else
    {
        trigger_time_us = bt_syn_ble_bt_time_to_bts(ptrPeerStreamStatus->master_clk_cnt,
            ptrPeerStreamStatus->master_bit_cnt);
    }

    if (NULL != pStreamEnv)
    {
        aob_tws_capture_stream_set_trigger_time(pStreamEnv, trigger_time_us);
        if (GAF_STREAM_INVALID_US_SINCE_LAST_ANCHOR_POINT != ptrPeerStreamStatus->usSinceLatestAnchorPoint)
        {
            pStreamEnv->stream_context.isUsSinceLatestAnchorPointConfigured = true;
            pStreamEnv->stream_context.usSinceLatestAnchorPoint = ptrPeerStreamStatus->usSinceLatestAnchorPoint;
        }
    }
}

void aob_tws_sync_us_since_latest_anchor_point_handler(uint16_t rsp_seq, uint8_t* data, uint16_t len)
{
    AOB_TWS_SYNC_US_SINCE_LATEST_ANCHOR_POINT_T* info = (AOB_TWS_SYNC_US_SINCE_LATEST_ANCHOR_POINT_T *)data;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = NULL;

#if !defined (GAF_AUDIO_USE_ONE_STREAM_ONLY_ENABLED)
    ASSERT(0, "add more function to get stream you want");
#endif

    if (gaf_audio_running_stream_ref[0] !=NULL)
    {
        pStreamEnv = gaf_audio_running_stream_ref[0];
        LOG_I("get peer side context %d usSinceLatestAnchorPoint %d", info->streamContext, info->usSinceLatestAnchorPoint);
    }
    else
    {
        LOG_I("Local stream cb don't register.");
        return;
    }

    if (pStreamEnv)
    {
        LOG_I("local stream state %d", pStreamEnv->stream_context.capture_stream_state);
        LOG_I("isUsSinceLatestAnchorPointConfigured %d usSinceLatestAnchorPoint %d",
            pStreamEnv->stream_context.isUsSinceLatestAnchorPointConfigured, pStreamEnv->stream_context.usSinceLatestAnchorPoint);
        if (pStreamEnv->stream_context.capture_stream_state >= GAF_CAPTURE_STREAM_INITIALIZED)
        {
            pStreamEnv->stream_context.isUsSinceLatestAnchorPointConfigured = true;
            pStreamEnv->stream_context.usSinceLatestAnchorPoint = info->usSinceLatestAnchorPoint;
        }
    }
}

void gaf_stream_common_sync_us_since_latest_anchor_point(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
#if defined(IBRT)
    if (app_ble_tws_get_conn_state())
    {
        if (app_ble_tws_get_tws_local_role() != APP_BLE_TWS_SLAVE)
        {
            AOB_TWS_SYNC_US_SINCE_LATEST_ANCHOR_POINT_T info;
            info.streamContext = pStreamEnv->stream_info.contextType;
            info.usSinceLatestAnchorPoint = pStreamEnv->stream_context.usSinceLatestAnchorPoint;
            app_ble_tws_sync_send_cmd(BLE_TWS_SYNC_CAPTURE_US_SINCE_LATEST_ANCHOR_POINT, 1,\
                                      (uint8_t*)&info, sizeof(info));
        }
    }
#endif
}

void gaf_stream_common_start_sync_capture(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    uint32_t trigger_bt_time = latest_iso_bt_time+(pStreamEnv->stream_info.captureInfo.cigSyncDelayUs/pStreamEnv->stream_info.captureInfo.bnS2M/2);
    // move ahead of trigger time by 1ms to leave more margin for long CIG delay
    if (pStreamEnv->stream_info.captureInfo.cigSyncDelayUs > 3000)
    {
        trigger_bt_time -= 1000;
    }

    while (trigger_bt_time < current_bt_time + GAF_AUDIO_MAX_DIFF_BT_TIME)
    {
        trigger_bt_time += pStreamEnv->stream_info.captureInfo.isoIntervalUs;
    }


    LOG_I("iso anch %d cur time %d trigger time %d",
        latest_iso_bt_time, current_bt_time, trigger_bt_time);

#if defined(IBRT)
    if (app_ble_tws_get_conn_state())
    {
        uint32_t triggertimeUs = trigger_bt_time+
            ((200000/pStreamEnv->stream_info.captureInfo.isoIntervalUs)*
            pStreamEnv->stream_info.captureInfo.isoIntervalUs);
        if (app_ble_tws_get_tws_local_role() == APP_BLE_TWS_SLAVE)
        {
            gaf_stream_common_trigger_sync_capture(
                pStreamEnv->stream_info.contextType, 0, 0,
                GAF_STREAM_INVALID_US_SINCE_LAST_ANCHOR_POINT, triggertimeUs);
        }
        else
        {
            uint32_t masterClockCnt;
            uint16_t finecnt;
            btdrv_reg_op_bts_to_bt_time(triggertimeUs, &masterClockCnt, &finecnt);
            gaf_stream_common_trigger_sync_capture(
                pStreamEnv->stream_info.contextType, masterClockCnt, finecnt,
                GAF_STREAM_INVALID_US_SINCE_LAST_ANCHOR_POINT, triggertimeUs);
        }
    }
    else
#endif
    {
        gaf_stream_common_set_capture_trigger_time(pStreamEnv,
            trigger_bt_time+((20000/pStreamEnv->stream_info.captureInfo.isoIntervalUs)*
            pStreamEnv->stream_info.captureInfo.isoIntervalUs));
    }
}

POSSIBLY_UNUSED uint8_t gaf_stream_common_get_allocation_lr_cnt(uint32_t audio_allocation_bf)
{
    uint8_t audio_allocation_cnt = 0;
    /// TODO:we just need to count bits here
    if (audio_allocation_bf & (BES_BLE_LOC_SIDE_LEFT | BES_BLE_LOC_FRONT_LEFT))
    {
        audio_allocation_cnt++;
    }
    if (audio_allocation_bf & (BES_BLE_LOC_SIDE_RIGHT | BES_BLE_LOC_FRONT_RIGHT))
    {
        audio_allocation_cnt++;
    }
    /// dft is 1
    if (audio_allocation_cnt == 0)
    {
        audio_allocation_cnt = 1;
    }
    return audio_allocation_cnt;
}

bool gaf_stream_get_prefill_status(void)
{
    return gaf_is_doing_prefill;
}

void gaf_stream_set_prefill_status(bool is_doing)
{
    gaf_is_doing_prefill = is_doing;
}

void gaf_stream_common_set_custom_data_handler(GAF_STREAM_USER_CASE_E gaf_user_case,
                                               const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T *func_list)
{
    ASSERT((gaf_user_case < GAF_STREAM_USER_CASE_MAX), "%s case idx err", __func__);

    GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T *func_local = &gaf_stream_common_custom_data_func_table[gaf_user_case];
    *func_local = *func_list;
    LOG_I("%s, [%p] [%p] [%p] [%p]", __func__, func_local->decoded_raw_data_cb, func_local->raw_pcm_data_cb,
                                               func_local->encoded_packet_recv_cb, func_local->decoded_raw_data_cb);
}

const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T
     *gaf_stream_common_get_custom_data_handler(GAF_STREAM_USER_CASE_E gaf_user_case)
{
    ASSERT((gaf_user_case < GAF_STREAM_USER_CASE_MAX), "%s case idx err", __func__);

    return &gaf_stream_common_custom_data_func_table[gaf_user_case];
}

void gaf_stream_common_set_custom_presdelay_us(uint32_t presDelayUs)
{
    if (presDelayUs <= 0)
    {
        LOG_E("%s presDelayUs %d is not right", __func__, presDelayUs);
        return;
    }

    gaf_stream_custom_presdelay_us = presDelayUs;
    LOG_I("%s %d", __func__, presDelayUs);
}

uint32_t gaf_stream_common_get_custom_presdelay_us(void)
{
    return gaf_stream_custom_presdelay_us;
}

uint8_t gaf_stream_common_get_ase_idx_in_ase_lid_list(uint8_t* ase_lid_list, uint8_t ase_lid)
{
    for (uint8_t idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
    {
        if (ase_lid_list[idx] == ase_lid)
        {
            return idx;
        }
    }
    return GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT;
}

uint8_t gaf_stream_common_get_valid_idx_in_ase_lid_list(uint8_t* ase_lid_list)
{
    for (uint8_t idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
    {
        if (ase_lid_list[idx] == GAF_INVALID_ASE_INDEX)
        {
            return idx;
        }
    }

    return GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT;
}

static void gaf_media_stream_freq_boost_timeout_handler(void const *n);
osTimerDef (GAF_MEIDA_STREAM_FREQ_BOOST_TIMER, gaf_media_stream_freq_boost_timeout_handler);
osTimerId gaf_media_stream_freq_boost_timer_id = NULL;
void gaf_media_stream_boost_freq(uint32_t stayingMs)
{
    if (NULL == gaf_media_stream_freq_boost_timer_id)
    {
        gaf_media_stream_freq_boost_timer_id = osTimerCreate(
            osTimer(GAF_MEIDA_STREAM_FREQ_BOOST_TIMER), osTimerOnce, NULL);
    }

    app_sysfreq_req(APP_SYSFREQ_USER_STREAM_BOOST, APP_SYSFREQ_208M);

    osTimerStart(gaf_media_stream_freq_boost_timer_id, stayingMs);
}

static void gaf_media_stream_freq_boost_timeout_handler(void const *n)
{
    app_sysfreq_req(APP_SYSFREQ_USER_STREAM_BOOST, APP_SYSFREQ_32K);
}

bool gaf_stream_is_capture_stream_iso_created(void *_pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)_pStreamEnv;

    if (pStreamEnv->stream_info.is_bis)
    {
        for (uint8_t i = 0; i < 2; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.captureInfo.bisChInfo[i].iso_channel_hdl)
            {
                return true;
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl)
            {
                return true;
            }
        }
    }
    return false;
}

bool gaf_stream_is_playback_stream_iso_created(void *_pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)_pStreamEnv;
    if (pStreamEnv->stream_info.is_bis)
    {
        for (uint8_t i = 0; i < 2; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.bisChInfo[i].iso_channel_hdl)
            {
                return true;
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl)
            {
                return true;
            }
        }
    }
    return false;
}

void* gaf_bis_stream_get_codec_core_info(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                            uint8_t direction, uint8_t iso_channel)
{
    GAF_AUDIO_STREAM_COMMON_INFO_T *pCommonInfo = NULL;
    if (direction == BES_BLE_GAF_DIRECTION_SINK) {
        pCommonInfo = &pStreamEnv->stream_info.playbackInfo;
    }
    else {
        pCommonInfo = &pStreamEnv->stream_info.captureInfo;
    }

    for (uint8_t chn_lid = 0; chn_lid < 2; chn_lid++)
    {
        if (pCommonInfo->bisChInfo[chn_lid].iso_channel_hdl == iso_channel)
        {
            return &(pCommonInfo->bisChInfo[chn_lid].codec_core_info);
        }
    }

    LOG_E("%s iso_channel:%d err!!!", __func__, iso_channel);
    return NULL;
}

bool gaf_stream_common_store_pcm(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint32_t dmairqhappentime,
            uint8_t* ptrBuf, uint32_t length, uint32_t frameLen, CODEC_CORE_INFO_T *coreInfo)
{
    if (coreInfo->instance_status < INSTANCE_INITIALIZED) {
        return false;
    }

    cfifo *fifo = pStreamEnv->stream_context.capture_pcm_fifo[coreInfo->instance_handle];
    CC_CAPTURE_DATA_T header;

    if (cfifo_free_space(fifo) < length + sizeof(CC_CAPTURE_DATA_T))
    {
        LOG_W("instance:%d pcm fifo overflow", coreInfo->instance_handle);
        goto exit;
    }

    header.time_stamp = dmairqhappentime;
    header.data_len = length;
    header.frame_size = frameLen;

    cfifo_put(fifo, (uint8_t*)(&header), sizeof(CC_CAPTURE_DATA_T));
    cfifo_put(fifo, ptrBuf, length);

exit:
    if (!(*pStreamEnv->stream_context.isEncoding[coreInfo->instance_handle]))
    {
        gaf_cc_capture_data_notify(coreInfo->core_type);
    }

    return true;
}

bool gaf_stream_common_fetch_frame(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t *ptrData,
            uint32_t dataLen, uint32_t currentDmaIrqTimeUs, uint8_t instance_handle)
{
    uint8_t waitFrameCnt = 0;
    cfifo *fifo = pStreamEnv->stream_context.capture_frame_fifo[instance_handle];
    uint32_t headerLen = sizeof(CC_CAPTURE_DATA_T);
    uint32_t frame_size = pStreamEnv->stream_info.captureInfo.codec_info.frame_size * \
                pStreamEnv->stream_info.captureInfo.codec_info.num_channels;
    CC_CAPTURE_DATA_T out_frame = {0};

    do
    {
peek_packet:
        if (GAF_CAPTURE_STREAM_STREAMING_TRIGGERED != pStreamEnv->stream_context.capture_stream_state)
        {
            break;
        }

        if (cfifo_len(fifo) < frame_size + headerLen)
        {
            waitFrameCnt++;
            if (GAF_MAX_WAIT_FRAME_CNT >= waitFrameCnt)
            {
                LOG_W("instance:%d frame fifo empty, wait %d ms!",
                    instance_handle, waitFrameCnt * GAF_WAIT_FRAME_MS);
                osDelay(GAF_WAIT_FRAME_MS);
                goto peek_packet;
            }
            else
            {
                return false;
            }
        }

        cfifo_peek_to_buf(fifo, (uint8_t *)&out_frame, headerLen);

        int32_t gapUs = (int32_t)(currentDmaIrqTimeUs - out_frame.time_stamp -
                    pStreamEnv->stream_context.usStorePcmToFetchFrame);
        LOG_D("cur dma ts:%u  pkt ts:%u  gapUs:%d",
                currentDmaIrqTimeUs, out_frame.time_stamp, gapUs);

        if (GAF_AUDIO_ABS(gapUs) < (int32_t)pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2)
        {
            cfifo_pop(fifo, NULL, headerLen);
            cfifo_pop(fifo, ptrData, dataLen);
            cfifo_pop(fifo, NULL, frame_size - dataLen);
        }
        else
        {
            LOG_I("cur dma ts:%u  pkt ts:%u  gapUs:%d fifo len:%d",
                currentDmaIrqTimeUs, out_frame.time_stamp, gapUs, cfifo_len(fifo));
            if (gapUs < 0)
            {
                LOG_I("frame's transmission time is not arriving yet.");
            }
            else
            {
                LOG_I("frame is expired, discard it.");
                cfifo_pop(fifo, NULL, frame_size + headerLen);
                goto peek_packet;
            }
        }
    }while(0);
    return true;
}

bool gaf_stream_common_fetch_multi_pcm(GAF_AUDIO_STREAM_ENV_T *pStreamEnv,
        uint8_t* ptrBuf, uint32_t dataLen, uint32_t dmaIrqHappeningTimeUs)
{
    uint8_t instance_handle = 0;
    uint32_t data_offset = 0;
    GAF_AUDIO_STREAM_COMMON_INFO_T *playbackInfo = &pStreamEnv->stream_info.playbackInfo;
    uint32_t num_channels = playbackInfo->dma_info.num_channels;
    uint32_t bits_depth = playbackInfo->dma_info.bits_depth;
    uint32_t pcm_len = playbackInfo->codec_info.pcm_size;
    uint8_t *pcm_cache = pStreamEnv->stream_context.playback_pcm_cache;

    memset(ptrBuf, 0, dataLen);

    for (uint32_t index = 0; index < GAF_AUDIO_ASE_TOTAL_COUNT; index++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL == playbackInfo->aseChInfo[index].iso_channel_hdl ||
            playbackInfo->aseChInfo[index].codec_core_info.instance_status < INSTANCE_INITIALIZED)
        {
            continue;
        }
        if (playbackInfo->aseChInfo[index].allocation_bf &
            (BES_BLE_LOC_SIDE_LEFT | BES_BLE_LOC_FRONT_LEFT))
        {
            data_offset = 0;
        }
        else
        {
            data_offset = 1;
        }

        instance_handle = playbackInfo->aseChInfo[index].codec_core_info.instance_handle;
        gaf_stream_common_fetch_pcm(pStreamEnv, pcm_cache, pcm_len,
            instance_handle, dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, 0 == data_offset);

        if (24 == bits_depth)
        {
            int32_t *data = (int32_t*)ptrBuf;
            int32_t *cache = (int32_t*)pcm_cache;
            uint32_t len = pcm_len / sizeof(int32_t);
            for (uint32_t i = 0; i < len; i++) {
                data[i * num_channels + data_offset] = cache[i];
            }
        }
        else if (16 == bits_depth)
        {
            int16_t *data = (int16_t*)ptrBuf;
            int16_t *cache = (int16_t*)pcm_cache;
            uint32_t len = pcm_len / sizeof(int16_t);
            for (uint32_t i = 0; i < len; i++) {
                data[i * num_channels + data_offset] = cache[i];
            }
        }
        else
        {
            ASSERT(false, "%s bits_depth:%d", __func__, bits_depth);
        }
    }
    return true;
}

bool gaf_stream_common_fetch_pcm(GAF_AUDIO_STREAM_ENV_T *pStreamEnv,
    uint8_t* ptrData, uint32_t dataLen, uint8_t instance_handle,
     uint32_t dmaIrqHappeningTimeUs, uint32_t sink_play_delay, bool pidAdjust)
{
    cfifo *fifo = pStreamEnv->stream_context.playback_pcm_fifo[instance_handle];
    uint8_t waitPcmDataCount = 0;
    CC_PLAYBACK_DATA_T media_data;
    uint32_t header_len = sizeof(CC_PLAYBACK_DATA_T);
    uint32_t isVaildPacket = GAF_TIME_STAMP_CHECK_MATCH_EXPECED;

    do
    {
peek_again:
        if (GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED != pStreamEnv->stream_context.playback_stream_state)
        {
            return false;
        }

        if (cfifo_len(fifo) < dataLen + header_len)
        {
            waitPcmDataCount++;
            if (GAF_MAX_WAIT_PCM_CNT >= waitPcmDataCount)
            {
                LOG_W("instance:%d pcm fifo empty, wait %d ms!", instance_handle, waitPcmDataCount * GAF_WAIT_PCM_MS);
                osDelay(GAF_WAIT_PCM_MS);
                goto peek_again;
            }
            else
            {
                LOG_W("instance:%d can not get pcm!!!", instance_handle);
                return false;
            }
        }

        cfifo_peek_to_buf(fifo, (uint8_t *)&media_data, header_len);

        uint16_t expectedSeqNum = pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX];
        LOG_D("bth fetch_pcm_data pkt_seq:%d expected_seq:%d", media_data.seq_nb, expectedSeqNum);
        if (expectedSeqNum != media_data.seq_nb)
        {
            LOG_D("fetch pcm seq mismatch, seq: %d-%d", media_data.seq_nb, expectedSeqNum);
        }

        if (!gaf_stream_get_prefill_status())
        {
            isVaildPacket = gaf_stream_common_get_packet_status(pStreamEnv,
                    dmaIrqHappeningTimeUs, &media_data, sink_play_delay, pidAdjust);
        }

        if (GAF_TIME_STAMP_CHECK_MATCH_EXPECED == isVaildPacket)
        {
            cfifo_pop(fifo, NULL, header_len);
            cfifo_pop(fifo, ptrData, dataLen);
        }
        else
        {
            int32_t gapUs = int32_t(dmaIrqHappeningTimeUs - media_data.time_stamp);
            LOG_W("TIME check:%d cur dma ts:%u pkt ts:%u gapUs:%d fifo len:%d",
                isVaildPacket, dmaIrqHappeningTimeUs, media_data.time_stamp, gapUs, cfifo_len(fifo));
            if (GAF_TIME_STAMP_CHECK_EARLIER_THAN_EXPECED == isVaildPacket)
            {
                pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]++;
                cfifo_pop(fifo, NULL, header_len + dataLen);
                goto peek_again;
            }
            else
            {
                memset(ptrData, 0, dataLen);
            }
        }
    } while(0);

    pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX] = media_data.seq_nb;

    return true;
}

bool gaf_bis_playback_fetch_multi_pcm_data(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t* ptrBuf,
        uint32_t dataLen, uint32_t audio_play_delay, uint32_t dmaIrqHappeningTimeUs)
{
    uint8_t instance_handle = 0;
    uint32_t data_offset = 0;
    GAF_AUDIO_STREAM_COMMON_INFO_T *playbackInfo = &pStreamEnv->stream_info.playbackInfo;
    uint32_t num_channels = playbackInfo->dma_info.num_channels;
    uint32_t bits_depth = playbackInfo->dma_info.bits_depth;
    uint32_t pcm_len = playbackInfo->codec_info.pcm_size;
    uint8_t *pcm_cache = pStreamEnv->stream_context.playback_pcm_cache;

    memset(ptrBuf, 0, dataLen);

    for (uint32_t index = 0; index < 2; index++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL == playbackInfo->bisChInfo[index].iso_channel_hdl ||
            playbackInfo->bisChInfo[index].codec_core_info.instance_status < INSTANCE_INITIALIZED)
        {
            continue;
        }
        if (playbackInfo->bisChInfo[index].select_ch_map &
            (BES_BLE_LOC_SIDE_LEFT | BES_BLE_LOC_FRONT_LEFT))
        {
            data_offset = 0;
        }
        else
        {
            data_offset = 1;
        }

        instance_handle = playbackInfo->bisChInfo[index].codec_core_info.instance_handle;
        gaf_stream_common_fetch_pcm(pStreamEnv, pcm_cache, pcm_len,
            instance_handle, dmaIrqHappeningTimeUs, audio_play_delay, 0 == data_offset);

        if (24 == bits_depth)
        {
            int32_t *data = (int32_t*)ptrBuf;
            int32_t *cache = (int32_t*)pcm_cache;
            uint32_t len = pcm_len / sizeof(int32_t);
            for (uint32_t i = 0; i < len; i++) {
                data[i * num_channels + data_offset] = cache[i];
            }
        }
        else if (16 == bits_depth)
        {
            int16_t *data = (int16_t*)ptrBuf;
            int16_t *cache = (int16_t*)pcm_cache;
            uint32_t len = pcm_len / sizeof(int16_t);
            for (uint32_t i = 0; i < len; i++) {
                data[i * num_channels + data_offset] = cache[i];
            }
        }
        else
        {
            ASSERT(false, "%s bits_depth:%d", __func__, bits_depth);
        }
    }

    return true;
}

void *gaf_stream_common_buf_alloc(void *pool, uint32_t size)
{
    uint8_t *buf;
    app_audio_mempool_get_buff(&buf, size);
    ASSERT(buf, "%s size:%d", __func__, size);
    return buf;
}

void gaf_playback_decoder_init(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t ase_lid)
{
    LOG_I("%s ", __func__);
    GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T *aseInfo = pStreamEnv->stream_info.playbackInfo.aseChInfo;
    GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T *bisInfo = pStreamEnv->stream_info.playbackInfo.bisChInfo;
    CC_PLAYBACK_INIT_REQ_T initReq;
    uint32_t core_type = M33_CORE;
    bool isBis = pStreamEnv->stream_info.is_bis;

    if ((!isBis && aseInfo[ase_lid].codec_core_info.instance_status != INSTANCE_IDLE) ||
        (isBis && bisInfo[ase_lid].codec_core_info.instance_status != INSTANCE_IDLE))
    {
        LOG_W("instance has been initialized!");
        return;
    }

    //TO DO: choose core type
#ifdef GAF_CODEC_CROSS_CORE
    core_type = M55_CORE;
#endif
#ifdef AOB_CODEC_CP
    core_type = CP_CORE;
#endif

    if (isBis) {
        bisInfo[ase_lid].codec_core_info.core_type = core_type;
        bisInfo[ase_lid].codec_core_info.instance_status = INSTANCE_INITIALIZING;
        initReq.ptr = (void*)&bisInfo[ase_lid];
    }
    else {
        aseInfo[ase_lid].codec_core_info.core_type = core_type;
        aseInfo[ase_lid].codec_core_info.instance_status = INSTANCE_INITIALIZING;
        initReq.ptr = (void*)&aseInfo[ase_lid];
    }
    initReq.decoder_info = pStreamEnv->stream_info.playbackInfo.codec_info;
    initReq.codec_type = pStreamEnv->stream_info.codec_type;
    initReq.isBis = isBis;
    initReq.ptr2 = pStreamEnv;
    gaf_cc_playback_feed_init_req(&initReq, core_type);
}

static void gaf_playback_decoder_init_rsp(void *_initRsp)
{
    CC_PLAYBACK_INIT_RSP_T *initRsp = (CC_PLAYBACK_INIT_RSP_T*)_initRsp;
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)initRsp->ptr2;
    if (initRsp->isBis) {
        GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T *pInfo = (GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T*)initRsp->ptr;
        pInfo->codec_core_info.instance_handle = initRsp->instance_handle;
        pInfo->codec_core_info.instance_status = INSTANCE_INITIALIZED;
    }
    else {
        GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T *pInfo = (GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T*)initRsp->ptr;
        pInfo->codec_core_info.instance_handle = initRsp->instance_handle;
        pInfo->codec_core_info.instance_status = INSTANCE_INITIALIZED;
    }
    pStreamEnv->stream_context.playback_frame_fifo[initRsp->instance_handle] = initRsp->frame_fifo;
    pStreamEnv->stream_context.playback_pcm_fifo[initRsp->instance_handle] = initRsp->pcm_fifo;
    pStreamEnv->stream_context.isDecoding[initRsp->instance_handle] = initRsp->isDecoding;
    LOG_I("%s instance_handle:%d", __func__, initRsp->instance_handle);
}

void gaf_playback_decoder_deinit_by_ase(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t ase_lid, bool isRetrigger)
{
    LOG_I("%s ase_lid:%d", __func__, ase_lid);
    GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T *aseInfo = &pStreamEnv->stream_info.playbackInfo.aseChInfo[ase_lid];
    if (aseInfo->codec_core_info.instance_status == INSTANCE_IDLE)
    {
        LOG_I("instance has been deinit instance:%d", aseInfo->codec_core_info.instance_handle);
        return;
    }

    CC_PLAYBACK_DEINIT_REQ_T deinitReq;
    deinitReq.isRetrigger = isRetrigger;
    deinitReq.isBis = false;
    deinitReq.instance_handle = aseInfo->codec_core_info.instance_handle;
    aseInfo->codec_core_info.instance_status = INSTANCE_IDLE;
    gaf_cc_playback_feed_deinit_req(&deinitReq, aseInfo->codec_core_info.core_type);
}

void gaf_playback_decoder_deinit_all_ase(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, bool isRetrigger)
{
    LOG_I("%s ", __func__);

    CC_PLAYBACK_DEINIT_REQ_T deinitReq;
    uint32_t core_type = M33_CORE;
    bool isBis = pStreamEnv->stream_info.is_bis;
    GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T *aseInfo = pStreamEnv->stream_info.playbackInfo.aseChInfo;
    GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T *bisInfo = pStreamEnv->stream_info.playbackInfo.bisChInfo;
    uint32_t indexMax = isBis? 2: GAF_AUDIO_ASE_TOTAL_COUNT;

    for (uint32_t index = 0; index < indexMax; index++)
    {
        if ((isBis && bisInfo[index].codec_core_info.instance_status == INSTANCE_IDLE) ||
            (!isBis && aseInfo[index].codec_core_info.instance_status == INSTANCE_IDLE))
        {
            continue;
        }

        if (isBis) {
            deinitReq.instance_handle = bisInfo[index].codec_core_info.instance_handle;
            core_type= bisInfo[index].codec_core_info.core_type;
            bisInfo[index].codec_core_info.instance_status = INSTANCE_IDLE;
        }
        else {
            deinitReq.instance_handle = aseInfo[index].codec_core_info.instance_handle;
            core_type= aseInfo[index].codec_core_info.core_type;
            aseInfo[index].codec_core_info.instance_status = INSTANCE_IDLE;
        }
        deinitReq.isRetrigger = isRetrigger;
        deinitReq.isBis = isBis;
        gaf_cc_playback_feed_deinit_req(&deinitReq, core_type);
    }
}

void gaf_capture_encoder_init(GAF_AUDIO_STREAM_ENV_T *pStreamEnv)
{
    GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T *aseInfo = &pStreamEnv->stream_info.captureInfo.aseChInfo[0];
    GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T *bisChInfo = &pStreamEnv->stream_info.captureInfo.bisChInfo[0];
    CC_CAPTURE_INIT_REQ_T initReq;
    uint32_t core_type = M33_CORE;

    //TO DO: choose core type
#ifdef GAF_CODEC_CROSS_CORE
    core_type = M55_CORE;
#endif
#ifdef AOB_CODEC_CP
    core_type = CP_CORE;
#endif
    if (pStreamEnv->stream_info.is_bis) {
        bisChInfo->codec_core_info.core_type = core_type;
        bisChInfo->codec_core_info.instance_status = INSTANCE_INITIALIZING;
    }
    else {
        aseInfo->codec_core_info.core_type = core_type;
        aseInfo->codec_core_info.instance_status = INSTANCE_INITIALIZING;
    }

    initReq.tx_algo_cfg.bypass = pStreamEnv->stream_info.tx_algo_cfg.bypass;
    initReq.tx_algo_cfg.frame_len = pStreamEnv->stream_info.tx_algo_cfg.frame_len;
    initReq.tx_algo_cfg.algo_frame_len = pStreamEnv->stream_info.tx_algo_cfg.algo_frame_len;
    initReq.tx_algo_cfg.sample_rate = pStreamEnv->stream_info.tx_algo_cfg.sample_rate;
    initReq.tx_algo_cfg.channel_num = pStreamEnv->stream_info.tx_algo_cfg.channel_num;
    initReq.tx_algo_cfg.bits = pStreamEnv->stream_info.tx_algo_cfg.bits;

    initReq.encoder_info = pStreamEnv->stream_info.captureInfo.codec_info;
    initReq.codec_type = pStreamEnv->stream_info.codec_type;
    initReq.pcm_size = pStreamEnv->stream_info.captureInfo.codec_info.pcm_size;

    if (initReq.tx_algo_cfg.channel_num != 0) {
        uint32_t algo_data_bytes = initReq.tx_algo_cfg.bits <= 16 ? 2 : 4;
        initReq.pcm_size = initReq.tx_algo_cfg.frame_len * \
            initReq.tx_algo_cfg.channel_num * algo_data_bytes;
    }

    initReq.algoProcess = (!pStreamEnv->stream_info.is_mobile && M55_CORE == core_type);
    initReq.isBis = pStreamEnv->stream_info.is_bis;
    initReq.ptr = initReq.isBis ? (void*)bisChInfo: (void*)aseInfo;
    initReq.ptr2 = pStreamEnv;
    gaf_cc_capture_feed_init_req(&initReq, core_type);
}

static void gaf_capture_encoder_init_rsp(void *_initRsp)
{
    CC_CAPTURE_INIT_RSP_T *initRsp = (CC_CAPTURE_INIT_RSP_T*)_initRsp;
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)initRsp->ptr2;
    if (initRsp->isBis) {
        GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T *pInfo = (GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T*)initRsp->ptr;
        pInfo->codec_core_info.instance_handle = initRsp->instance_handle;
        pInfo->codec_core_info.instance_status = INSTANCE_INITIALIZED;
    }
    else {
        GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T *pInfo = (GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T*)initRsp->ptr;
        pInfo->codec_core_info.instance_handle = initRsp->instance_handle;
        pInfo->codec_core_info.instance_status = INSTANCE_INITIALIZED;
    }
    pStreamEnv->stream_context.capture_frame_fifo[initRsp->instance_handle] = initRsp->frame_fifo;
    pStreamEnv->stream_context.capture_pcm_fifo[initRsp->instance_handle] = initRsp->pcm_fifo;
    pStreamEnv->stream_context.isEncoding[initRsp->instance_handle] = initRsp->isEncoding;
    LOG_I("%s instance_handle:%d", __func__, initRsp->instance_handle);
}

void gaf_capture_encoder_deinit(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, bool isRetrigger)
{
    CC_CAPTURE_DEINIT_REQ_T deinitReq;
    uint32_t core_type;
    deinitReq.isRetrigger = isRetrigger;
    deinitReq.isBis = pStreamEnv->stream_info.is_bis;
    if (pStreamEnv->stream_info.is_bis) {
        deinitReq.instance_handle = pStreamEnv->stream_info.captureInfo.bisChInfo[0].codec_core_info.instance_handle;
        core_type = pStreamEnv->stream_info.captureInfo.bisChInfo[0].codec_core_info.core_type;
        pStreamEnv->stream_info.captureInfo.bisChInfo[0].codec_core_info.instance_status = INSTANCE_IDLE;
    }
    else {
        deinitReq.instance_handle = pStreamEnv->stream_info.captureInfo.aseChInfo[0].codec_core_info.instance_handle;
        core_type = pStreamEnv->stream_info.captureInfo.aseChInfo[0].codec_core_info.core_type;
        pStreamEnv->stream_info.captureInfo.aseChInfo[0].codec_core_info.instance_status = INSTANCE_IDLE;
    }

    gaf_cc_capture_feed_deinit_req(&deinitReq, core_type);
}

const MEDIA_STREAM_CC_FUNC_LIST cc_func_list =
{
    .decoder_core_init_rsp = gaf_playback_decoder_init_rsp,
    .encoder_core_init_rsp = gaf_capture_encoder_init_rsp,
};

void gaf_stream_common_register_cc_func_list(void)
{
    gaf_cc_media_stream_func_register((void*)&cc_func_list);
}

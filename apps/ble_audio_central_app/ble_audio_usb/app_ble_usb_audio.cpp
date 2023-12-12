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
#ifdef AOB_MOBILE_ENABLED
#ifdef BLE_USB_AUDIO_SUPPORT
#ifdef BLE_USB_AUDIO_IS_DONGLE_ROLE
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "app_trace_rx.h"
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "plat_types.h"
#include "audioflinger.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "app_audio.h"
#include "app_ble_usb_audio.h"
#include "app_ble_audio_central_stream_stm.h"
#include "usb_audio_app.h"
#include "gaf_mobile_media_stream.h"
#include "aob_media_api.h"
#include "aob_mgr_gaf_evt.h"
#include "ble_audio_dbg.h"
#include "gaf_media_sync.h"
#include "gaf_media_pid.h"
#include "lc3_process.h"
#include "gaf_codec_lc3.h"
#include "gaf_media_common.h"
#include "ble_audio_ase_stm.h"
#include "app_bt_sync.h"
#include "rwble_config.h"
#include "audio_dump.h"
#include "app_ble_audio_central_pairing_stm.h"
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
#include "app_usb_hw_timer.h"
#endif

//TODO: need a better way to replace USB_CAPTURE_START_DEBOUNCE_TIME_MS
#define USB_CAPTURE_START_DEBOUNCE_TIME_MS          1000
#define BLE_USB_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS  3000
#define BLE_USE_STREAM_CONTROL_RETRY_TIMES  5

#define USB_BLE_AUDIO_PLAYBACK_TRIGGER_CHANNEL  0
#define USB_BLE_AUDIO_CAPTURE_TRIGGER_CHANNEL   1

#define USB_REC_AUDIO_DUMP   0

extern GAF_AUDIO_STREAM_ENV_T gaf_mobile_audio_stream_env;

static void app_ble_usb_stream_playback_start_supervisor_timer(void);
static void app_ble_usb_stream_capture_start_supervisor_timer(void);

static void gaf_mobile_usb_start_debounce_handler(void const *param);
static void ble_use_stream_playback_control_supervisor_timer_cb(void const *n);
static void ble_use_stream_capture_control_supervisor_timer_cb(void const *n);

static osTimerId gaf_mobile_usb_start_debounce_timer = NULL;
static osTimerId ble_use_stream_capture_control_supervisor_timer_id = NULL;
static osTimerId ble_use_stream_playback_control_supervisor_timer_id = NULL;

static uint8_t ble_usb_stream_capture_control_try_times_till_now = 0;
static uint8_t ble_usb_stream_playback_control_try_times_till_now = 0;
static bool app_ble_is_ready_to_process_first_usb_data = false;
static bool app_ble_is_usb_playback_on = false;
static bool app_ble_is_usb_capture_on = false;

osTimerDef (GAF_USB_CAPTURE_START_DEBOUNCE, gaf_mobile_usb_start_debounce_handler);
osTimerDef(BLE_USB_STREAM_CAPTURE_CONTROL_SUPERVISOR_TIMER, ble_use_stream_capture_control_supervisor_timer_cb);
osTimerDef(BLE_USB_STREAM_PLAYBACK_CONTROL_SUPERVISOR_TIMER, ble_use_stream_playback_control_supervisor_timer_cb);

static void usb_ble_audio_update_timing_test_handler(void)
{
    CIS_TIMING_CONFIGURATION_T updatedCisTimig;
    updatedCisTimig.m2s_bn = 1,
    updatedCisTimig.m2s_nse = 2;
    updatedCisTimig.s2m_bn = 1,
    updatedCisTimig.s2m_nse = 2,
    updatedCisTimig.frame_cnt_per_sdu = 1,
#ifdef BLE_USB_AUDIO_SUPPORT
    updatedCisTimig.m2s_ft = 2,
    updatedCisTimig.s2m_ft = 2,
#else
    updatedCisTimig.m2s_ft = 3,
    updatedCisTimig.s2m_ft = 3,
#endif

#ifdef AOB_LOW_LATENCY_MODE
    updatedCisTimig.iso_interval = 4,  // 5ms = 4 * 1.25ms
#else
#ifdef BLE_AUDIO_FRAME_DUR_7_5MS
    updatedCisTimig.iso_interval = 6,  // 7.5ms = 6 * 1.25ms
#else
    updatedCisTimig.iso_interval = 8,  // 10ms = 8 * 1.25ms
#endif
#endif
    TRACE(0, "Update USB AUDIO CIS timing:");
    TRACE(0, "m2s_bn %d", updatedCisTimig.m2s_bn);
    TRACE(0, "m2s_nse %d", updatedCisTimig.m2s_nse);
    TRACE(0, "m2s_ft %d", updatedCisTimig.m2s_ft);
    TRACE(0, "s2m_bn %d", updatedCisTimig.s2m_bn);
    TRACE(0, "s2m_nse %d", updatedCisTimig.s2m_nse);
    TRACE(0, "s2m_ft %d", updatedCisTimig.s2m_ft);
    TRACE(0, "frame_cnt_per_sdu %d", updatedCisTimig.frame_cnt_per_sdu);
    TRACE(0, "iso_interval %d", updatedCisTimig.iso_interval);

    app_bap_update_cis_timing(&updatedCisTimig);
}

void app_ble_usb_media_ascc_release_stream_test(void)
{
    TRACE(1,"%s",__func__);
    aob_media_mobile_release_stream(0);
    aob_media_mobile_release_stream(1);
}

void app_ble_usb_media_ascc_start_stream_test(void)
{
    TRACE(1,"%s",__func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  BES_BLE_GAF_CONTEXT_TYPE_MEDIA_BIT
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, 1);
    }
}

uint32_t app_ble_usb_media_get_cur_ase_state(void)
{
    uint8_t ase_lid = aob_media_mobile_get_cur_streaming_ase_lid(0, AOB_MGR_DIRECTION_SINK);
    return aob_media_mobile_get_cur_ase_state(ase_lid);
}

static void gaf_media_usb_prepare_playback_trigger(uint8_t trigger_channel)
{
    LOG_D("%s start trigger_channel %d", __func__, trigger_channel);

    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_enable_dma_tc(adma_ch&0xFF, dma_base);
    }
    else
    {
        LOG_I("error adma_ch HAL_DMA_CHAN_NONE %d", adma_ch);
    }
    btdrv_syn_clr_trigger(trigger_channel);
    af_codec_set_device_bt_sync_source(AUD_STREAM_USE_INT_CODEC,AUD_STREAM_PLAYBACK, trigger_channel);
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);

    LOG_D("%s end", __func__);
}

static void gaf_media_usb_prepare_capture_trigger(uint8_t trigger_channel)
{
    LOG_D("%s start", __func__);
    uint8_t adma_ch = 0;
    uint32_t dma_base;
    btdrv_syn_clr_trigger(trigger_channel);

    af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_enable_dma_tc(adma_ch&0xFF, dma_base);
    }

    af_codec_set_device_bt_sync_source(AUD_STREAM_USE_INT_CODEC,AUD_STREAM_CAPTURE, trigger_channel);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);

    LOG_D("%s end", __func__);
}

static void* gaf_mobile_audio_get_media_stream_env(void)
{
    return &gaf_mobile_audio_stream_env;
}

static uint32_t gaf_usb_calculate_capture_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
    uint32_t curr_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    uint32_t trigger_bt_time = latest_iso_bt_time + GAF_AUDIO_CAPTURE_TRIGGER_DELAY_IN_US;

    while ((int32_t)(trigger_bt_time - curr_bt_time) < 0)
    {
        trigger_bt_time += pStreamEnv->stream_info.captureInfo.isoIntervalUs;
    }

    LOG_I("usb capture iso anch %u cur time %u trigger time %u",
        latest_iso_bt_time, curr_bt_time, trigger_bt_time);

    return trigger_bt_time;
}

/**
 ****************************************************************************************
 * @brief Called when cis establishment
 *
 * @return void
 ****************************************************************************************
 */
void gap_mobile_start_usb_audio_receiving_dma(void)
{
#ifndef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
        (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv == NULL)
    {
         LOG_E("%s, pStreamEnv is NULL", __func__);
         return;
    }

    LOG_I("%s", __func__);

    uint32_t trigger_bt_time = gaf_usb_calculate_capture_trigger_time(pStreamEnv);
    gaf_stream_common_set_capture_trigger_time_generic(pStreamEnv, AUD_STREAM_PLAYBACK, trigger_bt_time);
#endif
}

void gap_mobile_start_usb_audio_transmission_dma(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
        (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv == NULL)
    {
         LOG_E("%s, pStreamEnv is NULL", __func__);
         return;
    }
    LOG_I("%s", __func__);

    bes_ble_bap_dp_itf_data_come_callback_register((void *)gaf_mobile_audio_receive_data);
}

static void gaf_usb_mobile_audio_process_pcm_data_send(void *pStreamEnv_,void *payload_,
    uint32_t payload_size, uint32_t ref_time)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T* )pStreamEnv_;
    uint8_t *payload = (uint8_t *)payload_;

    uint32_t payload_len_per_channel = 0;
    uint32_t channel_shift = 0;
    uint32_t audio_allocation_bf = 0;
    uint8_t audio_allocation_cnt = 0;
    bool stereo_channel_support = false;
    bool is_right_channel = false;
    /// @see pCommonInfo->num_channels in SINK
    ASSERT(pStreamEnv->stream_info.captureInfo.codec_info.num_channels == AUD_CHANNEL_NUM_2, "need stereo channel here");
    // Check all ase for streaming send
    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl == GAF_AUDIO_INVALID_ISO_CHANNEL)
        {
            continue;
        }
        channel_shift = 0;
        stereo_channel_support = app_bap_capa_cli_is_peer_support_stereo_channel(
                    pStreamEnv->stream_info.con_lid, APP_GAF_DIRECTION_SINK);
        audio_allocation_bf = pStreamEnv->stream_info.captureInfo.aseChInfo[i].allocation_bf;
        is_right_channel = ((audio_allocation_bf & (BES_BLE_LOC_SIDE_RIGHT | BES_BLE_LOC_FRONT_RIGHT)) != 0);
        audio_allocation_cnt = app_bap_get_audio_location_l_r_cnt(audio_allocation_bf);

        // Config palyload len  for AUD_CHANNEL_NUM_2
        payload_len_per_channel = payload_size / AUD_CHANNEL_NUM_2;
        // Most of time, pStreamEnv->stream_info.captureInfo.num_channels == AUD_CHANNEL_NUM_2
        if (!stereo_channel_support)
        {
            // Only one channel in this ASE, choose one pcm channel
            if (is_right_channel)
            {
                // shift to right channel
                channel_shift += payload_len_per_channel;
            }
        }
        else// if STEREO CHANNNEL
        {
            //Check is there use two ASE
            if (audio_allocation_cnt == AUD_CHANNEL_NUM_1 && is_right_channel)
            {
                // shift to right channel
                channel_shift += payload_len_per_channel;
            }
            else if (audio_allocation_cnt == AUD_CHANNEL_NUM_2)
            {
                payload_len_per_channel = payload_size;
            }
        }
        LOG_D("[CAPTURE SEND] p_len:%d stereo supp: %d, allocation_bf: 0x%x, shift :%d",
              payload_len_per_channel, stereo_channel_support, audio_allocation_bf, channel_shift);
        bes_ble_bap_iso_dp_send_data(pStreamEnv->stream_info.captureInfo.aseChInfo[i].ase_handle,
                                pStreamEnv->stream_context.latestCaptureSeqNum,
                                payload + channel_shift, payload_len_per_channel,
                                ref_time);
    }
}

static void gaf_usb_mobile_audio_process_pcm_data(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t *ptrBuf, uint32_t length)
{
    uint32_t dmaIrqHappeningTimeUs = 0;
    GAF_AUDIO_STREAM_CONTEXT_T *stream_context = &pStreamEnv->stream_context;
    GAF_AUDIO_STREAM_COMMON_INFO_T *captureInfo = &pStreamEnv->stream_info.captureInfo;
    POSSIBLY_UNUSED CODEC_CORE_INFO_T *coreInfo =
        &pStreamEnv->stream_info.captureInfo.aseChInfo[0].codec_core_info;
    uint8_t *output_buf = stream_context->capture_frame_cache;
    uint32_t frame_len = (uint32_t)(captureInfo->codec_info.frame_size *
                        captureInfo->codec_info.num_channels);

    if (coreInfo->instance_status < INSTANCE_INITIALIZED) {
        return;
    }
    if (!app_ble_is_usb_playback_on) {
        memset(ptrBuf, 0, length);
    }
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    int32_t curr_bt_time = (int32_t)bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    int32_t latest_anch_time = (int32_t)gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);

    latest_anch_time -= (((int32_t)(latest_anch_time - curr_bt_time)) /
        (int32_t)captureInfo->isoIntervalUs) * (int32_t)captureInfo->isoIntervalUs;

    dmaIrqHappeningTimeUs = latest_anch_time - captureInfo->isoIntervalUs;
#else
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal microsecond -- 0.5 us
    uint8_t adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    uint32_t dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }
#endif

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((GAF_CAPTURE_STREAM_STREAMING_TRIGGERED ==stream_context->capture_stream_state) &&
        (dmaIrqHappeningTimeUs == stream_context->lastCaptureDmaIrqTimeUs))
    {
        LOG_W("accumulated irq messages happen!");
        return;
    }
    if (GAF_CAPTURE_STREAM_START_TRIGGERING == stream_context->capture_stream_state)
    {
        uint32_t expectedDmaIrqHappeningTimeUs =
            stream_context->lastCaptureDmaIrqTimeUs +
            (uint32_t)stream_context->captureAverageDmaChunkIntervalUs;

        int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, expectedDmaIrqHappeningTimeUs);

        int32_t gapUs_abs = GAF_AUDIO_ABS(gapUs);

        if ((gapUs > 0) && (gapUs_abs > (int32_t)captureInfo->dma_info.dmaChunkIntervalUs/2)) {
            LOG_I("%s, gapUs = %d, dmaChunkIntervalUs = %d", __func__, gapUs, captureInfo->dma_info.dmaChunkIntervalUs);
            return;
        }
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_STREAMING_TRIGGERED);
        gaf_stream_common_clr_trigger(stream_context->playbackTriggerChannel);
        uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
        stream_context->usSinceLatestAnchorPoint = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, latest_iso_bt_time);
    }

    gaf_stream_common_playback_timestamp_checker(pStreamEnv, dmaIrqHappeningTimeUs);

    LOG_D("length %d encoded_len %d filled timestamp %u", length,
        captureInfo->codec_info.frame_size,dmaIrqHappeningTimeUs);

#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
    if (stream_context->isUpStreamingStarted)
    {
        // bth fetch encoded data
        gaf_stream_common_fetch_frame(pStreamEnv, output_buf,
            frame_len, dmaIrqHappeningTimeUs, coreInfo->instance_handle);
        // bth send out encoded data
        gaf_usb_mobile_audio_process_pcm_data_send(pStreamEnv,
            output_buf, frame_len, dmaIrqHappeningTimeUs);
    }
    gaf_stream_common_store_pcm(pStreamEnv,
        dmaIrqHappeningTimeUs, ptrBuf, length, frame_len, coreInfo);
#else
    pStreamEnv->func_list->encoder_func_list->encoder_encode_frame_func(0,
        &(captureInfo->codec_info), length, ptrBuf, frame_len, output_buf);

    gaf_usb_mobile_audio_process_pcm_data_send(pStreamEnv, output_buf, frame_len, dmaIrqHappeningTimeUs);
#endif
}

static bool gaf_mobile_usb_is_any_capture_stream_iso_created(GAF_AUDIO_STREAM_ENV_T *pStreamEnv)
{
    GAF_AUDIO_STREAM_COMMON_INFO_T* captureInfo = &(pStreamEnv->stream_info.captureInfo);

    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL != captureInfo->aseChInfo[i].iso_channel_hdl)
        {
            return true;
        }
    }

    return false;
}

static bool gaf_mobile_usb_is_processing_first_packet(void)
{
    if (app_ble_is_ready_to_process_first_usb_data)
    {
        GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

        if (pStreamEnv && (pStreamEnv->stream_context.capture_stream_state > GAF_CAPTURE_STREAM_INITIALIZED))
        {
            bool isAnyCaptureIsoCreated = gaf_mobile_usb_is_any_capture_stream_iso_created(pStreamEnv);
            if (isAnyCaptureIsoCreated)
            {
                app_ble_is_ready_to_process_first_usb_data = false;
                return true;
            }
        }
    }

    return false;
}

static uint32_t  gaf_mobile_usb_processing_received_data(uint8_t* ptrBuf, uint32_t length)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv && (pStreamEnv->stream_context.capture_stream_state > GAF_CAPTURE_STREAM_INITIALIZED))
    {
        bool isAnyCaptureIsoCreated = gaf_mobile_usb_is_any_capture_stream_iso_created(pStreamEnv);
        if (isAnyCaptureIsoCreated)
        {
            gaf_usb_mobile_audio_process_pcm_data(pStreamEnv, ptrBuf, length);
        }
    }
    return length;
}

static void gaf_usb_mobile_audio_process_encoded_data(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t *ptrBuf, uint32_t length)
{
    LOG_D("%s start, len = %d", __func__,length);
    uint32_t dmaIrqHappeningTimeUs = 0;
    GAF_AUDIO_STREAM_COMMON_INFO_T *playbackInfo = &pStreamEnv->stream_info.playbackInfo;

#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    int32_t curr_bt_time = (int32_t)bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    int32_t latest_anch_time = (int32_t)gaf_media_common_get_latest_rx_iso_evt_timestamp(pStreamEnv);

    latest_anch_time -= (((int32_t)(latest_anch_time - curr_bt_time)) /
        (int32_t)playbackInfo->isoIntervalUs) * (int32_t)playbackInfo->isoIntervalUs;

    dmaIrqHappeningTimeUs = latest_anch_time - playbackInfo->isoIntervalUs;
#else
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;

    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }
#endif

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
   if ((pStreamEnv->stream_context.playback_stream_state < GAF_PLAYBACK_STREAM_START_TRIGGERING) ||
        ((GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED ==
       pStreamEnv->stream_context.playback_stream_state) &&
       (dmaIrqHappeningTimeUs ==
       pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs)))
   {
        memset(ptrBuf, 0, length);
        return;
   }

    gaf_stream_common_updated_expeceted_playback_seq_and_time(pStreamEnv, GAF_AUDIO_DFT_PLAYBACK_LIST_IDX, dmaIrqHappeningTimeUs);

    if (GAF_PLAYBACK_STREAM_START_TRIGGERING ==
        pStreamEnv->stream_context.playback_stream_state)
    {
        gaf_stream_common_update_playback_stream_state(pStreamEnv,
            GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED);
        gaf_stream_common_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);
        pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]--;
        LOG_I("Update playback seq to 0x%x", pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]);
    }

    int ret = 0;
    uint8_t instance_handle = 0;
    POSSIBLY_UNUSED uint32_t instance_status = 0;
    uint8_t connected_iso_num = 0;
    uint32_t sample_bytes = pStreamEnv->stream_info.playbackInfo.dma_info.bits_depth <= 16 ? 2 : 4;
    uint32_t sample_cnt = length / sample_bytes;
    uint8_t *pcm_cache = pStreamEnv->stream_context.playback_pcm_cache;

    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (playbackInfo->aseChInfo[i].iso_channel_hdl == GAF_AUDIO_INVALID_ISO_CHANNEL)
        {
            continue;
        }

        instance_handle = playbackInfo->aseChInfo[i].codec_core_info.instance_handle;
        instance_status = playbackInfo->aseChInfo[i].codec_core_info.instance_status;
#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
        if (instance_status < INSTANCE_INITIALIZED)
        {
            continue;
        }
        ret = !gaf_stream_common_fetch_pcm(pStreamEnv,
            pcm_cache, playbackInfo->codec_info.pcm_size, instance_handle,
            dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, 0 == connected_iso_num);
#else
        instance_handle = connected_iso_num;
        CC_PLAYBACK_DATA_T *out_frame = (CC_PLAYBACK_DATA_T*)pStreamEnv->stream_context.playback_frame_cache;
        out_frame->data = pStreamEnv->stream_context.playback_frame_cache + sizeof(CC_PLAYBACK_DATA_T);

        /* use one iso data to do pid */
        gaf_stream_common_get_packet(pStreamEnv, out_frame, instance_handle,
            dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, 0 == connected_iso_num);

        ret = pStreamEnv->func_list->decoder_func_list->decoder_decode_frame_func
                  (instance_handle, &(playbackInfo->codec_info), out_frame->data_len,
                  out_frame->data, pcm_cache, out_frame->isPLC);

        LOG_D("(%d) dec ret:%d frame_len:%d %d length %d", i, ret,
            out_frame->data_len, playbackInfo->aseChInfo[i].iso_channel_hdl, length);
#endif
        if (ret) {
            LOG_E("ret:%d", ret);
            memset(pcm_cache, 0, length);
        }

        connected_iso_num++;
        if (connected_iso_num >= PLAYBACK_INSTANCE_MAX)
        {
            break;
        }
        pcm_cache += playbackInfo->codec_info.pcm_size;
    }

    pcm_cache = pStreamEnv->stream_context.playback_pcm_cache;
#if USB_REC_AUDIO_DUMP
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, pcm_cache, sample_cnt);
    audio_dump_run();
#endif

    /* Merge stream data from multiple devices iso data */
    if (4 == sample_bytes)
    {
        int32_t pcm_data = 0;
        int32_t *buf = (int32_t*)ptrBuf;
        int32_t *cache = (int32_t*)pcm_cache;
        for (uint32_t samples = 0; samples < sample_cnt; samples++)
        {
            pcm_data = 0;
            for (uint32_t index = 0; index < connected_iso_num; index++)
            {
                pcm_data += (cache + sample_cnt * index)[samples];
            }
            buf[samples] = (int32_t)(pcm_data / MAX(connected_iso_num, 1));
        }
    }
    else
    {
        int32_t pcm_data = 0;
        int16_t *buf = (int16_t*)ptrBuf;
        int16_t *cache = (int16_t*)pcm_cache;
        for (uint32_t samples = 0; samples < sample_cnt; samples++)
        {
            pcm_data = 0;
            for (uint32_t index = 0; index < connected_iso_num; index++)
            {
                pcm_data += (cache + sample_cnt * index)[samples];
            }
            buf[samples] = (int16_t)(pcm_data / MAX(connected_iso_num, 1));
        }
    }
}

static void gaf_mobile_usb_feed_data(uint8_t* ptrBuf, uint32_t length)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();
    if (pStreamEnv && (pStreamEnv->stream_context.playback_stream_state > GAF_PLAYBACK_STREAM_INITIALIZED)) {
        gaf_usb_mobile_audio_process_encoded_data(pStreamEnv, ptrBuf, length);
    }
}

void gaf_mobile_usb_dma_playback_stop(void)
{
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv) {
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        if (adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
        }

        btdrv_syn_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
    }
}


void gaf_mobile_usb_dma_capture_stop(void)
{
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
        (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    af_stream_dma_tc_irq_disable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
    }

    btdrv_syn_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);
}

static void gaf_mobile_usb_set_sysfreq(void)
{
    if (app_ble_is_usb_capture_on && app_ble_is_usb_playback_on)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_USB_AUDIO_STREAMING, APP_SYSFREQ_26M);
    }
    else if (app_ble_is_usb_capture_on && !app_ble_is_usb_playback_on)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_USB_AUDIO_STREAMING, APP_SYSFREQ_26M);
    }
    else if (!app_ble_is_usb_capture_on && app_ble_is_usb_playback_on)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_USB_AUDIO_STREAMING, APP_SYSFREQ_26M);
    }
    else
    {
        app_sysfreq_req(APP_SYSFREQ_USER_USB_AUDIO_STREAMING, APP_SYSFREQ_32K);
    }
}

static void gaf_mobile_usb_playback_vol_changed(uint32_t level)
{
    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        uint8_t ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, APP_GAF_DIRECTION_SINK);
        if (ase_lid != 0xFF)
        {
            bes_ble_arc_mobile_set_abs_vol(i, level);
        }
    }
}

static void gaf_mobile_usb_start_debounce_handler(void const *param)
{
    if (!app_ble_is_usb_capture_on) {
        app_ble_usb_stream_capture_start_supervisor_timer();
        app_ble_is_usb_capture_on = true;
        gaf_mobile_usb_set_sysfreq();
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START, 0, 0);
    }
}

static void gaf_mobile_usb_playback_start(void)
{
    if (!app_ble_is_usb_playback_on)
    {
        app_ble_is_ready_to_process_first_usb_data = true;
        app_ble_usb_stream_playback_start_supervisor_timer();
        app_ble_is_usb_playback_on = true;
        gaf_mobile_usb_set_sysfreq();
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START, 0, 0);
    }
}

void gaf_mobile_usb_playback_stop(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
        (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (app_ble_is_usb_playback_on)
    {
        app_ble_usb_stream_playback_start_supervisor_timer();
        app_ble_is_usb_playback_on = false;
        gaf_mobile_usb_set_sysfreq();
#ifndef USB_BLE_AUDIO_HW_TIMER_TRIGGER
        gaf_mobile_usb_dma_playback_stop();
#endif
        if (pStreamEnv->stream_context.capture_stream_state > GAF_CAPTURE_STREAM_INITIALIZED) {
            gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);
        }
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP, 0, 0);
    }
}

static void gaf_mobile_usb_capture_start(void)
{
    LOG_D("%s", __func__);
    osTimerStart(gaf_mobile_usb_start_debounce_timer, USB_CAPTURE_START_DEBOUNCE_TIME_MS);
}

static void gaf_mobile_usb_capture_stop(void)
{
    LOG_D("%s", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();
    osTimerStop(gaf_mobile_usb_start_debounce_timer);
    if (app_ble_is_usb_capture_on) {
        app_ble_is_usb_capture_on = false;
        app_ble_usb_stream_capture_start_supervisor_timer();
        gaf_mobile_usb_set_sysfreq();
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
        gaf_usb_hw_timer_stop(AUD_STREAM_CAPTURE);
#else
        gaf_mobile_usb_dma_capture_stop();
#endif
        bes_ble_bap_dp_itf_data_come_callback_deregister();
        if (pStreamEnv->stream_context.playback_stream_state > GAF_PLAYBACK_STREAM_INITIALIZED)
        {
            gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);
        }

        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP, 0, 0);
    }
}

POSSIBLY_UNUSED static void gaf_mobile_usb_prepare_handler(enum AUD_STREAM_T stream)
{
    if (AUD_STREAM_PLAYBACK == stream)
    {
        gaf_media_usb_prepare_playback_trigger(USB_BLE_AUDIO_PLAYBACK_TRIGGER_CHANNEL);
    }
    else
    {
        gaf_media_usb_prepare_capture_trigger(USB_BLE_AUDIO_CAPTURE_TRIGGER_CHANNEL);
    }
}

static bool gaf_mobile_usb_reset_codec_feasibility_check_handler(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();
    if (pStreamEnv)
    {
        TRACE(0, "cp state %d pb state %d", pStreamEnv->stream_context.capture_stream_state,
            pStreamEnv->stream_context.playback_stream_state);
    }

    // ble audio stream capture pcm data from usb af playback stream
    if (pStreamEnv &&
        ((GAF_CAPTURE_STREAM_INITIALIZED == pStreamEnv->stream_context.capture_stream_state) ||
        (GAF_CAPTURE_STREAM_START_TRIGGERING == pStreamEnv->stream_context.capture_stream_state)))
    {
        return false;
    }

    // ble audio stream play decoded pcm data into usb af capture  stream
    if (pStreamEnv &&
        ((GAF_PLAYBACK_STREAM_INITIALIZED == pStreamEnv->stream_context.playback_stream_state) ||
        (GAF_PLAYBACK_STREAM_START_TRIGGERING == pStreamEnv->stream_context.playback_stream_state)))
    {
        return false;
    }

    return true;
}

static void gaf_mobile_usb_cmd_received(enum AUDIO_CMD_T cmd)
{
#ifdef BLE_AUDIO_CENTRAL_SELF_PAIRING_FEATURE_ENABLED
    // when receive the first command, start self pairing state machine
    // started flag is checked inside the function to avoid duplicated starting
    ble_audio_central_start_pairing_state_machine();
#endif
}

static const USB_AUDIO_SOURCE_EVENT_CALLBACK_T ble_usb_audio_stream_func_cb_list =
{
    .check_first_processed_usb_data_cb = gaf_mobile_usb_is_processing_first_packet,
    .data_playback_cb = gaf_mobile_usb_processing_received_data,
    .data_capture_cb = gaf_mobile_usb_feed_data,
    .reset_codec_feasibility_check_cb = gaf_mobile_usb_reset_codec_feasibility_check_handler,
    .data_prep_cb = gaf_mobile_usb_prepare_handler,
    .playback_start_cb = gaf_mobile_usb_playback_start,
    .playback_stop_cb = gaf_mobile_usb_playback_stop,

    .capture_start_cb = gaf_mobile_usb_capture_start,
    .capture_stop_cb = gaf_mobile_usb_capture_stop,

    .playback_vol_change_cb = gaf_mobile_usb_playback_vol_changed,
    .cmd_received_cb = gaf_mobile_usb_cmd_received,
};

void app_ble_usb_stream_stm_post_op_checker(BLE_AUDIO_CENTRAL_STREAM_EVENT_E event)
{
    switch (event)
    {
        case EVT_PLAYBACK_STREAM_STARTED:
        {
            if (!app_ble_is_usb_playback_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP, 0, 0);
            }
            break;
        }
        case EVT_CAPTURE_STREAM_STARTED:
        {
            if (!app_ble_is_usb_capture_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP, 0, 0);
            }
            break;
        }
        case EVT_PLAYBACK_STREAM_STOPPED:
        {
            if (app_ble_is_usb_playback_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START, 0, 0);
            }
            break;
        }
        case EVT_CAPTURE_STREAM_STOPPED:
        {
            if (app_ble_is_usb_capture_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START, 0, 0);
            }
            break;
        }
        default:
            break;
    }
}

static void ble_use_stream_playback_control_supervisor_timer_cb(void const *n)
{
    if (ble_usb_stream_playback_control_try_times_till_now < BLE_USE_STREAM_CONTROL_RETRY_TIMES)
    {
        if (app_ble_is_usb_playback_on)
        {
            TRACE(0, "Starting ble usb playback stream timeout!Retry time count %d.", ble_usb_stream_playback_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START, 0, 0);
        }
        else
        {
            TRACE(0, "Stopping ble usb playback stream timeout!Retry time count %d.", ble_usb_stream_playback_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP, 0, 0);
        }

        ble_usb_stream_playback_control_try_times_till_now++;
        osTimerStart(ble_use_stream_playback_control_supervisor_timer_id, BLE_USB_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
    }
}

static void app_ble_usb_stream_playback_start_supervisor_timer(void)
{
    if (NULL == ble_use_stream_playback_control_supervisor_timer_id)
    {
        ble_use_stream_playback_control_supervisor_timer_id =
            osTimerCreate(osTimer(BLE_USB_STREAM_PLAYBACK_CONTROL_SUPERVISOR_TIMER), osTimerOnce, NULL);
    }

    ble_usb_stream_playback_control_try_times_till_now = 0;
    osTimerStart(ble_use_stream_playback_control_supervisor_timer_id, BLE_USB_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
}

static void app_ble_usb_stream_playback_stop_supervisor_timer(void)
{
    osTimerStop(ble_use_stream_playback_control_supervisor_timer_id);
}

static void ble_use_stream_capture_control_supervisor_timer_cb(void const *n)
{
    if (ble_usb_stream_capture_control_try_times_till_now < BLE_USE_STREAM_CONTROL_RETRY_TIMES)
    {
        if (app_ble_is_usb_capture_on)
        {
            TRACE(0, "Starting ble usb capture stream timeout!Retry time count %d.", ble_usb_stream_capture_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START, 0, 0);
        }
        else
        {
            TRACE(0, "Stopping ble usb capture stream timeout!Retry time count %d.", ble_usb_stream_capture_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP, 0, 0);
        }

        ble_usb_stream_capture_control_try_times_till_now++;
        osTimerStart(ble_use_stream_capture_control_supervisor_timer_id, BLE_USB_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
    }
}

static void app_ble_usb_stream_capture_start_supervisor_timer(void)
{
    if (NULL == ble_use_stream_capture_control_supervisor_timer_id)
    {
        ble_use_stream_capture_control_supervisor_timer_id =
            osTimerCreate(osTimer(BLE_USB_STREAM_CAPTURE_CONTROL_SUPERVISOR_TIMER), osTimerOnce, NULL);
    }

    ble_usb_stream_capture_control_try_times_till_now = 0;
    osTimerStart(ble_use_stream_capture_control_supervisor_timer_id, BLE_USB_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
}

static void app_ble_usb_stream_capture_stop_supervisor_timer(void)
{
    osTimerStop(ble_use_stream_capture_control_supervisor_timer_id);
}

void app_ble_usb_stream_inform_playback_state(uint32_t event)
{
    if (app_ble_is_usb_playback_on && (EVT_PLAYBACK_STREAM_STARTED == event))
    {
        app_ble_usb_stream_playback_stop_supervisor_timer();
    }
    else if ((!app_ble_is_usb_playback_on) && (EVT_PLAYBACK_STREAM_STOPPED == event))
    {
        app_ble_usb_stream_playback_stop_supervisor_timer();
    }

    ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), (BLE_AUDIO_CENTRAL_STREAM_EVENT_E)event, 0, 0);
}

void app_ble_usb_stream_inform_capture_state(uint32_t event)
{
    if (app_ble_is_usb_capture_on && (EVT_CAPTURE_STREAM_STARTED == event))
    {
        app_ble_usb_stream_capture_stop_supervisor_timer();
    }
    else if ((!app_ble_is_usb_capture_on) && (EVT_CAPTURE_STREAM_STOPPED == event))
    {
        app_ble_usb_stream_capture_stop_supervisor_timer();
    }

    ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), (BLE_AUDIO_CENTRAL_STREAM_EVENT_E)event, 0, 0);
}

static void app_ble_usb_stream_stm_post_cis_disconn_op_checker(void)
{
    if (ble_use_stream_capture_control_supervisor_timer_id)
    {
        if (osTimerIsRunning(ble_use_stream_capture_control_supervisor_timer_id))
        {
            ble_use_stream_capture_control_supervisor_timer_cb(NULL);
        }
    }

    if (ble_use_stream_playback_control_supervisor_timer_id)
    {
        if (osTimerIsRunning(ble_use_stream_playback_control_supervisor_timer_id))
        {
            ble_use_stream_playback_control_supervisor_timer_cb(NULL);
        }
    }
}

void app_ble_usb_audio_init(void)
{
    ble_audio_central_stream_stm_init();

    ble_audio_central_stream_start_receiving_handler_register(gap_mobile_start_usb_audio_receiving_dma);;
    ble_audio_central_stream_start_transmission_handler_register(gap_mobile_start_usb_audio_transmission_dma);
    ble_audio_central_stream_post_operation_check_cb_register(app_ble_usb_stream_stm_post_op_checker);
    ble_audio_central_stream_post_cis_discon_operation_check_cb_register(app_ble_usb_stream_stm_post_cis_disconn_op_checker);

    usb_ble_audio_update_timing_test_handler();
    usb_audio_source_config_init(&ble_usb_audio_stream_func_cb_list);

    ble_audio_central_stream_stm_t* usb_stream_stm_ptr = ble_audio_central_get_stream_sm();
    if (NULL != usb_stream_stm_ptr) {
        ble_audio_central_stream_stm_startup(usb_stream_stm_ptr);
    }

    if (gaf_mobile_usb_start_debounce_timer == NULL) {
        gaf_mobile_usb_start_debounce_timer =
            osTimerCreate (osTimer(GAF_USB_CAPTURE_START_DEBOUNCE), osTimerOnce, NULL);
    }
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    gaf_usb_hw_timer_init();
#endif
}

POSSIBLY_UNUSED static uint32_t gaf_usb_calculate_playback_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint32_t anch_time = gaf_media_common_get_latest_rx_iso_evt_timestamp(pStreamEnv);
    uint32_t curr_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    uint32_t trigger_time = anch_time;

    while ((int32_t)(trigger_time - curr_bt_time) < 1000)
    {
        trigger_time += pStreamEnv->stream_info.playbackInfo.isoIntervalUs;
    }

    LOG_I("usb playback iso anch %u cur time %u trigger time %u", anch_time, curr_bt_time, trigger_time);
    return trigger_time;
}

int gaf_mobile_usb_audio_media_stream_start_handler(void* _pStreamEnv)
{
    LOG_I("%s", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    if (GAF_CAPTURE_STREAM_INITIALIZING == pStreamEnv->stream_context.capture_stream_state)
    {
        pStreamEnv->func_list->stream_func_list.capture_init_stream_buf_func(pStreamEnv);
        if (pStreamEnv->func_list->encoder_func_list)
        {
            pStreamEnv->func_list->encoder_func_list->encoder_init_func(
                        0, &(pStreamEnv->stream_info.captureInfo.codec_info));
        }
#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
        gaf_capture_encoder_init(pStreamEnv);
#endif
        pStreamEnv->stream_context.captureTriggerChannel = USB_BLE_AUDIO_PLAYBACK_TRIGGER_CHANNEL;
        LOG_D("%s captureTriggerChannel %d", __func__,pStreamEnv->stream_context.captureTriggerChannel);

         gaf_media_pid_init(&(pStreamEnv->stream_context.playback_pid_env));
        // capture pcm data from usb audio
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);

#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
        uint32_t trigger_bt_time = gaf_usb_calculate_capture_trigger_time(pStreamEnv);
        gaf_stream_common_set_capture_trigger_info(pStreamEnv,
            trigger_bt_time - pStreamEnv->stream_info.captureInfo.isoIntervalUs);
        gaf_usb_capture_hw_timer_start(pStreamEnv, trigger_bt_time);
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_START_TRIGGERING);
#else
        af_set_priority(AF_USER_AI, osPriorityHigh);
#endif

        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_208M);
        app_ble_usb_stream_inform_playback_state(EVT_PLAYBACK_STREAM_STARTED);
        LOG_I("%s end", __func__);

        return 0;
    }

    return -1;
}

int gaf_mobile_usb_audio_media_stream_stop_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.capture_retrigger_onprocess;
    gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_IDLE);

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    gaf_capture_encoder_deinit(pStreamEnv, isRetrigger);
#endif
    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_deinit_func(0);
    }
    pStreamEnv->func_list->stream_func_list.capture_deinit_stream_buf_func(pStreamEnv);
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    gaf_usb_hw_timer_stop(AUD_STREAM_PLAYBACK);
#else
    app_bt_sync_release_trigger_channel(pStreamEnv->stream_context.captureTriggerChannel);
    af_set_priority(AF_USER_AI, osPriorityAboveNormal);
#endif

    app_ble_usb_stream_inform_playback_state(EVT_PLAYBACK_STREAM_STOPPED);
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);
    return 0;
}

int gaf_mobile_usb_audio_capture_start_handler(void* _pStreamEnv)
{
    LOG_I("%s", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    pStreamEnv->func_list->stream_func_list.playback_init_stream_buf_func(pStreamEnv);
    if (pStreamEnv->func_list->decoder_func_list)
    {
        for (uint32_t instance_handle = 0; instance_handle < PLAYBACK_INSTANCE_MAX; instance_handle++)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_init_func(
                instance_handle, &(pStreamEnv->stream_info.playbackInfo.codec_info));
        }
    }
    pStreamEnv->stream_context.playbackTriggerChannel = USB_BLE_AUDIO_CAPTURE_TRIGGER_CHANNEL;

    gaf_media_pid_init(&(pStreamEnv->stream_context.capture_pid_env));
    gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);

#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    uint32_t trigger_bt_time = gaf_usb_calculate_playback_trigger_time(pStreamEnv);
    gaf_usb_playback_hw_timer_start(pStreamEnv, trigger_bt_time);
#else
    af_set_priority(AF_USER_AI, osPriorityHigh);
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_208M);
    app_ble_usb_stream_inform_capture_state(EVT_CAPTURE_STREAM_STARTED);

#if defined(USB_REC_AUDIO_DUMP)
    audio_dump_init(480, sizeof(short), 2);
#endif
    return 0;
}

int gaf_mobile_usb_audio_capture_stop_handler(void* _pStreamEnv)
{
    LOG_I("%s", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_IDLE);

    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_deinit_func(0);
    }
    pStreamEnv->func_list->stream_func_list.playback_deinit_stream_buf_func(pStreamEnv);
#ifndef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    app_bt_sync_release_trigger_channel(pStreamEnv->stream_context.playbackTriggerChannel);
    af_set_priority(AF_USER_AI, osPriorityAboveNormal);
#endif
    app_ble_usb_stream_inform_capture_state(EVT_CAPTURE_STREAM_STOPPED);

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_32K);
    return 0;
}

bool gaf_mobile_usb_audio_check_capture_need_start(void)
{
    return usb_audio_check_capture_need_start();
}

#endif //BLE_USB_AUDIO_IS_DONGLE_ROLE
#endif //BLE_USB_AUDIO_SUPPORT
#endif //AOB_MOBILE_ENABLED

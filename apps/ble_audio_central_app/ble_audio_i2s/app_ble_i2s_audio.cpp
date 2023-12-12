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
#ifdef AOB_MOBILE_ENABLED
#ifdef BLE_I2S_AUDIO_SUPPORT
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
#include "app_ble_i2s_audio.h"
#include "app_ble_audio_central_stream_stm.h"
#include "gaf_mobile_media_stream.h"
#include "aob_media_api.h"
#include "aob_mgr_gaf_evt.h"
#include "ble_audio_dbg.h"
#include "gaf_media_sync.h"
#include "gaf_media_pid.h"
#include "lc3_process.h"
#include "gaf_codec_lc3.h"
#include "app_bap_data_path_itf.h"
#include "gaf_media_common.h"
#include "app_gaf_custom_api.h"
#include "ble_audio_ase_stm.h"
#include "app_bt_sync.h"
#include "rwble_config.h"
#include "aob_volume_api.h"
#include "audio_dump.h"
#include "app_ble_i2s_audio_stream.h"

#define I2S_BLE_AUDIO_PLAYBACK_TRIGGER_CHANNEL  0
#define I2S_BLE_AUDIO_CAPTURE_TRIGGER_CHANNEL   1

#define I2S_REC_AUDIO_DUMP   0
extern GAF_AUDIO_STREAM_CONTEXT_TYPE_E g_cur_context_type ;
extern GAF_AUDIO_STREAM_ENV_T gaf_mobile_audio_stream_env;

extern "C" uint32_t btdrv_reg_op_cig_anchor_timestamp(uint8_t link_id);

#ifdef BLE_USB_AUDIO_IS_DONGLE_ROLE

static void gaf_mobile_i2s_start_debounce_handler(void const *param);
osTimerDef (GAF_I2S_CAPTURE_START_DEBOUNCE, gaf_mobile_i2s_start_debounce_handler);
static osTimerId gaf_mobile_i2s_start_debounce_timer = NULL;

static void app_ble_i2s_stream_playback_start_supervisor_timer(void);
static void app_ble_i2s_stream_capture_start_supervisor_timer(void);

//TODO: need a better way to replace I2S_CAPTURE_START_DEBOUNCE_TIME_MS
#define I2S_CAPTURE_START_DEBOUNCE_TIME_MS          50

static void i2s_ble_audio_update_timing_test_handler(void)
{
    CIS_TIMING_CONFIGURATION_T updatedCisTimig;
    updatedCisTimig.m2s_bn = 1,
    updatedCisTimig.m2s_nse = 2;
    updatedCisTimig.s2m_bn = 1,
    updatedCisTimig.s2m_nse = 2,
    updatedCisTimig.frame_cnt_per_sdu = 1,
#ifdef BLE_I2S_AUDIO_SUPPORT
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
    TRACE(0, "Update I2S AUDIO CIS timing:");
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
#endif

extern uint8_t aob_media_mobile_get_cur_streaming_ase_lid(uint8_t con_lid, \
    AOB_MGR_DIRECTION_E direction);
extern void aob_media_mobile_release_stream(uint8_t ase_id);

void app_ble_i2s_media_ascc_release_stream_test(void)
{
    TRACE(1,"%s",__func__);
    aob_media_mobile_release_stream(0);
    aob_media_mobile_release_stream(1);
}

void app_ble_i2s_media_ascc_start_stream_test(void)
{
    TRACE(1,"%s",__func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, BES_BLE_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  BES_BLE_GAF_CONTEXT_TYPE_MEDIA_BIT
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, 1);
    }
}

AOB_MGR_STREAM_STATE_E app_ble_i2s_media_get_cur_ase_state(void)
{
    uint8_t ase_lid = aob_media_mobile_get_cur_streaming_ase_lid(0, AOB_MGR_DIRECTION_SINK);
    return aob_media_mobile_get_cur_ase_state(ase_lid);
}

#ifdef BLE_USB_AUDIO_IS_DONGLE_ROLE

static void gaf_media_i2s_prepare_playback_trigger(uint8_t trigger_channel)
{
    LOG_D("%s start trigger_channel %d", __func__, trigger_channel);

    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    af_stream_dma_tc_irq_enable(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
    adma_ch = af_stream_get_dma_chan(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
    dma_base = af_stream_get_dma_base_addr(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
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

static void gaf_media_i2s_prepare_capture_trigger(uint8_t trigger_channel)
{
    LOG_D("%s start", __func__);
    uint8_t adma_ch = 0;
    uint32_t dma_base;
    btdrv_syn_clr_trigger(trigger_channel);

    af_stream_dma_tc_irq_enable(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    adma_ch = af_stream_get_dma_chan(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_enable_dma_tc(adma_ch&0xFF, dma_base);
    }

    af_codec_set_device_bt_sync_source(AUD_STREAM_USE_INT_CODEC,AUD_STREAM_CAPTURE, trigger_channel);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);

    LOG_D("%s end", __func__);
}

static uint32_t gaf_mobile_calculate_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint32_t latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(pStreamEnv->stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    uint32_t trigger_bt_time = latest_iso_bt_time+GAF_AUDIO_CAPTURE_TRIGGER_DELAY_IN_US;

    while (trigger_bt_time < current_bt_time)
    {
        trigger_bt_time += pStreamEnv->stream_info.captureInfo.dmaChunkIntervalUs;
    }

    LOG_I("iso anch %u cur time %u trigger time %u",
        latest_iso_bt_time, current_bt_time, trigger_bt_time);

    return trigger_bt_time;
}

static void* gaf_mobile_audio_get_media_stream_env(void)
{
    return &gaf_mobile_audio_stream_env;
}

/**
 ****************************************************************************************
 * @brief Called when cis establishment
 *
 * @return void
 ****************************************************************************************
 */
void gap_mobile_start_i2s_audio_receiving_dma(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
        (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv == NULL)
    {
         LOG_E("%s, pStreamEnv is NULL", __func__);
         return;
    }

    LOG_I("%s", __func__);

    uint32_t trigger_bt_time = gaf_mobile_calculate_trigger_time(pStreamEnv);
    gaf_stream_common_set_capture_trigger_time_generic(pStreamEnv, AUD_STREAM_PLAYBACK,trigger_bt_time);
}

void gap_mobile_start_i2s_audio_transmission_dma(void)
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

static void gaf_i2s_mobile_audio_process_pcm_data_send(void *pStreamEnv_,void *payload_,
    uint32_t payload_size, uint32_t ref_time)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T* )pStreamEnv_;
#if defined(BLE_AUDIO_USE_TWO_CHANNEL_SINK_FOR_DONGLE) || defined(BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
    uint32_t payload_len_per_channel = payload_size;
#else
    uint32_t payload_len_per_channel = payload_size/pStreamEnv->stream_info.captureInfo.num_channels;
#endif
    uint8_t ase_count = gaf_cis_mobile_stream_type_op_rule.capture_ase_count;
    uint8_t *payload = (uint8_t *)payload_;
    for (uint8_t i = 0; i < ase_count; i++) {
        bes_ble_bap_iso_dp_send_data(pStreamEnv->stream_info.captureInfo.aseChInfo[i].ase_handle,
                                pStreamEnv->stream_context.latestCaptureSeqNum,
                                payload, payload_len_per_channel,
                                ref_time);
        LOG_D("send cis ase index %d data len %d.", i, payload_len_per_channel);
#ifndef BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT
        payload+=payload_len_per_channel;
#endif
    }
}

static void gaf_i2s_mobile_audio_process_pcm_data(GAF_AUDIO_STREAM_ENV_T *_pStreamEnv, uint8_t *ptrBuf, uint32_t length)
{
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal microsecond -- 0.5 us
    GAF_AUDIO_STREAM_INFO_T stream_info = _pStreamEnv->stream_info;
    GAF_AUDIO_STREAM_CONTEXT_T stream_context = _pStreamEnv->stream_context;

    adma_ch = af_stream_get_dma_chan(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
    dma_base = af_stream_get_dma_base_addr(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((GAF_CAPTURE_STREAM_STREAMING_TRIGGERED ==stream_context.capture_stream_state) &&
        (dmaIrqHappeningTimeUs ==stream_context.lastCaptureDmaIrqTimeUs))
    {
        LOG_W("accumulated irq messages happen!");
        return;
    }

    if (GAF_CAPTURE_STREAM_STREAMING_TRIGGERED != stream_context.capture_stream_state)
    {
        uint32_t expectedDmaIrqHappeningTimeUs =
            _pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs +
            (uint32_t)_pStreamEnv->stream_context.captureAverageDmaChunkIntervalUs;

        int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, expectedDmaIrqHappeningTimeUs);

        int32_t gapUs_abs = GAF_AUDIO_ABS(gapUs);

        if ((gapUs > 0) && (gapUs_abs > (int32_t)_pStreamEnv->stream_info.captureInfo.dmaChunkIntervalUs/2)) {
            LOG_I("%s, gapUs = %d, dmaChunkIntervalUs = %d", __func__, gapUs, _pStreamEnv->stream_info.captureInfo.dmaChunkIntervalUs);
            return;
        }
        gaf_stream_common_update_capture_stream_state(_pStreamEnv,
                                                     GAF_CAPTURE_STREAM_STREAMING_TRIGGERED);
        gaf_stream_common_clr_trigger(stream_context.playbackTriggerChannel);
        uint32_t latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
            BLE_ISOHDL_TO_ACTID(_pStreamEnv->stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));
        _pStreamEnv->stream_context.usSinceLatestAnchorPoint = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, latest_iso_bt_time);
    }

    gaf_stream_common_playback_timestamp_checker(_pStreamEnv, dmaIrqHappeningTimeUs);

    dmaIrqHappeningTimeUs += (uint32_t)stream_info.captureInfo.dmaChunkIntervalUs;
    LOG_D("length %d encoded_len %d filled timestamp %u", length,
        _pStreamEnv->stream_info.captureInfo.encoded_frame_size,dmaIrqHappeningTimeUs);

    _pStreamEnv->func_list->encoder_func_list->encoder_encode_frame_func(
        _pStreamEnv, dmaIrqHappeningTimeUs,length, ptrBuf,
        &_pStreamEnv->stream_context.codec_alg_context[0],&gaf_i2s_mobile_audio_process_pcm_data_send);
}

static bool gaf_mobile_i2s_is_any_capture_stream_iso_created(GAF_AUDIO_STREAM_ENV_T *pStreamEnv)
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

POSSIBLY_UNUSED static void gaf_mobile_i2s_processing_received_data(uint8_t* ptrBuf, uint32_t length)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv && (pStreamEnv->stream_context.capture_stream_state > GAF_CAPTURE_STREAM_INITIALIZED))
    {
        bool isAnyCaptureIsoCreated = gaf_mobile_i2s_is_any_capture_stream_iso_created(pStreamEnv);
        if (isAnyCaptureIsoCreated)
        {
            gaf_i2s_mobile_audio_process_pcm_data(pStreamEnv, ptrBuf, length);
        }
    }
}

static void gaf_i2s_mobile_audio_process_encoded_data(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t *ptrBuf, uint32_t length)
{
    LOG_D("%s start, len = %d", __func__,length);
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    uint32_t len = 0;

    adma_ch = af_stream_get_dma_chan(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    len = length;
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED ==
    pStreamEnv->stream_context.playback_stream_state) &&
    (dmaIrqHappeningTimeUs ==
    pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs))
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
#ifdef GAF_DECODER_CROSS_CORE_USE_M55
        if (true == is_support_ble_audio_mobile_m55_decode)
        {
            pStreamEnv->stream_context.lastestPlaybackSeqNumR--;
            pStreamEnv->stream_context.lastestPlaybackSeqNumL--;
        }
        else
        {
            pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]--;
        }
#else
            pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]--;
#endif

        LOG_I("Update playback seq to 0x%x", pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]);
    }

    bool isOneChannelNoData = false;
    uint8_t ase_count = gaf_cis_mobile_stream_type_op_rule.playback_ase_count;
    GAF_AUDIO_STREAM_COMMON_INFO_T playbackInfo = pStreamEnv->stream_info.playbackInfo;
    uint8_t num_channels = playbackInfo.num_channels;
    uint32_t sample_cnt;

#ifdef __BLE_AUDIO_24BIT__
    if (num_channels == 1)
    {
        sample_cnt = (len/sizeof(int32_t))/2;
    }
    else if (num_channels == 2)
    {
        sample_cnt = (len/sizeof(int32_t));
    }
    else if (num_channels == 4)
    {
        sample_cnt = (len/sizeof(int32_t))*2;
    }
    else
    {
        ASSERT(0, "%s:%d,num_channels err:%d", __func__, __LINE__, num_channels)
    }
    int32_t tmp_buf[ase_count][sample_cnt];
#else
    if (num_channels == 1)
    {
        sample_cnt = (len/sizeof(int16_t))/2;
    }
    else if (num_channels == 2)
    {
        sample_cnt = (len/sizeof(int16_t));
    }
    //todo: need to verrity lc3 codec 4ch
    else if (num_channels == 4)
    {
        sample_cnt = (len/sizeof(int16_t))*2;
    }
    else
    {
        ASSERT(0, "%s:%d,num_channels err:%d", __func__, __LINE__, num_channels)
    }
    int16_t tmp_buf[ase_count][sample_cnt];
#endif
    memset(tmp_buf, 0, sizeof(tmp_buf));

    LOG_I("contextType %d ase_count %d,sample_cnt %d,num_channel %d",
        pStreamEnv->stream_info.contextType, ase_count,sample_cnt,num_channels);

    uint8_t detected_ase_index = 0;
    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++) {
        if (playbackInfo.aseChInfo[i].iso_channel_hdl == GAF_AUDIO_INVALID_ISO_CHANNEL) {
             continue;
        }

        int ret = 0;
        POSSIBLY_UNUSED int32_t diff_bt_time = 0;
        gaf_media_data_t *decoder_frame_p = NULL;
        GAF_AUDIO_STREAM_COMMON_INFO_T playback_Info = pStreamEnv->stream_info.playbackInfo;
        decoder_frame_p = gaf_mobile_audio_get_packet(pStreamEnv, dmaIrqHappeningTimeUs,
                playback_Info.aseChInfo[i].iso_channel_hdl);

        LOG_I("%s %d %d %d length %d", __func__, detected_ase_index, decoder_frame_p->data_len,
                playback_Info.aseChInfo[i].iso_channel_hdl, length);

        ret = pStreamEnv->func_list->decoder_func_list->decoder_decode_frame_func
                (pStreamEnv, decoder_frame_p->data_len, decoder_frame_p->sdu_data,
                &pStreamEnv->stream_context.codec_alg_context[detected_ase_index], tmp_buf[detected_ase_index]);

        LOG_D("dec ret %d ", ret);
        if (detected_ase_index == 0)
        {
            diff_bt_time = GAF_AUDIO_CLK_32_BIT_DIFF(decoder_frame_p->time_stamp +
                                                    (GAF_AUDIO_FIXED_MOBILE_SIDE_PRESENT_LATENCY_US),
                                                    dmaIrqHappeningTimeUs);
            gaf_media_pid_adjust(AUD_STREAM_CAPTURE, &(pStreamEnv->stream_context.playback_pid_env),
                                            diff_bt_time);
            LOG_D("index %d Decoded seq 0x%02x expected play time %u local time %u diff %d", detected_ase_index, decoder_frame_p->pkt_seq_nb,
                    decoder_frame_p->time_stamp, dmaIrqHappeningTimeUs, diff_bt_time);
        }
        gaf_stream_data_free(decoder_frame_p);

        detected_ase_index++;
    }
#if defined(I2S_REC_AUDIO_DUMP)
    for(int ch = 0; ch < num_channels; ch++)
    {
        audio_dump_clear_up();
        audio_dump_add_channel_data(0, tmp_buf+ch*sample_cnt, sample_cnt);
        audio_dump_run();
    }
#endif
    // Merge stream data
    if (!isOneChannelNoData) {
#if BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 1
        memcpy((void *)ptrBuf, (void *)tmp_buf[0], len);
#else
#ifdef __BLE_AUDIO_24BIT__
        uint16_t i = 0;

        if (num_channels = 1)
        {
            for (uint32_t samples = 0; samples < sample_cnt; samples++)
            {
                ((int32_t *)ptrBuf)[i++] = (int32_t)tmp_buf[0][samples];
                ((int32_t *)ptrBuf)[i++] = (int32_t)tmp_buf[1][samples];
            }
        }
        else
        {
            for (uint32_t samples = 0; samples < sample_cnt; samples++)
            {
                ((int32_t *)ptrBuf)[samples] =
                    (int32_t)(((int32_t)tmp_buf[0][samples] + (int32_t)tmp_buf[1][samples])/2);
            }
        }
#else
        uint16_t i = 0;
        if (num_channels == 1)
        {
            for (uint32_t samples = 0; samples < sample_cnt; samples++)
            {
                ((int16_t *)ptrBuf)[i++] = (int16_t)tmp_buf[0][samples];
                ((int16_t *)ptrBuf)[i++] = (int16_t)tmp_buf[1][samples];
            }
        }
        else
        {
            for (uint32_t samples = 0; samples < sample_cnt; samples++)
            {
                ((int16_t *)ptrBuf)[samples] =
                    (int16_t)(((int16_t)tmp_buf[0][samples] + (int16_t)tmp_buf[1][samples])/2);
            }
        }
        //LOG_I("dump source record pcm data:");
        //DUMP8("%02x ",ptrBuf, 16);
#endif
#endif
    } else {
       memset(ptrBuf, 0, length);
    }
}

POSSIBLY_UNUSED static void gaf_mobile_i2s_feed_data(uint8_t* ptrBuf, uint32_t length)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();
    if (pStreamEnv && (pStreamEnv->stream_context.playback_stream_state > GAF_PLAYBACK_STREAM_INITIALIZED)) {
        gaf_i2s_mobile_audio_process_encoded_data(pStreamEnv, ptrBuf, length);
    }
}

void gaf_mobile_i2s_dma_playback_stop(void)
{
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (pStreamEnv) {
        af_stream_dma_tc_irq_disable(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
        dma_base = af_stream_get_dma_base_addr(I2S_AUD_STREAM_ID_PLAYBACK, AUD_STREAM_PLAYBACK);
        if (adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
        }

        btdrv_syn_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);

        if (pStreamEnv->stream_context.capture_stream_state > GAF_CAPTURE_STREAM_INITIALIZED) {
            gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);
        }
    }
}


void gaf_mobile_i2s_dma_capture_stop(void)
{
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();

    if (!pStreamEnv)
    {
       return;
    }

    af_stream_dma_tc_irq_disable(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    adma_ch = af_stream_get_dma_chan(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(I2S_AUD_STREAM_ID_CAPTURE, AUD_STREAM_CAPTURE);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
    }

    btdrv_syn_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);

    bes_ble_bap_dp_itf_data_come_callback_deregister();

    if (pStreamEnv->stream_context.playback_stream_state > GAF_PLAYBACK_STREAM_INITIALIZED)
    {
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);
    }
}

static bool app_ble_is_i2s_playback_on = false;
static bool app_ble_is_i2s_capture_on = false;

static void gaf_mobile_i2s_set_sysfreq(void)
{
    if (app_ble_is_i2s_capture_on && app_ble_is_i2s_playback_on)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_I2S_AUDIO_STREAMING, APP_SYSFREQ_26M);
    }
    else if (app_ble_is_i2s_capture_on && !app_ble_is_i2s_playback_on)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_I2S_AUDIO_STREAMING, APP_SYSFREQ_26M);
    }
    else if (!app_ble_is_i2s_capture_on && app_ble_is_i2s_playback_on)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_I2S_AUDIO_STREAMING, APP_SYSFREQ_26M);
    }
    else
    {
        app_sysfreq_req(APP_SYSFREQ_USER_I2S_AUDIO_STREAMING, APP_SYSFREQ_32K);
        af_set_priority(AF_USER_AI, osPriorityAboveNormal);
    }
}

POSSIBLY_UNUSED static void gaf_mobile_i2s_playback_vol_changed(uint32_t level)
{
    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        uint8_t ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, BES_BLE_GAF_DIRECTION_SINK);
        if (ase_lid != 0xFF)
        {
            aob_mobile_vol_set_abs(i, level);
        }
    }
}

static void gaf_mobile_i2s_start_debounce_handler(void const *param)
{
    if (!app_ble_is_i2s_capture_on) {
        app_ble_i2s_stream_capture_start_supervisor_timer();
        app_ble_is_i2s_capture_on = true;
        gaf_mobile_i2s_set_sysfreq();
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START, 0, 0);
    }
}

POSSIBLY_UNUSED static void gaf_mobile_i2s_playback_start(void)
{
    if (!app_ble_is_i2s_playback_on)
    {
        app_ble_i2s_stream_playback_start_supervisor_timer();
        app_ble_is_i2s_playback_on = true;
        gaf_mobile_i2s_set_sysfreq();
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START, 0, 0);
    }
}

void gaf_mobile_i2s_playback_stop(void)
{
    if (app_ble_is_i2s_playback_on)
    {
        app_ble_i2s_stream_playback_start_supervisor_timer();
        app_ble_is_i2s_playback_on = false;
        gaf_mobile_i2s_set_sysfreq();
        gaf_mobile_i2s_dma_playback_stop();
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP, 0, 0);
    }
}

POSSIBLY_UNUSED static void gaf_mobile_i2s_capture_start(void)
{
    LOG_D("%s", __func__);
    osTimerStart(gaf_mobile_i2s_start_debounce_timer, I2S_CAPTURE_START_DEBOUNCE_TIME_MS);
}

POSSIBLY_UNUSED static void gaf_mobile_i2s_capture_stop(void)
{
    LOG_D("%s", __func__);
    osTimerStop(gaf_mobile_i2s_start_debounce_timer);
    if (app_ble_is_i2s_capture_on) {
        app_ble_is_i2s_capture_on = false;
        app_ble_i2s_stream_capture_start_supervisor_timer();
        gaf_mobile_i2s_set_sysfreq();
        gaf_mobile_i2s_dma_capture_stop();
        ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP, 0, 0);
    }
}

POSSIBLY_UNUSED static void gaf_mobile_i2s_prepare_handler(enum AUD_STREAM_T stream)
{
    if (AUD_STREAM_PLAYBACK == stream)
    {
        gaf_media_i2s_prepare_playback_trigger(I2S_BLE_AUDIO_PLAYBACK_TRIGGER_CHANNEL);
    }
    else
    {
        gaf_media_i2s_prepare_capture_trigger(I2S_BLE_AUDIO_CAPTURE_TRIGGER_CHANNEL);
    }
}

POSSIBLY_UNUSED static bool gaf_mobile_i2s_reset_codec_feasibility_check_handler(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*)gaf_mobile_audio_get_media_stream_env();
    if (pStreamEnv)
    {
        TRACE(0, "cp state %d pb state %d", pStreamEnv->stream_context.capture_stream_state,
            pStreamEnv->stream_context.playback_stream_state);
    }

    if (pStreamEnv &&
        ((GAF_CAPTURE_STREAM_INITIALIZED == pStreamEnv->stream_context.capture_stream_state) ||
        (GAF_CAPTURE_STREAM_START_TRIGGERING == pStreamEnv->stream_context.capture_stream_state)))
    {
        return false;
    }

    if (pStreamEnv &&
        ((GAF_PLAYBACK_STREAM_INITIALIZED == pStreamEnv->stream_context.playback_stream_state) ||
        (GAF_PLAYBACK_STREAM_START_TRIGGERING == pStreamEnv->stream_context.playback_stream_state)))
    {
        return false;
    }

    return true;
}

static const I2S_AUDIO_SOURCE_EVENT_CALLBACK_T ble_i2s_audio_stream_func_cb_list =
{
    .data_playback_cb = gaf_mobile_i2s_processing_received_data,
    .data_capture_cb = gaf_mobile_i2s_feed_data,
    .reset_codec_feasibility_check_cb = gaf_mobile_i2s_reset_codec_feasibility_check_handler,
    .data_prep_cb = gaf_mobile_i2s_prepare_handler,
    .playback_start_cb = gaf_mobile_i2s_playback_start,
    .playback_stop_cb = gaf_mobile_i2s_playback_stop,

    .capture_start_cb = gaf_mobile_i2s_capture_start,
    .capture_stop_cb = gaf_mobile_i2s_capture_stop,

    .playback_vol_change_cb = gaf_mobile_i2s_playback_vol_changed,
};

void app_ble_i2s_stream_stm_post_op_checker(BLE_AUDIO_CENTRAL_STREAM_EVENT_E event)
{
    switch (event)
    {
        case EVT_PLAYBACK_STREAM_STARTED:
        {
            if (!app_ble_is_i2s_playback_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP, 0, 0);
            }
            break;
        }
        case EVT_CAPTURE_STREAM_STARTED:
        {
            if (!app_ble_is_i2s_capture_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP, 0, 0);
            }
            break;
        }
        case EVT_PLAYBACK_STREAM_STOPPED:
        {
            if (app_ble_is_i2s_playback_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START, 0, 0);
            }
            break;
        }
        case EVT_CAPTURE_STREAM_STOPPED:
        {
            if (app_ble_is_i2s_capture_on)
            {
                ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START, 0, 0);
            }
            break;
        }
        default:
            break;
    }
}

#define BLE_I2S_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS  3000
#define BLE_USE_STREAM_CONTROL_RETRY_TIMES  5
static void ble_use_stream_playback_control_supervisor_timer_cb(void const *n);
osTimerDef(BLE_I2S_STREAM_PLAYBACK_CONTROL_SUPERVISOR_TIMER, ble_use_stream_playback_control_supervisor_timer_cb);
static osTimerId ble_use_stream_playback_control_supervisor_timer_id = NULL;
static uint8_t ble_i2s_stream_playback_control_try_times_till_now = 0;

static void ble_use_stream_playback_control_supervisor_timer_cb(void const *n)
{
    if (ble_i2s_stream_playback_control_try_times_till_now < BLE_USE_STREAM_CONTROL_RETRY_TIMES)
    {
        if (app_ble_is_i2s_playback_on)
        {
            TRACE(0, "Starting ble i2s playback stream timeout!Retry time count %d.", ble_i2s_stream_playback_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START, 0, 0);
        }
        else
        {
            TRACE(0, "Stopping ble i2s playback stream timeout!Retry time count %d.", ble_i2s_stream_playback_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP, 0, 0);
        }

        ble_i2s_stream_playback_control_try_times_till_now++;
        osTimerStart(ble_use_stream_playback_control_supervisor_timer_id, BLE_I2S_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
    }
}

static void app_ble_i2s_stream_playback_start_supervisor_timer(void)
{
    if (NULL == ble_use_stream_playback_control_supervisor_timer_id)
    {
        ble_use_stream_playback_control_supervisor_timer_id =
            osTimerCreate(osTimer(BLE_I2S_STREAM_PLAYBACK_CONTROL_SUPERVISOR_TIMER), osTimerOnce, NULL);
    }

    ble_i2s_stream_playback_control_try_times_till_now = 0;
    osTimerStart(ble_use_stream_playback_control_supervisor_timer_id, BLE_I2S_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
}

static void app_ble_i2s_stream_playback_stop_supervisor_timer(void)
{
    osTimerStop(ble_use_stream_playback_control_supervisor_timer_id);
}

static void ble_use_stream_capture_control_supervisor_timer_cb(void const *n);
osTimerDef(BLE_I2S_STREAM_CAPTURE_CONTROL_SUPERVISOR_TIMER, ble_use_stream_capture_control_supervisor_timer_cb);
static osTimerId ble_use_stream_capture_control_supervisor_timer_id = NULL;
static uint8_t ble_i2s_stream_capture_control_try_times_till_now = 0;

static void ble_use_stream_capture_control_supervisor_timer_cb(void const *n)
{
    if (ble_i2s_stream_capture_control_try_times_till_now < BLE_USE_STREAM_CONTROL_RETRY_TIMES)
    {
        if (app_ble_is_i2s_capture_on)
        {
            TRACE(0, "Starting ble i2s capture stream timeout!Retry time count %d.", ble_i2s_stream_capture_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START, 0, 0);
        }
        else
        {
            TRACE(0, "Stopping ble i2s capture stream timeout!Retry time count %d.", ble_i2s_stream_capture_control_try_times_till_now);
            ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP, 0, 0);
        }

        ble_i2s_stream_capture_control_try_times_till_now++;
        osTimerStart(ble_use_stream_capture_control_supervisor_timer_id, BLE_I2S_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
    }
}

static void app_ble_i2s_stream_capture_start_supervisor_timer(void)
{
    if (NULL == ble_use_stream_capture_control_supervisor_timer_id)
    {
        ble_use_stream_capture_control_supervisor_timer_id =
            osTimerCreate(osTimer(BLE_I2S_STREAM_CAPTURE_CONTROL_SUPERVISOR_TIMER), osTimerOnce, NULL);
    }

    ble_i2s_stream_capture_control_try_times_till_now = 0;
    osTimerStart(ble_use_stream_capture_control_supervisor_timer_id, BLE_I2S_STREAM_CONTROL_SUPERVISOR_TIMEOUT_MS);
}

static void app_ble_i2s_stream_capture_stop_supervisor_timer(void)
{
    osTimerStop(ble_use_stream_capture_control_supervisor_timer_id);
}

void app_ble_i2s_stream_inform_playback_state(uint32_t event)
{
    if (app_ble_is_i2s_playback_on && (EVT_PLAYBACK_STREAM_STARTED == event))
    {
        app_ble_i2s_stream_playback_stop_supervisor_timer();
    }
    else if ((!app_ble_is_i2s_playback_on) && (EVT_PLAYBACK_STREAM_STOPPED == event))
    {
        app_ble_i2s_stream_playback_stop_supervisor_timer();
    }

    ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), (BLE_AUDIO_CENTRAL_STREAM_EVENT_E)event, 0, 0);
}

void app_ble_i2s_stream_inform_capture_state(uint32_t event)
{
    if (app_ble_is_i2s_capture_on && (EVT_CAPTURE_STREAM_STARTED == event))
    {
        app_ble_i2s_stream_capture_stop_supervisor_timer();
    }
    else if ((!app_ble_is_i2s_capture_on) && (EVT_CAPTURE_STREAM_STOPPED == event))
    {
        app_ble_i2s_stream_capture_stop_supervisor_timer();
    }

    ble_audio_central_stream_send_message(ble_audio_central_get_stream_sm(), (BLE_AUDIO_CENTRAL_STREAM_EVENT_E)event, 0, 0);
}

void app_ble_i2s_audio_init(void)
{
    ble_audio_central_stream_stm_init();

    ble_audio_central_stream_start_receiving_handler_register(gap_mobile_start_i2s_audio_receiving_dma);;
    ble_audio_central_stream_start_transmission_handler_register(gap_mobile_start_i2s_audio_transmission_dma);
    ble_audio_central_stream_post_operation_check_cb_register(app_ble_i2s_stream_stm_post_op_checker);

    i2s_ble_audio_update_timing_test_handler();
#ifndef DONGLE_AS_I2S_MASTER
    i2s_audio_source_config_init(&ble_i2s_audio_stream_func_cb_list);
#endif
    ble_audio_central_stream_stm_t* i2s_stream_stm_ptr = ble_audio_central_get_stream_sm();
    if (NULL != i2s_stream_stm_ptr) {
        ble_audio_central_stream_stm_startup(i2s_stream_stm_ptr);
    }

    if (gaf_mobile_i2s_start_debounce_timer == NULL) {
        gaf_mobile_i2s_start_debounce_timer =
            osTimerCreate (osTimer(GAF_I2S_CAPTURE_START_DEBOUNCE), osTimerOnce, NULL);
    }
}

/// Transmit i2s data to remote side.
int gaf_mobile_i2s_audio_media_stream_start_handler(void* _pStreamEnv)
{
    LOG_I("%s", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    if (GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_208M);
        af_set_priority(AF_USER_AI, osPriorityHigh);

        pStreamEnv->func_list->stream_func_list.capture_init_stream_buf_func(pStreamEnv);
        pStreamEnv->func_list->encoder_func_list->encoder_init_func(pStreamEnv);
        pStreamEnv->stream_context.captureTriggerChannel = I2S_BLE_AUDIO_PLAYBACK_TRIGGER_CHANNEL;
        LOG_D("%s captureTriggerChannel %d", __func__,pStreamEnv->stream_context.captureTriggerChannel);

         gaf_media_pid_init(&(pStreamEnv->stream_context.playback_pid_env));

        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);

        app_ble_i2s_stream_inform_playback_state(EVT_PLAYBACK_STREAM_STARTED);
        LOG_I("%s end", __func__);

        return 0;
    }

    return -1;
}


int gaf_mobile_i2s_audio_media_stream_stop_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    pStreamEnv->func_list->encoder_func_list->encoder_deinit_func(pStreamEnv);
    pStreamEnv->func_list->stream_func_list.capture_deinit_stream_buf_func(pStreamEnv);

    app_bt_sync_release_trigger_channel(pStreamEnv->stream_context.captureTriggerChannel);

    gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_IDLE);
#ifdef GAF_CODEC_CROSS_CORE
#ifdef DSP_M55
    app_dsp_m55_deinit();
#endif
#endif
    af_set_priority(AF_USER_AI, osPriorityAboveNormal);

    app_ble_i2s_stream_inform_playback_state(EVT_PLAYBACK_STREAM_STOPPED);

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_32K);
    return 0;
}

int gaf_mobile_i2s_audio_capture_start_handler(void* _pStreamEnv)
{
    LOG_I("%s", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    uint8_t ase_count = gaf_cis_mobile_stream_type_op_rule.playback_ase_count;

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_208M);
    af_set_priority(AF_USER_AI, osPriorityHigh);

    pStreamEnv->func_list->stream_func_list.playback_init_stream_buf_func(pStreamEnv);
    pStreamEnv->func_list->decoder_func_list->decoder_init_func(pStreamEnv, ase_count);
    pStreamEnv->stream_context.playbackTriggerChannel = I2S_BLE_AUDIO_CAPTURE_TRIGGER_CHANNEL;

    gaf_media_pid_init(&(pStreamEnv->stream_context.capture_pid_env));
    gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);

    app_ble_i2s_stream_inform_capture_state(EVT_CAPTURE_STREAM_STARTED);

#if defined(I2S_REC_AUDIO_DUMP)
    audio_dump_init(480, sizeof(short), 2);
#endif
    return 0;
}

int gaf_mobile_i2s_audio_capture_stop_handler(void* _pStreamEnv)
{
    LOG_I("%s", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    pStreamEnv->func_list->decoder_func_list->decoder_deinit_func();
    pStreamEnv->func_list->stream_func_list.playback_deinit_stream_buf_func(pStreamEnv);
    gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_IDLE);

    app_bt_sync_release_trigger_channel(pStreamEnv->stream_context.playbackTriggerChannel);
    af_set_priority(AF_USER_AI, osPriorityAboveNormal);

    app_ble_i2s_stream_inform_capture_state(EVT_CAPTURE_STREAM_STOPPED);

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);

    return 0;
}

bool gaf_mobile_i2s_audio_check_capture_need_start(void)
{
    return true;
    // return i2s_audio_check_capture_need_start();
}

#endif //BLE_USB_AUDIO_IS_DONGLE_ROLE
#endif //BLE_I2S_AUDIO_SUPPORT
#endif //AOB_MOBILE_ENABLED

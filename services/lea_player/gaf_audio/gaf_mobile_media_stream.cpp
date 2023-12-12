/**
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
#include "hal_dma.h"
#include "hal_trace.h"
#include "app_trace_rx.h"
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "plat_types.h"
#include "cqueue.h"
#include "app_gaf_dbg.h"
#include "heap_api.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "app_audio.h"
#include "rwble_config.h"
#include "besbt_string.h"
#include "gaf_media_sync.h"
#include "gaf_media_pid.h"
#include "gaf_media_common.h"
#include "gaf_media_stream.h"
#include "gaf_codec_lc3.h"
#include "gaf_media_common.h"
#include "gaf_mobile_media_stream.h"
#include "app_gaf_custom_api.h"
#include "gaf_media_sync.h"
#include "app_gaf_custom_api.h"
#include "ble_audio_ase_stm.h"
#include "app_bt_sync.h"
#include "ble_audio_mobile_info.h"
#include "bluetooth_ble_api.h"
#include "app_bap.h"
#ifdef AOB_MOBILE_ENABLED
#include "aob_cis_api.h"
#include "gaf_ull_hid_support.h"
#endif
#ifdef BLE_USB_AUDIO_SUPPORT
#include "app_ble_usb_audio.h"
#endif
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
#include "app_usb_hw_timer.h"
#endif
#ifdef BLE_I2S_AUDIO_SUPPORT
#include "app_ble_i2s_audio.h"
#endif
#ifdef GAF_DSP
#include "mcu_dsp_hifi4_app.h"
#endif
#ifdef AOB_MOBILE_ENABLED
/*********************external function declaration*************************/
extern "C" uint32_t btdrv_reg_op_cig_anchor_timestamp(uint8_t link_id);
#ifdef BLE_USB_AUDIO_SUPPORT
extern void app_usbaudio_entry(void);
extern "C"  bool app_usbaudio_mode_on(void);
#endif
/************************private macro defination***************************/
#ifndef BT_AUDIO_CACHE_2_UNCACHE
#define BT_AUDIO_CACHE_2_UNCACHE(addr) \
    ((unsigned char *)((unsigned int)addr & ~(0x04000000)))
#endif
/************************private variable defination************************/
GAF_AUDIO_STREAM_ENV_T gaf_mobile_audio_stream_env;
static GAF_MEDIA_DWELLING_INFO_T gaf_cis_mobile_media_dwelling_info[GAF_MOB_MAXIMUM_CONNECTION_COUNT];

const GAF_MEDIA_STREAM_TYPE_OPERATION_RULE_T gaf_cis_mobile_stream_type_op_rule_all =
{
#ifdef LEA_FOUR_CIS_ENABLED
    GAF_AUDIO_STREAM_TYPE_FLEXIBLE, GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM, 4, 4,
#else
    GAF_AUDIO_STREAM_TYPE_FLEXIBLE, GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM, 2, 2,
#endif
};

#ifdef AOB_UC_TEST
uint8_t mobile_freq;
#endif
/*******************************GAF CUSTOM**************************************/
static const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T *gaf_uc_cli_custom_data_callback = NULL;

/****************************function defination****************************/
static uint8_t gaf_mobile_get_connected_device_count(void)
{
    uint8_t gaf_connected_dev_cnt = 0;
    for (uint8_t con_lid = 0; con_lid < GAF_MOB_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        for (uint8_t idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
        {
            if (gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[idx]
                != GAF_INVALID_ASE_INDEX)
            {
                gaf_connected_dev_cnt += 1;
                break;
            }
        }
    }

    return gaf_connected_dev_cnt;
}

static void gaf_mobile_audio_add_ase_into_playback_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d playback ase lid %d", con_lid, ase_lid);
    uint8_t idx = 0;
    if (ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT)
    {
        if (GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT ==
            gaf_stream_common_get_ase_idx_in_ase_lid_list(
                &gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[0], ase_lid))
        {
            idx = gaf_stream_common_get_valid_idx_in_ase_lid_list(
                    &gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[0]);
            if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
            {
                LOG_E("no more space for adding playback ase lid: %d into list!!!", ase_lid);
            }
            else
            {
                gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[idx] = ase_lid;
            }
        }
        else
        {
            LOG_E("ase_lid: %d already in playback ase list!!!", ase_lid);
        }
    }
    else
    {
        LOG_E("no more space for adding playback ase list!!!");
    }
}

static void gaf_mobile_audio_remove_ase_from_playback_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d playback ase lid %d", con_lid, ase_lid);
    uint8_t idx = 0;
    if (ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT)
    {
        idx = gaf_stream_common_get_ase_idx_in_ase_lid_list(
                &gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[0], ase_lid);
        if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
        {
            LOG_E("ase lid: %d is not in the list!!!", ase_lid);
            return;
        }
        gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[idx] = GAF_INVALID_ASE_INDEX;
    }
    else
    {
        LOG_E("no more space for removing playback ase list!!!");
    }
}

static void gaf_mobile_audio_add_ase_into_capture_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d capture ase lid %d", con_lid, ase_lid);
    uint8_t idx = 0;
    if (ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT)
    {
        if (GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT ==
            gaf_stream_common_get_ase_idx_in_ase_lid_list(
                &gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[0], ase_lid))
        {
            idx = gaf_stream_common_get_valid_idx_in_ase_lid_list(
                    &gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[0]);
            if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
            {
                LOG_E("no more space for adding capture ase lid: %d into list!!!", ase_lid);
            }
            else
            {
                gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[idx] = ase_lid;
            }
        }
        else
        {
            LOG_E("ase_lid: %d already in capture ase list!!!", ase_lid);
        }
    }
    else
    {
        LOG_E("no more space for adding capture ase list!!!");
    }
}

static void gaf_mobile_audio_remove_ase_from_capture_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d capture ase lid %d", con_lid, ase_lid);
    uint8_t idx = 0;
    if (ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT)
    {
        idx = gaf_stream_common_get_ase_idx_in_ase_lid_list(
                &gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[0], ase_lid);
        if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
        {
            LOG_E("ase lid: %d is not in the list!!!", ase_lid);
            return;
        }
        gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[idx] = GAF_INVALID_ASE_INDEX;
    }
    else
    {
        LOG_E("no more space for removing capture ase list!!!");
    }
}

static uint8_t* gaf_mobile_audio_get_playback_ase_index_list(uint8_t con_lid)
{
    LOG_D("get playback con_lid %d", con_lid);
    return &gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[0];
}

static uint8_t* gaf_mobile_audio_get_capture_ase_index_list(uint8_t con_lid)
{
    LOG_D("get capture con_lid %d", con_lid);
    return &gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[0];
}

static void gaf_mobile_audio_clear_playback_ase_index_list(uint8_t con_lid)
{
    LOG_D("clear playback ase list con_lid %d", con_lid);
    memset_s(&gaf_cis_mobile_media_dwelling_info[con_lid].playback_ase_id[0],
        GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t), GAF_INVALID_ASE_INDEX,
        GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t));
}

static void gaf_mobile_audio_clear_capture_ase_index_list(uint8_t con_lid)
{
    LOG_D("clear capture ase list con_lid %d", con_lid);
    memset_s(&gaf_cis_mobile_media_dwelling_info[con_lid].capture_ase_id[0],
        GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t), GAF_INVALID_ASE_INDEX,
        GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t));
}

static GAF_AUDIO_STREAM_ENV_T* gaf_mobile_audio_get_stream_env_by_chl_index(uint8_t channel)
{
    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++) {
        if (gaf_mobile_audio_stream_env.stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl == channel)
        {
            return &gaf_mobile_audio_stream_env;
        }
    }
    //ASSERT(0, "Receive a CIS packet before cooresponding stream is ready %d!", channel);
    return NULL;
}

void gaf_mobile_audio_receive_data(uint16_t conhdl, GAF_ISO_PKT_STATUS_E pkt_status)
{
    uint8_t channel = BLE_ISOHDL_TO_ACTID(conhdl);
    // map to gaf stream context

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_mobile_audio_get_stream_env_by_chl_index(channel);
    gaf_media_data_t* p_media_data = NULL;

    /// Check for stream env info for app unsetup ASE
    if (!pStreamEnv)
    {
        while ((p_media_data = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
        {
            gaf_stream_data_free(p_media_data);
        }
        return;
    }

    uint8_t ase_lid = gaf_media_common_get_ase_chan_lid_from_iso_channel(
                                    pStreamEnv, BES_BLE_GAF_DIRECTION_SINK, channel);
    if (ase_lid >= GAF_AUDIO_ASE_TOTAL_COUNT)
    {
        LOG_E("can not get ase chan lid by cis handle: 0x%x", conhdl);
        return;
    }

    CODEC_CORE_INFO_T *coreInfo = &pStreamEnv->stream_info.playbackInfo.aseChInfo[ase_lid].codec_core_info;
    POSSIBLY_UNUSED uint32_t current_bt_time = 0;
    POSSIBLY_UNUSED uint32_t trigger_bt_time = 0;
    CC_PLAYBACK_DATA_T frame = {0};
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (coreInfo->instance_status < INSTANCE_INITIALIZED) {
        while ((p_media_data = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
        {
            gaf_stream_data_free(p_media_data);
        }
        return;
    }
#else
    const bes_ble_bap_ascc_ase_t *p_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);
    coreInfo->instance_handle = p_ase_info->con_lid;
#endif

    while ((p_media_data = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
    {
        ASSERT(p_media_data->data_len <= pStreamEnv->stream_info.playbackInfo.maxFrameSize,
            "%s len %d %d, channel:%d, playbackInfo:%p", __func__, p_media_data->data_len,
            pStreamEnv->stream_info.playbackInfo.maxFrameSize, channel, &(pStreamEnv->stream_info.playbackInfo));
        LOG_D("%s pkt_status %d pkt_seq_nb 0x%x channel %d, len %d", __func__,
            p_media_data->pkt_status, p_media_data->pkt_seq_nb, channel, p_media_data->data_len);

        frame.time_stamp = p_media_data->time_stamp;
        frame.seq_nb = p_media_data->pkt_seq_nb;
        frame.data_len = p_media_data->data_len;
        frame.data = p_media_data->sdu_data;
        frame.isPLC = ((GAF_ISO_PKT_STATUS_VALID != p_media_data->pkt_status) || (0 == p_media_data->data_len));

        if ((GAF_PLAYBACK_STREAM_INITIALIZED == pStreamEnv->stream_context.playback_stream_state) &&
             ((GAF_ISO_PKT_STATUS_VALID == p_media_data->pkt_status) && (p_media_data->data_len > 0)))
        {
            if ((GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM ==
                gaf_cis_mobile_stream_type_op_rule_all.trigger_stream_type ) ||
                (GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM == gaf_cis_mobile_stream_type_op_rule_all.trigger_stream_type))
            {
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
                uint32_t latest_anch_time = gaf_media_common_get_latest_rx_iso_evt_timestamp(pStreamEnv);
                uint32_t isoInterval = pStreamEnv->stream_info.playbackInfo.isoIntervalUs;
                LOG_I("expected play us %u latest_anch_time us %u seq 0x%x",
                    p_media_data->time_stamp, latest_anch_time, p_media_data->pkt_seq_nb);
                pStreamEnv->stream_info.playbackInfo.presDelayUs = 2 * isoInterval -
                    (((p_media_data->time_stamp + 2*isoInterval) - latest_anch_time) % isoInterval);
                pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs =
                    p_media_data->time_stamp + pStreamEnv->stream_info.playbackInfo.presDelayUs - isoInterval;
                LOG_I("presDelayUs:%d", pStreamEnv->stream_info.playbackInfo.presDelayUs);
                gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_START_TRIGGERING);
#else
                current_bt_time = gaf_media_sync_get_curr_time();
                LOG_I("%s expected play us %u current us %u seq 0x%x", __func__,
                    p_media_data->time_stamp, current_bt_time, p_media_data->pkt_seq_nb);
                trigger_bt_time = p_media_data->time_stamp + pStreamEnv->stream_info.playbackInfo.presDelayUs
                    - pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;
                LOG_I("calculated trigger ticks %u", trigger_bt_time);
                if ((int32_t)(trigger_bt_time - current_bt_time) >= GAF_MARGIN_BETWEEN_TRIGGER_TIME_AND_CURRENT_TIME_US)
                {
                    LOG_I("Starting playback seq num 0x%x", p_media_data->pkt_seq_nb);
                    pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX] = p_media_data->pkt_seq_nb;

#if defined(BLE_USB_AUDIO_SUPPORT) && defined(BLE_USB_AUDIO_IS_DONGLE_ROLE)
                    if (gaf_mobile_usb_audio_check_capture_need_start())
                    {
                        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
                    }
                    gaf_stream_common_set_playback_trigger_time_generic(pStreamEnv, AUD_STREAM_CAPTURE,
                                                trigger_bt_time);
#elif defined(BLE_I2S_AUDIO_SUPPORT)&&defined(BLE_USB_AUDIO_IS_DONGLE_ROLE)
                    if (gaf_mobile_i2s_audio_check_capture_need_start())
                    {
                        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
                    }
                    gaf_stream_common_set_playback_trigger_time_generic(pStreamEnv, AUD_STREAM_CAPTURE,
                                                trigger_bt_time);
#else
                    if (GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM == gaf_cis_mobile_stream_type_op_rule_all.trigger_stream_type)
                    {
                        gaf_media_prepare_playback_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
                        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
                    }
                    gaf_stream_common_set_playback_trigger_time(pStreamEnv, trigger_bt_time);
#endif
                }
                else
                {
                    LOG_I("time_stamp error");
                }
#endif
            }
        }

        if (pStreamEnv->stream_context.playback_stream_state >= GAF_PLAYBACK_STREAM_START_TRIGGERING)
        {
            if (frame.isPLC) {
                LOG_I("PLC seq:%u ts:%u pk status:%d data len:%d",
                    p_media_data->pkt_seq_nb, p_media_data->time_stamp, p_media_data->pkt_status, p_media_data->data_len);
            }
            if (gaf_uc_cli_custom_data_callback->encoded_packet_recv_cb)
            {
                gaf_uc_cli_custom_data_callback->encoded_packet_recv_cb(p_media_data);
            }
            gaf_stream_common_store_packet(pStreamEnv, &frame, coreInfo);
        }

        gaf_stream_data_free(p_media_data);
    }
}

static void gaf_mobile_audio_process_pcm_data_send(void *pStreamEnv_,void *payload_,
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
        stereo_channel_support = app_bap_capa_cli_is_peer_support_stereo_channel(pStreamEnv->stream_info.con_lid,
                                                                                                    BES_BLE_GAF_DIRECTION_SINK);
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
        /// gaf custom, maybe a data watch point, add some pattern in packet before send
        if (gaf_uc_cli_custom_data_callback->encoded_packet_send_cb)
        {
            gaf_uc_cli_custom_data_callback->encoded_packet_send_cb(payload + channel_shift, payload_len_per_channel);
        }
        LOG_D("[CAPTURE SEND] p_len:%d stereo supp: %d, allocation_bf: 0x%x, shift :%d",
              payload_len_per_channel, stereo_channel_support, audio_allocation_bf, channel_shift);
        bes_ble_bap_iso_dp_send_data(pStreamEnv->stream_info.captureInfo.aseChInfo[i].ase_handle,
                                pStreamEnv->stream_context.latestCaptureSeqNum,
                                payload + channel_shift, payload_len_per_channel,
                                ref_time);
    }

}

static void gaf_mobile_audio_process_pcm_data(GAF_AUDIO_STREAM_ENV_T *_pStreamEnv, uint8_t *ptrBuf, uint32_t length)
{
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal microsecond -- 0.5 us
    POSSIBLY_UNUSED CODEC_CORE_INFO_T *coreInfo =
        &_pStreamEnv->stream_info.captureInfo.aseChInfo[0].codec_core_info;
    uint8_t *output_buf = _pStreamEnv->stream_context.capture_frame_cache;
    uint32_t frame_len = (uint32_t)(_pStreamEnv->stream_info.captureInfo.codec_info.frame_size);
    int32_t channelNum = _pStreamEnv->stream_info.captureInfo.dma_info.num_channels;

    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }
    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((!_pStreamEnv) ||
        (GAF_CAPTURE_STREAM_START_TRIGGERING > _pStreamEnv->stream_context.capture_stream_state) ||
        ((GAF_CAPTURE_STREAM_STREAMING_TRIGGERED == _pStreamEnv->stream_context.capture_stream_state) &&
        (dmaIrqHappeningTimeUs == _pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs)))
    {
        LOG_W("accumulated irq messages happen!");
        memset(ptrBuf, 0x0, length);
        return;
    }
    if (GAF_CAPTURE_STREAM_STREAMING_TRIGGERED != _pStreamEnv->stream_context.capture_stream_state)
    {
        gaf_stream_common_update_capture_stream_state(_pStreamEnv,
                                                     GAF_CAPTURE_STREAM_STREAMING_TRIGGERED);
        gaf_stream_common_clr_trigger(_pStreamEnv->stream_context.captureTriggerChannel);
    }
    gaf_stream_common_capture_timestamp_checker(_pStreamEnv, dmaIrqHappeningTimeUs);
    dmaIrqHappeningTimeUs += (uint32_t)_pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs;
    LOG_D("length %d encoded_len %d filled timestamp %u", length,
        _pStreamEnv->stream_info.captureInfo.codec_info.frame_size, dmaIrqHappeningTimeUs);

    /// gaf custom, may be a watch point to add some pattern in pcm data to be encoded
    if (gaf_uc_cli_custom_data_callback->raw_pcm_data_cb)
    {
        gaf_uc_cli_custom_data_callback->raw_pcm_data_cb(ptrBuf, length);
    }

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (_pStreamEnv->stream_context.isUpStreamingStarted)
    {
        gaf_stream_common_fetch_frame(_pStreamEnv, output_buf,
            frame_len * channelNum, dmaIrqHappeningTimeUs, coreInfo->instance_handle);
        gaf_mobile_audio_process_pcm_data_send(_pStreamEnv,
            output_buf, frame_len * channelNum, dmaIrqHappeningTimeUs);
    }
    gaf_stream_common_store_pcm(_pStreamEnv, dmaIrqHappeningTimeUs,
            ptrBuf, length, frame_len * channelNum, coreInfo);
#else
    _pStreamEnv->func_list->encoder_func_list->encoder_encode_frame_func(0,
        &(_pStreamEnv->stream_info.captureInfo.codec_info), length, ptrBuf, frame_len, output_buf);

    gaf_mobile_audio_process_pcm_data_send(_pStreamEnv,
            output_buf, frame_len * channelNum, dmaIrqHappeningTimeUs);
#endif
}

static void gaf_mobile_audio_process_encoded_data(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t *ptrBuf, uint32_t length)
{
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    POSSIBLY_UNUSED int32_t diff_bt_time = 0;
    GAF_AUDIO_STREAM_COMMON_INFO_T *playback_Info = &(pStreamEnv->stream_info.playbackInfo);

    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }
    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((!pStreamEnv) ||
        (GAF_PLAYBACK_STREAM_START_TRIGGERING > pStreamEnv->stream_context.playback_stream_state) ||
        ((GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED == pStreamEnv->stream_context.playback_stream_state) &&
        (dmaIrqHappeningTimeUs == pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs)))
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
        gaf_stream_common_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
        pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]--;
        LOG_I("Update playback seq to 0x%x", pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]);
    }

    /*******************************************stream decode frame*********************************************/
    int ret = 0;
    uint8_t instance_handle = 0;
    POSSIBLY_UNUSED uint32_t instance_status = 0;
    uint8_t connected_iso_num = 0;
    uint32_t sample_bytes = playback_Info->dma_info.bits_depth <= 16 ? 2 : 4;
    uint32_t sample_cnt = length / sample_bytes;
    uint8_t *pcm_cache = pStreamEnv->stream_context.playback_pcm_cache;

    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (playback_Info->aseChInfo[i].iso_channel_hdl == GAF_AUDIO_INVALID_ISO_CHANNEL)
        {
            continue;
        }

        instance_handle = playback_Info->aseChInfo[i].codec_core_info.instance_handle;
        instance_status = playback_Info->aseChInfo[i].codec_core_info.instance_status;
#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
        if (instance_status < INSTANCE_INITIALIZED)
        {
            continue;
        }
        ret = !gaf_stream_common_fetch_pcm(pStreamEnv,
            pcm_cache, length, instance_handle,
            dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, 0 == connected_iso_num);
#else
        instance_handle = connected_iso_num;
        CC_PLAYBACK_DATA_T *out_frame = (CC_PLAYBACK_DATA_T*)pStreamEnv->stream_context.playback_frame_cache;
        out_frame->data = pStreamEnv->stream_context.playback_frame_cache + sizeof(CC_PLAYBACK_DATA_T);

        /* use one iso data to do pid */
        gaf_stream_common_get_packet(pStreamEnv, out_frame, instance_handle,
            dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, 0 == connected_iso_num);

        ret = pStreamEnv->func_list->decoder_func_list->decoder_decode_frame_func
                  (instance_handle, &(playback_Info->codec_info), out_frame->data_len,
                  out_frame->data, pcm_cache, out_frame->isPLC);

        LOG_D("(%d) dec ret:%d frame_len:%d %d length %d", i, ret,
            out_frame->data_len, playback_Info->aseChInfo[i].iso_channel_hdl, length);
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
        pcm_cache += length;
    }

    pcm_cache = pStreamEnv->stream_context.playback_pcm_cache;
#if defined(AUDIO_STREAM_TEST_AUDIO_DUMP)
    audio_dump_add_channel_data_from_multi_channels_32bit_to_16bit(0 , pcm_cache, sample_cnt, 2, 0, 8);
    audio_dump_add_channel_data_from_multi_channels_32bit_to_16bit(1 , pcm_cache + length, sample_cnt, 2, 1, 8);
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

POSSIBLY_UNUSED static int gaf_mobile_audio_flexible_playback_stream_start_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    LOG_I("%s start", __func__);
    if (GAF_PLAYBACK_STREAM_INITIALIZING <= pStreamEnv->stream_context.playback_stream_state)
    {
        // TODO: shall use reasonable cpu frequency
        app_sysfreq_req(APP_SYSFREQ_USER_AOB, APP_SYSFREQ_208M);
        af_set_priority(AF_USER_AI, osPriorityHigh);
        struct AF_STREAM_CONFIG_T stream_cfg;
        memset((void *)&stream_cfg, 0, sizeof(struct AF_STREAM_CONFIG_T));
        stream_cfg.bits         = (enum AUD_BITS_T)(pStreamEnv->stream_info.playbackInfo.dma_info.bits_depth);
        stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)(pStreamEnv->stream_info.playbackInfo.dma_info.num_channels);
        stream_cfg.io_path      = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;

        if (stream_cfg.channel_num == AUD_CHANNEL_NUM_2)
        {
           stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);
        }

        stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)pStreamEnv->stream_info.playbackInfo.dma_info.sample_rate;
        // TODO: get vol from VCC via ase_lid
        stream_cfg.vol          = TGT_VOLUME_LEVEL_7;
        stream_cfg.data_size    = (uint32_t)(2 * pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize);
        pStreamEnv->func_list->stream_func_list.playback_init_stream_buf_func(pStreamEnv);
        if (pStreamEnv->func_list->decoder_func_list)
        {
            for (uint32_t instance_handle = 0; instance_handle < PLAYBACK_INSTANCE_MAX; instance_handle++)
            {
                pStreamEnv->func_list->decoder_func_list->decoder_init_func(
                    instance_handle, &(pStreamEnv->stream_info.playbackInfo.codec_info));
            }
        }
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr);
        stream_cfg.handler = pStreamEnv->func_list->stream_func_list.playback_dma_irq_handler_func;
        bes_ble_bap_dp_itf_data_come_callback_register((void *)gaf_mobile_audio_receive_data);
        af_codec_tune(AUD_STREAM_PLAYBACK, 0);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
#if defined(AUDIO_STREAM_TEST_AUDIO_DUMP)
        audio_dump_init(480, sizeof(short), 2);
#endif
        pStreamEnv->stream_context.playbackTriggerChannel = app_bt_sync_get_avaliable_trigger_channel();
        TRACE(0,"%s __LINE__ %d pbtrigerCh %d", __func__,__LINE__, pStreamEnv->stream_context.playbackTriggerChannel);
        gaf_media_prepare_playback_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
        // put PID env into stream context
        gaf_media_pid_init(&(pStreamEnv->stream_context.playback_pid_env));
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);
        return 0;
    }
    return -1;
}

static void gaf_mobile_audio_flexible_common_buf_init(GAF_AUDIO_STREAM_ENV_T * pStreamEnv, GAF_STREAM_TYPE_E stream_type)
{
    uint32_t lock = int_lock();
    if ((pStreamEnv->stream_context.playback_stream_state <= GAF_PLAYBACK_STREAM_INITIALIZING) &&
        (pStreamEnv->stream_context.capture_stream_state <= GAF_CAPTURE_STREAM_INITIALIZING))
    {
        pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr = NULL;
        pStreamEnv->stream_info.captureInfo.dma_info.storedDmaBufPtr = NULL;
        pStreamEnv->stream_context.capture_frame_cache = NULL;
        pStreamEnv->stream_context.playback_frame_cache = NULL;
        pStreamEnv->stream_context.playback_pcm_cache = NULL;
#if !defined (GAF_CODEC_CROSS_CORE) && !defined (AOB_CODEC_CP)
        for (uint32_t instance = 0; instance < PLAYBACK_INSTANCE_MAX; instance++) {
            pStreamEnv->stream_context.playback_frame_buf[instance] = NULL;
            pStreamEnv->stream_context.playback_frame_fifo[instance] = NULL;
        }
#endif
#ifndef BLE_I2S_AUDIO_SUPPORT
        app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
#endif
        uint32_t audioCacheHeapSize = 0;
        uint8_t* heapBufStartAddr = NULL;
        if (GAF_STREAM_PLAYBACK == stream_type)
        {
            audioCacheHeapSize = pStreamEnv->stream_info.playbackInfo.maxCachedFrameCount*
                pStreamEnv->stream_info.playbackInfo.maxFrameSize;
        }
        else
        {
            audioCacheHeapSize = pStreamEnv->stream_info.captureInfo.maxCachedFrameCount*
                pStreamEnv->stream_info.captureInfo.maxFrameSize;
        }
        app_audio_mempool_get_buff(&heapBufStartAddr, audioCacheHeapSize);
        gaf_stream_heap_init(heapBufStartAddr, audioCacheHeapSize);
    }
    if (GAF_STREAM_PLAYBACK == stream_type)
    {
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_BUF_INITIALIZED);
    }
    else
    {
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_BUF_INITIALIZED);
    }
    int_unlock(lock);
}

static void gaf_mobile_audio_flexible_playback_buf_init(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    gaf_mobile_audio_flexible_common_buf_init(pStreamEnv, GAF_STREAM_PLAYBACK);

#if !defined (GAF_CODEC_CROSS_CORE) && !defined (AOB_CODEC_CP)
    uint32_t frame_size = pStreamEnv->stream_info.playbackInfo.codec_info.frame_size *
        pStreamEnv->stream_info.playbackInfo.codec_info.num_channels +  sizeof(CC_PLAYBACK_DATA_T);
    uint32_t frame_buf_size = pStreamEnv->stream_info.playbackInfo.maxCachedFrameCount * frame_size;
    if (NULL == pStreamEnv->stream_context.playback_frame_cache)
    {
        app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_frame_cache), frame_size);
    }

    if (NULL == pStreamEnv->stream_context.playback_frame_fifo[0])
    {
        app_audio_mempool_get_buff(
            (uint8_t**)&(pStreamEnv->stream_context.playback_frame_fifo[0]), sizeof(cfifo));
    }
    if (NULL == pStreamEnv->stream_context.playback_frame_buf[0])
    {
        app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_frame_buf[0]), frame_buf_size);
    }
    if (NULL == pStreamEnv->stream_context.playback_frame_fifo[1])
    {
        app_audio_mempool_get_buff(
            (uint8_t**)&(pStreamEnv->stream_context.playback_frame_fifo[1]), sizeof(cfifo));
    }
    if (NULL == pStreamEnv->stream_context.playback_frame_buf[1])
    {
        app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_frame_buf[1]), frame_buf_size);
    }

    cfifo_init(pStreamEnv->stream_context.playback_frame_fifo[0],
        pStreamEnv->stream_context.playback_frame_buf[0], frame_buf_size);
    cfifo_init(pStreamEnv->stream_context.playback_frame_fifo[1],
        pStreamEnv->stream_context.playback_frame_buf[1], frame_buf_size);
#endif

    uint32_t pcm_buf_size = pStreamEnv->stream_info.playbackInfo.codec_info.pcm_size * PLAYBACK_INSTANCE_MAX;
    if (NULL == pStreamEnv->stream_context.playback_pcm_cache)
    {
        app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_pcm_cache), pcm_buf_size);
    }

    if (NULL == pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr)
    {
        app_audio_mempool_get_buff(
        &(pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr),
            pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize * 2);
    }
    pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr = pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr;

    if (pStreamEnv->func_list->decoder_func_list)
    {
        for (uint32_t instance_handle = 0; instance_handle < 2; instance_handle++)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_init_buf_func(instance_handle,
                &(pStreamEnv->stream_info.playbackInfo.codec_info), (void*)gaf_stream_common_buf_alloc);
        }
    }

    LOG_D("%s end", __func__);
}

POSSIBLY_UNUSED static int gaf_mobile_audio_flexible_playback_stream_stop_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_IDLE);
    bes_ble_bap_dp_itf_data_come_callback_deregister();
    af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
    }
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_codec_tune(AUD_STREAM_PLAYBACK, 0);
    gaf_stream_common_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
    app_sysfreq_req(APP_SYSFREQ_USER_AOB, APP_SYSFREQ_32K);
    af_set_priority(AF_USER_AI, osPriorityAboveNormal);
    bes_ble_bap_dp_rx_iso_stop(pStreamEnv->stream_info.playbackInfo.aseChInfo[0].ase_handle);
    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_deinit_func(0);
    }
    pStreamEnv->func_list->stream_func_list.playback_deinit_stream_buf_func(pStreamEnv);
    return 0;
}

static uint32_t gaf_mobile_stream_flexible_playback_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_mobile_audio_stream_env;
    gaf_mobile_audio_process_encoded_data(pStreamEnv, ptrBuf, length);
    LOG_D("%s end", __func__);
    return length;
}

static void gaf_mobile_audio_flexible_playback_buf_deinit(void* _pStreamEnv)
{
    LOG_I("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr = NULL;
    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_deinit_buf_func(0);
    }
    LOG_I("%s end", __func__);
}

POSSIBLY_UNUSED static int gaf_mobile_audio_flexible_capture_stream_start_handler(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    if (GAF_CAPTURE_STREAM_INITIALIZING <= pStreamEnv->stream_context.capture_stream_state)
    {
        // TODO: shall use reasonable cpu frequency
#ifdef AOB_UC_TEST
        app_sysfreq_req(APP_SYSFREQ_USER_AOB, (enum APP_SYSFREQ_FREQ_T)mobile_freq);
#else
        app_sysfreq_req(APP_SYSFREQ_USER_AOB, APP_SYSFREQ_208M);
#endif
        af_set_priority(AF_USER_AI, osPriorityHigh);
        struct AF_STREAM_CONFIG_T stream_cfg;
        // capture stream
        memset((void *)&stream_cfg, 0, sizeof(struct AF_STREAM_CONFIG_T));
        stream_cfg.bits         = (enum AUD_BITS_T)(pStreamEnv->stream_info.captureInfo.dma_info.bits_depth);
        stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)(pStreamEnv->stream_info.captureInfo.dma_info.num_channels);
        stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0|AUD_CHANNEL_MAP_CH1);
        // TODO: changed to mic
        stream_cfg.io_path      = AUD_INPUT_PATH_LINEIN;
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)pStreamEnv->stream_info.captureInfo.dma_info.sample_rate;
        // TODO: get vol from VCC via ase_lid
        stream_cfg.vol          = TGT_VOLUME_LEVEL_7;
        stream_cfg.data_size    = (uint32_t)(2 * pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkSize);
        pStreamEnv->func_list->stream_func_list.capture_init_stream_buf_func(pStreamEnv);
        if (pStreamEnv->func_list->encoder_func_list)
        {
            pStreamEnv->func_list->encoder_func_list->encoder_init_func(
                        0, &(pStreamEnv->stream_info.captureInfo.codec_info));
        }

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        gaf_capture_encoder_init(pStreamEnv);
#endif
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr);
        stream_cfg.handler = pStreamEnv->func_list->stream_func_list.capture_dma_irq_handler_func;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        pStreamEnv->stream_context.captureTriggerChannel = app_bt_sync_get_avaliable_trigger_channel();
        gaf_media_prepare_capture_trigger(pStreamEnv->stream_context.captureTriggerChannel);
        gaf_media_pid_init(&(pStreamEnv->stream_context.capture_pid_env));
        gaf_media_pid_update_threshold(&(pStreamEnv->stream_context.capture_pid_env),
            pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2);

        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);
        if (GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM  ==
            gaf_cis_mobile_stream_type_op_rule_all.trigger_stream_type)
        {
            uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
            uint32_t current_bt_time = gaf_media_sync_get_curr_time();
            uint32_t trigger_bt_time = latest_iso_bt_time+(pStreamEnv->stream_info.captureInfo.cigSyncDelayUs/pStreamEnv->stream_info.captureInfo.bnM2S/2);
            // move ahead of trigger time by 1ms to leave more margin for long CIG delay
            if (pStreamEnv->stream_info.captureInfo.cigSyncDelayUs > 3000)
            {
                trigger_bt_time -= 1000;
            }

            while (trigger_bt_time < current_bt_time + GAF_AUDIO_MAX_DIFF_BT_TIME)
            {
                trigger_bt_time += pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs;
            }

            LOG_I("iso anch %d cur time %d trigger time %d",
                latest_iso_bt_time, current_bt_time, trigger_bt_time);
            gaf_stream_common_set_capture_trigger_time(pStreamEnv, trigger_bt_time);
        }
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        return 0;
    }
    return -1;
}

static void gaf_mobile_audio_flexible_capture_buf_init(void* _pStreamEnv)
{
    LOG_I("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    gaf_mobile_audio_flexible_common_buf_init(pStreamEnv, GAF_STREAM_CAPTURE);

    uint32_t frame_len = pStreamEnv->stream_info.captureInfo.codec_info.frame_size *
            pStreamEnv->stream_info.captureInfo.codec_info.num_channels;
    if (NULL == pStreamEnv->stream_context.capture_frame_cache)
    {
        app_audio_mempool_get_buff(&(pStreamEnv->stream_context.capture_frame_cache), frame_len);
    }

    if (NULL == pStreamEnv->stream_info.captureInfo.dma_info.storedDmaBufPtr)
    {
        app_audio_mempool_get_buff(&(pStreamEnv->stream_info.captureInfo.dma_info.storedDmaBufPtr),
                        pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkSize*2);
    }
    pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr = pStreamEnv->stream_info.captureInfo.dma_info.storedDmaBufPtr;

    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_init_buf_func(0,
            &(pStreamEnv->stream_info.captureInfo.codec_info), (void*)gaf_stream_common_buf_alloc);
    }
    LOG_I("%s end", __func__);
}

POSSIBLY_UNUSED static int gaf_mobile_audio_flexible_capture_stream_stop_handler(void* _pStreamEnv)
{
    LOG_I("%s stop", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.capture_retrigger_onprocess;
    gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_IDLE);
    af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
    }
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    gaf_stream_common_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);
    app_sysfreq_req(APP_SYSFREQ_USER_AOB, APP_SYSFREQ_32K);
    af_set_priority(AF_USER_AI, osPriorityAboveNormal);
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    gaf_capture_encoder_deinit(pStreamEnv, isRetrigger);
#endif
    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_deinit_func(0);
    }
    pStreamEnv->func_list->stream_func_list.capture_deinit_stream_buf_func(pStreamEnv);
    return 0;
}

static uint32_t gaf_mobile_stream_flexible_capture_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_mobile_audio_stream_env;
    gaf_mobile_audio_process_pcm_data(pStreamEnv, ptrBuf, length);
    return length;
}
static void gaf_mobile_audio_flexible_capture_buf_deinit(void* _pStreamEnv)
{
    LOG_I("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr = NULL;

    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_deinit_buf_func(0);
    }

    LOG_I("%s end", __func__);
}
static GAF_AUDIO_FUNC_LIST_T gaf_mobile_audio_flexible_stream_func_list =
{
    {
        .playback_dma_irq_handler_func = gaf_mobile_stream_flexible_playback_dma_irq_handler,
        .capture_dma_irq_handler_func = gaf_mobile_stream_flexible_capture_dma_irq_handler,
#if defined(BLE_USB_AUDIO_SUPPORT)&&defined(BLE_USB_AUDIO_IS_DONGLE_ROLE)
        .playback_start_stream_func = gaf_mobile_usb_audio_capture_start_handler,
        .playback_init_stream_buf_func = gaf_mobile_audio_flexible_playback_buf_init,
        .playback_stop_stream_func = gaf_mobile_usb_audio_capture_stop_handler,
#elif defined(BLE_I2S_AUDIO_SUPPORT)&&defined(BLE_USB_AUDIO_IS_DONGLE_ROLE)
        .playback_start_stream_func = gaf_mobile_i2s_audio_capture_start_handler,
        .playback_init_stream_buf_func = gaf_mobile_audio_flexible_playback_buf_init,
        .playback_stop_stream_func = gaf_mobile_i2s_audio_capture_stop_handler,
#else
        .playback_start_stream_func = gaf_mobile_audio_flexible_playback_stream_start_handler,
        .playback_init_stream_buf_func = gaf_mobile_audio_flexible_playback_buf_init,
        .playback_stop_stream_func = gaf_mobile_audio_flexible_playback_stream_stop_handler,
#endif
        .playback_deinit_stream_buf_func = gaf_mobile_audio_flexible_playback_buf_deinit,
#if defined(BLE_USB_AUDIO_SUPPORT)&&defined(BLE_USB_AUDIO_IS_DONGLE_ROLE)
        .capture_start_stream_func = gaf_mobile_usb_audio_media_stream_start_handler,
        .capture_init_stream_buf_func = gaf_mobile_audio_flexible_capture_buf_init,
        .capture_stop_stream_func = gaf_mobile_usb_audio_media_stream_stop_handler,
#elif defined(BLE_I2S_AUDIO_SUPPORT)&&defined(BLE_USB_AUDIO_IS_DONGLE_ROLE)
        .capture_start_stream_func = gaf_mobile_i2s_audio_media_stream_start_handler,
        .capture_init_stream_buf_func = gaf_mobile_audio_flexible_capture_buf_init,
        .capture_stop_stream_func = gaf_mobile_i2s_audio_media_stream_stop_handler,
#else
        .capture_start_stream_func = gaf_mobile_audio_flexible_capture_stream_start_handler,
        .capture_init_stream_buf_func = gaf_mobile_audio_flexible_capture_buf_init,
        .capture_stop_stream_func = gaf_mobile_audio_flexible_capture_stream_stop_handler,
#endif
        .capture_deinit_stream_buf_func = gaf_mobile_audio_flexible_capture_buf_deinit,
    },
};

static GAF_AUDIO_STREAM_ENV_T* gaf_mobile_audio_get_stream_env_from_ase(uint8_t ase_lid)
{
    return &gaf_mobile_audio_stream_env;
}

static GAF_AUDIO_STREAM_ENV_T* gaf_mobile_audio_refresh_stream_info_from_ase(uint8_t ase_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_mobile_audio_get_stream_env_from_ase(ase_lid);
    const bes_ble_bap_ascc_ase_t *p_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);
    GAF_AUDIO_STREAM_COMMON_INFO_T* pCommonInfo;

    if (p_ase_info == NULL)
    {
        LOG_W("%s ASE info is NULL!!!", __func__);
        return pStreamEnv;
    }

    /*
    * Direction is defined from the perspective of the ASE server.
    * So for mobile phone, sink is capture stream.
    */
    if (BES_BLE_GAF_DIRECTION_SINK == p_ase_info->direction)
    {
        pCommonInfo = &(pStreamEnv->stream_info.captureInfo);
        /// should always use stereo channel for mobile capture
        pCommonInfo->codec_info.num_channels = GAF_MOBILE_AUDIO_STREAM_CAPTURE_CHANNEL_NUM;
    }
    else
    {
        pCommonInfo = &(pStreamEnv->stream_info.playbackInfo);
        pCommonInfo->presDelayUs = p_ase_info->qos_cfg.pres_delay_us > GAF_AUDIO_FIXED_MOBILE_SIDE_PRESENT_LATENCY_US ?
                                   p_ase_info->qos_cfg.pres_delay_us : GAF_AUDIO_FIXED_MOBILE_SIDE_PRESENT_LATENCY_US;
        /// depend on uc srv capablity
        pCommonInfo->codec_info.num_channels =
        app_bap_capa_cli_is_peer_support_stereo_channel(p_ase_info->con_lid, BES_BLE_GAF_DIRECTION_SRC) ?
                                    AUD_CHANNEL_NUM_2 : AUD_CHANNEL_NUM_1;
        if (GAF_MOBILE_AUDIO_STREAM_PLAYBACK_CHANNEL_NUM < pCommonInfo->codec_info.num_channels)
        {
            LOG_E("%s unsupport channel num %d", __func__, pCommonInfo->codec_info.num_channels);
        }
    }
    // cig-cis timing info
    pCommonInfo->cigSyncDelayUs = p_ase_info->cig_sync_delay;
    pCommonInfo->isoIntervalUs = p_ase_info->iso_interval_us;
    pCommonInfo->bnM2S = p_ase_info->bn_m2s;
    pCommonInfo->bnS2M = p_ase_info->bn_s2m;
    // codec info
    pCommonInfo->codec_info.bits_depth = GAF_MOBILE_AUDIO_STREAM_BIT_NUM;
    pCommonInfo->aseChInfo[p_ase_info->ase_lid].allocation_bf = p_ase_info->p_cfg->param.location_bf;
    LOG_I("refresh: ase_lid %d direction:%d codec_id:%d con_lid %d ase_id:%d",
        p_ase_info->ase_lid, p_ase_info->direction, p_ase_info->codec_id.codec_id[0], p_ase_info->con_lid, ase_lid);

    LOG_I("cig sync delay %d us - iso interval %d us - presDelayUs:%d - bnM2S %d - bnS2M %d",
        pCommonInfo->cigSyncDelayUs, pCommonInfo->isoIntervalUs, pCommonInfo->presDelayUs, pCommonInfo->bnM2S,
        pCommonInfo->bnS2M);
    switch (p_ase_info->codec_id.codec_id[0])
    {
        case BES_BLE_GAF_CODEC_TYPE_LC3:
        {
            AOB_BAP_CFG_T* p_lc3_cfg = (AOB_BAP_CFG_T *)p_ase_info->p_cfg;
            pCommonInfo->codec_info.frame_ms =
                gaf_stream_common_frame_duration_parse(p_lc3_cfg->param.frame_dur);
            pCommonInfo->codec_info.sample_rate =
                gaf_stream_common_sample_freq_parse(p_lc3_cfg->param.sampling_freq);
            pCommonInfo->codec_info.frame_size = p_lc3_cfg->param.frame_octet;
            pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER;
            pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size();
            pStreamEnv->stream_info.codec_type = LC3;
#if !defined(GAF_CODEC_CROSS_CORE) && !defined(AOB_CODEC_CP)
            gaf_audio_lc3_update_decoder_func_list(&(pStreamEnv->func_list->decoder_func_list));
            gaf_audio_lc3_update_encoder_func_list(&(pStreamEnv->func_list->encoder_func_list));
#endif
            break;
        }
#ifdef LC3PLUS_SUPPORT
        case BES_BLE_GAF_CODEC_TYPE_VENDOR:
        {
            AOB_BAP_CFG_T* p_lc3plus_cfg = p_ase_info->p_cfg;
            pCommonInfo->codec_info.frame_ms =
                gaf_stream_common_frame_duration_parse(p_lc3plus_cfg->param.frame_dur);
            pCommonInfo->codec_info.sample_rate =
                gaf_stream_common_sample_freq_parse(p_lc3plus_cfg->param.sampling_freq);
            pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER;
            pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size();
            pCommonInfo->codec_info.frame_size = p_lc3plus_cfg->param.frame_octet;
            if (2.5 == pCommonInfo->codec_info.frame_ms)
            {
                pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER*2;
                pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size()/2;
            }
            pStreamEnv->stream_info.codec_type = LC3PLUS;
#if !defined(GAF_CODEC_CROSS_CORE) && !defined(AOB_CODEC_CP)
            gaf_audio_lc3_update_decoder_func_list(&(pStreamEnv->func_list->decoder_func_list));
            gaf_audio_lc3_update_encoder_func_list(&(pStreamEnv->func_list->encoder_func_list));
#endif
            break;
        }
#endif
#ifdef HID_ULL_ENABLE
        case BES_BLE_GAF_CODEC_TYPE_ULL:
        {
            AOB_BAP_CFG_T* p_ull_cfg = p_ase_info->p_cfg;
            pCommonInfo->codec_info.frame_ms =
                gaf_stream_common_frame_duration_parse(p_ull_cfg->param.frame_dur);
            pCommonInfo->codec_info.sample_rate =
                gaf_stream_common_sample_freq_parse(p_ull_cfg->param.sampling_freq);
#if BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 1
            pCommonInfo->codec_info.frame_size = p_ull_cfg->param.frame_octet;
#elif BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2
            pCommonInfo->codec_info.frame_size = p_ull_cfg->param.frame_octet/2; // Will mul 2 chnl at ull encode in mobile role
#else
            pCommonInfo->codec_info.frame_size = p_ull_cfg->param.frame_octet;
#endif
            pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER;
            pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size();
            pStreamEnv->stream_info.codec_type = LC3;
#if !defined(GAF_CODEC_CROSS_CORE) && !defined(AOB_CODEC_CP)
            gaf_audio_lc3_update_decoder_func_list(&(pStreamEnv->func_list->decoder_func_list));
            gaf_audio_lc3_update_encoder_func_list(&(pStreamEnv->func_list->encoder_func_list));
#endif
            break;
        }
#endif
        default:
            ASSERT(false, "unknown codec type!");
            return NULL;
    }

    uint32_t sample_bytes = pCommonInfo->codec_info.bits_depth/8;
    if (24 == pCommonInfo->codec_info.bits_depth) {
        sample_bytes = 4;
    }

    pCommonInfo->codec_info.pcm_size =
        (uint32_t)(((pCommonInfo->codec_info.frame_ms * 1000)*
        pCommonInfo->codec_info.sample_rate * sample_bytes *
        pCommonInfo->codec_info.num_channels) / (1000 * 1000));

    pCommonInfo->dma_info.bits_depth = pCommonInfo->codec_info.bits_depth;
    pCommonInfo->dma_info.num_channels = pCommonInfo->codec_info.num_channels;
    pCommonInfo->dma_info.frame_ms = pCommonInfo->codec_info.frame_ms;
    pCommonInfo->dma_info.sample_rate = pCommonInfo->codec_info.sample_rate;
    pCommonInfo->dma_info.dmaChunkIntervalUs = (uint32_t)(pCommonInfo->dma_info.frame_ms*1000);
    pCommonInfo->dma_info.dmaChunkSize =
        (uint32_t)((pCommonInfo->dma_info.sample_rate*sample_bytes*
        (pCommonInfo->dma_info.dmaChunkIntervalUs)*
        pCommonInfo->dma_info.num_channels)/(1000*1000));

    pCommonInfo->aseChInfo[p_ase_info->ase_lid].iso_channel_hdl = BLE_ISOHDL_TO_ACTID(p_ase_info->cis_hdl);
    pCommonInfo->aseChInfo[p_ase_info->ase_lid].ase_handle = p_ase_info->cis_hdl;
    LOG_I("frame encode size:%d num channels:%d",
        pCommonInfo->codec_info.frame_size, pCommonInfo->codec_info.num_channels);
    LOG_I("cis handle 0x%x ,iso_channel_hdl %d", p_ase_info->cis_hdl,
        pCommonInfo->aseChInfo[p_ase_info->ase_lid].iso_channel_hdl);
    LOG_I("frame len %d us, sample rate %d dma chunk time %d us dma chunk size %d",
        (uint32_t)(pCommonInfo->dma_info.frame_ms*1000), pCommonInfo->dma_info.sample_rate,
        pCommonInfo->dma_info.dmaChunkIntervalUs, pCommonInfo->dma_info.dmaChunkSize);
    LOG_I("allocation bf: 0x%x", pCommonInfo->aseChInfo[p_ase_info->ase_lid].allocation_bf);
    LOG_I("codec: %s", gaf_stream_common_print_code_type(p_ase_info->codec_id.codec_id[0]));
    LOG_I("context: %s", gaf_stream_common_print_context(p_ase_info->p_metadata->param.context_bf));
    return pStreamEnv;
}

static uint8_t gaf_mobile_audio_get_enabled_playback_ase_count(void)
{
    uint8_t enabled_ase_count = 0;
    for (uint8_t con_id = 0; con_id < GAF_MOB_MAXIMUM_CONNECTION_COUNT; con_id++)
    {
        for (uint8_t idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
        {
           enabled_ase_count +=
            (gaf_cis_mobile_media_dwelling_info[con_id].playback_ase_id[idx] !=
                GAF_INVALID_ASE_INDEX);
        }
    }
    return enabled_ase_count;
}

static uint8_t gaf_mobile_audio_get_enabled_capture_ase_count(void)
{
    uint8_t enabled_ase_count = 0;
    for (uint8_t con_id = 0;con_id < GAF_MOB_MAXIMUM_CONNECTION_COUNT;con_id++)
    {
        for (uint8_t idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
        {
           enabled_ase_count +=
            (gaf_cis_mobile_media_dwelling_info[con_id].capture_ase_id[idx] !=
                GAF_INVALID_ASE_INDEX);
        }
    }
    return enabled_ase_count;
}

static gaf_stream_context_state_t gaf_mobile_audio_update_stream_info_from_ase(
                                GAF_AUDIO_UPDATE_STREAM_INFO_PURPOSE_E purpose, uint8_t ase_lid, uint8_t con_lid)
{
    uint8_t enabled_playback_ase_cnt = 0;
    uint8_t enabled_capture_ase_cnt = 0;
    uint8_t gaf_connected_dev_num = 0;
    const bes_ble_bap_ascc_ase_t *p_bap_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);

    GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo = &gaf_cis_mobile_media_dwelling_info[con_lid];
    /*
    * Direction is defined from the perspective of the ASE server.
    * So for mobile phone, sink is capture stream.
    */
    LOG_I("update stream ase %d purpose %d", ase_lid, purpose);

    if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction)
    {
        if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_START == purpose)
        {
            gaf_mobile_audio_add_ase_into_capture_ase_list(p_bap_ase_info->con_lid, ase_lid);
            pDewellingInfo->startedStreamTypes |= GAF_AUDIO_STREAM_TYPE_CAPTURE;
        }
        else if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_STOP == purpose)
        {
            gaf_mobile_audio_remove_ase_from_capture_ase_list(p_bap_ase_info->con_lid, ase_lid);
            pDewellingInfo->startedStreamTypes &= (~GAF_AUDIO_STREAM_TYPE_CAPTURE);
        }
    }
    else
    {
        if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_START == purpose)
        {
            gaf_mobile_audio_add_ase_into_playback_ase_list(p_bap_ase_info->con_lid, ase_lid);
            pDewellingInfo->startedStreamTypes |= GAF_AUDIO_STREAM_TYPE_PLAYBACK;
        }
        else if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_STOP == purpose)
        {
            gaf_mobile_audio_remove_ase_from_playback_ase_list(p_bap_ase_info->con_lid, ase_lid);
            pDewellingInfo->startedStreamTypes &= (~GAF_AUDIO_STREAM_TYPE_PLAYBACK);
        }
    }

    enabled_playback_ase_cnt = gaf_mobile_audio_get_enabled_playback_ase_count();
    enabled_capture_ase_cnt = gaf_mobile_audio_get_enabled_capture_ase_count();
    gaf_connected_dev_num = gaf_mobile_get_connected_device_count();

    LOG_I("direction %d started stream types updated to %d", p_bap_ase_info->direction,
        pDewellingInfo->startedStreamTypes);

    if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_START == purpose)
    {
        return APP_GAF_CONTEXT_STREAM_STARTED;
    }
    else
    {
        if ((0 == enabled_playback_ase_cnt)
            && (0 == enabled_capture_ase_cnt)
            && (0 == pDewellingInfo->startedStreamTypes))
        {
            return APP_GAF_CONTEXT_ALL_STREAMS_STOPPED;
        }
        else
        {
            return APP_GAF_CONTEXT_SINGLE_STREAM_STOPPED;
        }
    }

    LOG_I("enabled playback/capture ase cnt: %d/%d, gaf connected dev cnt: %d",
            enabled_playback_ase_cnt, enabled_capture_ase_cnt, gaf_connected_dev_num);
}

static bool gaf_mobile_audio_is_all_used_ase_streaming(uint8_t direction)
{
    uint8_t capture_ase_used_cnt = gaf_mobile_audio_get_enabled_capture_ase_count();

    uint8_t nb_ase = 0;
    uint8_t ase_lid_list[GAF_AUDIO_ASE_TOTAL_COUNT] = {0};
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        nb_ase = app_bap_uc_cli_get_specific_state_ase_lid_list(
                            i, direction, APP_GAF_BAP_UC_ASE_STATE_ENABLING, ase_lid_list);
        if (nb_ase != 0)
        {
            LOG_I("Wait all ase enter streaming to send iso together, %d/%d",
                                                        nb_ase, capture_ase_used_cnt);
            return false;
        }
    }

    return true;
}

static void _gaf_mobile_audio_stream_start(uint8_t con_lid, uint8_t ase_lid)
{
#ifdef GAF_DSP
    app_dsp_hifi4_init(APP_DSP_HIFI4_USER_GAF_DSP);
#endif
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_mobile_audio_refresh_stream_info_from_ase(ase_lid);
    GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo = &gaf_cis_mobile_media_dwelling_info[con_lid];

    if (pStreamEnv)
    {
        LOG_I("%s both connected %d streaming %d/%d state %d/%d con_lid:%d ase_lid:%d", __func__,
                ble_audio_mobile_both_is_connected(),
                gaf_mobile_audio_is_all_used_ase_streaming((uint8_t)BES_BLE_GAF_DIRECTION_SINK),
                gaf_mobile_audio_is_all_used_ase_streaming((uint8_t)BES_BLE_GAF_DIRECTION_SRC),
                pStreamEnv->stream_context.capture_stream_state,
                pStreamEnv->stream_context.playback_stream_state,
                con_lid, ase_lid);

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        const bes_ble_bap_ascc_ase_t *p_bap_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);
        if (p_bap_ase_info && BES_BLE_GAF_DIRECTION_SRC == p_bap_ase_info->direction)
        {
            gaf_playback_decoder_init(pStreamEnv, ase_lid);
        }
#endif
        if ((pDewellingInfo->startedStreamTypes&GAF_AUDIO_STREAM_TYPE_PLAYBACK) &&
            (GAF_PLAYBACK_STREAM_IDLE == pStreamEnv->stream_context.playback_stream_state))
        {
            if (gaf_mobile_audio_is_all_used_ase_streaming(BES_BLE_GAF_DIRECTION_SRC))
            {
                gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZING);
                pStreamEnv->func_list->stream_func_list.playback_start_stream_func(pStreamEnv);
            }
        }

        if ((pDewellingInfo->startedStreamTypes&GAF_AUDIO_STREAM_TYPE_CAPTURE) &&
            (GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state))
        {
            if (gaf_mobile_audio_is_all_used_ase_streaming(BES_BLE_GAF_DIRECTION_SINK))
            {
                gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZING);
                pStreamEnv->func_list->stream_func_list.capture_start_stream_func(pStreamEnv);
            }
        }
    }
}

// TODO: to be registered to media manager module
static void gaf_mobile_audio_stream_start(uint8_t con_lid, uint8_t ase_lid)
{
    bt_adapter_write_sleep_enable(0); // Current thread is BT thread
    app_bt_start_custom_function_in_bt_thread((uint32_t)con_lid,
                                              (uint32_t)ase_lid,
                                              (uint32_t)_gaf_mobile_audio_stream_start);
}

#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
void gaf_mobile_sensor_receive_data(uint16_t conhdl, GAF_ISO_PKT_STATUS_E pkt_status)
{
    gaf_media_data_t *p_sdu_buf = NULL;
    gaf_mobile_sensor_data_t sensor_frame_info = {0};

    while ((p_sdu_buf = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
    {
        sensor_frame_info.time_stamp = p_sdu_buf->time_stamp;
        sensor_frame_info.pkt_seq_nb = p_sdu_buf->pkt_seq_nb;
        sensor_frame_info.pkt_status = (GAF_ISO_PKT_STATUS_E)p_sdu_buf->pkt_status;
        sensor_frame_info.data_len = p_sdu_buf->data_len;

        if (((GAF_ISO_PKT_STATUS_VALID == sensor_frame_info.pkt_status) &&(sensor_frame_info.data_len > 0)))
        {
            LOG_D("%s time_stamp:%d seq_numb:%d data_len:%d", __func__, sensor_frame_info.time_stamp, sensor_frame_info.pkt_seq_nb, sensor_frame_info.data_len);
            LOG_D("mobile_received_data:");
            DUMP8("%02x ", p_sdu_buf->sdu_data, sensor_frame_info.data_len);
        }
        app_bap_dp_itf_rx_data_done(conhdl, p_sdu_buf->data_len, p_sdu_buf->time_stamp, p_sdu_buf->sdu_data);
        gaf_stream_data_free(p_sdu_buf);
    }
}

static void gaf_mobile_simulated_sensor_data_upstream_start(void)
{
    LOG_D("enter_sensor_stream_start_handler!");

    /// register iso data callback api
    bes_ble_bap_dp_itf_data_come_callback_register((void *)gaf_mobile_sensor_receive_data);

    /// keep bt_sleep funvtion close
    btif_me_write_bt_sleep_enable(0);

    /// add the system frequence
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_26M);
}

static void gaf_mobile_simulated_sensor_data_upstream_stop(void)
{
    btif_me_write_bt_sleep_enable(1);
    bes_ble_bap_dp_itf_data_come_callback_deregister();
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_32K);
}
#endif

static void _gaf_mobile_audio_stream_start_handler(uint8_t ase_lid, uint8_t con_lid)
{
    gaf_stream_context_state_t updatedContextStreamState =
        gaf_mobile_audio_update_stream_info_from_ase(GAF_AUDIO_UPDATE_STREAM_INFO_TO_START, ase_lid, con_lid);
    if (APP_GAF_CONTEXT_STREAM_STARTED == updatedContextStreamState)
    {

#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
        /// enter sensor_data process
        const bes_ble_bap_ascc_ase_t *p_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);
        if ((BES_BLE_GAF_DIRECTION_SRC == p_ase_info->direction) && (p_ase_info->p_cfg->param.frame_octet == 14))
        {
            GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_mobile_audio_stream_env;
            pStreamEnv->stream_info.playbackInfo.aseChInfo[0].iso_channel_hdl = BLE_ISOHDL_TO_ACTID(p_ase_info->cis_hdl);

            pStreamEnv->stream_info.playbackInfo.cigSyncDelayUs = p_ase_info->cig_sync_delay;
            pStreamEnv->stream_info.playbackInfo.isoIntervalUs = p_ase_info->iso_interval_us;
            pStreamEnv->stream_info.playbackInfo.bnM2S = p_ase_info->bn_m2s;
            pStreamEnv->stream_info.playbackInfo.bnS2M = p_ase_info->bn_s2m;
            pStreamEnv->stream_info.playbackInfo.dma_info.frame_ms =
                    gaf_stream_common_frame_duration_parse(p_ase_info->p_cfg->param.frame_dur);
            gaf_mobile_simulated_sensor_data_upstream_start();
            return ;
        }
#endif
        gaf_mobile_audio_stream_start(con_lid, ase_lid);
    }
}

void gaf_mobile_audio_stream_update_and_start_handler(uint8_t ase_lid, uint8_t con_lid)
{
    LOG_I("%s", __func__);
    gaf_media_stream_boost_freq(1500);
    app_bt_start_custom_function_in_bt_thread((uint32_t)ase_lid,
                                              (uint32_t)con_lid,
                                              (uint32_t)_gaf_mobile_audio_stream_start_handler);
}

static void gaf_mobile_audio_stream_stop(uint8_t con_lid, uint8_t ase_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_mobile_audio_get_stream_env_from_ase(ase_lid);

    // If all stream bond to stream env is stopped, we cannot get any stream env from,
    // refresh so use dft stream env to stop af thread, deinit decoder and used buf

    uint8_t enabled_playback_ase_cnt = gaf_mobile_audio_get_enabled_playback_ase_count();
    uint8_t enabled_capture_ase_cnt = gaf_mobile_audio_get_enabled_capture_ase_count();

    const bes_ble_bap_ascc_ase_t *p_bap_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);

    LOG_I("%s con_lid:%d ase_lid:%d", __func__, con_lid, ase_lid);
    LOG_D("enabled_playback_ase_cnt:%d enabled_capture_ase_cnt:%d",
        enabled_playback_ase_cnt, enabled_capture_ase_cnt);

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (p_bap_ase_info && BES_BLE_GAF_DIRECTION_SRC == p_bap_ase_info->direction)
    {
        gaf_playback_decoder_deinit_by_ase(pStreamEnv, ase_lid, pStreamEnv->stream_context.playback_retrigger_onprocess);
    }
#endif

    // Only both buds stop stream, dongle stop stream
    if ((0 == enabled_playback_ase_cnt) &&
        (GAF_PLAYBACK_STREAM_IDLE != pStreamEnv->stream_context.playback_stream_state))
    {
        pStreamEnv->func_list->stream_func_list.playback_stop_stream_func(pStreamEnv);
    }

    if (0 == enabled_capture_ase_cnt &&
        (GAF_CAPTURE_STREAM_IDLE != pStreamEnv->stream_context.capture_stream_state))
    {
        pStreamEnv->func_list->stream_func_list.capture_stop_stream_func(pStreamEnv);
    }

    if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction)
    {
        // only stop tx hdl that belong to specific connection
        bes_ble_bap_dp_tx_iso_stop(pStreamEnv->stream_info.captureInfo.aseChInfo[ase_lid].ase_handle);
        pStreamEnv->stream_info.captureInfo.aseChInfo[ase_lid].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
    }
    else
    {
        pStreamEnv->stream_info.playbackInfo.aseChInfo[ase_lid].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
    }

    if (pStreamEnv->stream_context.capture_stream_state == GAF_CAPTURE_STREAM_IDLE &&
        pStreamEnv->stream_context.playback_stream_state == GAF_PLAYBACK_STREAM_IDLE)
    {
        bt_adapter_write_sleep_enable(1); // Current thread is BT thread
    }
#ifdef GAF_DSP
    app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_GAF_DSP);
#endif
}

static void _gaf_mobile_audio_stream_stop_handler(uint8_t ase_lid, uint8_t con_lid)
{
    gaf_mobile_audio_update_stream_info_from_ase(GAF_AUDIO_UPDATE_STREAM_INFO_TO_STOP, ase_lid,con_lid);
#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
    const bes_ble_bap_ascc_ase_t *p_bap_ase_info = bes_ble_bap_ascc_get_ase_info(ase_lid);
    if ((p_bap_ase_info->direction == BES_BLE_GAF_DIRECTION_SRC) && (p_bap_ase_info->codec_id.codec_id[0] == 0xFF))
    {
        gaf_mobile_simulated_sensor_data_upstream_stop();
        return;
    }
#endif
    gaf_mobile_audio_stream_stop(con_lid, ase_lid);
}

void gaf_mobile_audio_stream_update_and_stop_handler(uint8_t ase_lid, uint8_t con_lid)
{
    LOG_I("%s, con_lid=%d", __func__, con_lid);
    gaf_media_stream_boost_freq(1500);
    app_bt_start_custom_function_in_bt_thread((uint32_t)ase_lid,
                                              (uint32_t)con_lid,
                                              (uint32_t)_gaf_mobile_audio_stream_stop_handler);
}

static void gaf_mobile_audio_steam_retrigger_handler(void* stream_env, uint32_t StreamType)
{
    GAF_MEDIA_DWELLING_INFO_T gaf_cis_mobile_media_dwelling_info_rec[GAF_MOB_MAXIMUM_CONNECTION_COUNT];
    // Record
    memcpy_s(gaf_cis_mobile_media_dwelling_info_rec,
                sizeof(gaf_cis_mobile_media_dwelling_info),
                gaf_cis_mobile_media_dwelling_info,
                sizeof(gaf_cis_mobile_media_dwelling_info));
    // Stop
    uint8_t *playback_ase_lid_list = NULL;
    uint8_t *capture_ase_lid_list = NULL;
    for (uint8_t con_lid = 0; con_lid < GAF_MOB_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        if (GAF_AUDIO_STREAM_TYPE_PLAYBACK == StreamType)
        {
            gaf_cis_mobile_media_dwelling_info[0].startedStreamTypes &= ~GAF_AUDIO_STREAM_TYPE_PLAYBACK;
            gaf_cis_mobile_media_dwelling_info[1].startedStreamTypes &= ~GAF_AUDIO_STREAM_TYPE_PLAYBACK;
            playback_ase_lid_list = gaf_mobile_audio_get_playback_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != playback_ase_lid_list[ase_lid])
                {
                    gaf_mobile_audio_stream_stop(con_lid, playback_ase_lid_list[ase_lid]);
                }
            }
        }
        else
        {
            gaf_cis_mobile_media_dwelling_info[0].startedStreamTypes &= ~GAF_AUDIO_STREAM_TYPE_CAPTURE;
            gaf_cis_mobile_media_dwelling_info[1].startedStreamTypes &= ~GAF_AUDIO_STREAM_TYPE_CAPTURE;
            capture_ase_lid_list = gaf_mobile_audio_get_capture_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != capture_ase_lid_list[ase_lid])
                {
                    gaf_mobile_audio_stream_stop(con_lid, capture_ase_lid_list[ase_lid]);
                    break;
                }
            }
        }
    }
    // Recovery
    memcpy_s(gaf_cis_mobile_media_dwelling_info,
                sizeof(gaf_cis_mobile_media_dwelling_info),
                gaf_cis_mobile_media_dwelling_info_rec,
                sizeof(gaf_cis_mobile_media_dwelling_info));
    // Start
    for (uint8_t con_lid = 0; con_lid < GAF_MOB_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        if (GAF_AUDIO_STREAM_TYPE_PLAYBACK == StreamType)
        {
            playback_ase_lid_list = gaf_mobile_audio_get_playback_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != playback_ase_lid_list[ase_lid])
                {
                    gaf_mobile_audio_stream_start(con_lid, playback_ase_lid_list[ase_lid]);
                }
            }
        }
        else
        {
            capture_ase_lid_list = gaf_mobile_audio_get_capture_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != capture_ase_lid_list[ase_lid])
                {
                    gaf_mobile_audio_stream_start(con_lid, capture_ase_lid_list[ase_lid]);
                    break;
                }
            }
        }
    }
}

void gaf_mobile_audio_stream_init(void)
{
    /// Get custom callback function ptr
    gaf_uc_cli_custom_data_callback = gaf_stream_common_get_custom_data_handler(GAF_STREAM_USER_CASE_UC_CLI);
    /// Sanity check
    ASSERT(gaf_uc_cli_custom_data_callback, "Invalid custom data callback, user case %d", GAF_STREAM_USER_CASE_UC_CLI);

    memset(gaf_cis_mobile_media_dwelling_info, 0, sizeof(gaf_cis_mobile_media_dwelling_info));

    uint8_t con_id = 0;
    for (con_id = 0; con_id < GAF_MOB_MAXIMUM_CONNECTION_COUNT; con_id++)
    {
        gaf_mobile_audio_clear_playback_ase_index_list(con_id);
        gaf_mobile_audio_clear_capture_ase_index_list(con_id);
    }
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_mobile_audio_stream_env;
    memset((uint8_t *)pStreamEnv, 0, sizeof(GAF_AUDIO_STREAM_ENV_T));
    pStreamEnv->stream_context.playback_stream_state = GAF_PLAYBACK_STREAM_IDLE;
    pStreamEnv->stream_context.capture_stream_state = GAF_CAPTURE_STREAM_IDLE;
    pStreamEnv->stream_context.playback_trigger_supervisor_timer_id = NULL;
    pStreamEnv->stream_context.capture_trigger_supervisor_timer_id = NULL;
    pStreamEnv->stream_context.playback_retrigger_onprocess = false;
    pStreamEnv->stream_context.capture_retrigger_onprocess = false;
    pStreamEnv->stream_info.is_mobile = true;
    pStreamEnv->stream_info.is_bis = false;
    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
        pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
        pStreamEnv->stream_info.playbackInfo.aseChInfo[i].codec_core_info.instance_status = INSTANCE_IDLE;
        pStreamEnv->stream_info.captureInfo.aseChInfo[i].codec_core_info.instance_status = INSTANCE_IDLE;
    }
    GAF_AUDIO_STREAM_INFO_T* pStreamInfo;
    pStreamInfo = &(gaf_mobile_audio_stream_env.stream_info);
    pStreamInfo->contextType = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
    gaf_stream_common_register_cc_func_list();
    gaf_stream_common_register_func_list(&gaf_mobile_audio_stream_env,
        &gaf_mobile_audio_flexible_stream_func_list);
    gaf_mobile_stream_register_retrigger_callback(gaf_mobile_audio_steam_retrigger_handler);
}
#endif

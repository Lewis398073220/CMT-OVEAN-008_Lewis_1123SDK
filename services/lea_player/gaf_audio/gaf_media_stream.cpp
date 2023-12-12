/**
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
#if BLE_AUDIO_ENABLED
#include "bluetooth_bt_api.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "hal_codec.h"
#include "app_trace_rx.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "plat_types.h"
#include "cqueue.h"
#include "heap_api.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "list.h"
#include "lc3_process.h"
#include "besbt_string.h"

#include "gaf_media_sync.h"
#include "gaf_media_common.h"
#include "gaf_codec_lc3.h"
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#include "gaf_media_stream.h"
#include "gaf_stream_process.h"
#include "gaf_stream_dbg.h"
#ifndef BLE_STACK_NEW_DESIGN
#include "app_ble_tws_sync.h"
#else
#include "ble_tws.h"
#endif
#include "app_audio_active_device_manager.h"
#include "app_bt_media_manager.h"
#include "aob_stream_handler.h"

#include "nvrecord_ble.h"
#include "app_anc.h"
#include "app_anc_assist.h"
#include "anc_assist_mic.h"
#include "anc_assist_resample.h"

#include "rwble_config.h"
#include "bes_aob_api.h"

#ifdef CODEC_DAC_MULTI_VOLUME_TABLE
#include "hal_codec.h"
#endif

#ifdef SPEECH_BONE_SENSOR
#include "speech_bone_sensor.h"
#define SPEECH_BS_SINGLE_CHANNEL    (SPEECH_BS_CHANNEL_Z)
static bool g_capture_vpu_enabled = false;
#ifdef GAF_ENCODER_CROSS_CORE_USE_M55
static int32_t *g_capture_combo_vpu_pcm_buf = NULL;
#endif
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
#include "audio_prompt_sbc.h"
#endif

#ifdef BLE_I2S_AUDIO_SUPPORT
#include "app_ble_i2s_audio_stream.h"
#endif

#ifdef GAF_DSP
#include "mcu_dsp_hifi4_app.h"
#endif


// #define GAF_STREAM_PLAYBACK_AUDIO_DUMP      (16)
#ifdef BLE_AUDIO_SPEECH_ALGORITHM_SUPPORT
#define STREAM_PROCESS_ENABLE
#endif

/*********************external function declaration*************************/

/************************private type defination****************************/
#ifndef BT_AUDIO_CACHE_2_UNCACHE
#define BT_AUDIO_CACHE_2_UNCACHE(addr) \
    ((unsigned char *)((unsigned int)addr & ~(0x04000000)))
#endif

#define APP_BAP_MAX_ASCS_NB_ASE_CFG               (APP_BAP_MAX_ASCS_NB_ASE_CHAR * BLE_AUDIO_CONNECTION_CNT)
#define APP_BAP_MAX_RECORD_CNT                    (APP_BAP_MAX_ASCS_NB_ASE_CFG + 1)

/**********************private function declaration*************************/

/************************private variable defination************************/
#ifdef AOB_UC_TEST
uint8_t buds_freq;
#endif
/****************************GAF CUSTOM*****************************/
static const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T *gaf_uc_srv_custom_data_callback = NULL;

static GAF_AUDIO_STREAM_ENV_T gaf_audio_stream_env[GAF_MAXIMUM_CONNECTION_COUNT];
static GAF_MEDIA_DWELLING_INFO_T gaf_cis_media_dwelling_info[GAF_MAXIMUM_CONNECTION_COUNT];
static uint32_t POSSIBLY_UNUSED g_capture_ch_num = 4;
static uint32_t POSSIBLY_UNUSED g_speech_algo_ch_num = 4;
static uint32_t POSSIBLY_UNUSED g_capture_frame_len = 16 * 10;
static uint32_t POSSIBLY_UNUSED g_capture_sample_rate = 16000;
static uint32_t POSSIBLY_UNUSED g_capture_sample_bytes = sizeof(int32_t);
static GAF_STREAM_PLAYBACK_STATUS_E gaf_stream_playback_status = GAF_STREAM_PLAYBACK_STATUS_IDLE;

#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
#define RESAMPLE_HEAP_BUFF_SIZE         (1024 * 24)
static uint8_t *g_anc_assist_interval_buf = NULL;
static uint8_t *g_anc_assist_resample_buf = NULL;
static uint32_t g_anc_assist_resample_buf_used = 0;
#endif

#ifdef GAF_STREAM_PLAYBACK_AUDIO_DUMP
static uint32_t g_stream_pcm_dump_frame_len = 0;
static uint32_t g_stream_pcm_dump_channel_num = 0;
static uint32_t g_stream_pcm_dump_sample_bytes = 0;
#endif

#ifdef BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE
static uint32_t g_bin_record_sample_rate_div = BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE / 48000;
uint32_t gaf_stream_get_real_capture_sample_rate(void)
{
    return g_capture_sample_rate;
}
#endif

osMutexDef(gaf_playback_status_mutex_0);
#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
osMutexDef(gaf_playback_status_mutex_1);
#endif


/*
 * GAF_AUDIO_STREAM_CONTEXT_TYPE_MIN           = 0
 * GAF_AUDIO_STREAM_CONTEXT_TYPE_MEDIA         = GAF_AUDIO_STREAM_CONTEXT_TYPE_MIN,
 * GAF_AUDIO_STREAM_CONTEXT_TYPE_CALL,
 * GAF_AUDIO_STREAM_CONTEXT_TYPE_AI,
 * GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE,
*/
const static GAF_MEDIA_STREAM_TYPE_OPERATION_RULE_T gaf_cis_stream_type_op_rule[GAF_AUDIO_CONTEXT_NUM_MAX] =
{
    { GAF_AUDIO_STREAM_TYPE_PLAYBACK, GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM, 1, 0},
#ifdef AOB_MOBILE_ENABLED
    { GAF_AUDIO_STREAM_TYPE_PLAYBACK|GAF_AUDIO_STREAM_TYPE_CAPTURE, GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM, 1, 1},
    { GAF_AUDIO_STREAM_TYPE_CAPTURE, GAF_AUDIO_TRIGGER_BY_CAPTURE_STREAM, 0, 2},
#endif
    { GAF_AUDIO_STREAM_TYPE_FLEXIBLE, GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM, 1, 1},
};

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
static uint8_t* promptTmpSourcePcmDataBuf = NULL;
static uint8_t* promptTmpTargetPcmDataBuf = NULL;
static uint8_t* promptPcmDataBuf = NULL;
static uint8_t* promptResamplerBuf = NULL;
#endif

#define GAF_MEDIA_STREAM_POST_START_FREQ_BOOST_TIMEOUT_VALUE  (1500)

/****************************function defination****************************/
static void gaf_audio_add_ase_into_playback_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d playback ase lid %d", con_lid, ase_lid);
    uint8_t idx = 0;

    if (GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT ==
        gaf_stream_common_get_ase_idx_in_ase_lid_list(
            &gaf_cis_media_dwelling_info[con_lid].playback_ase_id[0], ase_lid))
    {
        idx = gaf_stream_common_get_valid_idx_in_ase_lid_list(
                &gaf_cis_media_dwelling_info[con_lid].playback_ase_id[0]);
        if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
        {
            LOG_E("no more space for adding playback ase lid: %d into list!!!", ase_lid);
        }
        else
        {
            gaf_cis_media_dwelling_info[con_lid].playback_ase_id[idx] = ase_lid;
        }
    }
    else
    {
        LOG_E("ase_lid: %d already in playback ase list!!!", ase_lid);
    }
}

static void gaf_audio_remove_ase_from_playback_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d playback ase lid %d", con_lid, ase_lid);
    uint8_t idx = gaf_stream_common_get_ase_idx_in_ase_lid_list(
                &gaf_cis_media_dwelling_info[con_lid].playback_ase_id[0], ase_lid);
    if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
    {
        LOG_E("ase lid: %d is not in the list!!!", ase_lid);
        return;
    }
    gaf_cis_media_dwelling_info[con_lid].playback_ase_id[idx] = GAF_INVALID_ASE_INDEX;
}

static void gaf_audio_add_ase_into_capture_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d capture ase lid %d", con_lid, ase_lid);
    uint8_t idx = 0;

    if (GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT ==
        gaf_stream_common_get_ase_idx_in_ase_lid_list(
            &gaf_cis_media_dwelling_info[con_lid].capture_ase_id[0], ase_lid))
    {
        idx = gaf_stream_common_get_valid_idx_in_ase_lid_list(
                &gaf_cis_media_dwelling_info[con_lid].capture_ase_id[0]);
        if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
        {
            LOG_E("no more space for adding capture ase lid: %d into list!!!", ase_lid);
        }
        else
        {
            gaf_cis_media_dwelling_info[con_lid].capture_ase_id[idx] = ase_lid;
        }
    }
    else
    {
        LOG_E("ase_lid: %d already in capture ase list!!!", ase_lid);
    }
}

static void gaf_audio_remove_ase_from_capture_ase_list(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_D("set con_lid %d capture ase lid %d", con_lid, ase_lid);
    uint8_t idx = gaf_stream_common_get_ase_idx_in_ase_lid_list(
                &gaf_cis_media_dwelling_info[con_lid].capture_ase_id[0], ase_lid);
    if (idx == GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT)
    {
        LOG_E("ase lid: %d is not in the list!!!", ase_lid);
        return;
    }
    gaf_cis_media_dwelling_info[con_lid].capture_ase_id[idx] = GAF_INVALID_ASE_INDEX;
}

static uint8_t* gaf_audio_get_playback_ase_index_list(uint8_t con_lid)
{
    LOG_D("get playback con_lid %d", con_lid);
    return &gaf_cis_media_dwelling_info[con_lid].playback_ase_id[0];
}

static uint8_t* gaf_audio_get_capture_ase_index_list(uint8_t con_lid)
{
    LOG_D("get capture con_lid %d", con_lid);
    return &gaf_cis_media_dwelling_info[con_lid].capture_ase_id[0];
}

static void gaf_audio_clear_playback_ase_index_list(uint8_t con_lid)
{
    LOG_D("clear playback ase list con_lid %d", con_lid);
    memset_s(&gaf_cis_media_dwelling_info[con_lid].playback_ase_id[0],
            GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t), GAF_INVALID_ASE_INDEX,
            GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t));
}

static void gaf_audio_clear_capture_ase_index_list(uint8_t con_lid)
{
    LOG_D("clear playback ase list con_lid %d", con_lid);
    memset_s(&gaf_cis_media_dwelling_info[con_lid].capture_ase_id[0],
            GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t), GAF_INVALID_ASE_INDEX,
            GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT*sizeof(uint8_t));
}

static void gaf_audio_dwelling_info_list_init(void)
{
    memset_s((void *)&gaf_cis_media_dwelling_info, sizeof(gaf_cis_media_dwelling_info),
             0, sizeof(gaf_cis_media_dwelling_info));
}

static GAF_AUDIO_STREAM_ENV_T* gaf_audio_get_stream_from_channel_index(uint8_t channel)
{
    for (uint32_t con_lid = 0; con_lid < GAF_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
        {
            if ((gaf_audio_stream_env[con_lid].stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl == channel) ||
                (gaf_audio_stream_env[con_lid].stream_info.captureInfo.aseChInfo[i].iso_channel_hdl == channel))
            {
                return &gaf_audio_stream_env[con_lid];
            }
        }
    }

    LOG_E("Receive a CIS packet from channel: %d before cooresponding stream is ready!", channel);
    return NULL;
}

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
GAF_AUDIO_STREAM_ENV_T* gaf_audio_get_stream_env_from_con_lid(uint8_t con_lid)
{
    for (uint32_t type = GAF_AUDIO_STREAM_CONTEXT_TYPE_MEDIA; type < GAF_AUDIO_CONTEXT_NUM_MAX; type++)
    {
        if (gaf_audio_stream_env[type].stream_info.con_lid == con_lid)
        {
            return &gaf_audio_stream_env[type];
        }
    }

    LOG_W("(d%d)%s fail invaild", con_lid, __func__);
    return NULL;
}

static gaf_stream_dma_irq_cb capture_dma_irq_info_record_handler = NULL;
void gaf_stream_capture_register_dma_irq_cb(gaf_stream_dma_irq_cb func)
{
    capture_dma_irq_info_record_handler = func;
}
#endif

static void gaf_stream_configure_playback_trigger(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, CC_PLAYBACK_DATA_T *frame)
{
    int32_t bt_time_diff = 0;
    uint32_t trigger_bt_time = 0;
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();

    if (0xFF == pStreamEnv->stream_context.playbackTriggerChannel)
    {
        pStreamEnv->stream_context.playbackTriggerChannel = app_ble_tws_sync_get_avaliable_trigger_channel();
        if (0xFF != pStreamEnv->stream_context.playbackTriggerChannel)
        {
            gaf_media_prepare_playback_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
        }
        else
        {
            LOG_W("cann't get avaliable trigger channel!!!");
            return;
        }
    }

    // time-stamp + present deay - one dma empty run
    trigger_bt_time = frame->time_stamp
        + (uint32_t)(pStreamEnv->stream_info.playbackInfo.presDelayUs)
        - pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;
    bt_time_diff = trigger_bt_time - current_bt_time;

    LOG_I("playback trigger pk timeStamp %u current us %u seq 0x%x",
        frame->time_stamp, current_bt_time, frame->seq_nb);
    LOG_I("calculated trigger ticks:%u trigger offset:%d", trigger_bt_time, bt_time_diff);

    if (bt_time_diff > GAF_MARGIN_BETWEEN_TRIGGER_TIME_AND_CURRENT_TIME_US)
    {
        pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX] = frame->seq_nb - 1;
        LOG_I("Starting playback seq num 0x%x", pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX] + 1);

        gaf_stream_common_set_playback_trigger_time(pStreamEnv, trigger_bt_time);

        gaf_stream_set_prefill_status(true);
    }
    else
    {
        LOG_I("time_stamp pass");
    }
}

POSSIBLY_UNUSED static void gaf_media_stop_all_iso_dp_rx(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl)
        {
            bes_ble_bap_dp_rx_iso_stop(pStreamEnv->stream_info.playbackInfo.aseChInfo[i].ase_handle);
        }
    }
}

POSSIBLY_UNUSED static void gaf_media_stop_all_iso_dp_tx(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl)
        {
            bes_ble_bap_dp_tx_iso_stop(pStreamEnv->stream_info.captureInfo.aseChInfo[i].ase_handle);
        }
    }
}

static void gaf_stream_receive_data(uint16_t conhdl, GAF_ISO_PKT_STATUS_E pkt_status)
{
    // map to gaf stream context
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = NULL;
    uint8_t channel = BLE_ISOHDL_TO_ACTID(conhdl);
    pStreamEnv = gaf_audio_get_stream_from_channel_index(channel);
    gaf_media_data_t *p_media_data = NULL;

    /// Check for stream env info for app unsetup ASE
    if (!pStreamEnv)
    {
        while ((p_media_data = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
        {
            gaf_stream_data_free(p_media_data);
        }
        return;
    }

    CC_PLAYBACK_DATA_T frame = {0};
    uint8_t ase_chan_lid = gaf_media_common_get_ase_chan_lid_from_iso_channel(
                                    pStreamEnv, BES_BLE_GAF_DIRECTION_SINK, channel);
    if (ase_chan_lid >= GAF_AUDIO_ASE_TOTAL_COUNT)
    {
        LOG_E("can not get ase chan lid by cis handle: 0x%x", conhdl);
        return;
    }

    CODEC_CORE_INFO_T *coreInfo = &pStreamEnv->stream_info.playbackInfo.aseChInfo[ase_chan_lid].codec_core_info;
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (coreInfo->instance_status < INSTANCE_INITIALIZED)
    {
        while ((p_media_data = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
        {
            gaf_stream_data_free(p_media_data);
        }
        return;
    }
#else
    coreInfo->instance_handle = 0;
#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
    coreInfo->instance_handle = ase_chan_lid;
#endif
#endif

    while ((p_media_data = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
    {
        ASSERT(p_media_data->data_len <= pStreamEnv->stream_info.playbackInfo.maxFrameSize,
            "%s len %d %d, channel:%d, playbackInfo:%p", __func__, p_media_data->data_len,
            pStreamEnv->stream_info.playbackInfo.maxFrameSize, channel, &(pStreamEnv->stream_info.playbackInfo));

        frame.time_stamp = p_media_data->time_stamp;
        frame.seq_nb = p_media_data->pkt_seq_nb;
        frame.data_len = p_media_data->data_len;
        frame.data = p_media_data->sdu_data;
        frame.isPLC = ((GAF_ISO_PKT_STATUS_VALID != p_media_data->pkt_status) || (0 == p_media_data->data_len));

        if (((GAF_ISO_PKT_STATUS_VALID == p_media_data->pkt_status) &&
            (p_media_data->data_len > 0)) &&
            (GAF_PLAYBACK_STREAM_INITIALIZED == pStreamEnv->stream_context.playback_stream_state))
        {
            if ((GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM ==
                  gaf_cis_stream_type_op_rule[pStreamEnv->stream_info.contextType].trigger_stream_type) ||
                 (GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM ==
                  gaf_cis_stream_type_op_rule[pStreamEnv->stream_info.contextType].trigger_stream_type))
            {
                gaf_stream_configure_playback_trigger(pStreamEnv, &frame);
            }
            else
            {
                LOG_W("[ISO SDU RECV] current stream context type = %d, state = %d",
                            pStreamEnv->stream_info.contextType,
                            pStreamEnv->stream_context.playback_stream_state);
            }
        }
#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
        /// If we'd like to recv multi chan, set seq trigger but do not set dma trigger again for this channel
        else if ((pStreamEnv->stream_context.playback_stream_state == GAF_PLAYBACK_STREAM_START_TRIGGERING) &&
                 ((GAF_ISO_PKT_STATUS_VALID == p_media_data->pkt_status) && (p_media_data->data_len > 0)) &&
                 (pStreamEnv->stream_context.lastestPlaybackSeqNum[0] == GAF_AUDIO_INVALID_SEQ_NUMBER))
        {
            gaf_stream_configure_playback_trigger(pStreamEnv, &frame);
        }
#endif

        if (pStreamEnv->stream_context.playback_stream_state >=  GAF_PLAYBACK_STREAM_START_TRIGGERING)
        {
            if (frame.isPLC) {
                LOG_I("PLC seq:%u ts:%u pk status:%d data len:%d",
                    p_media_data->pkt_seq_nb, p_media_data->time_stamp, p_media_data->pkt_status, p_media_data->data_len);
            }
            /// gaf custom, may be a watch point to check some pattern in encoded packet
            if (gaf_uc_srv_custom_data_callback->encoded_packet_recv_cb)
            {
                gaf_uc_srv_custom_data_callback->encoded_packet_recv_cb(p_media_data);
            }
            gaf_stream_common_store_packet(pStreamEnv, &frame, coreInfo);
        }

        gaf_stream_data_free(p_media_data);
    }
}

static int gaf_audio_flexible_playback_stream_start_handler(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    uint8_t rt_vol = bes_ble_arc_vol_get_real_time_volume(pStreamEnv->stream_info.con_lid);
    uint8_t vol = bes_ble_arc_convert_le_vol_to_local_vol(rt_vol);

    if (GAF_PLAYBACK_STREAM_INITIALIZING == pStreamEnv->stream_context.playback_stream_state)
    {
        pStreamEnv->func_list->stream_func_list.playback_init_stream_buf_func(pStreamEnv);
        if (pStreamEnv->func_list->decoder_func_list)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_init_func(
                0, &(pStreamEnv->stream_info.playbackInfo.codec_info));
        }

        struct AF_STREAM_CONFIG_T stream_cfg;
        af_set_priority(AF_USER_AOB_PLAYBACK, osPriorityHigh);

#ifdef CODEC_DAC_MULTI_VOLUME_TABLE
        if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT) {
            hal_codec_set_dac_volume_table(codec_dac_hfp_vol, ARRAY_SIZE(codec_dac_hfp_vol));
        } else {
            hal_codec_set_dac_volume_table(codec_dac_a2dp_vol, ARRAY_SIZE(codec_dac_a2dp_vol));
        }
#endif
        // playback stream
        memset_s((void *)&stream_cfg, sizeof(struct AF_STREAM_CONFIG_T),
                0, sizeof(struct AF_STREAM_CONFIG_T));
        stream_cfg.bits         = (enum AUD_BITS_T)(pStreamEnv->stream_info.playbackInfo.dma_info.bits_depth);
        stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)(pStreamEnv->stream_info.playbackInfo.dma_info.num_channels);

        stream_cfg.io_path      = AUD_OUTPUT_PATH_SPEAKER;
#ifdef PSAP_SW_USE_DAC1
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC3;
#else
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)pStreamEnv->stream_info.playbackInfo.dma_info.sample_rate;
#ifdef BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT
        if (stream_cfg.channel_num == AUD_CHANNEL_NUM_2)
        {
            stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)( AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);
        }
#endif
        stream_cfg.vol          = vol;
        stream_cfg.data_size    = (uint32_t)(2 * pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr);
        stream_cfg.handler = pStreamEnv->func_list->stream_func_list.playback_dma_irq_handler_func;

        bes_ble_bap_dp_itf_data_come_callback_register((void *)gaf_stream_receive_data);

        af_codec_tune(AUD_STREAM_PLAYBACK, 0);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#ifdef STREAM_PROCESS_ENABLE
        gaf_stream_process_playback_open(pStreamEnv->stream_info.bap_contextType, &stream_cfg);
#endif

#if defined(SPA_AUDIO_ENABLE)
        //spatial_audio_app_init_if(sample_rate, sample_bits, hw_ch_num, frame_size, (void*)app_audio_mempool_get_buff, app_ibrt_if_get_ui_role(), ((ibrt_ctrl_t *)app_tws_ibrt_get_bt_ctrl_ctx())->audio_chnl_sel);
        spatial_audio_app_init_if(stream_cfg.sample_rate,stream_cfg.bits,stream_cfg.channel_num,stream_cfg.data_size/stream_cfg.channel_num/(stream_cfg.bits <= AUD_BITS_16 ? 2 : 4)/2,(void*)app_audio_mempool_get_buff, app_ibrt_if_get_ui_role(), ((ibrt_ctrl_t *)app_tws_ibrt_get_bt_ctrl_ctx())->audio_chnl_sel);
#endif

#ifdef GAF_STREAM_PLAYBACK_AUDIO_DUMP
        if (stream_cfg.bits == 16) {
            g_stream_pcm_dump_sample_bytes = sizeof(int16_t);
        } else {
            g_stream_pcm_dump_sample_bytes = sizeof(int32_t);
        }
        g_stream_pcm_dump_channel_num = stream_cfg.channel_num;
        g_stream_pcm_dump_frame_len = stream_cfg.data_size / 2 / g_stream_pcm_dump_sample_bytes / g_stream_pcm_dump_channel_num;
#if GAF_STREAM_PLAYBACK_AUDIO_DUMP == 16
        audio_dump_init(g_stream_pcm_dump_frame_len, sizeof(int16_t), 1);
#else
        audio_dump_init(g_stream_pcm_dump_frame_len, g_stream_pcm_dump_sample_bytes, 1);
#endif
#endif

#if defined(IBRT)
        pStreamEnv->stream_context.playbackTriggerChannel = app_ble_tws_sync_get_avaliable_trigger_channel();
#endif

        gaf_media_prepare_playback_trigger(pStreamEnv->stream_context.playbackTriggerChannel);

        gaf_media_pid_init(&(pStreamEnv->stream_context.playback_pid_env));

        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_stop_playing();

        audio_prompt_buffer_config(MIX_WITH_BLE_AUDIO_FLEXIBLE_STREAMING,
                                    stream_cfg.channel_num,
                                    stream_cfg.bits,
                                    promptTmpSourcePcmDataBuf,
                                    promptTmpTargetPcmDataBuf,
                                    promptPcmDataBuf,
                                    AUDIO_PROMPT_PCM_BUFFER_SIZE,
                                    promptResamplerBuf,
                                    AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);
#endif

        uint32_t playback_freq = APP_SYSFREQ_52M;
        if (pStreamEnv->func_list->decoder_func_list)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_set_freq(
                &(pStreamEnv->stream_info.playbackInfo.codec_info), playback_freq, M33_CORE);
        }
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, (enum APP_SYSFREQ_FREQ_T)playback_freq);

        LOG_D("%s end", __func__);

        return 0;
    }

    return -1;
}

#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
static uint32_t gaf_audio_anc_resample_heap_free_size()
{
    return RESAMPLE_HEAP_BUFF_SIZE - g_anc_assist_resample_buf_used;
}

static void gaf_audio_anc_resample_heap_init()
{
    g_anc_assist_resample_buf_used = 0;
    memset_s(g_anc_assist_resample_buf, RESAMPLE_HEAP_BUFF_SIZE, 0, RESAMPLE_HEAP_BUFF_SIZE);
}

static int gaf_audio_anc_resample_get_heap_buff(uint8_t **buff, uint32_t size)
{
    uint32_t buff_size_free;

    buff_size_free = gaf_audio_anc_resample_heap_free_size();

    if (size % 4){
        size = size + (4 - size % 4);
    }
    ASSERT (size <= buff_size_free, "anc resample pool in shortage! To allocate size %d but free size %d.",
        size, buff_size_free);
    *buff = g_anc_assist_resample_buf + g_anc_assist_resample_buf_used;
    g_anc_assist_resample_buf_used += size;
    LOG_I("[%s] ptr=%p size=%u free=%u", __func__, *buff, size, buff_size_free);
    return buff_size_free;
}

void *gaf_anc_assist_resample_get_ext_buf(size_t nitems, size_t len)
{
    uint8_t *pBuf = NULL;
    int size;

    size = nitems * len;

    if (size % 4) {
        size = size + (4 - size % 4);
    }

    gaf_audio_anc_resample_get_heap_buff(&pBuf, size);

    return (void *)pBuf;
}

void gaf_anc_assist_resample_free_ext_buf(void *mem_ptr)
{

}
#endif

static void gaf_audio_flexible_common_buf_init(GAF_AUDIO_STREAM_ENV_T * pStreamEnv, GAF_STREAM_TYPE_E stream_type)
{
    uint32_t audioCacheHeapSize = 0;
    uint8_t* heapBufStartAddr = NULL;
    uint32_t lock = int_lock();
    if ((pStreamEnv->stream_context.playback_stream_state <= GAF_PLAYBACK_STREAM_INITIALIZING) &&
        (pStreamEnv->stream_context.capture_stream_state <= GAF_CAPTURE_STREAM_INITIALIZING))
    {
        pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr = NULL;
        pStreamEnv->stream_info.captureInfo.dma_info.storedDmaBufPtr = NULL;
        pStreamEnv->stream_context.capture_frame_cache = NULL;
        pStreamEnv->stream_context.playback_frame_cache =  NULL;
        pStreamEnv->stream_context.playback_pcm_cache = NULL;
#if !defined (GAF_CODEC_CROSS_CORE) && !defined (AOB_CODEC_CP)
        for (uint32_t instance = 0; instance < PLAYBACK_INSTANCE_MAX; instance++) {
            pStreamEnv->stream_context.playback_frame_buf[instance] = NULL;
            pStreamEnv->stream_context.playback_frame_fifo[instance] = NULL;
        }
#endif
#if defined(SPEECH_BONE_SENSOR) && defined(GAF_ENCODER_CROSS_CORE_USE_M55)
        g_capture_combo_vpu_pcm_buf = NULL;
#endif
#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
        g_anc_assist_interval_buf = NULL;
        g_anc_assist_resample_buf = NULL;
#endif

#ifdef STREAM_PROCESS_ENABLE
        gaf_stream_process_set_playback_buf(NULL, 0);
        gaf_stream_process_set_capture_buf(NULL, 0);
#endif

        if (BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT == pStreamEnv->stream_info.bap_contextType)
        {
            app_audio_mempool_init();
        }
        else
        {
            app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
        }

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

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        app_audio_mempool_get_buff(&promptTmpSourcePcmDataBuf, AUDIO_PROMPT_SOURCE_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptTmpTargetPcmDataBuf, AUDIO_PROMPT_TARGET_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptPcmDataBuf, AUDIO_PROMPT_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptResamplerBuf, AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);
#endif

    }

    if (GAF_STREAM_PLAYBACK == stream_type)
    {
#ifdef STREAM_PROCESS_ENABLE
        uint32_t stream_process_buf_size = gaf_stream_process_need_playback_buf_size(pStreamEnv->stream_info.bap_contextType);
        uint8_t* buf_tmp = gaf_stream_process_get_playback_buf();
        if ((buf_tmp == NULL) && (stream_process_buf_size != 0)) {
            gaf_stream_process_set_playback_buf((uint8_t *)app_audio_mempool_calloc(stream_process_buf_size, sizeof(int8_t)), stream_process_buf_size);
        }
#endif
        if (NULL == pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr)
        {
            app_audio_mempool_get_buff(&(pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr),
                            pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize*2);
        }
        pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr = pStreamEnv->stream_info.playbackInfo.dma_info.storedDmaBufPtr;
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_BUF_INITIALIZED);
    }
    else
    {
#ifdef STREAM_PROCESS_ENABLE
        uint32_t stream_process_buf_size = gaf_stream_process_need_capture_buf_size(pStreamEnv->stream_info.bap_contextType);
        uint8_t* buf_tmp = gaf_stream_process_get_capture_buf();
        if ((buf_tmp == NULL) && (stream_process_buf_size != 0)) {
            gaf_stream_process_set_capture_buf((uint8_t *)app_audio_mempool_calloc(stream_process_buf_size, sizeof(int8_t)), stream_process_buf_size);
        }
#endif

        GAF_AUDIO_STREAM_COMMON_INFO_T *pCommonInfo = &pStreamEnv->stream_info.captureInfo;
        g_capture_frame_len = (uint32_t)(g_capture_sample_rate * pCommonInfo->dma_info.frame_ms / 1000);

        if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT) {
            g_capture_ch_num = 1;
            g_speech_algo_ch_num = 1;
#ifdef BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE
            ASSERT((g_capture_sample_rate == 32000) || (g_capture_sample_rate == 48000),"[%s] Don't support this sample rate: %d", __func__, g_capture_sample_rate);
            g_bin_record_sample_rate_div = BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE / g_capture_sample_rate;
            pCommonInfo->dma_info.dmaChunkSize *= g_bin_record_sample_rate_div;
#endif
        } else {
#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
            g_capture_ch_num = app_anc_assist_get_mic_ch_num(AUD_INPUT_PATH_MAINMIC);
            pCommonInfo->dma_info.dmaChunkSize *= g_capture_ch_num;
#else
            g_capture_ch_num = hal_codec_get_input_path_chan_num(AUD_INPUT_PATH_MAINMIC);
#ifndef BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT
            pCommonInfo->dma_info.dmaChunkSize *= g_capture_ch_num;
#endif
#endif

            g_speech_algo_ch_num = hal_codec_get_input_path_chan_num(AUD_INPUT_PATH_MAINMIC);
            LOG_I("[%s] speech algo ch_num: %d", __func__, g_speech_algo_ch_num);

#if defined(SPEECH_BONE_SENSOR) && defined(GAF_ENCODER_CROSS_CORE_USE_M55)
            if ((g_capture_combo_vpu_pcm_buf == NULL) && g_capture_vpu_enabled) {
                g_capture_combo_vpu_pcm_buf = (int32_t *)app_audio_mempool_calloc(g_capture_frame_len * (g_speech_algo_ch_num + 1), sizeof(int32_t));
            }
#endif
        }
#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
        if (g_anc_assist_interval_buf == NULL) {
            app_audio_mempool_get_buff(&g_anc_assist_interval_buf, g_capture_frame_len * g_capture_ch_num * sizeof(int32_t));
            if (g_capture_sample_rate != AUD_SAMPRATE_16000) {
                app_audio_mempool_get_buff(&g_anc_assist_resample_buf, RESAMPLE_HEAP_BUFF_SIZE);
            }
        }
#endif

        if (NULL == pCommonInfo->dma_info.storedDmaBufPtr)
        {
            app_audio_mempool_get_buff(&(pCommonInfo->dma_info.storedDmaBufPtr), pCommonInfo->dma_info.dmaChunkSize*2);
        }
        pCommonInfo->dma_info.dmaBufPtr = pCommonInfo->dma_info.storedDmaBufPtr;
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_BUF_INITIALIZED);
    }
    int_unlock(lock);
}

static void gaf_audio_flexible_playback_buf_init(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    gaf_audio_flexible_common_buf_init(pStreamEnv, GAF_STREAM_PLAYBACK);

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
    cfifo_init(pStreamEnv->stream_context.playback_frame_fifo[0],
        pStreamEnv->stream_context.playback_frame_buf[0], frame_buf_size);
#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
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
    cfifo_init(pStreamEnv->stream_context.playback_frame_fifo[1],
        pStreamEnv->stream_context.playback_frame_buf[1], frame_buf_size);
#endif
#else
#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
    if (NULL == pStreamEnv->stream_context.playback_pcm_cache)
    {
        app_audio_mempool_get_buff(&(pStreamEnv->stream_context.playback_pcm_cache),
            pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize);
    }
#endif
#endif
    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_init_buf_func(0,
            &(pStreamEnv->stream_info.playbackInfo.codec_info), (void*)gaf_stream_common_buf_alloc);
    }

    LOG_D("%s end", __func__);
}

static int gaf_playback_status_mutex_init(void **mutex)
{
    if (mutex[0] == NULL){
        mutex[0] = osMutexCreate((osMutex(gaf_playback_status_mutex_0)));
    }
#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
    if (mutex[1] == NULL){
        mutex[1] = osMutexCreate((osMutex(gaf_playback_status_mutex_1)));
    }
#endif
    return 0;
}

static int gaf_stream_set_playback_status(uint8_t con_lid,
                                          GAF_STREAM_PLAYBACK_STATUS_E playback_status)
{
    void *mutex = gaf_audio_stream_env[con_lid].stream_info.gaf_playback_status_mutex;

    gaf_playback_status_mutex_lock(mutex);
    gaf_stream_playback_status = playback_status;
    gaf_playback_status_mutex_unlock(mutex);

    return 0;
}

GAF_STREAM_PLAYBACK_STATUS_E gaf_stream_get_playback_status(uint8_t con_lid)
{
    void *mutex = gaf_audio_stream_env[con_lid].stream_info.gaf_playback_status_mutex;
    GAF_STREAM_PLAYBACK_STATUS_E playback_status;

    gaf_playback_status_mutex_lock(mutex);
    playback_status = gaf_stream_playback_status;
    gaf_playback_status_mutex_unlock(mutex);

    return playback_status;
}

void gaf_stream_dump_dma_trigger_status()
{
    uint8_t context_id = 0;
    uint8_t len = 0;
    char state_str[100];
    for (context_id = 0; context_id < GAF_AUDIO_CONTEXT_NUM_MAX; context_id++)
    {
        GAF_PLAYBACK_STREAM_STATE_E trigger_status = gaf_audio_stream_env[context_id].stream_context.playback_stream_state;
        len += sprintf(state_str + len, "context %d state %d ", context_id, trigger_status);
    }
    LOG_I("gaf tri: %s", state_str);
}

static int gaf_stream_playback_stop(uint8_t con_lid)
{
    LOG_D("%s",__func__);
    int cnt = 50;

    do {
        if (gaf_stream_get_playback_status(con_lid) == GAF_STREAM_PLAYBACK_STATUS_IDLE)
        {
            LOG_I("[STOP] PLAYBACK_STATUS_IDLE cnt:%d", cnt);
            break;
        }
        else
        {
            osDelay(1);
        }
    } while(--cnt > 0);

    return 0;
}

static int gaf_audio_flexible_playback_stream_stop_handler(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    if (GAF_PLAYBACK_STREAM_IDLE != pStreamEnv->stream_context.playback_stream_state)
    {
        bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.playback_retrigger_onprocess;
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_208M);
        bes_ble_bap_dp_itf_data_come_callback_deregister();
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_IDLE);
        gaf_stream_playback_stop(pStreamEnv->stream_info.con_lid);

        uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
        uint32_t dma_base;
        // sink
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

#ifdef STREAM_PROCESS_ENABLE
        gaf_stream_process_playback_close();
#endif

        if (pStreamEnv->func_list->decoder_func_list)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_deinit_func(0);
        }
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        gaf_playback_decoder_deinit_all_ase(pStreamEnv, isRetrigger);
#endif

        gaf_stream_common_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);

        af_codec_set_bt_sync_source(AUD_STREAM_PLAYBACK, 0);

        af_set_priority(AF_USER_AOB_PLAYBACK, osPriorityAboveNormal);
        pStreamEnv->func_list->stream_func_list.playback_deinit_stream_buf_func(pStreamEnv);

#ifdef BLE_I2S_AUDIO_SUPPORT
        app_ble_audio_stream_stop_post_handler();
#endif
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_32K);
        LOG_D("%s end", __func__);
        return 0;
    }

    return -1;
}

static bool gaf_stream_is_any_downstream_iso_created(uint8_t con_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_audio_stream_env[con_lid];

    for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL != pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl)
        {
            return true;
        }
    }

    return false;
}

#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
static bool gaf_stream_flexible_is_skip_prefill_handler(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint8_t ase_idx = 0;
    /// Check is there need wait for second iso data to do prefill
    if (gaf_stream_get_prefill_status())
    {
        uint8_t set_trigger_times = 0;
        for (ase_idx = 0; ase_idx < (GAF_AUDIO_ASE_TOTAL_COUNT - 1); ase_idx++)
        {
            if (GAF_AUDIO_INVALID_ISO_CHANNEL == pStreamEnv->stream_info.playbackInfo.aseChInfo[ase_idx].iso_channel_hdl)
            {
                break;
            }

            if (pStreamEnv->stream_context.lastestPlaybackSeqNum[ase_idx] != GAF_AUDIO_INVALID_SEQ_NUMBER)
            {
                set_trigger_times++;
            }
        }
        /// If (ase idx > set_trigger_times) means two iso connected but just one channel has recved data
        if (ase_idx > set_trigger_times)
        {
            return true;
        }
    }

    return false;
}
#endif

static uint32_t _gaf_stream_flexible_playback_dma_irq_handler(uint8_t con_lid, uint8_t* ptrBuf, uint32_t length)
{
    LOG_D("%s buffer len = %d", __func__,length);
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = &gaf_audio_stream_env[con_lid];

    if (!gaf_stream_is_any_downstream_iso_created(pStreamEnv->stream_info.con_lid))
    {
        return length;
    }

#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
    /// Check is here any neccesary to do prefill, maybe need wait for two channel ready
    if (gaf_stream_flexible_is_skip_prefill_handler(pStreamEnv))
    {
        return length;
    }
#endif

    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    int ret = CODEC_OK;
    POSSIBLY_UNUSED uint8_t instance_handle = 0;

    gaf_stream_set_playback_status(con_lid, GAF_STREAM_PLAYBACK_STATUS_BUSY);
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
    if (!gaf_stream_get_prefill_status())
    {
        if ((!pStreamEnv) ||
            (GAF_PLAYBACK_STREAM_START_TRIGGERING > pStreamEnv->stream_context.playback_stream_state) ||
            ((GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED == pStreamEnv->stream_context.playback_stream_state) &&
            (dmaIrqHappeningTimeUs == pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs)))
        {
            memset_s(ptrBuf, length, 0, length);
            gaf_stream_set_playback_status(con_lid, GAF_STREAM_PLAYBACK_STATUS_IDLE);
            return length;
        }

#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
        gaf_stream_common_update_multi_channel_expect_seq_and_time(pStreamEnv, dmaIrqHappeningTimeUs);
#else
        gaf_stream_common_updated_expeceted_playback_seq_and_time(pStreamEnv, GAF_AUDIO_DFT_PLAYBACK_LIST_IDX, dmaIrqHappeningTimeUs);
#endif

        if (GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED !=
            pStreamEnv->stream_context.playback_stream_state)
        {
            gaf_stream_common_update_playback_stream_state(pStreamEnv,  GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED);
            gaf_stream_common_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
            LOG_I("%s Update playback 0 seq to [0x%x]", __func__, pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]);
#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
            if (pStreamEnv->stream_context.lastestPlaybackSeqNum[1] != GAF_AUDIO_INVALID_SEQ_NUMBER)
            {
               LOG_I("%s Update playback 1 seq to [0x%x]", __func__, pStreamEnv->stream_context.lastestPlaybackSeqNum[1]);
            }
#endif
        }
    }

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    bool isSuccessful = false;
#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
    isSuccessful = gaf_stream_common_fetch_multi_pcm(pStreamEnv, ptrBuf, length, dmaIrqHappeningTimeUs);
#else
    isSuccessful = gaf_stream_common_fetch_pcm(pStreamEnv, ptrBuf, length,
                instance_handle, dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, true);
#endif
    if (!isSuccessful)
    {
        ret = CODEC_ERR;
    }
#else
    CC_PLAYBACK_DATA_T *frame = (CC_PLAYBACK_DATA_T*)pStreamEnv->stream_context.playback_frame_cache;
    frame->data = pStreamEnv->stream_context.playback_frame_cache + sizeof(CC_PLAYBACK_DATA_T);

#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
    gaf_stream_common_get_combined_packet_from_multi_channels(pStreamEnv, frame, dmaIrqHappeningTimeUs);
#else
    gaf_stream_common_get_packet(pStreamEnv, frame,
        instance_handle, dmaIrqHappeningTimeUs, GAF_INVALID_SINK_PLAY_DELAY, true);
#endif

    ret = pStreamEnv->func_list->decoder_func_list->decoder_decode_frame_func
            (instance_handle, &(pStreamEnv->stream_info.playbackInfo.codec_info),
            frame->data_len, frame->data,  ptrBuf, frame->isPLC);

    LOG_D("decoded seq 0x%02x expected play time %u local time %u dec ret %d",
            frame->seq_nb, frame->time_stamp, dmaIrqHappeningTimeUs, ret);

    /// gaf custom, may be a watch point to check some pattern in decoded raw data
    // if (gaf_uc_srv_custom_data_callback->decoded_raw_data_cb)
    // {
    //     gaf_uc_srv_custom_data_callback->decoded_raw_data_cb(frame, ptrBuf, length);
    // }
#endif

    if (CODEC_OK == ret)
    {
#ifdef STREAM_PROCESS_ENABLE
        length = gaf_stream_process_playback_run(ptrBuf, length);
#endif
    }
    else
    {
        //LOG_E("[le][err]%dms,%p,%d->%p,%d",dmaIrqHappeningTimeUs/1000,decoder_frame_p->data,decoder_frame_p->data_len,ptrBuf,length);
        memset_s(ptrBuf, length, 0, length);
    }
    gaf_stream_set_playback_status(con_lid, GAF_STREAM_PLAYBACK_STATUS_IDLE);

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    if (audio_prompt_is_playing_ongoing()){
        audio_prompt_processing_handler(0, length, ptrBuf);
    }
#endif

    LOG_D("%s end", __func__);

    return 0;
}

static uint32_t gaf_stream_flexible_playback_idx_0_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    return _gaf_stream_flexible_playback_dma_irq_handler(0, ptrBuf, length);
}

static uint32_t gaf_stream_flexible_playback_idx_1_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    return _gaf_stream_flexible_playback_dma_irq_handler(1, ptrBuf, length);
}

/// TODO: maybe need more

static void gaf_audio_flexible_playback_buf_deinit(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr = NULL;
    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_deinit_buf_func(0);
    }

    LOG_I("[%s] syspool free size: %d/%d", __func__, syspool_free_size(), syspool_total_size());
}

static int gaf_audio_flexible_capture_stream_start_handler(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    uint8_t rt_vol = bes_ble_arc_vol_get_real_time_volume(pStreamEnv->stream_info.con_lid);
    uint8_t vol = bes_ble_arc_convert_le_vol_to_local_vol(rt_vol);

    if (GAF_CAPTURE_STREAM_INITIALIZING == pStreamEnv->stream_context.capture_stream_state)
    {
#ifdef BLE_I2S_AUDIO_SUPPORT
        app_ble_audio_stream_start_pre_handler();
#endif
#ifdef __BIXBY
        app_bixby_on_suspend_handle();
#endif

        if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT) {
            app_overlay_select(APP_OVERLAY_SPEECH_ALGO);
        }

#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
        if (pStreamEnv->stream_info.bap_contextType != BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT) {
            app_anc_assist_set_mode(ANC_ASSIST_MODE_LE_CALL);
        }
#endif

#ifdef SPEECH_BONE_SENSOR
        if ((pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT) && (g_capture_sample_rate == AUD_SAMPRATE_16000)) {
            LOG_I("[%s] Enable VPU", __func__);
            g_capture_vpu_enabled = true;
        } else {
            LOG_I("[%s] Disable VPU", __func__);
            g_capture_vpu_enabled = false;
        }
#endif

        pStreamEnv->func_list->stream_func_list.capture_init_stream_buf_func(pStreamEnv);

        /**
         * Call after ANC_ASSIST switch mode, because ANC_ASSIST perhaps open VPU.
         * Make sure power on VPU in here.
         * If use VMIC for VPU and shared with MIC, you must move open VPU after af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
         */
#ifdef SPEECH_BONE_SENSOR
        if (g_capture_vpu_enabled) {
            speech_bone_sensor_open(16000, g_capture_frame_len);
// #ifdef VPU_STREAM_SYNC_TUNING
//         bt_sco_i2s_sync_tuning_reset();
// #endif

// #ifdef HW_I2S_TDM_TRIGGER
//         af_i2s_sync_config(AUD_STREAM_CAPTURE, AF_I2S_SYNC_TYPE_BT, true);
// #else
//         ASSERT(0, "Need to enable HW_I2S_TDM_TRIGGER");
// #endif
        }
#endif

#ifdef GAF_ENCODER_CROSS_CORE_USE_M55
        pStreamEnv->stream_info.tx_algo_cfg.channel_num = g_speech_algo_ch_num;
#ifdef SPEECH_BONE_SENSOR
        if (g_capture_vpu_enabled) {
            pStreamEnv->stream_info.tx_algo_cfg.channel_num++;
        }
#endif
        pStreamEnv->stream_info.tx_algo_cfg.frame_len = g_capture_frame_len;
        // Please set real algo processing frame len in this place.
        pStreamEnv->stream_info.tx_algo_cfg.algo_frame_len = g_capture_frame_len;
        pStreamEnv->stream_info.tx_algo_cfg.sample_rate = g_capture_sample_rate;
        pStreamEnv->stream_info.tx_algo_cfg.bits = pStreamEnv->stream_info.captureInfo.dma_info.bits_depth;

        if ((pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT) &&
        ((g_capture_sample_rate == 16000) || (g_capture_sample_rate == 32000)) &&
        (pStreamEnv->stream_info.tx_algo_cfg.bits == 24)) {
            pStreamEnv->stream_info.tx_algo_cfg.bypass = false;
            app_overlay_subsys_select(APP_OVERLAY_M55, APP_OVERLAY_SUBSYS_SPEECH_ALGO);
        } else {
            pStreamEnv->stream_info.tx_algo_cfg.bypass = true;
        }
#endif

        if (pStreamEnv->func_list->encoder_func_list)
        {
            pStreamEnv->func_list->encoder_func_list->encoder_init_func(
                        0, &(pStreamEnv->stream_info.captureInfo.codec_info));
        }
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        gaf_capture_encoder_init(pStreamEnv);
#endif

        struct AF_STREAM_CONFIG_T stream_cfg;

        af_set_priority(AF_USER_AOB_CAPTURE, osPriorityHigh);
        // capture stream
        memset_s((void *)&stream_cfg, sizeof(struct AF_STREAM_CONFIG_T),
               0, sizeof(struct AF_STREAM_CONFIG_T));
        stream_cfg.bits         = (enum AUD_BITS_T)(pStreamEnv->stream_info.captureInfo.dma_info.bits_depth);
        stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)g_capture_sample_rate;
        stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)g_capture_ch_num;
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path      = AUD_INPUT_PATH_MAINMIC;

        if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT)
        {
#ifdef BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE
            ASSERT((stream_cfg.sample_rate == 32000) || (stream_cfg.sample_rate == 48000),
                    "[%s] Don't support this sample rate: %d", __func__, stream_cfg.sample_rate);
            stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE;
#endif
            stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0);
        } else {
#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
            ASSERT(stream_cfg.bits == AUD_BITS_24, "[%s] bits(%d) != AUD_BITS_24", __func__, stream_cfg.bits);
            ASSERT(stream_cfg.sample_rate % AUD_SAMPRATE_16000 == 0, "[%s] sample_rate(%d) is invalid", __func__, stream_cfg.sample_rate);
            stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)app_anc_assist_get_mic_ch_map(AUD_INPUT_PATH_MAINMIC);
            app_anc_assist_set_capture_info(g_capture_frame_len / (g_capture_sample_rate / AUD_SAMPRATE_16000));
            if (g_capture_sample_rate != AUD_SAMPRATE_16000) {
                gaf_audio_anc_resample_heap_init();
                anc_assist_resample_init(g_capture_sample_rate,
                g_capture_frame_len, gaf_anc_assist_resample_get_ext_buf, gaf_anc_assist_resample_free_ext_buf);
            }
#elif defined(BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
            stream_cfg.io_path      = AUD_INPUT_PATH_MAINMIC;
            stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)(pStreamEnv->stream_info.captureInfo.dma_info.num_channels);
            if (AUD_CHANNEL_NUM_2 == stream_cfg.channel_num) {
                stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH1 | AUD_CHANNEL_MAP_CH0);
            }
            else {
                stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0);
            }
#endif
        }

        g_capture_ch_num        = stream_cfg.channel_num;

        stream_cfg.vol          = vol;
        stream_cfg.data_size    = (uint32_t)(2 * pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkSize);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr);
        stream_cfg.handler = pStreamEnv->func_list->stream_func_list.capture_dma_irq_handler_func;

#ifdef PSAP_SW_USE_DAC1
        af_codec_replace_echo_ref0_init(&stream_cfg, &g_capture_stream_channel_map, &g_capture_stream_channel_num);
#endif

        LOG_I("[%s] global frame_len: %d, ch_num: %d", __func__, g_capture_frame_len, g_capture_ch_num);

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

#ifdef STREAM_PROCESS_ENABLE
        gaf_stream_process_capture_open(pStreamEnv->stream_info.bap_contextType, &stream_cfg);
#endif
        gaf_stream_process_capture_upsampling_init(&pStreamEnv->stream_info, &stream_cfg);
        uint32_t stream_process_upsampling_buf_size = gaf_stream_process_need_upsampling_buf_size(pStreamEnv->stream_info.bap_contextType);
        if (stream_process_upsampling_buf_size != 0) {
            gaf_stream_process_set_upsampling_buf((uint8_t *)app_audio_mempool_calloc(stream_process_upsampling_buf_size, sizeof(int8_t)),
                                                  stream_process_upsampling_buf_size);
        }

        pStreamEnv->stream_context.isUsSinceLatestAnchorPointConfigured = false;
#if defined(IBRT)
        pStreamEnv->stream_context.captureTriggerChannel = app_ble_tws_sync_get_avaliable_trigger_channel();
#endif

        gaf_media_prepare_capture_trigger(pStreamEnv->stream_context.captureTriggerChannel);

        gaf_media_pid_init(&(pStreamEnv->stream_context.capture_pid_env));
        gaf_media_pid_update_threshold(&(pStreamEnv->stream_context.capture_pid_env),
            pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2);

        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);
        gaf_stream_common_start_sync_capture(pStreamEnv);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

#ifdef SPEECH_BONE_SENSOR
        if (g_capture_vpu_enabled) {
            speech_bone_sensor_start();
        }
#endif

        uint32_t capture_freq = APP_SYSFREQ_78M;
        if (pStreamEnv->func_list->encoder_func_list)
        {
            capture_freq = pStreamEnv->func_list->encoder_func_list->encoder_set_freq(
                &(pStreamEnv->stream_info.captureInfo.codec_info), APP_SYSFREQ_78M, M33_CORE);
        }
        if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT)
        {
#ifdef BINAURAL_RECORD_PROCESS
            capture_freq = APP_SYSFREQ_104M;
#else
            capture_freq = APP_SYSFREQ_78M;
#endif
        }
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, (enum APP_SYSFREQ_FREQ_T)capture_freq);
        LOG_I("flexible freq %d.", capture_freq);

        // FIXME: It's a workaround method
        if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT) {
            // Set sysfreq depend on algos
#ifdef GAF_CODEC_CROSS_CORE
            app_sysfreq_req(APP_SYSFREQ_USER_SPEECH_ALGO, APP_SYSFREQ_32K);
#else
            // app_sysfreq_req(APP_SYSFREQ_USER_SPEECH_ALGO, APP_SYSFREQ_104M);
            app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_104M);
#endif
        }

        LOG_D("%s end", __func__);
        return 0;
    }

    return -1;
}

static void gaf_audio_flexible_capture_buf_init(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    gaf_audio_flexible_common_buf_init(pStreamEnv, GAF_STREAM_CAPTURE);

    uint32_t frame_len =  pStreamEnv->stream_info.captureInfo.codec_info.frame_size *
            pStreamEnv->stream_info.captureInfo.codec_info.num_channels;
    if (NULL == pStreamEnv->stream_context.capture_frame_cache)
    {
        app_audio_mempool_get_buff(&(pStreamEnv->stream_context.capture_frame_cache), frame_len);
    }
    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_init_buf_func(0,
            &(pStreamEnv->stream_info.captureInfo.codec_info), (void*)gaf_stream_common_buf_alloc);
    }
    LOG_D("%s end", __func__);
}

static int gaf_audio_flexible_capture_stream_stop_handler(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    if (GAF_CAPTURE_STREAM_IDLE != pStreamEnv->stream_context.capture_stream_state)
    {
        bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.capture_retrigger_onprocess;
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_208M);
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_IDLE);

        uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
        uint32_t dma_base;
        // source
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_disable_dma_tc(adma_ch&0xFF, dma_base);
        }

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

#ifdef SPEECH_BONE_SENSOR
        if (g_capture_vpu_enabled) {
            speech_bone_sensor_stop();
            speech_bone_sensor_close();
// #ifdef VPU_STREAM_SYNC_TUNING
//         bt_sco_i2s_sync_tuning_reset();
// #endif
        }
        g_capture_vpu_enabled = false;
#endif

        gaf_stream_process_capture_upsampling_deinit();
#ifdef STREAM_PROCESS_ENABLE
        gaf_stream_process_capture_close();
#endif

        gaf_stream_common_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);

        af_codec_set_bt_sync_source(AUD_STREAM_CAPTURE, 0);

        af_set_priority(AF_USER_AOB_CAPTURE, osPriorityAboveNormal);

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        gaf_capture_encoder_deinit(pStreamEnv, isRetrigger);
#endif
        if (pStreamEnv->func_list->encoder_func_list)
        {
            pStreamEnv->func_list->encoder_func_list->encoder_deinit_func(0);
        }

        pStreamEnv->func_list->stream_func_list.capture_deinit_stream_buf_func(pStreamEnv);
#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
        if (g_capture_sample_rate != AUD_SAMPRATE_16000) {
            anc_assist_resample_deinit(g_capture_sample_rate);
        }
        app_anc_assist_set_mode(ANC_ASSIST_MODE_STANDALONE);
#endif
#ifdef BLE_I2S_AUDIO_SUPPORT
        app_ble_audio_stream_stop_post_handler();
#endif

        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);
        LOG_D("%s end", __func__);
        return 0;
    }

    return -1;
}

static void gaf_stream_capture_dma_irq_handler_send(void* pStreamEnv_, void *payload,
    uint32_t payload_size, uint32_t ref_time)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T*) pStreamEnv_;

    uint8_t ase_count = GAF_AUDIO_ASE_TOTAL_COUNT;
    uint8_t channel_shift = 0;
    uint8_t audio_allocation_cnt = 0;
    uint16_t payload_len_per_channel = 0;

    for (uint8_t i = 0; i < ase_count; i++)
    {
        if (GAF_AUDIO_INVALID_ISO_CHANNEL == pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl)
        {
            continue;
        }

        channel_shift = 0;
        audio_allocation_cnt = bes_ble_audio_get_location_fs_l_r_cnt(pStreamEnv->stream_info.captureInfo.aseChInfo[i].allocation_bf);
        // Config palyload len  for AUD_CHANNEL_NUM_2
        payload_len_per_channel = payload_size / pStreamEnv->stream_info.captureInfo.codec_info.num_channels;
        // Check is there use two ASE
        if (audio_allocation_cnt == AUD_CHANNEL_NUM_1)
        {
            if ((pStreamEnv->stream_info.captureInfo.aseChInfo[i].allocation_bf &
                (BES_BLE_LOC_SIDE_RIGHT | BES_BLE_LOC_FRONT_RIGHT)) &&
                pStreamEnv->stream_info.captureInfo.codec_info.num_channels == AUD_CHANNEL_NUM_2)
            {
                // shift to right channel
                channel_shift += payload_len_per_channel;
            }
        }
        else// if (audio_allocation_cnt == AUD_CHANNEL_NUM_2)
        {
            payload_len_per_channel = payload_size;
            channel_shift = 0;
        }

        /// gaf custom, may be a watch point to put some pattern in encoded packet to be send
        if (gaf_uc_srv_custom_data_callback->encoded_packet_send_cb)
        {
            gaf_uc_srv_custom_data_callback->encoded_packet_send_cb((uint8_t *)payload + channel_shift, payload_len_per_channel);
        }

        LOG_D("channel_shift: %d, payload_len_per_channel: %d", channel_shift, payload_len_per_channel);
        bes_ble_bap_iso_dp_send_data(pStreamEnv->stream_info.captureInfo.aseChInfo[i].ase_handle,
                                pStreamEnv->stream_context.latestCaptureSeqNum,
                                (uint8_t *)payload + channel_shift, payload_len_per_channel,
                                ref_time);
    }
}

static uint8_t *gaf_stream_captured_data_pre_processing_handler(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint8_t* ptrBuf, uint32_t* pLength)
{
    uint32_t length = *pLength;
    int32_t POSSIBLY_UNUSED *pcm_buf = (int32_t *)ptrBuf;
    uint32_t POSSIBLY_UNUSED pcm_len = length / sizeof(int32_t);
    uint32_t POSSIBLY_UNUSED frame_len = pcm_len / g_capture_ch_num;

#ifdef BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE
    if (pStreamEnv->stream_info.bap_contextType == BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT)
    {
        frame_len /= g_bin_record_sample_rate_div;
        for (uint32_t i=0; i<frame_len; i++) {
            for (uint32_t ch=0; ch<g_capture_ch_num; ch++) {    //BES intentional code. g_capture_ch_num will never be 0.
                pcm_buf[g_capture_ch_num * i + ch] = pcm_buf[g_capture_ch_num * g_bin_record_sample_rate_div * i + ch];
            }
        }
        length /= g_bin_record_sample_rate_div;
        pcm_len /= g_bin_record_sample_rate_div;
    }
#endif

#ifdef ANC_ASSIST_LE_AUDIO_SUPPORT
    if (pStreamEnv->stream_info.bap_contextType != BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT)
    {
        uint32_t anc_assist_len = length / (g_capture_sample_rate / AUD_SAMPRATE_16000);
        uint32_t frame_len = length / g_capture_ch_num / sizeof(float);
        float *temp_anc = (float *)g_anc_assist_interval_buf;

        if(app_anc_assist_is_runing()) {
            for (uint8_t ch = 0; ch < g_capture_ch_num; ch++) { //BES intentional code. g_capture_ch_num will never be 0.
                for (uint32_t i = 0; i < frame_len; i++) {
                    temp_anc[ch * frame_len + i] = (float)pcm_buf[i * g_capture_ch_num + ch];
                }
            }

            anc_assist_resample_process(temp_anc, frame_len, frame_len);

            if (g_capture_sample_rate != AUD_SAMPRATE_16000) {
                for (uint8_t ch = 0; ch < g_capture_ch_num; ch++) { //BES intentional code. g_capture_ch_num will never be 0.
                    for (uint32_t i = 0; i < (frame_len / (g_capture_sample_rate / AUD_SAMPRATE_16000)); i++) {
                        temp_anc[ch * (frame_len / (g_capture_sample_rate / AUD_SAMPRATE_16000)) + i] = temp_anc[ch * frame_len + i];
                    }
                }
            }
            //BES intentional code. g_capture_ch_num will never be 0.
            app_anc_assist_process_interval(temp_anc, anc_assist_len);
        }
        app_anc_assist_parser_app_mic_buf(ptrBuf, &length);
    }
#endif

// NOTE: Add gain in algo process flow
#if 0
    // Add 18 dB gain for MIC
    if (pStreamEnv->stream_info.captureInfo.bits_depth == 16) {
        int16_t *pcm_buf = (int16_t *)ptrBuf;
        uint32_t pcm_len = length / sizeof(int16_t);
        for (uint32_t i=0; i<pcm_len; i++) {
            pcm_buf[i] = __SSAT(((int32_t)(pcm_buf[i])) << 3, 16);
        }
    } else if (pStreamEnv->stream_info.captureInfo.bits_depth == 24) {
        int32_t *pcm_buf = (int32_t *)ptrBuf;
        uint32_t pcm_len = length / sizeof(int32_t);
        for (uint32_t i=0; i<pcm_len; i++) {
            pcm_buf[i] = __SSAT(pcm_buf[i] << 3, 24);
        }
    } else {
        ASSERT(0, "[%s] bits(%d) is invalid", __func__, pStreamEnv->stream_info.captureInfo.bits_depth);
    }
#endif

#ifdef STREAM_PROCESS_ENABLE
    length = gaf_stream_process_capture_run(ptrBuf, length);
#endif
    length = gaf_stream_process_capture_upsampling_run(ptrBuf, length);
    uint8_t *capture_upsampling_buf = gaf_stream_process_get_upsampling_buf();
    if (capture_upsampling_buf){
        ptrBuf = capture_upsampling_buf;
    }

#if defined(SPEECH_BONE_SENSOR) && defined(GAF_ENCODER_CROSS_CORE_USE_M55)
    if (g_capture_vpu_enabled) {
        speech_bone_sensor_get_data(g_capture_combo_vpu_pcm_buf, frame_len, SPEECH_BS_SINGLE_CHANNEL, 24);
        for (int32_t i=frame_len-1; i>=0; i--) {
            g_capture_combo_vpu_pcm_buf[i * (g_speech_algo_ch_num + 1) + g_speech_algo_ch_num] = g_capture_combo_vpu_pcm_buf[i];
        }

        for (uint32_t ch=0; ch<g_speech_algo_ch_num; ch++) {
            for (uint32_t i=0; i<frame_len; i++) {
                g_capture_combo_vpu_pcm_buf[i * (g_speech_algo_ch_num + 1) + ch] = pcm_buf[i * g_speech_algo_ch_num + ch];
            }
        }

        ptrBuf = (uint8_t *)g_capture_combo_vpu_pcm_buf;
        length = (length / g_speech_algo_ch_num) * (g_speech_algo_ch_num + 1);
    }
#endif

    *pLength = length;

    return ptrBuf;
}

static uint32_t _gaf_stream_flexible_capture_dma_irq_handler(uint8_t con_lid, uint8_t* ptrBuf, uint32_t length)
{
    LOG_D("%s start,length = %d", __func__,length);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = NULL;

    pStreamEnv = &gaf_audio_stream_env[con_lid];
    uint8_t muted = bes_ble_arc_get_mic_state(pStreamEnv->stream_info.con_lid);
    if (muted) {
        memset((uint8_t*)ptrBuf, 0x0, length);
    }

    if (!gaf_stream_is_capture_stream_iso_created(pStreamEnv))
    {
        return length;
    }

    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal microsecond -- 0.5 us
    uint8_t *output_buf = pStreamEnv->stream_context.capture_frame_cache;
    POSSIBLY_UNUSED CODEC_CORE_INFO_T *coreInfo = &pStreamEnv->stream_info.captureInfo.aseChInfo[0].codec_core_info;
    uint32_t frame_len = (uint32_t)(pStreamEnv->stream_info.captureInfo.codec_info.frame_size *
                pStreamEnv->stream_info.captureInfo.codec_info.num_channels);

    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }

    if ((GAF_CAPTURE_STREAM_START_TRIGGERING > pStreamEnv->stream_context.capture_stream_state)) {
        memset_s(ptrBuf, length, 0x00, length);
        return length;
    } else if (GAF_CAPTURE_STREAM_START_TRIGGERING == pStreamEnv->stream_context.capture_stream_state) {
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_STREAMING_TRIGGERED);
        gaf_stream_common_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);
        uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
        if (!pStreamEnv->stream_context.isUsSinceLatestAnchorPointConfigured)
        {
            pStreamEnv->stream_context.isUsSinceLatestAnchorPointConfigured = true;
            pStreamEnv->stream_context.usSinceLatestAnchorPoint = GAF_AUDIO_CLK_32_BIT_DIFF(dmaIrqHappeningTimeUs, latest_iso_bt_time);
            LOG_I("initial usSinceAnch %d", pStreamEnv->stream_context.usSinceLatestAnchorPoint);
            LOG_I("anch time %d dma irq time %d", latest_iso_bt_time, dmaIrqHappeningTimeUs);
            gaf_stream_common_sync_us_since_latest_anchor_point(pStreamEnv);
        }
    }

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((GAF_CAPTURE_STREAM_STREAMING_TRIGGERED ==
        pStreamEnv->stream_context.capture_stream_state) &&
        (dmaIrqHappeningTimeUs ==
        pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs))
    {
        LOG_W("accumulated irq messages happen!");
        return length;
    }

    gaf_stream_common_capture_timestamp_checker(pStreamEnv, dmaIrqHappeningTimeUs);

    LOG_D("%s length %d encoded_len %d filled timestamp %u", __func__, length,
        pStreamEnv->stream_info.captureInfo.codec_info.frame_size,
        dmaIrqHappeningTimeUs);

    /// gaf custom, may be a watch point to put some pattern in pcm raw data to be encode
    if (gaf_uc_srv_custom_data_callback->raw_pcm_data_cb)
    {
        gaf_uc_srv_custom_data_callback->raw_pcm_data_cb(ptrBuf, length);
    }

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
    if (NULL != capture_dma_irq_info_record_handler)
    {
        capture_dma_irq_info_record_handler(pStreamEnv->stream_info.con_lid, dmaIrqHappeningTimeUs,
                                            pStreamEnv->stream_context.latestCaptureSeqNum);
    }
#endif

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (pStreamEnv->stream_context.isUpStreamingStarted)
    {
        // bth fetch encoded data
        gaf_stream_common_fetch_frame(pStreamEnv, output_buf,
            frame_len, dmaIrqHappeningTimeUs, coreInfo->instance_handle);
        LOG_D("%s frame_len %d ", __func__, frame_len);
        gaf_stream_capture_dma_irq_handler_send(pStreamEnv, output_buf, frame_len, dmaIrqHappeningTimeUs);
    }

    ptrBuf = gaf_stream_captured_data_pre_processing_handler(pStreamEnv, ptrBuf, &length);
    gaf_stream_common_store_pcm(pStreamEnv, dmaIrqHappeningTimeUs, ptrBuf, length, frame_len, coreInfo);
#else
    ptrBuf = gaf_stream_captured_data_pre_processing_handler(pStreamEnv, ptrBuf, &length);

    pStreamEnv->func_list->encoder_func_list->encoder_encode_frame_func(0,
        &(pStreamEnv->stream_info.captureInfo.codec_info), length, ptrBuf, frame_len, output_buf);

    gaf_stream_capture_dma_irq_handler_send(pStreamEnv, output_buf, frame_len, dmaIrqHappeningTimeUs);
#endif
    LOG_D("%s end", __func__);
    return length;
}

static uint32_t gaf_stream_flexible_capture_idx_0_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    return _gaf_stream_flexible_capture_dma_irq_handler(0, ptrBuf, length);
}

static uint32_t gaf_stream_flexible_capture_idx_1_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    return _gaf_stream_flexible_capture_dma_irq_handler(0, ptrBuf, length);
}

static void gaf_audio_flexible_capture_buf_deinit(void* _pStreamEnv)
{
    LOG_D("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr = NULL;

    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_deinit_buf_func(0);
    }
    LOG_I("[%s] syspool free size: %d/%d", __func__, syspool_free_size(), syspool_total_size());
    LOG_D("%s end", __func__);
}

static GAF_AUDIO_FUNC_LIST_T gaf_audio_flexible_stream_func_list[GAF_AUDIO_CONTEXT_NUM_MAX] =
{
    {
        {
            .playback_dma_irq_handler_func = gaf_stream_flexible_playback_idx_0_dma_irq_handler,
            .capture_dma_irq_handler_func = gaf_stream_flexible_capture_idx_0_dma_irq_handler,

            .playback_start_stream_func = gaf_audio_flexible_playback_stream_start_handler,
            .playback_init_stream_buf_func = gaf_audio_flexible_playback_buf_init,
            .playback_stop_stream_func = gaf_audio_flexible_playback_stream_stop_handler,
            .playback_deinit_stream_buf_func = gaf_audio_flexible_playback_buf_deinit,

            .capture_start_stream_func = gaf_audio_flexible_capture_stream_start_handler,
            .capture_init_stream_buf_func = gaf_audio_flexible_capture_buf_init,
            .capture_stop_stream_func = gaf_audio_flexible_capture_stream_stop_handler,
            .capture_deinit_stream_buf_func = gaf_audio_flexible_capture_buf_deinit,
        }
    },

    {
        {
            .playback_dma_irq_handler_func = gaf_stream_flexible_playback_idx_1_dma_irq_handler,
            .capture_dma_irq_handler_func = gaf_stream_flexible_capture_idx_1_dma_irq_handler,

            .playback_start_stream_func = gaf_audio_flexible_playback_stream_start_handler,
            .playback_init_stream_buf_func = gaf_audio_flexible_playback_buf_init,
            .playback_stop_stream_func = gaf_audio_flexible_playback_stream_stop_handler,
            .playback_deinit_stream_buf_func = gaf_audio_flexible_playback_buf_deinit,

            .capture_start_stream_func = gaf_audio_flexible_capture_stream_start_handler,
            .capture_init_stream_buf_func = gaf_audio_flexible_capture_buf_init,
            .capture_stop_stream_func = gaf_audio_flexible_capture_stream_stop_handler,
            .capture_deinit_stream_buf_func = gaf_audio_flexible_capture_buf_deinit,
        }
    },
};

static GAF_AUDIO_STREAM_ENV_T* gaf_audio_get_stream_env_from_ase(uint8_t ase_lid)
{
    const bes_ble_bap_ascs_ase_t *p_bap_ase_info = bes_ble_get_ascs_ase_info(ase_lid);

    return &gaf_audio_stream_env[p_bap_ase_info->con_lid];
}

void gaf_audio_update_stream_iso_hdl(uint8_t ase_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_audio_get_stream_env_from_ase(ase_lid);
    if (!pStreamEnv)
    {
        LOG_E("[UPDATE ISO HDL] ase lid %d with no stream info now, please check!!!", ase_lid);
        return;
    }

    const bes_ble_bap_ascs_ase_t *p_bap_ase_info = bes_ble_get_ascs_ase_info(ase_lid);
    uint8_t aseChnInfoIdx = p_bap_ase_info->ase_lid % GAF_AUDIO_ASE_TOTAL_COUNT;
    GAF_AUDIO_STREAM_COMMON_INFO_T* pCommonInfo;
    if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction)
    {
        pCommonInfo = &(pStreamEnv->stream_info.playbackInfo);
    }
    else
    {
        pCommonInfo = &(pStreamEnv->stream_info.captureInfo);
    }

    pCommonInfo->aseChInfo[aseChnInfoIdx].iso_channel_hdl = BLE_ISOHDL_TO_ACTID(p_bap_ase_info->cis_hdl);
    pCommonInfo->aseChInfo[aseChnInfoIdx].ase_handle = p_bap_ase_info->cis_hdl;
    LOG_I("[UPDATE ISO HDL] ase lid %d cis hdl %d con_lid %d channel %d",
        ase_lid, pCommonInfo->aseChInfo[aseChnInfoIdx].ase_handle,
        p_bap_ase_info->con_lid, pCommonInfo->aseChInfo[aseChnInfoIdx].iso_channel_hdl);

    /// CIS connected and check for capture trigger set
    if (BES_BLE_GAF_DIRECTION_SRC == p_bap_ase_info->direction &&
        pStreamEnv->stream_context.capture_stream_state == GAF_CAPTURE_STREAM_INITIALIZED)
    {
        gaf_stream_common_start_sync_capture(pStreamEnv);
    }
}

POSSIBLY_UNUSED static GAF_AUDIO_STREAM_ENV_T* gaf_audio_refresh_stream_info_from_ase(uint8_t ase_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_audio_get_stream_env_from_ase(ase_lid);
    if (!pStreamEnv)
    {
        LOG_W("ase is not ready yet for refresh stream info from");
        return NULL;
    }
    const bes_ble_bap_ascs_ase_t *p_bap_ase_info = bes_ble_get_ascs_ase_info(ase_lid);
    GAF_AUDIO_STREAM_COMMON_INFO_T* pCommonInfo;
    uint8_t audio_alloc_l_r_cnt = 0;
    uint8_t aseChnInfoIdx = p_bap_ase_info->ase_lid % GAF_AUDIO_ASE_TOTAL_COUNT;
    uint8_t loc_supp_audio_location = 0;

    pStreamEnv->stream_info.bap_contextType = p_bap_ase_info->p_metadata->param.context_bf;
    LOG_I("[%s] TYPE bap: %d, app: %d", __func__, pStreamEnv->stream_info.bap_contextType, pStreamEnv->stream_info.contextType);

    if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction)
    {
        loc_supp_audio_location = bes_ble_bap_capa_get_location_bf(BES_BLE_GAF_DIRECTION_SINK);
        pCommonInfo = &(pStreamEnv->stream_info.playbackInfo);
    }
    else
    {
        loc_supp_audio_location = bes_ble_bap_capa_get_location_bf(BES_BLE_GAF_DIRECTION_SRC);
        pCommonInfo = &(pStreamEnv->stream_info.captureInfo);
    }
    /// Check for this stream's playback or capture info is refreshed
    if (pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf != 0)
    {
        LOG_I("already been refreshed %s stream info, allocation 0x%x via ase lid: %d",
              (p_bap_ase_info->direction == BES_BLE_GAF_DIRECTION_SINK ? "playback" : "capture"),
              pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf,
              ase_lid);
        return pStreamEnv;
    }
    /// update stream common info
    pCommonInfo->codec_info.num_channels = bes_ble_audio_get_location_fs_l_r_cnt(loc_supp_audio_location);
    pCommonInfo->cigSyncDelayUs = p_bap_ase_info->cig_sync_delay;
    pCommonInfo->isoIntervalUs = p_bap_ase_info->iso_interval_us;
    pCommonInfo->bnM2S = p_bap_ase_info->bn_m2s;
    pCommonInfo->bnS2M = p_bap_ase_info->bn_s2m;
    pCommonInfo->codec_info.bits_depth = GAF_AUDIO_STREAM_BIT_NUM;
    /// get custom set presDelay first
    pCommonInfo->presDelayUs = gaf_stream_common_get_custom_presdelay_us();
    if (pCommonInfo->presDelayUs == 0)
    {
        pCommonInfo->presDelayUs = p_bap_ase_info->qos_cfg.pres_delay_us > GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US ?
                                   p_bap_ase_info->qos_cfg.pres_delay_us : GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US;
    }

    pCommonInfo->aseChInfo[aseChnInfoIdx].iso_channel_hdl = BLE_ISOHDL_TO_ACTID(p_bap_ase_info->cis_hdl);
    pCommonInfo->aseChInfo[aseChnInfoIdx].ase_handle = p_bap_ase_info->cis_hdl;
    pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf = p_bap_ase_info->p_cfg->param.location_bf;

    audio_alloc_l_r_cnt = bes_ble_audio_get_location_fs_l_r_cnt(p_bap_ase_info->p_cfg->param.location_bf);
    /// multi channel allocation or no specified allocation check
    if ((audio_alloc_l_r_cnt >= AUD_CHANNEL_NUM_2 && p_bap_ase_info->qos_cfg.max_sdu_size ==
         p_bap_ase_info->p_cfg->param.frame_octet * p_bap_ase_info->p_cfg->param.frames_sdu) ||
        /// absence of audio allocation, value == 0
        (audio_alloc_l_r_cnt == 0) ||
        /// Sanity check
        (audio_alloc_l_r_cnt > pCommonInfo->codec_info.num_channels))
    {
        if (BES_BLE_AUDIO_TWS_SLAVE == bes_ble_audio_get_tws_nv_role())
        {
            pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf = BES_BLE_LOC_FRONT_LEFT;
        }
        else
        {
            pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf = BES_BLE_LOC_FRONT_RIGHT;
        }

        LOG_W("use mono channel with allocation: 0x%x",
              pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf);
    }

    LOG_I("ase_id:%d direction:%d codec_id:%d presDelayus:%u cigDelay:%u", ase_lid,
        p_bap_ase_info->direction, p_bap_ase_info->codec_id.codec_id[0], pCommonInfo->presDelayUs,
        pCommonInfo->cigSyncDelayUs);

    switch (p_bap_ase_info->codec_id.codec_id[0])
    {
        case BES_BLE_GAF_CODEC_TYPE_LC3:
        {
            AOB_BAP_CFG_T* p_lc3_cfg = (AOB_BAP_CFG_T *)p_bap_ase_info->p_cfg;
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
            AOB_BAP_CFG_T* p_lc3plus_cfg = p_bap_ase_info->p_cfg;
            pCommonInfo->codec_info.frame_ms =
                gaf_stream_common_frame_duration_parse(p_lc3plus_cfg->param.frame_dur);
            pCommonInfo->codec_info.sample_rate =
                gaf_stream_common_sample_freq_parse(p_lc3plus_cfg->param.sampling_freq);
            pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER;
            pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size();
            /// 2.5ms * GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER <= BAP Presdelay
            if (p_lc3plus_cfg->param.frame_dur == BES_BLE_GAF_BAP_FRAME_DURATION_2_5MS)
            {
                pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER*2;
                pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size()/2;
            }
            pCommonInfo->codec_info.frame_size = p_lc3plus_cfg->param.frame_octet;
            pCommonInfo->codec_info.bits_depth = AUD_BITS_24;
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
            AOB_BAP_CFG_T* p_ull_cfg = p_bap_ase_info->p_cfg;
            pCommonInfo->codec_info.frame_ms =
                gaf_stream_common_frame_duration_parse(p_ull_cfg->param.frame_dur);
            pCommonInfo->codec_info.sample_rate =
                gaf_stream_common_sample_freq_parse(p_ull_cfg->param.sampling_freq);
            pCommonInfo->codec_info.frame_size = p_ull_cfg->param.frame_octet;
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

    g_capture_sample_rate = pCommonInfo->codec_info.sample_rate;
    g_capture_sample_bytes = (pCommonInfo->codec_info.bits_depth / 8);
#ifdef __BLE_AUDIO_24BIT__
    g_capture_sample_bytes = sizeof(int32_t);
#endif
#ifdef LEA_CALL_FIX_ADC_SAMPLE_RATE
    if ((BES_BLE_GAF_DIRECTION_SRC == p_bap_ase_info->direction) &&
        (p_bap_ase_info->p_metadata->param.context_bf == BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT) &&
        (pCommonInfo->codec_info.sample_rate == AUD_SAMPRATE_32000)) {
        g_capture_sample_rate = AUD_SAMPRATE_16000;
    }
#endif

#if (defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)) && (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
    pCommonInfo->codec_info.num_channels = 1;
#endif

    pCommonInfo->codec_info.pcm_size =
        (uint32_t)(((pCommonInfo->codec_info.frame_ms * 1000)
        * pCommonInfo->codec_info.sample_rate * g_capture_sample_bytes *
        pCommonInfo->codec_info.num_channels) / (1000 * 1000));

    pCommonInfo->dma_info.num_channels = pCommonInfo->codec_info.num_channels;
#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT == 2)
    if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction) {
        pCommonInfo->dma_info.num_channels = 2;
    }
#endif
    pCommonInfo->dma_info.bits_depth = pCommonInfo->codec_info.bits_depth;
    pCommonInfo->dma_info.frame_ms = pCommonInfo->codec_info.frame_ms;
    pCommonInfo->dma_info.sample_rate = pCommonInfo->codec_info.sample_rate;
    pCommonInfo->dma_info.dmaChunkIntervalUs = (uint32_t)(pCommonInfo->dma_info.frame_ms * 1000);
    pCommonInfo->dma_info.dmaChunkSize = (uint32_t)((g_capture_sample_rate *
        g_capture_sample_bytes * (pCommonInfo->dma_info.dmaChunkIntervalUs) *
        pCommonInfo->dma_info.num_channels) / (1000 * 1000));

    LOG_I("isoIntervalUs:%d presDelayus:%u cigDelay:%u, bn m2s:%u s2m:%u appCtxType:%d",
         p_bap_ase_info->iso_interval_us, p_bap_ase_info->qos_cfg.pres_delay_us,
        p_bap_ase_info->cig_sync_delay, pCommonInfo->bnM2S, pCommonInfo->bnS2M, pStreamEnv->stream_info.contextType);
    LOG_I("ase lid %d cis hdl %d con_lid %d channel %d", ase_lid, pCommonInfo->aseChInfo[aseChnInfoIdx].ase_handle,
        p_bap_ase_info->con_lid, pCommonInfo->aseChInfo[aseChnInfoIdx].iso_channel_hdl);
    LOG_I("frame len %d us, sample rate %d dma chunk time %d us dma chunk size %d",
        (uint32_t)(pCommonInfo->codec_info.frame_ms*1000), pCommonInfo->dma_info.sample_rate,
        pCommonInfo->dma_info.dmaChunkIntervalUs, pCommonInfo->dma_info.dmaChunkSize);
    LOG_I("num of channel = %d", pCommonInfo->dma_info.num_channels);
    LOG_I("allocation: 0x%x", pCommonInfo->aseChInfo[aseChnInfoIdx].allocation_bf);
    LOG_I("context: %s",
        gaf_stream_common_print_context(p_bap_ase_info->p_metadata->param.context_bf));
    LOG_I("codec: %s", gaf_stream_common_print_code_type(p_bap_ase_info->codec_id.codec_id[0]));

    return pStreamEnv;
}

bool gaf_audio_is_capture_stream_on(void)
{
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = NULL;
    for (uint8_t con_lid = 0; con_lid < GAF_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        pStreamEnv = &gaf_audio_stream_env[con_lid];

        if (pStreamEnv->stream_context.capture_stream_state != GAF_CAPTURE_STREAM_IDLE) {
            return true;
        }
    }

    return false;
}

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
uint32_t gaf_get_ble_audio_playback_sample_rate(void)
{
    return gaf_audio_stream_env[0]->stream_info.playbackInfo.dma_info.sample_rate;
}
#endif

bool gaf_audio_is_playback_stream_on(void)
{
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = NULL;
    GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo = NULL;
    for (uint8_t con_lid = 0; con_lid < GAF_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        pStreamEnv = &gaf_audio_stream_env[con_lid];

        if (pStreamEnv->stream_context.capture_stream_state != GAF_CAPTURE_STREAM_IDLE ||
            pStreamEnv->stream_context.playback_stream_state != GAF_PLAYBACK_STREAM_IDLE)
        {
            pDewellingInfo = &gaf_cis_media_dwelling_info[con_lid];

            if (GAF_AUDIO_STREAM_TYPE_PLAYBACK & pDewellingInfo->startedStreamTypes ||
                GAF_AUDIO_STREAM_TYPE_FLEXIBLE & pDewellingInfo->startedStreamTypes)
            {
                return true;
            }
        }
    }

    return false;
}

gaf_stream_context_state_t gaf_audio_update_stream_state_info_from_ase(
                            GAF_AUDIO_UPDATE_STREAM_INFO_PURPOSE_E purpose, uint8_t ase_lid)
{
    const bes_ble_bap_ascs_ase_t *p_bap_ase_info = bes_ble_get_ascs_ase_info(ase_lid);

    LOG_I("update stream ase %d purpose %d", ase_lid, purpose);

    GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo =
        &gaf_cis_media_dwelling_info[p_bap_ase_info->con_lid];

    if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction)
    {
        if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_START == purpose)
        {
            gaf_audio_add_ase_into_playback_ase_list(p_bap_ase_info->con_lid, ase_lid);
            pDewellingInfo->startedStreamTypes |= GAF_AUDIO_STREAM_TYPE_PLAYBACK;
        }
        else
        {
            gaf_audio_remove_ase_from_playback_ase_list(p_bap_ase_info->con_lid, p_bap_ase_info->ase_lid);
            pDewellingInfo->startedStreamTypes &= (~GAF_AUDIO_STREAM_TYPE_PLAYBACK);
        }
    }
    else
    {
        if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_START == purpose)
        {
            gaf_audio_add_ase_into_capture_ase_list(p_bap_ase_info->con_lid, ase_lid);
            pDewellingInfo->startedStreamTypes |= GAF_AUDIO_STREAM_TYPE_CAPTURE;
        }
        else
        {
            gaf_audio_remove_ase_from_capture_ase_list(p_bap_ase_info->con_lid, p_bap_ase_info->ase_lid);
            pDewellingInfo->startedStreamTypes &= (~GAF_AUDIO_STREAM_TYPE_CAPTURE);
        }
    }

    LOG_I("direction %d started stream types updated to %d", p_bap_ase_info->direction,
        pDewellingInfo->startedStreamTypes);

    if (GAF_AUDIO_UPDATE_STREAM_INFO_TO_START == purpose)
    {
        // any direction of stream can start the context
        return APP_GAF_CONTEXT_STREAM_STARTED;
    }
    else
    {
        if (0 == pDewellingInfo->startedStreamTypes)
        {
            return APP_GAF_CONTEXT_ALL_STREAMS_STOPPED;
        }
        else if (0 == (pDewellingInfo->startedStreamTypes & GAF_AUDIO_STREAM_TYPE_CAPTURE))
        {
            return APP_GAF_CONTEXT_CAPTURE_STREAMS_STOPPED;
        }
        else
        {
            return APP_GAF_CONTEXT_PLAYBACK_STREAMS_STOPPED;
        }
    }
}

#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED)
uint8_t gaf_audio_get_stream_started_type(uint8_t con_lid)
{
    return gaf_cis_media_dwelling_info[con_lid].startedStreamTypes;
}
#endif

gaf_stream_context_state_t gaf_audio_stream_update_and_start_handler(uint8_t ase_lid)
{
    LOG_I("%s start", __func__);
    gaf_stream_context_state_t updatedContextStreamState =
        gaf_audio_update_stream_state_info_from_ase(GAF_AUDIO_UPDATE_STREAM_INFO_TO_START, ase_lid);

    gaf_media_stream_boost_freq(GAF_MEDIA_STREAM_POST_START_FREQ_BOOST_TIMEOUT_VALUE);

    return updatedContextStreamState;
}

static GAF_AUDIO_STREAM_ENV_T* _gaf_refresh_all_stream_info_by_con_lid(uint8_t con_lid)
{
    uint8_t *playback_ase_lid_list = gaf_audio_get_playback_ase_index_list(con_lid);
    uint8_t *capture_ase_lid_list = gaf_audio_get_capture_ase_index_list(con_lid);
    uint8_t idx = 0;
    GAF_AUDIO_STREAM_ENV_T* pPlaybackStreamEnv = NULL;
    GAF_AUDIO_STREAM_ENV_T* pCaptureStreamEnv = NULL;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = NULL;

    for (idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
    {
        if (GAF_INVALID_ASE_INDEX != capture_ase_lid_list[idx])
        {
            pStreamEnv = gaf_audio_refresh_stream_info_from_ase(capture_ase_lid_list[idx]);
            pCaptureStreamEnv = (pStreamEnv == NULL) ?
                                        pCaptureStreamEnv : pStreamEnv;
            // No break for update all un refresh stream info
        }
    }

    for (idx = 0; idx < GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT; idx++)
    {
        if (GAF_INVALID_ASE_INDEX != playback_ase_lid_list[idx])
        {
            pStreamEnv = gaf_audio_refresh_stream_info_from_ase(playback_ase_lid_list[idx]);
            pPlaybackStreamEnv = (pStreamEnv == NULL) ?
                                        pPlaybackStreamEnv : pStreamEnv;
            // No break for update all un refresh stream info
        }
    }

    if (pPlaybackStreamEnv && pCaptureStreamEnv)
    {
        ASSERT(pPlaybackStreamEnv == pCaptureStreamEnv, "Same context should use the same env!");
    }

    if (pPlaybackStreamEnv)
    {
        pPlaybackStreamEnv->stream_info.con_lid = con_lid;
        return pPlaybackStreamEnv;
    }
    else if (pCaptureStreamEnv)
    {
        pCaptureStreamEnv->stream_info.con_lid = con_lid;
        return pCaptureStreamEnv;
    }
    else
    {
        return NULL;
    }
}

static void _gaf_audio_stream_start(uint8_t con_lid, uint8_t ase_lid)
{
    LOG_I("%s start,con_lid = 0x%x,ase_lid = 0x%x", __func__,con_lid,ase_lid);
#ifdef GAF_DSP
    app_dsp_hifi4_init(APP_DSP_HIFI4_USER_GAF_DSP);
#endif

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = _gaf_refresh_all_stream_info_by_con_lid(con_lid);

    if (pStreamEnv)
    {
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        const bes_ble_bap_ascs_ase_t *p_bap_ase_info = bes_ble_get_ascs_ase_info(ase_lid);
        if (BES_BLE_GAF_DIRECTION_SINK == p_bap_ase_info->direction)
        {
            gaf_playback_decoder_init(pStreamEnv, ase_lid);
        }
#endif

        if ((GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state) &&
            (GAF_PLAYBACK_STREAM_IDLE == pStreamEnv->stream_context.playback_stream_state))
        {
            bt_adapter_write_sleep_enable(0);
            gaf_stream_register_running_stream_ref(con_lid, pStreamEnv);
        }

        GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo = &gaf_cis_media_dwelling_info[con_lid];

        if ((pDewellingInfo->startedStreamTypes & GAF_AUDIO_STREAM_TYPE_PLAYBACK) &&
            (GAF_PLAYBACK_STREAM_IDLE == pStreamEnv->stream_context.playback_stream_state))
        {
            uint8_t playback_ase_lid = GAF_INVALID_ASE_INDEX;
            uint8_t *playback_ase_lid_list = gaf_audio_get_playback_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != playback_ase_lid_list[ase_lid])
                {
                    playback_ase_lid = playback_ase_lid_list[ase_lid];
                    break;
                }
            }

            ASSERT(playback_ase_lid != GAF_INVALID_ASE_INDEX, "invalid playback ase lid");

            gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZING);
            pStreamEnv->func_list->stream_func_list.playback_start_stream_func(pStreamEnv);
        }

        if ((pDewellingInfo->startedStreamTypes & GAF_AUDIO_STREAM_TYPE_CAPTURE) &&
            (GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state))
        {
            uint8_t capture_ase_lid = GAF_INVALID_ASE_INDEX;
            uint8_t *capture_ase_lid_list = gaf_audio_get_capture_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != capture_ase_lid_list[ase_lid])
                {
                    capture_ase_lid = capture_ase_lid_list[ase_lid];
                    break;
                }
            }
            ASSERT(capture_ase_lid != GAF_INVALID_ASE_INDEX, "invalid capture ase lid");

            gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZING);
            pStreamEnv->func_list->stream_func_list.capture_start_stream_func(pStreamEnv);
        }
    }

    LOG_D("%s end", __func__);
}

void gaf_audio_stream_start(uint8_t con_lid, uint8_t ase_lid)
{
    _gaf_audio_stream_start(con_lid, ase_lid);
}

static void gaf_audio_stream_stop_generic(bool isCheckServiceState, uint8_t con_lid, uint8_t stream_lid)
{
    LOG_D("%s", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_audio_stream_env[con_lid];

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    uint8_t pendingStopOp;
    if (isCheckServiceState)
    {
        pendingStopOp = PENDING_TO_STOP_BLE_AUDIO_SINGLE_STREAMING;
    }
    else
    {
        pendingStopOp = PENDING_TO_STOP_BLE_AUDIO_STREAMING;
    }

    bool ret = audio_prompt_check_on_stopping_ble_audio_stream(pendingStopOp,
                    con_lid, stream_lid, (void*)gaf_audio_stream_stop_generic);
    if (!ret)
    {
        TRACE(0,"Pending stop BLE_AUDIO_STREAM");
        return;
    }
#endif

    /// Check stream state
    if (pStreamEnv &&
        (GAF_CAPTURE_STREAM_IDLE != pStreamEnv->stream_context.capture_stream_state ||
         GAF_PLAYBACK_STREAM_IDLE != pStreamEnv->stream_context.playback_stream_state))
    {
        GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo = &gaf_cis_media_dwelling_info[con_lid];

        if (isCheckServiceState)
        {
            if ((0 == (pDewellingInfo->startedStreamTypes&GAF_AUDIO_STREAM_TYPE_PLAYBACK)) &&
                (GAF_PLAYBACK_STREAM_IDLE != pStreamEnv->stream_context.playback_stream_state))
            {
                pStreamEnv->func_list->stream_func_list.playback_stop_stream_func(pStreamEnv);
                for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
                {
                    pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
                    pStreamEnv->stream_info.playbackInfo.aseChInfo[i].allocation_bf = 0;
                    pStreamEnv->stream_context.lastestPlaybackSeqNum[i] = GAF_AUDIO_INVALID_SEQ_NUMBER;
                }
            }

            if ((0 == (pDewellingInfo->startedStreamTypes&GAF_AUDIO_STREAM_TYPE_CAPTURE)) &&
                (GAF_CAPTURE_STREAM_IDLE != pStreamEnv->stream_context.capture_stream_state))
            {
                pStreamEnv->func_list->stream_func_list.capture_stop_stream_func(pStreamEnv);
                for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
                {
                    pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
                    pStreamEnv->stream_info.captureInfo.aseChInfo[i].allocation_bf = 0;
                }
            }
        }
        else
        {
            pStreamEnv->func_list->stream_func_list.playback_stop_stream_func(pStreamEnv);
            for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
            {
                pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
                pStreamEnv->stream_info.playbackInfo.aseChInfo[i].allocation_bf = 0;
                pStreamEnv->stream_context.lastestPlaybackSeqNum[i] = GAF_AUDIO_INVALID_SEQ_NUMBER;
            }

            pStreamEnv->func_list->stream_func_list.capture_stop_stream_func(pStreamEnv);
            for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
            {
                pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
                pStreamEnv->stream_info.captureInfo.aseChInfo[i].allocation_bf = 0;
            }
        }

        /// Should be transfered to IDLE by previous procedure, avoid multi call
        if ((GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state) &&
            (GAF_PLAYBACK_STREAM_IDLE == pStreamEnv->stream_context.playback_stream_state))
        {
            gaf_stream_register_running_stream_ref(con_lid, NULL);
            bt_adapter_write_sleep_enable(1);
        }
    }
    else
    {
        LOG_W("%s no stream to stop, may be already stopped", __func__);
    }

#ifdef GAF_DSP
    app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_GAF_DSP);
#endif

}

void gaf_audio_stream_stop(uint8_t con_lid, uint8_t ase_lid)
{
    gaf_audio_stream_stop_generic(false, con_lid, ase_lid);
}

void gaf_audio_stream_refresh_and_stop(uint8_t conlid, uint16_t stream_type, uint8_t stream_lid)
{
    gaf_audio_stream_stop_generic(true, conlid, stream_lid);
}

static void gaf_audio_steam_retrigger_handler(void* stream_env, uint32_t streamType)
{
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)stream_env;
    GAF_MEDIA_DWELLING_INFO_T gaf_cis_media_dwelling_info_rec;
    // Record this
    memcpy_s(&gaf_cis_media_dwelling_info_rec, sizeof(GAF_MEDIA_DWELLING_INFO_T),
                &gaf_cis_media_dwelling_info[pStreamEnv->stream_info.con_lid],
                sizeof(GAF_MEDIA_DWELLING_INFO_T));
    // Stop this must in this thread
    gaf_cis_media_dwelling_info[pStreamEnv->stream_info.con_lid].startedStreamTypes &= ~streamType;
    gaf_audio_stream_stop_generic(true, pStreamEnv->stream_info.con_lid, pStreamEnv->stream_info.contextType);

    // Recovery this
    memcpy_s(&gaf_cis_media_dwelling_info[pStreamEnv->stream_info.con_lid],
                sizeof(GAF_MEDIA_DWELLING_INFO_T),
                &gaf_cis_media_dwelling_info_rec, sizeof(GAF_MEDIA_DWELLING_INFO_T));
    // Start this in other thread is ok
    for (uint8_t con_lid = 0; con_lid < GAF_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        if (GAF_AUDIO_STREAM_TYPE_PLAYBACK == streamType)
        {
            uint8_t *playback_ase_lid_list = gaf_audio_get_playback_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != playback_ase_lid_list[ase_lid])
                {
                    gaf_audio_stream_start(con_lid, playback_ase_lid_list[ase_lid]);
#if (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT != 2)
                    break;
#endif
                }
            }
        }
        else
        {
            uint8_t *capture_ase_lid_list = gaf_audio_get_capture_ase_index_list(con_lid);
            for (uint8_t ase_lid = 0; ase_lid < GAF_AUDIO_ASE_TOTAL_COUNT - 1; ase_lid++)
            {
                if (GAF_INVALID_ASE_INDEX != capture_ase_lid_list[ase_lid])
                {
                    gaf_audio_stream_start(con_lid, capture_ase_lid_list[ase_lid]);
                    break;
                }
            }
        }
    }
}

void gaf_audio_stream_register_func_list(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, GAF_AUDIO_FUNC_LIST_T* funcList)
{
    pStreamEnv->func_list = funcList;
}

void gaf_audio_stream_init(void)
{
    /// GAF CUSTOM CALLBACK PTR
    gaf_uc_srv_custom_data_callback = gaf_stream_common_get_custom_data_handler(GAF_STREAM_USER_CASE_UC_SRV);
    /// Sanity check
    ASSERT(gaf_uc_srv_custom_data_callback, "Invalid custom data callback, user case %d", GAF_STREAM_USER_CASE_UC_SRV);

    gaf_audio_dwelling_info_list_init();

    for (uint8_t con_lid = 0; con_lid < GAF_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        GAF_AUDIO_STREAM_ENV_T *pStreamEnv = &gaf_audio_stream_env[con_lid];
        memset_s((uint8_t *)pStreamEnv,sizeof(GAF_AUDIO_STREAM_ENV_T),  0, sizeof(GAF_AUDIO_STREAM_ENV_T));
        pStreamEnv->stream_context.playback_stream_state = GAF_PLAYBACK_STREAM_IDLE;
        pStreamEnv->stream_context.capture_stream_state = GAF_CAPTURE_STREAM_IDLE;
        pStreamEnv->stream_info.playbackInfo.presDelayUs = GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US;
        pStreamEnv->stream_info.captureInfo.presDelayUs = GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US;
        pStreamEnv->stream_context.playback_trigger_supervisor_timer_id = NULL;
        pStreamEnv->stream_context.capture_trigger_supervisor_timer_id = NULL;
        pStreamEnv->stream_context.playback_retrigger_onprocess = false;
        pStreamEnv->stream_context.capture_retrigger_onprocess = false;
        pStreamEnv->stream_info.is_mobile = false;
        pStreamEnv->stream_info.is_bis = false;
        pStreamEnv->stream_info.contextType = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
        gaf_stream_common_register_func_list(pStreamEnv, &gaf_audio_flexible_stream_func_list[con_lid]);
        gaf_playback_status_mutex_init(pStreamEnv->stream_info.gaf_playback_status_mutex);
        gaf_audio_clear_playback_ase_index_list(con_lid);
        gaf_audio_clear_capture_ase_index_list(con_lid);

        for (uint8_t i = 0; i < GAF_AUDIO_ASE_TOTAL_COUNT; i++)
        {
            pStreamEnv->stream_info.playbackInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
            pStreamEnv->stream_info.captureInfo.aseChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
            pStreamEnv->stream_info.playbackInfo.aseChInfo[i].allocation_bf = 0;
            pStreamEnv->stream_info.captureInfo.aseChInfo[i].allocation_bf = 0;
            pStreamEnv->stream_context.lastestPlaybackSeqNum[i] = GAF_AUDIO_INVALID_SEQ_NUMBER;
            pStreamEnv->stream_info.playbackInfo.aseChInfo[i].codec_core_info.instance_status = INSTANCE_IDLE;
            pStreamEnv->stream_info.captureInfo.aseChInfo[i].codec_core_info.instance_status = INSTANCE_IDLE;
        }
    }

    gaf_stream_common_register_cc_func_list();
    gaf_stream_register_retrigger_callback(gaf_audio_steam_retrigger_handler);
}

void gaf_audio_playback_closeall(void)
{
    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = NULL;
    GAF_MEDIA_DWELLING_INFO_T* pDewellingInfo = NULL;
    for (uint8_t con_lid = 0; con_lid < GAF_MAXIMUM_CONNECTION_COUNT; con_lid++)
    {
        pStreamEnv = &gaf_audio_stream_env[con_lid];

        if (pStreamEnv->stream_context.capture_stream_state != GAF_CAPTURE_STREAM_IDLE ||
            pStreamEnv->stream_context.playback_stream_state != GAF_PLAYBACK_STREAM_IDLE)
        {
            pDewellingInfo = &gaf_cis_media_dwelling_info[con_lid];

            if (GAF_AUDIO_STREAM_TYPE_PLAYBACK & pDewellingInfo->startedStreamTypes ||
                GAF_AUDIO_STREAM_TYPE_FLEXIBLE & pDewellingInfo->startedStreamTypes)
            {
                gaf_audio_stream_stop(con_lid,0);
            }
        }
    }
}
#endif

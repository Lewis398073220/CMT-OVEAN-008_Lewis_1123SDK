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
#ifdef USB_BIS_AUDIO_STREAM
#ifdef BLE_USB_AUDIO_IS_DONGLE_ROLE
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "app_utils.h"
#include "plat_types.h"
#include "audioflinger.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "app_bt_func.h"
#include "app_audio.h"
#include "usb_audio_app.h"
#include "aob_media_api.h"
#include "ble_audio_dbg.h"
#include "gaf_codec_lc3.h"
#include "app_bap_data_path_itf.h"
#include "gaf_media_common.h"
#include "rwble_config.h"
#include "app_usb_bis_stream.h"
#include "bt_drv_interface.h"
#include "aob_bis_api.h"
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
#include "app_usb_hw_timer.h"
#endif

/************************private macro defination***************************/

/************************private strcuture defination****************************/

/************************private variable defination************************/
static bool app_is_usb_bis_src_streaming = false;
GAF_AUDIO_STREAM_ENV_T gaf_usb_bis_src_audio_stream_env;
static GAF_AUDIO_FUNC_LIST_T gaf_usb_bis_src_audio_stream_func_list;

/****************************function defination****************************/

static void* gaf_usb_bis_src_audio_get_media_stream_env(void)
{
    return &gaf_usb_bis_src_audio_stream_env;
}

static void gaf_bis_src_iso_data_send(void* pStreamEnv_,void *payload,
    uint32_t payload_size, uint32_t ref_time)
{
    uint8_t *output_buf[2];
    output_buf[0] = (uint8_t*)payload;
    output_buf[1] = NULL;
    bes_ble_bis_src_send_iso_data_to_all_channel(output_buf, payload_size, ref_time);
}

static uint32_t gaf_usb_bis_src_processing_pcm_data(uint8_t *ptrBuf, uint32_t length)
{
    uint32_t dmaIrqHappeningTimeUs = 0;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
            (GAF_AUDIO_STREAM_ENV_T *)gaf_usb_bis_src_audio_get_media_stream_env();
    CODEC_CORE_INFO_T *coreInfo =
        &pStreamEnv->stream_info.captureInfo.bisChInfo[0].codec_core_info;
    uint8_t *output_buf = pStreamEnv->stream_context.capture_frame_cache;
    uint32_t frame_len = (uint32_t)(pStreamEnv->stream_info.captureInfo.codec_info.frame_size);
    int32_t channelNum = pStreamEnv->stream_info.captureInfo.dma_info.num_channels;

    if (coreInfo->instance_status < INSTANCE_INITIALIZED) {
        return length;
    }
    if (!app_is_usb_bis_src_streaming) {
        memset(ptrBuf, 0, length);
    }
    if (GAF_CAPTURE_STREAM_START_TRIGGERING == pStreamEnv->stream_context.capture_stream_state)
    {
        gaf_stream_common_update_capture_stream_state(pStreamEnv,
                                            GAF_CAPTURE_STREAM_STREAMING_TRIGGERED);
    }
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
    uint32_t curr_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    while ((int32_t)(latest_iso_bt_time - curr_bt_time) < 0)
    {
        latest_iso_bt_time += pStreamEnv->stream_info.captureInfo.isoIntervalUs;
    }
    dmaIrqHappeningTimeUs = latest_iso_bt_time - pStreamEnv->stream_info.captureInfo.isoIntervalUs;
#else
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint8_t adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    uint32_t dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }
#endif

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if ((GAF_CAPTURE_STREAM_STREAMING_TRIGGERED == pStreamEnv->stream_context.capture_stream_state)
            && (dmaIrqHappeningTimeUs == pStreamEnv->stream_context.lastCaptureDmaIrqTimeUs))
    {
        LOG_W("accumulated irq messages happen!");
        return length;
    }
    gaf_stream_common_capture_timestamp_checker(pStreamEnv, dmaIrqHappeningTimeUs);

    LOG_D("length %d encoded_len %d filled timestamp %d", length,
        pStreamEnv->stream_info.captureInfo.codec_info.frame_size, dmaIrqHappeningTimeUs);

#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
     if (pStreamEnv->stream_context.isUpStreamingStarted)
     {
         gaf_stream_common_fetch_frame(pStreamEnv, output_buf,
             frame_len * channelNum, dmaIrqHappeningTimeUs, coreInfo->instance_handle);
         gaf_bis_src_iso_data_send(pStreamEnv,
             output_buf, frame_len * channelNum, dmaIrqHappeningTimeUs);
     }
     gaf_stream_common_store_pcm(pStreamEnv, dmaIrqHappeningTimeUs,
             ptrBuf, length, frame_len * channelNum, coreInfo);
#else
     pStreamEnv->func_list->encoder_func_list->encoder_encode_frame_func(0,
         &(pStreamEnv->stream_info.captureInfo.codec_info), length, ptrBuf, frame_len, output_buf);

     gaf_bis_src_iso_data_send(pStreamEnv,
             output_buf, frame_len * channelNum, dmaIrqHappeningTimeUs);
#endif
    return length;
}

static void gaf_usb_bis_src_prepare(enum AUD_STREAM_T stream)
{
    if (AUD_STREAM_PLAYBACK == stream)
    {
        uint8_t adma_ch = HAL_DMA_CHAN_NONE;
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        uint32_t dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);

        if (HAL_DMA_CHAN_NONE != adma_ch)
        {
            bt_drv_reg_op_enable_dma_tc(adma_ch&0xFF, dma_base);
        }
        else
        {
            LOG_I("error adma_ch HAL_DMA_CHAN_NONE %d", adma_ch);
        }
    }
}

static void gaf_usb_bis_src_stop(void)
{
    app_is_usb_bis_src_streaming = false;
}

static void gaf_usb_bis_src_start(void)
{
    app_is_usb_bis_src_streaming = true;
}

static const USB_AUDIO_SOURCE_EVENT_CALLBACK_T usb_bis_src_audio_stream_func_cb_list =
{
    .data_playback_cb = gaf_usb_bis_src_processing_pcm_data,
    .data_capture_cb = NULL,
    .data_prep_cb = gaf_usb_bis_src_prepare,

    .playback_start_cb = gaf_usb_bis_src_start,
    .playback_stop_cb = gaf_usb_bis_src_stop,
    .capture_start_cb = NULL,
    .capture_stop_cb = NULL,

    .playback_vol_change_cb = NULL,
};

static void app_usb_start_bis_src_stream(void)
{
    uint32_t big_idx = 0;
    app_bap_bc_src_grp_info_t *p_grp_t = app_bap_bc_src_get_big_info_by_big_idx(big_idx);

    if (APP_BAP_BC_SRC_STATE_IDLE == p_grp_t->big_state)
    {
        app_bap_bc_src_start(big_idx);
    }
    else if (APP_BAP_BC_SRC_STATE_CONFIGURED == p_grp_t->big_state)
    {
        app_bap_bc_src_enable(p_grp_t);
    }
    else
    {
        LOG_W("big_state:%d bis src has been streaming", p_grp_t->big_state);
    }
}

void app_usb_bis_src_init(void)
{
    app_sysfreq_req(APP_SYSFREQ_USER_USB_AUDIO_STREAMING, APP_SYSFREQ_208M);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv =
        (GAF_AUDIO_STREAM_ENV_T*)gaf_usb_bis_src_audio_get_media_stream_env();
    memset((uint8_t *)pStreamEnv, 0, sizeof(GAF_AUDIO_STREAM_ENV_T));
    pStreamEnv->stream_context.capture_stream_state = GAF_CAPTURE_STREAM_IDLE;
    pStreamEnv->stream_context.capture_trigger_supervisor_timer_id = NULL;
    pStreamEnv->stream_info.is_mobile = true;
    pStreamEnv->stream_info.is_bis = true;
    for (uint8_t i = 0; i < 2; i++)
    {
        pStreamEnv->stream_info.captureInfo.bisChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
        pStreamEnv->stream_info.captureInfo.bisChInfo[i].codec_core_info.instance_status = INSTANCE_IDLE;
    }
    gaf_stream_common_register_cc_func_list();
    usb_audio_source_config_init(&usb_bis_src_audio_stream_func_cb_list);
    gaf_stream_common_register_func_list(pStreamEnv, &gaf_usb_bis_src_audio_stream_func_list);
    gaf_stream_register_retrigger_callback(NULL);
    gaf_mobile_stream_register_retrigger_callback(NULL);
    app_bt_start_custom_function_in_bt_thread(0,
                0, (uint32_t)app_usb_start_bis_src_stream);
#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
    gaf_usb_hw_timer_init();
#endif
}

#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
static void gaf_usb_bis_capture_hw_timer_trigger_start(GAF_AUDIO_STREAM_ENV_T* pStreamEnv)
{
    uint32_t curr_bt_time = bt_syn_ble_bt_time_to_bts(btdrv_syn_get_curr_ticks(), 0);
    uint32_t latest_iso_bt_time = gaf_media_common_get_latest_tx_iso_evt_timestamp(pStreamEnv);
    while ((int32_t)(latest_iso_bt_time - curr_bt_time) < GAF_AUDIO_CAPTURE_TRIGGER_DELAY_IN_US)
    {
        latest_iso_bt_time += pStreamEnv->stream_info.captureInfo.isoIntervalUs;
    }
    gaf_stream_common_set_capture_trigger_info(pStreamEnv,
        latest_iso_bt_time - pStreamEnv->stream_info.captureInfo.isoIntervalUs);
    gaf_usb_capture_hw_timer_start(pStreamEnv, latest_iso_bt_time);
    gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_START_TRIGGERING);
}
#endif

static GAF_AUDIO_STREAM_ENV_T *gaf_usb_bis_src_refresh_stream_env_from_grp_info(uint8_t grp_lid)
{
    uint8_t big_idx = app_bap_bc_src_find_big_idx(grp_lid);
    app_bap_bc_src_grp_info_t *p_grp_t = app_bap_bc_src_get_big_info_by_big_idx(big_idx);
    AOB_BAP_CFG_T* p_stream_cfg = (AOB_BAP_CFG_T*)p_grp_t->stream_info->p_cfg;

    GAF_AUDIO_STREAM_ENV_T *pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)gaf_usb_bis_src_audio_get_media_stream_env();
    GAF_AUDIO_STREAM_COMMON_INFO_T* pCommonInfo = &(pStreamEnv->stream_info.captureInfo);

    pCommonInfo->bisChInfo[0].iso_channel_hdl = BLE_BISHDL_TO_ACTID(p_grp_t->stream_info->bis_hdl);
    pCommonInfo->isoIntervalUs = p_grp_t->big_param.grp_param.sdu_intv_us;
    pCommonInfo->codec_info.num_channels = AUD_CHANNEL_NUM_2;
    pCommonInfo->codec_info.bits_depth = AUD_BITS_16;

    switch (p_grp_t->big_param.bcast_id.id[0])
    {
    case BES_BLE_GAF_CODEC_TYPE_LC3:
    {
        pCommonInfo->codec_info.frame_ms =
            gaf_stream_common_frame_duration_parse(p_stream_cfg->param.frame_dur);
        pCommonInfo->codec_info.sample_rate =
            gaf_stream_common_sample_freq_parse(p_stream_cfg->param.sampling_freq);
        pCommonInfo->codec_info.frame_size = p_stream_cfg->param.frame_octet;
        pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER;
        pCommonInfo->maxFrameSize = gaf_audio_lc3_encoder_get_max_frame_size();
        pStreamEnv->stream_info.codec_type = LC3;
#if !defined(GAF_CODEC_CROSS_CORE) && !defined(AOB_CODEC_CP)
        gaf_audio_lc3_update_decoder_func_list(&(pStreamEnv->func_list->decoder_func_list));
        gaf_audio_lc3_update_encoder_func_list(&(pStreamEnv->func_list->encoder_func_list));
#endif
        break;
    }
#ifdef GSBC_SUPPORT
    case BES_BLE_GAF_CODEC_TYPE_GSBC:
    {
        pCommonInfo->codec_info.frame_ms =
            gaf_stream_common_frame_duration_parse(p_stream_cfg->param.frame_dur);
        pCommonInfo->codec_info.sample_rate =
            gaf_stream_common_sample_freq_parse(p_stream_cfg->param.sampling_freq);
        pCommonInfo->codec_info.frame_size = p_stream_cfg->param.frame_octet;
        pCommonInfo->maxCachedFrameCount = GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER;
        pCommonInfo->maxFrameSize = gaf_audio_gsbc_encoder_get_max_frame_size();
        pStreamEnv->stream_info.codec_type = GSBC;
#if !defined(GAF_CODEC_CROSS_CORE) && !defined(AOB_CODEC_CP)
        gaf_audio_gsbc_update_decoder_func_list(&(pStreamEnv->func_list->decoder_func_list));
        gaf_audio_gsbc_update_encoder_func_list(&(pStreamEnv->func_list->encoder_func_list));
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

    LOG_I("pres_delay_us:%d encrypted:%d",  p_grp_t->big_param.pres_delay_us,  p_grp_t->big_param.encrypted);
    DUMP8("%02x ", p_grp_t->big_param.bcast_id.id, BAP_BC_BROADCAST_ID_LEN);
    DUMP8("%02x ", p_grp_t->big_param.bcast_code.bcast_code, APP_GAP_KEY_LEN);

    LOG_I("iso interval:%d num_channels:%d",
        pCommonInfo->isoIntervalUs, pCommonInfo->dma_info.num_channels);
    LOG_I("frame len %d us, sample rate %d dma chunk time %d us dma chunk size %d",
        (uint32_t)(pCommonInfo->dma_info.frame_ms*1000), pCommonInfo->dma_info.sample_rate,
        pCommonInfo->dma_info.dmaChunkIntervalUs, pCommonInfo->dma_info.dmaChunkSize);

    return pStreamEnv;
}

static void gaf_usb_bis_src_buf_init(void* _pStreamEnv)
{
    LOG_I("%s start", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    app_audio_mempool_init_with_specific_size(1024*30);

    uint32_t frame_len = pStreamEnv->stream_info.captureInfo.codec_info.frame_size *
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
    gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_BUF_INITIALIZED);

    LOG_I("%s end", __func__);
}

int gaf_usb_bis_src_audio_stream_start_handler(uint8_t grp_lid)
{
    LOG_D("%s", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_usb_bis_src_refresh_stream_env_from_grp_info(grp_lid);

    if (GAF_CAPTURE_STREAM_IDLE == pStreamEnv->stream_context.capture_stream_state)
    {
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZING);

        gaf_usb_bis_src_buf_init(pStreamEnv);
        if (pStreamEnv->func_list->encoder_func_list)
        {
            pStreamEnv->func_list->encoder_func_list->encoder_init_func(
                        0, &(pStreamEnv->stream_info.captureInfo.codec_info));
        }

#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
        gaf_capture_encoder_init(pStreamEnv);
#endif
        gaf_media_pid_init(&(pStreamEnv->stream_context.capture_pid_env));
        gaf_media_pid_update_threshold(&(pStreamEnv->stream_context.capture_pid_env),
                        (1000000/pStreamEnv->stream_info.captureInfo.dma_info.sample_rate) + 1);
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);

#ifdef USB_BLE_AUDIO_HW_TIMER_TRIGGER
        gaf_usb_bis_capture_hw_timer_trigger_start(pStreamEnv);
#else
        af_set_priority(AF_USER_AI, osPriorityHigh);
#endif

        return 0;
    }
    return -1;
}

int gaf_usb_bis_src_audio_stream_stop_handler(uint8_t grp_lid)
{
    LOG_D("%s", __func__);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_usb_bis_src_refresh_stream_env_from_grp_info(grp_lid);

    if (pStreamEnv)
    {
        bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.capture_retrigger_onprocess;
        if (pStreamEnv->func_list->encoder_func_list)
        {
            pStreamEnv->func_list->encoder_func_list->encoder_deinit_buf_func(0);
            pStreamEnv->func_list->encoder_func_list->encoder_deinit_func(0);
        }
#if defined(GAF_CODEC_CROSS_CORE) || defined(AOB_CODEC_CP)
        gaf_capture_encoder_deinit(pStreamEnv, isRetrigger);
#else
        af_set_priority(AF_USER_AI, osPriorityNormal);
#endif
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_IDLE);
    }
    return 0;
}

/*TBD:rewrite function to make sure that bap bc src eable would be called fater stop has been finished*/

void app_usb_stop_bis_src_stream(void)
{
    aob_bis_src_stop_streaming(0);
}

void app_usb_start_bis_src_stream_with_new_bid(void)
{
    uint32_t big_idx = 0;
    app_bap_bc_src_grp_info_t *p_grp_t = app_bap_bc_src_get_big_info_by_big_idx(big_idx);
    app_usb_start_bis_src_stream();
    LOG_D("%s with Bcast ID:", __func__);
    DUMP8("%02x ", p_grp_t->big_param.bcast_id.id, BAP_BC_BROADCAST_ID_LEN);
    LOG_D("and Bcast Code:");
    DUMP8("%02x ", p_grp_t->big_param.bcast_code.bcast_code, APP_GAP_KEY_LEN);
}

#endif /* BLE_USB_AUDIO_IS_DONGLE_ROLE */
#endif /* USB_BIS_AUDIO_STREAM */
#endif /* AOB_MOBILE_ENABLED */

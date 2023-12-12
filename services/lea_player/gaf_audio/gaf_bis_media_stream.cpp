/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if BLE_AUDIO_ENABLED
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "cqueue.h"
#include "hal_dma.h"
#include "hal_aud.h"
#include "hal_trace.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "app_audio.h"

/// TODO:include path
#include "rwble_config.h"

#ifndef BLE_STACK_NEW_DESIGN
#include "app_ble_tws_sync.h"
#else
#include "ble_tws.h"
#endif

#include "gaf_stream_dbg.h"
#include "gaf_media_pid.h"
#include "gaf_media_sync.h"
#include "gaf_bis_media_stream.h"
#include "gaf_codec_lc3.h"

#include "app_overlay.h"

#include "ble_audio_earphone_info.h"

#include "bes_aob_api.h"

#include "gaf_cc_api.h"

#ifdef DSP_HIFI4
#include "mcu_dsp_hifi4_app.h"
#endif

/************************private macro defination***************************/
//#define BAP_DUMP_AUDIO_DATA
//#define BAP_CALCULATE_CODEC_MIPS

#define BT_AUDIO_CACHE_2_UNCACHE(addr) \
    ((unsigned char *)((unsigned int)addr & ~(0x04000000)))

#define LC3_BPS                             AUD_BITS_16
#define LC3_FRAME_MS                        10

//audio
#define LC3_AUDIO_CHANNEL_NUM               AUD_CHANNEL_NUM_1

//local source audio play
#define BIS_AUDIO_CHANNEL_NUM               AUD_CHANNEL_NUM_2
#define BIS_SAMPLE_RATE                     AUD_SAMPRATE_48000

#define TIME_CALCULATE_REVERSAL_THRESHOLD   0x7F000000
#define BIS_AUDIO_PLAY_DLAY_US              2000   //trigger delay time
#define BIS_AUDIO_PLAY_INTERVAL_US          10000   //af DMA irq interval
#define LC3_ALGORITH_CODEC_DELAY_US         2500    //lc3 encode decode delay
#define BIS_DIFF_ANCHOR_US                  4000    //current time diff to bis send anchor


/************************private type defination****************************/

/************************extern function declearation***********************/

/**********************private function declearation************************/

/************************private variable defination************************/
static gaf_bis_src_param_t bis_src_parameter = {0};
uint32_t expected_play_time = 0;

GAF_AUDIO_STREAM_ENV_T gaf_bis_src_audio_stream_env;
GAF_AUDIO_STREAM_ENV_T gaf_bis_audio_stream_env;

const static GAF_MEDIA_STREAM_TYPE_OPERATION_RULE_T gaf_bis_sink_stream_types_op_rule =
{
    GAF_AUDIO_STREAM_TYPE_PLAYBACK,
    GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM,
    1,
    0,
};

const static GAF_MEDIA_STREAM_TYPE_OPERATION_RULE_T gaf_bis_src_stream_types_op_rule =
{
    GAF_AUDIO_STREAM_TYPE_PLAYBACK | GAF_AUDIO_STREAM_TYPE_CAPTURE,
    GAF_AUDIO_TRIGGER_BY_CAPTURE_STREAM,
    1,
    1,
};

/****************************function defination****************************/
static void gaf_bis_src_audio_media_buf_init(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
    gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_BUF_INITIALIZED);

    app_audio_mempool_get_buff(&(pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr),
        pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkSize*2);

    app_audio_mempool_get_buff(&(pStreamEnv->stream_context.capture_frame_cache),
        pStreamEnv->stream_info.captureInfo.codec_info.frame_size *
        pStreamEnv->stream_info.captureInfo.codec_info.num_channels + sizeof(CC_CAPTURE_DATA_T));

    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_init_buf_func(0,
            &(pStreamEnv->stream_info.captureInfo.codec_info), (void*)gaf_stream_common_buf_alloc);
    }
}

static int gaf_bis_src_audio_media_stream_start_handler(void* _pStreamEnv)
{
    uint32_t trigger_bt_time = 0,current_bt_time = 0,bis_anchor_time = 0, diff_time = 0;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    if (GAF_CAPTURE_STREAM_INITIALIZING == pStreamEnv->stream_context.capture_stream_state)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_AOB, APP_SYSFREQ_208M);
        af_set_priority(AF_USER_AI, osPriorityHigh);

        struct AF_STREAM_CONFIG_T stream_cfg;

        // capture stream
        memset((void *)&stream_cfg, 0, sizeof(struct AF_STREAM_CONFIG_T));
        stream_cfg.bits         = (enum AUD_BITS_T)(pStreamEnv->stream_info.captureInfo.dma_info.bits_depth);
        stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)(pStreamEnv->stream_info.captureInfo.dma_info.num_channels);
        stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)(AUD_CHANNEL_MAP_CH0|AUD_CHANNEL_MAP_CH1);

        stream_cfg.io_path      = AUD_INPUT_PATH_LINEIN;
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)pStreamEnv->stream_info.captureInfo.dma_info.sample_rate;

        // TODO: get vol from VCC via ase_lid
        stream_cfg.vol          = TGT_VOLUME_LEVEL_7;

        stream_cfg.data_size    = (uint32_t)(2 * pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkSize);

        pStreamEnv->func_list->stream_func_list.init_stream_buf_func(pStreamEnv);

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

        pStreamEnv->stream_context.captureTriggerChannel = 0;//app_bt_sync_get_avaliable_trigger_channel();

        gaf_media_prepare_capture_trigger(pStreamEnv->stream_context.captureTriggerChannel);
        gaf_media_pid_init(&(pStreamEnv->stream_context.capture_pid_env));
        gaf_media_pid_update_threshold(&(pStreamEnv->stream_context.capture_pid_env),
            pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs/2);

        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZED);

        if (GAF_AUDIO_TRIGGER_BY_CAPTURE_STREAM ==
            gaf_bis_src_stream_types_op_rule.trigger_stream_type)
        {
            current_bt_time = gaf_media_sync_get_curr_time();
            bis_anchor_time = btdrv_reg_op_big_anchor_timestamp(bis_src_parameter.conhdl&0xFF);
            LOG_I("current_bt_time %d bis_anchor_time %d big_sync_delay %d",current_bt_time, bis_anchor_time,bis_src_parameter.big_sync_delay);

            if(current_bt_time >= bis_anchor_time)
            {
                diff_time = bis_anchor_time + pStreamEnv->stream_info.captureInfo.isoIntervalUs - current_bt_time;
                if(diff_time >= BIS_DIFF_ANCHOR_US){
                    if(bis_anchor_time + bis_src_parameter.big_sync_delay > current_bt_time){
                        current_bt_time = bis_anchor_time + bis_src_parameter.big_sync_delay;
                    }
                }
                else{
                    current_bt_time = bis_anchor_time + pStreamEnv->stream_info.captureInfo.isoIntervalUs;
                    osDelay(diff_time/1000);
                }
            }
            else
            {
                diff_time = bis_anchor_time - current_bt_time;
                if(diff_time >= BIS_DIFF_ANCHOR_US){
                    if(bis_anchor_time + bis_src_parameter.big_sync_delay - pStreamEnv->stream_info.captureInfo.isoIntervalUs > current_bt_time){
                        current_bt_time = bis_anchor_time + bis_src_parameter.big_sync_delay - pStreamEnv->stream_info.captureInfo.isoIntervalUs;
                    }
                }
                else{
                    current_bt_time = bis_anchor_time;
                    osDelay(diff_time/1000);
                }
            }

            trigger_bt_time = current_bt_time + bis_src_parameter.big_trans_latency -
                pStreamEnv->stream_info.captureInfo.isoIntervalUs + LC3_ALGORITH_CODEC_DELAY_US;
            expected_play_time = trigger_bt_time + pStreamEnv->stream_info.captureInfo.isoIntervalUs;
            gaf_stream_common_set_capture_trigger_time(pStreamEnv, trigger_bt_time);
        }

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        return 0;
    }

    return -1;
}

static int gaf_bis_src_audio_media_stream_stop_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    if (GAF_CAPTURE_STREAM_IDLE != pStreamEnv->stream_context.capture_stream_state)
    {
        uint8_t adma_ch = HAL_DMA_CHAN_NONE;
        uint32_t dma_base;
        bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.capture_retrigger_onprocess;
        // source
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
        pStreamEnv->func_list->stream_func_list.deinit_stream_buf_func(pStreamEnv);

        return 0;
    }

    return -1;
}

static void gaf_bis_src_capture_dma_irq_handler_send(void* pStreamEnv_,void *payload,
    uint32_t payload_size, uint32_t ref_time)
{
    uint8_t *output_buf[2];
    output_buf[0] = (uint8_t*)payload;
    output_buf[1] = NULL;
    bes_ble_bis_src_send_iso_data_to_all_channel(output_buf, payload_size, ref_time);
}

static uint32_t gaf_bis_src_capture_dma_irq_handler(uint8_t* ptrBuf, uint32_t length)
{
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
     uint32_t dma_base;
    uint8_t *output_buf = NULL;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_src_audio_stream_env;
    POSSIBLY_UNUSED CODEC_CORE_INFO_T *coreInfo =
        &pStreamEnv->stream_info.captureInfo.bisChInfo[0].codec_core_info;

    uint32_t frame_len = (uint32_t)(pStreamEnv->stream_info.captureInfo.codec_info.frame_size *
        pStreamEnv->stream_info.captureInfo.codec_info.num_channels);

    if ((GAF_CAPTURE_STREAM_START_TRIGGERING > pStreamEnv->stream_context.capture_stream_state)) {
        memset(ptrBuf, 0x00, length);
        return length;
    } else if (GAF_CAPTURE_STREAM_START_TRIGGERING == pStreamEnv->stream_context.capture_stream_state) {
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_STREAMING_TRIGGERED);
        gaf_stream_common_clr_trigger(pStreamEnv->stream_context.captureTriggerChannel);
    }

    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
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

    //gaf_bis_src_store_pcm_buffer(ptrBuf, length);
    gaf_stream_common_capture_timestamp_checker(pStreamEnv, dmaIrqHappeningTimeUs);

    dmaIrqHappeningTimeUs += (uint32_t)pStreamEnv->stream_info.captureInfo.dma_info.dmaChunkIntervalUs;
    LOG_D("length %d encoded_len %d filled timestamp %d", length,
        pStreamEnv->stream_info.captureInfo.codec_info.frame_size,
        dmaIrqHappeningTimeUs);
    output_buf = pStreamEnv->stream_context.capture_frame_cache;

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    if (pStreamEnv->stream_context.isUpStreamingStarted)
    {
        // bth fetch encoded data
        gaf_stream_common_fetch_frame(pStreamEnv, output_buf, frame_len, dmaIrqHappeningTimeUs, coreInfo->instance_handle);
        // bth send out encoded data
        gaf_bis_src_capture_dma_irq_handler_send(pStreamEnv, output_buf, frame_len, dmaIrqHappeningTimeUs);
    }
    gaf_stream_common_store_pcm(pStreamEnv, dmaIrqHappeningTimeUs, ptrBuf, length, frame_len, coreInfo);
#else
    pStreamEnv->func_list->encoder_func_list->encoder_encode_frame_func(0,
        &(pStreamEnv->stream_info.captureInfo.codec_info), length, ptrBuf, frame_len, output_buf);

    gaf_bis_src_capture_dma_irq_handler_send(pStreamEnv, output_buf, frame_len, dmaIrqHappeningTimeUs);
#endif
    return 0;
}

static void gaf_bis_src_audio_media_buf_deinit(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    pStreamEnv->stream_info.captureInfo.dma_info.dmaBufPtr = NULL;

    if (pStreamEnv->func_list->encoder_func_list)
    {
        pStreamEnv->func_list->encoder_func_list->encoder_deinit_buf_func(0);
    }
}

static GAF_AUDIO_FUNC_LIST_T gaf_bis_src_audio_media_stream_func_list =
{
    {
        .start_stream_func = gaf_bis_src_audio_media_stream_start_handler,
        .init_stream_buf_func = gaf_bis_src_audio_media_buf_init,
        .stop_stream_func = gaf_bis_src_audio_media_stream_stop_handler,
        .capture_dma_irq_handler_func = gaf_bis_src_capture_dma_irq_handler,
        .deinit_stream_buf_func = gaf_bis_src_audio_media_buf_deinit,
    },
};

static GAF_AUDIO_STREAM_ENV_T *gaf_bis_src_audio_stream_update_stream_env_from_grp_info(uint8_t grp_lid)
{
    uint16_t bis_hdl = bes_ble_bis_src_get_bis_hdl_by_big_idx(grp_lid);

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_src_audio_stream_env;
    GAF_AUDIO_STREAM_COMMON_INFO_T* pCommonInfo;
    pCommonInfo = &(pStreamEnv->stream_info.captureInfo);

    pCommonInfo->codec_info.bits_depth = LC3_BPS;

    const AOB_BAP_CFG_T* p_lc3_cfg = bes_ble_bis_src_get_codec_cfg_by_big_idx(grp_lid);

    pCommonInfo->bisChInfo[0].iso_channel_hdl = BLE_BISHDL_TO_ACTID(bis_hdl);

    pCommonInfo->codec_info.num_channels = bes_ble_audio_get_location_fs_l_r_cnt(p_lc3_cfg->param.location_bf);

    const AOB_CODEC_ID_T *p_codec_id = bes_ble_bis_src_get_codec_id_by_big_idx(grp_lid, 0);

    pCommonInfo->isoIntervalUs = bes_ble_bis_src_get_iso_interval_ms_by_big_idx(grp_lid);

    pStreamEnv->func_list->decoder_func_list = NULL;
    pStreamEnv->func_list->encoder_func_list = NULL;

    switch (p_codec_id->codec_id[0])
    {
        case BES_BLE_GAF_CODEC_TYPE_LC3:
        {
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
    pCommonInfo->dma_info.dmaChunkIntervalUs = (uint32_t)(pCommonInfo->dma_info.frame_ms * 1000);
    pCommonInfo->dma_info.dmaChunkSize =
                (uint32_t)((pCommonInfo->dma_info.sample_rate * sample_bytes *
                (pCommonInfo->dma_info.dmaChunkIntervalUs/1000)*
                pCommonInfo->dma_info.num_channels)/1000);

    LOG_I("isoInterval:%d us", pCommonInfo->isoIntervalUs);
    LOG_I("frame_len:%d us, sample_rate:%d dmaInterval:%d us dmaChunkSize:%d",
        (uint32_t)(pCommonInfo->dma_info.frame_ms*1000), pCommonInfo->dma_info.sample_rate,
        pCommonInfo->dma_info.dmaChunkIntervalUs, pCommonInfo->dma_info.dmaChunkSize);

    return pStreamEnv;
}

static void _gaf_bis_src_audio_stream_start_handler(uint32_t grp_param)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = NULL;

    memcpy(&bis_src_parameter, (void *)grp_param, sizeof(gaf_bis_src_param_t));
    pStreamEnv= gaf_bis_src_audio_stream_update_stream_env_from_grp_info(bis_src_parameter.grp_lid);
    if (pStreamEnv)
    {
        bt_adapter_write_sleep_enable(0);
        gaf_stream_common_update_capture_stream_state(pStreamEnv, GAF_CAPTURE_STREAM_INITIALIZING);
        pStreamEnv->func_list->stream_func_list.start_stream_func(pStreamEnv);
    }
}

void gaf_bis_src_audio_stream_start_handler(gaf_bis_src_param_t *src_para)
{
    LOG_I("%s", __func__);
    app_bt_start_custom_function_in_bt_thread((uint32_t)src_para,
                                              0,
                                              (uint32_t)_gaf_bis_src_audio_stream_start_handler);
}

static void _gaf_bis_src_audio_stream_stop_handler(uint8_t grp_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = gaf_bis_src_audio_stream_update_stream_env_from_grp_info(grp_lid);

    if (pStreamEnv)
    {
        pStreamEnv->func_list->stream_func_list.stop_stream_func(pStreamEnv);
        bt_adapter_write_sleep_enable(1);
    }
}

void gaf_bis_src_audio_stream_stop_handler(uint8_t grp_lid)
{
    LOG_I("%s", __func__);
    app_bt_start_custom_function_in_bt_thread((uint32_t)grp_lid,
                                              0,
                                              (uint32_t)_gaf_bis_src_audio_stream_stop_handler);
}

void gaf_bis_src_audio_stream_init(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_src_audio_stream_env;
    memset((uint8_t *)pStreamEnv, 0, sizeof(GAF_AUDIO_STREAM_ENV_T));
    pStreamEnv->stream_context.capture_stream_state = GAF_CAPTURE_STREAM_IDLE;
    pStreamEnv->stream_info.captureInfo.bisChInfo[0].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
    pStreamEnv->stream_info.captureInfo.presDelayUs = GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US;
    pStreamEnv->stream_info.captureInfo.bisChInfo[0].codec_core_info.instance_status = INSTANCE_IDLE;
    pStreamEnv->stream_info.is_bis = true;

    GAF_AUDIO_STREAM_INFO_T* pStreamInfo = NULL;
    pStreamInfo = &(gaf_bis_src_audio_stream_env.stream_info);
    pStreamInfo->contextType = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
    gaf_stream_common_register_func_list(&gaf_bis_src_audio_stream_env,
        &gaf_bis_src_audio_media_stream_func_list);
}
#endif

static void gaf_bis_stream_pkt_data_parse_store(GAF_AUDIO_STREAM_ENV_T *pStreamEnv,
                            gaf_media_data_t *p_sdu_buf, CODEC_CORE_INFO_T *coreInfo,
                            GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T *channel_info)
{
    CC_PLAYBACK_DATA_T frame = {0};

    if (channel_info->sdu_num < 2)
    {
        frame.time_stamp = p_sdu_buf->time_stamp;
        frame.seq_nb = p_sdu_buf->pkt_seq_nb;
        frame.data_len = p_sdu_buf->data_len;
        frame.data = p_sdu_buf->sdu_data;
        frame.isPLC = ((p_sdu_buf->pkt_status != GAF_ISO_PKT_STATUS_VALID) || (0 == p_sdu_buf->data_len));
        gaf_stream_common_store_packet(pStreamEnv, &frame, coreInfo);
    }
    else
    {
        for (int i=0; i<channel_info->sdu_num; ++i)
        {
            frame.time_stamp = (uint32_t)(channel_info->frame_ms *1000) * i + p_sdu_buf->time_stamp;
            frame.seq_nb = p_sdu_buf->pkt_seq_nb * channel_info->sdu_num +i;
            frame.data_len = p_sdu_buf->data_len/channel_info->sdu_num;
            frame.data = p_sdu_buf->sdu_data+frame.data_len*i;
            frame.isPLC = ((p_sdu_buf->pkt_status != GAF_ISO_PKT_STATUS_VALID) || (0 == p_sdu_buf->data_len));
            gaf_stream_common_store_packet(pStreamEnv, &frame, coreInfo);
        }
    }
    if (frame.isPLC) {
        LOG_I("PLC seq:%u ts:%u pk status:%d data len:%d",
            p_sdu_buf->pkt_seq_nb, p_sdu_buf->time_stamp, p_sdu_buf->pkt_status, p_sdu_buf->data_len);
    }
}

static void gaf_bis_stream_receive_data(uint16_t conhdl, GAF_ISO_PKT_STATUS_E pkt_status)
{
    // map to gaf stream context
    // TODO: get the correct stream context based on active stream type.
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_audio_stream_env;
    uint8_t channel = BLE_ISOHDL_TO_ACTID(conhdl);
    CODEC_CORE_INFO_T *coreInfo =
    (CODEC_CORE_INFO_T*)gaf_bis_stream_get_codec_core_info(pStreamEnv, BES_BLE_GAF_DIRECTION_SINK, channel);
    uint32_t current_bt_time = 0;
    uint32_t trigger_bt_time = 0;
    gaf_media_data_t *p_sdu_buf = NULL;
    bool isDecoderReady = true;
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    isDecoderReady = (coreInfo->instance_status >= INSTANCE_INITIALIZED);
#else
    coreInfo->instance_handle = 0;
#endif

    if (!isDecoderReady || !coreInfo)
    {
        while ((p_sdu_buf = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
        {
            gaf_stream_data_free(p_sdu_buf);
        }
        return;
    }

    while ((p_sdu_buf = (gaf_media_data_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)) && isDecoderReady)
    {
        ASSERT(p_sdu_buf->data_len <= pStreamEnv->stream_info.playbackInfo.maxFrameSize,
            "%s len %d %d, channel:%d, playbackInfo:%p", __func__, p_sdu_buf->data_len,
            pStreamEnv->stream_info.playbackInfo.maxFrameSize,
            channel, &(pStreamEnv->stream_info.playbackInfo));

        if ((GAF_PLAYBACK_STREAM_INITIALIZED == pStreamEnv->stream_context.playback_stream_state) &&
            (GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM == gaf_bis_sink_stream_types_op_rule.trigger_stream_type) &&
            (GAF_ISO_PKT_STATUS_VALID == p_sdu_buf->pkt_status) && (p_sdu_buf->data_len > 0))
        {
            for (int i=0; i <2; ++i)
            {
                if(conhdl == pStreamEnv->stream_info.playbackInfo.bisChInfo[i].iso_channel_hdl)
                {
                    pStreamEnv->stream_info.playbackInfo.trigger_stream_lid = i;
                }
            }
            current_bt_time = gaf_media_sync_get_curr_time();

            trigger_bt_time = p_sdu_buf->time_stamp + pStreamEnv->stream_info.playbackInfo.presDelayUs -
                pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;
            while ((int32_t)(trigger_bt_time - current_bt_time) < GAF_MARGIN_BETWEEN_TRIGGER_TIME_AND_CURRENT_TIME_US)
            {
                trigger_bt_time += pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;
            }
            LOG_I("current_bt_time %u calculated trigger ticks %u ",current_bt_time, trigger_bt_time);
            if (current_bt_time < trigger_bt_time)
            {
                LOG_I("Starting playback seq num 0x%x", p_sdu_buf->pkt_seq_nb);
                pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX] = p_sdu_buf->pkt_seq_nb;
                gaf_stream_common_set_playback_trigger_time(pStreamEnv, trigger_bt_time);

                gaf_stream_set_prefill_status(true);
            }
            else
            {
                LOG_I("time_stamp error");
            }
        }

        if (pStreamEnv->stream_context.playback_stream_state >= GAF_PLAYBACK_STREAM_START_TRIGGERING)
        {
            uint16_t more_data;
            for (int i=0; i<2; ++i)
            {
                if (pStreamEnv->stream_info.playbackInfo.bisChInfo[i].iso_channel_hdl == BLE_ISOHDL_TO_ACTID(conhdl))
                {
                    //Extra data is considered private data
                    more_data = p_sdu_buf->data_len % pStreamEnv->stream_info.playbackInfo.bisChInfo[i].blocks_size;
                    if (more_data)
                    {
                        bes_ble_bis_sink_private_data_callback(p_sdu_buf->sdu_data, p_sdu_buf->data_len);
                        p_sdu_buf->data_len -= more_data;
                    }

                    gaf_bis_stream_pkt_data_parse_store(pStreamEnv, p_sdu_buf, coreInfo, &pStreamEnv->stream_info.playbackInfo.bisChInfo[i]);
                    break;;
                }
            }
        }
        gaf_stream_data_free(p_sdu_buf);
    }
}

static int gaf_bis_audio_media_stream_start_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    if (GAF_PLAYBACK_STREAM_INITIALIZING == pStreamEnv->stream_context.playback_stream_state)
    {
        // TODO: shall use reasonable cpu frequency
        app_sysfreq_req(APP_SYSFREQ_USER_AOB, APP_SYSFREQ_104M);
        af_set_priority(AF_USER_AI, osPriorityHigh);

        struct AF_STREAM_CONFIG_T stream_cfg;

        memset((void *)&stream_cfg, 0, sizeof(struct AF_STREAM_CONFIG_T));
        stream_cfg.bits         = (enum AUD_BITS_T)(pStreamEnv->stream_info.playbackInfo.dma_info.bits_depth);
        stream_cfg.channel_num  = (enum AUD_CHANNEL_NUM_T)(pStreamEnv->stream_info.playbackInfo.dma_info.num_channels);
        if (stream_cfg.channel_num > 1)
        {
            stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)( AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);
        }

        stream_cfg.io_path      = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.sample_rate  = (enum AUD_SAMPRATE_T)pStreamEnv->stream_info.playbackInfo.dma_info.sample_rate;

        // TODO: get vol from VCC
        stream_cfg.vol          = TGT_VOLUME_LEVEL_7;

        stream_cfg.data_size    = (uint32_t)(2 * pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize);

        pStreamEnv->func_list->stream_func_list.init_stream_buf_func(pStreamEnv);

        if (pStreamEnv->func_list->decoder_func_list)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_init_func(
                0, &pStreamEnv->stream_info.playbackInfo.codec_info);
        }

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr);

        stream_cfg.handler = pStreamEnv->func_list->stream_func_list.playback_dma_irq_handler_func;

        af_codec_tune(AUD_STREAM_PLAYBACK, 0);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        pStreamEnv->stream_context.playbackTriggerChannel = app_ble_tws_sync_get_avaliable_trigger_channel();
        gaf_media_prepare_playback_trigger(pStreamEnv->stream_context.playbackTriggerChannel);

        // put PID env into stream context
        gaf_media_pid_init(&(pStreamEnv->stream_context.playback_pid_env));

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZED);

        bes_ble_bap_dp_itf_data_come_callback_register((void *)gaf_bis_stream_receive_data);

        return 0;
    }

    return -1;
}

static void gaf_bis_audio_media_playback_buf_init(void* _pStreamEnv)
{
    uint8_t* heapBufStartAddr = NULL;
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    if (pStreamEnv->stream_context.playback_stream_state <= GAF_PLAYBACK_STREAM_INITIALIZING)
    {
        app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_BUF_INITIALIZED);
    }
    uint32_t audioCacheHeapSize = pStreamEnv->stream_info.playbackInfo.maxCachedFrameCount *
                pStreamEnv->stream_info.playbackInfo.maxFrameSize;
    app_audio_mempool_get_buff(&(pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr),
        pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize * 2);
    app_audio_mempool_get_buff(&(pStreamEnv->stream_context.playback_pcm_cache),
        pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkSize);
    app_audio_mempool_get_buff(&heapBufStartAddr, audioCacheHeapSize);
    gaf_stream_heap_init(heapBufStartAddr, audioCacheHeapSize);

#if !defined (GAF_CODEC_CROSS_CORE) && !defined (AOB_CODEC_CP)
    AOB_BIS_GROUP_INFO_T *aob_bis_group_info = ble_audio_earphone_info_get_bis_group_info();
    uint32_t frame_size = pStreamEnv->stream_info.playbackInfo.codec_info.frame_size *
        pStreamEnv->stream_info.playbackInfo.codec_info.num_channels + sizeof(CC_PLAYBACK_DATA_T);
    uint32_t frame_buf_size = pStreamEnv->stream_info.playbackInfo.maxCachedFrameCount * frame_size;
    app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_frame_cache), frame_size);
    app_audio_mempool_get_buff(
            (uint8_t**)&(pStreamEnv->stream_context.playback_frame_fifo[0]), sizeof(cfifo));
    app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_frame_buf[0]), frame_buf_size);
    cfifo_init(pStreamEnv->stream_context.playback_frame_fifo[0],
        pStreamEnv->stream_context.playback_frame_buf[0], frame_buf_size);

    if (aob_bis_group_info->play_stream_num == 2) {
        app_audio_mempool_get_buff(
            (uint8_t**)&(pStreamEnv->stream_context.playback_frame_fifo[1]), sizeof(cfifo));
        app_audio_mempool_get_buff(
            &(pStreamEnv->stream_context.playback_frame_buf[1]), frame_buf_size);
        cfifo_init(pStreamEnv->stream_context.playback_frame_fifo[1],
            pStreamEnv->stream_context.playback_frame_buf[1], frame_buf_size);
    }
#endif

    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_init_buf_func(0,
            &(pStreamEnv->stream_info.playbackInfo.codec_info), (void*)gaf_stream_common_buf_alloc);
    }
}

static int gaf_bis_audio_media_stream_stop_handler(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;

    if (GAF_PLAYBACK_STREAM_IDLE != pStreamEnv->stream_context.playback_stream_state)
    {
        uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
        uint32_t dma_base;
        bool POSSIBLY_UNUSED isRetrigger = pStreamEnv->stream_context.playback_retrigger_onprocess;
        bes_ble_bap_dp_itf_data_come_callback_deregister();
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_IDLE);
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

        if (pStreamEnv->func_list->decoder_func_list)
        {
            pStreamEnv->func_list->decoder_func_list->decoder_deinit_func(0);
        }
#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
        gaf_playback_decoder_deinit_all_ase(pStreamEnv, isRetrigger);
#endif
        pStreamEnv->func_list->stream_func_list.deinit_stream_buf_func(pStreamEnv);

        return 0;
    }

    return -1;

}

static uint32_t gaf_bis_stream_media_dma_irq_handler(uint8_t *buf, uint32_t len)
{
    uint32_t btclk; //hal slot -- 312.5us
    uint16_t btcnt; //hal  microsecond -- 0.5 us
    uint32_t dmaIrqHappeningTimeUs = 0;
    uint8_t adma_ch = HAL_DMA_CHAN_NONE;
    uint32_t dma_base;
    int ret = 0;
    uint8_t instance_handle = 0;
    uint32_t audio_play_delay = ble_audio_earphone_info_get_sink_audio_paly_delay();
    AOB_BIS_GROUP_INFO_T *aob_bis_group_info = ble_audio_earphone_info_get_bis_group_info();
    adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    dma_base = af_stream_get_dma_base_addr(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    if (adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch&0xFF, dma_base);
        dmaIrqHappeningTimeUs = bt_syn_ble_bt_time_to_bts(btclk, btcnt);
    }

    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_audio_stream_env;

    // it's possible the multiple DMA irq triggered message accumulates,
    // so the acuiqred dmaIrqHappeningTimeUs is the last dma irq, which has
    // been handled. For this case, ignore this dma irq message handling
    if (!gaf_stream_get_prefill_status())
    {
        if ((GAF_PLAYBACK_STREAM_START_TRIGGERING > pStreamEnv->stream_context.playback_stream_state) ||
            ((GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED == pStreamEnv->stream_context.playback_stream_state) &&
            (dmaIrqHappeningTimeUs == pStreamEnv->stream_context.lastPlaybackDmaIrqTimeUs)))
        {
            memset(buf, 0, len);
            return len;
        }

        gaf_bis_stream_common_update_multi_channel_expect_seq_and_time(pStreamEnv, dmaIrqHappeningTimeUs);

        if (GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED !=
            pStreamEnv->stream_context.playback_stream_state)
        {
            gaf_stream_common_update_playback_stream_state(pStreamEnv,
                GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED);
            gaf_stream_common_clr_trigger(pStreamEnv->stream_context.playbackTriggerChannel);
            pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]--;
            LOG_D("Trigger ticks %d Update playback seq to %d",
                dmaIrqHappeningTimeUs,
                pStreamEnv->stream_context.lastestPlaybackSeqNum[GAF_AUDIO_DFT_PLAYBACK_LIST_IDX]);
        }
    }

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    bool isSuccessful = 0;
    if (aob_bis_group_info->play_stream_num < 2) {
        isSuccessful = gaf_stream_common_fetch_pcm(pStreamEnv,
            buf, len, instance_handle, dmaIrqHappeningTimeUs, audio_play_delay, true);
    } else {
        isSuccessful = gaf_bis_playback_fetch_multi_pcm_data(pStreamEnv, buf, len,
             audio_play_delay, dmaIrqHappeningTimeUs);
    }
    if (!isSuccessful)
    {
        memset(buf, 0, len);
        ret = CODEC_ERR;
    }
#else
#ifdef ADVANCE_FILL_ENABLED
    // Broadcast sink prefill is not ready yet
    dmaIrqHappeningTimeUs -= pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs;
#endif
    CC_PLAYBACK_DATA_T *frame = (CC_PLAYBACK_DATA_T*)pStreamEnv->stream_context.playback_frame_cache;
    frame->data = pStreamEnv->stream_context.playback_frame_cache + sizeof(CC_PLAYBACK_DATA_T);

    if (aob_bis_group_info->play_stream_num < 2) {
        gaf_stream_common_get_packet(pStreamEnv, frame, instance_handle, dmaIrqHappeningTimeUs, audio_play_delay, true);
    } else {
        gaf_bis_stream_common_get_combined_packet_from_multi_channels(pStreamEnv, frame, dmaIrqHappeningTimeUs,audio_play_delay);
    }

    ret = pStreamEnv->func_list->decoder_func_list->decoder_decode_frame_func(
            0, &(pStreamEnv->stream_info.playbackInfo.codec_info), frame->data_len,
            frame->data, buf, frame->isPLC);
    LOG_I("seq 0x%02x expected play time %u local time %u dec ret %d", frame->seq_nb,
                frame->time_stamp, dmaIrqHappeningTimeUs, ret);
#endif
    if (ret) {
        memset(buf, 0, len);
    }

    return 0;
}

static void gaf_bis_audio_media_playback_buf_deinit(void* _pStreamEnv)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = (GAF_AUDIO_STREAM_ENV_T *)_pStreamEnv;
    pStreamEnv->stream_info.playbackInfo.dma_info.dmaBufPtr = NULL;

    if (pStreamEnv->func_list->decoder_func_list)
    {
        pStreamEnv->func_list->decoder_func_list->decoder_deinit_buf_func(0);
    }
}

static GAF_AUDIO_FUNC_LIST_T gaf_audio_media_stream_func_list =
{
    {
        .start_stream_func = gaf_bis_audio_media_stream_start_handler,
        .init_stream_buf_func = gaf_bis_audio_media_playback_buf_init,
        .stop_stream_func = gaf_bis_audio_media_stream_stop_handler,
        .playback_dma_irq_handler_func = gaf_bis_stream_media_dma_irq_handler,
        .deinit_stream_buf_func = gaf_bis_audio_media_playback_buf_deinit,
    },
};

static GAF_AUDIO_STREAM_ENV_T *gaf_bis_audio_stream_update_stream_env_from_grp_info(uint8_t stream_lid)
{
    uint32_t sinktransdelay = 0;
    uint32_t audio_play_delay = 0;
    AOB_BIS_GROUP_INFO_T *aob_bis_group_info = ble_audio_earphone_info_get_bis_group_info();
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_audio_stream_env;
    GAF_AUDIO_STREAM_COMMON_INFO_T* pCommonInfo = &(pStreamEnv->stream_info.playbackInfo);
    AOB_BAP_CFG_T* p_lc3_cfg = (AOB_BAP_CFG_T*)&aob_bis_group_info->play_stream_info[0].stream_info.cfg_param;

    if (NULL == aob_bis_group_info)
    {
        ASSERT(0, "%s bc sink can not find earbud info", __func__);
    }

    if(pStreamEnv->stream_context.playback_stream_state == GAF_PLAYBACK_STREAM_IDLE)
    {
        pCommonInfo->bisChInfo[0].iso_channel_hdl = BLE_BISHDL_TO_ACTID(aob_bis_group_info->play_stream_info[0].bis_hdl);
        pCommonInfo->bisChInfo[0].select_ch_map = aob_bis_group_info->play_stream_info[0].select_ch_bf;
        pCommonInfo->bisChInfo[0].blocks_size = aob_bis_group_info->play_stream_info[0].blocks_size;
        pCommonInfo->bisChInfo[0].encoded_frame_size = p_lc3_cfg->param.frame_octet;
        pCommonInfo->bisChInfo[0].sdu_num    = p_lc3_cfg->param.frames_sdu;
        pCommonInfo->bisChInfo[0].frame_ms = gaf_stream_common_frame_duration_parse(p_lc3_cfg->param.frame_dur);

        if (aob_bis_group_info->play_stream_num == 2)
        {
            pCommonInfo->bisChInfo[1].iso_channel_hdl = BLE_BISHDL_TO_ACTID(aob_bis_group_info->play_stream_info[1].bis_hdl);
            pCommonInfo->bisChInfo[1].select_ch_map  = aob_bis_group_info->play_stream_info[1].select_ch_bf;
            pCommonInfo->bisChInfo[1].blocks_size = aob_bis_group_info->play_stream_info[1].blocks_size;
            pCommonInfo->bisChInfo[1].encoded_frame_size = p_lc3_cfg->param.frame_octet;
            pCommonInfo->bisChInfo[1].sdu_num   = p_lc3_cfg->param.frames_sdu;
            pCommonInfo->bisChInfo[1].frame_ms  = gaf_stream_common_frame_duration_parse(p_lc3_cfg->param.frame_dur);
        }
        else
        {
            pCommonInfo->bisChInfo[1].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
            pCommonInfo->bisChInfo[1].select_ch_map = 0;
            pCommonInfo->bisChInfo[1].blocks_size = 0;
            pCommonInfo->bisChInfo[1].encoded_frame_size = 0;
            pCommonInfo->bisChInfo[1].sdu_num    = 0;
            pCommonInfo->bisChInfo[1].frame_ms   = 0;
        }
        LOG_I("[%s][%d]: %d, %d, %d", __func__, __LINE__, aob_bis_group_info->play_stream_num,
            pCommonInfo->bisChInfo[0].select_ch_map, pCommonInfo->bisChInfo[1].select_ch_map);

        pCommonInfo->codec_info.num_channels =
            bes_ble_audio_get_location_fs_l_r_cnt(aob_bis_group_info->play_stream_info[stream_lid].select_ch_bf);
        pCommonInfo->codec_info.bits_depth = LC3_BPS;

        pStreamEnv->func_list->decoder_func_list = NULL;
        pStreamEnv->func_list->encoder_func_list = NULL;

        switch (aob_bis_group_info->play_stream_info[0].stream_info.codec_id.codec_id[0])
        {
            case BES_BLE_GAF_CODEC_TYPE_LC3:
            {
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
        pCommonInfo->dma_info.num_channels = aob_bis_group_info->play_ch_num;
        pCommonInfo->dma_info.frame_ms = pCommonInfo->codec_info.frame_ms;
        pCommonInfo->dma_info.sample_rate = pCommonInfo->codec_info.sample_rate;
        pCommonInfo->dma_info.dmaChunkIntervalUs = (uint32_t)(pCommonInfo->dma_info.frame_ms * 1000);
        pCommonInfo->dma_info.dmaChunkSize =
                    (uint32_t)((pCommonInfo->dma_info.sample_rate* sample_bytes *
                    (pCommonInfo->dma_info.dmaChunkIntervalUs/1000)*
                    pCommonInfo->dma_info.num_channels)/1000);

        sinktransdelay = ble_audio_earphone_info_get_sink_sync_ref_offset();
        ble_audio_earphone_info_update_sink_audio_paly_delay(BIS_AUDIO_PLAY_DLAY_US + pStreamEnv->stream_info.playbackInfo.dma_info.dmaChunkIntervalUs);
        audio_play_delay = ble_audio_earphone_info_get_sink_audio_paly_delay();
        pStreamEnv->stream_info.playbackInfo.presDelayUs = audio_play_delay + sinktransdelay;

        LOG_I("predelay: %d", pStreamEnv->stream_info.playbackInfo.presDelayUs);
        LOG_I("frame_len:%d us, sample_rate:%d dmaInterval:%d us dmaChunkSize:%d",
            (uint32_t)(pCommonInfo->dma_info.frame_ms*1000), pCommonInfo->dma_info.sample_rate,
            pCommonInfo->dma_info.dmaChunkIntervalUs, pCommonInfo->dma_info.dmaChunkSize);
    }

    return pStreamEnv;
}

void gaf_bis_audio_stream_stop_handler(uint8_t stream_lid)
{
    LOG_I("%s", __func__);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_audio_stream_env;

    if (pStreamEnv)
    {
        pStreamEnv->func_list->stream_func_list.stop_stream_func(pStreamEnv);
        bt_adapter_write_sleep_enable(1);
    }
#ifdef DSP_SMF
    app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_GAF_DSP);
#endif
}

void gaf_bis_audio_stream_start_handler(uint8_t stream_lid)
{
        // TODO: triggered by bt media manager
    LOG_I("%s, stream_lid=%d", __func__, stream_lid);
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_audio_stream_env;

    if(pStreamEnv->stream_context.playback_stream_state != GAF_PLAYBACK_STREAM_IDLE)
    {
        LOG_I("%s, stream started", __func__);
        return;
    }
    gaf_media_stream_boost_freq(1500);

#ifdef DSP_SMF
    app_dsp_hifi4_init(APP_DSP_HIFI4_USER_GAF_DSP);
    int32_t wait_cnt = 0;
    while(app_dsp_hifi4_is_running() == false) {
        hal_sys_timer_delay_us(10);
        if (wait_cnt++ > 300000) {    // 3s
            ASSERT(0, "%s: hifi4 is hung", __func__);
        }
    }
#endif
    LOG_I("%s stream_lid:%d", __func__, stream_lid);
    gaf_bis_audio_stream_update_stream_env_from_grp_info(stream_lid);
    POSSIBLY_UNUSED AOB_BIS_GROUP_INFO_T *aob_bis_group_info = ble_audio_earphone_info_get_bis_group_info();

#if defined (GAF_CODEC_CROSS_CORE) || defined (AOB_CODEC_CP)
    for (uint32_t index = 0; index < aob_bis_group_info->play_stream_num; index++)
    {
        gaf_playback_decoder_init(pStreamEnv, index);
    }
#endif

    if (pStreamEnv && (GAF_PLAYBACK_STREAM_IDLE == pStreamEnv->stream_context.playback_stream_state))
    {
        gaf_stream_common_update_playback_stream_state(pStreamEnv, GAF_PLAYBACK_STREAM_INITIALIZING);
        bt_adapter_write_sleep_enable(0);
        pStreamEnv->func_list->stream_func_list.start_stream_func(pStreamEnv);
    }
}

void gaf_bis_stream_dump_dma_trigger_status()
{
    uint8_t context_id = 0;
    uint8_t len = 0;
    char state_str[100];
    GAF_PLAYBACK_STREAM_STATE_E trigger_status = gaf_bis_audio_stream_env.stream_context.playback_stream_state;
    len += sprintf(state_str + len, "context %d state %d ", context_id, trigger_status);
    LOG_I("gaf tri: %s", state_str);
}

void gaf_bis_audio_stream_init(void)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gaf_bis_audio_stream_env;
    memset((uint8_t *)pStreamEnv, 0, sizeof(GAF_AUDIO_STREAM_ENV_T));
    pStreamEnv->stream_context.playback_stream_state = GAF_PLAYBACK_STREAM_IDLE;
    pStreamEnv->stream_info.playbackInfo.presDelayUs = GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US;
    pStreamEnv->stream_info.is_bis = true;

    GAF_AUDIO_STREAM_INFO_T* pStreamInfo = NULL;

    for (uint8_t i = 0; i < 2; i++)
    {
        pStreamEnv->stream_info.playbackInfo.bisChInfo[i].iso_channel_hdl = GAF_AUDIO_INVALID_ISO_CHANNEL;
        pStreamEnv->stream_info.playbackInfo.bisChInfo[i].codec_core_info.instance_status = INSTANCE_IDLE;
    }

    pStreamInfo = &(gaf_bis_audio_stream_env.stream_info);
    pStreamInfo->contextType = GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE;
    gaf_stream_common_register_func_list(&gaf_bis_audio_stream_env,
         &gaf_audio_media_stream_func_list);
    gaf_stream_common_register_cc_func_list();
    ble_audio_earphone_info_update_sink_audio_paly_delay(BIS_AUDIO_PLAY_DLAY_US);
}

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
#include "cmsis.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "a2dp_decoder_cc_common.h"
#include "ssc.h"
#include "app_dsp_m55.h"
/************************private macro defination***************************/
#define CC_SBM_FAST_SPEED                   (67380) // 840 2.76%
#define CC_SBM_SLOW_SPEED                   (63730) // 888 2.76%
#define CC_SBM_NUM_NORMAL                   (100)
#define CC_SCALABLE_MEMPOOL_SIZE            (30 * 1024)
#define CC_SCALABLE_MAX_FRAME_SIZE          (900)
#define CC_SCALABLE_FRAME_SIZE              (864)
#define CC_SCALABLE_HEAD_SIZE               (4)     /* pick something big enough to hold a bunch of frames */
#define CC_SCALABLE_DECODER_SIZE            (17456) /* pick something big enough to hold a bunch of frames */
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
#define CC_SCALABLE_READBUF_SIZE            (800)
#else
#define CC_SCALABLE_READBUF_SIZE            (600)
#endif
/************************private type defination****************************/
typedef void *HANDLE_DECODER;
/**********************private function declaration*************************/
static void a2dp_audio_cc_scalable_init(void);
static void a2dp_audio_cc_scalable_deinit(void);
static bool a2dp_audio_cc_scalable_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
        a2dp_decoder_cc_media_data_t* pPcmFrame);
static void audio_audio_cc_scalable_get_sync_custom_data(uint8_t* customData);
static void audio_audio_cc_scalable_push_sync_custom_data(uint8_t* customData);

/************************private variable defination************************/
static HANDLE_DECODER cc_scalableDec_handle = NULL;
unsigned int cc_sbm_operation_fast = 0;
unsigned int cc_sbm_error_fast = 0;
unsigned int cc_sbm_operation_slow = 0;
unsigned int cc_sbm_error_slow = 0;
unsigned int cc_Frame_sequence_mismatch_Cnt;
unsigned int cc_FrameCnt;
unsigned int cc_frame_cnt_audio = 0;
static SBM_OPERATION_ENV_T cc_sbm_operation_env;
static SBM_STATUS_E cc_lastSbmStatus = SBM_IN_NORMAL_SPEED;
static uint8_t *cc_codec_mem_pool = NULL;
static uint8_t *cc_pcm_buff = NULL;
static uint8_t cc_24_bitdepth = 0;

int sbm_output_size = 0;

//#define SS_CL_DEBUG_ENABLED
#if defined(SS_CL_DEBUG_ENABLED)
static uint8_t cc_encoded_packet_indexes[16] =
    {1, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1};
static uint8_t cc_encoded_packet_index = 0;
static int cc_encoded_packet_flag = -1;
#endif
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
static uint8_t *cc_pcm_buff1 = NULL;
//static uint8_t in_buf_tmp[372];
//static uint8_t in_buf_tmp1[292];
#endif

A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_scalable_decoder_config =
{
    {44100, 1},
    a2dp_audio_cc_scalable_init,
    a2dp_audio_cc_scalable_deinit,
    a2dp_audio_cc_scalable_decode_frame,
    audio_audio_cc_scalable_get_sync_custom_data,
    audio_audio_cc_scalable_push_sync_custom_data,
};


/****************************function defination****************************/
static void a2dp_audio_cc_scalable_sbm_operation_env_reset(void)
{
    uint32_t lock = int_lock();
    memset((uint8_t *)&cc_sbm_operation_env, 0, sizeof(cc_sbm_operation_env));
    cc_sbm_operation_env.sbmOperationToDo = SBM_USE_NORMAL_SPEED;
    int_unlock(lock);

    // play speed setting for slow and fast speed
    // when normal or game mode triggered and vice versa. Default +-2.76%
    ssc_set_SBMspeed_test(CC_SBM_SLOW_SPEED, CC_SBM_FAST_SPEED, CC_SBM_NUM_NORMAL);
}

static void a2dp_audio_cc_scalable_init(void)
{
    if (cc_scalableDec_handle == NULL) {
        int decoder_size;
        decoder_size = ssc_decoder_get_size(a2dp_audio_cc_scalable_decoder_config.stream_config.num_channels,
                                                a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate);
        TRACE(0, "decoder size %d", decoder_size);
        off_bth_syspool_get_buff(&cc_codec_mem_pool, decoder_size);
        off_bth_syspool_get_buff(&cc_pcm_buff, CC_SCALABLE_FRAME_SIZE * 4 * 2);
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
        off_bth_syspool_get_buff(&cc_pcm_buff1, CC_SCALABLE_FRAME_SIZE * 4 * 2);
#endif
        ASSERT(cc_pcm_buff && cc_codec_mem_pool, "%s", __func__);

        cc_scalableDec_handle = (unsigned char *)cc_codec_mem_pool;
        cc_sbm_operation_fast = 0;
        cc_sbm_error_fast = 0;
        cc_sbm_operation_slow = 0;
        cc_sbm_error_slow = 0;
        sbm_output_size = 0;
        CC_DECODE_LOG_I("%s, channels:%d, sample_rate:%d", __func__,
            a2dp_audio_cc_scalable_decoder_config.stream_config.num_channels,
            a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate);

        if (a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate != 44100) {
            cc_24_bitdepth = 1;
        } else{
            cc_24_bitdepth = 0;
        }

        if(a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate == 96000){
            TRACE(0,"SSC UHQ ssc_decoder_init sampling rate = %d",a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate);
            ssc_decoder_init(cc_scalableDec_handle, a2dp_audio_cc_scalable_decoder_config.stream_config.num_channels,
                a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate, false,
                cc_24_bitdepth);
        }else{
            TRACE(0,"SSC Normal ssc_decoder_init sampling rate = %d",a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate);
            ssc_decoder_init(cc_scalableDec_handle, a2dp_audio_cc_scalable_decoder_config.stream_config.num_channels,
                a2dp_audio_cc_scalable_decoder_config.stream_config.sample_rate, true,
                cc_24_bitdepth);
        }

        a2dp_audio_cc_scalable_sbm_operation_env_reset();
    }
}

static void a2dp_audio_cc_scalable_deinit(void)
{
    if (cc_scalableDec_handle) {
        cc_scalableDec_handle = NULL;
    }
}

static void a2dp_audio_cc_scalable_decoder_reinit(void)
{
    CC_DECODE_LOG_I("%s", __func__);
    a2dp_audio_cc_scalable_deinit();
    a2dp_audio_cc_scalable_init();
}

static void a2dp_audio_cc_scalable_send_sbm_status(void)
{
    SBM_STATUS_MSG_T msg;
    msg.sbm_status = (SBM_STATUS_E)ssc_SBM_getstatus();
    memset(msg.sbm_param, 0, sizeof(int)*3);

    if (cc_lastSbmStatus != msg.sbm_status)
    {
        cc_lastSbmStatus = msg.sbm_status;
        CC_DECODE_LOG_I("SBM M55 cc_currentSbmStatus %d", msg.sbm_status);
        if (SBM_IN_NORMAL_SPEED == msg.sbm_status)
        {
            int ret = 0;
            app_sysfreq_req(APP_SYSFREQ_USER_SBM, APP_SYSFREQ_32K);
            ret = ssc_sbm_param_get(msg.sbm_param);
            TRACE(0, "ret %d param1 %d param2 %d param3 %d", ret, msg.sbm_param[0], msg.sbm_param[1],msg.sbm_param[2]);
        }
        else
        {
            app_sysfreq_req(APP_SYSFREQ_USER_SBM, APP_SYSFREQ_72M);
        }
        app_dsp_m55_bridge_send_cmd(
                    CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_STATUS,
                    (uint8_t *)&msg, sizeof(SBM_STATUS_MSG_T));
    }
}

static void a2dp_audio_cc_scalable_send_sbm_env(void)
{
    app_dsp_m55_bridge_send_cmd(
                    CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_BTH,
                    (uint8_t *)&cc_sbm_operation_env, sizeof(SBM_OPERATION_ENV_T));
}

static bool a2dp_audio_cc_scalable_pre_decoding_handler(uint32_t currentSeqNum)
{
    bool isNeedToRetrigger = false;

    uint32_t lock = int_lock();
    if (cc_sbm_operation_env.isToProcess)
    {
        // check for the SBM parameter validity
        if(cc_sbm_operation_env.sbmOperationToDo != SBM_USE_NORMAL_SPEED)
        {
            if(ssc_SBM_getstatus() != 0)
            {
                CC_DECODE_LOG_W("[SSC] while SBM, request SBM again");
            }
            switch(cc_sbm_operation_env.chunkOffset)
            {
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 11:
                    //normal operation!!
                    break;
                default :
                    if(cc_sbm_operation_env.sbmOperationToDo == 1)
                    {
                        if(cc_sbm_error_fast != cc_sbm_operation_env.chunkOffset)
                        {
                            cc_sbm_operation_fast++;
                        }
                        cc_sbm_error_fast = cc_sbm_operation_env.chunkOffset;
                    }
                    else if (cc_sbm_operation_env.sbmOperationToDo == -1)
                    {
                        if(cc_sbm_error_slow != cc_sbm_operation_env.chunkOffset)
                        {
                            cc_sbm_operation_slow++;
                        }
                        cc_sbm_error_slow = cc_sbm_operation_env.chunkOffset;
                    }
                    CC_DECODE_LOG_I("[SSC] SBM direction = %d, Offset = %d", cc_sbm_operation_env.sbmOperationToDo,
                        cc_sbm_operation_env.chunkOffset);
                    break;
            }
        }
        if (currentSeqNum == cc_sbm_operation_env.sequenceNumToApplyNewSpeed)
        {
            CC_DECODE_LOG_I("[SSC] SBM direction = %d, SBM offsets = %d",cc_sbm_operation_env.sbmOperationToDo,cc_sbm_operation_env.chunkOffset);
            ssc_sbm_interface(cc_sbm_operation_env.sbmOperationToDo,
                cc_sbm_operation_env.chunkOffset);
            a2dp_audio_cc_scalable_send_sbm_status();
        }
        else if (currentSeqNum > cc_sbm_operation_env.sequenceNumToApplyNewSpeed)
        {
            if (!(cc_sbm_operation_env.isSequenceNumToApplyRollBack))
            {
                CC_DECODE_LOG_W("SBM: miss the applying speed sequence number!!!");
                isNeedToRetrigger = true;
            }
            else
            {
                ssc_sbm_interface(SBM_USE_NORMAL_SPEED, 0);
                a2dp_audio_cc_scalable_send_sbm_status();
                int_unlock(lock);
                return false;
            }
        }
        else
        {
            ssc_sbm_interface(SBM_USE_NORMAL_SPEED, 0);
            a2dp_audio_cc_scalable_send_sbm_status();
            int_unlock(lock);
            return false;
        }

        CC_DECODE_LOG_I("SBM start updating.");
        CC_DECODE_LOG_I("SBM: expected seq %d current seq %d",
            cc_sbm_operation_env.sequenceNumToApplyNewSpeed, currentSeqNum);

        cc_sbm_operation_env.isToProcess = false;

        a2dp_audio_cc_scalable_send_sbm_env();
    }
    else
    {
        ssc_sbm_interface(SBM_USE_NORMAL_SPEED, 0);
        a2dp_audio_cc_scalable_send_sbm_status();
    }
    int_unlock(lock);

    return isNeedToRetrigger;
}

static void a2dp_audio_cc_scalable_covert_to_24bit_buf(int32_t * out, int32_t * in, int size)
{
    memcpy((uint8_t *)out, (uint8_t *)in, sizeof(int32_t)*size);
}

static void a2dp_audio_cc_scalable_covert_to_24bit_buf_HQ(int32_t *out, uint8_t *in, int size)
{
    for(int i=0; i< 2*size; i++){
        out[i] = (int32_t)in[3*i] + (((int32_t)in[3*i+1])<<8) + (((int32_t)in[3*i+2])<<16);
        out[i] = (int32_t)(out[i]<<8);
        out[i] = out[i]>>8;
    }
}
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
POSSIBLY_UNUSED static void a2dp_audio_cc_scalable_covert_to_24bit_buf_uhq(int32_t * out, uint8_t *in, uint8_t *in1,int size, int size1)
{
    for(int i=0; i< 2*size; i++){
        out[i] = (int32_t)in[3*i] + (((int32_t)in[3*i+1])<<8) + (((int32_t)in[3*i+2])<<16);
        out[i] = (int32_t)(out[i]<<8);
        out[i] = out[i]>>8;
    }

    for(int i=0; i< 2*size1; i++){
        out[i+2*size] = (int32_t)in1[3*i] + (((int32_t)in1[3*i+1])<<8) + (((int32_t)in1[3*i+2])<<16);
        out[i+2*size] = (int32_t)(out[i+2*size]<<8);
        out[i+2*size] = out[i+2*size]>>8;
    }
}
#endif

static bool a2dp_audio_cc_scalable_is_valid_frame(uint8_t *inputData)
{
    // Add more condition here
    if((0xFF != inputData[0]) || (0xE0 != (inputData[1] & 0xF0)))
    {
        CC_DECODE_LOG_W("scalable_decode:invalid a2dp frame,sync words not right");
        return false;
    }
    return true;
}

static bool a2dp_audio_cc_scalable_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame)
{
    int output_samples = 0;
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
    //unsigned char uhq_syncword;
#endif

    if (!cc_scalableDec_handle) {
        CC_DECODE_LOG_E("scalable_decode not ready");
        return false;
    }

    bool isNeedToRetrigger = a2dp_audio_cc_scalable_pre_decoding_handler(pEncodedPacket->pkt_seq_nb);
    if (isNeedToRetrigger)
    {
        CC_DECODE_LOG_E("scalable_decode retrigger");
        return false;
    }

    if ((!pEncodedPacket->isPLC) &&
        (!a2dp_audio_cc_scalable_is_valid_frame(pEncodedPacket->data))) {
        pEncodedPacket->isPLC = true;
    }

#if defined(SS_CL_DEBUG_ENABLED)
    if (cc_encoded_packet_flag == -1) {
        if (pEncodedPacket->data[2] & 0x01) {
            // R, S OS
            cc_encoded_packet_flag = 0;
        } else {
            // Q OS
            cc_encoded_packet_flag = 1;
        }
    }

    if (cc_encoded_packet_flag) {
        // Q OS
        if (pEncodedPacket->data[2] == 0x0) {
            cc_encoded_packet_index = 0;
        }

        if (pEncodedPacket->data[2] != cc_encoded_packet_index) {
            CC_DECODE_LOG_W("index is wrong; %x, %x", pEncodedPacket->data[2], cc_encoded_packet_index);
            cc_encoded_packet_index = pEncodedPacket->data[2];
        }

        ++cc_encoded_packet_index %= 0x100;
    } else {
        // R, S OS
        if (pEncodedPacket->data[2] == cc_encoded_packet_indexes[0]) {
            cc_encoded_packet_index = 0;
        }

        if (pEncodedPacket->data[2] != cc_encoded_packet_indexes[cc_encoded_packet_index]) {
            CC_DECODE_LOG_W("index is wrong; %x, %x",
                pEncodedPacket->data[2], cc_encoded_packet_indexes[cc_encoded_packet_index]);
            cc_encoded_packet_index
                += ((pEncodedPacket->data[2] - cc_encoded_packet_indexes[cc_encoded_packet_index]) / 0x10);
        }

        ++cc_encoded_packet_index %= 0x10;
    }
#endif

    /* decode one SSC frame */
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
    // uhq_syncword = (pEncodedPacket->data[3])&(0x1F);  // take 5 bit to get bitrate info.
    // if(uhq_syncword == 0x00 || uhq_syncword == 0x06 || uhq_syncword == 0x03) // 291kbps (0x00), 154kbps(0x06), 584kbps(0x03)
    // {
	//     TRACE(0,"UHQ uhq_syncword %x, bit size = %d seq %d",uhq_syncword,pEncodedPacket->data_len, pEncodedPacket->pkt_seq_nb);
    //     TRACE(0,"data[0] = %x,data[1] = %x,data[2] = %x,data[3] = %x",pEncodedPacket->data[0],pEncodedPacket->data[1],pEncodedPacket->data[2],pEncodedPacket->data[3]);
    // }
#endif

    output_samples = ssc_decode(cc_scalableDec_handle, &pEncodedPacket->data[0],
    cc_pcm_buff, CC_SCALABLE_FRAME_SIZE, pEncodedPacket->isPLC,
    a2dp_audio_cc_scalable_decoder_config.stream_config.num_channels);

    if(sbm_output_size != output_samples || pEncodedPacket->isPLC == true)
    {
        CC_DECODE_LOG_W("[SSC] sbm_output_size = %d, output_samples = %d",sbm_output_size, output_samples);
        sbm_output_size = output_samples;
    }

    if (0 == output_samples)
    {
        a2dp_audio_cc_scalable_decoder_reinit();
        CC_DECODE_LOG_W("scalable_decode reint codec");
        return false;
    }

    if (!cc_24_bitdepth) {
        a2dp_audio_cc_scalable_covert_to_24bit_buf((int32_t *)(pPcmFrame->data), (int32_t *)cc_pcm_buff, output_samples);
        pPcmFrame->data_len = output_samples * 4;
    } else {
        a2dp_audio_cc_scalable_covert_to_24bit_buf_HQ((int32_t *)(pPcmFrame->data), cc_pcm_buff, output_samples);
        pPcmFrame->data_len = output_samples * 4 * 2;
    }

    return true;
}

void audio_audio_cc_scalable_get_sync_custom_data(uint8_t* custom_data)
{
    uint32_t lock = int_lock();
    memcpy(custom_data, (uint8_t *)&cc_sbm_operation_env, sizeof(cc_sbm_operation_env));
    int_unlock(lock);
}

void audio_audio_cc_scalable_push_sync_custom_data(uint8_t* custom_data)
{
    uint32_t lock = int_lock();
    memcpy((uint8_t *)&cc_sbm_operation_env, custom_data, sizeof(cc_sbm_operation_env));
    int_unlock(lock);
    CC_DECODE_LOG_I("receive bth sbm env %d, %d, %d, %d, %d",
                cc_sbm_operation_env.isToProcess,
                cc_sbm_operation_env.sbmOperationToDo,
                cc_sbm_operation_env.chunkOffset,
                cc_sbm_operation_env.sequenceNumToApplyNewSpeed,
                cc_sbm_operation_env.isSequenceNumToApplyRollBack);
}

void audio_audio_cc_scalable_set_sbm_param(int* custom_data)
{
    ssc_sbm_param_set(custom_data);
}

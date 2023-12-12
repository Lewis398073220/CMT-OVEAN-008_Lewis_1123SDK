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
#ifdef A2DP_ENCODER_CROSS_CORE
#include "cmsis.h"
#include "cmsis_os.h"
#include "plat_types.h"
#include "cqueue.h"
#include "list.h"
#include "heap_api.h"
#include "app_bt.h"
#include "a2dp_encoder_cc_common.h"
#include "a2dp_encoder_cc_bth.h"
#include "app_overlay.h"

#ifdef A2DP_ENCODER_CROSS_CORE_USE_M55
#include "app_dsp_m55.h"
#endif


/************************private macro defination***************************/
#define A2DP_ENCODER_MAX_CACHED_ENCODED_FRAME_CNT       (80)
#define A2DP_ENCODER_CC_TIMEROUT_MS                     (2000)
/************************private type defination****************************/
typedef struct
{
    A2DP_ENCODER_CODEC_TYPE_E       codecType;
    uint16_t                        maxseqnum;
    uint32_t                        maxEncodedSizePerFrame;

    uint16_t                        encodedPacketEntryCount;
    uint16_t                        pcmPacketEntryCount;
    uint32_t                        cachedPacketCount;

    uint8_t                         isA2dpStarted;
    CQueue                          cachedEncodedDataQ;
    bool                            offCoreIsReady;
} A2DP_ENCODER_CC_BTH_ENV_T;

/************************private variable defination************************/
static A2DP_ENCODER_CC_BTH_ENV_T a2dp_encoder_cc_bth_env;
static uint8_t a2dp_encoder_cc_cached_encoded_frame_buf[A2DP_ENCODER_MAX_CACHED_ENCODED_FRAME_CNT * sizeof(a2dp_encoder_cc_media_data_t)];

/**********************private function declaration*************************/
static void a2dp_encoder_bth_fed_encoded_data_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_bth_ack_fed_pcm_data_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_bth_init_req_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_bth_init_wait_rsp_timeout_handler(uint8_t*, uint16_t);
static void a2dp_encoder_bth_init_rsp_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_bth_ack_fed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_bth_fed_pcm_data_transmit_handler(uint8_t* param, uint16_t len);
static void a2dp_encoder_bth_deinit_req_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_bth_deinit_wait_rsp_timeout_handler(uint8_t*, uint16_t);
static void a2dp_encoder_bth_deinit_rsp_received_handler(uint8_t* ptr, uint16_t len);

osSemaphoreDef(a2dp_encoder_cc_sema);
osSemaphoreId a2dp_encoder_cc_sema_id = NULL;

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET,
                            "FEED_PCM_DATA_PACKET",
                            a2dp_encoder_bth_fed_pcm_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET,
                            "ACK_FED_PCM_DATA_PACKET",
                            NULL,
                            a2dp_encoder_bth_ack_fed_pcm_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP,
                            "CC_A2DP_INIT_WAITING_RSP",
                            a2dp_encoder_bth_init_req_transmit_handler,
                            NULL,
                            3000,
                            a2dp_encoder_bth_init_wait_rsp_timeout_handler,
                            a2dp_encoder_bth_init_rsp_received_handler,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP,
                            "CC_A2DP_DEINIT_WAITING_RSP",
                            a2dp_encoder_bth_deinit_req_transmit_handler,
                            NULL,
                            3000,
                            a2dp_encoder_bth_deinit_wait_rsp_timeout_handler,
                            a2dp_encoder_bth_deinit_rsp_received_handler,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET,
                            "FEED_ENCODED_DATA_PACKET",
                            NULL,
                            a2dp_encoder_bth_fed_encoded_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET,
                            "ACK_FED_ENCODED_DATA_PACKET",
                            a2dp_encoder_bth_ack_fed_encoded_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);


/****************************function defination****************************/
int a2dp_encoder_cc_sema_init(void)
{
    if (a2dp_encoder_cc_sema_id == NULL) {
        a2dp_encoder_cc_sema_id = osSemaphoreCreate(osSemaphore(a2dp_encoder_cc_sema), 0);
    }
    ASSERT(a2dp_encoder_cc_sema_id, "create a2dp_encoder_cc_sema_id fail!");

    return 0;
}

int a2dp_encoder_cc_sema_deinit(void)
{
    if(a2dp_encoder_cc_sema_id) {
        osSemaphoreDelete(a2dp_encoder_cc_sema_id);
        a2dp_encoder_cc_sema_id = NULL;
    }

    return 0;
}

int a2dp_encoder_cc_sema_wait(uint32_t timeout)
{
    int ret = 0;
    if(a2dp_encoder_cc_sema_id) {
        ret = osSemaphoreAcquire(a2dp_encoder_cc_sema_id, timeout);
    }

    return ret;
}

static int a2dp_encoder_cc_sema_release()
{
    int ret = 0;
    if(a2dp_encoder_cc_sema_id) {
        ret = osSemaphoreRelease(a2dp_encoder_cc_sema_id);
    }

    return ret;
}

bool a2dp_encoder_bth_feed_pcm_data_into_off_core(uint8_t * pcm_buf, uint16_t len)
{
    if (!a2dp_encoder_cc_bth_env.offCoreIsReady) {
        CC_ENCODE_LOG_W("feed_pcm_pkt, off core not ready");
        return false;
    }

    CC_ENCODE_LOG_I("feed_pcm_pkt,len:%u", len);
    a2dp_encoder_cc_media_data_t pcmPkt;
    pcmPkt.org_addr = NULL;
    pcmPkt.data = pcm_buf;
    pcmPkt.data_len = len;
    {
        a2dp_encoder_cc_accumulate_cmd_send(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET, (uint8_t *)&pcmPkt);
    }
    return true;
}

static void a2dp_encoder_bth_ack_fed_pcm_data_received_handler(uint8_t* ptr, uint16_t len)
{
    A2DP_ENCODER_ACK_FED_ENCODED_DATA_REQ_T* pReq = (A2DP_ENCODER_ACK_FED_ENCODED_DATA_REQ_T *)ptr;

    a2dp_encoder_cc_bth_env.encodedPacketEntryCount = pReq->encodedPacketEntryCount;
    a2dp_encoder_cc_bth_env.pcmPacketEntryCount = pReq->pcmPacketEntryCount;

    CC_ENCODE_LOG_D("a2dp pkt cnt %u pcm cnt %u", a2dp_encoder_cc_bth_env.encodedPacketEntryCount,
        a2dp_encoder_cc_bth_env.pcmPacketEntryCount);
    a2dp_encoder_cc_bth_env.cachedPacketCount = a2dp_encoder_cc_bth_env.encodedPacketEntryCount +
        a2dp_encoder_cc_bth_env.pcmPacketEntryCount;
}

uint32_t a2dp_encoder_bth_get_cached_frame_cnt(void)
{
    return a2dp_encoder_cc_bth_env.cachedPacketCount;
}

static void a2dp_encoder_bth_init_req_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP, ptr, len);
}

static void a2dp_encoder_bth_init_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    a2dp_encoder_cc_bth_env.offCoreIsReady = true;
}

static void a2dp_encoder_bth_init_wait_rsp_timeout_handler(uint8_t*, uint16_t)
{
    if (a2dp_encoder_cc_bth_env.isA2dpStarted) {
        ASSERT(false, "wait for a2dp init rsp from off bth core time out!");
    }
}

static void a2dp_encoder_bth_deinit_req_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP, ptr, len);
}

static void a2dp_encoder_bth_deinit_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    CC_ENCODE_LOG_I("%s", __func__);
    a2dp_encoder_cc_sema_release();
}

static void a2dp_encoder_bth_deinit_wait_rsp_timeout_handler(uint8_t*, uint16_t)
{
    ASSERT(false, "wait for a2dp deinit rsp from off bth core time out!");
}


static void a2dp_encoder_bth_fed_encoded_data_received_handler(uint8_t* param, uint16_t len)
{
    CC_ENCODE_LOG_I("%s", __func__);
    uint16_t unitOfInfo = sizeof(a2dp_encoder_cc_media_data_t);
    uint8_t packetCnt = len / unitOfInfo;
    while (packetCnt--) {
        a2dp_encoder_cc_media_data_t* p_src_media_data_info = (a2dp_encoder_cc_media_data_t *)param;

        uint32_t lock = int_lock();
        EnCQueue(&a2dp_encoder_cc_bth_env.cachedEncodedDataQ,
            (uint8_t *)p_src_media_data_info, sizeof(a2dp_encoder_cc_media_data_t));
        int_unlock(lock);

        param += unitOfInfo;
    }
}

static void a2dp_encoder_bth_fed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    CC_ENCODE_LOG_I("%s", __func__);
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET, ptr, len);
}


static a2dp_encoder_cc_media_data_t* a2dp_encoder_bth_peek_from_cached_encodec_buff(void)
{
    a2dp_encoder_cc_media_data_t* pRet = NULL;
    uint32_t lock = int_lock();
    CQueue* data_q = &a2dp_encoder_cc_bth_env.cachedEncodedDataQ;
    if (LengthOfCQueue(data_q))
    {
        pRet = (a2dp_encoder_cc_media_data_t *)GetCQueueReadPointer(data_q);
    }
    int_unlock(lock);

    return pRet;
}


static void a2dp_encoder_bth_pop_oldest_cached_encoded_packet(void)
{
    CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
    a2dp_encoder_cc_media_data_t* pPopedMediaPacket = NULL;
    uint32_t lock = int_lock();
    CQueue* data_q = &a2dp_encoder_cc_bth_env.cachedEncodedDataQ;
    if (LengthOfCQueue(data_q))
    {
        pPopedMediaPacket = (a2dp_encoder_cc_media_data_t *)GetCQueueReadPointer(data_q);
        DeCQueue(data_q, NULL, sizeof(a2dp_encoder_cc_media_data_t));
    }
    int_unlock(lock);

    if (pPopedMediaPacket)
    {
        // pop from a2dp audio list when its decoded pcm data is fed
        // if(pPopedMediaPacket->coorespondingPcmDataPacketPtr)
        //     a2dp_encoder_remove_from_list(pPopedMediaPacket->coorespondingPcmDataPacketPtr);

        a2dp_encoder_cc_accumulate_cmd_send(
            CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET, \
            (uint8_t*)pPopedMediaPacket);
    }
}


int a2dp_encoder_bth_fetch_encoded_data(a2dp_source_packet_t *source_packet)
{
    CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
    a2dp_encoder_cc_media_data_t *pMediaData = NULL;
    pMediaData = a2dp_encoder_bth_peek_from_cached_encodec_buff();

    if (pMediaData) {
        if(pMediaData->data_len) {
            btif_a2dp_sbc_packet_t *Packet = &source_packet->packet;
            Packet->dataLen = pMediaData->data_len;
            // Packet->frameNum = pMediaData->data_len/876;
            // Packet->frameSize = pMediaData->data_len/Packet->frameNum;
            // source_packet->codec_type = BT_A2DP_CODEC_TYPE_LHDC;
            uint32_t lock = int_lock();
            memcpy(Packet->data, pMediaData->data, pMediaData->data_len);
            int_unlock(lock);
        }
        a2dp_encoder_bth_pop_oldest_cached_encoded_packet();
        CC_ENCODE_LOG_I("%s, %d, %d", __func__, __LINE__, pMediaData->data_len);
        return 1;
    } else {
        //CC_ENCODE_LOG_W("%s, %d", __func__, __LINE__);
        return 0;
    }
}

static void a2dp_encoder_bth_ack_fed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET, ptr, len);
}


static void a2dp_encoder_bth_reset(void)
{
    uint32_t lock = int_lock();

    memset(&a2dp_encoder_cc_bth_env, 0x00, sizeof(A2DP_ENCODER_CC_BTH_ENV_T));
    a2dp_encoder_cc_bth_env.isA2dpStarted = true;

    InitCQueue(&a2dp_encoder_cc_bth_env.cachedEncodedDataQ,
        sizeof(a2dp_encoder_cc_cached_encoded_frame_buf),
        (CQItemType *)a2dp_encoder_cc_cached_encoded_frame_buf);
    int_unlock(lock);
}

void a2dp_encoder_bth_stop_stream(void)
{
    a2dp_encoder_cc_cmd_cache_deinit();

    a2dp_encoder_cc_bth_env.offCoreIsReady = false;
    a2dp_encoder_cc_bth_env.isA2dpStarted = false;
    int res = a2dp_encoder_cc_sema_wait(A2DP_ENCODER_CC_TIMEROUT_MS);
    if (res != osOK) {
        CC_ENCODE_LOG_E("%s err = %d",__func__,res);
    }
    app_overlay_subsys_unloadall(APP_OVERLAY_M55);
}

void a2dp_encoder_bth_start_stream(
        A2DP_ENCODER_CODEC_TYPE_E codecType,
        A2DP_AUDIO_CC_OUTPUT_CONFIG_T *config,
        uint16_t maxseqnunm, uint16 maxosize_pframe)
{
    uint8_t chnlCount = 1;

    a2dp_encoder_bth_reset();
    CC_ENCODE_LOG_I("%s, chnlCount:%d, frame_samples %d", __func__,
        chnlCount, config->frame_samples);

    a2dp_encoder_cc_bth_env.codecType = codecType;
    a2dp_encoder_cc_bth_env.maxseqnum = maxseqnunm;
    uint8_t bits_depth = config->bits_depth;
    a2dp_encoder_cc_bth_env.maxEncodedSizePerFrame = maxosize_pframe;
    //
    A2DP_ENCODER_INIT_REQ_T initReq;
    initReq.codecType = a2dp_encoder_cc_bth_env.codecType;
    initReq.maxEncodeDataSizePerFrame = a2dp_encoder_cc_bth_env.maxEncodedSizePerFrame;
    memcpy((uint8_t *)&initReq.stream_cfg, (uint8_t *)config, sizeof(A2DP_AUDIO_CC_OUTPUT_CONFIG_T));
    initReq.stream_cfg.bits_depth = bits_depth;
    app_dsp_m55_bridge_send_cmd(
                     CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP, \
                     (uint8_t*)&initReq, \
                     sizeof(A2DP_ENCODER_INIT_REQ_T));
}
#endif

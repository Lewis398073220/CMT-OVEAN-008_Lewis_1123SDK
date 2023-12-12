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
#ifdef A2DP_DECODER_CROSS_CORE
#include "cmsis.h"
#include "cmsis_os.h"
#include "plat_types.h"
#include "cqueue.h"
#include "list.h"
#include "heap_api.h"
#include "aac_api.h"
#include "app_bt.h"
#include "a2dp_decoder_cc_common.h"
#include "a2dp_decoder_cc_bth.h"
#include "app_overlay.h"
#include "a2dp_decoder_internal.h"
#include "app_audio.h"
#include "btapp.h"
#include "app_ibrt_internal.h"
#include "app_tws_ibrt_audio_analysis.h"
#ifdef A2DP_LHDC_ON
#include "lhdcBesCCUtil.h"
extern int bes_bt_local_info_get(bes_bt_local_info *local_info);
#endif
#ifdef A2DP_DECODER_CROSS_CORE_USE_M55
#include "app_dsp_m55.h"
#endif

#ifdef IBRT
#include "app_tws_ibrt_audio_analysis.h"
#endif

/*********************external function declaration*************************/
extern "C" int app_ibrt_if_force_audio_retrigger(uint8_t retriggerType);
extern "C" void a2dp_decoder_remove_from_list(void *data);
extern bool a2dp_audio_ibrt_is_slave(void);
extern void a2dp_audio_send_master_refill(uint8_t *refill);

extern "C" void a2dp_decoder_update_last_frame_info(
    uint16_t sequenceNumber,
    uint32_t timestamp,
    uint16_t curSubSequenceNumber,
    uint16_t totalSubSequenceNumber);
#ifdef A2DP_SCALABLE_ON
extern void audio_audio_scalable_push_sync_custom_data(uint8_t* custom_data);
#endif

#ifdef ADVANCE_FILL_ENABLED
extern bool is_doing_pre_fill;
#endif

/************************private macro defination***************************/
#define A2DP_DECODER_MAX_CACHED_PCM_FRAME_CNT       (80)
#define A2DP_MAXIMUM_WAITING_PCM_COUNT              (5)
#define A2DP_WAITING_PCM_MS_PER_ROUND               (2)
#define CC_DECODER_TEMP_BUFFER_SIZE                 (8 * 1024)
#define A2DP_DECODER_CC_TIMEROUT_MS                 (2000)
/************************private type defination****************************/
typedef struct
{
    A2DP_DECODER_CODEC_TYPE_E       codecType;
    A2DP_DECODER_CHANNEL_SELECT_E   channel;
    uint16_t                        maxseqnum;
    uint32_t                        maxPCMSizePerFrame;

    uint16_t                        encodedPacketEntryCount;
    uint16_t                        pcmPacketEntryCount;
    uint32_t                        cachedPacketCount;

    uint8_t                         isA2dpReceivingJustStarted;
    uint8_t                         isDecodingFirstRun;

    uint16_t                        lastReceivedSeqNum;
    uint16_t                        lastReceivedTotalSubSeqCount;
    uint16_t                        lastReceivedSubSeq;

    CQueue                          cachedPcmDataQ;

    bool                            offCoreBoostProcessing;
    uint16_t                        offCoreEncodedListLen;
    bool                            offCoreIsReady;
} A2DP_DECODER_CC_BTH_ENV_T;

/************************private variable defination************************/
static A2DP_DECODER_CC_BTH_ENV_T a2dp_decoder_cc_bth_env;
static uint8_t a2dp_decoder_cc_cached_pcm_frame_buf[A2DP_DECODER_MAX_CACHED_PCM_FRAME_CNT * sizeof(a2dp_decoder_cc_media_data_t)];
static SBM_STATUS_MSG_T cc_sbm_status;
static bool a2dp_decoder_cc_trigger_flag = false;

static uint8_t cc_off_core_retry_count = 0;
static uint8_t cc_plc_cnt = 0;
static uint8_t *ccLeftBuff = NULL;
static uint16_t ccLeftBytes = 0;
static uint16_t ccLeftBufOffset = 0;

#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
extern uint8_t __lhdc_license_start[];
static uint32_t lhdc_data = 0;
#endif
/**********************private function declaration*************************/
static void a2dp_decoder_bth_feed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_ack_fed_encoded_data_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_init_req_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_init_wait_rsp_timeout_handler(uint8_t*, uint16_t);
static void a2dp_decoder_bth_init_rsp_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_ack_fed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_retrigger_req_received_handler(uint8_t* param, uint16_t len);
static void a2dp_decoder_bth_rm_specific_frame_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_fed_pcm_data_received_handler(uint8_t* param, uint16_t len);
static void a2dp_decoder_bth_req_boost_freq_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_boost_done_received_handler(uint8_t* param, uint16_t len);
static void a2dp_decoder_bth_deinit_req_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_deinit_wait_rsp_timeout_handler(uint8_t*, uint16_t);
static void a2dp_decoder_bth_deinit_rsp_received_handler(uint8_t* ptr, uint16_t len);

#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
static void a2dp_decoder_bth_fed_lhdc_data_received_handler(uint8_t* ptr, uint16_t len);
extern uint8_t bt_lhdc_player_get_ext_flags(void);
#endif
#ifdef A2DP_SCALABLE_ON
static void a2dp_decoder_bth_sbm_to_off_bth_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_sbm_to_bth_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_bth_sbm_status_received_handler(uint8_t* ptr, uint16_t len);
#endif

static void a2dp_decoder_cc_set_triggered_state_transmit_handler(uint8_t* ptr, uint16_t len);

osSemaphoreDef(a2dp_decoder_cc_sema);
osSemaphoreId a2dp_decoder_cc_sema_id = NULL;

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET,
                            "FEED_ENCODED_DATA_PACKET",
                            a2dp_decoder_bth_feed_encoded_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET,
                            "ACK_FED_ENCODED_DATA_PACKET",
                            NULL,
                            a2dp_decoder_bth_ack_fed_encoded_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP,
                            "CC_A2DP_INIT_WAITING_RSP",
                            a2dp_decoder_bth_init_req_transmit_handler,
                            NULL,
                            3000,
                            a2dp_decoder_bth_init_wait_rsp_timeout_handler,
                            a2dp_decoder_bth_init_rsp_received_handler,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP,
                            "CC_A2DP_DEINIT_WAITING_RSP",
                            a2dp_decoder_bth_deinit_req_transmit_handler,
                            NULL,
                            3000,
                            a2dp_decoder_bth_deinit_wait_rsp_timeout_handler,
                            a2dp_decoder_bth_deinit_rsp_received_handler,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET,
                            "FEED_PCM_DATA_PACKET",
                            NULL,
                            a2dp_decoder_bth_fed_pcm_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET,
                            "ACK_FED_PCM_DATA_PACKET",
                            a2dp_decoder_bth_ack_fed_pcm_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_RETRIGGER_REQ_NO_RSP,
                            "CC_A2DP_RETRIGGER_REQ_NO_RSP",
                            NULL,
                            a2dp_decoder_bth_retrigger_req_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME,
                            "A2DP_REMOVE_SPECIFIC_FRAME",
                            a2dp_decoder_bth_rm_specific_frame_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_REQ,
                            "BOOST_FREQ_REQ",
                            a2dp_decoder_bth_req_boost_freq_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_DONE,
                            "BOOST_FREQ_DONE",
                            NULL,
                            a2dp_decoder_bth_boost_done_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);
#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_LHDC_FED_DATA_PACKET,
                            "LHDC_RANDOM_DATA",
                            NULL,
                            a2dp_decoder_bth_fed_lhdc_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);  
#endif
#ifdef A2DP_SCALABLE_ON
RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_OFF_BTH,
                            "SBM_TO_OFF_BTH",
                            a2dp_decoder_bth_sbm_to_off_bth_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_BTH,
                            "SBM_TO_BTH",
                            NULL,
                            a2dp_decoder_bth_sbm_to_bth_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_STATUS,
                            "SBM_STATUS",
                            NULL,
                            a2dp_decoder_bth_sbm_status_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);
#endif

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SET_TRIGGERED_STATE,
                            "CC_A2DP_SET_TRIGGERED_STATE",
                            a2dp_decoder_cc_set_triggered_state_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

/****************************function defination****************************/
#ifdef A2DP_SCALABLE_ON
static void a2dp_decoder_bth_sbm_to_off_bth_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_OFF_BTH, ptr, len);
}

static void a2dp_decoder_bth_sbm_to_bth_received_handler(uint8_t* ptr, uint16_t len)
{
    audio_audio_scalable_push_sync_custom_data(ptr);
}

static void a2dp_decoder_bth_sbm_status_received_handler(uint8_t* ptr, uint16_t len)
{
    uint32_t lock = int_lock();
    memcpy(&cc_sbm_status, ptr, sizeof(SBM_STATUS_MSG_T));
    TRACE(0, "BTH STATUS %d PARAM1 %d PARAM2 %d PARAM3 %d", cc_sbm_status.sbm_status, cc_sbm_status.sbm_param[0], cc_sbm_status.sbm_param[1], cc_sbm_status.sbm_param[2]);
    int_unlock(lock);
}

uint8_t a2dp_decoder_bth_get_cc_sbm_status(void)
{
    uint8_t status = 0;
    status = (uint8_t)cc_sbm_status.sbm_status;
    return status;
}

int *a2dp_decoder_bth_get_cc_sbm_param(void)
{
    return cc_sbm_status.sbm_param;
}

#endif

int a2dp_decoder_cc_sema_init(void)
{
    if (a2dp_decoder_cc_sema_id == NULL) {
        a2dp_decoder_cc_sema_id = osSemaphoreCreate(osSemaphore(a2dp_decoder_cc_sema), 0);
    }
    ASSERT(a2dp_decoder_cc_sema_id, "create a2dp_decoder_cc_sema_id fail!");

    return 0;
}

int a2dp_decoder_cc_sema_deinit(void)
{
    if(a2dp_decoder_cc_sema_id) {
        osSemaphoreDelete(a2dp_decoder_cc_sema_id);
        a2dp_decoder_cc_sema_id = NULL;
    }

    return 0;
}

int a2dp_decoder_cc_sema_wait(uint32_t timeout)
{
    int ret = 0;
    if(a2dp_decoder_cc_sema_id) {
        ret = osSemaphoreAcquire(a2dp_decoder_cc_sema_id, timeout);
    }

    return ret;
}

static int a2dp_decoder_cc_sema_release()
{
    int ret = 0;
    if(a2dp_decoder_cc_sema_id) {
        ret = osSemaphoreRelease(a2dp_decoder_cc_sema_id);
    }

    return ret;
}

static void a2dp_decoder_cc_set_triggered_state_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_SET_TRIGGERED_STATE, ptr, len);
}

void a2dp_decoder_bth_set_trigger_flag(bool flag)
{
    TRACE(0, "%s %d", __func__, flag);
    a2dp_decoder_cc_trigger_flag = flag;
}

void a2dp_decoder_bth_set_first_run(A2DP_AUDIO_STARTFRAME_INFO_T headframe)
{
    TRACE(0, "%s start seq %d subseq %d is ignore %d", __func__, headframe.sequenceNumber, headframe.curSubSequenceNumber, headframe.is_ignore);
    a2dp_decoder_cc_bth_env.isDecodingFirstRun = true;
    a2dp_audio_start_seq_set(headframe);
    a2dp_decoder_bth_set_trigger_flag(true);
}

uint16_t a2dp_decoder_get_off_bth_encoded_list_len(void)
{
    return a2dp_decoder_cc_bth_env.offCoreEncodedListLen;
}

// Return value:
// normal the a2dp packek shall be consumed after receiving
// the ack from off bth core
bool a2dp_decoder_bth_feed_encoded_data_into_off_core(
    uint8_t* list_data,
    uint32_t time_stamp, uint16_t seq_num,
    uint16_t total_sub_seq, uint16_t sub_seq,
    uint16_t data_len, uint8_t* pData)
{
    if (!a2dp_decoder_cc_bth_env.offCoreIsReady) {
        CC_DECODE_LOG_W("feed_encoded_pkt, off core not ready");
        return false;
    }

    if (!a2dp_decoder_cc_trigger_flag) {
        CC_DECODE_LOG_W("feed_encoded_pkt, trigger not ready");
        return false;
    }

    CC_DECODE_LOG_D("feed_encoded_pkt,len:%u seq:%u subseq:%u", data_len, seq_num, sub_seq);
    a2dp_decoder_cc_media_data_t encodedPkt;
    encodedPkt.org_addr = list_data;
    encodedPkt.pkt_seq_nb = seq_num;
    encodedPkt.time_stamp = time_stamp;
    encodedPkt.subSequenceNumber = sub_seq;
    encodedPkt.totalSubSequenceNumber = total_sub_seq;
    encodedPkt.data = pData;
    encodedPkt.data_len = data_len;

    if (UINT32_MAX == time_stamp) {
        encodedPkt.isPLC = true;
    } else {
        encodedPkt.isPLC = false;
    }

    a2dp_decoder_cc_accumulate_cmd_send(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET, (uint8_t *)&encodedPkt);
    return true;
}

static void a2dp_decoder_bth_feed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET, ptr, len);
}

static void a2dp_decoder_bth_req_boost_freq_handler(uint8_t* ptr, uint16_t len)
{
    a2dp_decoder_cc_bth_env.offCoreBoostProcessing = true;

    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_REQ, ptr, len);
}

static void a2dp_decoder_bth_ack_fed_encoded_data_received_handler(uint8_t* ptr, uint16_t len)
{
    A2DP_DECODER_ACK_FED_ENCODED_DATA_REQ_T* pReq = (A2DP_DECODER_ACK_FED_ENCODED_DATA_REQ_T *)ptr;

    // update the encoded and pcm list entry count on the off bth core side
    a2dp_decoder_cc_bth_env.encodedPacketEntryCount = pReq->encodedPacketEntryCount;
    a2dp_decoder_cc_bth_env.pcmPacketEntryCount = pReq->pcmPacketEntryCount;

    CC_DECODE_LOG_D("a2dp pkt cnt %u pcm cnt %u", a2dp_decoder_cc_bth_env.encodedPacketEntryCount,
        a2dp_decoder_cc_bth_env.pcmPacketEntryCount);
    a2dp_decoder_cc_bth_env.cachedPacketCount = a2dp_decoder_cc_bth_env.encodedPacketEntryCount +
        a2dp_decoder_cc_bth_env.pcmPacketEntryCount;
}

uint32_t a2dp_decoder_bth_get_cached_frame_cnt(void)
{
    return a2dp_decoder_cc_bth_env.cachedPacketCount;
}

static void a2dp_decoder_bth_init_req_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP, ptr, len);
}

static void a2dp_decoder_bth_init_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    a2dp_decoder_cc_bth_env.offCoreIsReady = true;
}

static void a2dp_decoder_bth_init_wait_rsp_timeout_handler(uint8_t*, uint16_t)
{
    if (cc_off_core_retry_count++ < 3) {
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_OFF_CORE_BOOT_FAILED);
        return;
    }

    ASSERT(false, "wait for a2dp init rsp from off bth core time out!");
}

static void a2dp_decoder_bth_deinit_req_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_with_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP, ptr, len);
}

static void a2dp_decoder_bth_deinit_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "%s", __func__);
    a2dp_decoder_cc_sema_release();
}

static void a2dp_decoder_bth_deinit_wait_rsp_timeout_handler(uint8_t*, uint16_t)
{
    ASSERT(false, "wait for a2dp deinit rsp from off bth core time out!");
}

static void a2dp_decoder_bth_fed_pcm_data_received_handler(uint8_t* param, uint16_t len)
{
    uint16_t unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
    uint8_t packetCnt = len / unitOfInfo;
    while (packetCnt--) {
        a2dp_decoder_cc_media_data_t* p_src_media_data_info = (a2dp_decoder_cc_media_data_t *)param;
        CC_DECODE_LOG_D("rec fed pcm seq %d-%d data_len %d", p_src_media_data_info->pkt_seq_nb, 
                            p_src_media_data_info->subSequenceNumber, p_src_media_data_info->data_len);
        if (p_src_media_data_info->encoded_list_len>150)
        {
            TRACE(0, "encoded_list %d", p_src_media_data_info->encoded_list_len);
        }
        a2dp_decoder_cc_bth_env.offCoreEncodedListLen = p_src_media_data_info->encoded_list_len;
        uint32_t lock = int_lock();
        int ret = EnCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ,
            (uint8_t *)p_src_media_data_info, sizeof(a2dp_decoder_cc_media_data_t));
        int_unlock(lock);

        if (CQ_OK != ret)
        {
            CC_DECODE_LOG_E("local a2dp pcm cache overflow: %u",
                LengthOfCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ)/
                sizeof(a2dp_decoder_cc_media_data_t));
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_LOCAL_PCM_BUF_OVERFLOW);
        }
        param += unitOfInfo;
    }
}

#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
static void a2dp_decoder_bth_fed_lhdc_data_received_handler(uint8_t* ptr, uint16_t len)
{
    lhdc_data = *ptr;
    CC_DECODE_LOG_D("[BRUCE DEBUG]%s Data received 0x%02X", __func__, *ptr);
    return;
}
#endif

static a2dp_decoder_cc_media_data_t* a2dp_decoder_bth_peek_from_cached_pcm(void)
{
    a2dp_decoder_cc_media_data_t* pRet = NULL;
    uint32_t lock = int_lock();

    if (LengthOfCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ))
    {
        pRet = (a2dp_decoder_cc_media_data_t *)GetCQueueReadPointer(&a2dp_decoder_cc_bth_env.cachedPcmDataQ);
    }
    int_unlock(lock);

    return pRet;
}

static void a2dp_decoder_bth_pop_oldest_cached_pcm(void)
{
    a2dp_decoder_cc_media_data_t* pPopedMediaPacket = NULL;
    uint32_t lock = int_lock();
    if (LengthOfCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ))
    {

        pPopedMediaPacket = (a2dp_decoder_cc_media_data_t *)GetCQueueReadPointer(&a2dp_decoder_cc_bth_env.cachedPcmDataQ);
        DeCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ, NULL, sizeof(a2dp_decoder_cc_media_data_t));
    }
    int_unlock(lock);

    if (pPopedMediaPacket)
    {
        // pop from a2dp audio list when its decoded pcm data is fed
        a2dp_decoder_remove_from_list(pPopedMediaPacket->coorespondingEncodedDataPacketPtr);

        a2dp_decoder_cc_accumulate_cmd_send(
            CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET, \
            (uint8_t*)pPopedMediaPacket);
    }
}

/********************************************************************************************
This buffer(leftBuff) is used to store the remaining PCM data
[Scalable codec]
fast speed,840samples/frame:
two frames need to be fetched in one DMA irq
The remaining length is 840 * 2 - 864 = 816 (samples)
low speed,888samples/frame:
The remaining length is 888 - 864 = 24 (samples)

[SBC codec]
If the earbud support 3M packet. Some devices will choose to
send more than 5 subframes in one AVDTP data. Like 7 or 8
ping/pang buffer size = 512 * 5 bytes
BT_AUDIO_BUFF_SBC_44P1K_SIZE (128*5*2*2*2)
The remaining length is 3 or 2 * 512

@CC_DECODER_TEMP_BUFFER_SIZE
*********************************************************************************************/
int a2dp_decoder_bth_fetch_pcm_data(uint16_t last_seq_num,
    uint16_t last_total_sub_seq, uint16_t last_sub_seq,
    uint8_t *ptrData, uint16_t dataLen)
{
    uint8_t waitPcmDataCount = 0;
    uint16_t expectedSeqNum = 0;
    uint16_t expectedSubSeqNum = 0;
    uint16_t fetchedLen = 0;
    a2dp_decoder_cc_media_data_t *pMediaData = NULL;
    uint16_t expectedLen = dataLen;
    uint8_t *pBufOffset = ptrData;
    uint8_t *pOperationCusor = NULL;
    uint8_t wait_ms_per_round = 0;

    // Using the remaining PCM data to fill ping-pang buffer first
    if (ccLeftBytes) {
        if (ccLeftBytes < dataLen) {
            memcpy(ptrData, ccLeftBuff + ccLeftBufOffset, ccLeftBytes);
            pBufOffset += ccLeftBytes;
            expectedLen = dataLen - ccLeftBytes;
            ccLeftBufOffset = 0;
            ccLeftBytes = 0;
        } else {
            // the remaining PCM data is enough,fill ping-pang buffer directly
            memcpy(ptrData, ccLeftBuff + ccLeftBufOffset, dataLen);
            ccLeftBytes -= dataLen;
            ccLeftBufOffset += dataLen;
            return A2DP_DECODER_NO_ERROR;
        }
    }

    int8_t refill_subframes = app_tws_ibrt_audio_analysis_get_refill_frames();

    if (0 == last_total_sub_seq)
    {
        // no sub-frame
        if (refill_subframes<0)
        {
            expectedSeqNum = last_seq_num + 1 - refill_subframes;
        }
        else if (refill_subframes>0)
        {
            if(a2dp_audio_ibrt_is_slave())
            {
                TRACE(0, "REFILL LET MASTER DO");
                uint8_t refill_info = refill_subframes;
                //let master do
                a2dp_audio_send_master_refill(&refill_info);
            }
            expectedSeqNum = last_seq_num + 1;
        }
        else
        {
            expectedSeqNum = last_seq_num + 1;
        }
        expectedSubSeqNum = 0;
    }
    else
    {
        uint8_t total_missubframe = abs(refill_subframes);
        int8_t misframes = 0;
        int8_t subframes = 0;
        if (refill_subframes < 0)
        {
            misframes = (last_sub_seq + total_missubframe) / last_total_sub_seq;
            subframes = (last_sub_seq + total_missubframe) % last_total_sub_seq;
            last_seq_num += misframes;
            last_sub_seq = subframes;//BES intentional code.It will not cause numerical error
        }
        else if (refill_subframes > 0)
        {
            if(a2dp_audio_ibrt_is_slave())
            {
                TRACE(0, "REFILL LET MASTER DO");
                uint8_t refill_info = refill_subframes;
                //let master do
                a2dp_audio_send_master_refill(&refill_info);
            }
        }

        if (last_sub_seq < last_total_sub_seq - 1)
        {
            expectedSeqNum = last_seq_num;
            expectedSubSeqNum = last_sub_seq+1;
        } else {
            expectedSeqNum = last_seq_num+1;
            expectedSubSeqNum = 0;
        }
    }
    if (refill_subframes)
    {
        app_tws_ibrt_audio_analysis_update_refill_frames(-refill_subframes);
    }

    if (a2dp_decoder_cc_bth_env.isDecodingFirstRun) {
        A2DP_AUDIO_STARTFRAME_INFO_T start_info;
        a2dp_audio_start_seq_get(&start_info);
        if (start_info.is_ignore)
        {
            A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
            a2dp_audio_decoder_headframe_info_get(&headframe_info);
            expectedSeqNum = headframe_info.sequenceNumber;
            expectedSubSeqNum = headframe_info.curSubSequenceNumber;
        }
        else
        {
            expectedSeqNum = start_info.sequenceNumber;
            expectedSubSeqNum = start_info.curSubSequenceNumber;
        }
        CC_DECODE_LOG_I("cc decoding first run seq %d-%d", expectedSeqNum, expectedSubSeqNum);
        a2dp_decoder_cc_bth_env.isDecodingFirstRun = false;
    }

    if (a2dp_audio_list_length_TWP() > last_total_sub_seq)
    {
        cc_plc_cnt = 0;
    }

    if (a2dp_decoder_cc_bth_env.maxseqnum == UINT8_MAX)
    {
        wait_ms_per_round = 1;
    }
    else
    {
        wait_ms_per_round = A2DP_WAITING_PCM_MS_PER_ROUND;
    }

    do
    {
        pOperationCusor = ccLeftBuff + fetchedLen;
peek_again:
        pMediaData = a2dp_decoder_bth_peek_from_cached_pcm();
        if (NULL == pMediaData)
        {
            if (waitPcmDataCount > 1)
            {
                CC_DECODE_LOG_W("pcm queue empty,wait %d ms, seq:%u-%u, boost:%d",
                    waitPcmDataCount * wait_ms_per_round,
                    expectedSeqNum, expectedSubSeqNum, a2dp_decoder_cc_bth_env.offCoreBoostProcessing);
            }

            if ((waitPcmDataCount > 1) && (!a2dp_decoder_cc_bth_env.offCoreBoostProcessing)) {
                a2dp_audio_set_freq_user_case(A2DP_AUDIO_BOOST_MODE_FREQ);
                a2dp_decoder_cc_bth_env.offCoreBoostProcessing = true;
            }

            waitPcmDataCount++;
            if (1 == waitPcmDataCount)
            {
                osDelay(wait_ms_per_round);
                goto peek_again;
            }
            else if (2 == waitPcmDataCount)
            {            
            #ifdef ADVANCE_FILL_ENABLED
                if ((0 == a2dp_audio_list_length_TWP()) && (cc_plc_cnt < 1) && !is_doing_pre_fill)
            #else
                if ((0 == a2dp_audio_list_length_TWP()) && (cc_plc_cnt < 1))
            #endif
                {
                    cc_plc_cnt++;
                    a2dp_audio_store_fake_packet(expectedSeqNum);
                }
                osDelay(wait_ms_per_round);
                goto peek_again;
            }
            else if (waitPcmDataCount <= A2DP_MAXIMUM_WAITING_PCM_COUNT)
            {
                osDelay(wait_ms_per_round);
                goto peek_again;
            }
            else
            {
                cc_plc_cnt = 0;
                memset(ptrData, 0, dataLen);
                return A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
            }
        }

        if (a2dp_decoder_cc_bth_env.maxseqnum == UINT8_MAX)
        {
            expectedSeqNum &= 0x00FF;
        }

        // update total sub seq according to the current packet
        last_total_sub_seq = pMediaData->totalSubSequenceNumber;

        uint16_t checkRet = a2dp_decoder_cc_check_seq_number(pMediaData->pkt_seq_nb, expectedSeqNum,
            last_total_sub_seq, pMediaData->subSequenceNumber, expectedSubSeqNum);

        CC_DECODE_LOG_D("fetch_pcm_data:%d-%d expected seq:%u-%u sub seq:%u-%u, len:%u", checkRet, last_total_sub_seq,
            expectedSeqNum, pMediaData->pkt_seq_nb, expectedSubSeqNum, pMediaData->subSequenceNumber, pMediaData->data_len);

        if (A2DP_SEQ_CHECK_RESULT_MATCH != checkRet)
        {
            CC_DECODE_LOG_W("fetch_pcm_data_check:%d-%d, expected:%d-%d, actSeq:%d-%d", checkRet, fetchedLen,
                expectedSeqNum, expectedSubSeqNum, pMediaData->pkt_seq_nb, pMediaData->subSequenceNumber);
        }

        if (checkRet & A2DP_SEQ_CHECK_RESULT_DIFF_EXCEEDS_LIMIT)
        {
            a2dp_decoder_bth_pop_oldest_cached_pcm();
            goto peek_again;
        }

        if (A2DP_SEQ_CHECK_RESULT_MATCH == checkRet)
        {
            a2dp_decoder_update_last_frame_info(
                expectedSeqNum,
                pMediaData->time_stamp,
                expectedSubSeqNum,
                pMediaData->totalSubSequenceNumber);

            uint32_t lock = int_lock();
            memcpy(pOperationCusor, pMediaData->data, pMediaData->data_len);
            int_unlock(lock);
            fetchedLen += pMediaData->data_len;
            a2dp_decoder_bth_pop_oldest_cached_pcm();
        }
        else if (A2DP_SEQ_CHECK_RESULT_LATER_THAN_EXPECTED == checkRet)
        {
            // fill the fake frame
            a2dp_decoder_update_last_frame_info(
                expectedSeqNum,
                pMediaData->time_stamp,
                expectedSubSeqNum,
                last_total_sub_seq);

            uint32_t lock = int_lock();
//            memcpy(pOperationCusor, pMediaData->data, pMediaData->data_len);
            memset(pOperationCusor, 0, pMediaData->data_len);
            int_unlock(lock);
            fetchedLen += pMediaData->data_len;
        }
        else if (A2DP_SEQ_CHECK_RESULT_EARLIER_THAN_EXPECTED == checkRet)
        {
            // discard the frame
            a2dp_decoder_bth_pop_oldest_cached_pcm();
            goto peek_again;
        }
        else
        {
            break;
        }

        if (fetchedLen < expectedLen) {
            waitPcmDataCount = 0;
            if (last_total_sub_seq == 0) {
                // no sub frame
                expectedSeqNum++;
            } else if (++expectedSubSeqNum == last_total_sub_seq) {
                expectedSeqNum++;
                expectedSubSeqNum = 0;
            }
        } else {
            break;
        }
    } while (1);

    // fill ping-pang buffer
    memcpy(pBufOffset, ccLeftBuff, expectedLen);
    if (fetchedLen > expectedLen) {
        // save the remaining PCM data
        ccLeftBytes = fetchedLen - expectedLen;
        ccLeftBufOffset = expectedLen;
    }
    return A2DP_DECODER_NO_ERROR;
}

void a2dp_decoder_bth_a2dp_boost_freq_req(void)
{
    if (!a2dp_decoder_cc_bth_env.offCoreBoostProcessing) {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_REQ, NULL, 0);
        a2dp_decoder_cc_bth_env.offCoreBoostProcessing = true;
    }
}
static void a2dp_decoder_bth_ack_fed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET, ptr, len);
}

static void a2dp_decoder_bth_retrigger_req_received_handler(uint8_t* param, uint16_t len)
{
    app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_DECODE_STATUS_ERROR);
}

static void a2dp_decoder_bth_boost_done_received_handler(uint8_t* param, uint16_t len)
{
    a2dp_decoder_cc_bth_env.offCoreBoostProcessing = false;
    a2dp_audio_set_freq_user_case(A2DP_AUDIO_FREQ_USER_CASE_INVALID);
    a2dp_audio_sysfreq_update_normalfreq();
}

void a2dp_decoder_bth_remove_specifc_frame(    A2DP_DECODER_CC_FLUSH_T flushInfo)
{
    //a2dp_decoder_cc_bth_env.isDecodingFirstRun = false;
    a2dp_decoder_cc_media_data_t* pMediaData = NULL;

    do
    {
        pMediaData = a2dp_decoder_bth_peek_from_cached_pcm();
        if (pMediaData)
        {
            // remove its local cached pcm if existing
            uint16_t checkRet = a2dp_decoder_cc_check_seq_number(pMediaData->pkt_seq_nb,
                flushInfo.seqNum, flushInfo.totalSubSeq,
                pMediaData->subSequenceNumber, flushInfo.subSeq);

            CC_DECODE_LOG_D("Remove_frame_check:%d, cached entry seq:%u frame addr 0x%x",
                checkRet, pMediaData->pkt_seq_nb,
                (uint32_t)pMediaData->coorespondingEncodedDataPacketPtr);

            if ((A2DP_SEQ_CHECK_RESULT_MATCH == checkRet) ||
                (A2DP_SEQ_CHECK_RESULT_EARLIER_THAN_EXPECTED & checkRet))
            {
                DeCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ, NULL,
                                sizeof(a2dp_decoder_cc_media_data_t));
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }

    } while (1);

    // remove the cooresponding
    // a2dp packet and pcm list entries on the off bth core side
    a2dp_decoder_cc_accumulate_cmd_send(CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME, (uint8_t *)&flushInfo);

}

static void a2dp_decoder_bth_rm_specific_frame_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(
        CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME, ptr, len);
}

static void a2dp_decoder_bth_reset(void)
{
    uint32_t lock = int_lock();

    memset(&a2dp_decoder_cc_bth_env, 0x00, sizeof(A2DP_DECODER_CC_BTH_ENV_T));
    a2dp_decoder_cc_bth_env.isDecodingFirstRun = true;
    a2dp_decoder_cc_bth_env.isA2dpReceivingJustStarted = true;
    cc_sbm_status.sbm_status = SBM_IN_NORMAL_SPEED;

    InitCQueue(&a2dp_decoder_cc_bth_env.cachedPcmDataQ,
        sizeof(a2dp_decoder_cc_cached_pcm_frame_buf),
        (CQItemType *)a2dp_decoder_cc_cached_pcm_frame_buf);
    int_unlock(lock);
}

void a2dp_decoder_bth_stop_stream(void)
{
    a2dp_decoder_cc_cmd_cache_deinit();

    a2dp_decoder_cc_bth_env.offCoreIsReady = false;
    cc_sbm_status.sbm_status = SBM_IN_NORMAL_SPEED;
    memset(cc_sbm_status.sbm_param, 0, sizeof(int)*3);
    app_dsp_m55_bridge_send_cmd(
                     CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP, \
                     NULL, \
                     0);
    int res = a2dp_decoder_cc_sema_wait(A2DP_DECODER_CC_TIMEROUT_MS);
    if (res != osOK) {
        TRACE(2,"%s err = %d",__func__,res);
    }
    else
    {
        TRACE(0,"%s",__func__);
    }
    app_overlay_subsys_unloadall(APP_OVERLAY_M55);
}

void a2dp_decoder_bth_start_stream(
        A2DP_DECODER_CODEC_TYPE_E codecType,
        A2DP_DECODER_CHANNEL_SELECT_E chnlSel,
        A2DP_AUDIO_CC_OUTPUT_CONFIG_T *config,
        uint16_t maxseqnum)
{
    uint8_t chnlCount = 1;
    uint16_t tempBufSize = CC_DECODER_TEMP_BUFFER_SIZE;
    uint8_t additionalLen = 0;

    if ((A2DP_DECODER_CHANNEL_SELECT_STEREO == chnlSel) ||
        (A2DP_DECODER_CHANNEL_SELECT_LRMERGE == chnlSel)) {
        chnlCount = 2;
        if (codecType == A2DP_DECODER_CODEC_TYPE_LHDC)
        {
            chnlCount = 1;
        }
    }

    switch(codecType) {
        case A2DP_DECODER_CODEC_TYPE_SBC:
            app_overlay_subsys_select(APP_OVERLAY_M55, APP_OVERLAY_SUBSYS_SBC);
        break;
#ifdef A2DP_AAC_ON
        case A2DP_DECODER_CODEC_TYPE_MPEG2_4_AAC:
            app_overlay_subsys_select(APP_OVERLAY_M55, APP_OVERLAY_SUBSYS_AAC);
        break;
#endif
#ifdef A2DP_SCALABLE_ON
        case A2DP_DECODER_CODEC_TYPE_SCALABL:
        {
            app_overlay_subsys_select(APP_OVERLAY_M55, APP_OVERLAY_SUBSYS_SCALABLE);
            chnlCount = config->num_channels;
            if (config->sample_rate >= 48000) {
                tempBufSize = tempBufSize * 2;
                // Frame samples is dynamic for scalable
                // SBM Low speed:(888 - 864) * 4 * 2
                additionalLen = 192;
            }
        }
        break;
#endif

#ifdef A2DP_LHDC_ON
        case A2DP_DECODER_CODEC_TYPE_LHDC:
        {
            app_overlay_subsys_select(APP_OVERLAY_M55, APP_OVERLAY_SUBSYS_LHDC);
            if (config->sample_rate >= 48000) {
                tempBufSize = tempBufSize * 2;
                // Frame samples is dynamic for lhdc
                additionalLen = 0;  //Adaptive XIAOMI space audio
            }
        }
        break;
#endif

#ifdef A2DP_LDAC_ON
        case A2DP_DECODER_CODEC_TYPE_LDAC:
            app_overlay_subsys_select(APP_OVERLAY_M55, APP_OVERLAY_SUBSYS_LDAC);
        break;
#endif

        default:
        break;
    }

    a2dp_decoder_bth_reset();
    a2dp_decoder_bth_set_trigger_flag(false);
    CC_DECODE_LOG_I("%s, chnlCount:%d, additionalLen:%d, frame_samples %d", __func__,
        chnlCount, additionalLen, config->frame_samples);

    a2dp_decoder_cc_bth_env.codecType = codecType;
    a2dp_decoder_cc_bth_env.channel = chnlSel;
    a2dp_decoder_cc_bth_env.maxseqnum = maxseqnum;
    uint8_t bits_depth = bt_sbc_player_get_sample_bit();
    //bits_depth = 24;
    CC_DECODE_LOG_I("%s, chnlCount:%d, additionalLen:%d, depth:%d/%d", __func__, chnlCount, additionalLen, config->bits_depth, bits_depth);

    if(codecType == A2DP_DECODER_CODEC_TYPE_MPEG2_4_AAC) {
        a2dp_decoder_cc_bth_env.maxPCMSizePerFrame = 4096;
    } else if (codecType == A2DP_DECODER_CODEC_TYPE_LHDC) {
        a2dp_decoder_cc_bth_env.maxPCMSizePerFrame = chnlCount * config->frame_samples *
        ((((bits_depth + 15)/16)*16)/8) + additionalLen;
    } else {
        a2dp_decoder_cc_bth_env.maxPCMSizePerFrame = chnlCount * config->frame_samples *
        ((((config->bits_depth + 15)/16)*16)/8) + additionalLen;
    }

    A2DP_DECODER_INIT_REQ_T initReq;
    initReq.codecType = a2dp_decoder_cc_bth_env.codecType;
    initReq.channel = chnlSel;
    initReq.maxPcmDataSizePerFrame = a2dp_decoder_cc_bth_env.maxPCMSizePerFrame;
    memcpy((uint8_t *)&initReq.stream_cfg, (uint8_t *)config, sizeof(A2DP_AUDIO_CC_OUTPUT_CONFIG_T));
    initReq.stream_cfg.bits_depth = bits_depth;
    cc_off_core_retry_count = 0;
    ccLeftBytes = 0;
    ccLeftBufOffset = 0;
    app_audio_mempool_get_buff(&ccLeftBuff, tempBufSize);
    ASSERT(ccLeftBuff, "%s size:%d", __func__, tempBufSize);

#ifdef A2DP_LDAC_ON
    initReq.stream_cfg.channel_mode = bt_ldac_player_get_channelmode();
#endif

#ifdef A2DP_LHDC_ON
    if(codecType == A2DP_DECODER_CODEC_TYPE_LHDC) {
        // uint8_t* pLHDC_lic = (uint8_t*)__lhdc_license_start;
        // lhdcSetData_mcu(initReq.stream_cfg.lhdc_data, LHDC_DATA_LEN, pLHDC_lic, bes_bt_local_info_get, lhdc_data);
        initReq.stream_cfg.lhdc_ext_flags = bt_lhdc_player_get_ext_flags();
        CC_DECODE_LOG_D("lhdc_ext_flags = %d", initReq.stream_cfg.lhdc_ext_flags);
        CC_DECODE_LOG_D("[BRUCE DEBUG][M55] LIC first 8 byte To Send:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ", initReq.stream_cfg.lhdc_data[0], \
            initReq.stream_cfg.lhdc_data[1], \
            initReq.stream_cfg.lhdc_data[2], \
            initReq.stream_cfg.lhdc_data[3], \
            initReq.stream_cfg.lhdc_data[4], \
            initReq.stream_cfg.lhdc_data[5], \
            initReq.stream_cfg.lhdc_data[6], \
            initReq.stream_cfg.lhdc_data[7]
            );
    }
#endif
    CC_DECODE_LOG_D("%s, LHDC m55 send cmd, initReq.maxPcmDataSizePerFrame: %u", __func__, initReq.maxPcmDataSizePerFrame);
    memcpy(initReq.custom_data, cc_sbm_status.sbm_param, sizeof(int)*3);
    TRACE(0, "init param1 %d param2 %d, param3 %d", cc_sbm_status.sbm_param[0], cc_sbm_status.sbm_param[1], cc_sbm_status.sbm_param[2]);
    app_dsp_m55_bridge_send_cmd(
                     CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP, \
                     (uint8_t*)&initReq, \
                     sizeof(A2DP_DECODER_INIT_REQ_T));
}

#endif

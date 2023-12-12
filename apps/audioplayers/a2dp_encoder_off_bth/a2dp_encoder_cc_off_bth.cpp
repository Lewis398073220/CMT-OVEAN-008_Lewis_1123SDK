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
#include "hal_sysfreq.h"
#include "app_utils.h"
#include "a2dp_encoder_cc_common.h"
#include "a2dp_encoder_cc_off_bth.h"
#ifdef A2DP_ENCODER_CROSS_CORE_USE_M55
#include "app_dsp_m55.h"
#else
#include "rpc_bth_dsp.h"
#endif

extern const A2DP_AUDIO_CC_ENCODER_T* a2dp_encoder_lhdc_config();
extern const A2DP_AUDIO_CC_ENCODER_T* a2dp_encoder_aac_config();
extern const A2DP_AUDIO_CC_ENCODER_T* a2dp_encoder_ldac_config();

/************************private macro defination***************************/
#define A2DP_ENCODER_OFF_BTH_PROCESSOR_THREAD_STACK_SIZE            (1024 * 10)
#define A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_DATA_RECEIVED            (1 << 0)
#define A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_STOP                     (1 << 1)
#define A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_START                    (1 << 2)
#define A2DP_ENCODER_CC_PCM_MTU_LIMITER                             (32)

#define A2DP_ENCODER_CC_ENCODED_MTU_LIMITER                         (8)

#define A2DP_ENCODER_CC_MEM_POOL_SIZE                               ((A2DP_ENCODER_CC_ENCODED_MTU_LIMITER * 1756) + \
                                                                    (A2DP_ENCODER_CC_PCM_MTU_LIMITER * 7200) + \
                                                                    ((A2DP_ENCODER_CC_ENCODED_MTU_LIMITER + A2DP_ENCODER_CC_PCM_MTU_LIMITER) * \
                                                                    sizeof(a2dp_encoder_cc_media_data_t)))

/************************private type defination****************************/
typedef struct
{
    void        *mutex;
    list_t      *list;
    list_node_t *node;
} a2dp_encoder_cc_buff_list_t;

typedef struct
{
    a2dp_encoder_cc_buff_list_t     encoded_data_list;
    a2dp_encoder_cc_buff_list_t     pcm_data_list;

    A2DP_ENCODER_INIT_REQ_T         cachedInitReq;

    // const values once the a2dp is started
    bool                            isA2dpStreamingOnGoing;
    A2DP_ENCODER_CODEC_TYPE_E       codecType;
    uint16_t                        maxEncodeDataSizePerFrame;

    A2DP_AUDIO_CC_ENCODER_T         audioEncoder;

    uint8_t                         encodedLimiter;
} A2DP_ENCODER_CC_OFF_BTH_ENV_T;

/************************private variable defination************************/
static A2DP_ENCODER_CC_OFF_BTH_ENV_T a2dp_encoder_cc_off_bth_env;
static osThreadId a2dp_encoder_off_bth_processor_thread_id = NULL;
static uint8_t *a2dp_encoder_cc_mem_pool = NULL;
static heap_handle_t a2dp_encoder_cc_heap_handle = NULL;
static enum APP_SYSFREQ_FREQ_T a2dp_encoder_cc_base_freq = APP_SYSFREQ_30M;

/**********************private function declaration*************************/
static void a2dp_encoder_cc_process_encoding(a2dp_encoder_cc_media_data_t* pMediaPacket);
static void a2dp_encoder_cc_off_bth_stream_reset(void);
static void a2dp_encoder_cc_init_handler(void);
static void a2dp_encoder_cc_pcm_data_received_handler(uint8_t* param, uint16_t len);
static void a2dp_encoder_cc_ack_fed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_cc_feed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_cc_init_req_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_cc_ack_fed_encoded_data_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_encoder_cc_deinit_req_received_handler(uint8_t* ptr, uint16_t len);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET,
                            "FEED_PCM_DATA_PACKET",
                            NULL,
                            a2dp_encoder_cc_pcm_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET,
                            "ACK_FED_PCM_DATA_PACKET",
                            a2dp_encoder_cc_ack_fed_pcm_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET,
                            "FEED_ENCODED_DATA_PACKET",
                            a2dp_encoder_cc_feed_encoded_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET,
                            "ACK_FED_ENCODED_DATA_PACKET",
                            NULL,
                            a2dp_encoder_cc_ack_fed_encoded_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP,
                            "CC_A2DP_INIT_WAITING_RSP",
                            NULL,
                            a2dp_encoder_cc_init_req_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP,
                            "CC_A2DP_DEINIT_NO_RSP",
                            NULL,
                            a2dp_encoder_cc_deinit_req_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);


/****************************function defination****************************/
static void a2dp_encoder_cc_heap_init(void *begin_addr, uint32_t size)
{
    a2dp_encoder_cc_heap_handle = heap_register(begin_addr,size);
}

static void *a2dp_encoder_cc_heap_malloc(uint32_t size)
{
    size = ((size >> 2) + 1) << 2;
    void *ptr = heap_malloc(a2dp_encoder_cc_heap_handle, size);
    ASSERT(ptr, "%s size:%d", __func__, size);
#ifdef A2DP_ENCODER_CC_HEAP_DEBUG
    CC_ENCODE_LOG_I("ptr=%p size=%u user=%p left/mini left=%u/%u",
        ptr, size, __builtin_return_address(0),
        heap_free_size(a2dp_encoder_cc_heap_handle),
        heap_minimum_free_size(a2dp_encoder_cc_heap_handle));
#endif
    return ptr;
}

static void *a2dp_encoder_cc_heap_cmalloc(uint32_t size)
{
    size = ((size >> 2) + 1) << 2;
    void *ptr = heap_malloc(a2dp_encoder_cc_heap_handle, size);
    ASSERT(ptr, "%s size:%d", __func__, size);
#ifdef A2DP_ENCODER_CC_HEAP_DEBUG
    CC_ENCODE_LOG_I("ptr=%p size=%u user=%p left/mini left=%u/%u",
        ptr, size, __builtin_return_address(0),
        heap_free_size(a2dp_encoder_cc_heap_handle),
        heap_minimum_free_size(a2dp_encoder_cc_heap_handle));
#endif

    memset(ptr, 0, size);
    return ptr;
}

static void a2dp_encoder_cc_heap_free(void *rmem)
{
#ifdef A2DP_ENCODER_CC_HEAP_DEBUG
    CC_ENCODE_LOG_I("ptr=%p user=%p", rmem, __builtin_return_address(0));
#endif
    ASSERT(rmem, "%s rmem:%p", __func__, rmem);
    heap_free(a2dp_encoder_cc_heap_handle,rmem);
}

static void a2dp_encoder_cc_data_free(void *packet)
{
    ASSERT(packet, "%s packet = %p", __func__, packet);

    a2dp_encoder_cc_media_data_t *encoder_frame_p = (a2dp_encoder_cc_media_data_t *)packet;
    if (encoder_frame_p->data)
    {
        a2dp_encoder_cc_heap_free(encoder_frame_p->data);
    }
    a2dp_encoder_cc_heap_free(encoder_frame_p);
}

static a2dp_encoder_cc_media_data_t *a2dp_encoder_cc_data_frame_malloc(uint32_t packet_len)
{
    a2dp_encoder_cc_media_data_t *encoder_frame_p = NULL;
    uint8_t *pBuff = NULL;

    if (packet_len) {
        pBuff = (uint8_t *)a2dp_encoder_cc_heap_malloc(packet_len);
        ASSERT(pBuff != NULL, "%s buffer allocate failed,packet_len:%d", __func__, packet_len);
    }

    encoder_frame_p = (a2dp_encoder_cc_media_data_t *)a2dp_encoder_cc_heap_malloc(sizeof(a2dp_encoder_cc_media_data_t));
    ASSERT(encoder_frame_p != NULL, "%s encoder_frame allocate failed", __func__);

    encoder_frame_p->org_addr = (uint8_t *)encoder_frame_p;
    encoder_frame_p->data = pBuff;
    encoder_frame_p->data_len = packet_len;
    return encoder_frame_p;
}

static int inline a2dp_encoder_cc_buffer_mutex_lock(void *mutex)
{
    osMutexAcquire((osMutexId)mutex, osWaitForever);
    return 0;
}

static int inline a2dp_encoder_cc_buffer_mutex_unlock(void *mutex)
{
    osMutexRelease((osMutexId)mutex);
    return 0;
}

static list_node_t *a2dp_encoder_cc_list_begin(a2dp_encoder_cc_buff_list_t *list_info)
{
    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    list_node_t *node = list_begin(list_info->list);
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
    return node;
}

static uint32_t a2dp_encoder_cc_list_length(a2dp_encoder_cc_buff_list_t *list_info)
{
    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    uint32_t length = list_length(list_info->list);
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
    return length;
}

static void *a2dp_encoder_cc_list_node(a2dp_encoder_cc_buff_list_t *list_info)
{
    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    void *data = list_node(list_info->node);
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
    return data;
}

static bool a2dp_encoder_cc_list_remove(a2dp_encoder_cc_buff_list_t *list_info, void *data)
{
    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    bool nRet = list_remove(list_info->list, data);
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
    return nRet;
}

static bool a2dp_encoder_cc_list_append(a2dp_encoder_cc_buff_list_t *list_info, void *data)
{
    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    bool nRet = list_append(list_info->list, data);
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
    return nRet;
}

POSSIBLY_UNUSED static void a2dp_encoder_cc_list_free(a2dp_encoder_cc_buff_list_t *list_info)
{
    CC_ENCODE_LOG_I("free LIST 0x%x -> 0x%x", (uint32_t)list_info, (uint32_t)list_info->list);
    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    if (list_length(list_info->list) > 0)
    {
        list_free(list_info->list);
    }
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
}

static void a2dp_encoder_cc_list_new(a2dp_encoder_cc_buff_list_t *list_info, const osMutexAttr_t *mutex_attr,
    list_free_cb callback, list_mempool_zmalloc zmalloc, list_mempool_free free)
{
    if (NULL == list_info->mutex)
    {
        list_info->mutex = osMutexNew(mutex_attr);
    }
    ASSERT(list_info->mutex, "%s mutex create failed", __func__);

    a2dp_encoder_cc_buffer_mutex_lock(list_info->mutex);
    list_info->list = list_new(callback, zmalloc, free);
    a2dp_encoder_cc_buffer_mutex_unlock(list_info->mutex);
    CC_ENCODE_LOG_I("new LIST 0x%x -> 0x%x", (uint32_t)list_info, (uint32_t)list_info->list);
}

static void a2dp_encoder_cc_push_encode_run(void)
{
    // inform the processor thread
    osThreadFlagsSet(a2dp_encoder_off_bth_processor_thread_id, A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_DATA_RECEIVED);
}


static void a2dp_encoder_cc_process_encoded_data(void)
{
    a2dp_encoder_cc_buff_list_t *list = &a2dp_encoder_cc_off_bth_env.pcm_data_list;
    a2dp_encoder_cc_media_data_t* pcm_frame_p = NULL;

    do
    {
        list->node = a2dp_encoder_cc_list_begin(list);
        if (NULL == list->node)
        {
            CC_ENCODE_LOG_W("cached pcm list empty");
            break;
        }

        pcm_frame_p = (a2dp_encoder_cc_media_data_t *)a2dp_encoder_cc_list_node(list);

        a2dp_encoder_cc_process_encoding(pcm_frame_p);
        a2dp_encoder_cc_list_remove(list, pcm_frame_p);
    } while (0);

    a2dp_encoder_cc_push_encode_run();
}

static void a2dp_encoder_cc_ack_fed_encoded_data_to_bth(A2DP_ENCODER_ACK_FED_ENCODED_DATA_REQ_T* pReq)
{
   app_dsp_m55_bridge_send_cmd(
                    CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET, \
                    (uint8_t*)pReq, \
                    sizeof(A2DP_ENCODER_ACK_FED_ENCODED_DATA_REQ_T));
}

static void a2dp_encoder_cc_process_encoding(a2dp_encoder_cc_media_data_t* pMediaPacket)
{

    a2dp_encoder_cc_buff_list_t *pList = NULL;
    uint16_t lenPcmList = 0;

    pList = &a2dp_encoder_cc_off_bth_env.pcm_data_list;
    lenPcmList = a2dp_encoder_cc_list_length(pList);

    a2dp_encoder_cc_media_data_t* p_stored_encoded_data_info =
        a2dp_encoder_cc_data_frame_malloc(a2dp_encoder_cc_off_bth_env.maxEncodeDataSizePerFrame);
    p_stored_encoded_data_info->coorespondingPcmDataPacketPtr =
        pMediaPacket->coorespondingPcmDataPacketPtr;
    p_stored_encoded_data_info->pcm_list_len = lenPcmList;

    a2dp_encoder_cc_off_bth_env.audioEncoder.audio_encoder_cc_encode_frame(pMediaPacket, p_stored_encoded_data_info);
    if (!p_stored_encoded_data_info->data_len) {
        a2dp_encoder_cc_data_free(p_stored_encoded_data_info);
        return;
    }
    a2dp_encoder_cc_list_append(&a2dp_encoder_cc_off_bth_env.encoded_data_list,
        p_stored_encoded_data_info);

    // send encoded data notification to bth
    a2dp_encoder_cc_accumulate_cmd_send(
                    CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET,  \
                    (uint8_t*)p_stored_encoded_data_info);
}

static void a2dp_encoder_cc_pcm_data_received_handler(uint8_t* param, uint16_t len)
{
    uint16_t unitOfInfo = sizeof(a2dp_encoder_cc_media_data_t);

    if (!a2dp_encoder_cc_off_bth_env.isA2dpStreamingOnGoing)
    {
        CC_ENCODE_LOG_W("not ready to store packet");
        // send notification to bth
        // todo
        return;
    }

    uint32_t listLen = a2dp_encoder_cc_list_length(&a2dp_encoder_cc_off_bth_env.pcm_data_list);

    //CC_ENCODE_LOG_W("pcm list len:%d", listLen);
    if (listLen >= A2DP_ENCODER_CC_PCM_MTU_LIMITER)
    {
        CC_ENCODE_LOG_W("pcm list full:%d", listLen);
        return;
    }

    uint8_t packetCnt = len / unitOfInfo;

    while (packetCnt--) {
        a2dp_encoder_cc_media_data_t* p_src_data = (a2dp_encoder_cc_media_data_t *)param;

        a2dp_encoder_cc_media_data_t* p_stored_data =
            a2dp_encoder_cc_data_frame_malloc(p_src_data->data_len);
        p_stored_data->coorespondingPcmDataPacketPtr = (void *)p_src_data->org_addr;

        // copy to local buffer for decoding
        memcpy(p_stored_data->data, p_src_data->data, p_src_data->data_len);
        a2dp_encoder_cc_list_append(&a2dp_encoder_cc_off_bth_env.pcm_data_list, p_stored_data);
        param += unitOfInfo;
    }
    a2dp_encoder_cc_push_encode_run();
}

static void a2dp_encoder_cc_init_req_received_handler(uint8_t* ptr, uint16_t len)
{
    // Avoid timing issue
    a2dp_encoder_cc_off_bth_init();
    memcpy((uint8_t *)&(a2dp_encoder_cc_off_bth_env.cachedInitReq), ptr, sizeof(A2DP_ENCODER_INIT_REQ_T));
    osThreadFlagsSet(a2dp_encoder_off_bth_processor_thread_id, A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_START);
}

static void a2dp_encoder_cc_deinit_req_received_handler(uint8_t* ptr, uint16_t len)
{
    a2dp_encoder_cc_cmd_cache_deinit();
    // inform the processor thread
    osThreadFlagsSet(a2dp_encoder_off_bth_processor_thread_id, A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_STOP);
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP,
        NULL, 0);
}

static void a2dp_encoder_cc_ack_fed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET, ptr, len);
}

static void a2dp_encoder_cc_feed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET, ptr, len);
}

static void a2dp_encoder_cc_ack_fed_encoded_data_received_handler(uint8_t* ptr, uint16_t len)
{
    //
    uint16_t unitOfInfo = sizeof(a2dp_encoder_cc_media_data_t);

    if (!a2dp_encoder_cc_off_bth_env.isA2dpStreamingOnGoing)
    {
        CC_ENCODE_LOG_W("a2dp stream has already been stopped when get the pcm ack.");
        return;
    }

    CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
    uint8_t packetCnt = len / unitOfInfo;
    while (packetCnt--) {
        a2dp_encoder_cc_media_data_t* pMediaData = (a2dp_encoder_cc_media_data_t *)ptr;
        if(pMediaData->org_addr) {
            a2dp_encoder_cc_list_remove(&a2dp_encoder_cc_off_bth_env.encoded_data_list,
                (void *)(pMediaData->org_addr));
        }
        ptr += unitOfInfo;
    }
    a2dp_encoder_cc_push_encode_run();
}

static void a2dp_encoder_cc_off_bth_stream_reset(void)
{
    CC_ENCODE_LOG_I("%s", __func__);
}

static void a2dp_encoder_cc_init_handler(void)
{
    A2DP_ENCODER_INIT_REQ_T* pReq = &(a2dp_encoder_cc_off_bth_env.cachedInitReq);
    a2dp_encoder_cc_off_bth_env.codecType = pReq->codecType;
    a2dp_encoder_cc_off_bth_env.maxEncodeDataSizePerFrame = pReq->maxEncodeDataSizePerFrame;

    //a2dp_source_lhdcv5_enc_set_config(curr_device->aud_sample_rate, curr_device->base_device->sample_bit, bit_rate, curr_device->is_lossless_on);

    CC_ENCODE_LOG_I("%s", __func__);
    CC_ENCODE_LOG_I("codec type:0x%x, num_chnl:%d, sample_rate:%d, bits:%d, max_pcm:%d",
        pReq->codecType, pReq->stream_cfg.num_channels,
        pReq->stream_cfg.sample_rate, pReq->stream_cfg.bits_depth, pReq->maxEncodeDataSizePerFrame);

    a2dp_encoder_cc_off_bth_stream_reset();
    off_bth_syspool_get_buff(&a2dp_encoder_cc_mem_pool, A2DP_ENCODER_CC_MEM_POOL_SIZE);
    ASSERT(a2dp_encoder_cc_mem_pool, "%s size:%d", __func__, A2DP_ENCODER_CC_MEM_POOL_SIZE);
    a2dp_encoder_cc_heap_init(a2dp_encoder_cc_mem_pool, A2DP_ENCODER_CC_MEM_POOL_SIZE);
    osMutexAttr_t mutex_attr = {
        .name = "a2dp_encoder_cc_encoded_data_buffer_mutex",
        .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
        .cb_mem = NULL,
        .cb_size = 0U,
    };
    a2dp_encoder_cc_list_new(&a2dp_encoder_cc_off_bth_env.encoded_data_list,
                &mutex_attr,
                a2dp_encoder_cc_data_free,
                a2dp_encoder_cc_heap_cmalloc,
                a2dp_encoder_cc_heap_free);

    mutex_attr.name = "a2dp_encoder_cc_pcm_data_buffer_mutex";
    a2dp_encoder_cc_list_new(&a2dp_encoder_cc_off_bth_env.pcm_data_list,
                &mutex_attr,
                a2dp_encoder_cc_data_free,
                a2dp_encoder_cc_heap_cmalloc,
                a2dp_encoder_cc_heap_free);
    const A2DP_AUDIO_CC_ENCODER_T *p_audio_cc_encoder = NULL;

    a2dp_encoder_cc_off_bth_env.encodedLimiter = A2DP_ENCODER_CC_ENCODED_MTU_LIMITER;

    a2dp_encoder_cc_common_set_cmd_cached(false);

    switch (pReq->codecType)
    {
#ifdef A2DP_LHDC_ON
        case A2DP_ENCODER_CODEC_TYPE_LHDC:
            p_audio_cc_encoder = a2dp_encoder_lhdc_config();
            a2dp_encoder_cc_base_freq = APP_SYSFREQ_104M;
            break;
#endif
#ifdef A2DP_AAC_ON
        case A2DP_ENCODER_CODEC_TYPE_MPEG2_4_AAC:
            p_audio_cc_encoder = a2dp_encoder_aac_config();
            a2dp_encoder_cc_base_freq = APP_SYSFREQ_104M;
            break;
#endif

#ifdef A2DP_LDAC_ON
        case A2DP_ENCODER_CODEC_TYPE_LDAC:
            p_audio_cc_encoder = a2dp_encoder_ldac_config();
            a2dp_encoder_cc_base_freq = APP_SYSFREQ_104M;
            break;
#endif
        default:
            ASSERT(0, "%s, unknow codec type:%d", __func__, pReq->codecType);
            break;
    }

    //app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, a2dp_encoder_cc_base_freq);

    memcpy((uint8_t *)&p_audio_cc_encoder->stream_config , (uint8_t *)&pReq->stream_cfg, sizeof(A2DP_AUDIO_CC_OUTPUT_CONFIG_T));
    memcpy((uint8_t *)&a2dp_encoder_cc_off_bth_env.audioEncoder, (uint8_t *)p_audio_cc_encoder, sizeof(A2DP_AUDIO_CC_ENCODER_T));

    a2dp_encoder_cc_off_bth_env.audioEncoder.audio_encoder_cc_init();
    a2dp_encoder_cc_off_bth_env.isA2dpStreamingOnGoing = true;

    static A2DP_ENCODER_INIT_RSP_T rsp;
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP,
        (uint8_t *)&rsp, sizeof(rsp));
}

static void a2dp_encoder_cc_processor_thread(const void *arg)
{
    CC_ENCODE_LOG_I("%s", __func__);
    while(1)
    {
        osEvent evt;
        evt = osSignalWait(0x0, osWaitForever);
        // get role from signal value
        if (osEventSignal == evt.status) {
            if (evt.value.signals & A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_STOP)
            {
                CC_ENCODE_LOG_I("%s, encode deinit", __func__);
                a2dp_encoder_cc_off_bth_env.isA2dpStreamingOnGoing = false;
                a2dp_encoder_cc_off_bth_env.audioEncoder.audio_encoder_cc_deinit();
                a2dp_encoder_cc_off_bth_stream_reset();
                app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, APP_SYSFREQ_32K);
                off_bth_syspool_deinit(SYSPOOL_USER_A2DP);
            }

            if (evt.value.signals & A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_START)
            {
                off_bth_syspool_deinit(SYSPOOL_USER_A2DP);
                off_bth_syspool_init(SYSPOOL_USER_A2DP, NULL);
                a2dp_encoder_cc_init_handler();
            }

            if (evt.value.signals & A2DP_ENCODER_OFF_BTH_THREAD_SIGNAL_DATA_RECEIVED)
            {
                uint32_t lenEncodedList = 0;
                uint32_t lenDecodedList = 0;
                a2dp_encoder_cc_buff_list_t *pList = NULL;

                pList = &a2dp_encoder_cc_off_bth_env.encoded_data_list;
                lenEncodedList = a2dp_encoder_cc_list_length(pList);
                pList = &a2dp_encoder_cc_off_bth_env.pcm_data_list;
                lenDecodedList = a2dp_encoder_cc_list_length(pList);

                if ((lenDecodedList > 0) &&
                    (lenEncodedList < a2dp_encoder_cc_off_bth_env.encodedLimiter) &&
                    (a2dp_encoder_cc_off_bth_env.isA2dpStreamingOnGoing)) {
                    a2dp_encoder_cc_process_encoded_data();
                }
            }
        }
    }
}

void a2dp_encoder_cc_off_bth_init(void)
{
    if (NULL == a2dp_encoder_off_bth_processor_thread_id) {
        osThreadAttr_t threadattr_cc_processor;
        memset(&threadattr_cc_processor, 0, sizeof(osThreadAttr_t));
        threadattr_cc_processor.name = "a2dp_encoder_cc_processor_thread";
        threadattr_cc_processor.attr_bits = osThreadDetached;
        threadattr_cc_processor.stack_size = A2DP_ENCODER_OFF_BTH_PROCESSOR_THREAD_STACK_SIZE;
        threadattr_cc_processor.priority = osPriorityAboveNormal;
        a2dp_encoder_off_bth_processor_thread_id = osThreadNew((osThreadFunc_t)a2dp_encoder_cc_processor_thread,
                                                           NULL, &threadattr_cc_processor);
    }
}
#endif


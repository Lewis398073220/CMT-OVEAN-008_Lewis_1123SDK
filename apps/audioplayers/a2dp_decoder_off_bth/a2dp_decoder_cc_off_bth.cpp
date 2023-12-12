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
#include "hal_sysfreq.h"
#include "app_utils.h"
#include "a2dp_decoder_cc_common.h"
#include "a2dp_decoder_cc_off_bth.h"
#ifdef A2DP_DECODER_CROSS_CORE_USE_M55
#include "app_dsp_m55.h"
#else
#include "rpc_bth_dsp.h"
#endif
#ifdef A2DP_AAC_ON
#include "aac_api.h"
#endif

/*********************external variable declaration*************************/
extern A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_sbc_decoder_config;
#ifdef A2DP_AAC_ON
extern A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_aac_decoder_config;
#endif
#ifdef A2DP_SCALABLE_ON
extern A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_scalable_decoder_config;
#endif
#ifdef A2DP_LDAC_ON
extern A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_ldac_decoder_config;
#endif
#ifdef A2DP_LHDC_ON
extern A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_lhdc_decoder_config;
#ifdef A2DP_LHDCV5_ON
extern A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_lhdcv5_decoder_config;
extern "C"
{
    uint8_t lhdcv5_util_get_lhdcdata(void);
}
#endif
extern "C"
{
    uint8_t genLhdcData_dsp(void);
}
#endif

/************************private macro defination***************************/
#define IS_A2DP_DECODER_CC_OFF_BTH_LOG_ENABLEDx
#define A2DP_DECODER_CC_HEAP_DEBUGx
#define A2DP_DECODER_OFF_BTH_PROCESSOR_THREAD_STACK_SIZE            (1024 * 4)
#define A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_DATA_RECEIVED            (1 << 0)
#define A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_STOP                     (1 << 1)
#define A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_START                    (1 << 2)
#define A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_REMOVE_SPECIFIC_FRAME    (1 << 3)
#define A2DP_DECODER_CC_ENCODED_MTU_LIMITER                         (50) //#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (58)
// when the pcm entry list has up to this number, pause the decoding until
// the count of the pcm is cosumed down.
// This can decrease the ram cost
#define A2DP_DECODER_CC_PCM_MTU_LIMITER                             (30)
// shall increase this value if memory pool is not enough for some codec
// for aac, it's A2DP_MTU_LIMITER times one pcm frame+one a2dp frame
// Caculatation formula:
// A2DP_MTU_LIMITER * length of one AVDTP packet +
// MAXIMUM_CACHED_PCM_ENTRY_CNT 8 chnl_count * frame_samples * bits_depth +
// (A2DP_DECODER_CC_MTU_LIMITER + A2DP_DECODER_CC_MAXIMUM_PCM_ENTRY_CNT) * lengh of header
#define A2DP_DECODER_CC_MEM_POOL_SIZE                               ((A2DP_DECODER_CC_ENCODED_MTU_LIMITER * 1024) + \
                                                                    (A2DP_DECODER_CC_PCM_MTU_LIMITER * 7700) + \
                                                                    ((A2DP_DECODER_CC_ENCODED_MTU_LIMITER + A2DP_DECODER_CC_PCM_MTU_LIMITER) * \
                                                                    sizeof(a2dp_decoder_cc_media_data_t)))
#define MAXIMUM_PENDING_REMOVED_ENCODED_PACKET_CNT                  (A2DP_DECODER_CC_ENCODED_MTU_LIMITER*4)

#define BOOST_FREQ_CNT                                              (16)

#define DECODED_DATA_LIST_LENTH_CHECK_THR                           (4)
#define ENCODED_DATA_LIST_LENTH_CHECK_THR                           (10)
/************************private type defination****************************/
typedef struct
{
    void        *mutex;
    list_t      *list;
    list_node_t *node;
} a2dp_decoder_cc_buff_list_t;

typedef struct
{
    a2dp_decoder_cc_buff_list_t     encoded_data_list;
    a2dp_decoder_cc_buff_list_t     decoded_data_list;

    A2DP_DECODER_INIT_REQ_T         cachedInitReq;

    CQueue                          toBeRemovedEncodedPacketQueue;

    // const values once the a2dp is started
    bool                            isA2dpStreamingOnGoing;
    A2DP_DECODER_CODEC_TYPE_E       codecType;
    A2DP_DECODER_CHANNEL_SELECT_E   channel;
    uint16_t                        totalSubSeqNum;
    uint16_t                        maxPcmDataSizePerFrame;

    A2DP_AUDIO_CC_DECODER_T         audioDecoder;

    uint16_t                        boostFreqCnt;
    bool                            boostByBth;
    uint8_t                         pcmLimiter;
    uint8_t                         encodedThreshold;
    uint8_t                         decodedThreshold;
} A2DP_DECODER_CC_OFF_BTH_ENV_T;

/************************private variable defination************************/
static A2DP_DECODER_CC_OFF_BTH_ENV_T a2dp_decoder_cc_off_bth_env __attribute__((aligned(4)));
static osThreadId a2dp_decoder_off_bth_processor_thread_id = NULL;
static uint8_t *a2dp_decoder_cc_mem_pool = NULL;
static heap_handle_t a2dp_decoder_cc_heap_handle = NULL;
static A2DP_DECODER_CC_FLUSH_T a2dp_decoder_cached_to_be_removed_frame_ptr_queue_buf[MAXIMUM_PENDING_REMOVED_ENCODED_PACKET_CNT];
static enum APP_SYSFREQ_FREQ_T a2dp_decoder_cc_base_freq = APP_SYSFREQ_30M;

/**********************private function declaration*************************/
static void a2dp_decoder_cc_process_decoding(a2dp_decoder_cc_media_data_t* pMediaPacket);
static void a2dp_decoder_cc_off_bth_stream_reset(void);
static void a2dp_decoder_cc_init_handler(void);
static void a2dp_decoder_cc_rm_specific_frame_handler(void);
static void a2dp_decoder_cc_rm_specific_frame_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_encoded_data_received_handler(uint8_t* param, uint16_t len);
static void a2dp_decoder_cc_ack_fed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_feed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_retrigger_req_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_init_req_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_ack_fed_pcm_data_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_boost_freq_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_boost_done_transmit_handler(uint8_t* ptr, uint16_t len);
#if defined(A2DP_SCALABLE_ON)
static void a2dp_decoder_cc_sbm_to_off_bth_received_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_sbm_to_bth_transmit_handler(uint8_t* ptr, uint16_t len);
static void a2dp_decoder_cc_sbm_status_transmit_handler(uint8_t* ptr, uint16_t len);
#endif
static void a2dp_decoder_cc_deinit_req_received_handler(uint8_t* ptr, uint16_t len);
#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
static void a2dp_decoder_cc_feed_data_transmit_handler(uint8_t* ptr, uint16_t len);
#endif
static void a2dp_decoder_cc_set_triggered_state_handler(uint8_t* ptr, uint16_t len);

static void a2dp_decoder_cc_processor_thread(const void *arg);
osThreadDef(a2dp_decoder_cc_processor_thread, osPriorityAboveNormal, 1,
    (A2DP_DECODER_OFF_BTH_PROCESSOR_THREAD_STACK_SIZE), "a2dp_decoder_cc_processor_thread");
osMutexDef(a2dp_decoder_cc_encoded_data_buffer_mutex);
osMutexDef(a2dp_decoder_cc_pcm_data_buffer_mutex);


RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME,
                            "A2DP_REMOVE_SPECIFIC_FRAME",
                            NULL,
                            a2dp_decoder_cc_rm_specific_frame_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET,
                            "FEED_ENCODED_DATA_PACKET",
                            NULL,
                            a2dp_decoder_cc_encoded_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET,
                            "ACK_FED_ENCODED_DATA_PACKET",
                            a2dp_decoder_cc_ack_fed_encoded_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET,
                            "FEED_PCM_DATA_PACKET",
                            a2dp_decoder_cc_feed_pcm_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET,
                            "ACK_FED_PCM_DATA_PACKET",
                            NULL,
                            a2dp_decoder_cc_ack_fed_pcm_data_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_RETRIGGER_REQ_NO_RSP,
                            "CC_A2DP_RETRIGGER_REQ_NO_RSP",
                            a2dp_decoder_cc_retrigger_req_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP,
                            "CC_A2DP_INIT_WAITING_RSP",
                            NULL,
                            a2dp_decoder_cc_init_req_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_REQ,
                            "CC_A2DP_BOOST_REQ",
                            NULL,
                            a2dp_decoder_cc_boost_freq_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_DONE,
                            "CC_A2DP_BOOST_DONE",
                            a2dp_decoder_cc_boost_done_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

#if defined(A2DP_SCALABLE_ON)
RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_OFF_BTH,
                            "CC_SBM_TO_OFF_BTH",
                            NULL,
                            a2dp_decoder_cc_sbm_to_off_bth_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_BTH,
                            "CC_SBM_TO_BTH",
                            a2dp_decoder_cc_sbm_to_bth_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_STATUS,
                            "CC_SBM_STATUS",
                            a2dp_decoder_cc_sbm_status_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);
#endif

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP,
                            "CC_A2DP_DEINIT_NO_RSP",
                            NULL,
                            a2dp_decoder_cc_deinit_req_received_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_LHDC_FED_DATA_PACKET,
                            "FEED_PCM_DATA_PACKET",
                            a2dp_decoder_cc_feed_data_transmit_handler,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL);
#endif

RPC_CROSS_CORE_TASK_CMD_TO_ADD(CROSS_CORE_TASK_CMD_A2DP_SET_TRIGGERED_STATE,
                            "CC_A2DP_SET_TRIGGERED_STATE",
                            NULL,
                            a2dp_decoder_cc_set_triggered_state_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);

extern void audio_audio_cc_scalable_set_sbm_param(int* custom_data);

#ifdef A2DP_LDAC_ON
uint32_t a2dp_decoder_cc_get_sample_rate(void)
{
    return a2dp_decoder_cc_off_bth_env.cachedInitReq.stream_cfg.sample_rate;

}


uint32_t a2dp_decoder_cc_get_channnel_mode(void)
{

    return a2dp_decoder_cc_off_bth_env.cachedInitReq.stream_cfg.channel_mode;

}

#endif

#if defined(A2DP_LHDC_ON) || defined(A2DP_LHDCV5_ON)

uint32_t a2dp_decoder_cc_get_sample_rate(void)
{
    return a2dp_decoder_cc_off_bth_env.cachedInitReq.stream_cfg.sample_rate;
}

uint8_t a2dp_decoder_cc_get_bits_depth(void)
{
    return a2dp_decoder_cc_off_bth_env.cachedInitReq.stream_cfg.bits_depth;
}

uint8_t a2dp_decoder_cc_get_lhdc_ext_flag(void)
{
    return a2dp_decoder_cc_off_bth_env.cachedInitReq.stream_cfg.lhdc_ext_flags;
}

uint8_t *a2dp_decoder_cc_get_lhdc_data(void)
{
    return a2dp_decoder_cc_off_bth_env.cachedInitReq.stream_cfg.lhdc_data;
}

uint8_t a2dp_decoder_cc_get_channel_sel(void)
{
    return a2dp_decoder_cc_off_bth_env.cachedInitReq.channel;
}
#endif

/****************************function defination****************************/
static void a2dp_decoder_cc_heap_init(void *begin_addr, uint32_t size)
{
    a2dp_decoder_cc_heap_handle = heap_register(begin_addr,size);
}

static void *a2dp_decoder_cc_heap_malloc(uint32_t size)
{
    uint32_t lock = int_lock();

    size = ((size >> 2) + 1) << 2;
    void *ptr = heap_malloc(a2dp_decoder_cc_heap_handle, size);
    ASSERT(ptr, "%s size:%d", __func__, size);
#ifdef A2DP_DECODER_CC_HEAP_DEBUG
    CC_DECODE_LOG_I("ptr=%p size=%u user=%p left/mini left=%u/%u",
        ptr, size, __builtin_return_address(0),
        heap_free_size(a2dp_decoder_cc_heap_handle),
        heap_minimum_free_size(a2dp_decoder_cc_heap_handle));
#endif

    int_unlock(lock);
    return ptr;
}

static void *a2dp_decoder_cc_heap_cmalloc(uint32_t size)
{
    uint32_t lock = int_lock();

    size = ((size >> 2) + 1) << 2;
    void *ptr = heap_malloc(a2dp_decoder_cc_heap_handle, size);
    ASSERT(ptr, "%s size:%d", __func__, size);
#ifdef A2DP_DECODER_CC_HEAP_DEBUG
    CC_DECODE_LOG_I("ptr=%p size=%u user=%p left/mini left=%u/%u",
        ptr, size, __builtin_return_address(0),
        heap_free_size(a2dp_decoder_cc_heap_handle),
        heap_minimum_free_size(a2dp_decoder_cc_heap_handle));
#endif

    memset(ptr, 0, size);
    int_unlock(lock);
    return ptr;
}

static void a2dp_decoder_cc_heap_free(void *rmem)
{
#ifdef A2DP_DECODER_CC_HEAP_DEBUG
    CC_DECODE_LOG_I("ptr=%p user=%p", rmem, __builtin_return_address(0));
#endif
    ASSERT(rmem, "%s rmem:%p", __func__, rmem);

    uint32_t lock = int_lock();
    heap_free(a2dp_decoder_cc_heap_handle, rmem);
    int_unlock(lock);
}

static void a2dp_decoder_cc_data_free(void *packet)
{
    ASSERT(packet, "%s packet = %p", __func__, packet);

    a2dp_decoder_cc_media_data_t *decoder_frame_p = (a2dp_decoder_cc_media_data_t *)packet;
    if (decoder_frame_p->data)
    {
        a2dp_decoder_cc_heap_free(decoder_frame_p->data);
    }
    a2dp_decoder_cc_heap_free(decoder_frame_p);
}

static a2dp_decoder_cc_media_data_t *a2dp_decoder_cc_data_frame_malloc(uint32_t packet_len)
{
    a2dp_decoder_cc_media_data_t *decoder_frame_p = NULL;
    uint8_t *pBuff = NULL;

    if (packet_len) {
        pBuff = (uint8_t *)a2dp_decoder_cc_heap_malloc(packet_len);
        ASSERT(pBuff != NULL, "%s buffer allocate failed,packet_len:%d", __func__, packet_len);
    }

    decoder_frame_p = (a2dp_decoder_cc_media_data_t *)a2dp_decoder_cc_heap_malloc(sizeof(a2dp_decoder_cc_media_data_t));
    ASSERT(decoder_frame_p != NULL, "%s decoder_frame allocate failed", __func__);

    decoder_frame_p->org_addr = (uint8_t *)decoder_frame_p;
    decoder_frame_p->data = pBuff;
    decoder_frame_p->data_len = packet_len;
    return decoder_frame_p;
}

static int inline a2dp_decoder_cc_buffer_mutex_lock(void *mutex)
{
    osMutexWait((osMutexId)mutex, osWaitForever);
    return 0;
}

static int inline a2dp_decoder_cc_buffer_mutex_unlock(void *mutex)
{
    osMutexRelease((osMutexId)mutex);
    return 0;
}

static list_node_t *a2dp_decoder_cc_list_begin(a2dp_decoder_cc_buff_list_t *list_info)
{
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    list_node_t *node = list_begin(list_info->list);
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
    return node;
}

static uint32_t a2dp_decoder_cc_list_length(a2dp_decoder_cc_buff_list_t *list_info)
{
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    uint32_t length = list_length(list_info->list);
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
    return length;
}

static void *a2dp_decoder_cc_list_node(a2dp_decoder_cc_buff_list_t *list_info)
{
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    void *data = list_node(list_info->node);
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
    return data;
}

static bool a2dp_decoder_cc_list_remove(a2dp_decoder_cc_buff_list_t *list_info, void *data)
{
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    bool nRet = list_remove(list_info->list, data);
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
    return nRet;
}

static bool a2dp_decoder_cc_list_append(a2dp_decoder_cc_buff_list_t *list_info, void *data)
{
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    bool nRet = list_append(list_info->list, data);
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
    return nRet;
}

POSSIBLY_UNUSED static void a2dp_decoder_cc_list_free(a2dp_decoder_cc_buff_list_t *list_info)
{
    CC_DECODE_LOG_I("free LIST 0x%x -> 0x%x", (uint32_t)list_info, (uint32_t)list_info->list);
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    if (list_length(list_info->list) > 0)
    {
        list_free(list_info->list);
    }
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
}

static void a2dp_decoder_cc_list_new(a2dp_decoder_cc_buff_list_t *list_info, const osMutexDef_t *mutex_def,
    list_free_cb callback, list_mempool_zmalloc zmalloc, list_mempool_free free)
{
    if (NULL == list_info->mutex)
    {
        list_info->mutex = osMutexCreate(mutex_def);
    }
    ASSERT(list_info->mutex, "%s mutex create failed", __func__);

    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
    list_info->list = list_new(callback, zmalloc, free);
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
    CC_DECODE_LOG_I("new LIST 0x%x -> 0x%x", (uint32_t)list_info, (uint32_t)list_info->list);
}

static void a2dp_decoder_cc_push_decode_run(void)
{
    // inform the processor thread
    osSignalSet(a2dp_decoder_off_bth_processor_thread_id, A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_DATA_RECEIVED);
}

static void a2dp_decoder_cc_adjust_freq_dynamic(enum APP_SYSFREQ_FREQ_T freq)
{
    if (a2dp_decoder_cc_off_bth_env.boostFreqCnt) {
        --a2dp_decoder_cc_off_bth_env.boostFreqCnt;
        if (!a2dp_decoder_cc_off_bth_env.boostFreqCnt) {
            if (a2dp_decoder_cc_off_bth_env.boostByBth) {
                a2dp_decoder_cc_off_bth_env.boostByBth = false;
                app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_DONE, NULL, 0);
            }
            app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, a2dp_decoder_cc_base_freq);
        }
        return;
    }

    if ((A2DP_DECODER_CODEC_TYPE_SBC == a2dp_decoder_cc_off_bth_env.codecType) ||
        (48000 <= a2dp_decoder_cc_off_bth_env.audioDecoder.stream_config.sample_rate) ||
        (A2DP_DECODER_CODEC_TYPE_LHDC == a2dp_decoder_cc_off_bth_env.codecType)) {
        return;
    }

    app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, freq);
}

static void a2dp_decoder_cc_process_encoded_data(void)
{
    a2dp_decoder_cc_buff_list_t *list = &a2dp_decoder_cc_off_bth_env.encoded_data_list;
    a2dp_decoder_cc_media_data_t* decoded_frame_p = NULL;

    do
    {
        list->node = a2dp_decoder_cc_list_begin(list);
        if (NULL == list->node)
        {
            CC_DECODE_LOG_W("cached encoded list empty");
            break;
        }

        decoded_frame_p = (a2dp_decoder_cc_media_data_t *)a2dp_decoder_cc_list_node(list);

        //CC_DECODE_LOG_I("process_decoded_pkt, len:%u seq:%u subseq:%u", decoded_frame_p->data_len, decoded_frame_p->pkt_seq_nb, decoded_frame_p->subSequenceNumber);

        a2dp_decoder_cc_process_decoding(decoded_frame_p);
        a2dp_decoder_cc_list_remove(list, decoded_frame_p);
    } while (0);

    a2dp_decoder_cc_push_decode_run();
}

static void a2dp_decoder_cc_ack_fed_encoded_data_to_bth(A2DP_DECODER_ACK_FED_ENCODED_DATA_REQ_T* pReq)
{
   app_dsp_m55_bridge_send_cmd(
                    CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET, \
                    (uint8_t*)pReq, \
                    sizeof(A2DP_DECODER_ACK_FED_ENCODED_DATA_REQ_T));
}

static void a2dp_decoder_cc_process_decoding(a2dp_decoder_cc_media_data_t* pMediaPacket)
{

    a2dp_decoder_cc_buff_list_t *pList = NULL;
    uint16_t lenEncodedList = 0;

    pList = &a2dp_decoder_cc_off_bth_env.encoded_data_list;
    lenEncodedList = a2dp_decoder_cc_list_length(pList);

    a2dp_decoder_cc_media_data_t* p_stored_pcm_data_info =
        a2dp_decoder_cc_data_frame_malloc(a2dp_decoder_cc_off_bth_env.maxPcmDataSizePerFrame);
    p_stored_pcm_data_info->isPLC = pMediaPacket->isPLC;
    p_stored_pcm_data_info->pkt_seq_nb = pMediaPacket->pkt_seq_nb;
    p_stored_pcm_data_info->totalSubSequenceNumber = pMediaPacket->totalSubSequenceNumber;
    p_stored_pcm_data_info->subSequenceNumber = pMediaPacket->subSequenceNumber;
    p_stored_pcm_data_info->time_stamp = pMediaPacket->time_stamp;
    p_stored_pcm_data_info->coorespondingEncodedDataPacketPtr =
        pMediaPacket->coorespondingEncodedDataPacketPtr;
    p_stored_pcm_data_info->encoded_list_len = lenEncodedList;

    a2dp_decoder_cc_off_bth_env.audioDecoder.audio_decoder_cc_decode_frame(pMediaPacket, p_stored_pcm_data_info);

    a2dp_decoder_cc_list_append(&a2dp_decoder_cc_off_bth_env.decoded_data_list,
        p_stored_pcm_data_info);

    // send pcm data notification to bth
#ifdef IS_A2DP_DECODER_CC_OFF_BTH_LOG_ENABLED
    CC_DECODE_LOG_I("feed pcm into bth, seq:%d-%d", p_stored_pcm_data_info->pkt_seq_nb, p_stored_pcm_data_info->subSequenceNumber);
#endif

    a2dp_decoder_cc_accumulate_cmd_send(
                CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET,  \
                (uint8_t*)p_stored_pcm_data_info);
}

static void a2dp_decoder_cc_encoded_data_received_handler(uint8_t* param, uint16_t len)
{
    uint16_t unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);

    if (!a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing)
    {
        CC_DECODE_LOG_I("not ready to store packet");
        // send notification to bth
        A2DP_DECODER_ACK_FED_ENCODED_DATA_REQ_T ackReq;
        memcpy((uint8_t *)&(ackReq.ackedDataPacket), param, unitOfInfo);
        ackReq.encodedPacketEntryCount = 0;
        ackReq.pcmPacketEntryCount = 0;
        a2dp_decoder_cc_ack_fed_encoded_data_to_bth(&ackReq);
        return;
    }

    uint32_t listLen = a2dp_decoder_cc_list_length(&a2dp_decoder_cc_off_bth_env.encoded_data_list);

    if (listLen >= A2DP_DECODER_CC_ENCODED_MTU_LIMITER)
    {
        CC_DECODE_LOG_THROTTLED(W, 0, CC_DECODE_LOG_EVERY_1_S,
            "encoded list full:%d", listLen);
    }

    uint8_t packetCnt = len / unitOfInfo;

    while (packetCnt--) {
        a2dp_decoder_cc_media_data_t* p_src_data = (a2dp_decoder_cc_media_data_t *)param;
        //invalid packet length for seq error case
        if (!p_src_data->data_len) {
            p_src_data->isPLC = true;
        }

        a2dp_decoder_cc_media_data_t* p_stored_data =
            a2dp_decoder_cc_data_frame_malloc(p_src_data->data_len);
        p_stored_data->isPLC = p_src_data->isPLC;
        p_stored_data->pkt_seq_nb = p_src_data->pkt_seq_nb;
        p_stored_data->totalSubSequenceNumber = p_src_data->totalSubSequenceNumber;
        p_stored_data->subSequenceNumber = p_src_data->subSequenceNumber;
        p_stored_data->time_stamp = p_src_data->time_stamp;
        p_stored_data->coorespondingEncodedDataPacketPtr = (void *)p_src_data->org_addr;

        CC_DECODE_LOG_D("m55 rsv pkt, seq:%d-%d", p_stored_data->pkt_seq_nb, p_stored_data->subSequenceNumber);
        // copy to local buffer for decoding
        memcpy(p_stored_data->data, p_src_data->data, p_src_data->data_len);
        //CC_DECODE_LOG_I("p_src_data->data_len = %d", p_src_data->data_len);
        a2dp_decoder_cc_list_append(&a2dp_decoder_cc_off_bth_env.encoded_data_list, p_stored_data);
        param += unitOfInfo;
    }

    // send notification to bth
    // TODO: decrease core bridge command communication
    /*
    A2DP_DECODER_ACK_FED_ENCODED_DATA_REQ_T ackReq;
    memcpy((uint8_t *)&(ackReq.ackedDataPacket), (uint8_t *)p_src_media_data_info, sizeof(a2dp_decoder_cc_media_data_t));
    ackReq.encodedPacketEntryCount = a2dp_decoder_cc_list_length(&a2dp_decoder_cc_off_bth_env.encoded_data_list);
    ackReq.pcmPacketEntryCount = a2dp_decoder_cc_list_length(&a2dp_decoder_cc_off_bth_env.decoded_data_list);
    a2dp_decoder_ack_fed_encoded_data_packet_to_bth(&ackReq);
    */
    a2dp_decoder_cc_push_decode_run();
}

static void a2dp_decoder_cc_init_req_received_handler(uint8_t* ptr, uint16_t len)
{
    // Avoid timing issue
    a2dp_decoder_cc_off_bth_init();
    memcpy((uint8_t *)&(a2dp_decoder_cc_off_bth_env.cachedInitReq), ptr, sizeof(A2DP_DECODER_INIT_REQ_T));
    osSignalSet(a2dp_decoder_off_bth_processor_thread_id, A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_START);
}

static void a2dp_decoder_cc_deinit_req_received_handler(uint8_t* ptr, uint16_t len)
{   
    a2dp_decoder_cc_cmd_cache_deinit();
    // inform the processor thread
    osSignalSet(a2dp_decoder_off_bth_processor_thread_id, A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_STOP);
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP,
        NULL, 0);
}

static void a2dp_decoder_cc_ack_fed_encoded_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET, ptr, len);
}

static void a2dp_decoder_cc_feed_pcm_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET, ptr, len);
}

static void a2dp_decoder_cc_ack_fed_pcm_data_received_handler(uint8_t* ptr, uint16_t len)
{
    uint16_t unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);

    if (!a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing)
    {
        CC_DECODE_LOG_I("a2dp stream has already been stopped when get the pcm ack.");
        return;
    }

    uint8_t packetCnt = len / unitOfInfo;
    while (packetCnt--) {
        a2dp_decoder_cc_media_data_t* pMediaData = (a2dp_decoder_cc_media_data_t *)ptr;

#ifdef IS_A2DP_DECODER_CC_OFF_BTH_CC_DECODE_LOG_ENABLED
        CC_DECODE_LOG_I("rmv pcm seq %u", ((a2dp_decoder_cc_media_data_t *)(pMediaData->org_addr))->pkt_seq_nb);
#endif
        if (pMediaData->pkt_seq_nb == ((a2dp_decoder_cc_media_data_t *)(pMediaData->org_addr))->pkt_seq_nb)
        {
            a2dp_decoder_cc_list_remove(&a2dp_decoder_cc_off_bth_env.decoded_data_list,
                    (void *)(pMediaData->org_addr));
        }
        ptr += unitOfInfo;
    }
    a2dp_decoder_cc_push_decode_run();
}

static void a2dp_decoder_cc_boost_freq_received_handler(uint8_t* ptr, uint16_t len)
{
    CC_DECODE_LOG_I("%s", __func__);
    a2dp_decoder_cc_off_bth_env.boostFreqCnt = 2 * BOOST_FREQ_CNT;
    a2dp_decoder_cc_off_bth_env.boostByBth = true;

    app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, APP_SYSFREQ_104M);
}

static void a2dp_decoder_cc_retrigger_req_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_RETRIGGER_REQ_NO_RSP, ptr, len);
}

static void a2dp_decoder_cc_boost_done_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_DONE, ptr, len);
}

#if defined(A2DP_SCALABLE_ON)
static void a2dp_decoder_cc_sbm_to_off_bth_received_handler(uint8_t* ptr, uint16_t len)
{
    if (a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing) {
        a2dp_decoder_cc_off_bth_env.audioDecoder.audio_decoder_cc_push_sync_custom_data(ptr);
    }
}

static void a2dp_decoder_cc_sbm_to_bth_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_BTH, ptr, len);
}

static void a2dp_decoder_cc_sbm_status_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_STATUS, ptr, len);
}
#endif

#if defined(A2DP_LHDC_ON) || defined(A2DP_LHDCV5_ON)//LHDC_DSP_LIC_CHECK
static void a2dp_decoder_cc_feed_data_transmit_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(CROSS_CORE_TASK_CMD_LHDC_FED_DATA_PACKET, ptr, len);
}
#endif

static void a2dp_decoder_cc_set_triggered_state_handler(uint8_t* ptr, uint16_t len)
{
    bool isTriggered = *(uint8_t *)ptr;
    a2dp_decoder_cc_common_set_cmd_cached(isTriggered);
}

static void a2dp_decoder_cc_rm_node_with_specific_a2dp_packet(
    a2dp_decoder_cc_buff_list_t *list_info,
    void* pFlushedFrame, uint16_t seq, uint16_t sub_seq, uint16_t total_sub_seq)
{
    list_node_t *node = NULL;
    a2dp_decoder_cc_media_data_t* pListEntry = NULL;

    // should be able to find on the firs entry
    // TODO: may change to use sequence number to locate the removed entry
    // instead of the logic address
    a2dp_decoder_cc_buffer_mutex_lock(list_info->mutex);
peek_again:
    node = list_begin(list_info->list);

    if (node != list_end(list_info->list))
    {
        pListEntry = (a2dp_decoder_cc_media_data_t *)list_node(node);
        if (!pListEntry) {
            goto exit;
        }
        CC_DECODE_LOG_I("RemovepListEntry,%p-%p, seq:%u/%u/%u-%u/%u/%u",
            pListEntry->coorespondingEncodedDataPacketPtr, pFlushedFrame,
            pListEntry->pkt_seq_nb, pListEntry->subSequenceNumber, pListEntry->totalSubSequenceNumber,
            seq, sub_seq, total_sub_seq);

        if (pListEntry->totalSubSequenceNumber == 0)
        {
            if ((pListEntry->pkt_seq_nb < seq) || (ABS((int32_t)pListEntry->pkt_seq_nb - (int32_t)seq) > 32)) {
                list_remove(list_info->list, list_node(node));
                goto peek_again;
            }

            if ((pListEntry->coorespondingEncodedDataPacketPtr == pFlushedFrame) &&
                (pListEntry->pkt_seq_nb == seq)) {
                list_remove(list_info->list, list_node(node));
            }
        }
        else
        {
            if ((pListEntry->pkt_seq_nb < seq) || ((pListEntry->pkt_seq_nb == seq)&&(pListEntry->subSequenceNumber < sub_seq)) ||
                    (ABS((int32_t)pListEntry->pkt_seq_nb - (int32_t)seq) > 32)) {
                list_remove(list_info->list, list_node(node));
                goto peek_again;
            }

            if ((pListEntry->coorespondingEncodedDataPacketPtr == pFlushedFrame) &&
                (pListEntry->pkt_seq_nb == seq) && (pListEntry->subSequenceNumber == sub_seq)) {
                list_remove(list_info->list, list_node(node));
            }
        }
    }
exit:
    a2dp_decoder_cc_buffer_mutex_unlock(list_info->mutex);
}

static void a2dp_decoder_cc_rm_specific_frame_handler(void)
{
    A2DP_DECODER_CC_FLUSH_T pFlushedData;

    do
    {
        uint32_t lock = int_lock();
        int ret = DeCQueue(&a2dp_decoder_cc_off_bth_env.toBeRemovedEncodedPacketQueue,
            (uint8_t *)&pFlushedData, sizeof(A2DP_DECODER_CC_FLUSH_T));
        int_unlock(lock);
        if (CQ_OK == ret)
        {
            a2dp_decoder_cc_rm_node_with_specific_a2dp_packet(
                &a2dp_decoder_cc_off_bth_env.decoded_data_list,
                pFlushedData.flushFrame, pFlushedData.seqNum, pFlushedData.subSeq, pFlushedData.totalSubSeq);

             a2dp_decoder_cc_rm_node_with_specific_a2dp_packet(
                 &a2dp_decoder_cc_off_bth_env.encoded_data_list,
                 pFlushedData.flushFrame, pFlushedData.seqNum, pFlushedData.subSeq, pFlushedData.totalSubSeq);
        }
        else
        {
            break;
        }
    } while (1);

    a2dp_decoder_cc_push_decode_run();
}

static void a2dp_decoder_cc_rm_specific_frame_received_handler(uint8_t* ptr, uint16_t len)
{
    if (!a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing)
    {
        CC_DECODE_LOG_I("a2dp stream has already been stopped");
        return;
    }

    uint32_t lock = int_lock();
    int ret = EnCQueue(&a2dp_decoder_cc_off_bth_env.toBeRemovedEncodedPacketQueue, (uint8_t *)ptr, len);
    int_unlock(lock);

    ASSERT(CQ_OK == ret, "toBeRemovedEncodedPacketQueue overflow!!!");
    osSignalSet(a2dp_decoder_off_bth_processor_thread_id, A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_REMOVE_SPECIFIC_FRAME);
}

static void a2dp_decoder_cc_off_bth_stream_reset(void)
{
    InitCQueue(&a2dp_decoder_cc_off_bth_env.toBeRemovedEncodedPacketQueue,
        sizeof(a2dp_decoder_cached_to_be_removed_frame_ptr_queue_buf),
        (CQItemType *)a2dp_decoder_cached_to_be_removed_frame_ptr_queue_buf);
}

static void a2dp_decoder_cc_init_handler(void)
{
    A2DP_DECODER_INIT_REQ_T* pReq = &(a2dp_decoder_cc_off_bth_env.cachedInitReq);
    a2dp_decoder_cc_off_bth_env.codecType = pReq->codecType;
    a2dp_decoder_cc_off_bth_env.channel = pReq->channel;
    a2dp_decoder_cc_off_bth_env.maxPcmDataSizePerFrame = pReq->maxPcmDataSizePerFrame;

    if (a2dp_decoder_cc_off_bth_env.codecType == A2DP_DECODER_CODEC_TYPE_SBC)
    {
        a2dp_decoder_cc_off_bth_env.maxPcmDataSizePerFrame = 512;
    }
    CC_DECODE_LOG_I("%s", __func__);
    CC_DECODE_LOG_I("codec type:0x%x ch:%u, num_chnl:%d, sample_rate:%d, bits:%d, max_pcm:%d",
        pReq->codecType, pReq->channel, pReq->stream_cfg.num_channels,
        pReq->stream_cfg.sample_rate, pReq->stream_cfg.bits_depth, pReq->maxPcmDataSizePerFrame);

    a2dp_decoder_cc_off_bth_stream_reset();
    off_bth_syspool_get_buff(&a2dp_decoder_cc_mem_pool, A2DP_DECODER_CC_MEM_POOL_SIZE);
    ASSERT(a2dp_decoder_cc_mem_pool, "%s size:%d", __func__, A2DP_DECODER_CC_MEM_POOL_SIZE);
    a2dp_decoder_cc_heap_init(a2dp_decoder_cc_mem_pool, A2DP_DECODER_CC_MEM_POOL_SIZE);

    a2dp_decoder_cc_list_new(&a2dp_decoder_cc_off_bth_env.encoded_data_list,
                (osMutex(a2dp_decoder_cc_encoded_data_buffer_mutex)),
                a2dp_decoder_cc_data_free,
                a2dp_decoder_cc_heap_cmalloc,
                a2dp_decoder_cc_heap_free);

    a2dp_decoder_cc_list_new(&a2dp_decoder_cc_off_bth_env.decoded_data_list,
                (osMutex(a2dp_decoder_cc_pcm_data_buffer_mutex)),
                a2dp_decoder_cc_data_free,
                a2dp_decoder_cc_heap_cmalloc,
                a2dp_decoder_cc_heap_free);
    A2DP_AUDIO_CC_DECODER_T *p_audio_cc_decoder = NULL;

    a2dp_decoder_cc_off_bth_env.pcmLimiter = A2DP_DECODER_CC_PCM_MTU_LIMITER;
    a2dp_decoder_cc_off_bth_env.encodedThreshold = ENCODED_DATA_LIST_LENTH_CHECK_THR;
    a2dp_decoder_cc_off_bth_env.decodedThreshold = DECODED_DATA_LIST_LENTH_CHECK_THR;

    a2dp_decoder_cc_common_set_cmd_cached(false);

    switch (pReq->codecType)
    {
        case A2DP_DECODER_CODEC_TYPE_SBC:
            p_audio_cc_decoder = &a2dp_audio_cc_sbc_decoder_config;
            a2dp_decoder_cc_base_freq = APP_SYSFREQ_30M;
            // one packet contains multiple sub-frame
            a2dp_decoder_cc_off_bth_env.pcmLimiter = 64;
            a2dp_decoder_cc_off_bth_env.encodedThreshold = 80;
            a2dp_decoder_cc_off_bth_env.decodedThreshold = 32;
            break;
#ifdef A2DP_AAC_ON
        case A2DP_DECODER_CODEC_TYPE_MPEG2_4_AAC:
            p_audio_cc_decoder = &a2dp_audio_cc_aac_decoder_config;
            a2dp_decoder_cc_base_freq = APP_SYSFREQ_30M;
            break;
#endif
#ifdef A2DP_SCALABLE_ON
        case A2DP_DECODER_CODEC_TYPE_SCALABL:
            p_audio_cc_decoder = &a2dp_audio_cc_scalable_decoder_config;
            if (pReq->stream_cfg.sample_rate == 48000) {
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_30M;
            } else if (pReq->stream_cfg.sample_rate == 96000) {
                //TODO: Verifying
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_72M;
            } else {
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_30M;
            }
            break;
#endif
#ifdef A2DP_LDAC_ON
        case A2DP_DECODER_CODEC_TYPE_LDAC:
            p_audio_cc_decoder = &a2dp_audio_cc_ldac_decoder_config;
            a2dp_decoder_cc_base_freq = APP_SYSFREQ_104M;
            break;
#endif
#ifdef A2DP_LHDC_ON
        case A2DP_DECODER_CODEC_TYPE_LHDC:
            p_audio_cc_decoder = &a2dp_audio_cc_lhdc_decoder_config;
            a2dp_decoder_cc_base_freq = APP_SYSFREQ_104M;
            a2dp_decoder_cc_off_bth_env.encodedThreshold = 80;
            a2dp_decoder_cc_off_bth_env.decodedThreshold = 32;
            break;
#endif
#ifdef A2DP_LHDCV5_ON
        case A2DP_DECODER_CODEC_TYPE_LHDCV5:
            p_audio_cc_decoder = &a2dp_audio_cc_lhdcv5_decoder_config;
            if (pReq->stream_cfg.sample_rate == 48000) {
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_52M;
                a2dp_decoder_cc_off_bth_env.pcmLimiter = 30;
                a2dp_decoder_cc_off_bth_env.encodedThreshold = 80;
                a2dp_decoder_cc_off_bth_env.decodedThreshold = 32;
            } else if (pReq->stream_cfg.sample_rate == 96000) {
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_104M;
                a2dp_decoder_cc_off_bth_env.pcmLimiter = 60;
                a2dp_decoder_cc_off_bth_env.encodedThreshold = 80;
                a2dp_decoder_cc_off_bth_env.decodedThreshold = 32;
            } else if (pReq->stream_cfg.sample_rate == 192000) {
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_208M;
                a2dp_decoder_cc_off_bth_env.pcmLimiter = 30;
                a2dp_decoder_cc_off_bth_env.encodedThreshold = 40;
                a2dp_decoder_cc_off_bth_env.decodedThreshold = 16;
            } else {
                a2dp_decoder_cc_base_freq = APP_SYSFREQ_104M;
                a2dp_decoder_cc_off_bth_env.pcmLimiter = 50;
                a2dp_decoder_cc_off_bth_env.encodedThreshold = 80;
                a2dp_decoder_cc_off_bth_env.decodedThreshold = 32;
            }
            break;
#endif
        default:
            ASSERT(0, "%s, unknow codec type:%d", __func__, pReq->codecType);
            break;
    }

    app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, a2dp_decoder_cc_base_freq);

    memcpy((uint8_t *)&p_audio_cc_decoder->stream_config , (uint8_t *)&pReq->stream_cfg, sizeof(A2DP_AUDIO_CC_OUTPUT_CONFIG_T));
    memcpy((uint8_t *)&a2dp_decoder_cc_off_bth_env.audioDecoder, (uint8_t *)p_audio_cc_decoder, sizeof(A2DP_AUDIO_CC_DECODER_T));

    a2dp_decoder_cc_off_bth_env.audioDecoder.audio_decoder_cc_init();

#if defined(A2DP_SCALABLE_ON)
    int empty_data[3] = {0,0,0};
    if (memcmp(pReq->custom_data, empty_data, sizeof(int)*3))
    {
        TRACE(0, "M55 SET PARAM1 %d PARAM1 %d PARAM1 %d", pReq->custom_data[0], pReq->custom_data[1], pReq->custom_data[2]);
        audio_audio_cc_scalable_set_sbm_param(pReq->custom_data);
    }
#endif

    a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing = true;

    A2DP_DECODER_INIT_RSP_T rsp;
    app_dsp_m55_bridge_send_rsp(CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP,
        (uint8_t *)&rsp, sizeof(rsp));
}

static void a2dp_decoder_cc_processor_thread(const void *arg)
{
    CC_DECODE_LOG_I("%s", __func__);
#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
    CC_DECODE_LOG_D("%s,  a2dp_decoder_cc_processor_thread start ready to send", __func__);
    savitech_dsp_data_t savitech_dsp_data_send;
    savitech_dsp_data_send.lhdc_data = genLhdcData_dsp();
#ifdef A2DP_LHDCV5_ON
    savitech_dsp_data_send.lhdcv5_data = lhdcv5_util_get_lhdcdata();
#endif
    a2dp_decoder_cc_accumulate_cmd_send(CROSS_CORE_TASK_CMD_LHDC_FED_DATA_PACKET, (uint8_t*)&savitech_dsp_data_send);
    CC_DECODE_LOG_D("%s,  data sent", __func__);
#endif
    while(1)
    {
        osEvent evt;
        // wait any signal
        evt = osSignalWait(0x0, osWaitForever);

        // get role from signal value
        if (osEventSignal == evt.status)
        {
            if (evt.value.signals & A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_STOP)
            {
                CC_DECODE_LOG_I("%s, decode deinit", __func__);
                a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing = false;
                a2dp_decoder_cc_off_bth_env.audioDecoder.audio_decoder_cc_deinit();
                a2dp_decoder_cc_off_bth_stream_reset();
                app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, APP_SYSFREQ_32K);
                off_bth_syspool_deinit(SYSPOOL_USER_A2DP);
            }

            if (evt.value.signals & A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_START)
            {
                off_bth_syspool_init(SYSPOOL_USER_A2DP, NULL);
                a2dp_decoder_cc_init_handler();
            }

            if (evt.value.signals & A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_DATA_RECEIVED)
            {
                uint32_t lenEncodedList = 0;
                uint32_t lenDecodedList = 0;
                a2dp_decoder_cc_buff_list_t *pList = NULL;

                pList = &a2dp_decoder_cc_off_bth_env.encoded_data_list;
                lenEncodedList = a2dp_decoder_cc_list_length(pList);
                pList = &a2dp_decoder_cc_off_bth_env.decoded_data_list;
                lenDecodedList = a2dp_decoder_cc_list_length(pList);

                if ((lenEncodedList > a2dp_decoder_cc_off_bth_env.encodedThreshold) &&
                    (lenDecodedList < a2dp_decoder_cc_off_bth_env.decodedThreshold)) {
                    if (0 == a2dp_decoder_cc_off_bth_env.boostFreqCnt) {
                        a2dp_decoder_cc_off_bth_env.boostFreqCnt = BOOST_FREQ_CNT;
                        app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, APP_SYSFREQ_104M);
                    }
                }

                // If there is no encoded data for x seconds,
                // print warning information.
                // x should less than MTU * frameMs
                CC_DECODE_LOG_THROTTLED(W, lenEncodedList ? 1: 0,
                    CC_DECODE_LOG_EVERY_100_MS, "encoded list underflow, decoded list:%d", lenDecodedList);
                CC_DECODE_LOG_THROTTLED(W, lenDecodedList < a2dp_decoder_cc_off_bth_env.pcmLimiter ? 1: 0,
                    CC_DECODE_LOG_EVERY_100_MS, "decoded list full, encoded list:%d", lenEncodedList);

                if ((lenEncodedList > 0) &&
                    (lenDecodedList < a2dp_decoder_cc_off_bth_env.pcmLimiter) &&
                    (a2dp_decoder_cc_off_bth_env.isA2dpStreamingOnGoing)) {
                    a2dp_decoder_cc_adjust_freq_dynamic(a2dp_decoder_cc_base_freq);
                    a2dp_decoder_cc_process_encoded_data();
                    a2dp_decoder_cc_adjust_freq_dynamic(APP_SYSFREQ_15M);
                }
            }

            if (evt.value.signals & A2DP_DECODER_OFF_BTH_THREAD_SIGNAL_REMOVE_SPECIFIC_FRAME)
            {
                a2dp_decoder_cc_rm_specific_frame_handler();
            }
        }
    }
}

void a2dp_decoder_cc_off_bth_init(void)
{
    if (NULL == a2dp_decoder_off_bth_processor_thread_id)
    {
        a2dp_decoder_off_bth_processor_thread_id =
            osThreadCreate(osThread(a2dp_decoder_cc_processor_thread), NULL);
        a2dp_decoder_cc_cmd_cache_init();
    }
}

uint8_t a2dp_decoder_cc_get_channel(void)
{
    return (uint8_t)a2dp_decoder_cc_off_bth_env.channel;
}

#endif


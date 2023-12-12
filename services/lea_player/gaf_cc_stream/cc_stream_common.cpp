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
#include <string.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_location.h"
#include "plat_types.h"
#include "app_utils.h"
#include "app_gaf_dbg.h"
#include "gaf_codec_lc3.h"
#include "cc_stream_common.h"
#include "gaf_cc_stream_process.h"

/************************private macro defination***************************/
#define CC_STREAM_THREAD_STACK_SIZE        (1024*4)
#define CC_STREAM_SIGNAL_DATA_RECEIVED     (1 << 0)
#define CC_CAPTURE_SIGNAL_STOP             (1 << 1)
#define CC_CAPTURE_SIGNAL_START            (1 << 2)
#define CC_PLAYBACK_SIGNAL_STOP            (1 << 3)
#define CC_PLAYBACK_SIGNAL_START           (1 << 4)

#define PLAYBACK_INIT_REQ_BUF_SIZE      \
    ((sizeof(CC_PLAYBACK_INIT_REQ_T) + sizeof(CORE_PLAYBACK_INIT_INFO_T))*PLAYBACK_INSTANCE_MAX)
#define PLAYBACK_DEINIT_REQ_BUF_SIZE    \
    (sizeof(CC_PLAYBACK_DEINIT_REQ_T)*PLAYBACK_INSTANCE_MAX)
#define CAPTURE_INIT_REQ_BUF_SIZE       \
    ((sizeof(CC_CAPTURE_INIT_REQ_T) + sizeof(CORE_CAPTURE_INIT_INFO_T))*CAPTURE_INSTANCE_MAX)
#define CAPTURE_DEINIT_REQ_BUF_SIZE     \
    (sizeof(CC_CAPTURE_DEINIT_REQ_T)*CAPTURE_INSTANCE_MAX)

#define CC_PLAYBACK_MAX_FRAME_CNT   (16)
#define CC_PLAYBACK_MAX_PCM_CNT     (4)
#define CC_CAPTURE_MAX_FRAME_CNT    (8)
#define CC_CAPTURE_MAX_PCM_CNT      (4)

#define CC_STREAM_FREQ_BOOST_TIMEOUT (1500)

#ifndef AOB_CODEC_CP
#define CC_STREAM_TRACE(...) TRACE(0, ##__VA_ARGS__)
#else
#define CC_STREAM_TRACE(...)
#endif
/************************private type defination****************************/
typedef struct
{
    CC_PLAYBACK_INSTANCE playbackInstance[PLAYBACK_INSTANCE_MAX];
    CC_CAPTURE_INSTANCE captureInstance[CAPTURE_INSTANCE_MAX];

    cfifo playback_init_req_fifo;
    cfifo capture_init_req_fifo;
    cfifo playback_deinit_req_fifo;
    cfifo capture_deinit_req_fifo;

    uint8_t playback_init_req_buf[PLAYBACK_INIT_REQ_BUF_SIZE];
    uint8_t capture_init_req_buf[CAPTURE_INIT_REQ_BUF_SIZE];
    uint8_t playback_deinit_req_buf[PLAYBACK_DEINIT_REQ_BUF_SIZE];
    uint8_t capture_deinit_req_buf[CAPTURE_DEINIT_REQ_BUF_SIZE];

    bool isProcessing;
} CC_STREAM_ENV_T;

/************************private variable defination************************/
CC_STREAM_ENV_T ccStreamEnv;

static osThreadId cc_stream_processor_thread_id = NULL;
osTimerId cc_stream_freq_boost_timer_id = NULL;

/**********************private function declaration*************************/
static void cc_stream_boost_timeout_handler(void const *n);
static void cc_stream_processor_thread(const void *arg);

osThreadDef(cc_stream_processor_thread, osPriorityAboveNormal, 1,
            (CC_STREAM_THREAD_STACK_SIZE), "cc_stream_processor_thread");
osTimerDef(CC_STREAM_FREQ_BOOST_TIMER, cc_stream_boost_timeout_handler);

/****************************function defination****************************/

static void cc_stream_boost_timeout_handler(void const *n)
{
    LOG_I("%s", __func__);
    app_sysfreq_req(APP_SYSFREQ_USER_STREAM_BOOST, APP_SYSFREQ_32K);
}

void cc_stream_boost_freq(void)
{
    LOG_I("%s ", __func__);
    if (NULL == cc_stream_freq_boost_timer_id)
    {
        cc_stream_freq_boost_timer_id =
            osTimerCreate(osTimer(CC_STREAM_FREQ_BOOST_TIMER), osTimerOnce, NULL);
    }
    osTimerStop(cc_stream_freq_boost_timer_id);
    osTimerStart(cc_stream_freq_boost_timer_id, CC_STREAM_FREQ_BOOST_TIMEOUT);
    app_sysfreq_req(APP_SYSFREQ_USER_STREAM_BOOST, APP_SYSFREQ_208M);
}

FRAM_TEXT_LOC
static CC_PLAYBACK_DATA_T* cc_playback_get_frame(uint8_t instance_handle)
{
    CC_PLAYBACK_INSTANCE *instance = &(ccStreamEnv.playbackInstance[instance_handle]);
    cfifo *frame_fifo = instance->playback_context.frame_fifo;
    CC_PLAYBACK_DATA_T *frame = (CC_PLAYBACK_DATA_T *)instance->playback_context.frame_cache;

    uint32_t header_len = sizeof(CC_PLAYBACK_DATA_T);
    uint32_t frame_size = instance->playback_info.frameSize;
    uint16_t lastSeq = instance->playback_context.last_seq;

peek_again:
    if (cfifo_len(frame_fifo) >= frame_size)
    {
        cfifo_peek_to_buf(frame_fifo, (uint8_t*)frame, header_len);
    }
    else
    {
        CC_STREAM_TRACE("instance:%d playback get frame err!!!", instance_handle);
        return NULL;
    }

    if (INSTANCE_INITIALIZED == instance->status)
    {
        instance->playback_context.last_seq = lastSeq = frame->seq_nb - 1;
        instance->status = INSTANCE_STREAMING;
        CC_STREAM_TRACE("instance:%d first pkt seq:%d timeStamp:%u",
                instance_handle, frame->seq_nb, frame->time_stamp);
    }

    frame->data = instance->playback_context.frame_cache + header_len;
    frame->data_len = frame_size - header_len;

    int16_t checkResult = frame->seq_nb - 1 - lastSeq;
    instance->playback_context.last_seq = frame->seq_nb;

    if (checkResult == 0)
    {
        cfifo_pop(frame_fifo, NULL, header_len);
        cfifo_pop(frame_fifo, frame->data, frame_size - header_len);
    }
    else if (checkResult > 0)
    {
        CC_STREAM_TRACE("[PLC] instance:%d seq: %d/ %d", instance_handle, frame->seq_nb, lastSeq);
        frame->isPLC = true;
    }
    else
    {
        cfifo_pop(frame_fifo, NULL, frame_size);
        goto peek_again;
    }

    return frame;
}

FRAM_TEXT_LOC
static void cc_process_decoding(uint8_t instance_handle)
{
    CC_PLAYBACK_INSTANCE *instance = &(ccStreamEnv.playbackInstance[instance_handle]);
    cfifo *pcm_fifo = instance->playback_context.pcm_fifo;
    uint32_t header_len = sizeof(CC_PLAYBACK_DATA_T);
    uint32_t pcm_size = instance->playback_info.pcmSize;
    CC_PLAYBACK_DATA_T *pcm_data = (CC_PLAYBACK_DATA_T*)instance->playback_context.pcm_cache;
    uint32_t ret = 0;

    CC_PLAYBACK_DATA_T *frame = cc_playback_get_frame(instance_handle);
    if (!frame) {
        return;
    }

    pcm_data->data = instance->playback_context.pcm_cache + header_len;
    pcm_data->data_len = pcm_size - header_len;
    pcm_data->isPLC = frame->isPLC;
    pcm_data->seq_nb = frame->seq_nb;
    pcm_data->time_stamp = frame->time_stamp;

    ret = instance->dec_func_list->decoder_decode_frame_func(
                instance_handle, &(instance->playback_info.decoder_info),
                frame->data_len, frame->data, pcm_data->data, pcm_data->isPLC);
    if (CODEC_ERR == ret) {
        CC_STREAM_TRACE("dec err!!");
        memset(pcm_data->data, 0, pcm_size - header_len);
    }
    cfifo_put(pcm_fifo, instance->playback_context.pcm_cache, pcm_size);
}

FRAM_TEXT_LOC
static void cc_process_encoding(uint8_t instance_handle)
{
    CC_CAPTURE_INSTANCE *instance = &(ccStreamEnv.captureInstance[instance_handle]);
    cfifo *frame_fifo = instance->capture_context.frame_fifo;
    cfifo *pcm_fifo = instance->capture_context.pcm_fifo;
    CC_CAPTURE_DATA_T *frame = (CC_CAPTURE_DATA_T*)instance->capture_context.frame_cache;
    CC_CAPTURE_DATA_T *pcm_data = (CC_CAPTURE_DATA_T*)instance->capture_context.pcm_cache;

    uint32_t header_len = sizeof(CC_CAPTURE_DATA_T);
    uint32_t pcm_size = instance->capture_info.pcmSize;
    uint32_t normal_frame_size = instance->capture_info.frameSize;

    uint32_t ret = cfifo_pop(pcm_fifo, instance->capture_context.pcm_cache, pcm_size);
    if (FIFO_ERR == ret)
    {
        CC_STREAM_TRACE("capture get pcm err!!!");
        return;
    }

    pcm_data->data = instance->capture_context.pcm_cache + header_len;

    uint32_t pcm_len = pcm_data->data_len;
    if (instance->capture_algo_func_list) {
        pcm_len = instance->capture_algo_func_list->capture_algo_run(pcm_data->data, pcm_len);
    }

    frame->time_stamp = pcm_data->time_stamp;
    frame->frame_size = pcm_data->frame_size;
    frame->data = instance->capture_context.frame_cache + header_len;

    ret = instance->enc_func_list->encoder_encode_frame_func(
            instance_handle, &(instance->capture_info.encoder_info),
            pcm_len, pcm_data->data, pcm_data->frame_size, frame->data);
    if (ret == CODEC_ERR) {
        CC_STREAM_TRACE("enc err!!");
    }

    cfifo_put(frame_fifo, instance->capture_context.frame_cache, normal_frame_size);
}

FRAM_TEXT_LOC
static bool cc_process_again(void)
{
    uint8_t instance_handle = 0;
    CC_PLAYBACK_INSTANCE *playback_instance = NULL;
    CC_CAPTURE_INSTANCE *capture_instance = NULL;
    for (instance_handle = 0; instance_handle < PLAYBACK_INSTANCE_MAX; instance_handle++)
    {
        playback_instance = &(ccStreamEnv.playbackInstance[instance_handle]);
        if (INSTANCE_IDLE == playback_instance->status) {
            continue;
        }
        if (cfifo_len(playback_instance->playback_context.frame_fifo) >= \
            playback_instance->playback_info.frameSize  &&
            cfifo_free_space(playback_instance->playback_context.pcm_fifo) >= \
            playback_instance->playback_info.pcmSize)
        {
            return true;
        }
    }
    for (instance_handle = 0; instance_handle < CAPTURE_INSTANCE_MAX; instance_handle++)
    {
        capture_instance = &(ccStreamEnv.captureInstance[instance_handle]);
        if (INSTANCE_IDLE == capture_instance->status) {
            continue;
        }
        if (cfifo_len(capture_instance->capture_context.pcm_fifo) >= \
            capture_instance->capture_info.pcmSize &&
            cfifo_free_space(capture_instance->capture_context.frame_fifo) >= \
            capture_instance->capture_info.frameSize)
        {
            return true;
        }
    }
    return false;
}

FRAM_TEXT_LOC
void cc_stream_process(void)
{
    uint8_t instance_handle = 0;
    ccStreamEnv.isProcessing = true;
    do
    {
        for (instance_handle = 0; instance_handle < PLAYBACK_INSTANCE_MAX; instance_handle++)
        {
            CC_PLAYBACK_INSTANCE *instance = &(ccStreamEnv.playbackInstance[instance_handle]);
            if (INSTANCE_IDLE == instance->status) {
                continue;
            }
            if (cfifo_len(instance->playback_context.frame_fifo) >=  \
                instance->playback_info.frameSize &&
                cfifo_free_space(instance->playback_context.pcm_fifo) >= \
                instance->playback_info.pcmSize)
            {
                cc_process_decoding(instance_handle);
            }
        }
        for (instance_handle = 0; instance_handle < CAPTURE_INSTANCE_MAX; instance_handle++)
        {
            CC_CAPTURE_INSTANCE *instance = &(ccStreamEnv.captureInstance[instance_handle]);
            if (INSTANCE_IDLE == instance->status) {
                continue;
            }
            if (cfifo_len(instance->capture_context.pcm_fifo) >= \
                instance->capture_info.pcmSize &&
                cfifo_free_space(instance->capture_context.frame_fifo) >= \
                instance->capture_info.frameSize)
            {
                cc_process_encoding(instance_handle);
            }
        }
    } while (cc_process_again());
    ccStreamEnv.isProcessing = false;
}

static void cc_playback_buf_init(uint8_t instance_handle)
{
    CC_PLAYBACK_INFO_T *playbackInfo =
        &(ccStreamEnv.playbackInstance[instance_handle].playback_info);
    CC_PLAYBACK_CONTEXT_T *playbackContext =
        &(ccStreamEnv.playbackInstance[instance_handle].playback_context);
    CC_HEAP_ALLOC buf_alloc = ccStreamEnv.playbackInstance[instance_handle].core_playback_func_list->buf_alloc;

    uint32_t frame_buf_size = playbackInfo->frameSize * CC_PLAYBACK_MAX_FRAME_CNT;
    uint32_t pcm_buf_size = playbackInfo->pcmSize * CC_PLAYBACK_MAX_PCM_CNT;

    if (!ccStreamEnv.playbackInstance[instance_handle].buffer_initialized)
    {
        playbackContext->frame_fifo = (cfifo*)buf_alloc(NULL, sizeof(cfifo));
        playbackContext->pcm_fifo = (cfifo*)buf_alloc(NULL, sizeof(cfifo));

        playbackContext->frame_buf = (uint8_t*)buf_alloc(NULL, frame_buf_size);
        playbackContext->pcm_buf = (uint8_t*)buf_alloc(NULL, pcm_buf_size);

        playbackContext->frame_cache = (uint8_t*)buf_alloc(NULL, playbackInfo->frameSize);
        playbackContext->pcm_cache = (uint8_t*)buf_alloc(NULL, playbackInfo->pcmSize);

        ccStreamEnv.playbackInstance[instance_handle].buffer_initialized = true;
    }

    cfifo_init(playbackContext->frame_fifo, playbackContext->frame_buf, frame_buf_size);
    cfifo_init(playbackContext->pcm_fifo, playbackContext->pcm_buf, pcm_buf_size);
}

static uint8_t cc_playback_alloc_instance(uint32_t frameSize, uint32_t pcmSize)
{
    CC_PLAYBACK_INSTANCE *instance = NULL;
    for (uint8_t instance_handle = 0; instance_handle < PLAYBACK_INSTANCE_MAX; instance_handle++)
    {
        instance = &(ccStreamEnv.playbackInstance[instance_handle]);
        if (instance->status >= INSTANCE_INITIALIZED) {
            continue;
        }
        if (!instance->buffer_initialized ||
            (instance->buffer_initialized && instance->playback_info.pcmSize >= pcmSize &&
            instance->playback_info.frameSize >= frameSize))
        {
            return instance_handle;
        }
    }
    ASSERT(false, "%s frameSize:%d pcmSize:%d", __func__, frameSize, pcmSize);
}

void cc_playback_init(CORE_PLAYBACK_INIT_INFO_T *initInfo)
{
    CC_PLAYBACK_INIT_REQ_T *initReq = (CC_PLAYBACK_INIT_REQ_T*)(initInfo->initReq);

    uint32_t pcmSize = initReq->decoder_info.pcm_size + sizeof(CC_PLAYBACK_DATA_T);
    uint32_t frameSize = sizeof(CC_PLAYBACK_DATA_T) + \
        initReq->decoder_info.num_channels * initReq->decoder_info.frame_size;
    uint8_t instance_handle = cc_playback_alloc_instance(frameSize, pcmSize);
    CC_PLAYBACK_INSTANCE *instance = &(ccStreamEnv.playbackInstance[instance_handle]);
    CC_PLAYBACK_INFO_T *playbackInfo = &(instance->playback_info);

    instance->core = (CORE_TYPE_E)initInfo->core_type;
    instance->core_playback_func_list = initInfo->playback_func_list;
    playbackInfo->decoder_info = initReq->decoder_info;
    playbackInfo->pcmSize = pcmSize;
    playbackInfo->frameSize = frameSize;

    LOG_I("sample_rate:%d num_channels:%d frame_size:%d bits_depth:%d frame_ms:%d",
    playbackInfo->decoder_info.sample_rate, playbackInfo->decoder_info.num_channels,
    playbackInfo->decoder_info.frame_size, playbackInfo->decoder_info.bits_depth,
    (int)playbackInfo->decoder_info.frame_ms);
    LOG_I("pcmSize:%d", initReq->decoder_info.pcm_size);

    instance->core_playback_func_list->playback_init(instance_handle);

    cc_playback_buf_init(instance_handle);

    if (LC3 == initReq->codec_type || LC3PLUS == initReq->codec_type)
    {
        gaf_audio_lc3_update_decoder_func_list(&(instance->dec_func_list));
    }
    else
    {
        ASSERT(false, "%s codec_type:%dcodec type err", __func__, initReq->codec_type);
    }

    uint32_t dec_freq = instance->dec_func_list->decoder_set_freq(
                    &(playbackInfo->decoder_info), APP_SYSFREQ_52M, initInfo->core_type);

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, (enum APP_SYSFREQ_FREQ_T)dec_freq);

    instance->dec_func_list->decoder_init_buf_func(instance_handle,
        &(playbackInfo->decoder_info), (void*)(instance->core_playback_func_list->buf_alloc));
    instance->dec_func_list->decoder_init_func(instance_handle, &(playbackInfo->decoder_info));

    CC_PLAYBACK_INIT_RSP_T rsp;
    rsp.isBis = initReq->isBis;
    rsp.ptr = initReq->ptr;
    rsp.ptr2 = initReq->ptr2;
    rsp.frame_fifo = instance->playback_context.frame_fifo;
    rsp.pcm_fifo = instance->playback_context.pcm_fifo;
    rsp.isDecoding = &ccStreamEnv.isProcessing;
    rsp.instance_handle = instance_handle;

    instance->core_playback_func_list->playback_init_rsp(&rsp, sizeof(rsp));
    instance->status = INSTANCE_INITIALIZED;
}

static void cc_capture_buf_init(uint8_t instance_handle)
{
    CC_CAPTURE_INSTANCE *instance = &ccStreamEnv.captureInstance[instance_handle];
    CC_CAPTURE_CONTEXT_T *captureContext = &(instance->capture_context);
    CC_HEAP_ALLOC buf_alloc = instance->core_capture_func_list->buf_alloc;

    uint32_t frame_size = instance->capture_info.frameSize;
    uint32_t frame_buf_size = frame_size * CC_CAPTURE_MAX_FRAME_CNT;
    uint32_t pcm_size = instance->capture_info.pcmSize;
    uint32_t pcm_buf_size = pcm_size * CC_CAPTURE_MAX_PCM_CNT;

    if (!instance->buffer_initialized)
    {
        captureContext->pcm_fifo = (cfifo*)buf_alloc(NULL, sizeof(cfifo));
        captureContext->frame_fifo = (cfifo*)buf_alloc(NULL, sizeof(cfifo));

        captureContext->pcm_buf = (uint8_t*)buf_alloc(NULL, pcm_buf_size);
        captureContext->frame_buf = (uint8_t*)buf_alloc(NULL, frame_buf_size);

        captureContext->pcm_cache = (uint8_t*)buf_alloc(NULL, pcm_size);
        captureContext->frame_cache = (uint8_t*)buf_alloc(NULL, frame_size);

        instance->buffer_initialized = true;
    }

    instance->enc_func_list->encoder_init_buf_func(instance_handle,
                    &(instance->capture_info.encoder_info), (void*)(buf_alloc));

    if (instance->capture_algo_func_list) {
        instance->capture_algo_func_list->capture_algo_buf_init((void*)buf_alloc);
    }

    cfifo_init(captureContext->frame_fifo, captureContext->frame_buf, frame_buf_size);
    cfifo_init(captureContext->pcm_fifo, captureContext->pcm_buf, pcm_buf_size);
}

static uint8_t cc_capture_alloc_instance(uint32_t frameSize, uint32_t pcmSize)
{
    CC_CAPTURE_INSTANCE *instance = NULL;
    for (uint8_t instance_handle = 0; instance_handle < CAPTURE_INSTANCE_MAX; instance_handle++)
    {
        instance = &(ccStreamEnv.captureInstance[instance_handle]);
        if (instance->status >= INSTANCE_INITIALIZED){
            continue;
        }
        if (!instance->buffer_initialized ||
            (instance->buffer_initialized && instance->capture_info.pcmSize >= pcmSize &&
            instance->capture_info.frameSize >= frameSize))
        {
            return instance_handle;
        }
    }
    ASSERT(false, "%s frameSize:%d pcmSize:%d", __func__, frameSize, pcmSize);
}

void cc_capture_init(CORE_CAPTURE_INIT_INFO_T *initInfo)
{
    CC_CAPTURE_INIT_REQ_T *initReq = (CC_CAPTURE_INIT_REQ_T*)initInfo->initReq;

    uint32_t pcmSize = initReq->pcm_size + sizeof(CC_CAPTURE_DATA_T);
    uint32_t frameSize = sizeof(CC_CAPTURE_DATA_T) + \
        initReq->encoder_info.num_channels * initReq->encoder_info.frame_size;

    uint8_t instance_handle = cc_capture_alloc_instance(frameSize, pcmSize);
    CC_CAPTURE_INSTANCE *instance = &(ccStreamEnv.captureInstance[instance_handle]);
    CC_CAPTURE_INFO_T *captureInfo = &(instance->capture_info);

    instance->core = (CORE_TYPE_E)initInfo->core_type;
    instance->core_capture_func_list = initInfo->capture_func_list;
    captureInfo->encoder_info = initReq->encoder_info;
    captureInfo->pcmSize = pcmSize;
    captureInfo->frameSize = frameSize;

    LOG_I("sample_rate:%d num_channels:%d frame_size:%d bits_depth:%d",
    captureInfo->encoder_info.sample_rate, captureInfo->encoder_info.num_channels,
    captureInfo->encoder_info.frame_size, captureInfo->encoder_info.bits_depth);
    LOG_I("frame_ms:%d pcmSize:%d", (int)captureInfo->encoder_info.frame_ms, initReq->pcm_size);

    instance->core_capture_func_list->capture_init(instance_handle);

    if (initReq->algoProcess) {
        gaf_cc_capture_algo_func_register(&(instance->capture_algo_func_list));
    }

    if (LC3 == initReq->codec_type || LC3PLUS == initReq->codec_type)
    {
        gaf_audio_lc3_update_encoder_func_list(&(instance->enc_func_list));
    }
    else
    {
        ASSERT(false, "%s codec type err", __func__);
    }

    cc_capture_buf_init(instance_handle);

    uint32_t enc_freq = instance->enc_func_list->encoder_set_freq(
                    &(captureInfo->encoder_info), APP_SYSFREQ_52M, initInfo->core_type);
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, (enum APP_SYSFREQ_FREQ_T)enc_freq);

    instance->enc_func_list->encoder_init_func(instance_handle, &(captureInfo->encoder_info));

    if (instance->capture_algo_func_list) {
        instance->capture_algo_func_list->capture_algo_open(
                &initReq->tx_algo_cfg, &captureInfo->encoder_info);
    }

    CC_CAPTURE_INIT_RSP_T rsp;
    rsp.pcm_fifo = instance->capture_context.pcm_fifo;
    rsp.frame_fifo = instance->capture_context.frame_fifo;
    rsp.isEncoding = &ccStreamEnv.isProcessing;
    rsp.instance_handle = instance_handle;
    rsp.isBis = initReq->isBis;
    rsp.ptr = initReq->ptr;
    rsp.ptr2 = initReq->ptr2;

    instance->core_capture_func_list->capture_init_rsp(&rsp, sizeof(rsp));
    instance->status = INSTANCE_INITIALIZED;
}

void cc_playback_deinit(void *_deinitReq)
{
    CC_PLAYBACK_DEINIT_REQ_T *deinitReq = (CC_PLAYBACK_DEINIT_REQ_T*)_deinitReq;
    CC_PLAYBACK_DEINIT_RSP_T deinitRsp;
    deinitRsp.isBis = deinitReq->isBis;
    LOG_I("%s instance_handle:%d", __func__, deinitReq->instance_handle);

    CC_PLAYBACK_INSTANCE *instance = &(ccStreamEnv.playbackInstance[deinitReq->instance_handle]);
    if (INSTANCE_IDLE != instance->status)
    {
        instance->dec_func_list->decoder_deinit_func(deinitReq->instance_handle);
        instance->status = INSTANCE_IDLE;
    }

    if (deinitReq->isRetrigger) {
        deinitRsp.closeDecoderCore = false;
    }
    else {
        deinitRsp.closeDecoderCore =
            instance->core_playback_func_list->playback_deinit(deinitReq->instance_handle);
    }
    if (instance->core_playback_func_list->playback_deinit_rsp)
    {
        instance->core_playback_func_list->playback_deinit_rsp((uint8_t*)&deinitRsp, sizeof(deinitRsp));
    }
}

void cc_capture_deinit(void *_deinitReq)
{
    CC_CAPTURE_DEINIT_REQ_T *deinitReq = (CC_CAPTURE_DEINIT_REQ_T*)_deinitReq;
    CC_CAPTURE_DEINIT_RSP_T deinitRsp;
    deinitRsp.isBis = deinitReq->isBis;
    LOG_I("%s instance_handle:%d", __func__, deinitReq->instance_handle);

    CC_CAPTURE_INSTANCE *instance = &(ccStreamEnv.captureInstance[deinitReq->instance_handle]);
    if (INSTANCE_IDLE != instance->status)
    {
        if (instance->capture_algo_func_list) {
            instance->capture_algo_func_list->capture_algo_close();
        }

        instance->enc_func_list->encoder_deinit_func(deinitReq->instance_handle);
        instance->status = INSTANCE_IDLE;
    }

    if (deinitReq->isRetrigger) {
        deinitRsp.closeEncoderCore = false;
    }
    else {
        deinitRsp.closeEncoderCore =
            instance->core_capture_func_list->capture_deinit(deinitReq->instance_handle);
    }
    if (instance->core_capture_func_list->capture_deinit_rsp)
    {
        instance->core_capture_func_list->capture_deinit_rsp((uint8_t*)&deinitRsp, sizeof(deinitRsp));
    }
}

void cc_playback_buf_deinit(CORE_TYPE_E core_type)
{
    for (uint32_t index = 0; index < PLAYBACK_INSTANCE_MAX; index++) {
        if (core_type == ccStreamEnv.playbackInstance[index].core) {
            ccStreamEnv.playbackInstance[index].buffer_initialized = false;
            if (ccStreamEnv.playbackInstance[index].dec_func_list)
            {
                ccStreamEnv.playbackInstance[index].dec_func_list->decoder_deinit_buf_func(index);
            }
        }
    }
    LOG_I("%s", __func__);
}

void cc_capture_buf_deinit(CORE_TYPE_E core_type)
{
    for (uint32_t index = 0; index < CAPTURE_INSTANCE_MAX; index++) {
        if (core_type == ccStreamEnv.captureInstance[index].core) {
            ccStreamEnv.captureInstance[index].buffer_initialized = false;
            if (ccStreamEnv.captureInstance[index].enc_func_list)
            {
                ccStreamEnv.captureInstance[index].enc_func_list->encoder_deinit_buf_func(index);
            }
            if (ccStreamEnv.captureInstance[index].capture_algo_func_list) {
                ccStreamEnv.captureInstance[index].capture_algo_func_list->capture_algo_buf_deinit();
            }
        }
    }
    LOG_I("%s", __func__);
}

static void cc_playback_init_signal_handler(void)
{
    cc_stream_boost_freq();

    CC_PLAYBACK_INIT_REQ_T initReq;
    CORE_PLAYBACK_INIT_INFO_T initInfo;
    cfifo *fifo = &(ccStreamEnv.playback_init_req_fifo);
    while (cfifo_len(fifo) >= sizeof(initReq) + sizeof(initInfo))
    {
        cfifo_pop(fifo, (uint8_t*)&initReq, sizeof(initReq));
        cfifo_pop(fifo, (uint8_t*)&initInfo, sizeof(initInfo));
        initInfo.initReq = (uint8_t*)&initReq;
        cc_playback_init(&initInfo);
    }
}

static void cc_playback_deinit_signal_handler(void)
{
    CC_PLAYBACK_DEINIT_REQ_T deinitReq;
    cfifo *fifo = &(ccStreamEnv.playback_deinit_req_fifo);
    while (cfifo_len(fifo) >= sizeof(deinitReq))
    {
        cfifo_pop(fifo, (uint8_t*)&deinitReq, sizeof(deinitReq));
        cc_playback_deinit(&deinitReq);
    }
}

static void cc_capture_init_signal_handler(void)
{
    cc_stream_boost_freq();

    CC_CAPTURE_INIT_REQ_T initReq;
    CORE_CAPTURE_INIT_INFO_T initInfo;
    cfifo *fifo = &(ccStreamEnv.capture_init_req_fifo);
    while (cfifo_len(fifo) >= sizeof(initReq) + sizeof(initInfo))
    {
        cfifo_pop(fifo, (uint8_t*)&initReq, sizeof(initReq));
        cfifo_pop(fifo, (uint8_t*)&initInfo, sizeof(initInfo));
        initInfo.initReq = (uint8_t*)&initReq;
        cc_capture_init(&initInfo);
    }
}

static void cc_capture_deinit_signal_handler(void)
{
    CC_CAPTURE_DEINIT_REQ_T deinitReq;
    cfifo *fifo = &(ccStreamEnv.capture_deinit_req_fifo);
    while (cfifo_len(fifo) >= sizeof(deinitReq))
    {
        cfifo_pop(fifo, (uint8_t*)&deinitReq, sizeof(deinitReq));
        cc_capture_deinit(&deinitReq);
    }
}

void cc_playback_init_req_handler(CORE_PLAYBACK_INIT_INFO_T *initInfo)
{
    cfifo *fifo = &(ccStreamEnv.playback_init_req_fifo);
    uint32_t len = sizeof(CC_PLAYBACK_INIT_REQ_T) + sizeof(CORE_PLAYBACK_INIT_INFO_T);
    ASSERT(cfifo_free_space(fifo) >= len,
        "playback init fifo free space:%d len:%d", cfifo_free_space(fifo), len);
    cfifo_put(fifo, initInfo->initReq, sizeof(CC_PLAYBACK_INIT_REQ_T));
    cfifo_put(fifo, (uint8_t*)initInfo, sizeof(CORE_PLAYBACK_INIT_INFO_T));
    osSignalSet(cc_stream_processor_thread_id, CC_PLAYBACK_SIGNAL_START);
}

void cc_playback_deinit_req_handler(uint8_t *deinitReq)
{
    cfifo *fifo = &(ccStreamEnv.playback_deinit_req_fifo);
    ASSERT(cfifo_free_space(fifo) >= sizeof(CC_PLAYBACK_DEINIT_REQ_T),
        "playback deinit fifo free space:%d len:%d", cfifo_free_space(fifo), sizeof(CC_PLAYBACK_DEINIT_REQ_T));
    cfifo_put(fifo, deinitReq, sizeof(CC_PLAYBACK_DEINIT_REQ_T));
    osSignalSet(cc_stream_processor_thread_id, CC_PLAYBACK_SIGNAL_STOP);
}

void cc_capture_init_req_handler(CORE_CAPTURE_INIT_INFO_T *initInfo)
{
    cfifo *fifo = &(ccStreamEnv.capture_init_req_fifo);
    uint32_t len = sizeof(CC_CAPTURE_INIT_REQ_T) + sizeof(CORE_CAPTURE_INIT_INFO_T);
    ASSERT(cfifo_free_space(fifo) >= len,
        "capture init fifo free space:%d len:%d", cfifo_free_space(fifo), len);
    cfifo_put(fifo, initInfo->initReq, sizeof(CC_CAPTURE_INIT_REQ_T));
    cfifo_put(fifo, (uint8_t*)initInfo, sizeof(CORE_CAPTURE_INIT_INFO_T));
    osSignalSet(cc_stream_processor_thread_id, CC_CAPTURE_SIGNAL_START);
}

void cc_capture_deinit_req_handler(uint8_t *deinitReq)
{
    cfifo *fifo = &(ccStreamEnv.capture_deinit_req_fifo);
    ASSERT(cfifo_free_space(fifo) >= sizeof(CC_CAPTURE_DEINIT_REQ_T),
        "capture deinit fifo free space:%d len:%d", cfifo_free_space(fifo), sizeof(CC_CAPTURE_DEINIT_REQ_T));
    cfifo_put(fifo, deinitReq, sizeof(CC_CAPTURE_DEINIT_REQ_T));
    osSignalSet(cc_stream_processor_thread_id, CC_CAPTURE_SIGNAL_STOP);
}

void cc_stream_data_notify_handler(void)
{
    osSignalSet(cc_stream_processor_thread_id, CC_STREAM_SIGNAL_DATA_RECEIVED);
}

static void cc_stream_processor_thread(const void *arg)
{
    while(1)
    {
        osEvent evt;
        evt = osSignalWait(0x0, osWaitForever);

        if (osEventSignal == evt.status)
        {
            if (evt.value.signals & CC_PLAYBACK_SIGNAL_START)
            {
                cc_playback_init_signal_handler();
            }
            if (evt.value.signals & CC_PLAYBACK_SIGNAL_STOP)
            {
                cc_playback_deinit_signal_handler();
            }

            if (evt.value.signals & CC_CAPTURE_SIGNAL_START)
            {
                cc_capture_init_signal_handler();
            }
            if (evt.value.signals & CC_CAPTURE_SIGNAL_STOP)
            {
                cc_capture_deinit_signal_handler();
            }

            if (evt.value.signals & CC_STREAM_SIGNAL_DATA_RECEIVED)
            {
                cc_stream_process();
            }
        }
    }
}

void cc_stream_init(uint32_t core_type)
{
    uint32_t index = 0;
    ccStreamEnv.isProcessing = false;
    for (index = 0; index < PLAYBACK_INSTANCE_MAX; index++)
    {
        ccStreamEnv.playbackInstance[index].buffer_initialized = false;
        ccStreamEnv.playbackInstance[index].status = INSTANCE_IDLE;
    }
    for (index = 0; index < CAPTURE_INSTANCE_MAX; index++)
    {
        ccStreamEnv.captureInstance[index].buffer_initialized = false;
        ccStreamEnv.captureInstance[index].status = INSTANCE_IDLE;
    }
    if (NULL == cc_stream_processor_thread_id && core_type != CP_CORE)
    {
        cc_stream_processor_thread_id =
            osThreadCreate(osThread(cc_stream_processor_thread), NULL);
    }
    if (core_type != CP_CORE)
    {
        cfifo_init(&(ccStreamEnv.playback_init_req_fifo), ccStreamEnv.playback_init_req_buf, PLAYBACK_INIT_REQ_BUF_SIZE);
        cfifo_init(&(ccStreamEnv.capture_init_req_fifo), ccStreamEnv.capture_init_req_buf, CAPTURE_INIT_REQ_BUF_SIZE);
        cfifo_init(&(ccStreamEnv.playback_deinit_req_fifo), ccStreamEnv.playback_deinit_req_buf, PLAYBACK_DEINIT_REQ_BUF_SIZE);
        cfifo_init(&(ccStreamEnv.capture_deinit_req_fifo), ccStreamEnv.capture_deinit_req_buf, CAPTURE_DEINIT_REQ_BUF_SIZE);
    }
}

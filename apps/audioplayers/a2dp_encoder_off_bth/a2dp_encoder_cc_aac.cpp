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

#include "cmsis.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "a2dp_encoder_cc_common.h"
#include "a2dp_encoder_cc_off_bth.h"
#include "btapp.h"
#include "aac_api.h"
#include "aac_error_code.h"

/**********************private function declaration*************************/
static void a2dp_encoder_cc_aac_init(void);
static void a2dp_encoder_cc_aac_deinit(void);
static bool a2dp_encoder_cc_aac_encode_frame(a2dp_encoder_cc_media_data_t* pDecodedPacket,
    a2dp_encoder_cc_media_data_t* pEncodedFrame);

/************************private variable defination************************/

typedef struct _aac_encoder_t{
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate;
    uint32_t aot;
    uint32_t afterburner;
    uint32_t vbr;
    uint32_t eld_sbr;
    uint32_t package;
}aac_encoder_t;

static aac_encoder_t inst = {
    .sample_rate = 48000,
    .channels = 2,
    .bitrate = 128000,
    .aot = 2,
    .afterburner = 0,
    .vbr = 0,
    .eld_sbr = 0,
    .package = AAC_PACKAGE_MCP1,
};

static const A2DP_AUDIO_CC_ENCODER_T a2dp_audio_cc_aac_encoder_config =
{
    {44100, 1},
    a2dp_encoder_cc_aac_init,
    a2dp_encoder_cc_aac_deinit,
    a2dp_encoder_cc_aac_encode_frame,
};

#define AAC_OUT_SIZE 1024*2
static void * SRAM_BSS_LOC aacEnc_handle = NULL;

#define AAC_MEMPOLL_SIZE (70*1024)
static uint8_t  SRAM_BSS_LOC aac_pool_buff[AAC_MEMPOLL_SIZE];
static uint8_t * SRAM_DATA_LOC  source_aac_mempoll = NULL;
static heap_handle_t aac_memhandle = NULL;
static heap_api_t aac_heap_api;
static void * aac_malloc(const unsigned size)
{
    void * ptr = NULL;
    multi_heap_info_t info;
    heap_get_info(aac_memhandle, &info);
    if (size >= info.total_free_bytes){
        TRACE(0, "aac_malloc failed need:%d, free_bytes:%d \n", size, info.total_free_bytes);
        return ptr;
    }
    TRACE(0, "aac_malloc size=%u free=%u alloc=%u", size, info.total_free_bytes, info.total_allocated_bytes);
    ptr = heap_malloc(aac_memhandle,size);
    return ptr;
}

static void aac_free(void * ptr)
{
    heap_free(aac_memhandle, (int*)ptr);
}

static heap_api_t aac_get_heap_api()
{
    heap_api_t api;
    api.malloc = &aac_malloc;
    api.free = &aac_free;
    return api;
}

static int source_aac_meminit(void)
{
    source_aac_mempoll = aac_pool_buff;

    if(aac_memhandle == NULL)
        aac_memhandle = heap_register(source_aac_mempoll, AAC_MEMPOLL_SIZE);
    aac_heap_api = aac_get_heap_api();
    return 1;
}

/****************************function defination****************************/
static void a2dp_encoder_cc_aac_init(void)
{
    if (aacEnc_handle == NULL) {
        if (source_aac_meminit() < 0)
        {
            CC_ENCODE_LOG_I("aac_meminit error\n");
            return;
        }
        aac_enc_para_t enc_para;
        enc_para.aot = inst.aot;
        enc_para.vbr = inst.vbr;
        enc_para.sample_rate = inst.sample_rate;
        enc_para.channels = inst.channels;
        enc_para.bitrate = inst.bitrate;
        enc_para.package = inst.package;
        aacEnc_handle = aac_encoder_open(aac_heap_api, enc_para);
    }
}

static void a2dp_encoder_cc_aac_deinit(void)
{
    if (aacEnc_handle != NULL) {
        aac_encoder_close(&aacEnc_handle);
        aacEnc_handle = NULL;
    }
    CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
}


static bool a2dp_encoder_cc_aac_encode_frame(a2dp_encoder_cc_media_data_t* pPcmPacket,
                                             a2dp_encoder_cc_media_data_t* pEncodedPacket)
{
    CC_ENCODE_LOG_I("%s, %d, %d", __func__, __LINE__, pPcmPacket->data_len);

    pcm_frame_t pcm_data;
    pcm_data.pcm_data = (short *)(pPcmPacket->data);
    pcm_data.buffer_size = pPcmPacket->data_len;
    pcm_data.valid_size = pPcmPacket->data_len;
    aac_frame_t aac_data;
    aac_data.aac_data = (uint8_t*)pEncodedPacket->data;
    aac_data.buffer_size =  AAC_OUT_SIZE;
    aac_data.valid_size = 0;
    int err = aac_encoder_process_frame(aacEnc_handle, &pcm_data, &aac_data);
    if(err != AAC_OK || aac_data.valid_size <= 0)
    {
        CC_ENCODE_LOG_I("aacEncEncode err %x len %d", err, aac_data.valid_size);
        return false;
    }
    pEncodedPacket->data_len = aac_data.valid_size;
     CC_ENCODE_LOG_I("aacEncEncode %s %d  len:%d", __func__, __LINE__, pEncodedPacket->data_len);
    // CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
    return true;
}

const A2DP_AUDIO_CC_ENCODER_T* a2dp_encoder_aac_config()
{
    return &a2dp_audio_cc_aac_encoder_config;
}

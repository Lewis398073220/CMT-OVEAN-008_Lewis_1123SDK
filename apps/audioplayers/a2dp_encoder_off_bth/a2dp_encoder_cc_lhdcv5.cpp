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
#include "lhdcUtil.h"
#include "lhdcBesCCUtil.h"
#include "dwt.h"
#include "lhdcv5_util_enc.h"

/**********************private function declaration*************************/
static void a2dp_encoder_cc_lhdc_init(void);
static void a2dp_encoder_cc_lhdc_deinit(void);
static bool a2dp_encoder_cc_lhdc_encode_frame(a2dp_encoder_cc_media_data_t* pDecodedPacket,
    a2dp_encoder_cc_media_data_t* pEncodedFrame);


#ifndef A2DP_LHDCV5_TRANS_SIZE
#define A2DP_LHDCV5_TRANS_SIZE 960
#endif
/************************private variable defination************************/

typedef struct _lhdcv5_encoder_t{
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    uint32_t frame_duration;
    int32_t samples_per_frame;
    uint32_t audio_format;
    uint32_t version;
    uint32_t islossless;
    uint32_t bitrate_inx;
    uint32_t mtu;
    uint32_t interval;
    void* ptr;
}lhdcv5_encoder_t;

static lhdcv5_encoder_t inst = {
    .sample_rate = 48000,
    .bits_per_sample = 16,
    .frame_duration = LHDCV5_FRAME_5MS,
    .samples_per_frame = 0,
    .audio_format = 0,
    .version = LHDCV5_VERSION_1,
    .islossless = 1,
    .bitrate_inx = LHDCV5_QUALITY_HIGH5,
    .mtu = LHDCV5_MTU_4MBPS,
    .interval = LHDCV5_ENC_INTERVAL_10MS,
    .ptr = NULL,
};

static const A2DP_AUDIO_CC_ENCODER_T a2dp_audio_cc_lhdc_encoder_config =
{
    {44100, 1},
    a2dp_encoder_cc_lhdc_init,
    a2dp_encoder_cc_lhdc_deinit,
    a2dp_encoder_cc_lhdc_encode_frame,
};

#define LHDCV5_MEMPOOL_SIZE (131*1024)
static uint8_t  SRAM_BSS_LOC lhdcv5_pool_buff[LHDCV5_MEMPOOL_SIZE];
static uint8_t * SRAM_DATA_LOC  source_lhdcv5_mempoll = NULL;
static heap_handle_t lhdcv5_memhandle = NULL;
static int source_lhdcv5_meminit(void)
{
    CC_ENCODE_LOG_I("source_lhdcv5_meminit");
    source_lhdcv5_mempoll = lhdcv5_pool_buff;

    if(lhdcv5_memhandle == NULL)
        lhdcv5_memhandle = heap_register(source_lhdcv5_mempoll, LHDCV5_MEMPOOL_SIZE);

    return 1;
}

/****************************function defination****************************/
static void a2dp_encoder_cc_lhdc_init(void)
{
    int32_t     func_ret = LHDCV5_FRET_SUCCESS;
    uint32_t    mem_req_bytes = 0;

    CC_ENCODE_LOG_I("%s version %d", __func__, inst.version);
    func_ret = lhdcv5_util_enc_get_mem_req (inst.version, &mem_req_bytes);
    CC_ENCODE_LOG_I("%s: lhdcv5_util_get_mem_req (%d) mem_req_bytes %d\n", __func__, func_ret, mem_req_bytes);

    if (lhdcv5_memhandle == NULL)
    {
        source_lhdcv5_meminit();
    }
    inst.ptr = source_lhdcv5_mempoll;
    if (inst.ptr == NULL)
    {
        CC_ENCODE_LOG_E("malloc() err\n");
        return;
    }

    func_ret = lhdcv5_util_enc_get_handle (inst.version, inst.ptr, mem_req_bytes);
    CC_ENCODE_LOG_I("%s: lhdcv5_util_enc_get_handle (%d)\n", __func__, func_ret);

    CC_ENCODE_LOG_I("ptr %p sr %d bits_per_sample %d frm_drt %d mtu %d interval %d is_ll %d",
            inst.ptr,
            inst.sample_rate,
            inst.bits_per_sample,
            inst.frame_duration,
            inst.mtu,
            inst.interval,
            inst.islossless);

    func_ret = lhdcv5_util_init_encoder (inst.ptr,
                                         inst.sample_rate,
                                         inst.bits_per_sample,
                                         inst.bitrate_inx,
                                         inst.frame_duration,
                                         inst.mtu,
                                         inst.interval,
                                         inst.islossless);

    CC_ENCODE_LOG_I("%s: lhdcv5_util_init_encoder (%d)\n", __func__,func_ret);

    unsigned int hp_tmp=0;
    lhdcv5_util_enc_set_target_bitrate_inx(inst.ptr, inst.bitrate_inx, &hp_tmp, 1);
    CC_ENCODE_LOG_I("%s: bitrate_inx %d hp_tmp %d", __func__, inst.bitrate_inx, hp_tmp);
}


static void a2dp_encoder_cc_lhdc_deinit(void)
{
    CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
}


static bool a2dp_encoder_cc_lhdc_encode_frame(a2dp_encoder_cc_media_data_t* pPcmPacket,
    a2dp_encoder_cc_media_data_t* pEncodedPacket)
{
    CC_ENCODE_LOG_I("%s, %d, %d", __func__, __LINE__, pPcmPacket->data_len);

    uint32_t written = 0;
    uint32_t out_frames = 0;

    ASSERT(A2DP_LHDCV5_TRANS_SIZE == pPcmPacket->data_len, "pPcmPacket->data_len:%d error", pPcmPacket->data_len);
    int32_t res = lhdcv5_util_enc_process(  inst.ptr,
                                            pPcmPacket->data,
                                            A2DP_LHDCV5_TRANS_SIZE,
                                            pEncodedPacket->data,
                                            pEncodedPacket->data_len,
                                            &written,
                                            &out_frames);

    if (res != LHDCV5_FRET_SUCCESS)
    {
        CC_ENCODE_LOG_E("%s res %d", __func__, res);
        return false;
    }

    pEncodedPacket->data_len = written;
    if (written && out_frames)
    {
        //CC_ENCODE_LOG_I("%s dL %d w %d out %d", __func__, pEncodedPacket->data_len, written, out_frames);
        return true;
    }
    else
    {
        //CC_ENCODE_LOG_W("written %d out_frames %d", written, out_frames);
        return false;
    }

    return true;
}

const A2DP_AUDIO_CC_ENCODER_T* a2dp_encoder_lhdc_config()
{
    return &a2dp_audio_cc_lhdc_encoder_config;
}



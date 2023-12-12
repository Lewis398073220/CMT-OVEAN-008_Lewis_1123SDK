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
#include "aac_api.h"
#include "aac_error_code.h"
#include "a2dp_decoder_cc_common.h"
#include "a2dp_decoder_cc_off_bth.h"

#if defined(A2DP_AAC_PLC_ENABLED)
#include "sbcplc.h"

static PLC_State *aac_plc_state = NULL;
static PLC_State *aac_plc_state1 = NULL;
static float *aac_cos_buf = NULL;
#define AAC_PACKET_LEN (1024)
#define AAC_SMOOTH_LEN (AAC_PACKET_LEN*4)
#endif

/************************private macro defination***************************/
#define CC_AAC_OUTPUT_FRAME_SAMPLES        (1024)
#define CC_AAC_MEMPOOL_SIZE                (36*1024)
#define CC_AAC_READBUF_SIZE                (900)

/**********************private function declaration*************************/
static void a2dp_decoder_cc_aac_init(void);
static void a2dp_decoder_cc_aac_deinit(void);
static bool a2dp_decoder_cc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame);

/************************private variable defination************************/
static void * aacDec_handle = NULL;
static heap_handle_t aac_memhandle = NULL;
static uint8_t *a2dp_decoder_cc_codec_mem_pool = NULL;
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

A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_aac_decoder_config =
{
    {44100, 1},
    a2dp_decoder_cc_aac_init,
    a2dp_decoder_cc_aac_deinit,
    a2dp_decoder_cc_decode_frame,
    NULL,
    NULL,
};

/****************************function defination****************************/
static void a2dp_decoder_cc_aac_init(void)
{
    off_bth_syspool_get_buff(&a2dp_decoder_cc_codec_mem_pool, CC_AAC_MEMPOOL_SIZE);
#if defined(A2DP_AAC_PLC_ENABLED)
    off_bth_syspool_get_buff((uint8_t **)(&aac_plc_state), sizeof(PLC_State));
    off_bth_syspool_get_buff((uint8_t **)(&aac_plc_state1), sizeof(PLC_State));
    off_bth_syspool_get_buff((uint8_t **)(&aac_cos_buf), sizeof(float)*AAC_SMOOTH_LEN);

    a2dp_plc_init(aac_plc_state, A2DP_PLC_CODEC_TYPE_AAC);
    a2dp_plc_init(aac_plc_state1, A2DP_PLC_CODEC_TYPE_AAC);
    cos_generate(aac_cos_buf, AAC_SMOOTH_LEN, AAC_PACKET_LEN);
#endif
    ASSERT(a2dp_decoder_cc_codec_mem_pool, "%s size:%d", __func__, CC_AAC_MEMPOOL_SIZE);
    aac_memhandle = heap_register(a2dp_decoder_cc_codec_mem_pool, CC_AAC_MEMPOOL_SIZE);
    aac_heap_api = aac_get_heap_api();
    if (NULL == aacDec_handle)
    {
        aac_dec_para_t dec_para;
        dec_para.package = AAC_PACKAGE_MCP1;
#ifdef A2DP_AUDIO_STEREO_MIX_CTRL
        CC_DECODE_LOG_I("a2dp_decoder_cc_aac_init: stereo for mix");
        dec_para.channel_select = AAC_DECODER_CHANNEL_MODE_STEREO;
#else
        CC_DECODE_LOG_I("a2dp_decoder_cc_aac_init: ch: %d", a2dp_decoder_cc_get_channel());
        dec_para.channel_select = a2dp_decoder_cc_get_channel();
#endif
        aacDec_handle = aac_decoder_open(aac_heap_api, dec_para);
        ASSERT(aacDec_handle, "aacDecoder_Open failed");
    }
}

static void a2dp_decoder_cc_aac_deinit(void)
{
    if (aacDec_handle)
    {
        aac_decoder_close(aacDec_handle);
        aacDec_handle = NULL;
    }
}

static void a2dp_decoder_cc_aac_reinit(void)
{
    if (aacDec_handle)
    {
        a2dp_decoder_cc_aac_deinit();
    }
    a2dp_decoder_cc_aac_init();
}

static bool a2dp_decoder_cc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame)
{
    // TODO: support multiple codec types
    bool retVal = false;

    if (!aacDec_handle)
    {
        CC_DECODE_LOG_I("aac decoder is not ready");
        return false;
    }

    uint32_t aac_maxreadBytes = CC_AAC_READBUF_SIZE;
    aac_frame_t aac_data;
    aac_pcm_frame_t pcm_data;
    int decoder_err;
    aac_streaminfo_t stream_info;

    if (pEncodedPacket->data_len < 64)
    {
        aac_maxreadBytes = 64;
    }
    else if (pEncodedPacket->data_len < 128)
    {
        aac_maxreadBytes = 128;
    }
    else if (pEncodedPacket->data_len < 256)
    {
        aac_maxreadBytes = 256;
    }
    else if (pEncodedPacket->data_len < 512)
    {
        aac_maxreadBytes = 512;
    }
    else if (pEncodedPacket->data_len < 1024)
    {
        aac_maxreadBytes = 1024;
    }

    aac_data.aac_data = pEncodedPacket->data;
    aac_data.buffer_size = aac_maxreadBytes;
    aac_data.valid_size = aac_maxreadBytes;
    pcm_data.pcm_data = (short*)pPcmFrame->data;
    pcm_data.buffer_size = pPcmFrame->data_len;
    pcm_data.valid_size = 0;
    /* decode one AAC frame */
    decoder_err = aac_decoder_process_frame(aacDec_handle, &aac_data, &pcm_data);

#if defined(A2DP_AAC_PLC_ENABLED)
    uint8_t chan;
    chan = a2dp_decoder_cc_get_channel();
    if (A2DP_AUDIO_CHANNEL_SELECT_STEREO == chan) {
        if (pEncodedPacket->isPLC) {
            a2dp_plc_bad_frame(aac_plc_state, (short *)pPcmFrame->data, (short *)pPcmFrame->data, aac_cos_buf, AAC_PACKET_LEN, 2, 0);
            a2dp_plc_bad_frame(aac_plc_state1, (short *)pPcmFrame->data, (short *)pPcmFrame->data, aac_cos_buf, AAC_PACKET_LEN, 2, 1);
        } else {
            a2dp_plc_good_frame(aac_plc_state, (short *)pPcmFrame->data, (short *)pPcmFrame->data, aac_cos_buf, AAC_PACKET_LEN, 2, 0);
            a2dp_plc_good_frame(aac_plc_state1, (short *)pPcmFrame->data, (short *)pPcmFrame->data, aac_cos_buf, AAC_PACKET_LEN, 2, 1);
        }
    } else {
        if (pEncodedPacket->isPLC) {
            CC_DECODE_LOG_W("AAC PLC bad frame");
            a2dp_plc_bad_frame(aac_plc_state, (short *)pPcmFrame->data, (short *)pPcmFrame->data, aac_cos_buf, AAC_PACKET_LEN, 2, chan-2);
            a2dp_plc_bad_frame_smooth(aac_plc_state1, (short *)pPcmFrame->data, (short *)pPcmFrame->data, 2, 3-chan);
        } else {
            a2dp_plc_good_frame(aac_plc_state, (short *)pPcmFrame->data, (short *)pPcmFrame->data, aac_cos_buf, AAC_PACKET_LEN, 2, chan-2);
            a2dp_plc_good_frame_smooth(aac_plc_state1, (short *)pPcmFrame->data, (short *)pPcmFrame->data, 2, 3-chan);
        }
    }
#endif
    CC_DECODE_LOG_D("decoder seq:%d len:%d pcm len:%u err:%x",
        pEncodedPacket->pkt_seq_nb,
        pEncodedPacket->data_len,
        pPcmFrame->data_len/2,
        decoder_err);

    if (AAC_OK != decoder_err)
    {
        CC_DECODE_LOG_I("aac_lc_decode failed:0x%x", decoder_err);
        //if aac failed reopen it again
        if(aac_decoder_handle_valid(aacDec_handle))
        {
            a2dp_decoder_cc_aac_reinit();
            CC_DECODE_LOG_I("aac_lc_decode reinin codec \n");
        }
        goto end_decode;
    }

    aac_decoder_get_info(aacDec_handle, &stream_info);
    if (stream_info.sample_rate <= 0)
    {
        CC_DECODE_LOG_I("aac_lc_decode invalid stream info");
        goto end_decode;
    }

    ASSERT(CC_AAC_OUTPUT_FRAME_SAMPLES == stream_info.frame_size, "aac_lc_decode output mismatch samples:%d", stream_info.frame_size);

    retVal = true;

end_decode:

    return retVal;
}


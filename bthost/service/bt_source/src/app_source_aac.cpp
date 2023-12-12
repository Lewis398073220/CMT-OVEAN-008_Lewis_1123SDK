/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifdef BT_A2DP_SUPPORT
#ifdef A2DP_SOURCE_AAC_ON
#include "app_source_codec.h"
#include "a2dp_codec_aac.h"
#include "aac_api.h"
#include "aac_error_code.h"
#include "hal_location.h"

#define AAC_OUT_SIZE 1024*2
static void * SRAM_BSS_LOC aacEnc_handle = NULL;
static int8_t SRAM_BSS_LOC aacout_buffer[AAC_OUT_SIZE];

#define AAC_MEMPOLL_SIZE (70*1024)
static uint8_t  SRAM_BSS_LOC aac_pool_buff[AAC_MEMPOLL_SIZE];
static uint8_t * SRAM_DATA_LOC  source_aac_mempoll = NULL;

static char SRAM_BSS_LOC a2dp_aac_transmit_buffer[A2DP_AAC_TRANS_SIZE];

uint8_t *a2dp_source_aac_frame_buffer(void)
{
    return (uint8_t *)a2dp_aac_transmit_buffer;
}

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
#define MAX_SOURCE_AAC_BITRATE 128000

#if 0 ///for test 
static int _aac_count=0;
static int _aac_time_ms0=0;
static int _aac_sample_rate=0;
static int _aac_sample_ch=0;
#endif
int aacenc_init(void)
{
    if (aacEnc_handle == NULL) {
        int  sample_rate, channels;
        int aot = 129;//2;//- 129: MPEG-2 AAC Low Complexity.
        int vbr = 0;
        int bitrate =MAX_SOURCE_AAC_BITRATE;
        ///get media param from curr device
        uint8_t device_id = app_bt_source_get_streaming_a2dp();
        struct BT_SOURCE_DEVICE_T *curr_device = NULL;	
        if (device_id == BT_SOURCE_DEVICE_INVALID_ID){
            return false;
        }	
        curr_device = app_bt_source_get_device(device_id);
        ///set media param from curr device
        channels = curr_device->base_device->a2dp_channel_num;
        sample_rate = curr_device->aud_sample_rate;
        vbr = curr_device->base_device->vbr_support;
        bitrate = curr_device->aud_bit_rate;
        int sample_bits = curr_device->base_device->sample_bit;
        TRACE(2,"\n@@@aacenc_init:rate=%d,ch=%d,br=%d,vbr=%d,bits=%d,%dms\n"
            ,sample_rate,channels
            ,(int)bitrate
            ,(int)vbr
            ,(int)sample_bits	
            ,(int)1024000/sample_rate
        );

        TRACE(1,"start aacEncOpen sample_rate=%d",sample_rate);
        if (source_aac_meminit() < 0)
        {
            TRACE(0,"aac_meminit error\n");
            return 1;
        }
        aac_enc_para_t enc_para;
        enc_para.aot = aot;
        enc_para.vbr = vbr;
        enc_para.sample_rate = sample_rate;
        enc_para.channels = channels;
        enc_para.bitrate = bitrate;
        enc_para.package = AAC_PACKAGE_MCP1;
        aacEnc_handle = aac_encoder_open(aac_heap_api, enc_para);
        if (aacEnc_handle == NULL) {
            TRACE(1,"Unable to open encoder \n");
            return 1;
        }
    }
    return 0;
}

int aacenc_deinit(void)
{
    if (aacEnc_handle != NULL) {
        aac_encoder_close(&aacEnc_handle);
        aacEnc_handle = NULL;
    }
    return 0;
}

bool a2dp_source_encode_aac_packet(a2dp_source_packet_t *source_packet)
{
    btif_a2dp_sbc_packet_t *packet = &(source_packet->packet);
    int lock = int_lock();
    aacenc_init();
#if 0 ///for test
    unsigned t0 = hal_fast_sys_timer_get();   
#endif
    aac_pcm_frame_t pcm_data;
    pcm_data.pcm_data = (short *)(a2dp_aac_transmit_buffer);
    pcm_data.buffer_size = A2DP_AAC_TRANS_SIZE;
    pcm_data.valid_size = A2DP_AAC_TRANS_SIZE;
    aac_frame_t aac_data;
    aac_data.aac_data = (uint8_t*)aacout_buffer;
    aac_data.buffer_size =  AAC_OUT_SIZE;
    aac_data.valid_size = 0;
    int err = aac_encoder_process_frame(aacEnc_handle, &pcm_data, &aac_data);
#if 0 ///for test
	unsigned t1 = hal_fast_sys_timer_get();
#endif
    int_unlock(lock);
#if 0 ///for test
    {
        unsigned ms = FAST_TICKS_TO_MS(t0);
        if(_aac_time_ms0==0)_aac_time_ms0=ms;	
        ms-=_aac_time_ms0;
        TRACE(1,"%dms,%d,%u(%u,%u)(%u,%u)",FAST_TICKS_TO_MS(t1-t0),(int)out_args.numOutBytes,_aac_count
            ,ms,_aac_count*1024000u/_aac_sample_rate
            ,_aac_count?(ms/_aac_count):0,1024000u/_aac_sample_rate
        );
        _aac_count++;
        DUMP8("%02x ",aacout_buffer,out_args.numOutBytes);
    }
#endif
    if(err != AAC_OK || aac_data.valid_size <= 0)
    {
        TRACE(2,"aac_encoder_process_frame err %x len %d", err, aac_data.valid_size);
        return false;
    }
    packet->dataLen = aac_data.valid_size;
    packet->frameSize = (A2DP_AAC_TRANS_SIZE - pcm_data.valid_size)/2;
    ASSERT(packet->dataLen < packet->reserved_data_size, "aac encodec packet length too long");

    memcpy(packet->data, aacout_buffer, packet->dataLen);

    source_packet->codec_type = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
    return true;
}

static btif_avdtp_codec_t a2dp_source_aac_avdtpcodec;

#if BT_SOURCE_DEVICE_NUM > 1
static const unsigned char a2dp_source_codec_aac_elements[A2DP_AAC_OCTET_NUMBER] = {
    A2DP_AAC_OCTET0_MPEG2_AAC_LC,
    A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100,
    A2DP_AAC_OCTET2_CHANNELS_2,
    /* A2DP_AAC_OCTET3_VBR_SUPPORTED |*/ ((MAX_SOURCE_AAC_BITRATE >> 16) & 0x7f),
    (MAX_SOURCE_AAC_BITRATE >> 8) & 0xff,
    (MAX_SOURCE_AAC_BITRATE) & 0xff
};
#else
static const unsigned char a2dp_source_codec_aac_elements[A2DP_AAC_OCTET_NUMBER] = {
    A2DP_AAC_OCTET0_MPEG2_AAC_LC,
    A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100,
    A2DP_AAC_OCTET2_CHANNELS_1 | A2DP_AAC_OCTET2_CHANNELS_2 /*| A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000*/,
    /* A2DP_AAC_OCTET3_VBR_SUPPORTED |*/ ((MAX_SOURCE_AAC_BITRATE >> 16) & 0x7f),
    (MAX_SOURCE_AAC_BITRATE >> 8) & 0xff,
    (MAX_SOURCE_AAC_BITRATE) & 0xff
};
#endif

void a2dp_source_register_aac_codec(btif_a2dp_stream_t *btif_a2dp, btif_avdtp_content_prot_t *sep_cp, uint8_t sep_priority, btif_a2dp_callback callback)
{
    a2dp_source_aac_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
    a2dp_source_aac_avdtpcodec.discoverable = 1;
    a2dp_source_aac_avdtpcodec.elements = (U8 *)&a2dp_source_codec_aac_elements;
    a2dp_source_aac_avdtpcodec.elemLen  = sizeof(a2dp_source_codec_aac_elements);

    btif_a2dp_register(btif_a2dp, BTIF_A2DP_STREAM_TYPE_SOURCE, &a2dp_source_aac_avdtpcodec, sep_cp, sep_priority, callback);
}

#endif /* A2DP_SOURCE_AAC_ON */
#endif /* BT_A2DP_SUPPORT */
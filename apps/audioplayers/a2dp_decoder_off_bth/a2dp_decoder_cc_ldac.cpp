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
#include "a2dp_decoder_cc_off_bth.h"
#include "btapp.h"
#include "ldacBT.h"
#include "bes_mem_api.h"

#ifndef LDAC_MTU_LIMITER
#define LDAC_MTU_LIMITER (200)
#endif
#define DECODE_LDAC_PCM_FRAME_LENGTH  (256*4*2*2)

#define LDAC_LIST_SAMPLES (256)

// #define CC_LDAC_MEMPOOL_SIZE                (72*1024)


/**********************private function declaration*************************/
static void a2dp_decoder_cc_ldac_init(void);
static void a2dp_decoder_cc_ldac_deinit(void);
static bool a2dp_decoder_cc_ldac_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame);


/************************private variable defination************************/
static HANDLE_LDAC_BT LdacDecHandle = NULL;
// heap_handle_t ldac_memhandle = NULL;
// static uint8_t *a2dp_decoder_cc_codec_mem_pool = NULL;

A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_ldac_decoder_config =
{
    {44100, 1},
    a2dp_decoder_cc_ldac_init,
    a2dp_decoder_cc_ldac_deinit,
    a2dp_decoder_cc_ldac_decode_frame,
};

/****************************function defination****************************/
static void a2dp_decoder_cc_ldac_init(void)
{
    CC_DECODE_LOG_I("%s %d %d", __func__, a2dp_decoder_cc_get_channnel_mode(), a2dp_decoder_cc_get_sample_rate());

    int result;


    if(LdacDecHandle==NULL)
    {
        ldac_cc_mem_init();
        //syspool_init();
        // off_bth_syspool_get_buff(&a2dp_decoder_cc_codec_mem_pool, CC_LDAC_MEMPOOL_SIZE);
        // ASSERT(a2dp_decoder_cc_codec_mem_pool, "%s size:%d", __func__, CC_LDAC_MEMPOOL_SIZE);
        // ldac_memhandle = heap_register(a2dp_decoder_cc_codec_mem_pool, CC_LDAC_MEMPOOL_SIZE);

        if (( LdacDecHandle = ldacBT_get_handle()) == (HANDLE_LDAC_BT)NULL)
        {
            CC_DECODE_LOG_I("Error: Can not Get LDAC Handle!\n");
            osDelay(1);
        }

    }

    result = ldacBT_init_handle_decode(LdacDecHandle,a2dp_decoder_cc_get_channnel_mode(),a2dp_decoder_cc_get_sample_rate(),0,0,0);

    if(result != LDACBT_ERR_NONE)
    {
        CC_DECODE_LOG_I("ldacBT init failed,  Error code is %d", ldacBT_get_error_code(LdacDecHandle));
    }

}


static void a2dp_decoder_cc_ldac_deinit(void)
{
    if(LdacDecHandle != NULL)
    {
        ldacBT_free_handle(LdacDecHandle);
        LdacDecHandle = NULL;
    }
}


static uint8_t decoded_buf[4096] = {0};

static int a2dp_decoder_cc_ldac_convert_24bit_to_4byte(char * buf, int lens)
{
    int i = 0;
    int * p = (int*)(buf+lens/3*4 - 4);
    int val;
    /* start processing from the end of buf, no need to apply for memory */
    for(i=lens-3; i>=0; i=i-3, p--){
        val  = 0x000000ff & (buf[i]);
        val |= 0x0000ff00 & (buf[i+1] << 8);
        val |= 0xffff0000 & (buf[i+2] << 16);
        *p = (int)(val);
    }
    return lens/3*4;
}

static bool a2dp_decoder_cc_ldac_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame)
{
    CC_DECODE_LOG_D("%s dataLen = %d %d",__func__, pEncodedPacket->data_len, pPcmFrame->data_len);

    POSSIBLY_UNUSED int used_bytes = 0;
    int result;

    int encoded_len = pEncodedPacket->data_len;
    int wrote_bytes = 0;

    POSSIBLY_UNUSED uint8_t * temp_buf_out = pPcmFrame->data;

    if(LdacDecHandle){
        result = ldacBT_decode(LdacDecHandle,
                                pEncodedPacket->data,
                                decoded_buf,
                                LDACBT_SMPL_FMT_S24,
                                encoded_len,
                                (int *)&used_bytes,
                                (int *)&wrote_bytes);
    } else{
        ASSERT(0,"LdacDecHandle = NULL!");
    }

    if (result != LDACBT_ERR_NONE)
    {
        CC_DECODE_LOG_I("ldacBT decode failed[%d], Error code is %d", result,ldacBT_get_error_code(LdacDecHandle));

    }else{
        memcpy(pPcmFrame->data,decoded_buf,wrote_bytes);
        pPcmFrame->data_len = wrote_bytes;
	pPcmFrame->data_len = a2dp_decoder_cc_ldac_convert_24bit_to_4byte((char*)pPcmFrame->data,pPcmFrame->data_len);
    }

    return true;
}





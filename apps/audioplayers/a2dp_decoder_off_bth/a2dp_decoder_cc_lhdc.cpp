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
#include"lhdcUtil.h"
#include "lhdcBesCCUtil.h"
#include "dwt.h"

#ifndef LHDC_MTU_LIMITER
#define LHDC_MTU_LIMITER (210)
#endif
#define DECODE_LHDC_PCM_FRAME_LENGTH  (256*4*2*2)

#define LHDC_LIST_SAMPLES (256)

#define LHDC_EXT_FLAGS_LLAC   0x04
#define LHDC_EXT_FLAGS_V4     0x40
#define LHDC_DWT_DEBUG

#define BES_MEASURE_DECODE_TIMEx
extern "C"
{
    bool chkLhdcHandleExist(void);
    bool lhdcSetLicenseKeyTable_dsp(uint8_t* lhdc_data);
    uint8_t lhdcGetLicenseKeyResult(void);
    uint8_t lhdcGetVersionResult(void);
    void lhdcSetLicenseKeyChkPeriod (uint8_t period);
    uint8_t getLhdcData_dsp(void);
    uint32_t lhdcChannelGet(void);
    uint32_t lhdcChannelSelsect(lhdc_channel_t channel_type);

}
/**********************private function declaration*************************/
static void a2dp_decoder_cc_lhdc_init(void);
static void a2dp_decoder_cc_lhdc_deinit(void);
static bool a2dp_decoder_cc_lhdc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame);
int a2dp_audio_lhdc_channel_select(A2DP_DECODER_CHANNEL_SELECT_E chnl_sel);


/************************private variable defination************************/


A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_lhdc_decoder_config =
{
    {44100, 1},
    a2dp_decoder_cc_lhdc_init,
    a2dp_decoder_cc_lhdc_deinit,
    a2dp_decoder_cc_lhdc_decode_frame,
};

/****************************function defination****************************/
static void a2dp_decoder_cc_lhdc_init(void)
{
    hal_sysfreq_req(HAL_SYSFREQ_USER_DSP, HAL_CMU_FREQ_104M);
    uint8_t *pLhdc_data = a2dp_decoder_cc_get_lhdc_data();
        CC_DECODE_LOG_D("[BRUCE DEBUG][M55] LIC first 8 byte Received:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ", pLhdc_data[0], \
        pLhdc_data[1], \
        pLhdc_data[2], \
        pLhdc_data[3], \
        pLhdc_data[4], \
        pLhdc_data[5], \
        pLhdc_data[6], \
        pLhdc_data[7]
        ); 
    uint8_t lhdc_license_key = 0;
    A2DP_DECODER_CHANNEL_SELECT_E channel_sel = (A2DP_DECODER_CHANNEL_SELECT_E)a2dp_decoder_cc_get_channel_sel();

    lhdc_ver_t version = VERSION_3;
    if( (a2dp_decoder_cc_get_lhdc_ext_flag() & (LHDC_EXT_FLAGS_LLAC | LHDC_EXT_FLAGS_V4)) == (LHDC_EXT_FLAGS_LLAC | LHDC_EXT_FLAGS_V4) ){
        //LHDCV4 + LLAC
        if (a2dp_decoder_cc_get_sample_rate() <= 48000) {
            version = VERSION_LLAC;
        }else{
            version = VERSION_4;
        }
        CC_DECODE_LOG_I("%s: LLAC & LHDCV4 ", __func__);
    }else if ( (a2dp_decoder_cc_get_lhdc_ext_flag() & LHDC_EXT_FLAGS_LLAC) == LHDC_EXT_FLAGS_LLAC ) {
        //LLAC only
        if(a2dp_decoder_cc_get_sample_rate() == 96000) {
            version = VERSION_4;
            CC_DECODE_LOG_I("%s: LHDC V4 only", __func__);
        }else {
            version = VERSION_LLAC;
            CC_DECODE_LOG_I("%s: LLAC only", __func__);
        }
    }else if ((a2dp_decoder_cc_get_lhdc_ext_flag() & LHDC_EXT_FLAGS_V4) == LHDC_EXT_FLAGS_V4) {
        //LHDCV4 only
        version = VERSION_4;
        CC_DECODE_LOG_I("%s: LHDC V4 only", __func__);
    }else if (! ( ( a2dp_decoder_cc_get_lhdc_ext_flag() & (LHDC_EXT_FLAGS_LLAC | LHDC_EXT_FLAGS_V4) ) == (LHDC_EXT_FLAGS_LLAC | LHDC_EXT_FLAGS_V4) ) ) {
        //LHDCV3 only
        CC_DECODE_LOG_I("%s: LHDC V3 only", __func__);
    }                                                                                                                                                                                              
    lhdc_license_key = lhdcSetLicenseKeyTable_dsp(pLhdc_data);
    if(lhdc_license_key) { 
        CC_DECODE_LOG_I("[BRUCE DEBUG][M55] LHDC KEY OK"); 
    }
    else { 
        CC_DECODE_LOG_W("[BRUCE DEBUG][M55] LHDC KEY ERROR"); 
    }

    CC_DECODE_LOG_I("%s: [BRUCE DEBUG][M55] sample rate:%d bit depth:%d, version:%d", __func__, a2dp_decoder_cc_get_sample_rate(),a2dp_decoder_cc_get_bits_depth(), version);

    lhdcInit(a2dp_decoder_cc_get_bits_depth(), a2dp_decoder_cc_get_sample_rate(), 400000, version);
    lhdcSetLicenseKeyChkPeriod(10);
    a2dp_audio_lhdc_channel_select(channel_sel);

    
    //CC_DECODE_LOG_D("[BRUCE DEBUG][M55] license check:%d  version check:%d", lhdcGetLicenseKeyResult(), lhdcGetVersionResult());
}


static void a2dp_decoder_cc_lhdc_deinit(void)
{
    lhdcDestroy();
}


static uint8_t decoded_buf[1024*4] = {0};

static bool a2dp_decoder_cc_lhdc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame)
{
#if 0
    CC_DECODE_LOG_W("[BRUCE DEBUG][M55] encoded data:\n");
    for(int i = 0; ((i*8+7) < 16/*pEncodedPacket->data_len*/); i++){
         CC_DECODE_LOG_W("0x%02X|  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",i*8,pEncodedPacket->data[i*8+0],\
                                                                                                 pEncodedPacket->data[i*8+1],\
                                                                                                 pEncodedPacket->data[i*8+2],\
                                                                                                 pEncodedPacket->data[i*8+3],\
                                                                                                 pEncodedPacket->data[i*8+4],\
                                                                                                 pEncodedPacket->data[i*8+5],\
                                                                                                 pEncodedPacket->data[i*8+6],\
                                                                                                 pEncodedPacket->data[i*8+7]);
    }
#endif

    POSSIBLY_UNUSED int encoded_len = pEncodedPacket->data_len;
    int wrote_bytes = 0;

    POSSIBLY_UNUSED uint8_t * temp_buf_out = pPcmFrame->data;
#ifdef BES_MEASURE_DECODE_TIME        
        uint32_t start_time = hal_sys_timer_get(); 
#endif

    lhdc_frame_Info_t lhdc_frame_Info_dsp;
    memset(&lhdc_frame_Info_dsp, 0, sizeof(lhdc_frame_Info_dsp));
    int lhdc_should_decode = 1;
#ifdef BES_MEASURE_DECODE_TIME
    uint32_t start_time = hal_sys_timer_get();
#endif

#ifdef LHDC_DWT_DEBUG
    watchpoint_enable(0, (uint32_t *)&pEncodedPacket->data, WPT_WRITE);
#endif

    lhdcFetchFrameInfo(pEncodedPacket->data, &lhdc_frame_Info_dsp);
    if(lhdc_frame_Info_dsp.frame_len){
        if(lhdc_frame_Info_dsp.frame_len != pEncodedPacket->data_len){
            CC_DECODE_LOG_W("%s frame_info:%d data_len:%d", __func__, \
            lhdc_frame_Info_dsp.frame_len, pEncodedPacket->data_len);
            lhdc_should_decode = 0;
        }
    } else{
        lhdc_should_decode = 0;
    }

    if(lhdc_should_decode == 1){
        wrote_bytes = lhdcDecodeProcess(decoded_buf, pEncodedPacket->data, encoded_len);
    } else {
        wrote_bytes = pPcmFrame->data_len;
        CC_DECODE_LOG_I("%s wrote_bytes:%d", __func__, wrote_bytes);
    }
#ifdef LHDC_DWT_DEBUG
    watchpoint_disable(0);
#endif

#ifdef BES_MEASURE_DECODE_TIME        
        uint32_t end_time = hal_sys_timer_get();
        uint32_t deltaMs = TICKS_TO_US(end_time - start_time);
        TRACE(2, ":[BRUCE DEBUG][M55][DECODE TIME] LHDC Decode time %d uS. ch mode:%d", deltaMs,lhdcChannelGet());
#endif
    if(wrote_bytes > 0 && wrote_bytes <= pPcmFrame->data_len)
    {

#if 0
        for(int i = 0; ((i*8+7) < wrote_bytes>>3); i++){
         CC_DECODE_LOG_W("0x%02X|  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",i*8,decoded_buf[i*8+0],\
                                                                                                 decoded_buf[i*8+1],\
                                                                                                 decoded_buf[i*8+2],\
                                                                                                 decoded_buf[i*8+3],\
                                                                                                 decoded_buf[i*8+4],\
                                                                                                 decoded_buf[i*8+5],\
                                                                                                 decoded_buf[i*8+6],\
                                                                                                 decoded_buf[i*8+7]);
    }

#endif
        memcpy(pPcmFrame->data,decoded_buf,wrote_bytes);
        pPcmFrame->data_len = wrote_bytes;
    } else 
    {
//ASSERT(0,"LHDC WRONG DECODED BYTES: %d/%d", wrote_bytes,pPcmFrame->data_len);
        CC_DECODE_LOG_E("[Bruce Debug][M55] LHDC WRONG DECODED BYTES: %d/%d", wrote_bytes,pPcmFrame->data_len);
        memcpy(pPcmFrame->data,decoded_buf,pPcmFrame->data_len);
    }
    return true;
}

int a2dp_audio_lhdc_channel_select(A2DP_DECODER_CHANNEL_SELECT_E chnl_sel)
{
    lhdc_channel_t channel_type;
    switch(chnl_sel){
        case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
        channel_type = LHDC_OUTPUT_STEREO;
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] channel select stereo.");
        break;
        case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
        channel_type = LHDC_OUTPUT_LEFT_CAHNNEL;
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] channel select L channel.");
        break;
        case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
        channel_type = LHDC_OUTPUT_RIGHT_CAHNNEL;
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] channel select R channel.");
        break;

        default:
        case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
        TRACE_A2DP_DECODER_I("[BRUCE DEBUG][M55] Unsupported channel config(%d).", chnl_sel);
        return A2DP_DECODER_NOT_SUPPORT;
    }
    lhdcChannelSelsect(channel_type);
    return A2DP_DECODER_NO_ERROR;
}





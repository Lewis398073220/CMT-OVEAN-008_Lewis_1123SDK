/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#include "a2dp_decoder_internal.h"
#include "sbc_api.h"
#include "sbc_error_code.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "a2dp_decoder_cc_common.h"
#include "a2dp_decoder_cc_off_bth.h"

#if defined(A2DP_SBC_PLC_ENABLED)
#include "sbcplc.h"

static PLC_State *sbc_plc_state = NULL;
static PLC_State *sbc_plc_state1 = NULL;
static float *sbc_cos_buf = NULL;
#define SBC_PACKET_LEN (128*5)
#define SBC_SMOOTH_LEN (SBC_PACKET_LEN*4)
#endif
/************************private macro defination***************************/

/************************private type defination****************************/
static sbc_decoder_t *cc_sbc_decoder = NULL;

/**********************private function declaration*************************/
static void a2dp_audio_cc_sbc_init(void)
{
    CC_DECODE_LOG_I("%s", __func__);

    off_bth_syspool_get_buff((uint8_t **)(&cc_sbc_decoder), sizeof(sbc_decoder_t));
#if defined(A2DP_SBC_PLC_ENABLED)
    off_bth_syspool_get_buff((uint8_t **)(&sbc_plc_state), sizeof(PLC_State));
    off_bth_syspool_get_buff((uint8_t **)(&sbc_plc_state1), sizeof(PLC_State));
    off_bth_syspool_get_buff((uint8_t **)(&sbc_cos_buf), sizeof(float)*SBC_SMOOTH_LEN);

    a2dp_plc_init(sbc_plc_state, A2DP_PLC_CODEC_TYPE_SBC);
    a2dp_plc_init(sbc_plc_state1, A2DP_PLC_CODEC_TYPE_SBC);
    cos_generate(sbc_cos_buf, SBC_SMOOTH_LEN, SBC_PACKET_LEN);
#endif
    ASSERT(cc_sbc_decoder, "%s", __func__);

    sbc_decoder_open(cc_sbc_decoder);
}

void a2dp_audio_cc_sbc_deinit(void)
{
    memset(cc_sbc_decoder, 0, sizeof(sbc_decoder_t));
}

static bool a2dp_audio_cc_sbc_decode_frame(a2dp_decoder_cc_media_data_t* pEncodedPacket,
    a2dp_decoder_cc_media_data_t* pPcmFrame)
{
    int ret = SBC_OK;
    pcm_frame_t pcm_data;
    pcm_data.pcm_data = (int16_t *)pPcmFrame->data;
    pcm_data.valid_size = 0;
    pcm_data.buffer_size = CC_DECODE_SBC_PCMLEN_DEFAULT;
    sbc_frame_t sbc_data;
    sbc_data.sbc_data = pEncodedPacket->data,
    sbc_data.buffer_size = pEncodedPacket->data_len;
    sbc_data.valid_size = pEncodedPacket->data_len;
    ret = sbc_decoder_process_frame(cc_sbc_decoder, &sbc_data, &pcm_data);

#if defined(A2DP_SBC_PLC_ENABLED)
    uint8_t chan;
    chan = a2dp_decoder_cc_get_channel();
    if (A2DP_AUDIO_CHANNEL_SELECT_STEREO == chan) {
        if (pEncodedPacket->isPLC) {
            a2dp_plc_bad_frame(sbc_plc_state, (short *)pPcmFrame->data, (short *)pPcmFrame->data, sbc_cos_buf, SBC_PACKET_LEN, 2, 0);
            a2dp_plc_bad_frame(sbc_plc_state1, (short *)pPcmFrame->data, (short *)pPcmFrame->data, sbc_cos_buf, SBC_PACKET_LEN, 2, 1);
        } else {
            a2dp_plc_good_frame(sbc_plc_state, (short *)pPcmFrame->data, (short *)pPcmFrame->data, sbc_cos_buf, SBC_PACKET_LEN, 2, 0);
            a2dp_plc_good_frame(sbc_plc_state1, (short *)pPcmFrame->data, (short *)pPcmFrame->data, sbc_cos_buf, SBC_PACKET_LEN, 2, 1);
        }
    } else {
        if(pEncodedPacket->isPLC) {
            TRACE(0,"SBC PLC bad frame");
            a2dp_plc_bad_frame(sbc_plc_state, (short *)pcm_data.pcm_data, (short *)pcm_data.pcm_data, sbc_cos_buf, SBC_PACKET_LEN, 2, chan-2);
            a2dp_plc_bad_frame_smooth(sbc_plc_state1, (short *)pcm_data.pcm_data, (short *)pcm_data.pcm_data, 2, 3-chan);
        } else {
            a2dp_plc_good_frame(sbc_plc_state, (short *)pcm_data.pcm_data, (short *)pcm_data.pcm_data, sbc_cos_buf, SBC_PACKET_LEN, 2, chan-2);
            a2dp_plc_good_frame_smooth(sbc_plc_state1, (short *)pcm_data.pcm_data, (short *)pcm_data.pcm_data, 2, 3-chan);
        }
    }
#endif
    //TRACE(0, "chan = %d, len = %d", chan, pcm_data.dataLen);
    pPcmFrame->data_len = pcm_data.valid_size;

    return  ret == BT_STS_SUCCESS ? true: false;
}

A2DP_AUDIO_CC_DECODER_T a2dp_audio_cc_sbc_decoder_config =
{
    {44100, 1},
    a2dp_audio_cc_sbc_init,
    a2dp_audio_cc_sbc_deinit,
    a2dp_audio_cc_sbc_decode_frame,
    NULL,
    NULL,
};


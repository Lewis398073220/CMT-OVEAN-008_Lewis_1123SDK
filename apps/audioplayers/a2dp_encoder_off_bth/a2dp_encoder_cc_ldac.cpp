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
#if 1
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
#include "dwt.h"
#include "ldacBT.h"
#include "bes_mem_api.h"

/**********************private function declaration*************************/
static void a2dp_encoder_cc_ldac_init(void);
static void a2dp_encoder_cc_ldac_deinit(void);
static bool a2dp_encoder_cc_ldac_encode_frame(a2dp_encoder_cc_media_data_t* pDecodedPacket,
    a2dp_encoder_cc_media_data_t* pEncodedFrame);


static HANDLE_LDAC_BT hData = 0;
/************************private variable defination************************/

static const A2DP_AUDIO_CC_ENCODER_T a2dp_audio_cc_ldac_encoder_config =
{
    {44100, 1},
    a2dp_encoder_cc_ldac_init,
    a2dp_encoder_cc_ldac_deinit,
    a2dp_encoder_cc_ldac_encode_frame,
};

/****************************function defination****************************/
static void a2dp_encoder_cc_ldac_init(void)
{
    unsigned freq = hal_sys_timer_calc_cpu_freq(5, 0);
    CC_ENCODE_LOG_W("%s, %d, %d", __func__, __LINE__, freq);
	ldac_cc_mem_init();
    hData = ldacBT_get_handle();
	if(!hData)
	{
		CC_ENCODE_LOG_I("Error: Can not Get LDAC Handle!");
		return;
	}

	/* Initialize for Encoding */
	int ret = ldacBT_init_handle_encode(hData, 679, LDACBT_EQMID_HQ, LDACBT_CHANNEL_MODE_STEREO, LDACBT_SMPL_FMT_S32, 96000);
	if(ret)
	{
		CC_ENCODE_LOG_I("Initializing LDAC Handle for analysis!");
		return;
	}
	//CC_ENCODE_LOG_I("Initializing LDAC done, %d, %d", eqmid, cm);

    return;
}


static void a2dp_encoder_cc_ldac_deinit(void)
{
    CC_ENCODE_LOG_I("%s, %d", __func__, __LINE__);
    if (hData) {
        ldacBT_free_handle(hData);
        hData = 0;
    }

}

static bool a2dp_encoder_cc_ldac_encode_frame(a2dp_encoder_cc_media_data_t* pPcmPacket,
    a2dp_encoder_cc_media_data_t* pEncodedPacket)
{
    int written = 0;
	int frame_num = 0;
	uint8_t ret = 0;
    int used_pcm_size = 0;
    unsigned freq = hal_sys_timer_calc_cpu_freq(5, 0);
    CC_ENCODE_LOG_W("%s, %d, %d", __func__, __LINE__, freq);
    int32_t remain_len = pPcmPacket->data_len;
    if (hData) {
        while(remain_len > 0) {
            ret = ldacBT_encode(hData, pPcmPacket->data, &used_pcm_size, (unsigned char *)pEncodedPacket->data, &written, &frame_num);
            if (ret) break;
            remain_len -= used_pcm_size;
        }
    }

	if (ret) {
		CC_ENCODE_LOG_I("[ldac] LDAC encode error: %d", ret);
        return false;
	} else {
		CC_ENCODE_LOG_I("encode_size: %d, %d, %d", pPcmPacket->data_len, used_pcm_size, frame_num);
	}    

    pEncodedPacket->data_len = written;
    if (written && frame_num)
    {
        CC_ENCODE_LOG_I("%s dL %d w %d out %d", __func__, pEncodedPacket->data_len, written, frame_num);
        return true;
    }
    else
    {
        CC_ENCODE_LOG_I("written %d frame_num %d", written, frame_num);
        return false;
    }

    return true;
}

const A2DP_AUDIO_CC_ENCODER_T* a2dp_encoder_ldac_config()
{
    return &a2dp_audio_cc_ldac_encoder_config;
}

#endif
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
#include "cmsis_os.h"
#include "hal_trace.h"
#include "plat_types.h"
#include "app_gaf_dbg.h"
#include "gaf_cc_api.h"
#include "gaf_cc_off_m55.h"
#include "gaf_cp_stream.h"
#include "cc_stream_common.h"

/************************private macro defination***************************/

/************************private strcuture defination****************************/
ENCODER_CORE_FUNC_LIST_T *encoder_core_func_list[CORE_TYPE_MAX];
DECODER_CORE_FUNC_LIST_T *decoder_core_func_list[CORE_TYPE_MAX];
MEDIA_STREAM_CC_FUNC_LIST *media_stream_func_list;

/************************private variable defination************************/

/**********************private function declaration*************************/

/****************************function defination****************************/

void gaf_cc_playback_feed_init_req(void *_initReq, uint32_t core_type)
{
    if (core_type == M33_CORE)
    {
        ASSERT(false, "%s core type err!!", __func__);
    }
#ifdef GAF_DECODER_CROSS_CORE_USE_M55
    else if (core_type == M55_CORE)
    {
        gaf_off_m55_playback_update_func_list(&(decoder_core_func_list[core_type]));
    }
#endif
#ifdef AOB_CODEC_CP
    else if (core_type == CP_CORE)
    {
        gaf_cp_playback_update_func_list(&(decoder_core_func_list[core_type]));
    }
#endif

    decoder_core_func_list[core_type]->decoder_core_int(_initReq);
}

void gaf_cc_capture_feed_init_req(void *_initReq, uint32_t core_type)
{
    if (core_type == M33_CORE)
    {
        ASSERT(false, "%s core type err!!", __func__);
    }
#ifdef GAF_ENCODER_CROSS_CORE_USE_M55
    else if (core_type == M55_CORE)
    {
        gaf_off_m55_capture_update_func_list(&(encoder_core_func_list[core_type]));
    }
#endif
#ifdef AOB_CODEC_CP
    else if (core_type == CP_CORE)
    {
        gaf_cp_capture_update_func_list(&(encoder_core_func_list[core_type]));
    }
#endif

    encoder_core_func_list[core_type]->encoder_core_init(_initReq);
}

void gaf_cc_capture_feed_deinit_req(void *deinitReq, uint32_t core_type)
{
    if (encoder_core_func_list[core_type])
    {
        encoder_core_func_list[core_type]->encoder_core_deinit(deinitReq);
    }
}

void gaf_cc_playback_feed_deinit_req(void *deinitReq, uint32_t core_type)
{
    if (decoder_core_func_list[core_type])
    {
        decoder_core_func_list[core_type]->decoder_core_deinit(deinitReq);
    }
}

void gaf_cc_playback_init_rsp(void *rsp)
{
    if (media_stream_func_list)
    {
        media_stream_func_list->decoder_core_init_rsp(rsp);
    }
}

void gaf_cc_capture_init_rsp(void *rsp)
{
    if (media_stream_func_list)
    {
        media_stream_func_list->encoder_core_init_rsp(rsp);
    }
}

void gaf_cc_playback_data_notify(uint32_t core_type)
{
    if (decoder_core_func_list[core_type])
    {
        decoder_core_func_list[core_type]->decoder_core_notify();
    }
}

void gaf_cc_capture_data_notify(uint32_t core_type)
{
    if (encoder_core_func_list[core_type])
    {
        encoder_core_func_list[core_type]->encoder_core_notify();
    }
}

void gaf_cc_media_stream_func_register(void *func_list)
{
    media_stream_func_list = (MEDIA_STREAM_CC_FUNC_LIST*)func_list;
}

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

#ifndef __GAF_CC_API_H__
#define __GAF_CC_API_H__

/*****************************header include********************************/

#ifdef __cplusplus
extern "C"{
#endif

/******************************macro defination*****************************/

/******************************structure defination*****************************/
typedef void (*ENCODER_COREE_INIT_FUNC)(void *initReq);
typedef void (*ENCODER_COREE_DEINIT_FUNC)(void *deinitReq);
typedef void (*ENCODER_COREE_NOTIFY_FUNC)(void);

typedef void (*DECODER_CORE_INIT_FUNC)(void *initReq);
typedef void (*DECODER_CORE_DEINIT_FUNC)(void *deinitReq);
typedef void (*DECODER_CORE_NOTIFY_FUNC)(void);

typedef void (*DECODER_CORE_INIT_RSP_FUNC)(void *initRsp);
typedef void (*ENCODER_CORE_INIT_RSP_FUNC)(void *initRsp);

typedef struct
{
    ENCODER_COREE_INIT_FUNC    encoder_core_init;
    ENCODER_COREE_DEINIT_FUNC  encoder_core_deinit;
    ENCODER_COREE_NOTIFY_FUNC  encoder_core_notify;
} ENCODER_CORE_FUNC_LIST_T;

typedef struct
{
    DECODER_CORE_INIT_FUNC    decoder_core_int;
    DECODER_CORE_DEINIT_FUNC  decoder_core_deinit;
    DECODER_CORE_NOTIFY_FUNC  decoder_core_notify;
} DECODER_CORE_FUNC_LIST_T;

typedef struct
{
    DECODER_CORE_INIT_RSP_FUNC decoder_core_init_rsp;
    ENCODER_CORE_INIT_RSP_FUNC encoder_core_init_rsp;
} MEDIA_STREAM_CC_FUNC_LIST;

/****************************** function defination*****************************/

void gaf_cc_playback_feed_init_req(void *_initReq, uint32_t core_type);

void gaf_cc_capture_feed_init_req(void *_initReq,  uint32_t core_type);

void gaf_cc_capture_feed_deinit_req(void *deinitReq, uint32_t core_type);

void gaf_cc_playback_feed_deinit_req(void *deinitReq, uint32_t core_type);

void gaf_cc_capture_deinit_rsp_handler(void);

void gaf_cc_playback_deinit_rsp_handler(void);

void gaf_cc_playback_data_notify(uint32_t core_type);

void gaf_cc_capture_data_notify(uint32_t core_type);

void gaf_cc_playback_init_rsp(void *rsp);

void gaf_cc_capture_init_rsp(void *rsp);

void gaf_cc_media_stream_func_register(void *func_list);
#ifdef __cplusplus
}
#endif

#endif /// __GAF_CC_API_H__

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

#ifndef __GAF_CP_STREAM_H__
#define __GAF_CP_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************struct defination******************************/

/****************************function declaration***************************/

void gaf_cp_stream_init(void);

void gaf_cp_capture_update_func_list(void *_pEncoderEnv);

void gaf_cp_playback_update_func_list(void *_pDecoderEnv);

#ifdef __cplusplus
}
#endif

#endif /// __GAF_CP_STREAM_H__

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

#ifndef __GAF_CODEC_LC3_H__
#define __GAF_CODEC_LC3_H__

#ifdef __cplusplus
extern "C"{
#endif
/*****************************header include********************************/

/******************************macro defination*****************************/
#define LC3_MAX_FRAME_SIZE 870

/******************************struct defination******************************/

/****************************function declaration***************************/

void gaf_audio_lc3_update_encoder_func_list(void *enc_func_list);

void gaf_audio_lc3_update_decoder_func_list(void *dec_func_list);

int gaf_audio_lc3_encoder_get_max_frame_size(void);

#ifdef __cplusplus
}
#endif

#endif /// __GAF_CODEC_LC3_H__

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

#ifndef __GAF_CODEC_COMMMON_H__
#define __GAF_CODEC_COMMMON_H__

#ifdef __cplusplus
extern "C"{
#endif
/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************struct defination******************************/
typedef void (*GAF_DECODER_INIT_BUFFER)(uint8_t instance_handle, void *codec_info, void *alloc_cb);
typedef void (*GAF_DECODER_INIT_FUNC)(uint8_t instance_handle, void *codec_info);
typedef void (*GAF_DECODER_DEINIT_FUNC)(uint8_t instance_handle);
typedef int (*GAF_DECODER_DECODE_FRAME_FUNC)(uint8_t instance_handle, void *codec_info,
            uint32_t inputDataLength, void *input, void *output, bool isPlc);
typedef void (*GAF_DECODER_DEINIT_BUFFER)(uint8_t instance_handle);
typedef uint32_t (*GAF_DECODER_SET_FREQ)(void *codec_info, uint32_t base_freq, uint32_t core_type);

typedef void (*GAF_ENCODER_INIT_BUFFER)(uint8_t instance_handle, void *codec_info, void *alloc_cb);
typedef void (*GAF_ENCODER_INIT_FUNC)(uint8_t instance_handle, void *codec_info);
typedef void (*GAF_ENCODER_DEINIT_FUNC)(uint8_t instance_handle);
typedef int (*GAF_ENCODER_ENCODE_FRAME_FUNC)(uint8_t instance_handle, void *codec_info,
            uint32_t inputDataLength, void *input, uint16_t frame_size, uint8_t *output);
typedef void (*GAF_ENCODER_DEINIT_BUFFER)(uint8_t instance_handle);
typedef uint32_t (*GAF_ENCODER_SET_FREQ)(void *codec_info, uint32_t base_freq, uint32_t core_type);

typedef struct
{
    GAF_DECODER_INIT_BUFFER   decoder_init_buf_func;
    GAF_DECODER_INIT_FUNC     decoder_init_func;
    GAF_DECODER_DEINIT_FUNC   decoder_deinit_func;
    GAF_DECODER_DECODE_FRAME_FUNC decoder_decode_frame_func;
    GAF_DECODER_DEINIT_BUFFER   decoder_deinit_buf_func;
    GAF_DECODER_SET_FREQ      decoder_set_freq;
} GAF_DECODER_FUNC_LIST_T;

typedef struct
{
    GAF_ENCODER_INIT_BUFFER   encoder_init_buf_func;
    GAF_ENCODER_INIT_FUNC     encoder_init_func;
    GAF_ENCODER_DEINIT_FUNC   encoder_deinit_func;
    GAF_ENCODER_ENCODE_FRAME_FUNC encoder_encode_frame_func;
    GAF_ENCODER_DEINIT_BUFFER   encoder_deinit_buf_func;
    GAF_ENCODER_SET_FREQ      encoder_set_freq;
} GAF_ENCODER_FUNC_LIST_T;

typedef struct
{
    uint32_t sample_rate;
    uint8_t num_channels;
    uint8_t bits_depth;
    float frame_ms;
    uint16_t frame_size;
    uint32_t pcm_size;
} CODEC_INFO_T;

enum
{
    CODEC_OK = 0,
    CODEC_ERR = 1,
};

typedef enum
{
    LC3 = 0,
    LC3PLUS = 1,
    GSBC = 2,
} CODEC_TYPE_E;

typedef struct
{
    uint32_t start_time_in_ms;
    uint32_t total_time_in_us;
    bool codec_started;
    uint32_t codec_mips;
} aob_codec_time_info_t;

typedef struct
{
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t bit_rate;
    uint8_t frame_ms;
    uint32_t freq;
} codec_freq_setting_t;

/****************************function declaration***************************/

#ifdef __cplusplus
}
#endif

#endif /// __GAF_CODEC_COMMMON_H__
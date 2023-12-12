/**
 * @copyright Copyright (c) 2015-20223 BES Technic.
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
 */
#ifndef APP_BIS_TRANSPOND_CODEC_H_
#define APP_BIS_TRANSPOND_CODEC_H_

/*****************************header include********************************/


/*********************external function declaration*************************/


/************************private macro defination***************************/
typedef enum
{
    APP_BIS_TRAN_CODEC_OK,
    APP_BIS_TRAN_CODEC_ERROR,
} APP_BIS_TRAN_CODEC_RETURN_E;

typedef enum
{
    APP_BIS_TRAN_ENCODER,
    APP_BIS_TRAN_DECODER,
} APP_BIS_TRAN_CODEC_TYPE_E;

/************************private type defination****************************/

typedef struct
{
    uint8_t  codec_type:1;
    uint8_t  pcm_interlaced:1;
    uint8_t  bits_depth;
    uint8_t  channel;
    uint32_t sample_rate;
    uint32_t frame_duration_ms;
    uint32_t frame_size;
} app_bis_tran_codec_param_t;

/**********************private function declaration*************************/

/************************private variable defination************************/
void *app_bis_tran_codec_creat(app_bis_tran_codec_param_t *param);

bool app_bis_tran_codec_delete(void *codec);

int app_bis_tran_codec_decode(void *codec, uint8_t *input, uint32_t input_len,
                                                    uint8_t *output, uint32_t *output_len);

int app_bis_tran_codec_encode(void *codec, uint8_t *input, uint32_t input_len,
                                                    uint8_t *output, uint32_t *output_len);
#endif  //APP_BIS_TRANSPOND_CODEC_H_
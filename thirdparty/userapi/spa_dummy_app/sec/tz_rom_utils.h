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

#ifndef __TZ_ROM_UTILS_H__
#define __TZ_ROM_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif
#ifdef TZ_ROM_UTILS_IF
#include "sbc.h"
#endif

#ifndef ARM_CMNS


#else
#define sbc_decoder_init_tz_rom_utils_if                    sbc_decoder_init_tz_rom_utils_if_cmse
#define sbc_frames_parser_tz_rom_utils_if                   sbc_frames_parser_tz_rom_utils_if_cmse
#define sbc_frames_decode_do_tz_rom_utils_if                sbc_frames_decode_do_tz_rom_utils_if_cmse
#define sbc_frames_decode_tz_rom_utils_if                   sbc_frames_decode_tz_rom_utils_if_cmse
#define sbc_frames_decode_select_channel_tz_rom_utils_if    sbc_frames_decode_select_channel_tz_rom_utils_if_cmse
#define sbc_frames_decode_out_sbsamples_tz_rom_utils_if     sbc_frames_decode_out_sbsamples_tz_rom_utils_if_cmse
#define sbc_encoder_init_tz_rom_utils_if                    sbc_encoder_init_tz_rom_utils_if_cmse
#define sbc_frames_encode_arm_tz_utils_if                   sbc_frames_encode_arm_tz_utils_if_cmse
#define sbc_frames_encode_with_sbsamples_arm_tz_utils_if    sbc_frames_encode_with_sbsamples_arm_tz_utils_if_cmse
#endif

#if defined(ARM_CMSE) || defined(ARM_CMNS)

#if defined(TZ_ROM_UTILS_IF)
typedef struct{
    sbc_decoder_t *Decoder;
}sbc_decoder_init_t;

typedef struct{
    sbc_decoder_t *Decoder;
    unsigned char       *Buff;
    unsigned short       Len;
    unsigned short      *BytesDecoded;
}sbc_frames_parser_t;

typedef struct{
    sbc_decoder_t *Decoder;
    U8         *Buff;
    U16         Len;
    U16        *BytesDecoded;
    sbc_pcm_data_t *PcmData;
    U16         MaxPcmData;
    float* gains;
    SBC_DECODER_CHANNEL_SELECT_E channel_select;
}sbc_frames_decode_do_t;

typedef struct{
    sbc_decoder_t *Decoder;
    unsigned char      *Buff;
    unsigned short          Len;
    unsigned short         *BytesDecoded;
    sbc_pcm_data_t *PcmData;
    unsigned short          MaxPcmData;
    float* gains;
}sbc_frames_decode_t;

typedef struct{
    sbc_decoder_t *Decoder;
    unsigned char      *Buff;
    unsigned short          Len;
    unsigned short         *BytesDecoded;
    sbc_pcm_data_t *PcmData;
    unsigned short          MaxPcmData;
    float* gains;
    SBC_DECODER_CHANNEL_SELECT_E channel_select;
}sbc_frames_decode_select_channel_t;

typedef struct{
    sbc_decoder_t *Decoder;
    U8       *Buff;
    U16       Len;
    U16      *BytesDecoded;
    sbc_pcm_data_t *PcmData;
    U16       MaxPcmData;
    float* gains;
    U8     ChooseDecChannel;
    REAL       *SBSamplesBuf;
    U32        SBSamplesBufLen;
    U32        *SBSamplesBufUsed;
    U8     ChooseSplitChannel;
}sbc_frames_decode_out_sbsamples_t;

typedef struct{
    sbc_encoder_t *Encoder;
}sbc_encoder_init_t;

typedef struct{
    sbc_encoder_t *Encoder;
    sbc_pcm_data_t *PcmData;
    unsigned short         *BytesEncoded;
    unsigned char      *Buff;
    unsigned short         *Len;
    unsigned short          MaxSbcData;
}sbc_frames_encode_t;

typedef struct{
    sbc_encoder_t *Encoder;
    REAL       *SBSamplesBuf;
    U32        SBSamplesBufLen_bytes;
    U32        *SBSamplesBufUsed_bytes;
    U8         *Buff;
    U16        *Len;
    U16         MaxSbcData;
    U8      *number_freame_encoded;
}sbc_frames_encode_with_sbsamples_t;

void sbc_decoder_init_tz_rom_utils_if_cmse(sbc_decoder_init_t *info);
sbc_ret_status_t sbc_frames_parser_tz_rom_utils_if_cmse(sbc_frames_parser_t* info);
sbc_ret_status_t sbc_frames_decode_do_tz_rom_utils_if_cmse(sbc_frames_decode_do_t *info);
sbc_ret_status_t sbc_frames_decode_tz_rom_utils_if_cmse(sbc_frames_decode_t *info);
sbc_ret_status_t sbc_frames_decode_select_channel_tz_rom_utils_if_cmse(sbc_frames_decode_select_channel_t* info);
sbc_ret_status_t sbc_frames_decode_out_sbsamples_tz_rom_utils_if_cmse(sbc_frames_decode_out_sbsamples_t* info);
void sbc_encoder_init_tz_rom_utils_if_cmse(sbc_encoder_init_t* info);
sbc_ret_status_t sbc_frames_encode_arm_tz_utils_if_cmse(sbc_frames_encode_t * info);
sbc_ret_status_t sbc_frames_encode_with_sbsamples_arm_tz_utils_if_cmse(sbc_frames_encode_with_sbsamples_t * info);

#endif

#endif


#ifdef __cplusplus
}
#endif

#endif


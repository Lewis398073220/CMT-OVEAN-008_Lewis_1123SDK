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
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "hal_codec.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "crc32_c.h"
#include "cmsis.h"
#include "pmu.h"
#include "hal_location.h"

#ifdef TZ_ROM_UTILS_IF
#include "tz_rom_utils.h"
#endif

/*
    design for ARM_CMSE interface
*/
#ifdef ARM_CMSE

#elif defined(ARM_CMNS) && defined(TZ_ROM_UTILS_IF)

void sbc_decoder_init(sbc_decoder_t *Decoder)
{
    sbc_decoder_init_t info;
    info.Decoder = Decoder;

    sbc_decoder_init_tz_rom_utils_if(&info);
}

sbc_ret_status_t sbc_frames_parser(sbc_decoder_t *Decoder,
						  U8		 *Buff,
						  U16		  Len,
						  U16		 *BytesDecoded)
{
    sbc_frames_parser_t info;
    info.Decoder = Decoder;
    info.Buff = Buff;
    info.Len = Len;
    info.BytesDecoded = BytesDecoded;

    return sbc_frames_parser_tz_rom_utils_if(&info);
}

sbc_ret_status_t sbc_frames_decode_do(sbc_decoder_t *Decoder,
                        U8         *Buff,
                        U16         Len,
                        U16        *BytesDecoded,
                        sbc_pcm_data_t *PcmData,
                        U16         MaxPcmData,
                        float* gains, SBC_DECODER_CHANNEL_SELECT_E channel_select)
{
    sbc_frames_decode_do_t info;
    info.Decoder = Decoder;
    info.Buff = Buff;
    info.BytesDecoded = BytesDecoded;
    info.PcmData = PcmData;
    info.MaxPcmData = MaxPcmData;
    info.gains = gains;
    info.channel_select = channel_select;

    return sbc_frames_decode_do_tz_rom_utils_if(&info);
}

sbc_ret_status_t sbc_frames_decode(sbc_decoder_t *Decoder,
                          U8         *Buff,
                          U16         Len,
                          U16        *BytesDecoded,
                          sbc_pcm_data_t *PcmData,
                          U16         MaxPcmData,
                          float* gains)
{
    sbc_frames_decode_t info;
    info.Decoder = Decoder;
    info.Buff = Buff;
    info.Len = Len;
    info.BytesDecoded = BytesDecoded;
    info.PcmData = PcmData;
    info.MaxPcmData = MaxPcmData;
    info.gains = gains;

    return sbc_frames_decode_tz_rom_utils_if(&info);
}


sbc_ret_status_t sbc_frames_decode_select_channel(sbc_decoder_t *Decoder,
                        U8         *Buff,
                        U16         Len,
                        U16        *BytesDecoded,
                        sbc_pcm_data_t *PcmData,
                        U16         MaxPcmData,
                        float* gains, SBC_DECODER_CHANNEL_SELECT_E channel_select)
{
    sbc_frames_decode_select_channel_t info;
    info.Decoder = Decoder;
    info.Buff = Buff;
    info.Len = Len;
    info.BytesDecoded = BytesDecoded;
    info.PcmData = PcmData;
    info.MaxPcmData = MaxPcmData;
    info.gains = gains;
    info.channel_select = channel_select;

  return sbc_frames_decode_select_channel_tz_rom_utils_if(&info);
}

sbc_ret_status_t sbc_frames_decode_out_sbsamples(sbc_decoder_t *Decoder,
        U8       *Buff,
        U16       Len,
        U16      *BytesDecoded,
        sbc_pcm_data_t *PcmData,
        U16       MaxPcmData,
        float* gains,
        U8     ChooseDecChannel,
        REAL       *SBSamplesBuf,
        U32        SBSamplesBufLen,
        U32        *SBSamplesBufUsed,
        U8     ChooseSplitChannel)
{
    sbc_frames_decode_out_sbsamples_t info;
    info.Decoder = Decoder;
    info.Buff = Buff;
    info.BytesDecoded = BytesDecoded;
    info.PcmData = PcmData;
    info.MaxPcmData = MaxPcmData;
    info.gains = gains;
    info.ChooseDecChannel = ChooseDecChannel;
    info.SBSamplesBuf = SBSamplesBuf;
    info.SBSamplesBufLen = SBSamplesBufLen;
    info.SBSamplesBufUsed = SBSamplesBufUsed;
    info.ChooseSplitChannel = ChooseSplitChannel;

    return sbc_frames_decode_out_sbsamples_tz_rom_utils_if(&info);
}


void sbc_encoder_init(sbc_encoder_t *Encoder)
{
    sbc_encoder_init_t info;
    info.Encoder = Encoder;

    sbc_encoder_init_tz_rom_utils_if(&info);
}


sbc_ret_status_t sbc_frames_encode(sbc_encoder_t *Encoder,
						  sbc_pcm_data_t *PcmData,
						  U16		 *BytesEncoded,
						  U8		 *Buff,
						  U16		 *Len,
						  U16		  MaxSbcData)
{
    sbc_frames_encode_t info;
    info.Encoder = Encoder;
    info.PcmData = PcmData;
    info.BytesEncoded = BytesEncoded;
    info.Buff = Buff;
    info.Len = Len;
    info.MaxSbcData = MaxSbcData;

    return sbc_frames_encode_arm_tz_utils_if(&info);
}

sbc_ret_status_t sbc_frames_encode_with_sbsamples(sbc_encoder_t *Encoder,
                        REAL       *SBSamplesBuf,
                        U32        SBSamplesBufLen_bytes,
                        U32        *SBSamplesBufUsed_bytes,
                        U8         *Buff,
                        U16        *Len,
                        U16         MaxSbcData,
                        U8      *number_freame_encoded)
{
    sbc_frames_encode_with_sbsamples_t info;
    info.Encoder = Encoder;
    info.SBSamplesBuf = SBSamplesBuf;
    info.SBSamplesBufLen_bytes = SBSamplesBufLen_bytes;
    info.SBSamplesBufUsed_bytes = SBSamplesBufUsed_bytes;
    info.Buff = Buff;
    info.Len = Len;
    info.MaxSbcData = MaxSbcData;
    info.number_freame_encoded = number_freame_encoded;

    return sbc_frames_encode_with_sbsamples_arm_tz_utils_if(&info);
}

#endif


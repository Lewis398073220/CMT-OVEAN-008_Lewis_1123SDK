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
#include "export_fn_rom.h"
#endif

#include "tz_rom_utils.h"

/*
    design for ARM_CMSE interface
*/
#if defined(ARM_CMSE) && defined(TZ_ROM_UTILS_IF)
CMSE_API void sbc_decoder_init_tz_rom_utils_if_cmse(sbc_decoder_init_t *info)
{
    sbc_decoder_init(info->Decoder);
}

CMSE_API sbc_ret_status_t sbc_frames_parser_tz_rom_utils_if_cmse(sbc_frames_parser_t* info)
{
    return sbc_frames_parser(info->Decoder,info->Buff,info->Len,info->BytesDecoded);
}

CMSE_API sbc_ret_status_t sbc_frames_decode_do_tz_rom_utils_if_cmse(sbc_frames_decode_do_t *info)
{
#if 0
    return sbc_frames_decode_do(info->Decoder,info->Buff,info->Len,info->BytesDecoded,info->PcmData,info->MaxPcmData,
                                info->gains,info->channel_select);
#else
    return sbc_frames_decode(info->Decoder,info->Buff,info->Len,info->BytesDecoded,info->PcmData,info->MaxPcmData,info->gains);
#endif
}

CMSE_API sbc_ret_status_t sbc_frames_decode_tz_rom_utils_if_cmse(sbc_frames_decode_t *info)
{
    return sbc_frames_decode(info->Decoder,info->Buff,info->Len,info->BytesDecoded,info->PcmData,info->MaxPcmData,info->gains);
}

CMSE_API sbc_ret_status_t sbc_frames_decode_select_channel_tz_rom_utils_if_cmse(sbc_frames_decode_select_channel_t* info)
{
    return sbc_frames_decode_select_channel(info->Decoder,info->Buff,info->Len,info->BytesDecoded,info->PcmData,info->MaxPcmData,info->gains,info->channel_select);
}

CMSE_API sbc_ret_status_t sbc_frames_decode_out_sbsamples_tz_rom_utils_if_cmse(sbc_frames_decode_out_sbsamples_t* info)
{
    return sbc_frames_decode_out_sbsamples(info->Decoder,info->Buff,info->Len,info->BytesDecoded,info->PcmData,
                            info->MaxPcmData,info->gains,info->ChooseDecChannel,info->SBSamplesBuf,info->SBSamplesBufLen,
                                info->SBSamplesBufUsed,info->ChooseSplitChannel);
}

CMSE_API void sbc_encoder_init_tz_rom_utils_if_cmse(sbc_encoder_init_t* info)
{
    sbc_encoder_init(info->Encoder);
}

CMSE_API sbc_ret_status_t sbc_frames_encode_arm_tz_utils_if_cmse(sbc_frames_encode_t * info)
{
    return sbc_frames_encode(info->Encoder,info->PcmData,info->BytesEncoded,info->Buff,info->Len,info->MaxSbcData);
}

CMSE_API sbc_ret_status_t sbc_frames_encode_with_sbsamples_arm_tz_utils_if_cmse(sbc_frames_encode_with_sbsamples_t * info)
{

    return sbc_frames_encode_with_sbsamples(info->Encoder,info->SBSamplesBuf,info->SBSamplesBufLen_bytes,info->SBSamplesBufUsed_bytes,
                                            info->Buff,info->Len,info->MaxSbcData,info->number_freame_encoded);
}

#endif


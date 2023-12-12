/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#include "asm_macros.h"

    def_FuncExt v1, rom1_entry,                         0x00030000
    def_FuncExt v1, set__impure_ptr,                    0x00030004
    def_FuncExt v1, sbc_bits_for_mono_alloc,            0x00030004
    def_FuncExt v1, sbc_bits_for_stereo_alloc,          0x00030220
    def_FuncExt v1, sbc_header_process,                 0x00030520
    def_FuncExt v1, sbc_factors_process,                0x00030640
    def_FuncExt v1, sbc_sbsamples_make,                 0x0003073c
    def_FuncExt v1, sbc_factors_calc,                   0x00031970
    def_FuncExt v1, sbc_header_make,                    0x00031b94
    def_FuncExt v1, sbc_frame_mute,                     0x00031dc4
    def_FuncExt v1, sbc_samples_quant,                  0x00031df4
    def_FuncExt v1, sbc_frame_length_get,               0x00032cf4
    def_FuncExt v1, sbc_decoder_init,                   0x00032d84
    def_FuncExt v1, sbc_4band_synth_filter,             0x00032d90
    def_FuncExt v1, sbc_8band_synth_filter,             0x0003363c
    def_FuncExt v1, sbc_frames_parser,                  0x000348f4
    def_FuncExt v1, sbc_frames_decode_do,               0x00034af4
    def_FuncExt v1, sbc_frames_decode,                  0x00034ec0
    def_FuncExt v1, sbc_frames_decode_select_channel,   0x00034ee0
    def_FuncExt v1, sbc_frames_decode_out_sbsamples,    0x00034ef4
    def_FuncExt v1, sbc_encoder_init,                   0x00035360
    def_FuncExt v1, sbc_frames_encode,                  0x0003536c
    def_FuncExt v1, sbc_frames_encode_with_sbsamples,   0x00037338

    def_Symbol dec_coeff_4band_tbl,                     0x24017564
    def_Symbol dec_coeff_8band_tbl,                     0x24017604
    def_Symbol loudness_offs_4band,                     0x24017744
    def_Symbol loudness_offs_8band,                     0x24017754
    def_Symbol sbc_coef_4band,                          0x24017774
    def_Symbol sbc_coef_8band,                          0x24017814
    def_Symbol sbc_crc_tbl,                             0x24017954
    def_Symbol sbc_enc_analyze_4band_tbl,               0x24017a54
    def_Symbol sbc_enc_analyze_8band_tbl,               0x24017ad4
    def_Symbol sbc_samples_quant.str1.4,                0x24017cd4
    def_Symbol synth_4band_tbl,                         0x24017ce0
    def_Symbol synth_8band_tbl,                         0x24017d60
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
#ifndef __AUD_DATA_PRIMITIVE_H__
#define __AUD_DATA_PRIMITIVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

void aud_data_memcpy_i32_stereo_from_i32_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_stereo_from_i32_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_stereo_from_i32_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_stereo_from_i32_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i32_mono_from_i32_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_mono_from_i32_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_mono_from_i32_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_mono_from_i32_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i32_l24_stereo_from_i32_l24_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_stereo_from_i32_l24_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_stereo_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_stereo_from_i32_l24_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_stereo_from_i32_h24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_stereo_from_i32_h24_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_h24_stereo_from_i32_l24_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_h24_stereo_from_i32_l24_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i32_l24_mono_from_i32_l24_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_mono_from_i32_l24_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_mono_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_l24_mono_from_i32_l24_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i16_stereo_from_i16_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_stereo_from_i16_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_stereo_from_i16_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_stereo_from_i16_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_stereo_from_i16_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i16_mono_from_i16_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_mono_from_i16_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_mono_from_i16_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_mono_from_i16_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_mono_from_i16_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i24_2ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i24_2ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i24_1ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i24_1ch_from_i32_l24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i24_1ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i32_8ch_from_i32_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i32_8ch_from_i32_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

void aud_data_memcpy_i16_2ch_from_i32_l24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_2ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_1ch_from_i32_l24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_1ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_4ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_2ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_1ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_8ch_from_i16_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);
void aud_data_memcpy_i16_8ch_from_i16_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes);

#ifdef __cplusplus
}
#endif

#endif

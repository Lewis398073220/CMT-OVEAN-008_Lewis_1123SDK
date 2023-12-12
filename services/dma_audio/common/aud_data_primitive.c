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
#include "aud_data_primitive.h"

void aud_data_memcpy_i32_stereo_from_i32_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 4;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d += 8;
        s += 8;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];

        d[8]  = s[8];
        d[9]  = s[9];
        d[10] = s[10];
        d[11] = s[11];
        d[12] = s[12];
        d[13] = s[13];
        d[14] = s[14];
        d[15] = s[15];
        d += 16;
        s += 16;
    }
#else
#error "aud_data_memcpy_i32_stereo_from_i32_stereo: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
    }
#endif
}

void aud_data_memcpy_i32_stereo_from_i32_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/4;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[0];
        d += 2;
        s += 1;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d += 4;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d[4] = s[2];
        d[5] = s[2];
        d[6] = s[3];
        d[7] = s[3];
        d += 8;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d[4] = s[2];
        d[5] = s[2];
        d[6] = s[3];
        d[7] = s[3];

        d[8]  = s[4];
        d[9]  = s[4];
        d[10] = s[5];
        d[11] = s[5];
        d[12] = s[6];
        d[13] = s[6];
        d[14] = s[7];
        d[15] = s[7];
        d += 16;
        s += 8;
    }
#else
#error "aud_data_memcpy_i32_stereo_from_i32_mono: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[0];
        d += 2;
        s += 1;
    }
#endif
}

void aud_data_memcpy_i32_stereo_from_i32_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/12;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= 2) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[3];
        d[3] = s[4];
        d += 4;
        s += 6;
    }
#else
#error "aud_data_memcpy_i32_stereo_from_i32_3ch: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif
#else
    for (;i > 0; i--) {
#ifdef DMA_AUDIO_DATA_SW_MERGE
        d[0] = s[0]+s[1]+s[2];
        d[1] = d[0];
#else
        d[0] = s[0];
        d[1] = s[1];
#endif
        d += 2;
        s += 3;
    }
#endif
}

void aud_data_memcpy_i32_stereo_from_i32_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 4;
    }
}

void aud_data_memcpy_i32_l24_stereo_from_i32_h24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0] >> 8;
        d[1] = s[1] >> 8;
        d += 2;
        s += 2;
    }
}

void aud_data_memcpy_i32_l24_stereo_from_i32_h24_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/32;

    for (;i > 0; i--) {
        d[0] = s[0] >> 8;
        d[1] = s[1] >> 8;
        d += 2;
        s += 8;
    }
}

void aud_data_memcpy_i32_h24_stereo_from_i32_l24_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0] << 8;
        d[1] = s[1] << 8;
        d += 2;
        s += 2;
    }
}

void aud_data_memcpy_i32_h24_stereo_from_i32_l24_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/4;

    for (;i > 0; i--) {
        d[0] = s[0] << 8;
        d[1] = d[0];
        d += 2;
        s += 1;
    }
}

void aud_data_memcpy_i32_mono_from_i32_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d += 1;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[2];
        d += 2;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[2];
        d[2] = s[4];
        d[3] = s[6];
        d += 4;
        s += 8;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[2];
        d[2] = s[4];
        d[3] = s[6];
        d[4] = s[8];
        d[5] = s[10];
        d[6] = s[12];
        d[7] = s[14];
        d += 8;
        s += 16;
    }
#else
#error "aud_data_memcpy_i32_mono_from_i32_stereo: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 2;
    }
#endif
}

void aud_data_memcpy_i32_mono_from_i32_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/4;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 1;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 4;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d += 8;
        s += 8;
    }
#else
#error "aud_data_memcpy_i32_mono_from_i32_mono: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 1;
    }
#endif

}

void aud_data_memcpy_i32_mono_from_i32_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/12;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 3;
    }
}

void aud_data_memcpy_i32_mono_from_i32_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 4;
    }
}

void aud_data_memcpy_i32_l24_stereo_from_i32_l24_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_stereo_from_i32_stereo(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_stereo_from_i32_l24_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_stereo_from_i32_mono(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_stereo_from_i32_l24_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_stereo_from_i32_3ch(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_stereo_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_stereo_from_i32_4ch(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_mono_from_i32_l24_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_mono_from_i32_stereo(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_mono_from_i32_l24_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_mono_from_i32_mono(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_mono_from_i32_l24_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_mono_from_i32_3ch(dst, src, src_bytes);
}

void aud_data_memcpy_i32_l24_mono_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    aud_data_memcpy_i32_mono_from_i32_4ch(dst, src, src_bytes);
}

void aud_data_memcpy_i16_stereo_from_i16_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/4;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 4;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d += 8;
        s += 8;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];

        d[8]  = s[8];
        d[9]  = s[9];
        d[10] = s[10];
        d[11] = s[11];
        d[12] = s[12];
        d[13] = s[13];
        d[14] = s[14];
        d[15] = s[15];
        d += 16;
        s += 16;
    }
#else
#error "aud_data_memcpy_i16_stereo_from_i16_stereo: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
    }
#endif
}

void aud_data_memcpy_i16_stereo_from_i16_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/2;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[0];
        d += 2;
        s += 1;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d += 4;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d[4] = s[2];
        d[5] = s[2];
        d[6] = s[3];
        d[7] = s[3];
        d += 8;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d[4] = s[2];
        d[5] = s[2];
        d[6] = s[3];
        d[7] = s[3];

        d[8]  = s[4];
        d[9]  = s[4];
        d[10] = s[5];
        d[11] = s[5];
        d[12] = s[6];
        d[13] = s[6];
        d[14] = s[7];
        d[15] = s[7];
        d += 16;
        s += 8;
    }
#else
#error "aud_data_memcpy_i32_stereo_from_i32_mono: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[0];
        d += 2;
        s += 1;
    }
#endif

}

void aud_data_memcpy_i16_stereo_from_i16_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/6;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 3;
    }
}

void aud_data_memcpy_i16_stereo_from_i16_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 4;
    }
}

void aud_data_memcpy_i16_stereo_from_i16_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 8;
    }
}

void aud_data_memcpy_i16_mono_from_i16_stereo(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/4;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d += 1;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[2];
        d += 2;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[2];
        d[2] = s[4];
        d[3] = s[6];
        d += 4;
        s += 8;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[2];
        d[2] = s[4];
        d[3] = s[6];
        d[4] = s[8];
        d[5] = s[10];
        d[6] = s[12];
        d[7] = s[14];
        d += 8;
        s += 16;
    }
#else
#error "aud_data_memcpy_i32_mono_from_i32_stereo: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 2;
    }
#endif

}

void aud_data_memcpy_i16_mono_from_i16_mono(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/2;

#ifdef AUD_DATA_MEMCPY_SAMP_INC
#if (AUD_DATA_MEMCPY_SAMP_INC == 1)
    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 1;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 2)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 4)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 4;
        s += 4;
    }
#elif (AUD_DATA_MEMCPY_SAMP_INC == 8)
    for (;i > 0; i -= AUD_DATA_MEMCPY_SAMP_INC) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d += 8;
        s += 8;
    }
#else
#error "aud_data_memcpy_i32_mono_from_i32_mono: invalid AUD_DATA_MEMCPY_SAMP_INC !!"
#endif

#else /* !AUD_DATA_MEMCPY_SAMP_INC */
    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 1;
    }
#endif

}

void aud_data_memcpy_i16_mono_from_i16_3ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/6;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 3;
    }
}

void aud_data_memcpy_i16_mono_from_i16_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 4;
    }
}

void aud_data_memcpy_i16_mono_from_i16_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 8;
    }
}

void aud_data_memcpy_i24_1ch_from_i32_l24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 2;
    }
}

#define SRC16_CH_DATA(s) ((int16_t)((s) >> 8))

void aud_data_memcpy_i16_2ch_from_i32_l24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = SRC16_CH_DATA(s[0]);
        d[1] = SRC16_CH_DATA(s[1]);
        d += 2;
        s += 2;
    }
}

void aud_data_memcpy_i16_2ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0] >> 16;
        d[1] = s[1] >> 16;
        d += 2;
        s += 2;
    }
}

void aud_data_memcpy_i16_1ch_from_i32_l24_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = SRC16_CH_DATA(s[0]);
        d += 1;
        s += 2;
    }
}

void aud_data_memcpy_i16_1ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0] >> 16;
        d += 1;
        s += 2;
    }
}

void aud_data_memcpy_i24_2ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0]>>8;
        d[1] = s[1]>>8;
        d += 2;
        s += 2;
    }
}

void aud_data_memcpy_i24_1ch_from_i32_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0]>>8;
        d += 1;
        s += 2;
    }
}

void aud_data_memcpy_i32_8ch_from_i32_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[1];
        d[3] = s[1];
        d[4] = s[2];
        d[5] = s[2];
        d[6] = s[3];
        d[7] = s[3];
        d += 8;
        s += 4;
    }
}

void aud_data_memcpy_i32_8ch_from_i32_8ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/32;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d += 8;
        s += 8;
    }
}

void aud_data_memcpy_i24_2ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 4;
    }
}

void aud_data_memcpy_i24_1ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int32_t *d = (int32_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = s[0];
        d += 1;
        s += 4;
    }
}

void aud_data_memcpy_i16_4ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = SRC16_CH_DATA(s[0]);
        d[1] = SRC16_CH_DATA(s[1]);
        d[2] = SRC16_CH_DATA(s[2]);
        d[3] = SRC16_CH_DATA(s[3]);
        d += 4;
        s += 4;
    }
}

void aud_data_memcpy_i16_2ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = SRC16_CH_DATA(s[0]);
        d[1] = SRC16_CH_DATA(s[1]);
        d += 2;
        s += 4;
    }
}

void aud_data_memcpy_i16_1ch_from_i32_l24_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int32_t *s = (int32_t *)src;
    uint32_t i = src_bytes/16;

    for (;i > 0; i--) {
        d[0] = SRC16_CH_DATA(s[0]);
        d += 1;
        s += 4;
    }
}

void aud_data_memcpy_i16_8ch_from_i16_4ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/8;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 8;
        s += 4;
    }
}

void aud_data_memcpy_i16_8ch_from_i16_2ch(uint8_t *dst, uint8_t *src, uint32_t src_bytes)
{
    int16_t *d = (int16_t *)dst;
    int16_t *s = (int16_t *)src;
    uint32_t i = src_bytes/4;

    for (;i > 0; i--) {
        d[0] = s[0];
        d[1] = s[1];
        d += 8;
        s += 2;
    }
}



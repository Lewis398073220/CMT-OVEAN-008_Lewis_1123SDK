/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#ifndef __SPATIAL_AUDIO_ALGO_CP_H__
#define __SPATIAL_AUDIO_ALGO_CP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    unsigned int reserved;
}spatial_audio_algo_status_t;

static inline void algo_set_uint8(unsigned char *dst, unsigned char v, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = v;
    }
}

static inline void algo_set_int16(short *dst, short v, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = v;
    }
}

static inline void algo_set_int32(int *dst, int v, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = v;
    }
}

static inline void algo_set_f32(float *dst, float v, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = v;
    }
}

static inline void algo_copy_uint8(unsigned char *dst, unsigned char *src, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = src[i];
    }
}

static inline void algo_copy_int16(short *dst, short *src, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = src[i];
    }
}

static inline void algo_copy_int32(int *dst, int *src, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = src[i];
    }
}

static inline void algo_copy_f32(float *dst, float *src, int len)
{
    for (int i=0; i<len; i++) {
        dst[i] = src[i];
    }
}

int spatial_audio_algo_cp_init(int frame_in_len,int frame_out_len, int channel_num);

int spatial_audio_algo_cp_deinit(void);

int spatial_audio_algo_cp_process(short *pcm_buf, short *ref_buf, int _pcm_in_len,int *_pcm_out_len, spatial_audio_algo_status_t *status);

#ifdef __cplusplus
}
#endif
bool spatial_audio_cp_enable(void);

#endif


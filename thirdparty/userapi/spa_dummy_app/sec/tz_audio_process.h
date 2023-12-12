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

#ifndef __TZ_AUDIO_PROCESS_H__
#define __TZ_AUDIO_PROCESS_H__

typedef struct{
    unsigned int fs;
    unsigned int bits;
    unsigned int channels;
    unsigned int frame_len;
}tz_audio_process_info_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM_CMNS
int tz_audio_process_info_init(void * in_info);

int tz_audio_process_info_deinit(void);

void tz_audio_process_demo_func_enable_disable(bool enable_disable);

void tz_audio_process_stream_proc_handler(void * stream_buf, unsigned int stream_len);
#else
#define tz_audio_process_info_init tz_audio_process_info_init_cmse

#define tz_audio_process_info_deinit tz_audio_process_info_deinit_cmse

#define tz_audio_process_demo_func_enable_disable tz_audio_process_demo_func_enable_disable_cmse

#define tz_audio_process_stream_proc_handler tz_audio_process_stream_proc_handler_cmse
#endif

#if defined(ARM_CMSE) || defined(ARM_CMNS)
int tz_audio_process_info_init_cmse(void * in_info);

int tz_audio_process_info_deinit_cmse(void);

void tz_audio_process_demo_func_enable_disable_cmse(bool enable_disable);

void tz_audio_process_stream_proc_handler_cmse(void * stream_buf, unsigned int stream_len);
#endif


#ifdef __cplusplus
}
#endif

#endif


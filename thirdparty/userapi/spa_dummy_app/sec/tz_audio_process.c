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

#include "tz_audio_process.h"

/*
    design for ARM_CMSE interface
*/
#ifdef ARM_CMSE
static tz_audio_process_info_t _audio_info;

static bool demo_func_disable = false;

int tz_audio_process_info_init(void * in_info)
{
    if(in_info == NULL){
        return -1;
    }
    _audio_info = *(tz_audio_process_info_t*)in_info;

    return 0;
}

int tz_audio_process_info_deinit(void)
{
    return 0;
}

void tz_audio_process_demo_func_enable_disable(bool enable_disable)
{
    TRACE(1,"%s enable_disable [%d] [%d]",__func__,demo_func_disable,enable_disable);

    demo_func_disable = enable_disable;
}

void tz_audio_process_stream_proc_handler(void * stream_buf, unsigned int stream_len)
{
    unsigned char bits_bytes = 0;
    unsigned char stream_chnls = 0;

    if(demo_func_disable == true){
        return ;
    }

    if(_audio_info.bits == 16){
        bits_bytes = sizeof(short);
    }else if ((_audio_info.bits == 24) ||
        (_audio_info.bits == 32)){
        bits_bytes = sizeof(int);
    }else{
        ASSERT(0,"%s info : fs %d bits %d chnls %d frame_len %d",__func__,_audio_info.fs,_audio_info.bits,_audio_info.channels,_audio_info.frame_len);
    }

    unsigned int frame_len = stream_len / sizeof(_audio_info.channels) / sizeof(bits_bytes);

    if(frame_len != _audio_info.frame_len){
        ASSERT(0,"%s info : fs %d bits %d chnls %d frame_len %d %d",__func__,_audio_info.fs,_audio_info.bits,_audio_info.channels,_audio_info.frame_len,stream_len);
    }

    stream_chnls = _audio_info.channels;

    /*
        process handler for audio stream
    */
    // e.g. stream do left shitf 2 bits

    short  *sample_p = (short  *)stream_buf;
    unsigned int i = 0;

    if (stream_chnls == 1){
        for (i = 0; i < frame_len * stream_chnls; i += 1){
            sample_p[i + 0] = (int16_t)(sample_p[i + 0] * 0.2 );
        }
    }else if (stream_chnls == 2){
        for (i = 0; i < frame_len * stream_chnls; i += 2){
            sample_p[i + 0] = (int16_t)(sample_p[i + 0]* 0.2);
            sample_p[i + 1] = (int16_t)(sample_p[i + 1]*0.2);
        }
    }

}

#endif


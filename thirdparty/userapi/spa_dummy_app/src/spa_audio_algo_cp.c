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
#if defined(SPATIAL_AUDIO_ALGO_CP_ACCEL)
#include "cp_accel.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "math.h"
#include "norflash_api.h"
#include "spa_audio_algo_cp.h"
#include "heap_api.h"

// malloc data from pool in init function
#ifdef SPATIAL_AUDIO_ALGO_INPUT_16BIS
#define FRAME_LEN_IN_MAX           (1024)    //1024 per single channel frame, short pcm data
#else
#define FRAME_LEN_IN_MAX           (1024*(sizeof(int)/sizeof(short)))    //1024 per single channel frame, int pcm data
#endif
#define FRAME_LEN_OUT_MAX           (1024*(sizeof(int)/sizeof(short)))     //1024 per single channel frame, int pcm data
#define CHANNEL_NUM_MAX         (2)
#define FRAME_CACHE_DEPTH         (2)

enum CP_SPATIAL_AUDIO_ALGO_STATE_T {
    CP_SPATIAL_AUDIO_ALGO_STATE_NONE = 0,
    CP_SPATIAL_AUDIO_ALGO_STATE_IDLE,
    CP_SPATIAL_AUDIO_ALGO_STATE_WORKING,
};

enum SPATIAL_AUDIO_ALGO_CP_CMD_T {
    SPATIAL_AUDIO_ALGO_CP_CMD_PRO = 0,
    SPATIAL_AUDIO_ALGO_CP_CMD_OTHER,

    SPATIAL_AUDIO_ALGO_CP_CMD_QTY,
};

#if 0
#undef CP_DATA_LOC
#undef CP_BSS_LOC

#define RAM_A2DP_DECODER_THIRDPARTY_ALGO_LOC_A(n, l)               __attribute__((section(#n "." #l)))
#define RAM_A2DP_DECODER_THIRDPARTY_ALGO_LOC(n, l)                 RAM_A2DP_DECODER_THIRDPARTY_ALGO_LOC_A(n, l)

#define CP_DATA_LOC                       RAM_A2DP_DECODER_THIRDPARTY_ALGO_LOC(.overlay_a2dp_decoder_thirdparty_algo_cache, __LINE__)
#define CP_BSS_LOC                        RAM_A2DP_DECODER_THIRDPARTY_ALGO_LOC(.overlay_a2dp_decoder_thirdparty_algo_cache, __LINE__)
#endif

static CP_DATA_LOC enum CP_SPATIAL_AUDIO_ALGO_STATE_T g_cp_state = CP_SPATIAL_AUDIO_ALGO_STATE_NONE;

// TODO: Use malloc to replace array
// static CP_BSS_LOC char g_cp_heap_buf[1024 * 100];
static CP_BSS_LOC short g_in_pcm_buf[FRAME_LEN_IN_MAX * CHANNEL_NUM_MAX * FRAME_CACHE_DEPTH];
static CP_BSS_LOC short g_out_pcm_buf[FRAME_LEN_OUT_MAX * CHANNEL_NUM_MAX * FRAME_CACHE_DEPTH];
//static CP_BSS_LOC short g_in_ref_buf[FRAME_LEN_MAX];
static CP_BSS_LOC int g_pcm_in_len;
static CP_BSS_LOC int g_pcm_out_len;

static CP_BSS_LOC int g_channel_num;

static CP_BSS_LOC spatial_audio_algo_status_t g_spatial_audio_algo_status_in;
static CP_BSS_LOC spatial_audio_algo_status_t g_spatial_audio_algo_status_out;

static int g_require_cnt = 0;
static int g_run_cnt = 0;

CP_TEXT_SRAM_LOC
static int spatial_audio_algo_cp_algo(short *pcm_buf, short *ref_buf, int *_pcm_len, spatial_audio_algo_status_t *status)
{
    //TO DO

    return 0;
}

CP_TEXT_SRAM_LOC
static unsigned int spatial_audio_algo_cp_main(unsigned char event)
{
#ifdef TRACE_CP_ACCEL
    SPA_LOG_I("[%s] g_run_cnt: %d", __func__, g_run_cnt);
#endif

    // LOCK BUFFER

    // process pcm
#if 0
    // speech_copy_int16(g_out_pcm_buf, g_in_pcm_buf, g_pcm_len);

    for (int i = 0; i < g_pcm_len; i++)
    {
        g_out_pcm_buf[i] = (short)(sinf(2 * 3.1415926 * i / 16 ) * 10000);
    }
#else
    int frame_in_len = g_pcm_in_len / sizeof(short);
    int frame_out_len = g_pcm_out_len;
    algo_copy_int16(g_out_pcm_buf, g_in_pcm_buf, frame_in_len);
    spatial_audio_algo_cp_algo(g_out_pcm_buf, NULL, &frame_out_len, &g_spatial_audio_algo_status_in);
    memcpy(&g_spatial_audio_algo_status_out, &g_spatial_audio_algo_status_in, sizeof(spatial_audio_algo_status_t));
#endif

    // set status
    g_run_cnt++;
    g_cp_state = CP_SPATIAL_AUDIO_ALGO_STATE_IDLE;

#ifdef TRACE_CP_ACCEL
    SPA_LOG_I("[%s] CP_SPATIAL_AUDIO_ALGO_STATE_IDLE", __func__);
#endif

    // UNLOCK BUFFER

    return 0;
}
CP_DATA_LOC
static const struct cp_task_desc task_desc_spatial_audio_algo = 
{
    CP_ACCEL_STATE_CLOSED,
    spatial_audio_algo_cp_main,
    NULL,
    NULL,
    NULL
};

int spatial_audio_algo_cp_init(int frame_in_len,int frame_out_len, int channel_num)
{
    SPA_LOG_I("[%s] frame_len: %d %d, channel_num: %d", __func__, frame_in_len,frame_out_len, channel_num);
    ASSERT(frame_in_len <= FRAME_LEN_IN_MAX*sizeof(short), "[%s] frame_len(%d) > FRAME_LEN_MAX", __func__, frame_in_len);
    ASSERT(frame_out_len <= FRAME_LEN_OUT_MAX*sizeof(short), "[%s] frame_len(%d) > FRAME_LEN_MAX", __func__, frame_out_len);
    ASSERT(channel_num <= CHANNEL_NUM_MAX, "[%s] channel_num(%d) > CHANNEL_NUM_MAX", __func__, channel_num);

    g_require_cnt = 0;
    g_run_cnt = 0;

    if(!cp_accel_init_done())
    {
        cp_accel_init();
    }
    norflash_api_flush_disable(NORFLASH_API_USER_CP,(unsigned int)cp_accel_init_done);
    cp_accel_open(CP_TASK_SPATIAL_AUDIO_ALGO, &task_desc_spatial_audio_algo);

    unsigned int cnt=0;
    while(cp_accel_init_done() == false) {
        hal_sys_timer_delay_us(100);
        cnt++;
        if (cnt % 10 == 0) {
            if (cnt == 10 * 200) {     // 200ms
                ASSERT(0, "[%s] ERROR: Can not init cp!!!", __func__);
            } else {
                SPA_LOG_I( "[%s] Wait CP init done...%d(ms)", __func__, cnt/10);
            }
        }
    }
    norflash_api_flush_enable(NORFLASH_API_USER_CP);
#if 0
    speech_heap_cp_start();
    speech_heap_add_block(g_cp_heap_buf, sizeof(g_cp_heap_buf));
    speech_heap_cp_end();
#endif

    g_channel_num = channel_num;
    g_pcm_in_len = frame_in_len * g_channel_num;
    g_pcm_out_len = frame_out_len * g_channel_num;          // Initialize output pcm_len

    algo_set_int16(g_in_pcm_buf, 0, g_pcm_in_len/sizeof(short));
    algo_set_int16(g_out_pcm_buf, 0, g_pcm_out_len /sizeof(short));
//    algo_set_int16(g_in_ref_buf, 0, g_frame_len);
    g_cp_state = CP_SPATIAL_AUDIO_ALGO_STATE_IDLE;

    SPA_LOG_I("[%s] status = %d", __func__, g_cp_state);

    return 0;

}

int spatial_audio_algo_cp_deinit(void)
{
    SPA_LOG_I("[%s] ...", __func__);

    cp_accel_close(CP_TASK_SPATIAL_AUDIO_ALGO);

    g_cp_state = CP_SPATIAL_AUDIO_ALGO_STATE_NONE;

    return 0;
}

int spatial_audio_algo_cp_process(short *pcm_buf, short *ref_buf, int _pcm_in_len,int *_pcm_out_len, spatial_audio_algo_status_t *status)
{
    unsigned int wait_cnt = 0;
    int frame_out_len = 0;
    g_pcm_in_len = _pcm_in_len;
    int frame_in_len = g_pcm_in_len / sizeof(short);
    // ASSERT(g_frame_len * g_channel_num == pcm_len, "[%s] g_frame_len(%d) * g_channel_num(%d) != pcm_len(%d)", __func__, g_frame_len, g_channel_num, pcm_len);

    // Check CP has new data to get and can get data from buffer
#ifdef TRACE_CP_ACCEL
    SPA_LOG_I"[%s] g_require_cnt: %d, status: %d, pcm_len: %d", __func__, g_require_cnt, g_cp_state, _pcm_in_len);
#endif
    //SPA_LOG_I("require [%d]: %d %d %d",__LINE__,_pcm_in_len,*_pcm_out_len,g_pcm_out_len);
#ifdef TRACE_CP_ACCEL
    unsigned int time_s = hal_fast_sys_timer_get();
#endif
    while (g_cp_state == CP_SPATIAL_AUDIO_ALGO_STATE_WORKING)
    {
        hal_sys_timer_delay_us(10);

        if (wait_cnt++ > 300000) {      // 3s
            ASSERT(0, "cp is hung %d", g_cp_state);
        }
    }

    if (g_cp_state == CP_SPATIAL_AUDIO_ALGO_STATE_IDLE)
    {
        algo_copy_int16(g_in_pcm_buf, pcm_buf, frame_in_len);
//        if (ref_buf)
//        {
//            speech_copy_int16(g_in_ref_buf, ref_buf, pcm_len / g_channel_num);
//        }
        memcpy(&g_spatial_audio_algo_status_in, status, sizeof(spatial_audio_algo_status_t));

        frame_out_len = g_pcm_out_len / sizeof(short);
        algo_copy_int16(pcm_buf, g_out_pcm_buf, frame_out_len);
        memcpy(status, &g_spatial_audio_algo_status_out, sizeof(spatial_audio_algo_status_t));

        *_pcm_out_len = g_pcm_out_len;
//        g_pcm_len = pcm_len;

        g_require_cnt++;
        g_cp_state = CP_SPATIAL_AUDIO_ALGO_STATE_WORKING;
        cp_accel_send_event_mcu2cp(CP_BUILD_ID(CP_TASK_SPATIAL_AUDIO_ALGO, CP_EVENT_SPATIAL_AUDIO_ALGO));
    }
    else
    {
        // Multi channels to one channel
#if 0
        for (int i = 0; i < pcm_len / g_channel_num; i++)
        {
            pcm_buf[i] = pcm_buf[2*i];
        }

        *_pcm_len = pcm_len / g_channel_num;
#endif

        // Check abs(g_require_cnt - g_run_cnt) > threshold, reset or assert

        SPA_LOG_I("[%s] ERROR: status = %d", __func__, g_cp_state);
    }
    //SPA_LOG_I("require [%d]: %d %d %d",__LINE__,_pcm_in_len,*_pcm_out_len,g_pcm_out_len);
#ifdef TRACE_CP_ACCEL
    SPA_LOG_I("time cost = %d(us)",FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_s));
#endif
    return 0;
}

#endif

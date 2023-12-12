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
#ifndef __SPA_COMMON_H__
#define __SPA_COMMON_H__

#include "hal_trace.h"
#include "hal_timer.h"

/******************************macro defination*****************************/

/* SPA CTRL PARTION INFO*/
#ifndef SPA_AUDIO_HANDLE_DATA_CHNL_NUM
#define SPA_AUDIO_HANDLE_DATA_CHNL_NUM  (2)
#endif

#ifndef SPA_AUDIO_HANDLE_DATA_BITS
#define SPA_AUDIO_HANDLE_DATA_BITS  (16)
#endif

#ifndef SPA_AUDIO_HANDLE_BLOCK_SAMPLES  
#define SPA_AUDIO_HANDLE_BLOCK_SAMPLES  (1024)
#endif

#ifndef SPA_AUDIO_HANDLE_BLOCK_SAMPLE_RATE
#define SPA_AUDIO_HANDLE_BLOCK_SAMPLE_RATE  (44100)
#endif
/*End of SPA CTRL PARTION INFO*/

/* SPA SENS PARTION INFO*/
#ifndef SPA_SENS_HANDLE_DATA_CHNL_NUM
#define SPA_SENS_HANDLE_DATA_CHNL_NUM   (4)
#endif

#ifndef SPA_SENS_HANDLE_DATA_BITS
#define SPA_SENS_HANDLE_DATA_BITS  (16)
#endif

#ifndef SPA_SENS_HANDLE_BLOCK_SAMPLES
#define SPA_SENS_HANDLE_BLOCK_SAMPLES  (1)
#endif

#ifndef SPA_SENS_HANDLE_BLOCK_SAMPLE_RATE
#define SPA_SENS_HANDLE_BLOCK_SAMPLE_RATE  (100)
#endif
/*End of SPA SENS PARTION INFO*/

/******     debugging defination    ******/
#define SPA_MODULE_TRACE_LEVEL  TR_LEVEL_INFO

#define SPA_LOG_V(str, ...) if (SPA_MODULE_TRACE_LEVEL >= TR_LEVEL_VERBOSE) TR_VERBOSE(TR_MOD(SPA), str, ##__VA_ARGS__)
#define SPA_LOG_D(str, ...) if (SPA_MODULE_TRACE_LEVEL >= TR_LEVEL_DEBUG)   TR_DEBUG(TR_MOD(SPA), str, ##__VA_ARGS__)
#define SPA_LOG_I(str, ...) if (SPA_MODULE_TRACE_LEVEL >= TR_LEVEL_INFO)    TR_INFO(TR_MOD(SPA), str, ##__VA_ARGS__)
#define SPA_LOG_W(str, ...) if (SPA_MODULE_TRACE_LEVEL >= TR_LEVEL_WARN)    TR_WARN(TR_MOD(SPA), str, ##__VA_ARGS__)
#define SPA_LOG_E(str, ...) if (SPA_MODULE_TRACE_LEVEL >= TR_LEVEL_ERROR)   TR_ERROR(TR_MOD(SPA), str, ##__VA_ARGS__)
#define SPA_LOG_THROTTLED(level, isFreshOnly, threshold, ...) \
do { \
    static uint32_t last_ts = 0; \
    uint32_t now_ts = GET_CURRENT_MS(); \
    if (isFreshOnly) {  \
        last_ts = now_ts; \
    }   \
    if (!last_ts || (now_ts - last_ts) > (threshold)) { \
        CC_DECODE_LOG_##level(__VA_ARGS__); \
        last_ts = GET_CURRENT_MS(); \
    } \
} while(0)

#define SPA_LOG_EVERY_1_S             (1000)
#define SPA_LOG_EVERY_100_MS          (100)

#ifndef SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_
#define SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_ (100)
#endif
/******     end of debugging defination    ******/
/******************************end of macro defination*****************************/

typedef enum{
    SPA_ALGO_OP_MASTER      = 0X0,
    SPA_ALGO_OP_SLAVE       = 0X01,
    SPA_ALGO_OP_UNKNOWN     = 0Xff,
}spa_algo_op_role_e;

typedef enum {
    SPA_AUDIO_CHANNEL_ID_STEREO,
    SPA_AUDIO_CHANNEL_ID_LRMERGE,
    SPA_AUDIO_CHANNEL_ID_LEFT,
    SPA_AUDIO_CHANNEL_ID_RIGHT,    
}spa_audio_chan_id_e;

typedef enum{
    SPA_DATA_UPDATE_SOURCE_UNKNOWN,
    SPA_DATA_UPDATE_SOURCE_TWS,
    SPA_DATA_UPDATE_SOURCE_SENS,
    SPA_DATA_UPDATE_SOURCE_BTH,
    SPA_DATA_UPDATE_SOURCE_M55,
    SPA_DATA_UPDATE_SOURCE_TUNE,
}spa_data_update_source_e;

typedef struct{
    char name[4];
    unsigned int len;
}msg_header_t;

typedef struct{
    unsigned char data_channel;
    unsigned char data_bits;
    unsigned int block_samples;
    unsigned int sample_rate;
}data_info_body_t;

typedef struct{
    msg_header_t msg_head;
    data_info_body_t payload;
}data_info_t;

typedef struct{
    int data_Y;
    int data_P;
    int data_R;
    int data_opt;
}sens_msg_body_t;

typedef struct{
    msg_header_t msg_head;
    sens_msg_body_t payload;
}data_sens_t;

typedef struct{
    unsigned char * data_ptr;
    unsigned int data_size;
    unsigned int data_opt;
    spa_algo_op_role_e algo_op_role;
}data_aud_buffer_t;

typedef struct{
    data_info_t info;
    data_sens_t sens;
    data_aud_buffer_t buffer;
}data_aud_body_t;

typedef struct{
    msg_header_t msg_head;
    data_aud_body_t payload;
}data_aud_t;

typedef int(*spa_algo_post_handler_if_func)(void * buf,unsigned int len);
typedef int(*spa_raw_data_relay_func)(void*buf,unsigned int len);

typedef struct{
    spa_algo_post_handler_if_func func_algo_run;
    spa_raw_data_relay_func func_relay;
}spa_algo_op_handler_t;

typedef struct{
    /* internal data buffer */
    unsigned char * data_buf;
    int data_buf_size;
}spa_buffer_t;

typedef int(*ext_alloc_func_t)(unsigned char ** buf,unsigned int len);


#endif

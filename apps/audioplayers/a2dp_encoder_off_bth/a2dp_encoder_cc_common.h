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
#ifndef __A2DP_ENCODER_CC_COMMON_H__
#define __A2DP_ENCODER_CC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "a2dp_encoder_internal.h"

#define CC_ENCODE_MODULE_TRACE_LEVEL  TR_LEVEL_WARN

/******************************macro defination*****************************/
#define CC_ENCODE_LOG_V(str, ...) if (CC_ENCODE_MODULE_TRACE_LEVEL >= TR_LEVEL_VERBOSE) TR_VERBOSE(TR_MOD(CC_ENCODE), str, ##__VA_ARGS__)
#define CC_ENCODE_LOG_D(str, ...) if (CC_ENCODE_MODULE_TRACE_LEVEL >= TR_LEVEL_DEBUG)   TR_DEBUG(TR_MOD(CC_ENCODE), str, ##__VA_ARGS__)
#define CC_ENCODE_LOG_I(str, ...) if (CC_ENCODE_MODULE_TRACE_LEVEL >= TR_LEVEL_INFO)    TR_INFO(TR_MOD(CC_ENCODE), str, ##__VA_ARGS__)
#define CC_ENCODE_LOG_W(str, ...) if (CC_ENCODE_MODULE_TRACE_LEVEL >= TR_LEVEL_WARN)    TR_WARN(TR_MOD(CC_ENCODE), str, ##__VA_ARGS__)
#define CC_ENCODE_LOG_E(str, ...) if (CC_ENCODE_MODULE_TRACE_LEVEL >= TR_LEVEL_ERROR)   TR_ERROR(TR_MOD(CC_ENCODE), str, ##__VA_ARGS__)
#define CC_ENCODE_LOG_THROTTLED(level, isFreshOnly, threshold, ...) \
do { \
    static uint32_t last_ts = 0; \
    uint32_t now_ts = GET_CURRENT_MS(); \
    if (isFreshOnly) {  \
        last_ts = now_ts; \
    }   \
    if (!last_ts || (now_ts - last_ts) > (threshold)) { \
        CC_ENCODE_LOG_##level(__VA_ARGS__); \
        last_ts = GET_CURRENT_MS(); \
    } \
} while(0)

#define CC_ENCODE_LOG_EVERY_1_S             (1000)
#define CC_ENCODE_LOG_EVERY_100_MS          (100)
#define CC_ENCODE_VALUE_MAX(A,B)            ((A) > (B) ? (A) : (B))
#define CC_ENCODE_SBC_PCMLEN_DEFAULT        (512)
// @APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE
#define CC_ENCODER_MAX_LEN_IN_MAILBOX       (512)

typedef enum
{
    A2DP_ENCODER_CODEC_TYPE_SBC             = (1u<<0),
    A2DP_ENCODER_CODEC_TYPE_MPEG2_4_AAC     = (1u<<1),
    A2DP_ENCODER_CODEC_TYPE_OPUS            = (1u<<2),
    A2DP_ENCODER_CODEC_TYPE_SCALABL         = (1u<<3),
    A2DP_ENCODER_CODEC_TYPE_LHDC            = (1u<<4),
    A2DP_ENCODER_CODEC_TYPE_LDAC            = (1u<<5),
    A2DP_ENCODER_CODEC_TYPE_LC3             = (1u<<6),
} A2DP_ENCODER_CODEC_TYPE_E;

typedef struct
{
    // for removing the entry from list after receiving the ack from peer core
    uint8_t*        org_addr;
    // for bth to remove the encoded data packet entry when finally feeding the
    // cooresponding decoded pcm data packet back to it
    void*           coorespondingPcmDataPacketPtr;
    uint16_t        pcm_list_len;
    uint16_t        data_len;
    uint8_t         *data;
} a2dp_encoder_cc_media_data_t;

typedef struct  {
    uint32_t sample_rate;
    uint8_t num_channels;
    uint8_t bits_depth;
    uint32_t frame_samples;
} A2DP_AUDIO_CC_OUTPUT_CONFIG_T;

typedef struct
{
    A2DP_ENCODER_CODEC_TYPE_E       codecType;
    uint16_t                        maxEncodeDataSizePerFrame;
    A2DP_AUDIO_CC_OUTPUT_CONFIG_T   stream_cfg;
} A2DP_ENCODER_INIT_REQ_T;

typedef struct
{
    uint32_t reserve;

} A2DP_ENCODER_INIT_RSP_T;

typedef struct
{
    a2dp_encoder_cc_media_data_t ackedDataPacket;
    uint16_t encodedPacketEntryCount;
    uint16_t pcmPacketEntryCount;
} A2DP_ENCODER_ACK_FED_ENCODED_DATA_REQ_T;


typedef void (*AUDIO_ENCODER_CC_INIT)(void);
typedef void (*AUDIO_ENCODER_CC_DEINIT)(void);
typedef bool (*AUDIO_ENCODER_CC_ENCODE_FRAME)(a2dp_encoder_cc_media_data_t*, a2dp_encoder_cc_media_data_t*);
typedef void (*AUDIO_ENCODER_CC_GET_SYNC_CUSTOM_DATA)(uint8_t* customData);
typedef void (*AUDIO_ENCODER_CC_PUSH_SYNC_CUSTOM_DATA)(uint8_t* customData);

typedef struct {
    A2DP_AUDIO_CC_OUTPUT_CONFIG_T stream_config;
    AUDIO_ENCODER_CC_INIT audio_encoder_cc_init;
    AUDIO_ENCODER_CC_DEINIT audio_encoder_cc_deinit;
    AUDIO_ENCODER_CC_ENCODE_FRAME audio_encoder_cc_encode_frame;
    AUDIO_ENCODER_CC_GET_SYNC_CUSTOM_DATA audio_encoder_cc_get_sync_custom_data;
    AUDIO_ENCODER_CC_PUSH_SYNC_CUSTOM_DATA audio_encoder_cc_push_sync_custom_data;
} A2DP_AUDIO_CC_ENCODER_T;

uint16_t a2dp_encoder_cc_check_seq_number(
    uint16_t actualSeq, uint16_t exepctedSeq,
    uint16_t totalSubSeqCount, uint16_t actualSubSeq, uint16_t expectedSubSeq);

void a2dp_encoder_cc_accumulate_cmd_send(uint16_t cmd_code, uint8_t *data);
void a2dp_encoder_cc_cmd_cache_init(void);
void a2dp_encoder_cc_cmd_cache_deinit(void);

void a2dp_encoder_cc_common_set_cmd_cached(bool isTriggered);
bool a2dp_encoder_cc_common_is_cmd_cached(void);
#ifdef A2DP_ENCODER_CROSS_CORE_USE_M55
#define RPC_CROSS_CORE_TASK_CMD_TO_ADD      M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD
#define APP_RPC_CROSS_CORE                  APP_RPC_CORE_BTH_M55
#else
#define RPC_CROSS_CORE_TASK_CMD_TO_ADD      RPC_BTH_DSP_TASK_CMD_TO_ADD
#define APP_RPC_CROSS_CORE                  APP_RPC_CORE_BTH_DSP
#endif

#ifdef __cplusplus
}
#endif

#endif


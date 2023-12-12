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
#ifdef A2DP_ENCODER_CROSS_CORE

#include "a2dp_encoder_cc_common.h"
#include "app_dsp_m55.h"
#include "cmsis_os2.h"
#include "cmsis_os.h"
#include "cmsis.h"
#include <string.h>
static bool isA2dpCommandCached = false;

void a2dp_encoder_cc_common_set_cmd_cached(bool isCmdCacheEnabled)
{
    isA2dpCommandCached = isCmdCacheEnabled;
}

bool a2dp_encoder_cc_common_is_cmd_cached(void)
{
    return isA2dpCommandCached;
}

uint16_t a2dp_encoder_cc_check_seq_number(uint16_t actualSeq,
                                          uint16_t exepctedSeq,
                                          uint16_t totalSubSeqCount,
                                          uint16_t actualSubSeq,
                                          uint16_t expectedSubSeq)
{
    uint16_t checkResult = 0;

    return checkResult;
}

typedef struct
{
    uint8_t cachedCmdBuf[CC_ENCODER_MAX_LEN_IN_MAILBOX];
    uint32_t cachedCmdLen;
    osTimerId_t supervisorTimerId;
} A2DP_ENCODER_CC_CMD_CACHE_ENV_T;

typedef enum
{
    A2DP_FEED_ENCODED_DATA_PACKET_E     = 0,
    A2DP_ACK_FED_PCM_DATA_PACKET_E      = 1,
    A2DP_FEED_PCM_DATA_PACKET_E         = 2,
    A2DP_REMOVE_SPECIFIC_FRAME          = 3,
    A2DP_CACHED_CMD_TYPE_COUNT          = 4,
} A2DP_CACHED_CMD_TYPE_INDEX_E;

#define A2DP_ENCODER_CMD_CACHE_TIMEOUT_MS   5

static A2DP_ENCODER_CC_CMD_CACHE_ENV_T a2dp_encoder_cc_cmd_cache_env[A2DP_CACHED_CMD_TYPE_COUNT];
static void a2dp_feed_encoded_data_packet_cmd_cache_supervisor_timer_callback(void* param)
{
    A2DP_ENCODER_CC_CMD_CACHE_ENV_T* env = &a2dp_encoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E];
    if (env->cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET,
                                    env->cachedCmdBuf, env->cachedCmdLen);
        env->cachedCmdLen = 0;
    }
}

static void a2dp_ack_fed_pcm_data_packet_cmd_cache_supervisor_timer_callback(void* param)
{
    A2DP_ENCODER_CC_CMD_CACHE_ENV_T* env = &a2dp_encoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E];
    if (env->cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET,
                                    env->cachedCmdBuf, env->cachedCmdLen);
        env->cachedCmdLen = 0;
    }
}

static void a2dp_feed_pcm_data_packet_cmd_cache_supervisor_timer_callback(void* param)
{
    A2DP_ENCODER_CC_CMD_CACHE_ENV_T* env = &a2dp_encoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E];
    if (env->cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET,
                                    env->cachedCmdBuf, env->cachedCmdLen);
        env->cachedCmdLen = 0;
    }
}

static void a2dp_remove_specific_frame_cmd_cache_supervisor_timer_callback(void* param)
{
    A2DP_ENCODER_CC_CMD_CACHE_ENV_T* env = &a2dp_encoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME];
    if (env->cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME,
                                    env->cachedCmdBuf, env->cachedCmdLen);
        env->cachedCmdLen = 0;
    }
}

void a2dp_encoder_cc_cmd_cache_init(void)
{
    for (uint32_t index = A2DP_FEED_ENCODED_DATA_PACKET_E;index < A2DP_CACHED_CMD_TYPE_COUNT;index++)
    {
        a2dp_encoder_cc_cmd_cache_env[index].cachedCmdLen = 0;
    }

    osTimerAttr_t timerattr;
    memset(&timerattr, 0, sizeof(timerattr));
    timerattr.name = "A2DP_FEED_ENCODED_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER";
    a2dp_encoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].supervisorTimerId =
        osTimerNew(a2dp_feed_encoded_data_packet_cmd_cache_supervisor_timer_callback, osTimerOnce, NULL, &timerattr);
    timerattr.name = "A2DP_ACK_FED_PCM_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER";
    a2dp_encoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].supervisorTimerId =
        osTimerNew(a2dp_ack_fed_pcm_data_packet_cmd_cache_supervisor_timer_callback, osTimerOnce, NULL, &timerattr);
    timerattr.name = "A2DP_FEED_PCM_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER";
    a2dp_encoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].supervisorTimerId =
        osTimerNew(a2dp_feed_pcm_data_packet_cmd_cache_supervisor_timer_callback, osTimerOnce, NULL, &timerattr);
    timerattr.name = "A2DP_REMOVE_SPECIFIC_FRAME_CMD_CACHE_SUPERVISOR_TIMER";
    a2dp_encoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].supervisorTimerId =
        osTimerNew(a2dp_remove_specific_frame_cmd_cache_supervisor_timer_callback, osTimerOnce, NULL, &timerattr);

    if (!a2dp_encoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].supervisorTimerId ||
        !a2dp_encoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].supervisorTimerId  ||
        !a2dp_encoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].supervisorTimerId     ||
        !a2dp_encoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].supervisorTimerId)
    {
        ASSERT(false, "supervisorTimer create faile");
    }

}

void a2dp_encoder_cc_cmd_cache_deinit(void)
{
    for (uint32_t index = A2DP_FEED_ENCODED_DATA_PACKET_E;index < A2DP_CACHED_CMD_TYPE_COUNT;index++)
    {
        a2dp_encoder_cc_cmd_cache_env[index].cachedCmdLen = 0;
        osTimerStop(a2dp_encoder_cc_cmd_cache_env[index].supervisorTimerId);
    }
}

bool a2dp_encoder_cc_cache_cmd(uint16_t cmd_code, uint8_t *data)
{
    A2DP_CACHED_CMD_TYPE_INDEX_E cachedCmdIndex;
    uint16_t unitOfInfo = 0;
    switch(cmd_code) {
        case CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET:
            cachedCmdIndex = A2DP_FEED_ENCODED_DATA_PACKET_E;
            break;
        case CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET:
            cachedCmdIndex = A2DP_ACK_FED_PCM_DATA_PACKET_E;
            break;
        case CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET:
            cachedCmdIndex = A2DP_FEED_PCM_DATA_PACKET_E;
            break;
        default:
            return false;
    }
    unitOfInfo = sizeof(a2dp_encoder_cc_media_data_t);
    A2DP_ENCODER_CC_CMD_CACHE_ENV_T* env = &a2dp_encoder_cc_cmd_cache_env[cachedCmdIndex];
    osTimerStop(env->supervisorTimerId);
    if (env->cachedCmdLen + unitOfInfo >
        CC_ENCODER_MAX_LEN_IN_MAILBOX)
    {
        app_dsp_m55_bridge_send_cmd(cmd_code, env->cachedCmdBuf, env->cachedCmdLen);
        env->cachedCmdLen = 0;
    }
    uint32_t lock = int_lock();
    memcpy(env->cachedCmdBuf + env->cachedCmdLen, data, unitOfInfo);
    env->cachedCmdLen += unitOfInfo;
    int_unlock(lock);
    osTimerStart(env->supervisorTimerId, A2DP_ENCODER_CMD_CACHE_TIMEOUT_MS);
    return true;
}

void a2dp_encoder_cc_accumulate_cmd_send(uint16_t cmd_code, uint8_t *data)
{
    if (a2dp_encoder_cc_common_is_cmd_cached())
    {
        bool isCached = a2dp_encoder_cc_cache_cmd(cmd_code, data);
        if (isCached)
        {
            return;
        }
    }

    uint8_t fetchedData[CC_ENCODER_MAX_LEN_IN_MAILBOX];
    uint16_t lenFetched = 0;
    uint8_t maxAllowCnt = 0;
    uint16_t unitOfInfo = 0;
    uint16_t threshold = CC_ENCODER_MAX_LEN_IN_MAILBOX;

    switch(cmd_code) {
        case CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET:
            break;
        case CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET:
            break;
        case CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET:
            break;
        default:
            ASSERT(0, "%s, Wrong cmdCode:0x%x", __func__, cmd_code);
        break;
    }
    unitOfInfo = sizeof(a2dp_encoder_cc_media_data_t);
    // Accumulate when there are too many mailbox cnts
    // @APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX
    if (app_dsp_m55_bridge_get_tx_mailbox_cnt() < 10) {
        goto send_directly;
    }

    maxAllowCnt = (CC_ENCODER_MAX_LEN_IN_MAILBOX / unitOfInfo);
    threshold = (maxAllowCnt - 1) * unitOfInfo;

    app_dsp_m55_bridge_fetch_tx_mailbox(cmd_code, fetchedData, &lenFetched, threshold);
send_directly:
    memcpy(fetchedData + lenFetched, data, unitOfInfo);
    app_dsp_m55_bridge_send_cmd(cmd_code, fetchedData, lenFetched + unitOfInfo);
}

#endif


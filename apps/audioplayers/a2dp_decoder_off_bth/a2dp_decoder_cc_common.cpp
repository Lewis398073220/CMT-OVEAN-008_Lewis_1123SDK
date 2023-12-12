/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifdef A2DP_DECODER_CROSS_CORE

#include "a2dp_decoder_cc_common.h"
#include "app_dsp_m55.h"

#define A2DP_SEQ_GAP_THRESHOULD_TO_RETRIGGER 32

static bool isA2dpCommandCached = false;

void a2dp_decoder_cc_common_set_cmd_cached(bool isCmdCacheEnabled)
{
    isA2dpCommandCached = isCmdCacheEnabled;
}

bool a2dp_decoder_cc_common_is_cmd_cached(void)
{
    return isA2dpCommandCached;
}

uint16_t a2dp_decoder_cc_check_seq_number(uint16_t actualSeq, uint16_t exepctedSeq,
    uint16_t totalSubSeqCount, uint16_t actualSubSeq, uint16_t expectedSubSeq)
{
    uint16_t checkResult = 0;
    CC_DECODE_LOG_D("expected seq %u actual seq %u expected subSeq %u actual subSeq %u",
        exepctedSeq, actualSeq, expectedSubSeq, actualSubSeq);

    if ((actualSeq == UINT16_MAX) ||
        ((exepctedSeq == actualSeq) && (expectedSubSeq == actualSubSeq)))
    {
        return A2DP_SEQ_CHECK_RESULT_MATCH;
    }
    else
    {
         if (ABS((int32_t)exepctedSeq - (int32_t)actualSeq) > A2DP_SEQ_GAP_THRESHOULD_TO_RETRIGGER)
         {
            checkResult |= A2DP_SEQ_CHECK_RESULT_DIFF_EXCEEDS_LIMIT;
         }

         if ((0 == totalSubSeqCount) || (exepctedSeq != actualSeq))
         {
            if (exepctedSeq > 0)
            {
                if (actualSeq > exepctedSeq)
                {
                    checkResult |= A2DP_SEQ_CHECK_RESULT_LATER_THAN_EXPECTED;
                }
                else
                {
                    checkResult |= A2DP_SEQ_CHECK_RESULT_EARLIER_THAN_EXPECTED;
                }
            }
            else
            {
                // roll back case
                if (actualSeq > 0xFF00)
                {
                    checkResult |= A2DP_SEQ_CHECK_RESULT_EARLIER_THAN_EXPECTED;
                }
                else
                {
                    checkResult |= A2DP_SEQ_CHECK_RESULT_LATER_THAN_EXPECTED;
                }
            }
        }
        // sub sequence is valid
        else
        {
            if (actualSubSeq > expectedSubSeq)
            {
                checkResult |= A2DP_SEQ_CHECK_RESULT_LATER_THAN_EXPECTED;
            }
            else
            {
                checkResult |= A2DP_SEQ_CHECK_RESULT_EARLIER_THAN_EXPECTED;
            }
        }
    }

    return checkResult;
}

typedef struct
{
    uint8_t cachedCmdBuf[CC_DECODER_MAX_LEN_IN_MAILBOX];
    uint32_t cachedCmdLen;
    osTimerId_t supervisorTimerId; 
} A2DP_DECODER_CC_CMD_CACHE_ENV_T;

typedef enum
{
    A2DP_FEED_ENCODED_DATA_PACKET_E     = 0,
    A2DP_ACK_FED_PCM_DATA_PACKET_E      = 1,
    A2DP_FEED_PCM_DATA_PACKET_E         = 2,
    A2DP_REMOVE_SPECIFIC_FRAME          = 3,
    A2DP_CACHED_CMD_TYPE_COUNT          = 4,
} A2DP_CACHED_CMD_TYPE_INDEX_E;

#define A2DP_DECODER_CMD_CACHE_TIMEOUT_MS   5

static A2DP_DECODER_CC_CMD_CACHE_ENV_T a2dp_decoder_cc_cmd_cache_env[A2DP_CACHED_CMD_TYPE_COUNT];
static void a2dp_feed_encoded_data_packet_cmd_cache_supervisor_timer_callback(void const *param)
{
    if (a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].cachedCmdBuf, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].cachedCmdLen);
        a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].cachedCmdLen = 0;
    }
}
osTimerDef (A2DP_FEED_ENCODED_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER, a2dp_feed_encoded_data_packet_cmd_cache_supervisor_timer_callback);

static void a2dp_ack_fed_pcm_data_packet_cmd_cache_supervisor_timer_callback(void const *param)
{
    if (a2dp_decoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].cachedCmdBuf, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].cachedCmdLen);
        a2dp_decoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].cachedCmdLen = 0;
    }
}
osTimerDef (A2DP_ACK_FED_PCM_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER, a2dp_ack_fed_pcm_data_packet_cmd_cache_supervisor_timer_callback);

static void a2dp_feed_pcm_data_packet_cmd_cache_supervisor_timer_callback(void const *param)
{
    if (a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].cachedCmdBuf, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].cachedCmdLen);
        a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].cachedCmdLen = 0;
    }
}
osTimerDef (A2DP_FEED_PCM_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER, a2dp_feed_pcm_data_packet_cmd_cache_supervisor_timer_callback);

static void a2dp_remove_specific_frame_cmd_cache_supervisor_timer_callback(void const *param)
{
    if (a2dp_decoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].cachedCmdLen > 0)
    {
        app_dsp_m55_bridge_send_cmd(CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].cachedCmdBuf, 
            a2dp_decoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].cachedCmdLen);
        a2dp_decoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].cachedCmdLen = 0;
    }
}
osTimerDef (A2DP_REMOVE_SPECIFIC_FRAME_CMD_CACHE_SUPERVISOR_TIMER, a2dp_remove_specific_frame_cmd_cache_supervisor_timer_callback);

void a2dp_decoder_cc_cmd_cache_init(void)
{
    for (uint32_t index = A2DP_FEED_ENCODED_DATA_PACKET_E;index < A2DP_CACHED_CMD_TYPE_COUNT;index++)
    {
        a2dp_decoder_cc_cmd_cache_env[index].cachedCmdLen = 0;
    }

    a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_ENCODED_DATA_PACKET_E].supervisorTimerId = 
        osTimerCreate(osTimer(A2DP_FEED_ENCODED_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER), osTimerOnce, NULL);
    a2dp_decoder_cc_cmd_cache_env[A2DP_ACK_FED_PCM_DATA_PACKET_E].supervisorTimerId = 
        osTimerCreate(osTimer(A2DP_ACK_FED_PCM_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER), osTimerOnce, NULL);
    a2dp_decoder_cc_cmd_cache_env[A2DP_FEED_PCM_DATA_PACKET_E].supervisorTimerId = 
        osTimerCreate(osTimer(A2DP_FEED_PCM_DATA_PACKET_CMD_CACHE_SUPERVISOR_TIMER), osTimerOnce, NULL);
    a2dp_decoder_cc_cmd_cache_env[A2DP_REMOVE_SPECIFIC_FRAME].supervisorTimerId = 
        osTimerCreate(osTimer(A2DP_REMOVE_SPECIFIC_FRAME_CMD_CACHE_SUPERVISOR_TIMER), osTimerOnce, NULL);

}

void a2dp_decoder_cc_cmd_cache_deinit(void)
{
    for (uint32_t index = A2DP_FEED_ENCODED_DATA_PACKET_E;index < A2DP_CACHED_CMD_TYPE_COUNT;index++)
    {
        a2dp_decoder_cc_cmd_cache_env[index].cachedCmdLen = 0;        
        osTimerStop(a2dp_decoder_cc_cmd_cache_env[index].supervisorTimerId);
    }
}

bool a2dp_decoder_cc_cache_cmd(uint16_t cmd_code, uint8_t *data)
{
    A2DP_CACHED_CMD_TYPE_INDEX_E cachedCmdIndex;
    uint16_t unitOfInfo = 0;
    switch(cmd_code) {
        case CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET:
            unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
            cachedCmdIndex = A2DP_FEED_ENCODED_DATA_PACKET_E;
            break;
        case CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET:
            unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
            cachedCmdIndex = A2DP_ACK_FED_PCM_DATA_PACKET_E;
            break;
        case CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET:
            unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
            cachedCmdIndex = A2DP_FEED_PCM_DATA_PACKET_E;
            break;
        case CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME:
            unitOfInfo = sizeof(A2DP_DECODER_CC_FLUSH_T);            
            cachedCmdIndex = A2DP_REMOVE_SPECIFIC_FRAME;
            break;
        default:
            return false;
    }
    osTimerStop(a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].supervisorTimerId);
    if (a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdLen + unitOfInfo >
        CC_DECODER_MAX_LEN_IN_MAILBOX)
    {
        app_dsp_m55_bridge_send_cmd(cmd_code, 
            a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdBuf, 
            a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdLen);
        a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdLen = 0;
    }
    uint32_t lock = int_lock();
    memcpy(a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdBuf + a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdLen,
        data, unitOfInfo);    
    a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].cachedCmdLen += unitOfInfo;
    int_unlock(lock);
    osTimerStart(a2dp_decoder_cc_cmd_cache_env[cachedCmdIndex].supervisorTimerId, A2DP_DECODER_CMD_CACHE_TIMEOUT_MS);
    return true;
}

void a2dp_decoder_cc_accumulate_cmd_send(uint16_t cmd_code, uint8_t *data)
{
    if (a2dp_decoder_cc_common_is_cmd_cached())
    {
        bool isCached = a2dp_decoder_cc_cache_cmd(cmd_code, data);
        if (isCached)
        {
            return;
        }
    }

    uint8_t fetchedData[CC_DECODER_MAX_LEN_IN_MAILBOX];
    uint16_t lenFetched = 0;
    uint8_t maxAllowCnt = 0;
    uint16_t unitOfInfo = 0;
    uint16_t threshold = CC_DECODER_MAX_LEN_IN_MAILBOX;

    switch(cmd_code) {
        case CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET:
            unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
            break;
        case CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET:
            unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
            break;
        case CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET:
            unitOfInfo = sizeof(a2dp_decoder_cc_media_data_t);
            break;
        case CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME:
            unitOfInfo = sizeof(A2DP_DECODER_CC_FLUSH_T);            
            break;
#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
        case CROSS_CORE_TASK_CMD_LHDC_FED_DATA_PACKET:
            unitOfInfo = sizeof(uint8_t);
        break;
#endif
        default:
            ASSERT(0, "%s, Wrong cmdCode:0x%x", __func__, cmd_code);
        break;
    }

    // Accumulate when there are too many mailbox cnts
    // @APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX
    if (app_dsp_m55_bridge_get_tx_mailbox_cnt() < 10) {
        goto send_directly;
    }

    maxAllowCnt = (CC_DECODER_MAX_LEN_IN_MAILBOX / unitOfInfo);
    threshold = (maxAllowCnt - 1) * unitOfInfo;

    app_dsp_m55_bridge_fetch_tx_mailbox(cmd_code, fetchedData, &lenFetched, threshold);
send_directly:
    memcpy(fetchedData + lenFetched, data, unitOfInfo);
    app_dsp_m55_bridge_send_cmd(cmd_code, fetchedData, lenFetched + unitOfInfo);
}

#endif


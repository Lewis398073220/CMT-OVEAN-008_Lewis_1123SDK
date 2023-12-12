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
#ifndef __ANC_ASSIST_COMM_H__
#define __ANC_ASSIST_COMM_H__

#include "app_anc_assist.h"
#include "anc_assist.h"
#include "../../../utils/kfifo/kfifo.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BTH_AS_MAIN_MCU
#if defined(ANC_ASSIST_ON_DSP_HIFI)
#include "rpc_m55_dsp.h"
#define ANC_ASSIST_CORE                   APP_RPC_CORE_DSP_M55
#define TASK_ANC_ASSIST_CMD_RSP           M55_DSP_TASK_CMD_ANC_ASSIST_CMD_RSP
#define TASK_ANC_ASSIST_DATA_NO_RSP       M55_DSP_TASK_CMD_ANC_ASSIST_DATA_NO_RSP
#define RPC_ANC_ASSIST_TASK_CMD_TO_ADD    RPC_M55_DSP_TASK_CMD_TO_ADD
#elif defined(ANC_ASSIST_ON_DSP_SENS)
#include "app_sensor_hub.h"
#define ANC_ASSIST_CORE                   APP_RPC_CORE_MCU_SENSOR
#define TASK_ANC_ASSIST_CMD_RSP           BTH_SENS_TASK_CMD_ANC_ASSIST_CMD_RSP
#define TASK_ANC_ASSIST_DATA_NO_RSP       BTH_SENS_TASK_CMD_ANC_ASSIST_DATA_NO_RSP
#define RPC_ANC_ASSIST_TASK_CMD_TO_ADD    CORE_BRIDGE_TASK_COMMAND_TO_ADD
#else
#endif

#else
#if defined(ANC_ASSIST_ON_DSP_M55)
#include "app_dsp_m55.h"
#define ANC_ASSIST_CORE                   APP_RPC_CORE_BTH_M55
#define TASK_ANC_ASSIST_CMD_RSP           MCU_DSP_M55_ANC_ASSIST_CMD
#define TASK_ANC_ASSIST_DATA_NO_RSP       MCU_DSP_M55_ANC_ASSIST_DATA
#define RPC_ANC_ASSIST_TASK_CMD_TO_ADD    M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD
#elif defined(ANC_ASSIST_ON_DSP_HIFI)
#include "rpc_bth_dsp.h"
#define ANC_ASSIST_CORE                   APP_RPC_CORE_BTH_DSP
#define TASK_ANC_ASSIST_CMD_RSP           BTH_DSP_TASK_CMD_ANC_ASSIST_CMD_RSP
#define TASK_ANC_ASSIST_DATA_NO_RSP       BTH_DSP_TASK_CMD_ANC_ASSIST_DATA_NO_RSP
#define RPC_ANC_ASSIST_TASK_CMD_TO_ADD    RPC_BTH_DSP_TASK_CMD_TO_ADD
#elif defined(ANC_ASSIST_ON_DSP_SENS)
#include "app_sensor_hub.h"
#define ANC_ASSIST_CORE                   APP_RPC_CORE_MCU_SENSOR
#define TASK_ANC_ASSIST_CMD_RSP           BTH_SENS_TASK_CMD_ANC_ASSIST_CMD_RSP
#define TASK_ANC_ASSIST_DATA_NO_RSP       BTH_SENS_TASK_CMD_ANC_ASSIST_DATA_NO_RSP
#define RPC_ANC_ASSIST_TASK_CMD_TO_ADD    CORE_BRIDGE_TASK_COMMAND_TO_ADD
#else
#endif
#endif

#define MAX_MIC_CHANNEL_NUMBER 10

#ifdef ASSIST_RESULT_FIFO_BUF
#define ASSIST_RESULT_FIFO_BUF_LEN (32 * 1024)
#endif

typedef enum {
    APP_ANC_ASSIST_CORE_CLOSE = 0,
    APP_ANC_ASSIST_CORE_OPEN,
    APP_ANC_ASSIST_CORE_PROCESS,
    APP_ANC_ASSIST_CORE_RESET,
    APP_ANC_ASSIST_CORE_CTRL,
    APP_ANC_ASSIST_CORE_SET_MODE,
    APP_ANC_ASSIST_CORE_QTY
} app_anc_assist_core_cmd_t;

typedef struct {
    struct kfifo ff_mic_fifo[MAX_FF_CHANNEL_NUM];
    uint8_t *ff_mic_fifo_mem[MAX_FF_CHANNEL_NUM];
    struct kfifo fb_mic_fifo[MAX_FB_CHANNEL_NUM];
    uint8_t *fb_mic_fifo_mem[MAX_FB_CHANNEL_NUM];
    struct kfifo talk_mic_fifo[MAX_TALK_CHANNEL_NUM];
    uint8_t *talk_mic_fifo_mem[MAX_TALK_CHANNEL_NUM];
    struct kfifo ref_fifo[MAX_REF_CHANNEL_NUM];
    uint8_t *ref_fifo_mem[MAX_REF_CHANNEL_NUM];
    struct kfifo vpu_fifo;
    uint8_t *vpu_fifo_mem;
    struct kfifo result_fifo;
    uint8_t *result_fifo_mem;
} anc_assist_fifo_t;

typedef struct {
    uint8_t head;
    uint8_t user;
    uint8_t cmd;
    uint8_t crc;
    uint32_t data_len;
} app_anc_assist_quick_fifo_head;

typedef struct {
    anc_assist_user_t user;
    app_anc_assist_core_cmd_t cmd;
    uint32_t ctrl;
    uint32_t ctrl_buf_len;
    uint8_t ctrl_buf[128];
} app_anc_assist_core_to_dsp_data_t;

typedef struct {
    anc_assist_user_t user;
    app_anc_assist_core_cmd_t cmd;
    anc_assist_fifo_t *fifo_ptr;
} app_anc_assist_core_to_bth_rsp_t;

typedef struct {
    anc_assist_user_t user;
    uint32_t len;
    uint32_t sub_cmd;
    uint8_t buf[256];
} app_anc_assist_core_to_bth_data_t;

#ifdef __cplusplus
}
#endif

#endif
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
#ifndef __RPC_M55_DSP_H__
#define __RPC_M55_DSP_H__

#include "rpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RPC_M55_DSP_TASK_CMD_TO_ADD(cmdCode,                            \
                                    cmd_code_str,                   \
                                    cmd_transmit_handler,   \
                                    cmd_received_handler,                         \
                                    cmd_wait_rsp_timeout_ms,                \
                                    cmd_wait_rsp_timeout_handle,    \
                                    cmd_rsp_received_handle,                 \
                                    cmd_transmit_done_handler)   \
                                    static const rpc_cmd_inst_t cmdCode##task##_entry  \
                                    __attribute__((used, section(".rpc_m55_dsp_cmd_table"))) =     \
                                    {(cmdCode),       \
                                    (cmd_code_str),                         \
                                    (cmd_transmit_handler),         \
                                    (cmd_received_handler),                               \
                                    (cmd_wait_rsp_timeout_ms),                      \
                                    (cmd_wait_rsp_timeout_handle),  \
                                    (cmd_rsp_received_handle),               \
                                    (cmd_transmit_done_handler)};

typedef enum {
    // M55<--> dsp demo cmd
    M55_DSP_TASK_CMD_RSP                 = 0x01,
    M55_DSP_TASK_CMD_DEMO_REQ_RSP        = 0x02,
    M55_DSP_TASK_CMD_DEMO_REQ_NO_RSP     = 0x03,

    // MCPP M55 <--> HIFI
    M55_DSP_TASK_CMD_MCPP_RSP            = 0x04,
    M55_DSP_TASK_CMD_MCPP_NO_RSP         = 0x05,

    M55_DSP_TASK_CMD_DISABLE_DSP         = 0x08,
    M55_DSP_TASK_CMD_AXI_SYSFREQ_REQ     = 0x09,

    // VOICE_ASSIST M55 <--> HIFI
    M55_DSP_TASK_CMD_ANC_ASSIST_CMD_RSP       = 0xa,
    M55_DSP_TASK_CMD_ANC_ASSIST_DATA_NO_RSP   = 0xb,

    M55_DSP_TASK_CMD_QTY,
} RPC_M55_DSP_CMD_CODE_E;

int32_t rpc_m55_dsp_send_cmd(uint16_t cmdcode, const uint8_t *p_buff, uint16_t length);
int32_t rpc_m55_dsp_send_data_no_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
int32_t rpc_m55_dsp_send_data_wait_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
int32_t rpc_m55_dsp_send_cmd_rsp(uint16_t rsp_code, uint8_t *p_buff, uint16_t length);

void rpc_m55_dsp_rx_queue_data_process_handler(void);

unsigned int m55_dsp_data_received(const void* data, unsigned int len);
void m55_dsp_data_tx_done(const void* data, unsigned int len);
void m55_dsp_core_open(void);

void rpc_m55_dsp_ctx_init(void);
void rpc_m55_dsp_ctx_deinit(void);

bool rpc_m55_dsp_ctx_inited(void);

typedef void (*app_dsp_m55_init_done_callback)(void);
void app_dsp_m55_register_init_done_callback(app_dsp_m55_init_done_callback cb);
#ifdef __cplusplus
}
#endif
#endif
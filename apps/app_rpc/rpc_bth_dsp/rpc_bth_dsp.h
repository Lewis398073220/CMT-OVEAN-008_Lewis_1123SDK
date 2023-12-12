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
#ifndef __RPC_BTH_DSP_H__
#define __RPC_BTH_DSP_H__

#include "rpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RPC_BTH_DSP_TASK_CMD_TO_ADD(cmdCode,                            \
                                    cmd_code_str,                   \
                                    cmd_transmit_handler,   \
                                    cmd_received_handler,                         \
                                    cmd_wait_rsp_timeout_ms,                \
                                    cmd_wait_rsp_timeout_handle,    \
                                    cmd_rsp_received_handle,                 \
                                    cmd_transmit_done_handler)   \
                                    static const rpc_cmd_inst_t cmdCode##task##_entry  \
                                    __attribute__((used, section(".rpc_bth_dsp_cmd_table"))) =     \
                                    {(cmdCode),       \
                                    (cmd_code_str),                         \
                                    (cmd_transmit_handler),         \
                                    (cmd_received_handler),                               \
                                    (cmd_wait_rsp_timeout_ms),                      \
                                    (cmd_wait_rsp_timeout_handle),  \
                                    (cmd_rsp_received_handle),               \
                                    (cmd_transmit_done_handler)};

typedef enum {
    // bth<--> dsp demo cmd
    BTH_DSP_TASK_CMD_RSP                        = 0x01,
    BTH_DSP_TASK_CMD_DEMO_REQ_RSP               = 0x02,
    BTH_DSP_TASK_CMD_DEMO_REQ_NO_RSP            = 0x03,

    BTH_DSP_TASK_CMD_SMF_NO_RSP                 = 0x08,
    BTH_DSP_TASK_CMD_SMF_RSP                    = 0x09,

    BTH_DSP_TASK_CMD_HA_INIT_NO_RSP             = 0x0a,
    BTH_DSP_TASK_CMD_HA_DEINIT_NO_RSP           = 0x0b,
    BTH_DSP_TASK_CMD_HA_CAPTURE_NO_RSP          = 0x0c,
    BTH_DSP_TASK_CMD_HA_PLAYBACK_NO_RSP         = 0x0d,
    BTH_DSP_TASK_CMD_ANC_ASSIST_CMD_RSP         = 0x0f,
    BTH_DSP_TASK_CMD_ANC_ASSIST_DATA_NO_RSP     = 0x10,
#if !defined(APP_RPC_MCU_SENSOR_EN)
    MCU_BTH_M55_TASK_CMD_ESHELL_TRANSMIT_DATA   = 0x14,
    MCU_TO_BTH_TASK_CMD_BT_IF_COMM              = 0x15,
    BTH_TO_MCU_TASK_CMD_BT_IF_COMM              = 0x16,
    BTH_CORE_ALIVE_POLL_CMD                     = 0x17,
    BTH_TASK_CMD_STACK_READY_IND                = 0x18,
#endif

    // MCPP BTH <--> HIFI
    BTH_DSP_TASK_CMD_MCPP_RSP                   = 0x19,
    BTH_DSP_TASK_CMD_MCPP_NO_RSP                = 0x1a,

    BTH_DSP_TASK_CMD_DISABLE_DSP                = 0x1b,
    BTH_DSP_TASK_CMD_AXI_SYSFREQ_REQ            = 0x1c,

    BTH_DSP_TASK_CMD_QTY,
} RPC_BTH_DSP_CMD_CODE_E;

int32_t rpc_bth_dsp_send_cmd(uint16_t cmdcode, const uint8_t *p_buff, uint16_t length);
int32_t rpc_bth_dsp_send_data_no_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
int32_t rpc_bth_dsp_send_data_wait_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
int32_t rpc_bth_dsp_send_cmd_rsp(uint16_t rsp_code, uint8_t *p_buff, uint16_t length);

void rpc_bth_dsp_rx_queue_data_process_handler(void);

unsigned int bth_dsp_data_received(const void* data, unsigned int len);
void bth_dsp_data_tx_done(const void* data, unsigned int len);
void bth_dsp_core_open(void);

void rpc_bth_dsp_ctx_init(void);
void rpc_bth_dsp_ctx_deinit(void);

bool rpc_bth_dsp_ctx_inited(void);

typedef void (*app_dsp_hifi_init_done_callback)(void);
void app_dsp_hifi_register_init_done_callback(app_dsp_hifi_init_done_callback cb);
#ifdef __cplusplus
}
#endif
#endif
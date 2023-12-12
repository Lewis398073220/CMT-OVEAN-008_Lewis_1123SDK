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
#ifndef __APP_DSP_M55_H__
#define __APP_DSP_M55_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE
#define APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE    (512)
#endif

#ifndef APP_DSP_M55_BRIDGE_RX_BUFF_SIZE
#define APP_DSP_M55_BRIDGE_RX_BUFF_SIZE            (2048)
#endif

#ifndef APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX
#ifdef A2DP_DECODER_CROSS_CORE_USE_M55
#define APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX          (45)
#else
#define APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX          (16)
#endif
#endif

#ifndef APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE
#define APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE      (512)
#endif

#ifndef APP_DSP_M55_BRIDGE_RX_THREAD_TMP_BUF_SIZE
#define APP_DSP_M55_BRIDGE_RX_THREAD_TMP_BUF_SIZE  (512)
#endif

#ifndef APP_DSP_M55_BRIDGE_TX_THREAD_STACK_SIZE
#define APP_DSP_M55_BRIDGE_TX_THREAD_STACK_SIZE    (2048+1024-512)
#endif

#ifndef APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE
#define APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE    (1024*4+1024-512)
#endif

#define APP_DSP_M55_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS 500

#define M55_CORE_BRIDGE_CMD_GROUP_TASK      0x00
#define M55_CORE_BRIDGE_CMD_GROUP_INSTANT   0x01

#define M55_CORE_BRIDGE_CMD_GROUP_OFFSET           (8)
#define M55_CORE_BRIDGE_CMD_SUBCODE_BIT(subCode)   ((subCode)&((1<<M55_CORE_BRIDGE_CMD_GROUP_OFFSET)-1))
#define M55_CORE_BRIDGE_CMD_CODE(group, subCode)   (((group) << M55_CORE_BRIDGE_CMD_GROUP_OFFSET)|M55_CORE_BRIDGE_CMD_SUBCODE_BIT(subCode))
#define M55_CORE_BRIDGE_CMD_SUBCODE(cmdCode)       ((cmdCode)&((1 << M55_CORE_BRIDGE_CMD_GROUP_OFFSET)-1))
#define M55_CORE_BRIDGE_CMD_GROUP(cmdCode)         ((cmdCode) >> M55_CORE_BRIDGE_CMD_GROUP_OFFSET)

#define M55_CORE_BRIDGE_CMD_INSTANT(subCode) M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, (subCode))
#define M55_CORE_BRIDGE_CMD_TASK(subCode)    M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, (subCode))

#define SIGNAL_M55_BOOT_DONE                        (0x01)

typedef void (*app_dsp_m55_bridge_cmd_transmit_handler_t)(uint8_t*, uint16_t);
typedef void (*app_dsp_m55_bridge_cmd_receivd_handler_t)(uint8_t*, uint16_t);
typedef void (*app_dsp_m55_bridge_wait_rsp_timeout_handle_t)(uint8_t*, uint16_t);
typedef void (*app_dsp_m55_bridge_rsp_handle_t)(uint8_t*, uint16_t);
typedef void (*app_dsp_m55_bridge_cmd_transmission_done_handler_t) (uint16_t, uint8_t*, uint16_t);

typedef struct
{
    uint16_t                                cmdcode;
    const char                              *log_cmd_code_str;
    app_dsp_m55_bridge_cmd_transmit_handler_t  core_bridge_cmd_transmit_handler;
    app_dsp_m55_bridge_cmd_receivd_handler_t   cmdhandler;
    uint32_t                                wait_rsp_timeout_ms;
    app_dsp_m55_bridge_wait_rsp_timeout_handle_t       app_dsp_m55_bridge_wait_rsp_timeout_handle;
    app_dsp_m55_bridge_rsp_handle_t                    app_dsp_m55_bridge_rsp_handle;
    app_dsp_m55_bridge_cmd_transmission_done_handler_t app_dsp_m55_bridge_transmisson_done_handler;
} __attribute__((aligned(4))) app_dsp_m55_bridge_task_cmd_instance_t;

typedef struct
{
    uint16_t                                cmdcode;
    app_dsp_m55_bridge_cmd_transmit_handler_t  core_bridge_cmd_transmit_handler;
    app_dsp_m55_bridge_cmd_receivd_handler_t   cmdhandler;
} __attribute__((aligned(4))) app_dsp_m55_bridge_instant_cmd_instance_t;


#define M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(cmdCode,                            \
                                        log_cmd_code_str,                   \
                                        core_bridge_cmd_transmit_handler,   \
                                        cmdhandler,                         \
                                        wait_rsp_timeout_ms,                \
                                        app_dsp_m55_bridge_wait_rsp_timeout_handle,    \
                                        app_dsp_m55_bridge_rsp_handle,                 \
                                        app_dsp_m55_bridge_transmisson_done_handler)   \
                                        static const app_dsp_m55_bridge_task_cmd_instance_t cmdCode##task##_entry  \
                                        __attribute__((used, section(".m55_core_bridge_task_cmd_table"))) =     \
                                        {(cmdCode),       \
                                        (log_cmd_code_str),                         \
                                        (core_bridge_cmd_transmit_handler),         \
                                        (cmdhandler),                               \
                                        (wait_rsp_timeout_ms),                      \
                                        (app_dsp_m55_bridge_wait_rsp_timeout_handle),  \
                                        (app_dsp_m55_bridge_rsp_handle),               \
                                        (app_dsp_m55_bridge_transmisson_done_handler)};


#define M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(cmdCode,                         \
                                        core_bridge_cmd_transmit_handler,   \
                                        cmdhandler)                         \
                                        static const app_dsp_m55_bridge_instant_cmd_instance_t cmdCode##task##_entry   \
                                        __attribute__((used, section(".m55_core_bridge_instant_cmd_table"))) =      \
                                        {(cmdCode),        \
                                        (core_bridge_cmd_transmit_handler), \
                                        (cmdhandler)};

typedef enum
{
    MCU_DSP_M55_TASK_CMD_PING = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x80),
    MCU_DSP_M55_TASK_CMD_RSP  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x81),

    // sample command
    MCU_DSP_M55_TASK_CMD_DEMO_REQ_NO_RSP    = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x82),
    MCU_DSP_M55_TASK_CMD_DEMO_REQ_WITH_RSP  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x83),

    // voice detector
    MCU_DSP_M55_TASK_CMD_VAD  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x84),

    // ai kws enginne signaling cmd
    MCU_DSP_M55_TASK_CMD_AI  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x85),

    // capsensor touch cmd
    MCU_DSP_M55_TASK_CMD_TOUCH_REQ_NO_RSP  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x86),

    // soundplus vad cmd
    MCU_DSP_M55_TASK_CMD_SNDP  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x87),

    // anc_assist
    MCU_DSP_M55_ANC_ASSIST_CMD  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x8c),
    MCU_DSP_M55_ANC_ASSIST_DATA = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x8d),
#if defined(A2DP_DECODER_CROSS_CORE_USE_M55) || defined(A2DP_ENCODER_CROSS_CORE_USE_M55)
    CROSS_CORE_TASK_CMD_A2DP_INIT_WAITING_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x90),
    CROSS_CORE_TASK_CMD_A2DP_DEINIT_WAITING_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x91),

    // bth to off-bth core, no waiting but keep the a2dp encoded data buffer
    CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x92),

    // off-bth core to bth after copying data to local buffer
    CROSS_CORE_TASK_CMD_A2DP_ACK_FED_ENCODED_DATA_PACKET = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x93),

    // off-bth core to bth after decoding the data, no waiting but keep the
    // pcm data buffer
    CROSS_CORE_TASK_CMD_A2DP_FEED_PCM_DATA_PACKET = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x94),

    // bth to off-bth core after consuming the pcm data to codec dma
    CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x95),

    CROSS_CORE_TASK_CMD_A2DP_RETRIGGER_REQ_NO_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x96),

    CROSS_CORE_TASK_CMD_A2DP_REMOVE_SPECIFIC_FRAME = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x97),

    CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_REQ = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x98),

    CROSS_CORE_TASK_CMD_A2DP_BOOST_FREQ_DONE = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x99),
#ifdef A2DP_SCALABLE_ON
    CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_OFF_BTH = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x9a),

    CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_TO_BTH = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x9b),

    CROSS_CORE_TASK_CMD_A2DP_SCALABLE_SBM_STATUS = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x9c),
#endif
#ifdef A2DP_LHDC_ON//LHDC_DSP_LIC_CHECK
    CROSS_CORE_TASK_CMD_LHDC_FED_DATA_PACKET = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x9e),
#endif

    CROSS_CORE_TASK_CMD_A2DP_SET_TRIGGERED_STATE = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0x9f),
#endif

#ifdef GAF_ENCODER_CROSS_CORE_USE_M55
    CROSS_CORE_TASK_CMD_GAF_ENCODE_INIT_WAITING_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xa4),
    CROSS_CORE_TASK_CMD_GAF_ENCODE_DEINIT_WAITING_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xa5),

    // ask to mcu core to retriggle audio media
    CROSS_CORE_TASK_CMD_GAF_RETRIGGER_REQ_NO_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xa8),

    CROSS_CORE_INSTANT_CMD_GAF_ENCODE_DATA_NOTIFY = M55_CORE_BRIDGE_CMD_INSTANT(0xa9),
#endif

#ifdef GAF_DECODER_CROSS_CORE_USE_M55
    // mcu core ask m55 to decoder init, has waiting rsp
    CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_WAITING_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, 0xab),

    // mcu core ask m55 to decoder deinit, has waiting rsp
    CROSS_CORE_TASK_CMD_GAF_DECODE_DEINIT_WAITING_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xac),

    // m55 send decoder init rsp signal to bth
    CROSS_CORE_INSTANT_CMD_GAF_DECODE_INIT_RSP_TO_CORE = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, 0xb7),

    // ask to mcu core to retrigger audio media
    CROSS_CORE_TASK_CMD_GAF_DECODE_RETRIGGER_REQ_NO_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xb0),

    CROSS_CORE_INSTANT_CMD_GAF_DECODE_DATA_NOTIFY = M55_CORE_BRIDGE_CMD_INSTANT(0x86),
#endif

    MCU_DSP_M55_TASK_CMD_SMF_NO_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xb5),
    MCU_DSP_M55_TASK_CMD_SMF_RSP    = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xb6),

#if defined(UTILS_ESHELL_EN)
    MCU_BTH_M55_TASK_CMD_ESHELL_TRANSMIT_DATA = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xC0),
#endif
    // dummy info
    MCU_DSP_M55_DUMPPY_INFO = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xff),

    MCU_DSP_M55_INSTANT_CMD_DEMO_REQ  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, 0x80),

    MCU_DSP_M55_INSTANT_CMD_TOUCH_REQ  = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, 0x81),

    // m55 freq config req
    MCU_DSP_M55_INSTANT_CMD_AXI_SYSFREQ_REQ = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, 0x82),
    // notify m55 to disable
    MCU_DSP_M55_INSTANT_CMD_DISABLE_M55 = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_INSTANT, 0x83),

    // MCPP BTH <--> M55
    MCU_DSP_M55_TASK_CMD_MCPP_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xc0),
    MCU_DSP_M55_TASK_CMD_MCPP_NO_RSP = M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, 0xc1),

} CORE_BRIDGE_CMD_CODE_E;

/* Core-Bridge Thread Init/Deinit */
void app_dsp_m55_bridge_init(void);
void app_dsp_m55_bridge_deinit(void);
void app_dsp_m55_bridge_trigger_turn_off(void);
void dsp_m55_core_deinit(void);

/* Client Thread APIs, runs at Client Thread */
int32_t app_dsp_m55_bridge_send_cmd(uint16_t cmd_code, uint8_t *p_buff, uint16_t length);

/* Command Callback APIs, runs at Core-Bridge Thread */
int32_t app_dsp_m55_bridge_send_rsp(uint16_t rsp_code, uint8_t *p_buff, uint16_t length);
void app_dsp_m55_bridge_send_data_without_waiting_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
void app_dsp_m55_bridge_send_data_with_waiting_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
/* Low-level Driver Handler APIs */
unsigned int app_dsp_m55_bridge_data_received(const void* data, unsigned int len);
void app_dsp_m55_bridge_data_tx_done(const void* data, unsigned int len);

/* Not usually used APIs */
void app_dsp_m55_bridge_send_instant_cmd_data(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
void app_dsp_m55_notify_tx_thread_status(bool isEnabled);
app_dsp_m55_bridge_instant_cmd_instance_t* app_dsp_m55_bridge_find_instant_cmd_entry(uint16_t cmdcode);
app_dsp_m55_bridge_task_cmd_instance_t* app_dsp_m55_bridge_find_task_cmd_entry(uint16_t cmdcode);
void app_dsp_m55_bridge_free_tx_mailbox(void);
void app_dsp_m55_bridge_fetch_tx_mailbox(uint16_t cmd_code, uint8_t *buffer, uint16_t *length, uint16_t threshold);
void app_dsp_m55_bridge_clear_tx_mailbox(uint16_t cmd_code, uint16_t reserve_cnt);
uint32_t app_dsp_m55_bridge_get_tx_mailbox_cnt(void);
uint32_t app_dsp_m55_bridge_get_tx_mailbox_free_cnt(void);

#ifdef __cplusplus
}
#endif

#endif

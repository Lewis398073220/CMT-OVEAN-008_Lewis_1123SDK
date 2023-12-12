/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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

#ifndef __APP_BT_STAMP_H__
#define __APP_BT_STAMP_H__
#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************macro defination*****************************/
#define APP_BT_SYNC_THREAD_STACK_SIZE (2048) //This is default value. If your cmd handler has large arrays. You should increase it.
#define APP_BT_SYNC_INVALID_CHANNEL (0xFF)
#define APP_BT_SYNC_CHANNEL_OFFSET (2) //Total 4, channel0 reserved for HFP/A2DP, channel1 for prompt
#define APP_BT_SYNC_CHANNEL_MAX (2)
#define APP_BT_SYNC_SENT_TRIGGER_INFO_TIMEOUT_IN_MS (3000)

typedef enum
{
    APP_BT_SYNC_OP_RETRIGGER     = 0x00000000,
    APP_BT_SYNC_OP_VOLUME        = 0x00000001,
    APP_BT_SYNC_OP_VOLUME_OFFSET = 0x00000002,
// TODO: add more operation codes
} APP_BT_SYNC_OP_CODE_E;

extern uint32_t __app_bt_sync_command_handler_table_start[];
extern uint32_t __app_bt_sync_command_handler_table_end[];

#define APP_BT_SYNC_COMMAND_TO_ADD(opCode, cmdhandler, statusNotify)                          \
    static const app_bt_sync_instance_t syncOp##opCode##_entry                          \
    __attribute__((used, section(".app_bt_sync_command_handler_table"))) =      \
    {(opCode), (cmdhandler), (statusNotify)};

#define APP_BT_SYNC_COMMAND_PTR_FROM_ENTRY_INDEX(index)    \
    ((app_bt_sync_instance_t *)((uint32_t)__app_bt_sync_command_handler_table_start + (index)*sizeof(app_bt_sync_instance_t)))

/******************************type defination******************************/
typedef void (*APP_BT_SYNC_HANDLER_T)(void);
typedef void (*APP_BT_SYNC_STATUS_NOTIFY_T)(uint32_t opCode, bool triStatus, bool triInfoSentStatus);
typedef void (*APP_BT_SYNC_INFO_REPORT_T)(uint32_t opCode, uint8_t *ptrInfo, uint8_t length);

typedef struct{
    uint32_t opCode;
    uint8_t triSrc;
    bool used;
    uint32_t triTick;
} __attribute__((packed)) bt_trigger_info_s;

typedef struct {
    uint8_t triSrc;
    uint32_t opCode;
} __attribute__((packed)) APP_SYNC_MSG_BLOCK;

typedef struct
{
    uint32_t opCode;
    APP_BT_SYNC_HANDLER_T syncCallback;
    APP_BT_SYNC_STATUS_NOTIFY_T statusNotify;
} __attribute__((packed)) app_bt_sync_instance_t;

typedef struct{
    bool initiator;
    bool twsCmdSendDone;
    uint32_t opCode; /**< The command waiting for the irq */
    uint32_t msTillTimeout; /**< run-time timeout left milliseconds */
    uint8_t indexLocInfo;
} __attribute__((packed)) app_bt_sync_status_info_s;

typedef struct
{
    app_bt_sync_status_info_s statusInstance[APP_BT_SYNC_CHANNEL_MAX];
    uint32_t lastSysTicks;
    uint8_t timeoutSupervisorCount;
} __attribute__((packed)) app_bt_sync_env;

/**
 * @brief Get avaliable trigger channel from BT trigger module
 * NOTE: There are 4 channel for BT trigger, channel0 is reserved for A2DP/HFP playback
 *
 * @return uint8_t      The avaliable trigger channel
 */
uint8_t app_bt_sync_get_avaliable_trigger_channel(void);

/**
 * @brief Release the given BT trigger channel
 *
 * @param chnl          The trigger channel to release
 */
void app_bt_sync_release_trigger_channel(uint8_t chnl);

/**
 ****************************************************************************************
 * @brief Enable the both buds trigger function
 *
 * @param[out] none
 * @param[in]  opCode             Point to the expected callback handler
 * @param[in]  length             The length of information shared between TWS
 * @param[in]  p_buff             Point to the share info
 *
 * @return Execute result status
 ****************************************************************************************/
bool app_bt_sync_enable(uint32_t opCode, uint8_t length, uint8_t *p_buff);

/**
 ****************************************************************************************
 * @brief the slave device to process the sync command
 *
 * @param[out] none
 * @param[in]  triInfo            trigger info from master device
 *
 * @return none
 ****************************************************************************************/
void app_bt_sync_tws_cmd_handler(uint8_t *p_buff, uint16_t length);

/**
 ****************************************************************************************
 * @brief trigger info send done
 ****************************************************************************************/
void app_bt_sync_send_tws_cmd_done(uint8_t *ptrParam, uint16_t paramLen);

/**
 ****************************************************************************************
 * @brief register the callback for reporting the received information
 ****************************************************************************************/
void app_bt_sync_register_report_info_callback(APP_BT_SYNC_INFO_REPORT_T cb);

/**
 ****************************************************************************************
 * @brief covert the peer bt clock into local bt clock
 *
 * @param[out] pMasterBtClockCnt peer master bt clock in the unit of bt clock
 * @param[out] pSlaveBtTicksUs local slave bt clock in the unit of us
 *
 * @return none
 ****************************************************************************************/
void app_bt_sync_get_current_tws_bt_ticks_in_us_on_slave_side(uint32_t* pMasterBtClockCnt,
    uint32_t* pSlaveBtTicksUs);

void app_bt_sync_get_master_time_from_slave_time(uint32_t SlaveBtTicksUs, uint32_t* p_master_clk_cnt, uint16_t* p_master_bit_cnt);
uint32_t app_bt_sync_get_slave_time_from_master_time(uint32_t master_clk_cnt, uint16_t master_bit_cnt);

bool app_bt_sync_is_tws_slave(void);

uint32_t app_bt_sync_get_current_slave_bt_ticks_in_us(void);

#ifdef __cplusplus
}
#endif
#endif


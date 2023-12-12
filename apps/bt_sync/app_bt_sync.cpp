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
#include "plat_types.h"
#include "bt_drv_interface.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "app_bt_sync.h"
#include "hal_timer.h"
#include <stdarg.h>
#ifdef IBRT
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_internal.h"
#include "app_tws_ctrl_thread.h"
#endif
#if defined(__BT_SYNC__) && defined(IBRT)
static osThreadId app_bt_sync_thread_tid = NULL;
static bool app_bt_sync_initilized = false;
static bt_trigger_info_s locTriggerInfo[APP_BT_SYNC_CHANNEL_MAX];
static app_bt_sync_env app_bt_sync_local_env;

static void app_bt_sync_thread(void const *argument);
osThreadDef(app_bt_sync_thread, osPriorityRealtime, 1, APP_BT_SYNC_THREAD_STACK_SIZE, "app_bt_sync");
osMailQDef (app_sync_mailbox, 10, APP_SYNC_MSG_BLOCK);
static osMailQId app_sync_mailbox = NULL;
static void app_bt_sync_send_trigger_info_timeout_handler(void const *param);
osTimerDef(APP_BT_SYNC_SEND_TRIGGER_INFO, app_bt_sync_send_trigger_info_timeout_handler);
static osTimerId app_bt_sync_send_trigger_info_timer = NULL;
osMutexDef(app_bt_sync_timer_mutex);
static osMutexId app_bt_sync_timer_mutex_id = NULL;

static app_bt_sync_instance_t* app_bt_sync_get_entry_pointer_from_cmd_code(uint32_t opCode)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__app_bt_sync_command_handler_table_end-(uint32_t)__app_bt_sync_command_handler_table_start)/sizeof(app_bt_sync_instance_t);index++)
    {\
        //TRACE(2, "seek entry pointer opCode:%d %d", opCode,  APP_BT_SYNC_COMMAND_PTR_FROM_ENTRY_INDEX(index)->opCode);
        if (APP_BT_SYNC_COMMAND_PTR_FROM_ENTRY_INDEX(index)->opCode == opCode)
        {
            return APP_BT_SYNC_COMMAND_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    return NULL;
}

static app_bt_sync_status_info_s* app_bt_sync_get_status_instance(uint32_t opCode)
{
    for (uint8_t i = 0; i < app_bt_sync_local_env.timeoutSupervisorCount; i++) {
        if (opCode == app_bt_sync_local_env.statusInstance[i].opCode) {
            return &app_bt_sync_local_env.statusInstance[i];
        }
    }

    return NULL;
}

static void app_bt_sync_refresh_supervisor_env(void)
{
    // do nothing if no supervisor was added
    if (app_bt_sync_local_env.timeoutSupervisorCount > 0)
    {
        uint32_t currentTicks = GET_CURRENT_TICKS();
        uint32_t passedTicks = 0;

        if (currentTicks >= app_bt_sync_local_env.lastSysTicks)
        {
            passedTicks = (currentTicks - app_bt_sync_local_env.lastSysTicks);
        }
        else
        {
            passedTicks = (hal_sys_timer_get_max() - app_bt_sync_local_env.lastSysTicks + 1) + currentTicks;
        }

        uint32_t deltaMs = TICKS_TO_MS(passedTicks);

        app_bt_sync_status_info_s* pSupervisor = &(app_bt_sync_local_env.statusInstance[0]);
        for (uint32_t index = 0;index < app_bt_sync_local_env.timeoutSupervisorCount;index++)
        {
            ASSERT(pSupervisor[index].msTillTimeout > deltaMs,
                "(d%d)Supervisor timer missed,%d ms passed but the timeout is %d", index, deltaMs, pSupervisor[index].msTillTimeout);

            TRACE(2, "(d%d)refresh_supervisor:%d-%d, cnt:%d", index, pSupervisor[index].msTillTimeout, deltaMs,
                app_bt_sync_local_env.timeoutSupervisorCount);

            pSupervisor[index].msTillTimeout -= deltaMs;
        }
    }

    app_bt_sync_local_env.lastSysTicks = GET_CURRENT_TICKS();
}

static void app_bt_sync_remove_waiting_rsp_timeout_supervision(uint32_t opCode)
{
    ASSERT(app_bt_sync_local_env.timeoutSupervisorCount > 0,
        "%s supervisor empty", __FUNCTION__);

    TRACE(1, "remove supervision opcode:%d, cnt:%d", opCode, app_bt_sync_local_env.timeoutSupervisorCount);

    osMutexWait(app_bt_sync_timer_mutex_id, osWaitForever);

    uint32_t index = 0;
    uint32_t supervisorCnt = app_bt_sync_local_env.timeoutSupervisorCount;
    app_bt_sync_status_info_s *pInstance = &app_bt_sync_local_env.statusInstance[0];
    for (index = 0;index < supervisorCnt;index++)
    {
        if (pInstance->opCode == opCode)
        {
            if ((index + 1) != supervisorCnt) {
                // not last element
                memcpy(pInstance, pInstance + 1, (supervisorCnt - index - 1) * sizeof(app_bt_sync_status_info_s));
            }
            // already find
            break;
        }
        pInstance++;
    }

    // cannot find it, directly return
    if (index == supervisorCnt)
    {
        TRACE(1, "bt_sync,not find element");
        goto exit;
    }

    app_bt_sync_local_env.timeoutSupervisorCount--;

    if (app_bt_sync_local_env.timeoutSupervisorCount > 0)
    {
        // refresh supervisor environment firstly
        app_bt_sync_refresh_supervisor_env();

        // start timer, the first entry is the most close one
        osTimerStart(app_bt_sync_send_trigger_info_timer, app_bt_sync_local_env.statusInstance[0].msTillTimeout);
    }
    else
    {
        // no supervisor, directly stop the timer
        osTimerStop(app_bt_sync_send_trigger_info_timer);
    }

exit:
    osMutexRelease(app_bt_sync_timer_mutex_id);
}

static void app_bt_sync_add_waiting_irq_timeout_supervision(uint32_t opCode, uint8_t index)
{
    ASSERT(app_bt_sync_local_env.timeoutSupervisorCount < APP_BT_SYNC_CHANNEL_MAX,
        "%s supervisor is full", __func__);

    osMutexWait(app_bt_sync_timer_mutex_id, osWaitForever);

    // refresh supervisor environment firstly
    app_bt_sync_refresh_supervisor_env();

    app_bt_sync_local_env.statusInstance[app_bt_sync_local_env.timeoutSupervisorCount].initiator = true;

    app_bt_sync_local_env.statusInstance[app_bt_sync_local_env.timeoutSupervisorCount].opCode = opCode;

    app_bt_sync_local_env.statusInstance[app_bt_sync_local_env.timeoutSupervisorCount].indexLocInfo = index;
    app_bt_sync_local_env.statusInstance[app_bt_sync_local_env.timeoutSupervisorCount].msTillTimeout = APP_BT_SYNC_SENT_TRIGGER_INFO_TIMEOUT_IN_MS;

    app_bt_sync_local_env.timeoutSupervisorCount++;

    // start timer, the first entry is the most close one
    osTimerStart(app_bt_sync_send_trigger_info_timer, app_bt_sync_local_env.statusInstance[0].msTillTimeout);

    osMutexRelease(app_bt_sync_timer_mutex_id);
}

static uint8_t app_bt_sync_get_supervisor_count(void)
{
    return app_bt_sync_local_env.timeoutSupervisorCount;
}

static void app_bt_sync_update_supervisor(uint32_t opCode, uint8_t index)
{
    app_bt_sync_status_info_s *statusInfo = NULL;

    statusInfo = app_bt_sync_get_status_instance(opCode);

    if (NULL != statusInfo) {
        statusInfo->indexLocInfo = index;
    }
}

static int app_bt_sync_mailbox_free(APP_SYNC_MSG_BLOCK* msg_p)
{
    osStatus status;

    status = osMailFree(app_sync_mailbox, msg_p);
    if (osOK != status) {
        TRACE(1, "%s failed to free", __func__);
    }
    return (int)status;
}

static int app_bt_sync_mailbox_get(APP_SYNC_MSG_BLOCK** msg_p)
{
    osEvent evt;
    evt = osMailGet(app_sync_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (APP_SYNC_MSG_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

static int app_bt_sync_mailbox_put(APP_SYNC_MSG_BLOCK* msg_src)
{
    osStatus status;
    APP_SYNC_MSG_BLOCK *msg_p = NULL;

    msg_p = (APP_SYNC_MSG_BLOCK*)osMailAlloc(app_sync_mailbox, 0);

    if (!msg_p) {
        TRACE(1, "%s failed to alloc", __func__);
        return -1;
    }

    *msg_p = *msg_src;
    status = osMailPut(app_sync_mailbox, msg_p);
    if (osOK != status) {
        TRACE(1, "%s failed to put", __func__);
    }

    return (int)status;
}

static void app_bt_sync_thread(void const *argument)
{
    while(1)
    {
        APP_SYNC_MSG_BLOCK *msg_p = NULL;
        uint32_t opCode = 0xFFFF;
        uint8_t triSrc = 0xFF;
        app_bt_sync_instance_t *pCallback = NULL;
        app_bt_sync_status_info_s *statusInfo = NULL;
        bool twsCmdSendStatus = false;
        bool triResult = false;

        if (!app_bt_sync_mailbox_get(&msg_p)) {
            opCode = msg_p->opCode;
            triSrc = msg_p->triSrc;

            app_bt_sync_mailbox_free(msg_p);
            pCallback = app_bt_sync_get_entry_pointer_from_cmd_code(opCode);
            statusInfo = app_bt_sync_get_status_instance(opCode);

            TRACE(4, "bt_sync_thread: %p %p Code:0x%x,Src:0x%x", statusInfo, pCallback, opCode, triSrc);
            if (triSrc == APP_BT_SYNC_INVALID_CHANNEL) {
                triResult = false;
            }
            else
            {
                triResult = true;
            }

            if (statusInfo && statusInfo->initiator) {
                app_bt_sync_remove_waiting_rsp_timeout_supervision(opCode);
                twsCmdSendStatus = statusInfo->twsCmdSendDone;
                if (pCallback && pCallback->statusNotify)
                {
                    pCallback->statusNotify(opCode, triResult, twsCmdSendStatus);
                }
            }

            if ((triResult) && (pCallback) &&
                (pCallback->syncCallback))
            {
                pCallback->syncCallback();
            }
        }
    }
}

static void app_bt_sync_send_trigger_info_timeout_handler(void const *param)
{
    uint32_t opCode = app_bt_sync_local_env.statusInstance[0].opCode;
    uint8_t index = app_bt_sync_local_env.statusInstance[0].indexLocInfo;

    TRACE(2, "app_bt_sync:trigger timeout opCode:%d index:%d", opCode, index);

    locTriggerInfo[index].used = false;
    APP_SYNC_MSG_BLOCK msg;
    msg.triSrc = APP_BT_SYNC_INVALID_CHANNEL;
    msg.opCode = opCode;
    app_bt_sync_mailbox_put(&msg);

}

static void app_bt_sync_irq_handler(enum HAL_CMU_BT_TRIGGER_SRC_T src)
{
    TRACE(3, "%s src:%d opCode:%d", __func__, src, locTriggerInfo[src - APP_BT_SYNC_CHANNEL_OFFSET].opCode);
    btdrv_syn_clr_trigger((uint8_t)src);
    hal_cmu_bt_trigger_disable(src);
    locTriggerInfo[src-APP_BT_SYNC_CHANNEL_OFFSET].used = false; //channel0 is for A2DP/HFP

    APP_SYNC_MSG_BLOCK msg;
    msg.triSrc = ((uint8_t)src - APP_BT_SYNC_CHANNEL_OFFSET);
    msg.opCode = locTriggerInfo[src - APP_BT_SYNC_CHANNEL_OFFSET].opCode;
    app_bt_sync_mailbox_put(&msg);
}

static bool app_bt_sync_module_init(void)
{
    if (app_bt_sync_initilized == false) {
        memset(locTriggerInfo, 0x00, sizeof(locTriggerInfo[0]) * APP_BT_SYNC_CHANNEL_MAX);
        app_sync_mailbox = osMailCreate(osMailQ(app_sync_mailbox), NULL);
        if (app_sync_mailbox == NULL) {
            TRACE(0, "Failed to Create app_stamp_mailbox");
            return false;
        }

        app_bt_sync_thread_tid = osThreadCreate(osThread(app_bt_sync_thread), NULL);
        if (app_bt_sync_thread_tid == NULL) {
            TRACE(0, "Failed to Create app_bt_sync_thread");
            return false;
        }

        app_bt_sync_send_trigger_info_timer = osTimerCreate(osTimer(APP_BT_SYNC_SEND_TRIGGER_INFO), osTimerOnce, NULL);
        if (app_bt_sync_send_trigger_info_timer == NULL)
        {
            TRACE(0, "Warning: failed to Create app_bt_sync_timer");
        }

        app_bt_sync_timer_mutex_id = osMutexCreate(osMutex(app_bt_sync_timer_mutex));
        if (app_bt_sync_timer_mutex_id == NULL)
        {
            TRACE(0, "Warning: failed to Create app_bt_sync_timer_mutex");
        }

        memset(&app_bt_sync_local_env, 0x00, sizeof(app_bt_sync_env));
        app_bt_sync_initilized = true;
     }
    return true;
}

uint8_t app_bt_sync_get_avaliable_trigger_channel(void)
{
    // Need to initilize first. Otherwise the variable locTriggerInfo will be cleared by sync module
    if (!app_bt_sync_module_init()) {
        TRACE(1, "%s: failed to initilize", __func__);
        return APP_BT_SYNC_INVALID_CHANNEL;
    }

    uint8_t index = 0;
    uint8_t channel = APP_BT_SYNC_INVALID_CHANNEL; //!< invalid value

    for (; index < APP_BT_SYNC_CHANNEL_MAX; index++) {
        if (!locTriggerInfo[index].used) {
            break;
        }
    }

    if (index < APP_BT_SYNC_CHANNEL_MAX) {
        locTriggerInfo[index].used = true;
        //!< channel0 is reserved for A2DP/HFP, channel1 is reserved for prompt
        locTriggerInfo[index].triSrc = index + APP_BT_SYNC_CHANNEL_OFFSET;
        channel = locTriggerInfo[index].triSrc;
    }

    TRACE(2, "%s avaliable channel:%d", __func__, channel);
    return channel;
}

void app_bt_sync_release_trigger_channel(uint8_t chnl)
{
    uint8_t index = 0;
    for (; index < APP_BT_SYNC_CHANNEL_MAX; index++) {
        if (chnl == locTriggerInfo[index].triSrc) {
            break;
        }
    }

    if (APP_BT_SYNC_CHANNEL_MAX > index) {
        if (!locTriggerInfo[index].used) {
            TRACE(1, "[BT Trigger] chnl %d not used!", chnl);
        } else {
            locTriggerInfo[index].used = false;
            TRACE(1, "[BT Trigger] chnl %d released!", chnl);
        }
    } else {
        TRACE(1, "[WARN][BT Trigger] chnl %d not found!", chnl);
    }
}

bool app_bt_sync_enable(uint32_t opCode, uint8_t length, uint8_t *p_buff)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    bt_trigger_info_s p_trigger_info;
    uint32_t tg_tick = 0;
    uint32_t curr_ticks = 0;
    uint8_t index = 0xff;
    uint8_t btRole = BTIF_BCR_MASTER;
    uint32_t delayUs = 0;
    uint16_t twsSnifferInterval = 0;
    struct BT_DEVICE_T *curr_device = NULL;
    ibrt_mobile_info_t *p_mobile_info = NULL;
    bt_bdaddr_t *mobile_addr = NULL;
    bool mobileInSniff = false;

    if (app_bt_sync_get_supervisor_count() >= APP_BT_SYNC_CHANNEL_MAX) {
        return false;
    }

    /*
    * Support share info to peer bud
    * Max size: 128 - sizeof(bt_trigger_info_s)
    */
    uint8_t shareBuf[128] = {0};

    if ((length > (128 - sizeof(bt_trigger_info_s))) ||
        ((length > 0) && (NULL == p_buff))) {
        TRACE(1, "%s: parameter invalid! length = %d", __func__, length);
        return false;
    }

    btif_cmgr_handler_t *cmgrHandler = btif_lock_free_cmgr_get_acl_handler(p_ibrt_ctrl->p_tws_remote_dev);

    TRACE(3, "%s++ tws handle 0x%x opCode %d", __func__,
        p_ibrt_ctrl->tws_conhandle, opCode);

    if ((p_ibrt_ctrl->tws_conhandle == INVALID_HANDLE) || (cmgrHandler == NULL)) {
        TRACE(1, "%s: TWS link not exist", __func__);
        return false;
    }

    if (!app_bt_sync_module_init()) {
        TRACE(1, "%s: failed to initilize", __func__);
        return false;
    }

    btRole = btif_me_get_remote_device_role(p_ibrt_ctrl->p_tws_remote_dev);

    if (app_ibrt_conn_any_mobile_connected()) {
        for (uint8_t i = 0; i < BT_DEVICE_NUM; ++i)
        {
            curr_device = app_bt_get_device(i);
            mobile_addr = &curr_device->remote;
            p_mobile_info = app_ibrt_conn_get_mobile_info_by_addr(mobile_addr);

            if ((NULL != p_mobile_info) && (p_mobile_info->mobile_mode == IBRT_SNIFF_MODE)) {
                mobileInSniff = true;
                app_tws_ibrt_exit_sniff_with_mobile(mobile_addr);
                break;
            }
        }
    }

    if ( (mobileInSniff) || (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE)) {
        app_tws_ibrt_exit_sniff_with_tws();
        twsSnifferInterval = btif_cmgr_get_cmgrhandler_sniff_interval(cmgrHandler);
        TRACE(2, "%s: TWS in sniff mode,interval %d %d", __func__, twsSnifferInterval, mobileInSniff);
        delayUs = twsSnifferInterval * 1000 * 2;
    } else {
        delayUs = p_ibrt_ctrl->acl_interval * 625 * 3;
    }

    p_trigger_info.opCode = opCode;

    if (BTIF_BCR_MASTER == btRole) {
        /// get the trigger channel
        uint8_t chnl = app_bt_sync_get_avaliable_trigger_channel();
        if (chnl < (APP_BT_SYNC_CHANNEL_MAX + APP_BT_SYNC_CHANNEL_OFFSET)) {
            /// get the index of trigger management module(minus 2 for reserved channel0 for A2DP/HFP, channel1 for prompt)
            index = chnl - APP_BT_SYNC_CHANNEL_OFFSET;
        } else {
            return false;
        }
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle);
        tg_tick = curr_ticks + US_TO_BTCLKS(delayUs);

        locTriggerInfo[index].opCode = opCode;
        TRACE(4, "%s: triSrc %d, tg_tick 0x%x curr tick 0x%x delayUs %d", __func__,
            locTriggerInfo[index].triSrc, tg_tick, curr_ticks, delayUs);

        p_trigger_info.triSrc = locTriggerInfo[index].triSrc;
        p_trigger_info.used = locTriggerInfo[index].used;
        p_trigger_info.triTick = tg_tick;
    }

    memcpy(shareBuf, (uint8_t*)&p_trigger_info, sizeof(bt_trigger_info_s));
    if (length) {
        memcpy(shareBuf + sizeof(bt_trigger_info_s), p_buff, length);
    }

    if (tws_ctrl_send_cmd_high_priority(APP_TWS_CMD_SET_SYNC_TIME, shareBuf, sizeof(bt_trigger_info_s) + length)) {
        return false;
    } else {
        if (btRole == BTIF_BCR_MASTER) {
            bt_syn_set_tg_ticks(p_trigger_info.triTick, p_ibrt_ctrl->tws_conhandle,    \
                BT_TRIG_MASTER_ROLE, p_trigger_info.triSrc, false);

            hal_cmu_bt_trigger_set_handler((enum HAL_CMU_BT_TRIGGER_SRC_T)p_trigger_info.triSrc, app_bt_sync_irq_handler);
            hal_cmu_bt_trigger_enable((enum HAL_CMU_BT_TRIGGER_SRC_T)p_trigger_info.triSrc);
        }
    }

    app_bt_sync_add_waiting_irq_timeout_supervision(opCode, index);
    return true;
}

APP_BT_SYNC_INFO_REPORT_T app_bt_sync_info_report_func = NULL;

void app_bt_sync_register_report_info_callback(APP_BT_SYNC_INFO_REPORT_T cb)
{
    app_bt_sync_info_report_func = cb;
}

void app_bt_sync_get_current_tws_bt_ticks_in_us_on_slave_side(uint32_t* pMasterBtClockCnt,
    uint32_t* pSlaveBtTicksUs)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    int32_t offset;

    uint32_t slaveBtClkCnt = btdrv_syn_get_curr_ticks();

    uint8_t link_id = btdrv_conhdl_to_linkid(p_ibrt_ctrl->tws_conhandle);
    if (btdrv_is_link_index_valid(link_id))
    {
        offset = bt_drv_reg_op_get_clkoffset(link_id);
    }
    else
    {
        offset = 0;
    }

    *pMasterBtClockCnt = (slaveBtClkCnt + offset) & 0x0fffffff;

    *pSlaveBtTicksUs = btdrv_reg_op_bt_time_to_bts(slaveBtClkCnt, 0);
}

void app_bt_sync_get_master_time_from_slave_time(uint32_t SlaveBtTicksUs, uint32_t* p_master_clk_cnt, uint16_t* p_master_bit_cnt)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    int32_t offset;
    uint8_t link_id = btdrv_conhdl_to_linkid(p_ibrt_ctrl->tws_conhandle);
    uint32_t slaveClockCnt;
    uint16_t finecnt;
    btdrv_reg_op_bts_to_bt_time(SlaveBtTicksUs, &slaveClockCnt, &finecnt);

    if (btdrv_is_link_index_valid(link_id))
    {
        offset = bt_drv_reg_op_get_clkoffset(link_id);
    }
    else
    {
        offset = 0;
    }

    *p_master_clk_cnt = (slaveClockCnt + offset) & 0x0fffffff;
    *p_master_bit_cnt = finecnt;
}

uint32_t app_bt_sync_get_slave_time_from_master_time(uint32_t master_clk_cnt, uint16_t master_bit_cnt)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    int32_t offset;
    uint8_t link_id = btdrv_conhdl_to_linkid(p_ibrt_ctrl->tws_conhandle);

    if (btdrv_is_link_index_valid(link_id))
    {
        offset = bt_drv_reg_op_get_clkoffset(link_id);
    }
    else
    {
        offset = 0;
    }
    uint32_t slaveClockCnt = (master_clk_cnt - offset) & 0x0fffffff;

    uint32_t SlaveBtTicksUs = btdrv_reg_op_bt_time_to_bts(slaveClockCnt, master_bit_cnt);
    return SlaveBtTicksUs;
}

bool app_bt_sync_is_tws_slave(void)
{
    btif_connection_role_t connection_role = app_tws_ibrt_get_local_tws_role();

    if (connection_role == BTIF_BCR_SLAVE)
    {

        return true;
    }
    else
    {
        return false;
    }
}

void app_bt_sync_send_tws_cmd_done(uint8_t *ptrParam, uint16_t paramLen)
{
#if defined(__BT_SYNC__)
    if (paramLen < sizeof(bt_trigger_info_s)) {
        TRACE(1, "%s: length error", __func__);
        return;
    }

    bt_trigger_info_s triInfo;
    memcpy((uint8_t*)&triInfo, ptrParam, sizeof(bt_trigger_info_s));

    app_bt_sync_status_info_s *statusInfo = NULL;

    statusInfo = app_bt_sync_get_status_instance(triInfo.opCode);
    if (statusInfo) {
        statusInfo->twsCmdSendDone = true;
    }
#endif
}

void app_bt_sync_tws_cmd_handler(uint8_t *p_buff, uint16_t length)
{
#if defined(__BT_SYNC__)
    if (length < sizeof(bt_trigger_info_s)) {
        TRACE(1, "%s: length error", __func__);
        return;
    }

    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    uint32_t curr_ticks = 0;
    uint8_t btRole = BTIF_BCR_MASTER;
    bt_trigger_info_s triInfo;
    memcpy((uint8_t*)&triInfo, p_buff, sizeof(bt_trigger_info_s));

    if (app_bt_sync_info_report_func && length > sizeof(bt_trigger_info_s)) {
        app_bt_sync_info_report_func(triInfo.opCode, p_buff + sizeof(bt_trigger_info_s), length - sizeof(bt_trigger_info_s));
    }

    if (p_ibrt_ctrl->tws_conhandle == INVALID_HANDLE) {
        TRACE(1, "%s: TWS link not exist", __func__);
        return;
    }

    btRole = btif_me_get_remote_device_role(p_ibrt_ctrl->p_tws_remote_dev);
    TRACE(5, "%s++ triSrc %d opCode %d role %d tick 0x%x", __func__,
        triInfo.triSrc, triInfo.opCode, btRole, triInfo.triTick);

    if (BTIF_BCR_SLAVE == btRole) {
        if (!app_bt_sync_module_init()) {
            TRACE(1, "%s: failed to initilize", __func__);
            return;
        }

        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle);

        TRACE(5, "%s curr tick 0x%x", __func__,
             curr_ticks);

        if (curr_ticks >= triInfo.triTick){
            TRACE(1, "%s: exceed curTicks", __func__);
            return;
        }
        uint8_t index = triInfo.triSrc - APP_BT_SYNC_CHANNEL_OFFSET;

        memcpy (&locTriggerInfo[index], &triInfo, sizeof(bt_trigger_info_s));
        bt_syn_set_tg_ticks(locTriggerInfo[index].triTick, p_ibrt_ctrl->tws_conhandle,    \
            BT_TRIG_SLAVE_ROLE, locTriggerInfo[index].triSrc, false);
        hal_cmu_bt_trigger_set_handler((enum HAL_CMU_BT_TRIGGER_SRC_T)locTriggerInfo[index].triSrc, app_bt_sync_irq_handler);
        hal_cmu_bt_trigger_enable((enum HAL_CMU_BT_TRIGGER_SRC_T)locTriggerInfo[index].triSrc);
        app_bt_sync_update_supervisor(triInfo.opCode, index);
    }
    else if (BTIF_BCR_MASTER == btRole)
    {
        app_bt_sync_enable(triInfo.opCode, 0, NULL);
    } else {
        TRACE(1, "%s: error case", __func__);
    }
#endif
}

#endif


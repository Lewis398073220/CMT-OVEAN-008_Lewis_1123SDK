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
#ifdef CFG_APP_DATAPATH_SERVER
#include "gatt_service.h"
#include "app_ble.h"
#include "hal_norflash.h"
#include "hal_bootmode.h"
#include "retention_ram.h"

#define USE_128BIT_UUID 1
#define DATAPATHPS_MAX_LEN (509)

#if USE_128BIT_UUID

#ifdef IS_USE_CUSTOM_BLE_DATAPATH_PROFILE_UUID_ENABLED
#define datapath_service_uuid_128_le TW_BLE_DATAPATH_SERVICE_UUID
#define datapath_tx_character_uuid_128_le TW_BLE_DATAPATH_TX_CHAR_VAL_UUID
#define datapath_rx_character_uuid_128_le TW_BLE_DATAPATH_RX_CHAR_VAL_UUID
#else
#define datapath_service_uuid_128_le 0x12,0x34,0x56,0x78,0x90,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x01
#define datapath_tx_character_uuid_128_le 0x12,0x34,0x56,0x78,0x91,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x02
#define datapath_rx_character_uuid_128_le 0x12,0x34,0x56,0x78,0x92,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x00,0x03,0x00,0x03
#endif

GATT_DECL_128_LE_PRI_SERVICE(g_ble_datapath_service,
    datapath_service_uuid_128_le);

GATT_DECL_128_LE_CHAR(g_ble_datapath_rx_character,
    datapath_rx_character_uuid_128_le,
    GATT_WR_REQ|GATT_WR_CMD,
    ATT_WR_ENC);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_datapath_rx_cudd,
    ATT_SEC_NONE);

GATT_DECL_128_LE_CHAR(g_ble_datapath_tx_character,
    datapath_tx_character_uuid_128_le,
    GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CCCD_DESCRIPTOR(g_ble_datapath_tx_cccd,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_datapath_tx_cudd,
    ATT_SEC_NONE);

#else /* USE_128BIT_UUID */

#define datapath_service_uuid_16 0xFEF8
#define datapath_tx_character_uuid_16 0xFEF9
#define datapath_rx_character_uuid_16 0xFEFA

GATT_DECL_PRI_SERVICE(g_ble_datapath_service,
    datapath_service_uuid_16);

GATT_DECL_CHAR(g_ble_datapath_rx_character,
    datapath_rx_character_uuid_16,
    GATT_WR_REQ|GATT_WR_CMD,
    ATT_WR_ENC);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_datapath_rx_cudd,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_ble_datapath_tx_character,
    datapath_tx_character_uuid_16,
    GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CCCD_DESCRIPTOR(g_ble_datapath_tx_cccd,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_datapath_tx_cudd,
    ATT_SEC_NONE);

#endif /* USE_128BIT_UUID */

static const gatt_attribute_t g_ble_datapath_attr_list[] = {
    /* Service */
    gatt_attribute(g_ble_datapath_service),
    /* Characteristics */
    gatt_attribute(g_ble_datapath_rx_character),
        gatt_attribute(g_ble_datapath_rx_cudd),
    /* Characteristics */
    gatt_attribute(g_ble_datapath_tx_character),
        gatt_attribute(g_ble_datapath_tx_cccd),
        gatt_attribute(g_ble_datapath_tx_cudd),
};

struct app_datapath_server_env_tag app_datapath_server_env;

void app_datapath_server_send_data_via_notification(uint8_t* data, uint32_t len)
{
    uint8_t conidx = app_datapath_server_env.connectionIndex;
    gatts_send_notification(gap_conn_bf(conidx), g_ble_datapath_tx_character, data, (uint16_t)len);
}

void app_datapath_server_send_data_via_indication(uint8_t* data, uint32_t len)
{
    uint8_t conidx = app_datapath_server_env.connectionIndex;
    gatts_send_indication(gap_conn_bf(conidx), g_ble_datapath_tx_character, data, (uint16_t)len);
}

void app_datapath_server_send_data_via_write_command(uint8_t* ptrData, uint32_t length)
{

}

void app_datapath_server_send_data_via_write_request(uint8_t* ptrData, uint32_t length)
{

}

void app_datapath_server_control_notification(uint8_t conidx,bool isEnable)
{

}

static app_datapath_event_cb dp_event_callback = NULL;
static app_datapath_server_tx_done_t tx_done_callback = NULL;
static app_datapath_server_data_received_callback_func_t rx_done_callback = NULL;
static app_datapath_server_disconnected_done_t disconnected_done_callback = NULL;
static app_datapath_server_connected_done_t connected_done_callback = NULL;
static app_datapath_server_mtuexchanged_done_t mtuexchanged_done_callback = NULL;

#define BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT 8
#define BLE_RAW_DATA_XFER_BUF_SIZE 80

typedef struct
{
    uint16_t        entryIndex;         /**< The command waiting for the response */
    uint16_t        msTillTimeout;    /**< run-time timeout left milliseconds */
} BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T;

typedef struct
{
    uint8_t        isInRawDataXferStage;         /**< true if the received data is raw data,
                                                false if the received data is in the format of BLE_CUSTOM_CMD_PAYLOAD_T*/
    uint16_t    lengthOfRawDataXferToReceive;
    uint16_t    lengthOfReceivedRawDataXfer;
    uint8_t*    ptrRawXferDstBuf;
    BLE_RawDataReceived_Handler_t    rawDataHandler;
    BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T    waitingRspTimeoutInstance[BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT];
    uint32_t    lastSysTicks;
    uint8_t        timeoutSupervisorCount;
    osTimerId    supervisor_timer_id;
    osMutexId    mutex;
} BLE_CUSTOM_CMD_ENV_T;

static const char custom_tx_desc[] = "Data Path TX Data";
static const char custom_rx_desc[] = "Data Path RX Data";
static uint8_t rawDataXferBuf[BLE_RAW_DATA_XFER_BUF_SIZE];
static BLE_CUSTOM_CMD_ENV_T ble_custom_cmd_env;
osMutexDef(app_ble_cmd_mutex);
static void ble_custom_cmd_rsp_supervision_timer_cb(void const *n);
osTimerDef (APP_CUSTOM_CMD_RSP_SUPERVISION_TIMER, ble_custom_cmd_rsp_supervision_timer_cb);
BLE_CUSTOM_CMD_RET_STATUS_E BLE_custom_command_receive_data(uint8_t* ptrData, uint32_t dataLength);

static void app_datapath_server_mtu_exchanged(uint8_t conidx, uint16_t connhdl, uint16_t mtu)
{
    if (NULL != mtuexchanged_done_callback)
    {
        mtuexchanged_done_callback(conidx, mtu);
    }

    if(dp_event_callback)
    {
        app_dp_mtu_exchange_msg_t msg_data;
        msg_data.conidx = conidx;
        msg_data.mtu    = mtu;
        dp_event_callback(DP_MTU_CHANGE_DONE, (ble_if_app_dp_param_u *)&msg_data);
    }
}

static void app_datapath_server_connected(uint8_t conidx, uint16_t connhdl)
{
    TRACE(0,"app datapath server connected.");
    app_datapath_server_env.connectionIndex = conidx;

    if (NULL != connected_done_callback)
    {
        connected_done_callback(conidx);
    }

    if (dp_event_callback)
    {
        dp_event_callback(DP_CONN_DONE, (ble_if_app_dp_param_u *)&conidx);
    }
}

static void app_datapath_server_disconnected(uint8_t conidx, uint16_t connhdl)
{
    if (conidx == app_datapath_server_env.connectionIndex)
    {
        TRACE(0,"app datapath server dis-connected.");
        app_datapath_server_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
        app_datapath_server_env.isNotificationEnabled = false;

        tx_done_callback = NULL;
    }

    if (NULL != disconnected_done_callback)
    {
        disconnected_done_callback(conidx);
    }

    if (dp_event_callback)
    {
        dp_event_callback(DP_DISCONN_DONE, (ble_if_app_dp_param_u *)&conidx);
    }
}

static void app_datapath_server_tx_ccc_changed(uint8_t conidx, uint16_t connhdl, bool notify_enabled)
{
    app_datapath_server_env.isNotificationEnabled = notify_enabled;

    if (app_datapath_server_env.isNotificationEnabled) 
    {
        if (BLE_INVALID_CONNECTION_INDEX == app_datapath_server_env.connectionIndex)
        {
            app_datapath_server_connected(conidx, connhdl);
        }
    }
}

static void app_datapath_server_tx_data_sent(uint8_t conidx, uint16_t connhdl)
{
    if (NULL != tx_done_callback)
    {
        tx_done_callback();
    }

    if(dp_event_callback)
    {
        dp_event_callback(DP_TX_DONE, NULL);
    }
}

static void app_datapath_server_rx_data_received(uint8_t conidx, uint16_t connhdl, const uint8_t *data, uint16_t len)
{
    // loop back the received data
    app_datapath_server_send_data_via_notification((uint8_t *)data, len);

    TRACE(2,"%s length %d", __func__, len);

#ifndef __INTERCONNECTION__
    BLE_custom_command_receive_data((uint8_t *)data, len);
#endif

    if (NULL != rx_done_callback)
    {
        rx_done_callback((uint8_t *)data, len);
    }

    if (dp_event_callback)
    {
        app_dp_rec_data_msg_t data_msg;
        data_msg.data     = (uint8_t *)data;
        data_msg.data_len = len;
        dp_event_callback(DP_DATA_RECEIVED, (ble_if_app_dp_param_u *)&data_msg);
    }
}

static int ble_datapath_server_callback(gatt_svc_t *svc, gatt_server_event_t event, gatt_server_callback_param_t param)
{
    switch (event)
    {
        case GATT_SERV_EVENT_CHAR_WRITE:
        {
            gatt_server_char_write_t *p = param.char_write;
            if (p->value_offset != 0 || p->value_len == 0 || p->value == NULL)
            {
                return false;
            }
            app_datapath_server_rx_data_received(svc->con_idx, svc->connhdl, p->value, p->value_len);
            return true;
        }
        case GATT_SERV_EVENT_DESC_WRITE:
        {
            gatt_server_desc_write_t *p = param.desc_write;
            uint16_t config = CO_COMBINE_UINT16_LE(p->value);
            bool notify_enabled = false;
            if (config & GATT_CCCD_SET_NOTIFICATION)
            {
                notify_enabled = true;
            }
            app_datapath_server_tx_ccc_changed(svc->con_idx, svc->connhdl, notify_enabled);
            return true;
        }
        case GATT_SERV_EVENT_NTF_TX_DONE:
        case GATT_SERV_EVENT_INDICATE_CFM:
        {
            app_datapath_server_tx_data_sent(svc->con_idx, svc->connhdl);
            break;
        }
        case GATT_SERV_EVENT_MTU_CHANGED:
        {
            gatt_server_mtu_changed_t *p = param.mtu_changed;
            app_datapath_server_mtu_exchanged(svc->con_idx, svc->connhdl, p->mtu);
            break;
        }
        case GATT_SERV_EVENT_CONN_CLOSED:
        {
            app_datapath_server_disconnected(svc->con_idx, svc->connhdl);
            break;
        }
        case GATT_SERV_EVENT_DESC_READ:
        {
            gatt_server_desc_read_t *p = param.desc_read;
            if (p->descriptor == g_ble_datapath_tx_cudd)
            {
                gatts_write_read_rsp_data(p->ctx, (uint8_t *)custom_tx_desc, sizeof(custom_tx_desc));
                return true;
            }
            else if (p->descriptor == g_ble_datapath_rx_cudd)
            {
                gatts_write_read_rsp_data(p->ctx, (uint8_t *)custom_rx_desc, sizeof(custom_rx_desc));
                return true;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

void app_datapath_server_register_event_cb(app_datapath_event_cb callback)
{
    dp_event_callback = callback;
}

void app_datapath_server_register_tx_done(app_datapath_server_tx_done_t callback)
{
    tx_done_callback = callback;
}

void app_datapath_server_register_rx_done(app_datapath_server_data_received_callback_func_t callback)
{
    rx_done_callback = callback;
}

void app_datapath_server_register_disconnected_done(app_datapath_server_disconnected_done_t callback)
{
    disconnected_done_callback = callback;
}

void app_datapath_server_register_connected_done(app_datapath_server_connected_done_t callback)
{
    connected_done_callback = callback;
}

void app_datapath_server_register_mtu_exchanged_done(app_datapath_server_mtuexchanged_done_t callback)
{
    mtuexchanged_done_callback = callback;
}

void ble_datapath_init(void)
{
    app_datapath_server_env.connectionIndex =  BLE_INVALID_CONNECTION_INDEX;
    app_datapath_server_env.isNotificationEnabled = false;

    memset((uint8_t *)&ble_custom_cmd_env, 0, sizeof(ble_custom_cmd_env));
    ble_custom_cmd_env.supervisor_timer_id =
        osTimerCreate(osTimer(APP_CUSTOM_CMD_RSP_SUPERVISION_TIMER), osTimerOnce, NULL);
    ble_custom_cmd_env.mutex = osMutexCreate((osMutex(app_ble_cmd_mutex)));

    gatts_register_service(g_ble_datapath_attr_list, ARRAY_SIZE(g_ble_datapath_attr_list), ble_datapath_server_callback, NULL);
}

static void BLE_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex);
static void ble_custom_cmd_rsp_supervision_timer_cb(void const *n)
{
    uint32_t entryIndex = ble_custom_cmd_env.waitingRspTimeoutInstance[0].entryIndex;

    BLE_remove_waiting_rsp_timeout_supervision(entryIndex);

    // it means time-out happens before the response is received from the peer device,
    // trigger the response handler
    CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex)->cmdRspHandler(TIMEOUT_WAITING_RESPONSE, NULL, 0);
}

void BLE_get_response_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    // parameter length check
    if (paramLen > sizeof(BLE_CUSTOM_CMD_RSP_T))
    {
        return;
    }

    if (0 == ble_custom_cmd_env.timeoutSupervisorCount)
    {
        return;
    }

    BLE_CUSTOM_CMD_RSP_T* rsp = (BLE_CUSTOM_CMD_RSP_T *)ptrParam;

    BLE_CUSTOM_CMD_INSTANCE_T* ptCmdInstance =
        BLE_custom_command_get_entry_pointer_from_cmd_code(rsp->cmdCodeToRsp);
    if (NULL == ptCmdInstance)
    {
        return;
    }

    // remove the function code from the time-out supervision chain
    uint16_t entryIndex = BLE_custom_command_get_entry_index_from_cmd_code(rsp->cmdCodeToRsp);
    BLE_remove_waiting_rsp_timeout_supervision(entryIndex);

    // call the response handler
    if (ptCmdInstance->cmdRspHandler)
    {
        ptCmdInstance->cmdRspHandler(rsp->cmdRetStatus, rsp->rspData, rsp->rspDataLen);
    }
}

void BLE_control_raw_data_xfer(bool isStartXfer)
{
    ble_custom_cmd_env.isInRawDataXferStage = isStartXfer;

    if (true == isStartXfer)
    {
        ble_custom_cmd_env.lengthOfReceivedRawDataXfer = 0;
        // default configuration, can be customized by BLE_config_raw_data_xfer
        ble_custom_cmd_env.lengthOfRawDataXferToReceive = sizeof(rawDataXferBuf);
        ble_custom_cmd_env.ptrRawXferDstBuf = (uint8_t *)&rawDataXferBuf;
        ble_custom_cmd_env.rawDataHandler = NULL;
    }
}

void BLE_set_raw_data_xfer_received_callback(BLE_RawDataReceived_Handler_t callback)
{
    ble_custom_cmd_env.rawDataHandler = callback;
}

void BLE_raw_data_xfer_control_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    bool isStartXfer = false;

    if (OP_START_RAW_DATA_XFER == funcCode)
    {
        isStartXfer = true;
    }

    BLE_control_raw_data_xfer(isStartXfer);

    BLE_send_response_to_command(funcCode, NO_ERROR, NULL, 0, TRANSMISSION_VIA_NOTIFICATION);
}

void BLE_start_raw_data_xfer_control_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if (NO_ERROR == retStatus)
    {
        /**< now the sending data api won't wrap BLE_CUSTOM_CMD_PAYLOAD_T but will directly send the raw data */
        ble_custom_cmd_env.isInRawDataXferStage = true;
    }
}

void BLE_stop_raw_data_xfer_control_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if (NO_ERROR == retStatus)
    {
        ble_custom_cmd_env.isInRawDataXferStage = false;
    }
}

static void ble_custom_cmd_refresh_supervisor_env(void)
{
    // do nothing if no supervisor was added
    if (ble_custom_cmd_env.timeoutSupervisorCount > 0)
    {
        uint32_t currentTicks = GET_CURRENT_TICKS();
        uint32_t passedTicks;
        if (currentTicks >= ble_custom_cmd_env.lastSysTicks)
        {
            passedTicks = (currentTicks - ble_custom_cmd_env.lastSysTicks);
        }
        else
        {
            passedTicks = (hal_sys_timer_get_max() - ble_custom_cmd_env.lastSysTicks + 1) + currentTicks;
        }

        uint32_t deltaMs = TICKS_TO_MS(passedTicks);

        BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T* pRspSupervisor = &(ble_custom_cmd_env.waitingRspTimeoutInstance[0]);
        for (uint32_t index = 0;index < ble_custom_cmd_env.timeoutSupervisorCount;index++)
        {
            ASSERT(pRspSupervisor[index].msTillTimeout > deltaMs,
                "the waiting command response supervisor timer is missing!!!, \
                %d ms passed but the ms to trigger is %d", deltaMs, pRspSupervisor[index].msTillTimeout);
            pRspSupervisor[index].msTillTimeout -= deltaMs;
        }
    }

    ble_custom_cmd_env.lastSysTicks = GET_CURRENT_TICKS();
}

static void BLE_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
    ASSERT(ble_custom_cmd_env.timeoutSupervisorCount > 0,
        "%s The BLE custom command time-out supervisor is already empty!!!", __FUNCTION__);

    osMutexWait(ble_custom_cmd_env.mutex, osWaitForever);

    uint32_t index;
    for (index = 0;index < ble_custom_cmd_env.timeoutSupervisorCount;index++)
    {
        if (ble_custom_cmd_env.waitingRspTimeoutInstance[index].entryIndex == entryIndex)
        {
            memcpy(&(ble_custom_cmd_env.waitingRspTimeoutInstance[index]),
                &(ble_custom_cmd_env.waitingRspTimeoutInstance[index + 1]),
                (ble_custom_cmd_env.timeoutSupervisorCount - index - 1)*sizeof(BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T));
            break;
        }
    }

    // cannot find it, directly return
    if (index == ble_custom_cmd_env.timeoutSupervisorCount)
    {
        goto exit;
    }

    ble_custom_cmd_env.timeoutSupervisorCount--;

    if (ble_custom_cmd_env.timeoutSupervisorCount > 0)
    {
        // refresh supervisor environment firstly
        ble_custom_cmd_refresh_supervisor_env();

        // start timer, the first entry is the most close one
        osTimerStart(ble_custom_cmd_env.supervisor_timer_id, ble_custom_cmd_env.waitingRspTimeoutInstance[0].msTillTimeout);
    }
    else
    {
        // no supervisor, directly stop the timer
        osTimerStop(ble_custom_cmd_env.supervisor_timer_id);
    }

exit:
    osMutexRelease(ble_custom_cmd_env.mutex);
}

static void BLE_add_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
    ASSERT(ble_custom_cmd_env.timeoutSupervisorCount < BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT,
        "%s The BLE custom command time-out supervisor is full!!!", __FUNCTION__);

    osMutexWait(ble_custom_cmd_env.mutex, osWaitForever);

    // refresh supervisor environment firstly
    ble_custom_cmd_refresh_supervisor_env();

    BLE_CUSTOM_CMD_INSTANCE_T* pInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

    BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T    waitingRspTimeoutInstance[BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT];

    uint32_t index = 0, insertedIndex = 0;
    for (index = 0;index < ble_custom_cmd_env.timeoutSupervisorCount;index++)
    {
        uint32_t msTillTimeout = ble_custom_cmd_env.waitingRspTimeoutInstance[index].msTillTimeout;

        // in the order of low to high
        if ((ble_custom_cmd_env.waitingRspTimeoutInstance[index].entryIndex != entryIndex) &&
            (pInstance->timeoutWaitingRspInMs >= msTillTimeout))
        {
            waitingRspTimeoutInstance[insertedIndex++] = ble_custom_cmd_env.waitingRspTimeoutInstance[index];
        }
        else if (pInstance->timeoutWaitingRspInMs < msTillTimeout)
        {
            waitingRspTimeoutInstance[insertedIndex].entryIndex = entryIndex;
            waitingRspTimeoutInstance[insertedIndex].msTillTimeout = pInstance->timeoutWaitingRspInMs;

            insertedIndex++;
        }
    }

    // biggest one? then put it at the end of the list
    if (ble_custom_cmd_env.timeoutSupervisorCount == index)
    {
        waitingRspTimeoutInstance[insertedIndex].entryIndex = entryIndex;
        waitingRspTimeoutInstance[insertedIndex].msTillTimeout = pInstance->timeoutWaitingRspInMs;

        insertedIndex++;
    }

    // copy to the global variable
    memcpy((uint8_t *)&(ble_custom_cmd_env.waitingRspTimeoutInstance), (uint8_t *)&waitingRspTimeoutInstance,
        insertedIndex*sizeof(BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T));

    ble_custom_cmd_env.timeoutSupervisorCount = insertedIndex;

    // start timer, the first entry is the most close one
    osTimerStart(ble_custom_cmd_env.supervisor_timer_id, ble_custom_cmd_env.waitingRspTimeoutInstance[0].msTillTimeout);

    osMutexRelease(ble_custom_cmd_env.mutex);
}

uint8_t* BLE_custom_command_raw_data_buffer_pointer(void)
{
    return ble_custom_cmd_env.ptrRawXferDstBuf;
}

uint16_t BLE_custom_command_received_raw_data_size(void)
{
    return ble_custom_cmd_env.lengthOfReceivedRawDataXfer;
}

BLE_CUSTOM_CMD_RET_STATUS_E BLE_custom_command_receive_data(uint8_t* ptrData, uint32_t dataLength)
{
    TRACE(1,"Receive length %d data: ", dataLength);
    DUMP8("0x%02x ", ptrData, dataLength);
    BLE_CUSTOM_CMD_PAYLOAD_T* pPayload = (BLE_CUSTOM_CMD_PAYLOAD_T *)ptrData;

    if ((OP_START_RAW_DATA_XFER == pPayload->cmdCode) ||
        (OP_STOP_RAW_DATA_XFER == pPayload->cmdCode) ||
        (!(ble_custom_cmd_env.isInRawDataXferStage)))
    {
        // check command code
        if (pPayload->cmdCode >= OP_COMMAND_COUNT)
        {
            return INVALID_CMD_CODE;
        }

        // check parameter length
        if (pPayload->paramLen > sizeof(pPayload->param))
        {
            return PARAMETER_LENGTH_OUT_OF_RANGE;
        }

        BLE_CUSTOM_CMD_INSTANCE_T* pInstance =
            BLE_custom_command_get_entry_pointer_from_cmd_code(pPayload->cmdCode);

        // execute the command handler
        if (pInstance)
        {
            pInstance->cmdHandler(pPayload->cmdCode, pPayload->param, pPayload->paramLen);
        }
    }
    else
    {
        // the payload of the raw data xfer is 2 bytes cmd code + raw data
        if (dataLength < sizeof(pPayload->cmdCode))
        {
            return PARAMETER_LENGTH_TOO_SHORT;
        }

        dataLength -= sizeof(pPayload->cmdCode);
        ptrData += sizeof(pPayload->cmdCode);

        if (NULL == ble_custom_cmd_env.rawDataHandler)
        {
            // default handler

            // save the received raw data into raw data buffer
            uint32_t bytesToSave;
            if ((dataLength + ble_custom_cmd_env.lengthOfReceivedRawDataXfer) > \
                ble_custom_cmd_env.lengthOfRawDataXferToReceive)
            {
                bytesToSave = ble_custom_cmd_env.lengthOfRawDataXferToReceive - \
                    ble_custom_cmd_env.lengthOfReceivedRawDataXfer;
            }
            else
            {
                bytesToSave = dataLength;
            }
            memcpy((uint8_t *)&ble_custom_cmd_env.ptrRawXferDstBuf[ble_custom_cmd_env.lengthOfReceivedRawDataXfer], \
                ptrData, bytesToSave);


            ble_custom_cmd_env.lengthOfReceivedRawDataXfer += bytesToSave;
        }
        else
        {
            // custom handler that is set by BLE_set_raw_data_xfer_received_callback
            ble_custom_cmd_env.rawDataHandler(ptrData, dataLength);
        }
    }

    return NO_ERROR;
}

static void BLE_send_out_data(BLE_CUSTOM_CMD_TRANSMISSION_PATH_E path, BLE_CUSTOM_CMD_PAYLOAD_T* ptPayLoad)
{
    switch (path)
    {
        case TRANSMISSION_VIA_NOTIFICATION:
            app_datapath_server_send_data_via_notification((uint8_t *)ptPayLoad,
                (uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
            break;
        case TRANSMISSION_VIA_INDICATION:
            app_datapath_server_send_data_via_indication((uint8_t *)ptPayLoad,
                (uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
            break;
        case TRANSMISSION_VIA_WRITE_CMD:
            app_datapath_server_send_data_via_write_command((uint8_t *)ptPayLoad,
                (uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
            break;
        case TRANSMISSION_VIA_WRITE_REQ:
            app_datapath_server_send_data_via_write_request((uint8_t *)ptPayLoad,
                (uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
            break;
        default:
            break;
    }
}

BLE_CUSTOM_CMD_RET_STATUS_E BLE_send_response_to_command
    (uint32_t responsedCmdCode, BLE_CUSTOM_CMD_RET_STATUS_E returnStatus,
    uint8_t* rspData, uint32_t rspDataLen, BLE_CUSTOM_CMD_TRANSMISSION_PATH_E path)
{
    // check responsedCmdCode's validity
    if (responsedCmdCode >= OP_COMMAND_COUNT)
    {
        return INVALID_CMD_CODE;
    }

    BLE_CUSTOM_CMD_PAYLOAD_T payload;

    BLE_CUSTOM_CMD_RSP_T* pResponse = (BLE_CUSTOM_CMD_RSP_T *)&(payload.param);

    // check parameter length
    if (rspDataLen > sizeof(pResponse->rspData))
    {
        return PARAMETER_LENGTH_OUT_OF_RANGE;
    }

    pResponse->cmdCodeToRsp = responsedCmdCode;
    pResponse->cmdRetStatus = returnStatus;
    pResponse->rspDataLen = rspDataLen;
    memcpy(pResponse->rspData, rspData, rspDataLen);

    payload.paramLen = 3*sizeof(uint16_t) + rspDataLen;

    payload.cmdCode = OP_RESPONSE_TO_CMD;

    BLE_send_out_data(path, &payload);

    return NO_ERROR;
}

BLE_CUSTOM_CMD_RET_STATUS_E BLE_send_custom_command(uint32_t cmdCode,
    uint8_t* ptrParam, uint32_t paramLen, BLE_CUSTOM_CMD_TRANSMISSION_PATH_E path)
{
    // check cmdCode's validity
    if (cmdCode >= OP_COMMAND_COUNT)
    {
        return INVALID_CMD_CODE;
    }

    BLE_CUSTOM_CMD_PAYLOAD_T payload;

    // check parameter length
    if (paramLen > sizeof(payload.param))
    {
        return PARAMETER_LENGTH_OUT_OF_RANGE;
    }

     uint16_t entryIndex = BLE_custom_command_get_entry_index_from_cmd_code(cmdCode);
     BLE_CUSTOM_CMD_INSTANCE_T* pInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

    // wrap the command payload
    payload.cmdCode = cmdCode;
    payload.paramLen = paramLen;
    memcpy(payload.param, ptrParam, paramLen);

    // send out the data
    BLE_send_out_data(path, &payload);

    // insert into time-out supervison
    if (pInstance->isNeedResponse)
    {
        BLE_add_waiting_rsp_timeout_supervision(entryIndex);
    }

    return NO_ERROR;
}

BLE_CUSTOM_CMD_INSTANCE_T* BLE_custom_command_get_entry_pointer_from_cmd_code(uint16_t cmdCode)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__custom_handler_table_end-(uint32_t)__custom_handler_table_start)/sizeof(BLE_CUSTOM_CMD_INSTANCE_T);index++)
    {
        if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
        {
            return CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    return NULL;
}

uint16_t BLE_custom_command_get_entry_index_from_cmd_code(uint16_t cmdCode)
{
    for (uint32_t index = 0; index < ((uint32_t)__custom_handler_table_end - \
        (uint32_t)__custom_handler_table_start)/sizeof(BLE_CUSTOM_CMD_INSTANCE_T); index++)
    {
        if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
        {
            return index;
        }
    }

    return INVALID_CUSTOM_ENTRY_INDEX;
}

void BLE_dummy_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{

}

void BLE_test_no_response_print_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE(1,"%s Get OP_TEST_NO_RESPONSE_PRINT command!!!", __FUNCTION__);
}

void BLE_test_with_response_print_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE(1,"%s Get OP_TEST_WITH_RESPONSE_PRINT command!!!", __FUNCTION__);

    uint32_t currentTicks = GET_CURRENT_TICKS();
    BLE_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&currentTicks, sizeof(currentTicks), TRANSMISSION_VIA_NOTIFICATION);
}

void BLE_test_with_response_print_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if (NO_ERROR == retStatus)
    {
        TRACE(1,"%s Get the response of OP_TEST_WITH_RESPONSE_PRINT command!!!", __FUNCTION__);
    }
    else if (TIMEOUT_WAITING_RESPONSE == retStatus)
    {
        TRACE(1,"%s Timeout happens, doesn't get the response of OP_TEST_WITH_RESPONSE_PRINT command!!!", __FUNCTION__);
    }
}

static void app_otaMode_enter(void)
{
    TRACE(1,"%s",__func__);
    hal_norflash_disable_protection(HAL_FLASH_ID_0);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
    /*hal_cmu_reset_set(HAL_CMU_MOD_P_GLOBAL);*/
}

void BLE_enter_OTA_mode_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    fillBleBdAddrForOta(ptrParam);
    app_otaMode_enter();
}

#ifdef __SW_IIR_EQ_PROCESS__
int audio_config_eq_iir_via_config_structure(uint8_t *buf, uint32_t  len);
void BLE_iir_eq_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{

    audio_config_eq_iir_via_config_structure(BLE_custom_command_raw_data_buffer_pointer(), BLE_custom_command_received_raw_data_size());
}
#endif

void BLE_get_bt_address_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    app_datapath_server_send_data_via_notification((uint8_t *)gap_hci_bt_address(), sizeof(bt_bdaddr_t));
}

CUSTOM_COMMAND_TO_ADD(OP_RESPONSE_TO_CMD,             BLE_get_response_handler,                  false,  0,      NULL );
CUSTOM_COMMAND_TO_ADD(OP_START_RAW_DATA_XFER,         BLE_raw_data_xfer_control_handler,      true,      2000,      BLE_start_raw_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_STOP_RAW_DATA_XFER,         BLE_raw_data_xfer_control_handler,      true,      2000,      BLE_stop_raw_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_TEST_NO_RESPONSE_PRINT,    BLE_test_no_response_print_handler,     false,    0,        NULL );
CUSTOM_COMMAND_TO_ADD(OP_TEST_WITH_RESPONSE_PRINT,     BLE_test_with_response_print_handler,    true,    5000,     BLE_test_with_response_print_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_ENTER_OTA_MODE,             BLE_enter_OTA_mode_handler,            false,    0,         NULL );
#ifdef __SW_IIR_EQ_PROCESS__
CUSTOM_COMMAND_TO_ADD(OP_SW_IIR_EQ,                 BLE_iir_eq_handler,                    false,    0,         NULL );
#endif
CUSTOM_COMMAND_TO_ADD(OP_GET_BT_ADDRESS,             BLE_get_bt_address_handler,            false,    0,        NULL );

#endif /* CFG_APP_DATAPATH_SERVER */

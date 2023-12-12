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
#include "hal_trace.h"
#include "hal_timer.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_aud.h"
#include "hal_norflash.h"
#include "pmu.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
#include "cmsis_os.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_spp_tota.h"
#include "cqueue.h"
#ifdef __IAG_BLE_INCLUDE__
#include "bluetooth_ble_api.h"
#include "ota_ble_adapter.h"
#endif
#include "bluetooth_bt_api.h"
#include "app_bt_audio.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_location.h"
#include "app_hfp.h"
#include "bt_drv_reg_op.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif
#include "cmsis.h"
#include "app_battery.h"
#include "crc32_c.h"
#include "factory_section.h"
#include "app_ibrt_rssi.h"
#if (BLE_TOTA_ENABLED)
#ifndef BLE_STACK_NEW_DESIGN
#include "app_tota_ble.h"
#endif
#endif
#if defined(OTA_OVER_TOTA_ENABLED)
#include "ota_control.h"
#endif
#include "app_tota_encrypt.h"
#include "app_tota_flash_program.h"
#include "app_tota_audio_dump.h"
#include "app_tota_mic.h"
#include "app_tota_anc.h"
#include "app_tota_general.h"
#include "app_tota_custom.h"
#include "app_tota_conn.h"
#include "encrypt/aes.h"
#include "app_tota_audio_EQ.h"
#include "app_tota_common.h"
#include "app_bt_cmd.h"
#include "app_tota_multi_chip_updata.h"
#include <map>

#include "beslib_info.h"

#ifdef BLE_TOTA_ENABLED
#include "ota_ble_adapter.h"
#endif

#ifdef USB_OTA_ENABLE
#include "app_usb_tota.h"
#endif

using namespace std;

#define DONGLE_PACKET_FRAME 208
#define DONGLE_DATA_FRAME 192

static APP_TOTA_CMD_PAYLOAD_T payload;
/*call back sys for modules*/
static map<APP_TOTA_MODULE_E, const app_tota_callback_func_t *> s_module_map;
static const app_tota_callback_func_t *s_module_func = NULL;
static APP_TOTA_MODULE_E s_module;

#ifdef BLE_TOTA_ENABLED
/// health thermometer application environment structure
struct ble_tota_env_info
{
    uint8_t  connectionIndex;
    uint8_t  isNotificationEnabled;
    uint16_t connhdl;
    uint16_t mtu;
};

struct ble_tota_env_info ble_tota_env;
#endif

/* register callback module */
void app_tota_callback_module_register(APP_TOTA_MODULE_E module,
                                const app_tota_callback_func_t *tota_callback_func)
{
    map<APP_TOTA_MODULE_E, const app_tota_callback_func_t *>::iterator it = s_module_map.find(module);
    if ( it == s_module_map.end() )
    {
        TOTA_LOG_DBG(0, "add to map");
        s_module_map.insert(make_pair(module, tota_callback_func));
    }
    else
    {
        TOTA_LOG_DBG(0, "already exist, not add");
    }
}
/* set current module */
void app_tota_callback_module_set(APP_TOTA_MODULE_E module)
{
    map<APP_TOTA_MODULE_E, const app_tota_callback_func_t *>::iterator it = s_module_map.find(module);
    if ( it != s_module_map.end() )
    {
        s_module = module;
        s_module_func = it->second;
        TOTA_LOG_DBG(1, "set %d module success", module);
    }
    else
    {
        TOTA_LOG_DBG(0, "not find callback func by module");
    }
}

/* get current module */
APP_TOTA_MODULE_E app_tota_callback_module_get()
{
    return s_module;
}
/*-------------------------------------------------------------------*/

static void s_app_tota_connected();
static void s_app_tota_disconnected();
static void s_app_tota_tx_done();
static void s_app_tota_rx(uint8_t * cmd_buf, uint16_t len);

static void s_app_tota_connected()
{
    TOTA_LOG_DBG(0,"Tota connected.");
    app_tota_store_reset();
    tota_set_connect_status(TOTA_CONNECTED);
    tota_set_connect_path(APP_TOTA_VIA_SPP);
    if (s_module_func && s_module_func->connected_cb != NULL)
    {
        s_module_func->connected_cb();
    }
}

static void s_app_tota_disconnected()
{
    TOTA_LOG_DBG(0,"Tota disconnected.");
    app_tota_store_reset();
    app_tota_ctrl_reset();
    tota_rx_queue_deinit();
    tota_set_connect_status(TOTA_DISCONNECTED);
    tota_set_connect_path(APP_TOTA_PATH_IDLE);

    if (s_module_func && s_module_func->disconnected_cb != NULL)
    {
        s_module_func->disconnected_cb();
    }
}

static void s_app_tota_tx_done()
{
    TOTA_LOG_DBG(0,"Tota tx done.");
    if (s_module_func && s_module_func->tx_done_cb != NULL)
    {
        s_module_func->tx_done_cb();
    }
}

#ifdef BLE_TOTA_ENABLED
#if defined(OTA_OVER_TOTA_ENABLED)
static void ota_tota_send_notification(uint8_t* ptrData, uint32_t length)
{
    app_tota_send_data(OP_TOTA_OTA, ptrData, length);
}
#endif

static void tota_ble_event_ccc_change_handler(bes_ble_tota_event_param_t *param, uint8_t ntf_en)
{
    if (ntf_en)
    {
        if (!ble_tota_env.isNotificationEnabled)
        {
            ble_tota_env.isNotificationEnabled = ntf_en;
            ble_tota_env.connectionIndex = param->conidx;
            ble_tota_env.connhdl = param->connhdl;
            app_tota_store_reset();
            tota_set_connect_status(TOTA_CONNECTED);
            tota_set_connect_path(APP_TOTA_VIA_NOTIFICATION);
            TOTA_LOG_DBG(3,"[%s], condix = %d connectionIndex = %d",__func__,param->conidx, ble_tota_env.connectionIndex);
#if defined(BLE_TOTA_ENABLED) && defined(OTA_OVER_TOTA_ENABLED)
            ota_control_register_transmitter(ota_tota_send_notification);
            bes_ble_gap_conn_update_param(param->conidx, 10, 15, 20000, 0);
#endif

            if (ble_tota_env.mtu)
            {
                tota_set_trans_MTU(ble_tota_env.mtu);
#if defined(BLE_TOTA_ENABLED) && defined(OTA_OVER_TOTA_ENABLED)
                ota_control_update_MTU(ble_tota_env.mtu - TOTA_PACKET_VERIFY_SIZE);
#endif
            }
        }
    }
    else
    {
        if(ble_tota_env.isNotificationEnabled)
        {
            ble_tota_env.isNotificationEnabled = ntf_en;
            tota_set_connect_path(APP_TOTA_VIA_INDICATION);
            app_tota_ble_disconnected();
        }
    }
}

static void tota_ble_event_callback(bes_ble_tota_event_param_t *param)
{
    switch(param->event_type) {
    case BES_BLE_TOTA_CCC_CHANGED:
        TOTA_LOG_DBG(3,"[%s], condix = %d  %p",__func__, param->conidx,(void *)param);
        tota_ble_event_ccc_change_handler(param, param->param.ntf_en);
        break;
    case BES_BLE_TOTA_DIS_CONN_EVENT:
        TOTA_LOG_DBG(3,"[%s], condix = %d Index = %d",__func__, param->conidx, ble_tota_env.connectionIndex);
        if (param->conidx == ble_tota_env.connectionIndex)
        {
            ble_tota_env.connectionIndex = INVALID_CONNECTION_INDEX;
            ble_tota_env.isNotificationEnabled = false;
            ble_tota_env.mtu = 0;
            tota_set_connect_path(APP_TOTA_PATH_IDLE);
            app_tota_ble_disconnected();
        }
        break;
    case BES_BLE_TOTA_RECEVICE_DATA:
        app_tota_handle_received_data(param->param.receive_data.data, param->param.receive_data.data_len);
        break;
    case BES_BLE_TOTA_MTU_UPDATE:
        TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
        if (param->conidx == ble_tota_env.connectionIndex)
        {
            ble_tota_env.mtu = param->param.mtu;
            TRACE(1,"updated data packet size is %d", ble_tota_env.mtu);
        }
        else
        {
            ble_tota_env.mtu = param->param.mtu;
        }
        break;
    case BES_BLE_TOTA_SEND_DONE:
        break;
    default:
        TOTA_LOG_DBG(0,"TOTA ble not find event %x", param->event_type);
        break;
    }
}

uint8_t tota_ble_get_conidx(void)
{
    return ble_tota_env.connectionIndex;
}
static void tota_ble_init(void)
{
    ble_tota_env.connectionIndex =  INVALID_CONNECTION_INDEX;
    ble_tota_env.isNotificationEnabled = false;
    ble_tota_env.mtu = 0;

    bes_ble_tota_event_reg(tota_ble_event_callback);
}

#endif

static void s_app_tota_rx(uint8_t * cmd_buf, uint16_t len)
{
    int ret = 0;
    uint8_t *buf = cmd_buf;

    TOTA_LOG_DBG(0,"Tota rx.");

    //sanity check
    if(buf == NULL)
    {
        TOTA_LOG_DBG(0,"[%s] cmd_buf is null", __func__);
        return;
    }

    ret = tota_rx_queue_push(buf, len);
    ASSERT(ret == 0, "tota rx queue FULL!!!!!!");

    uint16_t queueLen= tota_rx_queue_length();
    while (TOTA_PACKET_VERIFY_SIZE < queueLen)
    {
        TOTA_LOG_DBG(1,"queueLen = 0x%x", queueLen);
        ret = app_tota_rx_unpack(buf, queueLen);
        if (ret)
        {
            break;
        }
        queueLen= tota_rx_queue_length();

        TOTA_LOG_DBG(1,"queueLen = 0x%x",queueLen);
    }
}

static const tota_callback_func_t app_tota_cb = {
    s_app_tota_connected,
    s_app_tota_disconnected,
    s_app_tota_tx_done,
    s_app_tota_rx
};

void app_tota_init(void)
{
    TOTA_LOG_DBG(0, "tota %s", BESLIB_INFO_STR);
    TOTA_LOG_DBG(0, "Init application test over the air.");

#ifdef BT_SPP_SUPPORT
    app_spp_tota_init(&app_tota_cb);
#endif

    // app_tota_cmd_handler_init();
    /* register callback modules */
#ifdef AUDIO_DEBUG_VIA_TOTA
    app_tota_audio_dump_init();
#endif
    /* set module to access spp callback */
    app_tota_callback_module_set(APP_TOTA_AUDIO_DUMP);

    tota_common_init();

#if defined(BLE_TOTA_ENABLED)
    tota_ble_init();
#if defined(OTA_OVER_TOTA_ENABLED)
    ota_ble_adapter_init();
#endif
#endif

#ifdef USB_OTA_ENABLE
    app_tota_usb_init();
#endif

}

void app_tota_handle_received_data(uint8_t* buffer, uint16_t maxBytes)
{
    TOTA_LOG_DBG(2,"[%s]data receive data length = %d",__func__,maxBytes);
    TOTA_LOG_DUMP("[0x%x]",buffer,(maxBytes>20 ? 20 : maxBytes));
    s_app_tota_rx(buffer,maxBytes);
}

void app_tota_ble_disconnected(void)
{
    s_app_tota_disconnected();
}

static bool app_tota_send_via_datapath(uint8_t * pdata, uint16_t dataLen)
{
    dataLen = app_tota_tx_pack(pdata, dataLen);
    if (0 == dataLen)
    {
        return false;
    }
    switch (tota_get_connect_path())
    {
#ifdef BT_SPP_SUPPORT
        case APP_TOTA_VIA_SPP:
            return app_spp_tota_send_data(pdata, dataLen);
#endif
#ifdef BLE_TOTA_ENABLED
        case APP_TOTA_VIA_NOTIFICATION:
            return bes_ble_tota_send_notification(ble_tota_env.connectionIndex, pdata, dataLen);
#endif
#ifdef USB_OTA_ENABLE
        case APP_TOTA_VIA_USB:
            return app_usb_tota_send_data(pdata, dataLen);
#endif
        default:
            return false;
    }
}

bool app_tota_send(uint8_t * pdata, uint16_t dataLen, APP_TOTA_CMD_CODE_E opCode)
{
    if ( opCode == OP_TOTA_NONE )
    {
        TOTA_LOG_DBG(0, "Send pure data");
        /* send pure data */
        return app_tota_send_via_datapath(pdata, dataLen);
    }

    APP_TOTA_CMD_PAYLOAD_T payload;
    /* sanity check: opcode is valid */
    if (opCode >= OP_TOTA_COMMAND_COUNT)
    {
        TOTA_LOG_DBG(0, "Warning: opcode not found");
        return false;
    }
    /* sanity check: data length */
    if (dataLen > sizeof(payload.param))
    {
        TOTA_LOG_DBG(0, "Warning: the length of the data is too lang");
        return false;
    }
    /* sanity check: opcode entry */
    // becase some cmd only for one side
    uint16_t entryIndex = app_tota_cmd_handler_get_entry_index_from_cmd_code(opCode);
    if (INVALID_TOTA_ENTRY_INDEX == entryIndex)
    {
        TOTA_LOG_DBG(0, "Warning: cmd not registered");
        return false;
    }

    payload.cmdCode = opCode;
    payload.paramLen = dataLen;
    memcpy(payload.param, pdata, dataLen);

    /* if is string, direct send */
    if ( opCode == OP_TOTA_STRING )
    {
        return app_tota_send_via_datapath((uint8_t*)&payload, dataLen+4);
    }
#if TOTA_ENCODE
    /* cmd filter */
    if ((TOTA_SHAKE_HANDED  == tota_get_connect_status()) && (opCode > OP_TOTA_CONN_CONFIRM))
    {
        // encode here
        TOTA_LOG_DBG(0, "do encode");
        uint16_t len = tota_encrypt(totaEnv.codeBuf, (uint8_t*)&payload, dataLen+4);
        if (app_tota_send_via_datapath(totaEnv.codeBuf, len))
        {
            APP_TOTA_CMD_INSTANCE_T* pInstance = TOTA_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);
            if (pInstance->isNeedResponse)
            {
                app_tota_cmd_handler_add_waiting_rsp_timeout_supervision(entryIndex);
            }
            return true;
        }
        else
        {
            return false;
        }
    }
#endif
    TOTA_LOG_DBG(0, "send normal cmd");
    if (app_tota_send_via_datapath((uint8_t*)&payload, dataLen+4))
    {
        APP_TOTA_CMD_INSTANCE_T* pInstance = TOTA_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);
        if (pInstance->isNeedResponse)
        {
            app_tota_cmd_handler_add_waiting_rsp_timeout_supervision(entryIndex);
        }
    }

    return true;
}

bool app_tota_send_rsp(APP_TOTA_CMD_CODE_E rsp_opCode, APP_TOTA_CMD_RET_STATUS_E rsp_status, uint8_t * pdata, uint16_t dataLen)
{
    TOTA_LOG_DBG(3,"[%s] opCode=0x%x, status=%d",__func__, rsp_opCode, rsp_status);
    // check responsedCmdCode's validity
    if ( rsp_opCode >= OP_TOTA_COMMAND_COUNT || rsp_opCode < OP_TOTA_STRING)
    {
        return false;
    }
    APP_TOTA_CMD_RSP_T* pResponse = (APP_TOTA_CMD_RSP_T *)(payload.param);

    // check parameter length
    if (dataLen > sizeof(pResponse->rspData))
    {
        return false;
    }
    pResponse->cmdCodeToRsp = rsp_opCode;
    pResponse->cmdRetStatus = rsp_status;
    pResponse->rspDataLen   = dataLen;
    memcpy(pResponse->rspData, pdata, dataLen);

    payload.cmdCode = OP_TOTA_RESPONSE_TO_CMD;
    payload.paramLen = 3*sizeof(uint16_t) + dataLen;

#if TOTA_ENCODE
    uint16_t len = tota_encrypt(totaEnv.codeBuf, (uint8_t*)&payload, payload.paramLen+4);
    return app_tota_send_via_datapath(totaEnv.codeBuf, len);
#else
    return app_tota_send_via_datapath((uint8_t*)&payload, payload.paramLen+4);
#endif
}

bool app_tota_send_data(APP_TOTA_CMD_CODE_E opCode, uint8_t * data, uint32_t dataLen)
{
    uint16_t trans_MTU = tota_get_trans_MTU();
    TOTA_LOG_DBG(1, "trans_MTU:%d",trans_MTU);

    /* sanity check: opcode is valid */
    if (opCode >= OP_TOTA_COMMAND_COUNT)
    {
        TOTA_LOG_DBG(0, "Warning: opcode not found");
        return false;
    }

    /* sanity check: opcode entry */
    // becase some cmd only for one side
    uint16_t entryIndex = app_tota_cmd_handler_get_entry_index_from_cmd_code(opCode);
    if (INVALID_TOTA_ENTRY_INDEX == entryIndex)
    {
        TOTA_LOG_DBG(0, "Warning: cmd not registered");
        return false;
    }

    uint8_t *pData = data;
    uint8_t *sendBuf = (uint8_t *)&payload;
    uint32_t sendBytes = 0;
#ifdef USB_OTA_ENABLE
    uint32_t leftBytes = 0;
    switch (tota_get_connect_path())
    {
        case APP_TOTA_VIA_USB:
            leftBytes = (dataLen>=28)?28:(dataLen%28);
            break;
        default:
            leftBytes = dataLen;
            break;
    }
#else
    uint32_t leftBytes = dataLen;
#endif
    payload.cmdCode = opCode;
    payload.paramLen = dataLen;
    sendBytes = 4;
    leftBytes += 4;
    do
    {
        TOTA_LOG_DBG(2, "leftBytes=%d,sendBytes=%d", leftBytes, sendBytes);

        if (leftBytes <= DONGLE_DATA_FRAME)
        {
            memcpy(sendBuf+sendBytes, pData, (leftBytes-sendBytes));
            pData += (leftBytes-sendBytes);
            sendBytes = leftBytes;
        }
        else
        {
            memcpy(sendBuf+sendBytes, pData, DONGLE_DATA_FRAME-sendBytes);
            pData += DONGLE_DATA_FRAME-sendBytes;
            sendBytes = DONGLE_DATA_FRAME;
        }

        leftBytes -= sendBytes;

        TOTA_LOG_DBG(2, "leftBytes=%d,sendBytes=%d",leftBytes,sendBytes);
#if TOTA_ENCODE
        if (TOTA_SHAKE_HANDED == tota_get_connect_status())
        {
            // encode here
            TOTA_LOG_DBG(0, "do encode");
            sendBytes = tota_encrypt(totaEnv.codeBuf, sendBuf, sendBytes);
            sendBuf = totaEnv.codeBuf;
        }
        else
        {
            return false;
        }
#else
        TOTA_LOG_DBG(0, "send normal cmd");
#endif

        app_tota_send_via_datapath(sendBuf, sendBytes);

        APP_TOTA_CMD_INSTANCE_T* pInstance = TOTA_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);
        if (pInstance->isNeedResponse)
        {
            app_tota_cmd_handler_add_waiting_rsp_timeout_supervision(entryIndex);
        }

        sendBytes = 0;
        sendBuf = (uint8_t *)&payload;

    }while(leftBytes > 0);

    return true;
}


#if defined(OTA_OVER_TOTA_ENABLED)
void app_ota_over_tota_receive_data(uint8_t* ptrParam, uint32_t paramLen)
{
    TOTA_LOG_DBG(1,"[%s] datapath %d", __func__, tota_get_connect_path());
    switch (tota_get_connect_path())
    {
#ifdef BT_SPP_SUPPORT
        case APP_TOTA_VIA_SPP:
            ota_control_handle_received_data(ptrParam, false, paramLen);
            break;
#endif
#ifdef BLE_TOTA_ENABLED
        case APP_TOTA_VIA_NOTIFICATION:
            ota_ble_push_rx_data(BLE_RX_DATA_SELF_TOTA_OTA, tota_ble_get_conidx(), ptrParam, paramLen);
            break;
#endif
#ifdef TOTA_CROSS_CHIP_OTA
        case APP_TOTA_VIA_USB:
        case APP_TOTA_VIA_UART:
            ota_control_handle_received_data(ptrParam, false, paramLen);
            break;
#endif
        default:
            break;
    }
}

void ota_spp_tota_send_data(uint8_t* ptrData, uint32_t length)
{
    app_tota_send_data(OP_TOTA_OTA, ptrData, length);
}
#endif

/*---------------------------------------------------------------------------------------------------------------------------*/

static char strBuf[MAX_SPP_PACKET_SIZE-4];

char *tota_get_strbuf(void)
{
    return strBuf;
}

void tota_printf(const char * format, ...)
{
    va_list vlist;
    va_start(vlist, format);
    vsprintf(strBuf, format, vlist);
    va_end(vlist);
    app_spp_tota_send_data((uint8_t*)strBuf, strlen(strBuf));
}

void tota_print(const char * format, ...)
{
    va_list vlist;
    va_start(vlist, format);
    vsprintf(strBuf, format, vlist);
    va_end(vlist);
    app_tota_send((uint8_t*)strBuf, strlen(strBuf), OP_TOTA_STRING);
}

static void app_tota_demo_cmd_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    switch(funcCode)
    {
        case OP_TOTA_STRING:
            app_bt_cmd_line_handler((char *)ptrParam, paramLen);
            break;
        default:
            break;
    }
}

TOTA_COMMAND_TO_ADD(OP_TOTA_STRING, app_tota_demo_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_DEMO_CMD, app_tota_demo_cmd_handler, false, 0, NULL );

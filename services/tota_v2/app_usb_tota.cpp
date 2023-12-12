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
#include "stdio.h"
#include "app_usb_tota.h"
#include "app_tota.h"
#include "ota_control.h"
#if USB_OTA_ENABLE
#include "usb_audio.h"
#endif
#include "hal_trace.h"
#include "app_tota_common.h"
#include "string.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "cqueue.h"

#define USB_TOTA_LOG_TAG "[USB_TOTA]"

#define USB_TOTA_LOG_DBG(num,str,...)   TRACE(num,USB_TOTA_LOG_TAG"" str, ##__VA_ARGS__)             // DEBUG OUTPUT
#define USB_TOTA_LOG_MSG(num,str,...)   TRACE(num,USB_TOTA_LOG_TAG"" str, ##__VA_ARGS__)             // MESSAGE OUTPUT
#define USB_TOTA_LOG_ERR(num,str,...)   TRACE(num,USB_TOTA_LOG_TAG"err:""" str, ##__VA_ARGS__)       // ERROR OUTPUT

#define USB_TOTA_RX_BUF_SIZE                 2048
#define UAB_TOTA_RX_HANDLE_SIGNAL            0x01
#define USB_TOTA_PACKET_MAX_SIZE             256
#define USB_TOTA_TX_REAL_DATA_MAX_SIZE       28


static uint8_t usb_tota_rx_buf[USB_TOTA_RX_BUF_SIZE];
static CQueue usb_tota_rx_cqueue;
static osThreadId app_usb_tota_rx_thread = NULL;

static void ota_usb_tota_rx_handler_thread(void const *argument);
osThreadDef(ota_usb_tota_rx_handler_thread, osPriorityAboveNormal, 1, 4096, "usb_tota_rx");

void app_tota_send_data_via_usb(uint8_t* ptrData, uint32_t length)
{
    USB_TOTA_LOG_DBG(1,"[%s] send data, data length %d: ", __func__, length);
    //DUMP8("%02x ", ptrData, length);
    uint8_t temp_data_length = length;
    while (length > USB_TOTA_TX_REAL_DATA_MAX_SIZE)
    {
        app_tota_send_data(OP_TOTA_OTA, ptrData, temp_data_length);
        ptrData += USB_TOTA_TX_REAL_DATA_MAX_SIZE;
        length -= USB_TOTA_TX_REAL_DATA_MAX_SIZE;
    }
    osDelay(1);
    app_tota_send_data(OP_TOTA_OTA, ptrData, length);
}

bool app_usb_tota_send_data(uint8_t* ptrData, uint16_t length)
{
    USB_TOTA_LOG_DBG(1, "app_usb_tota_send_data usb tx:%d", length);
    DUMP8("%02x ", ptrData, length>16?16:length);
    uint8_t temp_data[length]={0};
    memcpy(temp_data,ptrData,length);
#if USB_OTA_ENABLE
    hid_epint_in_send_report(temp_data, length);
#endif
    return true;
}

void app_tota_send_cmd_via_usb(uint8_t* ptrData, uint32_t length)
{
    USB_TOTA_LOG_DBG(1,"[%s] send cmd: ", __func__);
    DUMP8("%02x ", ptrData, (length<16?length:16));
#if USB_OTA_ENABLE
    hid_epint_in_send_report(ptrData, length);
#endif
}

POSSIBLY_UNUSED static void tota_usb_rx_handler(uint8_t* data, uint32_t length)
{
    usb_tota_push_rx_data(data, length);
}

void usb_tota_push_rx_data(uint8_t* ptr, uint16_t len)
{
    uint32_t lock = int_lock();
    int32_t ret = EnCQueue(&usb_tota_rx_cqueue, ptr, len);
    int_unlock(lock);
    USB_TOTA_LOG_DBG(2,"%s, %d", __func__, len);
    ASSERT(CQ_OK == ret, "BLE rx buffer overflow! %d,%d",AvailableOfCQueue(&usb_tota_rx_cqueue),len);

    osSignalSet(app_usb_tota_rx_thread, UAB_TOTA_RX_HANDLE_SIGNAL);
}

int usb_tota_rx_queue_peek(uint8_t *buff, uint16_t len)
{
    uint32_t lock = int_lock();
    CQueue *ptrQueue = &usb_tota_rx_cqueue;
    uint8_t *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    PeekCQueue(ptrQueue, len, &e1, &len1, &e2, &len2);
    if (len == (len1 + len2))
    {
        memcpy(buff, e1, len1);
        memcpy(buff + len1, e2, len2);
    }
    else
    {
        memset(buff, 0x00, len);
    }
    int_unlock(lock);
    return len;
}

static void ota_usb_tota_rx_handler_thread(void const *argument)
{
    osEvent evt;
    uint8_t signal = 0;
    while (true)
    {
        uint8_t extractedOtaRequestBuf[USB_TOTA_PACKET_MAX_SIZE];
        evt = osSignalWait(0x0, osWaitForever);
        signal = evt.value.signals;
        if (evt.status == osEventSignal && UAB_TOTA_RX_HANDLE_SIGNAL == signal)
        {
            uint16_t rxqueueLeftDataLen = LengthOfCQueue(&usb_tota_rx_cqueue);

            TOTA_PACKET_VERIFY_T totaPacketVerify;
            USB_TOTA_LOG_DBG(1, "rxqueueLeftDataLen:%d", rxqueueLeftDataLen);
            while (rxqueueLeftDataLen >= TOTA_PACKET_BUF_OFFSET)
            {
                usb_tota_rx_queue_peek((uint8_t *)&totaPacketVerify, TOTA_HEAD_SIZE+TOTA_LEN_SIZE);
                uint16_t payloadLen = totaPacketVerify.bufLen;
                if ((rxqueueLeftDataLen - TOTA_PACKET_VERIFY_SIZE) >= payloadLen)
                {
                    uint32_t lock = int_lock();
                    DeCQueue(&usb_tota_rx_cqueue, extractedOtaRequestBuf, (payloadLen+TOTA_PACKET_VERIFY_SIZE));
                    rxqueueLeftDataLen = LengthOfCQueue(&usb_tota_rx_cqueue);
                    int_unlock(lock);
                    app_tota_handle_received_data(extractedOtaRequestBuf, (payloadLen+TOTA_PACKET_VERIFY_SIZE));

                }
                else
                {
                    USB_TOTA_LOG_ERR(1,"rxqueueLeftDataLen %d !!!!!", rxqueueLeftDataLen);
                    break;
                }
            }
        }
    }
}

void app_tota_usb_init(void)
{
    USB_TOTA_LOG_MSG(1,"[%s], init USB TOTA!!!", __func__);
    app_tota_store_reset();
    tota_set_connect_status(TOTA_CONNECTED);
    tota_set_connect_path(APP_TOTA_VIA_USB);
    InitCQueue(&usb_tota_rx_cqueue, USB_TOTA_RX_BUF_SIZE, ( CQItemType * )usb_tota_rx_buf);
    app_usb_tota_rx_thread = osThreadCreate(osThread(ota_usb_tota_rx_handler_thread), NULL);

    ota_control_register_transmitter(app_tota_send_data_via_usb);
#if USB_OTA_ENABLE
    usb_hid_enint_out_callback_register(tota_usb_rx_handler);
#endif
}


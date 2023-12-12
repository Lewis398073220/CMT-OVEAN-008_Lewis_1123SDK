/***************************************************************************
 * Copyright 2015-2022 BES.
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
 ***************************************************************************/
#ifdef RTOS
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#define USB_MTP_THREAD_MSG_SIZE (32)
static const osMessageQueueAttr_t MessageQueueAttrMtp = {
    .name = "usb_mtp_msg_q",
    .attr_bits = 0U,
    .cb_mem = NULL,
    .cb_size = 0U,
    .mq_mem = NULL,
    .mq_size = 0U,
};
static osMessageQueueId_t  usb_mtp_msg_q_id;

static const osMessageQueueAttr_t MessageQueueAttrWrite = {
    .name = "mtp_write_msg_q",
    .attr_bits = 0U,
    .cb_mem = NULL,
    .cb_size = 0U,
    .mq_mem = NULL,
    .mq_size = 0U,
};
static osMessageQueueId_t  mtp_write_msg_q_id;

void *usb_mtp_msgque_init()
{
    TRACE(0, "[%s] enter 0", __func__);
    usb_mtp_msg_q_id = osMessageQueueNew(USB_MTP_THREAD_MSG_SIZE, sizeof(uint32_t), &MessageQueueAttrMtp);
    TRACE(0, "[%s] enter 1", __func__);
    if (usb_mtp_msg_q_id == NULL) {
        TRACE(0, "[%s] queue is null", __func__);
        return NULL;
    }
    return usb_mtp_msg_q_id;
}

int32_t usb_mtp_msgque_get(void *context)
{
    uint32_t   message;
    osStatus_t status;

    status = osMessageQueueGet(usb_mtp_msg_q_id, &message, NULL, osWaitForever);
    if (status == osOK)
        return (uint32_t)message;
    return -1;
}

int32_t usb_mtp_msgque_put(void *context, uint32_t msginfo)
{
    osMessageQueuePut(usb_mtp_msg_q_id, &msginfo, 0U, 0);
    return 0;
}

void *mtp_write_msgque_init()
{
    mtp_write_msg_q_id = osMessageQueueNew(32, sizeof(uint32_t), &MessageQueueAttrWrite);
    if (mtp_write_msg_q_id == NULL) {
        return NULL;
    }
    return mtp_write_msg_q_id;
}

int32_t mtp_write_msgque_get(void *context)
{
    uint32_t   message;
    osStatus_t status;

    status = osMessageQueueGet(mtp_write_msg_q_id, &message, NULL, osWaitForever);
    if (status == osOK)
        return (uint32_t)message;
    return -1;
}

int32_t mtp_write_msgque_put(void *context, uint32_t msginfo)
{
    osMessageQueuePut(mtp_write_msg_q_id, &msginfo, 0U, 0);
    return 0;
}

#endif



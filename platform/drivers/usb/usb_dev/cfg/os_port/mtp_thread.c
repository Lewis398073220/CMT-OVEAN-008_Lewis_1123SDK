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
#include "plat_types.h"

// --- thread & msg
#define USB_MTP_THREAD_FS_STACK_SIZE (1024*24)
#define USB_MTP_THREAD_DIO_STACK_SIZE (2048*2)

typedef void (*thread_func_t) (void const*argument);

static thread_func_t f_mtp_usb_fs = NULL;
static thread_func_t f_mtp_fs_dio = NULL;

static void usb_mtp_thread_fs(void const *argument);
static void usb_mtp_thread_dio(void const *argument);

static uint64_t os_thread_def_stack_usb_mtp_thread_fs [ROUND_UP(USB_MTP_THREAD_FS_STACK_SIZE, 8) / sizeof(uint64_t)];
static const osThreadAttr_t ThreadAttrFs = {
    .name = "usb_mtp_thread_fs",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0U,
    .stack_mem = os_thread_def_stack_usb_mtp_thread_fs,
    .stack_size = ROUND_UP(USB_MTP_THREAD_FS_STACK_SIZE, 8),
    .priority = osPriorityHigh,
    .tz_module = 1U,                  // indicate calls to secure mode
    .reserved = 0U,
};
static uint64_t os_thread_def_stack_usb_mtp_thread_dio [ROUND_UP(USB_MTP_THREAD_DIO_STACK_SIZE, 8) / sizeof(uint64_t)];
static const osThreadAttr_t ThreadAttrDio = {
    .name = "usb_mtp_thread_dio",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0U,
    .stack_mem = os_thread_def_stack_usb_mtp_thread_dio,
    .stack_size = ROUND_UP(USB_MTP_THREAD_DIO_STACK_SIZE, 8),
    .priority = osPriorityHigh,
    .tz_module = 1U,                  // indicate calls to secure mode
    .reserved = 0U,
};

static void usb_mtp_thread_fs(void const *argument)
{
    if (f_mtp_usb_fs) {
        f_mtp_usb_fs(argument);
    }
}

void *usb_mtp_thread_fs_init(thread_func_t func, void *param)
{
    void *id;
    f_mtp_usb_fs = func;
    id = osThreadNew((osThreadFunc_t)usb_mtp_thread_fs, NULL, &ThreadAttrFs);
    return id;
}

static void usb_mtp_thread_dio(void const *argument)
{
    if (f_mtp_fs_dio) {
        f_mtp_fs_dio(argument);
    }
}

void *usb_mtp_thread_dio_init(thread_func_t func, void *param)
{
    void *id;
    f_mtp_fs_dio = func;
    id = osThreadNew((osThreadFunc_t)usb_mtp_thread_dio, param, &ThreadAttrDio);
    return id;
}

#endif


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

#ifndef _USB_MTP_THREAD_H
#define _USB_MTP_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*thread_func_t) (void const *argument);

/** create a thread task
 *
 *  @param     func: task func
 *  @param   param: task func param
 *
 *  @returns
 *    thread_context_ptr: success   NULL: error
 */
void *usb_mtp_thread_fs_init(thread_func_t func, void *param);

/** create a thread task
 *
 *  @param     func: task func
 *  @param   param: task func param
 *
 *  @returns
 *    thread_context_ptr: success   NULL: error
 */
void *usb_mtp_thread_dio_init(thread_func_t func, void *param);

#define USB_MTP_THREAD_FS_INIT(func, param) usb_mtp_thread_fs_init(func, param)
#define USB_MTP_THREAD_DIO_INIT(func, param) usb_mtp_thread_dio_init(func, param)

#ifdef __cplusplus
}
#endif

#endif


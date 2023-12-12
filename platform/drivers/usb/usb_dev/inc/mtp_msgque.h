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
#ifndef _USB_MTP_MSGQUE_H
#define _USB_MTP_MSGQUE_H

#ifdef __cplusplus
extern "C" {
#endif

/** create a msg queue to put msginfo
 *
 *  @param void
 *
 *  @returns
 *    que_context_ptr: success   NULL: error
 */
void   *usb_mtp_msgque_init();

/** get a msg id from the msg queue
 *
 *  @param que_context: a que_context_ptr by usb_mtp_msgque_init returned
 *
 *  @returns
 *    a msg id defined by USB_MTP_MSG_E
 */
int32_t usb_mtp_msgque_get(void *que_context);

/** put a msg id into the msg queue
 *
 *  @param que_context: a queptr by usb_mtp_msgque_init returned
 *  @param       msginfo: msgid defined by USB_MTP_MSG_E
 *
 *  @returns
 *    0:success   -1: error
 */
int32_t usb_mtp_msgque_put(void *que_context, uint32_t msginfo);

/** create a msg queue to put msginfo
 *
 *  @param void
 *
 *  @returns
 *    que_context_ptr: success   NULL: error
 */
void   *mtp_write_msgque_init();

/** get a msg id from the msg queue
 *
 *  @param que_context: a que_context_ptr by mtp_write_msgque_init returned
 *
 *  @returns
 *    a msg id defined by USB_MTP_MSG_E
 */
int32_t mtp_write_msgque_get(void *que_context);

/** put a msg id into the msg queue
 *
 *  @param que_context: a queptr by mtp_write_msgque_init returned
 *  @param       msginfo: uint32_t type
 *
 *  @returns
 *    0:success   -1: error
 */
int32_t mtp_write_msgque_put(void *que_context, uint32_t msginfo);

#define USB_MTP_MSGQUE_INIT()                 usb_mtp_msgque_init()
#define USB_MTP_MSGQUE_GET(context)           usb_mtp_msgque_get(context);
#define USB_MTP_MSGQUE_PUT(context, msginfo)  usb_mtp_msgque_put(context, msginfo);
#define MTP_WRITE_MSGQUE_INIT()                 mtp_write_msgque_init()
#define MTP_WRITE_MSGQUE_GET(context)           mtp_write_msgque_get(context);
#define MTP_WRITE_MSGQUE_PUT(context, msginfo)  mtp_write_msgque_put(context, msginfo);

#ifdef __cplusplus
}
#endif

#endif



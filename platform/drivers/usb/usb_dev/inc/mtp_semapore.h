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
#ifndef MTP_SEMAPHORE_H
#define MTP_SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

/** semaphore create
 *
 *  @param mutex : NULL
 *
 *  @returns semaphore context ptr
 */
void   *mtp_semaphore_create();

/** semaphore wait
 *
 *  @param context : ptr returned by mtp_semaphore_create
 *
 *  @returns: 0 success -1 error
 */
int32_t mtp_semaphore_wait(void *context);

/** semaphore post
 *
 *  @param context : ptr returned by mtp_semaphore_create
 *
 *  @returns: 0 success -1 error
 */
int32_t mtp_semaphore_post(void *context);

#define MTP_SEMAPHORE_CREATE()      mtp_semaphore_create()
#define MTP_SEMAPHORE_WAIT(context) mtp_semaphore_wait(context)
#define MTP_SEMAPHORE_POST(context) mtp_semaphore_post(context)

#ifdef __cplusplus
}
#endif

#endif


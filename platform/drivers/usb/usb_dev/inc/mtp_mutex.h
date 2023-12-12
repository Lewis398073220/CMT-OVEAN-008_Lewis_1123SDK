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
#ifndef MTP_STORAGE_MUTEX_H
#define MTP_STORAGE_MUTEX_H

#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

/** create mutex, success to return a mutex ptr
 *
 *  @param void
 *
 *  @returns
 *    ptr: success  NULL: error
 */
void *mtp_storage_mutex_create();

/** mutex lock when enable is true
 *
 *  @param enable : mutex eanble
 *  @param mutex : the mutex ptr returned by mtp_storage_mutex_create
 *
 *  @returns
 */
void  mtp_storage_mutex_lock(bool enable, void *mutex);

/** mutex unlock when enable is true
 *
 *  @param enable : mutex eanble
 *  @param mutex : mtp_storage_mutex_create returned the mutex ptr
 *
 *  @returns
 */
void  mtp_storage_mutex_unlock(bool enable, void *mutex);

/** create mutex, success to return a mutex ptr
 *
 *  @param void
 *
 *  @returns
 *    ptr: success  NULL: error
 */
void *mtp_write_mutex_create();

/** mutex lock
 *
 *  @param mutex : the mutex ptr returned by mtp_write_mutex_create
 *
 *  @returns
 */
void  mtp_write_mutex_lock(void *mutex);

/** mutex unlock
 *
 *  @param mutex : mtp_write_mutex_create returned the mutex ptr
 *
 *  @returns
 */
void  mtp_write_mutex_unlock(void *mutex);

#define MTP_LOCK(enable, mutex)          mtp_storage_mutex_lock(enable, mutex)
#define MTP_UNLOCK(enable, mutex)        mtp_storage_mutex_unlock(enable, mutex)
#define MTP_LOCK_INIT()                  mtp_storage_mutex_create();

#define MTP_WRITE_LOCK_INIT()            mtp_write_mutex_create();
#define MTP_WRITE_LOCK(mutex)            mtp_write_mutex_lock(mutex)
#define MTP_WRITE_UNLOCK(mutex)          mtp_write_mutex_unlock(mutex)

#ifdef __cplusplus
}
#endif

#endif



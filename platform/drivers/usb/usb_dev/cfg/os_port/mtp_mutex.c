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
#include "cmsis_os.h"
#include "stdbool.h"

static const osMutexAttr_t MutexAttrStorage = {
    .name = "mtp_storage_mutex",
    .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
    .cb_mem = NULL,
    .cb_size = 0U,
};
static const osMutexAttr_t MutexAttrWrite = {
    .name = "mtp_write_mutex",
    .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
    .cb_mem = NULL,
    .cb_size = 0U,
};

void *mtp_storage_mutex_create()
{
    return osMutexNew(&MutexAttrStorage);
}

void mtp_storage_mutex_lock(bool enable, void *mutex)
{
    if (enable == true)
        osMutexAcquire((osMutexId_t)mutex, osWaitForever);
}

void mtp_storage_mutex_unlock(bool enable, void *mutex)
{
    if (enable == true)
        osMutexRelease((osMutexId_t)mutex);
}

void *mtp_write_mutex_create()
{
    return osMutexNew(&MutexAttrWrite);
}

void mtp_write_mutex_lock(void *mutex)
{
    osMutexAcquire((osMutexId_t)mutex, osWaitForever);
}

void mtp_write_mutex_unlock(void *mutex)
{
    osMutexRelease((osMutexId_t)mutex);
}
#endif

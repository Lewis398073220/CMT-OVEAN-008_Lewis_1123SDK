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
#include "cmsis.h"

#define MTP_THREAD_DONE_TIMEOUT  (5000)
static osSemaphoreId_t mtp_thread_done_signal_id = NULL;

void *mtp_semaphore_create()
{
    mtp_thread_done_signal_id = osSemaphoreNew(65535U, (uint32_t)0, NULL);
    return mtp_thread_done_signal_id;
}

int32_t mtp_semaphore_wait(void *context)
{
    int ret = osOK;
    ret = osSemaphoreAcquire(context, MTP_THREAD_DONE_TIMEOUT);
    if(ret != osOK) {
        return -1;
    }
    return 0;
}

int32_t mtp_semaphore_post(void *context)
{
    osSemaphoreRelease(context);
    return 0;
}
#endif


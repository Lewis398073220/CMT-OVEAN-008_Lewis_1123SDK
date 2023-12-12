/***************************************************************************
 *
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
 *
 ****************************************************************************/
/**
 * DOC - gra wakelock
 *
 * gra wakelock is a timer, grahic driver uses it to keep
 * system in right power state for certern time
 */

#include "gra_wakelock.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <hal_timer.h>
#include <hal_trace.h>
#include "hal_sysfreq.h"
#include "hal_sleep.h"

extern void *gpu_malloc(uint32_t size);

#define  gra_malloc  gpu_malloc
typedef void (*gra_timerhandler) (void const *argument);

typedef struct {
    uint32_t name[5];
    osTimerDef_t os_timer_def;
    void * timer_id;
}gra_wakelock_t;


/**
 * gra_wakelock_create - create gra wakelock
 *
 * @ cb : timeout call back, with args
 * @ args : args with timeout call back func
 * @ name : gra wakelock name (length < 5)
 *
 * Returns: handler of gra wakelock .
 *
 */

static uint8_t gpu_wakelock_buf[128];
void* gra_wakelock_create(void* cb, void *args, char* name)
{
    gra_wakelock_t *lock;
    void * timer_id;
    int ret = 0;
    lock = gpu_wakelock_buf;
    if (NULL == lock) {
        TRACE(3, "%s no mem for gra wakelock", __func__ );
        return NULL;
    }
    memcpy(lock->name, name, 5);
    lock->os_timer_def.ptimer = (gra_timerhandler)cb;
    //lock->os_timer_def.timer = lock->name;
    timer_id = osTimerCreate(&lock->os_timer_def, osTimerOnce, args);
    return timer_id;
}


/**
 * gra_wakelock_timeout - setup an active gra wakelock with timeout
 *
 * @ gralock : Semaphore to be initialized
 * @ ms : timeout value (in ms)
 *
 * Returns: NONE.
 * NOTE: Not allowed in ISR
 */

void gra_wakelock_timeout(void *gralock,uint32_t ms)
{
    //TRACE(0, "%s", __func__);
    osTimerId timer_id = (osTimerId)gralock;
    if (timer_id != NULL) {
        osTimerStart(timer_id, ms);
    }else{
        TRACE(3,"%s invalid lock id: %p",__func__,timer_id );
    }

}

static void* gra_wakelock = NULL;

static char *wakelockname = "graw";

/**
 * gra_enter_sleep - free sysfreq hold
 *
 */
static void gra_enter_sleep(void)
{
    //TRACE(3,"%s <",__func__);
    hal_sysfreq_req(HAL_SYSFREQ_USER_APP_1, HAL_CMU_FREQ_32K);
    //TRACE(3,"%s >",__func__);

}


void gra_wakelock_handler(void *argument)
{
    gra_enter_sleep( );
}

static void gra_open_wakelock(void)
{
    gra_wakelock = gra_wakelock_create(
                                          gra_wakelock_handler,
                                          NULL,
                                          wakelockname);
}


/**
 * gra_exit_sleep - keep system active for required time
 *
 */
void gra_exit_sleep(uint16_t active_time)
{
    //TRACE(3,"%s <",__func__);
    if (gra_wakelock == NULL) {
        gra_open_wakelock();
    }
    //hal_sysfreq_req(HAL_SYSFREQ_USER_APP_1, HAL_CMU_FREQ_104M);
    if (gra_wakelock != NULL) {
        hal_sysfreq_req(HAL_SYSFREQ_USER_APP_1, HAL_CMU_FREQ_208M);
        gra_wakelock_timeout(gra_wakelock, active_time);
    }
    //TRACE(3,"%s >",__func__);
}



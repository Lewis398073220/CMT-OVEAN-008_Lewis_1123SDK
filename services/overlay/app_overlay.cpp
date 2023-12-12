/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#include "cmsis_os.h"
#include "app_overlay.h"
#include "hal_trace.h"
#include "hal_location.h"

osMutexDef(app_overlay_mutex);

static osMutexId app_overlay_mutex_id = NULL;
static APP_OVERLAY_ID_T app_overlay_id = APP_OVERLAY_ID_QTY;

APP_OVERLAY_ID_T app_get_current_overlay(void)
{
    return app_overlay_id;
}

void app_overlay_select(enum APP_OVERLAY_ID_T id)
{
#if !defined(FPGA) || defined(FPGA_BUILD_IN_FLASH)
    TRACE(3,"%s id:%d->%d", __func__, app_overlay_id, id);

    osMutexWait(app_overlay_mutex_id, osWaitForever);
    if (app_overlay_id == APP_OVERLAY_ID_QTY) {
        app_overlay_load(id);
    } else {
#ifndef CORE_SLEEP_POWER_DOWN
        if (app_overlay_id != id)
#endif
        {
            app_overlay_unload(app_overlay_id);
            app_overlay_load(id);
        }
    }
    app_overlay_id = id;
    osMutexRelease(app_overlay_mutex_id);
#endif
}

void app_overlay_unloadall(void)
{
    osMutexWait(app_overlay_mutex_id, osWaitForever);
    if (app_overlay_id != APP_OVERLAY_ID_QTY){
        app_overlay_unload(app_overlay_id);
    }
    app_overlay_id = APP_OVERLAY_ID_QTY;
    osMutexRelease(app_overlay_mutex_id);
}

void app_overlay_open(void)
{
    if (app_overlay_mutex_id == NULL) {
        app_overlay_mutex_id = osMutexCreate(osMutex(app_overlay_mutex));
    }
}

void app_overlay_close(void)
{
    app_overlay_unloadall();
    if (app_overlay_mutex_id != NULL) {
        osMutexDelete(app_overlay_mutex_id);
        app_overlay_mutex_id = NULL;
    }
}

osMutexDef(app_overlay_subsys_mutex);
static osMutexId app_overlay_subsys_mutex_id = NULL;
static APP_OVERLAY_SUBSYS_ID_T app_overlay_subsys_id[APP_OVERLAY_SUBSYS_QTY][1] = {APP_OVERLAY_SUBSYS_ID_QTY};

APP_OVERLAY_SUBSYS_ID_T app_get_current_overlay_subsys(enum APP_OVERLAY_SUBSYS_T subsys)
{
    return app_overlay_subsys_id[subsys][0];
}

void app_overlay_subsys_select(enum APP_OVERLAY_SUBSYS_T subsys, enum APP_OVERLAY_SUBSYS_ID_T id)
{
#if !defined(FPGA) || defined(FPGA_BUILD_IN_FLASH)
    ASSERT(subsys < APP_OVERLAY_SUBSYS_QTY, "%s, subsys error:%d", __func__, subsys);

    osMutexWait(app_overlay_subsys_mutex_id, osWaitForever);
    APP_OVERLAY_SUBSYS_ID_T idOld = app_overlay_subsys_id[subsys][0];

    TRACE(3, "%s,subsys:%d, id:%d->%d", __func__, subsys, idOld, id);

    if (idOld == APP_OVERLAY_SUBSYS_ID_QTY) {
        app_overlay_subsys_load(subsys, id);
    } else {
#ifndef CORE_SLEEP_POWER_DOWN
        if (idOld != id)
#endif
        {
            app_overlay_subsys_unload(subsys, idOld);
            app_overlay_subsys_load(subsys, id);
        }
    }
    app_overlay_subsys_id[subsys][0] = id;
    osMutexRelease(app_overlay_subsys_mutex_id);
#endif
}

void app_overlay_subsys_unloadall(enum APP_OVERLAY_SUBSYS_T subsys)
{
    osMutexWait(app_overlay_subsys_mutex_id, osWaitForever);

    APP_OVERLAY_SUBSYS_ID_T overlay_id = app_overlay_subsys_id[subsys][0];

    if (overlay_id != APP_OVERLAY_SUBSYS_ID_QTY) {
        app_overlay_subsys_unload(subsys, overlay_id);
    }
    app_overlay_subsys_id[subsys][0] = APP_OVERLAY_SUBSYS_ID_QTY;
    osMutexRelease(app_overlay_subsys_mutex_id);
}

void app_overlay_subsys_open(void)
{
    // overlay section will be loaded as the first index during m55 boot-up
    app_overlay_subsys_id[APP_OVERLAY_M55][0] = APP_OVERLAY_SUBSYS_ID_QTY;
    if (app_overlay_subsys_mutex_id == NULL) {
        app_overlay_subsys_mutex_id = osMutexCreate(osMutex(app_overlay_subsys_mutex));
    }
}

void app_overlay_subsys_close(enum APP_OVERLAY_SUBSYS_T subsys)
{
    app_overlay_subsys_unloadall(subsys);
    if (app_overlay_subsys_mutex_id != NULL) {
        osMutexDelete(app_overlay_subsys_mutex_id);
        app_overlay_subsys_mutex_id = NULL;
    }
}


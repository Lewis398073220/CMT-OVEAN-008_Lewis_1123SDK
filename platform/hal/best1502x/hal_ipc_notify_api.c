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
#ifdef __HAL_IPC_NOTIFY_SUPPORT__
#include "hal_ipc_notify_api.h"
#include "hal_ipc_notify.h"
#include "hal_trace.h"
#include "hal_mcu2btc.h"
#include "hal_mcu2sens.h"
#include "hal_mcu2dsp.h"

int hal_ipc_notify_interrupt_core(enum HAL_IPC_NOTIFY_CORE_TYPE_T core, enum HAL_IPC_NOTIFY_SUB_ID_T id)
{
    if (core >= HAL_IPC_NOTIFY_CORE_QTY) {
        return 1;
    }

    if (id >= HAL_IPC_NOTIFY_SUB_ID_QTY) {
        return 1;
    }

    switch(core)
    {
        case HAL_IPC_NOTIFY_CORE_SYS_SENS:
            hal_mcu2sens_ipc_notify_interrupt_core(id);
            break;
        default:
            ASSERT(0, "%s Wrong core used:%d",__func__,core);
            break;
    }
    return 0;
}


int hal_ipc_notify_open(enum HAL_IPC_NOTIFY_CORE_TYPE_T core, enum HAL_IPC_NOTIFY_SUB_ID_T id, struct HAL_IPC_NOTIFY_IRQ_CB_T irq_cb)
{
    TRACE_HAL_IPC_NOTIFY_I("%s core:%d id:%d", __func__, core, id);

    if (core >= HAL_IPC_NOTIFY_CORE_QTY) {
        return 1;
    }

    if (id >= HAL_IPC_NOTIFY_SUB_ID_QTY) {
        return 1;
    }

    switch(core)
    {
        case HAL_IPC_NOTIFY_CORE_SYS_SENS:
            hal_mcu2sens_ipc_notify_open(id, irq_cb.rx_handler, irq_cb.tx_handler);
            hal_mcu2sens_ipc_notify_start_recv(id);
            break;
        default:
            ASSERT(0, "%s Wrong core used:%d",__func__,core);
            break;
    }

    return 0;
}


int hal_ipc_notify_close(enum HAL_IPC_NOTIFY_CORE_TYPE_T core, enum HAL_IPC_NOTIFY_SUB_ID_T id)
{
    TRACE_HAL_IPC_NOTIFY_I("%s core:%d id:%d", __func__, core, id);

    if (core >= HAL_IPC_NOTIFY_CORE_QTY) {
        return 1;
    }

    if (id >= HAL_IPC_NOTIFY_SUB_ID_QTY) {
        return 1;
    }

    switch(core)
    {
        case HAL_IPC_NOTIFY_CORE_SYS_SENS:
            hal_mcu2sens_ipc_notify_close(id);
            break;
        default:
            ASSERT(0, "%s Wrong core used:%d",__func__,core);
            break;
    }

    return 0;
}
#endif

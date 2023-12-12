/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#include "cmsis.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "mcpp_cmd.h"
#include "app_mcpp_comm.h"
#include "app_mcpp_cfg.h"

APP_MCPP_CORE_T g_mcpp_core = APP_MCPP_CORE_CP_SUBSYS;

#ifndef RTOS
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
#include "cp_subsys.h"
static int mcpp_bth_cp_subsys_tx_data(const void *data, uint32_t len)
{
    return cp_subsys_send(data, len);
}
#endif

bool mcpp_target_core_is_baremetal(APP_MCPP_CORE_T tar_core)
{
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    if (tar_core == APP_MCPP_CORE_BTH){
        return true;
    } else
#endif
    {
        ASSERT(0, "[%s]:target_core:%d is error", __func__, tar_core);
    }
    return false;
}

int32_t mcpp_core_baremetal_send_data(APP_MCPP_CORE_T tar_core, void *data, uint32_t len)
{
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
    if (tar_core == APP_MCPP_CORE_BTH){
        mcpp_bth_cp_subsys_tx_data(data, len);
    } else
#endif
    {
        ASSERT(0, "[%s]:target_core:%d is error", __func__, tar_core);
    }
    return 0;
}
#endif
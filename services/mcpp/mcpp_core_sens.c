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
#include "cmsis_os.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "app_rpc_api.h"
#include "mcpp_cmd.h"
#include "app_mcpp_comm.h"
#include "app_mcpp_cfg.h"

APP_MCPP_CORE_T g_mcpp_core = APP_MCPP_CORE_SENS;

extern const MCPP_CMD_CFG_T *g_mcpp_cmd_cfg[APP_MCPP_CORE_QTY];

#ifdef APP_MCPP_BTH_SENS_ENABLE
#include "app_sensor_hub.h"
/*****************************SENS to BTH*****************************/
static const MCPP_CMD_CFG_T g_mcpp_cmd_bth_cfg = {
    .core        = APP_RPC_CORE_MCU_SENSOR,

    .rsp_cmd     = BTH_SENS_TASK_CMD_MCPP_RSP,
    .no_rsp_cmd  = BTH_SENS_TASK_CMD_MCPP_NO_RSP,
};
#endif

void mcpp_load_core_cmd_cfg(void)
{
#ifdef APP_MCPP_BTH_SENS_ENABLE
    g_mcpp_cmd_cfg[APP_MCPP_CORE_BTH] = &g_mcpp_cmd_bth_cfg;
#endif
}

bool mcpp_target_core_is_baremetal(APP_MCPP_CORE_T server)
{
    return false;
}

int32_t mcpp_core_baremetal_send_data(APP_MCPP_CORE_T server, void *data, uint32_t len)
{
    return 0;
}
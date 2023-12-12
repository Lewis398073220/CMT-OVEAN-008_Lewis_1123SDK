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
#include "cmsis.h"
#include "cmsis_os.h"
#include "plat_types.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"

#include "app_rpc_api.h"

static void demo_no_rsp_cmd_transmit_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "%s: len=%d", ptr, len);
    app_rpc_send_data_no_rsp(APP_RPC_CORE_BTH_DSP, BTH_DSP_TASK_CMD_DEMO_REQ_NO_RSP, ptr, len);
}

static void demo_no_rsp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "%s: len=%d", ptr, len);
}

static void demo_no_rsp_cmd_tx_done_handler(uint16_t cmdCode, uint8_t* ptr, uint16_t len)
{
    TRACE(0, "%s: cmdCode 0x%x tx done", __func__, cmdCode);
}

RPC_BTH_DSP_TASK_CMD_TO_ADD(BTH_DSP_TASK_CMD_DEMO_REQ_NO_RSP,
                            "BTH2DSP_CMD_DEMO_REQ_NO_RSP",
                            demo_no_rsp_cmd_transmit_handler,
                            demo_no_rsp_cmd_received_handler,
                            0,
                            NULL,
                            NULL,
                            demo_no_rsp_cmd_tx_done_handler);
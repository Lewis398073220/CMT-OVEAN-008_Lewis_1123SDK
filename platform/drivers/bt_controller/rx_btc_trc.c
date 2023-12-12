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
#ifdef BTC_TRC_TO_SYS

#include "plat_types.h"
//#include "hal_sys2bth.h"
#include "hal_trace.h"
#include "rmt_trace_server.h"
#include "string.h"

#if 0
static struct RMT_TRC_SERVER_CFG_T server_cfg;
static const char *trc_name = "BTC";

static unsigned int msg_test_rx_handler(const void *data, unsigned int len)
{
    return rmt_trace_server_msg_handler(&server_cfg, data, len);
}

void rx_bt_controller_trace(void)
{
    int ret;

    memset(&server_cfg, 0, sizeof(server_cfg));
    server_cfg.name = trc_name;
    server_cfg.first_msg = true;

    ret = hal_sys2bth_open(HAL_SYS2BTH_ID_1, msg_test_rx_handler, NULL, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(HAL_SYS2BTH_ID_1);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);

    TRACE(0, "Start to rx bt_host trace");
}
#else
void rx_bt_controller_trace(void)
{
    TRACE(0, "Start to rx bt_host trace");
}
#endif
#endif

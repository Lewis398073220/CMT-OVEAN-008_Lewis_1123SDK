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
#ifdef DSP_M55_TRC_TO_MCU

#include "plat_types.h"
#include "hal_sys2bth.h"
#include "hal_trace.h"
#include "rmt_trace_msg.h"
#include "rmt_trace_server.h"
#include "string.h"

#ifndef RMT_TRC_IN_MSG_CHAN
#error "RMT_TRC_IN_MSG_CHAN should be used with DSP_M55_TRC_TO_MCU"
#endif

static struct RMT_TRC_SERVER_CFG_T server_cfg;
static const char *trc_name = "DSP_M55";

#ifdef RMT_TRC_IN_MSG_CHAN
#define RMT_TRC_CHAN_ID                     HAL_SYS2BTH_ID_0
#else
#define RMT_TRC_CHAN_ID                     HAL_SYS2BTH_ID_1
#endif

static int dsp_m55_trace_send(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_sys2bth_send_seq(RMT_TRC_CHAN_ID, data, len, seq);
}

static void dsp_m55_trace_server_dump(void)
{
    rmt_trace_server_dump(&server_cfg);
}

unsigned int dsp_m55_trace_rx_handler(const void *data, unsigned int len)
{
    return rmt_trace_server_msg_handler(&server_cfg, data, len);
}

void dsp_m55_trace_server_open(void)
{
    memset(&server_cfg, 0, sizeof(server_cfg));
    server_cfg.name = trc_name;
    server_cfg.first_msg = true;
    server_cfg.send_cb = dsp_m55_trace_send;
#ifdef RMT_TRC_IN_MSG_CHAN
    server_cfg.in_msg_chan = true;
#else
    int ret;

    ret = hal_sys2bth_open(RMT_TRC_CHAN_ID, dsp_m55_trace_rx_handler, NULL, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(RMT_TRC_CHAN_ID);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);
#endif

    rmt_trace_server_dump_register(dsp_m55_trace_server_dump);

    TRACE(0, "Start to rx dsp_m55 trace");
}

void dsp_m55_trace_server_close(void)
{
    rmt_trace_server_dump_deregister(dsp_m55_trace_server_dump);
#ifndef RMT_TRC_IN_MSG_CHAN
    hal_sys2bth_stop_recv(RMT_TRC_CHAN_ID);
    hal_sys2bth_close(RMT_TRC_CHAN_ID);
#endif

    TRACE(0, "Stop rx dsp_m55 trace");
}

#endif

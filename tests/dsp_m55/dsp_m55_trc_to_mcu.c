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
#include "rmt_trace_client.h"

#ifndef RMT_TRC_IN_MSG_CHAN
#error "RMT_TRC_IN_MSG_CHAN should be used with DSP_M55_TRC_TO_MCU"
#endif

#ifdef RMT_TRC_IN_MSG_CHAN
#define RMT_TRC_CHAN_ID                     HAL_SYS2BTH_ID_0
#else
#define RMT_TRC_CHAN_ID                     HAL_SYS2BTH_ID_1
#endif

static int dsp_m55_trc_send(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_sys2bth_send_seq(RMT_TRC_CHAN_ID, data, len, seq);
}

static int dsp_m55_trc_tx_active(unsigned int seq)
{
    return hal_sys2bth_tx_active(RMT_TRC_CHAN_ID, seq);
}

static void dsp_m55_trc_tx_irq_run(void)
{
    hal_sys2bth_tx_irq_run(RMT_TRC_CHAN_ID);
}

static const char dsp_m55_trc_tag[] = "DSPM/";

static const struct RMT_TRC_CLIENT_CFG_T dsp_m55_cfg = {
    .tag = dsp_m55_trc_tag,
    .send_cb = dsp_m55_trc_send,
    .tx_active_cb = dsp_m55_trc_tx_active,
    .tx_irq_run_cb = dsp_m55_trc_tx_irq_run,
};

void dsp_m55_trace_to_mcu(void)
{
    int ret;

#ifndef RMT_TRC_IN_MSG_CHAN
    ret = hal_sys2bth_open(RMT_TRC_CHAN_ID, rmt_trace_client_msg_handler, NULL, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(RMT_TRC_CHAN_ID);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);
#endif

    ret = rmt_trace_client_open(&dsp_m55_cfg);
    ASSERT(ret == 0, "rmt_trace_client_open failed: %d", ret);
}

#endif

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
#if !(defined(CHIP_ROLE_CP) || defined(CHIP_SUBSYS_SENS) || (defined(CHIP_SUBSYS_BTH) ^ defined(BTH_AS_MAIN_MCU)))
#ifdef DSP_HIFI4_TRC_TO_MCU

#include "plat_types.h"
#include "rx_dsp_hifi4_trc.h"
#ifdef BTH_AS_MAIN_MCU
#include "hal_sys2bth.h"
#else
#include "hal_mcu2dsp.h"
#endif
#include "hal_trace.h"
#include "rmt_trace_server.h"
#include "string.h"

#ifdef BTH_AS_MAIN_MCU
#ifndef RMT_TRC_IN_MSG_CHAN
#error "RMT_TRC_IN_MSG_CHAN should be used with DSP_HIFI4_TRC_TO_MCU when BTH_AS_MAIN_MCU=1"
#endif
#endif

static struct RMT_TRC_SERVER_CFG_T server_cfg;
static const char *trc_name = "DSP_HIFI4";

unsigned int dsp_hifi4_trace_rx_handler(const void *data, unsigned int len)
{
    return rmt_trace_server_msg_handler(&server_cfg, data, len);
}

void dsp_hifi4_trace_server_open(void)
{
    memset(&server_cfg, 0, sizeof(server_cfg));
    server_cfg.name = trc_name;
    server_cfg.first_msg = true;
#ifdef RMT_TRC_IN_MSG_CHAN
    server_cfg.in_msg_chan = true;
#else
    int ret;

#ifdef BTH_AS_MAIN_MCU
    ret = hal_sys2bth_open(HAL_SYS2BTH_ID_1, dsp_hifi4_trace_rx_handler, NULL, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(HAL_SYS2BTH_ID_1);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);
#else
    ret = hal_mcu2dsp_open(HAL_MCU2DSP_ID_1, dsp_hifi4_trace_rx_handler, NULL, false);
    ASSERT(ret == 0, "hal_mcu2dsp_open failed: %d", ret);

    ret = hal_mcu2dsp_start_recv(HAL_MCU2DSP_ID_1);
    ASSERT(ret == 0, "hal_mcu2dsp_start_recv failed: %d", ret);
#endif
#endif

    TRACE(0, "Start to rx dsp_hifi4 trace");
}

void dsp_hifi4_trace_server_close(void)
{
#ifndef RMT_TRC_IN_MSG_CHAN
#ifdef BTH_AS_MAIN_MCU
    hal_sys2bth_stop_recv(HAL_SYS2BTH_ID_1);
    hal_sys2bth_close(HAL_SYS2BTH_ID_1);
#else
    hal_mcu2dsp_stop_recv(HAL_MCU2DSP_ID_1);
    hal_mcu2dsp_close(HAL_MCU2DSP_ID_1);
#endif
#endif

    TRACE(0, "Stop rx dsp_hifi4 trace");
}

#endif
#endif

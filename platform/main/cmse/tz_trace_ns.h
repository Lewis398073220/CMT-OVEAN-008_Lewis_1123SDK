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
#ifndef __TZ_TRACE_NS_H__
#define __TZ_TRACE_NS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_trace.h"

int cmse_register_ns_trace_callback(uint8_t *buf, uint32_t len, HAL_TRACE_APP_NOTIFY_T notif_cb, HAL_TRACE_APP_OUTPUT_T out_cb);

void cmns_trace_init();

#ifdef __cplusplus
}
#endif

#endif

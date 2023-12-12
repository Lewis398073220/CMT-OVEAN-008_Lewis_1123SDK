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
#ifndef __MCPP_CORE_H__
#define __MCPP_CORE_H__

#include "app_mcpp_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern APP_MCPP_CORE_T g_mcpp_core;

void mcpp_load_core_cmd_cfg(void);

bool server_core_is_running(APP_MCPP_CORE_T server);
bool server_core_is_opened(APP_MCPP_CORE_T server);
int32_t mcpp_core_open(APP_MCPP_CORE_T server);
int32_t mcpp_core_close(APP_MCPP_CORE_T server);

bool mcpp_target_core_is_baremetal(APP_MCPP_CORE_T server);
int32_t mcpp_core_baremetal_send_data(APP_MCPP_CORE_T server, void *data, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif
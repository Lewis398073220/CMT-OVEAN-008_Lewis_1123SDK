/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#ifndef _APP_HAP_H_
#define _APP_HAP_H_

#include "hap.h"

#include "app_task.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

// /// HAP environment struct
// struct app_hap_env
// {
//     /// Status bit field
//     uint8_t status_bf;
// } app_hap_env_t;

#ifdef AOB_MOBILE_ENABLED
void app_hap_client_init(void);
#endif

void app_hap_server_init(void);

void app_hap_msg_configure_req(uint8_t cfg_bf);

void app_hap_start(uint8_t con_lid);

uint32_t app_hap_cmp_evt_handler(void const *param);

uint32_t app_hap_rsp_handler(void const *param);

uint32_t app_hap_ind_handler(void const *param);


#ifdef __cplusplus
}
#endif
#endif
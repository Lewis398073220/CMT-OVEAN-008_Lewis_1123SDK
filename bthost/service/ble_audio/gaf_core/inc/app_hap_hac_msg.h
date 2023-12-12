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
#ifndef _APP_HAP_HAC_MSG_H_
#define _APP_HAP_HAC_MSG_H_

#include "app_hap_hac_msg.h"
#include "app_task.h"
#include "string.h"

#ifdef AOB_MOBILE_ENABLED
uint32_t app_hap_hac_msg_rsp_handler(void const *param);

uint32_t app_hap_hac_msg_cmp_evt_handler(void const *param);

uint32_t app_hap_hac_ind_handler(void const *param);

void app_hap_hac_init(void);

void app_hap_hac_start(uint8_t con_lid);

#endif
#endif
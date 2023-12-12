/**
 * @file app_ancs.c
 * @author BES AI team
 * @version 0.1
 * @date 2021-01-03
 * 
 * @copyright Copyright (c) 2015-2021 BES Technic.
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
 */

/*****************************header include********************************/
#include "ke_msg.h"
#include "gapm_msg.h"
#include "gatt.h"
#include "app_ancs.h"

/*********************external function declearation************************/

/************************private macro defination***************************/

/************************private type defination****************************/

/************************extern function declearation***********************/

/**********************private function declearation************************/

/************************private variable defination************************/

/****************************function defination****************************/
void app_ancs_add_svc(void)
{
    TRACE(0, "Registering ANCS Proxy GATT Service");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM,
                         TASK_APP,
                         gapm_profile_task_add_cmd,
                         0);

    req->operation  = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl    = (uint8_t)(SVC_SEC_LVL(SECURE_CON) | SVC_UUID(128));
    req->user_prio  = 0;
    req->prf_api_id = TASK_ID_ANCSP;
    req->start_hdl  = 0;

    ke_msg_send(req);
}
/****************************************************************************************
 *
 * Copyright 2015-2021 BES.
 *
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
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP

 * @{
 ****************************************************************************************
 */

#ifndef _APP_IAP_TM_MSG_H_
#define _APP_IAP_TM_MSG_H_

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/

/******************************type defination******************************/

/****************************function declaration***************************/
uint32_t app_iap_tm_cmp_evt_handler(void const *param);

uint32_t app_iap_tm_ind_handler(void const *param);

void app_iap_tm_init(void);

#ifdef __cplusplus
}
#endif
#endif
/// @} APP_GAF_IAP_TM_MSG.H

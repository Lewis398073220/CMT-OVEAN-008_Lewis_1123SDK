/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
/**
 ****************************************************************************************
 * @addtogroup APP_ACC

 * @{
 ****************************************************************************************
 */

#ifndef APP_ACC_MCC_MSG_H_
#define APP_ACC_MCC_MSG_H_
#if BLE_AUDIO_ENABLED
#include "app_acc_mcs_msg.h"
#include "app_gaf_define.h"

#define APP_ACC_MCC_MAX_SUPP_MCS_INST   (1)

typedef struct acc_mcc_info
{
    uint8_t ots_num;
    app_gaf_prf_svc_t ots_svc_hdl[APP_ACC_DFT_MCS_NUM];
}acc_mcc_info_t;

typedef struct app_acc_mcc_info
{
    acc_mcc_info_t mcc_info[BLE_AUDIO_CONNECTION_CNT];
}app_acc_mcc_info_t;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t app_acc_mcc_cmp_evt_handler(void const *param);
uint32_t app_acc_mcc_rsp_handler(void const *param);
uint32_t app_acc_mcc_ind_handler(void const *param);

void app_acc_mcc_init(void);

/// APP_GAF_MCC_SVC_DISCOVERYED_IND
void app_acc_mcc_start(uint8_t con_lid);

void app_acc_mcc_restore_bond_data_req(uint8_t con_lid,uint8_t nb_media,void const *data);

#ifdef __cplusplus
}
#endif

#endif
#endif // APP_ACC_MCC_MSG_H_

/// @} APP_ACC_MCC_MSG_H_

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
 * @addtogroup APP_ARC
 
 * @{
 ****************************************************************************************
 */

#ifndef APP_ARC_VCS_MSG_H_
#define APP_ARC_VCS_MSG_H_
#if BLE_AUDIO_ENABLED

#ifdef __cplusplus
extern "C"{
#endif

uint32_t app_arc_vcs_rsp_handler(void const *param);
uint32_t app_arc_vcs_ind_handler(void const *param);
void app_arc_vcs_restore_bond_data_req(uint8_t con_lid, uint8_t cli_cfg_bf);

void app_arc_vcs_init(void);

#ifdef __cplusplus
}
#endif

#endif
#endif // APP_ARC_VCS_MSG_H_

/// @} APP_ARC_VCS

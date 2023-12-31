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
 * @addtogroup APP_CAP
 
 * @{
 ****************************************************************************************
 */

#ifndef APP_CAPC_MSG_H_
#define APP_CAPC_MSG_H_
#if BLE_AUDIO_ENABLED

typedef struct app_cap_cac_info
{
    uint16_t csis_shdl;
    uint16_t csis_ehdl;
}app_cap_cac_info_t;

uint32_t app_cap_cac_cmp_evt_handler(void const *param);
uint32_t app_cap_cac_rsp_handler(void const *param);
uint32_t app_cap_cac_ind_handler(void const *param);
void app_cap_cac_init(void);
void app_cap_cac_start(uint8_t con_lid);
#endif
#endif // APP_CAPC_MSG_H_
/// @} APP_CAPC_MSG_H_

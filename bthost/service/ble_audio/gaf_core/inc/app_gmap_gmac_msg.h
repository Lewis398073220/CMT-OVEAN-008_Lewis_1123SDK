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
/**
 ****************************************************************************************
 * @addtogroup APP_GMAP_GMAC
 
 * @{
 ****************************************************************************************
 */
#ifndef APP_GMAP_GMAC_MSG_H_
#define APP_GMAP_GMAC_MSG_H_
#if BLE_AUDIO_ENABLED
#include "cmsis.h"

uint32_t app_gmap_gmac_rsp_handler(void const *param);
uint32_t app_gmap_gmac_cmp_evt_handler(void const *param);
uint32_t app_gmap_gmac_ind_handler(void const *param);
void app_gmap_gmac_init(void);
void app_gmap_gmac_start(uint8_t con_lid);
void app_gmap_gmac_get_role(uint8_t con_lid);


#endif
#endif // APP_GMAP_GMAC_MSG_H_



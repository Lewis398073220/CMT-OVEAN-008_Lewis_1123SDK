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
 * @addtogroup APP_GMAP
 * @{
 ****************************************************************************************
 */

#ifndef APP_GMAP_H_
#define APP_GMAP_H_

#if BLE_AUDIO_ENABLED

#include "app_gaf_define.h"

/*
 * DEFINES
 ****************************************************************************************
 */

typedef struct gmap_info
{
    app_gaf_prf_svc_t gmap_svc_hdl;
}gmap_info_t;

typedef struct app_gmap_info
{
    gmap_info_t gmap_info[BLE_AUDIO_CONNECTION_CNT];
}app_gmap_info_t;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t app_gmap_cmp_evt_handler(void const *param);
uint32_t app_gmap_req_ind_handler(void const *param);
uint32_t app_gmap_ind_handler(void const *param);
uint32_t app_gmap_rsp_handler(void const *param);

/* ble audio gaf gmap (Gaming Audio profile) init */
void app_gmap_client_init(void);
void app_gmap_server_init(void);

/* ble audio gaf gmap (Gaming Audio profile) start */
void app_gmap_start(uint8_t con_lid);

#ifdef __cplusplus
}
#endif

#endif  
#endif // APP_GMAP_H_
/// @} APP_GMAP
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

#ifndef __APP_DBG_BT_INFO__
#define __APP_DBG_BT_INFO__


#include "stdint.h"
#include "app_tws_ibrt_analysis_system.h"
#define BT_NETWORK_TOPOLOGY_MAX_ACTIVE_CONN_NUM 7
#define BT_LINK_LOSS_AFH_MAP_LEN (3 * 4)

#ifndef BT_LINK_LOSS_RX_CLOCK_INFO_SIZE
#define BT_LINK_LOSS_RX_CLOCK_INFO_SIZE 25
#endif

typedef void (*app_dbg_bt_link_loss_related_info_rcv_cb)(uint8_t* info, uint32_t info_len);

#ifdef __cplusplus
extern "C" {
#endif

void app_dbg_bt_register_link_loss_related_info_rcv_cb(app_dbg_bt_link_loss_related_info_rcv_cb func);

void app_dbg_bt_register_link_loss_universal_info_received_cb(app_tws_ibrt_analysing_info_received_cb function);

void app_dbg_bt_register_link_loss_clock_info_received_cb(app_tws_ibrt_analysing_info_received_cb function);

void app_dbg_bt_register_a2dp_sink_info_received_cb(app_tws_ibrt_analysing_info_received_cb function);

tws_ibrt_link_loss_info_t* app_dbg_bt_get_link_loss_total_info(void);

tws_ibrt_a2dp_sink_info_t* app_dbg_bt_get_a2dp_sink_info(void);

app_dbg_bt_link_loss_related_info_rcv_cb app_dbg_bt_get_link_loss_related_info_rcv_cb(void);

void app_dbg_bt_info_system_init(void);

#ifdef __cplusplus
}
#endif

#endif   //#ifndef __APP_DBG_BT_INFO__
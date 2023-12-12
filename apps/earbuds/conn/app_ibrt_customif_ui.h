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
#ifndef __APP_IBRT_CUSTOMIF_UI_UI__
#define __APP_IBRT_CUSTOMIF_UI_UI__

#define  IBRT_UI_SCAN_INTERVAL_IN_SCO_TWS_DISCONNECTED           (BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL)
#define  IBRT_UI_SCAN_WINDOW_IN_SCO_TWS_DISCONNECTED             (BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW)

#define  IBRT_UI_SCAN_INTERVAL_IN_SCO_TWS_CONNECTED              (BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL+0x400)
#define  IBRT_UI_SCAN_WINDOW_IN_SCO_TWS_CONNECTED                (BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW)

#define  IBRT_UI_SCAN_INTERVAL_IN_A2DP_TWS_DISCONNECTED          (BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL)
#define  IBRT_UI_SCAN_WINDOW_IN_A2DP_TWS_DISCONNECTED            (BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW)

#ifdef SASS_ENABLED
#define  IBRT_UI_SCAN_INTERVAL_IN_A2DP_TWS_CONNECTED             (BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL)
#else
#define  IBRT_UI_SCAN_INTERVAL_IN_A2DP_TWS_CONNECTED             (BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL+0x400)
#endif
#define  IBRT_UI_SCAN_WINDOW_IN_A2DP_TWS_CONNECTED               (BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW)

int app_ibrt_customif_ui_start(void);
void app_ibrt_customif_ui_tws_switch(void);
bool app_ibrt_customif_ui_is_tws_switching(void);
void app_ibrt_customif_tws_ui_role_updated(uint8_t newRole);
void app_ibrt_customif_get_tws_side_handler(APP_TWS_SIDE_T* twsSide);

#if defined(IBRT_UI)
void app_ibrt_customif_global_state_callback(ibrt_global_state_change_event *state);
#endif

#ifdef CUSTOM_BITRATE
void app_ibrt_user_a2dp_codec_info_action(void);
#endif

#endif /*__APP_IBRT_CUSTOMIF_UI_UI__*/

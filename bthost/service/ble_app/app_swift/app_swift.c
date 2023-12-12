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
/**
 ****************************************************************************************
 * @addtogroup APP_SWIFT
 * @{
 ****************************************************************************************
 */
#include "rwapp_config.h"  // SW configuration

#if (BLE_APP_SWIFT)
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "cmsis_os.h"
#include "app.h"  // Application Manager Definitions
#include "apps.h"
#include <string.h>
#include "app_ble_mode_switch.h"
#include "app_bt.h"
#ifdef IBRT
#include "app_tws_ibrt.h"
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#include "app_ibrt_customif_cmd.h"
#include "app_tws_ibrt_cmd_handler.h"
#endif

/************************private macro defination***************************/


#define SWIFT_PAIR_MODE_LE_ONLY      (0)
#define SWIFT_PAIR_MODE_LE_AND_BT    (1)
#define SWIFT_PAIR_MODE_BT           (2)
#define SWIFT_PAIR_MODE             SWIFT_PAIR_MODE_BT

#define SWIFT_MS_HEADER_LEN         (6)
#define SWIFT_MS_VENDER_ID          (0xff0600)
#define SWIFT_MS_BEACON             (0x03)
#if (SWIFT_PAIR_MODE == SWIFT_PAIR_MODE_LE_ONLY)
#define SWIFT_MS_BEACON_SUB         (0)
#define SWIFT_ADV_DISPLAY_ICON      (0)
#define SWIFT_ADV_BT_ADDR           (0)
#elif (SWIFT_PAIR_MODE == SWIFT_PAIR_MODE_LE_AND_BT)
#define SWIFT_MS_BEACON_SUB         (0x02)
#define SWIFT_ADV_DISPLAY_ICON      (1)
#define SWIFT_ADV_BT_ADDR           (0)
#else
#define SWIFT_MS_BEACON_SUB         (0x01)
#define SWIFT_ADV_DISPLAY_ICON      (1)
#define SWIFT_ADV_BT_ADDR           (1)
#endif
#define SWIFT_RSSI                  (0x80)
#define SWIFT_DISPLAY_ICON_LEN      (3)
#define SWIFT_DISPLAY_ICON          (0x040424)

#define BLE_SWIFT_ADVERTISING_INTERVAL (100)

// adv mode should be ADV_MODE_EXTENDED or ADV_MODE_LEGACY
#define SWIFT_ADV_MODE_LEGACY        (0)
#define SWIFT_ADV_MODE_EXTEND        (1)
#define SWIFT_ADV_MODE               SWIFT_ADV_MODE_LEGACY

static bool enable_swift = false;

/**********************private function declearation************************/
/*---------------------------------------------------------------------------
 *            swift_ble_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    BLE advertisement and scan response data fill handler for Google fast pair
 *
 * Parameters:
 *    param - pointer of BLE parameter to be configure
 *
 * Return:
 *    void
 */
static void swift_ble_data_fill_handler(void *param)
{
    char *display_name;
    uint8_t nameLen = 0, maxLen = 0, tempLen = 0, advLen = 0;
    ASSERT(param, "invalid param");

    BLE_ADV_PARAM_T *advInfo = ( BLE_ADV_PARAM_T * )param;

    if (enable_swift)
    {
        advLen = 1 + SWIFT_MS_HEADER_LEN;
#if SWIFT_ADV_BT_ADDR
        advLen += sizeof(bt_bdaddr_t);
#endif

#if SWIFT_ADV_DISPLAY_ICON
        advLen += SWIFT_DISPLAY_ICON_LEN;
#endif

#if (SWIFT_ADV_MODE == SWIFT_ADV_MODE_EXTEND)
        maxLen = EXT_ADV_DATA_LEN - advInfo->advDataLen;
#else
        maxLen = 31 - advInfo->advDataLen;
#endif

        if (advLen > maxLen)
        {
            TRACE(0, "ADV is overflow, cannot fill swift data!!!");
            return;
        }

#ifdef IBRT
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        ibrt_role_e  role = app_ibrt_if_get_ui_role();

        if (IBRT_SLAVE != role && p_ibrt_ctrl->init_done)
#endif
        {     
            advInfo->advType = ADV_TYPE_UNDIRECT;
            advInfo->advUserInterval[USER_SWIFT] = BLE_SWIFT_ADVERTISING_INTERVAL;

            tempLen = advInfo->advDataLen;
            advInfo->advDataLen++; //reseverd for the first length byte.
            advInfo->advData[tempLen] =  SWIFT_MS_HEADER_LEN;
            advInfo->advData[advInfo->advDataLen++] = (SWIFT_MS_VENDER_ID >> 16) & 0xFF;
            advInfo->advData[advInfo->advDataLen++] = (SWIFT_MS_VENDER_ID >> 8) & 0xFF;
            advInfo->advData[advInfo->advDataLen++] = SWIFT_MS_VENDER_ID  & 0xFF;
            advInfo->advData[advInfo->advDataLen++] = SWIFT_MS_BEACON;
            advInfo->advData[advInfo->advDataLen++] = SWIFT_MS_BEACON_SUB;
            advInfo->advData[advInfo->advDataLen++] = SWIFT_RSSI;

#if SWIFT_ADV_BT_ADDR
            advInfo->advData[tempLen] += sizeof(bt_bdaddr_t);
            memcpy(advInfo->advData+advInfo->advDataLen, p_ibrt_ctrl->local_addr.address, sizeof(bt_bdaddr_t));
            advInfo->advDataLen += sizeof(bt_bdaddr_t);
#endif

#if SWIFT_ADV_DISPLAY_ICON
            advInfo->advData[tempLen] += SWIFT_DISPLAY_ICON_LEN;
            advInfo->advData[advInfo->advDataLen++] = (SWIFT_DISPLAY_ICON >> 16) & 0xFF;
            advInfo->advData[advInfo->advDataLen++] = (SWIFT_DISPLAY_ICON >> 8) & 0xFF;
            advInfo->advData[advInfo->advDataLen++] = SWIFT_DISPLAY_ICON  & 0xFF;
#endif

#if (SWIFT_ADV_MODE == SWIFT_ADV_MODE_EXTEND)
            advInfo->advMode = ADV_MODE_EXTENDED;
            maxLen = EXT_ADV_DATA_LEN - advInfo->advDataLen;
#else    
            advInfo->advMode = ADV_MODE_LEGACY;
            maxLen = 31 - advInfo->advDataLen;
#endif

            if (maxLen > 0)
            {
                display_name = (char *)app_ble_get_dev_name();
                nameLen = MIN(strlen(display_name) + 1, maxLen);

                memcpy(advInfo->advData+advInfo->advDataLen, (uint8_t *)display_name, nameLen);
                advInfo->advDataLen += nameLen;
                advInfo->advData[advInfo->advDataLen - 1] = '\0';
                advInfo->advData[tempLen] += nameLen;
            }
        }
    }
    app_ble_data_fill_enable(USER_SWIFT, enable_swift);

    TRACE(2,"[%s] swift_enable=%d", __func__, enable_swift);
}

/****************************function defination****************************/
void app_swift_init(void)
{
    TRACE(1,"[%s]", __func__);

    app_ble_register_data_fill_handle(USER_SWIFT, ( BLE_DATA_FILL_FUNC_T )swift_ble_data_fill_handler, false);
}

void app_swift_enter_pairing_mode(void)
{
    enable_swift = true;
    bes_bt_me_write_access_mode(BTIF_BAM_GENERAL_ACCESSIBLE, 0);
}

void app_swift_exit_pairing_mode(void)
{
    enable_swift = false;
}
#endif
/// @} APP_SWIFT

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
#include "hal_timer.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_aud.h"
#include "string.h"
#include "cmsis_os.h"
#include "ota_bes.h"
#include "bluetooth_bt_api.h"
#ifdef __IAG_BLE_INCLUDE__
#include "bluetooth_ble_api.h"
#endif
#include "ota_spp.h"
#include "cqueue.h"
#include "ota_dbg.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_location.h"
#include "ota_ble_adapter.h"
#ifdef IBRT
#include "app_ibrt_ota_cmd.h"
#include "app_tws_ctrl_thread.h"
#endif

typedef struct
{
    uint8_t connectedType;
} APP_OTA_ENV_T;

static APP_OTA_ENV_T app_ota_env=
    {
        0,
    };

bool app_is_in_ota_mode(void)
{
    return app_ota_env.connectedType;
}

void app_ota_connected(uint8_t connType)
{
    LOG_D("ota is connected.");
    app_ota_env.connectedType |= connType;
}

void app_ota_disconnected(uint8_t disconnType, uint8_t linkType)
{
    LOG_D("Ota is disconnected linkType %d", linkType);
    app_ota_env.connectedType &= disconnType;
    Bes_exit_ota_state();
    //ota_check_and_reboot_to_use_new_image();
    app_spp_ota_register_tx_done(NULL);

#ifdef IBRT
    uint8_t disconnected_parm = linkType;
    if (APP_OTA_LINK_TYPE_BLE == linkType)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_MOBILE_DISC_CMD, &disconnected_parm, 1);
    }
#endif
}

void bes_ota_init(void)
{
    app_spp_ota_init();
#ifdef __IAG_BLE_INCLUDE__
#if BLE_TOTA_ENABLED || (defined(BES_OTA) && !defined(OTA_OVER_TOTA_ENABLED))
    ota_ble_adapter_init();
#endif
#endif
}


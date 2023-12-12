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
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)

#include <string.h>

#include "app_task.h"                // Application task Definition
#include "gap.h"                     // GAP Definition
#include "tgt_hardware.h"
#include "gapm_le_msg.h"               // GAP Manager Task API
#include "gapc_le_msg.h"
#include "gapc_msg.h"               // GAP Controller Task API
#include "gapm.h"

#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "app.h"                     // Application Definition

#include "nvrecord_ble.h"
#include "prf.h"

#include "nvrecord_env.h"
#ifndef _BLE_NVDS_
#include "tgt_hardware.h"
#endif
#ifdef __AI_VOICE__
#include "app_ai_ble.h"                 // AI Voice Application Definitions
#endif //(__AI_VOICE__)

#if (BLE_APP_SEC)
#include "app_sec.h"                    // Application security Definition
#endif // (BLE_APP_SEC)

#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h"        // Data Path Server Application Definitions
#endif //(BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_SAS_SERVER)
#include "app_sas_server.h"             // Switching Ambient Server Application Definitions
#endif // (BLE_APP_SAS_SERVER)

#if (BLE_APP_AHP_SERVER)
#include "app_ahp_server.h"            // Advanced Headphone Server Application Definitions
#endif //(BLE_APP_AHP_SERVER)

#if (BLE_APP_DATAPATH_CLIENT)
#include "app_datapath_client.h"        // Data Path Client Application Definitions
#endif //(BLE_APP_DATAPATH_CLIENT)

#if (BLE_APP_DIS)
#include "app_dis.h"                    // Device Information Service Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"                   // Battery Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h"                    // HID Application Definitions
#endif //(BLE_APP_HID)

#if (BLE_APP_OTA)
#include "app_ota.h"                    // OTA Application Definitions
#endif //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h"               // OTA Application Definitions
#endif //(BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "app_ancc.h"                   // ANCC Application Definitions
#endif //(BLE_APP_ANCC)

#if (BLE_APP_ANCS)
#include "app_ancs.h"                   // ANCS Application Definitions
#endif // (BLE_APP_ANCS)

#if (BLE_APP_AMSC)
#include "app_amsc.h"                   // AMSC Module Definition
#endif // (BLE_APP_AMSC)

#if (BLE_APP_GFPS)
#include "app_gfps.h"                   // Google Fast Pair Service Definitions
#endif

#if (BLE_APP_SWIFT)
#include "app_swift.h"                  //Microsoft swift pair
#endif

#if (DISPLAY_SUPPORT)
#include "app_display.h"                // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"                    // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h"                       // NVDS Definitions
#endif //(NVDS_SUPPORT)

#include "cmsis_os.h"
#include "ke_timer.h"
#include "nvrecord_bt.h"
#include "ble_app_dbg.h"

#include "bluetooth_bt_api.h"
#include "app_bt.h"
#include "app_ble_mode_switch.h"
#include "apps.h"
#include "crc16_c.h"
#include "besbt.h"

#ifdef BISTO_ENABLED
#include "gsound_service.h"
#endif

#if defined(__INTERCONNECTION__)
#include "app_bt.h"
#include "app_battery.h"
#endif

#if (BLE_APP_TILE)
#include "app_tile.h"
#endif

#if defined(IBRT)
#include "app_tws_ibrt.h"
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#endif

#ifdef FPGA
#include "hal_timer.h"
#endif

#ifdef __GATT_OVER_BR_EDR__
#include "btgatt_api.h"
#endif
#include "bt_if.h"

#if BLE_BUDS
#include "buds.h"
#endif

#include "gapm.h"
#include "gap_int.h"

#include "bt_drv_reg_op.h"

#ifdef BLE_HOST_PTS_TEST_ENABLED
#include "pts_ble_host_app.h"
#endif

#include "app_bt_func.h"

#if BLE_AUDIO_ENABLED
#include "ble_audio_earphone_info.h"
#if (mHDT_LE_SUPPORT)
#include "mhdt_le_msg.h"
#endif
#endif

#include "app_ble_core.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Default Device Name
#define APP_DFLT_DEVICE_NAME            ("BES_BLE")
#define APP_DFLT_DEVICE_NAME_LEN        (sizeof(APP_DFLT_DEVICE_NAME))


#if (BLE_APP_HID)
// HID Mouse
#define DEVICE_NAME        "Hid Mouse"
#else
#define DEVICE_NAME        "RW DEVICE"
#endif

#define DEVICE_NAME_SIZE    sizeof(DEVICE_NAME)

/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x09\x18 - Health Thermometer Service UUID
 *   or
 * x12\x18 - HID Service UUID
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_UUID        "\x03\x03\x09\x18"
#define APP_HT_ADV_DATA_UUID_LEN    (4)
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID       "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN   (4)
#endif //(BLE_APP_HID)

#if (BLE_APP_DATAPATH_SERVER)
/*
 * x11 - Length
 * x07 - Complete list of 16-bit UUIDs available
 * .... the 128 bit UUIDs
 */
#define APP_DATAPATH_SERVER_ADV_DATA_UUID        "\x11\x07\x9e\x34\x9B\x5F\x80\x00\x00\x80\x00\x10\x00\x00\x00\x01\x00\x01"
#define APP_DATAPATH_SERVER_ADV_DATA_UUID_LEN    (18)
#endif //(BLE_APP_HT)

/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - Generic Thermometer
 *   or
 * xC2\x04 - HID Mouse
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_APPEARANCE    "\x03\x19\x00\x03"
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE   "\x03\x19\xC2\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN  (4)

/**
 * Default Scan response data
 * --------------------------------------------------------------------------------------
 * x09                             - Length
 * xFF                             - Vendor specific advertising type
 * x00\x60\x52\x57\x2D\x42\x4C\x45 - "RW-BLE"
 * --------------------------------------------------------------------------------------
 */
#define APP_SCNRSP_DATA         "\x09\xFF\x00\x60\x52\x57\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LEN     (10)

/**
 * Advertising Parameters
 */
#if (BLE_APP_HID)
/// Default Advertising duration - 30s (in multiple of 10ms)
#define APP_DFLT_ADV_DURATION   (3000)
#endif //(BLE_APP_HID)
/// Advertising channel map - 37, 38, 39
#define APP_ADV_CHMAP           (0x07)
/// Advertising minimum interval - 40ms (64*0.625ms)
#define APP_ADV_INT_MIN         (64)
/// Advertising maximum interval - 40ms (64*0.625ms)
#define APP_ADV_INT_MAX         (64)
/// Fast advertising interval
#define APP_ADV_FAST_INT        (32)

extern void gapm_read_rpa_addr_cmd(const ble_bdaddr_t *peer_identify_addr);

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list
{
#if (BLE_APP_HT)
    APPM_SVC_HTS,
#endif //(BLE_APP_HT)
#if (BLE_APP_DIS)
    APPM_SVC_DIS,
#endif //(BLE_APP_DIS)
#if (BLE_APP_BATT)
    APPM_SVC_BATT,
#endif //(BLE_APP_BATT)
#if (BLE_APP_HID)
    APPM_SVC_HIDS,
#endif //(BLE_APP_HID)
#ifdef BLE_APP_AM0
    APPM_SVC_AM0_HAS,
#endif //defined(BLE_APP_AM0)
#if (BLE_APP_HR)
    APPM_SVC_HRP,
#endif
#if (BLE_APP_DATAPATH_SERVER)
    APPM_SVC_DATAPATH_SERVER,
#endif //(BLE_APP_DATAPATH_SERVER)
#if (BLE_APP_SAS_SERVER)
    APPM_SVC_SAS_SERVER,
#endif //(BLE_APP_SAS_SERVER)
#if (BLE_APP_AHP_SERVER)
    APPM_SVC_AHP_SERVER,
#endif //(BLE_APP_AHP_SERVER)
#if (BLE_APP_DATAPATH_CLIENT)
    APPM_SVC_DATAPATH_CLIENT,
#endif //(BLE_APP_DATAPATH_CLIENT)
#if (BLE_APP_BMS)
    APPM_SVC_BMS,
#endif // (BLE_APP_BMS)
#if (BLE_APP_ANCC)
    APPM_SVC_ANCC,
#endif //(BLE_APP_ANCC)
#if (BLE_APP_ANCS)
    APPM_SVC_ANCSP,
#endif // (BLE_APP_ANCS)
#if (BLE_APP_AMS)
    APPM_SVC_AMSP,
#endif //(BLE_APP_AMS)
#if (BLE_APP_AMSC)
    APPM_SVC_AMSC,
#endif // (BLE_APP_AMSC)
#if (BLE_APP_OTA)
    APPM_SVC_OTA,
#endif //(BLE_APP_OTA)
#if (BLE_APP_GFPS)
    APPM_SVC_GFPS,
#endif //(BLE_APP_GFPS)
#if (BLE_AMA_VOICE)
    APPM_AI_AMA,
#endif //(BLE_AI_VOICE)

#if (BLE_DMA_VOICE)
    APPM_AI_DMA,
#endif //(BLE_AI_VOICE)

#if (BLE_GMA_VOICE)
    APPM_AI_GMA,
#endif //(BLE_AI_VOICE)

#if (BLE_SMART_VOICE)
    APPM_AI_SMARTVOICE,
#endif //(BLE_AI_VOICE)

#if (BLE_TENCENT_VOICE)
    APPM_AI_TENCENT,
#endif //(BLE_AI_VOICE)
#if (BLE_APP_TOTA)
    APPM_SVC_TOTA,
#endif //(BLE_APP_TOTA)

#if (BLE_APP_TILE)
    APPM_SVC_TILE,
#endif //(BLE_APP_TILE)

#if (BLE_BUDS)
    APPM_SVC_BUDS,
#endif //(BLE_BUDS)

    APPM_SVC_LIST_STOP,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Task Descriptor
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
#if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
#endif //(BLE_APP_HT)
#if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
#endif //(BLE_APP_DIS)
#if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
#endif //(BLE_APP_BATT)
#if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
#endif //(BLE_APP_HID)
#if (BLE_APP_AM0)
    (appm_add_svc_func_t)app_am0_add_has,
#endif //(BLE_APP_AM0)
#if (BLE_APP_HR)
    (appm_add_svc_func_t)app_hrps_add_profile,
#endif
#if (BLE_APP_SAS_SERVER)
    (appm_add_svc_func_t)app_sas_add_sass,
#endif // (BLE_APP_SAS_SERVER)
#if (BLE_APP_DATAPATH_SERVER)
    (appm_add_svc_func_t)app_datapath_add_datapathps,
#endif //(BLE_APP_DATAPATH_SERVER)
#if (BLE_APP_AHP_SERVER)
    (appm_add_svc_func_t)app_ahps_add_ahs,
#endif //(BLE_APP_AHP_SERVER)
#if (BLE_APP_DATAPATH_CLIENT)
    (appm_add_svc_func_t)app_datapath_add_datapathpc,
#endif //(CFG_APP_DATAPATH_CLIENT)
#if (BLE_APP_BMS)
    (appm_add_svc_func_t)app_ble_bms_add_svc,
#endif // BMS_ENABLED
#if (BLE_APP_ANCC)
    (appm_add_svc_func_t)app_ancc_add_ancc,
#endif
#if (BLE_APP_ANCS)
    (appm_add_svc_func_t)app_ancs_add_svc,
#endif
#if (BLE_APP_AMSC)
    (appm_add_svc_func_t)app_amsc_add_amsc,
#endif
#if (BLE_APP_AMS)
    (appm_add_svc_func_t)app_amsp_add_svc,
#endif
#if (BLE_APP_OTA)
    (appm_add_svc_func_t)app_ota_add_ota,
#endif //(BLE_APP_OTA)
#if (BLE_APP_GFPS)
    (appm_add_svc_func_t)app_gfps_add_gfps,
#endif
#if (BLE_APP_AMA_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_ama,
#endif //(BLE_APP_AMA_VOICE)
#if (BLE_APP_DMA_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_dma,
#endif //(BLE_APP_DMA_VOICE)
#if (BLE_APP_GMA_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_gma,
#endif
#if (BLE_APP_SMART_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_smartvoice,
#endif
#if (BLE_APP_TENCENT_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_tencent,
#endif
#if (BLE_APP_TOTA)
    (appm_add_svc_func_t)app_tota_add_tota,
#endif //(BLE_APP_TOTA)
#if (BLE_APP_TILE)
    (appm_add_svc_func_t)app_ble_tile_add_svc,
#endif
#if (BLE_BUDS)
    (appm_add_svc_func_t)app_add_buds,
#endif //(BLE_BUDS)
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

#ifdef CFG_SEC_CON
uint8_t ble_public_key[64];
uint8_t ble_private_key[32];
#endif

#if defined (BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT)
ble_bdaddr_t ble_rnd_addr;
#endif
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void appm_refresh_ble_irk(void)
{
    nv_record_blerec_get_local_irk(app_env.loc_irk);
    LOG_I("[APPM]local irk:");
    DUMP8("0x%02x ", app_env.loc_irk, 16);
    // TODO: update to use the new API
    gapm_update_irk(app_env.loc_irk);
}

uint8_t *appm_get_current_ble_irk(void)
{
    return app_env.loc_irk;
}

APP_BLE_CONN_CONTEXT_T *app_ble_get_device_conext_by_addr(uint8_t *addr)
{
    APP_BLE_CONN_CONTEXT_T *ctx = NULL;
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (!memcmp(addr, app_env.context[i].bdAddr, BD_ADDR_LEN))
        {
            ctx = &app_env.context[i];
            break;
        }
    }
    return ctx;
}

bool app_ble_get_rpa_only_state(uint8_t *addr)
{
    APP_BLE_CONN_CONTEXT_T *ctx = app_ble_get_device_conext_by_addr(addr);
    if (ctx && ctx->supportRpaOnly)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t appm_prepare_devices_info_added_to_resolving_list(ble_gap_ral_dev_info_t *devicesInfo)
{
    uint8_t devicesInfoNumber = 0;
    uint8_t support_num = 0;
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *nvrecord_ble_p = nv_record_blerec_get_ptr();
    uint8_t maxListNumber = RESOLVING_LIST_MAX_NUM < nvrecord_ble_p->saved_list_num ? RESOLVING_LIST_MAX_NUM : nvrecord_ble_p->saved_list_num;
    if (nvrecord_ble_p->saved_list_num > 0)
    {
        for (uint8_t index = 0; index < maxListNumber; index++)
        {
            if (get_bit(nvrecord_ble_p->ble_nv[index].pairingInfo.bond_info_bf, BONDED_WITH_IRK_DISTRIBUTED))
            {
                memcpy((devicesInfo + support_num)->addr.addr, nvrecord_ble_p->ble_nv[index].pairingInfo.peer_addr.addr, 6);
                (devicesInfo + support_num)->addr.addr_type = nvrecord_ble_p->ble_nv[index].pairingInfo.peer_addr.addr_type;
                if (app_ble_get_rpa_only_state(nvrecord_ble_p->ble_nv[index].pairingInfo.peer_rpa_addr))
                {
                    (devicesInfo + support_num)->priv_mode = 0;
                }
                else
                {
                    (devicesInfo + support_num)->priv_mode = 1;
                }
                TRACE(1, "privacy mode is:%d", (devicesInfo + support_num)->priv_mode);
                memcpy((devicesInfo + support_num)->peer_irk, nvrecord_ble_p->ble_nv[index].pairingInfo.IRK, 16);
                memcpy((devicesInfo + support_num)->local_irk, nvrecord_ble_p->self_info.ble_irk, 16);
                support_num++;
            }
        }//BES intentional code. RESOLVING_LIST_MAX_NUM is not always 8.
        devicesInfoNumber = support_num;
        TRACE(0, "devicesInfoNumber %d", devicesInfoNumber);
    }

#ifdef GFPS_ENABLED   // Always add local address into RPA list
    if (devicesInfoNumber >= RESOLVING_LIST_MAX_NUM)
    {
        devicesInfoNumber = devicesInfoNumber - 1;
    }
    uint8_t index = devicesInfoNumber;
    (devicesInfo + index)->priv_mode = 0;
    memcpy((devicesInfo + index)->local_irk,  appm_get_current_ble_irk(), 16);
    (devicesInfo + index)->addr.addr_type = 0;
    memcpy((devicesInfo + index)->addr.addr, bt_get_ble_local_address(), 6);
    memcpy((devicesInfo + index)->peer_irk, appm_get_current_ble_irk(), 16);
    devicesInfoNumber += 1;
#endif

    return devicesInfoNumber;
}

void appm_add_multiple_devices_to_resolving_list_in_controller(ble_gap_ral_dev_info_t *devicesInfoAddedToResolvingList, uint8_t numberOfDevicesAddedToResolvingList)
{
    TRACE(1, "%s", __func__);
    if (0 == numberOfDevicesAddedToResolvingList)
    {
        return;
    }

#if (BLE_AUDIO_ENABLED == 0) && defined(IBRT)
    if (TWS_UI_SLAVE != app_ibrt_if_get_ui_role())
    {
        app_ble_sync_ble_info();
    }
#endif

    if (numberOfDevicesAddedToResolvingList > RESOLVING_LIST_MAX_NUM)
    {
        numberOfDevicesAddedToResolvingList = RESOLVING_LIST_MAX_NUM;
    }

    app_env.numberOfDevicesAddedToResolvingList = numberOfDevicesAddedToResolvingList;
    memcpy(&app_env.devicesInfoAddedToResolvingList[0], devicesInfoAddedToResolvingList, sizeof(ble_gap_ral_dev_info_t)*numberOfDevicesAddedToResolvingList);
    /// Mark that ral is pending until it set success
    app_env.isNeedToAddDevicesToReslovingList = true;
    app_ble_add_resloving_list(devicesInfoAddedToResolvingList, numberOfDevicesAddedToResolvingList);
}

void appm_set_rpa_list(ble_gap_ral_dev_info_t *ral, uint8_t size)
{
    TRACE(1, "%s", __func__);
    uint8_t activityIndex = 0xFF;
    if (0 == size)
    {
        return;
    }

    if (size > RESOLVING_LIST_MAX_NUM)
    {
        size = RESOLVING_LIST_MAX_NUM;
    }

    for (uint8_t i = 0; i < BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        activityIndex = app_env.adv_actv_idx[i];
        if (activityIndex != INVALID_BLE_ACTIVITY_INDEX &&
                APP_ADV_STATE_STARTED == app_env.state[activityIndex])
        {
            appm_stop_advertising(activityIndex);
            app_env.isNeedToAddDevicesToReslovingList = true;
            TRACE(0, "%s Line:%d fail", __func__, __LINE__);
            return;
        }
    }

    if (app_env.scan_actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_SCAN_STATE_STARTED == app_env.state[app_env.scan_actv_idx])
    {
        appm_stop_scanning();
        app_env.isNeedToRestartScan = true;
        app_env.isNeedToAddDevicesToReslovingList = true;
    }
    else if (app_env.connect_actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             APP_CONNECT_STATE_STARTED == app_env.state[app_env.connect_actv_idx])
    {
        app_env.isNeedToAddDevicesToReslovingList = true;
    }
    else if (app_env.setting_white_list)
    {
        TRACE(0, "set wl ing, add ral pending");
        app_env.isNeedToAddDevicesToReslovingList = true;
    }
    else
    {
        app_env.isNeedToAddDevicesToReslovingList = false;
        app_env.addingDevicesToReslovingList = true;
        // Prepare the GAPM_ACTIVITY_START_CMD message
        struct gapm_list_set_ral_cmd *setReslovingListCmd = KE_MSG_ALLOC_DYN(GAPM_LIST_SET_CMD,
                                                                             TASK_GAPM, TASK_APP,
                                                                             gapm_list_set_ral_cmd,
                                                                             sizeof(ble_gap_ral_dev_info_t) * (size - __ARRAY_EMPTY));

        setReslovingListCmd->operation = GAPM_SET_RAL;
        setReslovingListCmd->size = size;
        memcpy(&setReslovingListCmd->ral_info[0], &ral[0],
               sizeof(ble_gap_ral_dev_info_t)*size);

        // Send the message
        ke_msg_send(setReslovingListCmd);
    }
}

void appm_set_white_list(ble_bdaddr_t *bdaddr, uint8_t size)
{
    uint8_t actv_idx = 0xFF;
    if (size > WHITE_LIST_MAX_NUM)
    {
        size = WHITE_LIST_MAX_NUM;
    }

    app_env.white_list_size = size;
    memcpy(&app_env.white_list_addr[0], bdaddr, sizeof(ble_bdaddr_t)*size);

    for (uint8_t i = 0; i < BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        actv_idx = app_env.adv_actv_idx[i];
        if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
                APP_ADV_STATE_STARTED == app_env.state[actv_idx])
        {
            appm_stop_advertising(actv_idx);
            app_env.need_set_white_list = true;
            return;
        }
    }

    if (app_env.scan_actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_SCAN_STATE_STARTED == app_env.state[app_env.scan_actv_idx])
    {
        appm_stop_scanning();
        app_env.isNeedToRestartScan = true;
        app_env.need_set_white_list = true;
    }
    else if (app_env.connect_actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             APP_CONNECT_STATE_STARTED == app_env.state[app_env.connect_actv_idx])
    {
        appm_stop_connecting();
        app_env.need_set_white_list = true;
    }
    else if (app_env.addingDevicesToReslovingList)
    {
        TRACE(1, "Adding device into ral ing, set white list pending");
        app_env.need_set_white_list = true;
    }
    else
    {
        app_env.need_set_white_list = false;
        app_env.setting_white_list = true;
        // Prepare the GAPM_ACTIVITY_START_CMD message
        struct gapm_list_set_wl_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_LIST_SET_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_list_set_wl_cmd,
                                                              sizeof(ble_bdaddr_t) * (size - __ARRAY_EMPTY));

        p_cmd->operation = GAPM_SET_WL;
        p_cmd->size = size;
        memcpy(&p_cmd->wl_info[0], bdaddr, sizeof(ble_bdaddr_t)*size);

        // Send the message
        ke_msg_send(p_cmd);
    }
}

uint8_t appm_get_all_paired_dev_addr_from_nv(ble_bdaddr_t *paired_addr_list)
{
    LOG_I("%s", __func__);
    BLE_ADDR_INFO_T addr_list[BLE_RECORD_NUM];
    uint8_t paired_dev_num = 0;

    paired_dev_num = nv_record_blerec_enum_paired_dev_addr(&addr_list[0]);
    if (paired_dev_num > 0 && paired_addr_list)
    {
        memcpy(&paired_addr_list[0], &addr_list[0], sizeof(BLE_ADDR_INFO_T)*paired_dev_num);
    }

    return paired_dev_num;
}

static void appm_kick_off_advertising(uint8_t actv_idx)
{
    LOG_I("%s actv_idx %d", __func__, actv_idx);
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_activity_start_cmd,
                                                             sizeof(gapm_adv_param_t));

    gapm_adv_param_t *adv_add_param = (gapm_adv_param_t *)(p_cmd->u_param);
    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = actv_idx;
    adv_add_param->duration = 0;
    adv_add_param->max_adv_evt = 0;

    // Send the message
    ke_msg_send(p_cmd);

    // Update connecting state
    appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTING);
}

void appm_stop_advertising(uint8_t actv_idx)
{
    LOG_I("%s actv_idx %d", __func__, actv_idx);
    if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_ADV_STATE_STARTED == app_env.state[actv_idx])
    {
        // Prepare the GAPM_ACTIVITY_STOP_CMD message
        struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_stop_cmd);

        // Fill the allocated kernel message
        cmd->operation = GAPM_STOP_ACTIVITY;
        cmd->actv_idx = actv_idx;

        // Send the message
        ke_msg_send(cmd);

        // Update advertising state
        appm_update_actv_state(actv_idx, APP_ADV_STATE_STOPPING);
    }
    else if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             app_env.state[actv_idx] > APP_ADV_STATE_STARTED &&
             app_env.state[actv_idx] <= APP_ADV_STATE_DELETING)
    {
        LOG_I("%s stopping now state %d", __func__, app_env.state[actv_idx]);
    }
    else
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);
    }
}

static void appm_kick_off_scanning(void)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_activity_start_cmd,
                                                             sizeof(gapm_scan_param_t));
    gapm_scan_param_t *scan_param = (gapm_scan_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = app_env.scan_actv_idx;

    if (app_env.ble_scan_param.scanType)
    {
        scan_param->prop = GAPM_SCAN_PROP_PHY_1M_BIT | GAPM_SCAN_PROP_ACTIVE_1M_BIT;
    }
    else
    {
        scan_param->prop = GAPM_SCAN_PROP_PHY_1M_BIT;
    }

    if (app_env.ble_scan_param.scanFolicyType & BLE_SCAN_ALLOW_ADV_WLST)
    {
        scan_param->type = GAPM_SCAN_TYPE_SEL_CONN_DISC;
    }
    else
    {
        scan_param->type = GAPM_SCAN_TYPE_GEN_DISC;
    }

    if (app_env.ble_scan_param.scanFolicyType & BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA)
    {
        scan_param->prop |= GAPM_SCAN_PROP_ACCEPT_RPA_BIT;
    }

    scan_param->dup_filt_pol = GAPM_DUP_FILT_DIS;
    scan_param->scan_param_1m.scan_intv = app_env.ble_scan_param.scanIntervalMs * 1000 / 625;
    scan_param->scan_param_1m.scan_wd = app_env.ble_scan_param.scanWindowMs * 1000 / 625;

    if (app_env.scan_coded_phy_en &&
            (app_env.scan_start_param_coded.scan_wd_ms + app_env.scan_start_param_coded.scan_wd_ms != 0))
    {
        scan_param->prop |= (GAPM_SCAN_PROP_PHY_CODED_BIT | GAPM_SCAN_PROP_ACTIVE_CODED_BIT);
        scan_param->scan_param_coded.scan_intv =
            app_env.scan_start_param_coded.scan_intv_ms * 1000 / 625;
        scan_param->scan_param_coded.scan_wd =
            app_env.scan_start_param_coded.scan_wd_ms * 1000 / 625;
    }

    scan_param->duration = app_env.ble_scan_param.scanDurationMs * 1000 / 10;
    scan_param->period = 0;

    if (scan_param->duration && scan_param->period)
    {
        ASSERT(scan_param->duration < scan_param->period,
               "%s duration %d is greater than or equal to period %d", __func__,
               scan_param->duration, scan_param->period);
    }

    // Send the message
    ke_msg_send(p_cmd);

    // Update connecting state
    appm_update_actv_state(app_env.scan_actv_idx, APP_SCAN_STATE_STARTING);
}

void appm_stop_scanning(void)
{
    uint8_t actv_idx = app_env.scan_actv_idx;
    if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_SCAN_STATE_STARTED == app_env.state[actv_idx])
    {
        // Prepare the GAPM_ACTIVITY_STOP_CMD message
        struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_stop_cmd);

        // Fill the allocated kernel message
        cmd->operation = GAPM_STOP_ACTIVITY;
        cmd->actv_idx = actv_idx;

        // Send the message
        ke_msg_send(cmd);

        // Update advertising state
        appm_update_actv_state(actv_idx, APP_SCAN_STATE_STOPPING);
    }
    else if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             app_env.state[actv_idx] > APP_SCAN_STATE_STARTED &&
             app_env.state[actv_idx] <= APP_SCAN_STATE_DELETING)
    {
        LOG_I("%s stopping now state %d", __func__, app_env.state[actv_idx]);
    }
    else
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_SCAN, actv_idx);
    }
}

static void appm_kick_off_connecting(void)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_activity_start_cmd,
                                                             sizeof(gapm_init_param_t));
    gapm_init_param_t *init_param = (gapm_init_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = app_env.connect_actv_idx;

    init_param->type = app_env.ble_init_param.gapm_init_type;
    init_param->prop = GAPM_INIT_PROP_1M_BIT;

    if (app_env.ble_init_param.init_conn_all_phy_en)
    {
        init_param->prop |= (GAPM_INIT_PROP_2M_BIT | GAPM_INIT_PROP_CODED_BIT);
    }

    init_param->conn_to = app_env.ble_init_param.conn_to; //!< 10s timeout

    /// scan param
    init_param->scan_param_1m.scan_intv = 0x0200;
    init_param->scan_param_1m.scan_wd =  0x0040;

    /// 1M phy connect param
#ifdef LEA_FOUR_CIS_ENABLED
    init_param->conn_param_1m.conn_intv_min = 32;
    init_param->conn_param_1m.conn_intv_max = 32;
#else
    init_param->conn_param_1m.conn_intv_min = 36;
    init_param->conn_param_1m.conn_intv_max = 36;
#endif
    init_param->conn_param_1m.conn_latency = 0;
    init_param->conn_param_1m.supervision_to = 0x01F4;
    init_param->conn_param_1m.ce_len_min = 0;
    init_param->conn_param_1m.ce_len_max = 8;

    /// update all conn param in every phy
    if (app_env.init_conn_param_universal.conn_intv_min != 0)
    {
        /// 1M phy connect param
        memcpy(&init_param->conn_param_1m,
               &app_env.init_conn_param_universal,
               sizeof(app_env.init_conn_param_universal));
        /// 2M phy connect param
        memcpy(&init_param->conn_param_2m,
               &app_env.init_conn_param_universal,
               sizeof(app_env.init_conn_param_universal));
        /// coded phy connect param
        memcpy(&init_param->conn_param_coded,
               &app_env.init_conn_param_universal,
               sizeof(app_env.init_conn_param_universal));
        memcpy(&init_param->scan_param_coded,
               &app_env.init_conn_scan_param_coded,
               sizeof(app_env.init_conn_scan_param_coded));
    }

    if (GAPM_INIT_TYPE_DIRECT_CONN_EST == init_param->type)
    {
        memcpy(init_param->peer_addr.addr, app_env.ble_init_param.peer_addr.addr, BD_ADDR_LEN);
        init_param->peer_addr.addr_type = app_env.ble_init_param.peer_addr.addr_type;
    }

    // Send the message
    ke_msg_send(p_cmd);

    // Update connecting state
    appm_update_actv_state(app_env.connect_actv_idx, APP_CONNECT_STATE_STARTING);
}

void appm_stop_connecting(void)
{
    uint8_t actv_idx = app_env.connect_actv_idx;
    if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_CONNECT_STATE_STARTED == app_env.state[actv_idx])
    {
        // Prepare the GAPM_ACTIVITY_STOP_CMD message
        struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_stop_cmd);

        // Fill the allocated kernel message
        cmd->operation = GAPM_STOP_ACTIVITY;
        cmd->actv_idx = actv_idx;

        // Send the message
        ke_msg_send(cmd);

        // Update connecting state
        //appm_update_actv_state(app_env.connect_actv_idx, APP_CONNECT_STATE_STOPPING);
    }
    else if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             app_env.state[actv_idx] > APP_CONNECT_STATE_STARTED &&
             app_env.state[actv_idx] <= APP_CONNECT_STATE_DELETING)
    {
        LOG_I("%s stopping now state %d", __func__, app_env.state[actv_idx]);
    }
    else
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_STOP_CONNECTING, actv_idx);
    }
}

static void appm_set_adv_data(uint8_t actv_idx)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           EXT_ADV_DATA_LEN);

    // Fill the allocated kernel message
    if (ADV_MODE_PERIODIC == app_env.advParam.advMode)
    {
        p_cmd->operation = GAPM_SET_PERIOD_ADV_DATA;
    }
    else
    {
        p_cmd->operation = GAPM_SET_ADV_DATA;
    }

    p_cmd->actv_idx = actv_idx;

    p_cmd->length = app_env.advParam.advDataLen;
    memcpy(p_cmd->data, app_env.advParam.advData, p_cmd->length);

    // Send the message
    ke_msg_send(p_cmd);

    if (false == app_env.need_set_rsp_data)
    {
        if (app_env.need_update_adv)
        {
            app_env.need_update_adv = false;
            appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTING);
        }
        else //if (GAPM_ADV_NON_CONN != app_env.advParam.advType)
        {
            // Update advertising state
            appm_update_actv_state(actv_idx, APP_ADV_STATE_SETTING_SCAN_RSP_DATA);
        }
    }
    else //if (GAPM_ADV_NON_CONN != app_env.advParam.advType)
    {
        // Update advertising state
        appm_update_actv_state(actv_idx, APP_ADV_STATE_SETTING_ADV_DATA);
    }
}

static void appm_set_scan_rsp_data(uint8_t actv_idx)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           EXT_ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_SCAN_RSP_DATA;
    p_cmd->actv_idx = actv_idx;

    p_cmd->length = app_env.advParam.scanRspDataLen;
    memcpy(p_cmd->data, app_env.advParam.scanRspData, p_cmd->length);

    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    if (app_env.need_update_adv)
    {
        app_env.need_update_adv = false;
        appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTING);
    }
    else
    {
        appm_update_actv_state(actv_idx, APP_ADV_STATE_SETTING_SCAN_RSP_DATA);
    }
}

/*
static void appm_send_gapm_reset_cmd(void)
{
    // Reset the stack
    struct gapm_reset_cmd *p_cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                                TASK_GAPM, TASK_APP,
                                                gapm_reset_cmd);

    p_cmd->operation = GAPM_RESET;

    ke_msg_send(p_cmd);
}
*/
void appm_adv_activity_index_init(void)
{
    for (uint8_t i = 0; i < BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        app_env.adv_actv_idx[i] = INVALID_BLE_ACTIVITY_INDEX;
    }
}

bool appm_check_adv_activity_index(uint8_t actv_idx)
{
    for (uint8_t i = 0; i < BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        if (app_env.adv_actv_idx[i] == actv_idx)
        {
            return true;
        }
    }

    LOG_I("%s %d %d %d %d", __func__,
          actv_idx,
          app_env.adv_actv_idx[0],
          app_env.adv_actv_idx[1],
          app_env.adv_actv_idx[2]);
    return false;
}

void appm_clear_adv_activity_index(uint8_t actv_idx)
{
    for (uint8_t i = 0; i < BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        if (app_env.adv_actv_idx[i] == actv_idx)
        {
            app_env.adv_actv_idx[i] = INVALID_BLE_ACTIVITY_INDEX;
            return;
        }
    }
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
//MSB->LSB
const uint8_t bes_demo_Public_key[64] =
{
    0x3E, 0x08, 0x3B, 0x0A, 0x5C, 0x04, 0x78, 0x84, 0xBE, 0x41,
    0xBE, 0x7E, 0x52, 0xD1, 0x0C, 0x68, 0x64, 0x6C, 0x4D, 0xB6,
    0xD9, 0x20, 0x95, 0xA7, 0x32, 0xE9, 0x42, 0x40, 0xAC, 0x02,
    0x54, 0x48, 0x99, 0x49, 0xDA, 0xE1, 0x0D, 0x9C, 0xF5, 0xEB,
    0x29, 0x35, 0x7F, 0xB1, 0x70, 0x55, 0xCB, 0x8C, 0x8F, 0xBF,
    0xEB, 0x17, 0x15, 0x3F, 0xA0, 0xAA, 0xA5, 0xA2, 0xC4, 0x3C,
    0x1B, 0x48, 0x60, 0xDA
};

//MSB->LSB
const uint8_t bes_demo_private_key[32] =
{
    0xCD, 0xF8, 0xAA, 0xC0, 0xDF, 0x4C, 0x93, 0x63, 0x2F, 0x48,
    0x20, 0xA6, 0xD8, 0xAB, 0x22, 0xF3, 0x3A, 0x94, 0xBF, 0x8E,
    0x4C, 0x90, 0x25, 0xB3, 0x44, 0xD2, 0x2E, 0xDE, 0x0F, 0xB7,
    0x22, 0x1F
};

void appm_init()
{
    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    appm_adv_activity_index_init();
    app_env.scan_actv_idx = INVALID_BLE_ACTIVITY_INDEX;
    app_env.connect_actv_idx = INVALID_BLE_ACTIVITY_INDEX;
    app_env.is_resolving_not_empty = false;

    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

#ifdef _BLE_NVDS_
    nv_record_blerec_init();
    nv_record_blerec_get_local_irk(app_env.loc_irk);
    TRACE(1, "%s,load local irk from nv:", __func__);
    DUMP8("%02x ", app_env.loc_irk, 16);
#else
    uint8_t counter;

    //avoid ble irk collision low probability
    uint32_t generatedSeed = hal_sys_timer_get();
    for (uint8_t index = 0; index < sizeof(bt_global_addr); index++)
    {
        generatedSeed ^= (((uint32_t)(bt_global_addr[index])) << (hal_sys_timer_get() & 0xF));
    }
    srand(generatedSeed);

    // generate a new IRK
    for (counter = 0; counter < KEY_LEN; counter++)
    {
        app_env.loc_irk[counter]    = (uint8_t)co_rand_word();
    }
#endif

#ifdef CFG_SEC_CON
    // Note: for the product that needs secure connection feature while google
    // fastpair is not included, the ble private/public key should be
    // loaded from custom parameter section
    memcpy(ble_public_key, bes_demo_Public_key, sizeof(ble_public_key));
    memcpy(ble_private_key, bes_demo_private_key, sizeof(ble_private_key));
#endif

    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/

    app_ble_mode_init();

    // load device information:
    LOG_I("%s", __func__);
#if (DISPLAY_SUPPORT)
    app_display_init();
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_SEC)
    // Security Module
    app_sec_init();
#endif // (BLE_APP_SEC)

#if (BLE_APP_HT)
    // Health Thermometer Module
    app_ht_init();
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
    // Device Information Module
    app_dis_init();
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
    // HID Module
    app_hid_init();
#endif //(BLE_APP_HID)

#if (BLE_APP_BATT)
    // Battery Module
    app_batt_init();
#endif //(BLE_APP_BATT)

#if (BLE_APP_AM0)
    // Audio Mode 0 Module
    app_am0_init();
#endif //(BLE_APP_AM0)

#if (BLE_APP_DATAPATH_SERVER)
    // Data Path Server Module
    app_datapath_server_init();
#endif //(BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_SAS_SERVER)
    // Switching Ambient Service Server Module
    app_sas_server_init();
#endif //(BLE_APP_SAS_SERVER)

#if (BLE_APP_AHP_SERVER)
    // Advanced Headphone Server Module
    app_ahps_server_init();
#endif // (BLE_APP_AHP_SERVER)

#if (BLE_APP_DATAPATH_CLIENT)
    // Data Path client Module
    app_datapath_client_init();
#endif //(BLE_APP_DATAPATH_CLIENT)

#if (BLE_APP_SWIFT)
    app_swift_init();
#endif

#ifdef BLE_HOST_PTS_TEST_ENABLED
    ble_host_pts_init();
#endif

    // Reset the stack
    //Only need to set reset CMD to host once when booting, and it will be set in btm_chip_init
    //appm_send_gapm_reset_cmd();
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }

    return more_svc;
}

uint16_t appm_get_conhdl_from_conidx(uint8_t conidx)
{
    if (BLE_INVALID_CONNECTION_INDEX == conidx)
    {
        return 0xFFFF;
    }

    return app_env.context[conidx].conhdl;
}

uint8_t appm_get_conidx_from_conhdl(uint16_t conhdl)
{
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (app_env.context[i].conhdl == conhdl)
        {
            return i;
        }
    }
    return BLE_INVALID_CONNECTION_INDEX;
}

void appm_disconnect(uint8_t conidx)
{
    ASSERT(conidx < BLE_CONNECTION_MAX, "%s conidx 0x%x", __func__, conidx);
    if (BLE_CONNECTED == app_env.context[conidx].connectStatus)
    {
        app_env.context[conidx].connectStatus = BLE_DISCONNECTING;
        struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                       TASK_GAPC,
                                                       TASK_APP,
                                                       gapc_disconnect_cmd);

        cmd->operation = GAPC_DISCONNECT;
        cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;
        cmd->conidx    = conidx;

        // Send the message
        ke_msg_send(cmd);
    }
    else
    {
        TRACE(0, "will not execute disconnect since state is:%d",
              app_env.context[conidx].connectStatus);

        ble_execute_pending_op(BLE_ACTV_ACTION_DISCONNECTING, 0xFF);
    }
}

uint8_t appm_is_connected(void)
{
    for (uint8_t index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (BLE_CONNECTED == app_env.context[index].connectStatus)
        {
            return 1;
        }
    }
    return 0;
}

// may not be applied as current ble host stack doesn't support
// multiple ble activities working at the same time?
void appm_delete_activity(uint8_t actv_idx)
{
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_ACTIVITY;
    p_cmd->actv_idx = actv_idx;

    // Send the message
    ke_msg_send(p_cmd);
}

void appm_update_actv_state(uint8_t actv_idx, enum app_actv_state newState)
{
    ASSERT(actv_idx < BLE_ACTIVITY_MAX, "%s actv_idx %d newState %d call %p", __func__, actv_idx, newState, __builtin_return_address(0));
    enum app_actv_state oldState = app_env.state[actv_idx];
    if (oldState == APP_ACTV_STATE_IDLE &&
            (newState == APP_ADV_STATE_CREATING ||
             newState == APP_SCAN_STATE_CREATING ||
             newState == APP_CONNECT_STATE_CREATING))
    {
    }
    else if (newState == APP_ACTV_STATE_IDLE &&
             (oldState == APP_ADV_STATE_DELETING ||
              oldState == APP_SCAN_STATE_DELETING ||
              oldState == APP_CONNECT_STATE_DELETING))
    {
    }
    else if (oldState == APP_ADV_STATE_STARTED &&
             (newState == APP_ADV_STATE_SETTING_ADV_DATA ||
              newState == APP_ADV_STATE_SETTING_SCAN_RSP_DATA ||
              newState == APP_ADV_STATE_STARTING))
    {
    }
    else if (newState == APP_ADV_STATE_STARTING && oldState == APP_ADV_STATE_SETTING_ADV_DATA)
    {
    }
    else if (oldState == APP_ADV_STATE_STARTING && newState == APP_ADV_STATE_DELETING)
    {
    }
    else if (oldState == APP_SCAN_STATE_STARTING && newState == APP_SCAN_STATE_DELETING)
    {
    }
    else if (oldState == APP_ADV_STATE_CREATING &&
             (newState == APP_ADV_STATE_STARTING ||
              newState == APP_ADV_STATE_SETTING_SCAN_RSP_DATA))
    {
    }
    else if (oldState == APP_SCAN_STATE_STARTED && newState == APP_SCAN_STATE_STARTING)
    {
    }
    else if (oldState == APP_CONNECT_STATE_STARTING && newState == APP_CONNECT_STATE_DELETING)
    {
    }
    else if ((newState - oldState) == 1)
    {
    }
    else
    {
        ASSERT(0, "%s actv_idx %d oldState %d newState %d call %p", __func__, actv_idx, oldState, newState, __builtin_return_address(0));
    }

    TRACE(0, "actv_idx %d actv state from %d to %d", actv_idx, app_env.state[actv_idx], newState);
    app_env.state[actv_idx] = newState;
}

void appm_actv_fsm_next(uint8_t actv_idx, uint8_t status)
{
    ASSERT(actv_idx < BLE_ACTIVITY_MAX, "%s actv_idx %d call %p", __func__, actv_idx, __builtin_return_address(0));
    switch (app_env.state[actv_idx])
    {
        case (APP_ADV_STATE_CREATING):
        {
            if ((app_env.advParam.advType == ADV_TYPE_DIRECT_LDC) ||
                    (app_env.advParam.advType == ADV_TYPE_DIRECT_HDC))
            {
                // kick off advertising activity
                appm_kick_off_advertising(actv_idx);
            }
            else
            {
                // Set advertising data
                appm_set_adv_data(actv_idx);
            }
        }
        break;

        case (APP_ADV_STATE_SETTING_ADV_DATA):
        {
            // Set scan response data
            appm_set_scan_rsp_data(actv_idx);
        }
        break;

        case (APP_ADV_STATE_SETTING_SCAN_RSP_DATA):
        {
            // kick off advertising activity
            appm_kick_off_advertising(actv_idx);
        }
        break;

        case (APP_ADV_STATE_STARTING):
        {
            if (GAP_ERR_NO_ERROR == status)
            {
                // Go to started state
                appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTED);
                app_advertising_started(actv_idx);
            }
            else
            {
                appm_update_actv_state(actv_idx, APP_ADV_STATE_DELETING);
                app_advertising_starting_failed(actv_idx, status);
                appm_delete_activity(actv_idx);
            }
            ble_execute_pending_op(BLE_ACTV_ACTION_STARTING_ADV, actv_idx);
        }
        break;

        case (APP_ADV_STATE_STOPPING):
        {
            ASSERT(appm_check_adv_activity_index(actv_idx), "%s error actv_idx %d", __func__, actv_idx);
            // Go next state
            appm_update_actv_state(actv_idx, APP_ADV_STATE_DELETING);
            appm_delete_activity(actv_idx);
        }
        break;
        case (APP_ADV_STATE_DELETING):
        {
            // Go next state
            appm_update_actv_state(actv_idx, APP_ACTV_STATE_IDLE);
            app_advertising_stopped(actv_idx);
            appm_clear_adv_activity_index(actv_idx);
            if (app_env.need_restart_adv)
            {
                app_env.need_restart_adv = false;
                appm_start_advertising(&app_env.advParam);
            }
            else
            {
                ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);

                TRACE(0, "%s, need_set_white_list %d, isNeedToAddDevicesToReslovingList %d", __func__, app_env.need_set_white_list, app_env.isNeedToAddDevicesToReslovingList);

                if (app_env.need_set_white_list)
                {
                    appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
                }
                else if (app_env.isNeedToAddDevicesToReslovingList)
                {
                    appm_set_rpa_list(&app_env.devicesInfoAddedToResolvingList[0],
                                      app_env.numberOfDevicesAddedToResolvingList);
                }
            }
        }
        break;

        case (APP_SCAN_STATE_CREATING):
        {
            // kick off scanning activity
            appm_kick_off_scanning();
        }
        break;

        case (APP_SCAN_STATE_STARTING):
        {
            if (GAP_ERR_NO_ERROR == status)
            {
                // Go to started state
                appm_update_actv_state(actv_idx, APP_SCAN_STATE_STARTED);
                app_scanning_started();
            }
            else
            {
                appm_update_actv_state(actv_idx, APP_SCAN_STATE_DELETING);
                appm_delete_activity(actv_idx);
                app_scanning_starting_failed(actv_idx, status);
            }
            ble_execute_pending_op(BLE_ACTV_ACTION_STARTING_SCAN, actv_idx);
        }
        break;

        case (APP_SCAN_STATE_STOPPING):
        {
            ASSERT(actv_idx == app_env.scan_actv_idx, "%s actv_idx %d scan_idx %d", __func__, actv_idx, app_env.scan_actv_idx);
            // Go next state
            appm_update_actv_state(actv_idx, APP_SCAN_STATE_DELETING);
            appm_delete_activity(actv_idx);
        }
        break;
        case (APP_SCAN_STATE_DELETING):
        {
            // Go created state
            appm_update_actv_state(actv_idx, APP_ACTV_STATE_IDLE);
            app_env.scan_actv_idx = INVALID_BLE_ACTIVITY_INDEX;
            if (app_env.need_restart_scan)
            {
                app_env.need_restart_scan = false;
                appm_start_scanning(&app_env.ble_scan_param);
            }
            else
            {
                ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_SCAN, actv_idx);
                app_scanning_stopped();

                if (app_env.isNeedToAddDevicesToReslovingList)
                {
                    appm_set_rpa_list(&app_env.devicesInfoAddedToResolvingList[0],
                                      app_env.numberOfDevicesAddedToResolvingList);
                }
                if (app_env.need_set_white_list)
                {
                    appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
                }
            }
        }
        break;

        case (APP_CONNECT_STATE_CREATING):
        {
            // kick off connecting activity
            appm_kick_off_connecting();
        }
        break;
        case (APP_CONNECT_STATE_STARTING):
        {
            if (GAP_ERR_NO_ERROR == status)
            {
                // Go to started state
                appm_update_actv_state(actv_idx, APP_CONNECT_STATE_STARTED);
            }
            else
            {
                appm_update_actv_state(actv_idx, APP_CONNECT_STATE_DELETING);
                appm_delete_activity(actv_idx);
                app_connecting_failed(&app_env.ble_init_param.peer_addr, status);
            }
            ble_execute_pending_op(BLE_ACTV_ACTION_CONNECTING, actv_idx);
        }
        break;
        case (APP_CONNECT_STATE_STOPPING):
        {
            ASSERT(actv_idx == app_env.connect_actv_idx, "%s actv_idx %d conn_idx %d", __func__, actv_idx, app_env.connect_actv_idx);
            // Go next state
            appm_update_actv_state(actv_idx, APP_CONNECT_STATE_DELETING);
            appm_delete_activity(actv_idx);
        }
        break;
        case (APP_CONNECT_STATE_DELETING):
        {
            // Go created state
            appm_update_actv_state(actv_idx, APP_ACTV_STATE_IDLE);
            app_env.connect_actv_idx = INVALID_BLE_ACTIVITY_INDEX;

            if (app_env.need_restart_connect)
            {
                app_env.need_restart_connect = false;
                appm_start_connecting(&app_env.ble_init_param);
            }
            else
            {
                ble_execute_pending_op(BLE_ACTV_ACTION_STOP_CONNECTING, actv_idx);

                if (app_env.isNeedToAddDevicesToReslovingList)
                {
                    appm_set_rpa_list(&app_env.devicesInfoAddedToResolvingList[0],
                                      app_env.numberOfDevicesAddedToResolvingList);
                }
                if (app_env.need_set_white_list)
                {
                    appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
                }
            }
        }
        break;

        default:
        {
            ASSERT_ERR(0);
        }
        break;
    }
}


/**
 ****************************************************************************************
 * Advertising Functions
 ****************************************************************************************
 */
void appm_start_advertising(void *param)
{
    BLE_APP_FUNC_ENTER();

    BLE_ADV_PARAM_T *pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_idx = app_env.adv_actv_idx[pAdvParam->adv_actv_user];
    app_env.advParam = *pAdvParam;
    app_env.need_set_rsp_data = false;
    if (actv_idx == INVALID_BLE_ACTIVITY_INDEX)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_adv_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

        // Fill the allocated kernel message
        p_cmd->type = pAdvParam->advMode;;
        p_cmd->adv_param.max_tx_pwr = btdrv_reg_op_txpwr_idx_to_rssidbm(pAdvParam->advTxPwr);
        p_cmd->adv_param.disc_mode = pAdvParam->discMode;// GAPM_ADV_MODE_GEN_DISC;
        p_cmd->adv_param.with_flags = !(pAdvParam->isBleFlagsAdvDataConfiguredByAppLayer);

        switch (pAdvParam->advType)
        {
            case ADV_TYPE_UNDIRECT:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
                break;
            case ADV_TYPE_DIRECT_LDC:
                // Device must be non discoverable and non scannable if directed advertising is used
                p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
                p_cmd->adv_param.prop = GAPM_ADV_PROP_DIR_CONN_LDC_MASK;
                break;
            case ADV_TYPE_DIRECT_HDC:
                // Device must be non discoverable and non scannable if directed advertising is used
                p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
                p_cmd->adv_param.prop = GAPM_ADV_PROP_DIR_CONN_HDC_MASK;
                break;
            case ADV_TYPE_CONN_EXT_ADV:
                p_cmd->adv_param.prop = GAPM_EXT_ADV_PROP_UNDIR_CONN_MASK;
                break;
            case ADV_TYPE_NON_CONN_SCAN:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_NON_CONN_SCAN_MASK;
                break;
            case ADV_TYPE_NON_CONN_NON_SCAN:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK;
                break;
            case ADV_TYPE_EXT_CON_DIRECT:
                // Device must be non discoverable and non scannable if directed advertising is used
                p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
                p_cmd->adv_param.prop = GAPM_EXT_ADV_PROP_DIR_CONN_MASK;
                break;
            default:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
                break;
        }

        p_cmd->own_addr_type = pAdvParam->localAddrType;
        //Host adv must set a arbitrary peer device address in cmd when using RPA,
        //so that Controller can find local IRK through address Host set in Adv cmd!!!
#ifdef CUSTOMER_DEFINE_ADV_DATA
        if (p_cmd->own_addr_type == GAPM_GEN_RSLV_ADDR)
        {
            memcpy(p_cmd->adv_param.local_addr.addr, pAdvParam->localAddr, GAP_BD_ADDR_LEN);
        }
        else if (p_cmd->own_addr_type == GAPM_STATIC_ADDR)
        {
            memcpy(p_cmd->adv_param.local_addr.addr, bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
        }
#else
#ifdef BLE_ADV_RPA_ENABLED
        uint8_t ADDR_FF[GAP_BD_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        memcpy(p_cmd->adv_param.local_addr.addr, ADDR_FF, GAP_BD_ADDR_LEN);
#else
        memcpy(p_cmd->adv_param.local_addr.addr, bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
#endif
#endif
        memcpy(p_cmd->adv_param.peer_addr.addr, pAdvParam->peerAddr.addr, GAP_BD_ADDR_LEN);
        p_cmd->adv_param.peer_addr.addr_type = pAdvParam->peerAddr.addr_type;
        LOG_I("[PEER ADDR]: %d", p_cmd->adv_param.peer_addr.addr_type);
        DUMP8("%02x ", p_cmd->adv_param.peer_addr.addr, BT_ADDR_OUTPUT_PRINT_NUM);

        p_cmd->adv_param.filter_pol = pAdvParam->filter_pol;
        ///channel 37 38 and 39 will be used together
        p_cmd->adv_param.prim_cfg.chnl_map = APP_ADV_CHMAP;
        ///primary advertising_PHY contains two class:GAP_PHY_LE_1MBPS or GAP_PHY_LE_CODED
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;
#if !defined(USE_MS_AS_BLE_ADV_INTERVAL_UNIT)
        p_cmd->adv_param.prim_cfg.adv_intv_min = pAdvParam->advInterval;
        p_cmd->adv_param.prim_cfg.adv_intv_max = pAdvParam->advInterval;
#else
        p_cmd->adv_param.prim_cfg.adv_intv_min = pAdvParam->advInterval * 8 / 5;
        p_cmd->adv_param.prim_cfg.adv_intv_max = pAdvParam->advInterval * 8 / 5;
#endif

        ///if the adv_mode is not legacy, the second _cfg will set
        if (p_cmd->type != ADV_MODE_LEGACY)
        {
            p_cmd->second_cfg.phy = GAP_PHY_2MBPS;

            ///adv_sid is the Value of the Advertising SID subfield in the ADI field of the PDU
            p_cmd->second_cfg.adv_sid = (uint8_t)pAdvParam->adv_actv_user;

            ///AUX_ADV_IND shall be sent prior to the next advertising event,
            ///Maximum advertising events the Controller can skip before sending the
            ///AUX_ADV_IND on the secondary advertising physical channel
            p_cmd->second_cfg.max_skip = 0;
        }

        if (p_cmd->type == ADV_MODE_PERIODIC)
        {
            p_cmd->period_cfg.interval_min = pAdvParam->PeriodicIntervalMin;
            p_cmd->period_cfg.interval_max = pAdvParam->PeriodicIntervalMax;
        }

        ///if the scan respnse data is not empty, the flag value of need_set_rsp_data is true, init value is false.
        if (app_env.advParam.scanRspDataLen &&
                ((GAPM_ADV_PROP_NON_CONN_SCAN_MASK == p_cmd->adv_param.prop) || (GAPM_ADV_PROP_UNDIR_CONN_MASK == p_cmd->adv_param.prop)))
        {
            app_env.need_set_rsp_data = true;
        }
        else
        {
            app_env.need_set_rsp_data = false;
        }

        ///check if the legacy adv data length exceed
        if (GAPM_ADV_TYPE_LEGACY == p_cmd->type)
        {
            TRACE(1, "enter the adv_param.type:%d,%d", p_cmd->type, pAdvParam->advDataLen);
            if (pAdvParam->isBleFlagsAdvDataConfiguredByAppLayer)
            {
                ASSERT(pAdvParam->advDataLen <= BLE_ADV_DATA_WITHOUT_FLAG_LEN, "adv data exceed without flag");
            }
            else
            {
                ASSERT(pAdvParam->advDataLen <= BLE_ADV_DATA_WITH_FLAG_LEN, "adv data exceed with flag");
            }
            ASSERT(pAdvParam->scanRspDataLen <= SCAN_RSP_DATA_LEN, "scan rsp data exceed");
        }

        ///check if the own_addr is private
        LOG_I("[ADDR_TYPE] own_addr_type:%d, adv_type:%d, interval:%d",
              p_cmd->own_addr_type, pAdvParam->advType, pAdvParam->advInterval);

#ifdef BLE_ADV_RPA_ENABLED
        LOG_I("[IRK]:");
        ///if the own_addr_type is RSLV, it need to get the IRK,KEY_LEN is 0x10
        DUMP8("0x%02x ", appm_get_current_ble_irk(), KEY_LEN);
#endif

        if (GAPM_GEN_RSLV_ADDR != p_cmd->own_addr_type)
        {
            LOG_I("[ADDR]:%02x:%02x:*:*:*:%02x", p_cmd->adv_param.local_addr.addr[0],
                  p_cmd->adv_param.local_addr.addr[1], p_cmd->adv_param.local_addr.addr[5]);
        }

        LOG_I("[ADV_TYPE]:%d", pAdvParam->advType);
        LOG_I("[ADV_INTERVAL]:%d", pAdvParam->advInterval);

        // Send the message
        ke_msg_send(p_cmd);

        // Set the state of the task to APPM_ADVERTISING
        ke_state_set(TASK_APP, APPM_ADVERTISING);
    }
    else
    {
        app_env.need_restart_adv = true;
        appm_stop_advertising(actv_idx);
    }

    BLE_APP_FUNC_LEAVE();
}

void appm_start_connecting(BLE_INIT_PARAM_T *init_param)
{
    if (init_param->own_addr_type > APP_GAPM_GEN_NON_RSLV_ADDR)
    {
        LOG_I("%s max_type=%d, para_type=%d", __func__, APP_GAPM_GEN_NON_RSLV_ADDR, init_param->own_addr_type);
        return;
    }

    if (GAPM_INIT_TYPE_DIRECT_CONN_EST == init_param->gapm_init_type &&
            app_ble_is_connection_on_by_addr(init_param->peer_addr.addr))
    {
        LOG_I("%s ble has connect", __func__);
        ble_execute_pending_op(BLE_ACTV_ACTION_CONNECTING, 0xFF);
        return;
    }

    app_env.ble_init_param = *init_param;

    if (app_env.connect_actv_idx == INVALID_BLE_ACTIVITY_INDEX)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_create_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_INIT_ACTIVITY;

        // Fill the allocated kernel message
        p_cmd->own_addr_type = init_param->own_addr_type;

        // Send the message
        ke_msg_send(p_cmd);
    }
    else if (APP_CONNECT_STATE_STARTED == app_env.state[app_env.connect_actv_idx])
    {
        app_env.need_restart_connect = true;
        appm_stop_connecting();
    }
    else if (APP_CONNECT_STATE_STOPPING == app_env.state[app_env.connect_actv_idx] ||
             APP_CONNECT_STATE_DELETING == app_env.state[app_env.connect_actv_idx])
    {
        app_env.need_restart_connect = true;
    }
}

void appm_start_scanning(BLE_SCAN_PARAM_T *param)
{
    uint32_t oldFiltPolicy = param->scanFolicyType;

    app_env.ble_scan_param.scanType       = param->scanType;
    app_env.ble_scan_param.scanFolicyType = param->scanFolicyType;
    app_env.ble_scan_param.scanIntervalMs = param->scanIntervalMs;
    app_env.ble_scan_param.scanWindowMs   = param->scanWindowMs;
    app_env.ble_scan_param.scanDurationMs = param->scanDurationMs;
    if (app_env.scan_actv_idx == INVALID_BLE_ACTIVITY_INDEX)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_create_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_SCAN_ACTIVITY;

        // Fill the allocated kernel message
        if (app_env.ble_scan_param.scanFolicyType & BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA)
        {
            p_cmd->own_addr_type = GAPM_GEN_RSLV_ADDR;
        }
        else
        {
            p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        }

        // Send the message
        ke_msg_send(p_cmd);
    }
    else
    {
        if ((app_env.ble_scan_param.scanFolicyType ^ oldFiltPolicy) & BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA)
        {
            app_env.need_restart_scan = true;
            appm_stop_scanning();
        }
        else
        {
            appm_kick_off_scanning();
        }
    }
}

void appm_update_param(uint8_t conidx, uint32_t min_interval, uint32_t max_interval,
                       uint32_t time_out, uint8_t  slaveLatency)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     TASK_GAPC,
                                                     TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = (uint16_t)min_interval;
    cmd->intv_max   = (uint16_t)max_interval;
    cmd->latency    = slaveLatency;
    cmd->time_out   = (uint16_t)(time_out / 10);
    cmd->conidx     = conidx;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}

#ifdef CFG_LE_PWR_CTRL
void appm_set_path_loss_rep_param_cmd(uint8_t conidx, uint8_t enable, uint8_t high_threshold,
                                      uint8_t high_hysteresis, uint8_t low_threshold,
                                      uint8_t low_hysteresis, uint8_t min_time)
{
    gapc_path_loss_ctrl_cmd_t *cmd = KE_MSG_ALLOC(GAPC_PATH_LOSS_CTRL_CMD,
                                                  TASK_GAPC, TASK_APP,
                                                  gapc_path_loss_ctrl_cmd);

    cmd->conidx = conidx;
    cmd->operation = GAPC_PATH_LOSS_REPORT_CTRL;
    cmd->enable = enable;
    cmd->high_threshold = high_threshold;
    cmd->high_hysteresis = high_hysteresis;
    cmd->low_threshold = low_threshold;
    cmd->low_hysteresis = low_hysteresis;
    cmd->min_time = min_time;

    // Send the message
    ke_msg_send(cmd);
}
#endif

void l2cap_update_param_in_standard_unit(uint8_t conidx,
                                         uint32_t min_interval, uint32_t max_interval,
                                         uint32_t supTimeout_in_ms, uint8_t  slaveLatency)
{
#ifdef __GATT_OVER_BR_EDR__
    if (btif_btgatt_is_connected_by_conidx(conidx))
    {
        LOG_I("l2cap_update_param failed because of btgatt conn!");
        return;
    }
#endif
    if (!app_ble_push_update_ci_list(conidx, min_interval, max_interval,
                                     supTimeout_in_ms, slaveLatency))
    {
        // The same CI param is in progress or is already the same as in use
        LOG_I("(d%d)%s, no need to update CI", conidx, __func__);
        return;
    }

    appm_update_param(conidx, min_interval, max_interval, supTimeout_in_ms, slaveLatency);
}

void l2cap_update_param(uint8_t  conidx,
                        uint32_t min_interval_in_ms, uint32_t max_interval_in_ms,
                        uint32_t supTimeout_in_ms, uint8_t  slaveLatency)
{
    l2cap_update_param_in_standard_unit(conidx, (uint32_t)(min_interval_in_ms * 4 / 5),
                                        (uint32_t)(max_interval_in_ms * 4 / 5), supTimeout_in_ms, slaveLatency);
}

int8_t appm_get_dev_name(uint8_t *name, uint16_t offset)
{
    uint16_t len = 0;
    // Avoid memory overwrite
    if (app_env.dev_name_len >= offset)
    {
        len = app_env.dev_name_len - offset;
        if (len > APP_DEVICE_NAME_MAX_LEN)
        {
            len = APP_DEVICE_NAME_MAX_LEN;
        }
        memcpy(name, app_env.dev_name + offset, len);
        return len;
    }
    else
    {
        return -1;
    }
}

void appm_exchange_mtu(uint8_t conidx)
{
    // TODO: update to new API
    /*
    struct gattc_exc_mtu_cmd *cmd = KE_MSG_ALLOC(GATTC_EXC_MTU_CMD,
                                                 KE_BUILD_ID(TASK_GATTC, conidx),
                                                 TASK_APP,
                                                 gattc_exc_mtu_cmd);

    cmd->operation = GATTC_MTU_EXCH;
    cmd->seq_num= 0;

    ke_msg_send(cmd);
    */
}

void appm_check_and_resolve_ble_address(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

#ifdef __GATT_OVER_BR_EDR__
    if (btif_btgatt_is_connected_by_conidx(conidx))
    {
        pContext->isGotSolvedBdAddr = true;
        pContext->isBdAddrResolvingInProgress = false;
        btif_btgatt_get_device_address(conidx, pContext->solvedBdAddr);
    }
#endif

    // solved already, return
    if (pContext->isGotSolvedBdAddr)
    {
        LOG_I("Already get solved bd addr.");
        return;
    }
    // not solved yet and the solving is in progress, return and wait
    else if (app_is_resolving_ble_bd_addr())
    {
        LOG_I("Random bd addr solving on going.");
        return;
    }

    if (BLE_RANDOM_ADDR == pContext->peerAddrType)
    {
        memset(pContext->solvedBdAddr, 0, BD_ADDR_LEN);
        bool isSuccessful = appm_resolve_random_ble_addr_from_nv(conidx, pContext->bdAddr);
        LOG_I("%s isSuccessful %d", __func__, isSuccessful);
        if (isSuccessful)
        {
            pContext->isBdAddrResolvingInProgress = true;
        }
        else
        {
            pContext->isGotSolvedBdAddr = true;
            pContext->isBdAddrResolvingInProgress = false;
        }
    }
    else
    {
        pContext->isGotSolvedBdAddr = true;
        pContext->isBdAddrResolvingInProgress = false;
        memcpy(pContext->solvedBdAddr, pContext->bdAddr, BD_ADDR_LEN);
    }

}

bool appm_resolve_random_ble_addr_from_nv(uint8_t conidx, uint8_t *randomAddr)
{
#ifdef _BLE_NVDS_
    struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                                                        KE_BUILD_ID(TASK_GAPM, conidx),
                                                        TASK_APP,
                                                        gapm_resolv_addr_cmd,
                                                        BLE_RECORD_NUM * GAP_KEY_LEN);

    uint8_t irkeyNum = nv_record_ble_fill_irk((uint8_t *)(cmd->irk));
    if (0 == irkeyNum)
    {
        LOG_I("No history irk, cannot solve bd addr.");
        KE_MSG_FREE(cmd);
        return false;
    }

    LOG_I("Start random bd addr solving, irk_nb=%d", irkeyNum);
    DUMP8("%02x ", cmd->irk[0].key, 16);

    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = irkeyNum;
    memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
    ke_msg_send(cmd);
    return true;
#else
    return false;
#endif

}

void appm_resolve_random_ble_addr_with_sepcific_irk(uint8_t conidx, uint8_t *randomAddr, uint8_t *pIrk)
{
    struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                                                        KE_BUILD_ID(TASK_GAPM, conidx), TASK_APP,
                                                        gapm_resolv_addr_cmd,
                                                        GAP_KEY_LEN);
    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = 1;
    memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
    memcpy(cmd->irk, pIrk, GAP_KEY_LEN);
    ke_msg_send(cmd);
}

void appm_random_ble_addr_solved(bool isSolvedSuccessfully, uint8_t *irkUsedForSolving)
{
    APP_BLE_CONN_CONTEXT_T *pContext;
    uint32_t conidx;
    for (conidx = 0; conidx < BLE_CONNECTION_MAX; conidx++)
    {
        pContext = &(app_env.context[conidx]);
        if (pContext->isBdAddrResolvingInProgress)
        {
            break;
        }
    }

    if (conidx < BLE_CONNECTION_MAX)
    {
        pContext->isBdAddrResolvingInProgress = false;
        pContext->isGotSolvedBdAddr = true;

        LOG_I("%s conidx %d isSolvedSuccessfully %d", __func__, conidx, isSolvedSuccessfully);
        if (isSolvedSuccessfully)
        {
#ifdef _BLE_NVDS_
            bool isSuccessful = nv_record_blerec_get_bd_addr_from_irk(app_env.context[conidx].solvedBdAddr, irkUsedForSolving);
            if (isSuccessful)
            {
                LOG_I("[CONNECT]Connected random address's original addr is:");
                DUMP8("%02x ", app_env.context[conidx].solvedBdAddr, BT_ADDR_OUTPUT_PRINT_NUM);
                app_ble_rpa_addr_parsed_success(conidx);
            }
            else
#endif
            {
                LOG_I("[CONNECT]Resolving of the connected BLE random addr failed.");
            }
        }
        else
        {
            LOG_I("[CONNECT]random resolving failed.");
        }
    }

    ke_task_msg_retrieve(TASK_GAPC);
    ke_task_msg_retrieve(TASK_APP);

    app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
}

void appm_le_set_rpa_timeout(uint16_t rpa_timeout)
{
    struct gapm_set_rpa_timeot_cmd *p_cmd =  KE_MSG_ALLOC(GAPM_LE_SET_RPA_TIMEOUT_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_set_rpa_timeot_cmd);

    p_cmd->operation = GAPM_SET_RPA_TIMEOUT;
    p_cmd->rpa_timeout = rpa_timeout;

    ke_msg_send(p_cmd);

}

uint8_t app_ble_get_actv_state(uint8_t actv_idx)
{
    return app_env.state[actv_idx];
}

uint8_t app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    uint8_t actv_idx = app_ble_get_user_activity_idx(actv_user);

    return app_env.state[actv_idx];
}

uint8_t app_ble_connection_count(void)
{
    return app_env.conn_cnt;
}

bool app_is_arrive_at_max_ble_connections(void)
{
    return (app_env.conn_cnt >= BLE_CONNECTION_MAX);
}

bool app_is_resolving_ble_bd_addr(void)
{
    for (uint32_t index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (app_env.context[index].isBdAddrResolvingInProgress)
        {
            return true;
        }
    }

    return false;
}

void app_trigger_ble_service_discovery(uint8_t conidx, uint16_t shl, uint16_t ehl)
{
    // TODO: update to new API
    /*
    struct gattc_send_svc_changed_cmd *cmd= KE_MSG_ALLOC(GATTC_SEND_SVC_CHANGED_CMD,\
                                                            KE_BUILD_ID(TASK_GATTC, conidx),\
                                                            TASK_APP,\
                                                            gattc_send_svc_changed_cmd);
    cmd->operation = GATTC_SVC_CHANGED;
    cmd->svc_shdl = shl;
    cmd->svc_ehdl = ehl;
    ke_msg_send(cmd);
    */
}

void appm_update_adv_data(void *param)
{
    // TODO: update to new API
    BLE_ADV_PARAM_T *pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_idx = app_env.adv_actv_idx[pAdvParam->adv_actv_user];
    app_env.need_set_rsp_data = false;

    ASSERT(actv_idx != INVALID_BLE_ACTIVITY_INDEX,
           "%s adv idx %d", __func__, actv_idx);
    ASSERT(app_env.state[actv_idx] == APP_ADV_STATE_STARTED,
           "%s state %d", __func__, app_env.state[actv_idx]);

    // if scan rsp data change, need set rsp data again
    if ((pAdvParam->scanRspDataLen != app_env.advParam.scanRspDataLen ||
            memcmp(pAdvParam->scanRspData, app_env.advParam.scanRspData, pAdvParam->scanRspDataLen)) &&
            pAdvParam->scanRspDataLen != 0)
    {
        app_env.need_set_rsp_data = true;
    }

    app_env.advParam = *pAdvParam;
    app_env.need_update_adv = true;

    appm_set_adv_data(actv_idx);
}

int8_t appm_ble_get_rssi(uint8_t conidx)
{
    int8_t rssi = 127;
    rx_agc_t tws_agc = {0};

    if (conidx != BLE_INVALID_CONNECTION_INDEX)
    {
        rssi = bt_drv_reg_op_read_ble_rssi_in_dbm(conidx, &tws_agc);
        rssi = bt_drv_reg_op_rssi_correction(tws_agc.rssi);
        //TRACE(1," headset to mobile RSSI:%d dBm",rssi);
    }

    return rssi;
}

void appm_get_tx_power(uint8_t conidx, ble_tx_object_e object, ble_phy_pwr_value_e phy)
{
    struct gapc_get_info_cmd *cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                 TASK_GAPC,
                                                 TASK_APP,
                                                 gapc_get_info_cmd);

    cmd->conidx    = conidx;
    if (object == BLE_TX_LOCAL)
    {
        cmd->operation = GAPC_GET_LOC_TX_PWR_LEVEL_1M;
    }
    else
    {
        cmd->operation = GAPC_GET_PEER_TX_PWR_LEVEL_1M;
    }
    cmd->operation += phy;

    // Send the message
    ke_msg_send(cmd);
}

void appm_tx_power_report_enable(uint8_t conidx, bool local_enable, bool remote_enable)
{
    struct gapc_tx_pwr_report_ctrl_cmd *cmd = KE_MSG_ALLOC(GAPC_TX_PWR_REPORT_CTRL_CMD,
                                                           TASK_GAPC,
                                                           TASK_APP,
                                                           gapc_tx_pwr_report_ctrl_cmd);

    cmd->conidx    = conidx;
    cmd->operation = GAPC_TX_PWR_REPORT_CTRL;
    cmd->local_en  = local_enable;
    cmd->remote_en = remote_enable;

    // Send the message
    ke_msg_send(cmd);
}

void appm_subrate_interface(uint8_t conidx, uint16_t subrate_min, uint16_t subrate_max,
                            uint16_t latency_max, uint16_t cont_num, uint16_t timeout)
{
    struct gapc_subrate_cmd *cmd = KE_MSG_ALLOC(GAPC_SUBRATE_CTRL_CMD,
                                                TASK_GAPC,
                                                TASK_APP,
                                                gapc_subrate_cmd);

    cmd->conidx       = conidx;
    if (conidx == INVALID_BLE_CONIDX)
    {
        cmd->operation    = GAPC_SET_DEFAULT_SUBRATE;
    }
    else
    {
        cmd->operation    = GAPC_SUBRATE_REQUEST;
    }
    cmd->subrate_min  = subrate_min;
    cmd->subrate_max  = subrate_max;
    cmd->latency_max  = latency_max;
    cmd->cont_num     = cont_num;
    cmd->timeout      = timeout;

    // Send the message
    ke_msg_send(cmd);
}

#if  (mHDT_LE_SUPPORT)
void app_task_mhdt_rd_local_proprietary_feat(void)
{
    mhdt_le_cmd_t *p_cmd = KE_MSG_ALLOC(MHDT_LE_CMD, TASK_MHDT_LE, TASK_APP, mhdt_le_cmd);

    p_cmd->cmd_code = MHDT_LE_RD_LOCAL_PROPRIETARY_FEAT_CMD;

    ke_msg_send(p_cmd);
}

void app_task_mhdt_rd_remote_proprietary_feat(uint8_t conidx)
{
    if (app_env.context[conidx].connectStatus != BLE_CONNECTED)
    {
        LOG_E("%s ble %d is not connected", __func__, conidx);
        return;
    }

    mhdt_le_cmd_t *p_cmd = (mhdt_le_cmd_t *)ke_msg_alloc(MHDT_LE_CMD, TASK_MHDT_LE, TASK_APP, sizeof(mhdt_le_cmd_t) + sizeof(uint16_t));

    p_cmd->cmd_code = MHDT_LE_RD_RM_PROPRIETARY_FEAT_CMD;
    *(uint8_t *)(&p_cmd->param[0]) = conidx;

    ke_msg_send(p_cmd);
}

void app_task_mhdt_mabr_set_dft_label_params(void *params)
{
    if (params == NULL)
    {
        LOG_E("%s NULL params", __func__);
        return;
    }

    params = (mhdt_mabr_dft_label_params_t *)params;

    mhdt_le_cmd_t *p_cmd = (mhdt_le_cmd_t *)ke_msg_alloc(MHDT_LE_CMD, TASK_MHDT_LE, TASK_APP, sizeof(mhdt_le_cmd_t) + sizeof(mhdt_mabr_dft_label_params_t));

    p_cmd->cmd_code = MHDT_LE_MABR_SET_DFT_LABEL_PARAMS_CMD;
    memcpy(&p_cmd->param[0], params, sizeof(mhdt_mabr_dft_label_params_t));

    ke_msg_send(p_cmd);
}
#endif

__attribute__((weak)) void app_ble_connected_evt_handler(uint8_t conidx, ble_bdaddr_t *pPeerBdAddress)
{

}

__attribute__((weak)) void app_ble_disconnected_evt_handler(uint8_t conidx, uint8_t errCode)
{

}

__attribute__((weak)) void app_advertising_stopped(uint8_t actv_idx)
{

}

__attribute__((weak)) void app_advertising_started(uint8_t actv_idx)
{

}

__attribute__((weak)) void app_connecting_started(void)
{

}

__attribute__((weak)) void app_scanning_stopped(void)
{

}

__attribute__((weak)) void app_connecting_stopped(ble_bdaddr_t *peer_addr)
{

}


__attribute__((weak)) void app_scanning_started(void)
{

}

__attribute__((weak)) void app_ble_system_ready(void)
{

}

__attribute__((weak)) void app_adv_reported_scanned(struct gapm_ext_adv_report_ind *ptInd)
{

}

__attribute__((weak)) void app_ble_update_param_failed(uint8_t conidx, uint8_t errCode)
{

}

__attribute((weak)) void app_ble_on_bond_status_changed(uint8_t conidx, bool success, uint16_t reason)
{

}

__attribute((weak))void app_ble_on_encrypt_success(uint8_t conidx, uint8_t pairing_lvl)
{

}

uint8_t *appm_get_current_ble_addr(void)
{
#ifdef BLE_ADV_RPA_ENABLED
    return (uint8_t *)appm_get_local_rpa_addr();
#else
    return ble_global_addr;
#endif
}

void appm_read_rpa_addr(void)
{
#ifdef BLE_ADV_RPA_ENABLED
    ble_bdaddr_t  peer_identify_address;

    peer_identify_address.addr_type = GAPM_STATIC_ADDR;
    memcpy(peer_identify_address.addr, (uint8_t *)bt_get_ble_local_address(), GAP_BD_ADDR_LEN);

    gapm_read_rpa_addr_cmd(&peer_identify_address);
#endif
}

uint8_t *appm_get_local_rpa_addr(void)
{
    return (uint8_t *)gapm_get_rpa_bdaddr();
}

uint8_t *appm_get_local_identity_ble_addr(void)
{
    return ble_global_addr;
}

// bit mask of the existing conn param modes
static uint32_t existingBleConnParamModes[BLE_CONNECTION_MAX] = {0};

// interval in the unit of 1.25ms
static const BLE_CONN_PARAM_CONFIG_T ble_conn_param_config[] =
{
    // default value: for the case of BLE just connected and the BT idle state
    {BLE_CONN_PARAM_MODE_DEFAULT, BLE_CONN_PARAM_PRIORITY_NORMAL, 36, 36, 0},

    {BLE_CONN_PARAM_MODE_AI_STREAM_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL1, 16, 16, 0},

    {BLE_CONN_PARAM_MODE_A2DP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL0, 48, 48, 0},

    {BLE_CONN_PARAM_MODE_HFP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL2, 48, 48, 0},

    {BLE_CONN_PARAM_MODE_OTA, BLE_CONN_PARAM_PRIORITY_HIGH, 6, 6, 0},

    {BLE_CONN_PARAM_MODE_OTA_SLOWER, BLE_CONN_PARAM_PRIORITY_HIGH, 20, 20, 0},

    {BLE_CONN_PARAM_MODE_SNOOP_EXCHANGE, BLE_CONN_PARAM_PRIORITY_HIGH, 8, 24, 0},

    // BAP Spec Table 8.4
    {BLE_CONN_PARAM_MODE_SVC_DISC, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL2, 8, 24, 0},

    {BLE_CONN_PARAM_MODE_ISO_DATA, BLE_CONN_PARAM_PRIORITY_HIGH, 48, 48, 0},
    // TODO: add mode cases if needed
};

void app_ble_reset_conn_param_mode(uint8_t conidx)
{
    existingBleConnParamModes[conidx] = 0;
}

void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool isEnable)
{
    for (uint8_t index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (BLE_CONNECTED == app_env.context[index].connectStatus)
        {
            app_ble_update_conn_param_mode_of_specific_connection(
                index, mode, isEnable);
        }
    }
}

void app_ble_update_conn_param_mode_of_specific_connection(uint8_t conidx, BLE_CONN_PARAM_MODE_E mode, bool isEnable)
{
#ifdef IS_INITIATIVE_BLE_UPDATE_PARAMETER_DISABLED
    return;
#endif

    ASSERT(mode < BLE_CONN_PARAM_MODE_NUM, "Wrong ble conn param mode %d!", mode);

#ifdef __GATT_OVER_BR_EDR__
    if (btif_btgatt_is_connected_by_conidx(conidx))
    {
        LOG_I("conn param mode update failed because of btgatt conn!");
        return;
    }
#endif


    // locate the conn param mode
    const BLE_CONN_PARAM_CONFIG_T *pConfig = &ble_conn_param_config[mode];
    uint32_t index = 0;
    LOG_I("(d%d)LE update conn param mode:0x%x->%d, isEnable:%d", conidx,
          existingBleConnParamModes[conidx], mode, isEnable);

    uint32_t lock = int_lock();
    if (isEnable)
    {
        if (existingBleConnParamModes[conidx] & (1 << mode))
        {
            // already existing, directly return
            int_unlock(lock);
            return;
        }
        else
        {
            // update the bit-mask
            existingBleConnParamModes[conidx] |= (1 << mode);
            // not existing yet, need to go throuth the existing params to see whether
            // we need to update the param
            for (index = 0; index < ARRAY_SIZE(ble_conn_param_config); index++)
            {
                if (((uint32_t)1 << ble_conn_param_config[index].ble_conn_param_mode) &
                        existingBleConnParamModes[conidx])
                {
                    if (ble_conn_param_config[index].priority > pConfig->priority)
                    {
                        // one of the exiting param has higher priority than this one,
                        // so do nothing but update the bit-mask
                        int_unlock(lock);
                        return;
                    }
                }
            }
            // no higher priority conn param existing, so we need to apply this one
        }
    }
    else
    {
        // doesn't exist, directly return
        if (!(existingBleConnParamModes[conidx] & (1 << mode)))
        {
            int_unlock(lock);
            return;
        }
        else
        {
            uint8_t priorityDisabled = pConfig->priority;
            // update the bit-mask
            existingBleConnParamModes[conidx] &= (~(1 << mode));

            if (0 == existingBleConnParamModes[conidx])
            {
                // restore to the default CI
                pConfig = &ble_conn_param_config[0];
                goto update;
            }

            pConfig = NULL;
            // existing, need to apply for the highest priority conn param
            for (index = 0; index < ARRAY_SIZE(ble_conn_param_config); index++)
            {
                if (((uint32_t)1 << (uint8_t)ble_conn_param_config[index].ble_conn_param_mode) &
                        existingBleConnParamModes[conidx])
                {
                    if (NULL != pConfig)
                    {
                        if (ble_conn_param_config[index].priority > pConfig->priority)
                        {
                            pConfig = &ble_conn_param_config[index];
                        }
                    }
                    else
                    {
                        pConfig = &ble_conn_param_config[index];
                    }
                }
            }

            if (pConfig && priorityDisabled < pConfig->priority)
            {
                // The priority of the disabled mode is less than that of the existing one,
                // indicating that it is already latest
                int_unlock(lock);
                return;
            }
        }
    }

update:
    int_unlock(lock);

    // if we can arrive here, it means we have got one config to apply
    ASSERT(NULL != pConfig, "(d%d)config pointer is NULL,mode:%d, isEnable:%d", conidx, mode, isEnable);
    l2cap_update_param_in_standard_unit(conidx, pConfig->conn_interval_min,
                                        pConfig->conn_interval_max,
                                        BLE_CONN_PARAM_SUPERVISE_TIMEOUT_MS,
                                        pConfig->conn_slave_latency_cnt);
}

bool app_ble_get_conn_param(uint8_t conidx,  APP_BLE_CONN_PARAM_T *pConnParam)
{
    if ((conidx < BLE_CONNECTION_MAX) &&
            (BLE_CONNECTED == app_env.context[conidx].connectStatus))
    {
        *pConnParam = app_env.context[conidx].connParam;
        return true;
    }
    else
    {
        return false;
    }
}

#if GFPS_ENABLED
uint8_t delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
#define FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE      (10000)
osTimerId fp_update_ble_param_timer = NULL;
static void fp_update_ble_connect_param_timer_handler(void const *param);
osTimerDef(FP_UPDATE_BLE_CONNECT_PARAM_TIMER, (void (*)(void const *))fp_update_ble_connect_param_timer_handler);
extern uint8_t amgr_is_bluetooth_sco_on(void);

static void fp_update_ble_connect_param_timer_handler(void const *param)
{
    LOG_I("fp_update_ble_connect_param_timer_handler");
    for (uint8_t index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if ((BLE_CONNECTED == app_env.context[index].connectStatus) &&
                (index == delay_update_conidx))
        {
            LOG_I("update connection interval of conidx %d", delay_update_conidx);

            if (amgr_is_bluetooth_sco_on())
            {
                app_ble_update_conn_param_mode_of_specific_connection(delay_update_conidx, BLE_CONN_PARAM_MODE_HFP_ON, true);
            }
            else
            {
                app_ble_update_conn_param_mode_of_specific_connection(delay_update_conidx, BLE_CONN_PARAM_MODE_DEFAULT, true);
            }
            break;
        }
    }
    delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
}

void fp_update_ble_connect_param_start(uint8_t ble_conidx)
{
    if (fp_update_ble_param_timer == NULL)
    {
        fp_update_ble_param_timer = osTimerCreate(osTimer(FP_UPDATE_BLE_CONNECT_PARAM_TIMER), osTimerOnce, NULL);
        return;
    }

    delay_update_conidx = ble_conidx;
    if (fp_update_ble_param_timer)
    {
        osTimerStart(fp_update_ble_param_timer, FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE);
    }
}

void fp_update_ble_connect_param_stop(uint8_t ble_conidx)
{
    if (delay_update_conidx == ble_conidx)
    {
        if (fp_update_ble_param_timer)
        {
            osTimerStop(fp_update_ble_param_timer);
        }
        delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
    }
}
#endif

bool app_ble_is_resolving_list_not_empty(void)
{
    return app_env.is_resolving_not_empty;
}

bool app_ble_is_connection_on_by_addr(uint8_t *addr)
{
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (!memcmp(app_env.context[i].bdAddr, addr, BD_ADDR_LEN))
        {
            if (BLE_CONNECTED == app_env.context[i].connectStatus)
            {
                return true;
            }
        }
    }

    return false;
}

uint8_t *app_ble_get_peer_addr(uint8_t conidx)
{
    return app_env.context[conidx].bdAddr;
}

bool app_ble_is_connection_on_by_index(uint8_t conidx)
{
    return (BLE_CONNECTED == app_env.context[conidx].connectStatus);
}

bool app_ble_get_peer_solved_addr(uint8_t conidx, ble_bdaddr_t *p_addr)
{
    if (app_ble_is_connection_on_by_index(conidx) == false)
    {
        return false;
    }

    if (app_env.context[conidx].isGotSolvedBdAddr)
    {
        if (p_addr != NULL)
        {
            memcpy(p_addr->addr, app_env.context[conidx].solvedBdAddr, BD_ADDR_LEN);
        }
        return true;
    }
    else
    {
        if (p_addr != NULL)
        {
            memcpy(p_addr->addr, app_env.context[conidx].bdAddr, BD_ADDR_LEN);
        }
        return false;
    }
}

uint8_t app_ble_get_peer_solved_addr_type(uint8_t conidx)
{
    if (app_env.context[conidx].isGotSolvedBdAddr)
    {
        return BLE_STATIC_ADDR;
    }
    else
    {
        return app_env.context[conidx].peerAddrType;
    }
}

bool app_ble_is_remote_mobile_connected(const ble_bdaddr_t *p_addr)
{
    uint8_t idx = 0;
    ble_bdaddr_t remote_addr = {{0}};

    do
    {
        app_ble_get_peer_solved_addr(idx, &remote_addr);
        //DUMP8("%02x ",p_addr, 6);
        //DUMP8("%02x ",remote_addr, 6);
        //LOG_I("device status = %d",app_env.context[idx].connectStatus);
        if (!memcmp(p_addr->addr, remote_addr.addr, BD_ADDR_LEN)
                && (BLE_CONNECTED == app_env.context[idx].connectStatus))
        {
            break;
        }

    } while (++idx < BLE_CONNECTION_MAX);

    if (idx >= BLE_CONNECTION_MAX)
    {
        return false;
    }

    return true;
}

void app_init_ble_name(const char *name)
{
    ASSERT(name, "%s null name ptr received", __func__);

    /// Load the device name from NVDS
    uint32_t nameLen = co_min(strlen(name) + 1, APP_DEVICE_NAME_MAX_LEN);

    // Get default Device Name (No name if not enough space)
    memcpy(app_env.dev_name, name, nameLen);
    app_env.dev_name[nameLen - 1] = '\0';
    app_env.dev_name_len = nameLen;
    LOG_I("device ble name:%s", app_env.dev_name);
}

__attribute__((weak)) uint32_t app_sec_get_P256_key(uint8_t *out_public_key, uint8_t *out_private_key)
{
    LOG_I("%s", __func__);
#ifdef CFG_SEC_CON
    memcpy(out_private_key, ble_private_key, sizeof(ble_private_key));
    memcpy(out_public_key, ble_public_key, sizeof(ble_public_key));
#endif
    return 0;
}

void app_ble_set_rpa_only(uint8_t conidx, uint8_t rpaOnly)
{
    app_env.context[conidx].supportRpaOnly = rpaOnly;
}

void app_ble_set_resolv_support(uint8_t conidx, uint8_t support)
{
    app_env.context[conidx].addr_resolv_supp = support;
    TRACE(0, "peer device supp adde resolv %d", app_env.context[conidx].addr_resolv_supp);
    DUMP8("%02x ", app_env.context[conidx].solvedBdAddr, 6);
    if (app_env.context[conidx].isGotSolvedBdAddr)
    {
        bool addr_resolv_support = false;
        nv_record_ble_read_addr_resolv_supp_via_bdaddr(app_env.context[conidx].solvedBdAddr, &addr_resolv_support);
        if (addr_resolv_support != support)
        {
            nv_record_ble_write_addr_resolv_supp_via_bdaddr(app_env.context[conidx].solvedBdAddr, support);
        }
    }
}

void app_ble_connect_req_callback_register(app_ble_connect_cb_t req_cb, app_ble_connect_cb_t done_cb)
{
    app_env.g_ble_connect_req_callback = req_cb;
    app_env.g_ble_connect_done_callback = done_cb;
    LOG_I("%s %p %p", __func__, app_env.g_ble_connect_req_callback,
          app_env.g_ble_connect_done_callback);
}

void app_ble_connect_req_callback_deregister(void)
{
    app_env.g_ble_connect_req_callback = NULL;
    app_env.g_ble_connect_done_callback = NULL;
}

void app_ble_mtu_exec_ind_callback_register(app_ble_mtu_exch_cb_t cb)
{
    app_env.ble_link_mtu_exch_ind_callback = cb;
    LOG_I("%s %p", __func__, app_env.ble_link_mtu_exch_ind_callback);
}

void app_ble_mtu_exec_ind_callback_deregister(void)
{
    app_env.ble_link_mtu_exch_ind_callback = NULL;
}

void app_ble_set_scan_coded_phy_en_and_param_before_start_scan(bool enable,
                                                               BLE_SCAN_WD_T *start_scan_coded_scan_wd)
{
    if (enable && start_scan_coded_scan_wd == NULL)
    {
        LOG_E("%s param err", __func__);
        return;
    }

    if (enable)
    {
        app_env.scan_start_param_coded = *start_scan_coded_scan_wd;
    }

    app_env.scan_coded_phy_en = enable;

    LOG_I("%s %d", __func__, app_env.scan_coded_phy_en);
}

void app_ble_set_init_conn_all_phy_param_before_start_connect(BLE_CONN_PARAM_T *init_param_universal, BLE_SCAN_WD_T *init_coded_scan_wd)
{
    if (init_param_universal == NULL || init_coded_scan_wd == NULL)
    {
        LOG_E("%s param err", __func__);
        return;
    }

    app_env.init_conn_param_universal = *init_param_universal;
    app_env.init_conn_scan_param_coded = *init_coded_scan_wd;
    LOG_I("%s", __func__);
}

void app_ble_set_list()
{
    TRACE(0, "%s, need_set_white_list %d, isNeedToAddDevicesToReslovingList %d", __func__, app_env.need_set_white_list, app_env.isNeedToAddDevicesToReslovingList);

    if (app_env.need_set_white_list)
    {
        TRACE(0, "%s set white list", __func__);
        appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
    }
    else if (app_env.isNeedToAddDevicesToReslovingList)
    {
        appm_set_rpa_list(&app_env.devicesInfoAddedToResolvingList[0],
                          app_env.numberOfDevicesAddedToResolvingList);
    }
}

#endif //(BLE_APP_PRESENT)
/// @} APP

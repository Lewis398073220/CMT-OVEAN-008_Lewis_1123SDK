/***************************************************************************
*
*Copyright 2015-2021 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "string.h"
#include "co_math.h" // Common Maths Definition
#include "cmsis_os.h"
#include "ble_app_dbg.h"
#include "stdbool.h"
#include "bluetooth_bt_api.h"
#include "app_a2dp.h"
#include "app_thread.h"
#include "app_utils.h"
#include "bt_drv_interface.h"
#include "apps.h"
#include "gapc_msg.h"            // GAP Controller Task API
#include "gapm_msg.h"            // GAP Manager Task API
#include "gapm_le.h"
#include "app.h"
#include "app_sec.h"
#include "app_ble_mode_switch.h"
#include "app_ble_param_config.h"
#include "app_ble_custom_api.h"

CUSTOMER_ADV_PARAM_T customer_adv_param[BLE_ADV_ACTIVITY_USER_NUM];

CUSTOMER_ADV_PARAM_T * app_ble_custom_adv_param_ptr(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    if (actv_user >= BLE_ADV_ACTIVITY_USER_NUM) {
        LOG_E("%s actv_user shouled be less than %d ", __func__, BLE_ADV_ACTIVITY_USER_NUM);
        return NULL;
    }

    return &customer_adv_param[actv_user];
}

bool app_ble_custom_adv_is_rpa(uint8_t *addr)
{
    uint8_t adv_addr_set[6]  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (!memcmp(addr, adv_addr_set, GAP_BD_ADDR_LEN))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_E actv_user,
                    bool is_custom_adv_flags,
                    BLE_ADV_ADDR_TYPE_E type,
                    uint8_t *local_addr,
                    ble_bdaddr_t *peer_addr,
                    uint32_t adv_interval,
                    BLE_ADV_TYPE_E adv_type,
                    ADV_MODE_E adv_mode,
                    int8_t tx_power_dbm,
                    uint8_t *adv_data, uint8_t adv_data_size,
                    uint8_t *scan_rsp_data, uint8_t scan_rsp_data_size)
{
    CUSTOMER_ADV_PARAM_T *adv_param_p = NULL;
    LOG_I("%s user %d, ADV ADDR TYPE %d", __func__, actv_user, type);
    DUMP8("%02x ", local_addr, GAP_BD_ADDR_LEN);
    if (actv_user >= BLE_ADV_ACTIVITY_USER_NUM) {
        LOG_E("%s actv_user shouled be less than %d ", __func__, BLE_ADV_ACTIVITY_USER_NUM);
        return;
    }
    app_ble_set_adv_txpwr_by_actv_user(actv_user, tx_power_dbm);

    adv_param_p = &customer_adv_param[actv_user];
    adv_param_p->adv_actv_user = actv_user;
    adv_param_p->withFlags = is_custom_adv_flags ? false:true;
    memset(adv_param_p->localAddr, 0, sizeof(adv_param_p->localAddr));
    switch (type){
        case BLE_ADV_PUBLIC_STATIC:
        {
            adv_param_p->localAddrType = GAPM_STATIC_ADDR;
            memcpy(adv_param_p->localAddr, bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
            break;
        }
        case BLE_ADV_PRIVATE_STATIC:
        {
            adv_param_p->localAddrType = GAPM_GEN_RSLV_ADDR;
            memcpy(adv_param_p->localAddr, local_addr, BTIF_BD_ADDR_SIZE);
            break;
        }
        case BLE_ADV_RPA:
        {
#ifdef BLE_ADV_RPA_ENABLED
            adv_param_p->localAddrType = GAPM_GEN_RSLV_ADDR;
            memcpy(adv_param_p->localAddr, local_addr, BTIF_BD_ADDR_SIZE);
#else
            adv_param_p->localAddrType = GAPM_STATIC_ADDR;
            memcpy(adv_param_p->localAddr, bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
#endif
            break;
        }
        default:
            ASSERT(0, "UNKNOWN ADDR TYPE %d", type);
            break;
    }

    if(peer_addr)
    {
        memcpy(&adv_param_p->peerAddr, peer_addr, sizeof(ble_bdaddr_t));
    }
    else
    {
        memset(&adv_param_p->peerAddr, 0, sizeof(ble_bdaddr_t));
    }
    adv_param_p->advUserInterval = adv_interval;
    adv_param_p->PeriodicIntervalMin = 0;
    adv_param_p->PeriodicIntervalMax = 0;
    adv_param_p->advType = adv_type;
    adv_param_p->advMode = adv_mode;
    if (adv_data && adv_data_size)
    {
        memcpy(adv_param_p->advData, adv_data, adv_data_size);
        adv_param_p->advDataLen = adv_data_size;
    }
    if (scan_rsp_data && scan_rsp_data_size)
    {
        memcpy(adv_param_p->scanRspData, scan_rsp_data, scan_rsp_data_size);
        adv_param_p->scanRspDataLen = scan_rsp_data_size;
    }
}

void app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    LOG_I("%s user %d", __func__, actv_user);
    customer_adv_param[actv_user].user_enable = true;
    app_ble_start_connectable_adv_by_custom_adv(BLE_ADVERTISING_INTERVAL);
}

void app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    LOG_I("%s user %d", __func__, actv_user);
    customer_adv_param[actv_user].user_enable = false;
    app_ble_refresh_adv_state_by_custom_adv(BLE_ADVERTISING_INTERVAL);
}

void app_ble_custom_adv_clear_enabled_flag(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    customer_adv_param[actv_user].user_enable = false;
}

static void app_ble_custom_fill_adv_param(BLE_ADV_PARAM_T *dst, CUSTOMER_ADV_PARAM_T *src)
{
    uint8_t empty_addr[BTIF_BD_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    bool already_fill_ad_flag = dst->advData[1] == GAP_AD_TYPE_FLAGS ? true : false;

    dst->adv_actv_user = src->adv_actv_user;
    dst->isBleFlagsAdvDataConfiguredByAppLayer = !(src->withFlags);
    dst->advType = src->advType;
    dst->advMode = src->advMode;
    dst->advUserInterval[src->adv_user] = src->advUserInterval;
    dst->PeriodicIntervalMin = src->PeriodicIntervalMin;
    dst->PeriodicIntervalMax = src->PeriodicIntervalMax;

    if (already_fill_ad_flag && dst->isBleFlagsAdvDataConfiguredByAppLayer)
    {
        // change adv flag
        memcpy(&dst->advData[0],&src->advData[0], 3);
        // copy flag
        memcpy(&dst->advData[dst->advDataLen],src->advData + 3, src->advDataLen - 3);
    }
    else
    {
        memcpy(&dst->advData[dst->advDataLen],src->advData, src->advDataLen);
    }
    dst->advDataLen += src->advDataLen;
    memcpy(&dst->scanRspData[dst->scanRspDataLen],src->scanRspData, src->scanRspDataLen);
    dst->scanRspDataLen += src->scanRspDataLen;

    dst->localAddrType = src->localAddrType;
    memcpy(&dst->localAddr, &src->localAddr, sizeof(src->localAddr));

    if (memcmp(src->peerAddr.addr, empty_addr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(&dst->peerAddr, &src->peerAddr, sizeof(ble_bdaddr_t));
    }
}



static void app_ble_custom_0_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_0];

    if (custom_adv_param_p->user_enable)
    {
        custom_adv_param_p->adv_user = USER_BLE_CUSTOMER_0;
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, custom_adv_param_p);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_0, custom_adv_param_p->user_enable);
}

static void app_ble_custom_0_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_0, (BLE_DATA_FILL_FUNC_T)app_ble_custom_0_user_data_fill_handler, false);
}

static void app_ble_custom_1_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_1];

    if (custom_adv_param_p->user_enable)
    {
        custom_adv_param_p->adv_user = USER_BLE_CUSTOMER_1;
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, custom_adv_param_p);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_1, custom_adv_param_p->user_enable);
}

static void app_ble_custom_1_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_1, (BLE_DATA_FILL_FUNC_T)app_ble_custom_1_user_data_fill_handler, false);
}

static void app_ble_custom_2_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_2];

    if (custom_adv_param_p->user_enable)
    {
        custom_adv_param_p->adv_user = USER_BLE_CUSTOMER_2;
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, custom_adv_param_p);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_2, custom_adv_param_p->user_enable);
}

#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
static void app_ble_custom_3_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_3];

    if (custom_adv_param_p->user_enable)
    {
        custom_adv_param_p->adv_user = USER_BLE_CUSTOMER_3;
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, custom_adv_param_p);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_3, custom_adv_param_p->user_enable);
}
#endif

static void app_ble_custom_2_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_2, (BLE_DATA_FILL_FUNC_T)app_ble_custom_2_user_data_fill_handler, false);
}

#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
static void app_ble_custom_3_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_3, (BLE_DATA_FILL_FUNC_T)app_ble_custom_3_user_data_fill_handler, false);
}
#endif

void app_ble_custom_init(void)
{
    static bool ble_custom_inited = false;

    if (!ble_custom_inited)
    {
        LOG_I("%s", __func__);
        ble_custom_inited = true;
        for (uint8_t actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
        {
            memset(&customer_adv_param[actv_user], 0, sizeof(CUSTOMER_ADV_PARAM_T));
        }
        app_ble_custom_0_user_init();
        app_ble_custom_1_user_init();
        app_ble_custom_2_user_init();
#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
        app_ble_custom_3_user_init();
#endif
    }
}


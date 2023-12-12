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
#include "bluetooth_bt_api.h"
#include "nvrecord_bt.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "nvrecord_ble.h"
#include "app_ble.h"

#ifdef IBRT
#include "app_ibrt_internal.h"
#endif

#if (BLE_AUDIO_ENABLED)
extern bool ble_audio_is_ux_mobile(void);
#endif

ble_adv_data_report_cb_t g_ble_scan_result_callback = NULL;
ble_link_event_report_cb_t g_ble_link_event_callback = NULL;

static void app_ble_customif_connect_event_handler(ble_event_t *event, void *output)
{
#ifdef TWS_SYSTEM_ENABLED
    app_ibrt_middleware_ble_connected_handler();
#endif

    if (g_ble_link_event_callback)
    {
        connect_handled_t* conn_handled_param = &event->p.connect_handled;
        g_ble_link_event_callback(conn_handled_param->conidx, conn_handled_param->peer_bdaddr.addr,
                                        BLE_LINK_CONNECTED_EVENT, 0);
    }
}

static void app_ble_customif_connect_bond_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_connect_nc_exch_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_connect_encrypt_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_stopped_connecting_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_connecting_failed_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_disconnect_event_handler(ble_event_t *event, void *output)
{
    if (g_ble_link_event_callback)
    {
        disconnect_handled_t* disc_handled_param = &event->p.disconnect_handled;
        g_ble_link_event_callback(disc_handled_param->conidx, NULL, BLE_DISCONNECT_EVENT, disc_handled_param->errCode);
    }
}

static void app_ble_customif_conn_param_update_req_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_conn_param_update_failed_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_conn_param_update_successful_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_set_random_bd_addr_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_adv_started_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_adv_starting_failed_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_adv_stopped_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_scan_started_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_scan_data_report_event_handler(ble_event_t *event, void *output)
{
    scan_data_report_handled_t *report = &event->p.scan_data_report_handled;
    const gap_adv_report_t *adv = report->adv;
    uint8_t evt_type = 0;
    ble_bdaddr_t peer_addr;

    if(adv->adv.connectable)
    {
        evt_type = 0;   //ADV_CONN_UNDIR
    }
    else
    {
        evt_type = 3;   //ADV_NONCONN_UNDIR
    }

    peer_addr.addr_type = adv->peer_type;
    memcpy(peer_addr.addr, adv->peer_addr.address, sizeof(bt_bdaddr_t));

    if (g_ble_scan_result_callback)
    {
        g_ble_scan_result_callback(&peer_addr, adv->rssi, evt_type, (uint8_t *)report->data, report->length);
    }
}

static void app_ble_customif_scan_starting_failed_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_scan_stopped_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_credit_based_conn_req_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_encrypt_ltk_report_event_handler(ble_event_t *event, void *output)
{

}

static const ble_event_handler_t app_ble_customif_event_handler_tab[] =
{
    {BLE_LINK_CONNECTED_EVENT, app_ble_customif_connect_event_handler},
    {BLE_CONNECT_BOND_EVENT, app_ble_customif_connect_bond_event_handler},
    {BLE_CONNECT_NC_EXCH_EVENT, app_ble_customif_connect_nc_exch_event_handler},
    {BLE_CONNECT_ENCRYPT_EVENT, app_ble_customif_connect_encrypt_event_handler},
    {BLE_CONNECTING_STOPPED_EVENT, app_ble_customif_stopped_connecting_event_handler},
    {BLE_CONNECTING_FAILED_EVENT, app_ble_customif_connecting_failed_event_handler},
    {BLE_DISCONNECT_EVENT, app_ble_customif_disconnect_event_handler},
    {BLE_CONN_PARAM_UPDATE_REQ_EVENT, app_ble_customif_conn_param_update_req_event_handler},
    {BLE_CONN_PARAM_UPDATE_FAILED_EVENT, app_ble_customif_conn_param_update_failed_event_handler},
    {BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT, app_ble_customif_conn_param_update_successful_event_handler},
    {BLE_SET_RANDOM_BD_ADDR_EVENT, app_ble_customif_set_random_bd_addr_event_handler},
    {BLE_ADV_STARTED_EVENT, app_ble_customif_adv_started_event_handler},
    {BLE_ADV_STARTING_FAILED_EVENT, app_ble_customif_adv_starting_failed_event_handler},
    {BLE_ADV_STOPPED_EVENT, app_ble_customif_adv_stopped_event_handler},
    {BLE_SCAN_STARTED_EVENT, app_ble_customif_scan_started_event_handler},
    {BLE_SCAN_DATA_REPORT_EVENT, app_ble_customif_scan_data_report_event_handler},
    {BLE_SCAN_STARTING_FAILED_EVENT, app_ble_customif_scan_starting_failed_event_handler},
    {BLE_SCAN_STOPPED_EVENT, app_ble_customif_scan_stopped_event_handler},
    {BLE_CREDIT_BASED_CONN_REQ_EVENT, app_ble_customif_credit_based_conn_req_event_handler},
    {BLE_ENCRYPT_LTK_REPORT_EVENT, app_ble_customif_encrypt_ltk_report_event_handler}
};

void app_ble_customif_adv_report_callback_register(ble_adv_data_report_cb_t cb)
{
    g_ble_scan_result_callback = cb;
}

void app_ble_customif_adv_report_callback_deregister(void)
{
    g_ble_scan_result_callback = NULL;
}

void app_ble_customif_link_event_callback_register(ble_link_event_report_cb_t cb)
{
    g_ble_link_event_callback = cb;
}

void app_ble_customif_link_event_callback_deregister(void)
{
    g_ble_link_event_callback = NULL;
}

void app_ble_customif_global_handler_ind(ble_event_t *event, void *output)
{
    uint8_t evt_type = event->evt_type;
    uint16_t index = 0;
    const ble_event_handler_t *p_ble_event_hand = NULL;

    for (index = 0; index < ARRAY_SIZE(app_ble_customif_event_handler_tab); index++)
    {
        p_ble_event_hand = &app_ble_customif_event_handler_tab[index];
        if (p_ble_event_hand->evt_type == evt_type)
        {
            p_ble_event_hand->func(event, output);
            break;
        }
    }
}

void app_ble_customif_init(void)
{
#ifdef IS_BLE_CUSTOM_IF_ENABLED
    TRACE(0, "%s", __func__);
    app_ble_core_register_global_handler_ind(app_ble_customif_global_handler_ind);
#endif
}

void app_ble_start_connectable_adv_by_custom_adv(uint16_t advInterval)
{
    app_ble_refresh_adv();
}

void app_ble_refresh_adv_state_by_custom_adv(uint16_t advInterval)
{
    app_ble_refresh_adv();
}

static CUSTOMER_ADV_PARAM_T customer_adv_param[BLE_ADV_ACTIVITY_USER_NUM];

static void app_ble_custom_clear_param(CUSTOMER_ADV_PARAM_T *param)
{
    gap_dt_buf_clear(&param->adv_data);
    gap_dt_buf_clear(&param->scan_rsp_data);
    memset(param, 0, sizeof(CUSTOMER_ADV_PARAM_T));
}

/**
 * app_ble_custom_adv_write_data
 *
 *    actv_user: The user of the adv activity
 *    is_custom_adv_flags: If this flag was set, custom can set adv flag by himself
 *    type : adv addr type
 *               BLE_ADV_PUBLIC_STATIC   Don't care about local_addr, just use identity ble addr.
 *               BLE_ADV_PRIVATE_STATIC  Just use local_addr.
 *               BLE_ADV_RPA    local_addr shall be set to ff:ff:ff:ff:ff:ff. If the resolving list contains
 *                              no matching entry, use rpa generated by host;otherwise use rpa generated by control.
 *               !!!If wants to use rpa, the premise is to open the macro BLE_ADV_RPA_ENABLED
 *    local_addr: The local address of this adv. ff:ff:ff:ff:ff:ff when rpa.
 *    peer_addr: If adv_type is direct adv, this param is the address of peer ble
 *    adv_interval: Adv interval
 *    adv_type: Adv type
 *    adv_mode: Adv mode
 *    tx_power_dbm: Adv tx power in dbm, range: -3~16
 *    adv_data: Adv data
 *    adv_data_size: Adv data size
 *    scan_rsp_data: Scan response data
 *    scan_rsp_data_size: Scan response data size
 */
void app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_E user_index,
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
    CUSTOMER_ADV_PARAM_T *custom = NULL;

    TRACE(0, "%s user %d, ADV ADDR TYPE %d", __func__, user_index, type);

    if (local_addr)
    {
        DUMP8("%02x ", local_addr, 6);
    }

    if (user_index >= BLE_ADV_ACTIVITY_USER_NUM)
    {
        return;
    }

    custom = customer_adv_param + user_index;

    app_ble_custom_clear_param(custom);

    custom->adv_actv_user = user_index;
    custom->withFlags = !(is_custom_adv_flags);

    switch (type)
    {
        case BLE_ADV_PUBLIC_STATIC:
        {
            custom->localAddrType = GAPM_STATIC_ADDR;
            custom->localAddr = *gap_factory_le_address();
            break;
        }
        case BLE_ADV_PRIVATE_STATIC:
        {
            custom->localAddrType = GAPM_GEN_RSLV_ADDR;
            if (local_addr)
            {
                custom->localAddr = *((bt_bdaddr_t *)local_addr);
            }
            break;
        }
        case BLE_ADV_RPA:
        {
#ifdef BLE_ADV_RPA_ENABLED
            custom->localAddrType = GAPM_GEN_RSLV_ADDR;
            if (local_addr)
            {
                custom->localAddr = *((bt_bdaddr_t *)local_addr);
            }
#else
            custom->localAddrType = GAPM_STATIC_ADDR;
            custom->localAddr = *gap_factory_le_address();
#endif
            break;
        }
        default:
        {
            ASSERT(0, "UNKNOWN ADDR TYPE %d", type);
            break;
        }
    }

    if (peer_addr)
    {
        custom->peerAddr = *peer_addr;
    }

    custom->tx_power_dbm = tx_power_dbm;
    custom->advUserInterval = adv_interval;
    custom->PeriodicIntervalMin = 0;
    custom->PeriodicIntervalMax = 0;
    custom->advType = adv_type;
    custom->advMode = adv_mode;

    if (adv_data && adv_data_size)
    {
        gap_dt_add_raw_data(&custom->adv_data, adv_data, adv_data_size);
    }

    if (scan_rsp_data && scan_rsp_data_size)
    {
        gap_dt_add_raw_data(&custom->adv_data, scan_rsp_data, scan_rsp_data_size);
    }
}

void app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_E user_index)
{
    TRACE(0, "%s user %d", __func__, user_index);
    customer_adv_param[user_index].user_enable = true;
    app_ble_start_connectable_adv_by_custom_adv(BLE_ADVERTISING_INTERVAL);
}

void app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_E user_index)
{
    TRACE(0, "%s user %d", __func__, user_index);
    customer_adv_param[user_index].user_enable = false;
    app_ble_disable_advertising(BLE_CUSTOMER0_ADV_HANDLE + user_index);
    app_ble_refresh_adv();
}

void app_ble_custom_adv_clear_enabled_flag(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    customer_adv_param[actv_user].user_enable = false;
}

static bool app_custom_adv_activity_prepare(BLE_ADV_USER_E user, ble_adv_activity_t *adv)
{
    CUSTOMER_ADV_PARAM_T *custom = NULL;
    BLE_ADV_PARAM_T app_adv_param = {0};
    app_ble_adv_data_param_t custom_adv_data = {0};
    bt_bdaddr_t zero_addr = {{0}};
    int user_index = (user - USER_BLE_CUSTOMER_0);
    bool legacy_adv = false;
    bool connectable = false;
    bool scannable = false;
    bool directed = false;

    custom = customer_adv_param + user_index;

    if (!ble_adv_is_allowed())
    {
        return false;
    }

    if (!custom->user_enable)
    {
        return false;
    }

    if (gap_filter_list_user_item_exist(BLE_WHITE_LIST_USER_MOBILE) || gap_filter_list_user_item_exist(BLE_WHITE_LIST_USER_TWS))
    {
        adv->adv_param.policy = GAP_ADV_ACCEPT_ALL_CONN_SCAN_REQS_IN_LIST;
    }
    else
    {
        adv->adv_param.policy = GAP_ADV_ACCEPT_ALL_CONN_SCAN_REQS;
    }

    legacy_adv = (custom->advMode == ADV_MODE_LEGACY);
    connectable = (custom->advType != ADV_TYPE_NON_CONN_SCAN && custom->advType != ADV_TYPE_NON_CONN_NON_SCAN);
    scannable = (custom->advType == ADV_TYPE_UNDIRECT || custom->advType == ADV_TYPE_NON_CONN_SCAN || custom->advType == ADV_TYPE_CONN_EXT_ADV);
    directed = (custom->advType == ADV_TYPE_DIRECT_LDC || custom->advType == ADV_TYPE_DIRECT_HDC);

    adv->user = user;
    adv->adv_param.connectable = connectable;
    adv->adv_param.scannable = scannable;
    adv->adv_param.directed_adv = directed;
    adv->adv_param.high_duty_directed_adv = (custom->advType == ADV_TYPE_DIRECT_HDC);
    adv->adv_param.use_legacy_pdu = legacy_adv;
    adv->adv_param.include_tx_power_data = true;
    adv->adv_param.own_addr_use_rpa = (custom->localAddrType == GAPM_GEN_RSLV_ADDR);
    adv->adv_param.peer_type = (bt_addr_type_t)custom->peerAddr.addr_type;
    adv->adv_param.peer_addr = *(bt_bdaddr_t *)custom->peerAddr.addr;

    if (memcmp(&custom->localAddr, &zero_addr, sizeof(bt_bdaddr_t)) != 0)
    {
        adv->adv_param.use_custom_local_addr = true;
        adv->adv_param.custom_local_addr = custom->localAddr;
    }

    app_ble_get_user_adv_data(adv, &app_adv_param, user_index);
    if (!adv->custom_adv_interval_ms || custom->advUserInterval < adv->custom_adv_interval_ms)
    {
        adv->custom_adv_interval_ms = custom->advUserInterval;
    }

    app_ble_set_adv_tx_power_dbm(adv, custom->tx_power_dbm);

    if (!gap_dt_buf_find_type(&custom->adv_data, GAP_DT_FLAGS, 0))
    {
        app_ble_dt_set_flags(&adv->adv_param, false); // add ad flags if custom does not provided
    }

    custom_adv_data.adv_data = gap_dt_buf_data(&custom->adv_data);
    custom_adv_data.adv_data_len = gap_dt_buf_len(&custom->adv_data);
    custom_adv_data.scan_rsp_data = gap_dt_buf_data(&custom->scan_rsp_data);
    custom_adv_data.scan_rsp_len = gap_dt_buf_len(&custom->scan_rsp_data);

    app_ble_dt_add_adv_data(adv, &app_adv_param, &custom_adv_data);

    app_ble_dt_set_local_name(&adv->adv_param, NULL);

    return true;
}

static bool app_custom_0_user_adv_prepare(ble_adv_activity_t *adv)
{
    return app_custom_adv_activity_prepare(USER_BLE_CUSTOMER_0, adv);
}

static bool app_custom_1_user_adv_prepare(ble_adv_activity_t *adv)
{
    return app_custom_adv_activity_prepare(USER_BLE_CUSTOMER_1, adv);
}

static bool app_custom_2_user_adv_prepare(ble_adv_activity_t *adv)
{
    return app_custom_adv_activity_prepare(USER_BLE_CUSTOMER_2, adv);
}

#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
static bool app_custom_3_user_adv_prepare(ble_adv_activity_t *adv)
{
    return app_custom_adv_activity_prepare(USER_BLE_CUSTOMER_3, adv);
}
#endif

static void app_ble_cusotm_register_adv_activity(BLE_ADV_USER_E user, app_ble_adv_activity_func adv_activity_func)
{
    ble_adv_activity_t *adv = NULL;
    uint8_t adv_handle = BLE_CUSTOMER0_ADV_HANDLE + (user - USER_BLE_CUSTOMER_0);

    adv = app_ble_register_advertising(adv_handle, adv_activity_func);
    if (adv == NULL)
    {
        return;
    }

    adv->user = user;
}

void app_ble_custom_init(void)
{
    static bool ble_custom_inited = false;
    if (!ble_custom_inited)
    {
        ble_custom_inited = true;
        memset(customer_adv_param, 0, sizeof(customer_adv_param));
        app_ble_cusotm_register_adv_activity(USER_BLE_CUSTOMER_0, app_custom_0_user_adv_prepare);
        app_ble_cusotm_register_adv_activity(USER_BLE_CUSTOMER_1, app_custom_1_user_adv_prepare);
        app_ble_cusotm_register_adv_activity(USER_BLE_CUSTOMER_2, app_custom_2_user_adv_prepare);
#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
        app_ble_cusotm_register_adv_activity(USER_BLE_CUSTOMER_3, app_custom_3_user_adv_prepare);
#endif
    }
}

#define APP_DEMO_DATA0 "\x02\x01\x06\x37\xFF\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00"
#define APP_DEMO_DATA0_LEN (59)
#define APP_DEMO_DATA1 "\x02\x01\x06\x03\x18\x04\xFE"
#define APP_DEMO_DATA1_LEN (7)
#define APP_DEMO_DATA2 "\x02\x01\xFF\x03\x19\x03\xFE"
#define APP_DEMO_DATA2_LEN (7)

void app_ble_start_three_adv_test(void)
{
    uint8_t adv_addr_set[6]  = {0x66, 0x34, 0x33, 0x23, 0x22, 0x11};
    uint8_t adv_addr_set2[6] = {0x77, 0x34, 0x33, 0x23, 0x22, 0x11};
    uint8_t adv_addr_set3[6] = {0x88, 0x34, 0x33, 0x23, 0x22, 0x11};

    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            BLE_ADV_PUBLIC_STATIC,
                            (uint8_t *)adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA0, APP_DEMO_DATA0_LEN,
                            (uint8_t *)APP_DEMO_DATA0, APP_DEMO_DATA0_LEN);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);

    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_1,
                            true,
                            BLE_ADV_PRIVATE_STATIC,
                            (uint8_t *)adv_addr_set2,
                            NULL,
                            200,
                            ADV_TYPE_NON_CONN_SCAN,
                            ADV_MODE_LEGACY,
                            -5,
                            (uint8_t *)APP_DEMO_DATA1, APP_DEMO_DATA1_LEN,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_1);

    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_2,
                            true,
                            BLE_ADV_RPA,
                            (uint8_t *)adv_addr_set3,
                            NULL,
                            200,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA2, APP_DEMO_DATA2_LEN,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_2);
}

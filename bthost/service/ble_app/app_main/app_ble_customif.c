/***************************************************************************
*
*Copyright 2015-2019 BES.
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
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"
#include "gapm_msg.h"               // GAP Manager Task API
#include "gapc_msg.h"               // GAP Controller Task API
#include "gapm_le.h"
#include "app.h"
#include "app_sec.h"
#include "app_ble_include.h"
#include "nvrecord_bt.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"
#include "nvrecord_ble.h"
#include "app_sec.h"

#ifdef IBRT
#include "app_ibrt_internal.h"
#endif

#if (BLE_AUDIO_ENABLED)
extern bool ble_audio_is_ux_mobile(void);
#endif

/**
 * @brief Private value adv/acl evt callback
 * 
 */
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
    // Indicate that ble connection pairing complete
}

static void app_ble_customif_connect_nc_exch_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection numeric comparison - Exchange of Numeric Value
}

static void app_ble_customif_connect_encrypt_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection encrypt complete
}

static void app_ble_customif_stopped_connecting_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_connecting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble start connecting failed. Mostly it's param invalid.
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
    // Indicate that ble connection param update failed
}

static void app_ble_customif_conn_param_update_successful_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update successful
}

static void app_ble_customif_set_random_bd_addr_event_handler(ble_event_t *event, void *output)
{
    // Indicate that a new random BD address set in lower layers
}

static void app_ble_customif_adv_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been started success
}

static void app_ble_customif_adv_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv starting failed
}

static void app_ble_customif_adv_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been stopped success
}

static void app_ble_customif_scan_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been started success
}

static void ble_adv_data_parse(ble_bdaddr_t *bleBdAddr,
                               int8_t rssi,
                               uint8_t info,
                               unsigned char *adv_buf,
                               unsigned char len)
{
    uint8_t evt_type = 0;
    if(info & GAPM_REPORT_INFO_CONN_ADV_BIT)
    {
        evt_type = 0;   //ADV_CONN_UNDIR
    }
    else
    {
        evt_type = 3;   //ADV_NONCONN_UNDIR
    }

    if (g_ble_scan_result_callback)
    {
        g_ble_scan_result_callback(bleBdAddr, rssi, evt_type, adv_buf, len);
    }
}

static void app_ble_customif_scan_data_report_event_handler(ble_event_t *event, void *output)
{
    // Scan data report
    ble_adv_data_parse((ble_bdaddr_t *)&event->p.scan_data_report_handled.trans_addr,
                           (int8_t)event->p.scan_data_report_handled.rssi,
                           event->p.scan_data_report_handled.info,
                           event->p.scan_data_report_handled.data,
                           (unsigned char)event->p.scan_data_report_handled.length);
}

static void app_ble_customif_scan_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan starting failed
}

static void app_ble_customif_scan_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static void app_ble_customif_credit_based_conn_req_event_handler(ble_event_t *event, void *output)
{

}

static void app_ble_customif_encrypt_ltk_report_event_handler(ble_event_t *event, void *output)
{
    le_conn_encrypt_ltk_handled_t* le_ltk_info_cb = (le_conn_encrypt_ltk_handled_t*)&(event->p);
    if (le_ltk_info_cb)
        LOG_I("[%s] con_lid=%d ltk_found=%d", __func__, le_ltk_info_cb->conidx, le_ltk_info_cb->ltk_existed);
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

//handle the event that from ble lower layers
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
    LOG_I("%s", __func__);
    app_ble_core_register_global_handler_ind(app_ble_customif_global_handler_ind);
#endif
}


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

#include "hal_trace.h"
#include <cstring>
#include "cqueue.h"
#include "bluetooth.h"
#include "app_tws_ibrt.h"
#include "app_dbg_bt_info.h"
#include "app_vendor_cmd_evt.h"

#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
static tws_ibrt_link_loss_info_t app_dbg_bt_link_loss_info[BT_DEVICE_NUM];

static tws_ibrt_a2dp_sink_info_t app_dbg_bt_a2dp_sink_info;

static app_tws_ibrt_analysing_info_received_cb link_loss_universal_info_received_cb = NULL;

static app_tws_ibrt_analysing_info_received_cb link_loss_clock_info_received_cb = NULL;

static app_tws_ibrt_analysing_info_received_cb a2dp_sink_info_received_cb = NULL;

static app_dbg_bt_link_loss_related_info_rcv_cb app_bt_link_loss_dbg_info_rcv_cb = NULL;

static tws_ibrt_link_loss_universal_info_t* app_dbg_bt_get_link_loss_universal_info(uint16_t conn_hdl)
{
    for (int i = 0;i < BT_DEVICE_NUM;i++)
    {
        uint16_t local_conn_hdl = app_dbg_bt_link_loss_info[i].link_loss_universal_info.conn_hdl;
        if (local_conn_hdl == conn_hdl)
        {
            return &app_dbg_bt_link_loss_info[i].link_loss_universal_info;
        }
        else if (local_conn_hdl == 0x00)
        {
            return &app_dbg_bt_link_loss_info[i].link_loss_universal_info;
        }
    }
    return &app_dbg_bt_link_loss_info[0].link_loss_universal_info;
}

static tws_ibrt_link_loss_clock_info_t* app_dbg_bt_get_link_loss_clock_info(uint16_t conn_hdl)
{
    for (int i = 0;i < BT_DEVICE_NUM;i++)
    {
        uint16_t local_conn_hdl = app_dbg_bt_link_loss_info[i].link_loss_universal_info.conn_hdl;
        if (local_conn_hdl == conn_hdl)
        {
            return &app_dbg_bt_link_loss_info[i].link_loss_clock_info;
        }
        else if (local_conn_hdl == 0x00)
        {
            return &app_dbg_bt_link_loss_info[i].link_loss_clock_info;
        }
    }
    return &app_dbg_bt_link_loss_info[0].link_loss_clock_info;
}

static bool app_dbg_bt_update_analysing_link_loss_universal_info(uint8_t* info, uint32_t info_len)
{
    if ((info_len != sizeof(tws_ibrt_link_loss_universal_info_t)) || (NULL == info))
    {
        TRACE(1,"Update link loss universal info fail! info_len=%d true_len=%d", info_len, sizeof(tws_ibrt_link_loss_universal_info_t));
        return false;
    }

    uint16_t conn_hdl = ((tws_ibrt_link_loss_universal_info_t*)info)->conn_hdl;
    tws_ibrt_link_loss_universal_info_t* link_loss_universal_info = app_dbg_bt_get_link_loss_universal_info(conn_hdl);

    TRACE(1,"Record link loss universal info connhdl=0x%x len=%d ", conn_hdl, info_len);
    memcpy(link_loss_universal_info, info, info_len);

    if (link_loss_universal_info_received_cb)
    {
        link_loss_universal_info_received_cb(LINK_LOSS_UNISERVAL_INFO_TYPE, conn_hdl, (uint8_t*)link_loss_universal_info, info_len);
    }

    return true;
}

static bool app_dbg_bt_update_analysing_link_loss_clock_info(uint8_t* info, uint32_t info_len)
{
    uint16_t conn_hdl = 0;
    uint32_t clock_info_len = info_len - sizeof(conn_hdl);
    if ((clock_info_len != sizeof(tws_ibrt_link_loss_clock_info_t)) || (NULL == info))
    {
        TRACE(1,"Update link loss clock info fail! info_len=%d true_len=%d", clock_info_len, sizeof(tws_ibrt_link_loss_clock_info_t));
        return false;
    }

    conn_hdl = *((uint16_t*)info);
    tws_ibrt_link_loss_clock_info_t* link_loss_clock_info = app_dbg_bt_get_link_loss_clock_info(conn_hdl);
    uint8_t* clock_info = &info[sizeof(conn_hdl)];

    TRACE(1,"Record link loss clock info connhdl=0x%x len=%d ", conn_hdl, clock_info_len);
    memcpy(link_loss_clock_info, clock_info, clock_info_len);

    if (link_loss_clock_info_received_cb)
    {
        link_loss_clock_info_received_cb(LINK_LOSS_CLOCK_INFO_TYPE, conn_hdl, (uint8_t*)link_loss_clock_info, clock_info_len);
    }
    return true;
}

static bool app_dbg_bt_update_analysing_a2dp_sink_info(uint8_t* info, uint32_t info_len)
{
    if ((info_len != sizeof(tws_ibrt_a2dp_sink_info_t)) || (NULL == info))
    {
        TRACE(1,"Update a2dp sink info fail! info_len=%d true_len=%d", info_len, sizeof(tws_ibrt_a2dp_sink_info_t));
        return false;
    }

    tws_ibrt_a2dp_sink_info_t* a2dp_sink_info = app_dbg_bt_get_a2dp_sink_info();

    TRACE(1,"Record a2dp sink info len=%d ", info_len);
    memcpy(a2dp_sink_info, info, info_len);

    if (a2dp_sink_info_received_cb)
    {
        a2dp_sink_info_received_cb(A2DP_SINK_INFO_TYPE, 0, (uint8_t*)a2dp_sink_info, info_len);
    }
    return true;
}

static void app_dbg_bt_link_loss_related_info_rcv_handle(uint8_t* info, uint32_t info_len)
{
    uint8_t subcode = info[0];
    switch (subcode)
    {
        case HCI_DBG_LINK_LOSS_INFO_EVT_SUBCODE:
            app_dbg_bt_update_analysing_link_loss_universal_info(&info[4], info_len - 4);
            break;
        case HCI_DBG_LINK_LOSS_CLOCK_INFO_EVT_SUBCODE:
            app_dbg_bt_update_analysing_link_loss_clock_info(&info[4], info_len - 4);
            break;
        case HCI_DBG_A2DP_SINK_INFO_EVT_SUBCODE:
            app_dbg_bt_update_analysing_a2dp_sink_info(&info[4], info_len - 4);
            break;
        default:
            TRACE(1, "app dbg link loss info:unknown subcode=0x%x",subcode);
            break;
    }
}

void app_dbg_bt_info_system_init(void)
{
    app_dbg_bt_register_link_loss_related_info_rcv_cb(app_dbg_bt_link_loss_related_info_rcv_handle);
}
#endif

app_dbg_bt_link_loss_related_info_rcv_cb app_dbg_bt_get_link_loss_related_info_rcv_cb(void)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    return app_bt_link_loss_dbg_info_rcv_cb;
#else
    return NULL;
#endif
}

void app_dbg_bt_register_link_loss_related_info_rcv_cb(app_dbg_bt_link_loss_related_info_rcv_cb func)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    app_bt_link_loss_dbg_info_rcv_cb = func;
#endif
}

void app_dbg_bt_register_link_loss_universal_info_received_cb(app_tws_ibrt_analysing_info_received_cb function)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    link_loss_universal_info_received_cb = function;
#endif
}

void app_dbg_bt_register_link_loss_clock_info_received_cb(app_tws_ibrt_analysing_info_received_cb function)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    link_loss_clock_info_received_cb = function;
#endif
}

void app_dbg_bt_register_a2dp_sink_info_received_cb(app_tws_ibrt_analysing_info_received_cb function)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    a2dp_sink_info_received_cb = function;
#endif
}

tws_ibrt_link_loss_info_t* app_dbg_bt_get_link_loss_total_info(void)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    return app_dbg_bt_link_loss_info;
#else
    return NULL;
#endif
}

tws_ibrt_a2dp_sink_info_t* app_dbg_bt_get_a2dp_sink_info(void)
{
#ifdef IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
    return &app_dbg_bt_a2dp_sink_info;
#else
    return NULL;
#endif
}


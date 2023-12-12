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
#ifndef APP_TILE_H_
#define APP_TILE_H_

#include "rwip_config.h"

#if (BLE_TILE)

typedef enum {
    APP_BLE_TILE_CONN_EVENT,
    APP_BLE_TILE_DISCONN_EVENT,
    APP_BLE_TILE_CONN_UPDATE_EVENT,
    APP_BLE_TILE_CH_CONN_EVENT,
    APP_BLE_TILE_CH_DISCONN_EVENT,
    APP_BLE_TILE_TX_DATA_DONE_EVENT,
    APP_BLE_TILE_RECEIVE_EVENT,
} APP_BLE_TILE_EVENT_TYPE_E;

typedef struct app_ble_tile_event_param
{
    /// connection index
    uint8_t  conidx;
    /// gatt event type, see@APP_BLE_TILE_EVENT_TYPE_E
    uint8_t  event_type;
    union {
         /// APP_BLE_TILE_TX_DATA_DONE_EVENT;
         uint8_t result;
         /// APP_BLE_TILE_CONN_EVENT
         /// APP_BLE_TILE_CONN_UPDATE_EVENT,
         struct {
             uint16_t interval;
             uint16_t latency;;
             uint16_t conn_sup_timeout;
         } ble_conn_param;
         /// APP_BLE_TILE_CH_CONN_EVENT
         /// APP_BLE_TILE_CH_DISCONN_EVENT
        struct {
            uint8_t hdl;
            uint8_t status;;
        } channel_conn;
        /// APP_BLE_TILE_RECEIVE_EVENT
        struct {
            uint8_t data_len;
            uint8_t *data;
        } receive;
    }data;
} app_ble_tile_event_param_t;

typedef void(* app_ble_tile_event_cb)(app_ble_tile_event_param_t *param);

extern const struct app_subtask_handlers app_tile_table_handler;

void app_ble_tile_add_svc(void);

void  app_tile_send_ntf(uint8_t conidx, uint8_t* ptrData, uint32_t length);

void app_tile_connected_evt_handler(uint8_t conidx, uint16_t interval, uint16_t latency, uint16_t timeout);

void app_tile_ble_conn_parameter_updated(uint8_t conidx, uint16_t interval, uint16_t latency, uint16_t timeout);

void app_tile_disconnected_evt_handler(uint8_t conidx);

void app_tile_event_cb_reg(app_ble_tile_event_cb cb);

#endif

#endif

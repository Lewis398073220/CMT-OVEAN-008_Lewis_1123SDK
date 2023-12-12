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
#ifndef __OTA_BLE_ADAPTER_H__
#define __OTA_BLE_ADAPTER_H__
#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus
#ifdef __IAG_BLE_INCLUDE__

enum ble_rx_data_handler_num {
#if defined(BES_OTA) && (!defined(OTA_OVER_TOTA_ENABLED))
    BLE_RX_DATA_SELF_OTA,
#endif
#ifdef BLE_TOTA_ENABLED
    BLE_RX_DATA_SELF_TOTA,
    BLE_RX_DATA_SELF_TOTA_OTA,
#endif
    BLE_RX_DATA_SELF_MAX,
};

typedef struct {
    uint8_t  flag;
    uint8_t  conidx;
    uint16_t len;
    uint8_t *ptr;
} BLE_RX_EVENT_T;

void ota_ble_adapter_init(void);

uint8_t ota_ble_get_conidx(void);

void ota_ble_push_rx_data(uint8_t flag, uint8_t conidx, uint8_t *ptr, uint16_t len);

uint8_t *ota_voicepath_get_common_ota_databuf(void);

#endif  // #ifndef __IAG_BLE_INCLUDE__

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // #ifndef __APP_BLE_RX_HANDLER_H__

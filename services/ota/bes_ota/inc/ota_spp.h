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
#ifndef __APP_SPP_OTA_H__
#define __APP_SPP_OTA_H__

#include "bluetooth_bt_api.h"
#include "spp_api.h"
#include "sdp_api.h"
#include "ota_control.h"

#define OTA_SPP_MAX_PACKET_SIZE     BT_SPP_MAX_TX_MTU
#define OTA_SPP_MAX_PACKET_NUM      3

#define OTA_SPP_RECV_BUFFER_SIZE   (BT_SPP_MAX_TX_MTU*6)

#define OTA_SPP_TX_BUF_SIZE         (OTA_SPP_MAX_PACKET_SIZE*OTA_SPP_MAX_PACKET_NUM)

#define APP_OTA_DATA_CMD_TIME_OUT_IN_MS	5000

typedef struct {
    bt_spp_channel_t  *ota_spp_dev;
    bool permissionToApply;
} OtaContext;

typedef void(*app_spp_ota_tx_done_t)(void);
void app_spp_ota_register_tx_done(app_spp_ota_tx_done_t callback);
void app_spp_ota_init(void);
void app_ota_send_cmd_via_spp(uint8_t* ptrData, uint16_t length);
void app_ota_send_data_via_spp(uint8_t* ptrData, uint32_t length);

uint16_t app_spp_ota_tx_buf_size(void);
void app_spp_ota_init_tx_buf(uint8_t* ptr);
uint8_t* app_spp_ota_fill_data_into_tx_buf(uint8_t* ptrData, uint32_t dataLen);
void ota_disconnect(void);

#endif


/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef __APP_SPP_TOTA_H__
#define __APP_SPP_TOTA_H__

#include "bluetooth_bt_api.h"

using namespace std;

typedef struct{
    void (*connected_cb)(void);
    void (*disconnected_cb)(void);
    void (*tx_done_cb)(void);
    void (*rx_cb)(uint8_t * buf, uint16_t len);
}tota_callback_func_t;

void app_spp_tota_init(const tota_callback_func_t *tota_callback_func);
bool app_spp_tota_send_data(uint8_t* ptrData, uint16_t length);
void app_spp_tota_client_open(bt_bdaddr_t *remote);

#endif
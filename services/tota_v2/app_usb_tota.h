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
#ifndef __APP_USB_TOTA_H__
#define __APP_USB_TOTA_H__

typedef struct {
    uint16_t len;
    uint8_t *ptr;
} USB_TOTA_RX_DATA_T;


void app_tota_usb_init(void);
void app_tota_send_data_via_usb(uint8_t* ptrData, uint32_t length);
bool app_usb_tota_send_data(uint8_t* ptrData, uint16_t length);
void usb_tota_push_rx_data(uint8_t* ptr, uint16_t len);
void app_tota_send_cmd_via_usb(uint8_t* ptrData, uint32_t length);

#endif


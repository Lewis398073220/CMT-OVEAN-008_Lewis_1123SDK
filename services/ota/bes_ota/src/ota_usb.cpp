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

#ifdef USB_OTA_ENABLE

#include "stdio.h"
#include "ota_usb.h"
#include "ota_control.h"
#include "usb_audio.h"
#include "hal_trace.h"

static bool usb_ota_flag_global = false;

void app_ota_send_data_via_usb(uint8_t* ptrData, uint32_t length){
    TRACE(1,"[%s] send data: ", __func__);
    DUMP8("%02x ", ptrData, (length<16?length:16));
    hid_epint_in_send_report(ptrData, length);
}

void app_ota_send_cmd_via_usb(uint8_t* ptrData, uint32_t length){
    TRACE(1,"[%s] send cmd: ", __func__);
    DUMP8("%02x ", ptrData, (length<16?length:16));
    hid_epint_in_send_report(ptrData, length);
}

static void ota_usb_rx_handler(uint8_t* data, uint32_t length){
    ota_control_handle_received_data(data, false, length);
}

void app_ota_usb_init(void){
    TRACE(1,"[%s], init USB OTA!!!", __func__);
    usb_ota_flag_global = true;
    ota_control_register_transmitter(app_ota_send_data_via_usb);
    usb_hid_enint_out_callback_register(ota_usb_rx_handler);
}

#endif


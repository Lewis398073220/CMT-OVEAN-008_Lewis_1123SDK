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

#ifndef __APP_USB_OTA_H__
#define __APP_USB_OTA_H__

void app_ota_usb_init(void);
void app_ota_send_data_via_usb(uint8_t* ptrData, uint32_t length);
void app_ota_send_cmd_via_usb(uint8_t* ptrData, uint32_t length);

#endif


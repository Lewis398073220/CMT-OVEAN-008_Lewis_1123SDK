
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
#ifndef __APP_USB_BIS_STREAM_H__
#define __APP_USB_BIS_STREAM_H__
#ifdef __cplusplus
extern "C" {
#endif
/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************type defination******************************/

/****************************function declaration***************************/
void app_usb_bis_src_init(void);

int gaf_usb_bis_src_audio_stream_start_handler(uint8_t grp_lid);

int gaf_usb_bis_src_audio_stream_stop_handler(uint8_t grp_lid);

void app_usb_stop_bis_src_stream(void);

void app_usb_start_bis_src_stream_with_new_bid(void);

#ifdef __cplusplus
}
#endif
#endif /* __APP_USB_BIS_STREAM_H__ */

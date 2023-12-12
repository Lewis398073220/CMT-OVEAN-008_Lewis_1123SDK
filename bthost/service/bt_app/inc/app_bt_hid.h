
/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifndef __APP_BT_HID_H__
#define __APP_BT_HID_H__
#include "hid_service.h"
#ifdef BT_HID_DEVICE
#ifdef __cplusplus
extern "C" {
#endif

#define HID_CTRL_KEY_NULL       0x00
#define HID_CTRL_KEY_POWER      0x01
#define HID_CTRL_KEY_PLAY       0x02
#define HID_CTRL_KEY_PAUSE      0x04
#define HID_CTRL_KEY_STOP       0x08
#define HID_CTRL_KEY_EJECT      0x10
#define HID_CTRL_KEY_PLAY_PAUSE 0x20
#define HID_CTRL_KEY_VOLUME_INC 0x40
#define HID_CTRL_KEY_VOLUME_DEC 0x80

#define HID_MOD_KEY_NULL    0x00
#define HID_MOD_KEY_L_CTRL  0x01
#define HID_MOD_KEY_L_SHIFT 0x02
#define HID_MOD_KEY_L_ALT   0x04
#define HID_MOD_KEY_L_WIN   0x08
#define HID_MOD_KEY_R_CTRL  0x10
#define HID_MOD_KEY_R_SHIFT 0x20
#define HID_MOD_KEY_R_ALT   0x40
#define HID_MOD_KEY_R_WIN   0x80

#define HID_KEY_CODE_NULL   0x00
#define HID_KEY_CODE_A      0x04
#define HID_KEY_CODE_Z      0x1d
#define HID_KEY_CODE_1      0x1e
#define HID_KEY_CODE_9      0x26
#define HID_KEY_CODE_0      0x27
#define HID_KEY_CODE_ENTER  0x28
#define HID_KEY_CODE_ESC    0x29
#define HID_KEY_CODE_DEL    0x2a
#define HID_KEY_CODE_TAB    0x2b
#define HID_KEY_CODE_SPACE  0x2c
#define HID_KEY_CODE_VOLUP  0x80
#define HID_KEY_CODE_VOLDN  0x81

typedef struct hid_control_t* hid_channel_t;

bool app_bt_hid_is_in_shutter_mode(void);
void app_bt_hid_enter_shutter_mode(void);
void app_bt_hid_exit_shutter_mode(void);
void app_bt_hid_send_capture(void);
void app_bt_hid_disconnect_all_channels(void);

bt_status_t app_bt_hid_init(bt_hid_callback_t callback);
bt_status_t app_bt_hid_cleanup(void);
bt_status_t app_bt_hid_virtual_cable_unplug(const bt_bdaddr_t *remote);
bt_status_t app_bt_hid_profile_connect(const bt_bdaddr_t *remote, int capture);
bt_status_t app_bt_hid_profile_disconnect(const bt_bdaddr_t *remote);
bt_status_t app_bt_hid_send_sensor_report(const bt_bdaddr_t *remote, const struct bt_hid_sensor_report_t *report);
bt_status_t app_bt_hid_process_sensor_report(const bt_bdaddr_t *remote, const struct bt_hid_sensor_report_t *report);
bt_status_t app_bt_hid_connect(const bt_bdaddr_t *remote);
bt_status_t app_bt_hid_disconnect(const bt_bdaddr_t *remote);
bt_status_t app_bt_hid_capture_process(const bt_bdaddr_t *remote);
bt_status_t app_bt_hid_send_keyboard_report(const bt_bdaddr_t *remote, uint8_t modifier_key, uint8_t key_code);
bt_status_t app_bt_hid_send_mouse_report(const bt_bdaddr_t *remote, int8_t x_pos, int8_t y_pos, uint8_t clk_buttons);
bt_status_t app_bt_hid_send_mouse_control_report(const bt_bdaddr_t *remote, uint8_t ctl_buttons);

#ifdef __cplusplus
}
#endif
#endif /* BT_HID_DEVICE */
#endif /* __APP_BT_HID_H__ */

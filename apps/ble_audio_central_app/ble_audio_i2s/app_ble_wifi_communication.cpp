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
#ifdef BLE_WIFI_DUAL_CHIP_MODE
#include "hal_trace.h"
#include "app_chip_bridge.h"
#include "app_ble_wifi_communication.h"

#if 1//def USB_AUDIO_CUSTOM_USB_HID_KEY
#include "app_ble_cmd_handler.h"
#include "app_ble_custom_cmd.h"
#endif

#ifdef DUAL_CHIP_MODE_ROLE_BLE
#include "besbt.h"
#include "gapm_le.h"
#include "app_ibrt_internal.h"
#include "app_tws_ibrt.h"
#include "bluetooth_ble_api.h"
#include "app_bt_media_manager.h"
#include "ble_audio_earphone_info.h"
#include "app_ble_i2s_audio_stream.h"

#define APP_BLE_WIFI_AUDIO_MANAGER_GET_AUD_ID(msg_id, aud_id) \
                (aud_id = (msg_id + APP_BLE_WIFI_AUDIO_MANAGER_CMD_BASE))

void app_ble_wifi_dual_mode_audio_manager_send_request_locally(APP_BLE_WIFI_AUDIO_MANAGER_ID_E cmd, uint32_t param0, uint32_t param1);
#endif

#ifdef DUAL_CHIP_MODE_ROLE_WIFI
#include <string.h>
#include "usb_audio.h"
extern "C" void vowlan_app_init(void);
#endif

static bool wifi_pairing_mode = false;

static APP_BLE_WIFI_PAIRING_INFO_PACKET_T wifi_pairing_info = {
    "vow_2005",
    "",
     sizeof("vow_2005"),
     sizeof("")
};

APP_BLE_WIFI_PAIRING_INFO_PACKET_T* app_get_wifi_pairing_info_ptr(void)
{
    return &wifi_pairing_info;
}

bool app_ble_wifi_is_pairing_mode(void)
{
    return wifi_pairing_mode;
}

void app_ble_wifi_enter_pairing_mode(void)
{
    wifi_pairing_mode = true;

#if defined(DUAL_CHIP_MODE_ROLE_BLE)
#if defined(AOB_MOBILE_ENABLED)
    if (ble_audio_is_ux_mobile()) {
        bes_ble_gap_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
    }
#else
    if (ble_audio_is_ux_master()) {
        bes_ble_scan_param_t scan_param = {0};

        scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
        scan_param.scanWindowMs = 20;
        scan_param.scanIntervalMs = 50;
        bes_ble_gap_start_scan(&scan_param);
    }
#endif // AOB_MOBILE_ENABLED
#elif defined(DUAL_CHIP_MODE_ROLE_WIFI) //DUAL_CHIP_MODE_ROLE_BLE
    vowlan_app_init();
#endif //DUAL_CHIP_MODE_ROLE_WIFI
}

static void app_ble_wifi_dual_mode_audio_manager_cmd_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "<---- send audio manager cmd,len = %d", length);
    app_chip_bridge_send_data_with_waiting_rsp(BLE_WIFI_AUDIO_MANAGER_CMD, (uint8_t *)p_buff, length);
}

static void app_ble_wifi_dual_mode_audio_manager_receive_handler(uint8_t *p_buff, uint16_t length)
{
    ASSERT(p_buff, "%s recv invalid cmd.", __func__);
    ASSERT(length == sizeof(APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T), "%s recv invalid cmd length", __func__);

    TRACE(1, "----> recv dual mode audio manager cmd:");
    DUMP8("%02x ", p_buff, sizeof(APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T));

#ifdef DUAL_CHIP_MODE_ROLE_BLE
    APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T r_packet;
    uint32_t audio_manager_id = 0;
    uint32_t audio_manager_param0 = 0;
    uint32_t audio_manager_param1 = 0;
    memcpy(&r_packet, p_buff, length);

    APP_BLE_WIFI_AUDIO_MANAGER_GET_AUD_ID(r_packet.message_id, audio_manager_id);
    audio_manager_param0 = r_packet.message_param0;
    audio_manager_param1 = r_packet.message_param1;

    switch (audio_manager_id)
    {
        case APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_VOLUME_CTRL:
            TRACE(1, "set new volume level:%d", audio_manager_param0);
            // fall through
        case APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_START:
        case APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_STOP:
        case APP_BLE_WIFI_AUDIO_MANAGER_CAPTURE_STREAM_START:
        case APP_BLE_WIFI_AUDIO_MANAGER_CAPTURE_STREAM_STOP:
            app_ble_wifi_dual_mode_audio_manager_send_request_locally((APP_BLE_WIFI_AUDIO_MANAGER_ID_E)audio_manager_id,
                audio_manager_param0, audio_manager_param1);
        break;

        default:
            ASSERT(0, "recv invalid cmd:0x%x", audio_manager_id);
        break;
    }
#endif
    app_chip_bridge_send_rsp(BLE_WIFI_AUDIO_MANAGER_CMD, p_buff, length);
}

static void app_ble_wifi_dual_mode_audio_manager_wait_rsp_timeout(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);

    static uint8_t resend_time = RESEND_TIME;
    if(resend_time > 0)
    {
        app_chip_bridge_send_cmd(BLE_WIFI_AUDIO_MANAGER_CMD, p_buff, length);
        resend_time--;
    }
    else
    {
        resend_time = RESEND_TIME;
    }
}

static void app_ble_wifi_dual_mode_audio_manager_receive_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
}

static void app_ble_wifi_dual_mode_audio_manager_send_cmd(uint32_t send_cmd, uint32_t param0, uint32_t param1)
{
#ifdef APP_CHIP_BRIDGE_MODULE
    APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T s_packet;
    s_packet.message_id = send_cmd;
    s_packet.message_param0 = param0;
    s_packet.message_param1 = param1;
    app_chip_bridge_send_cmd(BLE_WIFI_AUDIO_MANAGER_CMD, (uint8_t*)&s_packet, sizeof(APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T));
#endif
}

void app_ble_wifi_dual_mode_audio_manager_send_request_remotely(APP_BLE_WIFI_DUAL_MODE_COMM_ID_E cmd, uint32_t param0, uint32_t param1)
{
    app_ble_wifi_dual_mode_audio_manager_send_cmd(cmd, param0, param1);
}

CHIP_BRIDGE_TASK_COMMAND_TO_ADD(BLE_WIFI_AUDIO_MANAGER_CMD,
                                "dual mode audio manager cmd",
                                app_ble_wifi_dual_mode_audio_manager_cmd_handler,
                                app_ble_wifi_dual_mode_audio_manager_receive_handler,
                                APP_CHIP_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                app_ble_wifi_dual_mode_audio_manager_wait_rsp_timeout,
                                app_ble_wifi_dual_mode_audio_manager_receive_rsp_handler,
                                NULL);

static void app_ble_wifi_dual_mode_media_control_cmd_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "<---- send media control cmd,len = %d", length);
    app_chip_bridge_send_data_with_waiting_rsp(BLE_WIFI_MEDIA_CONTROL_CMD, (uint8_t *)p_buff, length);
}

static void app_ble_wifi_dual_mode_media_control_receive_handler(uint8_t *p_buff, uint16_t length)
{
    ASSERT(p_buff, "%s recv invalid cmd.", __func__);
    ASSERT(length == sizeof(APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E), "%s recv invalid cmd length", __func__);

    TRACE(1, "----> recv dual media control cmd:");
    DUMP8("%02x ", p_buff, sizeof(APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E));

#if defined(DUAL_CHIP_MODE_ROLE_WIFI)
#if defined(VOWLAN_MASTER)
    enum USB_AUDIO_HID_EVENT_T hid_event = (enum USB_AUDIO_HID_EVENT_T)(p_buff[0] | p_buff[1] << 8);
    // usb_audio_hid_set_event(hid_event, 1);
    // usb_audio_hid_set_event(hid_event, 0);
    usb_audio_hid_set_event(hid_event, 1);
    usb_audio_hid_set_event(hid_event, 0);
#endif
#endif

    app_chip_bridge_send_rsp(BLE_WIFI_MEDIA_CONTROL_CMD, p_buff, length);
}

static void app_ble_wifi_dual_mode_media_control_wait_rsp_timeout(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);

    static uint8_t resend_time = RESEND_TIME;
    if(resend_time > 0)
    {
        app_chip_bridge_send_cmd(BLE_WIFI_MEDIA_CONTROL_CMD, p_buff, length);
        resend_time--;
    }
    else
    {
        resend_time = RESEND_TIME;
    }
}

static void app_ble_wifi_dual_mode_media_control_receive_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
}

void app_ble_wifi_dual_mode_media_control_send_cmd(APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E cmd)
{
#if defined(DUAL_CHIP_MODE_ROLE_BLE)
#if defined(AOB_MOBILE_ENABLED)
    if (ble_audio_is_ux_mobile()) {
        app_chip_bridge_send_cmd(BLE_WIFI_MEDIA_CONTROL_CMD, (uint8_t*)&cmd, sizeof(cmd));
    }
#endif // AOB_MOBILE_ENABLED
#endif // DUAL_CHIP_MODE_ROLE_BLE
}

CHIP_BRIDGE_TASK_COMMAND_TO_ADD(BLE_WIFI_MEDIA_CONTROL_CMD,
                                "media control",
                                app_ble_wifi_dual_mode_media_control_cmd_handler,
                                app_ble_wifi_dual_mode_media_control_receive_handler,
                                APP_CHIP_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                app_ble_wifi_dual_mode_media_control_wait_rsp_timeout,
                                app_ble_wifi_dual_mode_media_control_receive_rsp_handler,
                                NULL);

static void app_ble_wifi_pairing_info_exchange_cmd_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "<---- send pairing info exchange cmd,len = %d", length);
    app_chip_bridge_send_data_with_waiting_rsp(BLE_WIFI_PAIRING_INFO_EXCHANGE, (uint8_t *)p_buff, length);
}

static void app_ble_wifi_pairing_info_exchange_receive_handler(uint8_t *p_buff, uint16_t length)
{
    ASSERT(p_buff, "%s recv invalid cmd.", __func__);
    ASSERT(length == sizeof(APP_BLE_WIFI_PAIRING_INFO_PACKET_T), "%s recv invalid cmd length", __func__);

    TRACE(1, "----> recv pairing info exchange cmd:");
    DUMP8("%02x ", p_buff, 16);

    memcpy(&wifi_pairing_info, p_buff, length);

    app_ble_wifi_enter_pairing_mode();

    app_chip_bridge_send_rsp(BLE_WIFI_PAIRING_INFO_EXCHANGE, p_buff, length);
}

static void app_ble_wifi_pairing_info_exchange_wait_rsp_timeout(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);

    static uint8_t resend_time = RESEND_TIME;
    if(resend_time > 0)
    {
        app_chip_bridge_send_cmd(BLE_WIFI_PAIRING_INFO_EXCHANGE, p_buff, length);
        resend_time--;
    }
    else
    {
        resend_time = RESEND_TIME;
    }
}

static void app_ble_wifi_pairing_info_exchange_receive_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
}

CHIP_BRIDGE_TASK_COMMAND_TO_ADD(BLE_WIFI_PAIRING_INFO_EXCHANGE,
                                "pairing info exchange",
                                app_ble_wifi_pairing_info_exchange_cmd_handler,
                                app_ble_wifi_pairing_info_exchange_receive_handler,
                                APP_CHIP_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                app_ble_wifi_pairing_info_exchange_wait_rsp_timeout,
                                app_ble_wifi_pairing_info_exchange_receive_rsp_handler,
                                NULL);

void app_ble_wifi_pairing_info_exchange_send_cmd(APP_BLE_WIFI_PAIRING_INFO_PACKET_T pairing_info)
{
    ASSERT(pairing_info.ap_ssid_length <= PAIRING_INFO_SSID_MAX_LENGTH, "%s invalid ssid:%d.", __func__, pairing_info.ap_ssid_length);
    ASSERT(pairing_info.ap_password_length <= PAIRING_INFO_PASSWORD_MAX_LENGTH, "%s invalid password:%d", __func__, pairing_info.ap_password_length);

    memcpy(&wifi_pairing_info, &pairing_info, sizeof(APP_BLE_WIFI_PAIRING_INFO_PACKET_T));

    TRACE(1, "ap ssid: %s", wifi_pairing_info.ap_ssid);
    DUMP8("%02x ", wifi_pairing_info.ap_ssid, wifi_pairing_info.ap_ssid_length > 16 ? 16 : wifi_pairing_info.ap_ssid_length);
    TRACE(1, "ap password: %s", wifi_pairing_info.ap_password);
    DUMP8("%02x ", wifi_pairing_info.ap_password, wifi_pairing_info.ap_password_length > 16 ? 16 : wifi_pairing_info.ap_password_length);

    app_chip_bridge_send_cmd(BLE_WIFI_PAIRING_INFO_EXCHANGE, (uint8_t*)&wifi_pairing_info, sizeof(APP_BLE_WIFI_PAIRING_INFO_PACKET_T));
}

#ifdef DONGLE_AS_I2S_MASTER
void app_ble_wifi_dual_mode_audio_manager_playback_stream_onoff(void)
{
    static bool on_off = false;

    if (on_off == false) {
        app_ble_wifi_dual_mode_audio_manager_send_request_locally(APP_BLE_WIFI_AUDIO_MANAGER_CAPTURE_STREAM_START, 0, 0);
        app_ble_wifi_dual_mode_audio_manager_send_request_remotely(APP_BLE_WIFI_COMM_PLAYBACK_STREAM_START, 0, 0);
    }else {
        app_ble_wifi_dual_mode_audio_manager_send_request_locally(APP_BLE_WIFI_AUDIO_MANAGER_CAPTURE_STREAM_STOP, 0, 0);
        app_ble_wifi_dual_mode_audio_manager_send_request_remotely(APP_BLE_WIFI_COMM_PLAYBACK_STREAM_STOP, 0, 0);
    }

    on_off = (on_off == true) ? false : true;

    TRACE(1, "aud manager: playback stream [%s]", on_off == true ? "start" : "stop");
}

void app_ble_wifi_dual_mode_audio_manager_capture_stream_onoff(void)
{
    static bool on_off = false;

    if (on_off == false) {
        app_ble_wifi_dual_mode_audio_manager_send_request_locally(APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_START, 0, 0);
        app_ble_wifi_dual_mode_audio_manager_send_request_remotely(APP_BLE_WIFI_COMM_CAPTURE_STREAM_START, 0, 0);
    }else {
        app_ble_wifi_dual_mode_audio_manager_send_request_locally(APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_STOP, 0, 0);
        app_ble_wifi_dual_mode_audio_manager_send_request_remotely(APP_BLE_WIFI_COMM_CAPTURE_STREAM_STOP, 0, 0);
    }

    on_off = (on_off == true) ? false : true;

    TRACE(1, "aud manager: capture stream [%s]", on_off == true ? "start" : "stop");
}
#endif // DONGLE_AS_I2S_MASTER

#ifdef DUAL_CHIP_MODE_ROLE_BLE
void app_ble_wifi_dual_mode_audio_manager_send_request_locally(APP_BLE_WIFI_AUDIO_MANAGER_ID_E cmd, uint32_t param0, uint32_t param1)
{
#ifdef WORKAROUND_FOR_UART1_NOT_WORKING
    APP_BLE_WIFI_DUAL_MODE_COMM_ID_E audio_cmd = (APP_BLE_WIFI_DUAL_MODE_COMM_ID_E)(cmd - APP_BLE_WIFI_AUDIO_MANAGER_CMD_BASE);
    if (param0 && (cmd == APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_START || cmd == APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_STOP)) {
        TRACE(1, "dongle transfers the uart cmd to the ble cmd to headser.(0x%0x)", audio_cmd);
        APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T s_packet = {0};
        s_packet.message_id = audio_cmd;
        BLE_send_custom_command(OP_SEND_HID_COMMAND_TO_DONGLE,
                                    (uint8_t*)&s_packet,
                                    sizeof(s_packet),
                                    TRANSMISSION_VIA_WRITE_CMD);
        return ;
    } else if (cmd == APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_VOLUME_CTRL) {
        TRACE(1, "dongle transfers the uart cmd to the ble cmd to headser.(0x%0x)", audio_cmd);
        APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T s_packet = {0};
        s_packet.message_id = audio_cmd;
        s_packet.message_param0 = param0;
        BLE_send_custom_command(OP_SEND_HID_COMMAND_TO_DONGLE,
                                    (uint8_t*)&s_packet,
                                    sizeof(APP_BLE_WIFI_DUAL_MODE_COMM_PACKET_T),
                                    TRANSMISSION_VIA_WRITE_CMD);
    }
#endif

    app_audio_manager_sendrequest(cmd, param0, param1, 0);
}

#ifdef AOB_MOBILE_ENABLED
APP_BLE_WIFI_AUDIO_MANAGER_CALLBACK_T lea_mobile_audio_manager =
{
    /** APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_START **/
    .playback_start = app_ble_audio_i2s_playback_stream_start,

    /** APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_STOP **/
    .playback_stop = app_ble_audio_i2s_playback_stream_stop,

    /** APP_BLE_WIFI_AUDIO_MANAGER_CAPTURE_STREAM_START **/
    .capture_start = app_ble_audio_i2s_capture_stream_start,

    /** APP_BLE_WIFI_AUDIO_MANAGER_CAPTURE_STREAM_STOP **/
    .capture_stop = app_ble_audio_i2s_capture_stream_stop,

    /** APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_VOLUME_CTRL **/
    .volume_control = app_ble_audio_i2s_playback_stream_vol_control,
};
#else
APP_BLE_WIFI_AUDIO_MANAGER_CALLBACK_T lea_master_audio_manager =
{
    /** APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_START **/
    .playback_start = app_ble_audio_low_latency_playback_stream_start,

    /** APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_STREAM_STOP **/
    .playback_stop = app_ble_audio_low_latency_playback_stream_stop,

    /** APP_BLE_WIFI_AUDIO_MANAGER_PLAYBACK_VOLUME_CTRL **/
    .volume_control = app_ble_audio_low_latency_playback_stream_vol_control,
};
#endif

void app_ble_wifi_dual_mode_init(void)
{
#ifdef AOB_MOBILE_ENABLED
    app_ble_wifi_audio_manager_register_callback(&lea_mobile_audio_manager);
#else
    app_ble_wifi_audio_manager_register_callback(&lea_master_audio_manager);
#endif
}

#if 1//def USB_AUDIO_CUSTOM_USB_HID_KEY

#ifdef AOB_MOBILE_ENABLED
static void app_ble_wifi_dongle_receive_hid_cmd_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE(0,"%s hid data: ",__func__);
    DUMP8("%02x ", ptrParam, paramLen);
    APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E hid_cmd = (APP_BLE_WIFI_DUAL_MODE_MEDIA_CTRL_CMD_E)*ptrParam;
    TRACE(0,"%s hid_cmd:0x%x ",__func__,hid_cmd);
    switch (hid_cmd)
    {
        case APP_BLE_WIFI_MEDIA_CTRL_VOLUME_UP:
        case APP_BLE_WIFI_MEDIA_CTRL_VOLUME_DOWN:
        case APP_BLE_WIFI_MEDIA_CTRL_PLAY_PAUSE:
        case APP_BLE_WIFI_MEDIA_CTRL_SCAN_NEXT:
        case APP_BLE_WIFI_MEDIA_CTRL_SCAN_PREV:
        case APP_BLE_WIFI_MEDIA_CTRL_VOL_MUTE:
        case APP_BLE_WIFI_MEDIA_CTRL_MIC_MUTE:
            app_ble_wifi_dual_mode_media_control_send_cmd(hid_cmd);
            break;
        default:
            ASSERT(0, "%s recv invalid hid cmd.", __func__);
            break;
    }
}

BLE_CUSTOM_COMMAND_TO_ADD(OP_SEND_HID_COMMAND_TO_DONGLE,
                        app_ble_wifi_dongle_receive_hid_cmd_handler,
                        false,
                        0,
                        NULL);
#else
static void app_ble_wifi_headset_receive_cmd_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE(0,"%s audio cmd: ",__func__);
    DUMP8("%02x ", ptrParam, paramLen);

    app_ble_wifi_dual_mode_audio_manager_receive_handler((uint8_t*)ptrParam, paramLen);
}

BLE_CUSTOM_COMMAND_TO_ADD(OP_SEND_HID_COMMAND_TO_DONGLE,
                        app_ble_wifi_headset_receive_cmd_handler,
                        false,
                        0,
                        NULL);
#endif // AOB_MOBILE_ENABLED

#endif // USB_AUDIO_CUSTOM_USB_HID_KEY

#endif // DUAL_CHIP_MODE_ROLE_BLE

#endif // BLE_WIFI_DUAL_CHIP_MODE
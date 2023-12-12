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

/*****************************header include********************************/
#include "app_trace_rx.h"
#include "plat_types.h"
#include "cqueue.h"
#include "ble_audio_dbg.h"
#include "ble_audio_test.h"
#include "app_key.h"
#include "bluetooth_bt_api.h"
#include "bluetooth_ble_api.h"
#include "app_bt_func.h"
#include "bluetooth_ble_api.h"
#include "app_sec.h"

#include "app_gaf_define.h"
#include "app_bap_uc_cli_msg.h"
#include "app_arc_micc_msg.h"
#include "app_acc_tbc_msg.h"
#include "app_acc_tbs_msg.h"
#include "app_tmap.h"
// TODO: delete this header file
#include "app_gaf_custom_api.h"
#include "aob_call_api.h"
#include "app_bap_bc_src_msg.h"
#include "app_bap_bc_scan_msg.h"
#include "app_bap_bc_deleg_msg.h"
#include "app_bap_bc_assist_msg.h"
#include "gaf_media_sync.h"
#include "aob_media_api.h"
#include "aob_volume_api.h"
#include "aob_conn_api.h"
#include "aob_bis_api.h"
#include "aob_csip_api.h"
#include "aob_gaf_api.h"
#include "ble_audio_earphone_info.h"
#include "ble_audio_core_api.h"
#include "ble_audio_mobile_info.h"
#include "app_gaf_custom_api.h"
#include "aob_iap_tm_api.h"
#include "aob_hap_api.h"
#include "aob_dts_api.h"
#include "aob_dtc_api.h"
#include "aob_csip_api.h"
#include "aob_pacs_api.h"
#include "gaf_media_common.h"
#include "apps.h"
#include "app_ibrt_internal.h"
#include "rwip_config.h"
#include "earbud_ux_api.h"

#include "app_ble_tws_sync.h"
#include "app_ble_cmd_handler.h"
#include "app_ble_custom_cmd.h"

#include "stdio.h"
#include "string.h"
#include "cmsis_os.h"
#include "stdlib.h"
#include "hal_timer.h"

#if BLE_AUDIO_ENABLED
#include "aob_gatt_cache.h"
#include "ble_audio_mobile_conn_stm.h"
#include "ble_audio_core.h"
#endif

#ifdef IS_BLE_AUDIO_PTS_TEST_ENABLED
#include "arc_aics.h"
#include "gapc_le_con.h"

#endif

#include "nvrecord_ble.h"

#include "gaf_media_stream.h"

#include "app_audio_active_device_manager.h"

#ifdef WIRELESS_MIC
#include "app_wireless_mic.h"
#include "app_wireless_mic_opcode.h"
#endif

/*********************external function declaration*************************/

/************************private macro defination***************************/
#define INVALID_CONIDX          (0xFF)
#define INVALID_CALL_ID         (0)
#define TEST_CALL_CON_LID       (1) //!< mobile connection index
#define TEST_CALL_BEARER_LID    (0)
#define TEST_CALL_CALL_ID       (0)
#define TEST_INCOMING_CALL_ID   (1)
#define EARBUDS_DFT_CALL_OUTGOING_URI "tel:+13222222222"

#define AOB_CUSTOM_GAF_STREAM_DATA_DWT_ENABLE (0)

uint8_t New_Rtn_num = 0xFF;
uint8_t BAP_Codec_Capability_Cfg = 0xFF;
/************************private type defination****************************/
typedef void (*ble_audio_uart_test_function_handle)(void);

typedef struct
{
    const char* string;
    ble_audio_uart_test_function_handle function;
} ble_audio_uart_handle_t;

typedef void (*ble_audio_uart_test_function_handle_with_param)(
    char* cmdParam, uint32_t cmdParamLen);

typedef struct
{
    const char* string;
    ble_audio_uart_test_function_handle_with_param function;
} ble_audio_uart_handle_with_param_t;

/**********************private function declaration*************************/

/************************private variable defination************************/
static uint8_t broadcast_id[3]    = {0x11, 0x22, 0x11};
static uint8_t broadcast_code[16] = {0x01, 0x02, 0x03, 0x04, \
                                     0x05, 0x06, 0x07, 0x08, \
                                     0x08, 0x07, 0x06, 0x05, \
                                     0x04, 0x03, 0x02, 0x01};

#ifdef AOB_MOBILE_ENABLED
static const CIS_TIMING_CONFIGURATION_T dft_cis_timing =
{
    .m2s_bn = 1,
    .m2s_nse = 2,
    .m2s_ft = 4,
    .s2m_bn = 1,
    .s2m_nse = 2,
    .s2m_ft = 4,
    .frame_cnt_per_sdu = 1,
#ifdef AOB_LOW_LATENCY_MODE
    .iso_interval = 4,  // 5ms = 4 * 1.25ms
#else
#ifdef BLE_AUDIO_FRAME_DUR_7_5MS
    .iso_interval = 6,  // 7.5ms = 6 * 1.25ms
#else
    .iso_interval = 8,  // 10ms = 8 * 1.25ms
#endif
#endif
};

#if defined (AOB_GMAP_ENABLED)
static const CIS_TIMING_CONFIGURATION_T gmap_cis_timing =
{
    .m2s_bn = 1, .m2s_nse = 3, .m2s_ft = 2,
    .s2m_bn = 1, .s2m_nse = 3, .s2m_ft = 2,
    .frame_cnt_per_sdu = 1,
    .iso_interval = 8,
};
#endif

#ifdef BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT
static const CIS_TIMING_CONFIGURATION_T stereo_cis_timing =
{
    .m2s_bn = 2, .m2s_nse = 4, .m2s_ft = 4,
    .s2m_bn = 2, .s2m_nse = 4, .s2m_ft = 4,
    .frame_cnt_per_sdu = 1,
#ifdef BLE_AUDIO_FRAME_DUR_7_5MS
    .iso_interval = 6,
#else
    .iso_interval = 8,
#endif
};
#endif
#endif
/****************************function defination****************************/

void ble_audio_test_key_io_event(APP_KEY_STATUS *status, void *param)
{
    TRACE(3, "%s %d,%d",__func__, status->code, status->event);

    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            if (status->code== APP_KEY_CODE_FN1)
            {
                // aob_ux_case_open_handler();
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
                // aob_ux_bud_undock_handler();
            }
            else if (status->code== APP_KEY_CODE_FN3)
            {
                // aob_ux_bud_wearon_handler();
            }
            break;

        case APP_KEY_EVENT_DOUBLECLICK:
            if (status->code== APP_KEY_CODE_FN1)
            {
                // aob_ux_case_close_handler();
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
                // aob_ux_bud_dock_handler();
            }
            else if (status->code== APP_KEY_CODE_FN3)
            {
                // aob_ux_bud_wearoff_handler();
            }
            break;

        case APP_KEY_EVENT_LONGPRESS:
            if (status->code== APP_KEY_CODE_FN1)
            {
                // if (app_is_ux_master())
                // {
                //     aob_media_play(0);
                // }
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
            }
            else
            {
            }
            break;

        case APP_KEY_EVENT_TRIPLECLICK:
            break;

        case HAL_KEY_EVENT_LONGLONGPRESS:
            break;

        case APP_KEY_EVENT_ULTRACLICK:
            break;

        case APP_KEY_EVENT_RAMPAGECLICK:
            break;
    }
}

const APP_KEY_HANDLE  ble_audio_test_key_cfg[] =
{
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_DOUBLECLICK},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_CLICK},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_DOUBLECLICK},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"aob_ux_io_test_key", ble_audio_test_key_io_event, NULL},
};

void ble_audio_test_key_init(void)
{
    TRACE(0, "%s", __func__);
    app_key_handle_clear();
    for (uint8_t i=0; i<ARRAY_SIZE(ble_audio_test_key_cfg); i++)
    {
        app_key_handle_registration(&ble_audio_test_key_cfg[i]);
    }
}

POSSIBLY_UNUSED static void aob_toggle_gpio_20_test(void)
{
    TRACE(0,"%s ",__func__);
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_detecter_cfg, 1);
    hal_gpio_pin_set_dir(HAL_GPIO_PIN_P2_0, HAL_GPIO_DIR_OUT, 0);
    hal_gpio_pin_set_dir(HAL_GPIO_PIN_P2_0, HAL_GPIO_DIR_OUT, 1);
}


#if (BLE_APP_DATAPATH_SERVER)
static void ble_datapath_transmission_by_notification_test(void)
{
    BLE_CUSTOM_CMD_RET_STATUS_E ret;
    //aob_toggle_gpio_20_test();
    uint8_t test_code[10]= {0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20};
    ret = BLE_send_custom_command(OP_DONGLE_TOTA_RECEIVE_DATA, test_code, sizeof(test_code),
                                    TRANSMISSION_VIA_NOTIFICATION);
    TRACE(0,"%s   ret %d",__func__,ret);
}

#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
typedef enum
{
    USB_AUDIO_HID_KEY_MIN = 0,
    USB_AUDIO_HID_KEY_VOL_UP = USB_AUDIO_HID_KEY_MIN ,
    USB_AUDIO_HID_KEY_VOL_DOWN,
    USB_AUDIO_HID_KEY_VOL_MUTE,
    USB_AUDIO_HID_KEY_PLAY,
    USB_AUDIO_HID_KEY_PAUSE,
    USB_AUDIO_HID_KEY_NEXT_TRACK,//5
    USB_AUDIO_HID_KEY_PREVIOUS_TRACK,
    USB_AUDIO_HID_KEY_MIC_MUTE,
    USB_AUDIO_HID_KEY_HOOK_SWITCH,
    USB_AUDIO_HID_KEY_PLAY_PAUSE,
    USB_AUDIO_HID_KEY_MAX,
} USB_AUDIO_HID_KEY_E;

static void send_hid_cmd_data_to_dongle_by_datapath(char *cmdParam, uint32_t cmdParam_len)
{
    BLE_CUSTOM_CMD_RET_STATUS_E ret;

    int hid_cmd= 0;
    USB_AUDIO_HID_KEY_E _hid_cmd = USB_AUDIO_HID_KEY_MIN;
    sscanf(cmdParam, "%d", &hid_cmd);
    if (hid_cmd >= USB_AUDIO_HID_KEY_MIN && hid_cmd < USB_AUDIO_HID_KEY_MAX)
    {
        _hid_cmd = (USB_AUDIO_HID_KEY_E)hid_cmd;
    }
    TRACE(0,"%s hid_cmd:%d  %d",__func__,hid_cmd,sizeof(hid_cmd));
    ret = BLE_send_custom_command(OP_SEND_HID_COMMAND_TO_DONGLE,
                                    (uint8_t*)&_hid_cmd,
                                    sizeof(_hid_cmd),
                                    TRANSMISSION_VIA_NOTIFICATION);
    TRACE(0,"%s ret:%d ",__func__,ret);
}
#endif
#endif

#if (BLE_APP_DATAPATH_CLIENT)
void ble_app_datapath_client_control_notification_test(void)
{
    TRACE(0,"ble_app_datapath_client_control_notification_test");
    bes_ble_datapath_client_control_notification(0, TRUE);
}

static void ble_datapath_transmission_by_command_test(void)
{
    BLE_CUSTOM_CMD_RET_STATUS_E ret;
    uint8_t test_code[90]= {
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20,
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x40,
    0x21,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20,
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x40,
    0x31,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20,
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x40};

    ret = BLE_send_custom_command(OP_HEADSET_TOTA_RECEIVE_DATA, test_code, sizeof(test_code),
                                    TRANSMISSION_VIA_WRITE_CMD);
    TRACE(0,"%s   ret %d",__func__,ret);
}
#endif

void ble_audio_open_adv_test(void)
{
    TRACE(0,"ble_audio_open_adv_test");
    bes_ble_gap_force_switch_adv(BLE_SWITCH_USER_BOX, true);
}

void ble_audio_close_adv_test(void)
{
    TRACE(0,"ble_audio_close_adv_test");
    bes_ble_gap_force_switch_adv(BLE_SWITCH_USER_BOX, false);
}

void ble_audio_open_scan_test(void)
{
    TRACE(0,"ble_audio_open_scan_test");
    bes_ble_scan_param_t scan_param = {0};

    scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
    scan_param.scanWindowMs   = 10;
    scan_param.scanIntervalMs   = 60;
    bes_ble_gap_start_scan(&scan_param);
}

extern "C" void ble_audio_close_scan_test(void)
{
    TRACE(0,"ble_audio_close_scan_test");
    bes_ble_gap_stop_scan();
}

void ble_audio_disconnect_test(void)
{
    TRACE(0,"ble_audio_disconnect_test");
    bes_ble_gap_disconnect_all();
}

void ble_set_rpa_timeout(void)
{
    uint16_t timeout = 0x0388;
    TRACE(0,"ble_set_rpa_timeout");
    appm_le_set_rpa_timeout(timeout);
}

static void _str_to_hex(const char *pSrc, uint8_t *pDst, int len)
{
    uint8_t shift_H = 0;
    uint8_t shift_L = 0;
    memset(&pDst[0], 0, len);

    for (uint8_t i = 0; i < len; i++)
    {
        if (pSrc[i * 2] <= '9')
        {
            shift_H = 0 - '0';
        }
        else if (pSrc[i * 2] <= 'F')
        {
            shift_H = 0 - 'A' + 0x0A;
        }
        else
        {
            shift_H = 0 - 'a' + 0x0A;
        }

        if (pSrc[i * 2 + 1] <= '9')
        {
            shift_L = 0 - '0';
        }
        else if (pSrc[i * 2 + 1] <= 'F')
        {
            shift_L = 0 - 'A' + 0x0A;
        }
        else
        {
            shift_L = 0 - 'a' + 0x0A;
        }

        pDst[i] = (((pSrc[i * 2 ] + shift_H) << 4) & 0xF0) + ((pSrc[i*2 + 1] + shift_L) & 0xF);
    }
}

static void ble_audio_mobile_connect_test(char *cmdParam, uint32_t cmdParam_len)
{
    POSSIBLY_UNUSED ble_bdaddr_t remote_device =
    {
        .addr = {0x50,0x33,0x33,0x23,0x22,0x11},    // Default address
        .addr_type = GAPM_STATIC_ADDR,
    };

    if (cmdParam != NULL) {
        _str_to_hex(cmdParam, &remote_device.addr[0], 6);
    }

    app_ble_start_connect(&remote_device, APP_GAPM_STATIC_ADDR);
}

static void ble_audio_mobile_disconnect_test(char *cmdParam, uint32_t cmdParam_len)
{
    ble_bdaddr_t remote_device =
    {
        .addr = {0x50,0x33,0x33,0x23,0x22,0x11},    // Default address
        .addr_type = GAPM_STATIC_ADDR,
    };

    //TODO: sscanf will cause smashing detected.
    int value[6];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%d %d %d %d %d %d", &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5]);
        for (uint8_t i = 0; i < 6; i++) {
            remote_device.addr[i] = value[i];
        }
    }

    ble_audio_mobile_req_disconnect(&remote_device);
}

void ble_audio_start_advertising()
{
    aob_conn_start_adv(true, true);
}

static void ble_audio_stop_adv()
{
    if(!aob_conn_stop_adv())
        TRACE(2, "%s:stop adv fail", __func__);
}

static void ble_audio_start_single_mode_advertising()
{
    aob_conn_start_adv(false, true);
}

static void aob_media_disable_test(void)
{
    uint8_t ase_lid = aob_media_get_cur_streaming_ase_lid(1, AOB_MGR_DIRECTION_SINK);
    TRACE(2, "%s %d", __func__, ase_lid);
    aob_media_disable_stream(ase_lid);
}

static void aob_media_read_iso_link_quality_test(void)
{
    uint8_t ase_lid = aob_media_get_cur_streaming_ase_lid(0, AOB_MGR_DIRECTION_SINK);
    TRACE(2, "%s %d", __func__, ase_lid);
    aob_media_read_iso_link_quality(ase_lid);
}

#ifdef AOB_MOBILE_ENABLED
static void ble_audio_test_update_cis_timing(CIS_TIMING_CONFIGURATION_T new_cis_timing)
{
    app_bap_update_cis_timing(&new_cis_timing);
}

static void ble_audio_test_update_sdu_intv(uint32_t sdu_intv_m2s_us, uint32_t sdu_intv_s2m_us)
{
    app_bap_update_sdu_intv(sdu_intv_m2s_us, sdu_intv_s2m_us);
}

static void aob_media_start_test_16k_handler(uint16_t frame_octet)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_16000HZ, frame_octet, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

static void aob_usb_media_start_32k_test(void)
{
    aob_media_start_test_16k_handler(40);
}
#endif

static void aob_media_release_test(void)
{
    uint8_t ase_lid = aob_media_get_cur_streaming_ase_lid(1, AOB_MGR_DIRECTION_SINK);
    TRACE(2, "%s %d", __func__, ase_lid);
    aob_media_release_stream(ase_lid);
}

static void aob_call_sink_disable_handler(void)
{
    aob_media_disable_stream(1);
}

static void aob_call_src_disable_handler(void)
{
    aob_media_disable_stream(2);
}

static void aob_get_streaming_info_test(void)
{
    TRACE(0, "local ble addr:");
    DUMP8("%02x ", ble_global_addr, sizeof(ble_global_addr));

    uint8_t link_count = 0;
    uint8_t con_lid[AOB_COMMON_MOBILE_CONNECTION_MAX] = {0,};
    uint8_t mob1_lid;
    link_count = aob_conn_get_connected_mobile_lid(con_lid);
    if (link_count == 0)
    {
        return;
    }
    mob1_lid = con_lid[0];

    uint8_t ase_lid = aob_media_get_cur_streaming_ase_lid(mob1_lid, AOB_MGR_DIRECTION_SINK);

    AOB_MEDIA_METADATA_CFG_T *p_metadata_cfg = aob_media_get_metadata(ase_lid);
    ble_bdaddr_t *p_addr = aob_conn_get_remote_address(mob1_lid);

    TRACE(2, "%s context %d sample rate %d codec type %d Metadata len %d", __func__, aob_media_get_cur_context_type(ase_lid),
        aob_media_get_sample_rate(ase_lid), aob_media_get_codec_type(ase_lid), p_metadata_cfg ? p_metadata_cfg->metadata_len : 0);

    if (NULL != p_metadata_cfg) {
        TRACE(0, "Metadata:");
        DUMP8("%02x ", p_metadata_cfg->metadata, p_metadata_cfg->metadata_len);
    }
    if (NULL != p_addr) {
        TRACE(0, "Mob address:");
        DUMP8("%02x ", p_addr, sizeof(ble_bdaddr_t));
    }
}

static void aob_set_iso_quality_rsp_test(void)
{
    uint8_t link_count = 0;
    uint8_t con_lid[AOB_COMMON_MOBILE_CONNECTION_MAX] = {0,};
    uint8_t mob1_lid;
    link_count = aob_conn_get_connected_mobile_lid(con_lid);
    if (link_count == 0)
    {
        return;
    }
    mob1_lid = con_lid[0];
    TRACE(2, "%s mob1_lid %d role %d", __func__, mob1_lid, aob_conn_get_cur_role());

    uint8_t ase_lid = aob_media_get_cur_streaming_ase_lid(mob1_lid, AOB_MGR_DIRECTION_SINK);
    aob_media_set_iso_quality_rep_thr(ase_lid,300,0,0,0,0,0,0,0);
}

static void aob_call_sink_release_handler(void)
{
    aob_media_release_stream(1);
}

static void aob_call_src_release_handler(void)
{
    aob_media_release_stream(2);
}

static void aob_read_iso_tx_sync(void)
{
    aob_iap_msg_read_iso_tx_sync_cmd(0);
}

static void aob_pacs_set_location_test(void)
{
    aob_pacs_set_audio_location(AOB_MGR_LOCATION_BACK_LEFT);
}

static void aob_uc_srv_set_presdelay_test(char *param, uint32_t len)
{
    uint32_t presdelay_us = 0;
    if (param == NULL || len <= 0)
    {
        TRACE(3, "%s invalid param %p %d", __func__, param, len);
        return;
    }

    sscanf(param, "%d", &presdelay_us);
    TRACE(3, "%s input presDelay: %d", __func__, presdelay_us);

    gaf_stream_common_set_custom_presdelay_us(presdelay_us);
}

static void aob_bc_scan_set_bcast_id(char *param, uint32_t len)
{
    if (param == NULL)
    {
        TRACE(3, "%s invalid param %d", __func__, len);
        return;
    }

    char input_bcast_id[16*2];
    uint8_t bcast_id[16];
    // if (need_change_format)
    {
        if (len != 8)
        {
            TRACE(3, "%s invalid param %p %d", __func__, param, len);
            return;
        }

        input_bcast_id[0] = param[0];
        input_bcast_id[1] = param[1];

        input_bcast_id[2] = param[3];
        input_bcast_id[3] = param[4];

        input_bcast_id[4] = param[6];
        input_bcast_id[5] = param[7];
    }

    _str_to_hex(input_bcast_id, bcast_id, 16);
    TRACE(1, "%s your input bcast id is:", __func__);
    DUMP8("%02x ", bcast_id, 16);
    aob_bis_scan_set_src_info(bcast_id, NULL);
}

static void aob_bc_scan_set_bcast_code(char *param, uint32_t len)
{
    if (param == NULL || len != APP_GAP_KEY_LEN)
    {
        TRACE(3, "%s invalid param %p %d", __func__, param, len);
        return;
    }

    uint8_t bcast_code[APP_GAP_KEY_LEN];
    _str_to_hex(param, bcast_code, APP_GAP_KEY_LEN);

    aob_bis_sink_set_src_id_key(NULL, (uint8_t *)param);
 }


static void aob_bc_scan_param_start_handler(char *param, uint32_t len)
{
    uint32_t audio_channel_bf = 0;
    if (param == NULL)
    {
        TRACE(3, "%s invalid param %p %d", __func__, param, len);
        return;
    }
    sscanf(param, "%d", &audio_channel_bf);

    aob_bis_sink_set_player_channel(audio_channel_bf);
    app_bap_set_activity_type(GAF_BAP_ACT_TYPE_BIS_AUDIO);
    aob_bis_start_scan();
}

static void aob_bc_scan_start_handler(void)
{
    aob_bis_sink_set_player_channel(0);
    app_bap_set_activity_type(GAF_BAP_ACT_TYPE_BIS_AUDIO);
    aob_bis_scan_set_src_info(broadcast_id, NULL);
    aob_bis_start_scan();
}

static void aob_bc_scan_stop_handler(void)
{
    aob_bis_stop_scan();
}

#ifdef APP_BLE_BIS_SRC_ENABLE
void aob_bis_src_set_encrypt(uint8_t big_idx, uint8_t *bcast_code);
void aob_bis_src_disable_encrypt(uint8_t big_idx);
static void ble_audio_test_config_bis_code(char *cmdParam, uint32_t cmdParam_len)
{
    int value[16];
    uint8_t bis_code[16];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
            &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5],
            &value[6], &value[7], &value[8], &value[9],
            &value[10], &value[11], &value[12], &value[13],
            &value[14], &value[15]);
        for (uint8_t i = 0; i < 16; i++) {
            bis_code[i] = value[i];
        }
        aob_bis_src_set_id_key(0, NULL, bis_code);
    }
}

static void ble_audio_test_config_bis_id(char *cmdParam, uint32_t cmdParam_len)
{
    int isEnabled;
    int value[3];
    uint8_t bis_id[3];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%d %x %x %x",
            &isEnabled,
            &value[0], &value[1],
            &value[2]);
        for (uint8_t i = 0; i < 3; i++) {
            bis_id[i] = value[i];
        }

        if (isEnabled)
        {
            app_bap_bc_src_set_bcast_id(0, bis_id, sizeof(bis_id));
        }
        else
        {
            aob_bis_src_disable_encrypt(0);
        }
    }
}

static void ble_audio_test_enable_bis_encryption(char *cmdParam, uint32_t cmdParam_len)
{
    int isEnabled;

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%d",
            &isEnabled);

        if (isEnabled)
        {
            aob_bis_src_set_id_key(0, NULL, broadcast_code);
        }
        else
        {
            aob_bis_src_disable_encrypt(0);
        }
    }
}

static void aob_bc_src_start_handler(void)
{
    app_bap_bc_src_set_bcast_id(0, broadcast_id, sizeof(broadcast_id));
    app_bap_bc_src_set_encrypt(0, 0, (app_gaf_bc_code_t *)broadcast_code);
    aob_bis_src_start(0, NULL);
}

static void aob_bc_src_stop_handler(void)
{
    aob_bis_src_stop(0);
}

static void aob_bc_src_start_streaming(void)
{
    // enable src, then start audio streaming
    aob_bis_src_enable(0);
}

static void aob_bc_src_stop_streaming(void)
{
    // stop audio straming, then disable src
    aob_bis_src_stop_streaming(0);
}
#endif

#ifdef AOB_MOBILE_ENABLED
static void aob_bc_assist_start_handler(void)
{
    app_sec_set_slave_security_request_enabled_flag(1);
    aob_bis_assist_set_src_id_key(broadcast_id, broadcast_code);
    app_bap_set_activity_type(GAF_BAP_ACT_TYPE_BIS_SHARE);
    aob_bis_assist_start_scan(0);
}

static void aob_bc_assist_stop_handler(void)
{
    aob_bis_assist_stop_scan();
}

static void aob_bc_assist_scan_src_handler(void)
{
    aob_bis_assist_scan_bc_src(0);
}
#endif

static void aob_bc_deleg_start_handler(void)
{
    app_sec_set_slave_security_request_enabled_flag(1);
    app_bap_set_activity_type(GAF_BAP_ACT_TYPE_BIS_SHARE);
    aob_bis_deleg_start_solicite(0, (APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL_BIT | APP_GAF_BAP_CONTEXT_TYPE_MEDIA_BIT));
}

static void aob_bc_deleg_stop_handler(void)
{
    aob_bis_deleg_stop_solicite();
}

static void aob_bc_sink_start_streaming(void)
{
    AOB_BIS_MEDIA_INFO_T media_info;
    media_info.sampling_freq = APP_GAF_BAP_SAMPLING_FREQ_48000HZ;
    media_info.frame_octet = 120;
    aob_bis_sink_start_streaming(0, 1, BES_BLE_GAF_CODEC_TYPE_LC3, &media_info);
}

static void aob_bc_sink_stop_streaming(void)
{
    aob_bis_sink_stop_streaming(0, 1);
}

static void aob_bc_sink_scan_pa_sync(void)
{
    aob_bis_scan_pa_sync(ble_audio_get_bis_src_ble_addr(), 0, 1);
}

static void aob_bc_sink_disable(void)
{
    aob_bis_sink_disable(0);
}

static void aob_bc_sink_scan_pa_terminate(void)
{
    aob_bis_sink_scan_pa_terminate(1);
}

static void aob_bc_sink_start(void)
{
    aob_bis_sink_start_param_t param = {0};

    param.ch_bf = 0;
    param.bc_id = broadcast_id;
    param.bc_code = broadcast_code;
    aob_bis_sink_start(&param);
}

static void aob_bc_sink_stop(void)
{
    aob_bis_sink_stop();
}

static void aob_media_mics_set_mute_test(void)
{
    aob_media_mics_set_mute(1);
}

static void aob_media_mics_set_unmute_test(void)
{
    aob_media_mics_set_mute(0);
}

/// call related test APIs
static void _earbud_accept_call(void)
{
    TRACE(1, "%s", __func__);

    uint8_t conidx = INVALID_CONIDX;
    uint8_t call_id = INVALID_CALL_ID;
    AOB_MOBILE_INFO_T *info = NULL;
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);
        if (!info)
        {
            continue;
        }
        if (info->connected)
        {
            for (uint8_t j = 0; j < AOB_COMMON_CALL_MAX_NB_IDS; j++)
            {
                if (AOB_CALL_STATE_INCOMING == info->call_env_info.single_call_info[j].state)
                {
                    conidx = info->conidx;
                    call_id = info->call_env_info.single_call_info[j].call_id;
                    break;
                }
            }
        }
    }

    if (INVALID_CALL_ID != call_id)
    {
        aob_call_if_accept_call(conidx, call_id);
    }
    else
    {
        LOG_W("No active call to accept!");
    }
}

static void earbud_terminate_call(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);

    uint8_t call_id = INVALID_CALL_ID;
    AOB_MOBILE_INFO_T *info = NULL;
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);
        if (!info)
        {
            continue;
        }

        call_id = info->call_env_info.single_call_info[callIndex].call_id;
        if ((0 != call_id) && (AOB_CALL_STATE_IDLE != info->call_env_info.single_call_info[callIndex].state))
        {
            aob_call_if_terminate_call(i, call_id);
        }
        else
        {
            LOG_W("No call to terminate!");
        }
    }
    if (INVALID_CALL_ID == call_id)
    {
        LOG_W("No connected device!");
    }
}

static void _earbud_terminate_call1(void)
{
    earbud_terminate_call(0);
}

static void _earbud_terminate_call2(void)
{
    earbud_terminate_call(1);
}

static void earbud_hold_call(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);

    AOB_MOBILE_INFO_T *info = NULL;
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);
        if (!info)
        {
            continue;
        }

        uint8_t call_id = info->call_env_info.single_call_info[callIndex].call_id;
        AOB_CALL_STATE_E state = info->call_env_info.single_call_info[callIndex].state;

        if (AOB_CALL_STATE_ACTIVE == state
              || (AOB_CALL_STATE_INCOMING == state)
              || (AOB_CALL_STATE_REMOTE_HELD == state))
        {
            aob_call_if_hold_call(i, call_id);
        }
        else
        {
            LOG_W("No call to terminate!");
        }
    }
}

static void _earbud_hold_call1(void)
{
    earbud_hold_call(0);
}

static void _earbud_hold_call2(void)
{
    earbud_hold_call(1);
}

static void earbud_retrieve_call(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);

    AOB_MOBILE_INFO_T *info = NULL;
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);
        if (!info)
        {
            continue;
        }

        uint8_t call_id = info->call_env_info.single_call_info[callIndex].call_id;
        AOB_CALL_STATE_E state = info->call_env_info.single_call_info[callIndex].state;

        if (AOB_CALL_STATE_LOCAL_AND_REMOTE_HELD == state
              || (AOB_CALL_STATE_LOCAL_HELD == state))
        {
            aob_call_if_retrieve_call(i, call_id);
        }
        else
        {
            LOG_W("No call to retrieve!");
        }
     }
}

static void _earbud_retrieve_call1(void)
{
    earbud_retrieve_call(0);
}

static void _earbud_retrieve_call2(void)
{
    earbud_retrieve_call(1);
}

static void _earbud_dial(void)
{
    TRACE(1, "%s", __func__);

    AOB_MOBILE_INFO_T *info = NULL;
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);
        if (info)
        {
            aob_call_if_outgoing_dial(i, (uint8_t *)EARBUDS_DFT_CALL_OUTGOING_URI,
                strlen(EARBUDS_DFT_CALL_OUTGOING_URI));
            break;
        }
    }
}

static void _earbud_call_state_get(void)
{
    AOB_MOBILE_INFO_T *info = NULL;

    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);

        if (!info) {
            continue;
        }

        for (uint8_t j = 0; j < AOB_COMMON_CALL_MAX_NB_IDS; j++)
        {
            uint8_t state = AOB_CALL_STATE_IDLE;
            uint8_t call_id = 0;
            uint8_t bearer_lid = 0xFF;

            state = info->call_env_info.single_call_info[i].state;
            call_id = info->call_env_info.single_call_info[i].call_id;
            bearer_lid = info->call_env_info.single_call_info[i].bearer_lid;

            LOG_I("[D%d]%s, bearer_lid:%d, call_id:%d, call_state:%d", i, __func__,
                     bearer_lid, call_id, state);
        }
    }

}

// csip for earbud
static void _earbud_csism_update_rsi_data(void)
{
    TRACE(1, "%s", __func__);
    aob_csip_if_update_rsi_data();
}

static void _earbud_csism_get_rsi_data(void)
{
    TRACE(1, "%s", __func__);
    uint8_t rsi[6]={0};
    aob_csip_if_get_rsi_data(rsi);
}

static void _earbud_csism_update_sirk(void)
{
    TRACE(1, "%s", __func__);
    uint8_t sirk[16+1] = "ABCDABCDABCDABCD";
    aob_csip_if_update_sirk(sirk, 16);
}


static void _earbud_iap_test_start_test(void)
{
    TRACE(1,"%s", __func__);

    aob_iap_msg_test_mode_start(0, 0, 0);
}

static void _earbud_iap_test_get_cnt_test(void)
{
    TRACE(1,"%s", __func__);

    aob_iap_msg_tm_cnt_get(0);
}


static void _earbud_iap_test_end_test(void)
{
    TRACE(1,"%s", __func__);

    aob_iap_msg_test_mode_stop(0);
}

static void _earbud_iap_test_mode_setup(void)
{
    TRACE(1,"%s", __func__);

    app_bap_uc_srv_set_test_mode(1);
}

static void _earbud_hap_has_cfg_preset(void)
{
    TRACE(0,"%s", __func__);

    char a[6] = {0};
    aob_hap_has_msg_cfg_preset(0, 0, 0, 6, a);
}

static void _earbud_hap_has_set_active_preset(void)
{
    TRACE(0,"%s", __func__);

    aob_hap_has_msg_set_active_preset(0);
}

static void _earbud_hap_has_set_coor_sup(void)
{
    TRACE(0,"%s", __func__);

    aob_hap_has_msg_set_coordination_sup(1);
}

static void _tx_power_report_enable(void)
{
    TRACE(0,"%s", __func__);
    aob_conn_tx_power_report_enable(0, true, true);
}

static void _tx_power_report_disenable(void)
{
    TRACE(0,"%s", __func__);
    aob_conn_tx_power_report_enable(0, false, false);
}

static void _tx_power_get_local(void)
{
    TRACE(0,"%s", __func__);
    aob_conn_get_tx_power(0, BLE_TX_LOCAL, BLE_TX_PWR_LEVEL_1M);
}

static void _tx_power_get_remote(void)
{
    TRACE(0,"%s", __func__);
    aob_conn_get_tx_power(0, BLE_TX_REMOTE, BLE_TX_PWR_LEVEL_1M);
}

static void _set_path_loss_test(void)
{
    TRACE(0,"%s", __func__);

    aob_conn_set_path_loss_param(0, 1, 1, 1, 0, 0, 1);
}

#ifdef CFG_SUBRATE
static void _set_default_subrate_test(void)
{
    TRACE(0,"%s", __func__);

    aob_conn_set_default_subrate(1, 1, 0, 0, 0x0C80);
}

static void _subrate_request_test(void)
{
    TRACE(0,"%s", __func__);

    aob_conn_subrate_request(0, 2, 2, 0, 0, 0x0C80);
}
#endif // CFG_SUBRATE
static void aob_media_play_handler(void)
{
    TRACE(0, "%s", __func__);
    uint8_t con_lid1 = app_audio_adm_get_le_audio_active_device();
    if (con_lid1 != BT_DEVICE_INVALID_ID)
    {
        aob_media_play(con_lid1);
    }
}

static void aob_media_pause_handler(void)
{
    TRACE(0, "%s", __func__);
    uint8_t con_lid1 = app_audio_adm_get_le_audio_active_device();
    if (con_lid1 != BT_DEVICE_INVALID_ID)
    {
        TRACE(2, "%s,device lid = %d", __func__,con_lid1);
        aob_media_pause(con_lid1);
    }
}

static void aob_media_prev_handler(void)
{
    TRACE(0, "%s", __func__);
    uint8_t con_lid1 = app_audio_adm_get_le_audio_active_device();
    if (con_lid1 != BT_DEVICE_INVALID_ID)
    {
        TRACE(2, "%s,device lid = %d", __func__,con_lid1);
        aob_media_prev(con_lid1);
    }
}

static void aob_media_next_handler(void)
{
    TRACE(0, "%s", __func__);
    uint8_t con_lid1 = app_audio_adm_get_le_audio_active_device();
    if (con_lid1 != BT_DEVICE_INVALID_ID)
    {
        TRACE(2, "%s,device lid = %d", __func__,con_lid1);
        aob_media_next(con_lid1);
    }
}

static void aob_media_stop_handler(void)
{
    TRACE(0, "%s", __func__);
    uint8_t con_lid1 = app_audio_adm_get_le_audio_active_device();
    if (con_lid1 != BT_DEVICE_INVALID_ID)
    {
        aob_media_stop(con_lid1);
    }
}

static void aob_tmap_start_handler(void)
{
    app_tmap_start(0);
}

static void dump_ble_core_status_test(void)
{
    ble_audio_dump_core_status();
}

static void dump_ble_audio_sirk_test()
{
    NV_RECORD_BLE_AUDIO_DEV_INFO_T* p_bleaudio_info = nv_record_bleaudio_get_ptr();
    if (!p_bleaudio_info)
    {
        return ;
    }
    LOG_I("%s:set mem = %d", __func__,p_bleaudio_info->set_member);
    DUMP8("%02x ", p_bleaudio_info->sirk, 16);
}

static void aob_set_vol_offset(void)
{
    aob_vol_set_volume_offset(0,-30);
}

static void aob_set_abs_vol(void)
{
    aob_vol_set_abs(6);
}

static void ble_check_connected_test(void)
{
    AOB_MOBILE_INFO_T *info = NULL;
    bool connect = false;
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        info = ble_audio_earphone_info_get_mobile_info(i);

        ASSERT(info != NULL, "%s", __FUNCTION__);

        if (true == info->connected)
        {
            LOG_I("BLE Audio connected!");
            connect = true;
            break;
        }
    }
    if(!connect)
    {
        LOG_W("Please connect ble acl link firstly!!");
    }
}

static void aob_get_media_state_test(void)
{
    uint8_t con_lid[AOB_COMMON_MOBILE_CONNECTION_MAX];
    uint8_t connected_num = 0;

    connected_num = ble_audio_get_mobile_connected_dev_lids(con_lid);
    for (uint8_t i = 0; i < connected_num; i++)
    {
        ble_bdaddr_t *p_lid1_addr = NULL;
        AOB_MGR_PLAYBACK_STATE_E media_state = aob_media_get_state(con_lid[i]);
        AOB_MGR_STREAM_STATE_E ase_state = aob_media_get_cur_ase_state(con_lid[i]);
        AOB_MGR_CONTEXT_TYPE_BF_E ase_context = aob_media_get_cur_context_type(con_lid[i]);
        p_lid1_addr = ble_audio_get_mobile_address_by_lid(con_lid[i]);
        if(NULL != p_lid1_addr)
        {
            LOG_I("[BLE Audio]device: %d media: %d, ase: %d, context: %d, addr: %02x:%02x:%02x:%02x:%02x:%02x",
                    con_lid[i], media_state, ase_state, ase_context,
                    p_lid1_addr->addr[0], p_lid1_addr->addr[1], p_lid1_addr->addr[2],
                    p_lid1_addr->addr[3], p_lid1_addr->addr[4], p_lid1_addr->addr[5]);
        }
    }
}


static void aob_get_call_state_test(void)
{
    for (uint8_t i = 0; i < AOB_COMMON_MOBILE_CONNECTION_MAX; i++)
    {
        AOB_MOBILE_INFO_T *info = NULL;
        uint8_t conidx = INVALID_CONIDX;
        info = ble_audio_earphone_info_get_mobile_info(i);
        if (NULL != info && true == info->connected)
        {
            conidx = info->conidx;
            ble_bdaddr_t *p_lid_addr = NULL;
            p_lid_addr = ble_audio_get_mobile_address_by_lid(conidx);
            if (INVALID_CONIDX != conidx)
            {
                LOG_I("[BLE Audio]: device: %d, call: %d, addr: %02x:%02x:%02x:%02x:%02x:%02x",
                    conidx, info->call_env_info.single_call_info[0].state,
                    p_lid_addr->addr[0], p_lid_addr->addr[1], p_lid_addr->addr[2],
                    p_lid_addr->addr[3], p_lid_addr->addr[4], p_lid_addr->addr[5]);
            }
        }
    }
}

static void ble_audio_set_no_available_context()
{
    aob_pacs_set_ava_audio_context(0, 0, 0);
}

static void ble_audio_restore_available_context()
{
    uint16_t ava_context_bf_sink = (uint16_t)(AOB_AUDIO_CONTEXT_TYPE_MEDIA | AOB_AUDIO_CONTEXT_TYPE_CONVERSATIONAL);
    uint16_t ava_context_bf_src = (uint16_t)(AOB_AUDIO_CONTEXT_TYPE_UNSPECIFIED | AOB_AUDIO_CONTEXT_TYPE_CONVERSATIONAL);
    aob_pacs_set_ava_audio_context(0, ava_context_bf_sink, ava_context_bf_src);
}

#define TEST_DTS_SPSM_0 0x0028
#define TEST_DTS_SPSM_1 0x0029

static void aob_dts_send_data_28(void)
{
    uint8_t a[500];
    memset(a, 0x28, 500);
    aob_dts_send_data(0, TEST_DTS_SPSM_0, 500, (const uint8_t *)a);
}

static void aob_dts_send_data_29(void)
{
    static const uint8_t a[20] = {0x29, 0x10, 0x33, 0x55, 0x00, 0x35, 0x03, 0x61, 0x00, 0x29, 0x29, 0x10, 0x33, 0x55, 0x00, 0x35, 0x03, 0x61, 0x00, 0x29};
    aob_dts_send_data(0, TEST_DTS_SPSM_1, 20, a);
}

static void aob_dts_configure_28(void)
{
    aob_dts_register_spsm(TEST_DTS_SPSM_0, 6);
}

static void aob_dts_configure_29(void)
{
    aob_dts_register_spsm(TEST_DTS_SPSM_1, 6);
}

static void aob_dts_disconnect_28(void)
{
    aob_dts_disconnect(0, TEST_DTS_SPSM_0);
}

static void aob_dts_disconnect_29(void)
{
    aob_dts_disconnect(0, TEST_DTS_SPSM_1);
}

#ifdef AOB_MOBILE_ENABLED

static void aob_dtc_connect_28(void)
{
    aob_dtc_connect(0, 512, TEST_DTS_SPSM_0);
}

static void aob_dtc_connect_29(void)
{
    aob_dtc_connect(0, 512, TEST_DTS_SPSM_1);
}

static void aob_dtc_send_data_28(void)
{
    uint8_t a[500];
    memset(a, 0x28, 500);
    aob_dtc_send_data(0, TEST_DTS_SPSM_0, 500, a);
}

static void aob_dtc_send_data_29(void)
{
    uint8_t a[500];
    memset(a, 0x29, 500);
    aob_dtc_send_data(0, TEST_DTS_SPSM_1, 500, a);
}

static void aob_dtc_disconnect_28(void)
{
    aob_dtc_disconnect(0, TEST_DTS_SPSM_0);
}

static void aob_dtc_disconnect_29(void)
{
    aob_dtc_disconnect(0, TEST_DTS_SPSM_1);
}

#endif


#ifdef AOB_MOBILE_ENABLED
extern "C" void aob_mobile_scan_start_handler(void)
{
    if (ble_audio_is_ux_mobile())
    {
        bes_ble_scan_param_t scan_param = {0};

        scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
        scan_param.scanWindowMs   = 20;
        scan_param.scanIntervalMs   = 50;

        bes_ble_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);
        bes_ble_gap_start_scan(&scan_param);
    }
}

static void ble_audio_mobile_connect_any_devices(char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t dev_num = cmdParam_len / (2*sizeof(ble_bdaddr_t));

    dev_num = (dev_num > BLE_CONNECTION_MAX) ?  BLE_CONNECTION_MAX : dev_num;

    if (dev_num == 0)
    {
        TRACE(1, "err dev num %d, use param: [addr1type1|...addrNtypeN] format please",
              dev_num);
        TRACE(1, "ex. [11222333336001|11222333336100]");
        return;
    }

    ble_bdaddr_t remote_device[BLE_CONNECTION_MAX] =
    {
        {
            .addr = {0x50,0x33,0x33,0x23,0x22,0x11},    // Default address
            .addr_type = GAPM_STATIC_ADDR,
        },
        {
            .addr = {0x51,0x33,0x33,0x23,0x22,0x11},    // Default address
            .addr_type = GAPM_STATIC_ADDR,
        }
    };

    if (cmdParam != NULL)
    {
        for (uint8_t i = 0; i < dev_num; i++)
        {
            /// Split with '|', so add 1
            _str_to_hex(cmdParam + i * (2*sizeof(ble_bdaddr_t) + 1), (uint8_t *)&remote_device[i], sizeof(ble_bdaddr_t));
        }
    }

    app_bap_set_device_num_to_be_connected(dev_num);
    /// preset cis num in cig
    app_bap_uc_cli_set_cis_num_in_cig(dev_num);
    app_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);
    /// timeout time 500*10ms
    app_ble_start_connect_with_white_list(&remote_device[0], dev_num, APP_GAPM_STATIC_ADDR, 500);
}

static void ble_audio_mobile_set_cis_num_in_cig(char *cmdParam, uint32_t cmdParam_len)
{
    uint32_t cis_num_in_cig = 0;

    sscanf(cmdParam, "%d", &cis_num_in_cig);
    app_bap_uc_cli_set_cis_num_in_cig(cis_num_in_cig);
}

const static uint8_t earphone_addr[6]  = {0x76, 0x78, 0x33, 0x23, 0x22, 0x11};
const static uint8_t earphone_addr1[6]  = {0x75, 0x78, 0x33, 0x23, 0x22, 0x11};

static void aob_mobile_start_reconnect(void)
{
    LOG_I("%s", __func__);
    app_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);
    ble_bdaddr_t ble_connect_addr;
    ble_connect_addr.addr_type = GAPM_STATIC_ADDR;
    memcpy(ble_connect_addr.addr, (uint8_t *)earphone_addr, BTIF_BD_ADDR_SIZE);
    ble_audio_mobile_start_connecting(&ble_connect_addr);
}

/// company with aob_media_start_test
static void aob_media_rejoin_media_test(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
#ifdef AOB_LOW_LATENCY_MODE
    ble_audio_test_update_sdu_intv(5000, 5000);
#else
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
#endif
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        /// if can not get streaming ase, means conn is a rejoin dev conn
        if (AOB_INVALID_LID == aob_media_mobile_get_cur_streaming_ase_lid(i, AOB_MGR_DIRECTION_SINK))
        {
            aob_media_mobile_start_stream(&ase_to_start, i, false);
        }
    }
}

static void aob_media_start_test_handler(uint16_t frame_octet)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
#ifdef AOB_LOW_LATENCY_MODE
    ble_audio_test_update_sdu_intv(5000, 5000);
#else
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
#endif
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, frame_octet, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

#ifdef LC3PLUS_SUPPORT
static void aob_media_start_test_lc3plus_media(CIS_TIMING_CONFIGURATION_T cis_timimg, uint32_t sdu_intval_us,
                                               uint16_t freq, uint16_t octs)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(cis_timimg);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(sdu_intval_us, sdu_intval_us);
    // 48K, LC3Plus, Media
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        freq, octs, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3PLUS, APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

static void aob_media_start_test_lc3plus_5ms_handler(void)
{
    CIS_TIMING_CONFIGURATION_T lc3plus_cis_timing_5ms =
    {
        .m2s_bn = 1, .m2s_nse = 2, .m2s_ft = 4,
        .s2m_bn = 1, .s2m_nse = 2, .s2m_ft = 4,
        .frame_cnt_per_sdu = 1,
        .iso_interval = 4,
    };

    aob_media_start_test_lc3plus_media(lc3plus_cis_timing_5ms, 5000,
                        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 80);
}

static void aob_media_start_test_lc3plus_handler(void)
{
    CIS_TIMING_CONFIGURATION_T lc3plus_cis_timing_10ms =
    {
        .m2s_bn = 2, .m2s_nse = 2, .m2s_ft = 4,
        .s2m_bn = 2, .s2m_nse = 2, .s2m_ft = 4,
        .frame_cnt_per_sdu = 1,
        .iso_interval = 8,
    };

    aob_media_start_test_lc3plus_media(lc3plus_cis_timing_10ms, 10000,
                        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 160);
}
#endif

static void aob_media_start_test(void)
{
    aob_media_start_test_handler(120);
}

#if defined (AOB_GMAP_ENABLED)
static void aob_gmap_start_test(void)
{
    // Use gmap cis timing @see gmap_cis_timing
    ble_audio_test_update_cis_timing(gmap_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 100, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_GAME},
        {APP_GAF_BAP_SAMPLING_FREQ_32000HZ,  80,  APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_GAME},
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start[0], i, true);
    }
}
#endif

static void aob_media_7_5ms_start_test(void)
{
    CIS_TIMING_CONFIGURATION_T cis_timing_7_5ms =
    {
        .m2s_bn = 1, .m2s_nse = 2, .m2s_ft = 4,
        .s2m_bn = 1, .s2m_nse = 2, .s2m_ft = 4,
        .frame_cnt_per_sdu = 1,
        .iso_interval = 6,//7.5ms iso
    };
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(cis_timing_7_5ms);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(7500, 7500);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 90, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)

#if defined (AOB_GMAP_ENABLED)
static void aob_stereo_gmap_start_test(void)
{
    // Use gmap cis timing @see gmap_cis_timing
    ble_audio_test_update_cis_timing(gmap_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 100, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_GAME},
        {APP_GAF_BAP_SAMPLING_FREQ_32000HZ,  80,  APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_GAME},
    };

    aob_media_mobile_start_stream(ase_to_start, 0, true);
}
#endif

static void aob_stereo_media_start_test(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(stereo_cis_timing);
#ifdef BLE_AUDIO_FRAME_DUR_7_5MS
    // Use 7.5ms sdu interval
    ble_audio_test_update_sdu_intv(7500, 7500);
#else
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
#endif

    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

static void aob_stereo_call_start_test(void)
{
    // Use dft cis timing @see dft_cis_stereo_cis_timingtiming
    ble_audio_test_update_cis_timing(dft_cis_timing);
#ifdef BLE_AUDIO_FRAME_DUR_7_5MS
    // Use 7.5ms sdu interval
    ble_audio_test_update_sdu_intv(7500, 7500);
#else
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
#endif
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_32000HZ, 80, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
        {APP_GAF_BAP_SAMPLING_FREQ_32000HZ, 80, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
    };

    aob_media_mobile_start_stream(ase_to_start, 0, true);
}

static void aob_stereo_ai_start_test(void)
{
    // Use dft cis timing @see stereo_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
#ifdef BLE_AUDIO_FRAME_DUR_7_5MS
    // Use 7.5ms sdu interval
    ble_audio_test_update_sdu_intv(7500, 7500);
#else
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
#endif
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_32000HZ, 80, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_MAN_MACHINE
    };
    // TODO: con_lid
    aob_media_mobile_start_stream(&ase_to_start, 0, false);
}
#endif

static void aob_media_start_32k_test(void)
{
    aob_media_start_test_handler(40);
}

static void aob_media_ascc_release_stream_test(void)
{
    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        aob_media_mobile_release_all_stream_for_specifc_direction(
                                            i, APP_GAF_DIRECTION_SINK);
        aob_media_mobile_release_all_stream_for_specifc_direction(
                                            i, APP_GAF_DIRECTION_SRC);
    }
}

static void aob_ull_ascc_release_stream_test(void)
{
    uint8_t ase_lid = AOB_INVALID_LID;
    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        ase_lid = app_bap_uc_cli_get_ase_lid(i, APP_GAF_BAP_UC_ASE_STATE_STREAMING, APP_GAF_DIRECTION_SRC);
        aob_media_mobile_release_stream(ase_lid);
    }
}

static void aob_media_ascc_enable_stream_test(void)
{
    uint8_t nb_ase = 0;
    uint8_t ase_lid_list[APP_BAP_DFT_ASCC_NB_ASE_CFG] = {0};

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        nb_ase = app_bap_uc_cli_get_specific_state_ase_lid_list(i, APP_GAF_DIRECTION_SRC,
                                                                APP_GAF_BAP_UC_ASE_STATE_QOS_CONFIGURED,
                                                                ase_lid_list);
        for (uint32_t ase_idx = 0; ase_idx < nb_ase; ase_idx++)
        {
            aob_media_mobile_enable_stream(ase_lid_list[ase_idx]);
        }
    }
}

static void aob_media_ascc_disable_stream_test(void)
{
    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        aob_media_mobile_disable_all_stream_for_specifc_direction(
                                            i, APP_GAF_DIRECTION_SINK);
        aob_media_mobile_disable_all_stream_for_specifc_direction(
                                            i, APP_GAF_DIRECTION_SRC);
    }
}

static void aob_resume_audio_test()
{
    ble_ase_stm_t *p_ase_sm = ble_audio_find_ase_sm_by_con_lid(0);
    if (p_ase_sm != NULL)
    {
        ble_ase_sm_start_auto_play(p_ase_sm);
    }
}

static void aob_ai_start_dual_channel_upstream_handler(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    static AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_MAN_MACHINE
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}
#ifdef HID_ULL_ENABLE
 static void ble_audio_ull_capture_start_handler_test(void)
{
    // Use dft cis timing @see dft_cis_timing
    static CIS_TIMING_CONFIGURATION_T dft_cis_timing_test =
    {
        .m2s_bn = 2,
        .m2s_nse = 2,
        .m2s_ft = 1,
        .s2m_bn = 2,
        .s2m_nse = 2,
        .s2m_ft = 1,
        .frame_cnt_per_sdu = 1,
        .iso_interval = 4,  // 5ms = 4 * 1.25ms
    };
    ble_audio_test_update_cis_timing(dft_cis_timing_test);
    ble_audio_test_update_sdu_intv(2500, 2500);

    LOG_I("%s", __func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_BAP_SAMPLING_FREQ_48000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_ULL,  BES_BLE_GAF_CONTEXT_TYPE_MAN_MACHINE_BIT},
    };
    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, false);
    }
}
#endif

static void aob_live_start_dual_channel_upstream_handler(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    static AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 120, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_LIVE
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

static void aob_usb_ai_start_dual_channel_upstream_handler(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_MAN_MACHINE
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

static void aob_call_start_stream_handler(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_16000HZ, 40, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
        {APP_GAF_BAP_SAMPLING_FREQ_16000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, true);
    }
}

static void aob_usb_call_start_stream_handler(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 40, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
        {APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, true);
    }
}

static void aob_game_start_conversational_stream_handler(void)
{
    CIS_TIMING_CONFIGURATION_T cis_timing_gmap =
    {
        .m2s_bn = 1, .m2s_nse = 3, .m2s_ft = 1,
        .s2m_bn = 1, .s2m_nse = 3, .s2m_ft = 1,
        .frame_cnt_per_sdu = 1,
        .iso_interval = 8,
    };
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(cis_timing_gmap);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_16000HZ, 40, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_GAME},
        {APP_GAF_BAP_SAMPLING_FREQ_16000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_GAME},
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, true);
    }
}

static void aob_game_start_stream_handler(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_GAF_BAP_SAMPLING_FREQ_16000HZ, 40, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_GAME},
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, false);
    }
}

POSSIBLY_UNUSED static void aob_pacc_set_location_test(void)
{
    aob_pacs_mobile_set_location(0, (uint32_t)AOB_MGR_LOCATION_LEFT_SURROUND);
    aob_pacs_mobile_set_location(1, (uint32_t)AOB_MGR_LOCATION_RIGHT_SURROUND);
}

static void aob_media_micc_set_mute(void)
{
    aob_media_mobile_micc_set_mute(0, 1);
    aob_media_mobile_micc_set_mute(1, 1);
}

static void aob_media_micc_set_unmute(void)
{
    aob_media_mobile_micc_set_mute(0, 0);
    aob_media_mobile_micc_set_mute(1, 0);
}

static void aob_arc_start_handler(void)
{
    app_arc_micc_start(0);
    app_arc_micc_start(1);
}

static void _mobile_on_call_incoming(void)
{
    TRACE(1, "%s", __func__);
    uint8_t len1 = strlen(APP_ACC_DFT_CALL_INCOMING_URI);
    uint8_t len2 = strlen(APP_ACC_DFT_CALL_INCOMING_TGT_URI);
    uint8_t len3 = strlen(APP_ACC_DFT_CALL_INCOMING_FRIENDLY_NAME);

    uint8_t ble_conidx = ble_audio_get_active_conidx();
    AOB_MOBILE_PHONE_INFO_T *info = ble_audio_mobile_info_get(ble_conidx);
    if (!info)
    {
        LOG_E("NULL pointer in %s", __func__);
        return;
    }

    if (INVALID_CONIDX != ble_conidx)
    {
        aob_call_mobile_if_incoming_call(ble_conidx, APP_ACC_DFT_CALL_INCOMING_URI, len1,
            APP_ACC_DFT_CALL_INCOMING_TGT_URI, len2, APP_ACC_DFT_CALL_INCOMING_FRIENDLY_NAME, len3);
    }
    else
    {
        LOG_W("No connected device!");
    }
}

static void _mobile_on_call_outgoing(void)
{
    TRACE(1, "%s", __func__);
    char str_buf[64] = {0};
    uint8_t len1 = strlen(APP_ACC_DFT_CALL_OUTGOING_URI);
    uint8_t len2 = strlen(APP_ACC_DFT_CALL_OUTGOING_FRIENDLY_NAME);
    memcpy(str_buf, APP_ACC_DFT_CALL_OUTGOING_URI, len1);
    memcpy(str_buf + len1, APP_ACC_DFT_CALL_OUTGOING_FRIENDLY_NAME, len2);

    uint8_t ble_conidx = ble_audio_get_active_conidx();
    if (INVALID_CONIDX != ble_conidx)
    {
        aob_call_mobile_if_outgoing_dial(ble_conidx, (uint8_t *)str_buf, len1, len2);
    }
    else
    {
        LOG_W("No connected device!");
    }
}

static void _mobile_on_call_accepted(void)
{
    TRACE(1, "%s", __func__);

    uint8_t conidx = ble_audio_get_active_conidx();
    uint8_t call_id = INVALID_CALL_ID;
    if (INVALID_CONIDX != conidx)
    {
        AOB_SINGLE_CALL_INFO_T *pFoundInfo = NULL;
        pFoundInfo = ble_audio_mobile_get_call_info_by_state(conidx, AOB_CALL_STATE_INCOMING);
        if (pFoundInfo) {
            call_id = pFoundInfo->call_id;
        }
    }
    else
    {
        LOG_W("No connected device!");
    }

    if (INVALID_CALL_ID != call_id)
    {
        aob_call_mobile_if_accept_call(conidx, call_id);
    }
    else
    {
        LOG_W("No call to accept!");
    }
}

static void mobile_on_call_terminate(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);

    uint8_t conidx = ble_audio_get_active_conidx();

    if (INVALID_CONIDX != conidx)
    {
        aob_call_mobile_if_terminate_call(0, callIndex);
    }
    else
    {
        LOG_W("No connected device!");
    }
}

static void _mobile_on_call_terminate_call1(void)
{
    mobile_on_call_terminate(0);
}

static void _mobile_on_call_terminate_call2(void)
{
    mobile_on_call_terminate(1);
}

static void mobile_on_call_hold_local(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);

    uint8_t conidx = ble_audio_get_active_conidx();

    if (INVALID_CONIDX != conidx)
    {
        aob_call_mobile_if_hold_local_call(0, callIndex);
    }
    else
    {
        LOG_W("No connected device!");
    }
}
static void _mobile_on_call_hold_local_call1(void)
{
    mobile_on_call_hold_local(0);
}

static void _mobile_on_call_hold_local_call2(void)
{
    mobile_on_call_hold_local(1);
}

static void mobile_on_call_hold_remote(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);
    uint8_t conidx = ble_audio_get_active_conidx();

    if (INVALID_CONIDX != conidx)
    {
        aob_call_mobile_if_hold_remote_call(0, callIndex);
    }
    else
    {
        LOG_W("No connected device!");
    }

}

static void _mobile_on_call_hold_remote_call1(void)
{
    mobile_on_call_hold_remote(0);
}

static void _mobile_on_call_hold_remote_call2(void)
{
    mobile_on_call_hold_remote(1);
}

static void mobile_on_call_retrieve_local(uint8_t callIndex)
{
    TRACE(1, "%s, callIndex:%d", __func__, callIndex);

    uint8_t conidx = ble_audio_get_active_conidx();

    if (INVALID_CONIDX != conidx)
    {
        aob_call_mobile_if_retrieve_local_call(0, callIndex);
    }
    else
    {
        LOG_W("No connected device!");
    }
}

static void _mobile_on_call_retrieve_local_call1(void)
{
    mobile_on_call_retrieve_local(0);
}

static void _mobile_on_call_retrieve_local_call2(void)
{
    mobile_on_call_retrieve_local(1);
}

static void _mobile_on_call_remote_answer(void)
{
    TRACE(1, "%s", __func__);

    uint8_t conidx = ble_audio_get_active_conidx();
    uint8_t call_id = 0;
    if (INVALID_CONIDX != conidx)
    {
        AOB_SINGLE_CALL_INFO_T *pFoundInfo = NULL;
        pFoundInfo = ble_audio_mobile_get_call_info_by_state(conidx, AOB_CALL_STATE_ALERTING);
        if (pFoundInfo) {
            call_id = pFoundInfo->call_id;
        }
        else
        {
            pFoundInfo = ble_audio_mobile_get_call_info_by_state(conidx, AOB_CALL_STATE_INCOMING);
            if (pFoundInfo) {
                call_id = pFoundInfo->call_id;
            }
        }
    }
    else
    {
        LOG_W("No connected device!");
    }

    if (0 != call_id)
    {
        aob_call_mobile_if_remote_answer_call(conidx, call_id);
    }
    else
    {
        LOG_W("No call to answer!");
    }
}

static void _mobile_get_call_info(void)
{
    TRACE(0, "%s todo", __func__);
}

static void _mobile_on_call_remote_alert(void)
{
    TRACE(1, "%s", __func__);

    uint8_t conidx = ble_audio_get_active_conidx();

    if (INVALID_CONIDX != conidx)
    {
        // TODO:
        aob_call_mobile_if_remote_alert_start(conidx, 0);
    }
    else
    {
        LOG_W("No connected device!");
    }
}

static void _mobile_iap_test_start_test(void)
{
    TRACE(1,"%s", __func__);

    aob_iap_msg_test_mode_start(0, 1, 0);
}

static void _mobile_iap_test_mode_setup(void)
{
    TRACE(1,"%s", __func__);

    app_bap_uc_cli_set_test_mode(1);
}

static void _mobile_csisc_read_size(void)
{
    TRACE(1, "%s", __func__);
    uint8_t group_dev_nbs = 0;
    if (aob_csip_mobile_if_get_group_device_numbers(0, &group_dev_nbs))
    {
        TRACE(1, "group_dev_nbs:%d", group_dev_nbs);
    }
}

static void _mobile_csisc_read_dev_rank_index(void)
{
    TRACE(1, "%s", __func__);
    uint8_t rank_index = 0;
    if (aob_csip_mobile_if_get_device_rank_index(0, &rank_index))
    {
        TRACE(1, "rank_index:%d", rank_index);
    }
}

static void _mobile_csisc_read_dev_lock_status(void)
{
    TRACE(1, "%s", __func__);
    uint8_t lock_status = 0;
    if (aob_csip_mobile_if_get_device_lock_status(0, &lock_status))
    {
        TRACE(1, "lock_status:%d", lock_status);
    }
}

static void _mobile_csisc_read_sirk(void)
{
    TRACE(1, "%s", __func__);
    uint8_t sirk[16] = {0};
    if (aob_csip_mobile_if_get_sirk_value(0, sirk))
    {
        DUMP8("%02x ", sirk, 16);
    }
}

static void _mobile_csisc_add_sirk(void)
{
    TRACE(1, "%s", __func__);
    uint8_t sirk[16] = {0};
    memcpy(sirk, AOB_CSIP_SIRK_TEST, 16);
    aob_csip_mobile_if_add_sirk(sirk);
}

static void _mobile_csisc_resolve_rsi(void)
{
    TRACE(1, "%s", __func__);
    uint8_t rsi[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    ble_bdaddr_t bdadd2conn;
    uint8_t addr[6] = {0x95, 0x33, 0x33, 0x23, 0x22, 0x11};
    memcpy(bdadd2conn.addr, addr, 6);
    bdadd2conn.addr_type = GAPM_STATIC_ADDR;
    aob_csip_mobile_if_resolve_rsi_data(rsi, 6, &bdadd2conn);
}

#ifdef IS_BLE_AUDIO_PTS_TEST_ENABLED
static void _mobile_csisc_discover_service(void)
{
    aob_csip_if_discovery_service(0);
}

static void _mobile_csisc_set_mem_lock(char* cmdParam, uint32_t cmd_length)
{
    do{
        if (cmd_length == 0)
        {
            TRACE(3, "invalid param %d", cmd_length);
            break;
        }

        ATC_CSISC_LOCK_T atc_csip_lock;

        sscanf(cmdParam, "%d %d %d",
            &atc_csip_lock.csip_conlid,
            &atc_csip_lock.csip_setlid,
            &atc_csip_lock.csip_lock
        );

        TRACE(0, "csip_conlid %d", atc_csip_lock.csip_conlid);
        TRACE(0, "csip_setlid %d", atc_csip_lock.csip_setlid);
        TRACE(0, "csip_lock %d", atc_csip_lock.csip_lock);

        aob_csip_if_lock(atc_csip_lock.csip_conlid, atc_csip_lock.csip_setlid, atc_csip_lock.csip_lock);
    } while(0);
}

static void _mobile_pts_test_call_action(char* cmdParam, uint32_t cmdParam_len)
{
    TRACE(1, "%s", __func__);

    int call_id;
    sscanf(cmdParam, "%d", &call_id);
    app_acc_tbs_call_action_req(0, call_id, 4, 4);
}

static void _mobile_pts_chartype_test(void)
{
    TRACE(1,"%s", __func__);

    app_acc_tbs_set_long_req(0, ACC_TB_CHAR_TYPE_URI_SCHEMES_LIST, (uint8_t *)APP_ACC_DFT_URI_SCHEMES_LIST,
                                    sizeof(APP_ACC_DFT_URI_SCHEMES_LIST));
}

static void aob_set_state_mute_disable(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_MUTE, ARC_AIC_MUTE_DISABLED);
}

static void aob_set_agin_mode_manual(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_GAIN_MODE, ARC_AIC_GAIN_MODE_MANUAL_ONLY);
}

static void aob_set_mode_manual(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_GAIN_MODE, ARC_AIC_GAIN_MODE_MANUAL);
}


static void aob_set_mode_auto(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_GAIN_MODE, ARC_AIC_GAIN_MODE_AUTO);
}

static void aob_set_mode_auto_only(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_GAIN_MODE, ARC_AIC_GAIN_MODE_AUTO_ONLY);
}

static void aob_set_state_mute(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_MUTE, ARC_AIC_MUTE_MUTED);
}

static void aob_set_not_mute(void)
{
    app_arc_aics_set(0, ARC_AICS_SET_TYPE_MUTE, ARC_AIC_MUTE_NOT_MUTED);
}

static void aob_set_desc(void)
{
    uint8_t *test = (uint8_t *)"1234";
    app_arc_aics_set_desc(0, test, 4);
}

static void aob_update_pac_record_info(void)
{
    app_gaf_codec_id_t *codec_id = NULL;
    codec_id->codec_id[0] = 6;
    app_bap_capa_srv_update_pac_record_info(0, 1, codec_id, NULL, NULL);
}

static void aob_set_ava_context_bf(void)
{
    app_bap_capa_srv_set_ava_context_bf(0, 0, 0);
}
#endif

static void aob_volume_up_from_mobile(void)
{
    aob_mobile_vol_up(0);
    aob_mobile_vol_up(1);
}

static void aob_volume_down_from_mobile(void)
{
    aob_mobile_vol_down(0);
    aob_mobile_vol_down(1);
}

static void aob_volume_mute_from_mobile(void)
{
    aob_mobile_vol_mute(0);
}

static void aob_volume_unmute_from_mobile(void)
{
    aob_mobile_vol_unmute(0);
}

static void aob_set_mobile_vol_offset(void)
{
    aob_mobile_vol_set_volume_offset(0,0,30);
    aob_mobile_vol_set_volume_offset(1,0,-30);
}

static void aob_set_mobile_abs_vol(void)
{
    aob_mobile_vol_set_abs(0,8);
    aob_mobile_vol_set_abs(1,8);
}

static void ble_audio_update_timing_test_handler(char* cmdParam, uint32_t cmdParam_len)
{
    do{
        if (cmdParam == NULL || cmdParam_len == 0)
        {
            TRACE(3, "%s invalid param %p %d", __func__, cmdParam, cmdParam_len);
            break;
        }

        CIS_TIMING_CONFIGURATION_T updatedCisTimig;

        sscanf(cmdParam, "%d %d %d %d %d %d %d %d",
            &updatedCisTimig.m2s_bn,
            &updatedCisTimig.m2s_nse,
            &updatedCisTimig.m2s_ft,
            &updatedCisTimig.s2m_bn,
            &updatedCisTimig.s2m_nse,
            &updatedCisTimig.s2m_ft,
            &updatedCisTimig.frame_cnt_per_sdu,
            &updatedCisTimig.iso_interval
        );

        TRACE(0, "Update CIS timing:");
        TRACE(0, "m2s_bn %d", updatedCisTimig.m2s_bn);
        TRACE(0, "m2s_nse %d", updatedCisTimig.m2s_nse);
        TRACE(0, "m2s_ft %d", updatedCisTimig.m2s_ft);
        TRACE(0, "s2m_bn %d", updatedCisTimig.s2m_bn);
        TRACE(0, "s2m_nse %d", updatedCisTimig.s2m_nse);
        TRACE(0, "s2m_ft %d", updatedCisTimig.s2m_ft);
        TRACE(0, "frame_cnt_per_sdu %d", updatedCisTimig.frame_cnt_per_sdu);
        TRACE(0, "iso_interval %d", updatedCisTimig.iso_interval);

        app_bap_update_cis_timing(&updatedCisTimig);
    } while(0);
}

static void aob_hap_hac_restore_bond_test(void)
{
    TRACE(0,"%s", __func__);

    aob_hap_hac_restore_bond_data(0);
}

static void aob_hap_hac_msg_get_test(void)
{
    TRACE(0,"%s", __func__);

    app_hap_hac_msg_get_cmd(0, 0, 0);
}

static void ble_audio_test_audio_sharing_init_dynamic_master(void)
{
    TRACE(0,"%s", __func__);

    ble_audio_set_tws_nv_role(BLE_AUDIO_TWS_MASTER);
    ble_audio_update_tws_current_role(BLE_AUDIO_TWS_MASTER);
    ble_audio_test_config_dynamic_audio_sharing_master();
}

#endif // AOB_MOBILE_ENABLED


#ifdef IS_BLE_AUDIO_PTS_TEST_ENABLED
static void aob_csisc_notify_lock_test(char *cmdParam, uint32_t cmdParam_len)
{
    int lock;

    sscanf(cmdParam, "%d", &lock);

    aob_csip_if_restore_bond(0, 0, lock);
}
#endif


void aob_delete_all_gattc_caching_test()
{
    aob_gattc_del_all_nv_cache();
}

#ifdef AOB_UC_TEST
#include "app_utils.h"
#include "app_bt_func.h"
extern uint8_t mobile_freq;
extern uint8_t buds_freq;
POSSIBLY_UNUSED static const enum app_bap_sampling_freq samp_rate_table[5] = {APP_GAF_BAP_SAMPLING_FREQ_16000HZ, APP_GAF_BAP_SAMPLING_FREQ_24000HZ,\
                                                                        APP_GAF_BAP_SAMPLING_FREQ_32000HZ ,APP_GAF_BAP_SAMPLING_FREQ_48000HZ,\
                                                                        APP_GAF_BAP_SAMPLING_FREQ_96000HZ};
POSSIBLY_UNUSED static const uint32_t bit_rate_table[4] = {32000, 48000, 64000, 96000};

POSSIBLY_UNUSED static const enum APP_SYSFREQ_FREQ_T sys_freq_table[6] = {APP_SYSFREQ_32K, APP_SYSFREQ_26M, APP_SYSFREQ_52M, APP_SYSFREQ_78M,\
                                                                    APP_SYSFREQ_104M, APP_SYSFREQ_208M};

POSSIBLY_UNUSED
static struct test_param
{
    uint8_t is_media;
    uint8_t is_10ms;
    uint8_t samp_rate;
    uint32_t bit_rate;
    uint8_t oct;
}test_case;

/**
 * @brief
 *
 * @param oct
 * @param freq
 * @return POSSIBLY_UNUSED
 */
POSSIBLY_UNUSED static void aob_uc_media_test(uint8_t oct, enum app_bap_sampling_freq freq)
{
#ifdef AOB_MOBILE_ENABLED
// Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        (uint16_t)freq, oct, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {

        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
#endif
}

/**
 * @brief
 *
 * @param oct
 * @param freq
 * @return POSSIBLY_UNUSED
 */
POSSIBLY_UNUSED static void aob_uc_call_test(uint8_t oct, enum app_bap_sampling_freq freq)
{
#ifdef AOB_MOBILE_ENABLED
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {(uint16_t)freq, oct, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
        {(uint16_t)freq, oct, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3,  APP_GAF_BAP_CONTEXT_TYPE_CONVERSATIONAL},
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, true);
    }
#endif
}

static void aob_mobile_uc_test_set_param(struct test_param input_param, uint8_t cpu_freq)
{
#ifdef AOB_MOBILE_ENABLED
    test_case.is_media  = input_param.is_media;
    test_case.is_10ms   = input_param.is_10ms;

    test_case.samp_rate = samp_rate_table[input_param.samp_rate];
    test_case.bit_rate  = bit_rate_table[input_param.bit_rate];
    test_case.oct       = (test_case.is_10ms ? 10000 : 7500) * test_case.bit_rate / 8 / 1000 / 1000;
    mobile_freq         = sys_freq_table[cpu_freq];
    TRACE(1, "[AOB_UC_TEST] samp_rate = %d ,bit_rate = %d, oct_per_frame = %d, mobile_freq = %d",\
            test_case.samp_rate, test_case.bit_rate, test_case.oct, mobile_freq);
#endif
}

static void aob_buds_uc_test_set_freq(uint8_t cpu_freq)
{
    buds_freq = sys_freq_table[cpu_freq];
}

static void aob_mobile_uc_set_test_case_param_handler(char *cmdParam, uint32_t cmdParam_len)
{
    /// [AOB,aob_uc_test|is_media is_10MS_frame samp_rate bit_rate]
    do{
        if (cmdParam == NULL || cmdParam_len == 0)
        {
            TRACE(3, "[AOB_UC_TEST] invalid param %p %d", cmdParam, cmdParam_len);
            break;
        }

        if (cmdParam_len < 2)
        {
            int cpu_freq;
            sscanf(cmdParam, "%d", &cpu_freq);
            aob_buds_uc_test_set_freq(cpu_freq);
            break;
        }

        struct test_param test_case_input;
        int cpu_freq;
        sscanf(cmdParam, "%d %d %d %d %d",\
        (int *)&test_case_input.is_media, (int *)&test_case_input.is_10ms, (int *)&test_case_input.samp_rate, (int *)&test_case_input.bit_rate, (int *)&cpu_freq);

        TRACE(2, "[AOB_UC_TEST] is_media = %d, is_10ms = %d, samp_rate = %d bit_rate = %d cpu_freq = %d",\
                test_case_input.is_media, test_case_input.is_10ms, test_case_input.samp_rate, test_case_input.bit_rate, cpu_freq);
        ///Set param into static value local
        aob_mobile_uc_test_set_param(test_case_input, cpu_freq);
    } while(0);
}

static void aob_mobile_uc_test_start_handler(void)
{
    if (1 == test_case.is_media)
    {
        aob_uc_media_test(test_case.oct, (enum app_bap_sampling_freq)test_case.samp_rate);
    }
    else
    {
        aob_uc_call_test(test_case.oct, (enum app_bap_sampling_freq)test_case.samp_rate);
    }
}
#endif

extern "C" void vol_hid_test_down(void);
extern "C" void vol_hid_test_up(void);
extern "C" void vol_hid_test_vol_mute(void);
extern "C" void vol_hid_test_play_swich(void);
extern "C" void vol_hid_test_pause_swich(void);
extern "C" void vol_hid_test_next_track(void);
extern "C" void vol_hid_test_previou_track(void);

extern "C" void vol_hid_test_mic_mute(void);
extern "C" void vol_hid_test_hood_swich(void);
extern "C" void vol_hid_test_play_pause(void);

void ble_hid_vol_down_test(void)
{
    TRACE(0,"======================> ble_hid_vol_down_test");
    vol_hid_test_down();
}

void ble_hid_vol_up_test(void)
{
    TRACE(0,"======================> ble_hid_vol_up_test");
    vol_hid_test_up();
}


void ble_hid_vol_mute_test(void)
{
    TRACE(0,"======================> ble_hid_vol_mute_test");
    vol_hid_test_vol_mute();
}

void ble_hid_test_play_swich_test(void)
{
    TRACE(0,"======================> ble_hid_test_play_swich_test");
    vol_hid_test_play_swich();
}

void ble_hid_test_pause_swich_test(void)
{
    TRACE(0,"======================> ble_hid_test_pause_swich_test");
    vol_hid_test_pause_swich();
}

void ble_hid_test_next_track_test(void)
{
    TRACE(0,"======================> ble_hid_test_next_track_test");
    vol_hid_test_next_track();
}

void ble_hid_test_previous_track_test(void)
{
    TRACE(0,"======================> ble_hid_test_previous_track_test");
    vol_hid_test_previou_track();
}

void ble_hid_mic_mute_test(void)
{
    TRACE(0,"======================> ble_hid_mic_mute_test");
    vol_hid_test_mic_mute();
}

void ble_hid_test_hood_swich_test(void)
{
    TRACE(0,"======================> ble_hid_test_hood_swich_test");
    vol_hid_test_hood_swich();
}

void ble_hid_test_play_pause_test(void)
{
    TRACE(0,"======================> ble_hid_test_play_pause_test");
    vol_hid_test_hood_swich();
}

static const ble_audio_uart_handle_t aob_uart_test_handle[] = {

#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
    {"ble_vol_down_test_hid", ble_hid_vol_down_test},
    {"ble_vol_up_test_hid", ble_hid_vol_up_test},
    {"ble_vol_mute_test_hid",ble_hid_vol_mute_test},
    {"ble_play_test_hid", ble_hid_test_play_swich_test},
    {"ble_pause_test_hid", ble_hid_test_pause_swich_test},
    {"ble_next_track_test_hid",ble_hid_test_next_track_test},
    {"ble_previous_track_test_hid",ble_hid_test_previous_track_test},
    {"ble_mic_mute_test_hid", ble_hid_mic_mute_test},
    {"ble_hook_swich_test_hid", ble_hid_test_hood_swich_test},
    {"ble_play_pause_test_hid", ble_hid_test_play_pause_test},
#endif
    {"open_adv", ble_audio_open_adv_test},
    {"close_adv", ble_audio_close_adv_test},
    {"open_scan", ble_audio_open_scan_test},
    {"close_scan", ble_audio_close_scan_test},
    {"ble_disconnect", ble_audio_disconnect_test},
///test for set rpa timeout interface
    {"set_rpa_timeout", ble_set_rpa_timeout},

    {"start_adv",ble_audio_start_advertising},
    {"start_single_mode_adv",ble_audio_start_single_mode_advertising},
    {"stop_adv",ble_audio_stop_adv},

    {"dump_sirk",dump_ble_audio_sirk_test},
    {"dump_ble_audio_status",dump_ble_core_status_test},

    /// BAP UC server
    {"uc_srv_media_disable", aob_media_disable_test},
    {"uc_srv_media_release", aob_media_release_test},
    {"uc_srv_media_read_iso_link_quality", aob_media_read_iso_link_quality_test},
    {"uc_get_steaming_info", aob_get_streaming_info_test},
    {"uc_set_iso_quality", aob_set_iso_quality_rsp_test},
    {"uc_call_sink_disable", aob_call_sink_disable_handler},
    {"uc_call_src_disable", aob_call_src_disable_handler},
    {"uc_call_sink_release", aob_call_sink_release_handler},
    {"uc_call_src_release", aob_call_src_release_handler},

    //BAP PACS
    {"pacs_set_location", aob_pacs_set_location_test},
    {"set_no_ava_context", ble_audio_set_no_available_context},
    {"restore_ava_context", ble_audio_restore_available_context},

    //BAP BC command for sink/deleg/assist
    {"bc_scan_start", aob_bc_scan_start_handler},
    {"bc_scan_stop", aob_bc_scan_stop_handler},

    //BAP BC command for deleg
    {"bc_deleg_start", aob_bc_deleg_start_handler},
    {"bc_deleg_stop", aob_bc_deleg_stop_handler},

    {"bc_sink_stream_start", aob_bc_sink_start_streaming},
    {"bc_sink_stream_stop", aob_bc_sink_stop_streaming},

    {"bc_sink_pa_sync", aob_bc_sink_scan_pa_sync},
    {"bc_sink_pa_term", aob_bc_sink_scan_pa_terminate},
    {"bc_sink_disable",aob_bc_sink_disable},
    {"bc_sink_start",aob_bc_sink_start},
    {"bc_sink_stop", aob_bc_sink_stop},

#ifdef APP_BLE_BIS_SRC_ENABLE
    //Bis src
    {"bc_src_stream_start", aob_bc_src_start_streaming},
    {"bc_src_stream_stop", aob_bc_src_stop_streaming},
    //BAP BC command for src
    {"bc_src_start", aob_bc_src_start_handler},
    {"bc_src_stop", aob_bc_src_stop_handler},
#endif

#ifdef AOB_MOBILE_ENABLED
    //BAP BC command for assist
    {"bc_assist_start", aob_bc_assist_start_handler},
    {"bc_assist_stop", aob_bc_assist_stop_handler},
    {"bc_assist_scan_src", aob_bc_assist_scan_src_handler},
#endif

    {"mics_set_mute", aob_media_mics_set_mute_test},
    {"mics_set_unmute", aob_media_mics_set_unmute_test},

    //ARC commands for earbud
    {"arc_vcs_mute", aob_vol_mute},
    {"arc_vcs_unmute", aob_vol_unmute},
    {"arc_vcs_up", aob_vol_up},
    {"arc_vcs_down", aob_vol_down},
    {"arc_vocs_offset", aob_set_vol_offset},
    {"arc_vcs_abs", aob_set_abs_vol},

    /// ACC commands for earbuds(master/slave)
    {"acc_tbc_call_accept", _earbud_accept_call},
    {"acc_tbc_call1_terminate", _earbud_terminate_call1},
    {"acc_tbc_call2_terminate", _earbud_terminate_call2},
    {"acc_tbc_hold_call1", _earbud_hold_call1},
    {"acc_tbc_hold_call2", _earbud_hold_call2},
    {"acc_tbc_retrieve_call1", _earbud_retrieve_call1},
    {"acc_tbc_retrieve_call2", _earbud_retrieve_call2},
    {"acc_tbc_call_outgoing", _earbud_dial},
    {"acc_tbc_call_state_get", _earbud_call_state_get},

    /// ATC CSIPM commands for earbud
    {"atc_csism_update_rsi", _earbud_csism_update_rsi_data},
    {"atc_csism_update_sirk", _earbud_csism_update_sirk},
    {"atc_csism_get_rsi", _earbud_csism_get_rsi_data},

    /// ACC MCC command for earbuds
    {"acc_mcc_media_play", aob_media_play_handler},
    {"acc_mcc_media_pause", aob_media_pause_handler},
    {"acc_mcc_media_stop", aob_media_stop_handler},
    {"acc_mcc_media_prev", aob_media_prev_handler},
    {"acc_mcc_media_next", aob_media_next_handler},

    ///get CIS state for earbuds
    {"aob_ble_check_connected", ble_check_connected_test},
    {"aob_get_call_state", aob_get_call_state_test},
    {"aob_get_media_state", aob_get_media_state_test},

    /// TMAP command for mobile
    {"tmap_svc_start", aob_tmap_start_handler},

    {"iap_test_earbud_start", _earbud_iap_test_start_test},
    {"iap_test_earbud_end", _earbud_iap_test_end_test},
    {"iap_test_earbud_get_cnt", _earbud_iap_test_get_cnt_test},
    {"iap_tm_earbud_setup", _earbud_iap_test_mode_setup},
    {"iap_hci_get_iso_tx_sync", aob_read_iso_tx_sync},

    /// DTS
    {"dts_config_28", aob_dts_configure_28},
    {"dts_config_29", aob_dts_configure_29},

    {"dts_send_data_28", aob_dts_send_data_28},
    {"dts_send_data_29", aob_dts_send_data_29},

    {"dts_disconnect", aob_dts_disconnect_28},
    {"dts_disconnect", aob_dts_disconnect_29},

    //{"aob_delete_all_gatt_catch", aob_gattc_delete_all_nv_cache},
    /// HAP
    {"has_cfg_preset", _earbud_hap_has_cfg_preset},
    {"has_set_active_preset", _earbud_hap_has_set_active_preset},
    {"has_set_soor_sup", _earbud_hap_has_set_coor_sup},

    /// tx power
    {"tx_power_report_enable", _tx_power_report_enable},
    {"tx_power_report_disenable", _tx_power_report_disenable},
    {"tx_power_get_local",  _tx_power_get_local},
    {"tx_power_get_remote", _tx_power_get_remote},
    {"set_path_loss", _set_path_loss_test},

#ifdef CFG_SUBRATE
    ///Subrate
    {"set_def_subrate",  _set_default_subrate_test},
    {"subrate_req",      _subrate_request_test},
#endif // (CFG_SUBRATE)

    // All commands for mobile phone add here
#ifdef AOB_MOBILE_ENABLED
//BAP PACC
    {"pacc_set_location", aob_pacc_set_location_test},

    /// BAP Unicast client
    {"uc_mobile_scan_start", aob_mobile_scan_start_handler},
    {"uc_mobile_start_reconnect", aob_mobile_start_reconnect},
    {"uc_media_start", aob_media_start_test},           //96kbps
    {"uc_media_rejoin", aob_media_rejoin_media_test}, //96kbps
#if defined (AOB_GMAP_ENABLED)
    {"uc_gmap_start", aob_gmap_start_test},
#endif
    {"uc_media_7_5ms_start", aob_media_7_5ms_start_test},
    {"uc_resume_audio",aob_resume_audio_test},
#ifdef BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT
#if defined (AOB_GMAP_ENABLED)
    {"uc_stereo_gmap_start", aob_stereo_gmap_start_test},
#endif
    {"uc_stereo_media_start", aob_stereo_media_start_test},//128kbps
    {"uc_stereo_call_start", aob_stereo_call_start_test},
    {"uc_stereo_ai_start", aob_stereo_ai_start_test},

#endif
#ifdef LC3PLUS_SUPPORT
    {"uc_media_lc3plus_start", aob_media_start_test_lc3plus_handler},
    {"uc_media_lc3plus_5ms_start", aob_media_start_test_lc3plus_5ms_handler},
#endif
    {"uc_media_32k_start", aob_media_start_32k_test},   //32kbps
    {"uc_ai_2ch_up_start", aob_ai_start_dual_channel_upstream_handler},
#ifdef HID_ULL_ENABLE
    {"uc_ULL_start", ble_audio_ull_capture_start_handler_test},
#endif
    {"uc_ULL_release", aob_ull_ascc_release_stream_test},
    {"uc_live_2ch_up_start", aob_live_start_dual_channel_upstream_handler},
    {"uc_media_release", aob_media_ascc_release_stream_test},
    {"uc_media_disable", aob_media_ascc_disable_stream_test},
    {"uc_call_stream_start", aob_call_start_stream_handler},
    {"uc_media_enable", aob_media_ascc_enable_stream_test},

    {"uc_usb_media_32k_start", aob_usb_media_start_32k_test},   //32kbps
    {"uc_usb_ai_2ch_up_start", aob_usb_ai_start_dual_channel_upstream_handler},
    {"uc_usb_call_stream_start", aob_usb_call_start_stream_handler},
#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
    {"uc_non_audio_stream_start", aob_non_audio_start_stream_handler},
    {"uc_non_audio_tws_stream_start", aob_non_audio_tws_start_stream_handler},
#endif
    {"uc_game_dual_stream_start", aob_game_start_conversational_stream_handler},
    {"uc_game_stream_start", aob_game_start_stream_handler},

    //ARC commands for mobile
    {"arc_svc_start", aob_arc_start_handler},
    {"arc_vcc_up", aob_volume_up_from_mobile},
    {"arc_vcc_down", aob_volume_down_from_mobile},
    {"arc_vcc_mute",aob_volume_mute_from_mobile},
    {"arc_vcc_unmute",aob_volume_unmute_from_mobile},
    {"arc_vocc_offset", aob_set_mobile_vol_offset},
    {"arc_vcc_abs", aob_set_mobile_abs_vol},

    {"micc_set_mute", aob_media_micc_set_mute},
    {"micc_set_unmute", aob_media_micc_set_unmute},

    /// TBS commands
    {"acc_tbs_call_incoming", _mobile_on_call_incoming},
    {"acc_tbs_call_outgoing", _mobile_on_call_outgoing},
    {"acc_tbs_call_accept", _mobile_on_call_accepted},
    {"acc_tbs_call_terminate_call1", _mobile_on_call_terminate_call1},
    {"acc_tbs_call_terminate_call2", _mobile_on_call_terminate_call2},
    {"acc_tbs_call_hold_local_call1", _mobile_on_call_hold_local_call1},
    {"acc_tbs_call_hold_local_call2", _mobile_on_call_hold_local_call2},
    {"acc_tbs_call_hold_remote_call1", _mobile_on_call_hold_remote_call1},
    {"acc_tbs_call_hold_remote_call2", _mobile_on_call_hold_remote_call2},
    {"acc_tbs_call_retrieve_local_call1", _mobile_on_call_retrieve_local_call1},
    {"acc_tbs_call_retrieve_local_call2", _mobile_on_call_retrieve_local_call2},
    {"acc_tbs_call_answer", _mobile_on_call_remote_answer},
    {"acc_tbs_call_alert", _mobile_on_call_remote_alert},
    {"aob_mobile_get_call_info", _mobile_get_call_info},

    /// ATC CSIPS commands
    {"atc_csisc_read_sirk", _mobile_csisc_read_sirk},
    {"atc_csisc_read_rank", _mobile_csisc_read_dev_rank_index},
    {"atc_csisc_read_lock", _mobile_csisc_read_dev_lock_status},
    {"atc_csisc_read_size", _mobile_csisc_read_size},
    {"atc_csisc_add_sirk", _mobile_csisc_add_sirk},
    {"atc_csisc_resolve_rsi", _mobile_csisc_resolve_rsi},

    // IAP Test mode commands
    {"iap_test_mobile_start_test", _mobile_iap_test_start_test},
    {"iap_tm_mobile_setup", _mobile_iap_test_mode_setup},

    /// Gatt cache test cmd
    {"gattc_caching_display", aob_gattc_display_cache_server},
    {"gattc_delete_caching", aob_delete_all_gattc_caching_test},
    {"gattc_unit_test", aob_gattc_caching_unit_test},

    /// DTC
    {"dtc_connect_28", aob_dtc_connect_28},
    {"dtc_connect_29", aob_dtc_connect_29},

    {"dtc_send_data_28", aob_dtc_send_data_28},
    {"dtc_send_data_29", aob_dtc_send_data_29},

    {"dtc_disconnect_28", aob_dtc_disconnect_28},
    {"dtc_disconnect_29", aob_dtc_disconnect_29},

    /// HAP
    {"hap_hac_res_bond", aob_hap_hac_restore_bond_test},
    {"hap_hac_msg_get",aob_hap_hac_msg_get_test},

    // DYNAMIC AUDIO SHARING
    // ble_audio_test_audio_sharing_init_dynamic_master
    // This uart cmd is for master part and should be called after the update of Master_BLE_addr
    {"audio_sharing_dynamic_master_init", ble_audio_test_audio_sharing_init_dynamic_master},
#endif
    // PTS commands add here
#ifdef IS_BLE_AUDIO_PTS_TEST_ENABLED
    {"atc_csisc_discover_service", _mobile_csisc_discover_service},
    {"tbs_chartype_test", _mobile_pts_chartype_test},
    {"aics_set_state_disable", aob_set_state_mute_disable},
    {"aics_set_agin_mode_manual", aob_set_agin_mode_manual},
    {"aics_set_mode_manual", aob_set_mode_manual},
    {"aics_set_mode_auto", aob_set_mode_auto},
    {"aics_set_mode_only_auto", aob_set_mode_auto_only},
    {"aics_set_state_mute", aob_set_state_mute},
    {"aics_set_not_mute", aob_set_not_mute},
    {"aics_set_set_desc", aob_set_desc},
    {"pacs_update_pac_record_info", aob_update_pac_record_info},
    {"bap_set_ava_context_bf", aob_set_ava_context_bf},
#endif

#ifdef DYNAMIC_SET_PB_TIME
    {"start_itfr",gaf_media_stream_start_interference},
    {"stop_itfr",gaf_media_stream_stop_interference},
#endif

#ifdef AOB_UC_TEST
    {"aob_uc_test_start", aob_mobile_uc_test_start_handler},
#endif
#if (BLE_APP_DATAPATH_CLIENT)
    {"datapath_notification_enable", ble_app_datapath_client_control_notification_test},
    {"datapath_client_command_send", ble_datapath_transmission_by_command_test},
#endif
#if (BLE_APP_DATAPATH_SERVER)
    {"datapath_notification_send", ble_datapath_transmission_by_notification_test},
#endif

#ifdef WIRELESS_MIC
   {"rec_start", app_wireless_mic_start_record},
   {"rec_op_test", app_wireless_mic_opcode_test},
#endif
};

void aob_test_config_peer_ble_addr(uint8_t index, uint8_t* pAddr);

static void ble_audio_test_config_peer_ble_addr(char *cmdParam, uint32_t cmdParam_len)
{
    int index;
    int value[6];
    uint8_t peer_addr[6];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%d %x %x %x %x %x %x", &index, &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5]);
        for (uint8_t i = 0; i < 6; i++) {
            peer_addr[i] = value[i];
        }

        aob_test_config_peer_ble_addr(index, peer_addr);
    }
}

void ble_audio_set_bis_src_ble_addr(const uint8_t *addr);
static void ble_audio_test_config_bis_src_ble_addr(char *cmdParam, uint32_t cmdParam_len)
{
    int value[6];
    uint8_t bis_src_addr[6];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x %x %x %x %x %x", &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5]);
        for (uint8_t i = 0; i < 6; i++) {
            bis_src_addr[i] = value[i];
        }

        ble_audio_set_bis_src_ble_addr((const uint8_t *)bis_src_addr);
    }
}

static void ble_audio_test_update_ble_addr(char *cmdParam, uint32_t cmdParam_len)
{
    int value[6];
    uint8_t ble_addr[6];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x %x %x %x %x %x", &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5]);
        for (uint8_t i = 0; i < 6; i++) {
            ble_addr[i] = value[i];
        }

        app_ble_tws_sync_write_ble_address(ble_addr);

        app_reset();
    }
}

static void ble_audio_test_config_encrypt_request(char *cmdParam, uint32_t cmdParam_len)
{
    int enabled;

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x", &enabled);
        app_sec_set_ble_master_send_encrypt_req_enabled_flag(enabled);
        TRACE(1, "is_ble_master_send_encrypt_req_enabled = %d", app_sec_is_ble_master_send_encrypt_req_enabled());
    }
}

static void ble_audio_test_slave_initiate_secure_connection(char *cmdParam, uint32_t cmdParam_len)
{
    int enabled;
    int value;

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x %d", &enabled, &value);
        app_sec_set_slave_security_request_enabled_flag(enabled);
        if (enabled)
        {
            app_sec_set_ble_connection_authentication_level(value);
        }
        TRACE(2, "is_slave_initiate_security_request = %d, ble_connection_authentication_level = %u", app_sec_is_slave_security_request_enabled(), app_sec_get_ble_connection_authentication_level());
    }
}

#ifdef AOB_MOBILE_ENABLED
static void ble_audio_test_start_pairing(char *cmdParam, uint32_t cmdParam_len)
{
    int conidx;
    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x", &conidx);
        app_sec_start_pairing(conidx);
    }
}

static void ble_audio_test_audio_sharing_config_master_addr(char *cmdParam, uint32_t cmdParam_len)
{
    int value[6];
    uint8_t master_ble_addr[6];

    if (cmdParam != NULL)
    {
        sscanf(cmdParam, "%x %x %x %x %x %x", &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5]);
        for (uint8_t i = 0; i < 6; i++)
        {
            master_ble_addr[i] = value[i];
        }

        ble_audio_set_tws_peer_ble_addr(master_ble_addr);
    }
}
#ifdef HID_ULL_ENABLE
static void ble_audio_ull_capture_start_handler_test_with_param(char *cmdParam, uint32_t cmdParam_len)
{
    int conidx;
    int value[4];
    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x", &conidx);
        app_sec_start_pairing(conidx);
    }

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x %x %x %x",
            &value[0], &value[1],
            &value[2], &value[3]);
    }
    static CIS_TIMING_CONFIGURATION_T dft_cis_timing_test =
    {
        .m2s_bn = value[1],
        .m2s_nse = value[2],
        .m2s_ft = value[3],
        .s2m_bn = value[1],
        .s2m_nse = value[2],
        .s2m_ft = value[3],
        .frame_cnt_per_sdu = 1,
        .iso_interval = value[0],  // 5ms = 4 * 1.25ms
    };
    ble_audio_test_update_cis_timing(dft_cis_timing_test);
    ble_audio_test_update_sdu_intv(2500, 2500);

    LOG_I("%s", __func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {APP_BAP_SAMPLING_FREQ_48000HZ, 40, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_ULL,  BES_BLE_GAF_CONTEXT_TYPE_MAN_MACHINE_BIT},
    };
    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++) {
        aob_media_mobile_start_stream(ase_to_start, i, false);
    }
}
#endif
#endif

static void ble_audio_set_audio_configuration_test(char *cmdParam, uint32_t cmdParam_len)
{
    uint32_t aud_cfg_select = 0;
    /// We use first byte to fetch idx select
    if (cmdParam_len > 0)
    {
        sscanf(cmdParam, "%d", &aud_cfg_select);
    }

#if !defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
    if (aud_cfg_select > AOB_AUD_CFG_TWS_MONO)
    {
        LOG_E("%s unsupport stereo %d", __func__, aud_cfg_select);
        return;
    }
#endif

    /// trans app_mode to bes aob aud cfg
    nvrec_appmode_e bes_app_mode = NV_APP_MONE_MIN;
    switch (aud_cfg_select)
    {
        case AOB_AUD_CFG_TWS_MONO:
            bes_app_mode = NV_APP_EARBUDS_MONO;
            break;
        case AOB_AUD_CFG_TWS_STEREO_ONE_CIS:
            bes_app_mode = NV_APP_EARBUDS_STEREO_ONE_CIS;
            break;
        case AOB_AUD_CFG_TWS_STEREO_TWO_CIS:
            bes_app_mode = NV_APP_EARBUDS_STEREO_TWO_CIS;
            break;
        case AOB_AUD_CFG_FREEMAN_STEREO_ONE_CIS:
            bes_app_mode = NV_APP_HEADSET_STEREO_ONE_CIS;
            break;
        case AOB_AUD_CFG_FREEMAN_STEREO_TWO_CIS:
            bes_app_mode = NV_APP_HEADSET_STEREO_TWO_CIS;
            break;
        default:
            LOG_E("%s unsupport %d", __func__, aud_cfg_select);
            return;
    }

    app_switch_mode(bes_app_mode, true);
}

#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
static void ble_audio_change_head_track_test_handler(char *cmdParam, uint32_t cmdParam_len)
{
    /// head_tracking data
    int value[14];
    uint8_t head_tracking_data[14];

    if (cmdParam != NULL) {
        sscanf(cmdParam, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x",
            &value[0], &value[1],
            &value[2], &value[3], &value[4], &value[5],
            &value[6], &value[7], &value[8], &value[9],
            &value[10], &value[11], &value[12], &value[13]);
        for (uint8_t i = 0; i < 14; i++) {
            head_tracking_data[i] = value[i];
        }
        aob_media_changed_head_tracking_data_handler(head_tracking_data);
    }
}
#endif

static void ble_audio_bap_codec_capability_setting(char *cmdParam, uint32_t cmdParam_len)
{
    int BAP_Codec_Cfg = 0;
    if (!cmdParam|| !cmdParam_len)
    {
        return;
    }
    if (1 != sscanf(cmdParam, "%d", &BAP_Codec_Cfg))
    {
        TRACE(2, "%s invalid cmdParam %s", __func__, cmdParam);
        return;
    }
    BAP_Codec_Capability_Cfg = BAP_Codec_Cfg;
    TRACE(0,"%s,BAP_Codec_Capability_Cfg==%d",__func__,BAP_Codec_Capability_Cfg);
}

uint8_t ble_audio_test_get_manually_configured_bap_codec_capability_cfg(void)
{
    return BAP_Codec_Capability_Cfg;
}

static void ble_audio_bap_Retransmiss_num_setting(char *cmdParam, uint32_t cmdParam_len)
{
    int Retransmiss_num = 0;
    if (!cmdParam|| !cmdParam_len)
    {
        return;
    }
    if (1 != sscanf(cmdParam, "%d", &Retransmiss_num))
    {
        TRACE(2, "%s invalid cmdParam %s", __func__, cmdParam);
        return;
    }
    New_Rtn_num = Retransmiss_num;
    TRACE(0,"%s,New_Rtn_num==%d",__func__,New_Rtn_num);
}

uint8_t ble_audio_test_get_manually_configured_rtn_num(void)
{
    return New_Rtn_num;
}

static const ble_audio_uart_handle_with_param_t aob_uart_test_handle_with_param[]=
{
#if defined(BLE_APP_DATAPATH_SERVER) && defined(USB_AUDIO_CUSTOM_USB_HID_KEY)
    {"usb_hid_cmd", send_hid_cmd_data_to_dongle_by_datapath},
#endif
    // Example: [AOB,mobile_connect|214 204 251 165 185 226]
    {"mobile_connect", ble_audio_mobile_connect_test},
    {"mobile_disconnect", ble_audio_mobile_disconnect_test},
    {"config_peer_addr", ble_audio_test_config_peer_ble_addr},
    {"config_bis_src_addr", ble_audio_test_config_bis_src_ble_addr},
    {"config_aud_cfg", ble_audio_set_audio_configuration_test},
    {"update_ble_addr", ble_audio_test_update_ble_addr},
    {"bap_codec_capability_setting",ble_audio_bap_codec_capability_setting},
    {"bap_retransmiss_num_setting",ble_audio_bap_Retransmiss_num_setting},
#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
    {"changed_head_tracking_data", ble_audio_change_head_track_test_handler},
#endif
#ifdef AOB_MOBILE_ENABLED
#ifdef HID_ULL_ENABLE
    {"uc_ULL_setparameter", ble_audio_ull_capture_start_handler_test_with_param},
#endif
    // All commands for mobile phone add here
    {"update_timing", ble_audio_update_timing_test_handler},
    // ex [AOB,aob_connect_devices|11222333336600|9AED365D25D401...]
    {"aob_connect_devices", ble_audio_mobile_connect_any_devices},
    {"aob_set_cis_num_in_cig", ble_audio_mobile_set_cis_num_in_cig},
    // [AOB,start_ble_pairing|0] to start pairing on the connection with index 0
    // the index can be updated accordingly
    {"start_ble_pairing", ble_audio_test_start_pairing},
    // ble_audio_test_audio_sharing_config_master_ble_address
    // This uart cmd is for mobile part
    {"audio_sharing_config_master_addr", ble_audio_test_audio_sharing_config_master_addr},
#endif

#ifdef APP_BLE_BIS_SRC_ENABLE
    {"config_bis_code", ble_audio_test_config_bis_code},
    {"config_bis_id", ble_audio_test_config_bis_id},
    {"enable_bis_encryption", ble_audio_test_enable_bis_encryption},
#endif

    {"uc_srv_set_presdelay_test", aob_uc_srv_set_presdelay_test},

    // BAP BC SCAN
    {"bc_set_bcast_id", aob_bc_scan_set_bcast_id},
    {"bc_set_bcast_code", aob_bc_scan_set_bcast_code},
    {"bc_scan_channel_start", aob_bc_scan_param_start_handler},

#ifdef AOB_UC_TEST
    {"aob_uc_test_set_param", aob_mobile_uc_set_test_case_param_handler},
#endif

    {"config_encrypt", ble_audio_test_config_encrypt_request},
    {"ble_slave_initiate_secure_connection", ble_audio_test_slave_initiate_secure_connection},

    // PTS commands add here

#ifdef IS_BLE_AUDIO_PTS_TEST_ENABLED
    {"csisc_notify_lock", aob_csisc_notify_lock_test},
    {"atc_csisc_set_mem_lock", _mobile_csisc_set_mem_lock},
    {"tbs_call_action_test", _mobile_pts_test_call_action},
#endif
};

#if AOB_CUSTOM_GAF_STREAM_DATA_DWT_ENABLE
/*****************************************GAF MEDIA TEST**********************************************/
struct HAL_IOMUX_PIN_FUNCTION_MAP aob_test_gaf_gpio[] =
{
    {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
};

static void aob_gpio_toggle_init(void)
{
    hal_iomux_init(aob_test_gaf_gpio, ARRAY_SIZE(aob_test_gaf_gpio));
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)aob_test_gaf_gpio[0].pin, HAL_GPIO_DIR_OUT, 1);
}

POSSIBLY_UNUSED static void aob_gpio_toggle_test(void)
{
    hal_gpio_pin_set((enum HAL_GPIO_PIN_T)aob_test_gaf_gpio[0].pin);
    hal_sys_timer_delay_us(2);
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)aob_test_gaf_gpio[0].pin);
}

/******************************UC SRV DATA CB SAMPLE***************************/
static uint8_t aob_uc_srv_get_encoded_packet_recv_cb(const gaf_media_data_t *header)
{
    /// Do something here
    return 0;
}

static uint8_t aob_uc_srv_get_decoded_raw_data_cb(const gaf_media_data_t *header, const uint8 *raw_data, uint32_t len)
{
    /// Do something here
    if (header->sdu_data != NULL)
    {
        if (header->sdu_data[header->data_len-5] == 0xFF &&
            header->sdu_data[header->data_len-4] == 0xFE &&
            header->sdu_data[header->data_len-3] == 0xFD)
        {
            TRACE(1, "FIND THE PATTERN");
            aob_gpio_toggle_test();
        }
    }

    return 0;
}

static const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T uc_srv_cb_list =
{
    .encoded_packet_recv_cb = aob_uc_srv_get_encoded_packet_recv_cb,
    .decoded_raw_data_cb = aob_uc_srv_get_decoded_raw_data_cb,
    .encoded_packet_send_cb = NULL,
    .raw_pcm_data_cb = NULL,
};

#ifdef AOB_MOBILE_ENABLED
/******************************UC CLI DATA CB***************************/
static uint8_t aob_uc_cli_get_pcm_raw_data_cb(uint8 *raw_data, uint32_t len)
{
    /// Do something here
    return 0;
}

static uint8_t aob_uc_cli_encoded_packet_send_cb(uint8 *packet_to_send, uint32_t len)
{
    /// Do something here
    static uint16_t pattern_put_times_checker = 0;
    if (packet_to_send && (pattern_put_times_checker++ % 100 == 0))
    {
        TRACE(1, "PUT THE PATTERN");
        packet_to_send[len-5] = 0xFF;
        packet_to_send[len-4] = 0xFE;
        packet_to_send[len-3] = 0xFD;
        aob_gpio_toggle_test();
    }

    if (pattern_put_times_checker > 1000)
    {
        pattern_put_times_checker = 1;
    }
    return 0;
}

static const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T uc_cli_cb_list =
{
    .encoded_packet_recv_cb = NULL,
    .decoded_raw_data_cb = NULL,
    .encoded_packet_send_cb = aob_uc_cli_encoded_packet_send_cb,
    .raw_pcm_data_cb = aob_uc_cli_get_pcm_raw_data_cb,
};
#endif

static void aob_set_gaf_media_test_callback(void)
{
    gaf_stream_common_set_custom_data_handler(GAF_STREAM_USER_CASE_UC_SRV,
                                              &uc_srv_cb_list);
#ifdef AOB_MOBILE_ENABLED
    gaf_stream_common_set_custom_data_handler(GAF_STREAM_USER_CASE_UC_CLI,
                                              &uc_cli_cb_list);
#endif
}
#endif

/*****************************************UART CMD TEST**********************************************/
ble_audio_uart_test_function_handle_with_param ble_audio_test_find_uart_handle_with_param(char* buf)
{
    ble_audio_uart_test_function_handle_with_param p = NULL;
    for(uint32_t i = 0; i < ARRAY_SIZE(aob_uart_test_handle_with_param); i++)
    {
        if (strncmp((char*)buf, aob_uart_test_handle_with_param[i].string,
            strlen(aob_uart_test_handle_with_param[i].string))==0 ||
            strstr(aob_uart_test_handle_with_param[i].string, (char*)buf))
        {
            TRACE(1, "AOB cmd:%s", aob_uart_test_handle_with_param[i].string);
            p = aob_uart_test_handle_with_param[i].function;
            break;
        }
    }
    return p;
}

int ble_audio_uart_cmd_with_param_handler(
    char* cmd, uint32_t cmdLen, char* cmdParam, uint32_t cmdParamLen)
{
    int ret = 0;

    ble_audio_uart_test_function_handle_with_param handl_function =
        ble_audio_test_find_uart_handle_with_param(cmd);
    if(handl_function)
    {
        handl_function(cmdParam, cmdParamLen);
    }
    else
    {
        ret = -1;
        TRACE(0,"can not find handle function");
    }
    return ret;
}

ble_audio_uart_test_function_handle ble_audio_test_find_uart_handle(unsigned char* buf)
{
    ble_audio_uart_test_function_handle p = NULL;
    for(uint32_t i = 0; i < ARRAY_SIZE(aob_uart_test_handle); i++)
    {
        if (strncmp((char*)buf, aob_uart_test_handle[i].string, strlen(aob_uart_test_handle[i].string))==0 ||
            strstr(aob_uart_test_handle[i].string, (char*)buf))
        {
            TRACE(1, "AOB cmd:%s", aob_uart_test_handle[i].string);
            p = aob_uart_test_handle[i].function;
            break;
        }
    }
    return p;
}

int ble_audio_uart_cmd_handler(unsigned char *buf, unsigned int length)
{
    int ret = 0;

    ble_audio_uart_test_function_handle handl_function = ble_audio_test_find_uart_handle(buf);
    if(handl_function)
    {
        //handl_function();
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)handl_function);
    }
    else
    {
        ret = -1;
        TRACE(0,"can not find handle function");
    }
    return ret;
}

unsigned int ble_audio_uart_cmd_callback(unsigned char *cmd, unsigned int cmd_length)
{
    int param_len = 0;
    char* cmd_param = NULL;
    char* cmd_end = (char *)cmd + cmd_length;

    cmd_param = strstr((char*)cmd, (char*)"|");

    if (cmd_param)
    {
        *cmd_param = '\0';
        cmd_length = cmd_param - (char *)cmd;
        cmd_param += 1;

        param_len = cmd_end - cmd_param;

        ble_audio_uart_cmd_with_param_handler((char *)cmd, cmd_length, cmd_param, param_len);
    }
    else
    {
        ble_audio_uart_cmd_handler((unsigned char*)cmd,strlen((char*)cmd));
    }

    return 0;
}

void ble_audio_uart_cmd_init(void)
{
#ifdef APP_TRACE_RX_ENABLE
    TRACE(0,"ble_audio_uart_cmd_init");
    app_trace_rx_register("AOB", ble_audio_uart_cmd_callback);
#endif
#if AOB_CUSTOM_GAF_STREAM_DATA_DWT_ENABLE
    aob_set_gaf_media_test_callback();
    aob_gpio_toggle_init();
#endif
}


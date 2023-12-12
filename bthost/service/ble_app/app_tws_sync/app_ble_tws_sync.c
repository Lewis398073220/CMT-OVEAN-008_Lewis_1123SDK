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
#include "app_ble_tws_sync.h"

#if defined(BLE_USE_TWS_SYNC) && defined(IBRT)
#include "app.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_ble_tws_sync.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "nvrecord_ble.h"
#include "app_ble_mode_switch.h"
#include "app_tws_ibrt.h"
#include "app_ibrt_custom_cmd.h"
#include "app_ibrt_middleware.h"
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#include "bt_drv_interface.h"
#include "app_bt_sync.h"

#if IBRT_UI
#include "app_ui_evt_thread.h"
#endif



#if BLE_AUDIO_ENABLED
#include "aob_csip_api.h"
#include "aob_bis_api.h"
#include "aob_service_sync.h"
#include "gaf_media_sync.h"
#include "app_gaf_common.h"
#endif

#if BLE_AUDIO_ENABLED && defined(IBRT_UI)

static void aob_tws_sync_csip_sirk_key(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
    app_ibrt_send_cmd_without_rsp(IBRT_AOB_CMD_SYNC_DEV_INFO, p_buff, length);
}

static void aob_tws_sync_csip_sirk_key_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
    NV_RECORD_BLE_AUDIO_DEV_INFO_T *pInfo = (NV_RECORD_BLE_AUDIO_DEV_INFO_T*)p_buff;
    NV_RECORD_BLE_AUDIO_DEV_INFO_T *pInfoLocal = nv_record_bleaudio_get_ptr();
    if ((pInfoLocal) && (memcmp(pInfoLocal->sirk, pInfo->sirk, 16)))
    {
        pInfo->set_member = 1;
        nv_record_bleaudio_update_devinfo((uint8_t*)pInfo);
        aob_csip_if_update_sirk(pInfo->sirk, 16);
    }
}


/*****************************************************************************
 Prototype    : app_ibrt_aob_tws_exch_ble_audio_info_rsp_timeout_handler
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/4
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_aob_tws_exch_ble_audio_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
}
/*****************************************************************************
 Prototype    : app_ibrt_aob_tws_exch_ble_audio_info_rsp_handler
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/4
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_aob_tws_exch_ble_audio_info_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
}
/*****************************************************************************
 Prototype    : app_ibrt_aob_tws_exch_ble_audio_info_trigger
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/1
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_aob_tws_exch_ble_audio_info(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
    app_ibrt_send_cmd_with_rsp(IBRT_AOB_CMD_EXCH_BLE_AUDIO_INFO, p_buff, length);
}

void app_ble_audio_share_gattc_service_info(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
    app_ibrt_send_cmd_with_rsp(AOB_CMD_SHARE_SERVICE_INFO, p_buff, length);
}

void app_ble_audio_share_gattc_service_info_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
    app_ble_audio_recv_service_data(p_buff,length);
    tws_ctrl_send_rsp(AOB_CMD_SHARE_SERVICE_INFO, rsp_seq,p_buff, length);
}

void aob_share_gattc_svc_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
}

void aob_share_gattc_svc_info_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
}

/*****************************************************************************
 Prototype    : app_ibrt_aob_tws_exch_ble_audio_info_handler
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/1
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_aob_tws_exch_ble_audio_info_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s", __func__);
#ifdef APP_BLE_BIS_SINK_ENABLE
    aob_bis_tws_sync_state_req_handler(p_buff);
    tws_ctrl_send_rsp(IBRT_AOB_CMD_EXCH_BLE_AUDIO_INFO, rsp_seq, p_buff, length);
#endif
}

void aob_tws_send_sync_us_since_latest_anchor_point_cmd(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_AOB_CMD_SYNC_CAPTURE_US_SINCE_LATEST_ANCHOR_POINT, p_buff, length);
}

void aob_tws_send_sync_capture_trigger_cmd(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_AOB_CMD_REQ_TRIGGER_SYNC_CAPTURE, p_buff, length);
}

static const app_tws_cmd_instance_t g_aob_tws_cmd_handler_table[]=
{
    {
        IBRT_AOB_CMD_EXCH_BLE_AUDIO_INFO,               "_AOB_CMD_EXCH_BLE_AUDIO_INFO",
        app_ibrt_aob_tws_exch_ble_audio_info,
        app_ibrt_aob_tws_exch_ble_audio_info_handler,   RSP_TIMEOUT_DEFAULT,
        app_ibrt_aob_tws_exch_ble_audio_info_rsp_timeout_handler,     app_ibrt_aob_tws_exch_ble_audio_info_rsp_handler,
        NULL
    },
    {
        IBRT_AOB_CMD_REQ_TRIGGER_SYNC_CAPTURE,           "_AOB_CMD_REQ_TRIGGER_SYNC_CAPTURE",
        aob_tws_send_sync_capture_trigger_cmd,
        aob_tws_sync_capture_trigger_handler,          3000,
        aob_tws_sync_capture_trigger_rsp_timeout_handler,     aob_tws_sync_capture_trigger_rsp_handler,
        NULL
    },
    {
        IBRT_AOB_CMD_SYNC_CAPTURE_US_SINCE_LATEST_ANCHOR_POINT,           "_AOB_CMD_SYNC_US_SINCE_LATEST_ANCHOR_POINT",
        aob_tws_send_sync_us_since_latest_anchor_point_cmd,
        aob_tws_sync_us_since_latest_anchor_point_handler,          0,
        NULL,     NULL,
        NULL
    },

    {
        IBRT_AOB_CMD_SYNC_DEV_INFO,                       "_AOB_CMD_SYNC_DEV_INFO",
        aob_tws_sync_csip_sirk_key,
        aob_tws_sync_csip_sirk_key_handler,                   0,
        NULL,     NULL,
    },

    {
        AOB_CMD_SHARE_SERVICE_INFO,                       "_AOB_CMD_SHARE_SERVICE_INFO",
        app_ble_audio_share_gattc_service_info,
        app_ble_audio_share_gattc_service_info_handler,    RSP_TIMEOUT_DEFAULT,
        aob_share_gattc_svc_info_rsp_timeout_handler,      aob_share_gattc_svc_info_rsp_handler,
    },
};

static void ble_audio_info_prepare_handler(uint8_t *buf,
    uint16_t *totalLen, uint16_t *len, uint16_t expectLen)
{
    NV_RECORD_BLE_AUDIO_DEV_INFO_T *info = nv_record_bleaudio_get_ptr();
    if (!info) {
        return;
    }

    *totalLen = *len = sizeof(NV_RECORD_BLE_AUDIO_DEV_INFO_T);
    memcpy(buf, (uint8_t*)info, *totalLen);

}

static void ble_audio_info_received_handler(uint8_t *buf, uint16_t length, bool isContinueInfo)
{
    NV_RECORD_BLE_AUDIO_DEV_INFO_T *pInfo = (NV_RECORD_BLE_AUDIO_DEV_INFO_T*)buf;
    nv_record_bleaudio_update_devinfo((uint8_t*)pInfo);
}

void ble_audio_tws_sync_init(void)
{
    TRACE(0, "ble_audio_tws_sync_init");
    TWS_SYNC_USER_T userLeAudio = {
        ble_audio_info_prepare_handler,
        ble_audio_info_received_handler,
        NULL,
        NULL,
        NULL,
    };

    app_ibrt_if_register_sync_user(TWS_SYNC_USER_LE_AUDIO, &userLeAudio);
}

int app_ble_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size)
{
    *cmd_tbl = (void *)&g_aob_tws_cmd_handler_table;
    *cmd_size = ARRAY_SIZE(g_aob_tws_cmd_handler_table);
    return 0;
}

bool app_ble_tws_get_conn_state()
{
    return app_ibrt_conn_is_tws_connected();
}

APP_BLE_TWS_SYNC_ROLE_E app_ble_tws_get_tws_ui_role()
{
    return (APP_BLE_TWS_SYNC_ROLE_E)app_ibrt_conn_get_ui_role();
}

APP_BLE_TWS_SYNC_ROLE_E app_ble_tws_get_tws_local_role()
{
    return (APP_BLE_TWS_SYNC_ROLE_E)app_tws_ibrt_get_local_tws_role();
}

int app_ble_tws_sync_send_cmd(APP_BLE_TWS_SYNC_CMD_CODE_E code, uint8_t high_priority, uint8_t *data, uint16_t data_len)
{
    int ret = 0;
    app_ibrt_aob_cmd_code_e tws_cmd;

    tws_cmd = IBRT_AOB_CMD_EXCH_BLE_AUDIO_INFO + (code-BLE_TWS_SYNC_EXCH_BLE_AUDIO_INFO);
    if (high_priority)
    {
        ret = tws_ctrl_send_cmd_high_priority(tws_cmd, data, data_len);
    }
    else
    {
        ret = tws_ctrl_send_cmd(tws_cmd, data, data_len);
    }
    return ret;
}

int app_ble_tws_sync_send_rsp(APP_BLE_TWS_SYNC_CMD_CODE_E code, uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    int ret = 0;
    app_ibrt_aob_cmd_code_e tws_cmd;

    tws_cmd = IBRT_AOB_CMD_EXCH_BLE_AUDIO_INFO + (code-BLE_TWS_SYNC_EXCH_BLE_AUDIO_INFO);
    ret = tws_ctrl_send_rsp(tws_cmd , rsp_seq, p_buff, length);
    return ret;
}

void app_ble_tws_sync_sniff_manager(uint8_t con_lid, bool streaming)
{
    ble_bdaddr_t remote_addr = {{0}};

    bool succ = app_ble_get_peer_solved_addr(con_lid, &remote_addr);
    TRACE(0, "d(%d) ble acl sniff manager streaming = %d", con_lid,streaming);
    if (succ)
    {
        DUMP8("%02x ",remote_addr.addr, 6);
    }

    if (streaming)
    {
        // TWS sniff control
        app_tws_ibrt_exit_sniff_with_tws();
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        bt_bdaddr_t *p_tws_addr = btif_me_get_remote_device_bdaddr(p_ibrt_ctrl->p_tws_remote_dev);
        if (p_tws_addr)
        {
            app_tws_ibrt_write_link_policy(p_tws_addr, BTIF_BLP_MASTER_SLAVE_SWITCH);
        }

        //Mobile sniff control
        bt_drv_reg_op_set_ibrt_reject_sniff_req(true);
        // public address
        if (succ)
        {
            app_tws_ibrt_exit_sniff_with_mobile((bt_bdaddr_t*)remote_addr.addr);
        }
    }
    else
    {
        // TWS sniff control
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        bt_bdaddr_t *p_tws_addr = btif_me_get_remote_device_bdaddr(p_ibrt_ctrl->p_tws_remote_dev);
        if (p_tws_addr)
        {
            app_tws_ibrt_write_link_policy(p_tws_addr, BTIF_BLP_MASTER_SLAVE_SWITCH | BTIF_BLP_SNIFF_MODE);
        }

        // Mobile sniff control
        bt_drv_reg_op_set_ibrt_reject_sniff_req(false);
    }
}

int app_ble_tws_sync_write_ble_address(uint8_t *ble_addr)
{
    app_ibrt_if_write_ble_local_address(ble_addr);
    return 0;
}

int app_ble_tws_sync_ui_ascs_bond_ntf(void *data)
{
    app_ui_notify_le_core_event(data);
    return 0;
}

#endif

#ifdef BLE_AOB_VOLUME_SYNC_ENABLED
app_ble_tws_sync_volume_rec_callback volume_sync_receive_handler = NULL;
app_ble_tws_sync_volume_callback volume_sync_trigger_handler = NULL;
app_ble_tws_sync_volume_callback volume_sync_offset_trigger_handler = NULL;;

static void  app_ble_tws_sync_volume_handler(void)
{

    if(volume_sync_trigger_handler)
    {
        volume_sync_trigger_handler();
    }
}

static void  app_ble_tws_sync_volume_offset_handler(void)
{

    if(volume_sync_offset_trigger_handler)
    {
        volume_sync_offset_trigger_handler();
    }
}

APP_BT_SYNC_COMMAND_TO_ADD(APP_BT_SYNC_OP_VOLUME, app_ble_tws_sync_volume_handler, NULL);
APP_BT_SYNC_COMMAND_TO_ADD(APP_BT_SYNC_OP_VOLUME_OFFSET, app_ble_tws_sync_volume_offset_handler, NULL);

static void app_ble_tws_sync_info_report(uint32_t opCode, uint8_t *buf, uint8_t len)
{
    switch (opCode)
    {
        case APP_BT_SYNC_OP_RETRIGGER:
            break;
        case APP_BT_SYNC_OP_VOLUME:
            if (volume_sync_receive_handler)
            {
                volume_sync_receive_handler(buf, len);
            }
            break;
        case APP_BT_SYNC_OP_VOLUME_OFFSET:
            break;
        default:
            break;
    }
}

bool app_ble_tws_sync_volume_register(app_ble_tws_sync_volume_rec_callback receive_cb,
                                                   app_ble_tws_sync_volume_callback trigger_cb,
                                                   app_ble_tws_sync_volume_callback offset_trigger_cb)
{
    volume_sync_receive_handler        = receive_cb;
    volume_sync_trigger_handler        = trigger_cb;
    volume_sync_offset_trigger_handler = offset_trigger_cb;
    app_bt_sync_register_report_info_callback(app_ble_tws_sync_info_report);
    return 1;
}

bool app_ble_tws_sync_volume(uint8_t *data, uint8_t data_len)
{
    return app_bt_sync_enable(APP_BT_SYNC_OP_VOLUME, data_len, data);
}
#endif

uint32_t app_ble_tws_get_conn_curr_ticks()
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    uint32_t curr_ticks = 0;
    curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle);
    return BTCLKS_TO_US(curr_ticks);
}

int app_ble_tws_sync_release_trigger_channel(uint8_t chnl)
{
    app_bt_sync_release_trigger_channel(chnl);
    return 0;
}

uint32_t app_ble_tws_sync_get_slave_time_from_master_time(uint32_t master_clk_cnt, uint16_t master_bit_cnt)
{
    return app_bt_sync_get_slave_time_from_master_time(master_clk_cnt, master_bit_cnt);
}

int app_ble_tws_sync_get_master_time_from_slave_time(uint32_t SlaveBtTicksUs, uint32_t* p_master_clk_cnt, uint16_t* p_master_bit_cnt)
{
    app_bt_sync_get_master_time_from_slave_time(SlaveBtTicksUs, p_master_clk_cnt, p_master_bit_cnt);
    return 0;
}

int app_ble_tws_sync_is_connected_register(app_ble_tws_is_connected_t cb)
{
    app_tws_is_connected_register((tws_is_connected_t)cb);;
    return 0;
}

uint8_t app_ble_tws_sync_get_avaliable_trigger_channel()
{
    return app_bt_sync_get_avaliable_trigger_channel();
}

int app_ble_tws_sync_release_cmd_semphore(void)
{
    app_ibrt_release_cmd_semphore();
    return 0;
}

int app_ble_tws_sync_send_cmd_via_ble_register(app_ble_tws_cmd_send_via_ble_t cb)
{
    app_ibrt_send_cmd_via_ble_register((app_tws_cmd_send_via_ble_t)cb);
    return 0;
}

#else

void ble_audio_tws_sync_init(void)
{

}


bool app_ble_tws_get_conn_state()
{
    return 0;
}
uint32_t app_ble_tws_get_conn_curr_ticks()
{
    return 0;
}

bool app_ble_tws_sync_volume_register(app_ble_tws_sync_volume_rec_callback receive_cb,
                                                   app_ble_tws_sync_volume_callback trigger_cb,
                                                   app_ble_tws_sync_volume_callback offset_trigger_cb)
{
    return 0;
}

bool app_ble_tws_sync_volume(uint8_t *data, uint8_t data_len)
{
    return 0;
}

APP_BLE_TWS_SYNC_ROLE_E app_ble_tws_get_tws_ui_role()
{
    return APP_BLE_TWS_UNKNOW;
}
APP_BLE_TWS_SYNC_ROLE_E app_ble_tws_get_tws_local_role()
{
    return APP_BLE_TWS_UNKNOW;
}

int app_ble_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size)
{
    *cmd_tbl  = NULL;
    *cmd_size = 0;
    return -1;
}
int app_ble_tws_sync_send_cmd(APP_BLE_TWS_SYNC_CMD_CODE_E code, uint8_t high_priority, uint8_t *data, uint16_t data_len)
{
    return -1;
}

int app_ble_tws_sync_send_rsp(APP_BLE_TWS_SYNC_CMD_CODE_E code, uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    return -1;
}

void app_ble_tws_sync_sniff_manager(uint8_t con_lid, bool streaming)
{
}

int app_ble_tws_sync_release_trigger_channel(uint8_t chnl)
{
    return -1;
}

uint32_t app_ble_tws_sync_get_slave_time_from_master_time(uint32_t master_clk_cnt, uint16_t master_bit_cnt)
{
    return 0;
}

int app_ble_tws_sync_get_master_time_from_slave_time(uint32_t SlaveBtTicksUs, uint32_t* p_master_clk_cnt, uint16_t* p_master_bit_cnt)
{
    return 0;
}

int app_ble_tws_sync_write_ble_address(uint8_t *ble_addr)
{
    return -1;
}

int app_ble_tws_sync_ui_ascs_bond_ntf(void *data)
{
    return -1;
}

uint8_t app_ble_tws_sync_get_avaliable_trigger_channel()
{
    /// You need to enable the IBRT control to obtain a valid trigger channel
    return 0xFF;
}

int app_ble_tws_sync_is_connected_register(app_ble_tws_is_connected_t cb)
{
    return -1;
}

int app_ble_tws_sync_release_cmd_semphore(void)
{
    return -1;
}

int app_ble_tws_sync_send_cmd_via_ble_register(app_ble_tws_cmd_send_via_ble_t cb)
{
    return -1;
}

#endif


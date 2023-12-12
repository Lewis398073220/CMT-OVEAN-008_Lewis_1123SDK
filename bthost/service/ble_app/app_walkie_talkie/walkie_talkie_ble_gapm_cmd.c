/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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

#include "ke_task.h"
#include "ke_msg.h"
#include "gapm_le_msg.h"
#include "gapc_msg.h"
#include "bt_drv_reg_op.h"
#include "co_hci.h"
#include "hl_hci.h"
#include "walkie_talkie_ble_gapm_cmd.h"

void walkie_stop_activity(uint8_t actv_idx)
{
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                  TASK_GAPM, TASK_BLE_WALKIE,
                                                  gapm_activity_stop_cmd);
    if (!cmd)
    {
        return;
    }

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_ACTIVITY;
    cmd->actv_idx = actv_idx;

    // Send the message
    ke_msg_send(cmd);
}

void walkie_delete_activity(uint8_t actv_idx)
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_delete_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                      TASK_GAPM, TASK_BLE_WALKIE,
                                                      gapm_activity_delete_cmd);
    if (!cmd)
    {
        return;
    }

    // Fill the allocated kernel message
    cmd->operation = GAPM_DELETE_ACTIVITY;
    cmd->actv_idx = actv_idx;

    // Send the message
    ke_msg_send(cmd);
}

void walkie_adv_creat(uint8_t *adv_para)
{
    struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                    TASK_GAPM, TASK_BLE_WALKIE,
                                                    gapm_activity_create_adv_cmd);

    if (!p_cmd)
    {
       return;
    }

    memcpy(p_cmd, adv_para, sizeof(struct gapm_activity_create_adv_cmd));
    // Set operation code
    p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

    // Send the message
    ke_msg_send(p_cmd);
}

void walkie_adv_enable(uint16_t duration, uint8_t max_adv_evt, uint8_t actv_idx)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                    TASK_GAPM, TASK_BLE_WALKIE,
                                                    gapm_activity_start_cmd,
                                                    sizeof(gapm_adv_param_t));
    if (!p_cmd)
    {
       return;
    }

    gapm_adv_param_t *adv_add_param = (gapm_adv_param_t *)(p_cmd->u_param);
    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = actv_idx;
    adv_add_param->duration = duration;
    adv_add_param->max_adv_evt = max_adv_evt;

    // Send the message
    ke_msg_send(p_cmd);
}


void walkie_set_adv_data_cmd(uint8_t operation, uint8_t actv_idx,
                                      uint8_t *adv_data, uint8_t data_len)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                TASK_GAPM, TASK_BLE_WALKIE,
                                                gapm_set_adv_data_cmd,
                                                data_len);
    if (!p_cmd)
    {
        return;
    }
    // Fill the allocated kernel message
    p_cmd->operation       = operation;
    p_cmd->actv_idx        = actv_idx;
    p_cmd->length          = data_len;

    memcpy(p_cmd->data, adv_data, data_len);

    // Send the message
    ke_msg_send(p_cmd);
}

void walkie_periodic_sync_create(uint8_t own_addr_type)
{
    struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                         TASK_GAPM, TASK_BLE_WALKIE,
                                                         gapm_activity_create_cmd);
    if (!p_cmd)
    {
        return;
    }

    // Set operation code
    p_cmd->operation = GAPM_CREATE_PERIOD_SYNC_ACTIVITY;
    p_cmd->own_addr_type = own_addr_type;

    // Send the message
    ke_msg_send(p_cmd);
}

void walkie_periodic_sync_enable(uint8_t actv_idx, uint8_t *per_para)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_BLE_WALKIE,
                                                         gapm_activity_start_cmd,
                                                         sizeof(gapm_per_sync_param_t));
    if (!p_cmd)
    {
        return;
    }

    gapm_per_sync_param_t *per_sync_param = (gapm_per_sync_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = actv_idx;
    memcpy(per_sync_param, per_para, sizeof(gapm_per_sync_param_t));

    // Send the message
    ke_msg_send(p_cmd);
}

void walkie_scan_creat(uint8_t own_addr_type)
{
    struct gapm_activity_create_cmd *p_cmd =
        KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,TASK_GAPM, TASK_BLE_WALKIE,gapm_activity_create_cmd);
    if (!p_cmd)
    {
        return;
    }

    // Set operation code
    p_cmd->operation = GAPM_CREATE_SCAN_ACTIVITY;
    p_cmd->own_addr_type = own_addr_type;

    // Send the message
    ke_msg_send(p_cmd);
}

void walkie_scan_enable(uint8_t actv_idx,uint8_t *scan_para)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_BLE_WALKIE,
                                                         gapm_activity_start_cmd,
                                                         sizeof(gapm_scan_param_t));
    if (!p_cmd)
    {
        return;
    }

    gapm_scan_param_t *scan_param = (gapm_scan_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = actv_idx;
    memcpy(scan_param, scan_para, sizeof(gapm_scan_param_t));

    // Send the message
    ke_msg_send(p_cmd);
}

void walkie_set_device_list(uint8_t list_type, uint8_t *bdaddr, uint8_t size)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    uint8_t para_len = ((list_type == GAPM_SET_WL)? sizeof(gap_bdaddr_t):sizeof(gap_per_adv_bdaddr_t)) * size;
    struct gapm_list_set_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_LIST_SET_CMD,
                                                         TASK_GAPM, TASK_BLE_WALKIE,
                                                         gapm_list_set_cmd,
                                                         para_len);
    if (!p_cmd)
    {
        return;
    }

    p_cmd->operation = list_type;
    p_cmd->size = size;
    if(bdaddr)
    {
        if (list_type == GAPM_SET_WL){
            struct gapm_list_set_wl_cmd *para_data = (struct gapm_list_set_wl_cmd *)p_cmd;
            memcpy(&para_data->wl_info[0], bdaddr, sizeof(gap_bdaddr_t)*size);
        }
        else
        {
            struct gapm_list_set_pal_cmd *para_data = (struct gapm_list_set_pal_cmd *)p_cmd;
            for(int i=0; i<size; i++)
            {
                memcpy(&para_data->pal_info[i], bdaddr + i*sizeof(gap_bdaddr_t), sizeof(gap_bdaddr_t));
                para_data->pal_info[i].adv_sid = 0;
            }
        }
    }
    TRACE(0, "W-T-G:%s type=%x, size=%x", __FUNCTION__, list_type, size);
    // Send the message
    ke_msg_send(p_cmd);
}

walie_gap_cb_func_t* walkie_cb = NULL;

static int walkie_gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                            struct gapm_cmp_evt *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
     if(walkie_cb){
          return(walkie_cb->cmp_evt_handler(param->operation, param->status, param->actv_idx));
     }else{
          return (KE_MSG_CONSUMED);
     }
}

static int walkie_gapm_activity_created_ind_handler(ke_msg_id_t const msgid,
                                      struct gapm_activity_created_ind const *p_param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    TRACE(0, "W-T-G:actv index %d created, type=%d, tx_pwr=%d", p_param->actv_idx,
                            p_param->actv_type, p_param->tx_pwr);

    return (KE_MSG_CONSUMED);
}

static int walkie_gapm_adv_report_evt_handler(ke_msg_id_t const msgid,
                                     struct gapm_ext_adv_report_ind *param,
                                     ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
	 if(walkie_cb){
          return(walkie_cb->adv_handler(param->data , param->actv_idx, param->trans_addr.addr, param->rssi, param->length));
     }else{
          return (KE_MSG_CONSUMED);
     }
}

static int walkie_gapm_activity_stopped_ind_handler(ke_msg_id_t const msgid,
                                      struct gapm_activity_stopped_ind const *p_param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
     TRACE(0, "W-T-G:activity_stopped:actv index %d, type=%d, %d,%d",
        p_param->actv_idx,p_param->actv_type, p_param->reason, p_param->per_adv_stop);
     if(walkie_cb){
          return(walkie_cb->stop_ind(p_param->actv_idx));
     }else{
          return (KE_MSG_CONSUMED);
     }
}
 int walkie_gapm_sync_established_evt_handler(ke_msg_id_t const msgid,
                                  struct gapm_sync_established_ind *param,
                                  ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    TRACE(0, "[%s][%d]W-T-G: sync device, actv_idx=%d", __FUNCTION__, __LINE__, param->actv_idx);
    DUMP8("%02x ", &param->addr.addr, GAP_BD_ADDR_LEN);
     if(walkie_cb){
          return(walkie_cb->per_sync_est_evt(param->actv_idx, param->addr.addr));
     }else{
          return (KE_MSG_CONSUMED);
     }

}
static int walkie_appm_msg_handler(ke_msg_id_t const msgid,
                          void *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}

KE_MSG_HANDLER_TAB(ble_walkie)
{
    /// please add in order of gapm_msg size
    // GAPM messages
    {GAPM_CMP_EVT,                 (ke_msg_func_t)walkie_gapm_cmp_evt_handler},
    {GAPM_ACTIVITY_CREATED_IND,    (ke_msg_func_t)walkie_gapm_activity_created_ind_handler},
    {GAPM_ACTIVITY_STOPPED_IND,    (ke_msg_func_t)walkie_gapm_activity_stopped_ind_handler},
    {GAPM_EXT_ADV_REPORT_IND,      (ke_msg_func_t)walkie_gapm_adv_report_evt_handler},
    {GAPM_SYNC_ESTABLISHED_IND,    (ke_msg_func_t)walkie_gapm_sync_established_evt_handler},
    {KE_MSG_DEFAULT_HANDLER,       (ke_msg_func_t)walkie_appm_msg_handler},
};

// Application task descriptor
static ke_state_t ble_walkie_state;
const struct ke_task_desc TASK_BLE_WALKIE_APP = {ble_walkie_msg_handler_tab, &ble_walkie_state, 1, ARRAY_LEN(ble_walkie_msg_handler_tab)};

void walkie_ble_gapm_task_init(walie_gap_cb_func_t * cb)
{
    if (ke_task_check(TASK_BLE_WALKIE) == TASK_NONE) {//task type exist
        // Create APP task
        ke_task_create(TASK_BLE_WALKIE, &TASK_BLE_WALKIE_APP);
        // Initialize Task state
        ke_state_set(TASK_BLE_WALKIE, 0);
    }
    walkie_cb = cb;
}

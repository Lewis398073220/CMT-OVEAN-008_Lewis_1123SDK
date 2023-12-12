/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"          // SW configuration


#if (BLE_APP_PRESENT)

#include "app_task.h"             // Application Manager Task API
#include "gapc_bt_msg.h"            // GAP Controller Task API
#include "gapm_le_msg.h"            // GAP LE Manager Task API
#include "gapc_le_msg.h"
#include "gapm_msg.h"
#include "gapc_msg.h"
#include "gatt_user.h"

#include "arch.h"                 // Platform Definitions
#include <string.h>
#include "co_utils.h"
#include "ke_timer.h"             // Kernel timer
#include "app_ble_include.h"
#include "app.h"                  // Application Manager Definition
#include "app_gatt.h"
#include "l2cap_msg.h"
#include "hci_ble.h"

#if (BLE_APP_SEC)
#include "app_sec.h"              // Security Module Definition
#endif //(BLE_APP_SEC)

#if (BLE_APP_HT)
#include "app_ht.h"               // Health Thermometer Module Definition
#include "htpt_msg.h"
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"             // Battery Module Definition
#include "bass_msg.h"//#include "bass_task.h"
#endif //(BLE_APP_BATT)

#if (BLE_APP_HID)
#include "app_hid.h"              // HID Module Definition
#include "hogpd_msg.h"//#include "hogpd_task.h"
#endif //(BLE_APP_HID)

#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h"       // Data Path Server Module Definition
#include "datapathps_task.h"
#endif // (BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_SAS_SERVER)
#include "app_sas_server.h"    // Switching Ambient Service Server Module Definition
#include "sas_task.h"
#endif // (BLE_APP_SAS_SERVER)

#if (BLE_APP_AHP_SERVER)
#include "app_ahp_server.h"            // Advanced Headphone Server Module Definition
#include "ahp_ahs.h"
#endif //(BLE_APP_AHP_SERVER)
#if (BLE_APP_DATAPATH_CLIENT)
#include "app_datapath_client.h"       // Data Path Client Module Definition
#include "datapathpc_task.h"
#endif // (BLE_APP_DATAPATH_CLIENT)

#if (BLE_APP_AI_VOICE)
#include "app_ai.h"       // ama Voice Module Definition
#endif // (BLE_APP_AI_VOICE)

#if (BLE_APP_OTA)
#include "app_ota.h"       // OTA Module Definition
#endif // (BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h"       // TOTA Module Definition
#endif // (BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "app_ancc.h"       // ANC Module Definition
#include "app_ancc_task.h"
#include "ancc_task.h"
#endif // (BLE_APP_ANCC)

#if (BLE_APP_AMSC)
#include "app_amsc.h"       // AMS Module Definition
#include "app_amsc_task.h"
#include "amsc_task.h"
#endif // (BLE_APP_AMS)

#if (BLE_APP_GFPS)
#include "app_gfps.h"       // google fast pair service provider
#include "gfps_provider_task.h"
#endif // (BLE_APP_GFPS)

#if (DISPLAY_SUPPORT)
#include "app_display.h"          // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#include "ble_app_dbg.h"
#include "nvrecord_ble.h"
#include "bluetooth_bt_api.h"
#include "app_fp_rfcomm.h"
#if (BLE_APP_TILE)
#include "app_tile.h"
#include "tile_gatt_server.h"
#endif
#if BLE_AUDIO_ENABLED
#include "app_gaf.h"
#endif

#if (BLE_BUDS)
#include "buds.h"
#endif

#if BLE_AUDIO_ENABLED
#include "aob_gatt_cache.h"
#endif

#include "bt_drv_interface.h"

#if (mHDT_LE_SUPPORT)
#include "mhdt_le_msg.h"
#endif

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
#include "app_dbg_ble_audio_info.h"
#endif

void app_adv_reported_scanned(struct gapm_ext_adv_report_ind *ptInd);
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#define APP_CONN_PARAM_INTERVEL_MIN    (20)

#ifndef BLE_ADV_REGENERATE_NEW_RPA_DURATION
#define BLE_ADV_REGENERATE_NEW_RPA_DURATION (60*15)
#endif

uint8_t ble_stack_ready = 0;

#if defined(BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT)
extern ble_bdaddr_t ble_rnd_addr;
#endif

extern bool app_factorymode_get(void);

static uint8_t app_get_handler(const struct app_subtask_handlers *handler_list_desc,
                               ke_msg_id_t msgid,
                               void *param,
                               ke_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list_desc->msg_cnt; 0 < counter; counter--)
    {
        struct ke_msg_handler handler
            = (struct ke_msg_handler)(*(handler_list_desc->p_msg_handler_tab + counter - 1));

        if ((handler.id == msgid) ||
                (handler.id == KE_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KE_MSG_CONSUMED);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_CREATED_IND event
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_created_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_created_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    LOG_I("adv actv index %d", p_param->actv_idx);
    return (KE_MSG_CONSUMED);
}

void app_ble_host_reset(void)
{
    if (ble_stack_ready)
    {
        return;
    }

#ifdef __FACTORY_MODE_SUPPORT__
    if (app_factorymode_get())
    {
        return;
    }
#endif

    BLE_APP_FUNC_ENTER();

    ble_stack_ready = 1;

    TRACE(0, "ble_stack_ready: initialize ble ...");

    // Reset the stack
    struct gapm_reset_cmd *cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                              TASK_GAPM, TASK_APP,
                                              gapm_reset_cmd);

    cmd->operation = GAPM_RESET;

    ke_msg_send(cmd);
    BLE_APP_FUNC_LEAVE();
}

void gapm_read_rpa_addr_cmd(const ble_bdaddr_t *peer_identify_addr)
{
    struct gapm_get_ral_addr_cmd *p_cmd = KE_MSG_ALLOC(GAPM_GET_RAL_ADDR_CMD,
                                                       TASK_GAPM, TASK_APP,
                                                       gapm_get_ral_addr_cmd);
    p_cmd->operation = GAPM_GET_RAL_LOC_ADDR;
    memcpy((uint8_t *)&p_cmd->peer_identity, (uint8_t *)peer_identify_addr, sizeof(ble_bdaddr_t));

    ke_msg_send(p_cmd);
}

#ifdef GFPS_ENABLED
void app_gfps_read_rpa_when_bt_connect(const bt_bdaddr_t *peer_addr)
{
    //to read the rsp address
    ble_bdaddr_t  peer_identify_address;

    peer_identify_address.addr_type = GAPM_STATIC_ADDR;
    memcpy(peer_identify_address.addr, (uint8_t *)bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
    gapm_read_rpa_addr_cmd(&peer_identify_address);

}
#endif

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_STOPPED_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_stopped_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_stopped_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    enum app_actv_state state = app_env.state[p_param->actv_idx];
    enum app_actv_state new_state = state;

    if (state == APP_ADV_STATE_STARTED)
    {
        new_state = APP_ADV_STATE_STOPPING;
    }
    else if (state == APP_SCAN_STATE_STARTED)
    {
        new_state = APP_SCAN_STATE_STOPPING;
    }
    else if (state == APP_CONNECT_STATE_STARTED)
    {
        new_state = APP_CONNECT_STATE_STOPPING;
        app_connecting_stopped((void *)&p_param->peer_addr);
    }

    LOG_I("%s actv_idx %d state %d new_state %d", __func__, p_param->actv_idx, state, new_state);
    if (new_state != state)
    {
        // Act as if activity had been stopped by the application
        appm_update_actv_state(p_param->actv_idx, new_state);
        // Perform next operation
        appm_actv_fsm_next(p_param->actv_idx, GAP_ERR_NO_ERROR);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    BLE_APP_FUNC_ENTER();
    LOG_I("param->operation: 0x%x status is 0x%x actv_idx is %d",
          param->operation, param->status, param->actv_idx);

    switch (param->operation)
    {
        // Reset completed
        case (GAPM_RESET):
        {
            if (param->status == GAP_ERR_NO_ERROR)
            {
#if (BLE_APP_HID)
                app_hid_start_mouse();
#endif //(BLE_APP_HID)

                // Set Device configuration
                struct gapm_set_dev_config_cmd *cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
                                                                   TASK_GAPM, TASK_APP,
                                                                   gapm_set_dev_config_cmd);
                // Set the operation
                cmd->operation = GAPM_SET_DEV_CONFIG;
                // Set the device role - Peripheral
                cmd->cfg.role      = GAP_ROLE_ALL;
                // Set Data length parameters
                cmd->cfg.sugg_max_tx_octets = APP_MAX_TX_OCTETS;
                cmd->cfg.sugg_max_tx_time   = APP_MAX_TX_TIME;
                cmd->cfg.pairing_mode = GAPM_PAIRING_LEGACY;
#ifdef CFG_SEC_CON
                cmd->cfg.pairing_mode |= GAPM_PAIRING_SEC_CON;
#endif

                // Disable ATT MTU EXEC
                SETF(cmd->cfg.att_cfg, GAPM_ATT_CLI_DIS_AUTO_MTU_EXCH, 1);

#if defined (BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT) || defined (BLE_HOST_PTS_TEST_ENABLED)
                SETF(cmd->cfg.att_cfg, GAPM_ATT_CLI_DIS_AUTO_FEAT_EN, 1);
                SETF(cmd->cfg.att_cfg, GAPM_ATT_CLI_DIS_AUTO_EATT, 1);
#else
                SETF(cmd->cfg.att_cfg, GAPM_DISABLE_DB_HASH_CHAR, 1);
#endif

#if defined (BLE_ADV_RPA_ENABLED)
                cmd->cfg.renew_dur = BLE_ADV_REGENERATE_NEW_RPA_DURATION;
                cmd->cfg.privacy_cfg = GAPM_PRIV_CFG_PRIV_EN_BIT;
#else
                cmd->cfg.renew_dur = BLE_ADV_REGENERATE_NEW_RPA_DURATION;
                cmd->cfg.privacy_cfg = 0;
#endif

                memcpy(cmd->cfg.addr.addr, ble_global_addr, BD_ADDR_LEN);
                cmd->cfg.tx_pref_phy = GAP_PHY_LE_2MBPS;
                cmd->cfg.rx_pref_phy = GAP_PHY_LE_2MBPS;

                if (app_env.tx_pref_phy != 0)
                {
                    cmd->cfg.tx_pref_phy = app_env.tx_pref_phy;
                }

                if (app_env.rx_pref_phy != 0)
                {
                    cmd->cfg.rx_pref_phy = app_env.rx_pref_phy;
                }

                // load IRK
                memcpy(cmd->cfg.irk.key, app_env.loc_irk, KEY_LEN);

                // Send message
                ke_msg_send(cmd);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;

        case (GAPM_PROFILE_TASK_ADD):
        {
            bool status = !appm_add_svc();
            // ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);
            // Add the next requested service
            if (status)
            {
                // Go to the ready state
                ke_state_set(TASK_APP, APPM_READY);

                //add user gatt module
                app_ble_gatt_reg(APP_BLE_GATT_ROLE_SERVER);
                app_ble_gatt_reg(APP_BLE_GATT_ROLE_CLIENT);

                // No more service to add
                app_ble_system_ready();
#if (mHDT_LE_SUPPORT)
                //app_task_mhdt_rd_local_proprietary_feat();
#endif
            }
        }
        break;

        // Device Configuration updated
        case (GAPM_SET_DEV_CONFIG):
        {
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            // Go to the create db state
            ke_state_set(TASK_APP, APPM_CREATE_DB);

            // Add the first required service in the database
            // and wait for the PROFILE_ADDED_IND
            appm_add_svc();
        }
        break;


        case (GAPM_CREATE_ADV_ACTIVITY):
        {
            if (param->status == GAP_ERR_NO_ERROR)
            {
                if (APP_ACTV_STATE_IDLE == app_env.state[param->actv_idx])
                {
                    // Store the advertising activity index
                    app_env.adv_actv_idx[app_env.advParam.adv_actv_user] = param->actv_idx;
                }
                appm_update_actv_state(param->actv_idx, APP_ADV_STATE_CREATING);
                appm_actv_fsm_next(param->actv_idx, param->status);
            }
            else
            {
                ASSERT(0, "%s GAPM_CREATE_ADV_ACTIVITY status 0x%x", __func__, param->status);
            }
            break;
        }
        case (GAPM_CREATE_SCAN_ACTIVITY):
        {
            if (param->status == GAP_ERR_NO_ERROR)
            {
                if (APP_ACTV_STATE_IDLE == app_env.state[param->actv_idx])
                {
                    // Store the scanning activity index
                    app_env.scan_actv_idx = param->actv_idx;
                }
                appm_update_actv_state(param->actv_idx, APP_SCAN_STATE_CREATING);
                appm_actv_fsm_next(param->actv_idx, param->status);
            }
            else
            {
                ASSERT(0, "%s GAPM_CREATE_SCAN_ACTIVITY status %d", __func__, param->status);
            }
            break;
        }
        case (GAPM_CREATE_INIT_ACTIVITY):
        {
            if (param->status == GAP_ERR_NO_ERROR)
            {
                if (APP_ACTV_STATE_IDLE == app_env.state[param->actv_idx])
                {
                    // Store the connecting activity index
                    app_env.connect_actv_idx = param->actv_idx;
                }
                appm_update_actv_state(param->actv_idx, APP_CONNECT_STATE_CREATING);
                appm_actv_fsm_next(param->actv_idx, param->status);
            }
            else
            {
                ASSERT(0, "%s GAPM_CREATE_INIT_ACTIVITY status %d", __func__, param->status);
            }
            break;
        }
        case (GAPM_STOP_ACTIVITY):
        case (GAPM_START_ACTIVITY):
        case (GAPM_DELETE_ACTIVITY):
        case (GAPM_SET_ADV_DATA):
        case (GAPM_SET_PERIOD_ADV_DATA):
        case (GAPM_SET_SCAN_RSP_DATA):
        {
            // Perform next operation
            appm_actv_fsm_next(param->actv_idx, param->status);
        }
        break;

        case (GAPM_DELETE_ALL_ACTIVITIES) :
        {
            //don't use this function!!!

            // Re-Invoke Advertising
            //appm_update_actv_state(param->actv_idx, APP_ACTV_STATE_IDLE);
            //appm_actv_fsm_next(param->actv_idx);

            //appm_update_actv_state(param->actv_idx, APP_SCAN_STATE_IDLE);
            //appm_actv_fsm_next(param->actv_idx);

            //appm_update_actv_state(param->actv_idx, APP_CONNECT_STATE_IDLE);
            //appm_actv_fsm_next(param->actv_idx);
        } break;

        case GAPM_RESOLV_ADDR:
        {
            LOG_I("Resolve result 0x%x", param->status);
            if (GAP_ERR_NOT_FOUND == param->status)
            {
                appm_random_ble_addr_solved(false, NULL);
            }
            break;
        }
        case GAPM_SET_WL:
        {
            LOG_I("GAPM_SET_WL result 0x%x", param->status);
            app_ble_set_white_list_complete();
            break;
        }
        case GAPM_SET_RAL:
        {
            LOG_I("GAPM_SET_RAL result 0x%x", param->status);
            app_ble_set_resolving_list_complete(param->status);
            break;
        }
        case GAPM_SET_RPA_TIMEOUT:
        {
            LOG_I("GAPM_SET_RPA_TIMEOUT result 0x%x", param->status);
            break;
        }
        default:
        {
            // Drop the message
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

#if (mHDT_LE_SUPPORT)
/**
 ****************************************************************************************
 * @brief Handles MHDT_LE command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_MHDT_LE).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_task_mhdt_le_cmd_cmp_evt_handler(ke_msg_id_t const msgid,
                                                struct mhdt_le_cmp_evt const *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    BLE_APP_FUNC_ENTER();
    LOG_I("MHDT_LE CMD_CODE: %d status is 0x%x",
          param->cmd_code - MHDT_LE_RD_LOCAL_PROPRIETARY_FEAT_CMD, param->status);
    switch (param->cmd_code)
    {
        case (MHDT_LE_RD_LOCAL_PROPRIETARY_FEAT_CMD):
        {
            // Do nothing
        } break;
        case (MHDT_LE_RD_RM_PROPRIETARY_FEAT_CMD):
        {
            // Do nothing
        } break;
        case (MHDT_LE_MABR_SET_DFT_LABEL_PARAMS_CMD):
        {
            // Do nothing
        } break;
        default:
            break;
    }
    BLE_APP_FUNC_LEAVE();
    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles MHDT_LE indication events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_MHDT_LE).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_task_mhdt_le_ind_handler(ke_msg_id_t const msgid,
                                        void const *p_param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    BLE_APP_FUNC_ENTER();
    struct mhdt_le_remote_feature_ind *param =
        (struct mhdt_le_remote_feature_ind *)p_param;
    LOG_I("MHDT_LE IND_CODE: %d status is 0x%x",
          param->ind_code - MHDT_LE_UNKNOWN_MSG, param->status);
    if (param->status == GAP_ERR_NO_ERROR)
    {
        switch (param->ind_code)
        {
            case (MHDT_LE_REMOTE_FEAT_IND):
            {
                LOG_I("%s Remote %d feature = 0x%08x",  __func__, param->conidx, param->feat);
            }
            break;
            case (MHDT_LE_LOCAL_FEAT_IND):
            {
                struct mhdt_le_local_feature_ind *param =
                    (struct mhdt_le_local_feature_ind *)p_param;
                LOG_I("%s Local feature = 0x%08x",  __func__, param->feat);
                if (param->feat & CO_BIT(MHDT_LE_FEATURE_MABR_SUP))
                {
                    app_task_mhdt_mabr_set_dft_label_params(NULL);
                }
            }
            break;
            case (MHDT_LE_UNKNOWN_MSG):
            // Fall through
            default:
                break;
        }
    }
    return (KE_MSG_CONSUMED);
}
#endif

static int gapc_get_dev_info_req_ind_handler(ke_msg_id_t const msgid,
                                             struct gapc_get_dev_info_req_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    LOG_I("(d%d)%s,req:%d, token:0x%x", param->conidx, __func__, param->req, param->token);
    switch (param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                                 src_id, dest_id,
                                                                 gapc_get_dev_info_cfm,
                                                                 APP_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->status = GAP_ERR_NO_ERROR;
            cfm->token = param->token;
            cfm->conidx = param->conidx;
            int8_t len = appm_get_dev_name(cfm->info.name.value, param->name_offset);
            if (len >= 0)
            {
                cfm->status = GAP_ERR_NO_ERROR;
                cfm->info.name.value_length = len;
            }
            else
            {
                cfm->info.name.value_length = 0;
                cfm->status = ATT_ERR_INVALID_OFFSET;
            }
            // Send message
            ke_msg_send(cfm);
        }
        break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                             src_id, dest_id,
                                                             gapc_get_dev_info_cfm);
            cfm->req = param->req;
            cfm->token = param->token;
            cfm->conidx = param->conidx;
            // Set the device appearance
            cfm->info.appearance = APPEARANCE_VALUE;

            // Send message
            ke_msg_send(cfm);
        }
        break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                             src_id, dest_id,
                                                             gapc_get_dev_info_cfm);
            cfm->req = param->req;
            cfm->token = param->token;
            cfm->conidx = param->conidx;
            // Slave preferred Connection interval Min
            cfm->info.slv_pref_params.con_intv_min = 8;
            // Slave preferred Connection interval Max
            cfm->info.slv_pref_params.con_intv_max = 10;
            // Slave preferred Connection latency
            cfm->info.slv_pref_params.latency  = 0;
            // Slave preferred Link supervision timeout
            cfm->info.slv_pref_params.conn_timeout    = 200;  // 2s (500*10ms)

            // Send message
            ke_msg_send(cfm);
        }
        break;

        default: /* Do Nothing */
            break;
    }


    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(ke_msg_id_t const msgid,
                                             struct gapc_set_dev_info_req_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Set Device configuration
    struct gapc_set_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                     gapc_set_dev_info_cfm);
    // Reject to change parameters
    cfm->status = GAP_ERR_REJECTED;
    cfm->req = param->req;
    cfm->conidx = param->conidx;
    // Send message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int POSSIBLY_UNUSED gapc_peer_att_info_ind_handler(ke_msg_id_t const msgid,
                                                          struct gapc_peer_att_info_ind *param,
                                                          ke_task_id_t const dest_id,
                                                          ke_task_id_t const src_id)
{
    LOG_I("%s req = %d", __func__, param->req - GAPC_DEV_NAME);

    switch (param->req)
    {
        case GAPC_DEV_DB_HASH:
        {
#if BLE_AUDIO_ENABLED
            ble_bdaddr_t remote_addr = {{0}};
            bool succ = app_ble_get_peer_solved_addr(param->conidx, &remote_addr);
            if (succ)
            {
                aob_gattc_cache_save(remote_addr.addr, GATT_CHAR_DB_HASH, &param->info.hash[0]);
            }
#endif
        }
        break;

        case GAPC_GET_RSLV_PRIV_ADDR_ONLY:
        {
            app_ble_set_rpa_only(param->conidx, param->info.rslv_priv_addr_only);
            break;
        }

        case GAPC_DEV_CTL_ADDR_RESOL:
        {
            app_ble_set_resolv_support(param->conidx, param->info.rslv_priv_addr_only);
        }
        break;
        case GAPC_DEV_NAME:
        case GAPC_DEV_APPEARANCE:
        default:
            break;
    }

    return (KE_MSG_CONSUMED);
}

static void POSSIBLY_UNUSED gapc_refresh_remote_dev_feature(uint8_t conidx)
{
    // Send a GAPC_GET_INFO_CMD in order to read the device name characteristic value
    struct gapc_get_info_cmd *p_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                   TASK_GAPC, TASK_APP,
                                                   gapc_get_info_cmd);

    // request peer device name.
    p_cmd->operation = GAPC_GET_PEER_FEATURES;
    p_cmd->conidx    = conidx;

    // send command
    ke_msg_send(p_cmd);
}

static void POSSIBLY_UNUSED gapc_get_remote_addr_resol_supp(uint8_t conidx)
{
    // Send a GAPC_GET_INFO_CMD in order to read the Central Address resolution characteristic value
    struct gapc_get_info_cmd *p_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                   TASK_GAPC, TASK_APP,
                                                   gapc_get_info_cmd);

    // Get if Central Address resolution supported.
    p_cmd->operation = GAPC_GET_ADDR_RESOL_SUPP;
    p_cmd->conidx    = conidx;

    // send command
    ke_msg_send(p_cmd);
}

static void POSSIBLY_UNUSED gapc_get_remote_db_hash(uint8_t conidx)
{
    // Send a GAPC_GET_INFO_CMD in order to read the database hash characteristic value
    struct gapc_get_info_cmd *p_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                   TASK_GAPC, TASK_APP,
                                                   gapc_get_info_cmd);

    // request peer device database hash.
    p_cmd->operation = GAPC_GET_PEER_DB_HASH;
    p_cmd->conidx    = conidx;

    // send command
    ke_msg_send(p_cmd);
}

/**
 * @brief Send a cmd to controller to gen a addr according to rnd_type
 *
 * @param rnd_type @see enum gap_rnd_addr_type
 */
static void POSSIBLY_UNUSED gapm_generate_rand_addr_cmd(uint8_t rnd_type)
{
    struct gapm_gen_rand_addr_cmd *p_cmd = KE_MSG_ALLOC(GAPM_GEN_RAND_ADDR_CMD,
                                                        TASK_GAPM, TASK_APP,
                                                        gapm_gen_rand_addr_cmd);
    p_cmd->operation = GAPM_GEN_RAND_ADDR;
    p_cmd->rnd_type  = rnd_type;

    ke_msg_send(p_cmd);
}

static void POSSIBLY_UNUSED gpac_exchange_data_packet_length(uint8_t conidx)
{
    LOG_I("%s", __func__);
    struct gapc_set_le_pkt_size_cmd *set_le_pakt_size_req = KE_MSG_ALLOC(GAPC_SET_LE_PKT_SIZE_CMD,
                                                                         TASK_GAPC,
                                                                         TASK_APP,
                                                                         gapc_set_le_pkt_size_cmd);

    set_le_pakt_size_req->operation = GAPC_SET_LE_PKT_SIZE;
    set_le_pakt_size_req->tx_octets = APP_MAX_TX_OCTETS;
    set_le_pakt_size_req->tx_time = APP_MAX_TX_TIME;
    set_le_pakt_size_req->conidx  = conidx;
    // Send message
    ke_msg_send(set_le_pakt_size_req);
}

#if (BLE_GATT_CLI)
void gapc_cli_exchange_mtu(uint8_t conidx)
{
    struct gapc_exchange_mtu_req *cmd = KE_MSG_ALLOC(GAPC_EXCHANGE_MTU_CMD,
                                                     TASK_GAPC,
                                                     TASK_APP,
                                                     gapc_exchange_mtu_req);

    cmd->conidx = conidx;
    ke_msg_send(cmd);
}
#endif

static int gapc_peer_features_ind_handler(ke_msg_id_t const msgid,
                                          struct gapc_peer_features_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    LOG_I("Peer dev feature is:");
    DUMP8("0x%02x ", param->features, GAP_LE_FEATS_LEN);
    uint8_t conidx = param->conidx;

    if (param->features[0] & (1 << BLE_FEAT_DATA_PKT_LEN_EXT))
    {
        gpac_exchange_data_packet_length(conidx);
    }

#if (BLE_GATT_CLI)
#ifndef GFPS_ENABLED
    gapc_cli_exchange_mtu(conidx);
#endif
#endif

#if (mHDT_LE_SUPPORT)
    app_task_mhdt_rd_remote_proprietary_feat(conidx);
#endif

    return (KE_MSG_CONSUMED);
}

void app_exchange_remote_feature(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

    LOG_I("connectStatus:%d, isFeatureExchanged:%d",
          pContext->connectStatus,
          pContext->isFeatureExchanged);

    if ((BLE_CONNECTED == pContext->connectStatus) && !pContext->isFeatureExchanged)
    {
        gapc_refresh_remote_dev_feature(conidx);
        pContext->isFeatureExchanged = true;
    }
}

void app_get_remote_addr_resol_supp(uint8_t conidx)
{
    LOG_I("ble get remote addr resolv support");
    gapc_get_remote_addr_resol_supp(conidx);
}

void app_get_rpa_only_char_handler(uint8_t conidx)
{
    // Send a GAPC_GET_INFO_CMD in order to read the device name characteristic value
    struct gapc_get_info_cmd *p_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                   TASK_GAPC, TASK_APP,
                                                   gapc_get_info_cmd);

    // request peer resolvable private addresss only characteristic.
    p_cmd->operation = GAPC_GET_RSLV_PRIV_ADDR_ONLY;
    p_cmd->conidx    = conidx;

    // send command
    ke_msg_send(p_cmd);
}

void appm_ble_get_dev_tx_pwr_range(void)
{
    struct gapm_get_dev_info_cmd *cmd = KE_MSG_ALLOC(GAPM_GET_DEV_INFO_CMD,
                                                     TASK_GAPM,
                                                     TASK_APP,
                                                     gapm_get_dev_info_cmd);
    if (cmd)
    {
        cmd->operation = GAPM_GET_DEV_TX_PWR;
        // Send the message
        ke_msg_send(cmd);
    }
}

void appm_ble_get_adv_txpower_value(void)
{
    struct gapm_get_dev_info_cmd *cmd = KE_MSG_ALLOC(GAPM_GET_DEV_INFO_CMD,
                                                     TASK_GAPM,
                                                     TASK_APP,
                                                     gapm_get_dev_info_cmd);
    if (cmd)
    {
        cmd->operation = GAPM_GET_DEV_ADV_TX_POWER;
        // Send the message
        ke_msg_send(cmd);
    }
}

void appm_ble_set_phy_mode(uint8_t conidx, enum le_phy_value phy_val, enum le_phy_opt phy_opt)
{
    struct gapc_set_phy_cmd *cmd = KE_MSG_ALLOC(GAPC_SET_PHY_CMD,
                                                TASK_GAPC,
                                                TASK_APP,
                                                gapc_set_phy_cmd);

    if (cmd)
    {
        cmd->operation  = GAPC_SET_PHY;
        if (phy_val == PHY_UNDEF_VALUE)
        {
            cmd->tx_phy = PHY_UNDEF_VALUE;
            cmd->rx_phy = PHY_UNDEF_VALUE;
        }
        else
        {
            cmd->tx_phy = 1 << (phy_val - 1);
            cmd->rx_phy = 1 << (phy_val - 1);
        }

        cmd->phy_opt    = phy_opt;

        // Send the message
        ke_msg_send(cmd);
    }
}

void appm_ble_get_phy_mode(uint8_t conidx)
{
    struct gapc_get_info_cmd *cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                 TASK_GAPC,
                                                 TASK_APP,
                                                 gapc_get_info_cmd);
    if (cmd)
    {
        cmd->conidx = conidx;
        cmd->operation = GAPC_GET_PHY;

        // Send the message
        ke_msg_send(cmd);
    }
}

void app_read_rpa_addr(void)
{
#ifdef BLE_ADV_RPA_ENABLED
    ble_bdaddr_t  peer_identify_address;

    peer_identify_address.addr_type = GAPM_STATIC_ADDR;
    memcpy(peer_identify_address.addr, (uint8_t *)bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
    gapm_read_rpa_addr_cmd(&peer_identify_address);
#endif
}

static int gapc_bt_connection_req_ind_handler(ke_msg_id_t const msgid,
                                              struct gapc_bt_connection_req_ind const *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_le_connection_req_ind_handler(ke_msg_id_t const msgid,
                                              struct gapc_le_connection_req_ind const *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    // Check if the received Connection Handle was valid
    ASSERT(conidx != GAP_INVALID_CONIDX, "%s invalid conidx 0x%x", __func__, conidx);

    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

    pContext->connectStatus = BLE_CONNECTED;

    ke_state_set(dest_id, APPM_CONNECTED);

    app_env.conn_cnt++;

    if (app_is_resolving_ble_bd_addr())
    {
        LOG_I("A ongoing ble addr solving is in progress, refuse the new connection.");
        app_ble_start_disconnect(conidx);
        return KE_MSG_CONSUMED;
    }

    BLE_APP_FUNC_ENTER();

    pContext->peerAddrType = param->peer_addr_type;
    memcpy(pContext->bdAddr, param->peer_addr.addr, BD_ADDR_LEN);

    // Retrieve the connection info from the parameters
    pContext->conhdl = param->conhdl;

    if (BLE_RANDOM_ADDR == pContext->peerAddrType)
    {
        pContext->isGotSolvedBdAddr = false;
    }
    else
    {
        pContext->isGotSolvedBdAddr = true;
        memcpy(pContext->solvedBdAddr, param->peer_addr.addr, BD_ADDR_LEN);
    }

    LOG_I("[BLE CONNECTED]device info:");
    LOG_I("peer addr:");
    DUMP8("%02x ", param->peer_addr.addr, BT_ADDR_OUTPUT_PRINT_NUM);
    LOG_I("peer addr type:%d", param->peer_addr_type);
    LOG_I("connection index:%d, isGotSolvedBdAddr:%d", conidx, pContext->isGotSolvedBdAddr);
    LOG_I("conn interval:%d, timeout:%d", param->con_interval, param->sup_to);

    app_env.context[conidx].connParam.con_interval = param->con_interval;
    app_env.context[conidx].connParam.con_latency = param->con_latency;
    app_env.context[conidx].connParam.sup_to = param->sup_to;
    app_env.context[conidx].addr_resolv_supp = 1;

    if (app_env.g_ble_connect_req_callback)
    {
        ble_bdaddr_t peer_bd_addr;
        memcpy(peer_bd_addr.addr, param->peer_addr.addr, BD_ADDR_LEN);
        peer_bd_addr.addr_type = param->peer_addr_type;
        app_env.g_ble_connect_req_callback(conidx, &peer_bd_addr);
    }

#if (BLE_APP_SEC)
    app_sec_reset_env_on_connection();
#endif

    // Send connection confirmation
    struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
                                                   TASK_GAPC, TASK_APP,
                                                   gapc_connection_cfm);


#if(BLE_APP_SEC)
    cfm->bond_data.pairing_lvl      = app_sec_get_bond_status() ? BLE_AUTHENTICATION_LEVEL : GAP_AUTH_REQ_NO_MITM_NO_BOND;
#else // !(BLE_APP_SEC)
    cfm->bond_data.pairing_lvl      = GAP_AUTH_REQ_NO_MITM_NO_BOND;
#endif // (BLE_APP_SEC)
    SETB(cfm->bond_data.cli_feat, GAPC_CLI_ROBUST_CACHE_EN, 1);
#if BLE_AUDIO_ENABLED
    /// Check sec key present
    uint8_t ltk[BLE_LTK_SIZE] = {0};
    uint8_t ret = nv_record_ble_record_find_ltk(pContext->bdAddr, ltk, 0);
    cfm->bond_data.enc_key_present = ret;
    /// gapc dond data
    SETB(cfm->bond_data.pairing_lvl, GAP_PAIRING_BOND_PRESENT, ret);
    /// Check gatt cache
    GATTC_SRV_ATTR_t *gatt_srv_valid_rec = aob_gattc_find_valid_cache(pContext->bdAddr, false);
    if (ret == true && gatt_srv_valid_rec != NULL)
    {
        cfm->bond_data.gatt_start_hdl = gatt_srv_valid_rec->gapc_cache_bond_data.gatt_start_hdl;
        cfm->bond_data.gatt_end_hdl = gatt_srv_valid_rec->gapc_cache_bond_data.gatt_end_hdl;
        cfm->bond_data.srv_feat = gatt_srv_valid_rec->gapc_cache_bond_data.srv_feat;
        cfm->bond_data.svc_chg_hdl = gatt_srv_valid_rec->gapc_cache_bond_data.svc_chg_hdl;
        /// check db update since last connection lost
        if (gatt_user_nb_get(GATT_ROLE_SERVER) != gatt_srv_valid_rec->gapc_cache_bond_data.srv_nb)
        {
            SETB(cfm->bond_data.cli_info, GAPC_CLI_DB_UPDATED, 1);
        }
        cfm->bond_data.cli_info = gatt_srv_valid_rec->gapc_cache_bond_data.cli_info;
    }
#endif

    cfm->conidx = conidx;
    // Send the message
    ke_msg_send(cfm);

    // We are now in connected State
    ke_state_set(dest_id, APPM_CONNECTED);
    app_exchange_remote_feature(conidx);
    app_get_remote_addr_resol_supp(conidx);

    ble_bdaddr_t ble_peer_addr;
    memcpy(ble_peer_addr.addr, param->peer_addr.addr, BD_ADDR_LEN);
    ble_peer_addr.addr_type = param->peer_addr_type;
    app_ble_connected_evt_handler(conidx, &ble_peer_addr);

#if (BLE_APP_TILE)
    app_tile_connected_evt_handler(conidx, param->con_interval, param->con_latency, param->sup_to);
#endif

#ifndef BLE_AUDIO_ENABLED
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_DEFAULT, true);
#endif

    //to read the rsp address
#ifdef BLE_ADV_RPA_ENABLED
    ble_bdaddr_t  peer_identify_address;

    peer_identify_address.addr_type = GAPM_STATIC_ADDR;
    memcpy(peer_identify_address.addr, (uint8_t *)bt_get_ble_local_address(), GAP_BD_ADDR_LEN);
    gapm_read_rpa_addr_cmd(&peer_identify_address);
#endif

    BLE_APP_FUNC_LEAVE();

#if (BLE_BUDS)
    buds_register_cli_event(conidx);
#endif

    app_get_rpa_only_char_handler(conidx);
    app_ble_reset_conn_param_mode(conidx);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;

    LOG_I("%s conidx 0x%02x op 0x%02x", __func__, conidx, param->operation);
    switch (param->operation)
    {
        case (GAPC_UPDATE_PARAMS):
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                app_ble_update_param_failed(conidx, param->status);
                //                appm_disconnect();
            }
        }
        break;
        case (GAPC_GET_LOC_TX_PWR_LEVEL_1M):
        case (GAPC_GET_LOC_TX_PWR_LEVEL_2M):
        case (GAPC_GET_LOC_TX_PWR_LEVEL_LE_CODED_S8):
        case (GAPC_GET_LOC_TX_PWR_LEVEL_LE_CODED_S2):
        {
            LOG_I("%s, get loc power conidx:%d, status:%x", __func__, param->conidx, param->status);
        }
        break;
        case (GAPC_GET_PEER_TX_PWR_LEVEL_1M):
        case (GAPC_GET_PEER_TX_PWR_LEVEL_2M):
        case (GAPC_GET_PEER_TX_PWR_LEVEL_LE_CODED_S8):
        case (GAPC_GET_PEER_TX_PWR_LEVEL_LE_CODED_S2):
        {
            LOG_I("%s, get peer power conidx:%d, status:%x", __func__, param->conidx, param->status);
        }
        break;
        case (GAPC_TX_PWR_REPORT_CTRL):
        {
            LOG_I("%s, power report ctrl conidx:%d, status:%x", __func__, param->conidx, param->status);
        }
        break;
        case (GAPC_PATH_LOSS_REPORT_CTRL):
        {
            LOG_I("%s, path loss conidx:%d, status:%x", __func__, param->conidx, param->status);
        }
        break;
        case (GAPC_SET_DEFAULT_SUBRATE):
        {
            LOG_I("%s, default subrate conidx:%d, status:%x", __func__, param->conidx, param->status);
        }
        break;
        case (GAPC_SUBRATE_REQUEST):
        {
            LOG_I("%s, subrate req conidx:%d, status:%x", __func__, param->conidx, param->status);
        }
        break;
        default:
        {
        } break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                       struct gapc_disconnect_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    LOG_I("[BLE DISCONNECTED] device info:");
    uint8_t conidx = param->conidx;
    LOG_I("connection index:%d, reason:0x%x", conidx, param->reason);

    if (CO_ERROR_TERMINATED_MIC_FAILURE == param->reason)
    {
        ble_bdaddr_t remote_addr = {{0}};
        app_ble_get_peer_solved_addr(conidx, &remote_addr);
        TRACE(1, "Delete security info!!! device:");
        DUMP8("%02x ", remote_addr.addr, 6);
        nv_record_ble_delete_entry(remote_addr.addr);
#if BLE_AUDIO_ENABLED
        aob_gattc_delete_nv_cache(remote_addr.addr, 0);
#endif

        ble_gap_ral_dev_info_t devicesInfo[RESOLVING_LIST_MAX_NUM];
        uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
        appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);
    }

    app_env.context[conidx].isBdAddrResolvingInProgress = false;
    app_env.context[conidx].isGotSolvedBdAddr = false;

    app_env.context[conidx].connectStatus = BLE_DISCONNECTED;
    app_env.context[conidx].isFeatureExchanged = false;
    memset(&app_env.context[conidx].connParam, 0x0, sizeof(APP_BLE_CONN_PARAM_T));
    app_env.context[conidx].supportRpaOnly = 0;
    //l2cm_buffer_reset(conidx);

    // Go to the ready state
    ke_state_set(TASK_APP, APPM_READY);

    app_env.conn_cnt--;

#if (BLE_DATAPATH_SERVER)
    app_datapath_server_disconnected_evt_handler(conidx);
#endif

#if (BLE_AHP_SERVER)
    app_ahp_server_disconnected_evt_handler(conidx);
#endif

#if (BLE_DATAPATH_CLIENT)
    app_datapath_client_disconnected_evt_handler(conidx);
#endif

#if (BLE_OTA)
    app_ota_disconnected_evt_handler(conidx);
#endif

#if (BLE_APP_TOTA)
    app_tota_disconnected_evt_handler(conidx);
#endif

#if(BLE_APP_GFPS)
    app_gfps_disconnected_evt_handler(conidx);
#endif

#if (BLE_SAS_SERVER)
    app_sas_server_disconnected_evt_handler(conidx);
#endif

#if (BLE_AI_VOICE)
    app_ai_ble_disconnected_evt_handler(conidx);
#endif

#if (BLE_APP_TILE)
    app_tile_disconnected_evt_handler(conidx);
#endif

    app_ble_disconnected_evt_handler(conidx, param->reason);

    app_ble_reset_conn_param_mode(conidx);
    return (KE_MSG_CONSUMED);
}

static int gapc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_mtu_changed_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    LOG_I("MTU has been negotiated as %d conidx %d", param->mtu, conidx);

#if (BLE_APP_DATAPATH_SERVER)
    app_datapath_server_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_OTA)
    app_ota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_APP_TOTA)
    app_tota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_AI_VOICE)
    app_ai_ble_mtu_exchanged_handler(conidx, param->mtu);
#endif

    if (app_env.ble_link_mtu_exch_ind_callback)
    {
        app_env.ble_link_mtu_exch_ind_callback(conidx, param->mtu);
    }

    return (KE_MSG_CONSUMED);
}

static void gapc_get_tx_pwr_ind_handler(ble_tx_object_e object, void *param)
{
    ble_event_t event;
    struct gapc_loc_tx_pwr_ind *param_p = param;

    event.evt_type = BLE_GET_TX_PWR_LEVEL;
    event.p.read_tx_power_handled.conidx     = param_p->conidx;
    event.p.read_tx_power_handled.object     = object;
    event.p.read_tx_power_handled.phy        = param_p->phy;
    event.p.read_tx_power_handled.tx_pwr     = param_p->tx_pwr;
    event.p.read_tx_power_handled.data       = param_p->max_tx_pwr;

    LOG_I("%s[%d]: %d, %d, %d, %d, %d", __FUNCTION__, __LINE__, param_p->conidx, object,
          param_p->phy, param_p->tx_pwr, param_p->max_tx_pwr);

    app_ble_core_global_handle(&event, NULL);
}

static int gapc_get_local_tx_pwr_ind_handler(ke_msg_id_t const msgid,
                                             struct gapc_loc_tx_pwr_ind *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    gapc_get_tx_pwr_ind_handler(BLE_TX_LOCAL, param);
    return KE_MSG_CONSUMED;
}

static int gapc_get_peer_tx_pwr_ind_handler(ke_msg_id_t const msgid,
                                            struct gapc_peer_tx_pwr_ind *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    gapc_get_tx_pwr_ind_handler(BLE_TX_REMOTE, param);
    return KE_MSG_CONSUMED;
}

static void gapc_tx_pwr_repot_ind_handler(ble_tx_object_e object, void *param)
{
    ble_event_t event;
    struct gapc_loc_tx_pwr_report_ind *param_p = param;

    event.evt_type = BLE_TX_PWR_REPORT_EVENT;
    event.p.tx_power_change_reporting_handled.conidx = param_p->conidx;
    event.p.tx_power_change_reporting_handled.object = object;
    event.p.tx_power_change_reporting_handled.phy    = param_p->phy;
    event.p.tx_power_change_reporting_handled.tx_pwr = param_p->tx_pwr;
    event.p.tx_power_change_reporting_handled.flags  = param_p->flags;
    event.p.tx_power_change_reporting_handled.delta  = param_p->delta;

    LOG_I("%s[%d]:%d, %d, %d, %d, %d, %d", __FUNCTION__, __LINE__, param_p->conidx, object,
          param_p->phy, param_p->tx_pwr, param_p->flags, param_p->delta);
    app_ble_core_global_handle(&event, NULL);
}

static int gapc_local_tx_pwr_repot_ind_handler(ke_msg_id_t const msgid,
                                               struct gapc_loc_tx_pwr_report_ind *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    gapc_tx_pwr_repot_ind_handler(BLE_TX_LOCAL, param);
    return KE_MSG_CONSUMED;
}

static int gapc_peer_tx_pwr_repot_ind_handler(ke_msg_id_t const msgid,
                                              struct gapc_loc_tx_pwr_report_ind *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    gapc_tx_pwr_repot_ind_handler(BLE_TX_REMOTE, param);
    return KE_MSG_CONSUMED;
}

static int gapc_phy_changed_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_le_phy_ind *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    LOG_I("%s tx phy: %d, rx phy: %d", __func__, param->tx_phy, param->rx_phy);
    return KE_MSG_CONSUMED;
}

static int gapc_subrate_change_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_subrate_change_ind *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    ble_event_t event;

    event.evt_type = BLE_SUBRATE_CHANGE_EVENT;
    event.p.subrate_change_handled.conidx      = param->conidx;
    event.p.subrate_change_handled.status      = param->status;
    event.p.subrate_change_handled.sub_factor  = param->sub_factor;
    event.p.subrate_change_handled.per_latency = param->per_latency;
    event.p.subrate_change_handled.cont_num    = param->cont_num;
    event.p.subrate_change_handled.timeout     = param->timeout;

    LOG_I("%s:%d, %d, %d, %d, %d", __FUNCTION__, param->conidx, param->sub_factor, param->per_latency,
          param->cont_num, param->timeout);
    app_ble_core_global_handle(&event, NULL);
    return KE_MSG_CONSUMED;
}

static int gapc_peer_path_loss_report_handler(ke_msg_id_t const msgid,
                                              struct gapc_path_loss_threshold_ind *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    ble_event_t event;

    LOG_I("%s:%d, %d, %d", __FUNCTION__, param->conidx, param->curr_path_loss, param->zone_entered);
    event.evt_type = BLE_PATH_LOSS_REPORT_EVENT;
    event.p.path_loss_report_handled.conidx = param->conidx;
    event.p.path_loss_report_handled.curr_path_loss = param->curr_path_loss;
    event.p.path_loss_report_handled.zone_entered = param->zone_entered;
    app_ble_core_global_handle(&event, NULL);
    return KE_MSG_CONSUMED;
}


static int gapm_profile_added_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    LOG_I("Profile prf_task_id %d is added.", param->prf_task_id);

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(ke_msg_id_t const msgid,
                            void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    ke_task_id_t src_task_id = MSG_T(msgid);
    // Message policy
    uint8_t msg_pol = KE_MSG_CONSUMED;

    switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
#if (BLE_APP_SEC)
            if ((msgid >= GAPC_BOND_CMD) &&
                    (msgid < GAPC_BOND_DATA_UPDATE_IND))
            {
                // Call the Security Module
                msg_pol = app_get_handler(&app_sec_handlers, msgid, param, src_id);
            }
#endif //(BLE_APP_SEC)
            // else drop the message
        }
        break;

#if BLE_AUDIO_ENABLED
        case (TASK_ID_GAF):
        {
            msg_pol = app_get_handler(&app_gaf_handlers, msgid, param, src_id);
        }
        break;
#endif

        case (TASK_ID_GATT):
        {
            msg_pol = app_get_handler(&app_gatt_handler, msgid, param, src_id);
        }
        break;

#if (BLE_APP_HT)
        case (TASK_ID_HTPT):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
        case (TASK_ID_DISS):
        {
            // Call the Device Information Module
            msg_pol = app_get_handler(&app_dis_handlers, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
        case (TASK_ID_HOGPD):
        {
            // Call the HID Module
            msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_HID)

#if (BLE_APP_BATT)
        case (TASK_ID_BASS):
        {
            // Call the Battery Module
            msg_pol = app_get_handler(&app_batt_handlers, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_BATT)

#if (BLE_APP_AM0)
        case (TASK_ID_AM0):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_handlers, msgid, param, src_id);
        }
        break;

        case (TASK_ID_AM0_HAS):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_has_handlers, msgid, param, src_id);
        }
        break;
#endif // defined(BLE_APP_AM0)

#if (BLE_APP_HR)
        case (TASK_ID_HRPS):
        {
            // Call the HRPS Module
            msg_pol = app_get_handler(&app_hrps_table_handler, msgid, param, src_id);
        }
        break;
#endif

#if (BLE_APP_DATAPATH_SERVER)
        case (TASK_ID_DATAPATHPS):
        {
            // Call the Data Path Module
            msg_pol = app_get_handler(&app_datapath_server_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_SAS_SERVER)
        case (TASK_ID_SAS):
        {
            // Call the Switching Ambient Service Module
            msg_pol = app_get_handler(&app_sas_server_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_SAS_SERVER)

#if (BLE_APP_AHP_SERVER)
        case (TASK_ID_AHPS):
        {
            // Call the AHP server MODULE
            msg_pol = app_get_handler(&app_ahp_server_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_AHP_SERVER)

#if (CFG_APP_DATAPATH_CLIENT)
        case (TASK_ID_DATAPATHPC):
        {
            // Call the Data Path Module
            msg_pol = app_get_handler(&app_datapath_client_table_handler, msgid, param, src_id);
        }
        break;
#endif //(CFG_APP_DATAPATH_CLIENT)


#if (BLE_APP_TILE)
        case (TASK_ID_TILE):
        {
            // Call the TILE Module
            msg_pol = app_get_handler(&app_tile_table_handler, msgid, param, src_id);
        }
        break;
#endif

#if (BLE_APP_AI_VOICE)
        case (TASK_ID_AI):
        {
            // Call the AI Voice
            msg_pol = app_get_handler(&app_ai_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_AI_VOICE)

#if (BLE_APP_OTA)
        case (TASK_ID_OTA):
        {
            // Call the OTA
            msg_pol = app_get_handler(&app_ota_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
        case (TASK_ID_TOTA):
        {
            // Call the TOTA
            msg_pol = app_get_handler(&app_tota_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_TOTA)

#if (BLE_APP_ANCC)
        case (TASK_ID_ANCC):
        {
            // Call the ANCC
            msg_pol = app_get_handler(&app_ancc_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_ANCC)

#if (BLE_APP_AMSC)
        case (TASK_ID_AMSC):
        {
            // Call the AMS
            msg_pol = app_get_handler(&app_amsc_table_handler, msgid, param, src_id);
        }
        break;
#endif //(BLE_APP_AMS)
#if (BLE_APP_GFPS)
        case (TASK_ID_GFPSP):
        {
            msg_pol = app_get_handler(&app_gfps_table_handler, msgid, param, src_id);
        }
        break;
#endif
        default:
        {
#if (BLE_APP_HT)
            if (msgid == APP_HT_MEAS_INTV_TIMER)
            {
                msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
            }
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
            if (msgid == APP_HID_MOUSE_TIMEOUT_TIMER)
            {
                msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
            }
#endif //(BLE_APP_HID)
        }
        break;
    }

    return (msg_pol);
}

static int gapm_adv_report_ind_handler(ke_msg_id_t const msgid,
                                       struct gapm_ext_adv_report_ind *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    app_adv_reported_scanned(param);
    return KE_MSG_CONSUMED;
}

static int gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
                                        struct gapm_addr_solved_ind *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    /// Indicate that resolvable random address has been solved
    appm_random_ble_addr_solved(true, param->irk.key);
    return KE_MSG_CONSUMED;
}

// TODO: update to use new API
#if 0
__STATIC int gattc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                           struct gattc_mtu_changed_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    LOG_I("MTU has been negotiated as %d conidx %d", param->mtu, conidx);

#if (BLE_APP_DATAPATH_SERVER)
    app_datapath_server_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_OTA)
    app_ota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_APP_TOTA)
    app_tota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_AI_VOICE)
    app_ai_mtu_exchanged_handler(conidx, param->mtu);
#endif

    return (KE_MSG_CONSUMED);
}
#endif

#define APP_CONN_PARAM_INTERVEL_MAX    (30)
__STATIC int gapc_conn_param_update_req_ind_handler(ke_msg_id_t const msgid,
                                                    struct gapc_param_update_req_ind const *param,
                                                    ke_task_id_t const dest_id,
                                                    ke_task_id_t const src_id)
{
    bool accept = true;
    ble_event_t event;

    LOG_I("Receive the conn param update request: min %d max %d latency %d timeout %d",
          param->intv_min,
          param->intv_max,
          param->latency,
          param->time_out);

    struct gapc_param_update_cfm *cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM, src_id, dest_id,
                                                     gapc_param_update_cfm);

    event.evt_type = BLE_CONN_PARAM_UPDATE_REQ_EVENT;
    event.p.conn_param_update_req_handled.conidx = param->conidx;
    event.p.conn_param_update_req_handled.intv_min = param->intv_min;
    event.p.conn_param_update_req_handled.intv_max = param->intv_max;
    event.p.conn_param_update_req_handled.latency  = param->latency;
    event.p.conn_param_update_req_handled.time_out = param->time_out;

    app_ble_core_global_handle(&event, &accept);
    LOG_I("%s ret %d ", __func__, accept);

    cfm->accept = true;
    cfm->conidx = param->conidx;

#ifdef GFPS_ENABLED
    //make sure ble cnt interval isnot less than 15ms, in order to prevent bt collision
    if (param->intv_min < (uint16_t)(15 / 1.25))
    {
        LOG_I("accept");
        fp_update_ble_connect_param_start(param->conidx);
    }
    else
    {
        fp_update_ble_connect_param_stop(param->conidx);
    }
#else

    if (!accept)
    {
        // when a2dp or sco streaming is on-going
        //make sure ble cnt interval isnot less than 15ms, in order to prevent bt collision
        if (param->intv_min <= (uint16_t)(15 / 1.25))
        {
            cfm->accept = false;
        }
    }
#endif

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int gapc_conn_param_updated_handler(ke_msg_id_t const msgid,
                                           struct gapc_param_updated_ind *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    LOG_I("Conidx %d BLE conn parameter is updated as interval %d timeout %d",
          conidx, param->con_interval, param->sup_to);

#if BLE_APP_TILE
    app_tile_ble_conn_parameter_updated(conidx, param->con_interval, param->con_latency, param->sup_to);
#endif

    app_env.context[conidx].connParam.con_interval = param->con_interval;
    app_env.context[conidx].connParam.con_latency = param->con_latency;
    app_env.context[conidx].connParam.sup_to = param->sup_to;
    app_ble_update_param_successful(conidx, &app_env.context[conidx].connParam);
    return (KE_MSG_CONSUMED);
}

static int gapm_dev_addr_ind_handler(ke_msg_id_t const msgid,
                                     struct gapm_dev_bdaddr_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Indicate that a new random BD address set in lower layers
    LOG_I("New dev addr:");
    DUMP8("%02x ", param->addr.addr, BT_ADDR_OUTPUT_PRINT_NUM);

#if defined (BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT)
    if (GAPM_GEN_RSLV_ADDR == param->addr.addr_type)
    {
        memcpy(&ble_rnd_addr, &param->addr, sizeof(ble_bdaddr_t));
    }
#endif

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of random number generated message
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_gen_rand_nb_ind_handler(ke_msg_id_t const msgid, struct gapm_gen_rand_nb_ind *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

    return KE_MSG_CONSUMED;
}

static int gapc_bond_data_update_ind_handler(ke_msg_id_t const msgid,
                                             struct gapc_bond_data_update_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    ble_bdaddr_t remote_addr = {{0}};
    app_ble_get_peer_solved_addr(param->conidx, &remote_addr);
    TRACE(1, "%s", __func__);
    DUMP8("%02x ", remote_addr.addr, 6);
#if BLE_AUDIO_ENABLED
    aob_gattc_cache_save(remote_addr.addr, GATT_SVC_GENERIC_ATTRIBUTE, (void *)&param->data);
#endif
    return KE_MSG_CONSUMED;
}

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(appm)
{
    /// please add in order of gapm_msg size
    // GAPM messages
    {GAPM_CMP_EVT, (ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPM_DEV_BDADDR_IND, (ke_msg_func_t)gapm_dev_addr_ind_handler},
    {GAPM_ADDR_SOLVED_IND, (ke_msg_func_t)gapm_addr_solved_ind_handler},
    {GAPM_GEN_RAND_NB_IND, (ke_msg_func_t)gapm_gen_rand_nb_ind_handler},
    {GAPM_ACTIVITY_CREATED_IND, (ke_msg_func_t)gapm_activity_created_ind_handler},
    {GAPM_ACTIVITY_STOPPED_IND, (ke_msg_func_t)gapm_activity_stopped_ind_handler},
    {GAPM_EXT_ADV_REPORT_IND, (ke_msg_func_t)gapm_adv_report_ind_handler},
    {GAPM_PROFILE_ADDED_IND, (ke_msg_func_t)gapm_profile_added_ind_handler},

    /// please add in order of gapc_msg size
    // GAPC messages
    {GAPC_CMP_EVT, (ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_LE_CONNECTION_REQ_IND, (ke_msg_func_t)gapc_le_connection_req_ind_handler},
    {GAPC_DISCONNECT_IND, (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GAPC_BT_CONNECTION_REQ_IND, (ke_msg_func_t)gapc_bt_connection_req_ind_handler},
    {GAPC_MTU_CHANGED_IND, (ke_msg_func_t)gapc_mtu_changed_ind_handler},
    {GAPC_PEER_ATT_INFO_IND, (ke_msg_func_t)gapc_peer_att_info_ind_handler},
    {GAPC_PEER_FEATURES_IND, (ke_msg_func_t)gapc_peer_features_ind_handler},
    {GAPC_GET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_set_dev_info_req_ind_handler},

    {GAPC_PARAM_UPDATE_REQ_IND, (ke_msg_func_t)gapc_conn_param_update_req_ind_handler},
    {GAPC_PARAM_UPDATED_IND, (ke_msg_func_t)gapc_conn_param_updated_handler},
    {GAPC_LE_PHY_IND, (ke_msg_func_t)gapc_phy_changed_ind_handler},

    {GAPC_BOND_DATA_UPDATE_IND, (ke_msg_func_t)gapc_bond_data_update_ind_handler},

    {GAPC_LOC_TX_PWR_IND, (ke_msg_func_t)gapc_get_local_tx_pwr_ind_handler},
    {GAPC_PEER_TX_PWR_IND, (ke_msg_func_t)gapc_get_peer_tx_pwr_ind_handler},
    {GAPC_LOC_TX_PWR_REPORT_IND, (ke_msg_func_t)gapc_local_tx_pwr_repot_ind_handler},
    {GAPC_PEER_TX_PWR_REPORT_IND, (ke_msg_func_t)gapc_peer_tx_pwr_repot_ind_handler},
    {GAPC_PATH_LOSS_THRESHOLD_IND, (ke_msg_func_t)gapc_peer_path_loss_report_handler},
    {GAPC_SUBRATE_CHANGE_IND, (ke_msg_func_t)gapc_subrate_change_ind_handler},

#if (mHDT_LE_SUPPORT)
    {MHDT_LE_CMP_EVT, (ke_msg_func_t)app_task_mhdt_le_cmd_cmp_evt_handler},
    {MHDT_LE_IND, (ke_msg_func_t)app_task_mhdt_le_ind_handler},
#endif

    {KE_MSG_DEFAULT_HANDLER, (ke_msg_func_t)appm_msg_handler},
};

/* Defines the place holder for the states of all the task instances. */
ke_state_t appm_state[APP_IDX_MAX];

// Application task descriptor
const struct ke_task_desc TASK_DESC_APP = {appm_msg_handler_tab, appm_state, APP_IDX_MAX, ARRAY_LEN(appm_msg_handler_tab)};

#endif //(BLE_APP_PRESENT)

/// @} APPTASK

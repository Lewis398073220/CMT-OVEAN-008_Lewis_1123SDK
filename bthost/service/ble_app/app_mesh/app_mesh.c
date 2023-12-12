/**
 ****************************************************************************************
 *
 * @file app_mesh.c
 *
 * @brief Mesh Application
 *
 * Copyright (C) RivieraWaves 2019-2019
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP_MESH
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_MESH)

#include "app_mesh.h"                // Mesh Application Definitions
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include <string.h>
#include "co_utils.h"
#include "gapm_task.h"
#include "ke_mem.h"                  // Kernel Memory Definitions
#include "m_defines.h"               // Mesh Profile Definitions
#include "mm_defines.h"              // Mesh Model Definitions

#if (DISPLAY_SUPPORT)
#include "app_display.h"
#endif //(DISPLAY_SUPPORT)

/*
 * DEFINES
 ****************************************************************************************
 */

/// Default Device UUID
#define APP_MESH_DFLT_DEVICE_UUID     ("\xFA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA")
/// Default choice of using storage
#define APP_MESH_USE_STORAGE_DFLT     (1)
/// Default group address
#define APP_MESH_GRP_ADDR_DFLT        (0xC000)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Mesh application state values
enum app_mesh_state
{
    /// Mesh Initialization
    APP_MESH_STATE_INIT = 0,
    /// Registering Server models
    APP_MESH_STATE_REGISTERING_SRV,
    /// Registering Generic OnOff Client model
    APP_MESH_STATE_REGISTERING_GENC_ONOFF,
    /// Registering Light Lightness Client model
    APP_MESH_STATE_REGISTERING_LIGHTC_LN,
    /// Configuring
    APP_MESH_STATE_CONFIGURING,
    /// Enabling
    APP_MESH_STATE_ENABLING,
    /// Waiting for provisioning
    APP_MESH_STATE_WAIT_PROV,
    /// Provisioned
    APP_MESH_STATE_PROV,
};

/// Mesh NVDS tag
enum app_mesh_nvds_tag
{
    /// The value to check if use storage (1: use, 0: not use)
    NVDS_TAG_APP_MESH_USE_STORAGE = NVDS_TAG_APP_MESH,
    /// Group address
    NVDS_TAG_APP_MESH_GRP_ADDR,
    /// Device UUID
    NVDS_TAG_APP_MESH_DEVICE_UUID,

    /// Parameter length
    NVDS_LEN_APP_MESH_USE_STORAGE = 1,
    NVDS_LEN_APP_MESH_GRP_ADDR = 2,
    NVDS_LEN_APP_MESH_DEVICE_UUID = 16,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Mesh application environment structure
typedef struct app_mesh_env
{
    /* ******************************** */
    /* *       MESH DEMO BASIC        * */
    /* ******************************** */
    /// Mesh application state
    uint8_t state;
    /// Mesh device UUID
    uint8_t dev_uuid[NVDS_LEN_APP_MESH_DEVICE_UUID];
    /// The value to record if use storage (1: use, 0: not use)
    uint8_t use_storage;
    /// Mesh profile task number
    uint16_t prf_task;
    /// Mesh demo type
    uint8_t mesh_demo_type;

    /* ******************************** */
    /* *        GENERIC SERVER        * */
    /* ******************************** */
    /// Local index for Generic Server OnOff Server model
    m_lid_t gens_oo_lid;

    /* ******************************** */
    /* *        GENERIC CLIENT        * */
    /* ******************************** */
    /// Local index for Generic Client OnOff and Lightness Cient OnOff model
    m_lid_t genc_oo_lid;
    /// Destination address for Generic Client and Lightness Client
    uint16_t dest_addr;
    /// Group application key local index for Generic Client and Lightness Client
    m_lid_t grp_app_key_lid;
    /// TID for Generic Client and Lightness Client
    uint8_t tid;

    /* ******************************** */
    /* *       LIGHTNESS SERVER       * */
    /* ******************************** */
    /// Local index for Light Lightness Server model
    m_lid_t lights_lid;

    /* ******************************** */
    /* *       LIGHTNESS CLIENT       * */
    /* ******************************** */
    /// Local index for Light Lightness Client model
    m_lid_t lightc_lid;
} app_mesh_env_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Mesh application environment structure
__STATIC app_mesh_env_t* p_env;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Register mesh models needed for the demonstration
 ****************************************************************************************
 */
__STATIC void app_mesh_register_model(void)
{
    // Check if server or client models need to be supported
    if ((p_env->mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
            || (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN))
    {
        mm_api_register_server_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_MDL_API_CMD,
                                                           p_env->prf_task, TASK_APP,
                                                           mm_api_register_server_cmd);

        // Fill the allocated message
        p_cmd->cmd_code = MM_API_REGISTER_SERVER;
        p_cmd->elmt_idx = 0;
        p_cmd->info = 0x01;

        if (p_env->mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
        {
            // Generic OnOff Server
            p_cmd->mdl_cfg_idx = MM_CFG_IDX_GENS_ONOFF;
        }
        else // if (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN)
        {
            // Lighting Server
            p_cmd->mdl_cfg_idx = MM_CFG_IDX_LIGHTS_LN;
        }

        // Send the message
        ke_msg_send(p_cmd);

        // Update state
        p_env->state = APP_MESH_STATE_REGISTERING_SRV;
    }
    else if ((p_env->mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
                || (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN))
    {
        mm_api_register_client_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_MDL_API_CMD,
                                                           p_env->prf_task, TASK_APP,
                                                           mm_api_register_client_cmd);

        if (p_env->state == APP_MESH_STATE_INIT)
        {
            p_cmd->cmd_code = MM_API_REGISTER_CLIENT;
            p_cmd->cmdl_idx = MM_CMDL_IDX_GENC_ONOFF;

            // Update state
            p_env->state = APP_MESH_STATE_REGISTERING_GENC_ONOFF;
        }
        else if (p_env->state == APP_MESH_STATE_REGISTERING_GENC_ONOFF)
        {
            p_cmd->cmd_code = MM_API_REGISTER_CLIENT;
            p_cmd->cmdl_idx = MM_CMDL_IDX_LIGHTC_LN;

            // Update state
            p_env->state = APP_MESH_STATE_REGISTERING_LIGHTC_LN;
        }

        ke_msg_send(p_cmd);
    }
}

#if (NVDS_SUPPORT)
/**
 ****************************************************************************************
 * @brief Trigger loading of information stored in NVDS
 ****************************************************************************************
 */
__STATIC void app_mesh_storage_load(void)
{
    // Send a load stored information command
    m_api_storage_load_cmd_t* p_cmd = KE_MSG_ALLOC_DYN(MESH_API_CMD,
                                                       p_env->prf_task, TASK_APP,
                                                       m_api_storage_load_cmd, 0);

    p_cmd->cmd_code = M_API_STORAGE_LOAD;
    p_cmd->length = 0;

    ke_msg_send(p_cmd);
}
#endif //(NVDS_SUPPORT)

/**
 ****************************************************************************************
 * @brief Enable the Mesh task
 ****************************************************************************************
 */
__STATIC void app_mesh_enable_task(void)
{
    // Allocate default command message
    m_api_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_API_CMD,
                                      p_env->prf_task, TASK_APP,
                                      m_api_cmd);

    // Enable Mesh profile
    p_cmd->cmd_code = M_API_ENABLE;

    // Send the command
    ke_msg_send(p_cmd);
}

/**
 ****************************************************************************************
 * @brief Set lightness default value
 ****************************************************************************************
 */
__STATIC void app_mesh_set_ln_dflt(void)
{
    mm_api_srv_set_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_MDL_API_CMD,
                                               p_env->prf_task, TASK_APP,
                                               mm_api_srv_set_cmd);

    // This command is sent to set current state
    p_cmd->cmd_code = MM_API_SRV_SET;
    // Fill current bool state in to the command
    p_cmd->state = 65535;
    // State identifier value is Generic OnOff state
    p_cmd->state_id = MM_STATE_LIGHT_LN_DFLT;
    // Set model local index
    p_cmd->mdl_lid = p_env->lights_lid;

    // Send the command
    ke_msg_send(p_cmd);
}

/**
 ****************************************************************************************
 * @brief Callback function to set state by the updating of the current Generic Server On Off screen
 *
 * @param[in] state    Current Generic OnOff state (true = ON, false = OFF)
 ****************************************************************************************
 */
__STATIC void app_mesh_cb_gens_onoff(bool state)
{
    mm_api_srv_set_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_MDL_API_CMD,
                                               p_env->prf_task, TASK_APP,
                                               mm_api_srv_set_cmd);

    // This command is sent to set current state
    p_cmd->cmd_code = MM_API_SRV_SET;
    // Fill current bool state in to the command
    p_cmd->state = state ? 1 : 0;
    // State identifier value is Generic OnOff state
    p_cmd->state_id = MM_STATE_GEN_ONOFF;
    // Set model local index
    p_cmd->mdl_lid = p_env->gens_oo_lid;

    // Send the command
    ke_msg_send(p_cmd);
}

/**
 ****************************************************************************************
 * @brief Callback function to set state by the updating of the current Generic Client On Off screen
 *
 * @param[in] state    Current Generic  OnOff state (true = ON, false = OFF)
 ****************************************************************************************
 */
__STATIC void app_mesh_cb_genc_onoff(bool state)
{
    mm_api_cli_transition_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_MDL_API_CMD,
                                                      p_env->prf_task, TASK_APP,
                                                      mm_api_cli_transition_cmd);

    // This command is sent to set current state
    p_cmd->cmd_code = MM_API_CLI_TRANSITION;
    // Fill current bool state in to the command
    p_cmd->state_1 = state ? 1 : 0;
    // Set transition timing values
    p_cmd->trans_time_ms = 0;
    // Set delay timing values
    p_cmd->delay_ms = 0;
    // Set transition information
    p_cmd->trans_info = 0;
    // Set Acknowledged
    SETB(p_cmd->trans_info, MM_TRANS_INFO_ACK, 1);
    // Set Transition Identifier
    SETF(p_cmd->trans_info, MM_TRANS_INFO_TID, p_env->tid++);
    // Set model local index
    p_cmd->mdl_lid = p_env->genc_oo_lid;
    // Set application key local index
    p_cmd->app_key_lid = p_env->grp_app_key_lid;
    // Destination address
    p_cmd->dst = p_env->dest_addr;

    // Send the command
    ke_msg_send(p_cmd);
}

/**
 ****************************************************************************************
 * @brief Callback function to set value by the updating of the Lightness Client Lightness screen
 *
 * @param[in] lightness    Current Lightness Client Lightness value (0 - 65535)
 ****************************************************************************************
 */
__STATIC void app_mesh_cb_lightc_lightness(uint16_t lightness)
{
    mm_api_cli_transition_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_MDL_API_CMD,
                                                      p_env->prf_task, TASK_APP,
                                                      mm_api_cli_transition_cmd);

    // This command is sent to set current state
    p_cmd->cmd_code = MM_API_CLI_TRANSITION;
    // Fill current bool state in to the command
    p_cmd->state_1 = lightness;
    // Set transition timing values
    p_cmd->trans_time_ms = 0;
    // Set delay timing values
    p_cmd->delay_ms = 0;
    // Set transition information
    p_cmd->trans_info = 0;
    // Set Acknowledged
    SETB(p_cmd->trans_info, MM_TRANS_INFO_ACK, 1);
    // Set Transition Identifier
    SETF(p_cmd->trans_info, MM_TRANS_INFO_TID, p_env->tid++);
    // Set model local index
    p_cmd->mdl_lid = p_env->lightc_lid;
    // Set application key local index
    p_cmd->app_key_lid = p_env->grp_app_key_lid;
    // Destination address
    p_cmd->dst = p_env->dest_addr;

    // Send the command
    ke_msg_send(p_cmd);
}

/**
 ****************************************************************************************
 * @brief Callback function to save current configuration
 ****************************************************************************************
 */

__STATIC void app_mesh_cb_save(void)
{
    m_api_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_API_CMD,
                                      p_env->prf_task, TASK_APP,
                                      m_api_cmd);

    // This command is sent to save current configuration
    p_cmd->cmd_code = M_API_STORAGE_SAVE;

    // Send the command
    ke_msg_send(p_cmd);
}

/**
 ****************************************************************************************
 * @brief Callback function to remove mesh specific storage
 ****************************************************************************************
 */

__STATIC void app_mesh_cb_remove(void)
{
    // Initialize parameter identifiers
    uint8_t param_id;

    // Remove all mesh specific storage in NVDS
    for (param_id = PARAM_ID_MESH_SPECIFIC_FIRST; param_id <= (PARAM_ID_MESH_SPECIFIC_LAST + 1); param_id++)
    {
        // Remove mesh stored information from NVDS
        nvds_del(param_id);
    }
}

const app_display_mesh_cb_t app_mesh_cb_display =
{
    .cb_gens_onoff = app_mesh_cb_gens_onoff,
    .cb_genc_onoff = app_mesh_cb_genc_onoff,
    .cb_lightc_lightness = app_mesh_cb_lightc_lightness,
    .cb_save = app_mesh_cb_save,
    .cb_remove = app_mesh_cb_remove,
};

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the mesh application
 ****************************************************************************************
 */
void app_mesh_init(uint8_t mesh_demo_type)
{
    // Allocate environment structure
    p_env = (app_mesh_env_t*)ke_malloc(sizeof(app_mesh_env_t), KE_MEM_ENV);

    if (p_env)
    {
        // Device UUID length
        uint8_t dev_uuid_len = NVDS_LEN_APP_MESH_DEVICE_UUID;
        // The value lengh of recording if use storage
        uint8_t use_storage_len = NVDS_LEN_APP_MESH_USE_STORAGE;
        // Group address length
        uint8_t group_addr_len = NVDS_LEN_APP_MESH_GRP_ADDR;

        // Initialize environment content
        memset(p_env, 0, sizeof(app_mesh_env_t));

        // Keep provided demonstration type
        p_env->mesh_demo_type = mesh_demo_type;
        // Initialize state
        p_env->state = APP_MESH_STATE_INIT;

        // Register callback functions for display
        app_display_register_cb_mesh(&app_mesh_cb_display);

        #if (NVDS_SUPPORT)
        // Get the device uuid from storage
        if (nvds_get(NVDS_TAG_APP_MESH_DEVICE_UUID, &dev_uuid_len, &p_env->dev_uuid[0]) != NVDS_OK)
        {
            // If can't get the device uuid from storage, set device uuid with the default uuid
            memcpy(p_env->dev_uuid, APP_MESH_DFLT_DEVICE_UUID, MESH_DEV_UUID_LEN);
        }
        
        // Get the device uuid from storage
        if (nvds_get(NVDS_TAG_APP_MESH_USE_STORAGE, &use_storage_len, &p_env->use_storage) != NVDS_OK)
        {
            // If can't get the parameter from storage, set device uuid with the default uuid
            p_env->use_storage = APP_MESH_USE_STORAGE_DFLT;
        }

        if ((p_env->mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
                || (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN))
        {
            uint8_t group_addr_str[NVDS_LEN_APP_MESH_GRP_ADDR];

            // Get the group address from NVDS
            if (nvds_get(NVDS_TAG_APP_MESH_GRP_ADDR, &group_addr_len, &group_addr_str[0]) == NVDS_OK)
            {
                // Convert uint8_t* to uint16_t
                p_env->dest_addr = co_read16p(&group_addr_str[0]);
            }
            else
            {
                // If can't get the group address from storage, set group address with the default uuid
                p_env->dest_addr = APP_MESH_GRP_ADDR_DFLT;
            }
        }
        #else //(!NVDS_SUPPORT)
        // NVDS is not supported, use default parameters
        memcpy(p_env->dev_uuid, APP_MESH_DFLT_DEVICE_UUID, MESH_DEV_UUID_LEN);
        p_env->use_storage = APP_MESH_USE_STORAGE_DFLT;

        // Generic Client OnOff and Lightness Client
        if ((p_env->mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
                || (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN))
        {
            p_env->dest_addr = APP_MESH_GRP_ADDR_DFLT;
        }
        #endif //(NVDS_SUPPORT)

        // Inform display module about the group address that will be used
        app_display_mesh_set_group_addr(p_env->dest_addr);
    }
}

/**
 ****************************************************************************************
 * @brief Management of the mesh state machine
 *
 * @param[in] evt    Event code of current event status
 ****************************************************************************************
 */
void app_mesh_evt(uint8_t evt)
{
    // Loop
    bool loop = true;

    while (loop)
    {
        loop = false;

        switch (evt)
        {
            case (APP_MESH_EVT_TASK_ADDED):
            {
                if (p_env->mesh_demo_type != APP_MESH_DEMO_PROXY)
                {
                    // Register models needed for the demonstration
                    app_mesh_register_model();
                    break;
                }

                // Loop and act as if the models had been added
                loop = true;
                evt = APP_MESH_EVT_MDL_REGISTERED;
            } break;

            case (APP_MESH_EVT_MDL_REGISTERED):
            {
                if (p_env->state == APP_MESH_STATE_REGISTERING_GENC_ONOFF)
                {
                    if (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN)
                    {
                        // Register models needed for the demonstration
                        app_mesh_register_model();
                        break;
                    }
                }

                #if (NVDS_SUPPORT)
                if (p_env->use_storage)
                {
                    // Load stored information
                    app_mesh_storage_load();
                    break;
                }
                #endif //(NVDS_SUPPORT)

                // Loop and act as if the stored information had been loaded
                loop = true;
                evt = APP_MESH_EVT_STORED_INFO_LOADED;
            } break;

            case (APP_MESH_EVT_STORED_INFO_LOADED):
            {
                // Update state
                p_env->state = APP_MESH_STATE_CONFIGURING;

                if (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN)
                {
                    app_mesh_set_ln_dflt();
                    break;
                }

                // Loop and act as if the model had been configured
                loop = true;
                evt = APP_MESH_EVT_MDL_CONFIGURED;
            } break;

            case (APP_MESH_EVT_MDL_CONFIGURED):
            {
                // Update state
                p_env->state = APP_MESH_STATE_ENABLING;

                // Enable the stack
                app_mesh_enable_task();
            } break;

            case (APP_MESH_EVT_ENABLED):
            {
                // Update state
                p_env->state = APP_MESH_STATE_WAIT_PROV;
            } break;

            default:
            {
            } break;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Add a Mesh Service instance in the DB
 ****************************************************************************************
 */
void app_mesh_add_svc(void)
{
    // Send a creating new task command
    struct gapm_profile_task_add_cmd* p_req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                               TASK_GAPM, TASK_APP,
                                                               gapm_profile_task_add_cmd, sizeof(m_cfg_t));
    // Pointer to profile configuration
    m_cfg_t* p_cfg = (m_cfg_t*)p_req->param;

    // Fill message
    p_req->operation = GAPM_PROFILE_TASK_ADD;
    // No sec
    p_req->sec_lvl = 0;
    // Set mesh profile task
    p_req->prf_task_id = TASK_ID_MESH;
    // Set application task number
    p_req->app_task = TASK_APP;
    // Service start handle
    p_req->start_hdl = 0;

    // Proxy node and PB GATT features are supported
    p_cfg->features = M_FEAT_PB_GATT_SUP | M_FEAT_MSG_API_SUP;

    //if (p_env->mesh_demo_type == APP_MESH_DEMO_PROXY)
    {
        p_cfg->features |= M_FEAT_PROXY_NODE_SUP;
    }

    // Company identifier assigned by the Bluetooth SIG
    p_cfg->cid = 0x0060;
    // Vendor-assigned product identifier
    p_cfg->pid = 0x001A;
    // Vendor-assigned product version identifier
    p_cfg->vid = 0x0001;
    p_cfg->loc = 0x0100;

    // Send the command
    ke_msg_send(p_req);
}

/**
 ****************************************************************************************
 * @brief Update the profile task number
 *
 * @param[in] task    Profile task number.
 ****************************************************************************************
 */
void app_mesh_set_prf_task(uint16_t task)
{
    // Update the profile task number
    p_env->prf_task = task;
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of mesh API provisioning parameter request message
 *        Send a mesh provisioning parameters response
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_MESH).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
*/

__STATIC int m_api_prov_param_req_ind_handler(ke_msg_id_t const msgid, void const* p_param,
                                              ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Send a mesh provisioning parameters response
    m_api_prov_param_cfm_t* p_cfm = KE_MSG_ALLOC(MESH_API_PROV_PARAM_CFM,
                                                 p_env->prf_task, TASK_APP,
                                                 m_api_prov_param_cfm);

    // Copy the Device UUID
    memcpy(&p_cfm->dev_uuid[0], &p_env->dev_uuid[0], MESH_DEV_UUID_LEN);

    // Set the parameters
    p_cfm->uri_hash = 0;
    p_cfm->oob_info = 0;
    p_cfm->pub_key_oob = 0;
    p_cfm->static_oob = 0;
    p_cfm->out_oob_size = 0;
    p_cfm->in_oob_size = 0;
    p_cfm->out_oob_action = 0;
    p_cfm->in_oob_action = 0;
    p_cfm->info = 0;

    // Send the message
    ke_msg_send(p_cfm);

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of mesh API command complete event
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int m_api_cmp_evt_handler(ke_msg_id_t const msgid, m_api_cmp_evt_t* p_param,
                                   ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Sanity check
    ASSERT_INFO(p_param->status == MESH_ERR_NO_ERROR, p_param->cmd_code, p_param->status);

    // Check if an error has been reported
    if (p_param->status == MESH_ERR_NO_ERROR)
    {
        switch (p_param->cmd_code)
        {
            #if (NVDS_SUPPORT)
            case (M_API_STORAGE_LOAD):
            {
                // Mesh stored information have been retrieved from NVDS, go to next step
                app_mesh_evt(APP_MESH_EVT_STORED_INFO_LOADED);
            } break;

            case (M_API_STORAGE_SAVE):
            {
                // Mesh information have been stored in NVDS, update displayed status
                app_display_mesh_save_end();
            } break;
            #endif //(NVDS_SUPPORT)

            case (M_API_ENABLE):
            {
                // Get complete event parameters
                m_api_enable_cmp_evt_t* p_cmp_evt = (m_api_enable_cmp_evt_t*)p_param;

                // Update displayed provisioning state
                app_display_mesh_set_prov(p_cmp_evt->prov);

                // Mesh profile has been enabled, go to next step
                app_mesh_evt(APP_MESH_EVT_ENABLED);

                if (p_cmp_evt->prov)
                {
                    // Send a command to control the Proxy Connectable mode.
                    m_api_proxy_ctl_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_API_CMD,
                                                                p_env->prf_task, TASK_APP,
                                                                m_api_proxy_ctl_cmd);

                    // This command is sent to control proxy.
                    p_cmd->cmd_code = M_API_PROXY_CTL;
                    // Start connectable advertising with Network ID
                    p_cmd->enable = MESH_PROXY_ADV_CTL_START_NET;

                    // Send the command
                    ke_msg_send(p_cmd);
                }
            } break;

            default:
            {
                // Drop the message
            } break;
        }
    }

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of M_API_ATTENTION_UPDATE_IND indication message
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int m_api_attention_update_ind_handler(ke_msg_id_t const msgid,
                                                m_api_attention_update_ind_t* p_param,
                                                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Inform display module about the new attention timer duration
    app_display_mesh_set_attention(p_param->attention_state);

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of M_API_PROV_STATE_IND indication message
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int m_api_prov_state_ind_handler(ke_msg_id_t const msgid,
                                          m_api_prov_state_ind_t *p_param,
                                          ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (p_param->state == M_PROV_FAILED)
    {
        // Send a command to control the Proxy Connectable mode
        m_api_proxy_ctl_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_API_CMD,
                                                    p_env->prf_task, TASK_APP,
                                                    m_api_proxy_ctl_cmd);

        // This command is sent to control proxy
        p_cmd->cmd_code = M_API_PROXY_CTL;
        // Start connectable advertising with Network ID
        p_cmd->enable = MESH_PROXY_ADV_CTL_START_NODE;

        // Send the command
        ke_msg_send(p_cmd);
    }
    else if (p_param->state == M_PROV_SUCCEED)
    {
        // Update content of the provisioning state screen
        p_env->state = APP_MESH_STATE_PROV;

        // Inform display module about end of provisioning
        app_display_mesh_set_prov(true);
    }

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of mesh API proxy advertising state message
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int m_api_proxy_adv_update_ind_handler(ke_msg_id_t const msgid,
                                                m_api_proxy_adv_update_ind_t* p_param,
                                                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Restart advertising when receive advertising state with Node Identity stop
    if ((p_param->state == MESH_PROXY_ADV_NODE_STOP)
            || (p_param->state == MESH_PROXY_ADV_NET_STOP))
    {
        // Send a command to control the Proxy Connectable mode.
        m_api_proxy_ctl_cmd_t* p_cmd = KE_MSG_ALLOC(MESH_API_CMD,
                                                    p_env->prf_task, TASK_APP,
                                                    m_api_proxy_ctl_cmd);

        // This command is sent to control proxy.
        p_cmd->cmd_code = M_API_PROXY_CTL;
        // Start connectable advertising with Network ID.
        p_cmd->enable = MESH_PROXY_ADV_CTL_START_NET;

        // Send the command
        ke_msg_send(p_cmd);
    }

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of M_API_GROUP_UPDATE_IND indication
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int m_api_group_update_ind_handler(ke_msg_id_t const msgid,
                                            m_api_group_update_ind_t* p_param,
                                            ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if ((p_env->mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
            || (p_env->mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN))
    {
        if (p_param->added)
        {
            // Use this application key for communication with the server
            p_env->grp_app_key_lid = p_param->app_key_lid;
        }
    }

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of a command complete event for a Mesh Model command
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int mm_api_cmp_evt_handler(ke_msg_id_t const msgid, mm_api_cmp_evt_t* p_param,
                                    ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Sanity check
    ASSERT_INFO(p_param->status == MESH_ERR_NO_ERROR, p_param->cmd_code, p_param->status);

    // Check if an error has been reported
    if (p_param->status == MESH_ERR_NO_ERROR)
    {
        if ((p_param->cmd_code == MM_API_REGISTER_SERVER)
                || (p_param->cmd_code == MM_API_REGISTER_CLIENT))
        {
            // Models have been registered, go to next state
            app_mesh_evt(APP_MESH_EVT_MDL_REGISTERED);
        }
        else if (p_param->cmd_code == MM_API_SRV_SET)
        {
            if (p_env->state == APP_MESH_STATE_CONFIGURING)
            {
                // Models have been configured
                app_mesh_evt(APP_MESH_EVT_MDL_CONFIGURED);
            }
        }
    }

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of a Mesh Model indication
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int mm_api_ind_handler(ke_msg_id_t const msgid, mm_api_ind_t* p_param,
                                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (p_param->ind_code)
    {
        case (MM_API_SRV_STATE_UPD_IND):
        {
            // Get information for this indication
            mm_api_srv_state_upd_ind_t* p_ind = (mm_api_srv_state_upd_ind_t*)p_param;

            if (p_ind->state_id == MM_STATE_GEN_ONOFF)
            {
                // Inform display module about the received state
                app_display_mesh_set_onoff(p_ind->trans_time_ms, p_ind->state);
            }
            else if (p_ind->state_id == MM_STATE_LIGHT_LN)
            {
                // Inform display module about the received state
                app_display_mesh_set_lightness(p_ind->trans_time_ms, p_ind->state);
            }
        } break;

        case (MM_API_REGISTER_IND):
        {
            // Get information for this indication
            mm_api_register_ind_t* p_ind = (mm_api_register_ind_t*)p_param;

            // Keep the provided local index
            if (p_ind->model_id == MM_ID_GENS_OO)
            {
                p_env->gens_oo_lid = p_ind->mdl_lid;
            }
            else if (p_ind->model_id == MM_ID_GENC_OO)
            {
                p_env->genc_oo_lid = p_ind->mdl_lid;
            }
            else if (p_ind->model_id == MM_ID_LIGHTS_LN)
            {
                p_env->lights_lid = p_ind->mdl_lid;
            }
            else if (p_ind->model_id == MM_ID_LIGHTC_LN)
            {
                p_env->lightc_lid = p_ind->mdl_lid;
            }
        } break;

        default:
        {
        } break;
    }

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

const struct ke_msg_handler app_mesh_msg_handler_list[] =
{
    {MESH_API_CMP_EVT, (ke_msg_func_t)m_api_cmp_evt_handler},
    {MESH_API_PROV_PARAM_REQ_IND, (ke_msg_func_t)m_api_prov_param_req_ind_handler},
    {MESH_API_ATTENTION_UPDATE_IND, (ke_msg_func_t)m_api_attention_update_ind_handler},
    {MESH_API_PROV_STATE_IND, (ke_msg_func_t)m_api_prov_state_ind_handler},
    {MESH_API_PROXY_ADV_UPDATE_IND, (ke_msg_func_t)m_api_proxy_adv_update_ind_handler},
    {MESH_API_GROUP_UPDATE_IND, (ke_msg_func_t)m_api_group_update_ind_handler},
    {MESH_MDL_API_CMP_EVT, (ke_msg_func_t)mm_api_cmp_evt_handler},
    {MESH_MDL_API_IND, (ke_msg_func_t)mm_api_ind_handler},
};

const struct app_subtask_handlers app_mesh_handlers = APP_HANDLERS(app_mesh);

#endif //(BLE_APP_MESH)

/// @} APP_MESH

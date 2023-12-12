/**
 ****************************************************************************************
 *
 * @file hap_has_msg.h
 *
 * @brief Hearing Aid Profile - Hearing Aid Service Server - Message API Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_HAS_MSG_H_
#define HAP_HAS_MSG_H_

/**
 ****************************************************************************************
 * @defgroup HAP_HAS_MSG Message API
 * @ingroup HAP_HAS
 * @brief Description of Message API for Hearing Aid Service Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hap_msg.h"        // Message API Definitions
#include "hap_has.h"        // Hearing Aid Service Server Definitions

/// @addtogroup HAP_HAS_MSG
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of GAF_REQ request code values for Hearing Aid Service Server module
enum hap_has_msg_req_code
{
    /// Restore Bond Data (see #hap_has_restore_bond_data_req_t)
    HAP_HAS_RESTORE_BOND_DATA = GAF_CODE(HAP, HAS, 0),
    /// Configure Preset (see #hap_has_configure_preset_req_t)
    HAP_HAS_CONFIGURE_PRESET = GAF_CODE(HAP, HAS, 1),
    /// Set Active Preset (see #hap_has_set_active_preset_req_t)
    HAP_HAS_SET_ACTIVE_PRESET = GAF_CODE(HAP, HAS, 2),
    /// Set Coordination Support (see #hap_has_set_coordination_support_req_t)
    HAP_HAS_SET_COORDINATION_SUPPORT = GAF_CODE(HAP, HAS, 3),
};

/// List of GAF_IND indication code values for Hearing Aid Service Server module
enum hap_has_msg_ind_code
{
    /// Bond Data (see #hap_has_bond_data_ind_t)
    HAP_HAS_BOND_DATA = GAF_CODE(HAP, HAS, 0),
    /// Preset Name (see #hap_has_preset_name_ind_t)
    HAP_HAS_PRESET_NAME = GAF_CODE(HAP, HAS, 1),
};

/// List of GAF_REQ_IND request indication code values for Hearing Aid Service Server module
enum hap_has_msg_req_ind_code
{
    /// Set Preset Active (see #hap_has_set_preset_active_req_ind_t)
    HAP_HAS_SET_PRESET_ACTIVE = GAF_CODE(HAP, HAS, 0),
};

/*
 * API MESSAGES
 ****************************************************************************************
 */

/// Structure for #HAP_HAS_RESTORE_BOND_DATA request message
typedef struct hap_has_restore_bond_data_req
{
    /// Request code (shall be set to #HAP_HAS_RESTORE_BOND_DATA)
    uint16_t req_code;
    /// Connection local index
    uint8_t con_lid;
    /// Client configuration bit field
    uint8_t cli_cfg_bf;
    /// Client configuration bit field for Presets characteristic instances
    uint8_t presets_cli_cfg_bf;
    /// Event configuration bit field
    uint8_t evt_cfg_bf;
    /// Event configuration bit field for Presets characteristic instances
    uint8_t presets_evt_cfg_bf;
} hap_has_restore_bond_data_req_t;

/// Structure for #HAP_HAS_CONFIGURE_PRESET request message
typedef struct hap_has_configure_preset_req
{
    /// Request code (shall be set to #HAP_HAS_CONFIGURE_PRESET)
    uint16_t req_code;
    /// Preset local index
    uint8_t preset_lid;
    /// Presets characteristic instance that will contain information about the preset
    uint8_t presets_instance_idx;
    /// Indicate if Preset name can be written (= 0) or not by a Client device
    uint8_t read_only;
    /// Length of Preset name
    /// Up to HAP_HAS_PRESET_NAME_LEN_MAX
    uint8_t length;
    /// Preset name
    uint8_t name[__ARRAY_EMPTY];
} hap_has_configure_preset_req_t;

/// Structure for #HAP_HAS_SET_ACTIVE_PRESET request message
typedef struct hap_has_set_active_preset_req
{
    /// Request code (shall be set to #HAP_HAS_SET_ACTIVE_PRESET)
    uint16_t req_code;
    /// Preset local index
    uint8_t preset_lid;
} hap_has_set_active_preset_req_t;

/// Structure for #HAP_HAS_SET_COORDINATION_SUPPORT request message
typedef struct hap_has_set_coordination_support_req
{
    /// Request code (shall be set to #HAP_HAS_SET_COORDINATION_SUPPORT)
    uint16_t req_code;
    /// Indicate if Preset Coordination is supported (>= 1) or not
    uint8_t supported;
} hap_has_set_coordination_support_req_t;

/// Structure of response message for Hearing Aid Service Server module
typedef struct hap_has_rsp
{
    /// Request code (see #hap_has_msg_req_code enumeration)
    uint16_t req_code;
    /// Status
    uint16_t status;
    /// Union
    union
    {
        /// Local index
        uint8_t lid;
        /// Connection local index
        uint8_t con_lid;
        /// Preset local index
        uint8_t preset_lid;
    } lid;
} hap_has_rsp_t;

/// Structure for #HAP_HAS_BOND_DATA indication message
typedef struct hap_has_bond_data_ind
{
    /// Indication code (set to #HAP_HAS_BOND_DATA)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Client configuration bit field
    uint8_t cli_cfg_bf;
    /// Client configuration bit field for Presets characteristic instances
    uint8_t presets_cli_cfg_bf;
} hap_has_bond_data_ind_t;

/// Structure for #HAP_HAS_PRESET_NAME indication message
typedef struct hap_has_preset_name_ind
{
    /// Indication code (set to #HAP_HAS_PRESET_NAME)
    uint16_t ind_code;
    /// Preset local index
    uint8_t preset_lid;
    /// Connection local index
    uint8_t con_lid;
    /// Length of Preset name\n
    /// Up to HAP_HAS_PRESET_NAME_LEN_MAX
    uint8_t length;
    /// Preset name
    uint8_t name[__ARRAY_EMPTY];
} hap_has_preset_name_ind_t;

/// Structure for #HAP_HAS_SET_PRESET_ACTIVE request indication message
typedef struct hap_has_set_preset_active_req_ind
{
    /// Request Indication code (set to #HAP_HAS_SET_PRESET_ACTIVE)
    uint16_t req_ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Preset local index
    uint8_t preset_lid;
    /// Indicate if Server must inform other member of the Coordinated Set about the new active preset (>= 1) or not
    uint8_t coordinated;
} hap_has_set_preset_active_req_ind_t;

/// Structure for #HAP_HAS_SET_PRESET_ACTIVE confirmation message
typedef struct hap_has_set_preset_active_cfm
{
    /// Request Indication code (shall be set to #HAP_HAS_SET_PRESET_ACTIVE)
    uint16_t req_ind_code;
    /// Status
    uint16_t status;
} hap_has_set_preset_active_cfm_t;

/// @} HAP_HAS_MSG

#endif // HAP_HAS_MSG_H_

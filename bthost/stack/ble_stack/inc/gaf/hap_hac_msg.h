/**
 ****************************************************************************************
 *
 * @file hap_hac_msg.h
 *
 * @brief Hearing Aid Profile - Hearing Aid Service Client - Message API Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_HAC_MSG_H_
#define HAP_HAC_MSG_H_

/**
 ****************************************************************************************
 * @defgroup HAP_HAC_MSG Message API
 * @ingroup HAP_HAC
 * @brief Description of Message API for Hearing Aid Service Client module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hap_msg.h"        // Message API Definitions
#include "hap_hac.h"        // Hearing Aid Service Client Definitions

/// @addtogroup HAP_HAC_MSG
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of GAF_CMD command code values for Hearing Aid Service Client module
enum hap_hac_msg_cmd_code
{
    /// Discover (see #hap_hac_discover_cmd_t)
    HAP_HAC_DISCOVER = GAF_CODE(HAP, HAC, HAP_HAC_CMD_TYPE_DISCOVER),
    /// Get (see #hap_hac_get_cmd_t)
    HAP_HAC_GET = GAF_CODE(HAP, HAC, HAP_HAC_CMD_TYPE_GET),
    /// Get Configuration (see #hap_hac_get_cfg_cmd_t)
    HAP_HAC_GET_CFG = GAF_CODE(HAP, HAC, HAP_HAC_CMD_TYPE_GET_CFG),
    /// Set Configuration (see #hap_hac_set_cfg_cmd_t)
    HAP_HAC_SET_CFG = GAF_CODE(HAP, HAC, HAP_HAC_CMD_TYPE_SET_CFG),
    /// Set Preset Name (see #hap_hac_set_preset_name_cmd_t)
    HAP_HAC_SET_PRESET_NAME = GAF_CODE(HAP, HAC, HAP_HAC_CMD_TYPE_SET_PRESET_NAME),
    /// Set Active Preset (see #hap_hac_set_active_preset_cmd_t)
    HAP_HAC_SET_ACTIVE_PRESET = GAF_CODE(HAP, HAC, HAP_HAC_CMD_TYPE_SET_ACTIVE_PRESET),
};

/// List of GAF_REQ request code values for Hearing Aid Service Client module
enum hap_hac_msg_req_code
{
    /// Restore Bond Data (see #hap_hac_restore_bond_data_req_t)
    HAP_HAC_RESTORE_BOND_DATA = GAF_CODE(HAP, HAC, 0),
};

/// List of GAF_IND indication code values for Hearing Aid Service Client module
enum hap_hac_msg_ind_code
{
    /// Bond Data (see #hap_hac_bond_data_ind_t)
    HAP_HAC_BOND_DATA = GAF_CODE(HAP, HAC, 0),
    /// Service Changed (see #hap_hac_svc_changed_ind_t)
    HAP_HAC_SVC_CHANGED = GAF_CODE(HAP, HAC, 1),
    /// Preset (see #hap_hac_preset_ind_t)
    HAP_HAC_PRESET = GAF_CODE(HAP, HAC, 2),
    /// Value (see #hap_hac_value_ind_t)
    HAP_HAC_VALUE = GAF_CODE(HAP, HAC, 3),
    /// Configuration (see #hap_hac_cfg_ind_t)
    HAP_HAC_CFG = GAF_CODE(HAP, HAC, 4),
};

/*
 * API MESSAGES
 ****************************************************************************************
 */

/// Structure for #HAP_HAC_RESTORE_BOND_DATA request message
typedef struct hap_hac_restore_bond_data_req
{
    /// Request code (shall be set to #HAP_HAC_RESTORE_BOND_DATA)
    uint16_t req_code;
    /// Connection local index
    uint8_t con_lid;
    /// Content description of Hearing Aid Service
    hap_hac_has_t has_info;
} hap_hac_restore_bond_data_req_t;

/// Structure of response message for Hearing Aid Service Client module
typedef struct hap_hac_rsp
{
    /// Request code (see #hap_hac_msg_req_code enumeration)
    uint16_t req_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
} hap_hac_rsp_t;

/// Structure for #HAP_HAC_DISCOVER command message
typedef struct hap_hac_discover_cmd
{
    /// Command code (shall be set to #HAP_HAC_DISCOVER)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Start handle for the discovery. Set GATT_INVALID_HDL if not provided
    uint16_t shdl;
    /// End handle for the discovery. Set GATT_INVALID_HDL if not provided
    uint16_t ehdl;
} hap_hac_discover_cmd_t;

/// Structure for #HAP_HAC_GET command message
typedef struct hap_hac_get_cmd
{
    /// Command code (shall be set to #HAP_HAC_GET)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Characteristic type (see #hap_has_char_type enumeration)\n
    /// Preset Control Point characteristic is not readable
    uint8_t char_type;
    /// Presets characteristic instance index\n
    /// Meaningful only if value for a Presets characteristic instance must be read
    uint8_t presets_instance_idx;
} hap_hac_get_cmd_t;

/// Structure for #HAP_HAC_GET_CFG command message
typedef struct hap_hac_get_cfg_cmd
{
    /// Command code (shall be set to #HAP_HAC_GET_CFG)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Characteristic type (see #hap_has_char_type enumeration)
    uint8_t char_type;
    /// Presets characteristic instance index\n
    /// Meaningful only if notification configuration for a Presets characteristic instance must be read
    uint8_t presets_instance_idx;
} hap_hac_get_cfg_cmd_t;

/// Structure for #HAP_HAC_SET_CFG command message
typedef struct hap_hac_set_cfg_cmd
{
    /// Command code (shall be set to #HAP_HAC_SET_CFG)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Characteristic type (see #hap_has_char_type enumeration)
    uint8_t char_type;
    /// Presets characteristic instance index\n
    /// Meaningful only if notification configuration for a Presets characteristic instance must be set
    uint8_t presets_instance_idx;
    /// Indicate if sending of notifications must be enabled (>= 1) or disable for the indicated characteristic
    uint8_t enable;
} hap_hac_set_cfg_cmd_t;

/// Structure for #HAP_HAC_SET_PRESET_NAME command message
typedef struct hap_hac_set_preset_name_cmd
{
    /// Command code (shall be set to #HAP_HAC_SET_PRESET_NAME)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Preset index
    uint8_t preset_idx;
    /// Length of Preset name
    uint8_t length;
    /// Preset name
    uint8_t name[__ARRAY_EMPTY];
} hap_hac_set_preset_name_cmd_t;

/// Structure for #HAP_HAC_SET_ACTIVE_PRESET command message
typedef struct hap_hac_set_active_preset_cmd
{
    /// Command code (shall be set to #HAP_HAC_SET_ACTIVE_PRESET)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Set type (see #hap_hac_set_type enumeration)
    uint8_t set_type;
    /// Indicate if Server device must inform other member of the Coordinated Set it belongs to (>= 1) or not
    uint8_t coordinated;
    /// Preset index
    uint8_t preset_idx;
} hap_hac_set_active_preset_cmd_t;

/// Structure for command complete event message for Hearing Aid Service Client module
typedef struct hap_hac_cmp_evt
{
    /// Command code (see #hap_hac_msg_cmd_code enumeration)
    uint16_t cmd_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
    /// Union
    union
    {
        /// Type
        uint8_t type;
        /// Set type (see #hap_hac_set_type enumeration)
        uint8_t set_type;
        /// Characteristic type (see #hap_has_char_type enumeration)
        uint8_t char_type;
    } type;
    /// Union
    union
    {
        /// Preset information
        uint8_t preset_info;
        /// Preset index
        uint8_t preset_idx;
        /// Presets characteristic instance index
        uint8_t presets_instance_idx;
    } preset_info;
} hap_hac_cmp_evt_t;

/// Structure for #HAP_HAC_BOND_DATA indication message
typedef struct hap_hac_bond_data_ind
{
    /// Indication code (set to #HAP_HAC_BOND_DATA)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Content description of Hearing Aid Service
    hap_hac_has_t has_info;
} hap_hac_bond_data_ind_t;

/// Structure for #HAP_HAC_SVC_CHANGED indication message
typedef struct hap_hac_svc_changed_ind
{
    /// Indication code (set to #HAP_HAC_SVC_CHANGED)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
} hap_hac_svc_changed_ind_t;

/// Structure for #HAP_HAC_PRESET indication message
typedef struct hap_hac_preset_ind
{
    /// Indication code (set to #HAP_HAC_PRESET)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Presets characteristic instance index
    uint8_t presets_instance_idx;
    /// Preset index
    uint8_t preset_idx;
    /// Indicate if Preset name can be written (>= 1) or not
    uint8_t read_only;
    /// Length of Preset name
    uint8_t length;
    /// Preset name
    uint8_t name[__ARRAY_EMPTY];
} hap_hac_preset_ind_t;

/// Structure for #HAP_HAC_VALUE indication message
typedef struct hap_hac_value_ind
{
    /// Indication code (set to #HAP_HAC_VALUE)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Characteristic type (see #hap_has_char_type enumeration)
    uint8_t char_type;
    /// Union
    union
    {
        /// Parameter
        uint8_t param;
        /// Active Preset index
        uint8_t preset_idx;
        /// Flags bit field
        uint8_t flags_bf;
    } param;
} hap_hac_value_ind_t;

/// Structure for #HAP_HAC_CFG indication message
typedef struct hap_hac_cfg_ind
{
    /// Indication code (set to #HAP_HAC_CFG)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Characteristic type (see #hap_has_char_type enumeration)
    uint8_t char_type;
    /// Presets characteristic instance index
    /// Meaningful only for Presets characteristic
    uint8_t presets_instance_idx;
    /// Indicate if sending of notifications is enabled (>= 1) or not for the indicated characteristic
    uint8_t enabled;
} hap_hac_cfg_ind_t;

/// @} HAP_HAC_MSG

#endif // HAP_HAC_MSG_H_

/**
 ****************************************************************************************
 *
 * @file hap_hac.h
 *
 * @brief Hearing Aid Profile - Hearing Aid Service Client - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_HAC_H_
#define HAP_HAC_H_


/**
 ****************************************************************************************
 * @defgroup HAP_HAC Hearing Aid Service Client
 * @ingroup HAP
 * @brief Description of Hearing Aid Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAC_ENUM Enumerations
 * @ingroup HAP_HAC
 * @brief Enumerations for Hearing Aid Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAC_STRUCT Structures
 * @ingroup HAP_HAC
 * @brief Structures for Hearing Aid Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAC_NATIVE_API Native API
 * @ingroup HAP_HAC
 * @brief Description of Native API for Hearing Aid Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAC_CALLBACK Callback Functions
 * @ingroup HAP_HAC_NATIVE_API
 * @brief Description of callback functions for Hearing Aid Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAC_FUNCTION Functions
 * @ingroup HAP_HAC_NATIVE_API
 * @brief Description of functions for Hearing Aid Service Client module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hap.h"                // Hearing Aid Profile Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup HAP_HAC_ENUM
/// @{

/// List of command type values for Hearing Aid Service Client module
enum hap_hac_cmd_type
{
    /// Hearing Aid Service Client - Discover
    HAP_HAC_CMD_TYPE_DISCOVER = 0,
    /// Hearing Aid Service Client - Get
    HAP_HAC_CMD_TYPE_GET,
    /// Hearing Aid Service Client - Get Configuration
    HAP_HAC_CMD_TYPE_GET_CFG,
    /// Hearing Aid Service Client - Set Configuration
    HAP_HAC_CMD_TYPE_SET_CFG,
    /// Hearing Aid Service Client - Set Preset Name
    HAP_HAC_CMD_TYPE_SET_PRESET_NAME,
    /// Hearing Aid Service Client - Set Active Preset
    HAP_HAC_CMD_TYPE_SET_ACTIVE_PRESET,
};

/// Set type values
enum hap_hac_set_type
{
    /// Set active Preset by providing its index
    HAP_HAC_SET_TYPE_INDEX = 0,
    /// Set active Preset by choosing next Preset
    HAP_HAC_SET_TYPE_NEXT,
    /// Set active Preset by choosing previous Preset
    HAP_HAC_SET_TYPE_PREVIOUS,

    /// Maximum value
    HAP_HAC_SET_TYPE_MAX,
};

/// @} HAP_HAC_ENUM

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// @addtogroup HAP_HAC_STRUCT
/// @{

/// Hearing Aid Service characteristic description structure
typedef struct hap_hac_has_char
{
    /// Characteristic value handle
    uint16_t val_hdl;
    /// Client Characteristic Configuration descriptor handle
    uint16_t desc_hdl;
} hap_hac_has_char_t;

/// Hearing Aid Service content description structure
typedef struct hap_hac_has
{
    /// Service description
    prf_svc_t svc_info;
    /// Number of Presets characteristic instances
    uint8_t nb_presets_chars;
    /// Characteristics description
    hap_hac_has_char_t char_info[HAP_HAS_CHAR_TYPE_PRESETS];
    /// Presets characteristics description
    hap_hac_has_char_t presets_char_info[__ARRAY_EMPTY];
} hap_hac_has_t;

/// @} HAP_HAC_STRUCT

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup HAP_HAC_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called when a command has been completed
 *
 * @param[in] cmd_type              Command type (see #hap_hac_cmd_type enumeration)
 * @param[in] status                Status
 * @param[in] con_lid               Connection local index
 * @param[in] type                  Type\n
 *                                      - Set type (see #hap_hac_set_type enumeration)\n
 *                                      - Characteristic type (see #hap_has_char_type enumeration)
 * @param[in] preset_info           Preset index or Presets characteristic instance index
 ****************************************************************************************
 */
typedef void (*hap_hac_cb_cmp_evt)(uint8_t cmd_type, uint16_t status, uint8_t con_lid, uint8_t type,
                                   uint8_t preset_info);

/**
 ****************************************************************************************
 * @brief Callback function called when Hearing Aid Service has been discovered in a Service device database
 *
 * @param[in] con_lid               Connection local index
 * @param[in] p_has_info            Pointer to Hearing Aid Service content description structure
 ****************************************************************************************
 */
typedef void (*hap_hac_cb_bond_data)(uint8_t con_lid, const hap_hac_has_t* p_has_info);

/**
 ****************************************************************************************
 * @brief Callback function called when a service changed indication for the Hearing Aid Service has been received
 *
 * @param[in] con_lid               Connection local index
 ****************************************************************************************
 */
typedef void (*hap_hac_cb_svc_changed)(uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Callback function called when a Preset Record is received
 *
 * @param[in] con_lid               Connection local index
 * @param[in] presets_instance_idx  Presets characteristic instance index
 * @param[in] preset_idx            Preset index
 * @param[in] read_only             Indicate if Preset name can be written
 * @param[in] length                Length of Preset name
 * @param[in] p_name                Pointer to Preset name
 ****************************************************************************************
 */
typedef void (*hap_hac_cb_preset)(uint8_t con_lid, uint8_t presets_instance_idx, uint8_t preset_idx,
                                  bool read_only, uint8_t length, const uint8_t* p_name);

/**
 ****************************************************************************************
 * @brief Callback function called when either Active Preset index or Flags bit field value is received
 *
 * @param[in] con_lid               Connection local index
 * @param[in] char_type             Characteristic type (see #hap_has_char_type enumeration)
 * @param[in] value                 Active Preset index or Flags bit field value
 ****************************************************************************************
 */
typedef void (*hap_hac_cb_value)(uint8_t con_lid, uint8_t char_type, uint8_t value);

/**
 ****************************************************************************************
 * @brief Callback function called when notification configuration has been received for a characteristic
 *
 * @param[in] con_lid               Connection local index
 * @param[in] char_type             Characteristic type (see #hap_has_char_type enumeration)
 * @param[in] presets_instance_idx  Presets characteristic instance index
 * @param[in] enabled               Indicate if sending of notifications is enabled or not
 ****************************************************************************************
 */
typedef void (*hap_hac_cb_cfg)(uint8_t con_lid, uint8_t char_type, uint8_t presets_instance_idx, bool enabled);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of callback functions for Hearing Aid Service Client module communication with upper layer
typedef struct hap_hac_cb
{
    /// Callback function called when a command has been completed
    hap_hac_cb_cmp_evt cb_cmp_evt;
    /// Callback function called when Hearing Aid Service has been discovered in a Service device database
    hap_hac_cb_bond_data cb_bond_data;
    /// Callback function called when a service changed indication for the Hearing Aid Service has been received
    hap_hac_cb_svc_changed cb_svc_changed;
    /// Callback function called when a Preset Record is received
    hap_hac_cb_preset cb_preset;
    /// Callback function called when either Active Preset index or Flags bit field value is received
    hap_hac_cb_value cb_value;
    /// Callback function called when notification configuration has been received for a characteristic
    hap_hac_cb_cfg cb_cfg;
} hap_hac_cb_t;

/// @} HAP_HAC_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_HAP_HAC)
/// @addtogroup HAP_HAC_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Enable Hearing Aid Profile block for use of Hearing Aid Profile as Client for a connected device with
 * which no bonding has been established during a previous connection\n
 * All readable characteristics are read during the procedure\n
 * Sending of notifications is enabled for each characteristic supporting this feature during the procedure
 *
 * @param[in] con_lid               Connection local index
 * @param[in] shdl                  Start handle for the discovery. Set GATT_INVALID_HDL if not provided
 * @param[in] ehdl                  End handle for the discovery. Set GATT_INVALID_HDL if not provided
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_discover(uint8_t con_lid, uint16_t shdl, uint16_t ehdl);

#if (GAF_DBG)
/**
 ****************************************************************************************
 * @brief Get value of either an instance of the Presets characteristic, or the Active Preset characteristic, or
 * the Flags characteristic
 *
 * @param[in] con_lid               Connection local index
 * @param[in] char_type             Characteristic type (see #hap_has_char_type enumeration)
 * @param[in] presets_instance_idx  Presets characteristic instance index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_get(uint8_t con_lid, uint8_t char_type, uint8_t presets_instance_idx);

/**
 ****************************************************************************************
 * @brief Get notification configuration for either an instance of the Presets characteristic, or the Active Preset
 * characteristic, or the Flags characteristic
 *
 * @param[in] con_lid               Connection local index
 * @param[in] char_type             Characteristic type (see #hap_has_char_type enumeration)
 * @param[in] presets_instance_idx  Presets characteristic instance index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_get_cfg(uint8_t con_lid, uint8_t char_type, uint8_t presets_instance_idx);

/**
 ****************************************************************************************
 * @brief Set notification configuration for either an instance of the Presets characteristic, or the Active Preset
 * characteristic, or the Flags characteristic
 *
 * @param[in] con_lid               Connection local index
 * @param[in] char_type             Characteristic type (see #hap_has_char_type enumeration)
 * @param[in] presets_instance_idx  Presets characteristic instance index
 * @param[in] enable                Indicate if sending of notifications must be enabled or not
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_set_cfg(uint8_t con_lid, uint8_t char_type, uint8_t presets_instance_idx, bool enable);
#endif //(GAF_DBG)

/**
 ****************************************************************************************
 * @brief Set name for a Preset exposed by a peer Server device
 *
 * @param[in] con_lid               Connection local index
 * @param[in] preset_idx            Preset index
 * @param[in] length                Length of Preset name
 * @param[in] p_name                Pointer to Preset name
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_set_preset_name(uint8_t con_lid, uint8_t preset_idx, uint8_t length, const uint8_t* p_name);

/**
 ****************************************************************************************
 * @brief Request peer Server device to update current active Preset
 *
 * @param[in] con_lid               Connection local index
 * @param[in] set_type              Set type (see #hap_hac_set_type enumeration)
 * @param[in] coordinated           Indicate if Server device must inform other member of the Coordinated Set it
 *                                  belongs to
 * @param[in] preset_idx            Preset index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_set_active_preset(uint8_t con_lid, uint8_t set_type, bool coordinated, uint8_t preset_idx);

/**
 ****************************************************************************************
 * @brief Enable Hearing Aid Profile block for use of the Hearing Aid Service as Client for a connected device with
 * which a bonding has been established during a previous connection
 *
 * @param[in] con_lid               Connection local index
 * @param[in] p_has_info            Pointer to Hearing Aid Service content description structure
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_hac_restore_bond_data(uint8_t con_lid, const hap_hac_has_t* p_has_info);

/// @} HAP_HAC_FUNCTION
#endif //(GAF_HAP_HAC)

#endif // HAP_HAC_H_

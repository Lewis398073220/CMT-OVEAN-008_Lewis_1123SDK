/**
 ****************************************************************************************
 *
 * @file hap_has.h
 *
 * @brief Hearing Aid Profile - Hearing Aid Service Server - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_HAS_H_
#define HAP_HAS_H_


/**
 ****************************************************************************************
 * @defgroup HAP_HAS Hearing Aid Service Server
 * @ingroup HAP
 * @brief Description of Hearing Aid Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAS_DEFINE Definitions
 * @ingroup HAP_HAS
 * @brief Definitions for Hearing Aid Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAS_ENUM Enumerations
 * @ingroup HAP_HAS
 * @brief Enumerations for Hearing Aid Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAS_NATIVE_API Native API
 * @ingroup HAP_HAS
 * @brief Description of Native API for Hearing Aid Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAS_CALLBACK Callback Functions
 * @ingroup HAP_HAS_NATIVE_API
 * @brief Description of callback functions for Hearing Aid Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_HAS_FUNCTION Functions
 * @ingroup HAP_HAS_NATIVE_API
 * @brief Description of functions for Hearing Aid Service Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hap.h"                // Hearing Aid Profile Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// @addtogroup HAP_HAS_DEFINE
/// @{

/// Maximum number of Preset Records that can be supported
#define HAP_HAS_PRESET_RECORDS_NB_MAX         (32)
/// Maximum number of Presets characteristics that can be supported
#define HAP_HAS_PRESETS_CHAR_NB_MAX           (8)

/// @} HAP_HAS_DEFINE

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup HAP_HAS_ENUM
/// @{

/// Configuration bit field meaning (see #hap_has_cfg_param_t structure)
enum hap_has_cfg_bf
{
    /// Indicate if Flags characteristic is supported (= 1) or not - Position
    HAP_HAS_CFG_FLAGS_SUPP_POS = 0,
    /// Indicate if Flags characteristic is supported (= 1) or not - Bit
    HAP_HAS_CFG_FLAGS_SUPP_BIT = CO_BIT(HAP_HAS_CFG_FLAGS_SUPP_POS),

    /// Indicate if sending of notifications is supported for Presets characteristic (= 1) or not - Position
    HAP_HAS_CFG_PRESETS_NTF_SUPP_POS = 1,
    /// Indicate if sending of notifications is supported for Presets characteristic (= 1) or not - Bit\n
    /// Meaningful only of at least one Presets characteristic is supported
    HAP_HAS_CFG_PRESETS_NTF_SUPP_BIT = CO_BIT(HAP_HAS_CFG_PRESETS_NTF_SUPP_POS),

    /// Indicate if sending of notifications is supported for Flags characteristic (= 1) or not - Position
    HAP_HAS_CFG_FLAGS_NTF_SUPP_POS = 2,
    /// Indicate if sending of notifications is supported for Flags characteristic (= 1) or not - Bit\n
    /// Meaningful only if Flags characteristic is supported
    HAP_HAS_CFG_FLAGS_NTF_SUPP_BIT = CO_BIT(HAP_HAS_CFG_FLAGS_NTF_SUPP_POS),
};

/// @} HAS_HAS_ENUM

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup HAP_HAS_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called when configuration for sending of notifications has been updated by a peer Client
 * device
 *
 * @param[in] con_lid               Connection local index
 * @param[in] cli_cfg_bf            Client configuration bit field
 * @param[in] presets_cli_cfg_bf    Client configuration bit field for Presets characteristic instances
 ****************************************************************************************
 */
typedef void (*hap_has_cb_bond_data)(uint8_t con_lid, uint8_t cli_cfg_bf, uint8_t presets_cli_cfg_bf);

/**
 ****************************************************************************************
 * @brief Callback function called when a peer Client device has updated name of a Preset
 *
 * @param[in] preset_lid            Preset local index
 * @param[in] con_lid               Connection local index
 * @param[in] length                Length of Preset name
 * @param[in] p_name                Pointer to Preset name
 ****************************************************************************************
 */
typedef void (*hap_has_cb_preset_name)(uint8_t preset_lid, uint8_t con_lid, uint8_t length, const uint8_t* p_name);

/**
 ****************************************************************************************
 * @brief Callback function called when a peer Client device requires to update the current active Preset
 * #hap_has_set_preset_active_cfm function shall be called by upper layer
 *
 * @param[in] con_lid               Connection local index
 * @param[in] preset_lid            Preset local index
 * @param[in] coordinated           Indicate if Server must inform other member of the Coordinated Set about the
 * new active Preset
 ****************************************************************************************
 */
typedef void (*hap_has_cb_preset_active_req)(uint8_t con_lid, uint8_t preset_lid, bool coordinated);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of callback functions for Hearing Aid Service Server module communication with upper layer
typedef struct hap_has_cb
{
    /// Callback function called when configuration for sending of notifications has been updated by a peer Client
    /// device
    hap_has_cb_bond_data cb_bond_data;
    /// Callback function called when a peer Client device has updated name of a Preset
    hap_has_cb_preset_name cb_preset_name;
    /// Callback function called when a peer Client device requires to update the current active Preset
    /// #hap_has_set_preset_active_cfm function shall be called by upper layer
    hap_has_cb_preset_active_req cb_preset_active_req;
} hap_has_cb_t;

/// @} HAP_HAS_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_HAP_HAS)
/// @addtogroup HAP_HAS_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Set bonding information for Hearing Aid Service after connection with a Client device with which a bonded
 * relationship had been established during a previous connection.
 *
 * @param[in] con_lid               Connection local index
 * @param[in] cli_cfg_bf            Client configuration bit field
 * @param[in] presets_cli_cfg_bf    Client configuration bit field for instances of Presets characteristics
 * @param[in] evt_cfg_bf            Event configuration bit field
 * @param[in] presets_evt_cfg_bf    Event configuration bit field for instances of Presets characteristics
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_has_restore_bond_data(uint8_t con_lid, uint8_t cli_cfg_bf, uint8_t presets_cli_cfg_bf,
                                   uint8_t evt_cfg_bf, uint8_t presets_evt_cfg_bf);

/**
 ****************************************************************************************
 * @brief Configure a preset\n
 * Can be used only if at least one Presets characteristic is supported.
 *
 * @param[in] preset_lid            Preset local index
 * @param[in] presets_instance_idx  Presets characteristic instance that will contain information about the Preset
 * @param[in] read_only             Indicate if Preset name can be written or not by a Client device
 * @param[in] length                Length of Preset Name. Up to HAP_HAS_PRESET_NAME_LEN_MAX.
 * @param[in] p_name                Pointer to Preset Name
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_has_configure_preset(uint8_t preset_lid, uint8_t presets_instance_idx, bool read_only, uint8_t length,
                                  const uint8_t* p_name);

/**
 ****************************************************************************************
 * @brief Update index of the active preset.
 * Can be used only if at least one Presets characteristic is supported.
 *
 * @param[in] preset_lid            Preset local index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_has_set_active_preset(uint8_t preset_lid);

/**
 ****************************************************************************************
 * @brief Update Preset Coordination Support status.\n
 * Can be used only if Flags characteristic is supported and if sending of notification is supported for the
 * characteristic.
 *
 * @param[in] supported             Indicate if Preset Coordination is supported or not
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_has_set_coordination_support(bool supported);

/**
 ****************************************************************************************
 * @brief Confirmation for @see hap_has_cb_preset_active_req callback function
 *
 * @param[in] accept                Indicate if request is accepted or not
 ****************************************************************************************
 */
void hap_has_set_preset_active_cfm(bool accept);

/**
 ****************************************************************************************
 * @return If use of Server Role for Hearing Aid Service has been configured
 ****************************************************************************************
 */
bool hap_has_is_configured();

/// @} HAP_HAS_FUNCTION
#endif //(GAF_HAP_HAS)

#endif // HAP_HAS_H_

/**
 ****************************************************************************************
 *
 * @file hap.h
 *
 * @brief Hearing Aid Profile - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_H_
#define HAP_H_

/**
 ****************************************************************************************
 * @defgroup HAP Hearing Aid Profile (HAP)
 * @ingroup GAF
 * @brief Description of Hearing Aid Profile block
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_COMMON Common
 * @ingroup HAP
 * @brief Description of Hearing Aid Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_COMMON_DEFINE Definitions
 * @ingroup HAP_COMMON
 * @brief Definitions for Hearing Aid Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_COMMON_ENUM Enumerations
 * @ingroup HAP_COMMON
 * @brief Enumerations for Hearing Aid Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_COMMON_STRUCT Structures
 * @ingroup HAP_COMMON
 * @brief Structures for Hearing Aid Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup HAP_COMMON_NATIVE_API Native API
 * @ingroup HAP_COMMON
 * @brief Description of Native API for Hearing Aid Profile Common module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf.h"             // GAF Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// @addtogroup HAP_COMMON_DEFINE
/// @{

/// Maximal length of Preset name value
#define HAP_HAS_PRESET_NAME_LEN_MAX         (29)
/// Minimal Preset Index value
#define HAP_HAS_PRESET_IDX_MIN              (1)

/// @} HAP_COMMON_DEFINE

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup HAP_COMMON_ENUM
/// @{

/// Characteristic type values for Hearing Aid Service
enum hap_has_char_type
{
    /// Active Preset characteristic
    HAP_HAS_CHAR_TYPE_ACTIVE_PRESET = 0,
    /// Flags characteristic
    HAP_HAS_CHAR_TYPE_FLAGS,

    /// Maximum value for characteristics supporting sending of notifications
    HAP_HAS_CHAR_TYPE_NTF_MAX,

    /// Preset Control Point characteristic
    HAP_HAS_CHAR_TYPE_PRESET_CP = HAP_HAS_CHAR_TYPE_NTF_MAX,
    /// Presets characteristic
    HAP_HAS_CHAR_TYPE_PRESETS,

    /// Maximum value
    HAP_HAS_CHAR_TYPE_MAX,
};

/// Error code values for Preset Control Point characteristic
enum hap_has_cp_error
{
    /// Opcode Not Supported
    HAP_HAS_CP_ERROR_OPCODE_NOT_SUPPORTED = 0x80,
    /// Write Name Not Allowed
    HAP_HAS_CP_ERROR_WRITE_NAME_NOT_ALLOWED,
    /// Index Out of Range
    HAP_HAS_CP_ERROR_INDEX_OUT_OF_RANGE,
    /// Preset Coordinated Not Supported
    HAP_HAS_CP_ERROR_PRESET_COORD_NOT_SUPPORTED,
    /// Preset Operation Not Possible
    HAP_HAS_CP_ERROR_PRESET_OP_NOT_POSSIBLE,
};

/// Flags characteristic value bit field meaning
enum hap_has_flags_bf
{
    /// Indicate if Preset Coordination feature is supported (= 1) or not - Position
    HAP_HAS_FLAGS_PRESET_COORD_SUPPORT_POS = 0,
    /// Indicate if Preset Coordination feature is supported (= 1) or not - Bit
    HAP_HAS_FLAGS_PRESET_COORD_SUPPORT_BIT = CO_BIT(HAP_HAS_FLAGS_PRESET_COORD_SUPPORT_POS),

    /// Mask providing RFU bits
    HAP_HAS_FLAGS_RFU_MASK = 0xFE,
};

/// Module type values for Hearing Aid Profile block
enum hap_module_type
{
    /// Common Module
    HAP_MODULE_COMMON = 0,
    /// Hearing Aid Service Server Module
    HAP_MODULE_HAS,
    /// Hearing Aid Service Client Module
    HAP_MODULE_HAC,

    /// Maximum value
    HAP_MODULE_MAX,
};

/// Configuration bit field meaning (see #hap_configure function)
enum hap_cfg_bf
{
    /// Indicate if Server role is supported for Hearing Aid Service - Position
    HAP_CFG_HAS_SUPP_POS = 0,
    /// Indicate if Server role is supported for Hearing Aid Service - Bit
    HAP_CFG_HAS_SUPP_BIT = CO_BIT(HAP_CFG_HAS_SUPP_POS),

    /// Indicate if Client role is supported for Hearing Aid Service - Position
    HAP_CFG_HAC_SUPP_POS = 1,
    /// Indicate if Client role is supported for Hearing Aid Service - Bit
    HAP_CFG_HAC_SUPP_BIT = CO_BIT(HAP_CFG_HAC_SUPP_POS),
};

/// @} HAP_COMMON_ENUM

/*
 * TYPE DEFINTIONS
 ****************************************************************************************
 */

/// @addtogroup HAP_COMMON_STRUCT
/// @{

typedef struct hap_has_cb hap_has_cb_t;
typedef struct hap_hac_cb hap_hac_cb_t;

/// Configuration Parameter structure for Hearing Aid Service Server
typedef struct hap_has_cfg_param
{
    /// Configuration bit field (see #hap_has_cfg_bf enumeration)
    uint8_t cfg_bf;
    /// Preferred MTU\n
    /// Values for 0 to 63 are equivalent to 64
    uint8_t pref_mtu;
    /// Required start handle\n
    /// If set to GATT_INVALID_HANDLE, the start handle will be automatically chosen
    uint16_t shdl;
    /// Number of Presets characteristics in the Service\n
    /// Up to 8
    uint8_t nb_presets_chars;
    /// Number of Presets\n
    /// Meaningful only if there is at least one Preset characteristic\n
    /// Up to 32
    uint8_t nb_presets;
    /// Flags bit field value (see #hap_has_flags_bf enumeration)\n
    /// Meaningful only if Flags characteristic is set as supported in cfg_bf
    uint8_t flags_bf;
} hap_has_cfg_param_t;

/// Configuration structure for Hearing Aid Service Server
typedef struct hap_has_cfg
{
    /// Configuration parameters
    hap_has_cfg_param_t cfg_param;
    /// Pointer to set of callback functions for communication with upper layer
    const hap_has_cb_t* p_cb;
} hap_has_cfg_t;

/// Configuration Parameter structure for Hearing Aid Service Client
typedef struct hap_hac_cfg_param
{
    /// Preferred MTU
    /// Values for 0 to 63 are equivalent to 64
    uint8_t pref_mtu;
} hap_hac_cfg_param_t;

/// Configuration structure for Hearing Aid Service Client
typedef struct hap_hac_cfg
{
    /// Configuration parameters
    hap_hac_cfg_param_t cfg_param;
    /// Pointer to set of callback functions for communication with upper layer
    const hap_hac_cb_t* p_cb;
} hap_hac_cfg_t;

/// @} HAP_COMMON_STRUCT

/*
 * API FUNCTION DEFINITION
 ****************************************************************************************
 */

#if (GAF_HAP)
/// @addtogroup HAP_COMMON_NATIVE_API
/// @{

/**
 ****************************************************************************************
 * @brief Configure Hearing Aid Profile block
 *
 * @param[in] cfg_bf            Configuration bit field (see #hap_cfg_bf enumeration)
 * @param[in] p_cfg_has         Pointer to Configuration structure for Hearing Aid Service Server\n
 *                              Cannot be NULL if support of Server Role for Hearing Aid Service is indicated in
 *                              cfg_bf
 * @param[in] p_cfg_hac         Pointer to Configuration structure for Hearing Aid Service Client\n
 *                              Cannot be NULL if support of Client Role for Hearing Aid Service is indicated in
 *                              cfg_bf
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t hap_configure(uint8_t cfg_bf, const hap_has_cfg_t* p_cfg_has, const hap_hac_cfg_t* p_cfg_hac);

/// @} HAP_COMMON_NATIVE_API
#endif //(GAF_HAP)

#endif // HAP_H_


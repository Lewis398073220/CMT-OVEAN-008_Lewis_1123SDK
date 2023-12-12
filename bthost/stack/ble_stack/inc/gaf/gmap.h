/**
 ****************************************************************************************
 *
 * @file gmap.h
 *
 * @brief Gaming Audio Profile - Definitions
 *
 * GmapCopyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

#ifndef GMAP_H_
#define GMAP_H_

/**
 ****************************************************************************************
 * @defgroup GMAP Gaming Audio Profile (GMAP)
 * @ingroup GAF
 * @brief Description of Gaming Audio Profile block
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_COMMON Common
 * @ingroup GMAP
 * @brief Description of Gaming Audio Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_COMMON_ENUM Enumerations
 * @ingroup GMAP_COMMON
 * @brief Enumerations for Gaming Audio Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_COMMON_STRUCT Structures
 * @ingroup GMAP_COMMON
 * @brief Structures for Gaming Audio Profile Common module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_COMMON_NATIVE_API Native API
 * @ingroup GMAP_COMMON
 * @brief Description of Native API for Gaming Audio Profile Common module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf.h"             // GAF Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup GMAP_COMMON_ENUM
/// @{

/// Module type values for Gaming Audio Profile block
enum gmap_module_type
{
    /// Common Module
    GMAP_MODULE_COMMON = 0,
    /// Gaming Audio Service Server Module
    GMAP_MODULE_GMAS = 1,
    /// Gaming Audio Service Client Module
    GMAP_MODULE_GMAC = 2,

    /// Maximum value
    GMAP_MODULE_MAX,
};

/// Configuration bit field meaning (see #gmap_configure function)
enum gmap_cfg_bf
{
    /// Indicate if Server role is supported for Gaming Audio Service
    GMAP_CFG_GMAS_SUPP_POS = 0,
    GMAP_CFG_GMAS_SUPP_BIT = CO_BIT(GMAP_CFG_GMAS_SUPP_POS),

    /// Indicate if Client role is supported for Gaming Audio Service
    GMAP_CFG_GMAC_SUPP_POS = 1,
    GMAP_CFG_GMAC_SUPP_BIT = CO_BIT(GMAP_CFG_GMAC_SUPP_POS),
};

/// Position of fields in Role characteristic value
enum gmap_role_pos
{
    /// Role field
    GMAP_ROLE_POS_ROLE = 0,

    /// Length of Role characteristic value
    GMAP_ROLE_LEN = 1,
};

/// Position of fields in UGT Feature characteristic value
enum gmap_ugt_feat_pos
{
    /// UGT Feature field
    GMAP_UGT_FEAT_POS = 0,

    /// Length of UGT Feature characteristic value
    GMAP_UGT_FEAT_LEN = 1,
};

/// Characteristic type values for Gaming Audio Service
enum gmap_char_type
{
    /// GMAP Role characteristic
    GMAP_CHAR_TYPE_ROLE = 0,
    /// GMAP UGT feature characteristic
    GMAP_CHAR_TYPE_UGT_FEAT = 1,

    GMAP_CHAR_TYPE_MAX,
};

/// GMAP Role characteristic bit field meaning
enum gmap_role_bf
{
    /// Indicate if Server supports UGG role (= 1) or not - Position
    GMAP_ROLE_UGG_POS = 0,
    /// Indicate if Server supports UGG role (= 1) or not - Bit
    GMAP_ROLE_UGG_BIT = CO_BIT(GMAP_ROLE_UGG_POS),

    /// Indicate if Server supports UGT role (= 1) or not - Position
    GMAP_ROLE_UGT_POS = 1,
    /// Indicate if Server supports UGT role (= 1) or not - Bit
    GMAP_ROLE_UGT_BIT = CO_BIT(GMAP_ROLE_UGT_POS),

    /// Indicate if Server supports BGS role (= 1) or not - Position
    GMAP_ROLE_BGS_POS = 2,
    /// Indicate if Server supports BGS role (= 1) or not - Bit
    GMAP_ROLE_BGS_BIT = CO_BIT(GMAP_ROLE_BGS_POS),

    /// Indicate if Server supports BGR role (= 1) or not - Position
    GMAP_ROLE_BGR_POS = 3,
    /// Indicate if Server supports BGR role (= 1) or not - Bit
    GMAP_ROLE_BGR_BIT = CO_BIT(GMAP_ROLE_BGR_POS),

    /// Mask indicating that all roles are supported - LSB
    GMAP_ROLE_ALLSUPP_LSB = 0,
    /// Mask indicating that all roles are supported
    GMAP_ROLE_ALLSUPP_MASK = 0x0F,

    /// Mask indicating RFU bits - LSB
    GMAP_ROLE_RFU_LSB = 4,
    /// Mask indicating RFU bits
    GMAP_ROLE_RFU_MASK = 0xF0,
};

/// GMAP UGT Feature characteristic bit field meaning
enum gmap_ugt_feature_bf
{
    /// Indicate if Server supports UGT Source (= 1) or not - Position
    GMAP_UGT_SRC_POS        = 0,
    /// Indicate if Server supports UGT Source (= 1) or not - Bit
    GMAP_UGT_SRC_BIT        = CO_BIT(GMAP_UGT_SRC_POS),

    /// Indicate if Server supports UGT Capture (= 1) or not - Position
    GMAP_UGT_CAPTURE_POS    = 1,
    /// Indicate if Server supports UGT Capture (= 1) or not - Bit
    GMAP_UGT_CAPTURE_BIT    = CO_BIT(GMAP_UGT_CAPTURE_POS),

    /// Indicate if Server supports UGT Sink (= 1) or not - Position
    GMAP_UGT_SINK_POS       = 2,
    /// Indicate if Server supports UGT Sink (= 1) or not - Bit
    GMAP_UGT_SINK_BIT       = CO_BIT(GMAP_UGT_SINK_POS),

    /// Indicate if Server supports UGT Multiplex (= 1) or not - Position
    GMAP_UGT_MULTIPLEX_POS  = 3,
    /// Indicate if Server supports UGT Multiplex (= 1) or not - Bit
    GMAP_UGT_MULTIPLEX_BIT  = CO_BIT(GMAP_UGT_MULTIPLEX_POS),

    /// Indicate if Server supports UGT Dual Sink (= 1) or not - Position
    GMAP_UGT_DUAL_SNK_POS   = 4,
    /// Indicate if Server supports UGT Dual Sink (= 1) or not - Bit
    GMAP_UGT_DUAL_SNK_BIT   = CO_BIT(GMAP_UGT_DUAL_SNK_POS),

    /// Indicate if Server supports UGT Dual Source (= 1) or not - Position
    GMAP_UGT_DUAL_SRC_POS   = 5,
    /// Indicate if Server supports UGT Dual Source (= 1) or not - Bit
    GMAP_UGT_DUAL_SRC_BIT   = CO_BIT(GMAP_UGT_DUAL_SRC_POS),

    /// Mask indicating that all UGT feat are supported - LSB
    GMAP_UGT_FEAT_ALLSUPP_LSB = 0,
    /// Mask indicating that all roles are supported
    GMAP_UGT_FEAT_ALLSUPP_MASK = 0x3F,

    /// Mask indicating RFU bits - LSB
    GMAP_UGT_FEAT_RFU_LSB = 6,
    /// Mask indicating RFU bits
    GMAP_UGT_FEAT_RFU_MASK = 0xC0,
};

/// @} GMAP_COMMON_ENUM

/*
 * TYPE DEFINTIONS
 ****************************************************************************************
 */

/// @addtogroup GMAP_COMMON_STRUCT
/// @{

typedef struct gmap_gmac_cb gmap_gmac_cb_t;

/// Configuration Parameter structure for Gaming Audio Service Server
typedef struct gmap_gmas_cfg_param
{
    /// Bit field indicating supported GMAP roles (see #gmap_role_bf enumeration)
    uint8_t role_bf;
    /// Bit field indicating supported GMAP UGT Features (see #gmap_ugt_feature_bf enumeration)
    uint8_t ugt_feature;
    /// Required start handle\n
    /// If set to GATT_INVALID_HANDLE, the start handle will be automatically chosen
    uint16_t shdl;
} gmap_gmas_cfg_param_t;

/// @} GMAP_COMMON_STRUCT

/*
 * EXTERNAL FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_GMAP)
/// @addtogroup GMAP_COMMON_NATIVE_API
/// @{

/**
 ****************************************************************************************
 * @brief Configure Telephony and Media Profile block
 *
 * @param[in] cfg_bf            Configuration bit field (see #gmap_cfg_bf enumeration)
 * @param[in] p_cfg_param_gmas  Pointer to Configuration Parameters for Gaming Audio Service Server\n
 *                              Cannot be NULL if support of Server Role for Gaming Audio Service is
 *                              indicated as supported in cfg_bf
 * @param[in] p_cb_gmac         Pointer to set of callback functions for Gaming Audio Service Client\n
 *                              Cannot be NULL if support of Client Role for Gaming Audio Service is
 *                              indicated as supported in cfg_bf
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gmap_configure(uint8_t cfg_bf, const gmap_gmas_cfg_param_t* p_cfg_param_gmas,
                        const gmap_gmac_cb_t* p_cb_gmac);

/**
 ****************************************************************************************
 * @brief Check validity of supported roles bit field
 *
 * @param[in] role_bf           Roles bit field (see #gmap_role_bf enumeration)
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gmap_check_role(uint8_t role_bf);

/**
 ****************************************************************************************
 * @brief Check validity of supported ugt feature bit field
 *
 * @param[in] role_bf           UGT Feature bit field (see #gmap_ugt_feature_bf enumeration)
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gmap_check_ugt_feat(uint8_t ugt_feat_bf);

/// @} GMAP_COMMON_NATIVE_API
#endif //(GAF_GMAP)


#endif // GMAP_H_


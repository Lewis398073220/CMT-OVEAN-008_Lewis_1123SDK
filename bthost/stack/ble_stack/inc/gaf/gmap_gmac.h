/**
 ****************************************************************************************
 *
 * @file gmap_gmac.h
 *
 * @brief Gaming Audio Profile - Gaming Audio Service Client - Definitions
 *
 * GmapCopyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

#ifndef GMAP_GMAC_H_
#define GMAP_GMAC_H_


/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC Gaming Audio Service Client
 * @ingroup GMAP
 * @brief Description of Gaming Audio Service Client module
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC_ENUM Enumerations
 * @ingroup GMAP_GMAC
 * @brief Enumerations for Gaming Audio Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC_STRUCT Structures
 * @ingroup GMAP_GMAC
 * @brief Structures for Gaming Audio Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC_NATIVE_API Native API
 * @ingroup GMAP_GMAC
 * @brief Description of Native API for Gaming Audio Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC_CALLBACK Callback Functions
 * @ingroup GMAP_GMAC_NATIVE_API
 * @brief Description of callback functions for Gaming Audio Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC_FUNCTION Functions
 * @ingroup GMAP_GMAC_NATIVE_API
 * @brief Description of functions for Gaming Audio Service Client module
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gmap.h"               // Gaming Audio Profile Definitions
#include "prf_types.h"          // Profile common types
#include "gaf.h"                // Generic Audio Framework Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup GMAP_GMAC_ENUM
/// @{

/// List of command type values for Gaming Audio Profile Client module
enum gmap_gmac_cmd_type
{
    /// Discover (see #gmap_gmac_discover function)
    GMAP_GMAC_CMD_TYPE_DISCOVER = 0,
    /// Get Role (see #gmap_gmac_get_role function)
    GMAP_GMAC_CMD_TYPE_GET_ROLE,
    /// Get Feature (see #gmap_gmac_get_feature function)
    GMAP_GMAC_CMD_TYPE_GET_UGT_FEAT,
};

/// @} GMAP_GMAC_ENUM

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// @addtogroup GMAP_GMAC_STRUCT
/// @{

/// Gaming Audio Service content description structure
typedef struct gmap_gmac_gmas
{
    /// Service description
    prf_svc_t svc_info;
    /// Characteristics description
    prf_char_t char_info[GMAP_CHAR_TYPE_MAX];
} gmap_gmac_gmas_t;

/// @} GMAP_GMAC_STRUCT

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup GMAP_GMAC_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called each time a Gaming Audio Client command has been completed
 *
 * @param[in] cmd_type      Command type (see #gmap_gmac_cmd_type enumeration)
 * @param[in] status        Status
 * @param[in] con_lid       Local index
 ****************************************************************************************
 */
typedef void (*gmap_gmac_cb_cmp_evt)(uint8_t cmd_type, uint16_t status, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Callback function called when Gaming Audio Service has been discovered
 *
 * @param[in] con_lid       Connection local index
 * @param[in] p_gmas_info   Pointer to Gaming Audio Service content description
 ****************************************************************************************
 */
typedef void (*gmap_gmac_cb_bond_data)(uint8_t con_lid, const gmap_gmac_gmas_t* p_gmas_info);

/**
 ****************************************************************************************
 * @brief Callback function called when a service changed indication has been received from a Server device
 *
 * @param[in] con_lid       Connection local index
 ****************************************************************************************
 */
typedef void (*gmap_gmac_cb_svc_changed)(uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Callback function called when GMAP Role characteristic value has been received from a Server device
 *
 * @param[in] con_lid       Connection local index
 * @param[in] role_bf       Bitmap of supported GMAP role (see #gmap_role_bf enumeration)
 ****************************************************************************************
 */
typedef void (*gmap_gmac_cb_role)(uint8_t con_lid, uint8_t role_bf);

/**
 ****************************************************************************************
 * @brief Callback function called when GMAP UGT Feature characteristic value has been received from a Server device
 *
 * @param[in] con_lid       Connection local index
 * @param[in] ugt_feat_bf   Bitmap of supported GMAP UGT Feature (see #gmap_ugt_feature_bf enumeration)
 ****************************************************************************************
 */
typedef void (*gmap_gmac_cb_ugt_feat)(uint8_t con_lid, uint8_t ugt_feat_bf);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of callback functions for Telephone and Media Audio Service Client module communication with upper layer
typedef struct gmap_gmac_cb
{
    /// Callback function called when a command has been completed
    gmap_gmac_cb_cmp_evt cb_cmp_evt;
    /// Callback function called when Gaming Audio Service has been discovered
    gmap_gmac_cb_bond_data cb_bond_data;
    /// Callback function called when a service changed indication has been received from a Server device
    gmap_gmac_cb_svc_changed cb_svc_changed;
    /// Callback function called when Role characteristic value has been received from a Server device
    gmap_gmac_cb_role cb_role;
    /// Callback function called when Role characteristic value has been received from a Server device
    gmap_gmac_cb_ugt_feat cb_ugt_feat;
} gmap_gmac_cb_t;

/// @} GMAP_GMAC_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_GMAP_GMAC)
/// @addtogroup GMAP_GMAC_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Enable use of Gaming Audio Profile as Client when discovery has already been performed
 *
 * @param[in] con_lid           Connection local index
 * @param[in] p_gmas_info       Pointer to content description of Gaming Audio Service
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gmap_gmac_restore_bond_data(uint8_t con_lid, const gmap_gmac_gmas_t* p_gmas_info);

/**
 ****************************************************************************************
 * @brief Enable use of Gaming Audio Profile as Client for a connected device with which no bonding has
 * been established during a previous connection.
 *
 * @param[in] con_lid           Connection local index
 * @param[in] shdl              Start handle for the discovery. Set GATT_INVALID_HDL if not provided
 * @param[in] ehdl              End handle for the discovery. Set GATT_INVALID_HDL if not provided
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gmap_gmac_discover(uint8_t con_lid, uint16_t shdl, uint16_t ehdl);

#if (GAF_DBG)
/**
 ****************************************************************************************
 * @brief Get roles supported by the peer device.
 *
 * @param[in] con_lid           Connection local index
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t gmap_gmac_get_role(uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Get UGT Features supported by the peer device.
 *
 * @param[in] con_lid           Connection local index
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t gmap_gmac_get_ugt_feat(uint8_t con_lid);
#endif //(GAF_DBG)

/// @} GMAP_GMAC_FUNCTION
#endif //(GAF_GMAP_GMAC)

#endif // GMAP_GMAC_H_

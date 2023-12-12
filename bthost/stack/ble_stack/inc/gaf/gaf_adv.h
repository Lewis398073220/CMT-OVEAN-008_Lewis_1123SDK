/**
 ****************************************************************************************
 *
 * @file gaf_adv.h
 *
 * @brief Generic Audio Framework - Advertiser - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef GAF_ADV_H_
#define GAF_ADV_H_


/**
 ****************************************************************************************
 * @defgroup GAF_ADV Advertiser module
 * @ingroup GAF_TB
 * @brief Description of Generic Audio Framework Advertiser module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GAF_ADV_ENUM Enumerations
 * @ingroup GAF_ADV
 * @brief Enumerations of Generic Audio Framework Advertiser module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GAF_ADV_NATIVE_API Native API
 * @ingroup GAF_ADV
 * @brief Description of Native API for Generic Audio Framework Advertiser module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GAF_ADV_CALLBACK Callback Functions
 * @ingroup GAF_ADV_NATIVE_API
 * @brief Description of callback functions for Generic Audio Framework Advertiser module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GAF_ADV_FUNCTION Functions
 * @ingroup GAF_ADV_NATIVE_API
 * @brief Description of functions for Generic Audio Framework Advertiser module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf_cfg.h"                // Generic Audio Framework Configuration
#include "gaf.h"                    // GAF Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup GAF_ADV_ENUM
/// @{

/// List of command type value for Generic Audio Framework Advertiser module
enum gaf_adv_cmd_type
{
    /// Start advertising
    GAF_ADV_CMD_TYPE_START = 0,
    /// Stop advertising
    GAF_ADV_CMD_TYPE_STOP,
};

/// Advertising configuration bit field meaning (see #gaf_adv_start function)
enum gaf_adv_cfg_bf
{
    /// Use Limited Discoverable or General Discoverable mode - Position
    GAF_ADV_CFG_LIMITED_DISCOVERABLE_POS = 0,
    /// Use Limited Discoverable or General Discoverable mode - Bit
    GAF_ADV_CFG_LIMITED_DISCOVERABLE_BIT = CO_BIT(GAF_ADV_CFG_LIMITED_DISCOVERABLE_POS),

    /// Use General Announcement or Targeted Announcement - Position
    GAF_ADV_CFG_GENERAL_ANNOUNCEMENT_POS = 1,
    /// Use General Announcement or Targeted Announcement - Bit
    GAF_ADV_CFG_GENERAL_ANNOUNCEMENT_BIT = CO_BIT(GAF_ADV_CFG_GENERAL_ANNOUNCEMENT_POS),

    /// Own address type - Position
    GAF_ADV_CFG_PRIVACY_POS = 2,
    /// Own address type - Mask
    GAF_ADV_CFG_PRIVACY_BIT = CO_BIT(GAF_ADV_CFG_PRIVACY_POS),
};

/// Advertising stop reason values
enum gaf_adv_stop_reason
{
    /// Required by Upper Layer
    GAF_ADV_STOP_REASON_UL = 0,
    /// Internal error
    GAF_ADV_STOP_REASON_INTERNAL_ERROR,
    /// Timeout
    GAF_ADV_STOP_REASON_TIMEOUT,
    /// Connection has been established
    GAF_ADV_STOP_REASON_CON_ESTABLISHED,
};

/// @} GAF_ADV_ENUM

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup GAF_ADV_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called when handling of a command has been completed
 *
 * @param[in] cmd_type      Command type (see #gaf_adv_cmd_type enumeration)
 * @param[in] status        Status
 ****************************************************************************************
 */
typedef void (*gaf_adv_cb_cmp_evt)(uint8_t cmd_type, uint16_t status);

/**
 ****************************************************************************************
 * @brief Callback function called when advertising has been stopped
 *
 * @param[in] reason        Stop reason (see #gaf_adv_stop_reason enumeration)
 ****************************************************************************************
 */
typedef void (*gaf_adv_cb_stopped)(uint8_t reason);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of callback functions for Generic Audio Framework Advertiser module
typedef struct gaf_adv_cb
{
    /// Callback function called when handling of a command has been completed
    gaf_adv_cb_cmp_evt cb_cmp_evt;
    /// Callback function called when advertising has been stopped
    gaf_adv_cb_stopped cb_stopped;
} gaf_adv_cb_t;

/// @} GAF_ADV_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup GAF_ADV_FUNCTION
/// @{

#if (GAF_ADV)

/**
 ****************************************************************************************
 * @brief Update advertising configuration used for started advertising procedures\n
 * Advertising is started with an advertising interval set to quick_adv_intv_ms\n
 * If a connection is not established within 30 seconds from the start of this procedure, the advertising interval is
 * updated (adv_intv_ms value is used) so that power consumption is reduced.
 *
 * @param[in] quick_adv_intv_ms     Advertising interval in milliseconds for quicker connection setup\n
 *                                  Set to 30ms by default
 * @param[in] adv_intv_ms           Advertising interval in milliseconds for reduced power advertising\n
 *                                  Set to 150ms by default
 * @param[in] phy_prim              Indicate on which PHY primary advertising has to be performed\n
 *                                  LE 1M PHY is used by default\n
 *                                  LE 2M PHY is not allowed
 * @param[in] phy_second            Indicate on which PHY secondary advertising has to be performed\n
 *                                  LE 1M PHY is used by default
 * @param[in] chnl_map              Channel Map
 * @param[in] max_tx_pwr            Maximum number of advertising events the controller can skip before sending the
 *                                  AUX_ADV_IND packets
 * @param[in] max_skip              Maximum power level at which the advertising packets have to be transmitted
 *                                  (between -127 and 126 dBm)
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gaf_adv_set_params(uint8_t quick_adv_intv_ms, uint8_t adv_intv_ms, uint8_t phy_prim, uint8_t phy_second,
                            uint8_t chnl_map, uint8_t max_tx_pwr, uint8_t max_skip);

/**
 ****************************************************************************************
 * @brief Start advertising\n
 * gaf_adv_cb_cmp_evt callback function called once procedure has been completed with #GAF_ADV_CMD_TYPE_START
 * command type value\n
 * gaf_adv_cb_adv_timeout callback function called at end of timeout duration if duration is set to a non-zero value
 *
 * Flags AD data type is included in the advertising data\n
 * Service UUID AD data type is included in the advertising data:\n
 *     - If use of BAP Unicast Server role has been configured, ASCS UUID is inserted in the Service UUID AD data
 *     type\n
 *     - If use of BAP Capabilities Server role has been configured, PACS UUID is inserted in the Service UUID AD
 *     data type\n
 *     - If use of HAP Server role has been configured, HAS UUID is inserted in the Service UUID AD data type\n
 *     - If use of TMAP Server role has been configured, TMAS UUID is inserted in the Service UUID AD data type\n
 * Appearance AD data type is included in the advertising data:\n
 *     - If use of TMAP Server role has been configured\n
 *     - If use of HAP Server role has been configured\n
 * Service Data AD data type is included in the advertising data:\n
 *     - If use of BAP Unicast Server role has been configured\n
 * Only one RSI AD data type is included in the advertising data:\n
 *     - If use of CSIP Set Member role has been configured\n
 *     - If several instances of the Coordinated Set Identification Service are supported (if device is part of
 *     several Sets), included RSI is the RSI for the Set bound with the CAS.
 *
 * @param[in] cfg_bf            Configuration bit field (see #gaf_adv_cfg_bf enumeration)
 * @param[in] timeout_s         Timeout duration in seconds\n
 *                              0 means that the scan will last until #GAF_ADV_STOP command is used
 * @param[in] adv_sid           Advertising SID
 * @param[in] adv_data_len      Length of additional advertising data
 * @param[in] p_adv_data        Pointer to additional advertising data
 * @param[in] p_metadata        Pointer to Metadata structure\n
 *                              Meaningful only if ASCS is supported
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gaf_adv_start(uint8_t cfg_bf, uint8_t timeout_s, uint8_t adv_sid, uint16_t adv_data_len,
                       const uint8_t* p_adv_data, const gaf_ltv_t* p_metadata);

/**
 ****************************************************************************************
 * @brief Stop advertising\n
 * #gaf_adv_cb_cmp_evt callback function called once procedure has been completed with #GAF_ADV_CMD_TYPE_STOP
 * command type value
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t gaf_adv_stop(void);

#endif //(GAF_ADV)

/// @} GAF_ADV_FUNCTION

#endif // GAF_ADV_H_

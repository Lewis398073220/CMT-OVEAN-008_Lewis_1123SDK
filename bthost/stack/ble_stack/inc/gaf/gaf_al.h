/**
 ****************************************************************************************
 *
 * @file gaf_al.h
 *
 * @brief GAF adaptation layer of the Generic Access profile - Native API
 *
 * Copyright (C) RivieraWaves 2009-2021
 *
 ****************************************************************************************
 */


#ifndef _GAF_AL_H_
#define _GAF_AL_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gaf_cfg.h"

#if (GAF_AL)

#include "co_buf.h"     // buffer usage
#include "gap.h"        // GAP types
#include "gapm_le_adv.h" // GAPM definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// Default TX power used for advertising
#define GAF_AL_ADV_TX_POWER_DEFAULT     (-70)

/// Maximum AD Type pattern size.
#define GAF_AL_SCAN_PATTERN_SIZE_MAX     (5)


/// type of advertising data
enum gaf_al_adv_data_type
{
    /// Advertising data
    GAF_AL_ADV_DATA_ADV,
    /// Periodic Advertising data
    GAF_AL_ADV_DATA_PER_ADV,
    /// Scan response data
    GAF_AL_ADV_DATA_SCAN_RSP,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Advertising parameters
typedef struct gaf_al_adv_param
{
    /// Minimum advertising interval in multiple of 0.625ms. Must be higher than 20ms.
    uint32_t     adv_intv_min_slot;
    /// Maximum advertising interval in multiple of 0.625ms. Must be higher than 20ms.
    uint32_t     adv_intv_max_slot;
    /// Discovery mode (see #gapm_adv_disc_mode enumeration)
    uint8_t      disc_mode;
    /// Own address type (see #gapm_own_addr enumeration)
    uint8_t      own_addr_type;
    /// Channel Map
    uint8_t      chnl_map;
    /// PHY for primary advertising. Only LE 1M and LE Codec PHYs are allowed
    uint8_t      phy_prim;
    /// PHY for secondary advertising
    uint8_t      phy_second;
    /// Maximum number of advertising events the controller can skip before sending the
    /// AUX_ADV_IND packets. 0 means that AUX_ADV_IND PDUs shall be sent prior each
    /// advertising events
    uint8_t      max_skip;
    /// Advertising SID
    uint8_t      adv_sid;
    /// Maximum power level at which the advertising packets have to be transmitted
    /// (between -127 and 126 dBm)
    int8_t       max_tx_pwr;
} gaf_al_adv_param_t;

/// Periodic Advertising parameters
typedef struct gaf_al_per_adv_param
{
    /// Minimum advertising interval in multiple of 0.625ms. Must be higher than 20ms.
    uint32_t     adv_intv_min_slot;
    /// Maximum advertising interval in multiple of 0.625ms. Must be higher than 20ms.
    uint32_t     adv_intv_max_slot;
    /// Minimum Periodic Advertising interval in multiple of 1.25ms. Must be higher than 7.5ms
    uint32_t     per_adv_intv_min_frame;
    /// Maximum Periodic Advertising interval in multiple of 1.25ms. Must be higher than 7.5ms
    uint32_t     per_adv_intv_max_frame;
    /// Discovery mode (see #gapm_adv_disc_mode enumeration)
    uint8_t      disc_mode;
    /// Own address type (see #gapm_own_addr enumeration)
    uint8_t      own_addr_type;
    /// Channel Map
    uint8_t      chnl_map;
    /// PHY for primary advertising. Only LE 1M and LE Codec PHYs are allowed
    uint8_t      phy_prim;
    /// PHY for secondary advertising
    uint8_t      phy_second;
    /// Maximum number of advertising events the controller can skip before sending the
    /// AUX_ADV_IND packets. 0 means that AUX_ADV_IND PDUs shall be sent prior each
    /// advertising events
    uint8_t      max_skip;
    /// Advertising SID
    uint8_t      adv_sid;
    /// Maximum power level at which the advertising packets have to be transmitted
    /// (between -127 and 126 dBm)
    int8_t       max_tx_pwr;
} gaf_al_per_adv_param_t;

/// Adaptation layer scan parameters
typedef struct gaf_al_scan_param
{
    /// Scan interval in multiple of 0.625ms - Must be higher than 2.5ms
    uint16_t interval_slot;
    /// Scan window in multiple of 0.625ms - Must be higher than 2.5ms
    uint16_t window_slot;
} gaf_al_scan_param_t;


/// Metadata present in advertising report
typedef struct gaf_al_adv_report_metadata
{
    /// Bit field providing information about the received report (see #gapm_adv_report_info_bf enumeration)
    uint8_t      info;
    /// Transmitter device address
    gap_bdaddr_t trans_addr;
    /// Target address (in case of a directed advertising report)
    gap_bdaddr_t target_addr;
    /// TX power (in dBm)
    int8_t       tx_pwr;
    /// RSSI (between -127 and +20 dBm)
    int8_t       rssi;
    /// Primary PHY on which advertising report has been received
    uint8_t      phy_prim;
    /// Secondary PHY on which advertising report has been received
    uint8_t      phy_second;
    /// Advertising SID
    /// Valid only for Periodic Advertising report
    uint8_t      adv_sid;
    /// Periodic Advertising interval (in unit of 1.25ms, min is 7.5ms)
    /// Valid only for Periodic Advertising report
    uint16_t     period_adv_intv;
} gaf_al_adv_report_metadata_t;

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * ADAPTATION LAYER CONFIGURATION API
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Configure adaptation layer to be used by a GAF layer.
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_configure(void);

/**
 ****************************************************************************************
 * @brief Retrieve GAP activity index associated with activity local index
 *
 * @param[in] act_lid       Activity local index
 *
 * @return GAP Activity Index (GAP_INVALID_ACTV_IDX if no association exists)
 ****************************************************************************************
 */
uint8_t gaf_al_actv_idx_get(uint8_t act_lid);

/*
 * ADVERTISING CONTROL API
 ****************************************************************************************
 */

#if (GAF_AL_ADV)
/// Advertising activity callback
typedef struct gaf_al_adv_cb
{
    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_adv_start function.
     *
     * if succeed the advertising is started.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Start status code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_start_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_adv_stop function or if
     *        the advertising is stopped due to an external event.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] reason        Stop reason (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_stopped)(uint8_t act_lid, uint16_t dummy, uint16_t reason);

    /**
     ****************************************************************************************
     * @brief  Callback function called as result of @see gaf_al_adv_data_upd function
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Data update satus code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_data_upd_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);
} gaf_al_adv_cb_t;

/**
 ****************************************************************************************
 * @brief Create a Advertising activity.
 *
 * @param[in]  dummy            Dummy parameter returned in callbacks
 * @param[in]  p_adv_params     Pointer to advertising parameters
 * @param[in]  p_cb             Pointer to callback functions used to return
 *                              procedure execution status
 * @param[out] p_act_lid        Pointer to activity local index return
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_adv_create(uint16_t dummy, const gaf_al_adv_param_t* p_adv_params,
                               const gaf_al_adv_cb_t* p_cb, uint8_t* p_act_lid);

/**
 ****************************************************************************************
 * @brief Remove a Advertising activity.
 *
 *        Advertising activity must be in stopped state to execute this function
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity remove (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_adv_remove(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Start a Advertising activity.
 *
 *        If function execution succeed, @see cb_start_cmp callback will be executed once
 *        Advertising is started or if an error occurs.
 *
 * @param[in] act_lid         Activity local index
 * @param[in] p_adv_data      Pointer to buffer that contains advertising data
 * @param[in] timeout_s       Timeout duration in seconds
 *                            0 means that Requests will last until stopped by the upper layer
 *
 * @return Status of  activity start (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_adv_start(uint8_t act_lid, co_buf_t* p_adv_data, uint16_t timeout_s);

/**
 ****************************************************************************************
 * @brief Stop an advertising activity.
 *
 *        If function execution succeed, @see cb_stopped callback will be executed once
 *        advertising is stopped or if an error occurs.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity stop (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_adv_stop(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Update data advertising data
 *
 *        If function execution succeed, @see cb_data_upd_cmp callback will be executed once
 *        advertising data procedure is over.
 *
 *        Advertising activity must be started to update data.
 *
 * @param[in] act_lid         Activity local index
 * @param[in] data_type       Type of advertising data (see #gaf_al_adv_data_type enumeration)
 * @param[in] p_adv_data      Pointer to buffer that contains advertising data
 *
 * @return Status of advertising activity data update (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_adv_data_upd(uint8_t act_lid, uint8_t data_type, co_buf_t* p_data);

/**
 ***************************************************************************************
 * @brief Set callback function allowing to inform a block that BD Address is about to be renewed for an
 * advertising activity\n
    /// !!!! FOR INTERNAL USE ONLY !!!!
 *
 * @param[in] actv_idx          Activity index
 * @param[in] cb_addr_renew     Callback function
 *
 * @return Status (see #gaf_err enumeration)
 ***************************************************************************************
 */
uint16_t gaf_al_adv_set_cb_addr_renew(uint8_t act_lid, gapm_adv_cb_addr_renew cb_addr_renew);
#endif // (GAF_AL_ADV)

/*
 * PERIODIC ADV CONTROL API
 ****************************************************************************************
 */

#if (GAF_AL_PER_ADV)
/// Periodic Advertising activity callback
typedef struct gaf_al_per_adv_cb
{
    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_per_adv_start function.
     *
     * if succeed the Periodic Advertising is started.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Start status code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_start_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_per_adv_stop function or if
     *        the Periodic Advertising is stopped due to an external event.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] reason        Stop reason (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_stopped)(uint8_t act_lid, uint16_t dummy, uint16_t reason);

    /**
     ****************************************************************************************
     * @brief  Callback function called as result of @see gaf_al_per_adv_data_upd function
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Data update satus code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_data_upd_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_per_adv_tranfer
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Status code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_transfer_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);
} gaf_al_per_adv_cb_t;

/**
 ****************************************************************************************
 * @brief Create a Periodic Advertising activity.
 *
 * @param[in]  dummy            Dummy parameter returned in callbacks
 * @param[in]  p_per_adv_params Pointer to Periodic Advertising parameters
 * @param[in]  p_cb             Pointer to callback functions used to return
 *                              procedure execution status
 * @param[out] p_act_lid        Pointer to activity local index return
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_create(uint16_t dummy, const gaf_al_per_adv_param_t* p_per_adv_params,
                               const gaf_al_per_adv_cb_t* p_cb, uint8_t* p_act_lid);

/**
 ****************************************************************************************
 * @brief Remove a Periodic Advertising activity.
 *
 *        Periodic Advertising activity must be in stopped state to execute this function
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity remove (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_remove(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Start a Periodic Advertising activity.
 *
 *        If function execution succeed, @see cb_start_cmp callback will be executed once
 *        Periodic Advertising is started or if an error occurs.
 *
 * @param[in] act_lid         Activity local index
 * @param[in] p_adv_data      Pointer to buffer that contains advertising data
 * @param[in] p_per_adv_data  Pointer to buffer that contains Periodic Advertising data
 *
 * @return Status of  activity start (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_start(uint8_t act_lid, co_buf_t* p_adv_data, co_buf_t* p_per_adv_data);

/**
 ****************************************************************************************
 * @brief Request to transfer information about a Periodic Advertising or about a local Periodic
 * Advertising so that a peer device may synchronize with a Periodic Advertising
 *
 * @param[in] act_lid            Activity local index
 * @param[in] con_lid            Connection local index
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_transfer(uint8_t act_lid, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Stop a Periodic Advertising activity.
 *
 *        If function execution succeed, @see cb_stopped callback will be executed once
 *        Periodic Advertising is stopped or if an error occurs.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity stop (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_stop(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Update data broadcasted on a Periodic Advertising data
 *
 *        If function execution succeed, @see cb_data_upd_cmp callback will be executed once
 *        Periodic Advertising data procedure is over.
 *
 *        Periodic Advertising activity must be started to update data.
 *
 * @param[in] act_lid         Activity local index
 * @param[in] data_type       Type of advertising data (see #gaf_al_adv_data_type enumeration)
 * @param[in] p_per_adv_data  Pointer to buffer that contains advertising or
 *                            Periodic Advertising data
 *
 * @return Status of Periodic Advertising activity data update (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_data_upd(uint8_t act_lid, uint8_t data_type, co_buf_t* p_data);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t gaf_al_per_adv_get_addr(uint8_t act_lid, uint8_t** pp_addr, uint8_t* p_addr_type, uint8_t* p_adv_sid);
#endif // (GAF_AL_PER_ADV)

/*
 * SCAN CONTROL API
 ****************************************************************************************
 */
#if (GAF_AL_SCAN)
/// Scan activity callback
typedef struct gaf_al_scan_cb
{
    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_scan_start function.
     *        If succeed the scan is started.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Start status code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_start_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_scan_stop function or if
     *        the scan is stopped due to an external event.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] reason        Stop reason (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_stopped)(uint8_t act_lid, uint16_t dummy, uint16_t reason);

    /**
     ****************************************************************************************
     * @brief Function called when an advertising report is received that matches pattern (if present)
     *
     * @note Advertising report can be received by several scanner, buffer that contains report
     *       is not supposed to be altered or put in a list.
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] p_adv_report  Pointer to advertising data report.
     *                          In metadata some information about advertising report can be found
     *                          (@see gaf_al_adv_report_metadata_t)
     ****************************************************************************************
     */
    void (*cb_report)(uint8_t act_lid, uint16_t dummy, co_buf_t* p_adv_report);
} gaf_al_scan_cb_t;


/**
 ****************************************************************************************
 * @brief Create a Scan activity.
 *
 * @param[in]  dummy            Dummy parameter returned in callbacks
 * @param[in]  type             Scan type (see #gapm_scan_type enumeration)
 * @param[in]  pattern_size     Advertising data pattern size, 0 = disable, Max GAF_AL_SCAN_PATTERN_SIZE_MAX
 * @param[in]  p_pattern        Pointer to advertising data pattern (byte 0 represents AD Type)
 *                              If pattern matches, advertising report is triggered
 * @param[in]  p_1m_param       Pointer to 1M PHY parameters (NULL means not enabled for that PHY)
 * @param[in]  p_coded_param    Pointer to LE Coded PHY parameters (NULL means not enabled for that PHY)
 * @param[in]  p_cb             Pointer to callback functions used to return
 *                              procedure execution status
 * @param[out] p_act_lid        Pointer to activity local index return
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_create(uint16_t dummy, uint8_t type, uint8_t pattern_size, const uint8_t* p_pattern,
                            const gaf_al_scan_param_t* p_1m_param, const gaf_al_scan_param_t* p_coded_param,
                            const gaf_al_scan_cb_t* p_cb, uint8_t* p_act_lid);

/**
 ****************************************************************************************
 * @brief Remove a scan activity.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity remove (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_remove(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Update scan activity with new parameters.
 *        Parameters are applied only when internal scan activity is restarted
 *
 * @param[in]  act_lid         Activity local index
 * @param[in]  p_1m_param       Pointer to 1M PHY parameters (NULL means not enabled for that PHY)
 * @param[in]  p_coded_param    Pointer to LE Coded PHY parameters (NULL means not enabled for that PHY)
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_param_upd(uint8_t act_lid, const gaf_al_scan_param_t* p_1m_param,
                               const gaf_al_scan_param_t* p_coded_param);


/**
 ****************************************************************************************
 * @brief Start scan activity.
 *        Since multiple scan activities can be present, GAF adaptation layer try as
 *        much as possible to respect requested duty cycle.
 *
 *        If function execution succeed, @see cb_start_cmp callback will be executed once
 *        scan is started or if an error occurs. Callback execution can occurs before
 *        function return.
 *
 * @param[in] act_lid         Activity local index
 * @param[in] timeout_s       Timeout in seconds
 *
 * @return Status of activity start (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_start(uint8_t act_lid, uint16_t timeout_s);

/**
 ****************************************************************************************
 * @brief Stop scan activity. if other activity are scanning, scan will continue but no
 *        more report will be triggered for this activity
 *
 *        If function execution succeed, @see cb_stopped callback will be executed once
 *        scan stopped or if an error occurs. Callback execution can occurs before
 *        function return.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity stop (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_stop(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Enable or disable reception of advertising reports
 *
 * @param[in] act_lid         Activity local index
 * @param[in] enable          True to enable reception of advertising reports,
 *                            False to disable reception
 *
 * @return Status of activity stop (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_report_ctrl(uint8_t act_lid, bool enable);

#endif // (GAF_AL_SCAN)

/*
 * PERIODIC SYNC CONTROL API
 ****************************************************************************************
 */
#if (GAF_AL_PER_SYNC)
/// Periodic sync activity callback
typedef struct gaf_al_per_sync_cb
{
    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_per_sync_scan_start or
     *        @see gaf_al_per_sync_con_start functions.
     *        If succeed the periodic sync waits for an establishment
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Start status code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_start_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_per_sync_stop function or if
     *        the periodic sync establishment is canceled or periodic sync is lost
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] reason        Stop reason (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_stopped)(uint8_t act_lid, uint16_t dummy, uint16_t reason);


    /**
     ****************************************************************************************
     * @brief Callback function called when periodic sync has been established
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] p_adv_addr    Pointer to Periodic Advertising identification
     * @param[in] phy               PHY on which synchronization has been established (see #gap_phy_val enumeration)
     * @param[in] internal_frames   Periodic advertising interval (in unit of 1.25ms, min is 7.5ms)
     * @param[in] serv_data     Only valid for a Periodic Advertising Sync Transfer, else ignore
     ****************************************************************************************
     */
    void (*cb_estab)(uint8_t act_lid, uint16_t dummy, const gap_per_adv_bdaddr_t* p_adv_addr, uint8_t phy,
                     uint16_t interval_frames, uint16_t serv_data);

    /**
     ****************************************************************************************
     * @brief Function called when a Periodic Advertising report is received *
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] p_adv_report  Pointer to advertising data report.
     *                          In metadata some information about advertising report can be found
     *                          (@see gaf_al_adv_report_metadata_t)
     ****************************************************************************************
     */
    void (*cb_report)(uint8_t act_lid, uint16_t dummy, co_buf_t* p_adv_report);

    /**
     ****************************************************************************************
     * @brief Function called when a BIG Info report is received
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] p_report      Pointer to structure that contains Big Info data
     ****************************************************************************************
     */
    void (*cb_big_info)(uint8_t act_lid, uint16_t dummy, const gap_big_info_t* p_report);

    /**
     ****************************************************************************************
     * @brief Callback function called as result of @see gaf_al_per_sync_scan_tranfer
     *
     * @param[in] act_lid       Activity local index
     * @param[in] dummy         Dummy parameter
     * @param[in] status        Status code (see #gaf_err enumeration)
     ****************************************************************************************
     */
    void (*cb_transfer_cmp)(uint8_t act_lid, uint16_t dummy, uint16_t status);
} gaf_al_per_sync_cb_t;


/**
 ****************************************************************************************
 * @brief Create a Periodic sync Activity
 *
 * @param[in]  dummy              Dummy parameter returned in callbacks
 * @param[in]  con_sync           True to start sync using PAST, using scan otherwise
 * @param[in]  p_1m_param         Pointer to 1M PHY parameters (NULL means not enabled for that PHY)
 * @param[in]  p_coded_param      Pointer to LE Coded PHY parameters (NULL means not enabled for that PHY)
 * @param[in]  con_lid            Connection local index (meaningful only for connection sync)
 * @param[in]  p_adv_addr         Pointer to Periodic Advertising identification (can be null)
 * @param[in]  p_cb               Pointer to callback functions used to return
 *                                procedure execution status
 * @param[out] p_act_lid          Pointer to activity local index return
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_sync_create(uint16_t dummy, bool con_sync, const gaf_al_scan_param_t* p_1m_param,
                                const gaf_al_scan_param_t* p_coded_param, const gap_per_adv_bdaddr_t* p_adv_addr,
                                uint8_t con_lid, const gaf_al_per_sync_cb_t* p_cb, uint8_t* p_act_lid);

/**
 ****************************************************************************************
 * @brief Remove a Periodic sync activity.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity remove (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_sync_remove(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Establish a periodic sync
 *
 *        If function execution succeed, @see cb_start_cmp callback will be executed once
 *        activity is started or if an error occurs.
 *        Once the periodic sync is established, @see cb_estab callback is executed.
 *
 * @param[in] act_lid            Activity local index
 * @param[in] sync_to_10ms       Synchronization timeout for the Periodic Advertising
 *                               (in unit of 10ms between 100ms and 163.84s)
 * @param[in] skip               Number of Periodic Advertising that can be skipped after a successful reception
 *                               Maximum authorized value is 499
 * @param[in] adv_report_en      True to enable Periodic Advertising report, False to disable them by default
 * @param[in] biginfo_report_en  True to enable BIG Info report, False to disable them by default
 * @param[in] timeout_s          Scan Timeout in seconds - meaningless for PAST
 *
 * @return Status of activity start (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_sync_start(uint8_t act_lid, uint16_t sync_to_10ms, uint16_t skip, bool adv_report_en,
                               bool biginfo_report_en, uint16_t timeout_s);

/**
 ****************************************************************************************
 * @brief Stop periodic sync activity, according to state of the activity it cancels
 *        periodic sync establishment or terminate it
 *
 *        If function execution succeed, @see cb_stopped callback will be executed once
 *        scan stopped or if an error occurs.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity stop (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_sync_stop(uint8_t act_lid);


/**
 ****************************************************************************************
 * @brief Enable or disable reception of Periodic Advertising reports
 *
 * @param[in] act_lid            Activity local index
 * @param[in] adv_report_en      True to enable Periodic Advertising report, False to disable them by default
 * @param[in] biginfo_report_en  True to enable BIG Info report, False to disable them by default
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_sync_report_ctrl(uint8_t act_lid, bool adv_report_en, bool biginfo_report_en);

#if (GAF_AL_PAST)
/**
 ****************************************************************************************
 * @brief Allow to know if synchronization has been established with a given Periodic Advertising. If such a
 * synchronization exist, return the Activity local index of the used Periodic Synchronization activity
 *
 * @param[in] p_adv_addr         Pointer to information allowing to identify a Periodic Advertising
 * @param[in] p_act_lid          Pointer at which the Activity local index is returned is indicated synchronization
 * has already been established
 *
 * @return True if synchronization with the indicated Periodic Advertising has been established, else False
 ****************************************************************************************
 */
bool gaf_al_per_sync_is_synced(const gap_per_adv_bdaddr_t* p_adv_addr, uint8_t* p_act_lid);
#endif //(GAF_AL_PAST)

/**
 ****************************************************************************************
 * @brief Request to transfer information about an established Periodic Synchronization or about a local Periodic
 * Advertising so that a peer device may synchronize with a Periodic Advertising
 *
 * @param[in] act_lid            Activity local index
 * @param[in] con_lid            Connection local index
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_per_sync_transfer(uint8_t act_lid, uint8_t con_lid);
#endif // (GAF_AL_PER_SYNC)

#endif // (GAF_AL)

#endif /* _GAF_AL_H_ */

/**
 ****************************************************************************************
 *
 * @file gaf_al_int.h
 *
 * @brief GAF adaptation layer of the Generic Access profile - Internals
 *
 * Copyright (C) RivieraWaves 2009-2021
 *
 ****************************************************************************************
 */


#ifndef _GAF_AL_INT_H_
#define _GAF_AL_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf_cfg.h"      // GAF Configuration

#if (GAF_AL)

#include "gaf_al.h"       // GAF adaptation layer API
#include "gaf.h"          // GAF Defines
#include "gap.h"          // GAP defines
#include "gapc_msg.h"     // GAPC Definitions

/*
 *
 * DEFINES
 ****************************************************************************************
 */

/// Number of activities
#define GAF_AL_ACT_NB (GAF_AL_ACT_MAX + GAF_AL_SCAN)

#if (GAF_AL_SCAN)
/// Activity that manages scan
#define GAF_AL_SCAN_MGR_ACT_LID (GAF_AL_ACT_MAX)
#endif // (GAF_AL_SCAN)

/// Type of non-connected activities supported by GAF adaptation layer
enum gaf_al_act_type
{
    /// No information about activity type
    GAF_AL_ACT_UNKNOWN,
    #if (GAF_AL_ADV)
    /// Advertising activity
    GAF_AL_ACT_ADV,
    #endif // (GAF_AL_ADV)
    #if (GAF_AL_PER_ADV)
    /// Periodic ADV activity
    GAF_AL_ACT_PER_ADV,
    #endif // (GAF_AL_PER_ADV)
    #if (GAF_AL_SCAN)
    /// Scan activity
    GAF_AL_ACT_SCAN,
    /// Scan Manager activity
    GAF_AL_ACT_SCAN_MGR,
    #endif // (GAF_AL_SCAN)
    #if (GAF_AL_PER_SYNC)
    /// Periodic Sync activity
    GAF_AL_ACT_PER_SYNC = GAF_AL_ACT_SCAN, // equals to a scan activity
    #endif // (GAF_AL_PER_SYNC)
};


/// Activity information bit field
enum gaf_al_act_info_bf
{
    /// Activity state (depends on activity type)
    GAF_AL_ACT_STATE_MASK      = 0x07,
    GAF_AL_ACT_STATE_LSB       = 0,
    /// True if a procedure is on-going for current activity, False else
    GAF_AL_ACT_PROC_BUSY_BIT   = 0x08,
    GAF_AL_ACT_PROC_BUSY_POS   = 3,
    /// Activity procedure state (depends on activity type and procedure)
    GAF_AL_ACT_PROC_STATE_MASK = 0xF0,
    GAF_AL_ACT_PROC_STATE_LSB  = 4,
};

/// Activity procedure transition
enum gaf_al_trans
{
    /// Procedure can be started
    GAF_AL_TRANS_PROC_START,
    /// Procedure is granted to send a GAP command
    GAF_AL_TRANS_GAP_CMD_SEND,
    /// Procedure receives an expected command complete status
    GAF_AL_TRANS_GAP_CMP_EVT,
    /// Unexpected error received during procedure execution
    GAF_AL_TRANS_ERROR,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback executed to continue procedure execution according to an event transition
 *
 * @param[in] act_lid         Activity local index
 * @param[in] transition      Procedure Event transition type (see #gaf_al_transition_type enumeration)
 * @param[in] status          Status of the transition
 ****************************************************************************************
 */
typedef void (*gaf_al_proc_continue)(uint8_t act_lid, uint8_t transition, uint16_t status);

/// Activity Header
typedef struct gaf_al_act_hdr
{
    /// Procedure execution continue callback
    gaf_al_proc_continue cb_continue;
    /// GAP activity index
    uint8_t              actv_idx;
    /// Type of activity (see #gaf_al_act_type enumeration)
    uint8_t              act_type;
    /// Activity information bit field (see #gaf_al_act_info_bf enumeration)
    uint8_t              info_bf;
} gaf_al_act_hdr_t;


#if (GAF_AL_SCAN)
/// Scanning Activity environment header
typedef struct gaf_al_scan_act_hdr
{
    /// Activity Header
    gaf_al_act_hdr_t           hdr;
    /// Activity owner callback set
    const gaf_al_scan_cb_t*    p_cb;
    /// 1M PHY scan parameters
    gaf_al_scan_param_t        param_1m;
    /// LE Coded PHY scan parameters
    gaf_al_scan_param_t        param_coded;
    /// Expire time instant (counter in milliseconds)
    uint32_t                   expire_time;
    /// Scan configuration bit field (@see eunm gaf_al_scan_cfg_bf)
    uint16_t                   cfg_bf;
    /// Dummy parameter returned in callbacks
    uint16_t                   dummy;
} gaf_al_scan_act_hdr_t;
#endif // (GAF_AL_SCAN)

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Create an activity and assign an activity local index
 *
 * @param[in] act_type        Type of activity (see #gaf_al_act_type enumeration)
 * @param[in] env_size        Environment size to allocate
 * @param[in] p_act_lid       Pointer to allocated activity local index
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_act_create(uint8_t act_type, uint16_t env_size, uint8_t* p_act_lid);

/**
 ****************************************************************************************
 * @brief Remove an activity. it assumes that activity is cleaned at GAP level
 *
 * @param[in] act_lid         Activity local index
 *
 * @return Status of activity removal (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_act_remove(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Retrieve environment data for an activity
 *
 * @param[in] act_lid         Activity local index
 * @param[in] act_type        Type of activity (see #gaf_al_act_type enumeration)
 *
 * @return Pointer to the activity environment or NULL if not found
 ****************************************************************************************
 */
gaf_al_act_hdr_t* gaf_al_act_get(uint8_t act_lid, uint8_t act_type);

/**
 ****************************************************************************************
 * @brief Start a procedure for a specific activty
 *
 * @param[in] act_lid         Activity local index
 * @param[in] cb_continue     Procedure state machine continue loop
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_act_proc_start(uint8_t act_lid, gaf_al_proc_continue cb_continue);

/**
 ****************************************************************************************
 * @brief Mark procedure stopped, this must be handle by procedure itself.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_act_proc_stop(uint8_t act_lid);

/**
 ****************************************************************************************
 * @brief Wait procedure to be granted for GAP command transmission
 *
 *        Command cannot be immediately sent, procedure must wait for GAF_AL_TRANS_GAP_CMD_SEND
 *        procedure transition before being allowed to send it.
 *
 * @param[in] act_lid         Activity local index
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_act_proc_cmd_send(uint8_t act_lid);


#if (GAF_AL_SCAN)

/**
 ****************************************************************************************
 * @brief Internal function used to create a Scan activity.
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
 * @param[in]  env_size         Size of environment to allocate
 * @param[out] p_act_lid        Pointer to activity local index return
 *
 * @return Status of activity creation (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t gaf_al_scan_create_int(uint16_t dummy, uint8_t type, uint8_t pattern_size, const uint8_t* p_pattern,
                            const gaf_al_scan_param_t* p_1m_param, const gaf_al_scan_param_t* p_coded_param,
                            const gaf_al_scan_cb_t* p_cb, uint16_t env_size, uint8_t* p_act_lid);
#endif // (GAF_AL_SCAN)
#endif // (GAF_AL)

#endif // _GAF_AL_INT_H_

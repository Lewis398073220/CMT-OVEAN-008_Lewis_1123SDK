/**
 ****************************************************************************************
 *
 * @file iap_int.h
 *
 * @brief Isochronous Access Profile - Internal Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef IAP_INT_H_
#define IAP_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "co_djob.h"        // Delayed Jobs Management
#include "co_utils.h"       // Common Utilities Definitions
#include <string.h>         // Strings Management
#include "iap.h"            // IAP Definitions
#include "iap_cfg.h"        // IAP Configuration

/*
 * DEFINES
 ****************************************************************************************
 */

/// Invalid connection handle
#define IAP_INVALID_CON_HDL         (0xFFFF)
/// IAP Max CIS num in CIG
#define IAP_MAX_CIS_NUM             (0x10)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Role values
enum iap_roles
{
    /// Master role
    IAP_ROLE_MASTER = 0,
    /// Slave role
    IAP_ROLE_SLAVE,

    IAP_ROLE_MAX,
};

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function called when a subprocedure has been completed
 *
 * @param[in] p_env             Pointer to group information
 * @param[in] stream_lid        Stream local index
 * @param[in] status            Status
 ****************************************************************************************
 */
typedef void (*iap_proc_cb)(void* p_env, uint8_t stream_lid, uint16_t status);

/**
 ****************************************************************************************
 * @brief Callback function called during disable procedure
 *
 * @param[in] p_env             Pointer to stream information structure
 ****************************************************************************************
 */
typedef void (*iap_disable_cb)(void* p_env);

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Procedure structure for a group
typedef struct iap_proc
{
    /// Delayed job
    co_djob_t djob;
    /// Callback function for delay the end of procedure
    iap_proc_cb cb;
    /// Command code being processed
    uint16_t cmd_code;
    /// Procedure state bit field
    uint8_t state_bf;
    /// Group or stream local index for which an HCI command has been sent
    uint8_t lid;
} iap_proc_t;

/// Test mode information structure for a stream
typedef struct iap_stream_tm
{
    /// Status bit field (see #iap_tm_status_bf enumeration)
    uint8_t status_bf;
} iap_stream_tm_t;

/// Data path information structure for a stream
typedef struct iap_stream_dp
{
    /// Pointer to Data path configuration for each direction
    iap_dp_config_t* p_config[IAP_DP_DIRECTION_MAX];
    /// Direction status bit field
    uint8_t dir_status_bf[IAP_DP_DIRECTION_MAX];
    /// Status bit field
    uint8_t status_bf;
} iap_stream_dp_t;

/// Common stream information structure
typedef struct iap_stream_info
{
    /// List header
    co_list_hdr_t hdr;
    /// Data path information - Content set and managed by Data Path Manager
    iap_stream_dp_t dp_info;
    /// Test mode information - Content set and managed by Test Mode Manager
    iap_stream_tm_t tm_info;
    /// Callback function used for disable procedure
    iap_disable_cb cb_disable;
    /// Connection handle for the stream
    uint16_t con_hdl;
    /// Group local index
    uint8_t group_lid;
    /// Stream local index
    uint8_t stream_lid;
    /// State bit field
    uint8_t state_bf;
    /// CIS ID if stream is a unicast stream or position in the group if stream is a broadcast stream
    uint8_t id_pos;
} iap_stream_info_t;

/// Unicast stream specific information structure
typedef struct iap_us_info
{
    /// Connection local index for bound connection
    uint8_t con_lid;
    /// Stream state
    uint8_t state_bf;
} iap_us_info_t;

/// Stream information structure for an unicast stream
typedef struct iap_us
{
    /// Common stream information
    iap_stream_info_t stream_info;
    /// Unicast stream specific management information
    iap_us_info_t info;
    /// Unicast stream configuration
    iap_us_config_t cfg;
    union
    {
        /// Stream parameters provided (normal parameters)
        iap_us_param_t params;
        /// Stream parameters provided by upper layer (test parameters)
        iap_us_test_param_t params_test;
    } u;
} iap_us_t;

/// Stream information structure for a broadcast stream
typedef struct iap_bs
{
    /// Common management information - Updated by stream manager only
    iap_stream_info_t stream_info;
} iap_bs_t;


/// Common group information structure
typedef struct iap_group_info
{
    /// List of Stream belonging to the group
    co_list_t list_streams;
    /// Procedure information
    iap_proc_t proc;
    /// Group configuration bit field
    uint8_t group_cfg_bf;
    /// Group local index
    uint8_t group_lid;
    /// Interface index
    uint8_t intf_lid;
    /// Group Identifier (BIG Handle or CIG ID)
    uint8_t id;
    /// Number of streams in the group
    uint8_t nb_streams;
} iap_group_info_t;

/// Unicast group specific management information structure
typedef struct iap_ug_info
{
    /// Number of streams ready to be configured
    uint8_t nb_streams_add;
    /// Number of bound streams
    uint8_t nb_streams_bnd;
    /// Number of enabled streams
    uint8_t nb_streams_en;
    /// Group state
    uint8_t state_bf;
    /// Pointer to first stream for remove procedure
    iap_us_t* p_us_first;
    /// Pointer to last stream for remove procedure
    iap_us_t* p_us_last;
} iap_ug_info_t;

/// Group information structure for an unicast group
typedef struct iap_ug
{
    /// Common management information - Updated by stream manager only
    iap_group_info_t common_info;
    /// Unicast group specific information
    iap_ug_info_t info;
    /// Unicast group configuration
    iap_ug_config_t cfg;
    union
    {
        /// Group parameters (normal parameters)
        iap_ug_param_t params;
        /// Group parameters (test parameters)
        iap_ug_test_param_t params_test;
    } u;
#ifdef BLE_STACK_PORTING_CHANGES
    /// CIG-CIS created info
    uint16_t cis_hdl[IAP_MAX_CIS_NUM];
#endif
} iap_ug_t;

/// Broadcast group specific group information structure
typedef struct iap_bg_info
{
    /// Command code
    uint16_t cmd_code;
    /// State bit field
    uint8_t state_bf;
    /// Pointer to Stream information for subprocedure
    iap_stream_info_t* p_stream_info_subproc;
} iap_bg_info_t;

/// Group information structure for an Broadcast Group
typedef struct iap_bg
{
    /// Common group information
    iap_group_info_t common_info;
    /// Specific group information
    iap_bg_info_t info;
    union
    {
        /// Group parameters (normal parameters)
        iap_bg_param_t params;
        /// Group parameters (test parameters)
        iap_bg_test_param_t params_test;
    } u;
} iap_bg_t;

/*
 * INTERNAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
void iap_tm_disable(iap_stream_info_t* p_stream_info, bool local);

#endif // IAP_INT_H_

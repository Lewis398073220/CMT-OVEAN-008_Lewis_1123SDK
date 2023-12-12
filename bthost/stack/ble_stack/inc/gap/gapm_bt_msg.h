/**
 ****************************************************************************************
 *
 * @file gapm_bt_msg.h
 *
 * @brief Generic Access Profile Manager Message API. BT-Classic
 *
 * Copyright (C) RivieraWaves 2009-2021
 *
 ****************************************************************************************
 */


#ifndef _GAPM_BT_MSG_H_
#define _GAPM_BT_MSG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gapm_msg.h"
#include "gapm_bt.h"
#include "gapm_bt_page.h"
#include "gapm_bt_page_scan.h"
#include "gapm_bt_inquiry.h"
#include "gapm_bt_inquiry_scan.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// @addtogroup GAPM_MSG_STRUCT_API Message Structures
/// @ingroup GAPM_MSG_API
/// @{

/// Parameters of the @ref GAPM_BT_OOB_DATA_IND message
/*@TRACE*/
struct gapm_bt_oob_data_ind
{
    /// Generated P-192 OOB data
    gap_oob_t oob_192;
    /// Generated P-256 OOB data
    gap_oob_t oob_256;
};



/// Parameters of the @ref GAPM_LINK_KEY_STORE_CMD message
/*@TRACE*/
struct gapm_link_key_store_cmd
{
    /// Requested operation type (see enum #gapm_operation)
    /// (shall be #GAPM_LINK_KEY_STORE)
    uint8_t       operation;
    /// Address related to the link key
    gap_bdaddr_t  addr;
    /// Link key
    gap_sec_key_t link_key;
    /// Link key Security level (see enum #gap_sec_lvl)
    uint8_t       sec_lvl;
};

/// Parameters of the @ref GAPM_LINK_KEY_REMOVE_CMD message
/*@TRACE*/
struct gapm_link_key_remove_cmd
{
    /// Requested operation type (see enum #gapm_operation)
    /// (shall be #GAPM_LINK_KEY_REMOVE)
    uint8_t       operation;
    /// Address related to the link key
    gap_bdaddr_t  addr;
};

/// Parameters of the @ref GAPM_LINK_KEY_REMOVE_ALL_CMD message
/*@TRACE*/
struct gapm_link_key_remove_all_cmd
{
    /// Requested operation type (see enum #gapm_operation)
    /// (shall be #GAPM_LINK_KEY_REMOVE_ALL)
    uint8_t       operation;
};

/// Parameters of the @ref GAPM_LINK_KEY_GET_CMD message
/*@TRACE*/
struct gapm_link_key_get_cmd
{
    /// Requested operation type (see enum #gapm_operation)
    /// (shall be #GAPM_LINK_KEY_GET)
    uint8_t       operation;
    /// Address related to the link key
    gap_bdaddr_t  addr;
};

/// Parameters of the @ref GAPM_LINK_KEY_IND message
/*@TRACE*/
struct gapm_link_key_ind
{
    /// Address related to the link key
    gap_bdaddr_t  addr;
    /// Link key
    gap_sec_key_t link_key;
    /// Link key Security level (see enum #gap_sec_lvl)
    uint8_t       sec_lvl;
};


/// Inquiry scan start parameters
/*@TRACE*/
typedef struct gapm_inquiry_scan_start_param
{
    /// scan parameters
    gapm_inquiry_scan_param_t scan;
    /// Extended Inquiry Response data length
    uint8_t                   eir_length;
    /// Extended Inquiry Response data
    uint8_t                   eir_data[__ARRAY_EMPTY];
} gapm_inquiry_scan_start_param_t;


/// Parameter of GAPM_INQUIRY_REPORT_IND message
/*@TRACE*/
struct gapm_inquiry_report_ind
{
    /// Activity identifier
    uint8_t               actv_idx;
    /// Inquiry report information
    gapm_inquiry_report_t report;
    /// Length of received EIR data, 0 if nothing received
    uint8_t               eir_length;
    /// Extend inquiry response data
    uint8_t               eir_data[__ARRAY_EMPTY];
};

/// Activity parameters
typedef union gapm_bt_start_param
{
    /// Inquiry parameters
    //@trc_union @activity_map[$parent.actv_idx] == GAPM_ACTV_TYPE_INQUIRY
    gapm_inquiry_param_t            inquiry_param;
    /// Inquiry Scan parameters
    //@trc_union @activity_map[$parent.actv_idx] == GAPM_ACTV_TYPE_INQUIRY_SCAN
    gapm_inquiry_scan_start_param_t inquiry_scan_param;
    /// Page parameters
    //@trc_union @activity_map[$parent.actv_idx] == GAPM_ACTV_TYPE_PAGE
    gapm_page_param_t               page_param;
    /// Page Scan parameters
    //@trc_union @activity_map[$parent.actv_idx] == GAPM_ACTV_TYPE_PAGE_SCAN
    gapm_page_scan_param_t          page_scan_param;
} gapm_bt_start_param_u;

/// Start a given activity command
/*@TRACE*/
struct gapm_bt_activity_start_cmd
{
    /// Requested operation type (see enum #gapm_operation)
    ///  - #GAPM_START_ACTIVITY: Start a given activity
    uint8_t operation;
    /// Activity identifier
    uint8_t actv_idx;
    /// Activity parameters
    gapm_bt_start_param_u u_param;
};

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/// @} GAPM_MSG_STRUCT_API

#endif /* _GAPM_BT_MSG_H_ */

/**
 ****************************************************************************************
 *
 * @file acc_ots_msg.h
 *
 * @brief Audio Content Control - Definition of Kernel Messages (Data Transfer Server)
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ACC_DTS_MSG_H_
#define ACC_DTS_MSG_H_

/**
 ****************************************************************************************
 * @defgroup ACC_OTS_MSG Message API
 * @ingroup ACC_OTS
 * @brief Description of Message API for Data Transfer Service Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf.h"             // GAF Defines
#include "acc_msg.h"         // Audio Content Control Kernel Messages Definitions
#include "dts_msg.h"         // Data Transfer Server - Message API Definitions

/// @addtogroup ACC_OTS_MSG
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of GAF_REQ request codes for Data Transfer Server
enum acc_dts_msg_req_codes
{
    ACC_DTS_CONFIGURE = GAF_CODE(ACC, DTS, 0),
};


/// List of GAF_IND indication codes for Data Transfer Server
enum acc_dts_msg_ind_codes
{
    ACC_DTS_COC_CONNECTED = GAF_CODE(ACC, DTS, 0),
    ACC_DTS_COC_DISCONNECTED = GAF_CODE(ACC, DTS, 1),
    ACC_DTS_COC_DATA = GAF_CODE(ACC, DTS, 2),
};

/// List of GAF_IND request indication codes for Data Transfer Server
enum acc_dts_msg_req_ind_codes
{
    ACC_DTS_COC_CONNECT = GAF_CODE(ACC, DTS, 0),
};

/*
 * API MESSAGES
 ****************************************************************************************
 */

/// Structure for response message
typedef dts_rsp_t acc_dts_rsp_t;

// /// Structure for #ACC_DTS_COC_RELEASE command message
// typedef dts_coc_release_cmd_t acc_dts_coc_release_cmd_t;

/// Structure for #ACC_DTS_COC_DISCONNECT command message
typedef dts_coc_disconnect_cmd_t acc_dts_coc_disconnect_cmd_t;

/// Structure for #ACC_DTS_COC_SEND command message
typedef dts_coc_send_cmd_t acc_dts_coc_send_cmd_t;

/// Structure for #ACC_DTS_COC_REGISTER command message
typedef dts_coc_register_cmd_t acc_dts_coc_register_cmd_t;

/// Structure for #ACC_DTS_COC_RELEASE command message
typedef dts_coc_release_cmd_t acc_dts_coc_release_cmd_t;

/// Structure command complete event
typedef dts_cmp_evt_t acc_dts_cmp_evt_t;

/// Structure for #ACC_DTS_CONFIGURE request message
typedef struct acc_dts_configure_req
{
    /// Request code
    uint16_t req_code;
} acc_dts_configure_req_t;

/// Structure for #ACC_DTS_COC_CONNECTED indication message
typedef dts_coc_connected_ind_t acc_dts_coc_connected_ind_t;

/// Structure for #ACC_DTS_COC_DISCONNECTED indication message
typedef dts_coc_disconnected_ind_t acc_dts_coc_disconnected_ind_t;

/// Structure for #ACC_DTS_COC_DATA indication message
typedef dts_coc_data_ind_t acc_dts_coc_data_ind_t;

/// Structure for #ACC_DTS_COC_REGISTER indication message
typedef dts_registerd_ind_t acc_dts_registerd_ind_t;

/// Structure for #ACC_DTS_COC_CONNECT request indication message
typedef dts_coc_connect_req_ind_t acc_dts_coc_connect_req_ind_t;

/// Structure for DTS_COC_CONNECT confirmation message
typedef dts_coc_connect_cfm_t acc_dts_coc_connect_cfm_t;

/// @} ACC_OTC_MSG

#endif // ACC_DTS_MSG_H_

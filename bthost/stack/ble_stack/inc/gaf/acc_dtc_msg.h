/**
 ****************************************************************************************
 *
 * @file acc_dtc_msg.h
 *
 * @brief Audio Content Control - Definition of Kernel Messages (Data Transfer Client)
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ACC_DTC_MSG_H_
#define ACC_DTC_MSG_H_

/**
 ****************************************************************************************
 * @defgroup ACC_DTC_MSG Message API
 * @ingroup ACC_DTC
 * @brief Description of Message API for Data Transfer Service Client module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "acc_msg.h"         // Audio Content Control Kernel Messages Definitions
#include "dtc_msg.h"         // Data Transfer Client - Message API Definitions
#include "gaf.h"             // GAF API


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// List of GAF_REQ request codes for Data Transfer Client
enum acc_dtc_msg_req_codes
{
    ACC_DTC_CONFIGURE = GAF_CODE(ACC, DTC, 0),
};

/// List of GAF_IND indication codes for Data Transfer Client
enum acc_dtc_msg_ind_codes
{
    ACC_DTC_COC_CONNECTED = GAF_CODE(ACC, DTC, 0),
    ACC_DTC_COC_DISCONNECTED = GAF_CODE(ACC, DTC, 1),
    ACC_DTC_COC_DATA = GAF_CODE(ACC, DTC, 2),
    // ACC_DTC_SVC_CHANGED = GAF_CODE(ACC, DTC, 17),
};

/// Structure for #ACC_DTC_COC_CONNECT command message
typedef dtc_coc_connect_cmd_t acc_dtc_coc_connect_cmd_t;

/// Structure for #ACC_DTC_COC_DISCONNECT command message
typedef dtc_coc_disconnect_cmd_t acc_dtc_coc_disconnect_cmd_t;

/// Structure for #ACC_DTC_COC_SEND command message
typedef dtc_coc_send_cmd_t acc_dtc_coc_send_cmd_t;

/// Structure for #ACC_DTC_RELEASE_SEND command message
typedef dtc_coc_release_cmd_t acc_dtc_coc_release_cmd_t;

/// Structure command complete event
typedef dtc_cmp_evt_t acc_dtc_cmp_evt_t;

/// Structure for #ACC_DTC_CONFIGURE request message
typedef struct acc_dtc_configure_req
{
    /// Request code
    uint16_t req_code;
} acc_dtc_configure_req_t;

/// Structure for #ACC_DTC_COC_CONNECTED indication message
typedef dtc_coc_connected_ind_t acc_dtc_coc_connected_ind_t;

/// Structure for #ACC_DTC_COC_DISCONNECTED indication message
typedef dtc_coc_disconnected_ind_t acc_dtc_coc_disconnected_ind_t;

/// Structure for #ACC_DTC_COC_DATA indication message
typedef dtc_coc_data_ind_t acc_dtc_coc_data_ind_t;

#endif
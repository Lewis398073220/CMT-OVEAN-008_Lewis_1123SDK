/**
 ****************************************************************************************
 *
 * @file acc_dtc.h
 *
 * @brief Audio Content Control - Data Transfer Client - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ACC_DTC_H_
#define ACC_DTC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "acc_ot.h"            // Audio Content Control - Data Tranfer Definitions

#include "gaf.h"               // GAF Defines
#include "dtc.h"               // Data Transfer Client Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup ACC_DTC_ENUM
/// @{

/// List of ACC_CMD command codes for Data Transfer Client
enum acc_dtc_cmd_codes
{
    /// Connection Oriented Channel - Connect
    ACC_DTC_COC_CONNECT = GAF_CODE(ACC, DTC, 0),
    /// Connection Oriented Channel - Disconnect
    ACC_DTC_COC_DISCONNECT = GAF_CODE(ACC, DTC, 1),
    /// Connection Oriented Channel - Send
    ACC_DTC_COC_SEND = GAF_CODE(ACC, DTC, 2),
    /// Connection Oriented Channel - Release
    ACC_DTC_COC_RELEASE = GAF_CODE(ACC, DTC, 3),
};

/// @} ACC_DTC_ENUM

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// @addtogroup ACC_DTC_CALLBACK
/// @{

/// Set of callback functions for Data Transfer Client
typedef dtc_cb_t acc_dtc_cb_t;

/// @} ACC_DTC_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup ACC_DTC_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Create and configure Data Transfer Client module
 *
 * @param[in] p_cb      Pointer to set of callback functions for communications with
 * upper layers
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t acc_dtc_configure(const acc_dtc_cb_t* p_cb);

/// Wrapper for functions defined in dtc.h
#define acc_dtc_coc_connect(con_lid, local_max_sdu, spsm)                                                         \
                                (dtc_coc_connect((con_lid), (local_max_sdu), (spsm)))
#define acc_dtc_coc_disconnect(con_lid, spsm)                                                                     \
                                (dtc_coc_disconnect((con_lid), (spsm)))
#define acc_dtc_coc_send(con_lid, spsm, length, p_sdu)                                                            \
                                (dtc_coc_send((con_lid), (spsm), (length), (p_sdu)))
#define acc_dtc_coc_release(con_lid, spsm)                                                                        \
                                (dtc_coc_release((con_lid), (spsm)))


/// @} ACC_DTC_FUNCTION

#endif // ACC_DTC_H_

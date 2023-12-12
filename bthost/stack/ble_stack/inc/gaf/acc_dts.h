/**
 ****************************************************************************************
 *
 * @file acc_dts.h
 *
 * @brief Audio Content Control - Data Transfer Server - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ACC_DTS_H_
#define ACC_DTS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "acc_ot.h"            // Audio Content Control - Data Transfer Definitions

#if (GAF_ACC_DTS)

#include "gaf.h"               // GAF Defines
#include "dts.h"               // Data Transfer Server Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup ACC_DTS_ENUM
/// @{

/// List of GAF_CMD command codes for Data Transfer Server
enum acc_dts_cmd_codes
{
    /// Register
    ACC_DTS_COC_REGISTER = GAF_CODE(ACC, DTS, 0),
    /// Disconnect Channel
    ACC_DTS_COC_DISCONNECT = GAF_CODE(ACC, DTS, 1),
    /// Send on Channel
    ACC_DTS_COC_SEND = GAF_CODE(ACC, DTS, 2),
    /// Release Channel
    ACC_DTS_COC_RELEASE = GAF_CODE(ACC, DTS, 3),
};

/// @} ACC_DTS_ENUM

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// @addtogroup ACC_DTS_CALLBACK
/// @{

/// Set of callback functions for Data Transfer Server
typedef dts_cb_t acc_dts_cb_t;

/// @} ACC_DTS_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup ACC_DTS_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Create and configure Data Transfer Client module
 *
 * @param[in] nb_transfers      Number of Data Transfer Service instances
 * @param[in] p_cb              Pointer to set of callback functions for communications with
 * upper layers
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t acc_dts_configure(const acc_dts_cb_t* p_cb);

/// Wrapper for functions defined in DTS.h
#define acc_dts_coc_regiseter(spsm, initial_credits) dts_coc_register((spsm), (initial_credits))
#define acc_dts_coc_disconnect(con_lid, spsm)                                                     \
                            dts_coc_disconnect((con_lid), (spsm))
#define acc_dts_coc_release_earliest_entry(con_lid, spsm)                                                        \
                            dts_coc_release_earliest_entry((con_lid), (spsm))
#define acc_dts_coc_send(con_lid, spsm, length, p_sdu)                                            \
                            dts_coc_send((con_lid), (spsm), (length), (p_sdu))
#define acc_dts_cfm_coc_connect(status, con_lid, token, local_max_sdu, spsm)                      \
                            dts_cfm_coc_connect((status), (con_lid), (token), (local_max_sdu), (spsm))

/// @} ACC_DTS_FUNCTION
#endif //(GAF_ACC_DTS)

#endif // ACC_DTS_H_

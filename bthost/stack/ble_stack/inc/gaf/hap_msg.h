/**
 ****************************************************************************************
 *
 * @file hap_msg.h
 *
 * @brief Hearing Aid Profile - Definition of Message API
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_MSG_H_
#define HAP_MSG_H_

/**
 ****************************************************************************************
 * @defgroup HAP_COMMON_MSG Message API
 * @ingroup HAP_COMMON
 * @brief Description of Message API for Hearing Aid Profile Common module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hap.h"            // Hearing Aid Profile Definitions
#include "gaf_msg.h"        // Generic Audio Framework API Message Definitions

/// @addtogroup HAP_COMMON_MSG
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of GAF_REQ request code value for Hearing Aid Profile Common module
enum hap_msg_req_code
{
    /// Configure (see #hap_configure_req_t)
    HAP_CONFIGURE = GAF_CODE(HAP, COMMON, 0),
};

/*
 * API MESSAGES
 ****************************************************************************************
 */

/// Structure for #HAP_CONFIGURE request message
typedef struct hap_configure_req
{
    /// Request code (shall be set to #HAP_CONFIGURE)
    uint16_t req_code;
    /// Configuration bit field (see #hap_cfg_bf enumeration)
    uint8_t cfg_bf;
    /// Configuration Parameters structure for Hearing Aid Service Server Role
    hap_has_cfg_param_t cfg_param_has;
    /// Configuration Parameters structure for Hearing Aid Service Client Role
    hap_hac_cfg_param_t cfg_param_hac;
} hap_configure_req_t;

/// @} HAP_COMMON_MSG

#endif // HAP_MSG_H_

/**
 ****************************************************************************************
 *
 * @file rscp_common.h
 *
 * @brief Header File - Running Speed and Cadence Profile common types.
 *
 * Copyright (C) RivieraWaves 2009-2020
 *
 ****************************************************************************************
 */


#ifndef _RSCP_COMMON_H_
#define _RSCP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup RSCP Running Speed and Cadence Profile
 * @ingroup Profile
 * @brief Running Speed and Cadence Profile
 *
 * The Running Speed and Cadence profile enables a Collector device to connect and
 * interact with a Running Speed and Cadence Sensor for use in sports and fitness
 * applications.
 *
 * This file contains all definitions that are common for the server and the client parts
 * of the profile.
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "co_math.h"
/*
 * DEFINES
 ****************************************************************************************
 */
/// Procedure Already in Progress Error Code
#define RSCP_ERROR_PROC_IN_PROGRESS         (0x80)
/// Client Characteristic Configuration descriptor improperly configured Error Code
#define RSCP_ERROR_CCC_INVALID_PARAM        (0x81)

/// RSC Measurement Value Min Length
#define RSCP_RSC_MEAS_MIN_LEN               (4)
/// RSC Measurement Value Max Length
#define RSCP_RSC_MEAS_MAX_LEN               (10)
/// SC Control Point Request Value Min Length
#define RSCP_SC_CNTL_PT_REQ_MIN_LEN         (1)
/// SC Control Point Request Value Max Length
#define RSCP_SC_CNTL_PT_REQ_MAX_LEN         (5)
/// SC Control Point Response Value Min Length
#define RSCP_SC_CNTL_PT_RSP_MIN_LEN         (3)
/// SC Control Point Response Value Max Length
#define RSCP_SC_CNTL_PT_RSP_MAX_LEN         (RSCP_SC_CNTL_PT_RSP_MIN_LEN + RSCP_LOC_MAX)

/// RSC Measurement all present
#define RSCP_MEAS_ALL_PRESENT               (0x07)
/// RSC Feature all supported
#define RSCP_FEAT_ALL_SUPP                  (0x001F)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Running Speed and Cadence Service Characteristics
enum rscp_rscs_char
{
    /// RSC Measurement
    RSCP_RSCS_RSC_MEAS_CHAR,
    /// RSC Feature
    RSCP_RSCS_RSC_FEAT_CHAR,
    /// Sensor Location
    RSCP_RSCS_SENSOR_LOC_CHAR,
    /// SC Control Point
    RSCP_RSCS_SC_CTNL_PT_CHAR,

    /// Maximum Characteristic
    RSCP_RSCS_CHAR_MAX,
};

/// RSC Measurement Flags bit field
enum rscp_meas_flags_bf
{
    /// Instantaneous Stride Length Present
    /// 0 False and 1 True
    RSCP_MEAS_INST_STRIDE_LEN_PRESENT_POS = 0,
    RSCP_MEAS_INST_STRIDE_LEN_PRESENT_BIT = CO_BIT(RSCP_MEAS_INST_STRIDE_LEN_PRESENT_POS),
    /// Total Distance Present
    /// 0 False and 1 True
    RSCP_MEAS_TOTAL_DST_MEAS_PRESENT_POS = 1,
    RSCP_MEAS_TOTAL_DST_MEAS_PRESENT_BIT = CO_BIT(RSCP_MEAS_TOTAL_DST_MEAS_PRESENT_POS),
    /// Walking or Running
    /// 0 Walking and 1 Running
    RSCP_MEAS_WALK_RUN_STATUS_POS = 2,
    RSCP_MEAS_WALK_RUN_STATUS_BIT = CO_BIT(RSCP_MEAS_WALK_RUN_STATUS_POS),
};

/// RSC Feature Flags bit field
enum rscp_feat_flags_bf
{
    /// Instantaneous Stride Length Measurement Supported
    RSCP_FEAT_INST_STRIDE_LEN_SUPP_POS = 0,
    RSCP_FEAT_INST_STRIDE_LEN_SUPP_BIT = CO_BIT(RSCP_FEAT_INST_STRIDE_LEN_SUPP_POS),

    /// Total Distance Measurement Supported
    RSCP_FEAT_TOTAL_DST_MEAS_SUPP_POS = 1,
    RSCP_FEAT_TOTAL_DST_MEAS_SUPP_BIT = CO_BIT(RSCP_FEAT_TOTAL_DST_MEAS_SUPP_POS),

    /// Walking or Running Status Supported
    RSCP_FEAT_WALK_RUN_STATUS_SUPP_POS = 2,
    RSCP_FEAT_WALK_RUN_STATUS_SUPP_BIT = CO_BIT(RSCP_FEAT_WALK_RUN_STATUS_SUPP_POS),

    /// Calibration Procedure Supported
    RSCP_FEAT_CALIB_PROC_SUPP_POS = 3,
    RSCP_FEAT_CALIB_PROC_SUPP_BIT = CO_BIT(RSCP_FEAT_CALIB_PROC_SUPP_POS),

    /// Multiple Sensor Locations Supported
    RSCP_FEAT_MULT_SENSOR_LOC_SUPP_POS = 4,
    RSCP_FEAT_MULT_SENSOR_LOC_SUPP_BIT = CO_BIT(RSCP_FEAT_MULT_SENSOR_LOC_SUPP_POS),
};

/// Sensor Locations Keys
enum rscp_sensor_loc
{
    /// Other (0)
    RSCP_LOC_OTHER          = 0,
    /// Top of shoe (1)
    RSCP_LOC_TOP_SHOE,
    /// In shoe (2)
    RSCP_LOC_IN_SHOE,
    /// Hip (3)
    RSCP_LOC_HIP,
    /// Chest (14)
    RSCP_LOC_CHEST          = 14,

    /// Maximum Sensor Location
    RSCP_LOC_MAX,
};

/// Control Point Operation Code Keys
enum rscp_sc_ctnl_pt_op_code
{
    /// Reserved value
    RSCP_CTNL_PT_OP_RESERVED        = 0,

    /// Set Cumulative Value
    RSCP_CTNL_PT_OP_SET_CUMUL_VAL,
    /// Start Sensor Calibration
    RSCP_CTNL_PT_OP_START_CALIB,
    /// Update Sensor Location
    RSCP_CTNL_PT_OP_UPD_LOC,
    /// Request Supported Sensor Locations
    RSCP_CTNL_PT_OP_REQ_SUPP_LOC,

    /// Response Code
    RSCP_CTNL_PT_RSP_CODE           = 16,
};

/// Control Point Response Value
enum rscp_sc_ctnl_pt_resp_val
{
    /// Reserved value
    RSCP_CTNL_PT_RESP_RESERVED      = 0,

    /// Success
    RSCP_CTNL_PT_RESP_SUCCESS,
    /// Operation Code Not Supported
    RSCP_CTNL_PT_RESP_NOT_SUPP,
    /// Invalid Parameter
    RSCP_CTNL_PT_RESP_INV_PARAM,
    /// Operation Failed
    RSCP_CTNL_PT_RESP_FAILED,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// RSC Measurement
typedef struct rscp_rsc_meas
{
    /// Flags
    uint8_t flags;
    /// Instantaneous Cadence
    uint8_t inst_cad;
    /// Instantaneous Speed
    uint16_t inst_speed;
    /// Instantaneous Stride Length
    uint16_t inst_stride_len;
    /// Total Distance
    uint32_t total_dist;
} rscp_rsc_meas_t;


/// SC Control Point Request Value
union rscp_sc_ctnl_pt_req_val
{
    /// Sensor Location
    uint8_t sensor_loc;
    /// Cumulative value
    uint32_t cumul_val;
};

/// SC Control Point Response Value
union rscp_sc_ctnl_pt_rsp_val
{
    /// List of supported locations
    uint16_t supp_sensor_loc;
    // Only valid on server side
    /// Sensor Location
    uint8_t  sensor_loc;
    /// Cumulative value
    uint32_t cumul_val;
};




/// @} rscp_common

#endif //(_RSCP_COMMON_H_)

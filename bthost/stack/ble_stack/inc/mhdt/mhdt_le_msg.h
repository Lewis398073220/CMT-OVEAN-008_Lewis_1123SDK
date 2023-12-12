/**
 ****************************************************************************************
 *
 * @file mhdt_le_msg.h
 *
 * @brief mhdt_le - LE mhdt
 *
 * Copyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

#ifndef _MHDT_LE_MSG_H_
#define _MHDT_LE_MSG_H_
/**
 ****************************************************************************************
 * @addtogroup mHDT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************/
#include "rwip_config.h"

#if (mHDT_LE_SUPPORT)
#include "rwip_task.h"

#if (HOST_MSG_API)
#include "ke_msg.h"                 // KE Message handler
#include "ke_task.h"                // KE Task handler
#endif // (HOST_MSG_API)

#include "mhdt_le_msg.h"
/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */
#ifndef __ARRAY_EMPTY
#define __ARRAY_EMPTY 1
#endif
/*
 * MESSAGES
 ****************************************************************************************
 */

/// Message API of the MHDT_LE task
/*@TRACE*/
enum mhdt_le_msg_id
{
    /// MHDT_LE command
    MHDT_LE_CMD                = MSG_ID(MHDT_LE, 0x00),
    /// MHDT_LE command complete event
    MHDT_LE_CMP_EVT            = MSG_ID(MHDT_LE, 0x01),
    /// MHDT_LE Indication
    MHDT_LE_IND                = MSG_ID(MHDT_LE, 0x02),
    /// MHDT_LE request indication
    MHDT_LE_REQ_IND            = MSG_ID(MHDT_LE, 0x03),
    /// MHDT_LE confirmation
    MHDT_LE_CFM                = MSG_ID(MHDT_LE, 0x04),
};


/// MHDT_LE_CMD command codes
/*@TRACE*/
enum mhdt_le_cmd_code
{
    MHDT_LE_RD_LOCAL_PROPRIETARY_FEAT_CMD    = 0x00,

    MHDT_LE_RD_RM_PROPRIETARY_FEAT_CMD       = 0x01,

    MHDT_LE_MABR_SET_DFT_LABEL_PARAMS_CMD    = 0x02,
};


/// MHDT_LE_IND indication codes
/*@TRACE*/
enum mhdt_le_ind_code
{
    /// Event triggered when an unknown message has been received by MHDT_LE layer from an upper layer.
    MHDT_LE_UNKNOWN_MSG                   = 0x00,

    MHDT_LE_REMOTE_FEAT_IND               = 0x01,

    MHDT_LE_LOCAL_FEAT_IND                = 0x02,
};


/*
 * STRUCTURES
 ****************************************************************************************
 */


/*
 * DEFAULT MESSAGE CONTENT
 ****************************************************************************************
 */


/*
 * STRUCTURES
 ****************************************************************************************
 */
enum mhdt_le_features
{
    MHDT_LE_FEATURE_4M_PHY      = 0,
    MHDT_LE_FEATURE_MABR_SUP    = 8,
};

/*
 * DEFAULT MESSAGE CONTENT
 ****************************************************************************************
 */

/// Default MHDT_LE command structure
/*@TRACE*/
typedef struct mhdt_le_cmd
{
    /// Command code (see enum #mhdt_le_cmd_code)
    uint16_t cmd_code;

    uint8_t param[__ARRAY_EMPTY];
} mhdt_le_cmd_t;

/// Default MHDT_LE command Complete structure
/*@TRACE*/
typedef struct mhdt_le_cmp_evt
{
    /// Command code (see enum #mhdt_le_cmd_code)
    uint16_t cmd_code;
    /// Status of the operation (see enum #hl_err)
    uint16_t status;
} mhdt_le_cmp_evt_t;

/// MHDT_LE_UNKNOWN_MSG Indication structure definition
/*@TRACE*/
typedef struct mhdt_le_unknown_msg_ind
{
    /// Indication code (see enum #mhdt_le_ind_code)
    ///  - MHDT_LE_UNKNOWN_MSG
    uint16_t ind_code;
    /// Message identifier
    uint16_t msg_id;
} mhdt_le_unknown_msg_ind_t;

/// Default MHDT_LE indication structure
/*@TRACE*/

typedef struct mhdt_le_remote_feature_ind
{
    /// Indication code (see enum #mhdt_le_ind_code)
    uint16_t ind_code;

    uint16_t status;

    uint16_t conidx;

    uint32_t feat;
} mhdt_le_remote_feature_ind_t;

typedef struct mhdt_le_local_feature_ind
{
    /// Indication code (see enum #mhdt_le_ind_code)
    uint16_t ind_code;

    uint16_t status;

    uint32_t feat;
} mhdt_le_local_feature_ind_t;

struct mhdt_mabr_dft_label_params
{
    /// (true/false)
    uint8_t keep_sdu_sync_ref_const;
    /// allow max PDU to change (true/false)
    uint8_t allow_max_pdu_change;
    /// allow TX PHY to change (true/false)
    uint8_t allow_tx_phy_change;
    /// allow RX PHY to change (true/false)
    uint8_t allow_rx_phy_change;
    /// LC3 or LC3plus
    uint8_t codec_used;
};

typedef struct mhdt_mabr_dft_label_params mhdt_mabr_dft_label_params_t;
#endif
#endif
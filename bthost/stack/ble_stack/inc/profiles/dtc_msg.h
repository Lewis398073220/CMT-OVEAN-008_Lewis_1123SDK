/**
 *
 * @file dtc_msg.h
 *
 * @brief Object Transfer Client - Message API Definitions
 *
 * @copyright Copyright (c) 2015-2021 BES Technic.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 */
/**
 ****************************************************************************************/

#ifndef DTC_MSG_H_
#define DTC_MSG_H_

/**
 ****************************************************************************************
 * @defgroup DTC_MSG_API Message API
 * @ingroup DTC
 * @brief Description of Message API for Object Transfer Profile Client module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "dtc.h"           // Object Transfer Client Definitions

#if (BLE_DT_CLIENT)

/// @addtogroup DTC_MSG_API
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Messages for Object Transfer Client
enum dtc_msg_id
{
    DTC_CMD = MSG_ID(DTC, 0x00),
    DTC_CMP_EVT = MSG_ID(DTC, 0x01),
    DTC_REQ = MSG_ID(DTC, 0x02),
    DTC_RSP = MSG_ID(DTC, 0x03),
    DTC_IND = MSG_ID(DTC, 0x04),
};

/// List of DTC_IND indication codes
enum dtc_msg_ind_codes
{
    DTC_UNKNOWN_MSG = 0x0000,
    DTC_COC_CONNECTED = 0x0001,
    DTC_COC_DISCONNECTED = 0x0002,
    DTC_COC_DATA = 0x0003,
};

/*
 * KERNEL MESSAGES
 ****************************************************************************************
 */

/// Basic structure for DTC_CMD message
typedef struct dtc_cmd
{
    /// Command code (see enum #dtc_cmd_codes)
    uint16_t cmd_code;
} dtc_cmd_t;

/// Basic structure for DTC_REQ message
typedef struct dtc_req
{
    /// Request code (see enum #dtc_msg_req_codes)
    uint16_t req_code;
} dtc_req_t;

/// Basic structure for DTC_IND message
typedef struct dtc_ind
{
    /// Indication code (see enum #dtc_msg_ind_codes)
    uint16_t ind_code;
} dtc_ind_t;

// Structure for DTC_COC_CONNECT command message
typedef struct dtc_coc_connect_cmd
{
    /// Command code (see enum #dtc_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Maximum SDU size that the local device can receive
    uint16_t local_max_sdu;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dtc_coc_connect_cmd_t;

/// Structure for DTC_COC_DISCONNECT command message
typedef struct dtc_coc_disconnect_cmd
{
    /// Command code (see enum #dtc_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dtc_coc_disconnect_cmd_t;

/// Structure for DTC_COC_SEND command message
typedef struct dtc_coc_send_cmd
{
    /// Command code (see enum #dtc_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// SDU data length
    uint16_t length;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
    /// SDU data to be tranferred to the peer device
    uint8_t sdu[__ARRAY_EMPTY];
} dtc_coc_send_cmd_t;

/// Structure for DTC_COC_RELEASE command message
typedef struct dtc_coc_release_cmd
{
    /// Command code (see enum #dtc_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dtc_coc_release_cmd_t;


/// Structure command complete event
typedef struct dtc_cmp_evt
{
    /// Command code (see enum #dtc_cmd_codes)
    uint16_t cmd_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dtc_cmp_evt_t;

/// Structure for DTC_UNKNOWN_MSG indication message
typedef struct dtc_unknown_msg_ind
{
    /// Indication code (see enum #dtc_msg_ind_codes)
    uint16_t ind_code;
    /// Message ID
    ke_msg_id_t msg_id;
} dtc_unknown_msg_ind_t;

/// Structure for DTC_COC_CONNECTED indication message
typedef struct dtc_coc_connected_ind
{
    /// Indication code (see enum #dtc_msg_ind_codes)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Maximum SDU size that the peer on the link can receive
    uint16_t tx_mtu;
    /// Maximum packet size that the peer on the link can receive
    uint16_t tx_mps;
    /// Connected L2CAP channel local index
    uint8_t chan_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dtc_coc_connected_ind_t;

/// Structure for DTC_COC_DISCONNECTED indication message
typedef struct dtc_coc_disconnected_ind
{
    /// Indication code (see enum #dtc_msg_ind_codes)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Disconnection reason
    uint16_t reason;
    /// Connected L2CAP channel local index
    uint8_t chan_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dtc_coc_disconnected_ind_t;

/// Structure for DTC_COC_DATA indication message
typedef struct dtc_coc_data_ind
{
    /// Indication code (see enum #dtc_msg_ind_codes)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// SDU data length
    uint16_t length;
    /// Connected L2CAP channel local index
    uint8_t chan_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
    /// SDU data
    uint8_t sdu[__ARRAY_EMPTY];
} dtc_coc_data_ind_t;

#endif // BLE_DT_CLIENT
#endif //DTC_MSG_H_
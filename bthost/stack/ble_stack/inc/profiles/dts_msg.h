/**
 *
 * @file dts_msg.h
 *
 * @brief Object Transfer Server - Message API Definitions
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

#ifndef DTS_MSG_H_
#define DTS_MSG_H_

/**
 ****************************************************************************************
 * @defgroup DTS_MSG_API Message API
 * @ingroup DTS
 * @brief Description of Message API for Object Transfer Profile Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <dts.h>           // Object Transfer Server Definitions

#if (BLE_DT_SERVER)

/// @addtogroup DTS_MSG_API
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Messages for Object Transfer Server
enum dts_msg_id
{
    DTS_CMD = MSG_ID(DTS, 0x00),
    DTS_CMP_EVT = MSG_ID(DTS, 0x01),
    DTS_REQ = MSG_ID(DTS, 0x02),
    DTS_RSP = MSG_ID(DTS, 0x03),
    DTS_IND = MSG_ID(DTS, 0x04),
    DTS_REQ_IND = MSG_ID(DTS, 0x05),
    DTS_CFM = MSG_ID(DTS, 0x06),
    // DTS_CONFIGURE_IND = MSG_ID(DTS, 0x07),
};

/// List of DTS_IND indication codes
enum dts_msg_ind_codes
{
    DTS_UNKNOWN_MSG = 0x0000,
    DTS_COC_CONNECTED = 0x0001,
    DTS_COC_DISCONNECTED = 0x0002,
    DTS_COC_DATA = 0x0003,
    DTS_COC_CONFIGURED = 0x0004,
};

/*
 * KERNEL MESSAGES
 ****************************************************************************************
 */

/// Basic structure for DTS_CMD message
typedef struct dts_cmd
{
    /// Command code (see enum #dts_cmd_codes)
    uint16_t cmd_code;
} dts_cmd_t;

/// Basic structure for DTS_REQ message
typedef struct dts_req
{
    /// Request code (see enum #dts_msg_req_codes)
    uint16_t req_code;
} dts_req_t;

/// Basic structure for DTS_IND message
typedef struct dts_ind
{
    /// Indication code (see enum #dts_msg_ind_codes)
    uint16_t ind_code;
} dts_ind_t;

/// Basic structure for DTS_CFM message
typedef struct dts_cfm
{
    /// Request Indication code (see enum #dts_msg_req_ind_codes)
    uint16_t req_ind_code;
    /// Status
    uint16_t status;
} dts_cfm_t;

/// Structure for response message
typedef struct dts_rsp
{
    /// Request code (see enum #dts_msg_req_codes)
    uint16_t req_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
} dts_rsp_t;

/// Structure for DTS_COC_REGISTER command message
typedef struct dts_coc_register_cmd
{
    /// Command code (see enum #dts_cmd_codes)
    uint16_t cmd_code;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
    /// initial credits
    uint16_t initial_credits;
} dts_coc_register_cmd_t;

/// Structure for DTS_COC_DISCONNECT command message
typedef struct dts_coc_disconnect_cmd
{
    /// Command code (see enum #dts_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_coc_disconnect_cmd_t;

/// Structure for DTS_COC_SEND command message
typedef struct dts_coc_send_cmd
{
    /// Command code (see enum #dts_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// SDU data length
    uint16_t length;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
    /// SDU data to be tranferred to the peer device
    const uint8_t *sdu;
} dts_coc_send_cmd_t;

/// Structure for DTS_COC_RELEASE command message
typedef struct dts_coc_release_cmd
{
    /// Command code (see enum #dts_cmd_codes)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_coc_release_cmd_t;

/// Structure command complete event
typedef struct dts_cmp_evt
{
    /// Command code (see enum #dts_cmd_codes)
    uint16_t cmd_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_cmp_evt_t;

/// Structure for DTS_ADD request message
typedef struct dts_add_req
{
    /// Request code (see enum #dts_msg_req_codes)
    uint16_t req_code;
    /// Configuration bit field (see enum #dts_add_cfg_bf)
    uint16_t cfg_bf;
    /// Required start handle
    /// If set to GATT_INVALID_LID, the start handle will be automatically chosen
    uint16_t shdl;
    /// Object Action Control Point features
    uint32_t oacp_features;
    /// Object List Control Point features
    uint32_t olcp_features;
} dts_add_req_t;

/// Structure for DTS_UNKNOWN_MSG indication message
typedef struct dts_unknown_msg_ind
{
    /// Indication code (see enum #dts_msg_ind_codes)
    uint16_t ind_code;
    /// Message ID
    ke_msg_id_t msg_id;
} dts_unknown_msg_ind_t;

/// Structure for DTS_COC_CONNECTED indication message
typedef struct dts_coc_connected_ind
{
    /// Indication code (see enum #dts_msg_ind_codes)
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
    /// initial credits
    uint16_t initial_credits;
} dts_coc_connected_ind_t;

/// Structure for DTS_COC_DISCONNECTED indication message
typedef struct dts_coc_disconnected_ind
{
    /// Indication code (see enum #dts_msg_ind_codes)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Disconnection reason
    uint16_t reason;
    /// Connected L2CAP channel local index
    uint8_t chan_lid;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_coc_disconnected_ind_t;

/// Structure for DTS_COC_DATA indication message
typedef struct dts_coc_data_ind
{
    /// Indication code (see enum #dts_msg_ind_codes)
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
} dts_coc_data_ind_t;

/// Structure for DTS_COC_CONNECT request indication message
typedef struct dts_coc_connect_req_ind
{
    /// Request Indication code (see enum #dts_msg_req_ind_codes)
    uint16_t req_ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Token value to return in the confirmation
    uint16_t token;
    /// Maximum SDU size that the peer on the link can receive
    uint16_t peer_max_sdu;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_coc_connect_req_ind_t;

/// Structure for DTS_COC_CONNECT confirmation messages
typedef struct dts_coc_connect_cfm
{
    /// Request Indication code (see enum #dts_msg_req_ind_codes)
    uint16_t req_ind_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
    /// Token value to return in the confirmation
    uint16_t token;
    /// Maximum SDU size that the local device can receive
    uint16_t local_max_sdu;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_coc_connect_cfm_t;

/// Structure command complete event
typedef struct dts_configured_ind
{
    /// Request Indication code (see enum #dts_msg_req_ind_codes)
    uint16_t req_ind_code;
    /// Status
    uint16_t status;
    /// Simplified Protocol/Service Multiplexer
    uint16_t spsm;
} dts_registerd_ind_t;

/// @} DTS_MSG_API

#endif //(BLE_DT_SERVER)

#endif // DTS_MSG_H_

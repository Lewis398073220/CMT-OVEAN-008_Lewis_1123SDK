/**
 *
 * @file dts.h
 *
 * @brief Data Transfer Client - Definitions
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
#ifndef DTC_H_
#define DTC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf_types.h"              // Profiles Types Definition
#include "co_math.h"                // Common Math Definitions

#if (BLE_DT_SERVER)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup OTS_ENUM
/// @{

/// List of OTS_CMD command codes
enum dts_cmd_codes
{
    DTS_COC_REGISTER = 0x0000,
    DTS_COC_DISCONNECT = 0x0001,
    DTS_COC_SEND = 0x0002,
    DTS_COC_RELEASE = 0x0003,
};

/// List of OTS_REQ_IND request indication codes
enum dts_msg_req_ind_codes
{
    DTS_COC_CONNECT = 0x0000,
};

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// @addtogroup OTS_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called when an LE Credit Based Connection Oriented Link has
 * been established
 *
 * @param[in] con_lid       Connection local index
 * @param[in] tx_mtu        Transmission Maximum Transmit Unit Size, mtu that peer device can receive
 * @param[in] tx_mps        Transmission Maximum Packet Size, mps that peer device can receive
 * @param[in] chan_lid      Connected L2CAP channel local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 * @param[in] initial_credits  Initial credits 
 ****************************************************************************************
 */
typedef void (*dts_cb_coc_connected)(uint8_t con_lid, uint16_t tx_mtu,
                                     uint16_t tx_mps, uint8_t chan_lid,
                                     uint16_t spsm, uint16_t initial_credits);

/**
 ****************************************************************************************
 * @brief Callback function called when an LE Credit Based Connection Oriented Link has
 * been disconnected
 *
 * @param[in] con_lid       Connection local index
 * @param[in] reason        Disconnection reason
 * @param[in] chan_lid      Connected L2CAP channel local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 *****************************************************************************************
 */
typedef void (*dts_cb_coc_disconnected)(uint8_t con_lid, uint16_t reason, uint8_t chan_lid,
                                        uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Callback function called when a peer Client requests to establish a LE Credit
 * Based Connection Oriented Link
 *
 * @param[in] con_lid       Connection local index
 * @param[in] token         Token value to return in the confirmation
 * @param[in] peer_max_sdu  Maximum SDU size that the peer on the link can receive
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 ****************************************************************************************
 */
typedef void (*dts_cb_coc_connect)(uint8_t con_lid, uint16_t token, uint16_t peer_max_sdu,
                                    uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Callback function called when data is received through LE Credit Based Connection
 * Oriented Link
 *
 * @param[in] con_lid       Connection local index
 * @param[in] length        SDU data length
 * @param[in] chan_lid      Connected L2CAP channel local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 * @param[in] p_sdu         Pointer to SDU data
 *****************************************************************************************
 */
typedef void (*dts_cb_coc_data)(uint8_t con_lid, uint16_t length, uint8_t chan_lid, uint16_t spsm, const uint8_t* p_sdu);

/**
 ****************************************************************************************
 * @brief Callback function called when a command has been handled
 *
 * @param[in] cmd_code      Command code (see enum #otc_cmd_codes)
 * @param[in] status        Status
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 ****************************************************************************************
 */
typedef void (*dts_cb_cmp_evt)(uint16_t cmd_code, uint16_t status,
                                uint8_t con_lid, uint16_t spsm);

/// Set of callback functions for Object Transfer Server
typedef struct dts_cb
{
    /// Callback function called when an LE Credit Based Connection Oriented Link has
    /// been established for an instance of the Object Transfer Service
    dts_cb_coc_connected cb_coc_connected;
    /// Callback function called when an LE Credit Based Connection Oriented Link has
    /// been disconnected for an instance of the Object Transfer Service
    dts_cb_coc_disconnected cb_coc_disconnected;
    /// Callback function called when data is received through LE Credit Based Connection
    /// Oriented Link for an instance of the Object Transfer Service
    dts_cb_coc_data cb_coc_data;
    /// Callback function called when a peer Client requests to establish a LE Credit
    /// Based Connection Oriented Link for an instance of the Object Transfer Service
    dts_cb_coc_connect cb_coc_connect;
    /// Callback function called when a command has been handled
    dts_cb_cmp_evt cb_cmp_evt;
} dts_cb_t;


/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup OTS_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief register a spsm in order to accept L2CAP connection oriented channel (COC) from a peer device
 *
 * @param[in] spsm              Simplified Protocol/Service Multiplexer
 * @param[in] initial_credits   Initial rx credits
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t dts_coc_register(uint16_t spsm, uint16_t initial_credits);

/**
 ****************************************************************************************
 * @brief Disconnect a LE Credit Based Connection Oriented Link
 *
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 * 
 * @return An error status
 ****************************************************************************************
 */
uint16_t dts_coc_disconnect(uint8_t con_lid, uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Release the earliest buffer which has been consumed by preceding data reception triggered
 * by OTS_DATA indication.
 *
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t dts_coc_release_earliest_entry(uint8_t con_lid, uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Transfer Object content data through a LE Credit Based Connection Oriented Link
 *
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 * @param[in] length        SDU length
 * @param[in] p_sdu         Pointer to SDU to be transferred to the peer device
 * @return An error status
 ****************************************************************************************
 */
uint16_t dts_coc_send(uint8_t con_lid, uint16_t spsm, uint16_t length, const uint8_t* p_sdu);

/**
 ****************************************************************************************
 * @brief Confirmation for OTS_COC_CONNECT request indication
 *
 * @param[in] status            Status
 * @param[in] con_lid           Connection local index
 * @param[in] token             Token value to return in the confirmation
 * @param[in] local_max_sdu     Maximum SDU size that the local device can receive
 * @param[in] spsm              Simplified Protocol/Service Multiplexer
 ****************************************************************************************
 */
void dts_cfm_coc_connect(uint16_t status, uint8_t con_lid,
                         uint16_t token, uint16_t local_max_sdu, uint16_t spsm);

#endif //(BLE_DT_SERVER)

#endif // DTS_H_

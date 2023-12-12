/**
 *
 * @file dtc.h
 *
 * @brief Data Transfer Profile - Definitions
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

#if (BLE_DT_CLIENT)


/// List of DTC_CMD command codes
enum dtc_cmd_codes
{
    DTC_COC_CONNECT = 0x0000,
    DTC_COC_DISCONNECT = 0x0001,
    DTC_COC_SEND = 0x0002,
    DTC_COC_RELEASE = 0x0003,
};


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
 ****************************************************************************************
 */
typedef void (*dtc_cb_coc_connected)(uint8_t con_lid, uint16_t tx_mtu,
                                     uint16_t tx_mps, uint8_t chan_lid, uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Callback function called when an LE Credit Based Connection Oriented Link has
 * been disconnected for an instance of the Data Transfer Service
 *
 * @param[in] con_lid       Connection local index
 * @param[in] reason        Disconnection reason
 * @param[in] chan_lid      Connected L2CAP channel local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 ****************************************************************************************
 */
typedef void (*dtc_cb_coc_disconnected)(uint8_t con_lid, uint16_t reason, uint8_t chan_lid, uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Callback function called when data is received through LE Credit Based Connection
 * Oriented Link for an instance of the Data Transfer Service
 *
 * @param[in] con_lid       Connection local index
 * @param[in] length        SDU data length
 * @param[in] chan_lid      Connected L2CAP channel local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 * @param[in] p_sdu         Pointer to SDU data
 ****************************************************************************************
 */
typedef void (*dtc_cb_coc_data)(uint8_t con_lid, uint16_t length, uint8_t chan_lid, uint16_t spsm, const uint8_t* p_sdu);

/**
 ****************************************************************************************
 * @brief Callback function called when a command has been handled
 *
 * @param[in] cmd_code      Command code (see enum #dtc_cmd_codes)
 * @param[in] status        Status
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 ****************************************************************************************
 */
typedef void (*dtc_cb_cmp_evt)(uint16_t cmd_code, uint16_t status, uint8_t con_lid, uint16_t spsm);

/// Set of callback functions for Data Transfer Client
typedef struct dtc_cb
{
    /// Callback function called when an LE Credit Based Connection Oriented Link has
    /// been established for an instance of the Data Transfer Service
    dtc_cb_coc_connected cb_coc_connected;
    /// Callback function called when an LE Credit Based Connection Oriented Link has
    /// been established for an instance of the Data Transfer Service
    dtc_cb_coc_disconnected cb_coc_disconnected;
    /// Callback function called when data is received through LE Credit Based Connection
    /// Oriented Link for an instance of the Data Transfer Service
    dtc_cb_coc_data cb_coc_data;
    /// Callback function called when a command has been handled
    dtc_cb_cmp_evt cb_cmp_evt;
} dtc_cb_t;

/**
 ****************************************************************************************
 * @brief Establish a LE Credit Based Connection Oriented Link
 *
 * @param[in] con_lid       Connection local index
 * @param[in] local_max_sdu Maximum SDU size that the local device can receive
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t dtc_coc_connect(uint8_t con_lid, uint16_t local_max_sdu, uint16_t spsm);

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
uint16_t dtc_coc_disconnect(uint8_t con_lid, uint16_t spsm);

/**
 ****************************************************************************************
 * @brief Transfer data through a LE Credit Based Connection Oriented Link
 *
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 * @param[in] length        SDU length
 * @param[in] p_sdu         Pointer to SDU to be transferred to the peer device
 *
 * @return An error status
 * ****************************************************************************************
 */
uint16_t dtc_coc_send(uint8_t con_lid, uint16_t spsm, uint16_t length, const uint8_t* p_sdu);

/**
 ****************************************************************************************
 * @brief Release buffers which have been consumed by preceding data reception triggered
 * by DTC_DATA indication
 *
 * @param[in] con_lid       Connection local index
 * @param[in] spsm          Simplified Protocol/Service Multiplexer
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t dtc_coc_release(uint8_t con_lid, uint16_t spsm);

#endif //(BLE_DT_CLIENT)

#endif // DTC_H_
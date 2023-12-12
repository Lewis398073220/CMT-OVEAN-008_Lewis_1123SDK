/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
 *
 ****************************************************************************/

#ifndef __BLE_AUDIO_CORE_API_H__
#define __BLE_AUDIO_CORE_API_H__

#include "ble_audio_core_evt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ble audio core initialize
 * @date 2021.07.20
 */
void ble_audio_core_init(void);

/**
 * @brief add these addresses to white list and start connectable adv
 *
 * @param addr
 * @param num_of_addr
 * @date 2021.07.20
 */
void ble_audio_start_mobile_connecting(ble_bdaddr_t *addr, uint8_t num_of_addr);

/**
 * @brief make a new leaudio sm
 *
 * @param conidx
 * @param peer_bdaddr
 * @date 2023.04.11
 */
bool ble_audio_make_new_le_core_sm(uint8_t conidx, uint8_t *peer_bdaddr);

/**
 * @brief disconnect the mobile device
 *
 * @param addr
 * @date 2021.07.20
 */
void ble_audio_mobile_req_disconnect(ble_bdaddr_t *addr);

/**
 * @brief return the ble link connect state
 *
 * @param addr
 * @date 2021.07.20
 */
bool ble_audio_is_mobile_link_connected(ble_bdaddr_t *addr);

/**
 * @brief disconnect the mobile device by connection id.
 *
 * @param connection id.
 * @date 2021.07.20
 */
void ble_audio_mobile_disconnect_device(uint8_t conidx);

/**
 * @brief disconnect all the leaudio mobile device
 *
 * @param addr
 * @date 2021.07.20
 */
void ble_audio_disconnect_all_connection(void);

/**
 * @brief determine if any ble audio mobile connected
 *
 * @return true
 * @return false
 * @date 2021.07.20
 */
bool ble_audio_is_any_mobile_connnected(void);


/**
 * @brief get ble-tws link conidx
 *
 * @return uint8_t
 * @date 2021.07.20
 */
uint8_t ble_audio_get_tws_link_conidx(void);


/**
 * @brief determine if ble-tws connected
 *
 * @return true
 * @return false
 * @date 2021.07.20
 */
bool ble_audio_tws_link_is_connected(void);


/**
 * @brief start tws connection, one earbud set white list, start adv and the other start connecting.
 *
 * @return true
 * @return false
 * @date 2021.07.20
 */
bool ble_audio_tws_connect_request(void);


/**
 * @brief cancel tws connecting request
 *
 * @return true
 * @return false
 * @date 2021.07.20
 */
bool ble_audio_tws_cancel_connecting_request(void);


/**
 * @brief disconnect tws link request
 *
 * @return true
 * @return false
 * @date 2021.07.20
 */
bool ble_audio_tws_disconnect_request(void);


/**
 * @brief dump ble audio core info, for debug
 *
 * @date 2021.07.20
 */
void ble_audio_dump_core_status();

/**
 * @brief get the local id of the connected mobile device
 *
 * @date 2021.08.05
 */
uint8_t ble_audio_get_mobile_connected_dev_lids(uint8_t con_lid[]);

/**
 * @brief get the remote device address by connection index
 *
 * @return connected device count
 * @date 2021.08.16
 */
ble_bdaddr_t *ble_audio_get_mobile_address_by_lid(uint8_t con_lid);

/**
 * @brief get the con_id by public address
 *
 * @return con_lid
 * @date 2021.08.16
 */
uint8_t ble_audio_get_mobile_lid_by_pub_address(uint8_t *pub_addr);

#if BLE_AUDIO_ENABLED
void ble_audio_sync_info(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

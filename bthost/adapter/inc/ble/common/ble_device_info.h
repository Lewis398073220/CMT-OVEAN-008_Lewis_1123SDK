/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#ifndef __BLE_DEVICE_INFO_H__
#define __BLE_DEVICE_INFO_H__

#include "stdint.h"
#include "gap_service.h"

#define BTIF_BD_ADDR_SIZE 6
#define BLE_ADDR_SIZE 6
#define BLE_ENC_RANDOM_SIZE 8
#define BLE_LTK_SIZE 16
#define BLE_IRK_SIZE 16
#define BLE_CSRK_SIZE 16

typedef struct {
    uint8_t ble_addr[BTIF_BD_ADDR_SIZE];
    uint8_t ble_irk[BLE_IRK_SIZE];
    uint8_t ble_csrk[BLE_CSRK_SIZE];
} __attribute__ ((packed)) BLE_BASIC_INFO_T;

typedef struct
{
    /// BD Address of device
    uint8_t addr[BLE_ADDR_SIZE];
    /// Address type of the device 0=public/1=private random
    uint8_t addr_type;
} __attribute__ ((packed)) BLE_ADDR_INFO_T;

enum bond_info
{
    BONDED_STATUS_POS       = 0,
    BONDED_WITH_IRK_DISTRIBUTED = 1,
    BONDED_WITH_CSRK_DISTRIBUTED = 2,
    BONDED_SECURE_CONNECTION = 3,
    BONDED_WITH_NUM_COMPARE = 4,
    BONDED_WITH_PASSKEY_ENTRY = 5,
    BONDED_WITH_OOB_METHOD = 6,
};

typedef struct {
    BLE_ADDR_INFO_T peer_addr;
    uint8_t bond_info_bf;
    uint16_t EDIV;
    uint16_t LOCAL_EDIV;
    uint8_t RANDOM[BLE_ENC_RANDOM_SIZE];
    uint8_t LTK[BLE_LTK_SIZE];
    uint8_t LOCAL_RANDOM[BLE_ENC_RANDOM_SIZE];
    uint8_t LOCAL_LTK[BLE_LTK_SIZE];
    uint8_t IRK[BLE_IRK_SIZE];
    uint8_t CSRK[BLE_CSRK_SIZE];
    uint8_t enc_key_size;
#ifndef BLE_STACK_NEW_DESIGN
    uint8_t peer_rpa_addr[BLE_ADDR_SIZE];
#endif
#ifdef BLE_STACK_NEW_DESIGN
    gatt_server_cache_t server_cache;
#endif
} __attribute__ ((packed)) BleDevicePairingInfo;

typedef struct {
    BleDevicePairingInfo pairingInfo;
    uint8_t volume;
} __attribute__ ((packed)) BleDeviceinfo;

#endif /* __BLE_DEVICE_INFO_H__ */

/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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

#ifndef __APP_BLE_AUDIO_CENTRAL_PAIRING_STM_H__
#define __APP_BLE_AUDIO_CENTRAL_PAIRING_STM_H__

#include "ble_audio_define.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
    BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_START,
    BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_START,
    BLE_AUDIO_CENTRAL_REQ_RECONN_OLD_HEADSET_STOP,
    BLE_AUDIO_CENTRAL_REQ_SCAN_NEW_HEADSET_STOP,
    BLE_AUDIO_CENTRAL_REQ_SCAN_CONNECT_ALTERNATE_HEADSET_START,
    PAIRING_EVENT_MAX,
} BLE_AUDIO_CENTRAL_PAIRING_EVENT_E;

typedef enum
{
    BLE_AUDIO_CENTRAL_PAIRING_STATE_IDLE,
    BLE_AUDIO_CENTRAL_PAIRING_STATE_RECONN_STARTED,
    BLE_AUDIO_CENTRAL_PAIRING_STATE_SCAN_STARTED,
    BLE_AUDIO_CENTRAL_PAIRING_STATE_UNKNOW,
} BLE_AUDIO_CENTRAL_PAIRING_STATE_E;

typedef struct
{
    /* ase hsm define */
    Hsm super;

    State idle;
    State reconn_old_headset_start;
    State scan_new_headset_start;
    State scan_reconn_alternate_start;

    bool used;
} ble_audio_central_pairing_stm_t;


typedef struct
{
    void *p_pend_stm;
    BLE_AUDIO_CENTRAL_PAIRING_EVENT_E op;
} ble_audio_central_pairing_PENDING_INFO_T;

int ble_audio_central_pairing_sm_on_event(uint32_t pairing_stm_ptr,uint32_t event,uint32_t param0,uint32_t param1);
void ble_audio_central_pairing_send_message(ble_audio_central_pairing_stm_t *p_pairing_sm, BLE_AUDIO_CENTRAL_PAIRING_EVENT_E event, uint32_t param0, uint32_t param1);
ble_audio_central_pairing_stm_t* ble_audio_central_get_pairing_sm(void);
BLE_AUDIO_CENTRAL_PAIRING_STATE_E ble_audio_central_pairing_stm_get_cur_state(void);
void ble_audio_central_pairing_stm_startup(ble_audio_central_pairing_stm_t *ble_audio_central_pairing_sm);
void ble_audio_central_pairing_stm_init(void);
void ble_audio_central_start_pairing_state_machine(void);

#ifdef __cplusplus
}
#endif

#endif


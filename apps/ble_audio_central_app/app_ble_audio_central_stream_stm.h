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

#ifndef __APP_BLE_AUDIO_CENTRAL_STREAM_STM_H__
#define __APP_BLE_AUDIO_CENTRAL_STREAM_STM_H__

#include "ble_audio_define.h"

#include "hsm.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
    BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_START,
    BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_START,
    BLE_AUDIO_CENTRAL_REQ_STREAM_PLAYBACK_STOP,
    BLE_AUDIO_CENTRAL_REQ_STREAM_CAPTURE_STOP,

    EVT_PLAYBACK_STREAM_STARTED,
    EVT_CAPTURE_STREAM_STARTED,

    EVT_PLAYBACK_STREAM_STOPPED,
    EVT_CAPTURE_STREAM_STOPPED,

    BLE_AUDIO_CENTRAL_REQ_REFRESH_PLAYBACK_STREAM,
    BLE_AUDIO_CENTRAL_REQ_REFRESH_CAPTURE_STREAM,

    STREAM_EVENT_MAX,
} BLE_AUDIO_CENTRAL_STREAM_EVENT_E;

typedef enum
{
    BLE_AUDIO_CENTRAL_STREAM_STATE_IDLE,
    BLE_AUDIO_CENTRAL_STREAM_STATE_PLAYBACK_STARTED,
    BLE_AUDIO_CENTRAL_STREAM_STATE_CAPTURE_STARTED,
    BLE_AUDIO_CENTRAL_STREAM_STATE_BOTH_STARTED,

    BLE_AUDIO_CENTRAL_STREAM_STATE_UNKNOW,
} BLE_AUDIO_CENTRAL_STREAM_STATE_E;

typedef void (*ble_audio_central_stream_start_receiving_func)(void);
typedef void (*ble_audio_central_stream_start_transmission_func)(void);
typedef void (*ble_audio_central_stream_stm_post_op_check_func)(BLE_AUDIO_CENTRAL_STREAM_EVENT_E event);
typedef void (*ble_audio_central_stream_stm_post_cis_discon_op_check_func)(void);

typedef struct
{
    /* ase hsm define */
    Hsm super;

    State idle;
    State playback_start;
    State capture_start;
    State both_start;

    bool used;
    ble_audio_central_stream_start_receiving_func       start_receiving_handler;
    ble_audio_central_stream_start_transmission_func    start_transmission_handler;
    ble_audio_central_stream_stm_post_op_check_func     post_op_checker;
    ble_audio_central_stream_stm_post_cis_discon_op_check_func     post_cis_discon_op_checker;
} ble_audio_central_stream_stm_t;

typedef struct
{
    void *p_pend_stm;
    BLE_AUDIO_CENTRAL_STREAM_EVENT_E op;
} USB_STREAM_PENDING_INFO_T;

void ble_audio_central_stream_start_receiving_handler_register(ble_audio_central_stream_start_receiving_func cb);
void ble_audio_central_stream_start_transmission_handler_register(ble_audio_central_stream_start_transmission_func cb);
void ble_audio_central_stream_post_operation_check_cb_register(ble_audio_central_stream_stm_post_op_check_func cb);
void ble_audio_central_stream_post_cis_discon_operation_check_cb_register(ble_audio_central_stream_stm_post_cis_discon_op_check_func cb);
void ble_audio_central_stream_post_cis_discon_operation_check(void);

void ble_audio_central_stream_resume_ble_audio(uint8_t con_lid);

void ble_audio_central_stream_send_message(ble_audio_central_stream_stm_t *p_stream_sm, BLE_AUDIO_CENTRAL_STREAM_EVENT_E event, uint32_t param0, uint32_t param1);
ble_audio_central_stream_stm_t* ble_audio_central_get_stream_sm(void);
BLE_AUDIO_CENTRAL_STREAM_STATE_E ble_audio_central_stream_stm_get_cur_state(void);

void ble_audio_central_stream_stm_startup(ble_audio_central_stream_stm_t *ble_audio_central_stream_sm);
void ble_audio_central_stream_stm_init(void);

#ifdef __cplusplus
}
#endif

#endif


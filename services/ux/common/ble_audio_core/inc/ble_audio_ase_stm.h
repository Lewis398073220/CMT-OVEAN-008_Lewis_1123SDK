/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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

#ifndef __BLE_AUDIO_ASE_STM_H__
#define __BLE_AUDIO_ASE_STM_H__
#ifdef AOB_MOBILE_ENABLED
#include "ble_audio_define.h"
#include "ble_aob_common.h"
#include "bes_aob_api.h"

#include "hsm.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
    BLE_REQ_ASE_START,
    BLE_REQ_ASE_CODEC_CONFIGURE,
    BLE_REQ_ASE_QOS_CONFIGURE,
    BLE_REQ_ASE_ENABLE,
    BLE_REQ_ASE_DISABLE,
    BLE_REQ_ASE_RELEASE,

    BLE_EVT_ASE_CODEC_CONFIGURED,

    /// Create unicast group cmp
    BLE_EVT_ASE_UC_GRP_CREATED,

    BLE_EVT_ASE_QOS_CONFIGURED,
    BLE_EVT_ASE_ENABLED,
    BLE_EVT_ASE_DISABLED,
    BLE_EVT_ASE_RELEASED,

    BLE_EVT_ASE_IDLE,

    /// Remove unicast group cmp
    BLE_EVT_ASE_UC_GRP_REMOVED,

    BLE_ASE_EVENT_MAX,
} BLE_ASE_EVENT_E;

typedef enum
{
    BLE_ASE_STATE_IDLE,
    BLE_ASE_STATE_CODEC_CONFIGUED,
    BLE_ASE_STATE_QOS_CONFIGUED,
    BLE_ASE_STATE_ENABLING,
    BLE_ASE_STATE_STREAMING,
    BLE_ASE_STATE_DISABLING,
    BLE_ASE_STATE_RELEASING,

    BLE_ASE_STATE_UNKNOW,
} BLE_ASE_STATE_E;

typedef struct
{
    uint16_t sample_rate;
    uint16_t frame_octet;
    bes_gaf_direction_t direction;
    const AOB_CODEC_ID_T *codec_id;
    uint16_t context_type;
} BLE_ASE_CFG_INFO_T;

typedef struct
{
    uint8_t ase_lid;
    uint8_t cis_id;
    BLE_ASE_CFG_INFO_T ase_cfg_info;
} BLE_ASE_ESTABLISH_INFO_T;

/* ble ase control block */
typedef struct
{
    /* ase hsm define */
    Hsm super;

    State idle;
    State codec_configure;
    State qos_configure;
    State enabling;
    State streaming;
    State disabling;
    State releasing;

    uint8_t con_lid;
    bool used;
    bool inProcess;

    uint8_t qos_cfg;
    uint8_t grp_lid;
    BLE_ASE_ESTABLISH_INFO_T ase_info;
    //TODO: Add protection timer for error handling
} ble_ase_stm_t;

typedef struct
{
    void *p_pend_stm;
    BLE_ASE_EVENT_E op;
} BLE_ASE_PENDING_INFO_T;

void ble_audio_ase_stm_send_msg(ble_ase_stm_t *p_ase_sm, BLE_ASE_EVENT_E event, uint32_t para0, uint32_t para1);
ble_ase_stm_t* ble_audio_find_ase_sm_by_ase_lid(uint8_t ase_lid);
ble_ase_stm_t* ble_audio_find_ase_sm_by_con_lid(uint8_t con_lid);
ble_ase_stm_t* ble_audio_find_ase_sm_by_grp_lid(uint8_t grp_lid);
BLE_ASE_STATE_E ble_audio_ase_stm_get_cur_state(uint8_t con_lid, uint8_t ase_lid);
BLE_ASE_STATE_E ble_ase_stm_get_other_ase_state(ble_ase_stm_t *p_ase);
ble_ase_stm_t* ble_audio_ase_stm_alloc(uint8_t con_lid, uint8_t ase_lid);
void ble_audio_ase_stm_startup(ble_ase_stm_t *ble_ase_sm);
void ble_audio_ase_stm_init(void);
void ble_ase_sm_start_auto_play(ble_ase_stm_t *me);
void ble_ase_stm_set_ase_stm_num_to_use(uint8_t con_lid, uint8_t ase_num);
void ble_ase_stm_increase_ase_stm_num_to_use(uint8_t con_lid, uint8_t step);

#ifdef __cplusplus
}
#endif
#endif

#endif

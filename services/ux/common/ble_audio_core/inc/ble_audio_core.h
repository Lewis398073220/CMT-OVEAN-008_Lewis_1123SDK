/**
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

#ifndef __BLE_AUDIO_CORE_H__
#define __BLE_AUDIO_CORE_H__

#include "ble_audio_core_evt.h"

#include "ble_audio_tws_stm.h"
#include "ble_audio_mobile_stm.h"

#ifdef AOB_MOBILE_ENABLED
#include "ble_audio_ase_stm.h"
#include "ble_audio_mobile_conn_stm.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    DISCOVER_START    = 0,
    DISCOVER_COMPLEPE = 1,

    DISCOVER_MAX      = 3,
}DISCOVER_OPERATE;

typedef enum{
    SERVICE_ACC_MCC = 0,
    SERVICE_ACC_TBC = 1,

    SERVICE_MAX     = 2,
}DISCOVER_SERVICE;

typedef enum {
    LEA_CI_MODE_SVC_DISC_START      = 0,
    LEA_CI_MODE_SVC_DISC_CMP        = 1,
    LEA_CI_MODE_ISO_DATA_ACT        = 2,
    LEA_CI_MODE_ISO_DATA_STOP       = 3,
    LEA_CI_MODE_DEFAULT             = 4,
} lea_ci_mode_t;

typedef struct
{
    uint32_t    mobile_reconnect_timeout;
    bool        is_ble_audio_central_self_pairing_feature_enable;
    uint32_t    ble_audio_central_start_alternate_scan_time_ms;
    uint32_t    ble_audio_central_start_alternate_reconn_time_ms;

}ble_audio_core_config;

typedef struct
{
    ble_audio_core_config config;
    ble_mobile_stm_t mobile_link_sm[AOB_COMMON_MOBILE_CONNECTION_MAX];
    BLE_AUD_CORE_EVT_CB_T event_cb;
}ble_audio_core_contxt_t;

#ifdef AOB_MOBILE_ENABLED
uint8_t ble_audio_discovery_modify_interval(uint8_t con_lid, DISCOVER_OPERATE operate, DISCOVER_SERVICE service);
#endif

ble_audio_core_config* ble_audio_get_core_config();

const BLE_AUD_CORE_EVT_CB_T* ble_audio_get_evt_cb(void);

void ble_audio_mobile_sm_send_event_by_conidx(uint8_t conidx,BLE_MOBILE_CONN_EVENT event,uint32_t param0 ,uint32_t param1);

void ble_audio_mobile_sm_send_event_by_addr(ble_bdaddr_t* bdaddr, BLE_MOBILE_CONN_EVENT event,uint32_t param0,uint32_t param1);

ble_mobile_stm_t* ble_audio_find_ble_mobile_sm_by_address(ble_bdaddr_t *addr);

ble_mobile_stm_t* ble_audio_find_ble_mobile_sm_by_conidx(uint8_t conidx);

ble_mobile_stm_t* ble_audio_make_ble_mobile_sm_by_address(ble_bdaddr_t *addr);

void ble_audio_core_evt_handle(ble_event_t *event);

uint8_t ble_audio_count_mobile_acl_connection();

void ble_audio_connection_interval_mgr(uint8_t con_lid, lea_ci_mode_t mode);

uint8_t ble_audio_get_mobile_sm_index_by_addr(ble_bdaddr_t *addr);

void ble_usb_audio_self_pairing_reature_enable(void);

void ble_audio_start_ble_audio(uint8_t con_lid);

#ifdef AOB_MOBILE_ENABLED
const BLE_AUD_MOB_CORE_EVT_CB_T* ble_audio_mobile_get_evt_cb();

void ble_audio_mobile_start_connecting(ble_bdaddr_t *addr);

ble_mobile_conn_stm_t* ble_mobile_find_sm_by_conidx(uint8_t conidx);

ble_mobile_conn_stm_t* ble_mobile_find_sm_by_link_id(uint8_t link_id);

ble_mobile_conn_stm_t* ble_mobile_find_sm_by_address(ble_bdaddr_t *addr);

ble_mobile_conn_stm_t* ble_mobile_make_sm_by_address(ble_bdaddr_t *addr);

void ble_audio_mobile_conn_sm_send_event_by_conidx(uint8_t conidx, BLE_MOBILE_CONNECT_EVENT event, uint32_t param0, uint32_t param1);

void ble_audio_mobile_conn_sm_send_event_by_addr(ble_bdaddr_t* bdaddr, BLE_MOBILE_CONNECT_EVENT event, uint32_t param0, uint32_t param1);

uint8_t* ble_audio_mobile_conn_get_connecting_dev(void);

void ble_mobile_connect_failed(bool is_failed);

bool ble_mobile_is_connect_failed(void);

void ble_audio_mobile_conn_next_paired_dev(ble_bdaddr_t* bdaddr);

bool ble_audio_mobile_conn_reconnect_dev(ble_bdaddr_t* bdaddr);

void ble_mobile_start_connect(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLE_AUDIO_CORE_H__ */

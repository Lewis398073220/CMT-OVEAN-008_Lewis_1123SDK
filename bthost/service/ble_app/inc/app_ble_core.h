/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

#ifndef __APP_BLE_CORE_H__
#define __APP_BLE_CORE_H__

/*****************************header include********************************/
#include "app_ble_mode_switch.h"
#include "ble_aob_common.h"
/******************************macro defination*****************************/


typedef struct {
    ble_callback_evnet_type_e evt_type;
    union {
        struct {
            uint8_t newRole;
        } rs_complete_handled;
        struct {
            uint8_t newRole;
        } role_update_handled;
        struct {
            uint8_t event;
        } ibrt_event_entry_handled;
    } p;
} ble_callback_event_t;

typedef void (*APP_BLE_CORE_EVENT_CALLBACK)(ble_event_t *);

typedef struct
{
    uint8_t head[3];
    uint8_t length;
    bool ble_is_busy;
    uint8_t actv_state[BES_BLE_ACTIVITY_MAX];
    uint8_t conn_cnt;
} __attribute__((__packed__)) AUTO_TEST_BLE_STATE_T;

#ifdef __cplusplus
extern "C" {
#endif

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            app_ble_core_register_global_handler_ind
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to register custom handler function
 *
 * Parameters:
 *    handler -- custom handler function
 *
 * Return:
 *    void
 */
void app_ble_core_register_global_handler_ind(APP_BLE_CORE_GLOBAL_HANDLER_FUNC handler);

/*---------------------------------------------------------------------------
 *            app_ble_core_global_handle
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble core to handle event
 *
 * Parameters:
 *    *event -- the event ble core need to handle
 *
 * Return:
 *    uint32_t
 */
void app_ble_core_global_handle(ble_event_t *event, void *output);

/*---------------------------------------------------------------------------
 *            app_ble_core_evt_cb_register
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to register custom callback function to ble core
 *
 * Parameters:
 *    cb -- custom callback function
 *
 * Return:
 *    void
 */
void app_ble_core_evt_cb_register(APP_BLE_CORE_EVENT_CALLBACK cb);

/*---------------------------------------------------------------------------
 *            app_ble_core_global_callback_event
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble core to handle callback event
 *
 * Parameters:
 *    *event -- the event ble core need to handle
 *
 * Return:
 *    void
 */
void app_ble_core_global_callback_event(ble_callback_event_t *event, void *output);

/*---------------------------------------------------------------------------
 *            ble_core_enable_stub_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    enable stub adv
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void ble_core_enable_stub_adv(void);

/*---------------------------------------------------------------------------
 *            ble_core_disable_stub_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    disable stub adv
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void ble_core_disable_stub_adv(void);

/*---------------------------------------------------------------------------
 *            app_ble_stub_user_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    None
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_stub_user_init(void);

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            app_ble_sync_ble_info
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for tws sync ble info
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_sync_ble_info(void);

/*---------------------------------------------------------------------------
 *            app_ble_mode_tws_sync_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    tws related environment initialization for ble module
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_mode_tws_sync_init(void);

/*---------------------------------------------------------------------------
 *            app_ble_core_print_ble_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    print ble state
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_core_print_ble_state(void);

void ble_roleswitch_start(void);

void ble_roleswitch_complete(uint8_t newRole);

void ble_role_update(uint8_t newRole);

void ble_ibrt_event_entry(uint8_t ibrt_evt_type);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_BLE_CORE_H__ */


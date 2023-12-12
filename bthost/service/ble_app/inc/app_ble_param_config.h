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

#ifndef __APP_BLE_PARAM_CONFIG_H__
#define __APP_BLE_PARAM_CONFIG_H__
#include "ble_core_common.h"

/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************type defination******************************/

/**
 * @brief The user of the ble adv
 *
 */

typedef struct {
    uint32_t adv_interval[BLE_ADV_INTERVALREQ_USER_NUM];
    BLE_ADV_USER_E adv_user[BLE_ADV_USER_NUM];
    BLE_ADV_TX_POWER_LEVEL_E adv_tx_power_level;
} BLE_ADV_FILL_PARAM_T;


/****************************function declearation**************************/
#ifdef __cplusplus
extern "C" {
#endif
/*---------------------------------------------------------------------------
 *            ble_adv_user2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    change BLE_ADV_USER_E to string
 *
 * Parameters:
 *    BLE_ADV_USER_E user
 *
 * Return:
 *    char *
 */
char *ble_adv_user2str(BLE_ADV_USER_E user);

/*---------------------------------------------------------------------------
 *            ble_adv_intv_user2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    change BLE_ADV_INTERVALREQ_USER_E to string
 *
 * Parameters:
 *    BLE_ADV_INTERVALREQ_USER_E user
 *
 * Return:
 *    char *
 */
char *ble_adv_intv_user2str(BLE_ADV_INTERVALREQ_USER_E user);

/*---------------------------------------------------------------------------
 *            app_ble_param_get_ctx
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the context of ble_adv_fill_param
 *
 * Parameters:
 *    void
 *
 * Return:
 *    BLE_ADV_FILL_PARAM_T *
 */
BLE_ADV_FILL_PARAM_T *app_ble_param_get_ctx(void);

/*---------------------------------------------------------------------------
 *            app_ble_param_get_adv_interval
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the activity user from adv user
 *
 * Parameters:
 *    user: adv user
 *
 * Return:
 *    BLE_ADV_ACTIVITY_USER_E: activity user
 */
BLE_ADV_ACTIVITY_USER_E app_ble_param_get_actv_user_from_adv_user(BLE_ADV_USER_E user);

/*---------------------------------------------------------------------------
 *            app_ble_param_get_adv_interval
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the adv interval of app_ble_param_get_ctx
 *
 * Parameters:
 *    BLE_ADV_ACTIVITY_USER_E actv_user
 *
 * Return:
 *    uint32_t
 */
uint32_t app_ble_param_get_adv_interval(BLE_ADV_ACTIVITY_USER_E actv_user);

/*---------------------------------------------------------------------------
 *            app_ble_param_set_adv_interval
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    set the adv interval of activity
 *
 * Parameters:
 *    adv_intv_user: the adv interval request user
 *    adv_user: the adv user
 *    interval: the interval you want to set
 *
 * Return:
 *    void
 */
void app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_E adv_intv_user,
                                        BLE_ADV_USER_E adv_user,
                                        uint32_t interval);

/*---------------------------------------------------------------------------
 *            app_ble_param_get_adv_tx_power_level
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the adv tx power level of activity
 *
 * Parameters:
 *    adv_intv_user: the adv activity request user
 *
 * Return:
 *    BLE_ADV_TX_POWER_LEVEL_E: the adv tx power level of activity
 */
BLE_ADV_TX_POWER_LEVEL_E app_ble_param_get_adv_tx_power_level(BLE_ADV_ACTIVITY_USER_E actv_user);

void app_ble_demo0_user_init(void);
void app_ble_demo1_user_init(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_BLE_PARAM_CONFIG_H__ */


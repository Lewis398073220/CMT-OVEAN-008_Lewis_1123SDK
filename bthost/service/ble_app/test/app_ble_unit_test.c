
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

#include "plat_types.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hal_uart.h"
#include "hal_trace.h"
#include "app_trace_rx.h"
#include "ble_app_dbg.h"
#include "app_ble_include.h"
#include "app_ble_uart.h"
#include "app.h"

#include "app_ble_unit_test.h"

#ifdef BLE_API_UT_ENABLED

void app_ble_unit_test_case_1()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_1 start");
    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_1 test pass!");
}

void app_ble_unit_test_case_2()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_2 start");

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_start_connect_test(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_connect(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_2 test pass!");
}

void app_ble_unit_test_case_3()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_3 start");

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_start_adv_1(0,0);

    ble_start_connect_test(0,0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_connect(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_3 test pass!");
}

void app_ble_unit_test_case_4()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_4 start");

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_connect_test(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_connect(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_4 test pass!");
}

void app_ble_unit_test_case_5()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_5 start");

    ble_update_adv_data_1_to_2(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_back_to_1(0,0);

    ble_disconnect_all(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_connect_test(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_connect(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_5 test pass!");
}

void app_ble_unit_test_case_6()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_6 start");

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_start_connect_test(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_stop_connect(0,0);
    osDelay(100);

    ble_open_scan(0,0); 
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_close_scan(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    LOG_I("ble_unit_test_case_6 test pass!");
}

void app_ble_unit_test_case_7()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_7 start");

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_to_2(0,0);

    ble_open_scan(0,0); 

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_connect_test(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_connect(0,0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_close_scan(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    LOG_I("ble_unit_test_case_7 test pass!");
}

void app_ble_unit_test_case_8()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_8 start");

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_open_scan(0,0); 
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_connect(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_close_scan(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    LOG_I("ble_unit_test_case_8 test pass!");
}

void app_ble_unit_test_case_9()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_9 start");

    ble_update_adv_data_1_to_2(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_back_to_1(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_to_2(0,0);

    ble_open_scan(0,0); 

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_connect(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_close_scan(0,0);
    osDelay(200);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    LOG_I("ble_unit_test_case_9 test pass!");
}

void app_ble_unit_test_case_10()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_10 start");

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_10 test pass!");
}

void app_ble_unit_test_case_11()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_11 start");

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_back_to_1(0,0);

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    
    ble_update_adv_data_1_back_to_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_11 test pass!");
}

void app_ble_unit_test_case_12()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_12 start");

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_2(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_start_adv_2(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_1);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_2(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_1);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_12 test pass!");
}

void app_ble_unit_test_case_13()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_13 start");

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_update_adv_data_1_back_to_1(0,0);

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_2(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    
    ble_update_adv_data_1_back_to_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_start_adv_2(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_1);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_2(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_1);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);
    LOG_I("ble_unit_test_case_13 test pass!");
}

void app_ble_unit_test_case_14()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_14 start");

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_add_devices_info_to_resolving();
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_update_adv_data_1_to_2(0,0);
    osDelay(100);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);
    osDelay(100);

    ble_update_adv_data_1_back_to_1(0,0);
    osDelay(100);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    LOG_I("ble_unit_test_case_14 test pass!");
}

void app_ble_unit_test_case_15()
{
    uint8_t adv_state;

    LOG_I("ble_unit_test_case_15 start");

    app_ble_add_devices_info_to_resolving();

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_1(0,0);

    ble_start_adv_1(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_add_devices_info_to_resolving();

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    app_ble_clear_white_list(BLE_ADV_ACTIVITY_USER_0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_update_adv_data_1_to_2(0,0);

    ble_start_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ADV_STATE_STARTED,"adv state = %d,line = %d",adv_state,__LINE__);

    ble_disconnect_all(0,0);

    ble_update_adv_data_1_back_to_1(0,0);

    ble_stop_adv_1(0,0);
    osDelay(100);
    adv_state = app_ble_get_activity_user_adv_state(BLE_ADV_ACTIVITY_USER_0);
    ASSERT(adv_state == APP_ACTV_STATE_IDLE,"adv state = %d,line = %d",adv_state,__LINE__);

    LOG_I("ble_unit_test_case_15 test pass!");
}
#endif

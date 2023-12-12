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
//#include "mbed.h"
#include "plat_types.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hal_uart.h"
#include "hal_trace.h"
#include "app_trace_rx.h"
#include "ble_app_dbg.h"
#include "app_ble_include.h"
#include "gapm_le.h"
#include "app_ble_uart.h"
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#if BLE_AUDIO_ENABLED
#include "aob_conn_api.h"
#endif


#ifdef BLE_HOST_PTS_TEST_ENABLED
#include "app_pts_test.h"
#include "pts_ble_host_app.h"
#endif

#include "app_ble_unit_test.h"


#define APP_BLE_UART_BUFF_SIZE     50  //16
#define APP_DEMO_DATA0            "\x02\x01\x06\x37\xFF\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00"
#define APP_DEMO_DATA0_LEN        (59)
#define APP_DEMO_DATA1            "\x02\x01\x06\x03\x18\x04\xFE"
#define APP_DEMO_DATA1_LEN        (7)
#define APP_DEMO_DATA2            "\x02\x01\xFF\x03\x19\x03\xFE"
#define APP_DEMO_DATA2_LEN        (7)

const static uint8_t adv_addr_set[6]  = {0x66, 0x34, 0x33, 0x23, 0x22, 0x11};
const static uint8_t adv_addr_set2[6] = {0x77, 0x34, 0x33, 0x23, 0x22, 0x11};
const static uint8_t adv_addr_set3[6] = {0x88, 0x34, 0x33, 0x23, 0x22, 0x11};

void ble_start_three_adv(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);

    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            BLE_ADV_PUBLIC_STATIC,
                            (uint8_t *)adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA0, APP_DEMO_DATA0_LEN,
                            (uint8_t *)APP_DEMO_DATA0, APP_DEMO_DATA0_LEN);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);

    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_1,
                            true,
                            BLE_ADV_PRIVATE_STATIC,
                            (uint8_t *)adv_addr_set2,
                            NULL,
                            200,
                            ADV_TYPE_NON_CONN_SCAN,
                            ADV_MODE_LEGACY,
                            -5,
                            (uint8_t *)APP_DEMO_DATA1, APP_DEMO_DATA1_LEN,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_1);

    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_2,
                            true,
                            BLE_ADV_RPA,
                            (uint8_t *)adv_addr_set3,
                            NULL,
                            200,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA2, APP_DEMO_DATA2_LEN,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_2);
}

void ble_stop_all_adv(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_0);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_1);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_2);
}


void ble_start_adv_1(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            BLE_ADV_PUBLIC_STATIC,
                            (uint8_t *)adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA0, APP_DEMO_DATA0_LEN,
                            NULL, 0);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);
}

void ble_stop_adv_1(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_0);
}

void ble_start_adv_2(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_1,
                            true,
                            BLE_ADV_PRIVATE_STATIC,
                            (uint8_t *)adv_addr_set2,
                            NULL,
                            200,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA1, APP_DEMO_DATA1_LEN,
                            (uint8_t *)APP_DEMO_DATA1, APP_DEMO_DATA1_LEN);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_1);
}

void ble_stop_adv_2(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_1);
}

void ble_start_adv_3(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_2,
                            true,
                            BLE_ADV_RPA,
                            (uint8_t *)adv_addr_set3,
                            NULL,
                            200,
                            ADV_TYPE_UNDIRECT,
                            ADV_MODE_LEGACY,
                            12,
                            (uint8_t *)APP_DEMO_DATA2, APP_DEMO_DATA2_LEN,
                            (uint8_t *)APP_DEMO_DATA2, APP_DEMO_DATA2_LEN);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_2);
}

void ble_stop_adv_3(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_2);
}

#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
void ble_start_adv_4(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    BLE_ADV_PARAM_T aob_audio_adv_param;
    app_ble_custom_init();
    //ble audio adv data was provided by "ble_audio_advData_prepare",
    //if you needed to modified, please to add new ble audio advData prepare api
#if BLE_AUDIO_ENABLED
    uint8_t br_edr_support = 0;
    ble_audio_advData_prepare(&aob_audio_adv_param, br_edr_support);
#endif
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_3,
                            true,
                            BLE_ADV_PRIVATE_STATIC,
                            (uint8_t *)adv_addr_set3,
                            NULL,
                            200,
                            ADV_TYPE_CONN_EXT_ADV,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)aob_audio_adv_param.advData,
                            aob_audio_adv_param.advDataLen,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_3);
}

void ble_stop_adv_4(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_3);
}
#endif

void ble_update_adv_data_1_to_2(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            BLE_ADV_PRIVATE_STATIC,
                            (uint8_t *)adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_NON_CONN_NON_SCAN,
                            ADV_MODE_EXTENDED,
                            12,
                            (uint8_t *)APP_DEMO_DATA1, APP_DEMO_DATA1_LEN,
                            NULL, 0);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);
}


void ble_update_adv_data_1_back_to_1(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    ble_start_adv_1(BufPtr, BufLen);
}
void ble_update_adv_data_2_back_to_2(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    ble_start_adv_2(BufPtr, BufLen);
}
void ble_update_adv_data_3_back_to_3(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    ble_start_adv_3(BufPtr, BufLen);
}

void ble_set_adv_txpwr_to_min(uint32_t BufPtr, uint32_t BufLen)
{
    app_ble_set_all_adv_txpwr(-21);
}

void ble_set_adv_txpwr_to_0(uint32_t BufPtr, uint32_t BufLen)
{
    app_ble_set_all_adv_txpwr(0);
}

void ble_set_adv_txpwr_to_max(uint32_t BufPtr, uint32_t BufLen)
{
    app_ble_set_all_adv_txpwr(16);
}

void ble_open_scan(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);

    BLE_SCAN_PARAM_T scan_param = {0};

    scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
    scan_param.scanWindowMs   = 10;
    scan_param.scanIntervalMs = 60;
    app_ble_start_scan(&scan_param);
}

void ble_close_scan(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_stop_scan();
}

void ble_start_connect_test(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    ble_bdaddr_t ble_connect_addr;
    ble_connect_addr.addr_type = GAPM_STATIC_ADDR;
    memcpy(ble_connect_addr.addr, (uint8_t *)adv_addr_set, BTIF_BD_ADDR_SIZE);
    app_ble_start_connect(&ble_connect_addr, APP_GAPM_STATIC_ADDR);
}

void ble_stop_connect(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_cancel_connecting();
}

void ble_disconnect_all(uint32_t BufPtr, uint32_t BufLen)
{
    LOG_I("%s", __func__);
    app_ble_disconnect_all();
}

const ble_uart_handle_t ble_uart_test_handle[]=
{
    {"start_three_adv", ble_start_three_adv},
    {"stop_all_adv", ble_stop_all_adv},
    {"start_adv_1", ble_start_adv_1},
    {"start_adv_2", ble_start_adv_2},
    {"start_adv_3", ble_start_adv_3},
    {"stop_adv_1", ble_stop_adv_1},
    {"stop_adv_2", ble_stop_adv_2},
    {"stop_adv_3", ble_stop_adv_3},
#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
    {"start_adv_4", ble_start_adv_4},
    {"stop_adv_4", ble_stop_adv_4},
#endif
    {"update_adv_data_1_to_2", ble_update_adv_data_1_to_2},
    {"update_adv_data_1_back_to_1", ble_update_adv_data_1_back_to_1},
    {"update_adv_data_2_back_to_2", ble_update_adv_data_2_back_to_2},
    {"update_adv_data_3_back_to_3", ble_update_adv_data_3_back_to_3},
    {"set_adv_txpwr_to_min", ble_set_adv_txpwr_to_min},
    {"set_adv_txpwr_to_0", ble_set_adv_txpwr_to_0},
    {"set_adv_txpwr_to_max", ble_set_adv_txpwr_to_max},
    {"open_scan", ble_open_scan},
    {"close_scan", ble_close_scan},
    {"start_connect", ble_start_connect_test},
    {"stop_connect", ble_stop_connect},
    {"disconnect_all", ble_disconnect_all},
#ifdef BLE_API_UT_ENABLED
    {"ble_ut_case_1",app_ble_unit_test_case_1},
    {"ble_ut_case_2",app_ble_unit_test_case_2},
    {"ble_ut_case_3",app_ble_unit_test_case_3},
    {"ble_ut_case_4",app_ble_unit_test_case_4},
    {"ble_ut_case_5",app_ble_unit_test_case_5},//X
    {"ble_ut_case_6",app_ble_unit_test_case_6},//X
    {"ble_ut_case_7",app_ble_unit_test_case_7},
    {"ble_ut_case_8",app_ble_unit_test_case_8},//x
    {"ble_ut_case_9",app_ble_unit_test_case_9},
    {"10_ble_ut_case",app_ble_unit_test_case_10},
    {"11_ble_ut_case",app_ble_unit_test_case_11},
    {"12_ble_ut_case",app_ble_unit_test_case_12},
    {"13_ble_ut_case",app_ble_unit_test_case_13},
    {"14_ble_ut_case",app_ble_unit_test_case_14},
    {"15_ble_ut_case",app_ble_unit_test_case_15},
#endif

#ifdef BLE_HOST_PTS_TEST_ENABLED
    {"host_pts_test",          ble_host_pts_cmd_handle  },
#endif

};

int ble_uart_cmd_handler(unsigned char *buf, unsigned int length)
{
    int i;
    int ret = 0;
    char *para_addr=NULL;
    unsigned int para_len=0;

    for (i = 0; i < ARRAY_SIZE(ble_uart_test_handle); i++)
    {
        if ((strncmp((char*)buf, ble_uart_test_handle[i].string, strlen(ble_uart_test_handle[i].string)) == 0) ||
            strstr(ble_uart_test_handle[i].string, (char*)buf))
        {
            para_addr = strstr((char*)buf, " ");
            if(para_addr != NULL)
            {
                para_addr++;
                para_len = length - (para_addr - (char *)buf);
            }

            ble_uart_test_handle[i].function((uint32_t)para_addr, para_len);
            break;
        }

        if(i == ARRAY_SIZE(ble_uart_test_handle))
        {
            ret = -1;
            TRACE(0,"ERROR:can not find handle function");
        }
    }

    return ret;
}

unsigned int ble_uart_cmd_callback(unsigned char *buf, unsigned int len)
{
    // Check len
    TRACE(2,"[%s] len = %d", __func__, len);
    ble_uart_cmd_handler((unsigned char*)buf,strlen((char*)buf));
    return 0;
}

void ble_uart_cmd_init(void)
{
#ifdef APP_TRACE_RX_ENABLE
    TRACE(0,"ble_uart_cmd_init");
    app_trace_rx_register("BLE", ble_uart_cmd_callback);
#endif

#ifdef BES_AUTOMATE_TEST
    app_ble_custom_init();
#endif
}


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
#ifndef __APP_BLE_UART_H__
#define __APP_BLE_UART_H__

typedef void (*ble_uart_test_function_handle)(uint32_t BufPtr, uint32_t BufLen);
typedef struct
{
    const char* string;
    ble_uart_test_function_handle function;
} ble_uart_handle_t;

#ifdef __cplusplus
extern "C"{
#endif

void ble_start_three_adv(uint32_t BufPtr, uint32_t BufLen);
void ble_stop_all_adv(uint32_t BufPtr, uint32_t BufLen);
void ble_start_adv_1(uint32_t BufPtr, uint32_t BufLen);
void ble_stop_adv_1(uint32_t BufPtr, uint32_t BufLen);
void ble_start_adv_2(uint32_t BufPtr, uint32_t BufLen);
void ble_stop_adv_2(uint32_t BufPtr, uint32_t BufLen);
void ble_stop_adv_3(uint32_t BufPtr, uint32_t BufLen);
void ble_update_adv_data_1_to_2(uint32_t BufPtr, uint32_t BufLen);
void ble_update_adv_data_1_back_to_1(uint32_t BufPtr, uint32_t BufLen);
void ble_update_adv_data_2_back_to_2(uint32_t BufPtr, uint32_t BufLen);
void ble_update_adv_data_3_back_to_3(uint32_t BufPtr, uint32_t BufLen);
void ble_open_scan(uint32_t BufPtr, uint32_t BufLen);
void ble_close_scan(uint32_t BufPtr, uint32_t BufLen);
void ble_start_connect_test(uint32_t BufPtr, uint32_t BufLen);
void ble_stop_connect(uint32_t BufPtr, uint32_t BufLen);
void ble_disconnect_all(uint32_t BufPtr, uint32_t BufLen);
#ifdef IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
void ble_start_adv_4(uint32_t BufPtr, uint32_t BufLen);
void ble_stop_adv_4(uint32_t BufPtr, uint32_t BufLen);
#endif

void ble_uart_cmd_init(void);

#ifdef __cplusplus
}
#endif

#endif


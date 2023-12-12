#ifndef __PTS_BLE_HOST_APP_H__
#define __PTS_BLE_HOST_APP_H__
#include "gap.h"

#define BLE_HOST_TEST_NO_CASE "NO_CASE"
#define BLE_HOST_TEST_MODULE 10

typedef void (*pts_case_num_function_handle)(char *BufPtr, uint32_t BufLen);

typedef struct pts_case_num{
    uint8_t number;
    pts_case_num_function_handle case_handle;
}pts_case_num_t;

typedef struct pts_test_case{
    char *case_name;
    pts_case_num_t *case_bv;
    pts_case_num_t *case_bi;
}pts_module_case_t;

typedef struct pts_sub_module{
    char *sub_module_name;
    pts_module_case_t *test_case;
}pts_sub_module_t;

typedef struct pts_module{
    char module_name[BLE_HOST_TEST_MODULE];
    pts_sub_module_t *sub_module;
}pts_module_t;

extern gap_bdaddr_t dongle_addr;

/****************PTS STACK***************************/
void ble_host_pts_init();

///****************PTS MODULE***************************/
void ble_host_pts_module_reg(char *module_name, pts_sub_module_t *sub_module);
void ble_host_pts_cmd_handle(uint32_t BufPtr, uint32_t str_len);

#endif


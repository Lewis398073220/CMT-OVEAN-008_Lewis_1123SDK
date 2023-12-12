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
#ifndef BESBT_H
#define BESBT_H
#include "bt_common_define.h"
#ifdef __cplusplus
extern "C" {
#endif

void BesbtInit(void);
void BesbtThread(void const *argument);
int Besbt_hook_handler_set(enum BESBT_HOOK_USER_T user, BESBT_HOOK_HANDLER handler);
unsigned char *bt_get_local_address(void);
void bt_set_local_address(unsigned char* btaddr);
unsigned char *bt_get_ble_local_address(void);
const char *bt_get_local_name(void);
void bt_set_local_name(const char* name);
const char *bt_get_ble_local_name(void);
void bt_set_ble_local_name(const char* name, uint8_t len_hint);
void bt_set_ble_local_address(uint8_t* bleAddr);
bool app_bt_is_besbt_thread(void);

int app_bt_start_custom_function_in_bt_thread(uint32_t param0, uint32_t param1, uint32_t funcPtr);
int app_bt_call_func_in_bt_thread(uint32_t param0, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t funcPtr);
int app_bt_defer_call_in_bt_thread(uintptr_t func, struct bt_alloc_param_t *param);

void bes_bt_app_init(void);
void bes_bt_init_global_handle(void);
void bes_bt_thread_init(void);
bool bes_bt_me_is_bt_thread(void);
uint8_t bes_bt_me_count_mobile_link(void);
int bes_bt_me_write_access_mode(uint8_t mode, bool disable_before_update);
void bes_bt_me_acl_disc_all_bt_links(bool power_off_flag);
void *bes_bt_me_profile_active_store_ptr_get(uint8_t *bdAddr);
void bes_bt_me_transfer_pairing_to_connectable(void);
void bes_bt_a2dp_report_speak_gain(void);
void bes_bt_a2dp_key_handler(uint8_t a2dp_key);
void bes_bt_hfp_report_speak_gain(void);
void bes_bt_hfp_key_handler(uint8_t hfp_key);
int8_t bes_bt_me_get_bt_rssi(void);
uint8_t bes_bt_me_get_max_sco_number(void);

#ifdef __cplusplus
}
#endif
#endif /* BESBT_H */

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
#ifndef __WALKIE_TALKIE_BLE_GAPM_CMD_H__
#define __WALKIE_TALKIE_BLE_GAPM_CMD_H__
typedef struct {
   ///
   int (*cmp_evt_handler)(uint8_t operation, uint16_t status, uint8_t actv_idx);
   ///
   int (*adv_handler)(uint8_t *data , uint8_t actv_idx, uint8_t *addr, int8_t rssi, uint8_t length);
   ///
   int (*stop_ind)(uint8_t actv_idx);
   ///
   int (*per_sync_est_evt)(uint8_t actv_idx, uint8_t *addr);
}walie_gap_cb_func_t;

void walkie_stop_activity(uint8_t actv_idx);
void walkie_delete_activity(uint8_t actv_idx);
void walkie_adv_creat(uint8_t *adv_para);
void walkie_adv_enable(uint16_t duration, uint8_t max_adv_evt, uint8_t actv_idx);
void walkie_set_adv_data_cmd(uint8_t operation, uint8_t actv_idx,
                                      uint8_t *adv_data, uint8_t data_len);
void walkie_periodic_sync_create(uint8_t own_addr_type);
void walkie_periodic_sync_enable(uint8_t actv_idx, uint8_t *per_para);
void walkie_scan_creat(uint8_t own_addr_type);
void walkie_scan_enable(uint8_t actv_idx,uint8_t *scan_para);
void walkie_set_device_list(uint8_t list_type, uint8_t *bdaddr, uint8_t size);
void walkie_ble_gapm_task_init(walie_gap_cb_func_t *cb);
#endif /*_WALKIE_TALKIE_BLE_GAPM_CMD_H__*/

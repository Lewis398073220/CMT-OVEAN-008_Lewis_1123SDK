/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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

#ifndef __GAF_ULL_HID_STREAM_H__
#define __GAF_ULL_HID_STREAM_H__
#include "app_bap_uc_srv_msg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*ULL_HID_DATA_CHANGED_CB_T)(uint8_t *hid_data);
void gaf_ull_hid_data_changed_register_cb(ULL_HID_DATA_CHANGED_CB_T cb);
void gaf_ull_hid_upstream_stop(uint8_t con_lid);
void gaf_ull_hid_data_update_upstream_start(app_bap_ascs_ase_t * p_bap_ase_info, uint8_t ase_lid);
#ifdef __cplusplus
}
#endif
#endif // __GAF_ULL_HID_STREAM_H__
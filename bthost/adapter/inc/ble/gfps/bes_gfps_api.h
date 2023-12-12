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
#ifndef __BES_GFPS_API_H__
#define __BES_GFPS_API_H__
#include "ble_gfps_common.h"
#ifdef BLE_HOST_SUPPORT
#ifdef GFPS_ENABLED
#ifdef __cplusplus
extern "C" {
#endif

void bes_ble_gfps_init(void);

void bes_ble_gfps_register_callback(fp_event_cb callback);

void bes_ble_gfps_send_keybase_pairing_via_notification(void *priv, uint8_t *data, uint16_t length);

uint8_t bes_ble_gfps_l2cap_send(uint8_t conidx, uint8_t *ptrData, uint32_t length);

void bes_ble_gfps_l2cap_disconnect(uint8_t conidx);

#ifdef IBRT
void bes_ble_gfps_ibrt_share_fastpair_info(uint8_t *p_buff, uint16_t length);
void bes_ble_gfps_ibrt_share_fastpair_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

#ifdef __cplusplus
}
#endif
#endif
#endif
#endif /* __BES_GFPS_API_H__ */
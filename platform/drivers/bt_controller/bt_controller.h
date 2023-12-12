/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef __BT_CONTROLLER_H__
#define __BT_CONTROLLER_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int (*BT_CONTROLLER_RX_IRQ_HANDLER)(const void *data, unsigned int len);

typedef void (*BT_CONTROLLER_TX_IRQ_HANDLER)(const void *data, unsigned int len);

int bt_controller_open(BT_CONTROLLER_RX_IRQ_HANDLER rx_hdlr, BT_CONTROLLER_TX_IRQ_HANDLER tx_hdlr);

int bt_controller_close(void);

void bt_radio_init(void);

int bt_controller_send_seq(const void *data, unsigned int len, unsigned int *seq);

int bt_controller_send(const void *data, unsigned int len);

int bt_controller_tx_active(unsigned int seq);

#ifdef __cplusplus
}
#endif

#endif


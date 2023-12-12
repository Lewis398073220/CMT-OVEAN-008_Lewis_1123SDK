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
#ifndef __DSP_M55_H__
#define __DSP_M55_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int (*DSP_M55_RX_IRQ_HANDLER)(const void *data, unsigned int len);

typedef void (*DSP_M55_TX_IRQ_HANDLER)(const void *data, unsigned int len);

int dsp_m55_open_in_sys_ram(DSP_M55_RX_IRQ_HANDLER rx_hdlr, DSP_M55_TX_IRQ_HANDLER tx_hdlr);

int dsp_m55_open(DSP_M55_RX_IRQ_HANDLER rx_hdlr, DSP_M55_TX_IRQ_HANDLER tx_hdlr);

int dsp_m55_close(void);

int dsp_m55_send_seq(const void *data, unsigned int len, unsigned int *seq);

int dsp_m55_send(const void *data, unsigned int len);

int dsp_m55_tx_active(unsigned int seq);

int dsp_hifi_send_seq(const void *data, unsigned int len, unsigned int *seq);

int dsp_hifi_send(const void *data, unsigned int len);

int dsp_hifi_tx_active(unsigned int seq);

#ifdef __cplusplus
}
#endif

#endif


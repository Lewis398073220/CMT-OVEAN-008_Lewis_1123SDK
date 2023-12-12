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
#ifndef __DSP_HIFI_CORE_H__
#define __DSP_HIFI_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

enum CORE_IRQ_HDLR_ID_T {
    CORE_IRQ_HDLR_ID_0,
    CORE_IRQ_HDLR_ID_1,

    CORE_IRQ_HDLR_ID_QTY
};

#define CORE_IRQ_HDLR_PRIO_HIGH CORE_IRQ_HDLR_ID_0
#define CORE_IRQ_HDLR_PRIO_LOW  CORE_IRQ_HDLR_ID_1

typedef unsigned int (*dsp_core_rx_irq_handler_t)(const void*, unsigned int);
typedef void (*dsp_core_tx_done_irq_handler_t)(const void*, unsigned int);

void dsp_core_register_rx_irq_handler(enum CORE_IRQ_HDLR_ID_T id, dsp_core_rx_irq_handler_t irqHandler);
void dsp_core_register_tx_done_irq_handler(enum CORE_IRQ_HDLR_ID_T id, dsp_core_tx_done_irq_handler_t irqHandler);

#ifdef __cplusplus
}
#endif

#endif


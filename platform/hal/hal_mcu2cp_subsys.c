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
#include "plat_addr_map.h"

#if defined(CP_AS_SUBSYS) || defined(CHIP_ROLE_CP)

#include "cmsis_nvic.h"
#include "hal_mcu2cp.h"
#include "hal_rmt_ipc.h"
#include "hal_trace.h"
#include CHIP_SPECIFIC_HDR(reg_cmu)

#ifndef MCU2CP_CHAN_REC_CNT
#define MCU2CP_CHAN_REC_CNT                 3
#endif

#define HAL_SYS_WAKE_LOCK_USER_MCU2CP       HAL_BUS_WAKE_LOCK_USER_4

#define MCU_CFG_PTR_LOC                     (CP_SUBSYS_MAILBOX_BASE + 0)
#define CP_CFG_PTR_LOC                      (CP_SUBSYS_MAILBOX_BASE + sizeof(struct HAL_RMT_IPC_CFG_T *))

STATIC_ASSERT((2 * sizeof(struct HAL_RMT_IPC_CFG_T *)) <= CP_SUBSYS_MAILBOX_SIZE, "CP_SUBSYS_MAILBOX_SIZE too small");

static struct CMU_T * const cmu = (struct CMU_T *)CMU_BASE;

static const IRQn_Type rx_irq_id[HAL_MCU2CP_ID_QTY] = {
    CP2MCU_DATA_IRQn,
    CP2MCU_DATA1_IRQn,
};

static const IRQn_Type tx_irq_id[HAL_MCU2CP_ID_QTY] = {
    MCU2CP_DONE_IRQn,
    MCU2CP_DONE1_IRQn,
};

static const struct HAL_RMT_IPC_CFG_T mcu2cp_cfg;

static struct HAL_RMT_IPC_CH_CFG_T mcu2cp_chan_cfg[HAL_MCU2CP_ID_QTY];
static struct HAL_RMT_IPC_SEND_RECORD_T mcu2cp_send_record[HAL_MCU2CP_ID_QTY * MCU2CP_CHAN_REC_CNT];

static bool busy_now;

static int hal_mcu2cp_peer_irq_set(enum HAL_MCU2CP_ID_T id, enum HAL_RMT_IPC_IRQ_TYPE_T type)
{
    uint32_t value;

#ifdef CHIP_ROLE_CP
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA_IND_SET;
        } else {
            value = CMU_MCU2CP_DATA_DONE_SET;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA1_IND_SET;
        } else {
            value = CMU_MCU2CP_DATA1_DONE_SET;
        }
    }

    cmu->CP2MCU_IRQ_SET = value;
#else
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA_IND_SET;
        } else {
            value = CMU_CP2MCU_DATA_DONE_SET;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA1_IND_SET;
        } else {
            value = CMU_CP2MCU_DATA1_DONE_SET;
        }
    }

    cmu->MCU2CP_IRQ_SET = value;
#endif

    return 0;
}

static int hal_mcu2cp_local_irq_clear(enum HAL_MCU2CP_ID_T id, enum HAL_RMT_IPC_IRQ_TYPE_T type)
{
    uint32_t value;

#ifdef CHIP_ROLE_CP
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA_IND_CLR;
        } else {
            value = CMU_CP2MCU_DATA_DONE_CLR;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA1_IND_CLR;
        } else {
            value = CMU_CP2MCU_DATA1_DONE_CLR;
        }
    }

    cmu->MCU2CP_IRQ_CLR = value;
#else
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA_IND_CLR;
        } else {
            value = CMU_MCU2CP_DATA_DONE_CLR;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA1_IND_CLR;
        } else {
            value = CMU_MCU2CP_DATA1_DONE_CLR;
        }
    }

    cmu->CP2MCU_IRQ_CLR = value;
#endif

    cmu->CP2MCU_IRQ_CLR;
    __DSB();

    return 0;
}

static int hal_mcu2cp_local_irq_set(enum HAL_MCU2CP_ID_T id, enum HAL_RMT_IPC_IRQ_TYPE_T type)
{
    uint32_t value;

#ifdef CHIP_ROLE_CP
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA_IND_SET;
        } else {
            value = CMU_CP2MCU_DATA_DONE_SET;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA1_IND_SET;
        } else {
            value = CMU_CP2MCU_DATA1_DONE_SET;
        }
    }

    cmu->MCU2CP_IRQ_SET = value;
#else
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA_IND_SET;
        } else {
            value = CMU_MCU2CP_DATA_DONE_SET;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA1_IND_SET;
        } else {
            value = CMU_MCU2CP_DATA1_DONE_SET;
        }
    }

    cmu->CP2MCU_IRQ_SET = value;
#endif

    return 0;
}

int hal_mcu2cp_rx_irq_pending(enum HAL_MCU2CP_ID_T id)
{
    uint32_t value;

#ifdef CHIP_ROLE_CP
    if (id == HAL_MCU2CP_ID_0) {
        value = CMU_MCU2CP_DATA_IND_SET;
    } else {
        value = CMU_MCU2CP_DATA1_IND_SET;
    }

    return !!(cmu->MCU2CP_IRQ_SET & value);
#else
    if (id == HAL_MCU2CP_ID_0) {
        value = CMU_CP2MCU_DATA_IND_SET;
    } else {
        value = CMU_CP2MCU_DATA1_IND_SET;
    }

    return !!(cmu->CP2MCU_IRQ_SET & value);
#endif
}

static void hal_mcu2cp_irq_init(uint32_t id)
{
    if (id == HAL_MCU2CP_ID_0) {
        cmu->MCU2CP_IRQ_CLR = CMU_CP2MCU_DATA_DONE_CLR | CMU_MCU2CP_DATA_IND_CLR;
        cmu->CP2MCU_IRQ_CLR = CMU_MCU2CP_DATA_DONE_CLR | CMU_CP2MCU_DATA_IND_CLR;
    } else {
        cmu->MCU2CP_IRQ_CLR = CMU_CP2MCU_DATA1_DONE_CLR | CMU_MCU2CP_DATA1_IND_CLR;
        cmu->CP2MCU_IRQ_CLR = CMU_MCU2CP_DATA1_DONE_CLR | CMU_CP2MCU_DATA1_IND_CLR;
    }

    cmu->CP2MCU_IRQ_CLR;
    __DSB();
}

static void hal_mcu2cp_peer_tx_irq_set(uint32_t id32)
{
    enum HAL_MCU2CP_ID_T id = (enum HAL_MCU2CP_ID_T)id32;

    hal_mcu2cp_peer_irq_set(id, HAL_RMT_IPC_IRQ_DATA_IND);
}

static void hal_mcu2cp_local_tx_irq_clear(uint32_t id32)
{
    enum HAL_MCU2CP_ID_T id = (enum HAL_MCU2CP_ID_T)id32;

    hal_mcu2cp_local_irq_clear(id, HAL_RMT_IPC_IRQ_DATA_DONE);
}

static void hal_mcu2cp_rx_irq_suspend(uint32_t id32)
{
    enum HAL_MCU2CP_ID_T id = (enum HAL_MCU2CP_ID_T)id32;

    hal_mcu2cp_local_irq_clear(id, HAL_RMT_IPC_IRQ_DATA_IND);
}

static void hal_mcu2cp_rx_irq_resume(uint32_t id32)
{
    enum HAL_MCU2CP_ID_T id = (enum HAL_MCU2CP_ID_T)id32;

    hal_mcu2cp_local_irq_set(id, HAL_RMT_IPC_IRQ_DATA_IND);
}

static void hal_mcu2cp_rx_done_id32(uint32_t id32)
{
    enum HAL_MCU2CP_ID_T id = (enum HAL_MCU2CP_ID_T)id32;

    hal_mcu2cp_peer_irq_set(id, HAL_RMT_IPC_IRQ_DATA_DONE);
}

static int hal_mcu2cp_irq_active(uint32_t id, uint32_t type)
{
    uint32_t value;

#ifdef CHIP_ROLE_CP
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA_IND_SET;
        } else {
            value = CMU_CP2MCU_DATA_DONE_SET;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_MCU2CP_DATA1_IND_SET;
        } else {
            value = CMU_CP2MCU_DATA1_DONE_SET;
        }
    }

    return !!(cmu->MCU2CP_IRQ_SET & value);
#else
    if (id == HAL_MCU2CP_ID_0) {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA_IND_SET;
        } else {
            value = CMU_MCU2CP_DATA_DONE_SET;
        }
    } else {
        if (type == HAL_RMT_IPC_IRQ_DATA_IND) {
            value = CMU_CP2MCU_DATA1_IND_SET;
        } else {
            value = CMU_MCU2CP_DATA1_DONE_SET;
        }
    }

    return !!(cmu->CP2MCU_IRQ_SET & value);
#endif
}

static void hal_mcu2cp_rx_irq_handler(void)
{
    hal_rmt_ipc_rx_irq_handler(&mcu2cp_cfg);
}

static void hal_mcu2cp_tx_irq_handler(void)
{
    hal_rmt_ipc_tx_irq_handler(&mcu2cp_cfg);
}

static const struct HAL_RMT_IPC_CFG_T mcu2cp_cfg = {
    .name = "MCU2CP",
#ifdef CHIP_ROLE_CP
    .peer_cfg_pp = (const struct HAL_RMT_IPC_CFG_T **)MCU_CFG_PTR_LOC,
    .local_cfg_pp = (const struct HAL_RMT_IPC_CFG_T **)CP_CFG_PTR_LOC,
#else
    .peer_cfg_pp = (const struct HAL_RMT_IPC_CFG_T **)CP_CFG_PTR_LOC,
    .local_cfg_pp = (const struct HAL_RMT_IPC_CFG_T **)MCU_CFG_PTR_LOC,
#endif

    .irq_init = hal_mcu2cp_irq_init,
    .peer_tx_irq_set = hal_mcu2cp_peer_tx_irq_set,
    .local_tx_irq_clear = hal_mcu2cp_local_tx_irq_clear,
    .rx_irq_suspend = hal_mcu2cp_rx_irq_suspend,
    .rx_irq_resume = hal_mcu2cp_rx_irq_resume,
    .rx_done = hal_mcu2cp_rx_done_id32,
    .irq_active = hal_mcu2cp_irq_active,
    .rx_irq_entry = hal_mcu2cp_rx_irq_handler,
    .tx_irq_entry = hal_mcu2cp_tx_irq_handler,
#ifdef CORE_SLEEP_POWER_DOWN
    .pm_notif_handler = NULL,
#endif

    .wake_lock = HAL_SYS_WAKE_LOCK_USER_MCU2CP,
    .chan_num = HAL_MCU2CP_ID_QTY,
    .rec_num_per_chan = ARRAY_SIZE(mcu2cp_send_record) / HAL_MCU2CP_ID_QTY,
    .chan_cfg = &mcu2cp_chan_cfg[0],
    .send_record = &mcu2cp_send_record[0],
    .rx_irq_id = &rx_irq_id[0],
    .tx_irq_id = &tx_irq_id[0],
    .busy_p = &busy_now,
};

int hal_mcu2cp_open(enum HAL_MCU2CP_ID_T id, HAL_MCU2CP_RX_IRQ_HANDLER rxhandler, HAL_MCU2CP_TX_IRQ_HANDLER txhandler, int rx_flowctrl)
{
    return hal_rmt_ipc_open(&mcu2cp_cfg, id, rxhandler, txhandler, rx_flowctrl);
}

int hal_mcu2cp_close(enum HAL_MCU2CP_ID_T id)
{
    return hal_rmt_ipc_close(&mcu2cp_cfg, id);
}

int hal_mcu2cp_start_recv(enum HAL_MCU2CP_ID_T id)
{
    return hal_rmt_ipc_start_recv(&mcu2cp_cfg, id);
}

int hal_mcu2cp_stop_recv(enum HAL_MCU2CP_ID_T id)
{
    return hal_rmt_ipc_stop_recv(&mcu2cp_cfg, id);
}

int hal_mcu2cp_send_seq(enum HAL_MCU2CP_ID_T id, const void *data, unsigned int len, unsigned int *seq)
{
    return hal_rmt_ipc_send_seq(&mcu2cp_cfg, id, data, len, seq);
}

int hal_mcu2cp_send(enum HAL_MCU2CP_ID_T id, const void *data, unsigned int len)
{
    return hal_mcu2cp_send_seq(id, data, len, NULL);
}

int hal_mcu2cp_rx_done(enum HAL_MCU2CP_ID_T id)
{
    return hal_rmt_ipc_rx_done(&mcu2cp_cfg, id);
}

int hal_mcu2cp_tx_irq_run(enum HAL_MCU2CP_ID_T id)
{
    return hal_rmt_ipc_tx_irq_run(&mcu2cp_cfg, id);
}

int hal_mcu2cp_tx_active(enum HAL_MCU2CP_ID_T id, unsigned int seq)
{
    return hal_rmt_ipc_tx_active(&mcu2cp_cfg, id, seq);
}

int hal_mcu2cp_opened(enum HAL_MCU2CP_ID_T id)
{
    return hal_rmt_ipc_opened(&mcu2cp_cfg, id);
}

#endif


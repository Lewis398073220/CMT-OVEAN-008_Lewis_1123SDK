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
#ifndef __HAL_RMT_IPC_H__
#define __HAL_RMT_IPC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis.h"
#include "hal_sleep.h"

enum HAL_RMT_IPC_ID_T {
    HAL_RMT_IPC_ID_0,
    HAL_RMT_IPC_ID_1,
    HAL_RMT_IPC_ID_2,
    HAL_RMT_IPC_ID_3,

    HAL_RMT_IPC_ID_QTY
};

#define HAL_RMT_IPC_ALL_SEND_SEQ                (~0UL)

#ifdef CPU_TO_DEV_ADDR_REMAP
#define RMT_IPC_OPEN_CFG_QUAL
#else
#define RMT_IPC_OPEN_CFG_QUAL                   const
#endif

typedef unsigned int (*HAL_RMT_IPC_RX_IRQ_HANDLER)(const void *data, unsigned int len);
typedef void (*HAL_RMT_IPC_TX_IRQ_HANDLER)(const void *data, unsigned int len);

typedef void (*HAL_RMT_IPC_IRQ_INIT)(uint32_t id);
typedef void (*HAL_RMT_IPC_PEER_TX_IRQ_SET)(uint32_t id);
typedef void (*HAL_RMT_IPC_LOCAL_TX_IRQ_CLEAR)(uint32_t id);
typedef void (*HAL_RMT_IPC_RX_IRQ_SUSPEND)(uint32_t id);
typedef void (*HAL_RMT_IPC_RX_IRQ_RESUME)(uint32_t id);
typedef void (*HAL_RMT_IPC_RX_DONE)(uint32_t id);
typedef int (*HAL_RMT_IPC_IRQ_ACTIVE)(uint32_t id, uint32_t type);
typedef void (*HAL_RMT_IPC_IRQ_ENTRY)(void);
typedef int (*HAL_RMT_IPC_PM_NOTIF_HANDLER)(enum HAL_PM_STATE_T state);

enum HAL_RMT_IPC_IRQ_TYPE_T {
    HAL_RMT_IPC_IRQ_DATA_IND,
    HAL_RMT_IPC_IRQ_DATA_DONE,

    HAL_RMT_IPC_IRQ_TYPE_QTY
};

struct HAL_RMT_IPC_MSG_T {
    struct HAL_RMT_IPC_MSG_T *next;        // pointer to next element in the list
    unsigned int len;                       // message data length in bytes
    const void *data;                       // pointer to the message data
};

struct HAL_RMT_IPC_SEND_RECORD_T {
    struct HAL_RMT_IPC_MSG_T msg;
    bool in_use;
};

struct HAL_RMT_IPC_CH_CFG_T {
    struct HAL_RMT_IPC_MSG_T * send_msg_list_p;
    struct HAL_RMT_IPC_MSG_T * send_pending_list_p;
    struct HAL_RMT_IPC_SEND_RECORD_T * send_record;
    uint32_t send_rec_num;

    const struct HAL_RMT_IPC_MSG_T ** recv_msg_list_pp;
    struct HAL_RMT_IPC_MSG_T recv_pending_head;

    HAL_RMT_IPC_RX_IRQ_HANDLER rx_irq_handler;
    HAL_RMT_IPC_TX_IRQ_HANDLER tx_irq_handler;

    bool need_flow_ctrl;
    bool chan_busy;
    bool chan_opened;
};

struct HAL_RMT_IPC_CFG_T {
    const char * name;
    const struct HAL_RMT_IPC_CFG_T ** peer_cfg_pp;
    const struct HAL_RMT_IPC_CFG_T ** local_cfg_pp;

    HAL_RMT_IPC_IRQ_INIT irq_init;
    HAL_RMT_IPC_PEER_TX_IRQ_SET peer_tx_irq_set;
    HAL_RMT_IPC_LOCAL_TX_IRQ_CLEAR local_tx_irq_clear;
    HAL_RMT_IPC_RX_IRQ_SUSPEND rx_irq_suspend;
    HAL_RMT_IPC_RX_IRQ_RESUME rx_irq_resume;
    HAL_RMT_IPC_RX_DONE rx_done;
    HAL_RMT_IPC_IRQ_ACTIVE irq_active;
    HAL_RMT_IPC_IRQ_ENTRY rx_irq_entry;
    HAL_RMT_IPC_IRQ_ENTRY tx_irq_entry;
    HAL_RMT_IPC_PM_NOTIF_HANDLER pm_notif_handler;

    uint8_t wake_lock;
    uint8_t chan_num;
    uint16_t rec_num_per_chan;
    struct HAL_RMT_IPC_CH_CFG_T * chan_cfg;
    struct HAL_RMT_IPC_SEND_RECORD_T * send_record;
    const IRQn_Type * rx_irq_id;
    const IRQn_Type * tx_irq_id;
    bool * busy_p;
};

void hal_rmt_ipc_rx_irq_handler(const struct HAL_RMT_IPC_CFG_T *cfg);

void hal_rmt_ipc_tx_irq_handler(const struct HAL_RMT_IPC_CFG_T *cfg);

int hal_rmt_ipc_pm_notif_handler(const struct HAL_RMT_IPC_CFG_T *cfg, enum HAL_PM_STATE_T state);

int hal_rmt_ipc_open(RMT_IPC_OPEN_CFG_QUAL struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, HAL_RMT_IPC_RX_IRQ_HANDLER rxhandler, HAL_RMT_IPC_TX_IRQ_HANDLER txhandler, int rx_flowctrl);

int hal_rmt_ipc_close(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id);

int hal_rmt_ipc_start_recv(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id);

int hal_rmt_ipc_stop_recv(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id);

int hal_rmt_ipc_send_seq(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, const void *data, unsigned int len, unsigned int *seq);

int hal_rmt_ipc_rx_done(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id);

int hal_rmt_ipc_tx_irq_run(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id);

int hal_rmt_ipc_tx_active(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, unsigned int seq);

int hal_rmt_ipc_opened(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id);

int hal_rmt_ipc_set_chan_state(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, int opened);

#ifdef __cplusplus
}
#endif

#endif

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
#include "hal_rmt_ipc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"

#define RMT_IPC_UNIFIED_IRQ

#define RMT_IPC_RX_VERBOSE_TRACE            0 //(1 << 0)
#define RMT_IPC_TX_VERBOSE_TRACE            0 //(1 << 0)

static void hal_rmt_ipc_busy(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, bool busy)
{
    uint32_t i;
    bool new_state;
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);

    if (chan_cfg[id].chan_busy == busy) {
        return;
    }

    chan_cfg[id].chan_busy = busy;

    if (*cfg->busy_p == busy) {
        return;
    }

    if (busy) {
        hal_bus_wake_lock(cfg->wake_lock);
        *cfg->busy_p = true;
    } else {
        new_state = false;
        for (i = 0; i < cfg->chan_num; i++) {
            if (chan_cfg[i].chan_busy) {
                new_state = true;
                break;
            }
        }
        if (!new_state) {
            hal_bus_wake_unlock(cfg->wake_lock);
            *cfg->busy_p = false;
        }
    }
}

void hal_rmt_ipc_rx_irq_handler(const struct HAL_RMT_IPC_CFG_T *cfg)
{
    uint32_t id;
    const struct HAL_RMT_IPC_MSG_T *msg_ptr;
    unsigned int processed;
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);

    for (id = 0; id < cfg->chan_num; id++) {
#ifdef RMT_IPC_UNIFIED_IRQ
        if (!(NVIC_GetEnableIRQ(cfg->rx_irq_id[id]) && cfg->irq_active(id, HAL_RMT_IPC_IRQ_DATA_IND))) {
            continue;
        }
#else
        bool irq_pending = false;

        if (!NVIC_GetActive(cfg->rx_irq_id[id])) {
            // Check pending IRQ for manual IRQ handler loop inside int_lock env
            if (NVIC_GetPendingIRQ(cfg->rx_irq_id[id]) && NVIC_GetEnableIRQ(cfg->rx_irq_id[id])) {
                irq_pending = true;
            } else {
                continue;
            }
        }
#endif

        cfg->rx_irq_suspend(id);

        if (chan_cfg[id].recv_pending_head.data) {
            // Previous unprocessed message
            msg_ptr = &chan_cfg[id].recv_pending_head;
        } else {
            // New message
            if (!chan_cfg[id].recv_msg_list_pp) {
                TR_INFO(0, "%s-RX:%d: Got peer=%p (%p)", cfg->name, id, cfg->peer_cfg_pp, *cfg->peer_cfg_pp);

                chan_cfg[id].recv_msg_list_pp = (const struct HAL_RMT_IPC_MSG_T **)
                    &(*cfg->peer_cfg_pp)->chan_cfg[id].send_msg_list_p;
                TR_INFO(0, "%s-RX:%d: Got rxPtr=%p", cfg->name, id, chan_cfg[id].recv_msg_list_pp);
            }
            msg_ptr = *chan_cfg[id].recv_msg_list_pp;
        }
        while (msg_ptr) {
            if (RMT_IPC_RX_VERBOSE_TRACE & (1 << id)) {
                TR_INFO(0, "%s-RX:%d: msg=%p len=%u data=%p content=0x%08X 0x%08X",
                    cfg->name, id, msg_ptr, msg_ptr->len, msg_ptr->data, msg_ptr->data ? *(uint32_t *)msg_ptr->data : 0,
                    msg_ptr->data ? *(uint32_t *)(msg_ptr->data + 4) : 0);
            }
            if (chan_cfg[id].rx_irq_handler) {
                processed = chan_cfg[id].rx_irq_handler(msg_ptr->data, msg_ptr->len);
                // Check if flow control needed
                if (processed < msg_ptr->len) {
                    chan_cfg[id].recv_pending_head.next = msg_ptr->next;
                    chan_cfg[id].recv_pending_head.len = msg_ptr->len - processed;
                    chan_cfg[id].recv_pending_head.data = msg_ptr->data + processed;
                    break;
                }
            } else {
                // Error
                ASSERT(false, "%s-RX:%d: Handler missing", cfg->name, id);
                break;
            }
            msg_ptr = msg_ptr->next;
        }

        if (msg_ptr == NULL) {
            if (!chan_cfg[id].need_flow_ctrl){
                cfg->rx_done(id);
            }
            chan_cfg[id].recv_pending_head.data = NULL;
        }

#ifndef RMT_IPC_UNIFIED_IRQ
        if (irq_pending) {
            // Clear NVIC pending state at last (after clearing the IRQ source)
            NVIC_ClearPendingIRQ(cfg->rx_irq_id[id]);
        }
#endif
    }
}

static void hal_rmt_ipc_tx_irq_handler_by_id(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    struct HAL_RMT_IPC_MSG_T *msg_ptr;
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

#ifdef RMT_IPC_UNIFIED_IRQ
    if (!(NVIC_GetEnableIRQ(cfg->tx_irq_id[id]) && cfg->irq_active(id, HAL_RMT_IPC_IRQ_DATA_DONE))) {
        return;
    }
#else
    bool irq_pending = false;

    if (!NVIC_GetActive(cfg->tx_irq_id[id])) {
        // Check pending IRQ for manual IRQ handler loop inside int_lock env
        if (NVIC_GetPendingIRQ(cfg->tx_irq_id[id]) && NVIC_GetEnableIRQ(cfg->tx_irq_id[id])) {
            irq_pending = true;
        } else {
            return;
        }
    }
#endif

    cfg->local_tx_irq_clear(id);

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);
    msg_ptr = chan_cfg[id].send_msg_list_p;
    while (msg_ptr) {
        msg_ptr = ADDR_DEV_TO_CPU(msg_ptr);
        if (chan_cfg[id].tx_irq_handler) {
            chan_cfg[id].tx_irq_handler(ADDR_DEV_TO_CPU(msg_ptr->data), msg_ptr->len);
        };
        CONTAINER_OF(msg_ptr, struct HAL_RMT_IPC_SEND_RECORD_T, msg)->in_use = false;
        msg_ptr = msg_ptr->next;
    }

    if (chan_cfg[id].send_pending_list_p) {
#ifdef CPU_TO_DEV_ADDR_REMAP
        struct HAL_RMT_IPC_MSG_T *cur;
        struct HAL_RMT_IPC_MSG_T *next;

        cur = chan_cfg[id].send_pending_list_p;
        while (cur->next) {
            next = cur->next;
            cur->next = ADDR_CPU_TO_DEV(next);
            cur = next;
        }
#endif
        chan_cfg[id].send_msg_list_p =  ADDR_CPU_TO_DEV(chan_cfg[id].send_pending_list_p);
        chan_cfg[id].send_pending_list_p = NULL;
        cfg->peer_tx_irq_set(id);
    } else {
        chan_cfg[id].send_msg_list_p = NULL;
        // Allow sleep
        hal_rmt_ipc_busy(cfg, id, false);
    }

#ifndef RMT_IPC_UNIFIED_IRQ
    if (irq_pending) {
        // Clear NVIC pending state at last (after clearing the IRQ source)
        NVIC_ClearPendingIRQ(cfg->tx_irq_id[id]);
    }
#endif
}

void hal_rmt_ipc_tx_irq_handler(const struct HAL_RMT_IPC_CFG_T *cfg)
{
    uint32_t id;

    for (id = 0; id < cfg->chan_num; id++) {
        hal_rmt_ipc_tx_irq_handler_by_id(cfg, id);
    }
}

#ifdef CORE_SLEEP_POWER_DOWN
int hal_rmt_ipc_pm_notif_handler(const struct HAL_RMT_IPC_CFG_T *cfg, enum HAL_PM_STATE_T state)
{
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);

    if (state == HAL_PM_STATE_POWER_DOWN_WAKEUP) {
        for (uint32_t i = 0; i < cfg->chan_num; i++) {
            if (chan_cfg[i].chan_opened) {
                cfg->irq_init(i);
            }
        }
    }

    return 0;
}
#endif

static int hal_rmt_ipc_check_config(const struct HAL_RMT_IPC_CFG_T *cfg)
{
    if (!cfg->name) {
        return -1;
    }
    if (!cfg->peer_cfg_pp) {
        return -2;
    }
    if (!cfg->local_cfg_pp) {
        return -3;
    }
    if (!cfg->irq_init) {
        return -4;
    }
    if (!cfg->peer_tx_irq_set) {
        return -5;
    }
    if (!cfg->local_tx_irq_clear) {
        return -6;
    }
    if (!cfg->rx_irq_suspend) {
        return -7;
    }
    if (!cfg->rx_irq_resume) {
        return -8;
    }
    if (!cfg->rx_done) {
        return -9;
    }
    if (!cfg->irq_active) {
        return -10;
    }
    if (!cfg->rx_irq_entry) {
        return -11;
    }
    if (!cfg->tx_irq_entry) {
        return -12;
    }
    if (!cfg->chan_num) {
        return -13;
    }
    if (!cfg->rec_num_per_chan) {
        return -14;
    }
    if (!cfg->chan_cfg) {
        return -15;
    }
    if (!cfg->send_record) {
        return -16;
    }
    if (!cfg->rx_irq_id) {
        return -17;
    }
    if (!cfg->tx_irq_id) {
        return -18;
    }
    if (!cfg->busy_p) {
        return -19;
    }
    return 0;
}

int hal_rmt_ipc_open(RMT_IPC_OPEN_CFG_QUAL struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, HAL_RMT_IPC_RX_IRQ_HANDLER rxhandler, HAL_RMT_IPC_TX_IRQ_HANDLER txhandler, int rx_flowctrl)
{
    int ret;

    ret = hal_rmt_ipc_check_config(cfg);
    if (ret) {
        return ret;
    }

    if (id >= cfg->chan_num) {
        return 1;
    }

    rx_flowctrl = !!rx_flowctrl;

    if (cfg->chan_cfg[id].chan_opened) {
        ASSERT(cfg->chan_cfg[id].need_flow_ctrl == rx_flowctrl, "RMT_IPC-OPEN: rx_flowctrl=%d (should be %d)", rx_flowctrl, cfg->chan_cfg[id].need_flow_ctrl);
        return 3;
    }

    // Stop IRQs by default
    NVIC_DisableIRQ(cfg->rx_irq_id[id]);
    NVIC_DisableIRQ(cfg->tx_irq_id[id]);

    NVIC_SetVector(cfg->rx_irq_id[id], (uint32_t)cfg->rx_irq_entry);
    NVIC_SetPriority(cfg->rx_irq_id[id], IRQ_PRIORITY_NORMAL);

    NVIC_SetVector(cfg->tx_irq_id[id], (uint32_t)cfg->tx_irq_entry);
    NVIC_SetPriority(cfg->tx_irq_id[id], IRQ_PRIORITY_NORMAL);

    cfg->irq_init(id);

    memset(&cfg->chan_cfg[id], 0, sizeof(cfg->chan_cfg[id]));

    cfg->chan_cfg[id].need_flow_ctrl = rx_flowctrl;
    cfg->chan_cfg[id].send_record = cfg->send_record + id * cfg->rec_num_per_chan;
    cfg->chan_cfg[id].send_rec_num = cfg->rec_num_per_chan;

    memset(cfg->chan_cfg[id].send_record, 0,
        sizeof(*cfg->chan_cfg[id].send_record) * cfg->chan_cfg[id].send_rec_num);

    TR_INFO(0, "%s-OPEN:%d: peer=%p (n/a) local=%p (%p)",
        cfg->name, id, cfg->peer_cfg_pp, cfg->local_cfg_pp, *cfg->local_cfg_pp);
    TR_INFO(0, "%s-OPEN:%d: rxPtr=(n/a) txPtr=%p flowCtrl=%d",
        cfg->name, id, &cfg->chan_cfg[id].send_msg_list_p, cfg->chan_cfg[id].need_flow_ctrl);

#ifdef CORE_SLEEP_POWER_DOWN
    if (cfg->pm_notif_handler) {
        uint32_t i;

        for (i = 0; i < cfg->chan_num; i++) {
            if (cfg->chan_cfg[i].chan_opened) {
                break;
            }
        }
        if (i >= cfg->chan_num) {
            hal_pm_notif_register(HAL_PM_USER_HAL, cfg->pm_notif_handler);
        }
    }
#endif

    cfg->chan_cfg[id].rx_irq_handler = rxhandler;
    cfg->chan_cfg[id].tx_irq_handler = txhandler;

    cfg->chan_cfg[id].chan_opened = true;

    // Enable TX IRQ
    NVIC_ClearPendingIRQ(cfg->tx_irq_id[id]);
    NVIC_EnableIRQ(cfg->tx_irq_id[id]);

    *cfg->local_cfg_pp = ADDR_CPU_TO_DEV(cfg);
#ifdef CPU_TO_DEV_ADDR_REMAP
    cfg->chan_cfg = ADDR_CPU_TO_DEV(cfg->chan_cfg);
#endif
    __DSB();

    return 0;
}

int hal_rmt_ipc_close(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    uint32_t lock;
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    if (id >= cfg->chan_num) {
        return 1;
    }

    // Stop IRQs by default
    NVIC_DisableIRQ(cfg->rx_irq_id[id]);
    NVIC_DisableIRQ(cfg->tx_irq_id[id]);

    // Clear IRQs generated by local
    cfg->irq_init(id);

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);

    lock = int_lock();
    // Release busy state
    hal_rmt_ipc_busy(cfg, id, false);
    int_unlock(lock);

    memset(chan_cfg[id].send_record, 0,
        sizeof(*chan_cfg[id].send_record) * chan_cfg[id].send_rec_num);
    memset(&chan_cfg[id], 0, sizeof(chan_cfg[id]));

#ifdef CORE_SLEEP_POWER_DOWN
    if (cfg->pm_notif_handler) {
        uint32_t i;

        for (i = 0; i < cfg->chan_num; i++) {
            if (chan_cfg[i].chan_opened) {
                break;
            }
        }
        if (i >= cfg->chan_num) {
            hal_pm_notif_deregister(HAL_PM_USER_HAL, cfg->pm_notif_handler);
        }
    }
#endif

    return 0;
}

int hal_rmt_ipc_start_recv(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    if (id >= cfg->chan_num) {
        return 1;
    }

    NVIC_ClearPendingIRQ(cfg->rx_irq_id[id]);
    NVIC_EnableIRQ(cfg->rx_irq_id[id]);

    // Check if there is any previous unprocessed message
    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);
    if (chan_cfg[id].recv_pending_head.data) {
        cfg->rx_irq_resume(id);
    }

    return 0;
}

int hal_rmt_ipc_stop_recv(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    if (id >= cfg->chan_num) {
        return 1;
    }

    NVIC_DisableIRQ(cfg->rx_irq_id[id]);

    return 0;
}

int hal_rmt_ipc_send_seq(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, const void *data, unsigned int len, unsigned int *seq)
{
    uint32_t lock;
    int ret;
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;
    struct HAL_RMT_IPC_SEND_RECORD_T *record;
    struct HAL_RMT_IPC_MSG_T *next;
    int i;

    if (id >= cfg->chan_num) {
        return 1;
    }
    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);
    if (!chan_cfg[id].chan_opened) {
        return 2;
    }

    ret = -1;
    record = chan_cfg[id].send_record;

    lock = int_lock();
    for (i = 0; i < chan_cfg[id].send_rec_num; i++) {
        if (record->in_use) {
            record++;
            continue;
        }
        record->in_use = true;
        record->msg.next = NULL;
        record->msg.len = len;
        record->msg.data = ADDR_CPU_TO_DEV(data);
        if (RMT_IPC_TX_VERBOSE_TRACE & (1 << id)) {
            TR_INFO(0, "%s-TX:%d: msg=%p len=%u data=%p content=0x%08X 0x%08X",
                cfg->name, id, &record->msg, len, data,
                data ? *(uint32_t *)data : 0, data ? *(uint32_t *)(data + 4) : 0);
        }
        if (chan_cfg[id].send_msg_list_p == NULL) {
            chan_cfg[id].send_msg_list_p = ADDR_CPU_TO_DEV(&record->msg);
            __DSB();
            cfg->peer_tx_irq_set(id);
        } else if (chan_cfg[id].send_pending_list_p == NULL) {
            chan_cfg[id].send_pending_list_p = &record->msg;
        } else {
            next = chan_cfg[id].send_pending_list_p;
            while (next->next) {
                next = next->next;
            }
            next->next = &record->msg;
        }
        ret = 0;
        if (seq) {
            *seq = i;
        }
        // Prohibit sleep here
        hal_rmt_ipc_busy(cfg, id, true);
        break;
    }
    int_unlock(lock);

    return ret;
}

int hal_rmt_ipc_send(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, const void *data, unsigned int len)
{
    return hal_rmt_ipc_send_seq(cfg, id, data, len, NULL);
}

int hal_rmt_ipc_rx_done(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    if (id >= cfg->chan_num) {
        return 1;
    }

    cfg->rx_done(id);

    return 0;
}

int hal_rmt_ipc_tx_irq_run(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    uint32_t lock;

    if (id >= cfg->chan_num) {
        return 1;
    }

    lock = int_lock();
    hal_rmt_ipc_tx_irq_handler_by_id(cfg, id);
    int_unlock(lock);

    return 0;
}

int hal_rmt_ipc_tx_active(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, unsigned int seq)
{
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    if (id >= cfg->chan_num) {
        return false;
    }

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);
    if (seq < chan_cfg[id].send_rec_num) {
        return chan_cfg[id].send_record[seq].in_use;
    } else if (seq == HAL_RMT_IPC_ALL_SEND_SEQ) {
        uint32_t i;
        uint32_t lock;

        lock = int_lock();
        for (i = 0; i < chan_cfg[id].send_rec_num; i++) {
            if (chan_cfg[id].send_record[i].in_use) {
                break;
            }
        }
        int_unlock(lock);
        if (i < chan_cfg[id].send_rec_num) {
            return true;
        }
    }

    return false;
}

int hal_rmt_ipc_opened(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id)
{
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    if (id >= cfg->chan_num) {
        return false;
    }

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);
    return chan_cfg[id].chan_opened;
}

int hal_rmt_ipc_set_chan_state(const struct HAL_RMT_IPC_CFG_T *cfg, uint32_t id, int opened)
{
    struct HAL_RMT_IPC_CH_CFG_T *chan_cfg;

    if (id >= cfg->chan_num) {
        return 1;
    }

    chan_cfg = ADDR_DEV_TO_CPU(cfg->chan_cfg);
    chan_cfg[id].chan_opened = !!opened;

    return 0;
}

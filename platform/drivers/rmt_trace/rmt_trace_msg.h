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
#ifndef __RMT_TRC_MSG_H__
#define __RMT_TRC_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

#define RMT_TRC_MSG_MAGIC_WORD              0xFFFFFFFF
#define RMT_TRC_CTRL_MAGIC_WORD             0xBE57A55A

typedef int (*RMT_TRC_SEND_CB)(const void *data, unsigned int len, unsigned int *seq);

enum RMT_TRC_MSG_ID_T {
    RMT_TRC_MSG_ID_TRACE,
    RMT_TRC_MSG_ID_FLUSH,
    RMT_TRC_MSG_ID_CRASH,
    RMT_TRC_MSG_ID_SERVER_DUMP,
};

enum RMT_TRC_STATE_T {
    RMT_TRC_STATE_TRACE,
    RMT_TRC_STATE_CRASH_ASSERT_START,
    RMT_TRC_STATE_CRASH_FAULT_START,
    RMT_TRC_STATE_CRASH_END,
};

struct RMT_TRC_CTRL_T {
    uint32_t ctrl_magic;
    const uint8_t *buf_start;
    const uint16_t *entry_start;
    uint16_t buf_size;
    uint16_t entry_cnt;
    volatile uint16_t entry_rpos;
    volatile uint16_t entry_wpos;
    uint16_t discard_cnt_r;
    uint16_t discard_cnt_w;
    uint8_t seq_r;
    uint8_t seq_w;
    uint8_t state;
};

struct RMT_TRC_MSG_T {
    uint32_t msg_magic;
    uint32_t id;
    void *param;
};

int rmt_trace_check_trace(const void *data, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif


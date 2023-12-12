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
#include "plat_addr_map.h"

#ifdef TRNG_BASE

#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_trng.h"
#include "stdlib.h"
#include "string.h"

#if 1
#define RANDOM_TRACE(...)                   TRACE_DUMMY(__VA_ARGS__)
#else
#define RANDOM_TRACE(...)                   TRACE(__VA_ARGS__)
#endif

#define TRNG_BUF_LEN                        (HAL_TRNG_DATA_LEN * 5)

#define TRNG_MAX_ERR_CNT                    20

static const struct HAL_TRNG_CFG_T trng_cfg = { 0x40, HAL_TRNG_RND_SRC_SHORTEST, };

static uint8_t trng_buf[TRNG_BUF_LEN];
static uint16_t buf_rpos;
static uint16_t buf_wpos;
static bool buf_full;

static bool trng_opened;
static bool trng_working;
static bool read_lock;

static uint16_t error_cnt;

static void trng_cb(const uint32_t *buf, uint32_t len, uint32_t error);

static uint32_t get_free_len(uint32_t rpos, uint32_t wpos, bool full)
{
    uint32_t free_len;

    if (wpos > rpos) {
        free_len = sizeof(trng_buf) - wpos + rpos;
    } else if (wpos < rpos) {
        free_len = rpos - wpos;
    } else {
        free_len = full ? 0 : sizeof(trng_buf);
    }

    return free_len;
}

static int random_loop_get_data(void)
{
    uint32_t lock;
    uint32_t free_len;
    uint32_t rpos = 0, wpos = 0;
    bool full = false;
    bool filled_data = false;
    int ret;

    lock = int_lock();
    rpos = buf_rpos;
    wpos = buf_wpos;
    full = buf_full;
    int_unlock(lock);

    for (uint32_t i = 0; i < 2; i++) {
        if (rpos < wpos) {
            free_len = sizeof(trng_buf) - wpos;
        } else if (rpos > wpos) {
            free_len = rpos - wpos;
        } else {
            free_len = full ? 0 : sizeof(trng_buf);
        }
        if (free_len == 0) {
            lock = int_lock();
            if (filled_data) {
                buf_wpos = wpos;
                buf_full = (buf_rpos == buf_wpos);
            }
            int_unlock(lock);
            return 0;
        }

        ret = hal_trng_get_data(&trng_buf[wpos], free_len);
        if (ret) {
            return ret;
        }
        wpos += free_len;
        if (wpos >= sizeof(trng_buf)) {
            wpos = 0;
        }
        if (rpos == wpos) {
            full = true;
        }
        filled_data = true;
    }

    return 101;
}

static void random_run(void)
{
    int ret = 0;

    if (!trng_opened) {
        ret = hal_trng_open(&trng_cfg);
        ASSERT(ret == 0, "%s: trng open error: %d", __func__, ret);
        trng_opened = true;
    }

    if (!trng_working) {
        ret = hal_trng_run(trng_cb);
        if (ret) {
            TRACE(0, "hal_trng_run failed: %d", ret);
            ret = random_loop_get_data();
            if (ret) {
                TRACE(0, "random_loop_get_data failed: %d", ret);
                ASSERT(++error_cnt <= TRNG_MAX_ERR_CNT, "%s: Too many errors: %u", __func__, error_cnt);
            } else {
                error_cnt = 0;
            }
        } else {
            trng_working = true;
        }
    }
}

static void check_and_start_transaction(void)
{
    uint32_t free_len;
    uint32_t lock;

    lock = int_lock();

    free_len = get_free_len(buf_rpos, buf_wpos, buf_full);
    if (free_len >= HAL_TRNG_DATA_LEN) {
        random_run();
    }

    int_unlock(lock);
}

static void trng_cb(const uint32_t *buf, uint32_t len, uint32_t error)
{
    uint32_t lock;
    uint32_t free_len;
    uint32_t rpos, wpos;
    bool full;
    bool filled_data;

    trng_working = false;

    if (!buf || !len || error) {
        TRACE(0, "TRNG irq error: buf=%p len=%u error=0x%0X", buf, len, error);
        ASSERT(++error_cnt <= TRNG_MAX_ERR_CNT, "%s: Too many errors: %u", __func__, error_cnt);
        return;
    }
    error_cnt = 0;

    filled_data = false;

    lock = int_lock();
    rpos = buf_rpos;
    wpos = buf_wpos;
    full = buf_full;
    int_unlock(lock);

    while (len) {
        if (rpos < wpos) {
            free_len = sizeof(trng_buf) - wpos;
        } else if (rpos > wpos) {
            free_len = rpos - wpos;
        } else {
            free_len = full ? 0 : sizeof(trng_buf);
        }
        if (!free_len) {
            break;
        }
        if (free_len > len) {
            free_len = len;
        }
        memcpy(&trng_buf[wpos], buf, free_len);
        buf += free_len;
        len -= free_len;
        wpos += free_len;
        if (wpos >= sizeof(trng_buf)) {
            wpos = 0;
        }
        if (rpos == wpos) {
            full = true;
        }
        filled_data = true;
    }

    lock = int_lock();
    if (filled_data) {
        buf_wpos = wpos;
        buf_full = (buf_rpos == buf_wpos);
        check_and_start_transaction();
    }
    int_unlock(lock);

    TRACE(0, "RANDOM/PUT: rpos=%3u wpos=%3u full=%u", rpos, wpos, full);
}

int random_get_data(unsigned char *buf, unsigned int len)
{
    uint32_t lock;
    uint32_t avail_len;
    uint32_t rpos = 0, wpos = 0;
    bool full = false;
    bool got_data;

    if (!buf) {
        return -1;
    }
    if (!len) {
        return -2;
    }

    while (len) {
        while (set_bool_flag(&read_lock)) {
            osDelay(1);
        }

        got_data = false;

        lock = int_lock();
        rpos = buf_rpos;
        wpos = buf_wpos;
        full = buf_full;
        int_unlock(lock);

        do {
            if (wpos > rpos) {
                avail_len = wpos - rpos;
            } else if (wpos < rpos) {
                avail_len = sizeof(trng_buf) - rpos;
            } else {
                if (full) {
                    avail_len = sizeof(trng_buf) - rpos;
                } else {
                    avail_len = 0;
                }
            }
            if (!avail_len) {
                break;
            }
            if (avail_len > len) {
                avail_len = len;
            }
            memcpy(buf, &trng_buf[rpos], avail_len);
            buf += avail_len;
            len -= avail_len;
            rpos += avail_len;
            if (rpos >= sizeof(trng_buf)) {
                rpos = 0;
            }
            got_data = true;
        } while (len);

        lock = int_lock();
        if (got_data) {
            buf_rpos = rpos;
            buf_full = false;
        }
        check_and_start_transaction();
        int_unlock(lock);

        clear_bool_flag(&read_lock);

        if (!len) {
            break;
        }
        osDelay(4);
    }

    TRACE(0, "RANDOM/GET: rpos=%3u wpos=%3u full=%u", rpos, wpos, full);

    return 0;
}

#if !defined(NUTTX_BUILD)
void srand(unsigned int init)
{
    check_and_start_transaction();
}

int rand(void)
{
    uint8_t data[4];
    uint8_t len;
    int val;
    int ret;

#if (RAND_MAX > 32767)
    len = 4;
#else
    len = 2;
#endif

    ret = random_get_data(data, len);
    ASSERT(!ret, "random_get_data failed: %d", ret);

#if (RAND_MAX > 32767)
    val = ((uint32_t)data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) / 2;
#else
    val = ((uint32_t)data[0] | (data[1] << 8)) / 2;
#endif
    return val;
}
#endif
#endif

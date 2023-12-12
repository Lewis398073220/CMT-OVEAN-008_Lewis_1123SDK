/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#include "plat_types.h"
#include "hwtimer_list.h"
#define IGNORE_HAL_TIMER_RAW_API_CHECK
#include "hal_timer_raw.h"
#include "hal_trace.h"
#include "stdio.h"
#include "cmsis.h"

#if defined(ROM_BUILD) && !defined(SIMU) && !defined(FPGA)
#error "The user of raw timer API must be unique. Now rom is using raw timer API."
#endif

#define CHECK_ACTIVE_NULL_IN_IRQ        1

#ifndef HWTIMER_NUM
#define HWTIMER_NUM                     15
#endif

#ifdef HWTIMER_DEBUG
#define HWTIMER_ASSERT(c, s, ...)       \
    do { \
        if (!(c)) { \
            dump_debug_info(); \
            hwtimer_dump(); \
            ASSERT(false, s, ##__VA_ARGS__); \
        } \
    } while (0)
#define SAVE_INFO(op, timer)            save_debug_info(op, __builtin_return_address(0), timer)
#define SAVE_INFO_CALLER(op, timer)     save_debug_info(op, caller, timer)
#define CALLER_PARAM                    , const void *caller
#define CALLER_ARG                      , __builtin_return_address(0)
#else
#define HWTIMER_ASSERT(c, s, ...)       ASSERT(c, s, ##__VA_ARGS__)
#define SAVE_INFO(op, timer)
#define SAVE_INFO_CALLER(op, timer)
#define CALLER_PARAM
#define CALLER_ARG
#endif

//#define HWTIMER_TEST

enum HWTIMER_STATE_T {
    HWTIMER_STATE_FREE = 0,
    HWTIMER_STATE_ALLOC,
    HWTIMER_STATE_ACTIVE,
    HWTIMER_STATE_FIRED,
    HWTIMER_STATE_CALLBACK,

    HWTIMER_STATE_QTY
};

enum HWTIMER_DEBUG_OP_T {
    HWTIMER_DEBUG_OP_NONE,
    HWTIMER_DEBUG_OP_START,
    HWTIMER_DEBUG_OP_STOP,
    HWTIMER_DEBUG_OP_FIRED,
};

struct HWTIMER_T {
    enum HWTIMER_STATE_T state;
    struct HWTIMER_T *next;
    uint32_t time;
    HWTIMER_CALLBACK_T callback;
    void *param;
};

struct HWTIMER_LIST_T {
    struct HWTIMER_T timer[HWTIMER_NUM];
    struct HWTIMER_T *free;
    struct HWTIMER_T *active;
    struct HWTIMER_T *fired;
};

struct HWTIMER_DEBUG_INFO_T {
    enum HWTIMER_DEBUG_OP_T op;
    uint32_t cur_time;
    uint32_t caller;
    uint32_t timer;
    uint32_t active;
};

static bool hwtimer_inited = false;
static struct HWTIMER_LIST_T hwtimer_list;
static uint32_t err_irq_active_null = 0;
static uint32_t err_irq_early = 0;

#ifdef HWTIMER_DEBUG
static uint32_t err_stop_in_cb = 0;

static struct HWTIMER_DEBUG_INFO_T debug_info[100];
static uint32_t debug_info_idx;

static void save_debug_info(enum HWTIMER_DEBUG_OP_T op, const void *caller, const struct HWTIMER_T *timer)
{
    struct HWTIMER_DEBUG_INFO_T *info;

    info = &debug_info[debug_info_idx];
    debug_info_idx++;
    if (debug_info_idx >= ARRAY_SIZE(debug_info)) {
        debug_info_idx = 0;
    }

    info->op = op;
    info->caller = (uint32_t)caller;
    info->timer = (uint32_t)timer;
    info->cur_time = hal_sys_timer_get();
    info->active = (uint32_t)hwtimer_list.active;
}

static void dump_debug_info(void)
{
    uint32_t i;
    uint32_t j;
    uint32_t cur_idx;
    uint32_t lock;
    const char *op_str;
    struct HWTIMER_DEBUG_INFO_T *info;

    lock = int_lock();

    // Save current index before any trace operation to avoid trace changing it (e.g., rmt trace timer)
    cur_idx = debug_info_idx;

    TRACE(TR_ATTR_NO_TS, "------");
    TRACE(TR_ATTR_NO_TS, "HWTIMER DEBUG INFO DUMP:");
    TRACE_FLUSH();

    TRACE(TR_ATTR_NO_TS, "err_irq_active_null=%u", err_irq_active_null);
    TRACE(TR_ATTR_NO_TS, "err_irq_early=%u", err_irq_early);
    TRACE(TR_ATTR_NO_TS, "err_stop_in_cb=%u", err_stop_in_cb);

    for (i = 0; i < ARRAY_SIZE(debug_info); i++) {
        j = cur_idx + i;
        if (j >= ARRAY_SIZE(debug_info)) {
            j -= ARRAY_SIZE(debug_info);
        }
        info = &debug_info[j];
        if (info->op != HWTIMER_DEBUG_OP_NONE) {
            if (info->op == HWTIMER_DEBUG_OP_START) {
                op_str = "START";
            } else if (info->op == HWTIMER_DEBUG_OP_STOP) {
                op_str = "STOP ";
            } else if (info->op == HWTIMER_DEBUG_OP_FIRED) {
                op_str = "FIRE ";
            } else {
                op_str = "BAD  ";
            }
            TRACE(TR_ATTR_NO_TS, "[%2u/0x%08X]: %s lr=0x%08X timer=0x%08X active=0x%08X",
                i, info->cur_time, op_str, info->caller, info->timer, info->active);
        }
        if (((i + 1) % 20) == 0) {
            TRACE_FLUSH();
        }
    }

    int_unlock(lock);

    TRACE(TR_ATTR_NO_TS, "------");
    TRACE_OUTPUT_LINEFEED();
    TRACE_FLUSH();
}
#endif

static void hwtimer_handler(uint32_t elapsed)
{
    struct HWTIMER_T *pre;
    struct HWTIMER_T *last;
    uint32_t lock = 0;

    lock = int_lock();

    if (hwtimer_list.active == NULL) {
        err_irq_active_null++;
        TRACE(1,"HWTIMER irq when active is null: might be deleted? %u", err_irq_active_null);
#if (CHECK_ACTIVE_NULL_IN_IRQ)
        HWTIMER_ASSERT(hal_timer_is_enabled() == 0, "HWTIMER collapsed: irq when active is null");
#endif
        goto _exit;
    }
    // Update elapsed time
    elapsed = hal_timer_get_elapsed_time();
    if (hwtimer_list.active->time > elapsed + HAL_TIMER_LOAD_DELTA) {
        err_irq_early++;
        TRACE(1,"HWTIMER irq occurred early: old active timer might be deleted? %u", err_irq_early);
        HWTIMER_ASSERT(hal_timer_is_enabled(), "HWTIMER collapsed: irq occurred too early");
        goto _exit;
    }

    if (elapsed > hwtimer_list.active->time) {
        elapsed -= hwtimer_list.active->time;
    } else {
        elapsed = 0;
    }
    pre = hwtimer_list.active;
    // TODO: Check state here ?
    pre->state = HWTIMER_STATE_FIRED;
    last = hwtimer_list.active->next;
    while (last && last->time <= elapsed) {
        elapsed -= last->time;
        pre = last;
        // TODO: Check state here ?
        pre->state = HWTIMER_STATE_FIRED;
        last = last->next;
    }
    pre->next = NULL;
    hwtimer_list.fired = hwtimer_list.active;
    hwtimer_list.active = last;
    if (last) {
        last->time -= elapsed;
        hal_timer_start(last->time);
#if (CHECK_ACTIVE_NULL_IN_IRQ)
    } else {
        hal_timer_stop();
#endif
    }

    while (hwtimer_list.fired) {
        last = hwtimer_list.fired;
        hwtimer_list.fired = last->next;
        // TODO: Check state here ?
        last->state = HWTIMER_STATE_CALLBACK;
        last->next = NULL;
        SAVE_INFO(HWTIMER_DEBUG_OP_FIRED, last);
        // Now this timer can be restarted, but not stopped or freed
        if (last->callback) {
            int_unlock(lock);
            last->callback(last->param);
            lock = int_lock();
        }
        if (last->state == HWTIMER_STATE_CALLBACK) {
            last->state = HWTIMER_STATE_ALLOC;
        }
    }

_exit:
    int_unlock(lock);
}

void hwtimer_init(void)
{
    int i;
    if (hwtimer_inited) {
        return;
    }
    hwtimer_inited = true;
    for (i = 0; i < HWTIMER_NUM - 1; i++) {
        hwtimer_list.timer[i].state = HWTIMER_STATE_FREE;
        hwtimer_list.timer[i].next = &hwtimer_list.timer[i + 1];
    }
    hwtimer_list.timer[HWTIMER_NUM - 1].next = NULL;
    hwtimer_list.free = &hwtimer_list.timer[0];
    hwtimer_list.active = NULL;
    hwtimer_list.fired = NULL;
    hal_timer_setup(HAL_TIMER_TYPE_ONESHOT, hwtimer_handler);
}

HWTIMER_ID hwtimer_alloc(HWTIMER_CALLBACK_T callback, void *param)
{
    struct HWTIMER_T *timer;
    uint32_t lock;

    timer = NULL;

    lock = int_lock();
    if (hwtimer_list.free != NULL) {
        timer = hwtimer_list.free;
        hwtimer_list.free = hwtimer_list.free->next;
    }
    int_unlock(lock);

    if (timer == NULL) {
        return NULL;
    }

    HWTIMER_ASSERT(timer->state == HWTIMER_STATE_FREE, "HWTIMER-ALLOC: Invalid state: %d", timer->state);
    timer->state = HWTIMER_STATE_ALLOC;
    timer->callback = callback;
    timer->param = param;
    timer->next = NULL;

    return timer;
}

enum E_HWTIMER_T hwtimer_free(HWTIMER_ID id)
{
    enum E_HWTIMER_T ret;
    struct HWTIMER_T *timer;
    uint32_t lock;

    timer = (struct HWTIMER_T *)id;

    if (timer < &hwtimer_list.timer[0] || timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
        return E_HWTIMER_INVAL_ID;
    }

    ret = E_HWTIMER_OK;

    lock = int_lock();

    if (timer->state != HWTIMER_STATE_ALLOC) {
        ret = E_HWTIMER_INVAL_ST;
        goto _exit;
    }
    timer->state = HWTIMER_STATE_FREE;

    timer->next = hwtimer_list.free;
    hwtimer_list.free = timer;

_exit:
    int_unlock(lock);

    return ret;
}

static enum E_HWTIMER_T __hwtimer_start_int(HWTIMER_ID id, int update, HWTIMER_CALLBACK_T callback, void *param, unsigned int ticks CALLER_PARAM)
{
    enum E_HWTIMER_T ret;
    struct HWTIMER_T *timer;
    struct HWTIMER_T *pre;
    struct HWTIMER_T *next;
    uint32_t lock;
    uint32_t cur_time;

    timer = (struct HWTIMER_T *)id;

    if (timer < &hwtimer_list.timer[0] || timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
        return E_HWTIMER_INVAL_ID;
    }

    if (ticks < HAL_TIMER_LOAD_DELTA) {
        ticks = HAL_TIMER_LOAD_DELTA;
    }

    ret = E_HWTIMER_OK;

    lock = int_lock();

    if (timer->state != HWTIMER_STATE_ALLOC && timer->state != HWTIMER_STATE_CALLBACK) {
        ret = E_HWTIMER_INVAL_ST;
        goto _exit;
    }
    timer->state = HWTIMER_STATE_ACTIVE;

    if (update) {
        timer->callback = callback;
        timer->param = param;
    }

    if (hwtimer_list.active == NULL) {
        timer->next = NULL;
        hwtimer_list.active = timer;
        hal_timer_start(ticks);
    } else {
        cur_time = hal_timer_get_raw_value();
        HWTIMER_ASSERT(cur_time <= hwtimer_list.active->time || cur_time <= HAL_TIMER_LOAD_DELTA,
            "HWTIMER-START collapsed: cur=%u active=%u",
            cur_time, hwtimer_list.active->time);
        if (cur_time > ticks) {
            hwtimer_list.active->time = cur_time - ticks;
            timer->next = hwtimer_list.active;
            hwtimer_list.active = timer;
            hal_timer_stop();
            hal_timer_start(ticks);
        } else {
            pre = hwtimer_list.active;
            next = hwtimer_list.active->next;
            ticks -= cur_time;
            while (next && next->time < ticks) {
                ticks -= next->time;
                pre = next;
                next = next->next;
            }
            if (next) {
                next->time -= ticks;
            }
            pre->next = timer;
            timer->next = next;
        }
    }
    timer->time = ticks;

    SAVE_INFO_CALLER(HWTIMER_DEBUG_OP_START, timer);

_exit:
    int_unlock(lock);

    return ret;
}

enum E_HWTIMER_T hwtimer_start(HWTIMER_ID id, unsigned int ticks)
{
    return __hwtimer_start_int(id, 0, NULL, NULL, ticks CALLER_ARG);
}

enum E_HWTIMER_T hwtimer_update_then_start(HWTIMER_ID id, HWTIMER_CALLBACK_T callback, void *param, unsigned int ticks)
{
    return __hwtimer_start_int(id, 1, callback, param, ticks CALLER_ARG);
}

enum E_HWTIMER_T hwtimer_update(HWTIMER_ID id, HWTIMER_CALLBACK_T callback, void *param)
{
    enum E_HWTIMER_T ret;
    struct HWTIMER_T *timer;
    uint32_t lock;

    timer = (struct HWTIMER_T *)id;

    if (timer < &hwtimer_list.timer[0] || timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
        return E_HWTIMER_INVAL_ID;
    }

    ret = E_HWTIMER_OK;

    lock = int_lock();

    if (timer->state == HWTIMER_STATE_ALLOC || timer->state == HWTIMER_STATE_CALLBACK) {
        timer->callback = callback;
        timer->param = param;
    } else {
        ret = E_HWTIMER_INVAL_ST;
    }

    int_unlock(lock);

    return ret;
}

enum E_HWTIMER_T hwtimer_stop(HWTIMER_ID id)
{
    enum E_HWTIMER_T ret;
    struct HWTIMER_T *timer;
    struct HWTIMER_T *pre;
    struct HWTIMER_T *next;
    uint32_t cur_time;
    uint32_t elapsed_time;
    uint32_t lock;

    timer = (struct HWTIMER_T *)id;

    if (timer < &hwtimer_list.timer[0] || timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
        return E_HWTIMER_INVAL_ID;
    }

    ret = E_HWTIMER_OK;

    lock = int_lock();

    if (timer->state == HWTIMER_STATE_ALLOC) {
        // Already stopped
        goto _exit;
    } else if (timer->state == HWTIMER_STATE_ACTIVE) {
        // Active timer
        if (hwtimer_list.active == timer) {
            cur_time = hal_timer_get_raw_value();
            HWTIMER_ASSERT(cur_time <= hwtimer_list.active->time || cur_time <= HAL_TIMER_LOAD_DELTA,
                "HWTIMER-STOP collapsed: cur=%u active=%u",
                cur_time, hwtimer_list.active->time);
            hal_timer_stop();
            next = hwtimer_list.active->next;
            if (next) {
                if (cur_time == 0) {
                    elapsed_time = hal_timer_get_elapsed_time();
                    HWTIMER_ASSERT(elapsed_time + HAL_TIMER_LOAD_DELTA >= hwtimer_list.active->time,
                        "HWTIMER-STOP collapsed: elapsed=%u active=%u",
                        elapsed_time, hwtimer_list.active->time);
                    if (elapsed_time > hwtimer_list.active->time) {
                        elapsed_time -= hwtimer_list.active->time;
                    } else {
                        elapsed_time = 0;
                    }
                    pre = next;
                    while (pre && pre->time <= elapsed_time) {
                        elapsed_time -= pre->time;
                        pre->time = 0;
                        pre = pre->next;
                    }
                    if (pre) {
                        pre->time -= elapsed_time;
                    }
                } else {
                    next->time += cur_time;
                }
                hal_timer_start(next->time);
            }
            hwtimer_list.active = next;
        } else if (hwtimer_list.active) {
            pre = hwtimer_list.active;
            next = hwtimer_list.active->next;
            while (next && next != timer) {
                pre = next;
                next = next->next;
            }
            if (next == timer) {
                pre->next = next->next;
                if (next->next) {
                    next->next->time += timer->time;
                }
            } else {
                ret = E_HWTIMER_NOT_FOUND;
            }
        } else {
            ret = E_HWTIMER_NOT_FOUND;
        }
        HWTIMER_ASSERT(ret == E_HWTIMER_OK, "HWTIMER-STOP collapsed: active timer 0x%08x not in list 0x%08x",
            (uint32_t)timer, (uint32_t)hwtimer_list.active);
    } else if (timer->state == HWTIMER_STATE_FIRED) {
        // Fired timer
        if (hwtimer_list.fired == timer) {
            // The timer handler is preempted
            hwtimer_list.fired = hwtimer_list.fired->next;
        } else if (hwtimer_list.fired) {
            pre = hwtimer_list.fired;
            next = hwtimer_list.fired->next;
            while (next && next != timer) {
                pre = next;
                next = next->next;
            }
            if (next == timer) {
                pre->next = next->next;
            } else {
                ret = E_HWTIMER_NOT_FOUND;
            }
        } else {
            ret = E_HWTIMER_NOT_FOUND;
        }
        HWTIMER_ASSERT(ret == E_HWTIMER_OK, "HWTIMER-STOP collapsed: fired timer 0x%08x not in list 0x%08x",
            (uint32_t)timer, (uint32_t)hwtimer_list.fired);
    } else if (timer->state == HWTIMER_STATE_CALLBACK) {
        // The timer handler is preempted and timer is being handled
#ifdef HWTIMER_DEBUG
        err_stop_in_cb++;
#endif
        ret = E_HWTIMER_IN_CALLBACK;
    } else {
        // Invalid state
        ret = E_HWTIMER_INVAL_ST;
    }

    if (ret == E_HWTIMER_OK) {
        timer->state = HWTIMER_STATE_ALLOC;
        timer->next = NULL;
    }

    SAVE_INFO(HWTIMER_DEBUG_OP_STOP, timer);

_exit:
    int_unlock(lock);

    return ret;
}

int hwtimer_active(HWTIMER_ID id)
{
    struct HWTIMER_T *timer;

    timer = (struct HWTIMER_T *)id;

    if (timer < &hwtimer_list.timer[0] || timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
        return false;
    }

    if (timer->state == HWTIMER_STATE_ACTIVE || timer->state == HWTIMER_STATE_FIRED) {
        return true;
    }

    return false;
}

static int hwtimer_get_index(HWTIMER_ID id)
{
    struct HWTIMER_T *timer;
    int i;

    timer = (struct HWTIMER_T *)id;

    if (timer < &hwtimer_list.timer[0] || timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
        return -1;
    }

    for (i = 0; i < HWTIMER_NUM; i++) {
        if (timer == &hwtimer_list.timer[i]) {
            return i;
        }
    }

    return -1;
}

void hwtimer_dump(void)
{
    int i;
    int idx;
    int cnt;
    bool checked[HWTIMER_NUM];
    char buf[100], *pos;
    const char *end = buf + sizeof(buf);
    struct HWTIMER_T *timer;
    uint32_t lock;
    enum HWTIMER_STATE_T state;

    for (i = 0; i < HWTIMER_NUM; i++) {
        checked[i] = false;
    }

    TRACE(TR_ATTR_NO_TS, "------");
    TRACE(TR_ATTR_NO_TS, "HWTIMER LIST DUMP");

    lock = int_lock();

    for (i = 0; i < 3; i++) {
        pos = buf;
        if (i == 0) {
            pos += format_string(pos, end - pos, "ACTIVE: ");
            timer = hwtimer_list.active;
            state = HWTIMER_STATE_ACTIVE;
        } else if (i == 1) {
            pos += format_string(pos, end - pos, "FIRED : ");
            timer = hwtimer_list.fired;
            state = HWTIMER_STATE_FIRED;
        } else {
            pos += format_string(pos, end - pos, "FREE  : ");
            timer = hwtimer_list.free;
            state = HWTIMER_STATE_FREE;
        }
        cnt = 0;
        while (timer) {
            if (cnt == 8) {
                TRACE(TR_ATTR_NO_TS, "%s", buf);
                pos = buf;
                pos += format_string(pos, end - pos, "        ", timer);
            }
            idx = hwtimer_get_index(timer);
            if (idx == -1) {
                pos += format_string(pos, end - pos, "<NA: %p>", timer);
                break;
            } else if (checked[idx]) {
                pos += format_string(pos, end - pos, "<DUP: %d>", idx);
                break;
            } else if (timer->state != state) {
                pos += format_string(pos, end - pos, "<ST-%d: %d> ", timer->state, idx);
            } else if (state == HWTIMER_STATE_ACTIVE) {
                pos += format_string(pos, end - pos, "%d-%u ", idx, timer->time);
            } else {
                pos += format_string(pos, end - pos, "%d ", idx);
            }
            checked[idx] = true;
            timer = timer->next;
            cnt++;
        }
        TRACE(TR_ATTR_NO_TS, "%s", buf);
    }

    pos = buf;
    pos += format_string(pos, end - pos, "ALLOC : ");
    cnt = 0;
    for (i = 0; i < HWTIMER_NUM; i++) {
        if (!checked[i]) {
            if (cnt == 8) {
                TRACE(TR_ATTR_NO_TS, "%s", buf);
                pos = buf;
                pos += format_string(pos, end - pos, "        ", timer);
            }
            if (hwtimer_list.timer[i].state == HWTIMER_STATE_ALLOC) {
                pos += format_string(pos, end - pos, "%d ", i);
            } else {
                pos += format_string(pos, end - pos, "<ST-%d: %d> ", hwtimer_list.timer[i].state, i);
            }
            cnt++;
        }
    }

    int_unlock(lock);

    TRACE(TR_ATTR_NO_TS, "%s", buf);
    TRACE(TR_ATTR_NO_TS, "------");
    TRACE_OUTPUT_LINEFEED();
}

#ifdef HWTIMER_TEST

#define HWTIMER_TEST_NUM            (HWTIMER_NUM + 2)

static HWTIMER_ID test_id[HWTIMER_TEST_NUM];

static void timer_stop_test(int id)
{
    int ret;

    ret = hwtimer_stop(test_id[id]);
    TRACE(3,"[%u] Stop %d / %d", hal_sys_timer_get(), id, ret);
    hwtimer_dump();
}

static void timer_callback(void *param)
{
    int id;

    id = (int)param;

    TRACE(2,"[%u] TIMER-CALLBACK: %d", hal_sys_timer_get(), id);

    if (id == 3) {
        timer_stop_test(3);
        timer_stop_test(5);
        timer_stop_test(7);
    }
}

void hwtimer_test(void)
{
    int i;
    int ret;
    uint32_t lock;

    hwtimer_init();

    for (i = 0; i < HWTIMER_TEST_NUM; i++) {
        test_id[i] = hwtimer_alloc(timer_callback, (void *)i);
        ret = hwtimer_start(test_id[i], (i + 1) * 10);
        TRACE(4,"[%u] START-TIMER: %u / %p / %d", hal_sys_timer_get(), i, test_id[i], ret);
    }

    hwtimer_dump();

    lock = int_lock();
    hal_sys_timer_delay(55);
    timer_stop_test(0);
    timer_stop_test(2);
    int_unlock(lock);

    hal_sys_timer_delay(300);
    hwtimer_dump();
}

#endif


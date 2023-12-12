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

#ifdef CPUDUMP_BASE

#include "hal_cmu.h"
#include "hal_cpudump.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_location.h"

#define CPUDUMP_EN_REG_OFFSET      0x2000
#define CPUDUMP_DISABLE            (0 << 0)
#define CPUDUMP_ENABLE             (1 << 0)
#define CPUDUMP_CTRL_REG_OFFSET    0x2004
#define CPUDUMP_CTRL_STOP          (1 << 0)
#define CPUDUMP_CTRL_START         (1 << 1)

#define DUMPPC_TOTAL_NUM           256
#define DUMPPC_NUM                 40

#define cpudump_read32(b,a)        (*(volatile uint32_t *)(b+a))
#define cpudump_write32(v,b,a)     ((*(volatile uint32_t *)(b+a)) = v)

#if defined(CHIP_BEST1603) && !(defined(CHIP_SUBSYS_SENS) || defined(CHIP_SUBSYS_BTH))
#define GET_VAL(a)                 (a << 1)
#else
#define GET_VAL(a)                 (a << 0)
#endif

void BOOT_TEXT_FLASH_LOC hal_cpudump_enable(void)
{
    uint32_t val;

    hal_cpudump_clock_enable();
    val = CPUDUMP_ENABLE;
    cpudump_write32(val, CPUDUMP_BASE, CPUDUMP_EN_REG_OFFSET);
    val = CPUDUMP_CTRL_START;
    cpudump_write32(val, CPUDUMP_BASE, CPUDUMP_CTRL_REG_OFFSET);
}

void hal_cpudump_disable(void)
{
    uint32_t val;

    val = CPUDUMP_CTRL_STOP;
    cpudump_write32(val, CPUDUMP_BASE, CPUDUMP_CTRL_REG_OFFSET);
    hal_sys_timer_delay(MS_TO_TICKS(1));
    val = CPUDUMP_DISABLE;
    cpudump_write32(val, CPUDUMP_BASE, CPUDUMP_EN_REG_OFFSET);
}

void hal_cpudump_dump_pc(void)
{
    uint32_t i, j = 0;
    uint32_t print_start_pos = 0;
    uint32_t last_pc_addr = 0;
    uint32_t pc_data[4] = {0};
    volatile uint32_t *maddr = (uint32_t*)CPUDUMP_BASE;

    hal_cpudump_disable();
    last_pc_addr = hal_cmu_cpudump_get_last_addr();
    if (last_pc_addr == 0x100) {
        for (i = 0; i < DUMPPC_TOTAL_NUM; i++){
            if ((maddr[i] & 0x1) == 1) {
                //TRACE(0, "%d, 0x%x, 0x%x", i, i*4, maddr[i]);
                last_pc_addr = i;
                break;
            }
        }
    }

    TRACE(0, "dump pc, last_pc_addr:%d", last_pc_addr);
    if (last_pc_addr >= (DUMPPC_NUM -1)) {
        print_start_pos = last_pc_addr + 1 - DUMPPC_NUM;
        for (j = 0; j < DUMPPC_NUM/4; j++)
            TRACE_IMM(0, "%08x %08x %08x %08x", GET_VAL(maddr[print_start_pos + j*4]), GET_VAL(maddr[print_start_pos + j*4 + 1]),
                GET_VAL(maddr[print_start_pos + j*4 + 2]), GET_VAL(maddr[print_start_pos + j*4 + 3]));
    } else {
        print_start_pos = DUMPPC_TOTAL_NUM - (DUMPPC_NUM - (last_pc_addr + 1));
        for (j = 0; j < DUMPPC_NUM/4; j++) {
            pc_data[0] = ((print_start_pos + j*4) >= DUMPPC_TOTAL_NUM) ? GET_VAL(maddr[print_start_pos + j*4 - DUMPPC_TOTAL_NUM]) : GET_VAL(maddr[print_start_pos + j*4]);
            pc_data[1] = ((print_start_pos + j*4) >= (DUMPPC_TOTAL_NUM -1)) ? GET_VAL(maddr[print_start_pos + j*4 + 1 - DUMPPC_TOTAL_NUM]) : GET_VAL(maddr[print_start_pos + j*4 + 1]);
            pc_data[2] = ((print_start_pos + j*4) >= (DUMPPC_TOTAL_NUM -2)) ? GET_VAL(maddr[print_start_pos + j*4 + 2 - DUMPPC_TOTAL_NUM]) : GET_VAL(maddr[print_start_pos + j*4 + 2]);
            pc_data[3] = ((print_start_pos + j*4) >= (DUMPPC_TOTAL_NUM -3)) ? GET_VAL(maddr[print_start_pos + j*4 + 3 - DUMPPC_TOTAL_NUM]) : GET_VAL(maddr[print_start_pos + j*4 + 3]);
            TRACE_IMM(0, "%08x %08x %08x %08x", pc_data[0], pc_data[1], pc_data[2], pc_data[3]);
        }
    }
}

#endif

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
#include "cmsis.h"
#include "norflash_drv.h"
#include "cmsis.h"
#include "hal_cache.h"
#include "hal_location.h"
#include "hal_norflaship.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "norflash_cfg.h"
#include "norflash_gd25q32c.h"
#include "plat_addr_map.h"
#include "string.h"
#include "tool_msg.h"

#ifdef PROGRAMMER
#include "sys_api_programmer.h"
#include "task_schedule.h"
#else
#define TASK_SCHEDULE                           true
#endif

#if defined(FLASH_DTR) && defined(CHIP_BEST1402)
#error "Not support FLASH_DTR on best1402 yet (be careful with 0x9F command issue)"
#endif
#if !defined(ROM_BUILD) && !defined(PROGRAMMER)
#if (CHIP_FLASH_CTRL_VER >= 2)
#define FLASH_BURST_WRAP
#endif
#endif
#if defined(FLASH_LOCK_MEM_READ) && (CHIP_FLASH_CTRL_VER >= 6) && !defined(PROGRAMMER)
#define FLASH_MEM_READ_BUS_LOCK
#endif

#define WB_UNIQUE_ID_LEN                        8
#define XMC_UNIQUE_ID_LEN                       8
#define ZBIT_UNIQUE_ID_LEN                      16
#define PUYA_UNIQUE_ID_LEN                      16
#define GD_UNIQUE_ID_LEN                        18

#define NORFLASH_UNIQUE_ID_LEN                  18

#define NORFLASH_MAX_DIV                        0xFF

#define NORFLASH_DEFAULT_MAX_SPEED              (104 * 1000 * 1000)

#define NORFLASH_SPEED_RATIO_DENOMINATOR        8

#define XTS_UNIQUE_ID_LEN                       16
#define XTS_UNIQUE_ID_CMD                       0x5A
#define XTS_UNIQUE_ID_PARAM                     0x00019400

// GigaDevice
extern const struct NORFLASH_CFG_T gd25le255e_cfg;
extern const struct NORFLASH_CFG_T gd25le128e_cfg;
extern const struct NORFLASH_CFG_T gd25lf128e_cfg;
extern const struct NORFLASH_CFG_T gd25lq64c_cfg;
extern const struct NORFLASH_CFG_T gd25lf64e_cfg;
extern const struct NORFLASH_CFG_T gd25lq32c_cfg;
extern const struct NORFLASH_CFG_T gd25lq16c_cfg;
extern const struct NORFLASH_CFG_T gd25lq80c_cfg;
extern const struct NORFLASH_CFG_T gd25q32c_cfg;
extern const struct NORFLASH_CFG_T gd25q80c_cfg;
extern const struct NORFLASH_CFG_T gd25d40c_cfg;
extern const struct NORFLASH_CFG_T gd25d20c_cfg;
extern const struct NORFLASH_CFG_T gd25uf128e_cfg;
extern const struct NORFLASH_CFG_T gd25uf64e_cfg;

// Puya
extern const struct NORFLASH_CFG_T p25q256l_cfg;
extern const struct NORFLASH_CFG_T p25q128l_cfg;
extern const struct NORFLASH_CFG_T p25q128h_cfg;
extern const struct NORFLASH_CFG_T p25q64l_cfg;
extern const struct NORFLASH_CFG_T p25q64sn_cfg;
extern const struct NORFLASH_CFG_T p25q32l_cfg;
extern const struct NORFLASH_CFG_T p25q32sn_cfg;
extern const struct NORFLASH_CFG_T p25q16l_cfg;
extern const struct NORFLASH_CFG_T p25q80h_cfg;
extern const struct NORFLASH_CFG_T p25q21h_cfg;
extern const struct NORFLASH_CFG_T p25q40h_cfg;

extern const struct NORFLASH_CFG_T p25q32sl_cfg;
extern const struct NORFLASH_CFG_T p25q16sl_cfg;

// XTS
extern const struct NORFLASH_CFG_T xt25q08b_cfg;

// XMC
extern const struct NORFLASH_CFG_T xm25qu256c_cfg;
extern const struct NORFLASH_CFG_T xm25qu128c_cfg;
extern const struct NORFLASH_CFG_T xm25lu64c_cfg;
extern const struct NORFLASH_CFG_T xm25qh16c_cfg;
extern const struct NORFLASH_CFG_T xm25qh80b_cfg;

// ZBIT
extern const struct NORFLASH_CFG_T zb25vq128b_cfg;
extern const struct NORFLASH_CFG_T zb25lq64a_cfg;

// EON
extern const struct NORFLASH_CFG_T en25s80b_cfg;

// WINBOND
extern const struct NORFLASH_CFG_T w25q32fw_cfg;
extern const struct NORFLASH_CFG_T w25q128jw_cfg;

// SiliconKaiser
extern const struct NORFLASH_CFG_T sk25le064_cfg;
extern const struct NORFLASH_CFG_T sk25le032_cfg;

static const struct NORFLASH_CFG_T * const flash_list[] = {
    // ----------------------
    // GigaDevice
    // ----------------------
#if defined(__NORFLASH_GD25LE255E__) || defined(__NORFLASH_ALL__)
    &gd25le255e_cfg,
#endif
#if defined(__NORFLASH_GD25LE128E__) || defined(__NORFLASH_ALL__)
    &gd25le128e_cfg,
#endif
#if defined(__NORFLASH_GD25LF128E__) || defined(__NORFLASH_ALL__)
    &gd25lf128e_cfg,
#endif
#if defined(__NORFLASH_GD25LQ64C__) || defined(__NORFLASH_ALL__)
    &gd25lq64c_cfg,
#endif
#if defined(__NORFLASH_GD25LF64E__) || defined(__NORFLASH_ALL__)
    &gd25lf64e_cfg,
#endif
#if defined(__NORFLASH_GD25LQ32C__) || defined(__NORFLASH_ALL__)
    &gd25lq32c_cfg,
#endif
#if defined(__NORFLASH_GD25LQ6C__) || defined(__NORFLASH_ALL__)
    &gd25lq16c_cfg,
#endif
#if defined(__NORFLASH_GD25LQ80C__) || defined(__NORFLASH_ALL__)
    &gd25lq80c_cfg,
#endif
#if defined(__NORFLASH_GD25Q32C__) || defined(__NORFLASH_ALL__)
    &gd25q32c_cfg,
#endif
#if defined(__NORFLASH_GD25Q80C__) || defined(__NORFLASH_ALL__)
    &gd25q80c_cfg,
#endif
#if defined(__NORFLASH_GD25D40C__) || defined(__NORFLASH_ALL__)
    &gd25d40c_cfg,
#endif
#if defined(__NORFLASH_GD25D20C__) || defined(__NORFLASH_ALL__)
    &gd25d20c_cfg,
#endif
#if defined(__NORFLASH_GD25UF128E__) || defined(__NORFLASH_ALL__)
    &gd25uf128e_cfg,
#endif
#if defined(__NORFLASH_GD25UF64E__) || defined(__NORFLASH_ALL__)
    &gd25uf64e_cfg,
#endif

    // ----------------------
    // Puya
    // ----------------------
#if defined(__NORFLASH_P25Q256L__) //|| defined(__NORFLASH_ALL__)
    &p25q256l_cfg,
#endif
#if defined(__NORFLASH_P25Q128L__) || defined(__NORFLASH_ALL__)
    &p25q128l_cfg,
#endif
#if defined(__NORFLASH_P25Q128H__) || defined(__NORFLASH_ALL__)
    &p25q128h_cfg,
#endif
#if defined(__NORFLASH_P25Q64L__) || defined(__NORFLASH_ALL__)
    &p25q64l_cfg,
#endif
#if defined(__NORFLASH_P25Q64SN__) || defined(__NORFLASH_ALL__)
    &p25q64sn_cfg,
#endif
#if defined(__NORFLASH_P25Q32L__) || defined(__NORFLASH_ALL__)
    &p25q32l_cfg,
#endif
#if defined(__NORFLASH_P25Q32SN__) || defined(__NORFLASH_ALL__)
    &p25q32sn_cfg,
#endif
#if defined(__NORFLASH_P25Q16L__) || defined(__NORFLASH_ALL__)
    &p25q16l_cfg,
#endif
#if defined(__NORFLASH_P25Q80H__) || defined(__NORFLASH_ALL__)
    &p25q80h_cfg,
#endif
#if defined(__NORFLASH_P25Q40H__) || defined(__NORFLASH_ALL__)
    &p25q40h_cfg,
#endif
#if defined(__NORFLASH_P25Q21H__) || defined(__NORFLASH_ALL__)
    &p25q21h_cfg,
#endif

#if defined(__NORFLASH_P25Q32SL__) || defined(__NORFLASH_ALL__)
    &p25q32sl_cfg,
#endif
#if defined(__NORFLASH_P25Q16SL__) || defined(__NORFLASH_ALL__)
    &p25q16sl_cfg,
#endif

    // ----------------------
    // Zbit
    // ----------------------
#if defined(__NORFLASH_ZB25VQ128B__) //|| defined(__NORFLASH_ALL__)
    &zb25vq128b_cfg,
#endif
#if defined(__NORFLASH_ZB25LQ64A__) || defined(__NORFLASH_ALL__)
    &zb25lq64a_cfg,
#endif

    // ----------------------
    // Xinxin
    // ----------------------
#if defined(__NORFLASH_XM25QU256C__) || defined(__NORFLASH_ALL__)
    &xm25qu256c_cfg,
#endif
#if defined(__NORFLASH_XM25QU128C__) || defined(__NORFLASH_ALL__)
    &xm25qu128c_cfg,
#endif
#if defined(__NORFLASH_XM25LU64C__) || defined(__NORFLASH_ALL__)
    &xm25lu64c_cfg,
#endif
#if defined(__NORFLASH_XM25QH16C__) //|| defined(__NORFLASH_ALL__)
    &xm25qh16c_cfg,
#endif
#if defined(__NORFLASH_XM25QH80B__) || defined(__NORFLASH_ALL__)
    &xm25qh80b_cfg,
#endif

    // ----------------------
    // XTS
    // ----------------------
#if defined(__NORFLASH_XT25Q08B__)
    &xt25q08b_cfg,
#endif

    // ----------------------
    // EON
    // ----------------------
#if defined(__NORFLASH_EN25S80B__)
    &en25s80b_cfg,
#endif

    // ----------------------
    // WINBOND
    // ----------------------
#if defined(__NORFLASH_W25Q32FW__) || defined(__NORFLASH_SIMU__)
    &w25q32fw_cfg,
#endif
#if defined(__NORFLASH_W25Q128JV__) || defined(__NORFLASH_ALL__)
    &w25q128jw_cfg,
#endif

    // ----------------------
    // SiliconKaiser
    // ----------------------
#if defined(__NORFLASH_SK25LE064__) || defined(__NORFLASH_ALL__)
    &sk25le064_cfg,
#endif
#if defined(__NORFLASH_SK25LE032__) || defined(__NORFLASH_ALL__)
    &sk25le032_cfg,
#endif
};

// Sample delay will be larger if:
// 1) flash speed is higher (major impact)
// 2) vcore voltage is lower (secondary major impact)
// 3) flash voltage is lower (minor impact)

// Sample delay unit:
// V1: 1/2 source_clk cycle when <= 2, 1 source_clk cycle when >= 3
// V2(BEST2300/BEST1400): 1/2 source_clk cycle when <= 3, 1 source_clk cycle when >= 4
// V2: 1/2 source_clk cycle when <= 7, 1 source_clk cycle when >= 8

// Flash clock low to output valid delay:
// T_clqv: 7 ns

// Flash IO latency:
// BEST1000/3001/1400: 4 ns
// BEST2000: 5 ns
// BEST2300: 2 ns

// Flash output time: T_clqv + T_io_latency
// Falling edge sample time: one spi_clk cycle (should > flash output time)

#ifdef CHIP_BEST2300
#define PREFERRED_SAMPLE_ADJ_FREQ               (110 * 1000 * 1000) // about 9 ns
#else
#define PREFERRED_SAMPLE_ADJ_FREQ               (77 * 1000 * 1000) // about 13 ns
#endif

#if (CHIP_FLASH_CTRL_VER <= 1)
#define SAM_EDGE_FALLING                        (1 << 4)
#define SAM_NEG_PHASE                           (1 << 5)
#define SAMDLY_MASK                             (0xF << 0)

#define DIV2_SAMP_DELAY_PREFERRED_IDX           2
#define DIVN_SAMP_DELAY_PREFERRED_IDX           3
// Sample delays: 0, 0.5, 1, 1.5, 2, 3, 4, 5, 6, 7
static const uint8_t samdly_list_divn[] = { /*0,*/ SAM_EDGE_FALLING | 1, 1, SAM_NEG_PHASE | SAM_EDGE_FALLING | 2, 2, 3, 4, /*5, 6, 7,*/ };
#else // (CHIP_FLASH_CTRL_VER >= 2)
#define DIV1_SAMP_DELAY_PREFERRED_IDX           1 // prefer 2nd falling edge
// Sample base point: first SCLK rising edge (falling edge + T * 1 / 2)
// Sample delays: 0, 0.5, 1, 1.5
static const uint8_t samdly_list_div1[] = { 0, 1, 2, 3, };
#define DIV2_SAMP_DELAY_PREFERRED_IDX           3 // prefer 2nd falling edge
#define DIVN_SAMP_DELAY_PREFERRED_IDX           5 // prefer 2nd falling edge
#define DIVDTR_SAMP_DELAY_PREFERRED_IDX         1 // prefer 1nd rising edge
// Sample base point: first SCLK rising edge (falling edge + T * IntRoundDown(div / 2))
// Sample delays: -0.5, 0, 0.5, 1, 1.5, 2, 2.5, 3, 4, 5, 6
static const uint8_t samdly_list_divn[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };
#endif

static uint8_t flash_idx[HAL_FLASH_ID_NUM];
static uint8_t div_read[HAL_FLASH_ID_NUM];
static uint8_t div_std_read[HAL_FLASH_ID_NUM];
static uint8_t div_others[HAL_FLASH_ID_NUM];
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
static uint8_t div_dtr_read[HAL_FLASH_ID_NUM];
#endif
static uint32_t norflash_op_mode[HAL_FLASH_ID_NUM];
static bool preferred_idx_adj[HAL_FLASH_ID_NUM];
static uint8_t sample_delay_index[HAL_FLASH_ID_NUM];

#ifdef FLASH_DUAL_CHIP
static bool flash_dual_chip[HAL_FLASH_ID_NUM];
static bool flash_tx_copy[HAL_FLASH_ID_NUM];
#ifdef FLASH_SEC_REG_DUAL_CHIP
static bool sec_reg_dual_chip[HAL_FLASH_ID_NUM];
#endif
#endif

#ifdef FLASH_SUSPEND
static uint32_t check_irq[HAL_FLASH_ID_NUM][(USER_IRQn_QTY + 31) / 32];
static bool specific_irq_check[HAL_FLASH_ID_NUM];
#ifdef FLASH_MEM_READ_BUS_LOCK
static bool flash_read_check[HAL_FLASH_ID_NUM];
#endif
#endif

#ifdef FLASH_MEM_READ_BUS_LOCK
static uint32_t mrb_status[HAL_FLASH_ID_NUM];
#endif

#ifdef FLASH_CALIB_DEBUG
static uint32_t norflash_source_clk[HAL_FLASH_ID_NUM];
static uint32_t norflash_speed[HAL_FLASH_ID_NUM];
static uint8_t calib_matched_idx[HAL_FLASH_ID_NUM][DRV_NORFLASH_CALIB_QTY];
static uint8_t calib_matched_cnt[HAL_FLASH_ID_NUM][DRV_NORFLASH_CALIB_QTY];
static uint8_t calib_final_idx[HAL_FLASH_ID_NUM][DRV_NORFLASH_CALIB_QTY];
#endif

static void norflash_delay(uint32_t us)
{
#ifdef CHIP_BEST1000
    hal_sys_timer_delay(US_TO_TICKS(us));
#else
    hal_sys_timer_delay_us(us);
#endif
}

#ifdef FLASH_HPM
static int norflash_set_hpm(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_cmd_addr(id, GD25Q32C_CMD_HIGH_PERFORMANCE, 0);
    } else {
        norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    }
    norflaship_busy_wait(id);

    return 0;
}
#endif

#ifdef FLASH_SUSPEND
static int norflash_suspend(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_PROGRAM_ERASE_SUSPEND, 0);
    // Suspend time: 20~30 us
    norflash_delay(40);
    return 0;
}

static int norflash_resume(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_PROGRAM_ERASE_RESUME, 0);
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        // PUYA flash requires the mean interval of resume to suspend >= 250us
        norflash_delay(250);
    } else {
        // At least resume the work for 100 us to avoid always staying in suspended state
        norflash_delay(100);
    }
    return 0;
}
#endif

uint8_t norflash_read_status_s0_s7(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    norflash_read_reg(id, GD25Q32C_CMD_READ_STATUS_S0_S7, &val, 1);
    return val;
}

void norflash_dual_chip_read_status_s0_s7(enum HAL_FLASH_ID_T id, uint8_t *val0, uint8_t *val1)
{
    norflash_dual_chip_read_reg_ex(id, GD25Q32C_CMD_READ_STATUS_S0_S7, NULL, 0, val0, val1, 1);
}

uint8_t norflash_read_status_s8_s15(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    norflash_read_reg(id, GD25Q32C_CMD_READ_STATUS_S8_S15, &val, 1);
    return val;
}

void norflash_dual_chip_read_status_s8_s15(enum HAL_FLASH_ID_T id, uint8_t *val0, uint8_t *val1)
{
    norflash_dual_chip_read_reg_ex(id, GD25Q32C_CMD_READ_STATUS_S8_S15, NULL, 0, val0, val1, 1);
}

static int norflash_status_WEL(enum HAL_FLASH_ID_T id)
{
    uint8_t status;
#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        uint8_t status1;
        norflash_dual_chip_read_status_s0_s7(id, &status, &status1);
        status &= status1;
    } else
#endif
    {
        status = norflash_read_status_s0_s7(id);
    }
    return !!(status & GD25Q32C_WEL_BIT_MASK);
}

static int norflash_status_WIP(enum HAL_FLASH_ID_T id)
{
    uint8_t status;
#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        uint8_t status1;
        norflash_dual_chip_read_status_s0_s7(id, &status, &status1);
        status |= status1;
    } else
#endif
    {
        status = norflash_read_status_s0_s7(id);
    }
    return !!(status & GD25Q32C_WIP_BIT_MASK);
}

void norflash_status_WEL_0_wait(enum HAL_FLASH_ID_T id)
{
    while (norflash_status_WEL(id) == 0 && TASK_SCHEDULE);
}

#ifdef FLASH_SUSPEND
int norflash_suspend_check_irq(enum HAL_FLASH_ID_T id, uint32_t irq_num, int enable)
{
    uint32_t idx;
    uint32_t offset;
    uint32_t i;

    idx = irq_num / 32;
    offset = irq_num % 32;

    if (idx >= ARRAY_SIZE(check_irq)) {
        return 1;
    }

    if (enable) {
        check_irq[id][idx] |= (1 << offset);
        specific_irq_check[id] = true;
    } else {
        check_irq[id][idx] &= ~(1 << offset);
        for (i = 0; i < ARRAY_SIZE(check_irq); i++) {
            if (check_irq[id][idx]) {
                break;
            }
        }
        if (i >= ARRAY_SIZE(check_irq)) {
            specific_irq_check[id] = true;
        }
    }

    return 0;
}

int norflash_suspend_check_flash_read(enum HAL_FLASH_ID_T id, int enable)
{
#ifdef FLASH_MEM_READ_BUS_LOCK
    flash_read_check[id] = !!enable;
#endif
    return 0;
}

static int norflash_system_active(enum HAL_FLASH_ID_T id)
{
#ifdef FLASH_MEM_READ_BUS_LOCK
    if (flash_read_check[id]) {
        if (norflaship_mem_read_pending(id)) {
            return true;
        }
    }
#endif
    if (specific_irq_check[id]) {
        return hal_sleep_specific_irq_pending(check_irq[id], ARRAY_SIZE(check_irq[id]));
    } else {
        return hal_sleep_irq_pending();
    }
}
#endif

enum HAL_NORFLASH_RET_T norflash_status_WIP_1_wait(enum HAL_FLASH_ID_T id, int suspend)
{
    while (norflash_status_WIP(id) && TASK_SCHEDULE) {
#ifdef FLASH_SUSPEND
        if (suspend && norflash_system_active(id)) {
            norflash_suspend(id);
            return HAL_NORFLASH_SUSPENDED;
        }
#endif
    }

    return HAL_NORFLASH_OK;
}

#ifdef FLASH_BURST_WRAP
static int norflash_set_burst_wrap(enum HAL_FLASH_ID_T id, uint32_t len)
{
    uint8_t wrap;
    uint32_t flash_blen = len;

#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        flash_blen /= 2;
    }
#endif

    if (flash_blen == 64) {
        wrap = (1 << 6) | (1 << 5);
    } else if (flash_blen == 32) {
        wrap = (1 << 6);
    } else if (flash_blen == 16) {
        wrap = (1 << 5);
    } else if (flash_blen == 8) {
        wrap = 0;
    } else if (flash_blen == 0) {
        // Disable wrap around
        wrap = (1 << 4);
    } else {
        return 1;
    }

#if (CHIP_FLASH_CTRL_VER >= 2)
    if ((flash_list[flash_idx[id]]->total_size > NORFLASH_4BYTE_ADDR_SIZE(id)) &&
            (flash_list[flash_idx[id]]->id[0] == NORFLASH_GD_ID_PREFIX)) {
        // Exit 4-byte-address mode on controller
        norflaship_4byteaddr_mode(id, 0);
    }
#endif

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, &wrap, 1);
    norflaship_cmd_addr(id, GD25Q32C_CMD_SET_BURST_WRAP, 0);
    norflash_status_WIP_1_wait(id, 0);

#if (CHIP_FLASH_CTRL_VER >= 2)
    if ((flash_list[flash_idx[id]]->total_size > NORFLASH_4BYTE_ADDR_SIZE(id)) &&
            (flash_list[flash_idx[id]]->id[0] == NORFLASH_GD_ID_PREFIX)) {
        // Enter 4-byte-address mode on controller
        norflaship_4byteaddr_mode(id, 1);
    }
#endif

    if (len) {
        norflaship_man_wrap_width(id, len);
        norflaship_man_wrap_enable(id);
    } else {
        norflaship_man_wrap_disable(id);
    }
    norflaship_man_mode_enable(id);

    return 0;
}
#endif

static int norflash_set_continuous_read(enum HAL_FLASH_ID_T id, uint8_t on)
{
    uint8_t cmd;

    if (on) {
        norflaship_continuous_read_on(id);
        norflaship_continuous_read_mode_bit(id, flash_list[flash_idx[id]]->crm_en_bits);
        // Continuous Read Mode takes effect after the first read
    } else {
        norflaship_continuous_read_off(id);
        norflaship_continuous_read_mode_bit(id, flash_list[flash_idx[id]]->crm_dis_bits);

        if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
            if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DTR) {
                cmd = GD25Q32C_CMD_FAST_DTR_QUAD_IO_READ;
            } else
#endif
            {
                cmd = GD25Q32C_CMD_FAST_QUAD_IO_READ;
            }
        } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_IO) {
            cmd = GD25Q32C_CMD_FAST_DUAL_IO_READ;
        } else {
            cmd = GD25Q32C_CMD_STANDARD_READ;
        }

        if (cmd != GD25Q32C_CMD_STANDARD_READ) {
            uint8_t len = 2;
#ifdef FLASH_DUAL_CHIP
            if (FLASH_IS_DUAL_CHIP(id)) {
                len *= 2;
            }
#endif
            norflaship_clear_rxfifo(id);
            norflaship_blksize(id, len);
            norflaship_cmd_addr(id, cmd, 0);
            norflaship_clear_rxfifo(id);
        }
    }

    norflaship_busy_wait(id);

    return 0;
}

static int norflash_set_quad(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    if (on) {
        flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_QE, 1);
    } else {
        flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_QE, 0);
    }
    return 0;
}

static int norflash_set_quad_io_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    norflash_set_quad(id, on);
    if (on) {
        norflaship_quad_mode(id, 1);
    } else {
        norflaship_quad_mode(id, 0);
    }
    norflaship_busy_wait(id);
    return 0;
}

static int norflash_set_quad_output_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    norflash_set_quad(id, on);
    if (on) {
        norflaship_rdcmd(id, GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ);
    } else {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t norflash_set_dual_io_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_dual_mode(id, 1);
    } else {
        norflaship_dual_mode(id, 0);
    }
    norflaship_busy_wait(id);

    return 0;
}

static uint8_t norflash_set_dual_output_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_rdcmd(id, GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ);
    } else {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t norflash_set_fast_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_FAST_READ);
    } else {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t norflash_set_stand_mode(enum HAL_FLASH_ID_T id)
{
    norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    norflaship_busy_wait(id);
    return 0;
}

#if defined(FLASH_PAGE_MPM_ENABLE)
static void norflash_set_mpm_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    uint8_t val;
    uint8_t mpm_mode;

    if (on){
        mpm_mode = PUYA_CR_MPM_MODE_2;
    } else {
        mpm_mode = PUYA_CR_MPM_MODE_0;
    }

    norflash_read_reg(id, PUYA_FLASH_CMD_READ_CFGREG, &val, 1);

    val = (val & (~PUYA_CR_MPM_BIT_MASK)) | (mpm_mode << PUYA_CR_MPM_BIT_SHIFT);
    norflash_write_reg(id, false, PUYA_FLASH_CMD_WRITE_CFGREG, &val, 1);
}
#endif

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
static int norflash_set_dtr_quad_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    uint8_t cmd;
    if (on) {
        norflaship_dummy_dtr4rd(id, flash_list[flash_idx[id]]->dtr_quad_cfg.s.dummy_cycles);
        cmd = GD25Q32C_CMD_FAST_DTR_QUAD_IO_READ;
    } else {
        cmd = GD25Q32C_CMD_FAST_QUAD_IO_READ;
    }
    norflaship_qrdcmd(id, cmd);
    norflaship_dtr_mode(id, on);
    norflaship_busy_wait(id);
    return 0;
}
#endif

uint32_t norflash_get_supported_mode(enum HAL_FLASH_ID_T id)
{
     return flash_list[flash_idx[id]]->mode;
}

uint32_t norflash_get_current_mode(enum HAL_FLASH_ID_T id)
{
     return norflash_op_mode[id];
}

union DRV_NORFLASH_SEC_REG_CFG_T norflash_get_security_register_config(enum HAL_FLASH_ID_T id)
{
    return flash_list[flash_idx[id]]->sec_reg_cfg;
}

uint32_t norflash_get_block_protect_mask(enum HAL_FLASH_ID_T id)
{
    return flash_list[flash_idx[id]]->block_protect_mask;
}

void norflash_set_dual_chip_mode(enum HAL_FLASH_ID_T id, int flash, int sec_reg, int tx_copy)
{
#ifdef FLASH_DUAL_CHIP
    if (FLASH_IS_DUAL_CHIP(id)) {
        flash_dual_chip[id] = !!flash;
        norflaship_dual_chip_mode(id, flash_dual_chip[id]);

        flash_tx_copy[id] = (flash && tx_copy);
        norflaship_tx_data_copy(id, flash_tx_copy[id]);

#ifdef FLASH_SEC_REG_DUAL_CHIP
        sec_reg_dual_chip[id] = (flash && sec_reg);
#endif
    }
#endif
}

void norflash_reset(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);

    // Release from deep power-down
#ifdef FLASH_QPI
    norflaship_qpi_mode(id, 1);
    norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    norflaship_busy_wait(id);
    norflaship_qpi_mode(id, 0);
#endif
    norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    // Wait 20us for flash to finish
    norflash_delay(40);

    // Quit from 4-byte-address mode on controller
    norflaship_4byteaddr_mode(id, 0);

    // Quit from quad/dual continuous read mode on controller
#if (CHIP_FLASH_CTRL_VER >= 3)
    norflaship_qrdcmd(id, GD25Q32C_CMD_FAST_QUAD_IO_READ);
    norflaship_dtr_mode(id, 0);
#endif
    norflaship_quad_mode(id, 0);
    norflaship_dual_mode(id, 0);
    norflaship_continuous_read_off(id);
    norflaship_busy_wait(id);

    // Quit from quad/dual continuous read mode on flash die
#ifdef NORFLASH_READ_QUIT_CRM
    uint8_t len;
#if (CHIP_FLASH_CTRL_VER >= 3) && !defined(CHIP_BEST1402)
    // Command 0x9F will make M4=1 (mode bit 4) in DTR quad or dual continuous read mode,
    // and this will disable the mode
    // (NOTE: On best1402 command 0x9F must be sent via extension command mode, not normal
    // command address mode.)
    norflaship_clear_rxfifo(id);
    len = 1;
#ifdef FLASH_DUAL_CHIP
    if (FLASH_IS_DUAL_CHIP(id)) {
        len *= 2;
    }
#endif
    norflaship_blksize(id, len);
    norflaship_cmd_addr(id, GD25Q32C_CMD_ID, 0);
    norflaship_busy_wait(id);
#endif
    // Command 0x03 and address 0xFFFFFE will make M4=1 (mode bit 4)
    // in quad or dual continuous read mode, and this will disable the mode
    norflaship_clear_rxfifo(id);
    len = 2;
#ifdef FLASH_DUAL_CHIP
    if (FLASH_IS_DUAL_CHIP(id)) {
        len *= 2;
    }
#endif
    norflaship_blksize(id, len);
    norflaship_cmd_addr(id, GD25Q32C_CMD_STANDARD_READ, 0xFFFFFE);
    norflaship_busy_wait(id);
#else
    // Output 20 clocks (cmd + data) with IO<3:0> = 0b'1111 to quit continuous read mode
    // (Worst case: dual IO with 4-byte-address mode)
    static const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF, };
    norflaship_clear_fifos(id);
    norflaship_write_txfifo(id, data, sizeof(data));
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_DISABLE_QPI, sizeof(data));
    norflaship_cmd_done(id);
#endif

#ifdef FLASH_QPI
    // Quit from QPI mode
    norflaship_clear_rxfifo(id);
    norflaship_qpi_mode(id, 1);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_DISABLE_QPI, 0);
    norflaship_cmd_done(id);
    norflaship_qpi_mode(id, 0);
#endif

    // Software reset
    norflaship_clear_rxfifo(id);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_ENABLE_RESET, 0);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_RESET, 0);
    // Reset recovery time: 20~60 us
    norflash_delay(200);

    // Quit from 4-byte-address mode on flash die
    norflaship_ext_tx_cmd(id, GD_FLASH_CMD_EXIT_4BYTEADDR, 0);

    norflaship_cmd_done(id);

    // Reset cfg
    flash_idx[id] = 0;
    div_read[id] = 0;
    div_std_read[id] = 0;
    div_others[id] = 0;
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    div_dtr_read[id] = 0;
#endif
    norflash_op_mode[id] = 0;
    preferred_idx_adj[id] = false;
}

int norflash_get_total_size(enum HAL_FLASH_ID_T id, uint32_t *total_size)
{
    uint32_t size;
    uint32_t multiplicand = 1;

#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        multiplicand = 2;
    }
#endif

    if (total_size) {
        size = flash_list[flash_idx[id]]->total_size;
        *total_size = size * multiplicand;
    }

    return 0;
}

int norflash_get_page_size(enum HAL_FLASH_ID_T id, uint32_t *page_size)
{
    uint32_t size;
    uint32_t multiplicand = 1;

#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        multiplicand = 2;
    }
#endif

    if (page_size) {
        size = NORFLASH_PAGE_SIZE;
#if defined(FLASH_PAGE_MPM_ENABLE)
        if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_PAGE_SIZE_1K) {
            size = NORFLASH_PAGE_SIZE_1K;
        }
#endif
        *page_size = size * multiplicand;
    }

    return 0;
}

static void norflash_set_cfg_div(enum HAL_FLASH_ID_T id)
{
    if (div_others[id]) {
        norflaship_div(id, div_others[id]);
    }
}

static void norflash_set_read_div(enum HAL_FLASH_ID_T id)
{
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_STAND_SPI) {
        if (div_std_read[id]) {
            norflaship_div(id, div_std_read[id]);
        }
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DTR) {
        if (div_dtr_read[id]) {
            norflaship_div(id, div_dtr_read[id]);
        }
#endif
    } else {
        if (div_read[id]) {
            norflaship_div(id, div_read[id]);
        }
    }
}

int norflash_set_mode(enum HAL_FLASH_ID_T id, uint32_t op)
{
    uint32_t read_mode = 0;
    uint32_t ext_mode = 0;
    uint32_t program_mode = 0;
    uint32_t self_mode;
    uint32_t mode;

    self_mode = norflash_get_supported_mode(id);
    mode = (self_mode & op);

    if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
        read_mode = HAL_NORFLASH_OP_MODE_QUAD_IO;
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
#if (CHIP_FLASH_CTRL_VER == 3)
        if ((mode & HAL_NORFLASH_OP_MODE_DTR) && div_dtr_read[id] != 2) {
            // For flash controller V3 (1402/2001/2300a), only div=2 can support DTR
            mode &= ~HAL_NORFLASH_OP_MODE_DTR;
        }
#endif
        if (mode & HAL_NORFLASH_OP_MODE_DTR) {
            ext_mode |= HAL_NORFLASH_OP_MODE_DTR;
#if defined(CHIP_BEST1402) || defined(CHIP_BEST2001)
            // ROM cannot recover from DTR continuous read modes
            mode &= ~HAL_NORFLASH_OP_MODE_CONTINUOUS_READ;
#endif
        }
#endif
    } else if (mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
        read_mode = HAL_NORFLASH_OP_MODE_QUAD_OUTPUT;
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
        read_mode = HAL_NORFLASH_OP_MODE_DUAL_IO;
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
        read_mode = HAL_NORFLASH_OP_MODE_DUAL_OUTPUT;
    } else if (mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
        read_mode = HAL_NORFLASH_OP_MODE_FAST_SPI;
    } else if(mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
        read_mode = HAL_NORFLASH_OP_MODE_STAND_SPI;
    } else {
        // Op error
        return  1;
    }

    if (mode & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        if ((read_mode & (HAL_NORFLASH_OP_MODE_QUAD_IO |
                HAL_NORFLASH_OP_MODE_QUAD_OUTPUT)) == 0) {
            return 1;
        }
        program_mode = HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM;
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM) {
        program_mode = HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM;
    } else if (mode & HAL_NORFLASH_OP_MODE_PAGE_PROGRAM) {
        program_mode = HAL_NORFLASH_OP_MODE_PAGE_PROGRAM;
    } else {
        // Op error
        return 1;
    }

#ifdef FLASH_HPM
    if (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
        ext_mode |= HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE;
    }
#endif

    if (mode & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) {
        if (mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
            ext_mode |= HAL_NORFLASH_OP_MODE_CONTINUOUS_READ;
        }
    }

#ifdef FLASH_BURST_WRAP
    if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
        if (mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
            ext_mode |= HAL_NORFLASH_OP_MODE_READ_WRAP;
        }
    }
#endif

#ifdef FLASH_PAGE_MPM_ENABLE
    if (mode & HAL_NORFLASH_OP_MODE_PAGE_SIZE_1K) {
        ext_mode |= HAL_NORFLASH_OP_MODE_PAGE_SIZE_1K;
    }
#endif

#if (CHIP_FLASH_CTRL_VER <= 6) && defined(FLASH_DUAL_CHIP)
    if (flash_dual_chip[id]) {
        // Dual chip only supports quad mode
        if ((read_mode & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) == 0) {
            return 1;
        }
        if ((program_mode & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) == 0) {
            return 1;
        }
    }
#endif

    mode = (read_mode | ext_mode | program_mode);

    if (norflash_op_mode[id] != mode) {
        // Continuous read off if flash supported
        if ((self_mode & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) &&
                (self_mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ)) {
            norflash_set_continuous_read(id, 0);
        }

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
        norflash_set_dtr_quad_mode(id, 0);
#endif

        norflaship_quad_mode(id, 0);

        norflaship_dual_mode(id, 0);

#ifdef FLASH_HPM
        if (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
            // High performance mode on
            norflash_set_hpm(id, 1);
        }
#endif

        if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            // Quad io mode
            norflash_set_quad_io_mode(id, 1);
#if (CHIP_FLASH_CTRL_VER >= 3)
            // Quad io with 8 dummy clock.
            if (flash_list[flash_idx[id]]->mode & HAL_NORFLASH_OP_MODE_QUAD_IO_DUMMY_8) {
                norflaship_dummy_qior(id, 8);
            }
#endif
        } else if (mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
            // Quad output mode
            norflash_set_quad_output_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
            // Dual io mode
            norflash_set_dual_io_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
            // Dual output mode
            norflash_set_dual_output_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
            // Fast mode
            norflash_set_fast_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
            // Standard spi mode
            norflash_set_stand_mode(id);
        }

#ifdef FLASH_BURST_WRAP
        if (self_mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
            if (mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
                norflash_set_burst_wrap(id, 32);
                hal_cache_wrap_enable(HAL_CACHE_ID_I_CACHE);
                hal_cache_wrap_enable(HAL_CACHE_ID_D_CACHE);
            } else {
                norflash_set_burst_wrap(id, 0);
                hal_cache_wrap_disable(HAL_CACHE_ID_I_CACHE);
                hal_cache_wrap_disable(HAL_CACHE_ID_D_CACHE);
            }
        }
#endif

#if defined(FLASH_PAGE_MPM_ENABLE)
        if (self_mode & HAL_NORFLASH_OP_MODE_PAGE_SIZE_1K) {
            norflash_set_mpm_mode(id, !!(mode & HAL_NORFLASH_OP_MODE_PAGE_SIZE_1K));
        }
#endif

#ifdef FLASH_HPM
        if ((self_mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) &&
                (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) == 0) {
            // High performance mode off
            norflash_set_hpm(id, 0);
        }
#endif

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
        if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            // Quad dtr read mode on
            if (mode & HAL_NORFLASH_OP_MODE_DTR) {
                norflash_set_dtr_quad_mode(id, 1);
            }
        }
#endif

        if (mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
            // Continuous read on
            norflash_set_continuous_read(id, 1);
        }

        // Update current mode at last
        norflash_op_mode[id] = mode;
    }

    norflaship_cmd_done(id);

    norflash_set_read_div(id);

    return 0;
}

int norflash_pre_operation_suspend(enum HAL_FLASH_ID_T id, int suspend)
{
#ifdef FLASH_MEM_READ_BUS_LOCK
    mrb_status[id] = norflaship_mem_read_bus_lock(id);

#ifdef FLASH_SUSPEND
    if (suspend && flash_read_check[id] && norflaship_mem_read_pending(id)) {
        return 1;
    }
#endif
#endif

    norflaship_busy_wait(id);

#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id] && !flash_tx_copy[id]) {
        norflaship_tx_data_copy(id, true);
    }
#endif

    norflash_set_cfg_div(id);

    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        norflash_set_continuous_read(id, 0);
    }
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    if ((norflash_op_mode[id] & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) ==
            (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) {
        norflash_set_dtr_quad_mode(id, 0);
    }
#endif

    return 0;
}

int norflash_pre_operation(enum HAL_FLASH_ID_T id)
{
    return norflash_pre_operation_suspend(id, false);
}

int norflash_post_operation_suspend(enum HAL_FLASH_ID_T id, int suspend, int skip_mode_change)
{
#if defined(FLASH_SUSPEND) && defined(FLASH_MEM_READ_BUS_LOCK)
    if (suspend && skip_mode_change) {
        goto _unlock;
    }
#endif

    norflaship_busy_wait(id);

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    if ((norflash_op_mode[id] & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) ==
            (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) {
        norflash_set_dtr_quad_mode(id, 1);
    }
#endif
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        norflash_set_continuous_read(id, 1);
    }

    norflaship_cmd_done(id);

    norflash_set_read_div(id);

#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id] && !flash_tx_copy[id]) {
        norflaship_tx_data_copy(id, false);
    }
#endif

#ifdef FLASH_MEM_READ_BUS_LOCK
_unlock: POSSIBLY_UNUSED;
    norflaship_mem_read_bus_unlock(id, mrb_status[id]);
#endif

    return 0;
}

int norflash_post_operation(enum HAL_FLASH_ID_T id)
{
    return norflash_post_operation_suspend(id, false, false);
}

int norflash_read_reg(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *val, uint32_t len)
{
    int i;

    norflaship_clear_fifos(id);
#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif
#ifdef TRY_EMBEDDED_CMD
    if ((cmd == GD25Q32C_CMD_READ_STATUS_S0_S7 || cmd == GD25Q32C_CMD_READ_STATUS_S8_S15) && len == 1) {
        norflaship_cmd_addr(id, cmd, 0);
    } else {
        // (NOTE: On best1402 command 0x9F must be sent via extension command mode, not normal
        // command address mode.)
#ifndef CHIP_BEST1402
        if (cmd == GD25Q32C_CMD_ID) {
            norflaship_blksize(id, len);
            norflaship_cmd_addr(id, cmd, 0);
        } else
#endif
        {
            norflaship_ext_rx_cmd(id, cmd, 0, len);
        }
    }
#else
    norflaship_ext_rx_cmd(id, cmd, 0, len);
#endif
    norflaship_rxfifo_count_wait(id, len);
    for (i = 0; i < len; i++) {
        val[i] = norflaship_read_rxfifo(id);
    }
#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    return 0;
}

int norflash_read_reg_ex(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *param, uint32_t param_len, uint8_t *val, uint32_t len)
{
    int i;

    norflaship_clear_fifos(id);
#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif
    if (param && param_len > 0) {
        norflaship_write_txfifo(id, param, param_len);
    } else {
        param_len = 0;
    }
    norflaship_ext_rx_cmd(id, cmd, param_len, len);
    for (i = 0; i < len; i++) {
        norflaship_rxfifo_empty_wait(id);
        val[i] = norflaship_read_rxfifo(id);
    }
#ifdef FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    return 0;
}

int norflash_dual_chip_read_reg_ex(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *param, uint32_t param_len, uint8_t *val0, uint8_t *val1, uint32_t len)
{
    int i;
    int j;
    int index = 0;
    uint8_t buf_len;

    norflaship_clear_fifos(id);

#if (CHIP_FLASH_CTRL_VER <= 6)
    uint8_t buf[8];

    buf_len = len * 8;
#else
    uint8_t buf[2];

    buf_len = len * 2;
#endif

    if (param && param_len > 0) {
        norflaship_write_txfifo(id, param, param_len);
    } else {
        param_len = 0;
    }
#if (CHIP_FLASH_CTRL_VER <= 6)
    norflaship_ext_rx_cmd_rx_4x(id, cmd, param_len, buf_len);
#else
    norflaship_ext_rx_cmd(id, cmd, param_len, buf_len);
#endif
    j = 0;
    for (i = 0; i < buf_len; i++) {
        norflaship_rxfifo_empty_wait(id);
        buf[j++] = norflaship_read_rxfifo(id);
#if (CHIP_FLASH_CTRL_VER <= 6)
        if (j == 8) {
            uint8_t v0 = 0;
            uint8_t v1 = 0;
            uint8_t k;
            for (j = 0; j < 8; j++) {
                k = 8 - 1 - j;
                v0 |= ((buf[k] >> 1) & 1) << j;
                v1 |= ((buf[k] >> 5) & 1) << j;
            }
            val0[index] = v0;
            val1[index] = v1;
            index++;
            j = 0;
        }
#else
        if (j == 2) {
            val0[index] = ((buf[0] & 0xF) << 4) | (buf[1] & 0xF);
            buf[0] >>= 4;
            buf[1] >>= 4;
            val1[index] = ((buf[0] & 0xF) << 4) | (buf[1] & 0xF);
            index++;
            j = 0;
        }
#endif
    }

    return 0;
}

int norflash_write_reg(enum HAL_FLASH_ID_T id, bool volatile_reg, uint8_t cmd, const uint8_t *val, uint32_t len)
{
    if (volatile_reg) {
        norflaship_ext_tx_cmd(id, GD25Q32C_CMD_ENBALE_VOLATILE_STATUS, 0);
    } else {
        norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, 0);
    }

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, val, len);

#ifdef TRY_EMBEDDED_CMD
    if (cmd == GD25Q32C_CMD_WRITE_STATUS_S0_S7) {
        norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_STATUS_S0_S7, 0);
    } else {
        norflaship_ext_tx_cmd(id, cmd, len);
    }
#else
    norflaship_ext_tx_cmd(id, cmd, len);
#endif

    norflash_status_WIP_1_wait(id, 0);

    return 0;
}

static int norflash_get_id_internal(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    if (len > NORFLASH_ID_LEN) {
        len = NORFLASH_ID_LEN;
    }
    norflash_read_reg(id, GD25Q32C_CMD_ID, value, len);

    return 0;
}

int norflash_get_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    norflash_pre_operation(id);

    norflash_get_id_internal(id, value, len);

    norflash_post_operation(id);

    return 0;
}

static int norflash_dual_chip_get_id_internal(enum HAL_FLASH_ID_T id, uint8_t *value0, uint8_t *value1, uint32_t len)
{
    if (len > NORFLASH_ID_LEN) {
        len = NORFLASH_ID_LEN;
    }
    norflash_dual_chip_read_reg_ex(id, GD25Q32C_CMD_ID, NULL, 0, value0, value1, len);

    return 0;
}

int norflash_dual_chip_get_id(enum HAL_FLASH_ID_T id, uint8_t *value0, uint8_t *value1, uint32_t len)
{
    norflash_pre_operation(id);

    norflash_dual_chip_get_id_internal(id, value0, value1, len);

    norflash_post_operation(id);

    return 0;
}

void norflash_get_unique_id_cmd(enum HAL_FLASH_ID_T id, uint8_t *cmd, uint32_t *param, uint32_t *max_len)
{
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_XTS_ID_PREFIX) {
        *max_len = XTS_UNIQUE_ID_LEN;
        *param = XTS_UNIQUE_ID_PARAM;
        *cmd = XTS_UNIQUE_ID_CMD;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_XMC_ID_PREFIX) {
        *max_len = XMC_UNIQUE_ID_LEN;
        *param = 0;
        *cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_ZBIT_ID_PREFIX) {
        *max_len = ZBIT_UNIQUE_ID_LEN;
        *param = 0;
        *cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_WB_ID_PREFIX) {
        *max_len = WB_UNIQUE_ID_LEN;
        *param = 0;
        *cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_GD_ID_PREFIX) {
        *max_len = GD_UNIQUE_ID_LEN;
        *param = 0;
        *cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        *max_len = PUYA_UNIQUE_ID_LEN;
        *param = 0;
        *cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else {
        *max_len = NORFLASH_UNIQUE_ID_LEN;
        *param = 0;
        *cmd = GD25Q32C_CMD_UNIQUE_ID;
    }
}

int norflash_get_unique_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    uint32_t param;
    uint8_t cmd;
    uint32_t max_len;

    norflash_get_unique_id_cmd(id, &cmd, &param, &max_len);
    if (len > max_len) {
        len = max_len;
    }

    norflash_pre_operation(id);

    // Assume 3-byte-address mode with 1 dummy byte
    norflash_read_reg_ex(id, cmd, (uint8_t *)&param, sizeof(param), value, len);

    norflash_post_operation(id);

    return 0;
}

int norflash_dual_chip_get_unique_id(enum HAL_FLASH_ID_T id, uint8_t *value0, uint8_t *value1, uint32_t len)
{
    uint32_t param;
    uint8_t cmd;
    uint32_t max_len;

    norflash_get_unique_id_cmd(id, &cmd, &param, &max_len);
    if (len > max_len) {
        len = max_len;
    }

    norflash_pre_operation(id);

    // Assume 3-byte-address mode with 1 dummy byte
    norflash_dual_chip_read_reg_ex(id, cmd, (uint8_t *)&param, sizeof(param), value0, value1, len);

    norflash_post_operation(id);

    return 0;
}

void norflash_enter_4byteaddr_mode(enum HAL_FLASH_ID_T id)
{
    // NOTE: norflash_pre_operation(id) must have been called

    // Enter 4-byte-address mode on flash die
    norflaship_clear_txfifo(id);
    norflaship_ext_tx_cmd(id, GD_FLASH_CMD_ENTER_4BYTEADDR, 0);
    norflaship_cmd_done(id);

    // Enter 4-byte-address mode on controller
    norflaship_4byteaddr_mode(id, 1);
}

void norflash_exit_4byteaddr_mode(enum HAL_FLASH_ID_T id)
{
    // NOTE: norflash_pre_operation(id) must have been called

    // Exit 4-byte-address mode on flash die
    norflaship_clear_txfifo(id);
    norflaship_ext_tx_cmd(id, GD_FLASH_CMD_EXIT_4BYTEADDR, 0);
    norflaship_cmd_done(id);

    // Exit 4-byte-address mode on controller
    norflaship_4byteaddr_mode(id, 0);
}

static void norflash_get_samdly_list(uint32_t div, const uint8_t **samdly_list_p, uint32_t *size_p)
{
    const uint8_t *samdly_list = NULL;
    uint32_t size = 0;

#if (CHIP_FLASH_CTRL_VER <= 1)
    if (div >= 2) {
        samdly_list = samdly_list_divn;
        size = ARRAY_SIZE(samdly_list_divn);
    }
#else
    if (div >= 1) {
        if (div == 1) {
            samdly_list = samdly_list_div1;
            size = ARRAY_SIZE(samdly_list_div1);
        } else {
            samdly_list = samdly_list_divn;
            size = ARRAY_SIZE(samdly_list_divn);
        }
    }
#endif

    if (samdly_list_p) {
        *samdly_list_p = samdly_list;
    }
    if (size_p) {
        *size_p = size;
    }
}

void norflash_set_sample_delay_index(enum HAL_FLASH_ID_T id, uint32_t index)
{
    const uint8_t *samdly_list;
    uint32_t size;
    uint32_t div;

    sample_delay_index[id] = index;

    div = norflaship_get_div(id);

    norflash_get_samdly_list(div, &samdly_list, &size);

    if (index < size) {
#if (CHIP_FLASH_CTRL_VER <= 1)
        norflaship_pos_neg(id, samdly_list[index] & SAM_EDGE_FALLING);
        norflaship_neg_phase(id, samdly_list[index] & SAM_NEG_PHASE);
        norflaship_samdly(id, samdly_list[index] & SAMDLY_MASK);
#else
        norflaship_samdly(id, samdly_list[index]);
#endif
    }
}

uint32_t norflash_get_sample_delay_index(enum HAL_FLASH_ID_T id)
{
    return sample_delay_index[id];
}

static bool norflash_calib_flash_id_valid(enum HAL_FLASH_ID_T id)
{
    uint8_t dev_id[HAL_NORFLASH_DEVICE_ID_LEN];
    const uint8_t *cmp_id;

    cmp_id = flash_list[flash_idx[id]]->id;

#if 0 //def FLASH_DUAL_CHIP
    if (flash_dual_chip[id]) {
        uint8_t dev_id1[HAL_NORFLASH_DEVICE_ID_LEN];

        norflash_dual_chip_get_id_internal(id, dev_id, dev_id1, sizeof(dev_id));

        if (dev_id1[0] != cmp_id[0] || dev_id1[1] != cmp_id[1] || dev_id1[2] != cmp_id[2]) {
            return false;
        }
    } else
#endif
    {
        norflash_get_id_internal(id, dev_id, sizeof(dev_id));
    }

    if (dev_id[0] != cmp_id[0] || dev_id[1] != cmp_id[1] || dev_id[2] != cmp_id[2]) {
        return false;
    }
    return true;
}

static bool norflash_calib_magic_word_valid(enum HAL_FLASH_ID_T id)
{
    uint32_t magic;
    uint32_t addr;

    if (0) {
    } else if (id == HAL_FLASH_ID_0) {
#if defined(ROM_BUILD) || (defined(PROGRAMMER) && !defined(PROGRAMMER_INFLASH)) || defined(NANDFLASH_BUILD)
        addr = FLASH_NC_BASE;
#else
        extern uint32_t __flash_start[];
        addr = FLASH_C_TO_NC((uint32_t)__flash_start);
#endif
#ifdef FLASH1_CTRL_BASE
    } else if (id == HAL_FLASH_ID_1) {
        addr = FLASH1_NC_BASE;
#endif
#ifdef FLASH2_CTRL_BASE
    } else if (id == HAL_FLASH_ID_2) {
        addr = FLASH2_NC_BASE;
#endif
    } else {
        ASSERT(false, "%s: Bad flash id: %d", __func__, id);
    }

#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(id, FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
#if (FLASH_NC_BASE == FLASH_BASE)
    hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, addr, sizeof(magic));
#endif
    magic = *(volatile uint32_t *)addr;

    if (magic == BOOT_MAGIC_NUMBER) {
        return true;
    }
    return false;
}

#if !(defined(ROM_BUILD) || (defined(PROGRAMMER) && !defined(PROGRAMMER_INFLASH)) || defined(NANDFLASH_BUILD))
static bool norflash_calib_seq_pattern_valid(enum HAL_FLASH_ID_T id)
{
#define CALIB_SEQ_PATTERN                   \
    { \
        /* QSPI */ \
        0xF0F0F0F0, 0xA5A5A5A5, 0x00FF00FF, 0x55AA55AA, \
        /* OSPI */ \
        0xFF00FF00, 0xAA55AA55, 0xFF00FF00, 0xAA55AA55, \
        0xFFFF0000, 0xFFFF0000, 0xAAAA5555, 0xAAAA5555, \
    }

    FLASH_RODATA_LOC static const uint32_t flash_pattern[] = CALIB_SEQ_PATTERN;
    static const uint32_t seq_pattern[sizeof(flash_pattern)] = CALIB_SEQ_PATTERN;
    uint32_t addr;
    uint32_t i;

    if (0) {
    } else if (id == HAL_FLASH_ID_0) {
        addr = FLASH_C_TO_NC((uint32_t)flash_pattern);
#if defined(FLASH1_CTRL_BASE) && defined(FLASH1_CALIB_SEQ_ADDR)
    } else if (id == HAL_FLASH_ID_1) {
        addr = FLASH1_CALIB_SEQ_ADDR;
#endif
#if defined(FLASH2_CTRL_BASE) && defined(FLASH2_CALIB_SEQ_ADDR)
    } else if (id == HAL_FLASH_ID_2) {
        addr = FLASH2_CALIB_SEQ_ADDR;
#endif
    } else {
        ASSERT(false, "%s: Bad flash id: %d", __func__, id);
    }

#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(id, FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
#if (FLASH_NC_BASE == FLASH_BASE) || defined(PROGRAMMER_INFLASH)
    hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, addr, sizeof(flash_pattern));
#endif
    for (i = 0; i < ARRAY_SIZE(flash_pattern); i++) {
        if (((volatile uint32_t *)addr)[i] != seq_pattern[i]) {
            return false;
        }
    }

    return true;
}
#endif

int norflash_sample_delay_calib(enum HAL_FLASH_ID_T id, enum DRV_NORFLASH_CALIB_T type)
{
    int i;
    uint32_t matched_cnt = 0;
    uint32_t matched_idx = 0;
    uint32_t preferred_idx = -1;
    uint32_t div;
    uint32_t size;
    bool valid = false;

    if (type >= DRV_NORFLASH_CALIB_QTY) {
        return 1;
    }

#ifdef FLASH_SAMP_DELAY_INDEX
    preferred_idx = FLASH_SAMP_DELAY_INDEX;
    valid = true;
#elif 0 //(CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DTR) {
        preferred_idx = DIVDTR_SAMP_DELAY_PREFERRED_IDX;
        valid = true;
    }
#endif

    if (valid) {
        matched_idx = preferred_idx;
        norflash_set_sample_delay_index(id, matched_idx);
#ifdef FLASH_CALIB_DEBUG
        calib_final_idx[id][type] = matched_idx;
#endif
        return 0;
    }

    div = norflaship_get_div(id);

    if (div == 0) {
        return -1;
#if (CHIP_FLASH_CTRL_VER <= 1)
    } else if (div == 1) {
        return -2;
#endif
    }

    norflash_get_samdly_list(div, NULL, &size);

    for (i = 0; i < size; i++) {
        norflaship_busy_wait(id);

        norflash_set_sample_delay_index(id, i);

        if (type == DRV_NORFLASH_CALIB_FLASH_ID) {
            valid = norflash_calib_flash_id_valid(id);
        } else if (type == DRV_NORFLASH_CALIB_MAGIC_WORD) {
            valid = norflash_calib_magic_word_valid(id);
#if !(defined(ROM_BUILD) || (defined(PROGRAMMER) && !defined(PROGRAMMER_INFLASH)) || defined(NANDFLASH_BUILD))
        } else if (type == DRV_NORFLASH_CALIB_SEQ_PATTERN) {
            valid = norflash_calib_seq_pattern_valid(id);
#endif
        } else {
            return -3;
        }

        if (valid) {
            if (matched_cnt == 0) {
                matched_idx = i;
            }
            matched_cnt++;
        } else if (matched_cnt) {
            break;
        }
    }

#ifdef FLASH_CALIB_DEBUG
    calib_matched_idx[id][type] = matched_idx;
    calib_matched_cnt[id][type] = matched_cnt;
#endif

    if (matched_cnt) {
        if (matched_cnt == 2) {
            if (0) {
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
            } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DTR) {
                preferred_idx = DIVDTR_SAMP_DELAY_PREFERRED_IDX;
#endif
#if (CHIP_FLASH_CTRL_VER >= 2)
            } else if (div == 1) {
                preferred_idx = DIV1_SAMP_DELAY_PREFERRED_IDX;
#endif
            } else if (div == 2) {
                preferred_idx = DIV2_SAMP_DELAY_PREFERRED_IDX;
                if (preferred_idx_adj[id]) {
                    preferred_idx++;
                }
            } else {
                preferred_idx = DIVN_SAMP_DELAY_PREFERRED_IDX;
            }
            if (matched_idx <= preferred_idx &&
                    preferred_idx < matched_idx + matched_cnt) {
                matched_idx = preferred_idx;
                matched_cnt = 1;
            }
        }
        matched_idx += matched_cnt / 2;
        norflash_set_sample_delay_index(id, matched_idx);

#ifdef FLASH_CALIB_DEBUG
        calib_final_idx[id][type] = matched_idx;
#endif

        return 0;
    }

#ifdef FLASH_CALIB_DEBUG
    calib_final_idx[id][type] = -1;
#endif

    return 1;
}

void norflash_show_calib_result(enum HAL_FLASH_ID_T id)
{
#ifdef FLASH_CALIB_DEBUG
    union DRV_NORFLASH_SPEED_RATIO_T ratio;
    uint32_t div;
    uint32_t size;
    const uint8_t *list;
    int i;
#ifdef PROGRAMMER
    unsigned char buf[16];
#endif

    TR_INFO(0, "FLASH_CALIB_RESULT:");
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<FREQ>\nsource_clk=%u speed=%u flash_max=%u",
        norflash_source_clk[id], norflash_speed[id], flash_list[flash_idx[id]]->max_speed);
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &norflash_source_clk[id], sizeof(norflash_source_clk[id]));
    memcpy(buf + 4, &norflash_speed[id], sizeof(norflash_speed[id]));
    memcpy(buf + 8, &flash_list[flash_idx[id]]->max_speed, sizeof(flash_list[flash_idx[id]]->max_speed));
    send_debug_event(buf, 12);
#endif

    ratio = flash_list[flash_idx[id]]->speed_ratio;
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<RATIO>\nstd_read=%u/8 others=%u/8 dtr_read=%u/8",
        (ratio.s.std_read + 1), (ratio.s.others + 1), (flash_list[flash_idx[id]]->dtr_quad_cfg.s.speed_ratio + 1));
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    buf[0] = (ratio.s.std_read + 1);
    buf[1] = (ratio.s.others + 1);
    buf[2] = (flash_list[flash_idx[id]]->dtr_quad_cfg.s.speed_ratio + 1);
    send_debug_event(buf, 3);
#endif

    div = norflaship_get_div(id);
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<DIV>\ndiv=%u", div);
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "div_read=%u div_std_read=%u div_others=%u",
        div_read[id], div_std_read[id], div_others[id]);
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    buf[0] = div;
    buf[1] = div_read[id];
    buf[2] = div_std_read[id];
    buf[3] = div_others[id];
    send_debug_event(buf, 4);
#endif

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "div_dtr_read=%u", div_dtr_read[id]);
#ifdef PROGRAMMER
    buf[0] = div_dtr_read[id];
    send_debug_event(buf, 1);
#endif
#endif

    norflash_get_samdly_list(div, &list, &size);
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<SAMDLY LIST>");
    if (list == NULL || size == 0) {
        TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "NONE");
    } else {
        DUMP8("%02X ", list, size);
#ifdef PROGRAMMER
        send_debug_event(list, size);
#endif
    }
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<CALIB RESULT>");
    for (i = 0; i < DRV_NORFLASH_CALIB_QTY; i++) {
        TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "type=%d idx=%2d cnt=%2d final=%2d",
            i, (int8_t)calib_matched_idx[id][i], (int8_t)calib_matched_cnt[id][i], (int8_t)calib_final_idx[id][i]);
#ifdef PROGRAMMER
        memset(buf, 0, sizeof(buf));
        buf[0] = i;
        buf[1] = calib_matched_idx[id][i];
        buf[2] = calib_matched_cnt[id][i];
        buf[3] = calib_final_idx[id][i];
        send_debug_event(buf, 4);
#endif
    }
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "\t");
#endif
}

int norflash_init_sample_delay_by_div(enum HAL_FLASH_ID_T id, uint32_t div)
{
    if (div == 0) {
        return -1;
    } if (div == 1) {
#if (CHIP_FLASH_CTRL_VER <= 1)
        return -2;
#else
        norflaship_samdly(id, 1);
#endif
    } else if (div == 2 && !preferred_idx_adj[id]) {
        // Set sample delay to clock falling edge
#if (CHIP_FLASH_CTRL_VER <= 1)
        norflaship_pos_neg(id, 1);
        norflaship_neg_phase(id, 1);
        norflaship_samdly(id, 2);
#else
        norflaship_samdly(id, 3);
#endif
    } else {
        // Set sample delay to nearest to but not later than clock falling edge
#if (CHIP_FLASH_CTRL_VER <= 1)
        norflaship_pos_neg(id, 0);
        norflaship_neg_phase(id, 0);
        norflaship_samdly(id, 2);
#else
        norflaship_samdly(id, 4);
#endif
    }

    return 0;
}

int norflash_init_div(enum HAL_FLASH_ID_T id, const struct HAL_NORFLASH_CONFIG_T *cfg)
{
    uint32_t max_speed;
    uint32_t read_speed;
    uint32_t std_read_speed;
    uint32_t others_speed;
    union DRV_NORFLASH_SPEED_RATIO_T ratio;
    uint32_t div;

#ifdef FLASH_CALIB_DEBUG
    norflash_source_clk[id] = cfg->source_clk;
    norflash_speed[id] = cfg->speed;
    for (int i = 0; i < DRV_NORFLASH_CALIB_QTY; i++) {
        calib_matched_idx[id][i] = -1;
        calib_matched_cnt[id][i] = -1;
        calib_final_idx[id][i] = -1;
    }
#endif

    max_speed = flash_list[flash_idx[id]]->max_speed;
    if (max_speed == 0) {
        max_speed = NORFLASH_DEFAULT_MAX_SPEED;
    }

    ratio = flash_list[flash_idx[id]]->speed_ratio;

    read_speed = max_speed;
    if (read_speed > cfg->speed) {
        read_speed = cfg->speed;
    }
    if (read_speed > cfg->source_clk) {
        read_speed = cfg->source_clk;
    }
    std_read_speed = max_speed * (1 + ratio.s.std_read) / NORFLASH_SPEED_RATIO_DENOMINATOR;
    if (std_read_speed > read_speed) {
        std_read_speed = read_speed;
    }
    others_speed = max_speed * (1 + ratio.s.others) / NORFLASH_SPEED_RATIO_DENOMINATOR;
    if (others_speed > read_speed) {
        others_speed = read_speed;
    }

    div = (cfg->source_clk + read_speed - 1) / read_speed;
    div_read[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
    div = (cfg->source_clk + std_read_speed - 1) / std_read_speed;
    div_std_read[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
    div = (cfg->source_clk + others_speed - 1) / others_speed;
    div_others[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    uint32_t dtr_speed;

    dtr_speed = max_speed * (1 + flash_list[flash_idx[id]]->dtr_quad_cfg.s.speed_ratio) / NORFLASH_SPEED_RATIO_DENOMINATOR;
    if (dtr_speed > read_speed) {
        dtr_speed = read_speed;
    }
    div = (cfg->source_clk + dtr_speed - 1) / dtr_speed;
    if (div > NORFLASH_MAX_DIV) {
        div = NORFLASH_MAX_DIV;
    }
    // Min div is 2
    if (div < 2) {
        div = 2;
    }
    // Div must be an even value
    div = (div + 1) & ~1;
    div_dtr_read[id] = div;
#endif

    if (div_read[id] == 2 && read_speed >= PREFERRED_SAMPLE_ADJ_FREQ) {
        preferred_idx_adj[id] = true;
    } else {
        preferred_idx_adj[id] = false;
    }

    if (div_read[id] && div_std_read[id] && div_others[id]) {
#if (CHIP_FLASH_CTRL_VER <= 1)
        if (div_read[id] == 1) {
            return -1;
        }
#endif
        // Init sample delay according to div_read[id]
        norflash_init_sample_delay_by_div(id, div_read[id]);
        // Still in command mode
        norflaship_div(id, div_others[id]);
        return 0;
    }

    return 1;
}

int norflash_match_chip(enum HAL_FLASH_ID_T id, const uint8_t *dev_id, uint32_t len)
{
    const uint8_t *cmp_id;

    if (len == NORFLASH_ID_LEN) {
        for (flash_idx[id] = 0; flash_idx[id] < ARRAY_SIZE(flash_list); flash_idx[id]++) {
            cmp_id = flash_list[flash_idx[id]]->id;
            if (dev_id[0] == cmp_id[0] && dev_id[1] == cmp_id[1] && dev_id[2] == cmp_id[2]) {
                return true;
            }
        }
    }

    return false;
}

void norflash_get_flash_list(const struct NORFLASH_CFG_T **list, uint32_t *len)
{
    *list = (struct NORFLASH_CFG_T *)&flash_list[0];
    *len = ARRAY_SIZE(flash_list);
}

#if defined(CONFIG_SMP)
extern void nuttx_smp_end_protection();
extern void nuttx_smp_start_protection();
#endif

#if (CHIP_FLASH_CTRL_VER >= 2) && defined(PUYA_FLASH_ERASE_PAGE_ENABLE)
static void norflaship_ext_cmd_addr(enum HAL_FLASH_ID_T id, uint8_t cmd, uint32_t addr, uint32_t tx_len, int addr_4byte)
{
    uint8_t buf[4];
    uint32_t offset;
    uint32_t addr_len;

    if (addr_4byte) {
        offset = 0;
        addr_len = 4;
    } else {
        offset = 1;
        addr_len = 3;
    }

    buf[3] = (uint8_t)(addr & 0xff);
    buf[2] = (uint8_t)((addr >> 8) & 0xff);
    buf[1] = (uint8_t)((addr >> 16) & 0xff);
    buf[0] = (uint8_t)((addr >> 24) & 0xff);

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo_cont(id, buf + offset, addr_len);
    norflaship_ext_tx_cmd(id, cmd, addr_len + tx_len);
}
#endif

enum HAL_NORFLASH_RET_T norflash_erase(enum HAL_FLASH_ID_T id, uint32_t start_address, enum DRV_NORFLASH_ERASE_T type, int suspend)
{
    enum HAL_NORFLASH_RET_T ret;
    uint8_t cmd;

    if (0) {
#if (CHIP_FLASH_CTRL_VER >= 2) && defined(PUYA_FLASH_ERASE_PAGE_ENABLE)
    } else if (type == DRV_NORFLASH_ERASE_PAGE) {
        cmd = PUYA_FLASH_CMD_PAGE_ERASE;
#endif
    } else if (type == DRV_NORFLASH_ERASE_SECTOR) {
        cmd = GD25Q32C_CMD_SECTOR_ERASE;
    } else if (type == DRV_NORFLASH_ERASE_BLOCK_32K) {
        cmd = GD25Q32C_CMD_BLOCK_ERASE_32K;
    } else if (type == DRV_NORFLASH_ERASE_BLOCK_64K) {
        cmd = GD25Q32C_CMD_BLOCK_ERASE_64K;
    } else if (type == DRV_NORFLASH_ERASE_CHIP) {
        cmd = GD25Q32C_CMD_CHIP_ERASE;
    } else {
        return HAL_NORFLASH_BAD_ERASE_TYPE;
    }

    if (flash_list[flash_idx[id]]->mode & HAL_NORFLASH_OP_MODE_ERASE_IN_STD) {
        if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            norflash_set_quad_io_mode(id, 0);
        } else if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
            norflash_set_quad_output_mode(id, 0);
        }
    }

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, 0);
    // Need 1us. Or norflash_status_WEL_0_wait(id), which needs 6us.

    if (0) {
#if (CHIP_FLASH_CTRL_VER >= 2) && defined(PUYA_FLASH_ERASE_PAGE_ENABLE)
    } else if (type == DRV_NORFLASH_ERASE_PAGE) {
        bool addr_4byte = false;

#ifdef FLASH_DUAL_CHIP
        if (flash_dual_chip[id]) {
            start_address /= 2;
        }
#endif
        if (flash_list[flash_idx[id]]->total_size > NORFLASH_4BYTE_ADDR_SIZE(id)) {
            addr_4byte = true;
        }
        norflaship_ext_cmd_addr(id, cmd, start_address, 0, addr_4byte);
#endif
    } else {
#if (CHIP_FLASH_CTRL_VER >= 2)
        norflaship_ext_addr(id, start_address);
#endif
        norflaship_cmd_addr(id, cmd, start_address);
    }

    norflaship_busy_wait(id);

#ifdef FLASH_SUSPEND
    // PUYA flash requires the first delay of erase >= 400us
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        norflash_delay(400);
    }
#endif

    ret = norflash_status_WIP_1_wait(id, suspend);

    if (flash_list[flash_idx[id]]->mode & HAL_NORFLASH_OP_MODE_ERASE_IN_STD) {
        if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            norflash_set_quad_io_mode(id, 1);
        } else if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
            norflash_set_quad_output_mode(id, 1);
        }
    }

    norflaship_cmd_done(id);

    return ret;
}

enum HAL_NORFLASH_RET_T norflash_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len, int suspend)
{
    enum HAL_NORFLASH_RET_T ret;
    uint32_t POSSIBLY_UNUSED remains;
    uint8_t cmd;
    uint32_t page_size;

    norflash_get_page_size(id, &page_size);

    if (len > page_size) {
        return HAL_NORFLASH_ERR;
    }

    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        cmd = GD25Q32C_CMD_QUAD_PAGE_PROGRAM;
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM) {
        cmd = GD25Q32C_CMD_DUAL_PAGE_PROGRAM;
    } else {
        cmd = GD25Q32C_CMD_PAGE_PROGRAM;
    }

    norflaship_clear_txfifo(id);

    remains = norflaship_write_txfifo(id, buffer, len);

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, 0);

#if (CHIP_FLASH_CTRL_VER >= 2)
    norflaship_ext_addr(id, start_address);
#endif
    norflaship_cmd_addr(id, cmd, start_address);

#if (CHIP_FLASH_CTRL_VER >= 2)
    while (remains > 0) {
        buffer += len - remains;
        len = remains;
        remains = norflaship_write_txfifo_cont(id, buffer, len);
    }
#endif

    norflaship_busy_wait(id);

#ifdef FLASH_SUSPEND
    // PUYA flash requires the first delay of byte program >= 450us
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        uint32_t page_size;

        norflash_get_page_size(id, &page_size);
        if (len < page_size) {
            norflash_delay(450);
        }
    }
#endif

    ret = norflash_status_WIP_1_wait(id, suspend);

    norflaship_cmd_done(id);

    return ret;
}

#ifdef FLASH_SUSPEND
enum HAL_NORFLASH_RET_T norflash_erase_resume(enum HAL_FLASH_ID_T id, int suspend)
{
    // TODO: Need to check SUS1 bit in status reg?

    enum HAL_NORFLASH_RET_T ret;
#if defined(CONFIG_SMP)
    nuttx_smp_start_protection(id);
#endif
    norflash_pre_operation(id);

    norflash_resume(id);

    ret = norflash_status_WIP_1_wait(id, suspend);

    norflash_post_operation(id);
#if defined(CONFIG_SMP)
    nuttx_smp_end_protection(id);
#endif
    return ret;
}

enum HAL_NORFLASH_RET_T norflash_write_resume(enum HAL_FLASH_ID_T id, int suspend)
{
    // TODO: Need to check SUS2 bit in status reg?

    return norflash_erase_resume(id, suspend);
}
#endif

int norflash_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint32_t index = 0;
    uint8_t val;
    uint8_t cmd;

    if (len > NORFLASHIP_RXFIFO_SIZE) {
        return 1;
    }

    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
        /* Quad , only fast */
        cmd = GD25Q32C_CMD_FAST_QUAD_IO_READ;
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
        /* Dual, only fast */
        cmd = GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ;
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_IO) {
        /* Dual, only fast */
        cmd = GD25Q32C_CMD_FAST_DUAL_IO_READ;
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
        /* Dual, only fast */
        cmd = GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ;
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_FAST_SPI){
        /* fast */
        cmd = GD25Q32C_CMD_STANDARD_FAST_READ;
    } else {
        /* normal */
        cmd = GD25Q32C_CMD_STANDARD_READ;
    }

    norflaship_clear_rxfifo(id);

    norflaship_busy_wait(id);

    norflaship_blksize(id, len);

#if (CHIP_FLASH_CTRL_VER >= 2)
    norflaship_ext_addr(id, start_address);
#endif
    norflaship_cmd_addr(id, cmd, start_address);

    while (1) {
        norflaship_rxfifo_empty_wait(id);

        val = norflaship_read_rxfifo(id);
        if (buffer) {
            buffer[index] = val;
        }

        ++index;
        if (index >= len) {
            break;
        }
    }

    norflaship_cmd_done(id);

    return 0;
}

void norflash_force_sleep(enum HAL_FLASH_ID_T id)
{
    norflaship_cmd_addr(id, GD25Q32C_CMD_DEEP_POWER_DOWN, 0);
}

void norflash_force_wakeup(enum HAL_FLASH_ID_T id)
{
    norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
}

void norflash_sleep(enum HAL_FLASH_ID_T id)
{
    norflash_pre_operation(id);
#ifdef FLASH_HPM
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
        norflash_set_hpm(id, 0);
    }
#endif
    norflaship_cmd_addr(id, GD25Q32C_CMD_DEEP_POWER_DOWN, 0);
}

void norflash_wakeup(enum HAL_FLASH_ID_T id)
{
    norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    // Wait 20us for flash to finish
    norflash_delay(40);
#ifdef FLASH_HPM
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
        norflash_set_hpm(id, 1);
    }
#endif
    norflash_post_operation(id);
}

void norflash_read_status(enum HAL_FLASH_ID_T id, uint16_t *status)
{
    uint8_t status_s0_s7;
    uint8_t status_s8_s15;

    status_s0_s7 = norflash_read_status_s0_s7(id);
    status_s8_s15 = norflash_read_status_s8_s15(id);

    *status = status_s0_s7 | (status_s8_s15 << 8);
}

void norflash_dual_chip_read_status(enum HAL_FLASH_ID_T id, uint16_t *status0, uint16_t *status1)
{
    uint8_t status_s0_s7[2];
    uint8_t status_s8_s15[2];

    norflash_dual_chip_read_status_s0_s7(id, &status_s0_s7[0], &status_s0_s7[1]);
    norflash_dual_chip_read_status_s8_s15(id, &status_s8_s15[0], &status_s8_s15[1]);

    *status0 = status_s0_s7[0] | (status_s8_s15[0] << 8);
    *status1 = status_s0_s7[1] | (status_s8_s15[1] << 8);
}

int norflash_init_status(enum HAL_FLASH_ID_T id, uint32_t status)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_INIT, status);

    return 0;
}

int norflash_init_block_protection(enum HAL_FLASH_ID_T id, uint32_t bp)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_BP_INIT, bp);

    return 0;
}

int norflash_set_block_protection(enum HAL_FLASH_ID_T id, uint32_t bp)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_BP, bp);

    return 0;
}

#ifdef FLASH_SECURITY_REGISTER
int norflash_security_register_get_lock_status(enum HAL_FLASH_ID_T id, uint32_t id_map, uint32_t *p_locked)
{
    union DRV_NORFLASH_SEC_REG_CFG_T cfg;
    uint8_t status_s8_s15;

    if (id_map == 0) {
        return 1;
    }

    cfg = norflash_get_security_register_config(id);
    if (!cfg.s.enabled) {
        return 2;
    }

    status_s8_s15 = norflash_read_status_s8_s15(id);

    if (cfg.s.lb == SEC_REG_LB_S11_S13) {
        if (id_map & ~7) {
            return 3;
        }
        *p_locked = ((status_s8_s15 >> STATUS_LB_S11_S13_BIT_SHIFT) & id_map);
    } else if (cfg.s.lb == SEC_REG_LB_S10) {
        if (id_map != 1) {
            return 3;
        }
        *p_locked = ((status_s8_s15 >> STATUS_LB_S10_BIT_SHIFT) & id_map);
    } else if (cfg.s.lb == SEC_REG_LB_S10_S13) {
        if (id_map & ~0xF) {
            return 3;
        }
        *p_locked = ((status_s8_s15 >> STATUS_LB_S10_BIT_SHIFT) & id_map);
    } else if (cfg.s.lb == SEC_REG_LB_S12_S13) {
        if (id_map & ~3) {
            return 3;
        }
        *p_locked = ((status_s8_s15 >> STATUS_LB_S12_S13_BIT_SHIFT) & id_map);
    } else {
        return 4;
    }

    return 0;
}

int norflash_security_register_lock(enum HAL_FLASH_ID_T id, uint32_t id_map)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_LB, id_map);

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    return 0;
}

enum HAL_NORFLASH_RET_T norflash_security_register_erase(enum HAL_FLASH_ID_T id, uint32_t start_address)
{
    enum HAL_NORFLASH_RET_T ret;

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, 0);
    // Need 1us. Or norflash_status_WEL_0_wait(id), which needs 6us.

    norflaship_cmd_addr(id, GD25Q32C_CMD_SECURITY_REGISTER_ERASE, start_address);

    ret = norflash_status_WIP_1_wait(id, 0);

    norflaship_cmd_done(id);

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    return ret;
}

enum HAL_NORFLASH_RET_T norflash_security_register_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    enum HAL_NORFLASH_RET_T ret;
    uint32_t remains;

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif

    // Security register page size might be larger than normal page size
    // E.g., the size of P25Q32L and P25Q64L is 1024

    norflaship_clear_txfifo(id);

#if (CHIP_FLASH_CTRL_VER <= 1)
    uint32_t div = 0;

    if (len > NORFLASHIP_TXFIFO_SIZE) {
        div = norflaship_get_div(id);

        // Slow down to avoid tx fifo underflow (it takes about 10 cpu cycles to fill one byte)
        norflaship_div(id, 16);
    }

    remains = norflaship_v1_write_txfifo_safe(buffer, len);
#else
    remains = norflaship_write_txfifo(id, buffer, len);
#endif

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, 0);

    norflaship_cmd_addr(id, GD25Q32C_CMD_SECURITY_REGISTER_PROGRAM, start_address);

#if (CHIP_FLASH_CTRL_VER <= 1)
    if (remains) {
        norflaship_v1_write_txfifo_all(buffer, len);
    }
#else
    while (remains > 0) {
        buffer += len - remains;
        len = remains;
        remains = norflaship_write_txfifo_cont(id, buffer, len);
    }
#endif

    norflaship_busy_wait(id);

    ret = norflash_status_WIP_1_wait(id, 0);

#if (CHIP_FLASH_CTRL_VER <= 1)
    if (div) {
        // Restore the old div
        norflaship_div(id, div);
    }
#endif

    norflaship_cmd_done(id);

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    return ret;
}

int norflash_security_register_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint32_t index = 0;

    if (len > NORFLASHIP_RXFIFO_SIZE) {
        return 1;
    }

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif

    norflaship_clear_rxfifo(id);

    norflaship_busy_wait(id);

    norflaship_blksize(id, len);

    norflaship_cmd_addr(id, GD25Q32C_CMD_SECURITY_REGISTER_READ, start_address);

    while (1) {
        norflaship_rxfifo_empty_wait(id);

        buffer[index] = norflaship_read_rxfifo(id);

        ++index;
        if (index >= len) {
            break;
        }
    }

    norflaship_cmd_done(id);

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    return 0;
}

uint32_t norflash_security_register_enable_read(enum HAL_FLASH_ID_T id)
{
    uint32_t mode;
    int result;

    mode = norflash_op_mode[id];

    result = norflash_set_mode(id, HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_PAGE_PROGRAM);
    ASSERT(result == 0, "Failed to set sec reg read mode");

    norflaship_busy_wait(id);
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(id, FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
    norflaship_busy_wait(id);
    norflaship_rdcmd(id, GD25Q32C_CMD_SECURITY_REGISTER_READ);

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, false);
    }
#endif

    return mode;
}

void norflash_security_register_disable_read(enum HAL_FLASH_ID_T id, uint32_t mode)
{
    int result;

#if defined(FLASH_DUAL_CHIP) && defined(FLASH_SEC_REG_DUAL_CHIP)
    if (flash_dual_chip[id] && !sec_reg_dual_chip[id]) {
        norflaship_dual_chip_mode(id, true);
    }
#endif

    norflaship_busy_wait(id);
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(id,FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
    norflaship_busy_wait(id);
    norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);

    result = norflash_set_mode(id, mode);
    ASSERT(result == 0, "Failed to restore normal mode after sec reg read");
}

#endif


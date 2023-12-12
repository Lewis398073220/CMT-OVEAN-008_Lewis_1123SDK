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
#if (CHIP_FLASH_CTRL_VER >= 5)

#include "nandflash_drv.h"
#include "cmsis.h"
#include "hal_dma.h"
#include "hal_norflaship.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "nandflash_gd5f4gq4.h"
#include "nandflash_w25m02gv.h"
#include "plat_addr_map.h"
#include "norflash_cfg.h"
#include "string.h"
#include "tool_msg.h"

#ifndef NANDFLASH_NC_BASE
#define NANDFLASH_NC_BASE                       FLASH1_NC_BASE
#endif

#ifdef PROGRAMMER
#include "sys_api_programmer.h"
#include "task_schedule.h"
#else
#define TASK_SCHEDULE                           true
#endif

#define NORFLASH_TRACE(...)                     TR_INFO(0, __VA_ARGS__)

#define SAMP_DELAY_PRIO_FALLING_EDGE

//#define NANDFLASH_UNIQUE_ID_LEN                 18

#define NANDFLASH_MAX_DIV                       0xFF

#define NANDFLASH_DEFAULT_MAX_SPEED             (104 * 1000 * 1000)

#define NANDFLASH_DIV1_MAX_SPEED                (99 * 1000 * 1000)

#define NANDFLASH_SPEED_RATIO_DENOMINATOR       8

enum NANDFLASH_CALIB_T {
    NANDFLASH_CALIB_FLASH_ID,
    NANDFLASH_CALIB_MAGIC_WORD,

    NANDFLASH_CALIB_QTY,
};

struct NANDFLASH_CTX_T {
    bool opened;
    enum NANDFLASH_RET_T open_state;
    uint8_t device_id[NANDFLASH_ID_LEN];
    uint32_t blocks_per_die;
    uint32_t cur_block;
    uint32_t cur_die;
};

static struct NANDFLASH_CTX_T nandflash_ctx[HAL_FLASH_ID_NUM];

static const char * const err_not_opened = "nandflash not opened";

static const struct NANDFLASH_CONFIG_T nandflash_cfg = {
#ifdef FPGA
    .source_clk = NANDFLASH_SPEED_13M * 2,
    .speed = NANDFLASH_SPEED_13M,
#elif defined(FLASH_LOW_SPEED)
    .source_clk = NANDFLASH_SPEED_26M * 2,
    .speed = NANDFLASH_SPEED_26M,
#elif defined(OSC_26M_X4_AUD2BB)
    .source_clk = NANDFLASH_SPEED_52M * 2,
    .speed = NANDFLASH_SPEED_52M,
#else
    .source_clk = NANDFLASH_SPEED_104M * 2,
    .speed = NANDFLASH_SPEED_104M,
#endif
    .mode = NANDFLASH_OP_MODE_STAND_SPI |
            NANDFLASH_OP_MODE_FAST_SPI |
            NANDFLASH_OP_MODE_DUAL_OUTPUT |
            NANDFLASH_OP_MODE_DUAL_IO |
            NANDFLASH_OP_MODE_QUAD_OUTPUT |
            NANDFLASH_OP_MODE_QUAD_IO |
            NANDFLASH_OP_MODE_PAGE_PROGRAM |
            NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
            NANDFLASH_OP_MODE_INTERNAL_ECC,
    .override_config = 0,
};

// GigaDevice
extern const struct NANDFLASH_CFG_T gd5f4gq4_cfg;
extern const struct NANDFLASH_CFG_T gd5f4gm5_cfg;

// XTS
extern const struct NANDFLASH_CFG_T xt26g02a_cfg;

// WINBOND
extern const struct NANDFLASH_CFG_T w25m02gv_cfg;

// MXIC
extern const struct NANDFLASH_CFG_T mx35lf1g_cfg;

// Dosilicon
extern const struct NANDFLASH_CFG_T ds35m2g_cfg;
extern const struct NANDFLASH_CFG_T ds35m1g_cfg;

static const struct NANDFLASH_CFG_T * const flash_list[] = {
    // ----------------------
    // GigaDevice
    // ----------------------
#if defined(__NANDFLASH_GD5F4GQ4__) || defined(__NANDFLASH_ALL__)
    &gd5f4gq4_cfg,
#endif
#if defined(__NANDFLASH_GD5F4GM5__) || defined(__NANDFLASH_ALL__)
    &gd5f4gm5_cfg,
#endif

    // ----------------------
    // XTS
    // ----------------------
#if defined(__NANDFLASH_XT26G02A__)
    &xt26g02a_cfg,
#endif

    // ----------------------
    // WINBOND
    // ----------------------
#if defined(__NANDFLASH_W25M02GV__) || defined(__NANDFLASH_ALL__)
    &w25m02gv_cfg,
#endif

    // ----------------------
    // Macronix
    // ----------------------
#if defined(__NANDFLASH_MX35LF1G__) || defined(__NANDFLASH_ALL__)
    &mx35lf1g_cfg,
#endif

    // ----------------------
    // Dosilicon
    // ----------------------
#if defined(__NANDFLASH_DS35M2G__) || defined(__NANDFLASH_ALL__)
    &ds35m2g_cfg,
#endif
#if defined(__NANDFLASH_DS35M1G__) || defined(__NANDFLASH_ALL__)
    &ds35m1g_cfg,
#endif
};

// Sample delay will be larger if:
// 1) flash speed is higher (major impact)
// 2) vcore voltage is lower (secondary major impact)
// 3) flash voltage is lower (minor impact)

// Sample delay unit:
// V1: 1/2 source_clk cycle when <= 2, 1 source_clk cycle when >= 2
// V2: 1/2 source_clk cycle when <= 4, 1 source_clk cycle when >= 4

// Flash clock low to output valid delay:
// T_clqv: 7 ns

// Flash IO latency:
// BEST1000/3001/1400: 4 ns
// BEST2000: 5 ns
// BEST2300: 2 ns

// Flash output time: T_clqv + T_io_latency
// Falling edge sample time: one spi_clk cycle (should > flash output time)

#define FALLING_EDGE_SAMPLE_ADJ_FREQ            (77 * 1000 * 1000) // about 13 ns

#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
#define DIV1_SAMP_DELAY_FALLING_EDGE_IDX        1
static const uint8_t samdly_list_div1[] = { 0, 1, 2, 3, };
#define DIV2_SAMP_DELAY_FALLING_EDGE_IDX        1
#define DIVN_SAMP_DELAY_FALLING_EDGE_IDX        3
static const uint8_t samdly_list_divn[] = { 2, 3, 4, 5, 6, 7, 8, 9, };
#else
static const uint8_t samdly_list_divn[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };
#endif

static uint8_t flash_idx[HAL_FLASH_ID_NUM];
static uint8_t div_read[HAL_FLASH_ID_NUM];
static uint32_t nandflash_op_mode[HAL_FLASH_ID_NUM];
static bool falling_edge_adj[HAL_FLASH_ID_NUM];
static uint8_t sample_delay_index[HAL_FLASH_ID_NUM];

#ifdef FLASH_CALIB_DEBUG
static uint32_t nandflash_source_clk;
static uint32_t nandflash_speed;
static uint8_t calib_matched_idx[NANDFLASH_CALIB_QTY];
static uint8_t calib_matched_cnt[NANDFLASH_CALIB_QTY];
static uint8_t calib_final_idx[NANDFLASH_CALIB_QTY];
#endif

#ifdef FLASH_DMA
static struct HAL_DMA_CH_CFG_T dma_cfg[HAL_FLASH_ID_NUM];
static struct HAL_DMA_DESC_T dma_desc[HAL_FLASH_ID_NUM][2];
#endif

//#define BUFFER_READ

static void nandflash_delay(uint32_t us)
{
    hal_sys_timer_delay_us(us);
}

static inline uint32_t nandflash_block_to_row_addr(uint32_t block)
{
    return (block << 6);
}

static inline uint32_t nandflash_block_page_to_row_addr(uint32_t block, uint32_t page)
{
    return (block << 6) | (page & 0x3F);
}

static inline uint32_t nandflash_block_page_offset_to_col_addr(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t offset)
{
    uint8_t plane_num = (flash_list[flash_idx[id]]->plane_cfg >> 4) & 0xF;
    uint8_t plane_offset = (flash_list[flash_idx[id]]->plane_cfg) & 0xF;

    if (plane_num > 1)
        return ((block&(plane_num-1)) << plane_offset) | offset;
    else
        return offset;
}

static inline uint32_t nandflash_block_to_die(enum HAL_FLASH_ID_T id, uint32_t block)
{
    uint8_t plane_num = (flash_list[flash_idx[id]]->plane_cfg >> 4) & 0xF;

    if (plane_num > 1) { //Dosilicon 2G/4G multi plane
        return (block&(plane_num-1));
    } else if (nandflash_ctx[id].blocks_per_die) { //Winbond multi die
        return block / nandflash_ctx[id].blocks_per_die;
    } else {
        return 0;
    }
}

#if defined(FLASH_DMA) || defined(BUFFER_READ)
static void nandflash_read_from_cache(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t len)
{
    // Switch to traditional command format
    norflaship_cmd_done(id);

    norflaship_blksize(id, len);

    if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_QUAD_IO) {
        /* Quad , only fast */
        norflaship_cmd_addr(id, GD5F4GQ4_CMD_FAST_QUAD_IO_READ, offset);
    } else if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_QUAD_OUTPUT) {
        /* Dual, only fast */
        norflaship_cmd_addr(id, GD5F4GQ4_CMD_FAST_QUAD_OUTPUT_READ, offset);
    } else if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_DUAL_IO) {
        /* Dual, only fast */
        norflaship_cmd_addr(id, GD5F4GQ4_CMD_FAST_DUAL_IO_READ, offset);
    } else if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_DUAL_OUTPUT) {
        /* Dual, only fast */
        norflaship_cmd_addr(id, GD5F4GQ4_CMD_FAST_DUAL_OUTPUT_READ, offset);
    } else if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_FAST_SPI){
        /* fast */
        norflaship_cmd_addr(id, GD5F4GQ4_CMD_STANDARD_FAST_READ, offset);
    } else {
        /* normal */
        norflaship_cmd_addr(id, GD5F4GQ4_CMD_STANDARD_READ, offset);
    }
}
#endif

#ifdef FLASH_DMA
static uint32_t nandflash_start_rx_dma(enum HAL_FLASH_ID_T id, uint8_t *buf, uint32_t len)
{
    enum HAL_DMA_PERIPH_T periph;
    enum HAL_DMA_RET_T ret;
    uint8_t ch;
    uint32_t xfer;

    if (len < 64) {
        return len;
    }

    norflaship_clear_rxfifo(id);

    if (id == HAL_FLASH_ID_0)
        periph = HAL_GPDMA_FLASH0;
    else
        periph = HAL_GPDMA_FLASH1;
    ch = hal_gpdma_get_chan(periph, HAL_DMA_HIGH_PRIO);
    if (ch == HAL_DMA_CHAN_NONE) {
        return len;
    }

    norflaship_dmactrl_rx_en(id, true);
    norflaship_busy_wait(id);

    memset(&dma_cfg[id], 0, sizeof(dma_cfg[id]));
    dma_cfg[id].handler = NULL;
    dma_cfg[id].dst = (uint32_t)buf;
    dma_cfg[id].dst_bsize = HAL_DMA_BSIZE_4;
    dma_cfg[id].dst_periph = 0;
    dma_cfg[id].dst_width = HAL_DMA_WIDTH_BYTE;
    dma_cfg[id].src = 0;
    dma_cfg[id].src_bsize = HAL_DMA_BSIZE_4;
    dma_cfg[id].src_periph = periph;
    dma_cfg[id].src_width = HAL_DMA_WIDTH_WORD;
    dma_cfg[id].type = HAL_DMA_FLOW_P2M_DMA;
    dma_cfg[id].try_burst = 0;
    dma_cfg[id].ch = ch;

    xfer = (len & ~0x3);
    dma_cfg[id].src_tsize = xfer / 4;

    ret = hal_dma_start(&dma_cfg[id]);
    if (ret != HAL_DMA_OK) {
        ASSERT(false, "%s: Failed to start dma: %d", __func__, ret);
    }

    return len - xfer;
}

static uint32_t nandflash_start_tx_dma(enum HAL_FLASH_ID_T id, const uint8_t *buf, uint32_t len)
{
    enum HAL_DMA_PERIPH_T periph;
    enum HAL_DMA_RET_T ret;
    uint8_t ch;
    uint32_t total_xfer;
    uint32_t xfer;

    if (len < 64) {
        return len;
    }

    if (id == HAL_FLASH_ID_0)
        periph = HAL_GPDMA_FLASH0;
    else
        periph = HAL_GPDMA_FLASH1;

    ch = hal_gpdma_get_chan(periph, HAL_DMA_HIGH_PRIO);
    if (ch == HAL_DMA_CHAN_NONE) {
        return len;
    }

    norflaship_dmactrl_tx_en(id, true);
    norflaship_blksize(id, len);
    norflaship_busy_wait(id);

    memset(&dma_cfg[id], 0, sizeof(dma_cfg[id]));
    dma_cfg[id].handler = NULL;
    dma_cfg[id].dst = 0;
    dma_cfg[id].dst_bsize = HAL_DMA_BSIZE_8;
    dma_cfg[id].dst_periph = periph;
    dma_cfg[id].dst_width = HAL_DMA_WIDTH_WORD;
    dma_cfg[id].src = (uint32_t)buf;
    dma_cfg[id].src_bsize = HAL_DMA_BSIZE_8;
    dma_cfg[id].src_periph = 0;
    dma_cfg[id].src_width = HAL_DMA_WIDTH_BYTE;
    dma_cfg[id].type = HAL_DMA_FLOW_M2P_DMA;
    dma_cfg[id].try_burst = 0;
    dma_cfg[id].ch = ch;

    total_xfer = (len & ~0x3);
    xfer = (HAL_DMA_MAX_DESC_XFER_SIZE & ~0x3);
    if (xfer < total_xfer) {
        dma_cfg[id].src_tsize = xfer;
        ret = hal_dma_init_desc(&dma_desc[id][0], &dma_cfg[id], &dma_desc[id][1], 0);
        if (ret != HAL_DMA_OK) {
            ASSERT(false, "%s Failed to init dma desc: %d", __func__, ret);
        }
        dma_cfg[id].src += xfer;
        dma_cfg[id].src_tsize = total_xfer - xfer;
        ret = hal_dma_init_desc(&dma_desc[id][1], &dma_cfg[id], NULL, 0);
    } else {
        dma_cfg[id].src_tsize = total_xfer;
        ret = hal_dma_init_desc(&dma_desc[id][0], &dma_cfg[id], NULL, 0);
    }
    if (ret != HAL_DMA_OK) {
        ASSERT(false, "%s Failed to init dma desc: %d", __func__, ret);
    }

    ret = hal_dma_sg_start(&dma_desc[id][0], &dma_cfg[id]);
    if (ret != HAL_DMA_OK) {
        ASSERT(false, "%s: Failed to start dma: %d", __func__, ret);
    }

    return len - total_xfer;
}

static void nandflash_dma_busy_wait(enum HAL_FLASH_ID_T id)
{
    if (dma_cfg[id].ch == HAL_DMA_CHAN_NONE) {
        return;
    }
    while (hal_dma_chan_busy(dma_cfg[id].ch) && TASK_SCHEDULE);

    hal_dma_free_chan(dma_cfg[id].ch);
    dma_cfg[id].ch = HAL_DMA_CHAN_NONE;

    //norflaship_busy_wait(id);
    if (dma_cfg[id].type == HAL_DMA_FLOW_P2M_DMA) {
        norflaship_dmactrl_rx_en(id, false);
    } else {
        norflaship_dmactrl_tx_en(id, false);
    }
    //norflaship_busy_wait(id);
}
#endif

POSSIBLY_UNUSED
static int nandflash_read_reg(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *val, uint32_t len)
{
    int i;

    norflaship_clear_fifos(id);
    norflaship_ext_rx_cmd(id, cmd, 0, len);
    norflaship_rxfifo_count_wait(id, len);
    for (i = 0; i < len; i++) {
        val[i] = norflaship_read_rxfifo(id);
    }
    norflaship_cmd_done(id);

    return 0;
}

static int nandflash_read_reg_ex(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *param, uint32_t param_len, uint8_t *val, uint32_t len)
{
    int i;

    norflaship_clear_fifos(id);
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
    norflaship_cmd_done(id);

    return 0;
}

static uint8_t nandflash_read_statusid(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    uint8_t param;

    param = GD5F4GQ4_REG_STATUS;
    nandflash_read_reg_ex(id, GD5F4GQ4_CMD_GET_FEATURE, &param, 1, &val, 1);
    return val;
}

static int nandflash_status_WEL(enum HAL_FLASH_ID_T id)
{
    uint32_t status;
    status = nandflash_read_statusid(id);
    return !!(status & GD5F4GQ4_ST_WEL);
}

static int nandflash_status_OIP(enum HAL_FLASH_ID_T id)
{
    uint32_t status;
    status = nandflash_read_statusid(id);
    return !!(status & GD5F4GQ4_ST_OIP);
}

POSSIBLY_UNUSED
static void nandflash_status_WEL_0_wait(enum HAL_FLASH_ID_T id)
{
    while (nandflash_status_WEL(id) == 0 && TASK_SCHEDULE);
}

static void nandflash_status_OIP_1_waitid(enum HAL_FLASH_ID_T id)
{
    while (nandflash_status_OIP(id) && TASK_SCHEDULE);
}

static int nandflash_write_reg(enum HAL_FLASH_ID_T id, uint8_t cmd, const uint8_t *val, uint32_t len)
{
    norflaship_cmd_addr(id, GD5F4GQ4_CMD_WRITE_ENABLE, 0);
    //nandflash_status_OIP_1_waitid();
    //nandflash_status_WEL_0_wait();

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, val, len);

    norflaship_ext_tx_cmd(id, cmd, len);

    nandflash_status_OIP_1_waitid(id);
    norflaship_cmd_done(id);

    return 0;
}

static int nandflash_read_device_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    uint8_t id_addr;

    if (len > NANDFLASH_ID_LEN) {
        len = NANDFLASH_ID_LEN;
    }
    id_addr = 0x00;
    nandflash_read_reg_ex(id, GD5F4GQ4_CMD_ID, &id_addr, 1, value, len);
    norflaship_cmd_done(id);

    return 0;
}

static uint8_t nandflash_read_protection(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    uint8_t param;

    param = GD5F4GQ4_REG_PROTECTION;
    nandflash_read_reg_ex(id, GD5F4GQ4_CMD_GET_FEATURE, &param, 1, &val, 1);
    return val;
}

static void nandflash_write_protection(enum HAL_FLASH_ID_T id, uint8_t val)
{
    uint8_t param[2];

    param[0] = GD5F4GQ4_REG_PROTECTION;
    param[1] = val;
    nandflash_write_reg(id, GD5F4GQ4_CMD_SET_FEATURE, &param[0], 2);
}

static uint8_t nandflash_read_configuration(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    uint8_t param;

    param = GD5F4GQ4_REG_CONFIGURATION;
    nandflash_read_reg_ex(id, GD5F4GQ4_CMD_GET_FEATURE, &param, 1, &val, 1);
    return val;
}

static void nandflash_write_configuration(enum HAL_FLASH_ID_T id, uint8_t val)
{
    uint8_t param[2];

    param[0] = GD5F4GQ4_REG_CONFIGURATION;
    param[1] = val;
    nandflash_write_reg(id, GD5F4GQ4_CMD_SET_FEATURE, &param[0], 2);
}

void nandflash_dump_features(enum HAL_FLASH_ID_T id)
{
    uint8_t val;

    if (!nandflash_ctx[id].opened) {
        NORFLASH_TRACE("%s", err_not_opened);
        return;
    }

    NORFLASH_TRACE("Mode: cur=0x%04X supported=0x%04X", nandflash_op_mode[id], flash_list[flash_idx[id]]->mode);
    val = nandflash_read_protection(id);
    NORFLASH_TRACE("Protection: 0x%02X", val);
    val = nandflash_read_configuration(id);
    NORFLASH_TRACE("Configuration: 0x%02X", val);
    val = nandflash_read_statusid(id);
    NORFLASH_TRACE("Status: 0x%02X", val);
}

static int nandflash_set_quad(enum HAL_FLASH_ID_T id, uint8_t on)
{
    uint8_t cfg;

    cfg = nandflash_read_configuration(id);
    if (on) {
        cfg |= GD5F4GQ4_CFG_QE;
    } else {
        cfg &= ~GD5F4GQ4_CFG_QE;
    }
    nandflash_write_configuration(id, cfg);
    return 0;
}

static int nandflash_set_quad_io_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    nandflash_set_quad(id, on);
    if (on) {
        norflaship_quad_mode(id, 1);
    } else {
        norflaship_quad_mode(id, 0);
    }
    norflaship_busy_wait(id);
    return 0;
}

static int nandflash_set_quad_output_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    nandflash_set_quad(id, on);
    if (on) {
        norflaship_rdcmd(id, GD5F4GQ4_CMD_FAST_QUAD_OUTPUT_READ);
    } else {
        norflaship_rdcmd(id, GD5F4GQ4_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t nandflash_set_dual_io_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_dual_mode(id, 1);
    } else {
        norflaship_dual_mode(id, 0);
    }
    norflaship_busy_wait(id);

    return 0;
}

static uint8_t nandflash_set_dual_output_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_rdcmd(id, GD5F4GQ4_CMD_FAST_DUAL_OUTPUT_READ);
    } else {
        norflaship_rdcmd(id, GD5F4GQ4_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t nandflash_set_fast_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_rdcmd(id, GD5F4GQ4_CMD_STANDARD_FAST_READ);
    } else {
        norflaship_rdcmd(id, GD5F4GQ4_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t nandflash_set_stand_mode(enum HAL_FLASH_ID_T id)
{
    norflaship_rdcmd(id, GD5F4GQ4_CMD_STANDARD_READ);
    norflaship_busy_wait(id);
    return 0;
}

static uint32_t nandflash_get_supported_mode(enum HAL_FLASH_ID_T id)
{
     return flash_list[flash_idx[id]]->mode;
}

static uint32_t nandflash_get_block_protect_mask(enum HAL_FLASH_ID_T id)
{
    return flash_list[flash_idx[id]]->block_protect_mask;
}

static void nandflash_reset(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);

    // Quit from quad/dual continuous read mode
    norflaship_quad_mode(id, 0);
    norflaship_dual_mode(id, 0);
    norflaship_busy_wait(id);

    // Software reset
    norflaship_ext_tx_cmd(id, GD5F4GQ4_CMD_RESET, 0);
    // Reset recovery time: 5~500 us
    nandflash_delay(500);

    norflaship_cmd_done(id);

    // Reset cfg
    flash_idx[id] = 0;
    div_read[id] = 0;
    nandflash_op_mode[id] = 0;
    falling_edge_adj[id] = false;
}

static void nandflash_software_die_select(enum HAL_FLASH_ID_T id, uint8_t die_id)
{
    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, &die_id, 1);
    norflaship_ext_tx_cmd(id, W25M02GV_CMD_DIE_SEL, 1);
    norflaship_cmd_done(id);
}

static void nandflash_select_die(enum HAL_FLASH_ID_T id, uint32_t block)
{
    uint32_t die;

    if (nandflash_ctx[id].blocks_per_die == 0) {
        return;
    }
    if (nandflash_ctx[id].cur_block == block) {
        return;
    }
    nandflash_ctx[id].cur_block = block;
    die = block / nandflash_ctx[id].blocks_per_die;
    if (nandflash_ctx[id].cur_die == die) {
        return;
    }
    nandflash_ctx[id].cur_die = die;

    nandflash_software_die_select(id, die);
}

static int nandflash_set_mode(enum HAL_FLASH_ID_T id, uint32_t op)
{
    uint32_t read_mode = 0;
    uint32_t ext_mode = 0;
    uint32_t program_mode = 0;
    uint32_t self_mode;
    uint32_t mode;
    uint8_t cfg;

    self_mode = nandflash_get_supported_mode(id);
    mode = (self_mode & op);

    if (mode & NANDFLASH_OP_MODE_QUAD_IO) {
        read_mode |= NANDFLASH_OP_MODE_QUAD_IO;
    } else if (mode & NANDFLASH_OP_MODE_QUAD_OUTPUT) {
        read_mode |= NANDFLASH_OP_MODE_QUAD_OUTPUT;
    } else if (mode & NANDFLASH_OP_MODE_DUAL_IO) {
        read_mode |= NANDFLASH_OP_MODE_DUAL_IO;
    } else if (mode & NANDFLASH_OP_MODE_DUAL_OUTPUT) {
        read_mode |= NANDFLASH_OP_MODE_DUAL_OUTPUT;
    } else if (mode & NANDFLASH_OP_MODE_FAST_SPI) {
        read_mode |= NANDFLASH_OP_MODE_FAST_SPI;
    } else if(mode & NANDFLASH_OP_MODE_STAND_SPI) {
        read_mode |= NANDFLASH_OP_MODE_STAND_SPI;
    } else {
        // Op error
        return  1;
    }

    if (mode & NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        program_mode |= NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM;
    } else if (mode & NANDFLASH_OP_MODE_PAGE_PROGRAM) {
        program_mode |= NANDFLASH_OP_MODE_PAGE_PROGRAM;
    } else {
        // Op error
        return 1;
    }

    if (mode & NANDFLASH_OP_MODE_INTERNAL_ECC) {
        ext_mode |= NANDFLASH_OP_MODE_INTERNAL_ECC;
    }
    if (self_mode & NANDFLASH_OP_MODE_READ_ADDR_24BIT) {
        ext_mode |= NANDFLASH_OP_MODE_READ_ADDR_24BIT;
    }

    mode = (read_mode | ext_mode | program_mode);

    if (nandflash_op_mode[id] != mode) {
        norflaship_quad_mode(id, 0);

        norflaship_dual_mode(id, 0);

        if (mode & NANDFLASH_OP_MODE_QUAD_IO) {
            // Quad io mode
            nandflash_set_quad_io_mode(id, 1);
        } else if (mode & NANDFLASH_OP_MODE_QUAD_OUTPUT) {
            // Quad output mode
            nandflash_set_quad_output_mode(id, 1);
        } else if (mode & NANDFLASH_OP_MODE_DUAL_IO) {
            // Dual io mode
            nandflash_set_dual_io_mode(id, 1);
        } else if (mode & NANDFLASH_OP_MODE_DUAL_OUTPUT) {
            // Dual output mode
            nandflash_set_dual_output_mode(id, 1);
        } else if (mode & NANDFLASH_OP_MODE_FAST_SPI) {
            // Fast mode
            nandflash_set_fast_mode(id, 1);
        } else if (mode & NANDFLASH_OP_MODE_STAND_SPI) {
            // Standard spi mode
            nandflash_set_stand_mode(id);
        }

        cfg = nandflash_read_configuration(id);
        if (mode & NANDFLASH_OP_MODE_INTERNAL_ECC) {
            cfg |= GD5F4GQ4_CFG_ECC_EN;
        } else {
            cfg &= ~GD5F4GQ4_CFG_ECC_EN;
        }
        nandflash_write_configuration(id, cfg);

        if (mode & NANDFLASH_OP_MODE_READ_ADDR_24BIT) {
            norflaship_nand_addr_byte_num(id, 2);
        } else {
            // Address 16BIT
            norflaship_nand_addr_byte_num(id, 1);
        }

        norflaship_cmd_done(id);

        // Update current mode at last
        nandflash_op_mode[id] = mode;
    }

    return 0;
}

static void nandflash_get_samdly_list(uint32_t div, const uint8_t **samdly_list_p, uint32_t *size_p)
{
    const uint8_t *samdly_list = NULL;
    uint32_t size = 0;

    if (div >= 1) {
        if (div == 1) {
            samdly_list = samdly_list_div1;
            size = ARRAY_SIZE(samdly_list_div1);
        } else {
            samdly_list = samdly_list_divn;
            size = ARRAY_SIZE(samdly_list_divn);
        }
    }

    if (samdly_list_p) {
        *samdly_list_p = samdly_list;
    }
    if (size_p) {
        *size_p = size;
    }
}

static void nandflash_set_sample_delay_index(enum HAL_FLASH_ID_T id, uint32_t index)
{
    const uint8_t *samdly_list;
    uint32_t size;
    uint32_t div;

    sample_delay_index[id] = index;

    div = norflaship_get_div(id);

    nandflash_get_samdly_list(div, &samdly_list, &size);

    if (index < size) {
        norflaship_samdly(id, samdly_list[index]);
    }
}

static bool nandflash_calib_flash_id_valid(enum HAL_FLASH_ID_T id)
{
    uint8_t flash_id[NANDFLASH_ID_LEN];
    const uint8_t *cmp_id;

    nandflash_read_device_id(id, flash_id, sizeof(flash_id));
    cmp_id = flash_list[flash_idx[id]]->id;

    if (flash_id[0] == cmp_id[0] && flash_id[1] == cmp_id[1] && flash_id[2] == cmp_id[2]) {
        return true;
    }
    return false;
}

static bool nandflash_calib_magic_word_valid(enum HAL_FLASH_ID_T id)
{
    uint32_t magic = 0;

#ifdef BUFFER_READ
    uint32_t len = 4;
    norflaship_clear_rxfifo(id);
    nandflash_read_from_cache(id, 0, len);
    while (len--) {
        norflaship_rxfifo_empty_wait(id);

        magic = (magic << 8) | norflaship_read_rxfifo(id);
    }
#else
    norflaship_clear_rxfifo(id);
    magic = *(volatile uint32_t *)NANDFLASH_NC_BASE;
#endif
    if (magic == BOOT_MAGIC_NUMBER) {
        return true;
    }
    return false;
}

static int nandflash_sample_delay_calib(enum HAL_FLASH_ID_T id, enum NANDFLASH_CALIB_T type)
{
    int i;
    uint32_t matched_cnt = 0;
    uint32_t matched_idx = 0;
    uint32_t div;
    uint32_t size;
    bool valid;

    if (type >= NANDFLASH_CALIB_QTY) {
        return 1;
    }
#if defined(ROM_BUILD) || defined(PROGRAMMER)
    if (type != NANDFLASH_CALIB_FLASH_ID) {
        return 0;
    }
#endif

    div = norflaship_get_div(id);

    if (div == 0) {
        return -1;
    }

    nandflash_get_samdly_list(div, NULL, &size);

    for (i = 0; i < size; i++) {
        norflaship_busy_wait(id);

        nandflash_set_sample_delay_index(id, i);

        if (type == NANDFLASH_CALIB_FLASH_ID) {
            valid = nandflash_calib_flash_id_valid(id);
        } else {
            valid = nandflash_calib_magic_word_valid(id);
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
    calib_matched_idx[type] = matched_idx;
    calib_matched_cnt[type] = matched_cnt;
#endif

    if (matched_cnt) {
#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
        if (matched_cnt == 2) {
            uint32_t falling_edge_idx;

            if (0) {
            } else if (div == 1) {
                falling_edge_idx = DIV1_SAMP_DELAY_FALLING_EDGE_IDX;
            } else if (div == 2) {
                falling_edge_idx = DIV2_SAMP_DELAY_FALLING_EDGE_IDX;
                if (falling_edge_adj[id]) {
                    falling_edge_idx++;
                }
            } else {
                falling_edge_idx = DIVN_SAMP_DELAY_FALLING_EDGE_IDX;
            }
            if (matched_idx <= falling_edge_idx &&
                    falling_edge_idx < matched_idx + matched_cnt) {
                matched_idx = falling_edge_idx;
                matched_cnt = 1;
            }
        }
#endif
        matched_idx += matched_cnt / 2;
        nandflash_set_sample_delay_index(id, matched_idx);

#ifdef FLASH_CALIB_DEBUG
        calib_final_idx[type] = matched_idx;
#endif

        return 0;
    }

#ifdef FLASH_CALIB_DEBUG
    calib_final_idx[type] = -1;
#endif

    return 1;
}

void nandflash_show_calib_result(enum HAL_FLASH_ID_T id)
{
#ifdef FLASH_CALIB_DEBUG
    uint32_t div;
    uint32_t size;
    const uint8_t *list;
    int i;
#ifdef PROGRAMMER
    unsigned char buf[16];
#endif

    NORFLASH_TRACE(0, "FLASH_CALIB_RESULT:");
    NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<FREQ>\nsource_clk=%u speed=%u flash_max=%u",
        nandflash_source_clk, nandflash_speed, flash_list[flash_idx[id]]->max_speed);
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &nandflash_source_clk, sizeof(nandflash_source_clk));
    memcpy(buf + 4, &nandflash_speed, sizeof(nandflash_speed));
    memcpy(buf + 8, &flash_list[flash_idx[id]]->max_speed, sizeof(flash_list[flash_idx[id]]->max_speed));
    send_debug_event(buf, 12);
#endif

    div = norflaship_get_div(id);
    NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<DIV>\ndiv=%u", div);
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    buf[0] = div;
    send_debug_event(buf, 1);
#endif

    nandflash_get_samdly_list(div, &list, &size);
    NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<SAMDLY LIST>");
    if (list == NULL || size == 0) {
        NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "NONE");
    } else {
        DUMP8("%02X ", list, size);
#ifdef PROGRAMMER
        send_debug_event(list, size);
#endif
    }
    NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<CALIB RESULT>");
    for (i = 0; i < NANDFLASH_CALIB_QTY; i++) {
        NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "type=%d idx=%02u cnt=%02u final=%02u",
            i, calib_matched_idx[i], calib_matched_cnt[i], calib_final_idx[i]);
#ifdef PROGRAMMER
        memset(buf, 0, sizeof(buf));
        buf[0] = i;
        buf[1] = calib_matched_idx[i];
        buf[2] = calib_matched_cnt[i];
        buf[3] = calib_final_idx[i];
        send_debug_event(buf, 4);
#endif
    }
    NORFLASH_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "\t");
#endif
}

static int nandflash_init_sample_delay_by_div(enum HAL_FLASH_ID_T id, uint32_t div)
{
    if (div == 0) {
        return -1;
    } if (div == 1) {
        norflaship_samdly(id, 1);
    } else if (div == 2 && !falling_edge_adj[id]) {
        // Set sample delay to clock falling edge
        norflaship_samdly(id, 3);
    } else {
        // Set sample delay to nearest to but not later than clock falling edge
        norflaship_samdly(id, 4);
    }

    return 0;
}

static int nandflash_init_div(enum HAL_FLASH_ID_T id, const struct NANDFLASH_CONFIG_T *cfg)
{
    uint32_t max_speed;
    uint32_t read_speed;
    uint32_t div;

#ifdef FLASH_CALIB_DEBUG
    nandflash_source_clk = cfg->source_clk;
    nandflash_speed = cfg->speed;
#endif

    max_speed = flash_list[flash_idx[id]]->max_speed;
    if (max_speed == 0) {
        max_speed = NANDFLASH_DEFAULT_MAX_SPEED;
    }

    read_speed = max_speed;
    if (read_speed > cfg->speed) {
        read_speed = cfg->speed;
    }
    if (read_speed > cfg->source_clk) {
        read_speed = cfg->source_clk;
    }

    div = (cfg->source_clk + read_speed - 1) / read_speed;
    div_read[id] = (div < NANDFLASH_MAX_DIV) ? div : NANDFLASH_MAX_DIV;

    if (div_read[id] == 2 && read_speed >= FALLING_EDGE_SAMPLE_ADJ_FREQ) {
        falling_edge_adj[id] = true;
    } else {
        falling_edge_adj[id] = false;
    }

    // Init sample delay according to div_read[id]
    nandflash_init_sample_delay_by_div(id, div_read[id]);
    norflaship_div(id, div_read[id]);

    return 0;
}

static int nandflash_match_chip(enum HAL_FLASH_ID_T id, const uint8_t *dev_id, uint32_t len)
{
    const uint8_t *cmp_id;

    if (len == NANDFLASH_ID_LEN) {
        for (flash_idx[id] = 0; flash_idx[id] < ARRAY_SIZE(flash_list); flash_idx[id]++) {
            cmp_id = flash_list[flash_idx[id]]->id;
            if (dev_id[0] == cmp_id[0] && dev_id[1] == cmp_id[1] && dev_id[2] == cmp_id[2]) {
                return true;
            }
        }
    }

    return false;
}

static enum NANDFLASH_RET_T nandflash_check_block(enum HAL_FLASH_ID_T id, uint32_t block)
{
    if ((block * flash_list[flash_idx[id]]->block_size) >= flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_BLOCK;
    }
    return NANDFLASH_RET_OK;
}

static enum NANDFLASH_RET_T nandflash_check_page(enum HAL_FLASH_ID_T id, uint32_t page)
{
    if ((page * flash_list[flash_idx[id]]->page_size) >= flash_list[flash_idx[id]]->block_size) {
        return NANDFLASH_RET_BAD_PAGE;
    }
    return NANDFLASH_RET_OK;
}

static enum NANDFLASH_RET_T nandflash_check_page_offset_len(enum HAL_FLASH_ID_T id, uint32_t page_offset, uint32_t len)
{
    if (page_offset >= flash_list[flash_idx[id]]->page_size) {
        return NANDFLASH_RET_BAD_PAGE_OFFSET;
    }
    if (len > flash_list[flash_idx[id]]->page_size) {
        return NANDFLASH_RET_BAD_LEN;
    }
    if (page_offset + len > flash_list[flash_idx[id]]->page_size) {
        return NANDFLASH_RET_BAD_LEN;
    }
    return NANDFLASH_RET_OK;
}

static enum NANDFLASH_RET_T nandflash_check_oob_offset_len(enum HAL_FLASH_ID_T id, uint32_t oob_offset, uint32_t len)
{
    if (oob_offset >= flash_list[flash_idx[id]]->page_spare_size) {
        return NANDFLASH_RET_BAD_PAGE_OFFSET;
    }
    if (len > flash_list[flash_idx[id]]->page_spare_size) {
        return NANDFLASH_RET_BAD_LEN;
    }
    if (oob_offset + len > flash_list[flash_idx[id]]->page_spare_size) {
        return NANDFLASH_RET_BAD_LEN;
    }
    return NANDFLASH_RET_OK;
}

static enum NANDFLASH_RET_T nandflash_erase_raw_block(enum HAL_FLASH_ID_T id, uint32_t block)
{
    uint32_t row_addr;
    uint8_t status;

    row_addr = nandflash_block_to_row_addr(block);

    nandflash_select_die(id, block);

    norflaship_cmd_addr(id, GD5F4GQ4_CMD_WRITE_ENABLE, 0);
    norflaship_cmd_addr(id, GD5F4GQ4_CMD_BLOCK_ERASE, row_addr);
    norflaship_busy_wait(id);
    nandflash_status_OIP_1_waitid(id);
    status = nandflash_read_statusid(id);
    norflaship_cmd_done(id);

    if (status & GD5F4GQ4_ST_E_FAIL) {
        return NANDFLASH_RET_ERASE_ERR;
    }
    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_erase_block(enum HAL_FLASH_ID_T id, uint32_t block)
{
    enum NANDFLASH_RET_T ret;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }

    return nandflash_erase_raw_block(id, block);
}

static void nandflash_program_load(enum HAL_FLASH_ID_T id, uint8_t cmd, uint32_t col_addr, const uint8_t *buffer, uint32_t len)
{
    uint32_t remains = 0;
    bool dma_en = false;;

    norflaship_clear_txfifo(id);

#ifdef FLASH_DMA
    remains = nandflash_start_tx_dma(id, buffer, len);
    if (remains < len) {
        dma_en = true;
    }
#endif

    if (!dma_en) {
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(UNALIGNED_ACCESS)
        remains = norflaship_write4_txfifo(id, buffer, len);
#else
        remains = norflaship_write_txfifo(id, buffer, len);
#endif
    }

    norflaship_cmd_addr(id, cmd, col_addr);

#ifdef FLASH_DMA
    if (dma_en) {
        nandflash_dma_busy_wait(id);
    }
#endif

    while (remains > 0) {
        buffer += len - remains;
        len = remains;
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(UNALIGNED_ACCESS)
        remains = norflaship_write4_txfifo_cont(id, buffer, len);
#else
        remains = norflaship_write_txfifo_cont(id, buffer, len);
#endif
    }

    norflaship_busy_wait(id);
}

static enum NANDFLASH_RET_T nandflash_program_execute(enum HAL_FLASH_ID_T id, uint32_t row_addr)
{
    uint8_t status;

    norflaship_cmd_addr(id, GD5F4GQ4_CMD_PROGRAM_EXECUTRE, row_addr);
    norflaship_busy_wait(id);
    nandflash_status_OIP_1_waitid(id);
    status = nandflash_read_statusid(id);

    if (status & GD5F4GQ4_ST_P_FAIL) {
        return NANDFLASH_RET_PROGRAM_ERR;
    }
    return NANDFLASH_RET_OK;
}

static enum NANDFLASH_RET_T nandflash_start_page_read(enum HAL_FLASH_ID_T id, uint32_t row_addr)
{
    uint8_t status;
    enum NANDFLASH_ECC_STATUS_T ecc_st;

    norflaship_cmd_addr(id, GD5F4GQ4_CMD_PAGE_READ, row_addr);
    norflaship_busy_wait(id);
    nandflash_status_OIP_1_waitid(id);
    status = nandflash_read_statusid(id);

    ecc_st = flash_list[flash_idx[id]]->ecc_status_type;
    if (ecc_st == NANDFLASH_ECC_ST_3BITS_111_BAD) {
        status = GET_BITFIELD(status, GD5F4GM5_ST_ECCS);
        if (status == 0b000) {
            return NANDFLASH_RET_OK;
        } else if (status == 0b111) {
            return NANDFLASH_RET_READ_ERR;
        } else {
            return NANDFLASH_RET_READ_CORRECTED;
        }
    } else if (ecc_st == NANDFLASH_ECC_ST_3BITS_010_BAD) {
        status = GET_BITFIELD(status, GD5F4GM5_ST_ECCS);
        if (status == 0b000) {
            return NANDFLASH_RET_OK;
        } else if (status == 0b001) {
            return NANDFLASH_RET_READ_CORRECTED;
        } else if (status == 0b010) {
            return NANDFLASH_RET_READ_ERR;
        } else if (status == 0b011) {
            return NANDFLASH_RET_READ_CORRECTED;
        } else if (status == 0b101) {
            return NANDFLASH_RET_READ_CORRECTED;
        }
    } else {
        status = GET_BITFIELD(status, GD5F4GQ4_ST_ECCS);
        if (status == 0b00) {
            return NANDFLASH_RET_OK;
        } else if (status == 0b01) {
            return NANDFLASH_RET_READ_CORRECTED;
        } else if (status == 0b10) {
            return NANDFLASH_RET_READ_ERR;
        }

        if (ecc_st == NANDFLASH_ECC_ST_2BITS_11_COR) {
            return NANDFLASH_RET_READ_CORRECTED;
        }
    }

    return NANDFLASH_RET_READ_ERR;
}

static enum NANDFLASH_RET_T nandflash_write_page_offset(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint8_t cmd;
    uint32_t row_addr, col_addr;

    if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        cmd = GD5F4GQ4_CMD_QUAD_PROGRAM_LOAD;
    } else {
        cmd = GD5F4GQ4_CMD_PROGRAM_LOAD;
    }

    row_addr = nandflash_block_page_to_row_addr(block, page);

    nandflash_select_die(id, block);

    norflaship_cmd_addr(id, GD5F4GQ4_CMD_WRITE_ENABLE, 0);

    col_addr = nandflash_block_page_offset_to_col_addr(id, block, offset);
    nandflash_program_load(id, cmd, col_addr, buffer, len);

    ret = nandflash_program_execute(id, row_addr);

    norflaship_cmd_done(id);

    return ret;
}

enum NANDFLASH_RET_T nandflash_write_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint32_t offset;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page_offset_len(id, 0, len);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    offset = 0;
    return nandflash_write_page_offset(id, block, page, offset, buffer, len);
}

enum NANDFLASH_RET_T nandflash_write_oob(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint32_t offset;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_oob_offset_len(id, 0, len);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    offset = flash_list[flash_idx[id]]->page_size;
    return nandflash_write_page_offset(id, block, page, offset, buffer, len);
}

static enum NANDFLASH_RET_T nandflash_update_page_offset(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint8_t cmd;
    uint32_t row_addr, col_addr;

    if (nandflash_op_mode[id] & NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        cmd = GD5F4GQ4_CMD_QUAD_PROGRAM_LOAD_RANDOM;
    } else {
        cmd = GD5F4GQ4_CMD_PROGRAM_LOAD_RANDOM;
    }

    row_addr = nandflash_block_page_to_row_addr(block, page);

    nandflash_select_die(id, block);

    ret = nandflash_start_page_read(id, row_addr);
    if (ret != NANDFLASH_RET_OK && ret != NANDFLASH_RET_READ_CORRECTED) {
        return ret;
    }

    norflaship_cmd_addr(id, GD5F4GQ4_CMD_WRITE_ENABLE, 0);

    col_addr = nandflash_block_page_offset_to_col_addr(id, block, offset);
    nandflash_program_load(id, cmd, col_addr, buffer, len);

    ret = nandflash_program_execute(id, row_addr);

    norflaship_cmd_done(id);

    return ret;
}

enum NANDFLASH_RET_T nandflash_update_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page_offset_len(id, offset, len);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    return nandflash_update_page_offset(id, block, page, offset, buffer, len);
}

enum NANDFLASH_RET_T nandflash_update_oob(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_oob_offset_len(id, offset, len);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    offset += flash_list[flash_idx[id]]->page_size;
    return nandflash_update_page_offset(id, block, page, offset, buffer, len);
}

enum NANDFLASH_RET_T nandflash_copy_page(enum HAL_FLASH_ID_T id, uint32_t dst_block, uint32_t dst_page, uint32_t src_block, uint32_t src_page)
{
    enum NANDFLASH_RET_T ret;
    uint32_t row_addr;
    uint32_t src_die, dst_die;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, dst_block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, dst_page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_block(id, src_block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, src_page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }

    src_die = nandflash_block_to_die(id, src_block);
    dst_die = nandflash_block_to_die(id, dst_block);
    if (src_die == dst_die) {
        row_addr = nandflash_block_page_to_row_addr(src_block, src_page);

        nandflash_select_die(id, src_block);

        ret = nandflash_start_page_read(id, row_addr);
        if (ret != NANDFLASH_RET_OK && ret != NANDFLASH_RET_READ_CORRECTED) {
            return ret;
        }

        row_addr = nandflash_block_page_to_row_addr(dst_block, dst_page);

        nandflash_select_die(id, dst_block);

        norflaship_cmd_addr(id, GD5F4GQ4_CMD_WRITE_ENABLE, 0);

        ret = nandflash_program_execute(id, row_addr);

        norflaship_cmd_done(id);
    } else {
        ASSERT(0, "cann't copy from different dir/plane through flash cache buffer");
        ret = NANDFLASH_RET_ERR;
    }

    return ret;
}

static enum NANDFLASH_RET_T nandflash_read_raw_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint32_t row_addr, col_addr;
    uint32_t remains;

    row_addr = nandflash_block_page_to_row_addr(block, page);

    nandflash_select_die(id, block);

    ret = nandflash_start_page_read(id, row_addr);
    norflaship_cmd_done(id);
    if (ret != NANDFLASH_RET_OK && ret != NANDFLASH_RET_READ_CORRECTED) {
        return ret;
    }

#ifdef FLASH_DMA
    remains = nandflash_start_rx_dma(id, buffer, len);

    if (remains < len) {
        uint32_t dma_len = len - remains;

        norflaship_busy_wait(id);
        // Rx word size
        norflaship_dma_rx_size(id, 2);

        col_addr = nandflash_block_page_offset_to_col_addr(id, block, offset);
        nandflash_read_from_cache(id, col_addr, dma_len);

        offset += dma_len;
        buffer += dma_len;

        nandflash_dma_busy_wait(id);
        norflaship_busy_wait(id);
        // Rx byte size
        norflaship_dma_rx_size(id, 0);
    }
#else
    remains = len;
#endif

    if (remains) {
#ifdef BUFFER_READ
        uint32_t index = 0;
        uint8_t val;
        norflaship_clear_rxfifo(id);
        col_addr = nandflash_block_page_offset_to_col_addr(id, block, offset);
        nandflash_read_from_cache(id, col_addr, remains);
        while (1) {
            norflaship_rxfifo_empty_wait(id);

            val = norflaship_read_rxfifo(id);
            if (buffer) {
                buffer[index] = val;
            }

            ++index;
            if (index >= remains) {
                break;
            }
        }
#else
        col_addr = nandflash_block_page_offset_to_col_addr(id, block, offset);
        memcpy(buffer, (uint8_t *)(NANDFLASH_NC_BASE + col_addr), remains);
#endif
    }

    return ret;
}

enum NANDFLASH_RET_T nandflash_read_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page_offset_len(id, offset, len);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    ret = nandflash_read_raw_page(id, block, page, offset, buffer, len);

    return ret;
}

enum NANDFLASH_RET_T nandflash_read_oob(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_page(id, page);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    ret = nandflash_check_oob_offset_len(id, offset, len);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    offset += flash_list[flash_idx[id]]->page_size;
    ret = nandflash_read_raw_page(id, block, page, offset, buffer, len);

    return ret;
}

static int nandflash_raw_block_is_bad(enum HAL_FLASH_ID_T id, uint32_t block)
{
    enum NANDFLASH_RET_T ret;
    uint32_t page;
    uint8_t mark[2];

    nandflash_select_die(id, block);
    for (page = 0; page < 2; page++) {
#ifdef BUFFER_READ
        ret = nandflash_read_raw_page(id, block, page, flash_list[flash_idx[id]]->page_size, mark+page, 1);
#else
        uint32_t row_addr, col_addr;
        row_addr = nandflash_block_page_to_row_addr(block, page);
        col_addr = nandflash_block_page_offset_to_col_addr(id, block, flash_list[flash_idx[id]]->page_size);
        ret = nandflash_start_page_read(id, row_addr);
        norflaship_cmd_done(id);
        mark[page] = *(volatile uint8_t *)(NANDFLASH_NC_BASE + col_addr);
#endif
        if (ret != NANDFLASH_RET_OK && ret != NANDFLASH_RET_READ_CORRECTED) {
            return true;
        }
    }

    if (mark[0] != 0xFF || mark[1] != 0xFF) {
        return true;
    }

    return false;
}

int nandflash_block_is_bad(enum HAL_FLASH_ID_T id, uint32_t block)
{
    enum NANDFLASH_RET_T ret;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return true;
    }

    return nandflash_raw_block_is_bad(id, block);
}

enum NANDFLASH_RET_T nandflash_mark_bad_block(enum HAL_FLASH_ID_T id, uint32_t block)
{
    enum NANDFLASH_RET_T ret;
    uint32_t offset;
    uint8_t mark;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    ret = nandflash_check_block(id, block);
    if (ret != NANDFLASH_RET_OK) {
        return ret;
    }

    offset = flash_list[flash_idx[id]]->page_size;
    mark = 0;

    nandflash_erase_raw_block(id, block);
    nandflash_write_page_offset(id, block, 0, offset, &mark, 1);
    nandflash_write_page_offset(id, block, 1, offset, &mark, 1);

    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_erase(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint32_t block_size;
    uint32_t block;
    uint32_t start;
    uint32_t end;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    if (offset >= flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_OFFSET;
    }
    if (offset + len > flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_LEN;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    block_size = flash_list[flash_idx[id]]->block_size;
    block = offset >> get_msb_pos(block_size);
    start = offset & ~(block_size - 1);
    end = offset + len;

    while (start < end) {
        ret = nandflash_erase_raw_block(id, block);
        if (ret != NANDFLASH_RET_OK) {
            return ret;
        }
        start += block_size;
    }

    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_write(enum HAL_FLASH_ID_T id, uint32_t offset, const uint8_t *buffer, uint32_t len)
{
    enum NANDFLASH_RET_T ret;
    uint32_t block_size;
    uint32_t page_size;
    uint32_t pages_per_block;
    uint32_t block;
    uint32_t page;
    uint32_t page_offset;
    const uint8_t *op_buf;
    uint32_t op_size;
    uint32_t remain_len;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    if (offset >= flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_OFFSET;
    }
    if (offset + len > flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_LEN;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    block_size = flash_list[flash_idx[id]]->block_size;
    page_size = flash_list[flash_idx[id]]->page_size;
    pages_per_block = block_size / page_size;

    block = offset >> get_msb_pos(block_size);
    page = (offset & (block_size - 1)) >> get_msb_pos(page_size);
    page_offset = offset & (page_size - 1);

    op_buf = buffer;
    remain_len = len;

    while (remain_len > 0) {
        op_size = page_size - page_offset;
        if (op_size > remain_len) {
            op_size = remain_len;
        }
        ret = nandflash_write_page_offset(id, block, page, page_offset, op_buf, op_size);
        if (ret != NANDFLASH_RET_OK) {
            return ret;
        }
        if (page_offset > 0) {
            page_offset = 0;
        }
        page++;
        if (page >= pages_per_block) {
            page = 0;
            block++;
        }
        op_buf += op_size;
        remain_len -= op_size;
    }

    return NANDFLASH_RET_OK;
}

static enum NANDFLASH_RET_T nandflash_skip_bad_block(enum HAL_FLASH_ID_T id, uint32_t *p_offset, uint32_t limit,
                                                     uint32_t len, uint32_t *p_block, uint32_t block_size)
{
    bool bad_block;

    while (1) {
        bad_block = nandflash_raw_block_is_bad(id, *p_block);
        if (bad_block) {
            (*p_offset) += block_size;
            if ((*p_offset) + len > limit) {
                return NANDFLASH_RET_LIMIT_REACHED;
            }
            (*p_block)++;
            continue;
        }
        break;
    }

    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_read_raw(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t limit, uint8_t *buffer, uint32_t len, bool skip_bad)
{
    enum NANDFLASH_RET_T ret;
    uint32_t block_size;
    uint32_t page_size;
    uint32_t pages_per_block;
    uint32_t block;
    uint32_t page;
    uint32_t page_offset;
    uint8_t *op_buf;
    uint32_t op_size;
    uint32_t remain_len;
    bool read_corrected = false;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    if (limit == 0) {
        limit = offset + len;
    }
    if (offset >= flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_OFFSET;
    }
    if (offset >= limit) {
        return NANDFLASH_RET_BAD_OFFSET;
    }
    if (limit > flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_LIMIT;
    }
    if (offset + len > limit) {
        return NANDFLASH_RET_BAD_LEN;
    }
    if (len == 0) {
        return NANDFLASH_RET_OK;
    }

    block_size = flash_list[flash_idx[id]]->block_size;
    page_size = flash_list[flash_idx[id]]->page_size;
    pages_per_block = block_size / page_size;

    block = offset >> get_msb_pos(block_size);
    page = (offset & (block_size - 1)) >> get_msb_pos(page_size);
    page_offset = offset & (page_size - 1);

    op_buf = buffer;
    remain_len = len;

    if (skip_bad) {
        ret = nandflash_skip_bad_block(id, &offset, limit, len, &block, block_size);
        if (ret != NANDFLASH_RET_OK) {
            return ret;
        }
    }

    while (remain_len > 0) {
        op_size = page_size - page_offset;
        if (op_size > remain_len) {
            op_size = remain_len;
        }
        ret = nandflash_read_raw_page(id, block, page, offset, op_buf, op_size);
        if (ret != NANDFLASH_RET_OK && ret != NANDFLASH_RET_READ_CORRECTED) {
            return ret;
        }
        if (ret == NANDFLASH_RET_READ_CORRECTED) {
            read_corrected = true;
        }
        if (page_offset > 0) {
            page_offset = 0;
        }
        page++;
        if (page >= pages_per_block) {
            page = 0;
            block++;
            if (skip_bad) {
                ret = nandflash_skip_bad_block(id, &offset, limit, len, &block, block_size);
                if (ret != NANDFLASH_RET_OK) {
                    return ret;
                }
            }
        }
        op_buf += op_size;
        remain_len -= op_size;
    }

    return read_corrected ? NANDFLASH_RET_READ_CORRECTED : NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_read(enum HAL_FLASH_ID_T id, uint32_t offset, uint8_t *buffer, uint32_t len)
{
    return nandflash_read_raw(id, offset, 0, buffer, len, false);
}

enum NANDFLASH_RET_T nandflash_load_valid_data(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t limit, uint8_t *buffer, uint32_t len)
{
    return nandflash_read_raw(id, offset, limit, buffer, len, true);
}

int nandflash_busy(enum HAL_FLASH_ID_T id)
{
    return norflaship_is_busy(id);
}

static void nandflash_set_block_protection(enum HAL_FLASH_ID_T id, uint32_t bp)
{
    uint8_t prt;
    uint32_t bp_mask;

    bp_mask = nandflash_get_block_protect_mask(id);

    prt = nandflash_read_protection(id);
    prt = (prt & ~bp_mask) | (bp & bp_mask);
    nandflash_write_protection(id, prt);
}

POSSIBLY_UNUSED
static void nandflash_init_protection(enum HAL_FLASH_ID_T id)
{
    nandflash_set_block_protection(id, 0);
}

static uint32_t nandflash_get_bp_cfg(enum HAL_FLASH_ID_T id)
{
    uint32_t val = 0;

    // Set BP and CMP bits
    if (flash_list[flash_idx[id]]->die_num) {
        // TODO: Set different BP value for each die
        return 0;
    }

    if (FLASH_SIZE != flash_list[flash_idx[id]]->total_size) {
        return val;
    }

    return val;
}

enum NANDFLASH_RET_T nandflash_enable_protection(enum HAL_FLASH_ID_T id)
{
    if (!nandflash_ctx[id].opened) {
        return NANDFLASH_RET_NOT_OPENED;
    }

    nandflash_set_block_protection(id, nandflash_get_bp_cfg(id));

    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_disable_protection(enum HAL_FLASH_ID_T id)
{
    if (!nandflash_ctx[id].opened) {
        return NANDFLASH_RET_NOT_OPENED;
    }

    nandflash_set_block_protection(id, 0);

    return NANDFLASH_RET_OK;
}

#ifdef FLASH_SECURITY_REGISTER
static union NANDFLASH_SEC_REG_CFG_T nandflash_get_security_register_config(enum HAL_FLASH_ID_T id)
{
    return flash_list[flash_idx[id]]->sec_reg_cfg;
}

int nandflash_security_register_is_locked(enum HAL_FLASH_ID_T id)
{
    union NANDFLASH_SEC_REG_CFG_T sec_cfg;
    uint8_t cfg;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    sec_cfg = nandflash_get_security_register_config(id);
    if (!sec_cfg.s.enabled) {
        return false;
    }

    cfg = nandflash_read_configuration(id);

    return !!(cfg & GD5F4GQ4_CFG_OTP_PRT);
}

enum NANDFLASH_RET_T nandflash_security_register_lock(enum HAL_FLASH_ID_T id)
{
    uint8_t cfg;
    uint8_t backup_cfg;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    cfg = nandflash_read_configuration(id);
    backup_cfg = cfg;
    cfg |= (GD5F4GQ4_CFG_OTP_PRT | GD5F4GQ4_CFG_OTP_EN);
    nandflash_write_configuration(id, cfg);

    nandflash_program_execute(id, 0); // execute otp lock bit

    backup_cfg &= ~(GD5F4GQ4_CFG_OTP_PRT);
    nandflash_write_configuration(id, backup_cfg);

    return NANDFLASH_RET_OK;
}

uint32_t nandflash_security_register_enable(enum HAL_FLASH_ID_T id)
{
    uint32_t mode;
    uint8_t cfg;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    cfg = nandflash_read_configuration(id);
    mode = !!(cfg & GD5F4GQ4_CFG_OTP_EN);
    if (!mode) {
        cfg |= GD5F4GQ4_CFG_OTP_EN;
        nandflash_write_configuration(id, cfg);
        norflaship_busy_wait(id);
        norflaship_clear_rxfifo(id);
        norflaship_busy_wait(id);
    }

    return mode;
}

void nandflash_security_register_disable(enum HAL_FLASH_ID_T id, uint32_t mode)
{
    uint8_t cfg;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    if (!mode) {
        norflaship_busy_wait(id);
        cfg = nandflash_read_configuration(id);
        cfg &= ~GD5F4GQ4_CFG_OTP_EN;
        nandflash_write_configuration(id, cfg);
        norflaship_clear_rxfifo(id);
        norflaship_busy_wait(id);
    }
}

enum NANDFLASH_RET_T nandflash_security_register_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    return nandflash_read_page(id, 0, start_address/flash_list[flash_idx[id]]->page_size, 0, buffer, len);
}

enum NANDFLASH_RET_T nandflash_security_register_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    return nandflash_write_page(id, 0, start_address/flash_list[flash_idx[id]]->page_size, buffer, len);
}
#endif

int nandflash_opened(enum HAL_FLASH_ID_T id)
{
    return nandflash_ctx[id].opened;
}

enum NANDFLASH_RET_T nandflash_get_open_state(enum HAL_FLASH_ID_T id)
{
    return nandflash_ctx[id].open_state;
}

enum NANDFLASH_RET_T nandflash_get_size(enum HAL_FLASH_ID_T id, uint32_t *total_size, uint32_t *block_size,
                                        uint32_t *page_size, uint32_t *page_spare_size, uint32_t *die_num)
{
    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    if (total_size) {
        *total_size  = flash_list[flash_idx[id]]->total_size;
    }
    if (block_size) {
        *block_size  = flash_list[flash_idx[id]]->block_size;
    }
    if (page_size) {
        *page_size   = flash_list[flash_idx[id]]->page_size;
    }
    if (page_spare_size) {
        *page_spare_size = flash_list[flash_idx[id]]->page_spare_size;
    }
    if (die_num) {
        *die_num = flash_list[flash_idx[id]]->die_num;
    }

    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_get_boundary(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t *block_boundary, uint32_t *page_boundary)
{
    uint32_t block;
    uint32_t remain;

    ASSERT(nandflash_ctx[id].opened, err_not_opened);

    if (offset >= flash_list[flash_idx[id]]->total_size) {
        return NANDFLASH_RET_BAD_OFFSET;
    }

    block = offset / flash_list[flash_idx[id]]->block_size;
    remain = offset % flash_list[flash_idx[id]]->block_size;

    if (block_boundary) {
        *block_boundary  = block;
    }
    if (page_boundary) {
        *page_boundary = remain / flash_list[flash_idx[id]]->page_size;
    }

    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_get_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    len = MIN(len, sizeof(nandflash_ctx[id].device_id));

    memcpy(value, nandflash_ctx[id].device_id, len);

    return NANDFLASH_RET_OK;
}

static enum HAL_CMU_FREQ_T nandflash_clk_to_cmu_freq(uint32_t clk)
{
    enum HAL_CMU_FREQ_T freq;

    if (clk >= NANDFLASH_SPEED_208M) {
        freq = HAL_CMU_FREQ_208M;
    } else if (clk >= NANDFLASH_SPEED_104M) {
        freq = HAL_CMU_FREQ_104M;
    } else if (clk >= NANDFLASH_SPEED_78M) {
        freq = HAL_CMU_FREQ_78M;
    } else if (clk >= NANDFLASH_SPEED_52M) {
        freq = HAL_CMU_FREQ_52M;
    } else {
        freq = HAL_CMU_FREQ_26M;
    }

#ifdef SIMU
#ifdef SIMU_FAST_FLASH
#define MAX_SIMU_FLASH_FREQ     HAL_CMU_FREQ_104M
#else
#define MAX_SIMU_FLASH_FREQ     HAL_CMU_FREQ_52M
#endif
    if (freq > MAX_SIMU_FLASH_FREQ) {
        freq = MAX_SIMU_FLASH_FREQ;
    }
#endif

    return freq;
}

static void _nandflash_set_source_freq(enum HAL_FLASH_ID_T id, enum HAL_CMU_FREQ_T freq)
{
#ifdef FLASH1_CTRL_BASE
    if (id == HAL_FLASH_ID_1) {
        hal_cmu_flash1_set_freq(freq);
        return;
    }
#endif
    hal_cmu_flash_set_freq(freq);
}

static void nandflash_reset_timing(enum HAL_FLASH_ID_T id)
{
    const uint32_t default_div = 8;

    // Restore default divider
    norflaship_div(id, default_div);
    nandflash_init_sample_delay_by_div(id, default_div);
}

static void nandflash_init_die_cfg(enum HAL_FLASH_ID_T id)
{
    if (flash_list[flash_idx[id]]->die_num) {
        nandflash_ctx[id].blocks_per_die = flash_list[flash_idx[id]]->total_size / flash_list[flash_idx[id]]->block_size / flash_list[flash_idx[id]]->die_num;
        nandflash_ctx[id].cur_block = 0;
        nandflash_ctx[id].cur_die = 0;
    } else {
        nandflash_ctx[id].blocks_per_die = 0;
    }
}

static void nandflash_init_dummy_cycle(enum HAL_FLASH_ID_T id)
{
    if (flash_list[flash_idx[id]]->dummy_cycle_cfg.s.quad_io_en) {
        norflaship_dummy_n_rcqio(id, flash_list[flash_idx[id]]->dummy_cycle_cfg.s.quad_io_val);
    }
}

enum NANDFLASH_RET_T nandflash_open(enum HAL_FLASH_ID_T id, const struct NANDFLASH_CONFIG_T *cfg)
{
    enum HAL_CMU_FREQ_T source_freq;
    int result;
    uint32_t op;
    int found;

    // Place the config into ram
    if (cfg == NULL) {
        return NANDFLASH_RET_CFG_NULL;
    }

    if (id == HAL_FLASH_ID_0) {
        hal_cmu_clock_enable(HAL_CMU_MOD_O_FLASH);
        hal_cmu_clock_enable(HAL_CMU_MOD_H_FLASH);
        hal_cmu_reset_clear(HAL_CMU_MOD_O_FLASH);
        hal_cmu_reset_clear(HAL_CMU_MOD_H_FLASH);
    } else {
        hal_cmu_clock_enable(HAL_CMU_MOD_O_FLASH1);
        hal_cmu_clock_enable(HAL_CMU_MOD_H_FLASH1);
        hal_cmu_reset_clear(HAL_CMU_MOD_O_FLASH1);
        hal_cmu_reset_clear(HAL_CMU_MOD_H_FLASH1);
    }

    norflaship_busy_wait(id);

    norflaship_nand_sel(id, true);
    norflaship_dmactrl_tx_en(id, false);
    norflaship_dmactrl_rx_en(id, false);
#ifdef FLASH_DMA
    norflaship_dma_tx_threshold(id, NORFLASHIP_TXFIFO_SIZE / 2);
    norflaship_dma_rx_threshold(id, NORFLASHIP_RXFIFO_SIZE / 2);
#endif

    // Reset states
    nandflash_ctx[id].opened = false;
    nandflash_ctx[id].open_state = NANDFLASH_RET_NOT_OPENED;

    // Set the direction of 4 IO pins to output when in idle
    norflaship_set_idle_io_dir(id, 0);

    // Reset norflash source clock
    _nandflash_set_source_freq(id, HAL_CMU_FREQ_26M);

    // Reset controller timing
    nandflash_reset_timing(id);

    // Reset norflash in slow clock configuration
    nandflash_reset(id);

    // Get device ID
    nandflash_read_device_id(id, nandflash_ctx[id].device_id, sizeof(nandflash_ctx[id].device_id));

    // For each driver in array, match chip and select drv_ops
    found = nandflash_match_chip(id, nandflash_ctx[id].device_id, sizeof(nandflash_ctx[id].device_id));
    if (!found) {
        result = NANDFLASH_RET_BAD_ID;
        goto _exit;
    }

    nandflash_init_die_cfg(id);

    // Init dummy cycle
    nandflash_init_dummy_cycle(id);

    // Set norflash source clock
    source_freq = nandflash_clk_to_cmu_freq(cfg->source_clk);
    _nandflash_set_source_freq(id, source_freq);

    /* over write config */
    if (cfg->override_config) {
        /* div */
        norflaship_div(id, cfg->div);
        /* sample delay */
        norflaship_samdly(id, cfg->samdly);
        /* ru en */
        norflaship_ruen(id, cfg->spiruen);
        /* rd en */
        norflaship_rden(id, cfg->spirden);
        /* rd cmd */
        norflaship_rdcmd(id, cfg->rdcmd);
        /* frd cmd */
        norflaship_frdcmd(id, cfg->frdcmd);
        /* qrd cmd */
        norflaship_qrdcmd(id, cfg->qrdcmd);
    } else {
        // Init divider
        result = nandflash_init_div(id, cfg);
        if (result != 0) {
            result = NANDFLASH_RET_BAD_DIV;
            goto _exit;
        }

        // Calib with the new divider
        result = nandflash_sample_delay_calib(id, NANDFLASH_CALIB_FLASH_ID);
        if (result != 0) {
            result = NANDFLASH_RET_BAD_CALIB_ID;
            goto _exit;
        }
    }

#if defined(PROGRAMMER) || defined(OTA_PROGRAMMER)
    nandflash_init_protection(id);
#else
    nandflash_set_block_protection(id, nandflash_get_bp_cfg(id));
#endif

    op = cfg->mode;

    // Divider will be set to normal read mode
    result = nandflash_set_mode(id, op);
    if (result != 0) {
        result = NANDFLASH_RET_BAD_OP;
        goto _exit;
    }

#if 0
    result = nandflash_sample_delay_calib(id, NANDFLASH_CALIB_MAGIC_WORD);
    if (result != 0) {
        result = NANDFLASH_RET_BAD_CALIB_MAGIC;
        goto _exit;
    }
#endif

    // Init cfg for each die
    if (flash_list[flash_idx[id]]->die_num) {
        uint8_t prt_val;
        uint8_t cfg_val;

        // TODO: Set different BP value for each die
        prt_val = nandflash_read_protection(id);
        cfg_val = nandflash_read_configuration(id);
        for (uint32_t i = 1; i < flash_list[flash_idx[id]]->die_num; i++) {
            nandflash_software_die_select(id, i);
            nandflash_write_protection(id, prt_val);
            nandflash_write_configuration(id, cfg_val);
        }
        nandflash_software_die_select(id, 0);
    }

    nandflash_ctx[id].opened = true;

    result = NANDFLASH_RET_OK;

_exit:
    if (result != NANDFLASH_RET_OK) {
        _nandflash_set_source_freq(id, HAL_CMU_FREQ_26M);
        nandflash_reset_timing(id);
        // Flash might be accessed again
    }

    nandflash_ctx[id].open_state = result;

    return result;
}

enum NANDFLASH_RET_T nandflash_close(enum HAL_FLASH_ID_T id)
{
    return NANDFLASH_RET_OK;
}

enum NANDFLASH_RET_T nandflash_init(enum HAL_FLASH_ID_T id)
{
    enum NANDFLASH_RET_T ret;
    const struct NANDFLASH_CONFIG_T *cfg = &nandflash_cfg;

    ret = nandflash_open(id, cfg);

    return ret;
}

#endif

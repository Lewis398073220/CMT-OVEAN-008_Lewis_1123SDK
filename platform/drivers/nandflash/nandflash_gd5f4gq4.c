/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "nandflash_drv.h"
#include "nandflash_gd5f4gq4.h"

// ----------------------
// GigaDevice
// ----------------------

const struct NANDFLASH_CFG_T gd5f4gq4_cfg = {
    .id = { 0xC8, 0xF4, 0xC8, },
    .ecc_status_type = NANDFLASH_ECC_ST_2BITS_11_COR,
    .block_protect_mask = 0x3E,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x00,
            .page_num = 0x04,
        },
    },
    .page_size = GD5F4GQ4_PAGE_SIZE,
    .page_spare_size = GD5F4GQ4_PAGE_SPARE_SIZE,
    .block_size = GD5F4GQ4_BLOCK_SIZE,
    .total_size = GD5F4GQ4_TOTAL_SIZE,
    .max_speed = 120 * 1000 * 1000,
    .mode = (NANDFLASH_OP_MODE_STAND_SPI |
                NANDFLASH_OP_MODE_FAST_SPI |
                NANDFLASH_OP_MODE_DUAL_OUTPUT |
                NANDFLASH_OP_MODE_DUAL_IO |
                NANDFLASH_OP_MODE_QUAD_OUTPUT |
                NANDFLASH_OP_MODE_QUAD_IO |
                NANDFLASH_OP_MODE_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_INTERNAL_ECC),
};

const struct NANDFLASH_CFG_T gd5f4gm5_cfg = {
    .id = { 0xB4, 0x68, 0xC8, },
    .ecc_status_type = NANDFLASH_ECC_ST_3BITS_111_BAD,
    .block_protect_mask = 0x3E,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x00,
            .page_num = 0x04,
        },
    },
    .page_size = GD5F4GM5_PAGE_SIZE,
    .page_spare_size = GD5F4GM5_PAGE_SPARE_SIZE,
    .block_size = GD5F4GM5_BLOCK_SIZE,
    .total_size = GD5F4GM5_TOTAL_SIZE,
    .max_speed = 120 * 1000 * 1000,
    .mode = (NANDFLASH_OP_MODE_STAND_SPI |
                NANDFLASH_OP_MODE_FAST_SPI |
                NANDFLASH_OP_MODE_DUAL_OUTPUT |
                NANDFLASH_OP_MODE_QUAD_OUTPUT |
                NANDFLASH_OP_MODE_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_INTERNAL_ECC |
                NANDFLASH_OP_MODE_READ_ADDR_24BIT),
};

// ----------------------
// XTS
// ----------------------

const struct NANDFLASH_CFG_T xt26g02a_cfg = {
    .id = { 0x0B, 0xE2, 0x0B, },
    .ecc_status_type = NANDFLASH_ECC_ST_2BITS_11_COR,
    .block_protect_mask = 0x3E,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x00,
            .page_num = 0x04,
        },
    },
    .page_size = GD5F4GQ4_PAGE_SIZE,
    .page_spare_size = GD5F4GQ4_PAGE_SPARE_SIZE,
    .block_size = GD5F4GQ4_BLOCK_SIZE,
    .total_size = XT26G02A_TOTAL_SIZE,
    .max_speed = 90 * 1000 * 1000,
    .mode = (NANDFLASH_OP_MODE_STAND_SPI |
                NANDFLASH_OP_MODE_FAST_SPI |
                NANDFLASH_OP_MODE_DUAL_OUTPUT |
                NANDFLASH_OP_MODE_DUAL_IO |
                NANDFLASH_OP_MODE_QUAD_OUTPUT |
                NANDFLASH_OP_MODE_QUAD_IO |
                NANDFLASH_OP_MODE_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_INTERNAL_ECC),
};

// ----------------------
// Macronix
// ----------------------

const struct NANDFLASH_CFG_T mx35lf1g_cfg = {
    .id = { 0xC2, 0x12, 0xC2, },
    .ecc_status_type = NANDFLASH_ECC_ST_2BITS_11_RES,
    .block_protect_mask = 0x3E,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x02,
            .page_num = 0x1E,
        },
    },
    .page_size = GD5F4GQ4_PAGE_SIZE,
    .page_spare_size = GD5F4GQ4_PAGE_SPARE_SIZE,
    .block_size = GD5F4GQ4_BLOCK_SIZE,
    .total_size = MX35LF1G_TOTAL_SIZE,
    .max_speed = 104 * 1000 * 1000,
    .mode = (NANDFLASH_OP_MODE_STAND_SPI |
                NANDFLASH_OP_MODE_FAST_SPI |
                NANDFLASH_OP_MODE_DUAL_OUTPUT |
                NANDFLASH_OP_MODE_QUAD_OUTPUT |
                NANDFLASH_OP_MODE_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_INTERNAL_ECC),
};

// ----------------------
// Dosilicon
// ----------------------

const struct NANDFLASH_CFG_T ds35m2g_cfg = {
    .id = { 0xE5, 0xA2, 0xE5, },
    .plane_cfg = 0x2c, // plane_num:0x2, plane_offset:0xc
    .ecc_status_type = NANDFLASH_ECC_ST_3BITS_010_BAD,
    .block_protect_mask = 0x3C,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x02,
            .page_num = 0x1E,
        },
    },
    .page_size = DS35X2GB_PAGE_SIZE,
    .page_spare_size = DS35X2GB_PAGE_SPARE_SIZE,
    .block_size = DS35X2GB_BLOCK_SIZE,
    .total_size = DS35X2GB_TOTAL_SIZE,
    .max_speed = 83 * 1000 * 1000,
    .mode = (NANDFLASH_OP_MODE_STAND_SPI |
                NANDFLASH_OP_MODE_FAST_SPI |
                NANDFLASH_OP_MODE_DUAL_OUTPUT |
                NANDFLASH_OP_MODE_QUAD_OUTPUT |
                NANDFLASH_OP_MODE_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_INTERNAL_ECC),
};
const struct NANDFLASH_CFG_T ds35m1g_cfg = {
    .id = { 0xE5, 0x21, 0xE5, },
    .ecc_status_type = NANDFLASH_ECC_ST_2BITS_11_RES,
    .block_protect_mask = 0x3C,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x02,
            .page_num = 0x1E,
        },
    },
    .page_size = DS35X1GB_PAGE_SIZE,
    .page_spare_size = DS35X1GB_PAGE_SPARE_SIZE,
    .block_size = DS35X1GB_BLOCK_SIZE,
    .total_size = DS35X1GB_TOTAL_SIZE,
    .max_speed = 104 * 1000 * 1000,
    .mode = (NANDFLASH_OP_MODE_STAND_SPI |
                NANDFLASH_OP_MODE_FAST_SPI |
                NANDFLASH_OP_MODE_DUAL_OUTPUT |
                NANDFLASH_OP_MODE_QUAD_OUTPUT |
                NANDFLASH_OP_MODE_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM |
                NANDFLASH_OP_MODE_INTERNAL_ECC),
};


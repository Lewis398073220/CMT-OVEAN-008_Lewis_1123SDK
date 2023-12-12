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
#include "nandflash_w25m02gv.h"

// ----------------------
// Winbond
// ----------------------

const struct NANDFLASH_CFG_T w25m02gv_cfg = {
    .id = { 0xEF, 0xAB, 0x21, },
    .die_num = 2,
    .ecc_status_type = NANDFLASH_ECC_ST_2BITS_11_RES,
    .block_protect_mask = 0x7C,
    .sec_reg_cfg = {
        .s = {
            .enabled = true,
            .page_start = 0x02,
            .page_num = 0x0A,
        },
    },
    .dummy_cycle_cfg = {
        .s = {
            .quad_io_en = true,
            .quad_io_val = 0x04,
        },
    },
    .page_size = W25M02GV_PAGE_SIZE,
    .page_spare_size = W25M02GV_PAGE_SPARE_SIZE,
    .block_size = W25M02GV_BLOCK_SIZE,
    .total_size = W25M02GV_TOTAL_SIZE,
    .max_speed = 104 * 1000 * 1000,
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


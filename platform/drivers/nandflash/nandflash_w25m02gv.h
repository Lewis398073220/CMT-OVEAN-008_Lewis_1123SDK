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
#ifndef __NANDFLASH_W25M02GV_H__
#define __NANDFLASH_W25M02GV_H__

#include "plat_types.h"

#define W25M02GV_PAGE_SIZE (2*1024)
#define W25M02GV_PAGE_SPARE_SIZE (64)
#define W25M02GV_BLOCK_SIZE (128*1024)
#define W25M02GV_TOTAL_SIZE (256*1024*1024)

/* device cmd */
#define W25M02GV_CMD_ID 0x9F
#define W25M02GV_CMD_WRITE_ENABLE 0x06
#define W25M02GV_CMD_WRITE_DISABLE 0x04
#define W25M02GV_CMD_PROGRAM_LOAD 0x02
#define W25M02GV_CMD_QUAD_PROGRAM_LOAD 0x32
#define W25M02GV_CMD_PROGRAM_EXECUTRE 0x10
#define W25M02GV_CMD_PROGRAM_LOAD_RANDOM 0x84
#define W25M02GV_CMD_QUAD_PROGRAM_LOAD_RANDOM 0x34
#define W25M02GV_CMD_BLOCK_ERASE 0xD8
#define W25M02GV_CMD_GET_FEATURE 0x0F
#define W25M02GV_CMD_SET_FEATURE 0x1F

#define W25M02GV_CMD_PAGE_READ 0x13
#define W25M02GV_CMD_FAST_QUAD_IO_READ 0xEB
#define W25M02GV_CMD_FAST_QUAD_OUTPUT_READ 0x6B
#define W25M02GV_CMD_FAST_DUAL_IO_READ 0xBB
#define W25M02GV_CMD_FAST_DUAL_OUTPUT_READ 0x3B
#define W25M02GV_CMD_STANDARD_READ 0x03
#define W25M02GV_CMD_STANDARD_FAST_READ 0x0B
#define W25M02GV_CMD_RESET 0xFF

#define W25M02GV_CMD_DIE_SEL 0xC2

#define W25M02GV_REG_PROTECTION             0xA0
#define W25M02GV_REG_CONFIGURATION          0xB0
#define W25M02GV_REG_STATUS                 0xC0

// W25M02GV_PRT
#define W25M02GV_PRT_SRP1                   (1 << 0)
#define W25M02GV_PRT_WP_E                   (1 << 1)
#define W25M02GV_PRT_TB                     (1 << 2)
#define W25M02GV_PRT_BP0_3_SHIFT            3
#define W25M02GV_PRT_BP0_3_MASK             (0xF << W25M02GV_PRT_BP0_3_SHIFT)
#define W25M02GV_PRT_BP0_3(n)               BITFIELD_VAL(W25M02GV_PRT_BP0_3, n)
#define W25M02GV_PRT_SRP0                   (1 << 7)

// W25M02GV_CFG
#define W25M02GV_CFG_BUF                    (1 << 3)
#define W25M02GV_CFG_ECC_EN                 (1 << 4)
#define W25M02GV_CFG_SR1_L                  (1 << 5)
#define W25M02GV_CFG_OTP_EN                 (1 << 6)
#define W25M02GV_CFG_OTP_PRT                (1 << 7)

// W25M02GV_ST
#define W25M02GV_ST_BUSY                    (1 << 0)
#define W25M02GV_ST_WEL                     (1 << 1)
#define W25M02GV_ST_E_FAIL                  (1 << 2)
#define W25M02GV_ST_P_FAIL                  (1 << 3)
#define W25M02GV_ST_ECCS_SHIFT              4
#define W25M02GV_ST_ECCS_MASK               (0x3 << W25M02GV_ST_ECCS_SHIFT)
#define W25M02GV_ST_ECCS(n)                 BITFIELD_VAL(W25M02GV_ST_ECCS, n)
#define W25M02GV_ST_LUT_F                   (1 << 3)

#endif

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
#ifndef __NANDFLASH_GD5F4GQ4_H__
#define __NANDFLASH_GD5F4GQ4_H__

#include "plat_types.h"

#define GD5F4GQ4_PAGE_SIZE (2*1024)
#define GD5F4GQ4_PAGE_SPARE_SIZE (64)
#define GD5F4GQ4_BLOCK_SIZE (128*1024)
#define GD5F4GQ4_TOTAL_SIZE (512*1024*1024)

#define XT26G02A_TOTAL_SIZE (256*1024*1024)

#define MX35LF1G_TOTAL_SIZE (128*1024*1024)

#define GD5F4GM5_PAGE_SIZE (4*1024)
#define GD5F4GM5_PAGE_SPARE_SIZE (256)
#define GD5F4GM5_BLOCK_SIZE (256*1024)
#define GD5F4GM5_TOTAL_SIZE (512*1024*1024)

#define DS35X2GB_PAGE_SIZE (2*1024)
#define DS35X2GB_PAGE_SPARE_SIZE (128)
#define DS35X2GB_BLOCK_SIZE (128*1024)
#define DS35X2GB_TOTAL_SIZE (512*1024*1024)

#define DS35X1GB_PAGE_SIZE (2*1024)
#define DS35X1GB_PAGE_SPARE_SIZE (64)
#define DS35X1GB_BLOCK_SIZE (128*1024)
#define DS35X1GB_TOTAL_SIZE (128*1024*1024)

/* device cmd */
#define GD5F4GQ4_CMD_ID 0x9F
#define GD5F4GQ4_CMD_WRITE_ENABLE 0x06
#define GD5F4GQ4_CMD_WRITE_DISABLE 0x04
#define GD5F4GQ4_CMD_PROGRAM_LOAD 0x02
#define GD5F4GQ4_CMD_QUAD_PROGRAM_LOAD 0x32
#define GD5F4GQ4_CMD_PROGRAM_EXECUTRE 0x10
#define GD5F4GQ4_CMD_PROGRAM_LOAD_RANDOM 0x84
#define GD5F4GQ4_CMD_QUAD_PROGRAM_LOAD_RANDOM 0x34
#define GD5F4GQ4_CMD_BLOCK_ERASE 0xD8
#define GD5F4GQ4_CMD_GET_FEATURE 0x0F
#define GD5F4GQ4_CMD_SET_FEATURE 0x1F

#define GD5F4GQ4_CMD_PAGE_READ 0x13
#define GD5F4GQ4_CMD_FAST_QUAD_IO_READ 0xEB
#define GD5F4GQ4_CMD_FAST_QUAD_OUTPUT_READ 0x6B
#define GD5F4GQ4_CMD_FAST_DUAL_IO_READ 0xBB
#define GD5F4GQ4_CMD_FAST_DUAL_OUTPUT_READ 0x3B
#define GD5F4GQ4_CMD_STANDARD_READ 0x03
#define GD5F4GQ4_CMD_STANDARD_FAST_READ 0x0B
#define GD5F4GQ4_CMD_RESET 0xFF

#define GD5F4GQ4_REG_PROTECTION             0xA0
#define GD5F4GQ4_REG_CONFIGURATION          0xB0
#define GD5F4GQ4_REG_STATUS                 0xC0
#define GD5F4GQ4_REG_DRIVER                 0xD0
#define GD5F4GQ4_REG_STATUS2                0xE0

// GD5F4GQ4_PRT
#define GD5F4GQ4_PRT_CMP                    (1 << 1)
#define GD5F4GQ4_PRT_INV                    (1 << 2)
#define GD5F4GQ4_PRT_BP0_2_SHIFT            3
#define GD5F4GQ4_PRT_BP0_2_MASK             (0x7 << GD5F4GQ4_PRT_BP0_2_SHIFT)
#define GD5F4GQ4_PRT_BP0_2(n)               BITFIELD_VAL(GD5F4GQ4_PRT_BP0_2, n)
#define GD5F4GQ4_PRT_BRWD                   (1 << 7)

// GD5F4GQ4_CFG
#define GD5F4GQ4_CFG_QE                     (1 << 0)
#define GD5F4GQ4_CFG_ECC_EN                 (1 << 4)
#define GD5F4GQ4_CFG_OTP_EN                 (1 << 6)
#define GD5F4GQ4_CFG_OTP_PRT                (1 << 7)

// GD5F4GQ4_ST
#define GD5F4GQ4_ST_OIP                     (1 << 0)
#define GD5F4GQ4_ST_WEL                     (1 << 1)
#define GD5F4GQ4_ST_E_FAIL                  (1 << 2)
#define GD5F4GQ4_ST_P_FAIL                  (1 << 3)
#define GD5F4GQ4_ST_ECCS_SHIFT              4
#define GD5F4GQ4_ST_ECCS_MASK               (0x3 << GD5F4GQ4_ST_ECCS_SHIFT)
#define GD5F4GQ4_ST_ECCS(n)                 BITFIELD_VAL(GD5F4GQ4_ST_ECCS, n)

#define GD5F4GM5_ST_ECCS_SHIFT              4
#define GD5F4GM5_ST_ECCS_MASK               (0x7 << GD5F4GM5_ST_ECCS_SHIFT)
#define GD5F4GM5_ST_ECCS(n)                 BITFIELD_VAL(GD5F4GM5_ST_ECCS, n)

// GD5F4GQ4_DR
#define GD5F4GQ4_DR_DS_S_SHIFT              5
#define GD5F4GQ4_DR_DS_S_MASK               (0x3 << GD5F4GQ4_DR_DS_S_SHIFT)
#define GD5F4GQ4_DR_DS_S(n)                 BITFIELD_VAL(GD5F4GQ4_DR_DS_S, n)

// GD5F4GQ4_ST2
#define GD5F4GQ4_ST2_ECCSE_SHIFT            4
#define GD5F4GQ4_ST2_ECCSE_MASK             (0x3 << GD5F4GQ4_ST2_ECCSE_SHIFT)
#define GD5F4GQ4_ST2_ECCSE(n)               BITFIELD_VAL(GD5F4GQ4_ST2_ECCSE, n)

#endif

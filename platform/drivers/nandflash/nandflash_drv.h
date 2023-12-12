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
#ifndef __NANDFLASH_DRV_H__
#define __NANDFLASH_DRV_H__

#include "plat_types.h"
#include "hal_cmu.h"
#include "nandflash_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NANDFLASH_ID_LEN                    3

enum NANDFLASH_RET_T {
    NANDFLASH_RET_OK,
    NANDFLASH_RET_ERR,
    NANDFLASH_RET_BAD_ID,
    NANDFLASH_RET_BAD_DIV,
    NANDFLASH_RET_BAD_CALIB_ID,
    NANDFLASH_RET_BAD_CFG,
    NANDFLASH_RET_BAD_OP,
    NANDFLASH_RET_BAD_CALIB_MAGIC,
    NANDFLASH_RET_BAD_OFFSET,
    NANDFLASH_RET_BAD_BLOCK,
    NANDFLASH_RET_BAD_PAGE,
    NANDFLASH_RET_BAD_PAGE_OFFSET,
    NANDFLASH_RET_BAD_LEN,
    NANDFLASH_RET_BAD_LIMIT,
    NANDFLASH_RET_LIMIT_REACHED,
    NANDFLASH_RET_NOT_OPENED,
    NANDFLASH_RET_CFG_NULL,
    NANDFLASH_RET_READ_CORRECTED,
    NANDFLASH_RET_READ_ERR,
    NANDFLASH_RET_ERASE_ERR,
    NANDFLASH_RET_PROGRAM_ERR,
};

enum NANDFLASH_SPEED {
    NANDFLASH_SPEED_13M  = 13000000,
    NANDFLASH_SPEED_26M  = 26000000,
    NANDFLASH_SPEED_52M  = 52000000,
    NANDFLASH_SPEED_78M  = 78000000,
    NANDFLASH_SPEED_104M = 104000000,
    NANDFLASH_SPEED_130M = 130000000,
    NANDFLASH_SPEED_156M = 156000000,
    NANDFLASH_SPEED_182M = 182000000,
    NANDFLASH_SPEED_208M = 208000000,
    NANDFLASH_SPEED_234M = 234000000,
};

enum NANDFLASH_OP_MODE {
    // Different groups can be used together, different flash-device may support different option(s)

    // (1) basic read mode
    // standard spi mode
    NANDFLASH_OP_MODE_STAND_SPI             = (1 << 0),
    // fast spi mode
    NANDFLASH_OP_MODE_FAST_SPI              = (1 << 1),
    // dual mode
    NANDFLASH_OP_MODE_DUAL_OUTPUT           = (1 << 2),
    // dual mode
    NANDFLASH_OP_MODE_DUAL_IO               = (1 << 3),
    // quad mode
    NANDFLASH_OP_MODE_QUAD_OUTPUT           = (1 << 4),
    // quad mode
    NANDFLASH_OP_MODE_QUAD_IO               = (1 << 5),

    // (2) program mode
    // page program mode
    NANDFLASH_OP_MODE_PAGE_PROGRAM          = (1 << 6),
    // quad program mode
    NANDFLASH_OP_MODE_QUAD_PAGE_PROGRAM     = (1 << 7),

    // (3) misc mode
    // internal ecc mode
    NANDFLASH_OP_MODE_INTERNAL_ECC          = (1 << 8),
    // read addr 24-bit mode
    NANDFLASH_OP_MODE_READ_ADDR_24BIT       = (1 << 9),

    NANDFLASH_OP_MODE_RESERVED              = 0xFFFFFFFF,
};

struct NANDFLASH_CONFIG_T {
    uint32_t source_clk;
    uint32_t speed;
    enum NANDFLASH_OP_MODE mode;

    /* internal use : can be config if need to (useful for rom) */
    uint8_t override_config:1;
    uint8_t neg_phase:1;
    uint8_t pos_neg:1;
    uint8_t reserved_3:1;
    uint8_t samdly:3;
    uint8_t div; /* least 2 */
    uint8_t dualmode:1;
    uint8_t holdpin:1;
    uint8_t wprpin:1;
    uint8_t quadmode:1;

    uint8_t spiruen:3;
    uint8_t spirden:3;

    uint8_t dualiocmd;
    uint8_t rdcmd;
    uint8_t frdcmd;
    uint8_t qrdcmd;
};

union NANDFLASH_SEC_REG_CFG_T {
    struct {
        uint8_t enabled    :1;
        uint8_t page_start :7;
        uint8_t page_num;
    } s;
    uint16_t v;
};

union NANDFLASH_DUMMY_CYCLE_CFG_T {
    struct {
        uint8_t quad_io_en  :1;
        uint8_t quad_io_val :7;
    } s;
    uint16_t v;
};

struct NANDFLASH_CFG_T {
    uint8_t id[NANDFLASH_ID_LEN];
    uint8_t die_num;
    uint8_t plane_cfg;
    uint8_t ecc_status_type;
    uint16_t block_protect_mask;
    union NANDFLASH_SEC_REG_CFG_T sec_reg_cfg;
    union NANDFLASH_DUMMY_CYCLE_CFG_T dummy_cycle_cfg;
    uint16_t page_size;
    uint16_t page_spare_size;
    uint32_t block_size;
    uint32_t total_size;
    uint32_t max_speed;
    uint32_t mode;
};

enum NANDFLASH_RET_T nandflash_open(enum HAL_FLASH_ID_T id, const struct NANDFLASH_CONFIG_T *cfg);

enum NANDFLASH_RET_T nandflash_close(enum HAL_FLASH_ID_T id);

enum NANDFLASH_RET_T nandflash_init(enum HAL_FLASH_ID_T id);

int nandflash_opened(enum HAL_FLASH_ID_T id);

enum NANDFLASH_RET_T nandflash_get_open_state(enum HAL_FLASH_ID_T id);

enum NANDFLASH_RET_T nandflash_get_size(enum HAL_FLASH_ID_T id, uint32_t *total_size, uint32_t *block_size,
                                        uint32_t *page_size, uint32_t *page_spare_size, uint32_t *die_num);

enum NANDFLASH_RET_T nandflash_get_boundary(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t *block_boundary, uint32_t *page_boundary);

enum NANDFLASH_RET_T nandflash_get_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len);

enum NANDFLASH_RET_T nandflash_enable_protection(enum HAL_FLASH_ID_T id);

enum NANDFLASH_RET_T nandflash_disable_protection(enum HAL_FLASH_ID_T id);

enum NANDFLASH_RET_T nandflash_erase(enum HAL_FLASH_ID_T id, uint32_t offset, uint32_t len);

enum NANDFLASH_RET_T nandflash_write(enum HAL_FLASH_ID_T id, uint32_t offset, const uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_read(enum HAL_FLASH_ID_T id, uint32_t offset, uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_load_valid_data(enum HAL_FLASH_ID_T id, uint32_t start, uint32_t end, uint8_t *buffer, uint32_t len);

int nandflash_block_is_bad(enum HAL_FLASH_ID_T id, uint32_t block);

enum NANDFLASH_RET_T nandflash_mark_bad_block(enum HAL_FLASH_ID_T id, uint32_t block);

enum NANDFLASH_RET_T nandflash_erase_block(enum HAL_FLASH_ID_T id, uint32_t block);

enum NANDFLASH_RET_T nandflash_write_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, const uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_write_oob(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, const uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_update_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, const uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_update_oob(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, const uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_copy_page(enum HAL_FLASH_ID_T id, uint32_t dst_block, uint32_t dst_page, uint32_t src_block, uint32_t src_page);

enum NANDFLASH_RET_T nandflash_read_page(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_read_oob(enum HAL_FLASH_ID_T id, uint32_t block, uint32_t page, uint32_t offset, uint8_t *buffer, uint32_t len);

void nandflash_dump_features(enum HAL_FLASH_ID_T id);

void nandflash_show_calib_result(enum HAL_FLASH_ID_T id);

int nandflash_busy(enum HAL_FLASH_ID_T id);

int nandflash_security_register_is_locked(enum HAL_FLASH_ID_T id);

enum NANDFLASH_RET_T nandflash_security_register_lock(enum HAL_FLASH_ID_T id);

uint32_t nandflash_security_register_enable(enum HAL_FLASH_ID_T id);

void nandflash_security_register_disable(enum HAL_FLASH_ID_T id, uint32_t mode);

enum NANDFLASH_RET_T nandflash_security_register_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len);

enum NANDFLASH_RET_T nandflash_security_register_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

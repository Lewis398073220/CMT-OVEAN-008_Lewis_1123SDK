/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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

#ifndef __APP_TOTA_MULTI_BIN_UPDATA_H__
#define __APP_TOTA_MULTI_BIN_UPDATA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ota_control.h"
void app_handle_multi_chip_tota_data(uint8_t* ptrParam,uint32_t paramLen);

uint32_t app_tota_get_dongle_image_size_via_flash(void);
uint32_t app_tota_get_headset_master_image_size_via_flash(void);
uint32_t app_tota_get_written_sum_image_size_via_flash(void);
void app_tota_store_written_sum_image_size_to_flash(uint32_t written_sum_zise);
bool app_tota_get_pending_for_reboot_flag(FLASH_OTA_BOOT_INFO_T* flash_ota_boot_info);
void app_tota_set_pending_for_reboot_flag(bool reboot);
int32_t uart_wait_send_sync_msg_done(void);

/**
 * @brief Type of the tota transmission path.
 *
 */
typedef enum
{
    CROSS_CHIP_OTA_ROLE_IS_DONGLE = 0X44,
    CROSS_CHIP_OTA_ROLE_IS_MASTER = 0X4D,
    CROSS_CHIP_OTA_ROLE_IS_SLAVE  = 0X53,
    CROSS_CHIP_OTA_ROLE_IS_IVAILD = 0xff
} APP_CROSS_CHIP_OTA_ROLE_E;

typedef struct
{
    uint32_t dongle_image_size;
    uint32_t headset_master_image_size;
    uint32_t headset_slave_image_size;
    uint32_t written_sum_image_size;
    bool pending_for_reboot;
    FLASH_OTA_BOOT_INFO_T flash_ota_boot_info;
}TOTA_OTA_IMAGE_SIZE_T;

#ifdef __cplusplus
}
#endif

#endif

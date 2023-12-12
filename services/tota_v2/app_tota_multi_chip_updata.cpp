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
#ifdef TOTA_CROSS_CHIP_OTA
#include "app_tota_cmd_code.h"
#include "app_tota_custom.h"
#include "bluetooth_bt_api.h"
#include "app_bt_stream.h"
#include "app_audio.h"
#include "nvrecord_bt.h"
#include "nvrecord_env.h"
#include "app_tota.h"
#include "app_tota_cmd_handler.h"
#include "apps.h"
#include "cmsis_os.h"
#ifndef BLE_STACK_NEW_DESIGN
#include "app_ble_cmd_handler.h"
#include "app_ble_custom_cmd.h"
#else
#include "app_custom.h"
#endif
#include "app_tota_custom.h"
#include "app_tota_common.h"
#include "ota_control.h"
#include "app_flash_api.h"
#include "bes_gap_api.h"
#include "ota_control.h"
#include "app_tota_multi_chip_updata.h"

#ifdef APP_CHIP_BRIDGE_MODULE
#include "app_chip_bridge.h"
#endif
#if defined(CFG_APP_DATAPATH_SERVER)
#ifndef BLE_STACK_NEW_DESIGN
#include "app_datapath_server.h"
#else
#include "ble_datapath.h"
#endif
#endif /* CFG_APP_DATAPATH_SERVER */

#if defined(CFG_APP_DATAPATH_CLIENT)
#ifndef BLE_STACK_NEW_DESIGN
#include "app_datapath_client.h"
#else
#include "ble_datapath.h"
#endif
#endif /* CFG_APP_DATAPATH_CLIENT */

#define  TOTA_CROSS_OTA_DUMP8(str, buf, cnt)        //DUMP8(str, buf, cnt)

#if defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_DONGLE)
APP_CROSS_CHIP_OTA_ROLE_E tota_cross_chip_ota_role = CROSS_CHIP_OTA_ROLE_IS_DONGLE;
#elif defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_MASTER)
APP_CROSS_CHIP_OTA_ROLE_E tota_cross_chip_ota_role = CROSS_CHIP_OTA_ROLE_IS_MASTER;
#elif defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_SLAVE)
APP_CROSS_CHIP_OTA_ROLE_E tota_cross_chip_ota_role = CROSS_CHIP_OTA_ROLE_IS_SLAVE;
#endif

extern OTA_CONTROL_ENV_T ota_control_env;
extern uint32_t __ota_upgrade_log_start[];
FLASH_OTA_UPGRADE_LOG_FLASH_T *otaUpgradeLogInFlash_p = (FLASH_OTA_UPGRADE_LOG_FLASH_T *)__ota_upgrade_log_start;
#define otaUpgradeLog   (*otaUpgradeLogInFlash_p)

static void dongle_tota_receive_data_from_headset(uint32_t funcCode, uint8_t *p_buff, uint32_t bufLength);
static void headset_tota_receive_data(uint32_t funcCode, uint8_t *p_buff, uint32_t bufLength);

BLE_CUSTOM_COMMAND_TO_ADD(OP_DONGLE_TOTA_RECEIVE_DATA,
                        dongle_tota_receive_data_from_headset,
                        false,
                        0,
                        NULL);

BLE_CUSTOM_COMMAND_TO_ADD(OP_HEADSET_TOTA_RECEIVE_DATA,
                        headset_tota_receive_data,
                        false,
                        0,
                        NULL);

POSSIBLY_UNUSED static void send_tota_data_to_headset_by_datapath(uint8_t* ptrParam, uint32_t paramLen)
{
    if (ptrParam != NULL && paramLen > 0) {
        BLE_CUSTOM_CMD_RET_STATUS_E ret;
        ret = BLE_send_custom_command(OP_HEADSET_TOTA_RECEIVE_DATA,
                                        ptrParam,
                                        paramLen,
                                        TRANSMISSION_VIA_WRITE_CMD);
        TRACE(0, "%s ret:%d", __func__, ret);
    } else {
        TRACE(0, "%s invalid param.", __func__);
    }
}

static void send_tota_data_to_dongle_by_datapath(uint8_t* ptrParam, uint32_t paramLen)
{
    if (ptrParam != NULL && paramLen > 0) {
        BLE_CUSTOM_CMD_RET_STATUS_E ret;
        ret = BLE_send_custom_command(OP_DONGLE_TOTA_RECEIVE_DATA,
                                        (uint8_t*)ptrParam,
                                        paramLen,
                                        TRANSMISSION_VIA_NOTIFICATION);
        TRACE(0, "%s ret:%d", __func__, ret);
    } else {
        TRACE(0, "%s invalid param.", __func__);
    }
}

static void headset_tota_receive_data(uint32_t funcCode, uint8_t *p_buff, uint32_t bufLength)
{
    tota_set_connect_path(APP_TOTA_VIA_SPP);
    tota_set_connect_status(TOTA_CONNECTED);
    ota_control_register_transmitter(send_tota_data_to_dongle_by_datapath);
    _custom_cmd_handle(OP_TOTA_OTA, p_buff, bufLength);
}

static void dongle_tota_receive_data_from_headset(uint32_t funcCode,
                                                            uint8_t *p_buff, uint32_t bufLength)
{
    TRACE(0, "%s len:%d", __func__, bufLength);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", p_buff, bufLength);

    TOTA_OTA_IMAGE_SIZE_T temp_tota_ota_image_size;
    uint32_t tempAccumulateCurrentReciveNumber = 0;
    if (*p_buff == CROSS_CHIP_OTA_ROLE_IS_MASTER)
    {
        temp_tota_ota_image_size.dongle_image_size = app_tota_get_dongle_image_size_via_flash();

        uint32_t headset_master_offset = 0;
        memcpy(&headset_master_offset, p_buff + sizeof(CROSS_CHIP_OTA_ROLE_IS_MASTER),
                                                        sizeof(headset_master_offset));
        tempAccumulateCurrentReciveNumber = temp_tota_ota_image_size.dongle_image_size +
                                        headset_master_offset;
        app_tota_store_written_sum_image_size_to_flash(tempAccumulateCurrentReciveNumber);
        updata_tota_ota_current_recive_number(tempAccumulateCurrentReciveNumber);
        TRACE(0, "headset_master_offset %d sumbinSize %d dongle_size %d", headset_master_offset,
            tempAccumulateCurrentReciveNumber, temp_tota_ota_image_size.dongle_image_size);
    }
    else if (*p_buff == CROSS_CHIP_OTA_ROLE_IS_SLAVE)
    {
        temp_tota_ota_image_size.dongle_image_size = app_tota_get_dongle_image_size_via_flash();
        temp_tota_ota_image_size.headset_master_image_size = app_tota_get_headset_master_image_size_via_flash();

        uint32_t headset_slave_offset = 0;
        memcpy(&headset_slave_offset, p_buff + sizeof(CROSS_CHIP_OTA_ROLE_IS_SLAVE),
                                                        sizeof(headset_slave_offset));
        tempAccumulateCurrentReciveNumber = temp_tota_ota_image_size.dongle_image_size +
                                        temp_tota_ota_image_size.headset_master_image_size +
                                        headset_slave_offset;
        app_tota_store_written_sum_image_size_to_flash(tempAccumulateCurrentReciveNumber);
        updata_tota_ota_current_recive_number(tempAccumulateCurrentReciveNumber);
        TRACE(0, "headset_slave_offset %d sumbinSize %d dongle_size %d headset_m_size %d", headset_slave_offset,
            tempAccumulateCurrentReciveNumber,temp_tota_ota_image_size.dongle_image_size,
            temp_tota_ota_image_size.headset_master_image_size);
    }
    else if (*p_buff == OTA_RSP_RESUME_VERIFY)
    {
        TRACE(0, "OTA_TLV_PACKET_HEADER_LEN %d",OTA_TLV_PACKET_HEADER_LEN);
        tempAccumulateCurrentReciveNumber = get_tota_ota_current_recive_number();
        memcpy((p_buff + OTA_TLV_PACKET_HEADER_LEN),
                (uint8_t *)&tempAccumulateCurrentReciveNumber, sizeof(uint32_t));
    }
    ota_spp_tota_send_data(p_buff, bufLength);
}

void app_tota_save_image_size_to_flash(uint8_t* ptrParam)
{
    OTA_LEGACY_PACKET_T packet;
    uint8_t* dataBufferForBurning = ota_voicepath_get_common_ota_databuf();
    TOTA_OTA_IMAGE_SIZE_T* tota_ota_image_size = (TOTA_OTA_IMAGE_SIZE_T*)(dataBufferForBurning + OTAUPLOG_HEADSIZE);
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    uint32_t image_size_offset = sizeof(APP_CROSS_CHIP_OTA_ROLE_E) + OTA_PACKET_LENGTH_HEADER_LEN + sizeof(packet.payload.cmdOtaStart.magicCode);
    uint32_t flash_op_length = OTAUPLOG_HEADSIZE + sizeof(TOTA_OTA_IMAGE_SIZE_T);
    APP_CROSS_CHIP_OTA_ROLE_E ota_chip_role = (APP_CROSS_CHIP_OTA_ROLE_E)ptrParam[0];

    switch (ota_chip_role) {
        case CROSS_CHIP_OTA_ROLE_IS_DONGLE:
            memset(dataBufferForBurning, 0x00, flash_op_length);

            app_flash_read(mod, 0, dataBufferForBurning, flash_op_length);
            app_flash_sector_erase(mod, 0);

            memcpy((uint8_t *)&(tota_ota_image_size->dongle_image_size), ptrParam + image_size_offset, 
                    sizeof(tota_ota_image_size->dongle_image_size));

            app_flash_program(mod, 0, dataBufferForBurning, flash_op_length, false);
            break;
        case CROSS_CHIP_OTA_ROLE_IS_MASTER:
            memset(dataBufferForBurning, 0x00, flash_op_length);

            app_flash_read(mod, 0, dataBufferForBurning, flash_op_length);
            app_flash_sector_erase(mod, 0);

            memcpy((uint8_t *)&(tota_ota_image_size->headset_master_image_size), ptrParam + image_size_offset, 
                    sizeof(tota_ota_image_size->headset_master_image_size));

            app_flash_program(mod, 0, dataBufferForBurning, flash_op_length, false);
            break;
        case CROSS_CHIP_OTA_ROLE_IS_SLAVE:
            memset(dataBufferForBurning, 0x00, flash_op_length);

            app_flash_read(mod, 0, dataBufferForBurning, flash_op_length);
            app_flash_sector_erase(mod, 0);

            memcpy((uint8_t *)&(tota_ota_image_size->headset_slave_image_size), ptrParam + image_size_offset, 
                    sizeof(tota_ota_image_size->headset_slave_image_size));

            app_flash_program(mod, 0, dataBufferForBurning, flash_op_length, false);
            break;
        default:
            break;
    }
}

uint32_t app_tota_get_dongle_image_size_via_flash(void)
{
    TRACE(0, "%s", __func__);
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    uint32_t dongle_image_size = 0xFFFFFFFF;
    uint32_t flash_op_offset = OTAUPLOG_HEADSIZE;

    app_flash_read(mod, flash_op_offset, (uint8_t*)&(dongle_image_size), sizeof(dongle_image_size));

    dongle_image_size = dongle_image_size == 0xFFFFFFFF ? 0 : dongle_image_size;

    TRACE(1, "tota read dongle image size:%d", dongle_image_size);

    return dongle_image_size;
}

uint32_t app_tota_get_headset_master_image_size_via_flash(void)
{
    TRACE(0, "%s", __func__);
    TOTA_OTA_IMAGE_SIZE_T tota_ota_image_size;
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    uint32_t master_image_size = 0xFFFFFFFF;
    uint32_t flash_op_offset = OTAUPLOG_HEADSIZE +
                                sizeof(tota_ota_image_size.dongle_image_size);

    app_flash_read(mod, flash_op_offset, (uint8_t*)&(master_image_size), sizeof(master_image_size));

    master_image_size = (master_image_size == 0xFFFFFFFF) ? 0 : master_image_size;

    TRACE(1, "tota read headset master image size:%d", master_image_size);

    return master_image_size;
}

uint32_t app_tota_get_written_sum_image_size_via_flash(void)
{
    TOTA_OTA_IMAGE_SIZE_T tota_ota_image_size;
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    uint32_t sum_image_size = 0xFFFFFFFF;
    uint32_t flash_op_offset = OTAUPLOG_HEADSIZE +
                                sizeof(tota_ota_image_size.dongle_image_size) +
                                sizeof(tota_ota_image_size.headset_master_image_size) +
                                sizeof(tota_ota_image_size.headset_slave_image_size);


    app_flash_read(mod, flash_op_offset, (uint8_t*)&(sum_image_size), sizeof(sum_image_size));

    sum_image_size = (sum_image_size == 0xFFFFFFFF) ? 0 : sum_image_size;

    updata_tota_ota_current_recive_number(sum_image_size);
    TRACE(1, "tota read sum image size:%d", sum_image_size);

    return sum_image_size;
}

void app_tota_store_written_sum_image_size_to_flash(uint32_t written_sum_size)
{
    uint8_t* dataBufferForBurning = ota_voicepath_get_common_ota_databuf();
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    TOTA_OTA_IMAGE_SIZE_T* tota_ota_image_size = (TOTA_OTA_IMAGE_SIZE_T*)(dataBufferForBurning + OTAUPLOG_HEADSIZE);

    uint32_t flash_op_length = OTAUPLOG_HEADSIZE + sizeof(TOTA_OTA_IMAGE_SIZE_T);

    memset(dataBufferForBurning, 0x00, flash_op_length);

    app_flash_read(mod, 0, dataBufferForBurning, flash_op_length);
    app_flash_sector_erase(mod, 0);

    tota_ota_image_size->written_sum_image_size = written_sum_size;

    app_flash_program(mod, 0, dataBufferForBurning, flash_op_length, false);

    TRACE(1, "tota write ota sum image size:%d", written_sum_size);
}

void app_tota_set_pending_for_reboot_flag(bool reboot)
{
    uint8_t* dataBufferForBurning = ota_voicepath_get_common_ota_databuf();
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    TOTA_OTA_IMAGE_SIZE_T* tota_ota_image_size = (TOTA_OTA_IMAGE_SIZE_T*)(dataBufferForBurning + OTAUPLOG_HEADSIZE);

    uint32_t flash_op_length = OTAUPLOG_HEADSIZE + sizeof(TOTA_OTA_IMAGE_SIZE_T);

    memset(dataBufferForBurning, 0x00, flash_op_length);

    app_flash_read(mod, 0, dataBufferForBurning, flash_op_length);
    app_flash_sector_erase(mod, 0);

    tota_ota_image_size->pending_for_reboot = reboot;
    tota_ota_image_size->flash_ota_boot_info.imageCrc = ota_control_env.crc32OfImage;
    tota_ota_image_size->flash_ota_boot_info.imageSize = ota_control_env.totalImageSize;
    tota_ota_image_size->flash_ota_boot_info.newImageFlashOffset = ota_control_env.dstFlashOffsetForNewImage;

    app_flash_program(mod, 0, dataBufferForBurning, flash_op_length, false);

    TRACE(1, "pending for reboot:%d, crc:0x%x, size:0x%x, dst:0x%x", tota_ota_image_size->pending_for_reboot, 
          tota_ota_image_size->flash_ota_boot_info.imageCrc, tota_ota_image_size->flash_ota_boot_info.imageSize, tota_ota_image_size->flash_ota_boot_info.newImageFlashOffset);
}

bool app_tota_get_pending_for_reboot_flag(FLASH_OTA_BOOT_INFO_T* flash_ota_boot_info)
{
    TOTA_OTA_IMAGE_SIZE_T tota_ota_image_size;
    NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_UPGRADE_LOG;
    uint8_t reboot = 0xFF;
    uint32_t flash_op_offset = OTAUPLOG_HEADSIZE;

    app_flash_read(mod, flash_op_offset, (uint8_t*)&(tota_ota_image_size), sizeof(tota_ota_image_size));

    reboot = tota_ota_image_size.pending_for_reboot;
    TRACE(1, "%s reboot:%d", __func__, reboot);

    memset(flash_ota_boot_info, 0x00, sizeof(FLASH_OTA_BOOT_INFO_T));

    if (reboot != 0xFF && reboot != 0) {
        memcpy(flash_ota_boot_info, &tota_ota_image_size.flash_ota_boot_info, sizeof(FLASH_OTA_BOOT_INFO_T));
        return true;
    }

    return false;
}

#if defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_DONGLE)
void app_handle_multi_chip_tota_data(uint8_t* ptrParam, uint32_t paramLen)
{
    do {
        if (ptrParam[0] == OTA_COMMAND_IMAGE_APPLY)
        {
            uint8_t _ptrParam[12] = {0};;
            memcpy(_ptrParam + sizeof(uint8_t), ptrParam, paramLen);
            uint32_t _paramLen = paramLen + sizeof(uint8_t);
            _ptrParam[0] = CROSS_CHIP_OTA_ROLE_IS_SLAVE;
            send_tota_data_to_headset_by_datapath(_ptrParam, _paramLen);
            osDelay(50);
            _ptrParam[0] = CROSS_CHIP_OTA_ROLE_IS_MASTER;
            send_tota_data_to_headset_by_datapath(_ptrParam, _paramLen);
        }

        if (ptrParam[0] == OTA_COMMAND_RESUME_VERIFY)
        {
            uint32_t sum_image_size = app_tota_get_written_sum_image_size_via_flash();
            uint32_t dongle_image_size = app_tota_get_dongle_image_size_via_flash();
            uint32_t master_image_size = app_tota_get_headset_master_image_size_via_flash();
            TRACE(0, "sum_image_size:%d, dongle_image_size:%d, master_image_size:%d", sum_image_size, dongle_image_size, master_image_size);

            if(sum_image_size > 0 && dongle_image_size >= 0 && sum_image_size > dongle_image_size)
            {
                OTA_LEGACY_PACKET_T packet;
                uint8_t _ptrParam[sizeof(packet.payload.cmdResumeBreakpoint) + 2 * sizeof(uint32_t)];
                memset(_ptrParam, 0, sizeof(packet.payload.cmdResumeBreakpoint) + 2 * sizeof(uint32_t));
                memcpy(_ptrParam + sizeof(APP_CROSS_CHIP_OTA_ROLE_E), ptrParam, paramLen);
                uint32_t _paramLen = paramLen + sizeof(APP_CROSS_CHIP_OTA_ROLE_E);
                if (sum_image_size > (dongle_image_size + master_image_size)) {
                    TRACE(0, "%s CROSS_CHIP_OTA_ROLE_IS_SLAVE", __func__);
                    _ptrParam[0] = CROSS_CHIP_OTA_ROLE_IS_SLAVE;
                } else {
                    TRACE(0, "%s CROSS_CHIP_OTA_ROLE_IS_MASTER", __func__);
                    _ptrParam[0] = CROSS_CHIP_OTA_ROLE_IS_MASTER;
                }
                send_tota_data_to_headset_by_datapath(_ptrParam, _paramLen);
                break;
            }
        }

        if ((ptrParam[1] == OTA_COMMAND_START))
        {
            //save dongle/headset master/heaset slave image size to flash
            app_tota_save_image_size_to_flash(ptrParam);
        }

        if (*ptrParam == tota_cross_chip_ota_role && *ptrParam != CROSS_CHIP_OTA_ROLE_IS_IVAILD)
        {
            ptrParam += sizeof(uint8_t);
            --paramLen;
        }
        else if ((*ptrParam == CROSS_CHIP_OTA_ROLE_IS_MASTER || *ptrParam == CROSS_CHIP_OTA_ROLE_IS_SLAVE)
            && tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_DONGLE)
        {
            //send data to headset master via datapath
            send_tota_data_to_headset_by_datapath(ptrParam, paramLen);
            break;
        }
        else if (*ptrParam == CROSS_CHIP_OTA_ROLE_IS_SLAVE && tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_MASTER)
        {
            ASSERT(0, "should not arrived here at dongle");
            break;
        }
        else if (*ptrParam == CROSS_CHIP_OTA_ROLE_IS_IVAILD || tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_IVAILD)
        {
            ASSERT(0, " CROSS_CHIP_OTA_ROLE_IS_IVAILD !!!!!!!!!!!!!");
            break;
        }

        app_ota_over_tota_receive_data(ptrParam, paramLen);
    } while(0);
}
#elif defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_MASTER)
void app_handle_multi_chip_tota_data(uint8_t* ptrParam, uint32_t paramLen)
{
    do {
        if (ptrParam[0] == OTA_COMMAND_IMAGE_APPLY)
        {
#ifdef APP_CHIP_BRIDGE_MODULE
            uint8_t _ptrParam[12] = {0};
            memcpy(_ptrParam + sizeof(uint8_t), ptrParam, paramLen);
            uint32_t _paramLen = paramLen + sizeof(uint8_t);
            _ptrParam[0] = CROSS_CHIP_OTA_ROLE_IS_SLAVE;
            app_chip_bridge_send_cmd(SET_OTA_MODE_CMD, (uint8_t *)_ptrParam, _paramLen);
            uart_wait_send_sync_msg_done();
#endif
        }

        if (ptrParam[0] == OTA_COMMAND_RESUME_VERIFY)
        {
            uint32_t sum_image_size = app_tota_get_written_sum_image_size_via_flash();
            uint32_t headset_master_image_size = app_tota_get_headset_master_image_size_via_flash();
            TRACE(0, "sum_image_size:%d, headset_master_image_size:%d", sum_image_size, headset_master_image_size);

            if (sum_image_size > 0 && headset_master_image_size >= 0 && sum_image_size > headset_master_image_size)
            {
                TRACE(0,"%s CROSS_CHIP_OTA_ROLE_IS_SLAVE", __func__);
                OTA_LEGACY_PACKET_T packet;
                uint8_t _ptrParam[sizeof(packet.payload.cmdResumeBreakpoint) + 2 * sizeof(uint32_t)];
                memset(_ptrParam, 0, sizeof(packet.payload.cmdResumeBreakpoint) + 2 * sizeof(uint32_t));
                memcpy(_ptrParam + sizeof(APP_CROSS_CHIP_OTA_ROLE_E), ptrParam, paramLen);
                _ptrParam[0] = CROSS_CHIP_OTA_ROLE_IS_SLAVE;
#ifdef APP_CHIP_BRIDGE_MODULE
                uint32_t _paramLen = paramLen + sizeof(APP_CROSS_CHIP_OTA_ROLE_E);
                app_chip_bridge_send_cmd(SET_OTA_MODE_CMD, (uint8_t *)_ptrParam, _paramLen);
                uart_wait_send_sync_msg_done();
#endif
                break;
            }
        }

        if (ptrParam[1] == OTA_COMMAND_START)
        {
            //save dongle/headset master/heaset slave image size to flash
            app_tota_save_image_size_to_flash(ptrParam);
#if defined(APP_CHIP_BRIDGE_MODULE) && defined(UART_TOGGLE_MODE)
            if( CROSS_CHIP_OTA_ROLE_IS_SLAVE == *ptrParam ){
                uint8_t uart_rx_keep_awake[] = "normal";
                app_chip_bridge_send_cmd(UART_MODE_SWITCH_CMD, uart_rx_keep_awake, strlen((char*)uart_rx_keep_awake));
                osDelay(100);
            }
#endif
        }

        if (*ptrParam == tota_cross_chip_ota_role && *ptrParam != CROSS_CHIP_OTA_ROLE_IS_IVAILD)
        {
            ptrParam += sizeof(uint8_t);
            --paramLen;
        }
        else if ((*ptrParam == CROSS_CHIP_OTA_ROLE_IS_MASTER || *ptrParam == CROSS_CHIP_OTA_ROLE_IS_SLAVE)
            && tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_DONGLE)
        {
            ASSERT(0, "should not arrived here at headset master");
            break;
        }
        else if (*ptrParam == CROSS_CHIP_OTA_ROLE_IS_SLAVE && tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_MASTER)
        {
#ifdef APP_CHIP_BRIDGE_MODULE
            //send data to headset slave via uart
            app_chip_bridge_send_cmd(SET_OTA_MODE_CMD, (uint8_t *)ptrParam, paramLen);
            uart_wait_send_sync_msg_done();
#endif
            break;
        }
        else if (*ptrParam == CROSS_CHIP_OTA_ROLE_IS_IVAILD || tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_IVAILD)
        {
            ASSERT(0, "CROSS_CHIP_OTA_ROLE_IS_IVAILD !!!!!!!!!!!!!");
            break;
        }

        app_ota_over_tota_receive_data(ptrParam, paramLen);
    } while(0);
}
#elif defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_SLAVE)
void app_handle_multi_chip_tota_data(uint8_t* ptrParam, uint32_t paramLen)
{
    do {
        TRACE(0,"##### custom: ota tota_cross_chip_ota_role 0x%02x", tota_cross_chip_ota_role);
        TOTA_CROSS_OTA_DUMP8("0x%02x ", ptrParam, paramLen);

        if (*ptrParam == tota_cross_chip_ota_role && *ptrParam != CROSS_CHIP_OTA_ROLE_IS_IVAILD)
        {
            ptrParam += sizeof(uint8_t);
            --paramLen;
        }
        else if ((*ptrParam == CROSS_CHIP_OTA_ROLE_IS_MASTER || *ptrParam == CROSS_CHIP_OTA_ROLE_IS_SLAVE)
            && tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_DONGLE)
        {
            ASSERT(0, "should not arrived here at headset slave");
            break;
        }
        else if (*ptrParam == CROSS_CHIP_OTA_ROLE_IS_SLAVE && tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_MASTER)
        {
            ASSERT(0, "should not arrived here at headset slave");
            break;
        }
        else if (*ptrParam == CROSS_CHIP_OTA_ROLE_IS_IVAILD || tota_cross_chip_ota_role == CROSS_CHIP_OTA_ROLE_IS_IVAILD)
        {
            ASSERT(0, " CROSS_CHIP_OTA_ROLE_IS_IVAILD !!!!!!!!!!!!!");
            break;
        }

        app_ota_over_tota_receive_data(ptrParam, paramLen);;
    } while(0);
}
#endif

#ifdef APP_CHIP_BRIDGE_MODULE

/*
* tb ota case for UART1 communication
*
*/
#if defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_MASTER)
typedef enum {
    CMD_STATUS_IDLE = 0,
    CMD_STATUS_SENDING,
    CMD_STATUS_TIMEOUT,
    CMD_STATUS_DONE
} cmd_status_t;
static cmd_status_t g_sync_cmd_status = CMD_STATUS_IDLE;

static void app_set_ota_mode_cmd_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "[%s] len = %d", __func__, length);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", p_buff, length > 16 ? 16 : length);
     g_sync_cmd_status = CMD_STATUS_SENDING;
    app_chip_bridge_send_data_with_waiting_rsp(SET_OTA_MODE_CMD, p_buff, length);
}

static void app_set_ota_mode_cmd_receive_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s length %d", __func__, length);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", p_buff, length > 16 ? 16 : length);

    if (p_buff[0] == OTA_COMMAND_SEGMENT_VERIFY && p_buff[1] == CROSS_CHIP_OTA_ROLE_IS_SLAVE) {
        TRACE(1, "master recv segment verify resp from slave and save sum size");
        dongle_tota_receive_data_from_headset(0, p_buff+1, length-1);
        ota_control_env.transmitHander(p_buff+1, length-1);
        return;
    }

    if (p_buff[0] == OTA_RSP_RESUME_VERIFY) {
        OTA_PACKET_T* rsp = (OTA_PACKET_T*)p_buff;
        uint32_t breakPoint = app_tota_get_headset_master_image_size_via_flash();
        breakPoint += rsp->payload.rspResumeBreakpoint.breakPoint;
        TRACE(1, "breakPoint:%d", breakPoint);
        rsp->payload.rspResumeBreakpoint.breakPoint = breakPoint;
        TOTA_CROSS_OTA_DUMP8("%02x ", p_buff, length);
    }

    ota_control_env.transmitHander(p_buff, length);
}

POSSIBLY_UNUSED static void app_set_ota_mode_cmd_wait_rsp_timeout(uint8_t *p_buff, uint16_t length)
{
    TRACE(1, "%s", __func__);
    g_sync_cmd_status = CMD_STATUS_TIMEOUT;
}

int32_t uart_wait_send_sync_msg_done(void)
{
    uint32_t wait_cnt = 0;

    while (g_sync_cmd_status != CMD_STATUS_DONE) {
        osDelay(10);

        if (wait_cnt++ > 100) {     // 1s
            break;
        }
    }
    ASSERT(wait_cnt <= 100, "[%s] Failed to send sync cmd. Status: %d", __func__, g_sync_cmd_status);
    g_sync_cmd_status = CMD_STATUS_IDLE;
    return 0;
}

POSSIBLY_UNUSED static void app_set_ota_mode_cmd_send_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(1, "%s", __func__);
    g_sync_cmd_status = CMD_STATUS_DONE;
}

CHIP_BRIDGE_TASK_COMMAND_TO_ADD(SET_OTA_MODE_CMD,
                                "ota for chip bridge",
                                app_set_ota_mode_cmd_handler,
                                app_set_ota_mode_cmd_receive_handler,
                                APP_CHIP_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                app_set_ota_mode_cmd_wait_rsp_timeout,
                                app_set_ota_mode_cmd_send_rsp_handler,
                                NULL);

#elif defined(TOTA_CROSS_CHIP_OTA_ROLE_IS_SLAVE)
void send_tota_data_to_dongle_by_uart(uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE(0, "%s len: %d", __func__, paramLen);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", ptrParam, paramLen > 16 ? 16 : paramLen);
    app_chip_bridge_send_cmd(SET_OTA_MODE_CMD, (uint8_t *)ptrParam, paramLen);
}

void handle_data_from_headset_via_uart(uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE(0, "%s len: %d", __func__, paramLen);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", ptrParam, paramLen > 16 ? 16 : paramLen);

    tota_set_connect_path(APP_TOTA_VIA_UART);
    tota_set_connect_status(TOTA_CONNECTED);
    ota_control_register_transmitter(send_tota_data_to_dongle_by_uart);
    _custom_cmd_handle(OP_TOTA_OTA, ptrParam, paramLen);
}

static void app_set_ota_mode_cmd_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "%s len: %d", __func__, length);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", p_buff, length > 16 ? 16 : length);
    app_chip_bridge_send_data_without_waiting_rsp(SET_OTA_MODE_CMD, (uint8_t *)p_buff, length);
}

static void app_set_ota_mode_cmd_receive_handler(uint8_t *p_buff, uint16_t length)
{
    app_chip_bridge_send_rsp(SET_OTA_MODE_CMD, p_buff, 1);
    TRACE(1,"%s len: %d", __func__, length);
    TOTA_CROSS_OTA_DUMP8("0x%02x ", p_buff, length > 16 ? 16 : length);
    handle_data_from_headset_via_uart(p_buff,length);
}

POSSIBLY_UNUSED static void app_set_ota_mode_cmd_wait_rsp_timeout(uint8_t *p_buff, uint16_t length)
{
    TRACE(1, "%s", __func__);

    static uint8_t resend_time = RESEND_TIME;
    if (resend_time > 0)
    {
        app_chip_bridge_send_cmd(SET_OTA_MODE_CMD, p_buff, length);
        resend_time--;
    }
    else
    {
        resend_time = RESEND_TIME;
    }
}

POSSIBLY_UNUSED static void app_set_ota_mode_cmd_send_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    TRACE(1, "%s", __func__);
}

CHIP_BRIDGE_TASK_COMMAND_TO_ADD(SET_OTA_MODE_CMD,
                                "ota for chip bridge",
                                app_set_ota_mode_cmd_handler,
                                app_set_ota_mode_cmd_receive_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);
#endif
#endif
#endif
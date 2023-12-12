/**
 * @file app_flash_api.cpp
 * @author BES AI team
 * @version 0.1
 * @date 2020-11-07
 *
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

/*****************************header include********************************/
#include "hal_trace.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "app_flash_api.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_timer.h"
#include "hal_location.h"

#if 1
#define APP_FLASH_TRACE TRACE
#else
#define APP_FLASH_TRACE(level,...)
#endif

/*********************external function declearation************************/

/************************private macro defination***************************/
#define DEFAULT_CACHE_BUF_LEN (2 * 4096)

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/

/****************************function defination****************************/

static uint32_t _get_block_size(enum NORFLASH_API_MODULE_ID_T module)
{
    enum HAL_FLASH_ID_T dev_id = HAL_FLASH_ID_0;

    norflash_api_get_dev_id(module, &dev_id);

    return norflash_api_get_block_size(dev_id);
}

static uint32_t _get_setctor_size(enum NORFLASH_API_MODULE_ID_T module)
{
    enum HAL_FLASH_ID_T dev_id = HAL_FLASH_ID_0;

    norflash_api_get_dev_id(module, &dev_id);

    return norflash_api_get_sector_size(dev_id);
}

void app_flash_register_module(uint8_t module,
                               enum HAL_FLASH_ID_T dev_id,
                               uint32_t baseAddr,
                               uint32_t len,
                               uint32_t imageHandler)
{
    enum NORFLASH_API_RET_T ret;
    uint32_t block_size = 0;
    uint32_t sector_size = 0;
    uint32_t page_size = 0;

    hal_norflash_get_size(dev_id,
                          NULL,
                          &block_size,
                          &sector_size,
                          &page_size);
    APP_FLASH_TRACE(1,"%s mod:%d, start:0x%x, len:0x%x, %d, %d, %d",
          __func__, module, baseAddr, len, block_size, sector_size, page_size);
    ret = norflash_api_register((NORFLASH_API_MODULE_ID_T)module,
                                dev_id,
                                baseAddr,
                                len,
                                block_size,
                                sector_size,
                                page_size,
                                DEFAULT_CACHE_BUF_LEN,
                                (NORFLASH_API_OPERA_CB)imageHandler);

    ASSERT(ret == NORFLASH_API_OK,
           "ota_init_flash: norflash_api register failed,ret = %d.",
           ret);
    APP_FLASH_TRACE(1,"_debug: %s done", __func__);
}

SRAM_TEXT_LOC void app_flash_flush_pending_op(enum NORFLASH_API_MODULE_ID_T module,
                                enum NORFLASH_API_OPRATION_TYPE type)
{
    do
    {
        norflash_api_flush();

        if (NORFLASH_API_ALL != type)
        {
            if (0 == norflash_api_get_used_buffer_count(module, type))
            {
                break;
            }
        }
        else
        {
            if (norflash_api_buffer_is_free(module))
            {
                break;
            }
        }
        osDelay(10);
    } while (1);

    // APP_FLASH_TRACE(1,"%s: module:%d, type:%d done", __func__, module, type);
}

SRAM_TEXT_LOC void app_flash_sector_erase(enum NORFLASH_API_MODULE_ID_T module, uint32_t flashOffset)
{
    /// get flash address according to the flash module and offset
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t start_addr = flashOffset;
    uint32_t sector_size = _get_setctor_size(module);

    ASSERT((flashOffset & (sector_size - 1)) == 0,
            "%s: addr No sec size alignment! offset = 0x%x, %d",
            __func__,
            flashOffset, sector_size);

    ret = norflash_api_get_base_addr(module, &start_addr);
    ASSERT(ret == NORFLASH_API_OK, "%s: get base addr fail, ret = %d", __func__, ret);

    start_addr += flashOffset;
    APP_FLASH_TRACE(1, "%s: start_addr:%x, %d", __func__, start_addr,sector_size);
    do
    {
        ret = norflash_api_erase(module,
                                 start_addr,
                                 sector_size,
                                 true);

        if (NORFLASH_API_OK == ret)
        {
            // APP_FLASH_TRACE(1,"%s: ok!", __func__);
            break;
        }
        else if (NORFLASH_API_BUFFER_FULL == ret)
        {
            // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
            app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
        }
        else
        {
            uint32_t addr;
            norflash_api_get_base_addr(module, &addr);

            ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x",
                   __func__, ret, start_addr, addr);
        }
    } while (1);
    // APP_FLASH_TRACE(1,"%s: erase: 0x%x,0x1000 done", __func__, start_addr);
}

SRAM_TEXT_LOC void app_flash_erase(enum NORFLASH_API_MODULE_ID_T module, uint32_t flashOffset, uint32_t len)
{
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t b_size = _get_block_size(module);
    uint32_t s_size = _get_setctor_size(module);
    uint32_t s_len;
    uint32_t b_len;
    uint32_t i;
    uint32_t start_time = hal_sys_timer_get();
    uint32_t start_addr = 0;

    ASSERT(((flashOffset & (s_size - 1)) == 0 &&
            (len & (s_size - 1)) == 0),
            "%s: No sec size alignment! offset = 0x%x, len = 0x%x",
            __func__,
            flashOffset,
            len);

    ret = norflash_api_get_base_addr(module, &start_addr);
    ASSERT(ret == NORFLASH_API_OK, "%s: get base addr fail, ret = %d", __func__, ret);

    start_addr += flashOffset;
    APP_FLASH_TRACE(1, "%s: start_addr: start_addr = 0x%x, len = 0x%x",
          __func__, start_addr, len);

    // erase pre-sectors.
    if(len > ((b_size-1) & flashOffset))
    {
        s_len = ((b_size-1) & flashOffset);
    }
    else
    {
        s_len = len;
    }

    APP_FLASH_TRACE(1, "%s: pre sectors erase: start_addr = 0x%x, s_len = 0x%x",
          __func__, start_addr, s_len);
    for(i = 0; i < s_len/s_size; i++)
    {
        // APP_FLASH_TRACE(1,"%s: sector(0x%x) erase", __func__, start_addr);
        do{
            ret = norflash_api_erase(module,
                                     start_addr,
                                     s_size,
                                     true);
            if (NORFLASH_API_OK == ret)
            {
                // APP_FLASH_TRACE(1,"%s: ok!", __func__);
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
                app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
            }
            else
            {
                uint32_t addr;
                norflash_api_get_base_addr(module, &addr);

                ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x, size: 0x%x",
                       __func__, ret, start_addr, addr, s_size);
            }
        }while(1);
        start_addr += s_size;
    }

    // erase blocks.
    b_len = ((len - s_len)/b_size)*b_size ;
    APP_FLASH_TRACE(1, "%s: blocks erase: start_addr = 0x%x, b_len = 0x%x",
          __func__, start_addr, b_len);
    for(i = 0; i < b_len/b_size; i++)
    {
        // APP_FLASH_TRACE(1,"%s: block(0x%x) erase", __func__, start_addr);
        do {
            ret = norflash_api_erase(module,
                                     start_addr,
                                     b_size,
                                     true);
            if (NORFLASH_API_OK == ret)
            {
                // APP_FLASH_TRACE(1,"%s: ok!", __func__);
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
                app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
            }
            else
            {
                uint32_t addr;
                norflash_api_get_base_addr(module, &addr);

                ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x, size: 0x%x",
                       __func__, ret, start_addr, addr, b_size);
            }
        }while(1);
        start_addr += b_size;
    }

    // erase post sectors.
    s_len = len - s_len - b_len;
    APP_FLASH_TRACE(1, "%s: last sectors erase: start_addr = 0x%x, s_len = 0x%x",
          __func__, start_addr, s_len);
    for(i = 0; i < s_len/s_size; i++)
    {
        // APP_FLASH_TRACE(1,"%s: sector(0x%x) erase", __func__, start_addr);
        do {
            ret = norflash_api_erase(module,
                                     start_addr,
                                     s_size,
                                     true);
            if (NORFLASH_API_OK == ret)
            {
                // APP_FLASH_TRACE(1,"%s: ok!", __func__);
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
                app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
            }
            else
            {
                uint32_t addr;
                norflash_api_get_base_addr(module, &addr);

                ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x, size: 0x%x",
                       __func__, ret, start_addr, addr, s_size);
            }
        }while(1);
        start_addr += s_size;
    }
    APP_FLASH_TRACE(1,"%s: erase: 0x%x,0x%x, time= %d(ms)done",
        __func__, start_addr, len, TICKS_TO_MS(hal_sys_timer_get() - start_time));
}

SRAM_TEXT_LOC void app_flash_program(enum NORFLASH_API_MODULE_ID_T module,
                       uint32_t flashOffset,
                       uint8_t *ptr,
                       uint32_t len,
                       bool synWrite)
{
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    bool is_async;
    uint32_t start_addr = 0;
    uint32_t sec_num;
    uint32_t write_len;
    uint32_t written_len = 0;
    uint32_t i;
    uint32_t sector_size = _get_setctor_size(module);

    /// get logic address to write
    ret = norflash_api_get_base_addr(module, &start_addr);
    ASSERT(ret == NORFLASH_API_OK, "%s: get base addr fail, ret = %d", __func__, ret);

    start_addr += flashOffset;

    if (synWrite)
    {
        is_async = false;
    }
    else
    {
        is_async = true;
    }

    APP_FLASH_TRACE(1,"%s: write: 0x%x,0x%x", __func__, start_addr, len);
    sec_num = (len + sector_size -1)/sector_size;

    for(i = 0; i < sec_num; i++)
    {
        if(len - written_len > sector_size)
        {
            write_len = sector_size;
        }
        else
        {
            write_len = len - written_len ;
        }

        do
        {
            ret = norflash_api_write(module,
                                     start_addr + written_len,
                                     ptr + written_len,
                                     write_len,
                                     is_async);

            if (NORFLASH_API_OK == ret)
            {
                APP_FLASH_TRACE(1,"%s: norflash_api_write ok!", __func__);
                written_len += write_len;
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                APP_FLASH_TRACE(1,"%s:buffer full! To flush it.", __func__);
                app_flash_flush_pending_op(module, NORFLASH_API_WRITTING);
            }
            else
            {
                ASSERT(0, "%s: norflash_api_write failed. ret = %d", __FUNCTION__, ret);
            }
        } while (1);
    }
    // APP_FLASH_TRACE(1,"%s: write: 0x%x,0x%x done.", __func__, start_addr, len);
}

SRAM_TEXT_LOC void app_flash_read(enum NORFLASH_API_MODULE_ID_T module,
                    uint32_t flashOffset,
                    uint8_t *ptr,
                    uint32_t len)
{
    /// get flash address according to the flash module and offset
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t start_addr = flashOffset;
    uint32_t lock;
    uint32_t sec_num;
    uint32_t read_len;
    uint32_t has_read_len = 0;
    uint32_t i;
    uint32_t leftBytesInFirstSector;
    uint32_t firstReadBytes;
    uint32_t sector_size = _get_setctor_size(module);

    ret = norflash_api_get_base_addr(module, &start_addr);
    ASSERT(NORFLASH_API_OK == ret,
           "%s get base addr fail, ret:%d, module:%d",
           __func__, ret, module);

    start_addr += flashOffset;

    leftBytesInFirstSector = sector_size - (start_addr % sector_size);

    if (leftBytesInFirstSector >= len)
    {
        sec_num = 1;
        firstReadBytes = len;
    }
    else
    {
        sec_num = 1 + (len-leftBytesInFirstSector + sector_size -1)/sector_size;
        firstReadBytes = leftBytesInFirstSector;
    }

    for(i = 0; i < sec_num; i++)
    {
        if (0 == i)
        {
            read_len = firstReadBytes;
        }
        else
        {
            if (len - has_read_len > sector_size)
            {
                read_len = sector_size;
            }
            else
            {
                read_len = len - has_read_len ;
            }
        }

        lock = int_lock_global();
        /// read flash, OTA_MODULE should take flash re-map into consideration if it is enabled
        ret = norflash_api_read(module, start_addr + has_read_len, ptr + has_read_len, read_len);

        int_unlock_global(lock);

        ASSERT(NORFLASH_API_OK == ret, "%s %d ret:%d", __func__, __LINE__, ret);
        has_read_len += read_len;
    }
}

SRAM_TEXT_LOC void app_flash_sync_read(enum NORFLASH_API_MODULE_ID_T module,
                    uint32_t flashOffset,
                    uint8_t *ptr,
                    uint32_t len)
{
    /// get flash address according to the flash module and offset
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t start_addr = flashOffset;
    uint32_t lock;
    uint32_t sec_num;
    uint32_t read_len;
    uint32_t has_read_len = 0;
    uint32_t i;
    uint32_t leftBytesInFirstSector;
    uint32_t firstReadBytes;
    uint32_t sector_size = _get_setctor_size(module);

    ret = norflash_api_get_base_addr(module, &start_addr);
    ASSERT(NORFLASH_API_OK == ret,
           "%s %d ret:%d, module:%d, offset:%d, len:%d",
           __func__, __LINE__, ret, module, flashOffset, len);

    start_addr += flashOffset;

    leftBytesInFirstSector = sector_size - (start_addr % sector_size);

    if (leftBytesInFirstSector >= len)
    {
        sec_num = 1;
        firstReadBytes = len;
    }
    else
    {
        sec_num = 1 + (len-leftBytesInFirstSector + sector_size -1)/sector_size;
        firstReadBytes = leftBytesInFirstSector;
    }

    for(i = 0; i < sec_num; i++)
    {
        if (0 == i)
        {
            read_len = firstReadBytes;
        }
        else
        {
            if (len - has_read_len > sector_size)
            {
                read_len = sector_size;
            }
            else
            {
                read_len = len - has_read_len ;
            }
        }


        lock = int_lock_global();
        /// read flash, OTA_MODULE should take flash re-map into consideration if it is enabled
        ret = norflash_sync_read(module, start_addr + has_read_len, ptr + has_read_len, read_len);
        int_unlock_global(lock);
        ASSERT(NORFLASH_API_OK == ret, "%s %d ret:%d", __func__, __LINE__, ret);
        has_read_len += read_len;
    }
}

SRAM_TEXT_LOC enum HAL_FLASH_ID_T app_flash_get_dev_id_by_addr(uint32_t addr)
{
    return norflash_api_get_dev_id_by_addr(addr);
}

#if 0
// static void app_flash_api_test_callback(void* param)
// {
//     NORFLASH_API_OPERA_RESULT *opera_result;

//     opera_result = (NORFLASH_API_OPERA_RESULT*)param;

//     TRACE(3,"%s type:%d, addr:0x%x",
//           __func__,
//           opera_result->type,
//           opera_result->addr);
//     TRACE(2,"len:0x%x, result:%d.",
//           opera_result->len,
//           opera_result->result);
// }

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

extern uint32_t __userdata_start[];
extern uint32_t __userdata_end[];
SRAM_TEXT_LOC void app_flash_api_test(void)
{
    uint8_t ptr[100];
    uint32_t len = 100;

    // app_flash_register_module(NORFLASH_API_MODULE_ID_USERDATA,
    //                             HAL_FLASH_ID_1,
    //                             ((uint32_t)__userdata_start),
    //                             ((uint32_t)__userdata_end - (uint32_t)__userdata_start),
    //                             (uint32_t)app_flash_api_test_callback);

    app_flash_sync_read(NORFLASH_API_MODULE_ID_USERDATA,
                        0,
                        ptr,
                        len);

    TRACE(0,"app flash read:");
    DUMP8("0x%02x ", ptr, len);

    osDelay(200);

    app_flash_erase(NORFLASH_API_MODULE_ID_USERDATA, 0, ((uint32_t)__userdata_end - (uint32_t)__userdata_start));

    osDelay(200);

    app_flash_sync_read(NORFLASH_API_MODULE_ID_USERDATA,
                        0,
                        ptr,
                        len);

    TRACE(0,"app flash erase:");
    DUMP8("0x%02x ", ptr, len);

    osDelay(200);

    for(uint32_t i=0; i<len; i++)
        ptr[i] = i;
    app_flash_program(NORFLASH_API_MODULE_ID_USERDATA,
                       0,
                       ptr,
                       len,
                       1);
    memset(ptr, 0, len);
    app_flash_sync_read(NORFLASH_API_MODULE_ID_USERDATA,
                        0,
                        ptr,
                        len);
    TRACE(0,"app flash write:");
    DUMP8("0x%02x ", ptr, len);
}
#endif

/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#include "cmsis.h"

#if (defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)) || \
        (defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U))

#include "cachel1_armv7.h"
#include "hal_cache.h"
#include "hal_location.h"

#define CACHE_SIZE_ICACHE                   0x8000
#define CACHE_SIZE_DCACHE                   0x8000

#define CACHE_LINE_SIZE_ICACHE              __SCB_ICACHE_LINE_SIZE
#define CACHE_LINE_SIZE_DCACHE              __SCB_DCACHE_LINE_SIZE

#define REG(a)                              (*(volatile uint32_t *)(a))

#define MSCR                                0xE001E000

__STATIC_FORCEINLINE void cacheip_enable_cache(enum HAL_CACHE_ID_T id)
{
#if (defined (__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE > 0U))
    if (id == HAL_CACHE_ID_I_CACHE) {
        REG(MSCR) |= (1 << 13);
    } else {
        REG(MSCR) |= (1 << 12);
    }
#endif

    if (id == HAL_CACHE_ID_I_CACHE) {
        SCB_EnableICache();
#if (defined (__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE > 0U))
        // Enable Loop and branch info cache
        SCB->CCR |= SCB_CCR_LOB_Msk;
        __DSB();
        __ISB();
#endif
    } else {
        SCB_EnableDCache();
    }
}

__STATIC_FORCEINLINE void cacheip_disable_cache(enum HAL_CACHE_ID_T id)
{
    if (id == HAL_CACHE_ID_I_CACHE) {
#if (defined (__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE > 0U))
        // Disable Loop and branch info cache
        SCB->CCR &= ~SCB_CCR_LOB_Msk;
        __DSB();
        __ISB();
        SCB->BPIALL = 0UL;
        __DSB();
        __ISB();
#endif
        SCB_DisableICache();
    } else {
        SCB_DisableDCache();
    }

#if (defined (__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE > 0U))
    if (id == HAL_CACHE_ID_I_CACHE) {
        REG(MSCR) &= ~(1 << 13);
    } else {
        REG(MSCR) &= ~(1 << 12);
    }
#endif
}

uint8_t BOOT_TEXT_FLASH_LOC hal_cache_enable(enum HAL_CACHE_ID_T id)
{
    cacheip_enable_cache(id);
    return 0;
}

uint8_t SRAM_TEXT_LOC hal_cache_disable(enum HAL_CACHE_ID_T id)
{
    cacheip_disable_cache(id);
    return 0;
}

uint8_t BOOT_TEXT_FLASH_LOC hal_cache_writebuffer_enable(enum HAL_CACHE_ID_T id)
{
    return 0;
}

uint8_t hal_cache_writebuffer_disable(enum HAL_CACHE_ID_T id)
{
    return 0;
}

uint8_t hal_cache_writebuffer_flush(enum HAL_CACHE_ID_T id)
{
    // Drain the store buffer
    __DSB();
    return 0;
}

uint8_t BOOT_TEXT_FLASH_LOC hal_cache_writeback_enable(enum HAL_CACHE_ID_T id)
{
    return 0;
}

uint8_t hal_cache_writeback_disable(enum HAL_CACHE_ID_T id)
{
    return 0;
}

// Wrap is configured during flash init
uint8_t BOOT_TEXT_SRAM_LOC hal_cache_wrap_enable(enum HAL_CACHE_ID_T id)
{
    return 0;
}

uint8_t BOOT_TEXT_SRAM_LOC hal_cache_wrap_disable(enum HAL_CACHE_ID_T id)
{
    return 0;
}

// Flash timing calibration might need to invalidate cache
uint8_t BOOT_TEXT_SRAM_LOC hal_cache_invalidate(enum HAL_CACHE_ID_T id, uint32_t start_address, uint32_t len)
{
    if (id == HAL_CACHE_ID_I_CACHE) {
        if (len > CACHE_SIZE_ICACHE - CACHE_LINE_SIZE_ICACHE) {
            SCB_InvalidateICache();
        } else {
            SCB_InvalidateICache_by_Addr((void *)start_address, len);
        }
    } else {
        // Sync and invalidate operation is much slower than invalidate operation,
        // so sync and invalidate the whole cache only when len is much larger than cache size
        if (len > CACHE_SIZE_DCACHE * 2 - CACHE_LINE_SIZE_DCACHE) {
            SCB_CleanInvalidateDCache();
        } else {
            // PREREQUISITE:
            // To support cache operations not aligned with cache line size,
            // all the buffer shared between processor and hardware device (e.g., DMA)
            // must be invalidated before assigning to the hardware device.
            // Otherwise the hardware device output data might be overwritten by
            // cache sync operaton.

            uint32_t offset;

            // Align start_address to cache line size
            offset = start_address & (CACHE_LINE_SIZE_DCACHE - 1);
            if (offset) {
                offset = CACHE_LINE_SIZE_DCACHE - offset;
                SCB_CleanInvalidateDCache_by_Addr((void *)start_address, offset);
                start_address += offset;
                len -= offset;
            }
            // Align len to cache line size
            offset = len & (CACHE_LINE_SIZE_DCACHE - 1);
            if (offset) {
                len -= offset;
                SCB_CleanInvalidateDCache_by_Addr((void *)start_address + len, offset);
            }
            SCB_InvalidateDCache_by_Addr((void *)start_address, len);
        }
    }

    return 0;
}

uint8_t hal_cache_invalidate_all(enum HAL_CACHE_ID_T id)
{
    if (id == HAL_CACHE_ID_I_CACHE) {
        SCB_InvalidateICache();
    } else {
        SCB_InvalidateDCache();
    }

    return 0;
}

uint8_t BOOT_TEXT_FLASH_LOC hal_cache_boot_sync_all(enum HAL_CACHE_ID_T id)
{
    if (id == HAL_CACHE_ID_D_CACHE) {
        SCB_CleanDCache();
    }

    return 0;
}

uint8_t hal_cache_sync_all(enum HAL_CACHE_ID_T id)
{
    if (id == HAL_CACHE_ID_D_CACHE) {
        SCB_CleanDCache();
    }

    return 0;
}

uint8_t hal_cache_sync_invalidate_all(enum HAL_CACHE_ID_T id)
{
    if (id == HAL_CACHE_ID_I_CACHE) {
        SCB_InvalidateICache();
    } else {
        SCB_CleanInvalidateDCache();
    }

    return 0;
}

uint8_t BOOT_TEXT_SRAM_LOC hal_cache_sync(enum HAL_CACHE_ID_T id, uint32_t start_address, uint32_t len)
{
    if (id == HAL_CACHE_ID_D_CACHE) {
        if (len > CACHE_SIZE_DCACHE - CACHE_LINE_SIZE_DCACHE) {
            SCB_CleanDCache();
        } else {
            SCB_CleanDCache_by_Addr((void *)start_address, len);
        }
    }

    return 0;
}

uint8_t BOOT_TEXT_SRAM_LOC hal_cache_sync_invalidate(enum HAL_CACHE_ID_T id, uint32_t start_address, uint32_t len)
{
    if (id == HAL_CACHE_ID_I_CACHE) {
        if (len > CACHE_SIZE_ICACHE - CACHE_LINE_SIZE_ICACHE) {
            SCB_InvalidateICache();
        } else {
            SCB_InvalidateICache_by_Addr((void *)start_address, len);
        }
    } else {
        if (len > CACHE_SIZE_DCACHE - CACHE_LINE_SIZE_DCACHE) {
            SCB_CleanInvalidateDCache();
        } else {
            SCB_CleanInvalidateDCache_by_Addr((void *)start_address, len);
        }
    }

    return 0;
}

#ifdef CORE_SLEEP_POWER_DOWN
SRAM_BSS_LOC
static bool cache_enabled[HAL_CACHE_ID_NUM];

void hal_cache_sleep(void)
{
    bool cache_en;

    cache_en = !!(SCB->CCR & SCB_CCR_DC_Msk);
    if (cache_en) {
        // D-Cache will be cleaned and invalidated after disabled
        cacheip_disable_cache(HAL_CACHE_ID_D_CACHE);
    }
    cache_enabled[HAL_CACHE_ID_D_CACHE] = cache_en;

    cache_en = !!(SCB->CCR & SCB_CCR_IC_Msk);
    if (cache_en) {
        cacheip_disable_cache(HAL_CACHE_ID_I_CACHE);
    }
    cache_enabled[HAL_CACHE_ID_I_CACHE] = cache_en;
}

void SRAM_TEXT_LOC hal_cache_wakeup(void)
{
    __DSB();
    __ISB();
    if (cache_enabled[HAL_CACHE_ID_I_CACHE]) {
        cacheip_enable_cache(HAL_CACHE_ID_I_CACHE);
    }
    if (cache_enabled[HAL_CACHE_ID_D_CACHE]) {
        cacheip_enable_cache(HAL_CACHE_ID_D_CACHE);
    }
}
#endif

#endif

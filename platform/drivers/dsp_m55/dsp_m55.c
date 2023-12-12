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
#include "dsp_m55.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_overlay_subsys.h"
#include "hal_psc.h"
#include "hal_sys2bth.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "rx_dsp_m55_trc.h"
#include "string.h"
#include "system_subsys.h"

#ifdef DSP_M55_FLASH_BOOT
#include "hal_norflash.h"
#endif

#ifndef DSP_M55_CODE_OFFSET
#ifdef DSP_M55_ALT_BOOT_FLASH
#define DSP_M55_CODE_OFFSET                 (FLASH_SIZE / 2)
#else
#define DSP_M55_CODE_OFFSET                 0
#endif
#endif

#ifdef DSP_M55_ALT_BOOT_FLASH
#define DSP_M55_FLASH_BOOT_STRUCT_ADDR      (FLASH_BASE + DSP_M55_CODE_OFFSET)
#else
#define DSP_M55_FLASH_BOOT_STRUCT_ADDR      (FLASH1_BASE + DSP_M55_CODE_OFFSET)
#endif

extern uint32_t __dsp_m55_code_start_flash[];
extern uint32_t __dsp_m55_code_end_flash[];

static const uint16_t m55_wfi_code[2] = {
    0xBF30, // WFI
    0xE7FD, // b (pc - 6)
};

#if defined(DSP_M55_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
static DSP_M55_RX_IRQ_HANDLER app_rx_hdlr;

static unsigned int general_rx_handler(const void *data, unsigned int len)
{
    if (dsp_m55_trace_rx_handler(data, len)) {
        return len;
    }

    if (app_rx_hdlr) {
        return app_rx_hdlr(data, len);
    }

    return len;
}
#endif

static void sys_ram_set_value_32(uint32_t addr, uint32_t val)
{
    uint32_t new_addr;
    uint32_t POSSIBLY_UNUSED offset;

    ASSERT((addr & 3) == 0, "%s: Bad addr=0x%X", __func__, addr);

    new_addr = addr;

    if (0) {
#if (M55_ITCM_SIZE > 0)
    } else if (M55_ITCM_BASE <= addr && addr < (M55_ITCM_BASE + M55_ITCM_SIZE)) {
        new_addr -= M55_MIN_ITCM_BASE;
        offset = new_addr & (SHR_RAM_BLK_SIZE / 2 - 1);
        if (offset) {
            if (offset < SHR_RAM_BLK_SIZE / 4) {
                offset *= 2;
            } else {
                offset = (offset - SHR_RAM_BLK_SIZE / 4) * 2 + 4;
            }
            new_addr = (new_addr & ~(SHR_RAM_BLK_SIZE / 2 - 1)) + offset;
        }
        new_addr += SYS_RAM0_BASE;
#endif
#if (M55_DTCM_SIZE > 0)
    } else if (M55_DTCM_BASE <= addr && addr < (M55_DTCM_BASE + M55_DTCM_SIZE)) {
        new_addr -= M55_MIN_DTCM_BASE;
        offset = new_addr & (SHR_RAM_BLK_SIZE - 1);
        if (offset) {
            if (offset & 8) {
                offset = offset / 16 * 8 + (offset & (8 - 1)) + SHR_RAM_BLK_SIZE / 2;
            } else {
                offset = offset / 16 * 8 + (offset & (8 - 1));
            }
            new_addr = (new_addr & ~(SHR_RAM_BLK_SIZE - 1)) + offset;
        }
        new_addr = (SYS_RAM2_BASE - SHR_RAM_BLK_SIZE) -
            new_addr / SHR_RAM_BLK_SIZE * SHR_RAM_BLK_SIZE +
            offset;
#endif
    }

    *(volatile uint32_t *)new_addr = val;
}

int dsp_m55_open_in_sys_ram(DSP_M55_RX_IRQ_HANDLER rx_hdlr, DSP_M55_TX_IRQ_HANDLER tx_hdlr)
{
    int ret;
    uint32_t code_start;
#ifdef CHIP_BEST1603
    volatile uint32_t *vector_base;
#else
    uint32_t entry;
    uint32_t sp;
#endif
    TR_INFO(0, "ITCM BASE:%08x SIZE:%08x", M55_ITCM_BASE, M55_ITCM_SIZE);
    TR_INFO(0, "DTCM BASE:%08x SIZE:%08x", M55_DTCM_BASE, M55_DTCM_SIZE);
    TR_INFO(0, "SRAM BASE:%08x SIZE:%08x", M55_SYS_RAM_BASE, M55_SYS_RAM_SIZE);

    hal_psc_sys_m55_enable();
    hal_cmu_m55_clock_enable();
    hal_cmu_m55_reset_clear();

    osDelay(2);

#ifdef DSP_M55_FLASH_BOOT
#ifndef DSP_M55_ALT_BOOT_FLASH
    hal_cmu_sys_flash_io_enable();
    hal_cmu_clock_enable(HAL_CMU_MOD_H_FLASH_SYS);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_FLASH_SYS);
#if defined(FLASH1_CTRL_BASE)
    hal_norflash_force_wakeup(HAL_FLASH_ID_1);
#endif
#endif

    ret = subsys_check_boot_struct(DSP_M55_FLASH_BOOT_STRUCT_ADDR, &code_start);
    if (ret) {
        ASSERT(false, "%s: Bad subsys boot struct", __func__);
        return 1;
    }
#else
    struct HAL_DMA_CH_CFG_T dma_cfg;
    enum HAL_DMA_RET_T dma_ret;
    uint32_t addr;
    uint32_t len;
    uint32_t load_start;
    uint32_t remains;
    int check_magic;
    const struct SUBSYS_IMAGE_DESC_T *desc;

#ifdef DSP_M55_LOAD_FLASH_ADDR
    addr = DSP_M55_LOAD_FLASH_ADDR;
    len = DSP_M55_LOAD_FLASH_SIZE;
    check_magic = true;
#else
    addr = (uint32_t)__dsp_m55_code_start_flash;
    len = (uint32_t)__dsp_m55_code_end_flash - addr;
    check_magic = false;
#endif
    TR_INFO(0, "%s: addr=0x%08X len=0x%X (%u)", __func__, addr, len, len);

    ret = subsys_loader_check_image(addr, len, check_magic);
    ASSERT(ret == 0, "%s: Failed to check image: %d", __func__, ret);

    desc = subsys_loader_get_image_desc(addr);
    ASSERT(desc->type == SUBSYS_IMAGE_TYPE_SIMPLE, "%s: Bad image type: %d", __func__, desc->type);

    load_start = addr + desc->code_start_offset;
    code_start = desc->u.exec_addr;
    remains = (desc->image_size - desc->code_start_offset) / sizeof(uint32_t);
    TR_INFO(0, "%s: load_start=0x%08X code_start=0x%08X remains=0x%X (%u)", __func__, load_start, code_start, remains, remains);

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.dst_periph = HAL_GPDMA_MEM;
    dma_cfg.ch = hal_dma_get_chan(dma_cfg.dst_periph, HAL_DMA_LOW_PRIO);
    if (dma_cfg.ch == HAL_DMA_CHAN_NONE) {
        memcpy((void *)code_start, (void *)load_start, remains * sizeof(uint32_t));
    } else {
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.handler = NULL;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.src_periph = HAL_GPDMA_MEM;
        dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.type = HAL_DMA_FLOW_M2M_DMA;
        dma_cfg.try_burst = false;

        dma_cfg.dst = code_start;
        dma_cfg.src = load_start;

        while (remains > 0) {
            if (remains > HAL_DMA_MAX_DESC_XFER_SIZE) {
                dma_cfg.src_tsize = HAL_DMA_MAX_DESC_XFER_SIZE;
            } else {
                dma_cfg.src_tsize = remains;
            }

            dma_ret = hal_dma_start(&dma_cfg);
            ASSERT(dma_ret == HAL_DMA_OK, "%s: Failed to start dma: %d", __func__, dma_ret);

            while (hal_dma_chan_busy(dma_cfg.ch));

            dma_cfg.dst += dma_cfg.src_tsize * 4;
            dma_cfg.src += dma_cfg.src_tsize * 4;
            remains -= dma_cfg.src_tsize;
        }

        hal_dma_free_chan(dma_cfg.ch);
    }
#endif

#if defined(DSP_M55_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
    app_rx_hdlr = rx_hdlr;
    rx_hdlr = general_rx_handler;
#endif
    ret = hal_sys2bth_open(HAL_SYS2BTH_ID_0, rx_hdlr, tx_hdlr, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(HAL_SYS2BTH_ID_0);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);

#ifdef DSP_M55_TRC_TO_MCU
    dsp_m55_trace_server_open();
#endif

#ifdef CHIP_BEST1603
    vector_base = (volatile uint32_t *)(DSP_M55_SYS_RAM_BASE + DSP_M55_SYS_RAM_SIZE - 128);

    vector_base[0] = (uint32_t)vector_base;
    vector_base[1] = code_start | 1;
    hal_cmu_m55_cpu_reset_clear((uint32_t)vector_base);
    hal_cmu_m55_start_cpu();
#else
    sp = (M55_SYS_RAM_BASE + M55_SYS_RAM_SIZE - 128);
    entry = code_start | 1;
    hal_cmu_m55_start_cpu(sp, entry);
#endif

    return 0;
}

void dsp_m55_setup_overlay(uint32_t addr)
{
    const struct OVERLAY_INFO_T *overlay_info = subsys_loader_get_overlay_info(addr);
    const struct SUBSYS_OVERLAY_EXEC_T *exec_info = &overlay_info->exec_info;
    const struct SUBSYS_OVERLAY_ITEM_T *seg_info = &overlay_info->overlay_seg[0];

    uint32_t overlay_text_sz = exec_info->text_end_addr - exec_info->text_start_addr;
    uint32_t overlay_data_sz = exec_info->data_end_addr - exec_info->data_start_addr;
    uint32_t text_vma = exec_info->text_start_addr;
    uint32_t text_size = overlay_text_sz;

    uint32_t data_vma = exec_info->data_start_addr;
    uint32_t data_size = overlay_data_sz;

    hal_overlay_subsys_set_text_exec_addr(OVERLAY_M55, text_vma, text_size,
                                            data_vma, data_size);

    for (int id = 0; id < SUBSYS_OVERLAY_CNT; id++) {
        uint32_t text_load_start, text_load_stop;
        uint32_t data_load_start, data_load_stop;

        text_load_start = addr + seg_info->text_offset;
        text_load_stop = text_load_start + seg_info->text_size;
        hal_overlay_subsys_set_text_addr(OVERLAY_M55, id, text_load_start, text_load_stop);

        data_load_start = addr + seg_info->data_offset;
        data_load_stop = data_load_start + seg_info->data_size;
        hal_overlay_subsys_set_data_addr(OVERLAY_M55, id, data_load_start, data_load_stop);

        seg_info++;
    }

    //for test...
    /* hal_overlay_subsys_load(OVERLAY_M55, 1); */
}

int dsp_m55_open(DSP_M55_RX_IRQ_HANDLER rx_hdlr, DSP_M55_TX_IRQ_HANDLER tx_hdlr)
{
    int ret;
    uint32_t entry;

    // Change tcm to sys ram
    hal_cmu_sys_m55_dtcm_cfg(false);

    hal_psc_sys_m55_enable();
    hal_cmu_m55_clock_enable();
    hal_cmu_m55_reset_clear();

    osDelay(2);

    sys_ram_set_value_32(M55_DTCM_BASE + 0, M55_MAILBOX_BASE);
    sys_ram_set_value_32(M55_DTCM_BASE + 4, (M55_DTCM_BASE + 8) | 1);
    sys_ram_set_value_32(M55_DTCM_BASE + 8, (m55_wfi_code[1] << 16) | m55_wfi_code[0]);
    __DSB();

    // Restore tcm cfg
    hal_cmu_sys_m55_dtcm_cfg(true);
#ifdef CHIP_BEST1603
    hal_cmu_m55_cpu_reset_clear(M55_DTCM_BASE);
#else
    hal_cmu_m55_start_cpu_vector(M55_DTCM_BASE);
#endif
#ifdef DSP_M55_FLASH_BOOT
#ifndef DSP_M55_ALT_BOOT_FLASH
    hal_cmu_sys_flash_io_enable();
    hal_cmu_clock_enable(HAL_CMU_MOD_H_FLASH_SYS);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_FLASH_SYS);
#if defined(FLASH1_CTRL_BASE)
    hal_norflash_force_wakeup(HAL_FLASH_ID_1);
#endif
#endif

    ret = subsys_check_boot_struct(DSP_M55_FLASH_BOOT_STRUCT_ADDR, &entry);
    if (ret) {
        ASSERT(false, "%s: Bad subsys boot struct", __func__);
        return 1;
    }
#else
    struct HAL_DMA_CH_CFG_T dma_cfg;
    enum HAL_DMA_RET_T dma_ret;
    uint32_t addr;
    uint32_t len;
    uint32_t seg_cnt;
    uint32_t i;
    uint32_t remains;
    uint32_t dst;
    uint32_t src;
    int check_magic;
    const struct SUBSYS_IMAGE_DESC_T *desc;
    const struct CODE_SEG_MAP_ITEM_T *seg_item;

#ifdef DSP_M55_LOAD_FLASH_ADDR
    addr = DSP_M55_LOAD_FLASH_ADDR;
    len = DSP_M55_LOAD_FLASH_SIZE;
    check_magic = true;
#else
    addr = (uint32_t)__dsp_m55_code_start_flash;
    len = (uint32_t)__dsp_m55_code_end_flash - addr;
    check_magic = false;
#endif
    TR_INFO(0, "%s: addr=0x%08X len=0x%X (%u)", __func__, addr, len, len);

    ret = subsys_loader_check_image(addr, len, check_magic);
    ASSERT(ret == 0, "%s: Failed to check image: %d", __func__, ret);

    desc = subsys_loader_get_image_desc(addr);
    ASSERT(desc->type == SUBSYS_IMAGE_TYPE_SEGMENT || desc->type == SUBSYS_IMAGE_TYPE_OVERLAY,
        "%s: Bad image type: %d", __func__, desc->type);

    seg_cnt = desc->u.seg_map_size / sizeof(struct CODE_SEG_MAP_ITEM_T);
    ASSERT(seg_cnt * sizeof(struct CODE_SEG_MAP_ITEM_T) == desc->u.seg_map_size,
        "%s: Bad seg size: %u", __func__, desc->u.seg_map_size);
    TR_INFO(0, "%s: seg_size=0x%X seg_cnt=%X", __func__, desc->u.seg_map_size, seg_cnt);

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.dst_periph = HAL_GPDMA_MEM;
    dma_cfg.ch = hal_dma_get_chan(dma_cfg.dst_periph, HAL_DMA_LOW_PRIO);
    if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.handler = NULL;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.src_periph = HAL_GPDMA_MEM;
        dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.type = HAL_DMA_FLOW_M2M_DMA;
        dma_cfg.try_burst = false;
    }

    seg_item = subsys_loader_get_seg_map(addr);
    // Entry is in the first segment
    entry = seg_item->exec_addr | 1;

    for (i = 0; i < seg_cnt; i++) {
        ASSERT((seg_item->load_offset & 3) == 0 && (seg_item->size & 3) == 0,
            "%s: Bad alignment: load_offset=%x size=%x", __func__, seg_item->load_offset, seg_item->size);
        ASSERT(desc->code_start_offset <= seg_item->load_offset && seg_item->load_offset < desc->image_size,
            "%s: Bad load_offset=%x", __func__, seg_item->load_offset);
        ASSERT(seg_item->load_offset + seg_item->size <= desc->image_size,
            "%s: Bad load_offset=%x size=%x", __func__, seg_item->load_offset, seg_item->size);

        dst = seg_item->exec_addr;
        src = addr + seg_item->load_offset;
        remains = seg_item->size / 4;
        TR_INFO(0, "%s:[%u] dst=0x%X src=0x%X size=%X", __func__, i, dst, src, seg_item->size);

        if (dma_cfg.ch == HAL_DMA_CHAN_NONE) {
            memcpy((void *)dst, (void *)src, remains * 4);
        } else {
            dma_cfg.dst = dst;
            dma_cfg.src = src;

            while (remains > 0) {
                if (remains > HAL_DMA_MAX_DESC_XFER_SIZE) {
                    dma_cfg.src_tsize = HAL_DMA_MAX_DESC_XFER_SIZE;
                } else {
                    dma_cfg.src_tsize = remains;
                }
                dma_ret = hal_dma_start(&dma_cfg);
                ASSERT(dma_ret == HAL_DMA_OK, "%s: Failed to start dma: %d", __func__, dma_ret);

                while (hal_dma_chan_busy(dma_cfg.ch));

                dma_cfg.dst += dma_cfg.src_tsize * 4;
                dma_cfg.src += dma_cfg.src_tsize * 4;
                remains -= dma_cfg.src_tsize;
            }
        }

        seg_item++;
    }

    if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
        hal_dma_free_chan(dma_cfg.ch);
    }

    if (desc->type == SUBSYS_IMAGE_TYPE_OVERLAY) {
        dsp_m55_setup_overlay(addr);
    }
#endif

#if defined(DSP_M55_TRC_TO_MCU) && defined(RMT_TRC_IN_MSG_CHAN)
    app_rx_hdlr = rx_hdlr;
    rx_hdlr = general_rx_handler;
#endif
    ret = hal_sys2bth_open(HAL_SYS2BTH_ID_0, rx_hdlr, tx_hdlr, false);
    ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    ret = hal_sys2bth_start_recv(HAL_SYS2BTH_ID_0);
    ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);

#ifdef DSP_M55_TRC_TO_MCU
    dsp_m55_trace_server_open();
#endif

    *(volatile uint32_t *)(M55_DTCM_BASE + 0) =  M55_MAILBOX_BASE;
    *(volatile uint32_t *)(M55_DTCM_BASE + 4) =  entry;
    __DSB();

#ifdef CHIP_BEST1603
    hal_cmu_m55_cpu_reset_set();
    hal_cmu_m55_cpu_reset_clear(DSP_M55_DTCM_BASE);
#else
    hal_cmu_m55_stop_cpu();
    hal_cmu_m55_start_cpu_vector(M55_DTCM_BASE);
#endif

    return 0;
}

int dsp_m55_close(void)
{
    TR_INFO(0, "%s", __FUNCTION__);

#ifdef DSP_M55_TRC_TO_MCU
    dsp_m55_trace_server_close();
#endif

    hal_sys2bth_close(HAL_SYS2BTH_ID_0);

    hal_cmu_m55_reset_set();
    hal_cmu_m55_clock_disable();
    hal_psc_sys_m55_disable();

    return 0;
}

int dsp_m55_send_seq(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_sys2bth_send_seq(HAL_SYS2BTH_ID_0, data, len, seq);
}

int dsp_m55_send(const void *data, unsigned int len)
{
    return hal_sys2bth_send(HAL_SYS2BTH_ID_0, data, len);
}

int dsp_m55_tx_active(unsigned int seq)
{
    return hal_sys2bth_tx_active(HAL_SYS2BTH_ID_0, seq);
}

int dsp_hifi_send_seq(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_sys2bth_send_seq(HAL_SYS2BTH_ID_1, data, len, seq);
}

int dsp_hifi_send(const void *data, unsigned int len)
{
    return hal_sys2bth_send(HAL_SYS2BTH_ID_1, data, len);
}

int dsp_hifi_tx_active(unsigned int seq)
{
    return hal_sys2bth_tx_active(HAL_SYS2BTH_ID_1, seq);
}


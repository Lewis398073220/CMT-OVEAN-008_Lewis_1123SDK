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
#include "sensor_hub.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_mcu2sens.h"
#include "hal_psc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "rx_sens_trc.h"
#include "string.h"
#include "system_subsys.h"

extern uint32_t __sensor_hub_code_start_flash[];
extern uint32_t __sensor_hub_code_end_flash[];

int sensor_hub_open_withipc(SENSOR_HUB_RX_IRQ_HANDLER rx_hdlr, SENSOR_HUB_TX_IRQ_HANDLER tx_hdlr, uint8_t with_ipc)
{
    uint32_t code_start;
    POSSIBLY_UNUSED int ret;
    uint32_t entry;
    uint32_t sp;

    hal_psc_sens_enable();
    hal_cmu_sens_clock_enable();
    hal_cmu_sens_reset_clear();

    osDelay(2);

    hal_cmu_sens_ram_cfg();

#ifdef SENS_FLASH_BOOT
#ifdef NO_SUBSYS_FLASH_ADDR_ACCESS
    const struct HAL_NORFLASH_CONFIG_T cfg = {
        .source_clk = HAL_NORFLASH_SPEED_13M * 2,
        .speed = HAL_NORFLASH_SPEED_13M,
        .mode = HAL_NORFLASH_OP_MODE_STAND_SPI |
                HAL_NORFLASH_OP_MODE_FAST_SPI |
                HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
                0,
        .opt =  HAL_NORFLASH_OPT_CALIB_FLASH_ID |
                0,
    };
    enum HAL_FLASH_ID_T id;

    id = hal_norflash_addr_to_id(SENS_FLASH_BOOT_STRUCT_ADDR);
    ASSERT(id < HAL_FLASH_ID_NUM, "%s: addr not in flash: 0x%08X", __func__, SENS_FLASH_BOOT_STRUCT_ADDR);
    ret = hal_norflash_open(id, &cfg);
    ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to open flash: %d", __func__, ret);
#endif

    ret = subsys_check_boot_struct(SENS_FLASH_BOOT_STRUCT_ADDR, &code_start);
    if (ret) {
        ASSERT(false, "%s: Bad sbusys boot struct", __func__);
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

#ifdef SENS_LOAD_FLASH_ADDR
    addr = SENS_LOAD_FLASH_ADDR;
    len = SENS_LOAD_FLASH_SIZE;
    check_magic = true;
#else
    addr = (uint32_t)__sensor_hub_code_start_flash;
    len = (uint32_t)__sensor_hub_code_end_flash - addr;
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

    ASSERT(code_start > SENS_RAM_BASE && code_start < (SENS_RAM_BASE + SENS_RAM_SIZE),
           "Invalid SENSOR RAM config!!! code_start %08x (RAM base %08x size %08x)",
            code_start, SENS_RAM_BASE, SENS_RAM_SIZE);

    if (with_ipc)
    {
        ret = hal_mcu2sens_open(HAL_MCU2SENS_ID_0, rx_hdlr, tx_hdlr, false);
        ASSERT(ret == 0, "hal_mcu2sens_open failed: %d", ret);

        ret = hal_mcu2sens_start_recv(HAL_MCU2SENS_ID_0);
        ASSERT(ret == 0, "hal_mcu2sens_start_recv failed: %d", ret);

#ifdef SENS_TRC_TO_MCU
        sensor_hub_trace_server_open();
#endif
    }
    sp = SENS_MAILBOX_BASE;
    entry = RAM_TO_RAMX(code_start | 1);

    hal_cmu_sens_start_cpu(sp, entry);

    return 0;
}



int sensor_hub_open(SENSOR_HUB_RX_IRQ_HANDLER rx_hdlr, SENSOR_HUB_TX_IRQ_HANDLER tx_hdlr)
{
    return sensor_hub_open_withipc(rx_hdlr, tx_hdlr, true);
}

int sensor_hub_close(void)
{
    TR_INFO(0, "%s", __FUNCTION__);

#ifdef SENS_TRC_TO_MCU
    sensor_hub_trace_server_close();
#endif

    hal_mcu2sens_close(HAL_MCU2SENS_ID_0);

    hal_cmu_sens_reset_set();
    hal_cmu_sens_clock_disable();
    hal_psc_sens_disable();

    return 0;
}

int sensor_hub_send_seq(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_mcu2sens_send_seq(HAL_MCU2SENS_ID_0, data, len, seq);
}

int sensor_hub_send(const void *data, unsigned int len)
{
    return hal_mcu2sens_send(HAL_MCU2SENS_ID_0, data, len);
}

int sensor_hub_tx_active(unsigned int seq)
{
    return hal_mcu2sens_tx_active(HAL_MCU2SENS_ID_0, seq);
}


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
#include "bt_controller.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_cmu.h"
#include "hal_dma.h"
//#include "hal_sys2bth.h"
#include "hal_psc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "rx_btc_trc.h"
#include "string.h"
#include "bt_radio.h"

extern uint32_t __bt_controller_code_start_flash[];
extern uint32_t __bt_controller_code_end_flash[];

int bt_controller_open(BT_CONTROLLER_RX_IRQ_HANDLER rx_hdlr, BT_CONTROLLER_TX_IRQ_HANDLER tx_hdlr)
{
    uint32_t code_start;
    struct HAL_DMA_CH_CFG_T dma_cfg;
    enum HAL_DMA_RET_T dma_ret;
    uint32_t remains;
    //int ret;
    uint32_t entry;
    uint32_t sp;

    TR_INFO(0, "%s: code_start_flash=%p code_end_flash=%p", __FUNCTION__, __bt_controller_code_start_flash, __bt_controller_code_end_flash);

    if (__bt_controller_code_end_flash - __bt_controller_code_start_flash < 2) {
        return 1;
    }

    hal_psc_bt_enable();
    hal_cmu_bt_clock_enable();
    osDelay(1);
    hal_cmu_bt_sys_reset_clear();
    hal_cmu_bt_module_init();
    osDelay(2);

    hal_cmu_btc_ram_cfg();

    code_start = *(__bt_controller_code_end_flash - 4);
    remains = __bt_controller_code_end_flash - __bt_controller_code_start_flash;
    TR_INFO(0, "%s: code_start=0x%X len=0x%X (%u)", __FUNCTION__, code_start, remains * 4, remains * 4);

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.dst_periph = HAL_GPDMA_MEM;
    dma_cfg.ch = hal_dma_get_chan(dma_cfg.dst_periph, HAL_DMA_LOW_PRIO);
    if (dma_cfg.ch == HAL_DMA_CHAN_NONE) {
        memcpy((void *)code_start, __bt_controller_code_start_flash, remains * sizeof(__bt_controller_code_start_flash[0]));
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
        dma_cfg.src = (uint32_t)__bt_controller_code_start_flash;

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

    //ret = hal_sys2bth_open(HAL_SYS2BTH_ID_0, rx_hdlr, tx_hdlr, false);
    //ASSERT(ret == 0, "hal_sys2bth_open failed: %d", ret);

    //ret = hal_sys2bth_start_recv(HAL_SYS2BTH_ID_0);
    //ASSERT(ret == 0, "hal_sys2bth_start_recv failed: %d", ret);

#ifdef BTC_TRC_TO_SYS
    rx_bt_controller_trace();
#endif

    sp = *(__bt_controller_code_end_flash - 1);
    code_start = *(__bt_controller_code_end_flash - 2);
    entry = (code_start | 1);//RAM_TO_RAMX(code_start | 1);

    hal_cmu_btc_start_cpu(sp, entry);

    return 0;
}

int bt_controller_close(void)
{
    TR_INFO(0, "%s", __FUNCTION__);

    //hal_sys2bth_close(HAL_SYS2BTH_ID_0);

    hal_cmu_bt_reset_set();
    hal_cmu_bt_clock_disable();
    hal_psc_bt_disable();

    return 0;
}

void bt_radio_init(void)
{
    btdrv_rf_init();
    btdrv_digital_common_config();
    btdrv_bt_modem_config();
    btdrv_ble_modem_config();
    btdrv_ecc_config();
}

int bt_controller_send_seq(const void *data, unsigned int len, unsigned int *seq)
{
    return 0;//hal_sys2bth_send_seq(HAL_SYS2BTH_ID_0, data, len, seq);
}

int bt_controller_send(const void *data, unsigned int len)
{
    return 0;//hal_sys2bth_send(HAL_SYS2BTH_ID_0, data, len);
}

int bt_controller_tx_active(unsigned int seq)
{
    return 0;//hal_sys2bth_tx_active(HAL_SYS2BTH_ID_0, seq);
}


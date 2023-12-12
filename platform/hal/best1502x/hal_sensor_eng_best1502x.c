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
#include "plat_addr_map.h"

#if defined(SENS_SENSOR_ENG0_BASE) && defined(CHIP_SUBSYS_SENS)

#include CHIP_SPECIFIC_HDR(reg_sensor_eng)
#include "hal_sensor_eng_v2.h"
#include "cmsis_nvic.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "reg_timer.h"
#include "reg_i2cip.h"
#include "reg_spi.h"
#include "string.h"

#ifdef RTOS
    #include "cmsis_os.h"
#endif

#ifdef SENSOR_ENGINE_DEBUG
    #define HAL_SENSOR_ENGINE_TRACE(n, s, ...)  TRACE(n, s, ##__VA_ARGS__)
    #define HAL_SENSOR_ENGINE_DUMP8(n, s, ...)  DUMP8(n, s, ##__VA_ARGS__)
#else
    #define HAL_SENSOR_ENGINE_TRACE(n, s, ...)
    #define HAL_SENSOR_ENGINE_DUMP8(n, s, ...)
#endif

/*-----------------Macros that can be used during debugging, local use--------*/
//#define ENABLE_SPI_DATA_PROCESS   //[Original mode]This macro has nothing to do with [read/write mode]
#define ENABLE_DESCRIPTOR_MODE      //Open: Wall crack recommended, LLI mode; Shield: old mode, waste of memory,This macro has nothing to do with [read/write mode]

#if (defined(ENABLE_SPI_DATA_PROCESS) && defined(ENABLE_DESCRIPTOR_MODE))
    #error "These two macros cannot be defined at the same time ,2021-03-06"
#endif

#if (!defined(ENABLE_SPI_DATA_PROCESS) && !defined(ENABLE_DESCRIPTOR_MODE))
    #error "These two macros must define one ,2021-03-16"
#endif

#define SENSOR_ENGINE_MASTER_CNT    1
#define SENSOR_SLAVE_CNT            8

#if (SENSOR_ENGINE_MASTER_CNT == 0)
    #error SENSOR_ENGINE_MASTER_CNT min value is 1!
#endif
#if (SENSOR_SLAVE_CNT == 0)
    #error SENSOR_SLAVE_CNT min value is 1!
#endif

#define __HAL_ISPI                  0
#define __HAL_SPI_ID_0              1
#define __HAL_SPI_ID_SLCD           2

static struct SENSOR_ENG_T *const sensor_eng[] = {
    (struct SENSOR_ENG_T *)SENS_SENSOR_ENG0_BASE,
};

static HAL_SENSOR_ENG_HANDLER_T sensor_eng_handler[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static enum HAL_SENSOR_ENGINE_DEVICE_T sensor_engine_device[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static uint32_t read_index[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static enum HAL_SENSOR_ENGINE_READ_WRITE_T read_write_mode[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static uint8_t sensor_engine_opened_ch[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static uint8_t *read_buffer_base[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static uint32_t read_buffer_size[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
static uint8_t  codec_sel[SENSOR_ENGINE_MASTER_CNT] = {0};
static uint32_t spi_base[SENSOR_ENGINE_MASTER_CNT] = {0xCAFE2021};
static uint8_t irq_table[SENSOR_ENGINE_MASTER_CNT] = {SENSOR_ENG0_IRQn};
static uint8_t cmu_mod_table[SENSOR_ENGINE_MASTER_CNT] = {HAL_CMU_MOD_H_SENSOR_ENG0};

#ifndef ENABLE_DESCRIPTOR_MODE
    #define WRITE_BUFFER_SIZE_MAX      1024
#else
    #define WRITE_BUFFER_SIZE_MAX      16
#endif
static uint16_t sx_write_buffer[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT][WRITE_BUFFER_SIZE_MAX];

#ifdef ENABLE_SPI_DATA_PROCESS
    //spi device-specific,2021-02-02
    #define SPI_READ_SIZE_MAX       512
    static uint8_t  sx_spi_read_buffer[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT][SPI_READ_SIZE_MAX];
    static uint32_t spi_copy_index[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
    static uint32_t spi_tx_size[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
    static uint32_t spi_rx_size[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT];
#endif

#ifdef ENABLE_DESCRIPTOR_MODE
#define SPI_CMD_DATA_READ_MASK  (1<<8)
// Transfer Descriptor structure typedef
struct SX_CTRL_DESC_T {
    uint32_t desc_tx_src_addr;      /* Tx source address */
    uint32_t desc_tx_transfer_size; /* Tx transfer size */
    uint32_t desc_addr_inc;         /* Does descriptor addr increase,When it is set to 1, LLI address will automatically add 1 */
    uint32_t desc_ptr;              /* Pointer to next descriptor structure */
};

static struct SX_CTRL_DESC_T sx_write_desc[SENSOR_ENGINE_MASTER_CNT][SENSOR_SLAVE_CNT][WRITE_BUFFER_SIZE_MAX + 2]; //2=read+stop/readstop,2021-03-13

static void spi_rxcr_cmd_enable(enum HAL_SENSOR_ENGINE_MASTER_T master, bool cfg)
{
    uint32_t ssprxcr;
    struct SPI_T *spi = (struct SPI_T *)spi_base[master];

    ssprxcr = spi->SSPRXCR;
    if (cfg == true) {
        spi->SSPRXCR = ssprxcr | (1 << 7);
    } else {
        spi->SSPRXCR = ssprxcr & (~(1 << 7));
    }

    HAL_SENSOR_ENGINE_TRACE(4, "***spi_base[%d]=0x%X,ssprxcr=0x%X,spi->SSPRXCR=0x%X", (uint32_t)master, spi_base[master], ssprxcr, (uint32_t)spi->SSPRXCR);
}
#endif

void hal_sensor_eng_get_status(enum HAL_SENSOR_ENGINE_MASTER_T master, uint32_t *status)
{
    *status = sensor_eng[master]->STATUS;
}

void hal_sensor_eng_intr_cls(enum HAL_SENSOR_ENGINE_MASTER_T master, uint32_t value)
{
    sensor_eng[master]->INTR_CLR = value;
}

void hal_sensor_eng_get_timer_value(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *value)
{
    *value = sensor_eng[master]->SX_CONFIG[id].SX_TIMER_VALUE;
}

void hal_sensor_eng_get_cur_tx_read_addr(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *read_addr)
{
    *read_addr = sensor_eng[master]->SX_CUR_TX_RX_ADDR[id].TX_SRC_ADDR;
}

void hal_sensor_eng_get_cur_rx_write_addr(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *write_addr)
{
    *write_addr = sensor_eng[master]->SX_CUR_TX_RX_ADDR[id].RX_DES_ADDR;
}

void hal_sensor_eng_get_init_tx_read_addr(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *read_addr)
{
    *read_addr = sensor_eng[master]->SX_CONFIG[id].SX_TX_SRC_ADDR;
}

void hal_sensor_eng_get_init_rx_write_addr(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *write_addr)
{
    *write_addr = sensor_eng[master]->SX_CONFIG[id].SX_RX_DES_ADDR;
}

void hal_sensor_eng_get_tx_buffer_size(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *buffer_size)
{
    *buffer_size = sensor_eng[master]->SX_CONFIG[id].SX_TX_ADDR_LIMIT;
}

void hal_sensor_eng_get_rx_buffer_size(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, uint32_t *buffer_size)
{
    *buffer_size = sensor_eng[master]->SX_CONFIG[id].SX_RX_ADDR_LIMIT;
}

void hal_sensor_eng_get_device(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id, enum HAL_SENSOR_ENGINE_DEVICE_T *device)
{
    *device = (sensor_eng[master]->SX_CONFIG[id].SX_CTRL) >> SENSOR_ENGINE_SX_I2C_SPI_SEL_SHIFT;
}

static void hal_sensor_process_rx_buffer(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    uint8_t i;
    uint32_t status;
    uint8_t *write_addr;
    uint8_t *read_addr = NULL;
    uint32_t cur_write_addr;
    uint32_t len_behind = 0;
    uint32_t len_front = 0;
    uint32_t result_len = 0;
#ifdef ENABLE_SPI_DATA_PROCESS
    uint8_t *spi_read_buffer_base = NULL;
#endif

    hal_sensor_eng_get_status(master, &status);
    for (i = 0; i < SENSOR_SLAVE_CNT; i++) {
        if ((status & (1 << (i + 1))) && (sensor_engine_opened_ch[master][i])) {
            //clear interrupt flag
            hal_sensor_eng_intr_cls(master, 1 << (i + 0));

            if (read_write_mode[master][i] == HAL_SENSOR_ENGINE_WRITE) {
                sensor_eng_handler[master][i](master, i, sensor_engine_device[master][i], 0, 0, HAL_SENSOR_ENGINE_WRITE);
            } else {
                ///get current write addr
                hal_sensor_eng_get_cur_rx_write_addr(master, i, &cur_write_addr);
                write_addr = (uint8_t *)cur_write_addr;
#ifdef ENABLE_SPI_DATA_PROCESS
                if (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_SPI) {
                    spi_read_buffer_base = (uint8_t *)&sx_spi_read_buffer[master][i][0];
                }
                if ((sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
                    //calc read addr
                    read_addr = read_buffer_base[master][i] + read_index[master][i];
                    read_index[master][i] = write_addr - read_buffer_base[master][i];
                } else if (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_SPI) {
                    read_addr = spi_read_buffer_base + read_index[master][i];
                    read_index[master][i] = write_addr - spi_read_buffer_base;
                }
#else
                //calc read addr
                read_addr = read_buffer_base[master][i] + read_index[master][i];
                read_index[master][i] = write_addr - read_buffer_base[master][i];
#endif
                if (read_addr != write_addr) {
                    if (read_addr < write_addr) {
                        result_len = write_addr - read_addr;
                    } else { /* The address flipped */
#ifdef ENABLE_SPI_DATA_PROCESS
                        if ((sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
                            (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
                            (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
                            (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
                            (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
                            len_behind = read_buffer_base[master][i] + read_buffer_size[master][i] - read_addr;
                        } else if (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_SPI) {
                            len_behind = spi_read_buffer_base + SPI_READ_SIZE_MAX - read_addr;
                        }
                        len_front  = read_index[master][i];
                        result_len = len_behind + len_front;
#else
                        len_behind = read_buffer_base[master][i] + read_buffer_size[master][i] - read_addr;
                        len_front  = read_index[master][i];
                        result_len = len_behind + len_front;
#endif
                    }
                } else {
                    result_len = 1;
                }
                if (sensor_eng_handler[master][i]) {
#ifdef ENABLE_SPI_DATA_PROCESS
                    if ((sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
                        (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
                        (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
                        (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
                        (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
                        sensor_eng_handler[master][i](master, i, sensor_engine_device[master][i], (uint8_t *)(read_addr), result_len, HAL_SENSOR_ENGINE_READ);
                    } else if (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_SPI) {
                        uint8_t *index;
                        uint32_t spi_data_len = 0;
                        uint32_t remaining_len;
                        uint8_t *spi_copy_addr_des;
                        uint8_t *spi_copy_addr_src;
                        uint8_t *spi_copy_addr_read;

                        spi_copy_addr_des = read_buffer_base[master][i] + spi_copy_index[master][i];
                        spi_copy_addr_read = spi_copy_addr_des;

                        spi_data_len = 0;
                        for (index = read_addr; index < read_addr + result_len;) {
                            if ((uint32_t)index > (uint32_t)(spi_read_buffer_base + SPI_READ_SIZE_MAX)) { //Position flipped
                                spi_copy_addr_src = index - SPI_READ_SIZE_MAX;
                            } else {
                                spi_copy_addr_src = index;
                            }

                            if (spi_copy_addr_des + spi_rx_size[master][i] > read_buffer_base[master][i] + read_buffer_size[master][i]) { //des addr Position flipped
                                if (spi_copy_addr_src + spi_tx_size[master][i] + spi_rx_size[master][i] > spi_read_buffer_base + SPI_READ_SIZE_MAX) { //src addr Position flipped
                                    uint32_t remaining_len_src_true;
                                    uint32_t remaining_len_des_true;

                                    remaining_len = spi_read_buffer_base + SPI_READ_SIZE_MAX - spi_copy_addr_src;
                                    if (remaining_len > spi_tx_size[master][i]) { //The real data is at the end and the beginning
                                        remaining_len_src_true = remaining_len - spi_tx_size[master][i];
                                        remaining_len_des_true = read_buffer_base[master][i] + read_buffer_size[master][i] - spi_copy_addr_des;
                                        if (remaining_len_src_true > remaining_len_des_true) {
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i]), remaining_len_des_true);

                                            spi_copy_addr_des = read_buffer_base[master][i];//Overwrite the data at the beginning
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i] + remaining_len_des_true), remaining_len_src_true - remaining_len_des_true);
                                            spi_copy_addr_des += remaining_len_src_true - remaining_len_des_true;

                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base), spi_rx_size[master][i] - remaining_len_src_true);
                                            spi_copy_addr_des += spi_rx_size[master][i] - remaining_len_src_true;
                                        } else {
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i]), remaining_len_src_true);
                                            spi_copy_addr_des += remaining_len_src_true;

                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base), remaining_len_des_true - remaining_len_src_true);

                                            spi_copy_addr_des = read_buffer_base[master][i];//Overwrite the data at the beginning
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base + remaining_len_des_true - remaining_len_src_true), spi_rx_size[master][i] - remaining_len_des_true);
                                            spi_copy_addr_des += spi_rx_size[master][i] - remaining_len_des_true;
                                        }
                                    } else { //The real data is at the beginning
                                        remaining_len_src_true = spi_rx_size[master][i];
                                        remaining_len_des_true = read_buffer_base[master][i] + read_buffer_size[master][i] - spi_copy_addr_des;
                                        if (remaining_len_src_true > remaining_len_des_true) {
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base + spi_tx_size[master][i] - remaining_len), remaining_len_des_true);

                                            spi_copy_addr_des = read_buffer_base[master][i];//Overwrite the data at the beginning
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base + spi_tx_size[master][i] - remaining_len + remaining_len_des_true), remaining_len_src_true - remaining_len_des_true);
                                            spi_copy_addr_des += remaining_len_src_true - remaining_len_des_true;
                                        } else {
                                            //It should never come in here
                                            memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base + spi_tx_size[master][i] - remaining_len), remaining_len_src_true);
                                            spi_copy_addr_des += remaining_len_src_true;
                                        }
                                    }
                                } else {
                                    remaining_len = read_buffer_base[master][i] + read_buffer_size[master][i] - spi_copy_addr_des;
                                    memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i]), remaining_len);

                                    spi_copy_addr_des = read_buffer_base[master][i];//Overwrite the data at the beginning
                                    memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i] + remaining_len), spi_rx_size[master][i] - remaining_len);
                                    spi_copy_addr_des += spi_rx_size[master][i] - remaining_len;
                                }
                            } else {
                                if (spi_copy_addr_src + spi_tx_size[master][i] + spi_rx_size[master][i] > spi_read_buffer_base + SPI_READ_SIZE_MAX) {
                                    remaining_len = spi_read_buffer_base + SPI_READ_SIZE_MAX - spi_copy_addr_src;
                                    if (remaining_len > spi_tx_size[master][i]) { //The real data is at the end and the beginning
                                        //Copy the second half
                                        memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i]), remaining_len - spi_tx_size[master][i]);
                                        spi_copy_addr_des += remaining_len - spi_tx_size[master][i];

                                        //Copy the first half
                                        memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base), spi_rx_size[master][i] + spi_tx_size[master][i] - remaining_len);
                                        spi_copy_addr_des += spi_rx_size[master][i] + spi_tx_size[master][i] - remaining_len;
                                    } else { //The real data is at the beginning
                                        memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_read_buffer_base + spi_tx_size[master][i] - remaining_len), spi_rx_size[master][i]);
                                        spi_copy_addr_des += spi_rx_size[master][i];
                                    }
                                } else {
                                    memcpy((uint8_t *)spi_copy_addr_des, (uint8_t *)(spi_copy_addr_src + spi_tx_size[master][i]), spi_rx_size[master][i]);
                                    spi_copy_addr_des += spi_rx_size[master][i];
                                }
                            }
                            index += spi_tx_size[master][i] + spi_rx_size[master][i];
                            spi_data_len += spi_rx_size[master][i];
                        }
                        if (spi_data_len > read_buffer_size[master][i]) {
                            //Prevent overflow, but you will never get in here,2021-02-05
                            spi_data_len = read_buffer_size[master][i];
                        }
                        if (spi_copy_index[master][i] < read_buffer_size[master][i]) {
                            spi_copy_index[master][i] += spi_data_len;
                            if (spi_copy_index[master][i] >= read_buffer_size[master][i]) {
                                spi_copy_index[master][i] -= read_buffer_size[master][i];
                            }
                        } else {
                            spi_copy_index[master][i] = 0;
                        }

                        sensor_eng_handler[master][i](master, i, sensor_engine_device[master][i], (uint8_t *)(spi_copy_addr_read), spi_data_len, HAL_SENSOR_ENGINE_READ);
                    }
#else
                    sensor_eng_handler[master][i](master, i, sensor_engine_device[master][i], (uint8_t *)(read_addr), result_len, HAL_SENSOR_ENGINE_READ);
#endif
                }

#ifdef ENABLE_FULL_DATA_LOG
#ifdef ENABLE_SPI_DATA_PROCESS
                HAL_SENSOR_ENGINE_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "\n"); /* output a enter */
                HAL_SENSOR_ENGINE_DUMP8("%02x ", read_buffer_base[master][i], read_buffer_size[master][i]);
                HAL_SENSOR_ENGINE_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "\n"); /* output a enter */
                if ((sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
                    (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
                    memset(write_addr, 0, read_buffer_base[master][i] + read_buffer_size[master][i] - write_addr);
                } else if (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_SPI) {
                    memset(read_buffer_base[master][i] + spi_copy_index[master][i], 0, read_buffer_size[master][i] - spi_copy_index[master][i]);
                }
#else
                memset(write_addr, 0, read_buffer_base[master][i] + read_buffer_size[master][i] - write_addr);
#endif
#endif
            }
        }
    }
}

static void hal_sensor_eng_irq_handler(void)
{
    uint8_t i;
    uint32_t status;

    for (i = 0; i < SENSOR_ENGINE_MASTER_CNT; i++) {
        status = sensor_eng[i]->STATUS;

        if (status) {
            hal_sensor_process_rx_buffer(i);
#if 0
            for (uint8_t j = 0; j < SENSOR_SLAVE_CNT; j++) {
                HAL_SENSOR_ENGINE_TRACE(2, "S[%d]_TIMER_VALUE=0x%X", j, sensor_eng[i]->SX_CONFIG[j].SX_TIMER_VALUE);
            }
#endif
        }
    }
}

static void hal_sensor_eng_irq_enable(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    NVIC_SetVector(irq_table[master], (uint32_t)hal_sensor_eng_irq_handler);
    NVIC_SetPriority(irq_table[master], IRQ_PRIORITY_HIGH);
    NVIC_ClearPendingIRQ(irq_table[master]);
    NVIC_EnableIRQ(irq_table[master]);
}

static void hal_sensor_eng_irq_disable(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    NVIC_DisableIRQ(irq_table[master]);
}

int hal_sensor_engine_cfg(enum HAL_SENSOR_ENGINE_MASTER_T master, const struct HAL_SENSOR_ENGINE_CFG_T *cfg)
{
    uint32_t i;
    uint32_t control = 0;
    uint8_t spi_id;
    uint8_t rx_width;
    uint8_t tx_width;

    ASSERT(cfg->id < HAL_SENSOR_ENGINE_ID_QTY, "The wrong sensor engine id: %d", cfg->id);
    ASSERT(cfg->device < HAL_SENSOR_ENGINE_DEVICE_QTY, "The wrong sensor engine device: %d", cfg->device);

    if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_TIMER) {
        ASSERT(cfg->timer_period_ms, "The wrong timer_period_ms:%d", cfg->timer_period_ms);
    }
    ASSERT(cfg->interrupt_count < 16, "The wrong interrupt_count:%d", cfg->interrupt_count);
    ASSERT(cfg->data_to_vad < HAL_SENSOR_ENGINE_CODEC_SEL_QTY, "The wrong data_to_vad:%d", cfg->data_to_vad);
    codec_sel[master] = cfg->data_to_vad;
    if (cfg->device == HAL_SENSOR_ENGINE_DEVICE_SPI) {
        ASSERT(cfg->spi_device_cs < HAL_SENSOR_ENGINE_SPI_CS_QTY, "The wrong spi_device_cs:%d", cfg->spi_device_cs);
        ASSERT(cfg->spi_device_rxds < HAL_SENSOR_ENGINE_SPI_RXDS_QTY, "The wrong spi_device_rxds:%d", cfg->spi_device_rxds);
        spi_id = cfg->spi_id;
        switch (spi_id) {
            case __HAL_ISPI: {
                spi_base[master] = ISPI_BASE;
                break;
            }
#ifdef CHIP_HAS_SPI
            case __HAL_SPI_ID_0: {
                spi_base[master] = SPI_BASE;
                break;
            }
#endif
#ifdef CHIP_HAS_SPILCD
            case __HAL_SPI_ID_SLCD: {
                spi_base[master] = SPILCD_BASE;
                break;
            }
#endif
            default: {
                ASSERT(false, "spi id error,line=%d,%s", __LINE__, __FUNCTION__);
                break;
            }
        }
    } else if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
               (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
               (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
               (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
               (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
        ASSERT(cfg->i2c_device_address, "The wrong i2c_device_address:0x%X", cfg->i2c_device_address);
    } else {
        ASSERT(false, "The wrong cfg->device=%d,file:%s,line=%d", cfg->device, __FILE__, __LINE__);
    }
#ifndef ENABLE_DESCRIPTOR_MODE
    if ((cfg->tx_transfer_size + cfg->rx_transfer_size) > WRITE_BUFFER_SIZE_MAX) {
        ASSERT(false, "The sum of send and receive sizes exceeds the maximum value defined by the driver layer,MAX=%d", WRITE_BUFFER_SIZE_MAX);
    }
#else
    if (cfg->tx_transfer_size > WRITE_BUFFER_SIZE_MAX) {
        ASSERT(false, "The length of the data sent exceeds the maximum,MAX=%d", WRITE_BUFFER_SIZE_MAX);
    }
#endif
    ASSERT(cfg->read_write < HAL_SENSOR_ENGINE_READ_WRITE_QTY, "The wrong read_write:%d", cfg->read_write);

    if (cfg->read_write == HAL_SENSOR_ENGINE_WRITE) {
        ASSERT(cfg->tx_data_width < HAL_SENSOR_ENGINE_DATA_WIDTH_QTY, "The wrong tx_data_width:%d", cfg->tx_data_width);
        ASSERT(cfg->tx_buffer_addr, "The wrong tx_buffer_addr:%p", cfg->tx_buffer_addr);
        ASSERT(cfg->tx_buffer_size, "The wrong tx_buffer_size:%d", cfg->tx_buffer_size);
        ASSERT(cfg->tx_transfer_size, "The wrong tx_transfer_size:%d", cfg->tx_transfer_size);
    } else {
        ASSERT(cfg->rx_data_width < HAL_SENSOR_ENGINE_DATA_WIDTH_QTY, "The wrong rx_data_width:%d", cfg->rx_data_width);
        ASSERT(cfg->rx_buffer_addr, "The wrong rx_buffer_addr:%p", cfg->rx_buffer_addr);
        ASSERT(cfg->rx_buffer_size, "The wrong rx_buffer_size:%d", cfg->rx_buffer_size);
        ASSERT(cfg->rx_transfer_size, "The wrong rx_transfer_size:%d", cfg->rx_transfer_size);
    }
    ASSERT(cfg->handler, "The wrong handler:%p", cfg->handler);

    hal_cmu_clock_enable(cmu_mod_table[master]);
    hal_cmu_reset_clear(cmu_mod_table[master]);

    control = SENSOR_ENGINE_SX_INTR_EN |
              SENSOR_ENGINE_SX_INTR_NUM(cfg->interrupt_count) |
              (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_TIMER ? SENSOR_ENGINE_SX_TRIGGER_SEL : 0) |
              SENSOR_ENGINE_SX_RX_HWIDTH(cfg->rx_data_width) |
              SENSOR_ENGINE_SX_TX_HWIDTH(cfg->tx_data_width) |
              SENSOR_ENGINE_SX_GPIO_SEL(cfg->trigger_gpio / 32) |
              SENSOR_ENGINE_SX_SPI_CS(cfg->spi_device_cs) |
              (cfg->spi_device_rxds == HAL_SENSOR_ENGINE_SPI_RXDS_1 ? SENSOR_ENGINE_SX_SPI_RXDS : 0) |
              (cfg->read_write == HAL_SENSOR_ENGINE_WRITE ? SENSOR_ENGINE_SX_RDN_WR : 0) |
              SENSOR_ENGINE_SX_I2C_SPI_SEL(cfg->device) |
              SENSOR_ENGINE_SX_TAR_DEV_ID(cfg->i2c_device_address) |
              SENSOR_ENGINE_SX_ENABLE;

    HAL_SENSOR_ENGINE_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "  ");
    HAL_SENSOR_ENGINE_TRACE(2, "sensor engine master=%d,id=%d", master, cfg->id);

    uint8_t *pbuf;

    pbuf = (uint8_t *)(cfg->tx_buffer_addr);
    for (i = 0; i < cfg->tx_transfer_size; i++) {
        // lo byte of short : for data
        // hi byte of short : for cmd/stop/restart
        sx_write_buffer[master][cfg->id][i] = *(pbuf++);

        //i2c/spi write mode process,2021-03-15
        if (cfg->read_write == HAL_SENSOR_ENGINE_WRITE) {
            if (i == (cfg->tx_transfer_size - 1)) {
                if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
                    (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
                    (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
                    (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
                    (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
                    sx_write_buffer[master][cfg->id][i] |= I2CIP_CMD_DATA_CMD_WRITE_MASK | I2CIP_CMD_DATA_STOP_MASK;
                }
            }
        }
        HAL_SENSOR_ENGINE_TRACE(4, "sx_write_buffer[%d][%d][%d]=0x%X", master, cfg->id, i, sx_write_buffer[master][cfg->id][i]);
    }

    if (cfg->read_write == HAL_SENSOR_ENGINE_WRITE) {
        //When i2c/spi is used, there is no need to do anything here, other agreements will depend on the situation.tanwenchen,2021-03-15
    } else {
#ifndef ENABLE_DESCRIPTOR_MODE
        /* i2c data process */
        if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
            for (i = 0; i < cfg->rx_transfer_size; i++) {
                // lo byte of short : for data
                // hi byte of short : for cmd/stop/restart
                if (i == cfg->rx_transfer_size - 1) {
                    sx_write_buffer[master][cfg->id][i + cfg->tx_transfer_size] = I2CIP_CMD_DATA_CMD_READ_MASK | I2CIP_CMD_DATA_STOP_MASK;
                } else {
                    sx_write_buffer[master][cfg->id][i + cfg->tx_transfer_size] = I2CIP_CMD_DATA_CMD_READ_MASK;
                }

                uint16_t temp;
                temp = sx_write_buffer[master][cfg->id][i + cfg->tx_transfer_size];
                HAL_SENSOR_ENGINE_TRACE(7, "sx_write_buffer[%d][%d][%d]=0x%X,restart=%d,stop=%d,cmd=%d", master, cfg->id,
                                        i + cfg->tx_transfer_size, temp, (temp >> 10) & 0x01, (temp >> 9) & 0x01, (temp >> 8) & 0x01);
            }
        }
#else
        //Ready to write buffer
        if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
            if (cfg->rx_transfer_size == 1) {
                sx_write_buffer[master][cfg->id][0 + cfg->tx_transfer_size] = I2CIP_CMD_DATA_CMD_READ_MASK | I2CIP_CMD_DATA_STOP_MASK;
            } else {
                sx_write_buffer[master][cfg->id][0 + cfg->tx_transfer_size] = I2CIP_CMD_DATA_CMD_READ_MASK;
                sx_write_buffer[master][cfg->id][1 + cfg->tx_transfer_size] = I2CIP_CMD_DATA_CMD_READ_MASK | I2CIP_CMD_DATA_STOP_MASK;
            }
        } else if (cfg->device == HAL_SENSOR_ENGINE_DEVICE_SPI) {
            sx_write_buffer[master][cfg->id][0 + cfg->tx_transfer_size] = SPI_CMD_DATA_READ_MASK;
        }

        //tx lli:write
        for (i = 0; i < cfg->tx_transfer_size; i++) { //This writing method is good: it can support multi byte send command,2021-02-07
            sx_write_desc[master][cfg->id][i].desc_tx_src_addr = (uint32_t)&sx_write_buffer[master][cfg->id][i];
            sx_write_desc[master][cfg->id][i].desc_tx_transfer_size = 1;
            sx_write_desc[master][cfg->id][i].desc_addr_inc = 0;
            sx_write_desc[master][cfg->id][i].desc_ptr = (uint32_t)&sx_write_desc[master][cfg->id][i + 1];

            //print write:sx_write_desc
            POSSIBLY_UNUSED struct SX_CTRL_DESC_T *p;
            p = &sx_write_desc[master][cfg->id][i];

            HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_tx_src_addr=0x%X,value=0x%X,addr=0x%X", master, cfg->id, i,
                                    (uint32_t)p->desc_tx_src_addr, (*((uint32_t *)p->desc_tx_src_addr)) & 0xFFFF, (uint32_t)p);
            HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_tx_transfer_size=%d", master, cfg->id, i, p->desc_tx_transfer_size);
            HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_addr_inc=%d", master, cfg->id, i, p->desc_addr_inc);
            HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_ptr=0x%X", master, cfg->id, i, (uint32_t)p->desc_ptr);
        }

        //rx lli:read
        if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
            if (cfg->rx_transfer_size == 1) {
                //rx+stop lli:read and stop
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_tx_src_addr = (uint32_t)&sx_write_buffer[master][cfg->id][0 + cfg->tx_transfer_size];
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_tx_transfer_size = cfg->rx_transfer_size;
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_addr_inc = 0;
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_ptr = 0; //lli stop flag,2021-02-08
            } else {
                //rx lli:read
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_tx_src_addr = (uint32_t)&sx_write_buffer[master][cfg->id][0 + cfg->tx_transfer_size];
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_tx_transfer_size = cfg->rx_transfer_size - 1; //The stop bit is subtracted
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_addr_inc = 0;
                sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_ptr = (uint32_t)&sx_write_desc[master][cfg->id][1 + cfg->tx_transfer_size];

                //stop lli:read and stop
                sx_write_desc[master][cfg->id][1 + cfg->tx_transfer_size].desc_tx_src_addr = (uint32_t)&sx_write_buffer[master][cfg->id][1 + cfg->tx_transfer_size];
                sx_write_desc[master][cfg->id][1 + cfg->tx_transfer_size].desc_tx_transfer_size = 1;
                sx_write_desc[master][cfg->id][1 + cfg->tx_transfer_size].desc_addr_inc = 0;
                sx_write_desc[master][cfg->id][1 + cfg->tx_transfer_size].desc_ptr = 0; //lli stop flag,2021-02-08
            }
        } else if (cfg->device == HAL_SENSOR_ENGINE_DEVICE_SPI) {
            //rx+stop lli:read and stop
            sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_tx_src_addr = (uint32_t)&sx_write_buffer[master][cfg->id][0 + cfg->tx_transfer_size];
            sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_tx_transfer_size = cfg->rx_transfer_size;
            sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_addr_inc = 0;
            sx_write_desc[master][cfg->id][0 + cfg->tx_transfer_size].desc_ptr = 0; //lli stop flag,2021-02-08
        }

        //print read:sx_write_desc
        POSSIBLY_UNUSED uint8_t j;
        POSSIBLY_UNUSED struct SX_CTRL_DESC_T *p;
        j = 0 + cfg->tx_transfer_size;
        p = &sx_write_desc[master][cfg->id][j];

        HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_tx_src_addr=0x%X,value=0x%X,addr=0x%X", master, cfg->id, j,
                                (uint32_t)p->desc_tx_src_addr, (*((uint32_t *)p->desc_tx_src_addr)) & 0xFFFF, (uint32_t)p);
        HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_tx_transfer_size=%d", master, cfg->id, j, p->desc_tx_transfer_size);
        HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_addr_inc=%d", master, cfg->id, j, p->desc_addr_inc);
        HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_ptr=0x%X", master, cfg->id, j, (uint32_t)p->desc_ptr);
        if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
            if (cfg->rx_transfer_size > 1) {
                j = 1 + cfg->tx_transfer_size;
                p = &sx_write_desc[master][cfg->id][j];
                HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_tx_src_addr=0x%X,value=0x%X,addr=0x%X", master, cfg->id, j,
                                        (uint32_t)p->desc_tx_src_addr, (*((uint32_t *)p->desc_tx_src_addr)) & 0xFFFF, (uint32_t)p);
                HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_tx_transfer_size=%d", master, cfg->id, j, p->desc_tx_transfer_size);
                HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_addr_inc=%d", master, cfg->id, j, p->desc_addr_inc);
                HAL_SENSOR_ENGINE_TRACE(1, "sx_write_desc[%d][%d][%d].desc_ptr=0x%X", master, cfg->id, j, (uint32_t)p->desc_ptr);
            }
        }
#endif  //ENABLE_DESCRIPTOR_MODE
    }


    switch (cfg->rx_data_width) {
        case HAL_SENSOR_ENGINE_DATA_BYTE: {
            rx_width = 1;
            break;
        }
        case HAL_SENSOR_ENGINE_DATA_HALF_WORD: {
            rx_width = 2;
            break;
        }
        case HAL_SENSOR_ENGINE_DATA_WORD: {
            rx_width = 4;
            break;
        }
        default: {
            ASSERT(0, "rx_data_width configuration error:%d", cfg->rx_data_width);
            break;
        }
    }
    switch (cfg->tx_data_width) {
        case HAL_SENSOR_ENGINE_DATA_BYTE: {
            tx_width = 1;
            break;
        }
        case HAL_SENSOR_ENGINE_DATA_HALF_WORD: {
            tx_width = 2;
            break;
        }
        case HAL_SENSOR_ENGINE_DATA_WORD: {
            tx_width = 4;
            break;
        }
        default: {
            ASSERT(0, "tx_data_width configuration error:%d", cfg->tx_data_width);
            break;
        }
    }

    sensor_eng[master]->SX_CONFIG[cfg->id].SX_CTRL = control;
    if (cfg->read_write == HAL_SENSOR_ENGINE_WRITE) {
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_SRC_ADDR = (uint32_t)&sx_write_buffer[master][cfg->id][0];//default 0
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_DES_ADDR = 0;
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_TRANSFER_SIZE = cfg->tx_transfer_size;
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_TRANSFER_SIZE = 0;
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT = (uint32_t)(&sx_write_buffer[master][cfg->id][0]) + (cfg->tx_transfer_size) * tx_width;
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_ADDR_LIMIT = 0;
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_CTRL = 1;
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_PTR = 0;
    } else {
        if ((cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C1) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C2) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C3) ||
            (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C4)) {
#ifndef ENABLE_DESCRIPTOR_MODE
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_SRC_ADDR = (uint32_t)&sx_write_buffer[master][cfg->id][0];//default 0
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_DES_ADDR = (uint32_t)cfg->rx_buffer_addr;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_TRANSFER_SIZE = cfg->tx_transfer_size + cfg->rx_transfer_size;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_TRANSFER_SIZE = cfg->rx_transfer_size;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT = (uint32_t)(&sx_write_buffer[master][cfg->id][0]) + (cfg->tx_transfer_size + cfg->rx_transfer_size) * tx_width;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_ADDR_LIMIT = (uint32_t)(cfg->rx_buffer_addr) + cfg->rx_buffer_size * rx_width;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_CTRL = 1;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_PTR = 0;
#else
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_SRC_ADDR = (uint32_t)&sx_write_buffer[master][cfg->id][0];//default 0
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_DES_ADDR = (uint32_t)cfg->rx_buffer_addr;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_TRANSFER_SIZE = 1;//1 is better than cfg->tx_transfer_size,2021-02-07
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_TRANSFER_SIZE = cfg->rx_transfer_size;
            if (cfg->rx_transfer_size == 1) {
                sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT = (uint32_t)(&sx_write_buffer[master][cfg->id][0]) + (cfg->tx_transfer_size + 1) * tx_width;//1=1(read+stop)
            } else {
                sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT = (uint32_t)(&sx_write_buffer[master][cfg->id][0]) + (cfg->tx_transfer_size + 2) * tx_width;//2=1(read)+1(stop)
            }
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_ADDR_LIMIT = (uint32_t)(cfg->rx_buffer_addr) + cfg->rx_buffer_size * rx_width;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_CTRL = 0;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_PTR = (uint32_t)&sx_write_desc[master][cfg->id][1];//Point to the next LLI
#endif
        } else if (cfg->device == HAL_SENSOR_ENGINE_DEVICE_SPI) {
#ifndef ENABLE_DESCRIPTOR_MODE
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_SRC_ADDR = (uint32_t)&sx_write_buffer[master][cfg->id][0];
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_DES_ADDR = (uint32_t)&sx_spi_read_buffer[master][cfg->id][0];
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_TRANSFER_SIZE = cfg->tx_transfer_size + cfg->rx_transfer_size;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_TRANSFER_SIZE = cfg->tx_transfer_size + cfg->rx_transfer_size;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT = (uint32_t)(&sx_write_buffer[master][cfg->id][0]) + (cfg->tx_transfer_size + cfg->rx_transfer_size) * tx_width;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_ADDR_LIMIT = (uint32_t)(&sx_spi_read_buffer[master][cfg->id][0]) + SPI_READ_SIZE_MAX;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_CTRL = 1;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_PTR = 0;
#else
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_SRC_ADDR = (uint32_t)&sx_write_buffer[master][cfg->id][0];
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_DES_ADDR = (uint32_t)cfg->rx_buffer_addr;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_TRANSFER_SIZE = 1;//1 is better than cfg->tx_transfer_size,2021-02-07
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_TRANSFER_SIZE = cfg->rx_transfer_size;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT = (uint32_t)(&sx_write_buffer[master][cfg->id][0]) + (cfg->tx_transfer_size + 1) * tx_width;//1=1(read+stop)
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_ADDR_LIMIT = (uint32_t)(cfg->rx_buffer_addr) + cfg->rx_buffer_size * rx_width;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_CTRL = 0;
            sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_PTR = (uint32_t)&sx_write_desc[master][cfg->id][1];//Point to the next LLI
#endif
        }
    }

    if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_GPIO) {
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_GPIO_INTR_MASK = ~(0x1 << cfg->trigger_gpio);
    } else {
        sensor_eng[master]->SX_CONFIG[cfg->id].SX_TIMER_LOAD = cfg->timer_period_ms * 33;//32.768KHz
    }

    sensor_eng_handler[master][cfg->id] = cfg->handler;
    sensor_engine_device[master][cfg->id] = cfg->device;
    read_buffer_base[master][cfg->id] = (uint8_t *)cfg->rx_buffer_addr;
    read_buffer_size[master][cfg->id] = cfg->rx_buffer_size;
    read_index[master][cfg->id] = 0;
    read_write_mode[master][cfg->id] = cfg->read_write;
    sensor_engine_opened_ch[master][cfg->id] = true;

#ifdef ENABLE_SPI_DATA_PROCESS
    //spi device-specific,2021-02-02
    spi_copy_index[master][cfg->id] = 0;
    spi_tx_size[master][cfg->id] = cfg->tx_transfer_size;
    spi_rx_size[master][cfg->id] = cfg->rx_transfer_size;
#endif

    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->interrupt_count=%d", master, cfg->id, cfg->interrupt_count);
    if (cfg->read_write == HAL_SENSOR_ENGINE_WRITE) {
        HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->read_write=%d:write", master, cfg->id, cfg->read_write);
    } else {
        HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->read_write=%d:read", master, cfg->id, cfg->read_write);
    }
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->rx_data_width=%d", master, cfg->id, cfg->rx_data_width);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->tx_data_width=%d", master, cfg->id, cfg->tx_data_width);
    HAL_SENSOR_ENGINE_TRACE(1, "rx_width=%d", rx_width);
    HAL_SENSOR_ENGINE_TRACE(1, "tx_width=%d", tx_width);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->rx_buffer_size=%d", master, cfg->id, cfg->rx_buffer_size);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->tx_buffer_size=%d", master, cfg->id, cfg->tx_buffer_size);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->rx_transfer_size=%d", master, cfg->id, cfg->rx_transfer_size);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->tx_transfer_size=%d", master, cfg->id, cfg->tx_transfer_size);
    if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_GPIO) {
        HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->trigger_type=%d:GPIO", master, cfg->id, cfg->trigger_type);
    } else {
        HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->trigger_type=%d:TIMER", master, cfg->id, cfg->trigger_type);
    }
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->trigger_gpio=%d", master, cfg->id, cfg->trigger_gpio);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->device=%d", master, cfg->id, cfg->device);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->i2c_device_address=0x%X", master, cfg->id, cfg->i2c_device_address);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->spi_device_cs=%d", master, cfg->id, cfg->spi_device_cs);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->spi_device_rxds=%d", master, cfg->id, cfg->spi_device_rxds);
    HAL_SENSOR_ENGINE_TRACE(1, "cfg[%d][%d]->spi_id=%d", master, cfg->id, cfg->spi_id);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_CTRL=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_CTRL);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_TX_SRC_ADDR=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_SRC_ADDR);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_RX_DES_ADDR=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_DES_ADDR);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_TX_TRANSFER_SIZE=%d", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_TRANSFER_SIZE);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_RX_TRANSFER_SIZE=%d", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_TRANSFER_SIZE);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_TX_ADDR_LIMIT=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_TX_ADDR_LIMIT);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_RX_ADDR_LIMIT=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_RX_ADDR_LIMIT);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_GPIO_INTR_MASK=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_GPIO_INTR_MASK);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_TIMER_LOAD=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_TIMER_LOAD);
#ifdef ENABLE_DESCRIPTOR_MODE
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_DESC_CTRL=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_CTRL);
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng[%d]->SX_CONFIG[%d].SX_DESC_PTR=0x%08X", master, cfg->id, (uint32_t)sensor_eng[master]->SX_CONFIG[cfg->id].SX_DESC_PTR);
#endif
    HAL_SENSOR_ENGINE_TRACE(1, "sensor_eng_handler[%d][%d]=0x%08X", master, cfg->id, (uint32_t)sensor_eng_handler[master][cfg->id]);

    return 0;
}

int hal_sensor_engine_open(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    //i2c base addr init
    sensor_eng[master]->I2C0_MBASE_ADDR = SENSOR_ENGINE_I2C0_MBASE_ADDR(I2C0_BASE);
#ifdef I2C1_BASE
    sensor_eng[master]->I2C1_MBASE_ADDR = SENSOR_ENGINE_I2C1_MBASE_ADDR(I2C1_BASE);
#endif
#ifdef I2C2_BASE
    sensor_eng[master]->I2C2_MBASE_ADDR = SENSOR_ENGINE_I2C2_MBASE_ADDR(I2C2_BASE);
#endif
#ifdef I2C3_BASE
    sensor_eng[master]->I2C3_MBASE_ADDR = SENSOR_ENGINE_I2C3_MBASE_ADDR(I2C3_BASE);
#endif
#ifdef I2C4_BASE
    sensor_eng[master]->I2C4_MBASE_ADDR = SENSOR_ENGINE_I2C4_MBASE_ADDR(I2C4_BASE);
#endif

    //spi base addr init
    sensor_eng[master]->SPI_MBASE_ADDR = SENSOR_ENGINE_SPI_MBASE_ADDR(spi_base[master]);

    //gpio base addr init
    sensor_eng[master]->GPIO0_BASE_ADDR = SENSOR_ENGINE_GPIO0_BASE_ADDR(GPIO_BASE);
#ifdef GPIO1_BASE
    sensor_eng[master]->GPIO1_BASE_ADDR = SENSOR_ENGINE_GPIO1_BASE_ADDR(GPIO1_BASE);
#endif
#ifdef GPIO2_BASE
    sensor_eng[master]->GPIO2_BASE_ADDR = SENSOR_ENGINE_GPIO2_BASE_ADDR(GPIO2_BASE);
#endif
#ifdef GPIO3_BASE
    sensor_eng[master]->GPIO3_BASE_ADDR = SENSOR_ENGINE_GPIO3_BASE_ADDR(GPIO3_BASE);
#endif

#ifdef ENABLE_DESCRIPTOR_MODE
    for (uint8_t i = 0; i < SENSOR_SLAVE_CNT; i++) {
        if (sensor_engine_device[master][i] == HAL_SENSOR_ENGINE_DEVICE_SPI) {
            spi_rxcr_cmd_enable(master, true);
            break;
        }
    }
#endif

    //clear interrupt flag
    sensor_eng[master]->INTR_CLR = SENSOR_ENGINE_S0_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S1_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S2_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S3_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S4_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S5_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S6_WAKEUP_INTR_CLR |
                                   SENSOR_ENGINE_S7_WAKEUP_INTR_CLR;
    //sensor engine enable
    sensor_eng[master]->CTRL_EN = SENSOR_ENGINE_CODEC_SX_SEL(codec_sel[master]) | SENSOR_ENGINE_SENSORENG_EN;

    //open interrupt
    hal_sensor_eng_irq_enable(master);

    return 0;
}

int hal_sensor_engine_close(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    uint8_t cnt = 0;

    hal_sensor_eng_irq_disable(master);
    sensor_eng[master]->CTRL_EN &= ~SENSOR_ENGINE_SENSORENG_EN;

    // Wait until sensor engine becomes idle(last data transfer completed)
    while ((sensor_eng[master]->STATUS & SENSOR_ENGINE_BUSY) && cnt < 10) {
        osDelay(1);
        cnt++;
    }
    if (sensor_eng[master]->STATUS & SENSOR_ENGINE_BUSY) {
        ASSERT(false, "%s: Sensor engine cannot become idle: 0x%08X", __FUNCTION__, sensor_eng[master]->STATUS);
    }
    for (cnt = 0; cnt < SENSOR_SLAVE_CNT; cnt++) {
        sensor_eng[master]->SX_CONFIG[cnt].SX_GPIO_INTR_MASK = ~0UL;//release gpio
        sensor_eng[master]->SX_CONFIG[cnt].SX_CTRL = 0x88008000;    //default value
    }

    hal_cmu_reset_set(cmu_mod_table[master]);
    hal_cmu_clock_disable(cmu_mod_table[master]);

    //clear buffer
    for (cnt = 0; cnt < SENSOR_ENGINE_MASTER_CNT; cnt++) {
        for (uint8_t i = 0; i < SENSOR_SLAVE_CNT; i++) {
            sensor_eng_handler[cnt][i] = 0;
            sensor_engine_device[cnt][i] = HAL_SENSOR_ENGINE_DEVICE_QTY;
            read_index[cnt][i] = 0;
            read_write_mode[cnt][i] = HAL_SENSOR_ENGINE_READ;
            sensor_engine_opened_ch[cnt][i] = false;
            read_buffer_size[cnt][i] = 0;
#ifdef ENABLE_SPI_DATA_PROCESS
            sx_spi_read_buffer[cnt][i][0] = 0;;
            spi_copy_index[cnt][i] = 0;
            spi_tx_size[cnt][i] = 0;
            spi_rx_size[cnt][i] = 0;
#endif
        }
    }

#ifdef ENABLE_DESCRIPTOR_MODE
    memset(sx_write_desc, 0, sizeof(sx_write_desc));
#endif
    return 0;
}

int hal_sensor_engine_open_sensor(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id)
{
    uint32_t ctrl;

    ctrl = sensor_eng[master]->SX_CONFIG[id].SX_CTRL;
    sensor_eng[master]->SX_CONFIG[id].SX_CTRL = SENSOR_ENGINE_SX_ENABLE | ctrl;

    return 0;
}

int hal_sensor_engine_close_sensor(enum HAL_SENSOR_ENGINE_MASTER_T master, enum HAL_SENSOR_ENGINE_ID_T id)
{
    uint32_t ctrl;

    ctrl = sensor_eng[master]->SX_CONFIG[id].SX_CTRL;
    sensor_eng[master]->SX_CONFIG[id].SX_CTRL = (~SENSOR_ENGINE_SX_ENABLE) & ctrl;

    return 0;
}

int hal_sensor_engine_enable(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    uint32_t ctrl;

    ctrl = sensor_eng[master]->CTRL_EN;
    sensor_eng[master]->CTRL_EN = SENSOR_ENGINE_SENSORENG_EN | ctrl;

    return 0;
}

int hal_sensor_engine_disable(enum HAL_SENSOR_ENGINE_MASTER_T master)
{
    uint32_t ctrl;

    ctrl = sensor_eng[master]->CTRL_EN;
    sensor_eng[master]->CTRL_EN = (~SENSOR_ENGINE_SENSORENG_EN) & ctrl;

    return 0;
}

#endif

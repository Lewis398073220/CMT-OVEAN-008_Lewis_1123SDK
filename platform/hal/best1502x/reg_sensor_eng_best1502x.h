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
#ifndef __REG_SENSOR_ENG_BEST1502X_H__
#define __REG_SENSOR_ENG_BEST1502X_H__

#include "plat_types.h"

#define SENSOR_SLAVE_CNT_MAX    8

struct SENSOR_ENG_T {
    __IO uint32_t CTRL_EN;                      //0x00
    __IO uint32_t I2C0_MBASE_ADDR;              //0x04
    __IO uint32_t I2C1_MBASE_ADDR;              //0x08
    __IO uint32_t I2C2_MBASE_ADDR;              //0x0C
    __IO uint32_t I2C3_MBASE_ADDR;              //0x10
    __IO uint32_t I2C4_MBASE_ADDR;              //0x14
    __IO uint32_t SPI_MBASE_ADDR;               //0x18
    __IO uint32_t GPIO0_BASE_ADDR;              //0x1C
    __IO uint32_t GPIO1_BASE_ADDR;              //0x20
    __IO uint32_t GPIO2_BASE_ADDR;              //0x24
    __IO uint32_t GPIO3_BASE_ADDR;              //0x28
    __IO uint32_t RESERVED0;                    //0x2C
    __IO uint32_t INTR_CLR;                     //0x30
    __I  uint32_t STATUS;                       //0x34
    __IO uint32_t RESERVED1[6];                 //0x38
    struct {
        __I uint32_t TX_SRC_ADDR;               //0x50+N*8
        __I uint32_t RX_DES_ADDR;               //0x54+N*8
    } SX_CUR_TX_RX_ADDR[SENSOR_SLAVE_CNT_MAX];
    __IO uint32_t RESERVED2[28];                //0x90
    struct {
        __IO uint32_t SX_CTRL;                  //0x100+N*0x30
        __IO uint32_t SX_TX_SRC_ADDR;           //0x104+N*0x30
        __IO uint32_t SX_RX_DES_ADDR;           //0x108+N*0x30
        __IO uint32_t SX_TX_TRANSFER_SIZE;      //0x10C+N*0x30
        __IO uint32_t SX_RX_TRANSFER_SIZE;      //0x110+N*0x30
        __IO uint32_t SX_TX_ADDR_LIMIT;         //0x114+N*0x30
        __IO uint32_t SX_RX_ADDR_LIMIT;         //0x118+N*0x30
        __IO uint32_t SX_GPIO_INTR_MASK;        //0x11C+N*0x30
        __IO uint32_t SX_TIMER_LOAD;            //0x120+N*0x30
        __I  uint32_t SX_TIMER_VALUE;           //0x124+N*0x30
        __IO uint32_t SX_DESC_CTRL;             //0x128+N*0x30
        __IO uint32_t SX_DESC_PTR;              //0x12C+N*0x30
    } SX_CONFIG[SENSOR_SLAVE_CNT_MAX];
};

// Sx_ctrl(0x100+N*0x30),new added,tanwenchen,2021-01-05
#define SENSOR_ENGINE_SX_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_SX_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_SX_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_SX_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_SX_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_SX_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_SX_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_SX_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_SX_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_SX_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_SX_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_SX_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_SX_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_SX_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_SX_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_SX_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_SX_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_SX_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_SX_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_SX_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_SX_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_SX_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_SX_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_SX_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_SX_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_SX_INTR_EN                            (1 << 31)
// Sx_desc_ctrl(0x128+N*0x30),new added,tanwenchen,2021-02-07
#define SENSOR_ENGINE_SX_DESC_ADDR_INC                      (1 << 0)

// reg_000
#define SENSOR_ENGINE_SENSORENG_EN                          (1 << 0)
#define SENSOR_ENGINE_CODEC_SX_SEL(n)                       (((n) & 0x7) << 1)
#define SENSOR_ENGINE_CODEC_SX_SEL_MASK                     (0x7 << 1)
#define SENSOR_ENGINE_CODEC_SX_SEL_SHIFT                    (1)

// reg_004
#define SENSOR_ENGINE_I2C0_MBASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_I2C0_MBASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_I2C0_MBASE_ADDR_SHIFT                 (0)

// reg_008
#define SENSOR_ENGINE_I2C1_MBASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_I2C1_MBASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_I2C1_MBASE_ADDR_SHIFT                 (0)

// reg_00c
#define SENSOR_ENGINE_I2C2_MBASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_I2C2_MBASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_I2C2_MBASE_ADDR_SHIFT                 (0)

// reg_010
#define SENSOR_ENGINE_I2C3_MBASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_I2C3_MBASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_I2C3_MBASE_ADDR_SHIFT                 (0)

// reg_014
#define SENSOR_ENGINE_I2C4_MBASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_I2C4_MBASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_I2C4_MBASE_ADDR_SHIFT                 (0)

// reg_018
#define SENSOR_ENGINE_SPI_MBASE_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_SPI_MBASE_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_SPI_MBASE_ADDR_SHIFT                  (0)

// reg_01c
#define SENSOR_ENGINE_GPIO0_BASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_GPIO0_BASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_GPIO0_BASE_ADDR_SHIFT                 (0)

// reg_020
#define SENSOR_ENGINE_GPIO1_BASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_GPIO1_BASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_GPIO1_BASE_ADDR_SHIFT                 (0)

// reg_024
#define SENSOR_ENGINE_GPIO2_BASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_GPIO2_BASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_GPIO2_BASE_ADDR_SHIFT                 (0)

// reg_028
#define SENSOR_ENGINE_GPIO3_BASE_ADDR(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_GPIO3_BASE_ADDR_MASK                  (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_GPIO3_BASE_ADDR_SHIFT                 (0)

// reg_02c
#define SENSOR_ENGINE_RESERVED0(n)                          (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_RESERVED0_MASK                        (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_RESERVED0_SHIFT                       (0)

// reg_030
#define SENSOR_ENGINE_S0_WAKEUP_INTR_CLR                    (1 << 0)
#define SENSOR_ENGINE_S1_WAKEUP_INTR_CLR                    (1 << 1)
#define SENSOR_ENGINE_S2_WAKEUP_INTR_CLR                    (1 << 2)
#define SENSOR_ENGINE_S3_WAKEUP_INTR_CLR                    (1 << 3)
#define SENSOR_ENGINE_S4_WAKEUP_INTR_CLR                    (1 << 4)
#define SENSOR_ENGINE_S5_WAKEUP_INTR_CLR                    (1 << 5)
#define SENSOR_ENGINE_S6_WAKEUP_INTR_CLR                    (1 << 6)
#define SENSOR_ENGINE_S7_WAKEUP_INTR_CLR                    (1 << 7)

// reg_034
#define SENSOR_ENGINE_BUSY                                  (1 << 0)
#define SENSOR_ENGINE_S0_WAKEUP_INTR_STS                    (1 << 1)
#define SENSOR_ENGINE_S1_WAKEUP_INTR_STS                    (1 << 2)
#define SENSOR_ENGINE_S2_WAKEUP_INTR_STS                    (1 << 3)
#define SENSOR_ENGINE_S3_WAKEUP_INTR_STS                    (1 << 4)
#define SENSOR_ENGINE_S4_WAKEUP_INTR_STS                    (1 << 5)
#define SENSOR_ENGINE_S5_WAKEUP_INTR_STS                    (1 << 6)
#define SENSOR_ENGINE_S6_WAKEUP_INTR_STS                    (1 << 7)
#define SENSOR_ENGINE_S7_WAKEUP_INTR_STS                    (1 << 8)
#define SENSOR_ENGINE_S0_TRIGGER_INTR_STS                   (1 << 9)
#define SENSOR_ENGINE_S1_TRIGGER_INTR_STS                   (1 << 10)
#define SENSOR_ENGINE_S2_TRIGGER_INTR_STS                   (1 << 11)
#define SENSOR_ENGINE_S3_TRIGGER_INTR_STS                   (1 << 12)
#define SENSOR_ENGINE_S4_TRIGGER_INTR_STS                   (1 << 13)
#define SENSOR_ENGINE_S5_TRIGGER_INTR_STS                   (1 << 14)
#define SENSOR_ENGINE_S6_TRIGGER_INTR_STS                   (1 << 15)
#define SENSOR_ENGINE_S7_TRIGGER_INTR_STS                   (1 << 16)
#define SENSOR_ENGINE_STATE(n)                              (((n) & 0x1F) << 17)
#define SENSOR_ENGINE_STATE_MASK                            (0x1F << 17)
#define SENSOR_ENGINE_STATE_SHIFT                           (17)

// reg_038
#define SENSOR_ENGINE_RESERVED1(n)                          (((n) & 0x7FFFFFFF) << 0)
#define SENSOR_ENGINE_RESERVED1_MASK                        (0x7FFFFFFF << 0)
#define SENSOR_ENGINE_RESERVED1_SHIFT                       (0)

// reg_050
#define SENSOR_ENGINE_S0_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_054
#define SENSOR_ENGINE_S0_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_058
#define SENSOR_ENGINE_S1_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_05c
#define SENSOR_ENGINE_S1_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_060
#define SENSOR_ENGINE_S2_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_064
#define SENSOR_ENGINE_S2_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_068
#define SENSOR_ENGINE_S3_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_06c
#define SENSOR_ENGINE_S3_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_070
#define SENSOR_ENGINE_S4_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_074
#define SENSOR_ENGINE_S4_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_078
#define SENSOR_ENGINE_S5_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_07c
#define SENSOR_ENGINE_S5_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_080
#define SENSOR_ENGINE_S6_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_084
#define SENSOR_ENGINE_S6_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_088
#define SENSOR_ENGINE_S7_CUR_TX_SRC_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_CUR_TX_SRC_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_CUR_TX_SRC_ADDR_SHIFT              (0)

// reg_08c
#define SENSOR_ENGINE_S7_CUR_RX_DES_ADDR(n)                 (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_CUR_RX_DES_ADDR_MASK               (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_CUR_RX_DES_ADDR_SHIFT              (0)

// reg_100
#define SENSOR_ENGINE_S0_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S0_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S0_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S0_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S0_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S0_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S0_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S0_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S0_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S0_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S0_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S0_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S0_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S0_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S0_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S0_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S0_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S0_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S0_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S0_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S0_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S0_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S0_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S0_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S0_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S0_INTR_EN                            (1 << 31)

// reg_104
#define SENSOR_ENGINE_S0_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_TX_SRC_ADDR_SHIFT                  (0)

// reg_108
#define SENSOR_ENGINE_S0_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_RX_DES_ADDR_SHIFT                  (0)

// reg_10c
#define SENSOR_ENGINE_S0_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_110
#define SENSOR_ENGINE_S0_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_114
#define SENSOR_ENGINE_S0_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_TX_ADDR_LIMIT_SHIFT                (0)

// reg_118
#define SENSOR_ENGINE_S0_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_RX_ADDR_LIMIT_SHIFT                (0)

// reg_11c
#define SENSOR_ENGINE_S0_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_GPIO_INTR_MASK_SHIFT               (0)

// reg_120
#define SENSOR_ENGINE_S0_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_TIMER_LOAD_SHIFT                   (0)

// reg_124
#define SENSOR_ENGINE_S0_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_TIMER_VALUE_SHIFT                  (0)

// reg_128
#define SENSOR_ENGINE_S0_DESC_ADDR_INC                      (1 << 0)

// reg_12c
#define SENSOR_ENGINE_S0_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S0_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S0_DESC_PTR_SHIFT                     (0)

// reg_130
#define SENSOR_ENGINE_S1_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S1_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S1_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S1_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S1_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S1_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S1_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S1_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S1_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S1_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S1_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S1_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S1_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S1_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S1_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S1_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S1_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S1_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S1_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S1_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S1_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S1_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S1_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S1_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S1_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S1_INTR_EN                            (1 << 31)

// reg_134
#define SENSOR_ENGINE_S1_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_TX_SRC_ADDR_SHIFT                  (0)

// reg_138
#define SENSOR_ENGINE_S1_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_RX_DES_ADDR_SHIFT                  (0)

// reg_13c
#define SENSOR_ENGINE_S1_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_140
#define SENSOR_ENGINE_S1_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_144
#define SENSOR_ENGINE_S1_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_TX_ADDR_LIMIT_SHIFT                (0)

// reg_148
#define SENSOR_ENGINE_S1_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_RX_ADDR_LIMIT_SHIFT                (0)

// reg_14c
#define SENSOR_ENGINE_S1_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_GPIO_INTR_MASK_SHIFT               (0)

// reg_150
#define SENSOR_ENGINE_S1_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_TIMER_LOAD_SHIFT                   (0)

// reg_154
#define SENSOR_ENGINE_S1_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_TIMER_VALUE_SHIFT                  (0)

// reg_158
#define SENSOR_ENGINE_S1_DESC_ADDR_INC                      (1 << 0)

// reg_15c
#define SENSOR_ENGINE_S1_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S1_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S1_DESC_PTR_SHIFT                     (0)

// reg_160
#define SENSOR_ENGINE_S2_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S2_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S2_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S2_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S2_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S2_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S2_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S2_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S2_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S2_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S2_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S2_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S2_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S2_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S2_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S2_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S2_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S2_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S2_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S2_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S2_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S2_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S2_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S2_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S2_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S2_INTR_EN                            (1 << 31)

// reg_164
#define SENSOR_ENGINE_S2_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_TX_SRC_ADDR_SHIFT                  (0)

// reg_168
#define SENSOR_ENGINE_S2_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_RX_DES_ADDR_SHIFT                  (0)

// reg_16c
#define SENSOR_ENGINE_S2_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_170
#define SENSOR_ENGINE_S2_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_174
#define SENSOR_ENGINE_S2_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_TX_ADDR_LIMIT_SHIFT                (0)

// reg_178
#define SENSOR_ENGINE_S2_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_RX_ADDR_LIMIT_SHIFT                (0)

// reg_17c
#define SENSOR_ENGINE_S2_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_GPIO_INTR_MASK_SHIFT               (0)

// reg_180
#define SENSOR_ENGINE_S2_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_TIMER_LOAD_SHIFT                   (0)

// reg_184
#define SENSOR_ENGINE_S2_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_TIMER_VALUE_SHIFT                  (0)

// reg_188
#define SENSOR_ENGINE_S2_DESC_ADDR_INC                      (1 << 0)

// reg_18c
#define SENSOR_ENGINE_S2_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S2_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S2_DESC_PTR_SHIFT                     (0)

// reg_190
#define SENSOR_ENGINE_S3_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S3_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S3_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S3_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S3_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S3_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S3_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S3_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S3_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S3_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S3_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S3_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S3_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S3_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S3_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S3_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S3_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S3_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S3_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S3_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S3_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S3_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S3_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S3_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S3_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S3_INTR_EN                            (1 << 31)

// reg_194
#define SENSOR_ENGINE_S3_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_TX_SRC_ADDR_SHIFT                  (0)

// reg_198
#define SENSOR_ENGINE_S3_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_RX_DES_ADDR_SHIFT                  (0)

// reg_19c
#define SENSOR_ENGINE_S3_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_1a0
#define SENSOR_ENGINE_S3_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_1a4
#define SENSOR_ENGINE_S3_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_TX_ADDR_LIMIT_SHIFT                (0)

// reg_1a8
#define SENSOR_ENGINE_S3_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_RX_ADDR_LIMIT_SHIFT                (0)

// reg_1ac
#define SENSOR_ENGINE_S3_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_GPIO_INTR_MASK_SHIFT               (0)

// reg_1b0
#define SENSOR_ENGINE_S3_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_TIMER_LOAD_SHIFT                   (0)

// reg_1b4
#define SENSOR_ENGINE_S3_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_TIMER_VALUE_SHIFT                  (0)

// reg_1b8
#define SENSOR_ENGINE_S3_DESC_ADDR_INC                      (1 << 0)

// reg_1bc
#define SENSOR_ENGINE_S3_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S3_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S3_DESC_PTR_SHIFT                     (0)

// reg_1c0
#define SENSOR_ENGINE_S4_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S4_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S4_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S4_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S4_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S4_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S4_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S4_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S4_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S4_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S4_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S4_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S4_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S4_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S4_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S4_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S4_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S4_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S4_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S4_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S4_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S4_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S4_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S4_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S4_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S4_INTR_EN                            (1 << 31)

// reg_1c4
#define SENSOR_ENGINE_S4_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_TX_SRC_ADDR_SHIFT                  (0)

// reg_1c8
#define SENSOR_ENGINE_S4_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_RX_DES_ADDR_SHIFT                  (0)

// reg_1cc
#define SENSOR_ENGINE_S4_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_1d0
#define SENSOR_ENGINE_S4_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_1d4
#define SENSOR_ENGINE_S4_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_TX_ADDR_LIMIT_SHIFT                (0)

// reg_1d8
#define SENSOR_ENGINE_S4_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_RX_ADDR_LIMIT_SHIFT                (0)

// reg_1dc
#define SENSOR_ENGINE_S4_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_GPIO_INTR_MASK_SHIFT               (0)

// reg_1e0
#define SENSOR_ENGINE_S4_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_TIMER_LOAD_SHIFT                   (0)

// reg_1e4
#define SENSOR_ENGINE_S4_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_TIMER_VALUE_SHIFT                  (0)

// reg_1e8
#define SENSOR_ENGINE_S4_DESC_ADDR_INC                      (1 << 0)

// reg_1ec
#define SENSOR_ENGINE_S4_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S4_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S4_DESC_PTR_SHIFT                     (0)

// reg_1f0
#define SENSOR_ENGINE_S5_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S5_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S5_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S5_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S5_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S5_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S5_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S5_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S5_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S5_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S5_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S5_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S5_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S5_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S5_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S5_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S5_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S5_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S5_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S5_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S5_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S5_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S5_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S5_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S5_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S5_INTR_EN                            (1 << 31)

// reg_1f4
#define SENSOR_ENGINE_S5_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_TX_SRC_ADDR_SHIFT                  (0)

// reg_1f8
#define SENSOR_ENGINE_S5_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_RX_DES_ADDR_SHIFT                  (0)

// reg_1fc
#define SENSOR_ENGINE_S5_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_200
#define SENSOR_ENGINE_S5_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_204
#define SENSOR_ENGINE_S5_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_TX_ADDR_LIMIT_SHIFT                (0)

// reg_208
#define SENSOR_ENGINE_S5_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_RX_ADDR_LIMIT_SHIFT                (0)

// reg_20c
#define SENSOR_ENGINE_S5_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_GPIO_INTR_MASK_SHIFT               (0)

// reg_210
#define SENSOR_ENGINE_S5_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_TIMER_LOAD_SHIFT                   (0)

// reg_214
#define SENSOR_ENGINE_S5_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_TIMER_VALUE_SHIFT                  (0)

// reg_218
#define SENSOR_ENGINE_S5_DESC_ADDR_INC                      (1 << 0)

// reg_21c
#define SENSOR_ENGINE_S5_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S5_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S5_DESC_PTR_SHIFT                     (0)

// reg_220
#define SENSOR_ENGINE_S6_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S6_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S6_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S6_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S6_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S6_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S6_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S6_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S6_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S6_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S6_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S6_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S6_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S6_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S6_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S6_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S6_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S6_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S6_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S6_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S6_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S6_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S6_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S6_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S6_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S6_INTR_EN                            (1 << 31)

// reg_224
#define SENSOR_ENGINE_S6_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_TX_SRC_ADDR_SHIFT                  (0)

// reg_228
#define SENSOR_ENGINE_S6_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_RX_DES_ADDR_SHIFT                  (0)

// reg_22c
#define SENSOR_ENGINE_S6_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_230
#define SENSOR_ENGINE_S6_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_234
#define SENSOR_ENGINE_S6_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_TX_ADDR_LIMIT_SHIFT                (0)

// reg_238
#define SENSOR_ENGINE_S6_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_RX_ADDR_LIMIT_SHIFT                (0)

// reg_23c
#define SENSOR_ENGINE_S6_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_GPIO_INTR_MASK_SHIFT               (0)

// reg_240
#define SENSOR_ENGINE_S6_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_TIMER_LOAD_SHIFT                   (0)

// reg_244
#define SENSOR_ENGINE_S6_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_TIMER_VALUE_SHIFT                  (0)

// reg_248
#define SENSOR_ENGINE_S6_DESC_ADDR_INC                      (1 << 0)

// reg_24c
#define SENSOR_ENGINE_S6_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S6_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S6_DESC_PTR_SHIFT                     (0)

// reg_250
#define SENSOR_ENGINE_S7_ENABLE                             (1 << 0)
#define SENSOR_ENGINE_S7_TAR_DEV_ID(n)                      (((n) & 0x3FF) << 1)
#define SENSOR_ENGINE_S7_TAR_DEV_ID_MASK                    (0x3FF << 1)
#define SENSOR_ENGINE_S7_TAR_DEV_ID_SHIFT                   (1)
#define SENSOR_ENGINE_S7_I2C_SPI_SEL(n)                     (((n) & 0x7) << 11)
#define SENSOR_ENGINE_S7_I2C_SPI_SEL_MASK                   (0x7 << 11)
#define SENSOR_ENGINE_S7_I2C_SPI_SEL_SHIFT                  (11)
#define SENSOR_ENGINE_S7_RDN_WR                             (1 << 14)
#define SENSOR_ENGINE_S7_SPI_RXDS                           (1 << 15)
#define SENSOR_ENGINE_S7_SPI_CS(n)                          (((n) & 0xF) << 16)
#define SENSOR_ENGINE_S7_SPI_CS_MASK                        (0xF << 16)
#define SENSOR_ENGINE_S7_SPI_CS_SHIFT                       (16)
#define SENSOR_ENGINE_S7_GPIO_SEL(n)                        (((n) & 0x3) << 20)
#define SENSOR_ENGINE_S7_GPIO_SEL_MASK                      (0x3 << 20)
#define SENSOR_ENGINE_S7_GPIO_SEL_SHIFT                     (20)
#define SENSOR_ENGINE_S7_TX_HWIDTH(n)                       (((n) & 0x3) << 22)
#define SENSOR_ENGINE_S7_TX_HWIDTH_MASK                     (0x3 << 22)
#define SENSOR_ENGINE_S7_TX_HWIDTH_SHIFT                    (22)
#define SENSOR_ENGINE_S7_RX_HWIDTH(n)                       (((n) & 0x3) << 24)
#define SENSOR_ENGINE_S7_RX_HWIDTH_MASK                     (0x3 << 24)
#define SENSOR_ENGINE_S7_RX_HWIDTH_SHIFT                    (24)
#define SENSOR_ENGINE_S7_TRIGGER_SEL                        (1 << 26)
#define SENSOR_ENGINE_S7_INTR_NUM(n)                        (((n) & 0xF) << 27)
#define SENSOR_ENGINE_S7_INTR_NUM_MASK                      (0xF << 27)
#define SENSOR_ENGINE_S7_INTR_NUM_SHIFT                     (27)
#define SENSOR_ENGINE_S7_INTR_EN                            (1 << 31)

// reg_254
#define SENSOR_ENGINE_S7_TX_SRC_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_TX_SRC_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_TX_SRC_ADDR_SHIFT                  (0)

// reg_258
#define SENSOR_ENGINE_S7_RX_DES_ADDR(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_RX_DES_ADDR_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_RX_DES_ADDR_SHIFT                  (0)

// reg_25c
#define SENSOR_ENGINE_S7_TX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_TX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_TX_TRANSFER_SIZE_SHIFT             (0)

// reg_260
#define SENSOR_ENGINE_S7_RX_TRANSFER_SIZE(n)                (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_RX_TRANSFER_SIZE_MASK              (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_RX_TRANSFER_SIZE_SHIFT             (0)

// reg_264
#define SENSOR_ENGINE_S7_TX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_TX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_TX_ADDR_LIMIT_SHIFT                (0)

// reg_268
#define SENSOR_ENGINE_S7_RX_ADDR_LIMIT(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_RX_ADDR_LIMIT_MASK                 (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_RX_ADDR_LIMIT_SHIFT                (0)

// reg_26c
#define SENSOR_ENGINE_S7_GPIO_INTR_MASK(n)                  (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_GPIO_INTR_MASK_MASK                (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_GPIO_INTR_MASK_SHIFT               (0)

// reg_270
#define SENSOR_ENGINE_S7_TIMER_LOAD(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_TIMER_LOAD_MASK                    (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_TIMER_LOAD_SHIFT                   (0)

// reg_274
#define SENSOR_ENGINE_S7_TIMER_VALUE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_TIMER_VALUE_MASK                   (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_TIMER_VALUE_SHIFT                  (0)

// reg_278
#define SENSOR_ENGINE_S7_DESC_ADDR_INC                      (1 << 0)

// reg_27c
#define SENSOR_ENGINE_S7_DESC_PTR(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENSOR_ENGINE_S7_DESC_PTR_MASK                      (0xFFFFFFFF << 0)
#define SENSOR_ENGINE_S7_DESC_PTR_SHIFT                     (0)

#endif


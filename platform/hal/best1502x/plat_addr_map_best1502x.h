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
#ifndef __PLAT_ADDR_MAP_BEST1502X_H__
#define __PLAT_ADDR_MAP_BEST1502X_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LARGE_SENS_RAM
#ifdef SMALL_RET_RAM
#error "LARGE_SENS_RAM conflicts with SMALL_RET_RAM"
#endif
#ifdef CORE_SLEEP_POWER_DOWN
#error "MCU retention ram needs shrinking when both LARGE_SENS_RAM and CORE_SLEEP_POWER_DOWN are defined"
#endif
#endif

#if defined(ROM_BUILD) && !defined(SIMU) && !defined(FPGA) && !defined(NO_MCU_RAM_ONLY)
#define MCU_RAM_ONLY
#define RAM_BASE                                RAM0_BASE
#define RAM_SIZE                                (RAM4_BASE - RAM_BASE)
#ifdef CORE_SLEEP_POWER_DOWN
#error "MCU RAM will be power down when CORE_SLEEP_POWER_DOWN defined"
#endif
#endif

#ifndef SMALL_RET_RAM
#define SENS_RAMRET
#endif

#define ROM_BASE                                0x24000000
#define ROMX_BASE                               0x00020000

#ifndef ROM_SIZE
#define ROM_SIZE                                0x00010000
#endif
#define ROM_EXT_SIZE                            0x00008000

#define PATCH_ENTRY_NUM                         8
#define PATCH_CTRL_BASE                         0x0004F000
#define PATCH_DATA_BASE                         0x0004F100

#define SHR_RAM_BLK_SIZE                        0x00040000

#define RAM0_BASE                               0x20000000
#define RAMX0_BASE                              0x00200000
#define RAM1_BASE                               0x20020000
#define RAMX1_BASE                              0x00220000
#define RAM2_BASE                               0x20040000
#define RAMX2_BASE                              0x00240000
#define RAM3_BASE                               0x20080000
#define RAMX3_BASE                              0x00280000
#define RAM4_BASE                               0x200C0000
#define RAMX4_BASE                              0x002C0000
#define RAM5_BASE                               0x20100000
#define RAMX5_BASE                              0x00300000
#define RAM6_BASE                               0x20140000
#define RAMX6_BASE                              0x00340000
#define RAM7_BASE                               0x20180000
#define RAMX7_BASE                              0x00380000
#define RAM8_BASE                               0x201C0000
#define RAMX8_BASE                              0x003C0000

#define RAM8_SIZE                               0x00040000

#define RAM_TOTAL_SIZE                          (RAM8_BASE + RAM8_SIZE - RAM0_BASE)

#define RAMSENS_BASE                            0x20200000
#define RAMXSENS_BASE                           0x00400000

#define SENS_MIN_RAM_SIZE                       0x00010000
#define CODEC_VAD_MAX_BUF_SIZE                  0x00018000

#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE > 0)
#if (CODEC_VAD_CFG_BUF_SIZE & (0x4000 - 1)) || (CODEC_VAD_CFG_BUF_SIZE > CODEC_VAD_MAX_BUF_SIZE)
#error "Bad sensor CODEC_VAD_CFG_BUF_SIZE"
#endif
#define RAMSENS_SIZE                            (SENS_MIN_RAM_SIZE + (CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_CFG_BUF_SIZE))
#else
#define RAMSENS_SIZE                            (SENS_MIN_RAM_SIZE + CODEC_VAD_MAX_BUF_SIZE)
#endif

#define SENS_NORM_MAILBOX_BASE                  (RAMSENS_BASE + RAMSENS_SIZE - SENS_NORM_MAILBOX_SIZE)

#define SENS_MAILBOX_SIZE                       0x10

#if defined(SENS_RAMCP_TOP) && (SENS_RAMCP_TOP != RAM5_BASE) && (SENS_RAMCP_TOP != RAM6_BASE) && \
                               (SENS_RAMCP_TOP != RAM7_BASE) && (SENS_RAMCP_TOP != RAM8_BASE)
#error "Bad SENS_RAMCP_TOP"
#endif

#ifndef RAM_SIZE
#ifndef SENS_RAMCP_TOP
#define SENS_RAMCP_TOP                          RAM6_BASE
#endif
#else
#if (RAM_SIZE != SHR_RAM_BLK_SIZE*1) && (RAM_SIZE != SHR_RAM_BLK_SIZE*2) && \
    (RAM_SIZE != SHR_RAM_BLK_SIZE*3) && (RAM_SIZE != SHR_RAM_BLK_SIZE*4)
#error "Bad mcu RAM_SIZE"
#endif

#ifndef SENS_RAMCP_TOP
#define SENS_RAMCP_TOP                          (RAM0_BASE+RAM_SIZE)
#endif
#endif

#ifdef CHIP_SUBSYS_SENS

#if defined(CP_IN_SAME_EE) && !defined(MCU_RAM_ONLY)
#ifndef RAMCPX_SIZE
#define RAMCPX_SIZE                             (RAMX1_BASE - RAMX0_BASE)
#endif
#define RAMCPX_BASE                             (SENS_RAMCP_TOP -RAM0_BASE + RAMX0_BASE)

#ifndef RAMCP_SIZE
#define RAMCP_SIZE                              (RAM2_BASE - RAM1_BASE)
#endif
#define RAMCP_BASE                              (RAMCPX_BASE + RAMCPX_SIZE - RAMX0_BASE + RAM0_BASE)

#define RAMCP_TOP                               (RAMCP_BASE + RAMCP_SIZE)
#else
#define RAMCP_TOP                               SENS_RAMCP_TOP
#endif

#else   /* CHIP_SUBSYS_SENS */

#if defined(CP_IN_SAME_EE) && !defined(MCU_RAM_ONLY)
#ifdef __BT_RAM_DISTRIBUTION__
#ifndef RAMCPX_SIZE
#define RAMCPX_SIZE                             (RAMX7_BASE - RAMX6_BASE)
#endif
#define RAMCPX_BASE                             (RAMX6_BASE)

#ifndef RAMCP_SIZE
#define RAMCP_SIZE                              (RAM8_SIZE + RAMX8_BASE - RAMX7_BASE)
#endif
#define RAMCP_BASE                              (RAMCPX_BASE + RAMCPX_SIZE - RAMX6_BASE + RAM6_BASE)

#define RAMCP_TOP                               (RAMCP_BASE + RAMCP_SIZE)
#else /* !__BT_RAM_DISTRIBUTION__ */
#ifndef RAMCPX_SIZE
#define RAMCPX_SIZE                             (RAMX1_BASE - RAMX0_BASE)
#endif
#define RAMCPX_BASE                             (RAMX0_BASE)

#ifndef RAMCP_SIZE
#define RAMCP_SIZE                              (RAM2_BASE - RAM1_BASE)
#endif
#define RAMCP_BASE                              (RAMCPX_BASE + RAMCPX_SIZE - RAMX0_BASE + RAM0_BASE)
#endif

#define RAMCP_TOP                               (RAMCP_BASE + RAMCP_SIZE)
#elif defined(CP_AS_SUBSYS) || defined(CHIP_ROLE_CP)
#ifdef __BT_RAM_DISTRIBUTION__
#define RAMCP_SUBSYS_BASE                       RAM7_BASE
#define RAMCP_SUBSYS_SIZE                       (RAM8_BASE - RAM7_BASE)
#else /* !__BT_RAM_DISTRIBUTION__ */
#define RAMCP_SUBSYS_BASE                       RAM0_BASE
#define RAMCP_SUBSYS_SIZE                       (RAM2_BASE - RAM0_BASE)
#endif
#define CP_SUBSYS_MAILBOX_SIZE                  0x20
#define CP_SUBSYS_MAILBOX_BASE                  (RAMCP_SUBSYS_BASE + RAMCP_SUBSYS_SIZE - CP_SUBSYS_MAILBOX_SIZE)

#define RAMCP_SIZE                              0

#define RAMCP_TOP                               (RAMCP_SUBSYS_BASE + RAMCP_SUBSYS_SIZE)
#else
#define RAMCP_TOP                               RAM0_BASE
#endif

#endif /* !CHIP_SUBSYS_SENS */

#ifdef SENS_RAMRET
#define SENS_MAILBOX_BASE                       SENS_RET_MAILBOX_BASE
#define SENS_RET_MAILBOX_SIZE                   SENS_MAILBOX_SIZE
#define SENS_NORM_MAILBOX_SIZE                  0

#ifdef CHIP_SUBSYS_SENS
#define SENS_RAM_BASE                           RAMCP_TOP
#else
#define SENS_RAM_BASE                           SENS_RAMCP_TOP
#endif

#ifdef SENS_RAM_BASE
#if (SENS_RAM_BASE < RAM4_BASE) || (SENS_RAM_BASE > RAMSENS_BASE)
#error "Bad sensor SENS_RAM_BASE"
#endif
#else
#define SENS_RAM_BASE                           RAM8_BASE
#endif

#define SENS_RAMRET_BASE                        SENS_RAM_BASE
#define SENS_RAMXRET_BASE                       (SENS_RAM_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef SENS_RAMRET_SIZE
#if (SENS_RAMRET_BASE + SENS_RAMRET_SIZE + SENS_RET_MAILBOX_SIZE) > RAMSENS_BASE
#error "Bad sensor SENS_RAMRET_SIZE"
#endif
#else
#define SENS_RAMRET_SIZE                        (RAMSENS_BASE - SENS_RET_MAILBOX_SIZE - SENS_RAMRET_BASE)
#endif

#define SENS_RET_MAILBOX_BASE                   (SENS_RAMRET_BASE + SENS_RAMRET_SIZE)

#else /* !SENS_RAMRET */
#define SENS_MAILBOX_BASE                       SENS_NORM_MAILBOX_BASE
#define SENS_RET_MAILBOX_SIZE                   0
#define SENS_NORM_MAILBOX_SIZE                  SENS_MAILBOX_SIZE

#ifdef SENS_RAM_BASE
#if (SENS_RAM_BASE < RAM4_BASE) || (SENS_RAM_BASE >= SENS_NORM_MAILBOX_BASE)
#error "Bad sensor SENS_RAM_BASE"
#endif
#else
#define SENS_RAM_BASE                           RAMSENS_BASE
#endif

#define SENS_RAMRET_BASE                        SENS_RAM_BASE
#define SENS_RAMRET_SIZE                        0

#endif /* !SENS_RAMRET */

#ifdef SENS_RAM_SIZE
#if (SENS_RAM_BASE + SENS_RAM_SIZE > SENS_NORM_MAILBOX_BASE)
#error "Bad sensor SENS_RAM_SIZE"
#endif
#else
#define SENS_RAM_SIZE                           (SENS_NORM_MAILBOX_BASE - SENS_RAM_BASE)
#endif
#define SENS_RAMX_BASE                          (SENS_RAM_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef CHIP_SUBSYS_SENS



#ifdef SENS_RAMRET
#ifdef RAMRET_BASE
#if (RAMRET_BASE < SENS_RAMRET_BASE) || (RAMRET_BASE >= (SENS_RAMRET_BASE + SENS_RAMRET_SIZE))
#error "Bad sensor RAMRET_BASE"
#endif
#else
#define RAMRET_BASE                             SENS_RAMRET_BASE
#endif
#define RAMXRET_BASE                            (RAMRET_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef RAMRET_SIZE
#if ((RAMRET_BASE + RAMRET_SIZE) > (SENS_RAMRET_BASE + SENS_RAMRET_SIZE))
#error "Bad sensor RAMRET_SIZE"
#endif
#else
#define RAMRET_SIZE                             SENS_RAMRET_SIZE
#endif

#define RAM_BASE                                (SENS_RAMRET_BASE + SENS_RAMRET_SIZE + SENS_RET_MAILBOX_SIZE)
#define RAM_SIZE                                (SENS_RAM_BASE + SENS_RAM_SIZE - RAM_BASE)

#else /* !SENS_RAMRET */
#ifdef RAM_BASE
#if (RAM_BASE < SENS_RAM_BASE) || (RAM_BASE >= (SENS_RAM_BASE + SENS_RAM_SIZE))
#error "Bad sensor RAM_BASE"
#endif
#else
#define RAM_BASE                                SENS_RAM_BASE
#endif

#ifdef RAM_SIZE
#if ((RAM_BASE + RAM_SIZE) > (SENS_RAM_BASE + SENS_RAM_SIZE))
#error "Bad sensor RAM_SIZE"
#endif
#else
#define RAM_SIZE                                SENS_RAM_SIZE
#endif

#endif /* !SENS_RAMRET */

#define RAMX_BASE                               (RAM_BASE - RAM0_BASE + RAMX0_BASE)

#else /* !CHIP_SUBSYS_SENS */

#ifdef CORE_SLEEP_POWER_DOWN

#ifdef __BT_RAMRUN__
/* RAM0-RAM3: 768K volatile memory
 * RAM4-RAM5: 512K BT controller memory
 * RAM6-RAM8: 384K retention memory
 */
#error "New LDS configuration is needed"
#endif

#ifdef MEM_POOL_BASE
#if (MEM_POOL_BASE < RAMCP_TOP) || (MEM_POOL_BASE >= SENS_RAMRET_BASE)
#error "Bad MEM_POOL_BASE with CORE_SLEEP_POWER_DOWN"
#endif
#else
#define MEM_POOL_BASE                           RAMCP_TOP
#endif

#ifdef MEM_POOL_SIZE
#if (MEM_POOL_BASE + MEM_POOL_SIZE > SENS_RAMRET_BASE)
#error "Bad MEM_POOL_SIZE with CORE_SLEEP_POWER_DOWN"
#endif
#else
#define MEM_POOL_SIZE                           (RAM4_BASE - MEM_POOL_BASE)
#endif

#ifdef SMALL_RET_RAM
#define RAM_BASE                                RAM6_BASE
#endif

#ifdef RAM_BASE
#if (RAM_BASE < MEM_POOL_BASE) || (RAM_BASE >= SENS_RAMRET_BASE)
#error "Bad RAM_BASE with CORE_SLEEP_POWER_DOWN"
#endif
#else
#define RAM_BASE                                (MEM_POOL_BASE + MEM_POOL_SIZE)
#endif
#define RAMX_BASE                               (RAM_BASE - RAM0_BASE + RAMX0_BASE)

#else /* !CORE_SLEEP_POWER_DOWN */

#ifdef CHIP_ROLE_CP
#define RAM_BASE                                RAMCP_SUBSYS_BASE
#elif defined(RAM_BASE)
#if (RAM_BASE < RAMCP_TOP) || (RAM_BASE >= SENS_RAMRET_BASE)
#error "Bad RAM_BASE"
#endif
#else
#ifdef __BT_RAM_DISTRIBUTION__
#define RAM_BASE                                RAM0_BASE
#else
#define RAM_BASE                                RAMCP_TOP
#endif
#endif
#define RAMX_BASE                               (RAM_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef __BT_RAMRUN__
#if (RAM_BASE >= RAM4_BASE)
#error "Bad RAM_BASE with BT ramrun"
#endif
#define RAM_SIZE                                (RAM4_BASE - RAM_BASE)
#endif

#endif /* !CORE_SLEEP_POWER_DOWN */

#ifdef CHIP_ROLE_CP
#define RAM_SIZE                                (CP_SUBSYS_MAILBOX_BASE - RAMCP_SUBSYS_BASE)
#elif defined(RAM_SIZE)
#if (RAM_BASE + RAM_SIZE) > SENS_RAMRET_BASE
#error "Bad RAM_SIZE"
#endif
#else
#define RAM_SIZE                                (SENS_RAMRET_BASE - RAM_BASE)
#endif

#if defined(ARM_CMSE) || defined(ARM_CMNS)
/*MPC: SRAM block size: 0x8000, FLASH block size 0x40000*/
#define RAM_S_SIZE                              0x00018000
#define RAM_NSC_SIZE                            0x00008000
#ifndef FLASH_S_SIZE
#define FLASH_S_SIZE                            0x00040000
#endif

#undef RAM_BASE
#undef RAMX_BASE
#undef RAM_SIZE
#undef RAMCP_BASE
#undef RAMCPX_BASE
#undef RAMCP_SIZE
#undef RAMCPX_SIZE

#define RAMCPX_BASE                             (RAMX0_BASE + RAM_S_SIZE + RAM_NSC_SIZE)
#define RAMCPX_SIZE                             0x20000
#define RAMCP_BASE                              (RAM0_BASE + RAM_S_SIZE + RAM_NSC_SIZE + RAMCPX_SIZE)
#define RAMCP_SIZE                              0x20000
#define RAM_NS_BASE                             (RAMCP_BASE + RAMCP_SIZE)
#define RAMX_NS_BASE                            (RAMCPX_BASE + RAMCPX_SIZE + RAMCP_SIZE)
#define RAM_NS_SIZE                             (SENS_RAMRET_BASE - RAM_BASE)

#if defined(ARM_CMNS)
#define RAM_BASE                                RAM_NS_BASE
#define RAMX_BASE                               RAMX_NS_BASE
#define RAM_SIZE                                RAM_NS_SIZE
#else
#if ((RAM_S_SIZE + RAM_NSC_SIZE) & (0x8000-1))
#error "RAM_S_SIZE should be 0x8000 aligned"
#endif
#if (FLASH_S_SIZE & (0x40000-1))
#error "FLASH_S_SIZE should be 0x40000 aligned"
#endif
#define RAM_BASE                                RAM0_BASE
#define RAMX_BASE                               RAMX0_BASE
#define RAM_SIZE                                RAM_S_SIZE
#ifndef NS_APP_START_OFFSET
#define NS_APP_START_OFFSET                     (FLASH_S_SIZE)
#endif
#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE                       FLASH_S_SIZE
#endif
#endif
#endif /* defined(ARM_CMSE) || defined(ARM_CMNS) */
#endif /* !CHIP_SUBSYS_SENS */

#define REAL_FLASH_BASE                         0x2C000000
#define REAL_FLASH_NC_BASE                      0x28000000
#define REAL_FLASHX_BASE                        0x0C000000
#define REAL_FLASHX_NC_BASE                     0x08000000

#define REAL_FLASH1_BASE                        0x34000000
#define REAL_FLASH1_NC_BASE                     0x30000000
#define REAL_FLASH1X_BASE                       0x14000000
#define REAL_FLASH1X_NC_BASE                    0x10000000


#define PSRAM_BASE                              0x3C000000
#define PSRAM_NC_BASE                           0x38000000
#define PSRAMX_BASE                             0x1C000000
#define PSRAMX_NC_BASE                          0x18000000

#ifndef CHIP_ROLE_CP
#define ICACHE_CTRL_BASE                        0x07FFA000
#define DCACHE_CTRL_BASE                        0x07FFC000
#endif

#define CMU_BASE                                0x40000000
#define MCU_WDT_BASE                            0x40001000
#define MCU_TIMER0_BASE                         0x40002000
#define MCU_TIMER1_BASE                         0x40003000
#define MCU_TIMER2_BASE                         0x40004000
#define I2S0_BASE                               0x4000F000
#define I2S1_BASE                               0x40011000

#ifndef CHIP_ROLE_CP
#define I2C0_BASE                               0x40005000
#define I2C1_BASE                               0x40006000
#define SPI_BASE                                0x40007000
#define TRNG_BASE                               0x40008000
#define ISPI_BASE                               0x40009000
#define UART0_BASE                              0x4000B000
#define UART1_BASE                              0x4000C000
#define UART2_BASE                              0x4000D000
#define BTPCM_BASE                              0x4000E000
#define SPDIF0_BASE                             0x40010000
#define SEC_ENG_BASE                            0x40020000
#define SEC_CTRL_BASE                           0x40030000
#define MPC_FLASH0_BASE                         0x40032000
#define PAGE_SPY_BASE                           0x40050000
#define I2C2_BASE                               0x40060000
#define I2C3_BASE                               0x40061000
#endif

#define AON_CMU_BASE                            0x40080000
#define AON_GPIO_BASE                           0x40081000
#define AON_WDT_BASE                            0x40082000
#define AON_PWM_BASE                            0x40083000
#define AON_TIMER0_BASE                         0x40084000
#define AON_PSC_BASE                            0x40085000
#define AON_IOMUX_BASE                          0x40086000
#define I2C_SLAVE_BASE                          0x40087000
#define AON_SEC_CTRL_BASE                       0x40088000
#define AON_GPIO1_BASE                          0x40089000
#define AON_GPIO2_BASE                          0x4008A000
#define AON_TIMER1_BASE                         0x4008B000
#define AON_PWM1_BASE                           0x4008C000

#ifndef CHIP_ROLE_CP
/* For Main CPU sub-system */
#define REAL_FLASH1_CTRL_BASE                   0x40500000
#define SDMMC0_BASE                             0x40110000
#define REAL_FLASH_CTRL_BASE                    0x40140000
#define BTDUMP_BASE                             0x40150000
#define PSRAM_CTRL_BASE                         0x40160000
#define USB_BASE                                0x40180000
#define SEDMA_BASE                              0x401D0000

#ifndef CHIP_SUBSYS_SENS
#define LCDC_BASE                               0x40200000
#define GA2D_BASE                               0x40210000
#define GPU_BASE                                0x40220000
#define DSI_BASE                                0x40230000
#define GAMU_BASE                               0x40240000
#endif /* !CHIP_SUBSYS_SENS */
#endif /* !CHIP_ROLE_CP */

#define SYS_AUDMA_BASE                          0x40120000
#define SYS_GPDMA_BASE                          0x40130000

#if defined(CHIP_DMA_CFG_IDX) && (CHIP_DMA_CFG_IDX == 1)
#ifdef CHIP_ROLE_CP
/* CP subsys use the AUDMA */
#define AUDMA_BASE                              SYS_AUDMA_BASE
#else
/* Main MCU use the GPDMA */
#define AUDMA_BASE                              SYS_GPDMA_BASE
#endif
#else /* !defined(CHIP_DMA_CFG_IDX) */
#if !defined(CHIP_ROLE_CP)
/* Main MCU use two DMA */
#define AUDMA_BASE                              SYS_AUDMA_BASE
#define GPDMA_BASE                              SYS_GPDMA_BASE
#endif /* !CHIP_ROLE_CP */
#endif

#define CODEC_BASE                              0x40300000

#define BT_SUBSYS_BASE                          0xA0000000
#define BT_RAM_BASE                             0xC0000000
#define BT_RAM_SIZE                             0x00010000
#define BT_EXCH_MEM_BASE                        0xD0210000
#define BT_EXCH_MEM_SIZE                        0x00010000
#define BT_UART_BASE                            0xD0300000
#define BT_CMU_BASE                             0xD0330000

#define SENS_CMU_BASE                           0x50000000
#define SENS_WDT_BASE                           0x50001000
#define SENS_TIMER0_BASE                        0x50002000
#define SENS_TIMER1_BASE                        0x50003000
#define SENS_TIMER2_BASE                        0x50004000
#define SENS_I2C0_BASE                          0x50005000
#define SENS_I2C1_BASE                          0x50006000
#define SENS_SPI_BASE                           0x50007000
#define SENS_SPILCD_BASE                        0x50008000
#define SENS_ISPI_BASE                          0x50009000
#define SENS_UART0_BASE                         0x5000A000
#define SENS_UART1_BASE                         0x5000B000
#define SENS_BTPCM_BASE                         0x5000C000
#define SENS_I2S0_BASE                          0x5000D000
#define SENS_I2C2_BASE                          0x5000E000
#define SENS_I2C3_BASE                          0x5000F000
#define SENS_CAP_SENSOR_BASE                    0x50010000
#define SENS_PAGE_SPY_BASE                      0x50011000

#define SENS_SENSOR_ENG0_BASE                   0x50100000
#define SENS_CODEC_SENSOR_BASE                  0x50110000
#define SENS_SDMA_BASE                          0x50120000
#define SENS_TEP_BASE                           0x50130000
#define SENS_AVS_BASE                           0x50140000

#define SENS_VAD_BASE                           0x50200000

#define CAP_SENSOR_BASE                         SENS_CAP_SENSOR_BASE

#ifdef CHIP_SUBSYS_SENS
/* For sensor hub sub-system */
#undef SPDIF0_BASE
#undef SEC_ENG_BASE
#undef BTDUMP_BASE
#undef TRNG_BASE
#undef SDMMC0_BASE
#undef SDMMC1_BASE
#undef LCDC_BASE
#undef UART2_BASE
#undef ICACHE_CTRL_BASE
#undef DCACHE_CTRL_BASE

#undef I2C0_BASE
#define I2C0_BASE                               SENS_I2C0_BASE
#undef I2C1_BASE
#define I2C1_BASE                               SENS_I2C1_BASE
#undef I2C2_BASE
#define I2C2_BASE                               SENS_I2C2_BASE
#undef I2C3_BASE
#define I2C3_BASE                               SENS_I2C3_BASE
#undef SPI_BASE
#define SPI_BASE                                SENS_SPI_BASE
#undef SPILCD_BASE
#define SPILCD_BASE                             SENS_SPILCD_BASE
#undef ISPI_BASE
#define ISPI_BASE                               SENS_ISPI_BASE
#undef UART0_BASE
#define UART0_BASE                              SENS_UART0_BASE
#undef UART1_BASE
#define UART1_BASE                              SENS_UART1_BASE
#undef BTPCM_BASE
#define BTPCM_BASE                              SENS_BTPCM_BASE
#undef I2S0_BASE
#undef I2S1_BASE
#define I2S0_BASE                               SENS_I2S0_BASE
#undef PAGE_SPY_BASE
#define PAGE_SPY_BASE                           SENS_PAGE_SPY_BASE
#undef SENSOR_ENG_BASE
#define SENSOR_ENG_BASE                         SENS_SENSOR_ENG_BASE
#undef AUDMA_BASE
#define AUDMA_BASE                              SENS_SDMA_BASE
#undef GPDMA_BASE
#undef TEP_BASE
#define TEP_BASE                                SENS_TEP_BASE
#undef AVS_BASE
#define AVS_BASE                                SENS_AVS_BASE

#ifdef CORE_SLEEP_POWER_DOWN
#define TIMER0_BASE                             AON_TIMER1_BASE
#else
#define TIMER0_BASE                             SENS_TIMER0_BASE
#endif
#define TIMER1_BASE                             SENS_TIMER1_BASE

#define NO_FLASH_BASE_ACCESS

#else /* !CHIP_SUBSYS_SENS */

#if defined(CP_AS_SUBSYS) || defined(CHIP_ROLE_CP)
#define CP_SUBSYS_TIMER0_BASE                   MCU_TIMER2_BASE
#define CP_SUBSYS_TIMER1_BASE                   MCU_TIMER1_BASE
#endif

#ifdef CHIP_ROLE_CP
#define TIMER0_BASE                             CP_SUBSYS_TIMER0_BASE
#define TIMER1_BASE                             CP_SUBSYS_TIMER1_BASE
#else /* !CHIP_ROLE_CP */
#ifdef CORE_SLEEP_POWER_DOWN
#define TIMER0_BASE                             AON_TIMER0_BASE
#ifndef CP_AS_SUBSYS
#define TIMER2_BASE                             MCU_TIMER0_BASE
#endif
#else /* !CORE_SLEEP_POWER_DOWN */
#define TIMER0_BASE                             MCU_TIMER0_BASE
#ifndef CP_AS_SUBSYS
#define TIMER2_BASE                             AON_TIMER0_BASE
#endif /* !CP_AS_SUBSYS */
#endif /* CORE_SLEEP_POWER_DOWN */
#define TIMER1_BASE                             MCU_TIMER1_BASE
#endif /* CHIP_ROLE_CP */

#ifndef CHIP_ROLE_CP
#ifdef ALT_BOOT_FLASH
#define FLASH_BASE                              REAL_FLASH1_BASE
#define FLASH_NC_BASE                           REAL_FLASH1_NC_BASE
#define FLASHX_BASE                             REAL_FLASH1X_BASE
#define FLASHX_NC_BASE                          REAL_FLASH1X_NC_BASE

#define FLASH1_BASE                             REAL_FLASH_BASE
#define FLASH1_NC_BASE                          REAL_FLASH_NC_BASE
#define FLASH1X_BASE                            REAL_FLASHX_BASE
#define FLASH1X_NC_BASE                         REAL_FLASHX_NC_BASE

#define FLASH_CTRL_BASE                         REAL_FLASH1_CTRL_BASE
#define FLASH1_CTRL_BASE                        REAL_FLASH_CTRL_BASE
#else
#define FLASH_BASE                              REAL_FLASH_BASE
#define FLASH_NC_BASE                           REAL_FLASH_NC_BASE
#define FLASHX_BASE                             REAL_FLASHX_BASE
#define FLASHX_NC_BASE                          REAL_FLASHX_NC_BASE

#define FLASH1_BASE                             REAL_FLASH1_BASE
#define FLASH1_NC_BASE                          REAL_FLASH1_NC_BASE
#define FLASH1X_BASE                            REAL_FLASH1X_BASE
#define FLASH1X_NC_BASE                         REAL_FLASH1X_NC_BASE

#define FLASH_CTRL_BASE                         REAL_FLASH_CTRL_BASE
#define FLASH1_CTRL_BASE                        REAL_FLASH1_CTRL_BASE
#endif

#if 1 /* defined(PROGRAMMER) && !defined(PROGRAMMER_INFLASH) */
/*
#define FLASH2_BASE                             SENS_FLASH_BASE
#define FLASH2_NC_BASE                          SENS_FLASH_NC_BASE
#define FLASH2X_BASE                            SENS_FLASHX_BASE
#define FLASH2X_NC_BASE                         SENS_FLASHX_NC_BASE

#define FLASH2_CTRL_BASE                        SENS_FLASH_CTRL_BASE
*/
#endif
#endif // !CHIP_ROLE_CP

#endif /* !CHIP_SUBSYS_SENS */

#define IOMUX_BASE                              AON_IOMUX_BASE
#define GPIO_BASE                               AON_GPIO_BASE
#define GPIO1_BASE                              AON_GPIO1_BASE
#define GPIO2_BASE                              AON_GPIO2_BASE
#define PWM_BASE                                AON_PWM_BASE
#define PWM1_BASE                               AON_PWM1_BASE
#define WDT_BASE                                AON_WDT_BASE

/* For linker scripts */
#if defined(CHIP_SUBSYS_SENS)
#define VECTOR_SECTION_SIZE                     320
#else
#define VECTOR_SECTION_SIZE                     380
#endif
#define REBOOT_PARAM_SECTION_SIZE               64
#define ROM_BUILD_INFO_SECTION_SIZE             40
#define ROM_EXPORT_FN_SECTION_SIZE              128
#define BT_INTESYS_MEM_OFFSET                   0x00004000

/* For module features */
#define CODEC_FREQ_CRYSTAL                      CODEC_FREQ_24M
#define CODEC_FREQ_EXTRA_DIV                    2
#define CODEC_PLL_DIV                           16
#define CODEC_CMU_DIV                           8
#define CODEC_PLAYBACK_BIT_DEPTH                24
#define CODEC_HAS_FIR
#define GPADC_CTRL_VER                          3
#define GPADC_VALUE_BITS                        16
#define GPADC_HAS_VSYS_DIV
#define GPADC_HAS_EXT_SLOPE_CAL
#define SEC_ENG_HAS_HASH
#define DCDC_CLOCK_CONTROL
#define PWRKEY_IRQ_IN_PMU
#ifndef AUD_SECTION_STRUCT_VERSION
#define AUD_SECTION_STRUCT_VERSION              4
#endif

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#define NO_FLASH_S_ACCESS
#endif

/* For boot struct version */
#ifndef SECURE_BOOT_VER
#define SECURE_BOOT_VER                         4
#endif

/* For ROM export functions */
#define NO_MEMMOVE
#define NO_EXPORT_QSORT
#define NO_EXPORT_BSEARCH
#define NO_EXPORT_VSSCANF

#ifdef __cplusplus
}
#endif

#endif

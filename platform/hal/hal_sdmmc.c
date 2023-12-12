/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#define SDMMCIP_REG_DATA0_BUSY          (1 << 9)
#define SDMMCIP_REG_STATE_MACHINE_BUSY  (1 << 10)
#ifdef SDMMC0_BASE

#include "hal_location.h"
#include "reg_sdmmcip.h"
#include "cmsis_nvic.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sdmmc.h"
#include "hal_sdmmc_pri.h"
#include "hal_iomux.h"
#include "hal_sleep.h"
#include "string.h"
#include "stdio.h"
#include "errno.h"
#ifdef RTOS
    #include "cmsis_os.h"
#endif

#ifdef SDMMC_DEBUG
    #define HAL_SDMMC_TRACE(n, s, ...)      TRACE(n, s , ##__VA_ARGS__)
#else
    #define HAL_SDMMC_TRACE(n, s, ...)      TRACE_DUMMY(n, s , ##__VA_ARGS__)
#endif

#ifdef SDMMC_ASSERT
    #define HAL_SDMMC_ASSERT(n, s, ...)     ASSERT(n, s , ##__VA_ARGS__)
#else
    #define HAL_SDMMC_ASSERT(n, s, ...)
#endif

#if 1
    #define HAL_SDMMC_ERROR(n, s, ...)      TRACE(n, s , ##__VA_ARGS__)
#else
    #define HAL_SDMMC_ERROR(n, s, ...)      TRACE_DUMMY(n, s , ##__VA_ARGS__)
#endif

/******************************************************************************/
/******************************************************************************/
#define SDMMC_FIFO_DEPTH                    16  //width 32bits
#define SDMMC_FIFO_BYTES                    (SDMMC_FIFO_DEPTH * 4)

/* SD/MMC version bits; 8 flags, 8 major, 8 minor, 8 change */
#define SD_VERSION_SD                       (1U << 31)
#define MMC_VERSION_MMC                     (1U << 30)

#define MAKE_SDMMC_VERSION(a, b, c)         ((((uint32_t)(a)) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(c))
#define MAKE_SD_VERSION(a, b, c)            (SD_VERSION_SD | MAKE_SDMMC_VERSION(a, b, c))
#define MAKE_MMC_VERSION(a, b, c)           (MMC_VERSION_MMC | MAKE_SDMMC_VERSION(a, b, c))

#define EXTRACT_SDMMC_MAJOR_VERSION(x)      (((uint32_t)(x) >> 16) & 0xff)
#define EXTRACT_SDMMC_MINOR_VERSION(x)      (((uint32_t)(x) >> 8) & 0xff)
#define EXTRACT_SDMMC_CHANGE_VERSION(x)     ((uint32_t)(x) & 0xff)

#define SD_VERSION_3                        MAKE_SD_VERSION(3, 0, 0)
#define SD_VERSION_2                        MAKE_SD_VERSION(2, 0, 0)
#define SD_VERSION_1_0                      MAKE_SD_VERSION(1, 0, 0)
#define SD_VERSION_1_10                     MAKE_SD_VERSION(1, 10, 0)

#define MMC_VERSION_UNKNOWN                 MAKE_MMC_VERSION(0, 0, 0)
#define MMC_VERSION_1_2                     MAKE_MMC_VERSION(1, 2, 0)
#define MMC_VERSION_1_4                     MAKE_MMC_VERSION(1, 4, 0)
#define MMC_VERSION_2_2                     MAKE_MMC_VERSION(2, 2, 0)
#define MMC_VERSION_3                       MAKE_MMC_VERSION(3, 0, 0)
#define MMC_VERSION_4                       MAKE_MMC_VERSION(4, 0, 0)
#define MMC_VERSION_4_1                     MAKE_MMC_VERSION(4, 1, 0)
#define MMC_VERSION_4_2                     MAKE_MMC_VERSION(4, 2, 0)
#define MMC_VERSION_4_3                     MAKE_MMC_VERSION(4, 3, 0)
#define MMC_VERSION_4_4                     MAKE_MMC_VERSION(4, 4, 0)
#define MMC_VERSION_4_41                    MAKE_MMC_VERSION(4, 4, 1)
#define MMC_VERSION_4_5                     MAKE_MMC_VERSION(4, 5, 0)
#define MMC_VERSION_5_0                     MAKE_MMC_VERSION(5, 0, 0)
#define MMC_VERSION_5_1                     MAKE_MMC_VERSION(5, 1, 0)

#define MMC_CAP(mode)                       (1 << mode)
#define MMC_MODE_HS                         (MMC_CAP(MMC_HS) | MMC_CAP(SD_HS))
#define MMC_MODE_HS_52MHz                    MMC_CAP(MMC_HS_52)
#define MMC_MODE_DDR_52MHz                   MMC_CAP(MMC_DDR_52)
#define MMC_MODE_HS200                       MMC_CAP(MMC_HS_200)
#define MMC_MODE_HS400                       MMC_CAP(MMC_HS_400)
#define MMC_MODE_HS400_ES                    MMC_CAP(MMC_HS_400_ES)

#define UHS_CAPS                            (MMC_CAP(UHS_SDR12) | MMC_CAP(UHS_SDR25)  | \
                                             MMC_CAP(UHS_SDR50) | MMC_CAP(UHS_SDR104) | \
                                             MMC_CAP(UHS_DDR50))

#define UHS_CAPS_MINI                       (MMC_CAP(UHS_SDR12) | MMC_CAP(UHS_SDR25)  | \
                                             MMC_CAP(UHS_SDR50))

#define BIT(nr)                             (1UL << (nr))
#define MMC_CAP_NONREMOVABLE                BIT(14)
#define MMC_CAP_NEEDS_POLL                  BIT(15)
#define MMC_CAP_CD_ACTIVE_HIGH              BIT(16)

#define MMC_MODE_8BIT                       BIT(30)
#define MMC_MODE_4BIT                       BIT(29)
#define MMC_MODE_1BIT                       BIT(28)
#define MMC_MODE_SPI                        BIT(27)

#define SD_DATA_4BIT                        0x00040000

#define IS_SD(x)                            ((x)->version & SD_VERSION_SD)
#define IS_MMC(x)                           ((x)->version & MMC_VERSION_MMC)

#define MMC_DATA_READ                       1
#define MMC_DATA_WRITE                      2

#define MMC_CMD62_ARG1                      0xefac62ec
#define MMC_CMD62_ARG2                      0xcbaea7

#define SD_CMD_SEND_RELATIVE_ADDR           3
#define SD_CMD_SWITCH_FUNC                  6
#define SD_CMD_SEND_IF_COND                 8
#define SD_CMD_SWITCH_UHS18V                11

#define SD_CMD_APP_SET_BUS_WIDTH            6
#define SD_CMD_APP_SD_STATUS                13
#define SD_CMD_ERASE_WR_BLK_START           32
#define SD_CMD_ERASE_WR_BLK_END             33
#define SD_CMD_APP_SEND_OP_COND             41
#define SD_CMD_APP_SEND_SCR                 51

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY                   0x00020000
#define SD_HIGHSPEED_SUPPORTED              0x00020000

#define UHS_SDR12_BUS_SPEED                 0
#define HIGH_SPEED_BUS_SPEED                1
#define UHS_SDR25_BUS_SPEED                 1
#define UHS_SDR50_BUS_SPEED                 2
#define UHS_SDR104_BUS_SPEED                3
#define UHS_DDR50_BUS_SPEED                 4

#define SD_MODE_UHS_SDR12                   BIT(UHS_SDR12_BUS_SPEED)
#define SD_MODE_UHS_SDR25                   BIT(UHS_SDR25_BUS_SPEED)
#define SD_MODE_UHS_SDR50                   BIT(UHS_SDR50_BUS_SPEED)
#define SD_MODE_UHS_SDR104                  BIT(UHS_SDR104_BUS_SPEED)
#define SD_MODE_UHS_DDR50                   BIT(UHS_DDR50_BUS_SPEED)

#define OCR_BUSY                            0x80000000
#define OCR_HCS                             0x40000000
#define OCR_S18R                            0x1000000
#define OCR_VOLTAGE_MASK                    0x007FFF80
#define OCR_ACCESS_MODE                     0x60000000

#define MMC_ERASE_ARG                       0x00000000
#define MMC_SECURE_ERASE_ARG                0x80000000
#define MMC_TRIM_ARG                        0x00000001
#define MMC_DISCARD_ARG                     0x00000003
#define MMC_SECURE_TRIM1_ARG                0x80000001
#define MMC_SECURE_TRIM2_ARG                0x80008000

#define MMC_STATUS_MASK                     (~0x0206BF7F)
#define MMC_STATUS_SWITCH_ERROR             (1 << 7)
#define MMC_STATUS_RDY_FOR_DATA             (1 << 8)
#define MMC_STATUS_CURR_STATE               (0xf << 9)
#define MMC_STATUS_ERROR                    (1 << 19)

#define MMC_STATE_PRG                       (7 << 9)
#define MMC_STATE_TRANS                     (4 << 9)

#define MMC_VDD_165_195                     0x00000080  /* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21                       0x00000100  /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22                       0x00000200  /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23                       0x00000400  /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24                       0x00000800  /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25                       0x00001000  /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26                       0x00002000  /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27                       0x00004000  /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28                       0x00008000  /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29                       0x00010000  /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30                       0x00020000  /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31                       0x00040000  /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32                       0x00080000  /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33                       0x00100000  /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34                       0x00200000  /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35                       0x00400000  /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36                       0x00800000  /* VDD voltage 3.5 ~ 3.6 */

#define MMC_SWITCH_MODE_CMD_SET             0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS            0x01 /* Set bits in EXT_CSD byte
                                                    addressed by index which are
                                                    1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS          0x02 /* Clear bits in EXT_CSD byte
                                                    addressed by index, which are
                                                    1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE          0x03 /* Set target byte to value */

#define SD_SWITCH_CHECK                     0
#define SD_SWITCH_SWITCH                    1

/*
 * EXT_CSD fields
 */
#define EXT_CSD_FLUSH_CACHE                 32  /* W */
#define EXT_CSD_CACHE_CTRL                  33  /* R/W */
#define EXT_CSD_ENH_START_ADDR              136 /* R/W */
#define EXT_CSD_ENH_SIZE_MULT               140 /* R/W */
#define EXT_CSD_GP_SIZE_MULT                143 /* R/W */
#define EXT_CSD_PARTITION_SETTING           155 /* R/W */
#define EXT_CSD_PARTITIONS_ATTRIBUTE        156 /* R/W */
#define EXT_CSD_MAX_ENH_SIZE_MULT           157 /* R */
#define EXT_CSD_PARTITIONING_SUPPORT        160 /* RO */
#define EXT_CSD_RST_N_FUNCTION              162 /* R/W */
#define EXT_CSD_BKOPS_EN                    163 /* R/W & R/W/E */
#define EXT_CSD_WR_REL_PARAM                166 /* R */
#define EXT_CSD_WR_REL_SET                  167 /* R/W */
#define EXT_CSD_RPMB_MULT                   168 /* RO */
#define EXT_CSD_USER_WP                     171 /* R/W & R/W/C_P & R/W/E_P */
#define EXT_CSD_BOOT_WP                     173 /* R/W & R/W/C_P */
#define EXT_CSD_BOOT_WP_STATUS              174 /* R */
#define EXT_CSD_ERASE_GROUP_DEF             175 /* R/W */
#define EXT_CSD_BOOT_BUS_WIDTH              177
#define EXT_CSD_PART_CONF                   179 /* R/W */
#define EXT_CSD_BUS_WIDTH                   183 /* R/W */
#define EXT_CSD_STROBE_SUPPORT              184 /* R/W */
#define EXT_CSD_HS_TIMING                   185 /* R/W */
#define EXT_CSD_REV                         192 /* RO */
#define EXT_CSD_CARD_TYPE                   196 /* RO */
#define EXT_CSD_PART_SWITCH_TIME            199 /* RO */
#define EXT_CSD_SEC_CNT                     212 /* RO, 4 bytes */
#define EXT_CSD_SLEEP_NOTIFICATION_TIME     216 /* RO */
#define EXT_CSD_S_A_TIMEOUT                 217 /* RO */
#define EXT_CSD_HC_WP_GRP_SIZE              221 /* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE           224 /* RO */
#define EXT_CSD_BOOT_MULT                   226 /* RO */
#define EXT_CSD_SEC_FEATURE                 231 /* RO */
#define EXT_CSD_GENERIC_CMD6_TIME           248 /* RO */
#define EXT_CSD_CACHE_SIZE                  249 /* RO, 4 bytes  */
#define EXT_CSD_BKOPS_SUPPORT               502 /* RO */

/*
 * EXT_CSD field definitions
 */
#define EXT_CSD_CMD_SET_NORMAL              (1 << 0)
#define EXT_CSD_CMD_SET_SECURE              (1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE            (1 << 2)

#define EXT_CSD_CARD_TYPE_26                (1 << 0)    /* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52                (1 << 1)    /* Card can run at 52MHz */
#define EXT_CSD_CARD_TYPE_DDR_1_8V          (1 << 2)
#define EXT_CSD_CARD_TYPE_DDR_1_2V          (1 << 3)
#define EXT_CSD_CARD_TYPE_DDR_52            (EXT_CSD_CARD_TYPE_DDR_1_8V | \
                                            EXT_CSD_CARD_TYPE_DDR_1_2V)

#define EXT_CSD_CARD_TYPE_HS200_1_8V        BIT(4)  /* Card can run at 200MHz */
                                                    /* SDR mode @1.8V I/O */
#define EXT_CSD_CARD_TYPE_HS200_1_2V        BIT(5)  /* Card can run at 200MHz */
                                                    /* SDR mode @1.2V I/O */
#define EXT_CSD_CARD_TYPE_HS200             (EXT_CSD_CARD_TYPE_HS200_1_8V | \
                                            EXT_CSD_CARD_TYPE_HS200_1_2V)
#define EXT_CSD_CARD_TYPE_HS400_1_8V        BIT(6)
#define EXT_CSD_CARD_TYPE_HS400_1_2V        BIT(7)
#define EXT_CSD_CARD_TYPE_HS400             (EXT_CSD_CARD_TYPE_HS400_1_8V | \
                                            EXT_CSD_CARD_TYPE_HS400_1_2V)

#define EXT_CSD_BUS_WIDTH_1                 0   /* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4                 1   /* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8                 2   /* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4             5   /* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8             6   /* Card is in 8 bit DDR mode */
#define EXT_CSD_DDR_FLAG                    BIT(2)  /* Flag for DDR mode */
#define EXT_CSD_BUS_WIDTH_STROBE            BIT(7)  /* Enhanced strobe mode */

#define EXT_CSD_TIMING_LEGACY               0   /* no high speed */
#define EXT_CSD_TIMING_HS                   1   /* HS */
#define EXT_CSD_TIMING_HS200                2   /* HS200 */
#define EXT_CSD_TIMING_HS400                3   /* HS400 */
#define EXT_CSD_DRV_STR_SHIFT               4   /* Driver Strength shift */

#define EXT_CSD_BOOT_ACK_ENABLE             (1 << 6)
#define EXT_CSD_BOOT_PARTITION_ENABLE       (1 << 3)
#define EXT_CSD_PARTITION_ACCESS_ENABLE     (1 << 0)
#define EXT_CSD_PARTITION_ACCESS_DISABLE    (0 << 0)

#define EXT_CSD_BOOT_ACK(x)                 (x << 6)
#define EXT_CSD_BOOT_PART_NUM(x)            (x << 3)
#define EXT_CSD_PARTITION_ACCESS(x)         (x << 0)

#define EXT_CSD_EXTRACT_BOOT_ACK(x)         (((x) >> 6) & 0x1)
#define EXT_CSD_EXTRACT_BOOT_PART(x)        (((x) >> 3) & 0x7)
#define EXT_CSD_EXTRACT_PARTITION_ACCESS(x) ((x) & 0x7)

#define EXT_CSD_BOOT_BUS_WIDTH_MODE(x)      (x << 3)
#define EXT_CSD_BOOT_BUS_WIDTH_RESET(x)     (x << 2)
#define EXT_CSD_BOOT_BUS_WIDTH_WIDTH(x)     (x)

#define EXT_CSD_PARTITION_SETTING_COMPLETED (1 << 0)

#define EXT_CSD_ENH_USR                     (1 << 0)        /* user data area is enhanced */
#define EXT_CSD_ENH_GP(x)                   (1 << ((x)+1))  /* GP part (x+1) is enhanced */

#define EXT_CSD_HS_CTRL_REL                 (1 << 0)        /* host controlled WR_REL_SET */

#define EXT_CSD_WR_DATA_REL_USR             (1 << 0)        /* user data area WR_REL */
#define EXT_CSD_WR_DATA_REL_GP(x)           (1 << ((x)+1))  /* GP part (x+1) WR_REL */

#define EXT_CSD_SEC_FEATURE_TRIM_EN         (1 << 4)        /* Support secure & insecure trim */

#define R1_ILLEGAL_COMMAND                  (1 << 22)
#define R1_APP_CMD                          (1 << 5)

#define MMC_RSP_PRESENT                     (1 << 0)
#define MMC_RSP_136                         (1 << 1)        /* 136 bit response */
#define MMC_RSP_CRC                         (1 << 2)        /* expect valid crc */
#define MMC_RSP_BUSY                        (1 << 3)        /* card may send busy */
#define MMC_RSP_OPCODE                      (1 << 4)        /* response contains opcode */

#define MMC_RSP_NONE                        (0)
#define MMC_RSP_R1                          (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b                         (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| MMC_RSP_BUSY)
#define MMC_RSP_R2                          (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3                          (MMC_RSP_PRESENT)
#define MMC_RSP_R4                          (MMC_RSP_PRESENT)
#define MMC_RSP_R5                          (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6                          (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7                          (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMCPART_NOAVAILABLE                 (0xff)
#define PART_ACCESS_MASK                    (0x7)
#define PART_SUPPORT                        (0x1)
#define ENHNCD_SUPPORT                      (0x2)
#define PART_ENH_ATTRIB                     (0x1f)

#define MMC_QUIRK_RETRY_SEND_CID            BIT(0)
#define MMC_QUIRK_RETRY_SET_BLOCKLEN        BIT(1)
#define MMC_QUIRK_RETRY_APP_CMD             BIT(2)

#define MMC_ALL_SIGNAL_VOLTAGE              (MMC_SIGNAL_VOLTAGE_120 | \
                                             MMC_SIGNAL_VOLTAGE_180 | \
                                             MMC_SIGNAL_VOLTAGE_330)

/* Maximum block size for MMC */
#define MMC_MAX_BLOCK_LEN                   512

#define PART_FORMAT_PCAT                    0x1
#define PART_FORMAT_GPT                     0x2

/* The number of MMC physical partitions.  These consist of:
 * boot partitions (2), general purpose partitions (4) in MMC v4.4.
 */
#define MMC_NUM_BOOT_PARTITION              2
#define MMC_PART_RPMB                       3       /* RPMB partition number */

/* timing specification used */
#define MMC_TIMING_LEGACY                   0
#define MMC_TIMING_MMC_HS                   1
#define MMC_TIMING_SD_HS                    2
#define MMC_TIMING_UHS_SDR12                3
#define MMC_TIMING_UHS_SDR25                4
#define MMC_TIMING_UHS_SDR50                5
#define MMC_TIMING_UHS_SDR104               6
#define MMC_TIMING_UHS_DDR50                7
#define MMC_TIMING_MMC_DDR52                8
#define MMC_TIMING_MMC_HS200                9
#define MMC_TIMING_MMC_HS400                10

#define ARCH_DMA_MINALIGN                   32

#define ROUND(a, b)                         (((a) + (b) - 1) & ~((b) - 1))
#define PAD_COUNT(s, pad)                   (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad)                    (PAD_COUNT(s, pad) * pad)

#define __ALIGN_MASK(x, mask)               (((x) + (mask)) &~ (mask))
#define __ALIGN(x, a)                       __ALIGN_MASK((x), (typeof(x))(a) - 1)

#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)            \
    char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)    \
                                        + (align - 1)];                 \
    type *name = (type *)__ALIGN((uintptr_t)__##name, align)

#define ALLOC_ALIGN_BUFFER(type, name, size, align)         \
    ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad) \
    ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)          \
    ALLOC_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)

/*
 * DEFINE_CACHE_ALIGN_BUFFER() is similar to ALLOC_CACHE_ALIGN_BUFFER, but it's
 * purpose is to allow allocating aligned buffers outside of function scope.
 * Usage of this macro shall be avoided or used with extreme care!
 */
#define DEFINE_ALIGN_BUFFER(type, name, size, align)        \
    static char __##name[__ALIGN(size * sizeof(type), align)] \
            __attribute__((__aligned__(align)));            \
                                                            \
    static type *name = (type *)__##name
#define DEFINE_CACHE_ALIGN_BUFFER(type, name, size)         \
    DEFINE_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)

#define be32_to_cpu(x) \
    ((((x) & 0xff000000) >> 24) | \
     (((x) & 0x00ff0000) >>  8) | \
     (((x) & 0x0000ff00) <<  8) | \
     (((x) & 0x000000ff) << 24))

#define LOG2(x) (((x & 0xaaaaaaaa) ? 1 : 0) + ((x & 0xcccccccc) ? 2 : 0) + \
                 ((x & 0xf0f0f0f0) ? 4 : 0) + ((x & 0xff00ff00) ? 8 : 0) + \
                 ((x & 0xffff0000) ? 16 : 0))

#define MMC_CLK_ENABLE              false
#define MMC_CLK_DISABLE             true
#define STATIC_VARIABLES

#ifdef CONFIG_MMC_SPI
    #define mmc_host_is_spi(mmc)    ((mmc)->cfg->host_caps & MMC_MODE_SPI)
#else
    #define mmc_host_is_spi(mmc)    0
#endif

/* Minimum partition switch timeout in units of 10-milliseconds */
#define MMC_MIN_PART_SWITCH_TIME    30 /* 300 ms */
#define DEFAULT_CMD6_TIMEOUT_MS     500

struct mmc;
struct mmc_ops {
    int (*send_cmd)(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data);
    int (*set_ios_clock)(struct mmc *mmc);
    int (*set_ios_width)(struct mmc *mmc);
    int (*init)(struct mmc *mmc);
    int (*wait_dat0)(struct mmc *mmc, int state, int timeout_us);
    int (*getcd)(struct mmc *mmc);
    int (*getwp)(struct mmc *mmc);
    int (*host_power_cycle)(struct mmc *mmc);
    int (*get_b_max)(struct mmc *mmc, void *dst, lbaint_t blkcnt);
};

enum dma_data_direction {
    DMA_BIDIRECTIONAL = 0,
    DMA_TO_DEVICE = 1,
    DMA_FROM_DEVICE = 2,
    DMA_NONE = 3,
};

#ifdef MMC_HW_PARTITIONING
struct mmc_hwpart_conf {
    struct {
        uint32_t enh_start; /* in 512-byte sectors */
        uint32_t enh_size;  /* in 512-byte sectors, if 0 no enh area */
        unsigned wr_rel_change : 1;
        unsigned wr_rel_set : 1;
    } user;
    struct {
        uint32_t size;  /* in 512-byte sectors */
        unsigned enhanced : 1;
        unsigned wr_rel_change : 1;
        unsigned wr_rel_set : 1;
    } gp_part[4];
};

enum mmc_hwpart_conf_mode {
    MMC_HWPART_CONF_CHECK,
    MMC_HWPART_CONF_SET,
    MMC_HWPART_CONF_COMPLETE,
};
#endif

/***************************ip independent function****************************/
static struct mmc sdmmc_devices[HAL_SDMMC_ID_NUM];
SYNC_FLAGS_LOC static uint32_t sdmmc_ext_csd[HAL_SDMMC_ID_NUM][MMC_MAX_BLOCK_LEN / 4];
#ifdef STATIC_VARIABLES
SYNC_FLAGS_LOC static uint32_t noncache_buf512[HAL_SDMMC_ID_NUM][MMC_MAX_BLOCK_LEN / 4];
SYNC_FLAGS_LOC static uint32_t noncache_buf8[HAL_SDMMC_ID_NUM][2];
#endif

//Functional declaration
static int mmc_start_init(struct mmc *mmc);
static int mmc_set_signal_voltage(struct mmc *mmc, uint32_t signal_voltage);
static int mmc_switch_part(struct mmc *mmc, uint32_t part_num);
static void hal_sdmmc_delay_ms(uint32_t ms);
static void hal_sdmmc_delay_us(uint32_t us);

static int sdmmc_ip_wait_cmd_start(struct sdmmc_ip_host *host, uint8_t en_volt_switch);
static inline void sdmmc_ip_writel(struct sdmmc_ip_host *host, uint32_t reg, uint32_t val);
static inline uint32_t sdmmc_ip_readl(struct sdmmc_ip_host *host, uint32_t reg);

#ifndef DM_MMC
    static void board_mmc_power_init(void);
#endif
#ifdef MMC_HS400_SUPPORT
    static int mmc_hs400_prepare_ddr(struct mmc *mmc);
#endif

struct mmc *find_mmc_device(int dev_num)
{
    return &sdmmc_devices[dev_num];
}

static void mmc_do_preinit(struct mmc *mmc)
{
    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);
    if (mmc->preinit) {
        mmc_start_init(mmc);
    }
}

struct blk_desc *mmc_get_blk_desc(struct mmc *mmc)
{
    return &mmc->block_dev;
}

bool mmc_is_tuning_cmd(uint16_t cmdidx)
{
    if ((cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200) ||
        (cmdidx == MMC_CMD_SEND_TUNING_BLOCK))
        return true;
    return false;
}

enum dma_data_direction mmc_get_dma_dir(struct mmc_data *data)
{
    return data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
}

static bool mmc_is_mode_ddr(enum bus_mode mode)
{
    if (mode == MMC_DDR_52)
        return true;
#ifdef MMC_UHS_SUPPORT
    else if (mode == UHS_DDR50)
        return true;
#endif
#ifdef MMC_HS400_SUPPORT
    else if (mode == MMC_HS_400)
        return true;
#endif
#ifdef MMC_HS400_ES_SUPPORT
    else if (mode == MMC_HS_400_ES)
        return true;
#endif
    else
        return false;
}

static bool supports_uhs(uint32_t caps)
{
#ifdef MMC_UHS_SUPPORT
    return (caps & UHS_CAPS) ? true : false;
#else
    return false;
#endif
}

static uint32_t __div64_32(uint64_t *n, uint32_t base)
{
    uint64_t rem = *n;
    uint64_t b = base;
    uint64_t res, d = 1;
    uint32_t high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;
    if (high >= base) {
        high /= base;
        res = (uint64_t) high << 32;
        rem -= (uint64_t)(high * base) << 32;
    }

    while ((int64_t)b > 0 && b < rem) {
        b = b + b;
        d = d + d;
    }

    do {
        if (rem >= b) {
            rem -= b;
            res += d;
        }
        b >>= 1;
        d >>= 1;
    } while (d);

    *n = res;
    return rem;
}

/* The unnecessary pointer compare is there
 * to check for type safety (n must be 64bit)
 */
#define do_div(n,base) ({                               \
        uint32_t __base = (base);                       \
        uint32_t __rem;                                 \
        (void)(((typeof((n)) *)0) == ((uint64_t *)0));  \
        if (((n) >> 32) == 0) {                         \
            __rem = (uint32_t)(n) % __base;             \
            (n) = (uint32_t)(n) / __base;               \
        } else                                          \
            __rem = __div64_32(&(n), __base);           \
        __rem;                                          \
    })

/* Wrapper for do_div(). Doesn't modify dividend and returns
 * the result, not reminder.
 */
static inline uint64_t sdmmc_lldiv(uint64_t dividend, uint32_t divisor)
{
    uint64_t __res = dividend;
    do_div(__res, divisor);
    return (__res);
}

static inline uint64_t div_u64_rem(uint64_t dividend, uint32_t divisor, uint32_t *remainder)
{
    *remainder = do_div(dividend, divisor);
    return dividend;
}

#ifndef DM_MMC
static int mmc_wait_dat0(struct mmc *mmc, int state, int timeout_us)
{
    if (mmc->cfg->ops->wait_dat0) {
        return mmc->cfg->ops->wait_dat0(mmc, state, timeout_us);
    }

    return HAL_SDMMC_INVALID_SYS_CALL;
}

//write protect
int mmc_getwp(struct mmc *mmc)
{
    int wp;

    if (mmc->cfg->ops->getwp)
        wp = mmc->cfg->ops->getwp(mmc);
    else
        wp = 0;

    return wp;
}
#endif

#ifdef SDMMC_DEBUG
static void mmmc_trace_before_send(struct mmc *mmc, struct mmc_cmd *cmd)
{
    HAL_SDMMC_TRACE(0, "CMD_SEND:%d", cmd->cmdidx);
    HAL_SDMMC_TRACE(0, "\t\tARG\t\t\t 0x%08x", cmd->cmdarg);
}

static void mmmc_trace_after_send(struct mmc *mmc, struct mmc_cmd *cmd, int ret)
{
    uint8_t i;
    uint8_t *ptr;

    if (ret) {
        HAL_SDMMC_TRACE(0, "\t\tRET\t\t\t %d", ret);
    } else {
        switch (cmd->resp_type) {
            case MMC_RSP_NONE:
                HAL_SDMMC_TRACE(0, "\t\tMMC_RSP_NONE");
                break;
            case MMC_RSP_R1:
                HAL_SDMMC_TRACE(0, "\t\tMMC_RSP_R1,5,6,7 \t 0x%08x ",
                                cmd->response[0]);
                break;
            case MMC_RSP_R1b:
                HAL_SDMMC_TRACE(0, "\t\tMMC_RSP_R1b\t\t 0x%08x ",
                                cmd->response[0]);
                break;
            case MMC_RSP_R2:
                HAL_SDMMC_TRACE(0, "\t\tMMC_RSP_R2\t\t 0x%08x ",
                                cmd->response[0]);
                HAL_SDMMC_TRACE(0, "\t\t          \t\t 0x%08x ",
                                cmd->response[1]);
                HAL_SDMMC_TRACE(0, "\t\t          \t\t 0x%08x ",
                                cmd->response[2]);
                HAL_SDMMC_TRACE(0, "\t\t          \t\t 0x%08x ",
                                cmd->response[3]);
                HAL_SDMMC_TRACE(0, " ");
                HAL_SDMMC_TRACE(0, "\t\t\t\t\tDUMPING DATA");
                for (i = 0; i < 4; i++) {
                    uint8_t j;
                    HAL_SDMMC_TRACE(TR_ATTR_NO_LF, "\t\t\t\t\t%03d - ", i * 4);
                    ptr = (uint8_t *)&cmd->response[i];
                    ptr += 3;
                    for (j = 0; j < 4; j++)
                        HAL_SDMMC_TRACE(TR_ATTR_NO_LF | TR_ATTR_NO_TS | TR_ATTR_NO_ID, "%02X ", *ptr--);
                    HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, " ");
                }
                break;
            case MMC_RSP_R3:
                HAL_SDMMC_TRACE(0, "\t\tMMC_RSP_R3,4\t\t 0x%08x ",
                                cmd->response[0]);
                break;
            default:
                HAL_SDMMC_TRACE(0, "\t\tERROR MMC rsp not supported");
                break;
        }
    }
}

static void mmc_trace_state(struct mmc *mmc, struct mmc_cmd *cmd)
{
    uint32_t status;

    status = (cmd->response[0] & MMC_STATUS_CURR_STATE) >> 9;
    HAL_SDMMC_TRACE(0, "CURR STATE:%d", status);
}
#endif

static const char *mmc_mode_name(enum bus_mode mode)
{
    static const char *const names[] = {
        [MMC_LEGACY]  = "MMC legacy",
        [MMC_HS]      = "MMC High Speed (24MHz)",
        [SD_HS]       = "SD High Speed (48MHz)",
        [UHS_SDR12]   = "UHS SDR12 (24MHz)",
        [UHS_SDR25]   = "UHS SDR25 (48MHz)",
        [UHS_SDR50]   = "UHS SDR50 (96MHz)",
        [UHS_SDR104]  = "UHS SDR104 (200MHz)",
        [UHS_DDR50]   = "UHS DDR50 (48MHz)",
        [MMC_HS_52]   = "MMC High Speed (48MHz)",
        [MMC_DDR_52]  = "MMC DDR52 (48MHz)",
        [MMC_HS_200]  = "HS200 (200MHz)",
        [MMC_HS_400]  = "HS400 (200MHz)",
        [MMC_HS_400_ES]   = "HS400ES (200MHz)",
    };

    if (mode >= MMC_MODES_END)
        return "Unknown mode";
    else
        return names[mode];
}

static uint32_t mmc_mode2freq(struct mmc *mmc, enum bus_mode mode)
{
    static const uint32_t freqs[] = {
        [MMC_LEGACY]  = 24000000,
        [MMC_HS]      = 24000000,
        [SD_HS]       = 48000000,
        [MMC_HS_52]   = 48000000,
        [MMC_DDR_52]  = 48000000,
        [UHS_SDR12]   = 24000000,
        [UHS_SDR25]   = 48000000,
        [UHS_SDR50]   = 96000000,
        [UHS_DDR50]   = 48000000,
        [UHS_SDR104]  = 200000000,
        [MMC_HS_200]  = 200000000,
        [MMC_HS_400]  = 200000000,
        [MMC_HS_400_ES]   = 200000000,
    };

    if (mode == MMC_LEGACY)
        return mmc->legacy_speed;
    else if (mode >= MMC_MODES_END)
        return 0;
    else
        return freqs[mode];
}

int hal_sdmmc_select_mode(struct mmc *mmc, enum bus_mode mode)
{
    mmc->selected_mode = mode;
    mmc->tran_speed = mmc_mode2freq(mmc, mode);
    mmc->ddr_mode = mmc_is_mode_ddr(mode);
    HAL_SDMMC_TRACE(0, "selecting mode %s (freq : %d MHz)", mmc_mode_name(mode),
                    mmc->tran_speed / 1000000);
    return HAL_SDMMC_ERR_NONE;
}

#ifndef DM_MMC
static int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int ret;

#ifdef SDMMC_DEBUG
    mmmc_trace_before_send(mmc, cmd);
#endif
    ret = mmc->cfg->ops->send_cmd(mmc, cmd, data);
#ifdef SDMMC_DEBUG
    mmmc_trace_after_send(mmc, cmd, ret);
#endif

    return ret;
}
#endif

static int mmc_send_cmd_retry(struct mmc *mmc, struct mmc_cmd *cmd,
                              struct mmc_data *data, uint32_t retries)
{
    int ret;

    do {
        ret = mmc_send_cmd(mmc, cmd, data);
    } while (ret && retries--);

    return ret;
}

static int mmc_send_cmd_quirks(struct mmc *mmc, struct mmc_cmd *cmd,
                               struct mmc_data *data, uint32_t quirk, uint32_t retries)
{
#ifdef MMC_QUIRKS
    if (mmc->quirks & quirk)
        return mmc_send_cmd_retry(mmc, cmd, data, retries);
    else
#endif
        return mmc_send_cmd(mmc, cmd, data);
}

static int mmc_send_status(struct mmc *mmc, uint32_t *status)
{
    struct mmc_cmd cmd;
    int ret;

    cmd.cmdidx = MMC_CMD_SEND_STATUS;
    cmd.resp_type = MMC_RSP_R1;
    if (!mmc_host_is_spi(mmc))
        cmd.cmdarg = mmc->rca << 16;

    ret = mmc_send_cmd_retry(mmc, &cmd, NULL, 4);
#ifdef SDMMC_DEBUG
    mmc_trace_state(mmc, &cmd);
#endif
    if (!ret)
        *status = cmd.response[0];

    return ret;
}

static int mmc_poll_for_busy(struct mmc *mmc, int timeout_ms)
{
    uint32_t status;
    int err;

    err = mmc_wait_dat0(mmc, 1, timeout_ms * 1000);
    if (err != HAL_SDMMC_INVALID_SYS_CALL)
        return err;

    while (1) {
        err = mmc_send_status(mmc, &status);
        if (err)
            return err;

        if ((status & MMC_STATUS_RDY_FOR_DATA) &&
            (status & MMC_STATUS_CURR_STATE) != MMC_STATE_PRG)
            break;

        if (status & MMC_STATUS_MASK) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
            HAL_SDMMC_ERROR(0, "Status Error: 0x%08x", status);
#endif
            return HAL_SDMMC_COMM_ERR;/* Communication error on send */
        }

        if (timeout_ms-- <= 0)
            break;

        hal_sdmmc_delay_us(1000);
    }

    if (timeout_ms <= 0) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
        HAL_SDMMC_ERROR(0, "Timeout waiting card ready");
#endif
        return HAL_SDMMC_COMM_TIMEOUT;
    }

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_set_blocklen(struct mmc *mmc, int len)
{
    struct mmc_cmd cmd;

    if (mmc->ddr_mode)
        return HAL_SDMMC_ERR_NONE;

    cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = len;

    return mmc_send_cmd_quirks(mmc, &cmd, NULL,
                               MMC_QUIRK_RETRY_SET_BLOCKLEN, 4);
}

#ifdef MMC_SUPPORTS_TUNING
static const uint8_t tuning_blk_pattern_4bit[] = {
    0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
    0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
    0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
    0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
    0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
    0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
    0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
    0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const uint8_t tuning_blk_pattern_8bit[] = {
    0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
    0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
    0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
    0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
    0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
    0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
    0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
    0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
    0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
    0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
    0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
    0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
    0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
    0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
    0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
    0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

int mmc_send_tuning(struct mmc *mmc, uint32_t opcode, int *cmd_error)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    const uint8_t *tuning_block_pattern;
    int size, err;

    if (mmc->bus_width == 8) {
        tuning_block_pattern = tuning_blk_pattern_8bit;
        size = sizeof(tuning_blk_pattern_8bit);
    } else if (mmc->bus_width == 4) {
        tuning_block_pattern = tuning_blk_pattern_4bit;
        size = sizeof(tuning_blk_pattern_4bit);
    } else {
        return HAL_SDMMC_INVALID_PARAMETER;
    }
#ifdef STATIC_VARIABLES
    struct sdmmc_ip_host *host = mmc->priv;
    uint8_t *data_buf = (uint8_t *)noncache_buf512[host->host_id];
#else
    ALLOC_CACHE_ALIGN_BUFFER(uint8_t, data_buf, size);
#endif

    cmd.cmdidx = opcode;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_R1;

    data.dest = (void *)data_buf;
    data.blocks = 1;
    data.blocksize = size;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd(mmc, &cmd, &data);
    if (err)
        return err;

    if (memcmp(data_buf, tuning_block_pattern, size))
        return HAL_SDMMC_IO_ERR;

    return HAL_SDMMC_ERR_NONE;
}
#endif

static int mmc_read_blocks(struct mmc *mmc, void *dst, lbaint_t start,
                           lbaint_t blkcnt)
{
    struct mmc_cmd cmd;
    struct mmc_data data;

    if (blkcnt > 1)
        cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
    else
        cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

    if (mmc->high_capacity)
        cmd.cmdarg = start;
    else
        cmd.cmdarg = start * mmc->read_bl_len;

    cmd.resp_type = MMC_RSP_R1;

    data.dest = dst;
    data.blocks = blkcnt;
    data.blocksize = mmc->read_bl_len;
    data.flags = MMC_DATA_READ;

    if (mmc_send_cmd(mmc, &cmd, &data)) {
        /*
            After the communication is abnormal, the STOP is forced to
            enable the card to enter the TRAN state
        */
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        mmc_send_cmd(mmc, &cmd, NULL);
        HAL_SDMMC_ERROR(0, "mmc read blocks failed");
        return 0;
    }

    if (blkcnt > 1) {
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        if (mmc_send_cmd(mmc, &cmd, NULL)) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
            HAL_SDMMC_ERROR(0, "mmc fail to send stop cmd");
#endif
            return 0;
        }
    }

    return blkcnt;
}

#ifndef DM_MMC
static int mmc_get_b_max(struct mmc *mmc, void *dst, lbaint_t blkcnt)
{
    if (mmc->cfg->ops->get_b_max)
        return mmc->cfg->ops->get_b_max(mmc, dst, blkcnt);
    else
        return mmc->cfg->b_max;
}
#endif

#ifndef MMC_TINY
static int blk_dselect_hwpart(uint32_t dev_num, uint32_t hwpart)
{
    struct mmc *mmc = find_mmc_device(dev_num);
    int ret;

    if (!mmc)
        return HAL_SDMMC_NO_SUCH_DEVICE;

    if (mmc->block_dev.hwpart == hwpart)
        return HAL_SDMMC_ERR_NONE;

    if (mmc->part_config == MMCPART_NOAVAILABLE) {
        HAL_SDMMC_ERROR(0, "Card doesn't support part_switch");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    ret = mmc_switch_part(mmc, hwpart);
    if (ret)
        return ret;

    return HAL_SDMMC_ERR_NONE;
}
#endif

#ifdef BLK
lbaint_t mmc_bread(struct udevice *dev, lbaint_t start, lbaint_t blkcnt, void *dst)
#else
lbaint_t mmc_bread(struct blk_desc *block_dev, lbaint_t start, lbaint_t blkcnt,
                   void *dst)
#endif
{
#ifdef BLK
    struct blk_desc *block_dev = dev_get_uclass_plat(dev);
#endif
    uint32_t dev_num = block_dev->devnum;
    int err;
    lbaint_t cur, blocks_todo = blkcnt;
    uint32_t b_max;

    if (blkcnt == 0)
        return HAL_SDMMC_BLK_CNT_ERR;

    struct mmc *mmc = find_mmc_device(dev_num);
    if (!mmc)
        return HAL_SDMMC_NO_DEVICE;

#ifdef MMC_TINY
    err = mmc_switch_part(mmc, block_dev->hwpart);
#else
    err = blk_dselect_hwpart(dev_num, block_dev->hwpart);
#endif
    if (err)
        return HAL_SDMMC_SWITCH_PART_ERR;

    if ((start + blkcnt) > block_dev->lba) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
        HAL_SDMMC_ERROR(0, "MMC: block number 0x" LBAF " exceeds max(0x" LBAF ")",
                        start + blkcnt, block_dev->lba);
#endif
        return HAL_SDMMC_EXCEED_MAX_CAPACITY;
    }

    if (mmc_set_blocklen(mmc, mmc->read_bl_len)) {
        HAL_SDMMC_ERROR(0, "%s: Failed to set blocklen", __func__);
        return HAL_SDMMC_SET_BLK_LEN_ERR;
    }

    b_max = mmc_get_b_max(mmc, dst, blkcnt);

    do {
        cur = (blocks_todo > b_max) ? b_max : blocks_todo;
        if (mmc_read_blocks(mmc, dst, start, cur) != cur) {
            HAL_SDMMC_ERROR(0, "%s: Failed to read blocks", __func__);
            return HAL_SDMMC_READ_BLK_ERR;
        }
        blocks_todo -= cur;
        start += cur;
        dst += cur * mmc->read_bl_len;
    } while (blocks_todo > 0);

    return blkcnt;
}

static int mmc_erase_t(struct mmc *mmc, lbaint_t start, lbaint_t blkcnt, uint32_t args)
{
    struct mmc_cmd cmd;
    lbaint_t end;
    int err, start_cmd, end_cmd;

    if (mmc->high_capacity) {
        end = start + blkcnt - 1;
    } else {
        end = (start + blkcnt - 1) * mmc->write_bl_len;
        start *= mmc->write_bl_len;
    }

    if (IS_SD(mmc)) {
        start_cmd = SD_CMD_ERASE_WR_BLK_START;
        end_cmd = SD_CMD_ERASE_WR_BLK_END;
    } else {
        start_cmd = MMC_CMD_ERASE_GROUP_START;
        end_cmd = MMC_CMD_ERASE_GROUP_END;
    }

    cmd.cmdidx = start_cmd;
    cmd.cmdarg = start;
    cmd.resp_type = MMC_RSP_R1;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        goto err_out;

    cmd.cmdidx = end_cmd;
    cmd.cmdarg = end;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        goto err_out;

    cmd.cmdidx = MMC_CMD_ERASE;
    cmd.cmdarg = args ? args : MMC_ERASE_ARG;
    cmd.resp_type = MMC_RSP_R1b;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        goto err_out;

    return HAL_SDMMC_ERR_NONE;

err_out:
    HAL_SDMMC_ERROR(0, "mmc erase failed");
    return err;
}

#ifdef BLK
    lbaint_t mmc_berase(struct udevice *dev, lbaint_t start, lbaint_t blkcnt)
#else
    lbaint_t mmc_berase(struct blk_desc *block_dev, lbaint_t start, lbaint_t blkcnt)
#endif
{
#ifdef BLK
    struct blk_desc *block_dev = dev_get_uclass_plat(dev);
#endif
    uint32_t dev_num = block_dev->devnum;
    int err = 0;
    uint32_t start_rem, blkcnt_rem, erase_args = 0;;
    struct mmc *mmc = find_mmc_device(dev_num);
    lbaint_t blk = 0, blk_r = 0;
    uint32_t timeout_ms = 1000;

    if (!mmc)
        return HAL_SDMMC_NO_SUCH_DEVICE;

    err = blk_dselect_hwpart(dev_num, block_dev->hwpart);
    if (err)
        return err;

    /*
     * We want to see if the requested start or total block count are
     * unaligned.  We discard the whole numbers and only care about the
     * remainder.
     */
    err = div_u64_rem(start, mmc->erase_grp_size, &start_rem);
    err = div_u64_rem(blkcnt, mmc->erase_grp_size, &blkcnt_rem);
    if (start_rem || blkcnt_rem) {
        if (mmc->can_trim) {
            /* Trim function applies the erase operation to write
            * blocks instead of erase groups.
            */
            erase_args = MMC_TRIM_ARG;
        } else {
            /* The card ignores all LSB's below the erase group
            * size, rounding down the addess to a erase group
            * boundary.
            */
            HAL_SDMMC_TRACE(0, "\n\nCaution! Your devices Erase group is 0x%x\n"
                            "The erase range would be change to "
                            "0x" LBAF "~0x" LBAF "\n\n",
                            mmc->erase_grp_size, start & ~(mmc->erase_grp_size - 1),
                            ((start + blkcnt + mmc->erase_grp_size - 1)
                             & ~(mmc->erase_grp_size - 1)) - 1);
        }
    }

    while (blk < blkcnt) {
        if (IS_SD(mmc) && mmc->ssr.au) {
            blk_r = ((blkcnt - blk) > mmc->ssr.au) ?
                    mmc->ssr.au : (blkcnt - blk);
        } else {
            blk_r = ((blkcnt - blk) > mmc->erase_grp_size) ?
                    mmc->erase_grp_size : (blkcnt - blk);
        }
        err = mmc_erase_t(mmc, start + blk, blk_r, erase_args);
        if (err)
            break;

        blk += blk_r;

        /* Waiting for the ready status */
        if (mmc_poll_for_busy(mmc, timeout_ms))
            return HAL_SDMMC_ERASE_TIMEOUT;
    }

    return blk;
}

static lbaint_t mmc_write_blocks(struct mmc *mmc, lbaint_t start,
                                 lbaint_t blkcnt, const void *src)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    uint32_t timeout_ms = 1000;

    if ((start + blkcnt) > mmc_get_blk_desc(mmc)->lba) {
        HAL_SDMMC_ERROR(0, "MMC: block number 0x" LBAF " exceeds max(0x" LBAF ")",
                        start + blkcnt, mmc_get_blk_desc(mmc)->lba);
        return 0;
    }

    /*
    //Put on the previous floor
    if (blkcnt == 0)
        return 0;
    else */
    if (blkcnt == 1)
        cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;
    else
        cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;

    if (mmc->high_capacity)
        cmd.cmdarg = start;
    else
        cmd.cmdarg = start * mmc->write_bl_len;

    cmd.resp_type = MMC_RSP_R1;

    data.src = src;
    data.blocks = blkcnt;
    data.blocksize = mmc->write_bl_len;
    data.flags = MMC_DATA_WRITE;

    if (mmc_send_cmd(mmc, &cmd, &data)) {
        /*
            After the communication is abnormal, the STOP is forced to
            enable the card to enter the TRAN state
        */
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        mmc_send_cmd(mmc, &cmd, NULL);
        HAL_SDMMC_ERROR(0, "mmc write failed");
        return 0;
    }

    /* SPI multiblock writes terminate using a special
     * token, not a STOP_TRANSMISSION request.
     */
    if (!mmc_host_is_spi(mmc) && blkcnt > 1) {
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        if (mmc_send_cmd(mmc, &cmd, NULL)) {
            HAL_SDMMC_ERROR(0, "mmc fail to send stop cmd");
            return 0;
        }
    }

    /* Waiting for the ready status */
    if (mmc_poll_for_busy(mmc, timeout_ms))
        return 0;

    return blkcnt;
}

#ifdef BLK
lbaint_t mmc_bwrite(struct udevice *dev, lbaint_t start, lbaint_t blkcnt,
                    const void *src)
#else
lbaint_t mmc_bwrite(struct blk_desc *block_dev, lbaint_t start, lbaint_t blkcnt,
                    const void *src)
#endif
{
#ifdef BLK
    struct blk_desc *block_dev = dev_get_uclass_plat(dev);
#endif
    uint32_t dev_num = block_dev->devnum;
    lbaint_t cur, blocks_todo = blkcnt;
    int err;

    if (blkcnt == 0)
        return HAL_SDMMC_BLK_CNT_ERR;

    struct mmc *mmc = find_mmc_device(dev_num);
    if (!mmc)
        return HAL_SDMMC_NO_DEVICE;

    err = blk_dselect_hwpart(dev_num, block_dev->hwpart);
    if (err)
        return HAL_SDMMC_SWITCH_PART_ERR;

    if (mmc_set_blocklen(mmc, mmc->write_bl_len)) {
        HAL_SDMMC_ERROR(0, "%s: Failed to set blocklen", __func__);
        return HAL_SDMMC_SET_BLK_LEN_ERR;
    }

    do {
        cur = (blocks_todo > mmc->cfg->b_max) ?
              mmc->cfg->b_max : blocks_todo;
        if (mmc_write_blocks(mmc, start, cur, src) != cur) {
            HAL_SDMMC_ERROR(0, "%s: Failed to write blocks", __func__);
            return HAL_SDMMC_WRITE_BLK_ERR;
        }
        blocks_todo -= cur;
        start += cur;
        src += cur * mmc->write_bl_len;
    } while (blocks_todo > 0);

    return blkcnt;
}

static int mmc_go_idle(struct mmc *mmc)
{
    struct mmc_cmd cmd;
    int err;

    hal_sdmmc_delay_us(1000);
    cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_NONE;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    hal_sdmmc_delay_us(2000);

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_set_ios_clock(struct mmc *mmc)
{
    int ret = HAL_SDMMC_ERR_NONE;

    if (mmc->cfg->ops->set_ios_clock)
        ret = mmc->cfg->ops->set_ios_clock(mmc);

    return ret;
}

static int mmc_set_ios_width(struct mmc *mmc)
{
    int ret = HAL_SDMMC_ERR_NONE;

    if (mmc->cfg->ops->set_ios_width)
        ret = mmc->cfg->ops->set_ios_width(mmc);

    return ret;
}

int hal_sdmmc_set_clock(struct mmc *mmc, uint32_t clock, bool disable)
{
    if (!disable) {
        if (clock > mmc->cfg->f_max)
            clock = mmc->cfg->f_max;

        if (clock < mmc->cfg->f_min)
            clock = mmc->cfg->f_min;
    }

    mmc->clock = clock;
    mmc->clk_disable = disable;
    HAL_SDMMC_TRACE(0, "%s, clock is %s (%dHz)", __func__, disable ? "disabled" : "enabled", clock);

    return mmc_set_ios_clock(mmc);
}

#ifdef MMC_UHS_SUPPORT
static int mmc_switch_voltage(struct mmc *mmc, int signal_voltage)
{
    uint32_t val;
    struct mmc_cmd cmd;
    int err = HAL_SDMMC_ERR_NONE;
    struct sdmmc_ip_host *host = (struct sdmmc_ip_host *)mmc->priv;

    /*
     * Send CMD11 only if the request is to switch the card to
     * 1.8V signalling.
     */
    if (signal_voltage == MMC_SIGNAL_VOLTAGE_330)
        return mmc_set_signal_voltage(mmc, signal_voltage);

    //stop low power clk function
    HAL_SDMMC_TRACE(0, "---disable clk low power mode");
    val = sdmmc_ip_readl(host, SDMMCIP_REG_CLKENA);
    val &= ~SDMMCIP_REG_CLKEN_LOW_PWR;
    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, val);
    err = sdmmc_ip_wait_cmd_start(host, 0);
    if (err) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
        goto errout;
    }

    cmd.cmdidx = SD_CMD_SWITCH_UHS18V;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_R1;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
        goto errout;
    }

    if (!mmc_host_is_spi(mmc) && (cmd.response[0] & MMC_STATUS_ERROR)) {
        err = HAL_SDMMC_IO_ERR;
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
        goto errout;
    }

    /*
     * The card should drive cmd and dat[0:3] low immediately
     * after the response of cmd11, but wait 100 us to be sure
     */
    err = mmc_wait_dat0(mmc, 0, 100);
    if (err == HAL_SDMMC_INVALID_SYS_CALL) {
        hal_sdmmc_delay_us(100);
    } else if (err) {
        err = HAL_SDMMC_COMM_TIMEOUT;
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
        goto errout;
    }

    /*
     * During a signal voltage level switch, the clock must be gated
     * for 5 ms according to the SD spec
     */
    hal_sdmmc_set_clock(mmc, mmc->clock, MMC_CLK_DISABLE);
    err = mmc_set_signal_voltage(mmc, signal_voltage);
    if (err) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
        goto errout;
    }

    /* Keep clock gated for at least 10 ms, though spec only says 5 ms */
    hal_sdmmc_delay_ms(10);
    hal_sdmmc_set_clock(mmc, mmc->clock, MMC_CLK_ENABLE);

    /*
     * Failure to switch is indicated by the card holding
     * dat[0:3] low. Wait for at least 1 ms according to spec
     */
    err = mmc_wait_dat0(mmc, 1, 1000);
    if (err == HAL_SDMMC_INVALID_SYS_CALL)
        hal_sdmmc_delay_us(1000);
    else if (err) {
        err = HAL_SDMMC_COMM_TIMEOUT;
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
        goto errout;
    }

errout:
    host->volt_switch_flag = 0;
    sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_ALL); //clear interrupt status
    HAL_SDMMC_ERROR(0, "---volt switch %s", err == 0 ? "success" : "failed");

    //enable low power clk
    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, SDMMCIP_REG_CLKEN_ENABLE |
                    SDMMCIP_REG_CLKEN_LOW_PWR);
    err = sdmmc_ip_wait_cmd_start(host, 0);
    if (err) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, err);
    }
    return err;
}
#endif

static int sd_send_op_cond(struct mmc *mmc, bool uhs_en)
{
    int32_t timeout = 1000;
    int err;
    struct mmc_cmd cmd;

    while (1) {
        cmd.cmdidx = MMC_CMD_APP_CMD;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = 0;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
        cmd.resp_type = MMC_RSP_R3;

        /*
         * Most cards do not answer if some reserved bits
         * in the ocr are set. However, Some controller
         * can set bit 7 (reserved for low voltages), but
         * how to manage low voltages SD card is not yet
         * specified.
         */
        cmd.cmdarg = mmc_host_is_spi(mmc) ? 0 :
                     (mmc->cfg->voltages & 0xff8000);

        if (mmc->version == SD_VERSION_2)
            cmd.cmdarg |= OCR_HCS;

        if (uhs_en)
            cmd.cmdarg |= OCR_S18R;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        if (cmd.response[0] & OCR_BUSY)
            break;

        if (timeout-- <= 0)
            return HAL_SDMMC_OP_NOT_SUPPORTED_EP;

        //hal_sdmmc_delay_us(1000);
        hal_sdmmc_delay_ms(1);
    }

    if (mmc->version != SD_VERSION_2)
        mmc->version = SD_VERSION_1_0;

    if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
        cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = 0;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;
    }

    mmc->ocr = cmd.response[0];

#ifdef MMC_UHS_SUPPORT
    if (uhs_en && !(mmc_host_is_spi(mmc)) && (cmd.response[0] & 0x41000000) == 0x41000000) {//bit24=S18A=1,bit30=CCS=1
        err = mmc_switch_voltage(mmc, MMC_SIGNAL_VOLTAGE_180);
        if (err)
            return err;
    }
#endif

    mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
    mmc->rca = 0;

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_send_op_cond_iter(struct mmc *mmc, int use_arg)
{
    struct mmc_cmd cmd;
    int err;

    cmd.cmdidx = MMC_CMD_SEND_OP_COND;
    cmd.resp_type = MMC_RSP_R3;
    cmd.cmdarg = 0;
    if (use_arg && !mmc_host_is_spi(mmc))
        cmd.cmdarg = OCR_HCS |
                     (mmc->cfg->voltages &
                      (mmc->ocr & OCR_VOLTAGE_MASK)) |
                     (mmc->ocr & OCR_ACCESS_MODE);

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        return err;
    mmc->ocr = cmd.response[0];
    return HAL_SDMMC_ERR_NONE;
}

static int mmc_send_op_cond(struct mmc *mmc)
{
    int err, i;
    uint32_t timeout = MS_TO_FAST_TICKS(1000);
    uint32_t start;

    /* Some cards seem to need this */
    mmc_go_idle(mmc);

    start = hal_fast_sys_timer_get();
    /* Asking to the card its capabilities */
    for (i = 0; ; i++) {
        err = mmc_send_op_cond_iter(mmc, i != 0);
        if (err)
            return err;

        /* exit if not busy (flag seems to be inverted) */
        if (mmc->ocr & OCR_BUSY)
            break;

        if (hal_fast_sys_timer_get() - start > timeout)
            return HAL_SDMMC_COMM_TIMEOUT;
        hal_sdmmc_delay_us(100);
    }
    mmc->op_cond_pending = 1;
    return HAL_SDMMC_ERR_NONE;
}

static int mmc_complete_op_cond(struct mmc *mmc)
{
    struct mmc_cmd cmd;
    uint32_t timeout = MS_TO_FAST_TICKS(1000);
    uint32_t start;
    int err;

    mmc->op_cond_pending = 0;
    if (!(mmc->ocr & OCR_BUSY)) {
        /* Some cards seem to need this */
        mmc_go_idle(mmc);

        start = hal_fast_sys_timer_get();
        while (1) {
            err = mmc_send_op_cond_iter(mmc, 1);
            if (err)
                return err;
            if (mmc->ocr & OCR_BUSY)
                break;
            if (hal_fast_sys_timer_get() - start > timeout)
                return HAL_SDMMC_OP_NOT_SUPPORTED_EP;
            hal_sdmmc_delay_us(100);
        }
    }

    if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
        cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = 0;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        mmc->ocr = cmd.response[0];
    }

    mmc->version = MMC_VERSION_UNKNOWN;

    mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
    mmc->rca = 1;

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_send_ext_csd(struct mmc *mmc, uint8_t *ext_csd)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    int err;

    /* Get the Card Status Register */
    cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;

    data.dest = (char *)ext_csd;
    data.blocks = 1;
    data.blocksize = MMC_MAX_BLOCK_LEN;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd(mmc, &cmd, &data);

    return err;
}

static int __mmc_switch(struct mmc *mmc, uint8_t set, uint8_t index, uint8_t value,
                        bool send_status)
{
    uint32_t status, start;
    struct mmc_cmd cmd;
    int timeout_ms = DEFAULT_CMD6_TIMEOUT_MS;
    bool is_part_switch = (set == EXT_CSD_CMD_SET_NORMAL) &&
                          (index == EXT_CSD_PART_CONF);
    int ret;

    if (mmc->gen_cmd6_time)
        timeout_ms = mmc->gen_cmd6_time * 10;

    if (is_part_switch  && mmc->part_switch_time)
        timeout_ms = mmc->part_switch_time * 10;

    cmd.cmdidx = MMC_CMD_SWITCH;
    cmd.resp_type = MMC_RSP_R1b;
    cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                 (index << 16) |
                 (value << 8);

    ret = mmc_send_cmd_retry(mmc, &cmd, NULL, 3);
    if (ret)
        return ret;

    start = hal_fast_sys_timer_get();

    /* poll dat0 for rdy/buys status */
    ret = mmc_wait_dat0(mmc, 1, timeout_ms * 1000);
    if (ret && ret != HAL_SDMMC_INVALID_SYS_CALL)
        return ret;

    /*
     * In cases when neiter allowed to poll by using CMD13 nor we are
     * capable of polling by using mmc_wait_dat0, then rely on waiting the
     * stated timeout to be sufficient.
     */
    if (ret == HAL_SDMMC_INVALID_SYS_CALL && !send_status) {
        hal_sdmmc_delay_ms(timeout_ms);
        return HAL_SDMMC_ERR_NONE;
    }

    if (!send_status)
        return HAL_SDMMC_ERR_NONE;

    /* Finally wait until the card is ready or indicates a failure
     * to switch. It doesn't hurt to use CMD13 here even if send_status
     * is false, because by now (after 'timeout_ms' ms) the bus should be
     * reliable.
     */
    do {
        ret = mmc_send_status(mmc, &status);

        if (!ret && (status & MMC_STATUS_SWITCH_ERROR)) {
            HAL_SDMMC_ERROR(0, "switch failed %d/%d/0x%x !", set, index, value);
            return HAL_SDMMC_IO_ERR;
        }
        if (!ret && (status & MMC_STATUS_RDY_FOR_DATA) &&
            (status & MMC_STATUS_CURR_STATE) == MMC_STATE_TRANS)
            return HAL_SDMMC_ERR_NONE;
        hal_sdmmc_delay_us(100);
    } while (hal_fast_sys_timer_get() - start < MS_TO_FAST_TICKS(timeout_ms));

    return HAL_SDMMC_COMM_TIMEOUT;
}

static int mmc_switch(struct mmc *mmc, uint8_t set, uint8_t index, uint8_t value)
{
    return __mmc_switch(mmc, set, index, value, true);
}

int mmc_boot_wp(struct mmc *mmc)
{
    return mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_WP, 1);
}

#ifndef MMC_TINY
static int mmc_set_card_speed(struct mmc *mmc, enum bus_mode mode,
                              bool hsdowngrade)
{
    int err;
    uint32_t speed_bits;
    struct sdmmc_ip_host *host = mmc->priv;
    uint8_t *test_csd = (uint8_t *)sdmmc_ext_csd[host->host_id];

    switch (mode) {
        case MMC_HS:
        case MMC_HS_52:
        case MMC_DDR_52:
            speed_bits = EXT_CSD_TIMING_HS;
            break;
#ifdef MMC_HS200_SUPPORT
        case MMC_HS_200:
            speed_bits = EXT_CSD_TIMING_HS200;
            break;
#endif
#ifdef MMC_HS400_SUPPORT
        case MMC_HS_400:
            speed_bits = EXT_CSD_TIMING_HS400;
            break;
#endif
#ifdef MMC_HS400_ES_SUPPORT
        case MMC_HS_400_ES:
            speed_bits = EXT_CSD_TIMING_HS400;
            break;
#endif
        case MMC_LEGACY:
            speed_bits = EXT_CSD_TIMING_LEGACY;
            break;
        default:
            return HAL_SDMMC_INVALID_PARAMETER;
    }

    err = __mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING,
                       speed_bits, !hsdowngrade);
    if (err)
        return err;

#if defined(MMC_HS200_SUPPORT) || \
    defined(MMC_HS400_SUPPORT)
    /*
     * In case the eMMC is in HS200/HS400 mode and we are downgrading
     * to HS mode, the card clock are still running much faster than
     * the supported HS mode clock, so we can not reliably read out
     * Extended CSD. Reconfigure the controller to run at HS mode.
     */
    if (hsdowngrade) {
        hal_sdmmc_select_mode(mmc, MMC_HS);
        hal_sdmmc_set_clock(mmc, mmc_mode2freq(mmc, MMC_HS), false);
    }
#endif

    if ((mode == MMC_HS) || (mode == MMC_HS_52)) {
        /* Now check to see that it worked */
        err = mmc_send_ext_csd(mmc, test_csd);
        if (err)
            return err;

        /* No high-speed support */
        if (!test_csd[EXT_CSD_HS_TIMING])
            return HAL_SDMMC_OP_NOT_SUPPORTED;
    }

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_get_capabilities(struct mmc *mmc)
{
    uint8_t *ext_csd = mmc->ext_csd;
    char cardtype;

    mmc->card_caps = MMC_MODE_1BIT | MMC_CAP(MMC_LEGACY);

    if (mmc_host_is_spi(mmc))
        return HAL_SDMMC_ERR_NONE;

    /* Only version 4 supports high-speed */
    if (mmc->version < MMC_VERSION_4)
        return HAL_SDMMC_ERR_NONE;

    if (!ext_csd) {
        HAL_SDMMC_ERROR(0, "No ext_csd found!"); /* this should enver happen */
        return HAL_SDMMC_OP_NOT_SUPPORTED;
    }

    mmc->card_caps |= MMC_MODE_4BIT | MMC_MODE_8BIT;

    cardtype = ext_csd[EXT_CSD_CARD_TYPE];
    mmc->cardtype = cardtype;

#ifdef MMC_HS200_SUPPORT
    if (cardtype & (EXT_CSD_CARD_TYPE_HS200_1_2V |
                    EXT_CSD_CARD_TYPE_HS200_1_8V)) {
        mmc->card_caps |= MMC_MODE_HS200;
    }
#endif
#if defined(MMC_HS400_SUPPORT) || \
    defined(MMC_HS400_ES_SUPPORT)
    if (cardtype & (EXT_CSD_CARD_TYPE_HS400_1_2V |
                    EXT_CSD_CARD_TYPE_HS400_1_8V)) {
        mmc->card_caps |= MMC_MODE_HS400;
    }
#endif
    if (cardtype & EXT_CSD_CARD_TYPE_52) {
        if (cardtype & EXT_CSD_CARD_TYPE_DDR_52)
            mmc->card_caps |= MMC_MODE_DDR_52MHz;
        mmc->card_caps |= MMC_MODE_HS_52MHz;
    }
    if (cardtype & EXT_CSD_CARD_TYPE_26)
        mmc->card_caps |= MMC_MODE_HS;

#ifdef MMC_HS400_ES_SUPPORT
    if (ext_csd[EXT_CSD_STROBE_SUPPORT] &&
        (mmc->card_caps & MMC_MODE_HS400)) {
        mmc->card_caps |= MMC_MODE_HS400_ES;
    }
#endif

    return HAL_SDMMC_ERR_NONE;
}
#endif

static int mmc_set_capacity(struct mmc *mmc, int part_num)
{
    switch (part_num) {
        case 0:
            mmc->capacity = mmc->capacity_user;
            break;
        case 1:
        case 2:
            mmc->capacity = mmc->capacity_boot;
            break;
        case 3:
            mmc->capacity = mmc->capacity_rpmb;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            mmc->capacity = mmc->capacity_gp[part_num - 4];
            break;
        default:
            return HAL_SDMMC_INVALID_PARAMETER;
    }

    mmc_get_blk_desc(mmc)->lba = sdmmc_lldiv(mmc->capacity, mmc->read_bl_len);

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_switch_part(struct mmc *mmc, uint32_t part_num)
{
    int ret;
    int retry = 3;

    do {
        ret = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_PART_CONF,
                         (mmc->part_config & ~PART_ACCESS_MASK)
                         | (part_num & PART_ACCESS_MASK));
    } while (ret && retry--);

    /*
     * Set the capacity if the switch succeeded or was intended
     * to return to representing the raw device.
     */
    if ((ret == 0) || ((ret == HAL_SDMMC_NO_SUCH_DEVICE) && (part_num == 0))) {
        ret = mmc_set_capacity(mmc, part_num);
        mmc_get_blk_desc(mmc)->hwpart = part_num;
    }

    return ret;
}

#ifdef MMC_HW_PARTITIONING
int mmc_hwpart_config(struct mmc *mmc,
                      const struct mmc_hwpart_conf *conf,
                      enum mmc_hwpart_conf_mode mode)
{
    uint8_t part_attrs = 0;
    uint32_t enh_size_mult;
    uint32_t enh_start_addr;
    uint32_t gp_size_mult[4];
    uint32_t max_enh_size_mult;
    uint32_t tot_enh_size_mult = 0;
    uint8_t wr_rel_set;
    int i, pidx, err;
    struct sdmmc_ip_host *host = mmc->priv;
    uint8_t *ext_csd = (uint8_t *)sdmmc_ext_csd[host->host_id];

    if (mode < MMC_HWPART_CONF_CHECK || mode > MMC_HWPART_CONF_COMPLETE)
        return HAL_SDMMC_INVALID_PARAMETER;

    if (IS_SD(mmc) || (mmc->version < MMC_VERSION_4_41)) {
        HAL_SDMMC_ERROR(0, "eMMC >= 4.4 required for enhanced user data area");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    if (!(mmc->part_support & PART_SUPPORT)) {
        HAL_SDMMC_ERROR(0, "Card does not support partitioning");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    if (!mmc->hc_wp_grp_size) {
        HAL_SDMMC_ERROR(0, "Card does not define HC WP group size");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    /* check partition alignment and total enhanced size */
    if (conf->user.enh_size) {
        if (conf->user.enh_size % mmc->hc_wp_grp_size ||
            conf->user.enh_start % mmc->hc_wp_grp_size) {
            HAL_SDMMC_ERROR(0, "User data enhanced area not HC WP group size aligned");
            return HAL_SDMMC_INVALID_PARAMETER;
        }
        part_attrs |= EXT_CSD_ENH_USR;
        enh_size_mult = conf->user.enh_size / mmc->hc_wp_grp_size;
        if (mmc->high_capacity) {
            enh_start_addr = conf->user.enh_start;
        } else {
            enh_start_addr = (conf->user.enh_start << 9);
        }
    } else {
        enh_size_mult = 0;
        enh_start_addr = 0;
    }
    tot_enh_size_mult += enh_size_mult;

    for (pidx = 0; pidx < 4; pidx++) {
        if (conf->gp_part[pidx].size % mmc->hc_wp_grp_size) {
            HAL_SDMMC_ERROR(0, "GP%i partition not HC WP group size "
                            "aligned", pidx + 1);
            return HAL_SDMMC_INVALID_PARAMETER;
        }
        gp_size_mult[pidx] = conf->gp_part[pidx].size / mmc->hc_wp_grp_size;
        if (conf->gp_part[pidx].size && conf->gp_part[pidx].enhanced) {
            part_attrs |= EXT_CSD_ENH_GP(pidx);
            tot_enh_size_mult += gp_size_mult[pidx];
        }
    }

    if (part_attrs && !(mmc->part_support & ENHNCD_SUPPORT)) {
        HAL_SDMMC_ERROR(0, "Card does not support enhanced attribute");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

#if 0   //No need to call it multiple times
    err = mmc_send_ext_csd(mmc, ext_csd);
    if (err)
        return err;
#endif

    max_enh_size_mult =
        (ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT + 2] << 16) +
        (ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT + 1] << 8) +
        ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT];
    if (tot_enh_size_mult > max_enh_size_mult) {
        HAL_SDMMC_ERROR(0, "Total enhanced size exceeds maximum (%u > %u)",
                        tot_enh_size_mult, max_enh_size_mult);
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    /* The default value of EXT_CSD_WR_REL_SET is device
     * dependent, the values can only be changed if the
     * EXT_CSD_HS_CTRL_REL bit is set. The values can be
     * changed only once and before partitioning is completed. */
    wr_rel_set = ext_csd[EXT_CSD_WR_REL_SET];
    if (conf->user.wr_rel_change) {
        if (conf->user.wr_rel_set)
            wr_rel_set |= EXT_CSD_WR_DATA_REL_USR;
        else
            wr_rel_set &= ~EXT_CSD_WR_DATA_REL_USR;
    }
    for (pidx = 0; pidx < 4; pidx++) {
        if (conf->gp_part[pidx].wr_rel_change) {
            if (conf->gp_part[pidx].wr_rel_set)
                wr_rel_set |= EXT_CSD_WR_DATA_REL_GP(pidx);
            else
                wr_rel_set &= ~EXT_CSD_WR_DATA_REL_GP(pidx);
        }
    }

    if (wr_rel_set != ext_csd[EXT_CSD_WR_REL_SET] &&
        !(ext_csd[EXT_CSD_WR_REL_PARAM] & EXT_CSD_HS_CTRL_REL)) {
        HAL_SDMMC_ERROR(0, "Card does not support host controlled partition write "
                        "reliability settings");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    if (ext_csd[EXT_CSD_PARTITION_SETTING] &
        EXT_CSD_PARTITION_SETTING_COMPLETED) {
        HAL_SDMMC_ERROR(0, "Card already partitioned");
        return HAL_SDMMC_OP_NOT_PERMITTED;
    }

    if (mode == MMC_HWPART_CONF_CHECK)
        return HAL_SDMMC_ERR_NONE;

    /* Partitioning requires high-capacity size definitions */
    if (!(ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x01)) {
        err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_ERASE_GROUP_DEF, 1);

        if (err)
            return err;

        ext_csd[EXT_CSD_ERASE_GROUP_DEF] = 1;

#ifdef MMC_WRITE
        /* update erase group size to be high-capacity */
        mmc->erase_grp_size =
            ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 1024;
#endif
    }

    /* all OK, write the configuration */
    for (i = 0; i < 4; i++) {
        err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_ENH_START_ADDR + i,
                         (enh_start_addr >> (i * 8)) & 0xFF);
        if (err)
            return err;
    }
    for (i = 0; i < 3; i++) {
        err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_ENH_SIZE_MULT + i,
                         (enh_size_mult >> (i * 8)) & 0xFF);
        if (err)
            return err;
    }
    for (pidx = 0; pidx < 4; pidx++) {
        for (i = 0; i < 3; i++) {
            err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                             EXT_CSD_GP_SIZE_MULT + pidx * 3 + i,
                             (gp_size_mult[pidx] >> (i * 8)) & 0xFF);
            if (err)
                return err;
        }
    }
    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                     EXT_CSD_PARTITIONS_ATTRIBUTE, part_attrs);
    if (err)
        return err;

    if (mode == MMC_HWPART_CONF_SET)
        return HAL_SDMMC_ERR_NONE;

    /* The WR_REL_SET is a write-once register but shall be
     * written before setting PART_SETTING_COMPLETED. As it is
     * write-once we can only write it when completing the
     * partitioning. */
    if (wr_rel_set != ext_csd[EXT_CSD_WR_REL_SET]) {
        err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_WR_REL_SET, wr_rel_set);
        if (err)
            return err;
    }

    /* Setting PART_SETTING_COMPLETED confirms the partition
     * configuration but it only becomes effective after power
     * cycle, so we do not adjust the partition related settings
     * in the mmc struct. */
    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                     EXT_CSD_PARTITION_SETTING,
                     EXT_CSD_PARTITION_SETTING_COMPLETED);
    if (err)
        return err;

    return HAL_SDMMC_ERR_NONE;
}
#endif

#ifndef DM_MMC
static int mmc_getcd(struct mmc *mmc)
{
    int cd;

    if (mmc->cfg->ops->getcd)
        cd = mmc->cfg->ops->getcd(mmc);
    else
        cd = 1;

    return cd;
}
#endif

#ifndef MMC_TINY
static int sd_switch(struct mmc *mmc, int mode, int group, uint8_t value, uint8_t *resp)
{
    struct mmc_cmd cmd;
    struct mmc_data data;

    /* Switch the frequency */
    cmd.cmdidx = SD_CMD_SWITCH_FUNC;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = (mode << 31) | 0xffffff;
    cmd.cmdarg &= ~(0xf << (group * 4));
    cmd.cmdarg |= value << (group * 4);

    data.dest = (char *)resp;
    data.blocksize = 64;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    return mmc_send_cmd(mmc, &cmd, &data);
}

static int sd_get_capabilities(struct mmc *mmc)
{
    int err;
    int timeout;
    struct mmc_cmd cmd;
    struct mmc_data data;

#ifdef STATIC_VARIABLES
    struct sdmmc_ip_host *host = mmc->priv;
    uint32_t *scr = (uint32_t *)noncache_buf8[host->host_id];
    uint32_t *switch_status = (uint32_t *)noncache_buf512[host->host_id];
#else
    ALLOC_CACHE_ALIGN_BUFFER(uint32_t, scr, 2);
    ALLOC_CACHE_ALIGN_BUFFER(uint32_t, switch_status, 16);
#endif

#ifdef MMC_UHS_SUPPORT
    uint32_t sd3_bus_mode;
#endif

    mmc->card_caps = MMC_MODE_1BIT | MMC_CAP(MMC_LEGACY);

    if (mmc_host_is_spi(mmc))
        return HAL_SDMMC_ERR_NONE;

    /* Read the SCR to find out if this card supports higher speeds */
    cmd.cmdidx = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = mmc->rca << 16;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    cmd.cmdidx = SD_CMD_APP_SEND_SCR;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;

    data.dest = (char *)scr;
    data.blocksize = 8;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd_retry(mmc, &cmd, &data, 3);

    if (err)
        return err;

    mmc->scr[0] = be32_to_cpu(scr[0]);
    mmc->scr[1] = be32_to_cpu(scr[1]);

    switch ((mmc->scr[0] >> 24) & 0xf) {
        case 0:
            mmc->version = SD_VERSION_1_0;
            break;
        case 1:
            mmc->version = SD_VERSION_1_10;
            break;
        case 2:
            mmc->version = SD_VERSION_2;
            if ((mmc->scr[0] >> 15) & 0x1)
                mmc->version = SD_VERSION_3;
            break;
        default:
            mmc->version = SD_VERSION_1_0;
            break;
    }

    if (mmc->scr[0] & SD_DATA_4BIT)
        mmc->card_caps |= MMC_MODE_4BIT;

    /* Version 1.0 doesn't support switching */
    if (mmc->version == SD_VERSION_1_0)
        return HAL_SDMMC_ERR_NONE;

    hal_sdmmc_delay_ms(1);//Necessary delay, otherwise switch fails
    timeout = 4;
    while (timeout--) {
        err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
                        (uint8_t *)switch_status);

        if (err)
            return err;

        /* The high-speed function is busy.  Try again */
        if (!(be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
            break;
    }

    /* If high-speed isn't supported, we return */
    if (be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED)
        mmc->card_caps |= MMC_CAP(SD_HS);

#ifdef MMC_UHS_SUPPORT
    /* Version before 3.0 don't support UHS modes */
    if (mmc->version < SD_VERSION_3)
        return HAL_SDMMC_ERR_NONE;

    sd3_bus_mode = be32_to_cpu(switch_status[3]) >> 16 & 0x1f;
    if (sd3_bus_mode & SD_MODE_UHS_SDR104)
        mmc->card_caps |= MMC_CAP(UHS_SDR104);
    if (sd3_bus_mode & SD_MODE_UHS_SDR50)
        mmc->card_caps |= MMC_CAP(UHS_SDR50);
    if (sd3_bus_mode & SD_MODE_UHS_SDR25)
        mmc->card_caps |= MMC_CAP(UHS_SDR25);
    if (sd3_bus_mode & SD_MODE_UHS_SDR12)
        mmc->card_caps |= MMC_CAP(UHS_SDR12);
    if (sd3_bus_mode & SD_MODE_UHS_DDR50)
        mmc->card_caps |= MMC_CAP(UHS_DDR50);
#endif

    return HAL_SDMMC_ERR_NONE;
}

static int sd_set_card_speed(struct mmc *mmc, enum bus_mode mode)
{
    int err;
    int speed;

#ifdef STATIC_VARIABLES
    struct sdmmc_ip_host *host = mmc->priv;
    uint32_t *switch_status = (uint32_t *)noncache_buf512[host->host_id];
#else
    ALLOC_CACHE_ALIGN_BUFFER(uint32_t, switch_status, 16);
#endif

    /* SD version 1.00 and 1.01 does not support CMD 6 */
    if (mmc->version == SD_VERSION_1_0)
        return HAL_SDMMC_ERR_NONE;

    switch (mode) {
        case MMC_LEGACY:
            speed = UHS_SDR12_BUS_SPEED;
            break;
        case SD_HS:
            speed = HIGH_SPEED_BUS_SPEED;
            break;
#ifdef MMC_UHS_SUPPORT
        case UHS_SDR12:
            speed = UHS_SDR12_BUS_SPEED;
            break;
        case UHS_SDR25:
            speed = UHS_SDR25_BUS_SPEED;
            break;
        case UHS_SDR50:
            speed = UHS_SDR50_BUS_SPEED;
            break;
        case UHS_DDR50:
            speed = UHS_DDR50_BUS_SPEED;
            break;
        case UHS_SDR104:
            speed = UHS_SDR104_BUS_SPEED;
            break;
#endif
        default:
            return HAL_SDMMC_INVALID_PARAMETER;
    }

    err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, speed, (uint8_t *)switch_status);
    if (err)
        return err;

    if (((be32_to_cpu(switch_status[4]) >> 24) & 0xF) != speed)
        return HAL_SDMMC_OP_NOT_SUPPORTED;

    return HAL_SDMMC_ERR_NONE;
}

static int sd_select_bus_width(struct mmc *mmc, int w)
{
    int err;
    struct mmc_cmd cmd;

    if ((w != 4) && (w != 1))
        return HAL_SDMMC_INVALID_PARAMETER;

    cmd.cmdidx = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = mmc->rca << 16;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        return err;

    cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
    cmd.resp_type = MMC_RSP_R1;
    if (w == 4)
        cmd.cmdarg = 2;
    else if (w == 1)
        cmd.cmdarg = 0;
    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        return err;

    return HAL_SDMMC_ERR_NONE;
}
#endif

#ifdef MMC_WRITE
#define SZ_1                0x00000001
#define SZ_2                0x00000002
#define SZ_4                0x00000004
#define SZ_8                0x00000008
#define SZ_16               0x00000010
#define SZ_32               0x00000020
#define SZ_64               0x00000040
#define SZ_128              0x00000080
#define SZ_256              0x00000100
#define SZ_512              0x00000200

#define SZ_1K               0x00000400
#define SZ_2K               0x00000800
#define SZ_4K               0x00001000
#define SZ_8K               0x00002000
#define SZ_16K              0x00004000
#define SZ_32K              0x00008000
#define SZ_64K              0x00010000
#define SZ_128K             0x00020000
#define SZ_256K             0x00040000
#define SZ_512K             0x00080000

#define SZ_1M               0x00100000
#define SZ_2M               0x00200000
#define SZ_4M               0x00400000
#define SZ_8M               0x00800000
#define SZ_16M              0x01000000
#define SZ_32M              0x02000000
#define SZ_64M              0x04000000
#define SZ_128M             0x08000000
#define SZ_256M             0x10000000
#define SZ_512M             0x20000000

#define SZ_1G               0x40000000
#define SZ_2G               0x80000000

static int sd_read_ssr(struct mmc *mmc)
{
    static const uint32_t sd_au_size[] = {
        0,              SZ_16K / 512,           SZ_32K / 512,
        SZ_64K / 512,   SZ_128K / 512,          SZ_256K / 512,
        SZ_512K / 512,  SZ_1M / 512,            SZ_2M / 512,
        SZ_4M / 512,    SZ_8M / 512,            (SZ_8M + SZ_4M) / 512,
        SZ_16M / 512,   (SZ_16M + SZ_8M) / 512, SZ_32M / 512,
        SZ_64M / 512,
    };
    int err, i;
    struct mmc_cmd cmd;
    struct mmc_data data;
    uint32_t au, eo, et, es;
#ifdef STATIC_VARIABLES
    struct sdmmc_ip_host *host = mmc->priv;
    uint32_t *ssr = (uint32_t *)noncache_buf512[host->host_id];
#else
    ALLOC_CACHE_ALIGN_BUFFER(uint32_t, ssr, 16);
#endif

    cmd.cmdidx = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = mmc->rca << 16;

    err = mmc_send_cmd_quirks(mmc, &cmd, NULL, MMC_QUIRK_RETRY_APP_CMD, 4);
    if (err)
        return err;

    cmd.cmdidx = SD_CMD_APP_SD_STATUS;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;

    data.dest = (char *)ssr;
    data.blocksize = 64;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd_retry(mmc, &cmd, &data, 3);
    if (err)
        return err;

    for (i = 0; i < 16; i++)
        ssr[i] = be32_to_cpu(ssr[i]);

    au = (ssr[2] >> 12) & 0xF;
    if ((au <= 9) || (mmc->version == SD_VERSION_3)) {
        mmc->ssr.au = sd_au_size[au];
        es = (ssr[3] >> 24) & 0xFF;
        es |= (ssr[2] & 0xFF) << 8;
        et = (ssr[3] >> 18) & 0x3F;
        if (es && et) {
            eo = (ssr[3] >> 16) & 0x3;
            mmc->ssr.erase_timeout = (et * 1000) / es;
            mmc->ssr.erase_offset = eo * 1000;
        }
        err = HAL_SDMMC_ERR_NONE;
    } else {
        HAL_SDMMC_ERROR(0, "Invalid Allocation Unit Size.");
        err = HAL_SDMMC_ALLOCATION_UNIT_ERR;
    }

    return err;
}
#endif

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
    10000,
    100000,
    1000000,
    10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const uint8_t multipliers[] = {
    0,  /* reserved */
    10,
    12,
    13,
    15,
    20,
    25,
    30,
    35,
    40,
    45,
    50,
    55,
    60,
    70,
    80,
};

static int bus_width(uint32_t cap)
{
    if (cap == MMC_MODE_8BIT)
        return 8;
    if (cap == MMC_MODE_4BIT)
        return 4;
    if (cap == MMC_MODE_1BIT)
        return 1;

    HAL_SDMMC_ERROR(0, "invalid bus witdh capability 0x%x", cap);
    return 0;
}

#ifndef DM_MMC
#ifdef MMC_SUPPORTS_TUNING
static int mmc_execute_tuning(struct mmc *mmc, uint32_t opcode)
{
    return HAL_SDMMC_OP_NOT_SUPPORTED;
}
#endif

static int mmc_host_power_cycle(struct mmc *mmc)
{
    int ret = HAL_SDMMC_ERR_NONE;

    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);
    if (mmc->cfg->ops->host_power_cycle)
        ret = mmc->cfg->ops->host_power_cycle(mmc);

    return ret;
}
#endif

int hal_sdmmc_set_bus_width(struct mmc *mmc, uint32_t width)
{
    mmc->bus_width = width;
    HAL_SDMMC_TRACE(0, "%s, width is %d", __func__, width);

    return mmc_set_ios_width(mmc);
}

#ifdef SDMMC_DEBUG
static void mmc_dump_capabilities(const char *text, uint32_t caps)
{
    enum bus_mode mode;

    HAL_SDMMC_TRACE(0, "  ");
    HAL_SDMMC_TRACE(TR_ATTR_NO_LF, "%s: widths [", text);
    if (caps & MMC_MODE_8BIT)
        HAL_SDMMC_TRACE(TR_ATTR_NO_LF | TR_ATTR_NO_TS | TR_ATTR_NO_ID, "8, ");
    if (caps & MMC_MODE_4BIT)
        HAL_SDMMC_TRACE(TR_ATTR_NO_LF | TR_ATTR_NO_TS | TR_ATTR_NO_ID, "4, ");
    if (caps & MMC_MODE_1BIT)
        HAL_SDMMC_TRACE(TR_ATTR_NO_LF | TR_ATTR_NO_TS | TR_ATTR_NO_ID, "1, ");
    HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "]");
    HAL_SDMMC_TRACE(0, "modes [");
    for (mode = MMC_LEGACY; mode < MMC_MODES_END; mode++)
        if (MMC_CAP(mode) & caps)
            HAL_SDMMC_TRACE(0, "        %s, ", mmc_mode_name(mode));
    HAL_SDMMC_TRACE(0, "      ]");
}
#endif

#ifdef MMC_IO_VOLTAGE
int mmc_voltage_to_mv(enum mmc_voltage voltage)
{
    switch (voltage) {
        case MMC_SIGNAL_VOLTAGE_000:
            return 0;
        case MMC_SIGNAL_VOLTAGE_330:
            return 3300;
        case MMC_SIGNAL_VOLTAGE_180:
            return 1800;
        case MMC_SIGNAL_VOLTAGE_120:
            return 1200;
        default:
            return 0;
    }
    return HAL_SDMMC_INVALID_PARAMETER;
}

static int mmc_set_signal_voltage(struct mmc *mmc, uint32_t signal_voltage)
{
    struct sdmmc_ip_host *host = mmc->priv;

    if (mmc->signal_voltage == signal_voltage)
        return HAL_SDMMC_ERR_NONE;

    mmc->signal_voltage = signal_voltage;
    if (host->callback->hal_sdmmc_signal_voltage_switch) {
        host->callback->hal_sdmmc_signal_voltage_switch(signal_voltage);
    }
    HAL_SDMMC_TRACE(0, "%s, signal_voltage is %d", __func__, signal_voltage);

    return HAL_SDMMC_ERR_NONE;
}
#else
static int mmc_set_signal_voltage(struct mmc *mmc, uint32_t signal_voltage)
{
    return HAL_SDMMC_ERR_NONE;
}
#endif

#ifndef MMC_TINY
struct mode_width_tuning {
    enum bus_mode mode;
    uint32_t widths;
#ifdef MMC_SUPPORTS_TUNING
    uint32_t tuning;
#endif
};

static const struct mode_width_tuning sd_modes_by_pref[] = {
#ifdef MMC_UHS_SUPPORT
#ifdef MMC_SUPPORTS_TUNING
    {
        .mode = UHS_SDR104,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
        .tuning = MMC_CMD_SEND_TUNING_BLOCK
    },
#endif
    {
        .mode = UHS_SDR50,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
    {
        .mode = UHS_DDR50,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
    {
        .mode = UHS_SDR25,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
#endif
    {
        .mode = SD_HS,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
#ifdef MMC_UHS_SUPPORT
    {
        .mode = UHS_SDR12,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
#endif
    {
        .mode = MMC_LEGACY,
        .widths = MMC_MODE_4BIT | MMC_MODE_1BIT,
    }
};

#define for_each_sd_mode_by_pref(caps, mwt) \
    for (mwt = sd_modes_by_pref;\
         mwt < sd_modes_by_pref + ARRAY_SIZE(sd_modes_by_pref);\
         mwt++) \
        if (caps & MMC_CAP(mwt->mode))

static int sd_select_mode_and_width(struct mmc *mmc, uint32_t card_caps)
{
    int err;
    uint32_t widths[] = {MMC_MODE_4BIT, MMC_MODE_1BIT};
    const struct mode_width_tuning *mwt;
#ifdef MMC_UHS_SUPPORT
    bool uhs_en = (mmc->ocr & OCR_S18R) ? true : false;
#else
    bool uhs_en = false;
#endif
    uint32_t caps;

#ifdef SDMMC_DEBUG
    mmc_dump_capabilities("sd card", card_caps);
    mmc_dump_capabilities("host", mmc->host_caps);
#endif

    if (mmc_host_is_spi(mmc)) {
        hal_sdmmc_set_bus_width(mmc, 1);
        hal_sdmmc_select_mode(mmc, MMC_LEGACY);
        hal_sdmmc_set_clock(mmc, mmc->tran_speed, MMC_CLK_ENABLE);
#ifdef MMC_WRITE
        err = sd_read_ssr(mmc);
        if (err)
            HAL_SDMMC_ERROR(0, "unable to read ssr");
#endif
        return HAL_SDMMC_ERR_NONE;
    }

    /* Restrict card's capabilities by what the host can do */
    caps = card_caps & mmc->host_caps;

    if (!uhs_en)
        caps &= ~UHS_CAPS;

    for_each_sd_mode_by_pref(caps, mwt) {
        uint32_t *w;

        for (w = widths; w < widths + ARRAY_SIZE(widths); w++) {
            if (*w & caps & mwt->widths) {
                HAL_SDMMC_TRACE(0, "trying mode %s width %d (at %d MHz)",
                                mmc_mode_name(mwt->mode),
                                bus_width(*w),
                                mmc_mode2freq(mmc, mwt->mode) / 1000000);

                /* configure the bus width (card + host) */
                err = sd_select_bus_width(mmc, bus_width(*w));
                if (err)
                    goto error;
                hal_sdmmc_set_bus_width(mmc, bus_width(*w));

                /* configure the bus mode (card) */
                err = sd_set_card_speed(mmc, mwt->mode);
                if (err)
                    goto error;

                /* configure the bus mode (host) */
                hal_sdmmc_select_mode(mmc, mwt->mode);
                hal_sdmmc_set_clock(mmc, mmc->tran_speed,
                                    MMC_CLK_ENABLE);

#ifdef MMC_SUPPORTS_TUNING
                /* execute tuning if needed */
                if (mwt->tuning && !mmc_host_is_spi(mmc)) {
                    err = mmc_execute_tuning(mmc,
                                             mwt->tuning);
                    if (err) {
                        HAL_SDMMC_ERROR(0, "tuning failed");
                        goto error;
                    }
                }
#endif

#ifdef MMC_WRITE
                err = sd_read_ssr(mmc);
                if (err)
                    HAL_SDMMC_ERROR(0, "unable to read ssr");
#endif
                if (!err)
                    return HAL_SDMMC_ERR_NONE;
error:
                /* revert to a safer bus speed */
                hal_sdmmc_select_mode(mmc, MMC_LEGACY);
                hal_sdmmc_set_clock(mmc, mmc->tran_speed,
                                    MMC_CLK_ENABLE);
            }
        }
    }

    HAL_SDMMC_ERROR(0, "unable to select a mode");
    return HAL_SDMMC_OP_NOT_SUPPORTED;
}

static int mmc_read_and_compare_ext_csd(struct mmc *mmc)
{
    int err;
    const uint8_t *ext_csd = mmc->ext_csd;

#ifdef STATIC_VARIABLES
    struct sdmmc_ip_host *host = mmc->priv;
    uint8_t *test_csd = (uint8_t *)noncache_buf512[host->host_id];
#else
    ALLOC_CACHE_ALIGN_BUFFER(uint8_t, test_csd, MMC_MAX_BLOCK_LEN);
#endif
    if (mmc->version < MMC_VERSION_4)
        return HAL_SDMMC_ERR_NONE;

    err = mmc_send_ext_csd(mmc, test_csd);
    if (err)
        return err;

    /* Only compare read only fields */
    if (ext_csd[EXT_CSD_PARTITIONING_SUPPORT]
        == test_csd[EXT_CSD_PARTITIONING_SUPPORT] &&
        ext_csd[EXT_CSD_HC_WP_GRP_SIZE]
        == test_csd[EXT_CSD_HC_WP_GRP_SIZE] &&
        ext_csd[EXT_CSD_REV]
        == test_csd[EXT_CSD_REV] &&
        ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
        == test_csd[EXT_CSD_HC_ERASE_GRP_SIZE] &&
        memcmp(&ext_csd[EXT_CSD_SEC_CNT],
               &test_csd[EXT_CSD_SEC_CNT], 4) == 0)
        return HAL_SDMMC_ERR_NONE;

    return HAL_SDMMC_NOT_DATA_MSG;
}

#ifdef MMC_IO_VOLTAGE
/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int generic_ffs(int x)
{
    int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

static int mmc_set_lowest_voltage(struct mmc *mmc, enum bus_mode mode,
                                  uint32_t allowed_mask)
{
    uint32_t card_mask = 0;

    HAL_SDMMC_TRACE(0, "%s:%d, bus_mode:%d, mask:0x%X", __func__, __LINE__, (uint32_t)mode, allowed_mask);
    switch (mode) {
        case MMC_HS_400_ES:
        case MMC_HS_400:
        case MMC_HS_200:
            if (mmc->cardtype & (EXT_CSD_CARD_TYPE_HS200_1_8V |
                                 EXT_CSD_CARD_TYPE_HS400_1_8V))
                card_mask |= MMC_SIGNAL_VOLTAGE_180;
            if (mmc->cardtype & (EXT_CSD_CARD_TYPE_HS200_1_2V |
                                 EXT_CSD_CARD_TYPE_HS400_1_2V))
                card_mask |= MMC_SIGNAL_VOLTAGE_120;
            break;
        case MMC_DDR_52:
            if (mmc->cardtype & EXT_CSD_CARD_TYPE_DDR_1_8V)
                card_mask |= MMC_SIGNAL_VOLTAGE_330 |
                             MMC_SIGNAL_VOLTAGE_180;
            if (mmc->cardtype & EXT_CSD_CARD_TYPE_DDR_1_2V)
                card_mask |= MMC_SIGNAL_VOLTAGE_120;
            break;
        default:
            card_mask |= MMC_SIGNAL_VOLTAGE_330;
            break;
    }

    while (card_mask & allowed_mask) {
        enum mmc_voltage best_match;

        best_match = 1 << (generic_ffs(card_mask & allowed_mask) - 1);
        HAL_SDMMC_TRACE(0, "best match:0x%X", best_match);
        if (!mmc_set_signal_voltage(mmc,  best_match))
            return HAL_SDMMC_ERR_NONE;

        allowed_mask &= ~best_match;
    }

    return HAL_SDMMC_OP_NOT_SUPPORTED;
}
#else
static int mmc_set_lowest_voltage(struct mmc *mmc, enum bus_mode mode,
                                  uint32_t allowed_mask)
{
    return HAL_SDMMC_ERR_NONE;
}
#endif

static const struct mode_width_tuning mmc_modes_by_pref[] = {
#ifdef MMC_HS400_ES_SUPPORT
    {
        .mode = MMC_HS_400_ES,
        .widths = MMC_MODE_8BIT,
    },
#endif
#ifdef MMC_HS400_SUPPORT
    {
        .mode = MMC_HS_400,
        .widths = MMC_MODE_8BIT,
        .tuning = MMC_CMD_SEND_TUNING_BLOCK_HS200
    },
#endif
#ifdef MMC_HS200_SUPPORT
    {
        .mode = MMC_HS_200,
        .widths = MMC_MODE_8BIT | MMC_MODE_4BIT,
        .tuning = MMC_CMD_SEND_TUNING_BLOCK_HS200
    },
#endif
    {
        .mode = MMC_DDR_52,
        .widths = MMC_MODE_8BIT | MMC_MODE_4BIT,
    },
    {
        .mode = MMC_HS_52,
        .widths = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
    {
        .mode = MMC_HS,
        .widths = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_1BIT,
    },
    {
        .mode = MMC_LEGACY,
        .widths = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_1BIT,
    }
};

#define for_each_mmc_mode_by_pref(caps, mwt) \
    for (mwt = mmc_modes_by_pref;\
         mwt < mmc_modes_by_pref + ARRAY_SIZE(mmc_modes_by_pref);\
         mwt++) \
        if (caps & MMC_CAP(mwt->mode))

static const struct ext_csd_bus_width {
    uint32_t cap;
    bool is_ddr;
    uint32_t ext_csd_bits;
} ext_csd_bus_width[] = {
    {MMC_MODE_8BIT, true, EXT_CSD_DDR_BUS_WIDTH_8},
    {MMC_MODE_4BIT, true, EXT_CSD_DDR_BUS_WIDTH_4},
    {MMC_MODE_8BIT, false, EXT_CSD_BUS_WIDTH_8},
    {MMC_MODE_4BIT, false, EXT_CSD_BUS_WIDTH_4},
    {MMC_MODE_1BIT, false, EXT_CSD_BUS_WIDTH_1},
};

#ifdef MMC_HS400_SUPPORT
static int mmc_select_hs400(struct mmc *mmc)
{
    int err;

    /* Set timing to HS200 for tuning */
    err = mmc_set_card_speed(mmc, MMC_HS_200, false);
    if (err)
        return err;

    /* configure the bus mode (host) */
    hal_sdmmc_select_mode(mmc, MMC_HS_200);
    hal_sdmmc_set_clock(mmc, mmc->tran_speed, false);

    /* execute tuning if needed */
    mmc->hs400_tuning = 1;
    err = mmc_execute_tuning(mmc, MMC_CMD_SEND_TUNING_BLOCK_HS200);
    mmc->hs400_tuning = 0;
    if (err) {
        HAL_SDMMC_ERROR(0, "tuning failed");
        return err;
    }

    /* Set back to HS */
    mmc_set_card_speed(mmc, MMC_HS, true);

    err = mmc_hs400_prepare_ddr(mmc);
    if (err)
        return err;

    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
                     EXT_CSD_BUS_WIDTH_8 | EXT_CSD_DDR_FLAG);
    if (err)
        return err;

    err = mmc_set_card_speed(mmc, MMC_HS_400, false);
    if (err)
        return err;

    hal_sdmmc_select_mode(mmc, MMC_HS_400);
    err = hal_sdmmc_set_clock(mmc, mmc->tran_speed, false);
    if (err)
        return err;

    return HAL_SDMMC_ERR_NONE;
}
#else
static int mmc_select_hs400(struct mmc *mmc)
{
    return HAL_SDMMC_OP_NOT_SUPPORTED;
}
#endif

#ifdef MMC_HS400_ES_SUPPORT
#ifndef DM_MMC
static int mmc_set_enhanced_strobe(struct mmc *mmc)
{
    return HAL_SDMMC_OP_NOT_SUPPORTED;
}
#endif
static int mmc_select_hs400es(struct mmc *mmc)
{
    int err;

    err = mmc_set_card_speed(mmc, MMC_HS, true);
    if (err)
        return err;

    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
                     EXT_CSD_BUS_WIDTH_8 | EXT_CSD_DDR_FLAG |
                     EXT_CSD_BUS_WIDTH_STROBE);
    if (err) {
        HAL_SDMMC_ERROR(0, "switch to bus width for hs400 failed");
        return err;
    }
    /* TODO: driver strength */
    err = mmc_set_card_speed(mmc, MMC_HS_400_ES, false);
    if (err)
        return err;

    hal_sdmmc_select_mode(mmc, MMC_HS_400_ES);
    err = hal_sdmmc_set_clock(mmc, mmc->tran_speed, false);
    if (err)
        return err;

    return mmc_set_enhanced_strobe(mmc);
}
#else
static int mmc_select_hs400es(struct mmc *mmc)
{
    return HAL_SDMMC_OP_NOT_SUPPORTED;
}
#endif

#define for_each_supported_width(caps, ddr, ecbv) \
    for (ecbv = ext_csd_bus_width;\
         ecbv < ext_csd_bus_width + ARRAY_SIZE(ext_csd_bus_width);\
         ecbv++) \
        if ((ddr == ecbv->is_ddr) && (caps & ecbv->cap))

static int mmc_select_mode_and_width(struct mmc *mmc, uint32_t card_caps)
{
    int err = 0;
    const struct mode_width_tuning *mwt;
    const struct ext_csd_bus_width *ecbw;

#ifdef SDMMC_DEBUG
    mmc_dump_capabilities("mmc", card_caps);
    mmc_dump_capabilities("host", mmc->host_caps);
#endif

    if (mmc_host_is_spi(mmc)) {
        hal_sdmmc_set_bus_width(mmc, 1);
        hal_sdmmc_select_mode(mmc, MMC_LEGACY);
        hal_sdmmc_set_clock(mmc, mmc->tran_speed, MMC_CLK_ENABLE);
        return HAL_SDMMC_ERR_NONE;
    }

    /* Restrict card's capabilities by what the host can do */
    card_caps &= mmc->host_caps;

    /* Only version 4 of MMC supports wider bus widths */
    if (mmc->version < MMC_VERSION_4)
        return HAL_SDMMC_ERR_NONE;

    if (!mmc->ext_csd) {
        HAL_SDMMC_ERROR(0, "No ext_csd found!"); /* this should enver happen */
        return HAL_SDMMC_OP_NOT_SUPPORTED;
    }

#if defined(MMC_HS200_SUPPORT) || \
    defined(MMC_HS400_SUPPORT) || \
    defined(MMC_HS400_ES_SUPPORT)
    /*
     * In case the eMMC is in HS200/HS400 mode, downgrade to HS mode
     * before doing anything else, since a transition from either of
     * the HS200/HS400 mode directly to legacy mode is not supported.
     */
    if (mmc->selected_mode == MMC_HS_200 ||
        mmc->selected_mode == MMC_HS_400 ||
        mmc->selected_mode == MMC_HS_400_ES)
        mmc_set_card_speed(mmc, MMC_HS, true);
    else
#endif
        hal_sdmmc_set_clock(mmc, mmc->legacy_speed, MMC_CLK_ENABLE);

    for_each_mmc_mode_by_pref(card_caps, mwt) {
        for_each_supported_width(card_caps & mwt->widths,
                                 mmc_is_mode_ddr(mwt->mode), ecbw) {
            enum mmc_voltage old_voltage;
            HAL_SDMMC_TRACE(0, "trying mode %s width %d (at %d MHz)",
                            mmc_mode_name(mwt->mode),
                            bus_width(ecbw->cap),
                            mmc_mode2freq(mmc, mwt->mode) / 1000000);
            old_voltage = mmc->signal_voltage;
            err = mmc_set_lowest_voltage(mmc, mwt->mode,
                                         MMC_ALL_SIGNAL_VOLTAGE);
            if (err)
                continue;

            /* configure the bus width (card + host) */
            err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                             EXT_CSD_BUS_WIDTH,
                             ecbw->ext_csd_bits & ~EXT_CSD_DDR_FLAG);
            if (err)
                goto error;
            hal_sdmmc_set_bus_width(mmc, bus_width(ecbw->cap));

            if (mwt->mode == MMC_HS_400) {
                err = mmc_select_hs400(mmc);
                if (err) {
                    HAL_SDMMC_ERROR(0, "Select HS400 failed %d", err);
                    goto error;
                }
            } else if (mwt->mode == MMC_HS_400_ES) {
                err = mmc_select_hs400es(mmc);
                if (err) {
                    HAL_SDMMC_ERROR(0, "Select HS400ES failed %d",
                                    err);
                    goto error;
                }
            } else {
                /* configure the bus speed (card) */
                err = mmc_set_card_speed(mmc, mwt->mode, false);
                if (err)
                    goto error;

                /*
                 * configure the bus width AND the ddr mode
                 * (card). The host side will be taken care
                 * of in the next step
                 */
                if (ecbw->ext_csd_bits & EXT_CSD_DDR_FLAG) {
                    err = mmc_switch(mmc,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     ecbw->ext_csd_bits);
                    if (err)
                        goto error;
                }

                /* configure the bus mode (host) */
                hal_sdmmc_select_mode(mmc, mwt->mode);
                hal_sdmmc_set_clock(mmc, mmc->tran_speed,
                                    MMC_CLK_ENABLE);
#ifdef MMC_SUPPORTS_TUNING
                /* execute tuning if needed */
                if (mwt->tuning) {
                    err = mmc_execute_tuning(mmc,
                                             mwt->tuning);
                    if (err) {
                        HAL_SDMMC_ERROR(0, "tuning failed : %d", err);
                        goto error;
                    }
                }
#endif
            }

            /* do a transfer to check the configuration */
            err = mmc_read_and_compare_ext_csd(mmc);
            if (!err)
                return HAL_SDMMC_ERR_NONE;
error:
            mmc_set_signal_voltage(mmc, old_voltage);
            /* if an error occurred, revert to a safer bus mode */
            mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                       EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_1);
            hal_sdmmc_select_mode(mmc, MMC_LEGACY);
            hal_sdmmc_set_bus_width(mmc, 1);
        }
    }

    HAL_SDMMC_ERROR(0, "unable to select a mode : %d", err);

    return HAL_SDMMC_OP_NOT_SUPPORTED;
}
#endif

#ifdef MMC_TINY
#ifdef STATIC_VARIABLES
SYNC_FLAGS_LOC static uint32_t ext_csd_bkup[MMC_MAX_BLOCK_LEN / 4];
#else
DEFINE_CACHE_ALIGN_BUFFER(uint8_t, ext_csd_bkup, MMC_MAX_BLOCK_LEN);
#endif
#endif
static int mmc_startup_v4(struct mmc *mmc)
{
    int err, i;
    uint64_t capacity;
    bool has_parts = false;
    bool part_completed;
    static const uint32_t mmc_versions[] = {
        MMC_VERSION_4,
        MMC_VERSION_4_1,
        MMC_VERSION_4_2,
        MMC_VERSION_4_3,
        MMC_VERSION_4_4,
        MMC_VERSION_4_41,
        MMC_VERSION_4_5,
        MMC_VERSION_5_0,
        MMC_VERSION_5_1
    };

#ifdef MMC_TINY
    uint8_t *ext_csd = (uint8_t *)ext_csd_bkup;

    if (IS_SD(mmc) || mmc->version < MMC_VERSION_4)
        return HAL_SDMMC_ERR_NONE;

    if (!mmc->ext_csd)
        memset(ext_csd_bkup, 0, sizeof(ext_csd_bkup) / sizeof(ext_csd_bkup[0]));

    err = mmc_send_ext_csd(mmc, ext_csd);
    if (err)
        goto error;

    /* store the ext csd for future reference */
    if (!mmc->ext_csd)
        mmc->ext_csd = ext_csd;
#else
    struct sdmmc_ip_host *host = mmc->priv;
    uint8_t *ext_csd = (uint8_t *)sdmmc_ext_csd[host->host_id];

    if (IS_SD(mmc) || mmc->version < MMC_VERSION_4)
        return HAL_SDMMC_ERR_NONE;

    /* check ext_csd version and capacity */
    err = mmc_send_ext_csd(mmc, ext_csd);
    if (err)
        goto error;

    /* store the ext csd for future reference */
    if (!mmc->ext_csd)
        mmc->ext_csd = ext_csd;
#endif
    if (ext_csd[EXT_CSD_REV] >= ARRAY_SIZE(mmc_versions))
        return HAL_SDMMC_INVALID_PARAMETER;

    mmc->version = mmc_versions[ext_csd[EXT_CSD_REV]];

    if (mmc->version >= MMC_VERSION_4_2) {
        /*
         * According to the JEDEC Standard, the value of
         * ext_csd's capacity is valid if the value is more
         * than 2GB
         */
        capacity = ext_csd[EXT_CSD_SEC_CNT] << 0
                   | ext_csd[EXT_CSD_SEC_CNT + 1] << 8
                   | ext_csd[EXT_CSD_SEC_CNT + 2] << 16
                   | ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
        capacity *= MMC_MAX_BLOCK_LEN;
        if ((capacity >> 20) > 2 * 1024)
            mmc->capacity_user = capacity;
    }

    //emmc cache
    mmc->cache_ctrl = 0;//default disable cache
    mmc->cache_size = (uint32_t)(*(uint32_t *)&ext_csd[EXT_CSD_CACHE_SIZE]);
    mmc->cache_size *= (1024 / 8);//convert to bytes

    if (mmc->version >= MMC_VERSION_4_5)
        mmc->gen_cmd6_time = ext_csd[EXT_CSD_GENERIC_CMD6_TIME];

    /* The partition data may be non-zero but it is only
     * effective if PARTITION_SETTING_COMPLETED is set in
     * EXT_CSD, so ignore any data if this bit is not set,
     * except for enabling the high-capacity group size
     * definition (see below).
     */
    part_completed = !!(ext_csd[EXT_CSD_PARTITION_SETTING] &
                        EXT_CSD_PARTITION_SETTING_COMPLETED);

    mmc->part_switch_time = ext_csd[EXT_CSD_PART_SWITCH_TIME];
    /* Some eMMC set the value too low so set a minimum */
    if (mmc->part_switch_time < MMC_MIN_PART_SWITCH_TIME && mmc->part_switch_time)
        mmc->part_switch_time = MMC_MIN_PART_SWITCH_TIME;

    /* store the partition info of emmc */
    mmc->part_support = ext_csd[EXT_CSD_PARTITIONING_SUPPORT];
    if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) ||
        ext_csd[EXT_CSD_BOOT_MULT])
        mmc->part_config = ext_csd[EXT_CSD_PART_CONF];
    if (part_completed &&
        (ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & ENHNCD_SUPPORT))
        mmc->part_attr = ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE];

    mmc->capacity_boot = ext_csd[EXT_CSD_BOOT_MULT] << 17;

    mmc->capacity_rpmb = ext_csd[EXT_CSD_RPMB_MULT] << 17;

    for (i = 0; i < 4; i++) {
        int idx = EXT_CSD_GP_SIZE_MULT + i * 3;
        uint32_t mult = (ext_csd[idx + 2] << 16) +
                        (ext_csd[idx + 1] << 8) + ext_csd[idx];
        if (mult)
            has_parts = true;
        if (!part_completed)
            continue;
        mmc->capacity_gp[i] = mult;
        mmc->capacity_gp[i] *=
            ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
        mmc->capacity_gp[i] *= ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
        mmc->capacity_gp[i] <<= 19;
    }

#ifndef CONFIG_SPL_BUILD
    if (part_completed) {
        mmc->enh_user_size =
            (ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16) +
            (ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8) +
            ext_csd[EXT_CSD_ENH_SIZE_MULT];
        mmc->enh_user_size *= ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
        mmc->enh_user_size *= ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
        mmc->enh_user_size <<= 19;
        mmc->enh_user_start =
            (ext_csd[EXT_CSD_ENH_START_ADDR + 3] << 24) +
            (ext_csd[EXT_CSD_ENH_START_ADDR + 2] << 16) +
            (ext_csd[EXT_CSD_ENH_START_ADDR + 1] << 8) +
            ext_csd[EXT_CSD_ENH_START_ADDR];
        if (mmc->high_capacity)
            mmc->enh_user_start <<= 9;
    }
#endif

    /*
     * Host needs to enable ERASE_GRP_DEF bit if device is
     * partitioned. This bit will be lost every time after a reset
     * or power off. This will affect erase size.
     */
    if (part_completed)
        has_parts = true;
    if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) &&
        (ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE] & PART_ENH_ATTRIB))
        has_parts = true;
    if (has_parts) {
        err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_ERASE_GROUP_DEF, 1);

        if (err)
            goto error;

        ext_csd[EXT_CSD_ERASE_GROUP_DEF] = 1;
    }

    if (ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x01) {
#ifdef MMC_WRITE
        /* Read out group size from ext_csd */
        mmc->erase_grp_size =
            ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 1024;
#endif
        /*
         * if high capacity and partition setting completed
         * SEC_COUNT is valid even if it is smaller than 2 GiB
         * JEDEC Standard JESD84-B45, 6.2.4
         */
        if (mmc->high_capacity && part_completed) {
            capacity = (ext_csd[EXT_CSD_SEC_CNT]) |
                       (ext_csd[EXT_CSD_SEC_CNT + 1] << 8) |
                       (ext_csd[EXT_CSD_SEC_CNT + 2] << 16) |
                       (ext_csd[EXT_CSD_SEC_CNT + 3] << 24);
            capacity *= MMC_MAX_BLOCK_LEN;
            mmc->capacity_user = capacity;
        }
    }
#ifdef MMC_WRITE
    else {
        /* Calculate the group size from the csd value. */
        int erase_gsz, erase_gmul;

        erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
        erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
        mmc->erase_grp_size = (erase_gsz + 1)
                              * (erase_gmul + 1);
    }
#endif
#ifdef MMC_HW_PARTITIONING
    mmc->hc_wp_grp_size = 1024
                          * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
                          * ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
#endif

    mmc->wr_rel_set = ext_csd[EXT_CSD_WR_REL_SET];
    mmc->can_trim = !!(ext_csd[EXT_CSD_SEC_FEATURE] & EXT_CSD_SEC_FEATURE_TRIM_EN);

    return HAL_SDMMC_ERR_NONE;
error:
    if (mmc->ext_csd) {
#ifndef MMC_TINY
        //free(mmc->ext_csd);
#endif
        mmc->ext_csd = NULL;
    }
    return err;
}

static void part_init(struct blk_desc *dev_desc)
{
    return;
}

static int mmc_startup(struct mmc *mmc)
{
    int err, i;
    uint32_t mult, freq;
    uint64_t cmult, csize;
    struct mmc_cmd cmd;
    struct blk_desc *bdesc;

#ifdef CONFIG_MMC_SPI_CRC_ON
    if (mmc_host_is_spi(mmc)) { /* enable CRC check for spi */
        cmd.cmdidx = MMC_CMD_SPI_CRC_ON_OFF;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = 1;
        err = mmc_send_cmd(mmc, &cmd, NULL);
        if (err)
            return err;
    }
#endif

    /* Put the Card in Identify Mode */
    cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
                 MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
    cmd.resp_type = MMC_RSP_R2;
    cmd.cmdarg = 0;

    err = mmc_send_cmd_quirks(mmc, &cmd, NULL, MMC_QUIRK_RETRY_SEND_CID, 4);
    if (err)
        return err;

    memcpy(mmc->cid, cmd.response, 16);

    /*
     * For MMC cards, set the Relative Address.
     * For SD cards, get the Relatvie Address.
     * This also puts the cards into Standby State
     */
    if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
        cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
        cmd.cmdarg = mmc->rca << 16;
        cmd.resp_type = MMC_RSP_R6;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        if (IS_SD(mmc))
            mmc->rca = (cmd.response[0] >> 16) & 0xffff;
    }

    /* Get the Card-Specific Data */
    cmd.cmdidx = MMC_CMD_SEND_CSD;
    cmd.resp_type = MMC_RSP_R2;
    cmd.cmdarg = mmc->rca << 16;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        return err;

    mmc->csd[0] = cmd.response[0];//high
    mmc->csd[1] = cmd.response[1];
    mmc->csd[2] = cmd.response[2];
    mmc->csd[3] = cmd.response[3];//low

    if (mmc->version == MMC_VERSION_UNKNOWN) {
        int version = (cmd.response[0] >> 26) & 0xf;

        switch (version) {
            case 0:
                mmc->version = MMC_VERSION_1_2;
                break;
            case 1:
                mmc->version = MMC_VERSION_1_4;
                break;
            case 2:
                mmc->version = MMC_VERSION_2_2;
                break;
            case 3:
                mmc->version = MMC_VERSION_3;
                break;
            case 4:
                mmc->version = MMC_VERSION_4;
                break;
            default:
                mmc->version = MMC_VERSION_1_2;
                break;
        }
    }

    /* divide frequency by 10, since the mults are 10x bigger */
    freq = fbase[(cmd.response[0] & 0x7)];
    mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

    mmc->legacy_speed = freq * mult;
    hal_sdmmc_select_mode(mmc, MMC_LEGACY);

    mmc->dsr_imp = ((cmd.response[1] >> 12) & 0x1);
    mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

#ifdef MMC_WRITE
    if (IS_SD(mmc))
        mmc->write_bl_len = mmc->read_bl_len;
    else
        mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);
#endif

    if (mmc->high_capacity) {
        csize = (mmc->csd[1] & 0x3f) << 16
                | (mmc->csd[2] & 0xffff0000) >> 16;
        cmult = 8;
    } else {
        csize = (mmc->csd[1] & 0x3ff) << 2
                | (mmc->csd[2] & 0xc0000000) >> 30;
        cmult = (mmc->csd[2] & 0x00038000) >> 15;
    }

    mmc->capacity_user = (csize + 1) << (cmult + 2);
    mmc->capacity_user *= mmc->read_bl_len;
    mmc->capacity_boot = 0;
    mmc->capacity_rpmb = 0;
    for (i = 0; i < 4; i++)
        mmc->capacity_gp[i] = 0;

    if (mmc->read_bl_len > MMC_MAX_BLOCK_LEN)
        mmc->read_bl_len = MMC_MAX_BLOCK_LEN;

#ifdef MMC_WRITE
    if (mmc->write_bl_len > MMC_MAX_BLOCK_LEN)
        mmc->write_bl_len = MMC_MAX_BLOCK_LEN;
#endif

    if ((mmc->dsr_imp) && (0xffffffff != mmc->dsr)) {
        cmd.cmdidx = MMC_CMD_SET_DSR;
        cmd.cmdarg = (mmc->dsr & 0xffff) << 16;
        cmd.resp_type = MMC_RSP_NONE;
        if (mmc_send_cmd(mmc, &cmd, NULL))
            HAL_SDMMC_ERROR(0, "MMC: SET_DSR failed");
    }

    /* Select the card, and put it into Transfer Mode */
    if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
        cmd.cmdidx = MMC_CMD_SELECT_CARD;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = mmc->rca << 16;
        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;
    }

    /*
     * For SD, its erase group is always one sector
     */
#ifdef MMC_WRITE
    mmc->erase_grp_size = 1;
#endif
    mmc->part_config = MMCPART_NOAVAILABLE;

    err = mmc_startup_v4(mmc);
    if (err)
        return err;

    err = mmc_set_capacity(mmc, mmc_get_blk_desc(mmc)->hwpart);
    if (err)
        return err;

#ifdef MMC_TINY
    hal_sdmmc_set_clock(mmc, mmc->legacy_speed, false);
    hal_sdmmc_select_mode(mmc, MMC_LEGACY);
    hal_sdmmc_set_bus_width(mmc, 1);
#else
    if (IS_SD(mmc)) {
        err = sd_get_capabilities(mmc);
        if (err)
            return err;
        err = sd_select_mode_and_width(mmc, mmc->card_caps);
    } else {
        err = mmc_get_capabilities(mmc);
        if (err)
            return err;
        err = mmc_select_mode_and_width(mmc, mmc->card_caps);
    }
#endif
    if (err)
        return err;

    mmc->best_mode = mmc->selected_mode;

    /* Fix the block length for DDR mode */
    if (mmc->ddr_mode) {
        mmc->read_bl_len = MMC_MAX_BLOCK_LEN;
#ifdef MMC_WRITE
        mmc->write_bl_len = MMC_MAX_BLOCK_LEN;
#endif
    }

    /* fill in device description */
    bdesc = mmc_get_blk_desc(mmc);
    bdesc->lun = 0;
    bdesc->hwpart = 0;
    bdesc->type = 0;
    bdesc->blksz = mmc->read_bl_len;
    bdesc->log2blksz = LOG2(bdesc->blksz);
    bdesc->lba = sdmmc_lldiv(mmc->capacity, mmc->read_bl_len);
#if !defined(CONFIG_SPL_BUILD) || \
        (defined(CONFIG_SPL_LIBCOMMON_SUPPORT) && \
        !defined(USE_TINY_PRINTF))
    sprintf(bdesc->vendor, "Man %06x Snr %04x%04x",
            mmc->cid[0] >> 24, (mmc->cid[2] & 0xffff),
            (mmc->cid[3] >> 16) & 0xffff);
    sprintf(bdesc->product, "%c%c%c%c%c%c", mmc->cid[0] & 0xff,
            (mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
            (mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff,
            (mmc->cid[2] >> 24) & 0xff);
    sprintf(bdesc->revision, "%d.%d", (mmc->cid[2] >> 20) & 0xf,
            (mmc->cid[2] >> 16) & 0xf);
#else
    bdesc->vendor[0] = 0;
    bdesc->product[0] = 0;
    bdesc->revision[0] = 0;
#endif

#if !defined(CONFIG_DM_MMC) && (!defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBDISK_SUPPORT))
    part_init(bdesc);
#endif

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_send_if_cond(struct mmc *mmc)
{
    struct mmc_cmd cmd;
    int err;

    cmd.cmdidx = SD_CMD_SEND_IF_COND;
    /* We set the bit if the host supports voltages between 2.7 and 3.6 V */
    cmd.cmdarg = ((mmc->cfg->voltages & 0xff8000) != 0) << 8 | 0xaa;
    cmd.resp_type = MMC_RSP_R7;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    if ((cmd.response[0] & 0xff) != 0xaa)
        return HAL_SDMMC_OP_NOT_SUPPORTED_EP;
    else
        mmc->version = SD_VERSION_2;

    return HAL_SDMMC_ERR_NONE;
}

static int mmc_power_init(struct mmc *mmc)
{
#ifdef DM_MMC
#ifdef DM_REGULATOR
    int ret;

    ret = device_get_supply_regulator(mmc->dev, "vmmc-supply",
                                      &mmc->vmmc_supply);
    if (ret)
        HAL_SDMMC_ERROR(0, "%s: No vmmc supply", mmc->dev->name);

    ret = device_get_supply_regulator(mmc->dev, "vqmmc-supply",
                                      &mmc->vqmmc_supply);
    if (ret)
        HAL_SDMMC_ERROR(0, "%s: No vqmmc supply", mmc->dev->name);
#endif
#else /* !CONFIG_DM_MMC */
    /*
     * Driver model should use a regulator, as above, rather than calling
     * out to board code.
     */
    board_mmc_power_init();
#endif
    return HAL_SDMMC_ERR_NONE;
}

static void mmc_set_initial_state(struct mmc *mmc)
{
    int err;

    /* First try to set 3.3V. If it fails set to 1.8V */
    err = mmc_set_signal_voltage(mmc, MMC_SIGNAL_VOLTAGE_330);
    if (err != 0)
        err = mmc_set_signal_voltage(mmc, MMC_SIGNAL_VOLTAGE_180);
    if (err != 0)
        HAL_SDMMC_ERROR(0, "mmc: failed to set signal voltage");

    hal_sdmmc_select_mode(mmc, MMC_LEGACY);
    hal_sdmmc_set_bus_width(mmc, 1);
    hal_sdmmc_set_clock(mmc, 0, MMC_CLK_ENABLE);//set clk to 400K
}

static int mmc_power_on(struct mmc *mmc)
{
    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);
#if defined(DM_MMC) && defined(DM_REGULATOR)
    if (mmc->vmmc_supply) {
        int ret = regulator_set_enable(mmc->vmmc_supply, true);

        if (ret && ret != -EACCES) {
            HAL_SDMMC_ERROR(0, "Error enabling VMMC supply : %d", ret);
            return ret;
        }
    }
#endif
    return HAL_SDMMC_ERR_NONE;
}

static int mmc_power_off(struct mmc *mmc)
{
    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);
    hal_sdmmc_set_clock(mmc, 0, MMC_CLK_DISABLE);//set clk to 400K
#if defined(DM_MMC) && defined(DM_REGULATOR)
    if (mmc->vmmc_supply) {
        int ret = regulator_set_enable(mmc->vmmc_supply, false);

        if (ret && ret != -EACCES) {
            HAL_SDMMC_ERROR(0, "Error disabling VMMC supply : %d", ret);
            return ret;
        }
    }
#endif
    return HAL_SDMMC_ERR_NONE;
}

static int mmc_power_cycle(struct mmc *mmc)
{
    int ret;

    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);
    ret = mmc_power_off(mmc);
    if (ret)
        return ret;

    ret = mmc_host_power_cycle(mmc);
    if (ret)
        return ret;

    /*
     * SD spec recommends at least 1ms of delay. Let's wait for 2ms
     * to be on the safer side.
     */
    //hal_sdmmc_delay_us(2000);
    hal_sdmmc_delay_ms(2);
    return mmc_power_on(mmc);
}

static int mmc_get_op_cond(struct mmc *mmc, bool quiet)
{
    bool uhs_en = supports_uhs(mmc->cfg->host_caps);
    int err;

    HAL_SDMMC_TRACE(0, "%s:%d, quiet:%d", __func__, __LINE__, quiet);
    if (mmc->has_init)
        return HAL_SDMMC_ERR_NONE;

    err = mmc_power_init(mmc);
    if (err)
        return err;

#ifdef CONFIG_MMC_QUIRKS
    mmc->quirks = MMC_QUIRK_RETRY_SET_BLOCKLEN |
                  MMC_QUIRK_RETRY_SEND_CID |
                  MMC_QUIRK_RETRY_APP_CMD;
#endif

    err = mmc_power_cycle(mmc);
    if (err) {
        /*
         * if power cycling is not supported, we should not try
         * to use the UHS modes, because we wouldn't be able to
         * recover from an error during the UHS initialization.
         */
        HAL_SDMMC_TRACE(0, "Unable to do a full power cycle. Disabling the UHS modes for safety");
        uhs_en = false;
        mmc->host_caps &= ~UHS_CAPS;
        err = mmc_power_on(mmc);
    }
    if (err)
        return err;

#ifdef DM_MMC
    /*
     * Re-initialization is needed to clear old configuration for
     * mmc rescan.
     */
    err = mmc_reinit(mmc);
#else
    /* made sure it's not NULL earlier */
    //err = mmc->cfg->ops->init(mmc);//moved to host init func
#endif
    if (err)
        return err;
    mmc->ddr_mode = 0;

retry:
    mmc_set_initial_state(mmc);

    /* Reset the Card */
    err = mmc_go_idle(mmc);

    if (err)
        return err;

    /* The internal partition reset to user partition(0) at every CMD0 */
    mmc_get_blk_desc(mmc)->hwpart = 0;

    /* Test for SD version 2 */
    err = mmc_send_if_cond(mmc);

    /* Now try to get the SD card's operating condition */
    err = sd_send_op_cond(mmc, uhs_en);

    if (err && uhs_en) {
        uhs_en = false;
        mmc_power_cycle(mmc);
        goto retry;
    }

    /* If the command timed out, we check for an MMC card */
    if (err == HAL_SDMMC_COMM_TIMEOUT) {
        err = mmc_send_op_cond(mmc);
        if (err) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
            if (!quiet)
                HAL_SDMMC_ERROR(0, "Card did not respond to voltage select! : %d", err);
#endif
            return HAL_SDMMC_OP_NOT_SUPPORTED_EP;
        }
    }

    return err;
}

static int mmc_start_init(struct mmc *mmc)
{
    bool no_card;
    int err = 0;

    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);
    /*
     * all hosts are capable of 1 bit bus-width and able to use the legacy
     * timings.
     */
    mmc->host_caps = mmc->cfg->host_caps | MMC_CAP(MMC_LEGACY) | MMC_MODE_1BIT;

#ifdef CONFIG_MMC_SPEED_MODE_SET
    if (mmc->user_speed_mode != MMC_MODES_END) {
        int i;
        /* set host caps */
        if (mmc->host_caps & MMC_CAP(mmc->user_speed_mode)) {
            /* Remove all existing speed capabilities */
            for (i = MMC_LEGACY; i < MMC_MODES_END; i++) {
                mmc->host_caps &= ~MMC_CAP(i);
            }
            mmc->host_caps |= (MMC_CAP(mmc->user_speed_mode)
                               | MMC_CAP(MMC_LEGACY) | MMC_MODE_1BIT);
        } else {
            HAL_SDMMC_ERROR(0, "bus_mode requested is not supported");
            return HAL_SDMMC_INVALID_PARAMETER;
        }
    }
#endif

#ifdef DM_MMC
    mmc_deferred_probe(mmc);
#endif
#if !defined(CONFIG_MMC_BROKEN_CD)
    no_card = mmc_getcd(mmc) == 0;
#else
    no_card = 0;
#endif
#ifndef DM_MMC
    /* we pretend there's no card when init is NULL */
    no_card = no_card || (mmc->cfg->ops->init == NULL);
#endif
    if (no_card) {
        mmc->has_init = 0;
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
        HAL_SDMMC_ERROR(0, "MMC: no card present");
#endif
        return HAL_SDMMC_NO_MEDIUM_FOUND;
    }

    err = mmc_get_op_cond(mmc, false);
    if (!err)
        mmc->init_in_progress = 1;

    return err;
}

static int mmc_complete_init(struct mmc *mmc)
{
    int err = 0;

    mmc->init_in_progress = 0;
    if (mmc->op_cond_pending)
        err = mmc_complete_op_cond(mmc);

    if (!err)
        err = mmc_startup(mmc);
    if (err)
        mmc->has_init = 0;
    else
        mmc->has_init = 1;
    return err;
}

static int mmc_init(struct mmc *mmc)
{
    int err = 0;
    POSSIBLY_UNUSED uint32_t start;
#ifdef DM_MMC
    struct mmc_uclass_priv *upriv = dev_get_uclass_priv(mmc->dev);
    upriv->mmc = mmc;
#endif
    if (mmc->has_init)
        return HAL_SDMMC_ERR_NONE;

    start = hal_fast_sys_timer_get();
    if (!mmc->init_in_progress)
        err = mmc_start_init(mmc);

    if (!err)
        err = mmc_complete_init(mmc);
    if (err)
        HAL_SDMMC_ERROR(0, "%s: %d, time %ums", __func__, err, FAST_TICKS_TO_MS(hal_fast_sys_timer_get() - start));

    return err;
}

#if defined(MMC_UHS_SUPPORT) || \
    defined(MMC_HS200_SUPPORT) || \
    defined(MMC_HS400_SUPPORT)
static int mmc_deinit(struct mmc *mmc)
{
    uint32_t caps_filtered;

    if (!mmc->has_init)
        return HAL_SDMMC_ERR_NONE;

    if (IS_SD(mmc)) {
        caps_filtered = mmc->card_caps &
                        ~(MMC_CAP(UHS_SDR12) | MMC_CAP(UHS_SDR25) |
                          MMC_CAP(UHS_SDR50) | MMC_CAP(UHS_DDR50) |
                          MMC_CAP(UHS_SDR104));

        return sd_select_mode_and_width(mmc, caps_filtered);
    } else {
        caps_filtered = mmc->card_caps &
                        ~(MMC_CAP(MMC_HS_200) | MMC_CAP(MMC_HS_400) | MMC_CAP(MMC_HS_400_ES));

        return mmc_select_mode_and_width(mmc, caps_filtered);
    }
}
#endif

int mmc_set_dsr(struct mmc *mmc, uint16_t val)
{
    mmc->dsr = val;
    return HAL_SDMMC_ERR_NONE;
}

static void mmc_set_preinit(struct mmc *mmc, int preinit)
{
    mmc->preinit = preinit;
}

#ifdef DM_MMC
static int mmc_probe(struct bd_info *bis)
{
    int ret, i;
    struct uclass *uc;
    struct udevice *dev;

    ret = uclass_get(UCLASS_MMC, &uc);
    if (ret)
        return ret;

    /*
     * Try to add them in sequence order. Really with driver model we
     * should allow holes, but the current MMC list does not allow that.
     * So if we request 0, 1, 3 we will get 0, 1, 2.
     */
    for (i = 0; ; i++) {
        ret = uclass_get_device_by_seq(UCLASS_MMC, i, &dev);
        if (ret == HAL_SDMMC_NO_SUCH_DEVICE)
            break;
    }
    uclass_foreach_dev(dev, uc) {
        ret = device_probe(dev);
        if (ret)
            HAL_SDMMC_ERROR(0, "%s - probe failed: %d", dev->name, ret);
    }

    return ret;
}
#else
static int mmc_probe(struct mmc *mmc)
{
    return HAL_SDMMC_ERR_NONE;
}
#endif

static int mmc_initialize(struct mmc *mmc)
{
    int ret = 0;
    HAL_SDMMC_TRACE(0, "%s:%d", __func__, __LINE__);

#ifndef BLK
#ifndef MMC_TINY
    //mmc_list_init();
#endif
#endif
    ret = mmc_probe(mmc);
    if (ret)
        return ret;

#ifndef CONFIG_SPL_BUILD
    //print_mmc_devices(',');
#endif

    mmc_do_preinit(mmc);
    return HAL_SDMMC_ERR_NONE;
}

#ifdef DM_MMC
int mmc_init_device(int num)
{
    struct udevice *dev;
    struct mmc *m;
    int ret;

    if (uclass_get_device_by_seq(UCLASS_MMC, num, &dev)) {
        ret = uclass_get_device(UCLASS_MMC, num, &dev);
        if (ret)
            return ret;
    }

    m = mmc_get_mmc_dev(dev);
    if (!m)
        return HAL_SDMMC_ERR_NONE;

    /* Initialising user set speed mode */
    m->user_speed_mode = MMC_MODES_END;

    if (m->preinit)
        mmc_start_init(m);

    return HAL_SDMMC_ERR_NONE;
}
#endif

#ifdef CONFIG_CMD_BKOPS_ENABLE
int mmc_set_bkops_enable(struct mmc *mmc)
{
    int err;
    struct sdmmc_ip_host *host = mmc->priv;
    uint8_t *ext_csd = (uint8_t *)sdmmc_ext_csd[host->host_id];

#if 0   //No need to call it multiple times
    err = mmc_send_ext_csd(mmc, ext_csd);
    if (err) {
        HAL_SDMMC_ERROR(0, "Could not get ext_csd register values");
        return err;
    }
#endif

    if (!(ext_csd[EXT_CSD_BKOPS_SUPPORT] & 0x1)) {
        HAL_SDMMC_ERROR(0, "Background operations not supported on device");
        return HAL_SDMMC_MEDIUM_TYPE_ERR;
    }

    if (ext_csd[EXT_CSD_BKOPS_EN] & 0x1) {
        HAL_SDMMC_ERROR(0, "Background operations already enabled");
        return HAL_SDMMC_ERR_NONE;
    }

    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BKOPS_EN, 1);
    if (err) {
        HAL_SDMMC_ERROR(0, "Failed to enable manual background operations");
        return err;
    }

    HAL_SDMMC_TRACE(0, "Enabled manual background operations");

    return HAL_SDMMC_ERR_NONE;
}
#endif

int mmc_get_env_dev(void)
{
#ifdef CONFIG_SYS_MMC_ENV_DEV
    return CONFIG_SYS_MMC_ENV_DEV;
#else
    return HAL_SDMMC_ERR_NONE;
#endif
}
/****************************platform related functions************************/
static HAL_SDMMC_DELAY_FUNC sdmmc_delay_ms = NULL;
static HAL_SDMMC_DELAY_US_FUNC sdmmc_delay_us = NULL;

static void hal_sdmmc_delay_ms(uint32_t ms)
{
    if (sdmmc_delay_ms) {
        sdmmc_delay_ms(ms);
    } else {
        osDelay(ms);
    }
}

static void hal_sdmmc_delay_us(uint32_t us)
{
    if (sdmmc_delay_us) {
        sdmmc_delay_us(us);
    } else {
        hal_sys_timer_delay_us(us);
    }
}

POSSIBLY_UNUSED static void delay_nop(uint32_t cnt)
{
    volatile uint32_t i = 0, c = 0;
    for (i = 0; i < cnt; ++i) {
        c++;
        __asm("nop");
    }
}

#ifndef DM_MMC
static void board_mmc_power_init(void)
{

}
#endif

/*****************************phy related functions****************************/
#ifdef MMC_HS400_SUPPORT
static int mmc_hs400_prepare_ddr(struct mmc *mmc)
{
    return HAL_SDMMC_ERR_NONE;
}
#endif

/***************************ip related functions*******************************/
#define DATA0_SM_BUSY_TIMEOUT           2000 //ms
#define SEND_CMD_TIMEOUT                1000 //ms
#define REG_CFG_TIMEOUT                 1000 //ms
#define DMA_XFER_TIMEOUT                5000 //ms
#define FIFO_XFER_TIMEOUT               1000 //ms  40ns/byte,1s=23.75MB

#define __SDMMC_DIV_ROUND_UP(n,d)       (((n) + (d) - 1) / (d))

#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
    #define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif

static struct sdmmc_ip_host sdmmc_host[HAL_SDMMC_ID_NUM];
static struct HAL_SDMMC_CB_T sdmmc_callback_default[HAL_SDMMC_ID_NUM];
static struct HAL_SDMMC_CB_T *sdmmc_callback[HAL_SDMMC_ID_NUM] = {NULL};
static uint32_t sdmmc_ip_base[HAL_SDMMC_ID_NUM] = {
    SDMMC0_BASE,
#ifdef SDMMC1_BASE
    SDMMC1_BASE,
#endif
};

#define SDMMC_DMA_LINK_SIZE     16368       //one desc can send and receive 16368(=4092*4) bytes
#define SDMMC_DMA_TSIZE_MAX     0xFFC       //4092,must be an integer multiple of 4, otherwise the host rx may have a problem
#ifndef SDMMC_DMA_DESC_CNT
    #define SDMMC_DMA_DESC_CNT  4           //4*16368=63.93KB
#endif
SYNC_FLAGS_LOC static struct HAL_DMA_DESC_T sdmmc_dma_desc[HAL_SDMMC_ID_NUM][SDMMC_DMA_DESC_CNT];
static void sdmmc_ip0_ext_dma_tx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli);
static void sdmmc_ip0_ext_dma_rx_handler(uint8_t chan, uint32_t remain_rsize, uint32_t error, struct HAL_DMA_DESC_T *lli);
#ifdef SDMMC1_BASE
static void sdmmc_ip1_ext_dma_tx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli);
static void sdmmc_ip1_ext_dma_rx_handler(uint8_t chan, uint32_t remain_rsize, uint32_t error, struct HAL_DMA_DESC_T *lli);
#endif
static HAL_DMA_IRQ_HANDLER_T sdmmc_ip_ext_dma_irq_handlers[HAL_SDMMC_ID_NUM * 2] = {
    sdmmc_ip0_ext_dma_tx_handler, sdmmc_ip0_ext_dma_rx_handler,
#ifdef SDMMC1_BASE
    sdmmc_ip1_ext_dma_tx_handler, sdmmc_ip1_ext_dma_rx_handler,
#endif
};

static inline void sdmmc_ip_writel(struct sdmmc_ip_host *host, uint32_t reg, uint32_t val)
{
    *((volatile uint32_t *)((uint32_t)(host->ioaddr) + reg)) = val;
}

static inline uint32_t sdmmc_ip_readl(struct sdmmc_ip_host *host, uint32_t reg)
{
    return *((volatile uint32_t *)((uint32_t)(host->ioaddr) + reg));
}

static int sdmmc_ip_wait_reset(struct sdmmc_ip_host *host, uint32_t value)
{
    uint32_t ctrl;
    uint32_t timeout = MS_TO_FAST_TICKS(REG_CFG_TIMEOUT), busy_t;

    sdmmc_ip_writel(host, SDMMCIP_REG_CTRL, value);
    busy_t = hal_fast_sys_timer_get();
    while (hal_fast_sys_timer_get() - busy_t < timeout) {
        ctrl = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
        if (!(ctrl & SDMMCIP_REG_RESET_ALL)) {
            return 0;//reset success
        }
    }

    return 1;//reset failure
}

static void sdmmc_ip_reset_fifo(struct sdmmc_ip_host *host)
{
    uint32_t ctrl;

    ctrl = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
    ctrl |= SDMMCIP_REG_CTRL_FIFO_RESET;
    if (sdmmc_ip_wait_reset(host, ctrl)) {
        if (sdmmc_ip_wait_reset(host, ctrl)) {
            HAL_SDMMC_ERROR(0, "%s failed", __func__);
        }
    }
}

static void sdmmc_ip_reset_dma(struct sdmmc_ip_host *host)
{
    uint32_t ctrl;

    if (host->dma_in_use) {
        host->dma_in_use = 0;
    } else {
        return;//Prevent multiple calls
    }

    //reset sdmmc ip dma
    ctrl = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
    ctrl |= SDMMCIP_REG_CTRL_DMA_RESET;
    if (sdmmc_ip_wait_reset(host, ctrl)) {
        if (sdmmc_ip_wait_reset(host, ctrl)) {
            HAL_SDMMC_ERROR(0, "%s failed", __func__);
        }
    }

    //free gpdma channel
    hal_gpdma_free_chan(host->dma_ch);

    //close sdmmc ip dma
    ctrl = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
    ctrl &= ~SDMMCIP_REG_DMA_EN;
    sdmmc_ip_writel(host, SDMMCIP_REG_CTRL, ctrl);
}

static void sdmmc_base_dma_tx_handler(enum HAL_SDMMC_ID_T id, uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    uint32_t ip_raw_int_status = 0;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    ip_raw_int_status = sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS);
    HAL_SDMMC_TRACE(3, "%s:%d, tx ip_raw_int_status 0x%x", __func__, __LINE__, (uint32_t)ip_raw_int_status);
    HAL_SDMMC_TRACE(4, "---tx dma handler,chan:%d,remain:%d,error:%d,lli:0x%X", chan, remain_tsize, error, (uint32_t)lli);
    if (ip_raw_int_status & (SDMMCIP_REG_DATA_ERR | SDMMCIP_REG_DATA_TOUT)) {
        HAL_SDMMC_TRACE(3, "%s:%d, sdmmcip0 tx dma error 0x%x", __func__, __LINE__, (uint32_t)ip_raw_int_status);
    }

    sdmmc_ip_reset_dma(host);
    host->sdmmc_dma_lock = 0;
}

static void sdmmc_base_dma_rx_handler(enum HAL_SDMMC_ID_T id, uint8_t chan, uint32_t remain_rsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    uint32_t ip_raw_int_status = 0;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    ip_raw_int_status = sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS);
    HAL_SDMMC_TRACE(3, "%s:%d, ip_raw_int_status 0x%x", __func__, __LINE__, (uint32_t)ip_raw_int_status);
    HAL_SDMMC_TRACE(4, "---rx dma handler,chan:%d,remain:%d,error:%d,lli:0x%X", chan, remain_rsize, error, (uint32_t)lli);
    if (ip_raw_int_status & (SDMMCIP_REG_DATA_ERR | SDMMCIP_REG_DATA_TOUT)) {
        HAL_SDMMC_TRACE(3, "%s:%d, sdmmcip0 rx dma error 0x%x", __func__, __LINE__, (uint32_t)ip_raw_int_status);
    }

    sdmmc_ip_reset_dma(host);
    host->sdmmc_dma_lock = 0;
}

static void sdmmc_ip0_ext_dma_tx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    sdmmc_base_dma_tx_handler(HAL_SDMMC_ID_0, chan, remain_tsize, error, lli);
}

static void sdmmc_ip0_ext_dma_rx_handler(uint8_t chan, uint32_t remain_rsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    sdmmc_base_dma_rx_handler(HAL_SDMMC_ID_0, chan, remain_rsize, error, lli);
}

#ifdef SDMMC1_BASE
static void sdmmc_ip1_ext_dma_tx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    sdmmc_base_dma_tx_handler(HAL_SDMMC_ID_1, chan, remain_tsize, error, lli);
}

static void sdmmc_ip1_ext_dma_rx_handler(uint8_t chan, uint32_t remain_rsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    sdmmc_base_dma_rx_handler(HAL_SDMMC_ID_1, chan, remain_rsize, error, lli);
}
#endif

static int sdmmc_ip_prepare_data(struct sdmmc_ip_host *host, struct mmc_data *data)
{
    uint8_t  real_src_width;
    uint16_t i;
    uint32_t ctrl;
    uint16_t link_cnt;
    uint32_t remain_transfer_size;
    POSSIBLY_UNUSED enum HAL_DMA_RET_T dret;
    struct HAL_DMA_CH_CFG_T dma_cfg;
    uint8_t id;
    uint32_t real_size;

    real_size = data->blocksize * data->blocks;
    sdmmc_ip_writel(host, SDMMCIP_REG_BLKSIZ, data->blocksize);
    sdmmc_ip_writel(host, SDMMCIP_REG_BYTCNT, real_size);

    if (host->dma_en) {
        if (host->dma_in_use) {
            return HAL_SDMMC_DMA_IN_USE;
        }
        host->sdmmc_dma_lock = 1;
        id = host->host_id;

        /* enable sdmmc ip dma function */
        ctrl = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
        ctrl |= SDMMCIP_REG_DMA_EN;
        sdmmc_ip_writel(host, SDMMCIP_REG_CTRL, ctrl);

        memset(&dma_cfg, 0, sizeof(dma_cfg));
        if (data->flags & MMC_DATA_READ) {
            dma_cfg.dst = (uint32_t)data->dest;
            if (dma_cfg.dst % 4) {
                dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
                dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
            } else {
                dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
                dma_cfg.dst_bsize = HAL_DMA_BSIZE_1;
            }
            dma_cfg.dst_periph = HAL_DMA_PERIPH_NULL;//useless
            dma_cfg.handler = host->rx_dma_handler;
            dma_cfg.src_bsize = HAL_DMA_BSIZE_1;
            if (id == HAL_SDMMC_ID_0) {
                dma_cfg.src_periph = HAL_GPDMA_SDMMC0;
            }
#ifdef SDMMC1_BASE
            else if (id == HAL_SDMMC_ID_1) {
                dma_cfg.src_periph = HAL_GPDMA_SDMMC1;
            }
#endif
            else {
                return HAL_SDMMC_INVALID_PARAMETER;
            }
            dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
            dma_cfg.try_burst = 1;
            dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
            dma_cfg.src = 0;//useless
            dma_cfg.ch = hal_gpdma_get_chan(dma_cfg.src_periph, HAL_DMA_HIGH_PRIO);
            real_src_width = dma_cfg.src_width ? dma_cfg.src_width * 2 : 1;

            HAL_SDMMC_TRACE(0, "  ");
            HAL_SDMMC_TRACE(0, "---sdmmc host use dma read");
            HAL_SDMMC_TRACE(1, "---dma read len      :%d", real_size);
            HAL_SDMMC_TRACE(1, "---dma_cfg.dst       :0x%x", dma_cfg.dst);
            HAL_SDMMC_TRACE(1, "---dma_cfg.dst_width :%d", dma_cfg.dst_width ? dma_cfg.dst_width * 2 : 1);
            HAL_SDMMC_TRACE(1, "---dma_cfg.src_width :%d", real_src_width);
            HAL_SDMMC_TRACE(1, "---dma_cfg.dst_bsize :%d", dma_cfg.dst_bsize ? (2 * (1 << dma_cfg.dst_bsize)) : 1);
            HAL_SDMMC_TRACE(1, "---dma_cfg.src_bsize :%d", dma_cfg.src_bsize ? (2 * (1 << dma_cfg.src_bsize)) : 1);
            HAL_SDMMC_TRACE(1, "---dma_cfg.src_periph:%d", dma_cfg.src_periph);
            HAL_SDMMC_TRACE(1, "---dma_cfg.ch        :%d", dma_cfg.ch);
            //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "  ");

            remain_transfer_size = real_size / 4;
            if (remain_transfer_size > SDMMC_DMA_TSIZE_MAX) {
                if (remain_transfer_size % SDMMC_DMA_TSIZE_MAX) {
                    link_cnt = remain_transfer_size / SDMMC_DMA_TSIZE_MAX + 1;
                } else {
                    link_cnt = remain_transfer_size / SDMMC_DMA_TSIZE_MAX;
                }
            } else {
                link_cnt = 1;
            }
            //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "---link_cnt          :%d", link_cnt);

            //Generate dma link configuration
            for (i = 0; i < link_cnt; i++) {
                dma_cfg.dst = (uint32_t)(data->dest + SDMMC_DMA_TSIZE_MAX * real_src_width * i);
                if (remain_transfer_size > SDMMC_DMA_TSIZE_MAX) {
                    dma_cfg.src_tsize = SDMMC_DMA_TSIZE_MAX;
                    remain_transfer_size -= SDMMC_DMA_TSIZE_MAX;
                } else {
                    dma_cfg.src_tsize = remain_transfer_size;
                    remain_transfer_size = 0;
                }
                if (i + 1 == link_cnt) {
                    dret = hal_dma_init_desc(&sdmmc_dma_desc[id][i], &dma_cfg, NULL, 1);
                } else {
                    dret = hal_dma_init_desc(&sdmmc_dma_desc[id][i], &dma_cfg, &sdmmc_dma_desc[id][i + 1], 0);
                }
                HAL_SDMMC_ASSERT(dret == HAL_DMA_OK, "%s:%d,sdmmc dma: Failed to init rx desc %d", __func__, __LINE__, i);

                //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "---dma_cfg.dst       :0x%x", dma_cfg.dst);
                //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "---dma_cfg.src_tsize :%d", dma_cfg.src_tsize);
                //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "sdmmc_dma_desc[%d][%d].src/dst/lli/ctrl=0x%08X 0x%08X 0x%08X 0x%08X@0x%08X", id, i,
                //                    sdmmc_dma_desc[id][i].src, sdmmc_dma_desc[id][i].dst, sdmmc_dma_desc[id][i].lli, sdmmc_dma_desc[id][i].ctrl, (uint32_t)&sdmmc_dma_desc[id][i]);
            }
            //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "  ");
        } else {
            dma_cfg.dst = 0;//useless
            dma_cfg.dst_bsize = HAL_DMA_BSIZE_1;
            if (id == HAL_SDMMC_ID_0) {
                dma_cfg.dst_periph = HAL_GPDMA_SDMMC0;
            }
#ifdef SDMMC1_BASE
            else if (id == HAL_SDMMC_ID_1) {
                dma_cfg.dst_periph = HAL_GPDMA_SDMMC1;
            }
#endif
            else {
                return HAL_SDMMC_INVALID_PARAMETER;
            }
            dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
            dma_cfg.handler = host->tx_dma_handler;
            dma_cfg.src_periph = HAL_DMA_PERIPH_NULL;//useless
            dma_cfg.try_burst = 0;//M2P mode is useless
            dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
            dma_cfg.src = (uint32_t)data->src;
            if (dma_cfg.src % 4) {
                dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
                dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
                remain_transfer_size = real_size;
            } else {
                dma_cfg.src_bsize = HAL_DMA_BSIZE_1;
                dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
                remain_transfer_size = real_size / 4;
            }
            dma_cfg.ch = hal_gpdma_get_chan(dma_cfg.dst_periph, HAL_DMA_HIGH_PRIO);
            real_src_width = dma_cfg.src_width ? dma_cfg.src_width * 2 : 1;

            HAL_SDMMC_TRACE(0, "  ");
            HAL_SDMMC_TRACE(0, "---sdmmc host use dma write");
            HAL_SDMMC_TRACE(1, "---dma write len     :%d", real_size);
            HAL_SDMMC_TRACE(1, "---dma_cfg.src       :0x%x", dma_cfg.src);
            HAL_SDMMC_TRACE(1, "---dma_cfg.dst_width :%d", dma_cfg.dst_width ? dma_cfg.dst_width * 2 : 1);
            HAL_SDMMC_TRACE(1, "---dma_cfg.src_width :%d", real_src_width);
            HAL_SDMMC_TRACE(1, "---dma_cfg.dst_bsize :%d", dma_cfg.dst_bsize ? (2 * (1 << dma_cfg.dst_bsize)) : 1);
            HAL_SDMMC_TRACE(1, "---dma_cfg.src_bsize :%d", dma_cfg.src_bsize ? (2 * (1 << dma_cfg.src_bsize)) : 1);
            HAL_SDMMC_TRACE(1, "---dma_cfg.dst_periph:%d", dma_cfg.dst_periph);
            HAL_SDMMC_TRACE(1, "---dma_cfg.ch        :%d", dma_cfg.ch);
            //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "  ");

            if (remain_transfer_size > SDMMC_DMA_TSIZE_MAX) {
                if (remain_transfer_size % SDMMC_DMA_TSIZE_MAX) {
                    link_cnt = remain_transfer_size / SDMMC_DMA_TSIZE_MAX + 1;
                } else {
                    link_cnt = remain_transfer_size / SDMMC_DMA_TSIZE_MAX;
                }
            } else {
                link_cnt = 1;
            }
            //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "---link_cnt          :%d", link_cnt);

            //Generate dma link configuration
            for (i = 0; i < link_cnt; i++) {
                dma_cfg.src = (uint32_t)(data->src + SDMMC_DMA_TSIZE_MAX * real_src_width * i);
                if (remain_transfer_size > SDMMC_DMA_TSIZE_MAX) {
                    dma_cfg.src_tsize = SDMMC_DMA_TSIZE_MAX;
                    remain_transfer_size -= SDMMC_DMA_TSIZE_MAX;
                } else {
                    dma_cfg.src_tsize = remain_transfer_size;
                    remain_transfer_size = 0;
                }
                if (i + 1 == link_cnt) {
                    dret = hal_dma_init_desc(&sdmmc_dma_desc[id][i], &dma_cfg, NULL, 1);
                } else {
                    dret = hal_dma_init_desc(&sdmmc_dma_desc[id][i], &dma_cfg, &sdmmc_dma_desc[id][i + 1], 0);
                }
                HAL_SDMMC_ASSERT(dret == HAL_DMA_OK, "%s:%d,sdmmc dma: Failed to init rx desc %d", __func__, __LINE__, i);

                //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "---dma_cfg.src       :0x%x", dma_cfg.src);
                //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "---dma_cfg.src_tsize :%d", dma_cfg.src_tsize);
                //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "sdmmc_dma_desc[%d][%d].src/dst/lli/ctrl=0x%08X 0x%08X 0x%08X 0x%08X@0x%08X", id, i,
                //                    sdmmc_dma_desc[id][i].src, sdmmc_dma_desc[id][i].dst, sdmmc_dma_desc[id][i].lli, sdmmc_dma_desc[id][i].ctrl, (uint32_t)&sdmmc_dma_desc[id][i]);
            }
            //HAL_SDMMC_TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "  ");
        }

        host->dma_in_use = 1;
        host->dma_ch = dma_cfg.ch;
        hal_gpdma_sg_start(&sdmmc_dma_desc[id][0], &dma_cfg);
    }

    return HAL_SDMMC_ERR_NONE;
}

static int sdmmc_ip_set_transfer_mode(struct sdmmc_ip_host *host, struct mmc_data *data)
{
    uint32_t mode;

    mode = SDMMCIP_REG_CMD_DATA_EXP;
    if (data->flags & MMC_DATA_WRITE)
        mode |= SDMMCIP_REG_CMD_RW;

    return mode;
}

POSSIBLY_UNUSED static void sdmmc_ip_func_int_mask(enum HAL_SDMMC_ID_T id, uint8_t mask0_unmask1)
{
    uint32_t mask;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    mask = sdmmc_ip_readl(host, SDMMCIP_REG_INTMASK);
    if (mask0_unmask1) {
        //unmask(enable) func interrupt
        mask |= (SDMMCIP_REG_INTMSK_SDIO_FUNC1 | SDMMCIP_REG_INTMSK_SDIO_FUNC2
                 | SDMMCIP_REG_INTMSK_SDIO_FUNC3 | SDMMCIP_REG_INTMSK_SDIO_FUNC4
                 | SDMMCIP_REG_INTMSK_SDIO_FUNC5 | SDMMCIP_REG_INTMSK_SDIO_FUNC6
                 | SDMMCIP_REG_INTMSK_SDIO_FUNC7);
    } else {
        //mask(disable) func interrupt
        mask &= ~(SDMMCIP_REG_INTMSK_SDIO_FUNC1 | SDMMCIP_REG_INTMSK_SDIO_FUNC2
                  | SDMMCIP_REG_INTMSK_SDIO_FUNC3 | SDMMCIP_REG_INTMSK_SDIO_FUNC4
                  | SDMMCIP_REG_INTMSK_SDIO_FUNC5 | SDMMCIP_REG_INTMSK_SDIO_FUNC6
                  | SDMMCIP_REG_INTMSK_SDIO_FUNC7);
    }
    sdmmc_ip_writel(host, SDMMCIP_REG_INTMASK, mask);
}

static void fifo_xfer_timeout(void *param)
{
    struct sdmmc_ip_host *host = (struct sdmmc_ip_host *)param;
    host->fifo_rw_flag = 0;
}

int hal_sdmmc_ip_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int ret = 0;
    int flags = 0;
    uint32_t mask = 0;
    uint32_t status;
    uint32_t cmd_reg;
    uint32_t busy_timeout, busy_t;
    struct sdmmc_ip_host *host = mmc->priv;

    //Check if data0 or state machine is busy
    busy_t = hal_fast_sys_timer_get();
    busy_timeout = MS_TO_FAST_TICKS(DATA0_SM_BUSY_TIMEOUT);
    while (1) {
        status = sdmmc_ip_readl(host, SDMMCIP_REG_STATUS);
        cmd_reg = sdmmc_ip_readl(host, SDMMCIP_REG_CMD);
        if ((status & (SDMMCIP_REG_DATA0_BUSY | SDMMCIP_REG_STATE_MACHINE_BUSY)) || (cmd_reg & SDMMCIP_REG_CMD_START)) {
            if (hal_fast_sys_timer_get() - busy_t < busy_timeout) {
                HAL_SDMMC_TRACE(0, "sdmmc %s busy", (status & SDMMCIP_REG_DATA0_BUSY) ? "data0" : "state machine");
                hal_sdmmc_delay_us(10);
            } else {
                if (status & SDMMCIP_REG_DATA0_BUSY) {
                    HAL_SDMMC_ERROR(0, "%s: sdmmc data0 busy, %dms timeout, status:0x%X", __func__, DATA0_SM_BUSY_TIMEOUT, status);
                    return HAL_SDMMC_DATA0_BUSY;
                } else if (status & SDMMCIP_REG_STATE_MACHINE_BUSY) {
                    HAL_SDMMC_ERROR(0, "%s: sdmmc data state machine busy, %dms timeout, status:0x%X", __func__, DATA0_SM_BUSY_TIMEOUT, status);
                    return HAL_SDMMC_DATA_STE_MACHINE_BUSY;
                } else {
                    HAL_SDMMC_ERROR(0, "%s: sdmmc cmd start error, %dms timeout, cmd reg:0x%X", __func__, DATA0_SM_BUSY_TIMEOUT, cmd_reg);
                    return HAL_SDMMC_CMD_START_ERR;
                }
            }
        } else {
            break;
        }
    }

    if (host->dma_en) {
        sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_CDONE |
                        SDMMCIP_REG_RINTSTS_RTO | SDMMCIP_REG_RINTSTS_RE); //clear command flag
    } else {
        sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_ALL);//clear interrupt status
    }
    sdmmc_ip_writel(host, SDMMCIP_REG_CMDARG, cmd->cmdarg);

    if (data) {
        flags = sdmmc_ip_set_transfer_mode(host, data);
    }

    if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) {
        flags |= SDMMCIP_REG_CMD_ABORT_STOP;
    } else {
        flags |= SDMMCIP_REG_CMD_PRV_DAT_WAIT;
    }

    if (cmd->cmdidx == SD_CMD_SWITCH_UHS18V) {
        flags |= SDMMCIP_REG_CMD_VOLT_SWITCH;
    }

    if (cmd->resp_type & MMC_RSP_PRESENT) {
        flags |= SDMMCIP_REG_CMD_RESP_EXP;
        if (cmd->resp_type & MMC_RSP_136) {
            flags |= SDMMCIP_REG_CMD_RESP_LENGTH;
        }
    }

    if (cmd->resp_type & MMC_RSP_CRC) {
        flags |= SDMMCIP_REG_CMD_CHECK_CRC;
    }

    if (data) {
        ret = sdmmc_ip_prepare_data(host, data);
        if (ret) {
            return ret;
        }
    }

    flags |= (cmd->cmdidx | SDMMCIP_REG_CMD_START | SDMMCIP_REG_CMD_USE_HOLD_REG);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, flags);

    busy_t = hal_fast_sys_timer_get();
    if (cmd->cmdidx != SD_CMD_SWITCH_UHS18V) {
        busy_timeout = MS_TO_FAST_TICKS(SEND_CMD_TIMEOUT);
        while (1) {
            //When there is subsequent data transmission, no longer wait for cmd resp
            if (cmd->cmdidx == MMC_CMD_READ_SINGLE_BLOCK || cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK
                || cmd->cmdidx == MMC_CMD_WRITE_SINGLE_BLOCK || cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
                break;
            }
            mask = sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS);
            if (mask & SDMMCIP_REG_RINTSTS_CDONE) {
                sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_CDONE);
                break;
            } else if (hal_fast_sys_timer_get() - busy_t > busy_timeout) {
                HAL_SDMMC_ERROR(1, "%s: Timeout", __func__);
                HAL_SDMMC_ERROR(1, "sdmmc raw int status:0x%X", sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS));
                HAL_SDMMC_ERROR(1, "sdmmc status        :0x%X", sdmmc_ip_readl(host, SDMMCIP_REG_STATUS));

                sdmmc_ip_reset_fifo(host);
                if (data) {
                    if (host->dma_en) {
                        sdmmc_ip_reset_dma(host);
                    }
                }
                return HAL_SDMMC_CMD_SEND_TIMEOUT;
            } else {
                ;
            }
        }
    } else {
        //wait R1 response
        busy_timeout = MS_TO_FAST_TICKS(4);
        do {
            if (host->volt_switch_flag) {
                break;
            } else {
                hal_sdmmc_delay_us(10);
            }
        } while (hal_fast_sys_timer_get() - busy_t < busy_timeout);
    }

    if (mask & SDMMCIP_REG_RINTSTS_RTO) {
        /*
         * Timeout here is not necessarily fatal. (e)MMC cards
         * will splat here when they receive CMD55 as they do
         * not support this command and that is exactly the way
         * to tell them apart from SD cards. Thus, this output
         * below shall be TRACE(). eMMC cards also do not favor
         * CMD8, please keep that in mind.
         */
        HAL_SDMMC_ERROR(1, "%s: Response Timeout", __func__);
        HAL_SDMMC_ERROR(1, "sdmmc raw int status:0x%X", sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS));
        HAL_SDMMC_ERROR(1, "sdmmc status        :0x%X", sdmmc_ip_readl(host, SDMMCIP_REG_STATUS));

        sdmmc_ip_reset_fifo(host);
        if (data) {
            if (host->dma_en) {
                sdmmc_ip_reset_dma(host);
            }
        }
        return HAL_SDMMC_COMM_TIMEOUT;
    } else if (mask & SDMMCIP_REG_RINTSTS_RE) {
        HAL_SDMMC_ERROR(1, "%s: Response Error.", __func__);
        HAL_SDMMC_ERROR(1, "sdmmc raw int status:0x%X", sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS));
        HAL_SDMMC_ERROR(1, "sdmmc status        :0x%X", sdmmc_ip_readl(host, SDMMCIP_REG_STATUS));

        sdmmc_ip_reset_fifo(host);
        if (data) {
            if (host->dma_en) {
                sdmmc_ip_reset_dma(host);
            }
        }
        return HAL_SDMMC_RESPONSE_ERR;
    }

    if (cmd->resp_type & MMC_RSP_PRESENT) {
        if (cmd->resp_type & MMC_RSP_136) {
            //The reason why response[1]/[3] uses the subscript is reversed, it must be flipped here
            cmd->response[0] = sdmmc_ip_readl(host, SDMMCIP_REG_RESP3);//high
            cmd->response[1] = sdmmc_ip_readl(host, SDMMCIP_REG_RESP2);
            cmd->response[2] = sdmmc_ip_readl(host, SDMMCIP_REG_RESP1);
            cmd->response[3] = sdmmc_ip_readl(host, SDMMCIP_REG_RESP0);//low
        } else {
            if (!(cmd->cmdidx == MMC_CMD_READ_SINGLE_BLOCK || cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK
                || cmd->cmdidx == MMC_CMD_WRITE_SINGLE_BLOCK || cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK)) {
                cmd->response[0] = sdmmc_ip_readl(host, SDMMCIP_REG_RESP0);
            }
        }
    }

    if (data) {
        //Processing after the completion of the transfer process: use dma
        if (host->dma_en) {
            uint32_t timeout = MS_TO_FAST_TICKS(DMA_XFER_TIMEOUT), busy_t;
            busy_t = hal_fast_sys_timer_get();
            while (host->sdmmc_dma_lock) {
                if (host->yield) {
                    /*
                        1) here can be replaced by a semaphore operation
                        2) when using semaphores, you may need to lock to prevent sleep, such as
                            hal_bus_wake_lock(user);
                            hal_bus_wake_unlock(user);
                    */
                    hal_sdmmc_delay_ms(1);
                }

                while (hal_fast_sys_timer_get() - busy_t > timeout) {
                    ret = HAL_SDMMC_DMA_XFER_TIMEOUT;
                    HAL_SDMMC_ERROR(0, "%s: DMA transfer timeout:%dms", __func__, ret);
                    goto out;
                }
            }
            ret = 0;
        } else {
            //Processing after the completion of the transfer process: do not use dma
            uint32_t i = 0;
            uint32_t fifo_data;
            uint32_t data_size;
            uint32_t byte_in_fifo;

            data_size = data->blocks * data->blocksize;
            hwtimer_start(host->fifo_timer, MS_TO_TICKS(FIFO_XFER_TIMEOUT));
            host->fifo_rw_flag = 1;
            while (host->fifo_rw_flag) {
                mask = sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS);
                if (mask & (SDMMCIP_REG_DATA_ERR | SDMMCIP_REG_DATA_TOUT)) {
                    HAL_SDMMC_ERROR(0, "***%s: READ DATA ERROR!", __func__);
                    ret = HAL_SDMMC_READ_DATA_ERR;
                    goto finish;
                }
                status = sdmmc_ip_readl(host, SDMMCIP_REG_STATUS);
                byte_in_fifo = (status & SDMMCIP_REG_FIFO_COUNT_MASK) >> SDMMCIP_REG_FIFO_COUNT_SHIFT;
                byte_in_fifo *= sizeof(fifo_data);//Change it to the number of bytes
                if (data->flags == MMC_DATA_READ) {
                    if (byte_in_fifo) {
                        while (byte_in_fifo) {
                            fifo_data = sdmmc_ip_readl(host, SDMMCIP_REG_FIFO_OFFSET);
                            if (i < data_size) {
                                memcpy(data->dest + i, &fifo_data, sizeof(fifo_data));
                                i += sizeof(fifo_data);
                                if (byte_in_fifo >= sizeof(fifo_data)) {
                                    byte_in_fifo -= sizeof(fifo_data);
                                } else {
                                    byte_in_fifo -= byte_in_fifo;
                                }
                            } else {
                                HAL_SDMMC_ERROR(0, "***%s: fifo data too much", __func__);
                                ret = HAL_SDMMC_FIFO_OVERFLOW;
                                goto finish;
                            }
                        }
                    }
                    /* nothing to read from fifo and DTO is set */
                    else if (mask & SDMMCIP_REG_RINTSTS_DTO) {
                        if (i != data_size) {
                            HAL_SDMMC_ERROR(0, "***%s: need to read %d, actually read %d", __func__, data->blocks * data->blocksize, i);
                        }
                        ret = HAL_SDMMC_ERR_NONE;
                        goto finish;
                    }
                } else {
                    /* nothing to write to fifo and DTO is set */
                    if (mask & SDMMCIP_REG_RINTSTS_DTO) {
                        /* check if number is right */
                        if (i != data_size) {
                            HAL_SDMMC_ERROR(0, "***%s: need to write %d, actually written %d", __func__, data->blocks * data->blocksize, i);
                        }
                        ret = HAL_SDMMC_ERR_NONE;
                        goto finish;
                    } else {
                        if (byte_in_fifo <= SDMMC_FIFO_BYTES) {
                            byte_in_fifo = SDMMC_FIFO_BYTES - byte_in_fifo;
                        } else {
                            HAL_SDMMC_ERROR(0, "***%s: fifo data cnt error", __func__);
                            ret = HAL_SDMMC_FIFO_DATA_CNT_ERR;
                            goto finish;
                        }

                        while (byte_in_fifo) {
                            if (i < data_size) {
                                memcpy(&fifo_data, data->src + i, sizeof(fifo_data));
                                i += sizeof(fifo_data);
                                sdmmc_ip_writel(host, SDMMCIP_REG_FIFO_OFFSET, fifo_data);

                                if (byte_in_fifo >= sizeof(fifo_data)) {
                                    byte_in_fifo -= sizeof(fifo_data);
                                } else {
                                    byte_in_fifo -= byte_in_fifo;
                                }
                            } else {
                                HAL_SDMMC_TRACE(0, "%s: no data to write to fifo, do nothing", __func__);
                                break;
                            }
                        }
                    }
                }
            }

finish:
            /* stop timer */
            hwtimer_stop(host->fifo_timer);

            /* fifo xfer timeout */
            if (!host->fifo_rw_flag) {
                sdmmc_ip_reset_fifo(host);
                ret = HAL_SDMMC_FIFO_XFER_TIMEOUT;
                HAL_SDMMC_ERROR(0, "***%s: fifo xfer timeout", __func__);
            }
        }
    }

out:

    return ret;
}

static enum HAL_SDMMC_ERR sdmmc_ip_check_status(struct sdmmc_ip_host *host, uint32_t timeout_ticks)
{
    uint32_t busy_t;
    uint32_t status;

    busy_t = hal_fast_sys_timer_get();
    do {
        status = sdmmc_ip_readl(host, SDMMCIP_REG_CMD);
        if (hal_fast_sys_timer_get() - busy_t > timeout_ticks) {
            return HAL_SDMMC_CMD_START_TIMEOUT;
        }
    } while (status & (uint32_t)SDMMCIP_REG_CMD_START);

    return HAL_SDMMC_ERR_NONE;
}

static int sdmmc_ip_setup_bus(struct sdmmc_ip_host *host, uint32_t freq)
{
    uint32_t div, status;
    uint32_t timeout = MS_TO_FAST_TICKS(REG_CFG_TIMEOUT);
    uint32_t sclk;

    if ((freq == host->desired_bus_speed) || (freq == 0))
        return HAL_SDMMC_ERR_NONE;
    if (host->src_clk_hz)
        sclk = host->src_clk_hz;
    else {
        HAL_SDMMC_ERROR(1, "%s: Didn't get source clock value.", __func__);
        return HAL_SDMMC_INVALID_PARAMETER;
    }

    if (sclk <= freq)
        div = 0;    /* bypass mode */
    else
        div = __SDMMC_DIV_ROUND_UP(sclk, 2 * freq);

    HAL_SDMMC_TRACE(5, "%s: freq %d, sclk %d, reg div %d, final div %d", __func__, freq, sclk, div, div * 2);
    host->div = div * 2;
    host->final_bus_speed = host->div ? sclk / host->div : sclk;

    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, 0);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, status);
        return status;
    }

    sdmmc_ip_writel(host, SDMMCIP_REG_CLKDIV, div);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, status);
        return status;
    }

    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, SDMMCIP_REG_CLKEN_ENABLE |
                    SDMMCIP_REG_CLKEN_LOW_PWR);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, status);
        return status;
    }

    host->desired_bus_speed = freq;

    return HAL_SDMMC_ERR_NONE;
}

#ifdef CORE_SLEEP_POWER_DOWN
static int sdmmc_ip_setup_div(struct sdmmc_ip_host *host, uint32_t div)
{
    uint32_t status;
    uint32_t timeout = MS_TO_FAST_TICKS(REG_CFG_TIMEOUT);

    sdmmc_ip_writel(host, SDMMCIP_REG_CLKDIV, div);
    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, SDMMCIP_REG_CLKEN_ENABLE |
                    SDMMCIP_REG_CLKEN_LOW_PWR);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s: error:%d", __func__, status);
        return status;
    }

    return HAL_SDMMC_ERR_NONE;
}
#endif

static int sdmmc_ip_clk_disable_low_power(struct sdmmc_ip_host *host)
{
    uint32_t status;
    uint32_t timeout = MS_TO_FAST_TICKS(REG_CFG_TIMEOUT);

    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, 0);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, status);
        return status;
    }

    sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, SDMMCIP_REG_CLKEN_ENABLE);
    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, status);
        return status;
    }

    return HAL_SDMMC_ERR_NONE;
}

static int sdmmc_ip_wait_cmd_start(struct sdmmc_ip_host *host, uint8_t en_volt_switch)
{
    uint32_t status;
    uint32_t timeout = MS_TO_FAST_TICKS(REG_CFG_TIMEOUT);

    sdmmc_ip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
                    SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START |
                    (en_volt_switch ? SDMMCIP_REG_CMD_VOLT_SWITCH : 0));

    status = sdmmc_ip_check_status(host, timeout);
    if (status) {
        HAL_SDMMC_ERROR(0, "%s: error:%d", __func__, status);
        return status;
    }

    return HAL_SDMMC_ERR_NONE;
}

static int sdmmc_ip_set_ios_clock(struct mmc *mmc)
{
    uint32_t val;
    int ret = HAL_SDMMC_ERR_NONE;
    struct sdmmc_ip_host *host = (struct sdmmc_ip_host *)mmc->priv;

    HAL_SDMMC_TRACE(3, "%s, clk_disable: %d, clock: %d", __func__, mmc->clk_disable, mmc->clock);
    if (mmc->clock) {
        ret = sdmmc_ip_setup_bus(host, mmc->clock);
        if (ret) {
            return ret;
        }
    }
    if (mmc->clk_disable != mmc->clk_disable_copy) {
        mmc->clk_disable_copy = mmc->clk_disable;
        if (mmc->clk_disable == MMC_CLK_DISABLE) {
            //stop clk supply
            val = sdmmc_ip_readl(host, SDMMCIP_REG_CLKENA);
            if (val & SDMMCIP_REG_CLKEN_ENABLE) {
                val &= ~SDMMCIP_REG_CLKEN_ENABLE;
                sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, val);
                ret = sdmmc_ip_wait_cmd_start(host, host->volt_switch_flag ? 1 : 0);
                if (ret) {
                    HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, ret);
                }
                HAL_SDMMC_TRACE(0, "---clkena bit0 set to 0,clk closed,ret:%d", ret);
            }
        } else {
            //supply clk
            val = sdmmc_ip_readl(host, SDMMCIP_REG_CLKENA);
            if (!(val & SDMMCIP_REG_CLKEN_ENABLE)) {
                val |= SDMMCIP_REG_CLKEN_ENABLE;
                sdmmc_ip_writel(host, SDMMCIP_REG_CLKENA, val);
                ret = sdmmc_ip_wait_cmd_start(host, host->volt_switch_flag ? 1 : 0);
                if (ret) {
                    HAL_SDMMC_ERROR(0, "%s:%d, error:%d", __func__, __LINE__, ret);
                }
                HAL_SDMMC_TRACE(0, "---clkena bit0 set to 1,clk open,ret:%d", ret);
            }
        }
    }

    return ret;
}

static int sdmmc_ip_set_ios_width(struct mmc *mmc)
{
    uint32_t ctype, regs;
    struct sdmmc_ip_host *host = (struct sdmmc_ip_host *)mmc->priv;

    HAL_SDMMC_TRACE(3, "%s, Buswidth = %d", __func__, mmc->bus_width);
    if (mmc->bus_width) {
        switch (mmc->bus_width) {
            case HAL_SDMMC_BUS_WIDTH_8:
                ctype = SDMMCIP_REG_CTYPE_8BIT;
                break;
            case HAL_SDMMC_BUS_WIDTH_4:
                ctype = SDMMCIP_REG_CTYPE_4BIT;
                break;
            default:
                ctype = SDMMCIP_REG_CTYPE_1BIT;
                break;
        }

        sdmmc_ip_writel(host, SDMMCIP_REG_CTYPE, ctype);
        regs = sdmmc_ip_readl(host, SDMMCIP_REG_UHS_REG);
        if (mmc->ddr_mode)
            regs |= SDMMCIP_REG_DDR_MODE;
        else
            regs &= ~SDMMCIP_REG_DDR_MODE;
        sdmmc_ip_writel(host, SDMMCIP_REG_UHS_REG, regs);
    }

    return HAL_SDMMC_ERR_NONE;
}

SRAM_TEXT_LOC static void sdmmc_base_irq_handler(enum HAL_SDMMC_ID_T id)
{
    uint32_t i;
    uint32_t branch;
    uint32_t raw_int_status;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    raw_int_status = sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS);//read raw interrupt status
    sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, raw_int_status & (~SDMMCIP_REG_RINTSTS_CDONE)); //clear interrupt status
    __DSB();
    HAL_SDMMC_TRACE(3, "%s:%d,raw_int_status=0x%X", __func__, __LINE__, raw_int_status);

#if 0
    uint32_t int_status;
    int_status = sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS);
    HAL_SDMMC_TRACE(3, "++++++int_status=0x%X", int_status);
#endif

    //Clear the redundant signs, because they are not used, the purpose is to reduce the number of subsequent cycles
    raw_int_status &= (~(SDMMCIP_REG_RINTSTS_CDONE | SDMMCIP_REG_RINTSTS_TXDR | SDMMCIP_REG_RINTSTS_RXDR |
                         SDMMCIP_REG_INTMSK_SDIO_FUNC1 | SDMMCIP_REG_INTMSK_SDIO_FUNC2 | SDMMCIP_REG_INTMSK_SDIO_FUNC3 |
                         SDMMCIP_REG_INTMSK_SDIO_FUNC4 | SDMMCIP_REG_INTMSK_SDIO_FUNC5 | SDMMCIP_REG_INTMSK_SDIO_FUNC6 |
                         SDMMCIP_REG_INTMSK_SDIO_FUNC7));

    while (raw_int_status) {
        i = get_lsb_pos(raw_int_status);
        branch = raw_int_status & (1 << i);
        raw_int_status &= ~(1 << i);
        switch (branch) {
            case SDMMCIP_REG_RINTSTS_DTO: {
                HAL_SDMMC_TRACE(0, "bit 3:Data transfer over (DTO)");
                if (sdmmc_callback[id]->hal_sdmmc_txrx_done) {
                    sdmmc_callback[id]->hal_sdmmc_txrx_done();
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_EBE: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 15:End-bit error (read)/write no CRC (EBE)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_READ_END_BIT_ERR_WRITE_NOCRC);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_SBE: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 13:Start-bit error (SBE)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_START_BIT_ERR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_HLE: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 12:Hardware locked write error (HLE)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_HARDWARE_LOCKED_WRITE_ERR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_FRUN: {
                sdmmc_ip_reset_fifo(host);
                HAL_SDMMC_ERROR(0, "sdmmc,bit 11:FIFO underrun/overrun error (FRUN)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_FIFO_ERR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_HTO: {
                host->volt_switch_flag = 1;
                HAL_SDMMC_TRACE(0, "sdmmc,bit 10:Data starvation-by-host timeout (HTO)/Volt_switch_int");
                break;
            }
            case SDMMCIP_REG_RINTSTS_DRTO: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 9:Data read timeout (DRTO)/Boot Data Start (BDS)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_DATA_READ_TIMEOUT_BDS);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_RTO: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 8:Response timeout (RTO)/Boot Ack Received (BAR)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_RESPONSE_TIMEOUT_BAR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_DCRC: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 7:Data CRC error (DCRC)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_DATA_CRC_ERR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_RCRC: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 6:Response CRC error (RCRC)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_RESPONSE_CRC_ERR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_RE: {
                HAL_SDMMC_ERROR(0, "sdmmc,bit 1:Response error (RE)");
                if (sdmmc_callback[id]->hal_sdmmc_host_error) {
                    sdmmc_callback[id]->hal_sdmmc_host_error(HAL_SDMMC_HOST_RESPONSE_ERR);
                }
                break;
            }
            case SDMMCIP_REG_RINTSTS_CD: {
                //ip does not support, detect signal is tie to 1, 2022-06-02
                HAL_SDMMC_TRACE(0, "bit 0:Card detect (RE)");
                break;
            }
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC1:
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC2:
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC3:
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC4:
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC5:
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC6:
            case SDMMCIP_REG_RINTSTS_SDIO_FUNC7: {
                HAL_SDMMC_TRACE(0, "bit 16:funcX int");
                break;
            }
            default : {
                break;
            }
        }
    }
}

static void hal_sdmmc0_irq_handler(void)
{
    sdmmc_base_irq_handler(HAL_SDMMC_ID_0);
}

#ifdef SDMMC1_BASE
static void hal_sdmmc1_irq_handler(void)
{
    sdmmc_base_irq_handler(HAL_SDMMC_ID_1);
}
#endif

int hal_sdmmc_host_int_enable(enum HAL_SDMMC_ID_T id)
{
    uint32_t val;
    uint32_t irq_num = SDMMC0_IRQn;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    HAL_SDMMC_TRACE(2, "%s:%d", __func__, __LINE__);
    sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_ALL); //clear interrupt status

    val = ~(SDMMCIP_REG_INTMSK_CDONE | SDMMCIP_REG_INTMSK_ACD | SDMMCIP_REG_INTMSK_TXDR |
            SDMMCIP_REG_INTMSK_RXDR | SDMMCIP_REG_INTMSK_RTO | SDMMCIP_REG_INTMSK_SDIO_FUNC1 |
            SDMMCIP_REG_INTMSK_SDIO_FUNC2 | SDMMCIP_REG_INTMSK_SDIO_FUNC3 | SDMMCIP_REG_INTMSK_SDIO_FUNC4 |
            SDMMCIP_REG_INTMSK_SDIO_FUNC5 | SDMMCIP_REG_INTMSK_SDIO_FUNC6 | SDMMCIP_REG_INTMSK_SDIO_FUNC7);
    if (!sdmmc_callback[host->host_id]->hal_sdmmc_txrx_done) {
        val &= ~SDMMCIP_REG_RINTSTS_DTO;
    }
    sdmmc_ip_writel(host, SDMMCIP_REG_INTMASK, SDMMCIP_REG_INTMSK_ALL & val);   //open interrupt except for values in parentheses
    val = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
    val |= SDMMCIP_REG_INT_EN;
    sdmmc_ip_writel(host, SDMMCIP_REG_CTRL, val);                               //enable interrupt

    if (host->host_id == HAL_SDMMC_ID_0) {
        irq_num = SDMMC0_IRQn;
        NVIC_SetVector((IRQn_Type)irq_num, (uint32_t)hal_sdmmc0_irq_handler);
    }
#ifdef SDMMC1_BASE
    else if (host->host_id == HAL_SDMMC_ID_1) {
        irq_num = SDMMC1_IRQn;
        NVIC_SetVector((IRQn_Type)irq_num, (uint32_t)hal_sdmmc1_irq_handler);
    }
#endif

    NVIC_SetPriority((IRQn_Type)irq_num, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ((IRQn_Type)irq_num);
    NVIC_EnableIRQ((IRQn_Type)irq_num);

    return HAL_SDMMC_ERR_NONE;
}

int hal_sdmmc_host_int_disable(enum HAL_SDMMC_ID_T id)
{
    uint32_t val;
    uint32_t irq_num = SDMMC0_IRQn;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    HAL_SDMMC_TRACE(2, "%s:%d", __func__, __LINE__);
    val = sdmmc_ip_readl(host, SDMMCIP_REG_CTRL);
    val &= ~SDMMCIP_REG_INT_EN;
    sdmmc_ip_writel(host, SDMMCIP_REG_INTMASK, ~SDMMCIP_REG_INTMSK_ALL);
    sdmmc_ip_writel(host, SDMMCIP_REG_CTRL, val);                           //disable interrupt
    sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_ALL);    //clear interrupt status

    if (host->host_id == HAL_SDMMC_ID_0) {
        irq_num = SDMMC0_IRQn;
    }
#ifdef SDMMC1_BASE
    else if (host->host_id == HAL_SDMMC_ID_1) {
        irq_num = SDMMC1_IRQn;
    }
#endif

    NVIC_DisableIRQ((IRQn_Type)irq_num);
    NVIC_ClearPendingIRQ((IRQn_Type)irq_num);

    return HAL_SDMMC_ERR_NONE;
}

static int sdmmc_ip_init(struct mmc *mmc)
{
    struct sdmmc_ip_host *host = mmc->priv;

    sdmmc_ip_writel(host, SDMMCIP_REG_PWREN, 1);
    HAL_SDMMC_TRACE(2, "%s, host->ioaddr:0x%X, ip_pwr:%d", __func__, (uint32_t)host->ioaddr, sdmmc_ip_readl(host, SDMMCIP_REG_PWREN));

    if (sdmmc_ip_wait_reset(host, SDMMCIP_REG_RESET_ALL)) {
        HAL_SDMMC_ASSERT(0, "%s:%d,Fail-reset!!", __func__, __LINE__);
        return HAL_SDMMC_RESET_FAIL;
    }

    sdmmc_ip_writel(host, SDMMCIP_REG_INTMASK, ~SDMMCIP_REG_INTMSK_ALL); //disable all int
    sdmmc_ip_writel(host, SDMMCIP_REG_TMOUT, SDMMCIP_REG_RINTSTS_ALL);   //modify response timeout value for maximum
    sdmmc_ip_writel(host, SDMMCIP_REG_IDINTEN, 0);   //disable internal DMAC interrupt
    sdmmc_ip_writel(host, SDMMCIP_REG_BMOD, 1);      //software reset internal DMA controller
    sdmmc_ip_writel(host, SDMMCIP_REG_FIFOTH, host->fifoth_val);

    return HAL_SDMMC_ERR_NONE;
}

static int sdmmc_ip_wait_dat0(struct mmc *mmc, int state, int timeout_us)
{
    uint32_t val;
    struct sdmmc_ip_host *host = mmc->priv;
    uint32_t busy_timeout = US_TO_FAST_TICKS(timeout_us), busy_t;

    //HAL_SDMMC_TRACE(2, "timeout_us:%d,busy_timeout:%d", timeout_us, busy_timeout);
    busy_t = hal_fast_sys_timer_get();
    do {
        val = sdmmc_ip_readl(host, SDMMCIP_REG_STATUS);
        if (!(val & SDMMCIP_REG_DATA0_BUSY) ==  !!state) {
            return HAL_SDMMC_ERR_NONE;
        }
    } while (hal_fast_sys_timer_get() - busy_t < busy_timeout);

    return HAL_SDMMC_WAIT_DAT0_TIMEOUT;
}

static const struct mmc_ops sdmmc_ip_ops = {
    .send_cmd           = hal_sdmmc_ip_send_cmd,
    .set_ios_clock      = sdmmc_ip_set_ios_clock,
    .set_ios_width      = sdmmc_ip_set_ios_width,
    .init               = sdmmc_ip_init,
    .wait_dat0          = sdmmc_ip_wait_dat0,
    .getcd              = NULL,
    .getwp              = NULL,
    .host_power_cycle   = NULL,
    .get_b_max          = NULL,
};

static struct mmc *mmc_create(enum HAL_SDMMC_ID_T id, const struct mmc_config *cfg, void *priv)
{
    struct blk_desc *bdesc;
    struct mmc *mmc;

    /* quick validation */
    if (cfg == NULL || cfg->f_min == 0 ||
        cfg->f_max == 0 || cfg->b_max == 0)
        return NULL;

#ifndef DM_MMC
    if (cfg->ops == NULL || cfg->ops->send_cmd == NULL)
        return NULL;
#endif

    mmc = find_mmc_device(id);
    if (mmc == NULL)
        return NULL;

    mmc->cfg = cfg;
    mmc->priv = priv;
    mmc->has_sleep = 0;
    mmc->clk_disable = MMC_CLK_DISABLE;
    mmc->clk_disable_copy = mmc->clk_disable;

    /* the following chunk was mmc_register() */

    /* Setup dsr related values */
    mmc->dsr_imp = 0;
    mmc->dsr = 0xffffffff;
    /* Setup the universal parts of the block interface just once */
    bdesc = mmc_get_blk_desc(mmc);
    bdesc->if_type = IF_TYPE_MMC;
    bdesc->removable = 1;
    bdesc->devnum = id;
    bdesc->block_read = mmc_bread;
    bdesc->block_write = mmc_bwrite;
    bdesc->block_erase = mmc_berase;

    /* setup initial part type */
    bdesc->part_type = mmc->cfg->part_type;

    return mmc;
}

#ifdef CORE_SLEEP_POWER_DOWN
struct SAVED_SDMMC_REGS_T {
    uint32_t REG_00;
    uint32_t REG_04;
    uint32_t REG_08;
    uint32_t REG_0C;
    uint32_t REG_14;
    uint32_t REG_18;
    uint32_t REG_24;
    uint32_t REG_4C;
    uint32_t REG_74;
};
SRAM_BSS_LOC static struct SAVED_SDMMC_REGS_T saved_sdmmc_regs[HAL_SDMMC_ID_NUM];

static int sdmmc_base_pm_notif_handler(enum HAL_SDMMC_ID_T id, enum HAL_PM_STATE_T state)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    if (state == HAL_PM_STATE_POWER_DOWN_SLEEP) {
        //save digital side registers
        saved_sdmmc_regs[id].REG_00 = sdmmc_ip_readl(host, 0x00);
        saved_sdmmc_regs[id].REG_04 = sdmmc_ip_readl(host, 0x04);
        saved_sdmmc_regs[id].REG_08 = sdmmc_ip_readl(host, 0x08);
        saved_sdmmc_regs[id].REG_0C = sdmmc_ip_readl(host, 0x0C);
        saved_sdmmc_regs[id].REG_14 = sdmmc_ip_readl(host, 0x14);
        saved_sdmmc_regs[id].REG_18 = sdmmc_ip_readl(host, 0x18);
        saved_sdmmc_regs[id].REG_24 = sdmmc_ip_readl(host, 0x24);
        saved_sdmmc_regs[id].REG_4C = sdmmc_ip_readl(host, 0x4C);
        saved_sdmmc_regs[id].REG_74 = sdmmc_ip_readl(host, 0x74);
    } else if (state == HAL_PM_STATE_POWER_DOWN_WAKEUP) {
        sdmmc_ip_writel(host, 0x04, saved_sdmmc_regs[id].REG_04);//SDMMCIP_REG_PWREN
        if (sdmmc_ip_wait_reset(host, SDMMCIP_REG_RESET_ALL)) {
            if (sdmmc_ip_wait_reset(host, SDMMCIP_REG_RESET_ALL)) {
                HAL_SDMMC_ERROR(0, "%s failed", __func__);
            }
        }
        sdmmc_ip_setup_div(host, saved_sdmmc_regs[id].REG_08);   //SDMMCIP_REG_CLKDIV + SDMMCIP_REG_CLKENA
        sdmmc_ip_writel(host, 0x0C, saved_sdmmc_regs[id].REG_0C);//SDMMCIP_REG_CLKSRC
        sdmmc_ip_writel(host, 0x14, saved_sdmmc_regs[id].REG_14);//SDMMCIP_REG_TMOUT
        sdmmc_ip_writel(host, 0x18, saved_sdmmc_regs[id].REG_18);//SDMMCIP_REG_CTYPE
        sdmmc_ip_writel(host, 0x24, saved_sdmmc_regs[id].REG_24);//SDMMCIP_REG_INTMASK
        sdmmc_ip_writel(host, 0x4C, saved_sdmmc_regs[id].REG_4C);//SDMMCIP_REG_FIFOTH
        sdmmc_ip_writel(host, 0x74, saved_sdmmc_regs[id].REG_74);//SDMMCIP_REG_UHS_REG

        sdmmc_ip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_RINTSTS_ALL); //clear interrupt status
        sdmmc_ip_writel(host, 0x00, saved_sdmmc_regs[id].REG_00);
    }

    return 0;
}

static int sdmmc0_pm_notif_handler(enum HAL_PM_STATE_T state)
{
    return sdmmc_base_pm_notif_handler(HAL_SDMMC_ID_0, state);
}

#ifdef SDMMC1_BASE
static int sdmmc1_pm_notif_handler(enum HAL_PM_STATE_T state)
{
    return sdmmc_base_pm_notif_handler(HAL_SDMMC_ID_1, state);
}
#endif

static HAL_PM_NOTIF_HANDLER hal_sdmmc_pm_notif_handler[HAL_SDMMC_ID_NUM] = {
    sdmmc0_pm_notif_handler,
#ifdef SDMMC1_BASE
    sdmmc1_pm_notif_handler,
#endif
};
#endif

static enum HAL_SDMMC_ERR hal_sdmmc_host_init_base(enum HAL_SDMMC_ID_T id, const struct HAL_SDMMC_CONFIG_T *cfg, bool dma_en)
{
    int ret = HAL_SDMMC_ERR_NONE;
    uint32_t src_clk;
    uint32_t sdmmc_speed;
    struct sdmmc_ip_host *host = NULL;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, "Invalid sdmmc host id: %d", id);

    if (cfg && cfg->bus_speed) {
        sdmmc_speed = cfg->bus_speed;
    } else {
        sdmmc_speed = 12 * 1000 * 1000;
    }

#ifdef FPGA
    switch (id) {
        case HAL_SDMMC_ID_0:
            hal_cmu_sdmmc0_set_freq(HAL_CMU_PERIPH_FREQ_26M);
            break;
#ifdef SDMMC1_BASE
        case HAL_SDMMC_ID_1:
            hal_cmu_sdmmc1_set_freq(HAL_CMU_PERIPH_FREQ_26M);
            break;
#endif
        default:
            break;
    }

    src_clk = hal_cmu_get_crystal_freq();
    HAL_SDMMC_ASSERT(src_clk >= sdmmc_speed, "%s:%d, sdmmc_speed %d > src clk %d", __func__, __LINE__, sdmmc_speed, src_clk);
#else
    if (0) {
#ifdef PERIPH_PLL_FREQ
    } else if (sdmmc_speed > 2 * hal_cmu_get_crystal_freq()) {
        uint32_t div;//division is one step in place, the sdmmc ip is no longer divided(Support even division)

        div = PERIPH_PLL_FREQ / sdmmc_speed;
        if (PERIPH_PLL_FREQ % sdmmc_speed) {
            div += 1;
        }

        src_clk = PERIPH_PLL_FREQ / div;
        switch (id) {
            case HAL_SDMMC_ID_0:
                ret = hal_cmu_sdmmc0_set_div(div);
                break;
#ifdef SDMMC1_BASE
            case HAL_SDMMC_ID_1:
                ret = hal_cmu_sdmmc1_set_div(div);
                break;
#endif
            default:
                break;
        }
        HAL_SDMMC_ASSERT(!ret, "The sdmmc_speed value is invalid, causing the div to be out of range, ret %d, div is %d", ret, div);
        HAL_SDMMC_ASSERT(src_clk <= sdmmc_speed, "%s:%d, div clk %d > sdmmc_speed %d", __func__, __LINE__, src_clk, sdmmc_speed);
        HAL_SDMMC_TRACE(1, "PERIPH_PLL_FREQ is %d, the final PLL div is %d", PERIPH_PLL_FREQ, div);
#endif
    } else if (sdmmc_speed >= 3 * hal_cmu_get_crystal_freq() / 2) {  //sdmmc ip may also be divided(Only even division)
        switch (id) {
            case HAL_SDMMC_ID_0:
                hal_cmu_sdmmc0_set_freq(HAL_CMU_PERIPH_FREQ_52M);
                break;
#ifdef SDMMC1_BASE
            case HAL_SDMMC_ID_1:
                hal_cmu_sdmmc1_set_freq(HAL_CMU_PERIPH_FREQ_52M);
                break;
#endif
            default:
                break;
        }
        src_clk = 2 * hal_cmu_get_crystal_freq();
        HAL_SDMMC_ASSERT(src_clk >= sdmmc_speed, "%s:%d, sdmmc_speed %d > src clk %d", __func__, __LINE__, sdmmc_speed, src_clk);
    } else {                                                        //sdmmc ip may also be divided(Only even division)
        switch (id) {
            case HAL_SDMMC_ID_0:
                hal_cmu_sdmmc0_set_freq(HAL_CMU_PERIPH_FREQ_26M);
                break;
#ifdef SDMMC1_BASE
            case HAL_SDMMC_ID_1:
                hal_cmu_sdmmc1_set_freq(HAL_CMU_PERIPH_FREQ_26M);
                break;
#endif
            default:
                break;
        }
        src_clk = hal_cmu_get_crystal_freq();
        HAL_SDMMC_ASSERT(src_clk >= sdmmc_speed, "%s:%d, sdmmc_speed %d > src clk %d", __func__, __LINE__, sdmmc_speed, src_clk);
    }
#endif
    HAL_SDMMC_TRACE(2, "sdmmc_speed %d, bus clk(Not necessarily the final speed) %d", sdmmc_speed, src_clk);

    /* sdmmc host clock and iomux */
    switch (id) {
        case HAL_SDMMC_ID_0:
            hal_iomux_set_sdmmc0();
            hal_cmu_sdmmc0_clock_enable();
            break;
#ifdef SDMMC1_BASE
        case HAL_SDMMC_ID_1:
            hal_iomux_set_sdmmc1();
            hal_cmu_sdmmc1_clock_enable();
            break;
#endif
        default:
            break;
    }

    if (cfg && cfg->ddr_mode) {
        POSSIBLY_UNUSED uint32_t freq_max;
        freq_max = mmc_mode2freq(NULL, MMC_DDR_52);
        HAL_SDMMC_ASSERT(sdmmc_speed <= freq_max, "%s:%d, mmc ddr50 speed %d > max speed %d", __func__, __LINE__, sdmmc_speed, freq_max);
    }

    host = &sdmmc_host[id];
    host->host_id    = id;
    host->dma_en     = dma_en;
    host->src_clk_hz = src_clk;
    host->ioaddr     = (void *)sdmmc_ip_base[id];
    host->desired_bus_speed = 0;
    if (cfg && cfg->bus_width) {
        host->buswidth   = cfg->bus_width;
    } else {
        host->buswidth   = HAL_SDMMC_BUS_WIDTH_4;
    }
    if (cfg) {
        host->volt_switch = cfg->volt_switch;
        host->yield = cfg->yield;
    } else {
        host->volt_switch = 0;
        host->yield = 0;
    }
    host->volt_switch_flag = 0;
    host->fifoth_val = MSIZE(0) | RX_WMARK(0) | TX_WMARK(1);

    host->cfg.ops = &sdmmc_ip_ops;
    host->cfg.voltages = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_165_195;
    host->cfg.f_min = 400 * 1000;     //Don't modify
    host->cfg.f_max = sdmmc_speed;
    host->cfg.b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;
    host->mmc = mmc_create(host->host_id, &host->cfg, host);
    HAL_SDMMC_TRACE(4, "%s:%d, sdmmc clk min %d, max %d", __func__, __LINE__, host->cfg.f_min, host->cfg.f_max);

    if (host->dma_en) {
        host->dma_ch = 0;
        host->dma_in_use = 0;
        host->sdmmc_dma_lock = 0;
        host->tx_dma_handler = sdmmc_ip_ext_dma_irq_handlers[id * 2];
        host->rx_dma_handler = sdmmc_ip_ext_dma_irq_handlers[id * 2 + 1];
        host->fifo_timer = NULL;
    } else {
        host->fifo_timer = hwtimer_alloc((HWTIMER_CALLBACK_T)fifo_xfer_timeout, host);
    }

    host->cfg.host_caps = 0;
    if (host->buswidth == 8) {
        host->cfg.host_caps |= MMC_MODE_4BIT | MMC_MODE_8BIT;
    } else if (host->buswidth == 4) {
        host->cfg.host_caps |= MMC_MODE_4BIT;
    } else {
        host->cfg.host_caps |= MMC_MODE_4BIT;
    }

    if (cfg && cfg->ddr_mode) {
        //The controller does not support this mode, 2022-06-09
        //host->cfg.host_caps |= MMC_MODE_DDR_52MHz;
    }

    if (cfg) {
        if (cfg->device_type == HAL_SDMMC_DEVICE_TYPE_SD) {
            if (host->volt_switch) {
                host->cfg.host_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz | UHS_CAPS_MINI;//max clk: 96M, sdr50
            } else {
                host->cfg.host_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz;//max clk: 48M, sdr25
            }
        } else {
            host->cfg.host_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz | UHS_CAPS_MINI;//max clk: 48M, sdr52
        }
    } else {
        //the default configuration of caps shared by emmc and sd card
        host->cfg.host_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz;//max clk: 48M
    }

    sdmmc_callback[id] = &sdmmc_callback_default[id];
    memset(sdmmc_callback[id], 0, sizeof(struct HAL_SDMMC_CB_T));
    HAL_SDMMC_TRACE(0, "default callback[%d] addr:0x%X", id, (uint32_t)sdmmc_callback[id]);
    if (cfg && cfg->callback) {
        HAL_SDMMC_TRACE(0, "new callback addr:0x%X", (uint32_t)cfg->callback);
        sdmmc_callback[id] = cfg->callback;
    }
    host->callback = sdmmc_callback[id];

    /* made sure it's not NULL earlier */
    ret = host->mmc->cfg->ops->init(host->mmc);
    if (ret) {
        HAL_SDMMC_ERROR(3, "%s:%d, sdmmc ip init error,ret=%d", __func__, __LINE__, ret);
        return ret;
    } else {
        HAL_SDMMC_TRACE(3, "%s:%d, sdmmc ip init ok,ret=%d", __func__, __LINE__, ret);
    }
    hal_sdmmc_host_int_enable(id);

#ifdef CORE_SLEEP_POWER_DOWN
    hal_pm_notif_register(HAL_PM_USER_HAL, hal_sdmmc_pm_notif_handler[id]);
#endif

    return ret;
}

static enum HAL_SDMMC_ERR hal_sdmmc_device_init_base(enum HAL_SDMMC_ID_T id, struct sdmmc_ip_host *host, bool device_init)
{
    int ret = HAL_SDMMC_ERR_NONE;

    if (host->mmc->has_init) {
        HAL_SDMMC_ERROR(2, "%s:%d, sdmmc device has been initialized", __func__, __LINE__);
        return ret;
    }
    if (!host->mmc->init_in_progress) {
        host->mmc->init_in_progress = 1;
        if (device_init) {
            mmc_set_preinit(host->mmc, 1);
            if (mmc_initialize(host->mmc)) {
                HAL_SDMMC_ERROR(1, "%s failed", __func__);
            }
            ret = mmc_init(host->mmc);
            if (ret) {
                return ret;
            }
        } else {
            //config host:UHS-I mode
            hal_sdmmc_set_bus_width(host->mmc, HAL_SDMMC_BUS_WIDTH_4);
            hal_sdmmc_set_clock(host->mmc, host->mmc->cfg->f_max, false);

            //Disable the function of stopping output when clk is idle
            sdmmc_ip_clk_disable_low_power(host);
            ret = HAL_SDMMC_ERR_NONE;
        }
        HAL_SDMMC_TRACE(2, "%s:%d, sdmmc device init complete", __func__, __LINE__);
        HAL_SDMMC_TRACE(0, "  ");
        HAL_SDMMC_TRACE(0, "  ");
    } else {
        ret = HAL_SDMMC_IN_PROGRESS_ERR;
        HAL_SDMMC_ERROR(2, "%s:%d, sdmmc device is being initialized", __func__, __LINE__);
    }

    return ret;
}

enum HAL_SDMMC_ERR hal_sdmmc_host_init(enum HAL_SDMMC_ID_T id, const struct HAL_SDMMC_CONFIG_T *cfg)
{
    return hal_sdmmc_host_init_base(id, cfg, 1);
}

enum HAL_SDMMC_ERR hal_sdmmc_device_init(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = NULL;
    host = &sdmmc_host[id];

    return hal_sdmmc_device_init_base(id, host, 1);
}

static enum HAL_SDMMC_ERR hal_sdmmc_host_device_init(enum HAL_SDMMC_ID_T id,
                                                    const struct HAL_SDMMC_CONFIG_T *cfg,
                                                    bool dma_en, bool device_init)
{
    struct sdmmc_ip_host *host = NULL;
    host = &sdmmc_host[id];
    enum HAL_SDMMC_ERR ret;

    ret = hal_sdmmc_host_init_base(id, cfg, dma_en);
    if (ret) {
        return ret;
    }
    return hal_sdmmc_device_init_base(id, host, device_init);
}

/*************************external interface function**************************/
POSSIBLY_UNUSED static const char *const invalid_id = "Invalid sdmmc id: %d";

HAL_SDMMC_DELAY_FUNC hal_sdmmc_set_delay_func(HAL_SDMMC_DELAY_FUNC new_func)
{
    HAL_SDMMC_DELAY_FUNC old_func = sdmmc_delay_ms;
    sdmmc_delay_ms = new_func;
    return old_func;
}

HAL_SDMMC_DELAY_US_FUNC hal_sdmmc_set_delay_us_func(HAL_SDMMC_DELAY_US_FUNC new_func)
{
    HAL_SDMMC_DELAY_US_FUNC old_func = sdmmc_delay_us;
    sdmmc_delay_us = new_func;
    return old_func;
}

uint32_t hal_sdmmc_read_blocks(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t *dest)
{
    struct mmc *mmc = sdmmc_host[id].mmc;

    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);
    HAL_SDMMC_ASSERT((((uint64_t)(block_count * MMC_MAX_BLOCK_LEN)) / SDMMC_DMA_LINK_SIZE) < (SDMMC_DMA_DESC_CNT - 1), "%s:%d,SDMMC_DMA_DESC_CNT is too small", __func__, __LINE__);

    sdmmc_host[id].dma_en = 1;
    return (uint32_t)mmc->block_dev.block_read(&mmc->block_dev, start_block, block_count, dest);
}

uint32_t hal_sdmmc_write_blocks(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t *src)
{
    struct mmc *mmc = sdmmc_host[id].mmc;

    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);
    HAL_SDMMC_ASSERT((((uint64_t)(block_count * MMC_MAX_BLOCK_LEN)) / SDMMC_DMA_LINK_SIZE) < (SDMMC_DMA_DESC_CNT - 1), "%s:%d,SDMMC_DMA_DESC_CNT is too small", __func__, __LINE__);

    sdmmc_host[id].dma_en = 1;
    return (uint32_t)mmc->block_dev.block_write(&mmc->block_dev, start_block, block_count, src);
}

uint32_t hal_sdmmc_read_blocks_dma(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t *dest)
{
    return hal_sdmmc_read_blocks(id, start_block, block_count, dest);
}

uint32_t hal_sdmmc_write_blocks_dma(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t *src)
{
    return hal_sdmmc_write_blocks(id, start_block, block_count, src);
}

uint32_t hal_sdmmc_read_blocks_polling(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t *dest)
{
    struct mmc *mmc = sdmmc_host[id].mmc;

    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    sdmmc_host[id].dma_en = 0;
    return (uint32_t)mmc->block_dev.block_read(&mmc->block_dev, start_block, block_count, dest);
}

uint32_t hal_sdmmc_write_blocks_polling(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t *src)
{
    struct mmc *mmc = sdmmc_host[id].mmc;

    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    sdmmc_host[id].dma_en = 0;
    return (uint32_t)mmc->block_dev.block_write(&mmc->block_dev, start_block, block_count, src);
}

uint32_t hal_sdmmc_erase(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count)
{
    struct mmc *mmc = sdmmc_host[id].mmc;

    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);
    HAL_SDMMC_ASSERT((((uint64_t)(block_count * MMC_MAX_BLOCK_LEN)) / SDMMC_DMA_LINK_SIZE) < (SDMMC_DMA_DESC_CNT - 1), "%s:%d,SDMMC_DMA_DESC_CNT is too small", __func__, __LINE__);

    sdmmc_host[id].dma_en = 1;
    return (uint32_t)mmc->block_dev.block_erase(&mmc->block_dev, start_block, block_count);
}

uint32_t hal_sdmmc_erase_dma(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count)
{
    return hal_sdmmc_erase(id, start_block, block_count);
}

uint32_t hal_sdmmc_erase_polling(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count)
{
    struct mmc *mmc = sdmmc_host[id].mmc;

    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    sdmmc_host[id].dma_en = 0;
    return (uint32_t)mmc->block_dev.block_erase(&mmc->block_dev, start_block, block_count);
}

enum HAL_SDMMC_ERR hal_sdmmc_open(enum HAL_SDMMC_ID_T id, const struct HAL_SDMMC_CONFIG_T *cfg)
{
    return (enum HAL_SDMMC_ERR)hal_sdmmc_host_device_init(id, cfg, 1, 1);
}

enum HAL_SDMMC_ERR hal_sdmmc_open_ext(enum HAL_SDMMC_ID_T id, struct HAL_SDMMC_CONFIG_T *cfg, struct HAL_SDMMC_CB_T *callback)
{
    cfg->callback = callback;
    return (enum HAL_SDMMC_ERR)hal_sdmmc_open(id, cfg);
}

void hal_sdmmc_close(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = NULL;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, "Invalid sdmmc host id: %d", id);
    HAL_SDMMC_TRACE(2, "%s:%d", __func__, __LINE__);

    host = &sdmmc_host[id];
    if (!host->mmc) {
        return;
    }

    if (!host->mmc->has_init) {
        HAL_SDMMC_ERROR(2, "%s:%d, sdmmc host has been closed", __func__, __LINE__);
        return;
    }

#ifdef CORE_SLEEP_POWER_DOWN
    hal_pm_notif_deregister(HAL_PM_USER_HAL, hal_sdmmc_pm_notif_handler[id]);
#endif

    while (host->mmc->init_in_progress)
        hal_sys_timer_delay_us(500);

    host->mmc->has_init = 0;

#if defined(MMC_UHS_SUPPORT) || \
    defined(MMC_HS200_SUPPORT) || \
    defined(MMC_HS400_SUPPORT)
    mmc_deinit(host->mmc);
#endif

    if (host->dma_en) {
        if (host->dma_in_use) {
            sdmmc_ip_reset_dma(host);
        }
    } else {
        hwtimer_free(host->fifo_timer);
    }

    hal_sdmmc_host_int_disable(id);
    switch (id) {
        case HAL_SDMMC_ID_0:
            hal_cmu_sdmmc0_clock_disable();
            hal_iomux_clear_sdmmc0();
            break;
#ifdef SDMMC1_BASE
        case HAL_SDMMC_ID_1:
            hal_cmu_sdmmc1_clock_disable();
            hal_iomux_clear_sdmmc1();
            break;
#endif
        default:
            break;
    }
}

enum HAL_SDMMC_STATUS hal_sdmmc_get_init_status(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = NULL;

    host = &sdmmc_host[id];
    if (!host->mmc) {
        return HAL_SDMMC_STATUS_UNKNOWN;
    }

    return (enum HAL_SDMMC_STATUS)host->mmc->has_init;
}

uint32_t hal_sdmmc_get_bus_speed(enum HAL_SDMMC_ID_T id)
{
    if (hal_sdmmc_get_init_status(id) == HAL_SDMMC_STATUS_HAS_INIT) {
        return sdmmc_host[id].final_bus_speed;
    } else {
        return 0;
    }
}

void hal_sdmmc_dump(enum HAL_SDMMC_ID_T id)
{
    struct mmc *mmc = sdmmc_host[id].mmc;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    TRACE(0, "-----------hal_sdmmc_dump-------------");
    TRACE(1, "[sdmmc id]                       : %d", id);
    TRACE(1, "[io register address]            : 0x%X", sdmmc_ip_base[id]);
    TRACE(1, "[ddr mode]                       : %d", mmc->ddr_mode);
    switch (mmc->version) {
        case SD_VERSION_1_0:
            TRACE(0, "[version]                        : SD_VERSION_1_0");
            break;
        case SD_VERSION_1_10:
            TRACE(0, "[version]                        : SD_VERSION_1_10");
            break;
        case SD_VERSION_2:
            TRACE(0, "[version]                        : SD_VERSION_2");
            break;
        case SD_VERSION_3:
            TRACE(0, "[version]                        : SD_VERSION_3");
            break;
        case MMC_VERSION_5_1:
            TRACE(0, "[version]                        : MMC_VERSION_5_1");
            break;
        case MMC_VERSION_5_0:
            TRACE(0, "[version]                        : MMC_VERSION_5_0");
            break;
        case MMC_VERSION_4_5:
            TRACE(0, "[version]                        : MMC_VERSION_4_5");
            break;
        case MMC_VERSION_4_41:
            TRACE(0, "[version]                        : MMC_VERSION_4_41");
            break;
        case MMC_VERSION_4_4:
            TRACE(0, "[version]                        : MMC_VERSION_4_4");
            break;
        case MMC_VERSION_4_3:
            TRACE(0, "[version]                        : MMC_VERSION_4_3");
            break;
        case MMC_VERSION_4_2:
            TRACE(0, "[version]                        : MMC_VERSION_4_2");
            break;
        case MMC_VERSION_4_1:
            TRACE(0, "[version]                        : MMC_VERSION_4_1");
            break;
        case MMC_VERSION_4:
            TRACE(0, "[version]                        : MMC_VERSION_4");
            break;
        case MMC_VERSION_3:
            TRACE(0, "[version]                        : MMC_VERSION_3");
            break;
        case MMC_VERSION_2_2:
            TRACE(0, "[version]                        : MMC_VERSION_2_2");
            break;
        case MMC_VERSION_1_4:
            TRACE(0, "[version]                        : MMC_VERSION_1_4");
            break;
        case MMC_VERSION_1_2:
            TRACE(0, "[version]                        : MMC_VERSION_1_2");
            break;
        default:
            TRACE(0, "[version]                        : unkown version");
            break;
    }
    TRACE(1, "[is SD card]                     : 0x%X", IS_SD(mmc));
    TRACE(1, "[high_capacity]                  : 0x%X", mmc->high_capacity);
    TRACE(1, "[bus_width]                      : 0x%X", mmc->bus_width);
    TRACE(1, "[clock]                          : %d", mmc->clock);
    TRACE(1, "[card_caps]                      : 0x%X", mmc->card_caps);
    TRACE(1, "[ocr]                            : 0x%X", mmc->ocr);
    TRACE(1, "[dsr]                            : 0x%X", mmc->dsr);
    TRACE(TR_ATTR_NO_LF, "[capacity_user]                  : 0x%X", (uint32_t)(mmc->capacity >> 32)); //high 4bytes
    TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "%X", (uint32_t)(mmc->capacity));                            //low 4bytes
    uint32_t capacity_user = (uint32_t)(mmc->capacity_user / 1024 / 1024);
    TRACE(1, "[capacity_user/1024/1024/1024]   : %d.%dGB", capacity_user / 1024, capacity_user * 100 / 1024 % 100);
    TRACE(1, "[read_bl_len]                    : %d", mmc->read_bl_len);
    TRACE(1, "[write_bl_len]                   : %d", mmc->write_bl_len);
    TRACE(0, "--------------------------------------");
}

void hal_sdmmc_info(enum HAL_SDMMC_ID_T id, uint32_t *sector_count, uint32_t *sector_size)
{
    struct mmc *mmc = sdmmc_host[id].mmc;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    if (sector_size) {
        *sector_size = mmc->read_bl_len;
        HAL_SDMMC_TRACE(1, "[sdmmc] sector_size %d", *sector_size);
    }
    if (sector_count) {
        if (mmc->read_bl_len != 0)
            *sector_count = mmc->capacity_user / mmc->read_bl_len;
        else
            *sector_count = 0;
        HAL_SDMMC_TRACE(1, "[sdmmc] sector_count 0x%X", *sector_count);
    }
}

#define mmc_unpack_bits(resp,start,size)                                    \
    ({                                                                      \
        const          int __size = size;                                   \
        const unsigned int __mask = (__size < 32 ? 1 << __size : 0) - 1;    \
        const          int __off = 3 - ((start) / 32);                      \
        const          int __shft = (start) & 31;                           \
        unsigned int   __res;                                               \
                                                                            \
        __res = resp[__off] >> __shft;                                      \
        if (__size + __shft > 32)                                           \
            __res |= resp[__off-1] << ((32 - __shft) % 32);                 \
        __res & __mask;                                                     \
    })

static void mmc_unpack_cid(struct sdmmc_ip_host *host, struct HAL_SDMMC_CARD_INFO_T *card_info, uint32_t *resp)
{
    if (IS_MMC(host->mmc)) {
        card_info->cid.mid    = mmc_unpack_bits(resp, 112 + 8,  8);
        card_info->cid.cbx    = mmc_unpack_bits(resp, 104 + 8,  8);
        card_info->cid.oid    = mmc_unpack_bits(resp,  96 + 8,  8);
        card_info->cid.pnm[0] = mmc_unpack_bits(resp,  88 + 8,  8);
        card_info->cid.pnm[1] = mmc_unpack_bits(resp,  80 + 8,  8);
        card_info->cid.pnm[2] = mmc_unpack_bits(resp,  72 + 8,  8);
        card_info->cid.pnm[3] = mmc_unpack_bits(resp,  64 + 8,  8);
        card_info->cid.pnm[4] = mmc_unpack_bits(resp,  56 + 8,  8);
        card_info->cid.pnm[5] = mmc_unpack_bits(resp,  48 + 8,  8);
        card_info->cid.prv    = mmc_unpack_bits(resp,  40 + 8,  8);
        card_info->cid.psn    = mmc_unpack_bits(resp,   8 + 8, 32);
        card_info->cid.mdt    = mmc_unpack_bits(resp,   0 + 8,  8);
    } else {
        card_info->cid.mid    = mmc_unpack_bits(resp, 112 + 8,  8);
        card_info->cid.oid    = mmc_unpack_bits(resp,  96 + 8, 16);
        card_info->cid.pnm[0] = mmc_unpack_bits(resp,  88 + 8,  8);
        card_info->cid.pnm[1] = mmc_unpack_bits(resp,  80 + 8,  8);
        card_info->cid.pnm[2] = mmc_unpack_bits(resp,  72 + 8,  8);
        card_info->cid.pnm[3] = mmc_unpack_bits(resp,  64 + 8,  8);
        card_info->cid.pnm[4] = mmc_unpack_bits(resp,  56 + 8,  8);
        card_info->cid.prv    = mmc_unpack_bits(resp,  48 + 8,  8);
        card_info->cid.psn    = mmc_unpack_bits(resp,  16 + 8, 32);
        card_info->cid.mdt    = mmc_unpack_bits(resp,   0 + 8, 12);
    }
}

static void mmc_unpack_csd(struct sdmmc_ip_host *host, struct HAL_SDMMC_CARD_INFO_T *card_info, uint32_t *resp)
{
    if (IS_MMC(host->mmc)) {
        card_info->csd.csd_structure      = mmc_unpack_bits(resp, 118 + 8,  2);
        card_info->csd.spec_vers          = mmc_unpack_bits(resp, 114 + 8,  4);
        card_info->csd.taac               = mmc_unpack_bits(resp, 104 + 8,  8);
        card_info->csd.nsac               = mmc_unpack_bits(resp,  96 + 8,  8);
        card_info->csd.tran_speed         = mmc_unpack_bits(resp,  88 + 8,  8);
        card_info->csd.ccc                = mmc_unpack_bits(resp,  76 + 8, 12);
        card_info->csd.read_bl_len        = mmc_unpack_bits(resp,  72 + 8,  4);
        card_info->csd.read_bl_partial    = mmc_unpack_bits(resp,  71 + 8,  1);
        card_info->csd.write_blk_misalign = mmc_unpack_bits(resp,  70 + 8,  1);
        card_info->csd.read_blk_misalign  = mmc_unpack_bits(resp,  69 + 8,  1);
        card_info->csd.dsr_imp            = mmc_unpack_bits(resp,  68 + 8,  1);
        card_info->csd.c_size             = mmc_unpack_bits(resp,  54 + 8, 12);
        card_info->csd.vdd_r_curr_min     = mmc_unpack_bits(resp,  51 + 8,  3);
        card_info->csd.vdd_r_curr_max     = mmc_unpack_bits(resp,  48 + 8,  3);
        card_info->csd.vdd_w_curr_min     = mmc_unpack_bits(resp,  45 + 8,  3);
        card_info->csd.vdd_w_curr_max     = mmc_unpack_bits(resp,  42 + 8,  3);
        card_info->csd.c_size_mult        = mmc_unpack_bits(resp,  39 + 8,  3);
        card_info->csd.erase_grp_size     = mmc_unpack_bits(resp,  34 + 8,  5);
        card_info->csd.erase_grp_mult     = mmc_unpack_bits(resp,  29 + 8,  5);
        card_info->csd.wp_grp_size        = mmc_unpack_bits(resp,  24 + 8,  5);
        card_info->csd.wp_grp_enable      = mmc_unpack_bits(resp,  23 + 8,  1);
        card_info->csd.default_ecc        = mmc_unpack_bits(resp,  21 + 8,  2);
        card_info->csd.r2w_factor         = mmc_unpack_bits(resp,  18 + 8,  3);
        card_info->csd.write_bl_len       = mmc_unpack_bits(resp,  14 + 8,  4);
        card_info->csd.write_bl_partial   = mmc_unpack_bits(resp,  13 + 8,  1);
        card_info->csd.content_prot_app   = mmc_unpack_bits(resp,   8 + 8,  1);
        card_info->csd.file_format_grp    = mmc_unpack_bits(resp,   7 + 8,  1);
        card_info->csd.copy               = mmc_unpack_bits(resp,   6 + 8,  1);
        card_info->csd.perm_write_protect = mmc_unpack_bits(resp,   5 + 8,  1);
        card_info->csd.tmp_write_protect  = mmc_unpack_bits(resp,   4 + 8,  1);
        card_info->csd.file_format        = mmc_unpack_bits(resp,   2 + 8,  2);
        card_info->csd.ecc                = mmc_unpack_bits(resp,   0 + 8,  2);
    } else {
        card_info->csd.csd_structure      = mmc_unpack_bits(resp, 118 + 8,  2);
        card_info->csd.taac               = mmc_unpack_bits(resp, 104 + 8,  8);
        card_info->csd.nsac               = mmc_unpack_bits(resp,  96 + 8,  8);
        card_info->csd.tran_speed         = mmc_unpack_bits(resp,  88 + 8,  8);
        card_info->csd.ccc                = mmc_unpack_bits(resp,  76 + 8, 12);
        card_info->csd.read_bl_len        = mmc_unpack_bits(resp,  72 + 8,  4);
        card_info->csd.read_bl_partial    = mmc_unpack_bits(resp,  71 + 8,  1);
        card_info->csd.write_blk_misalign = mmc_unpack_bits(resp,  70 + 8,  1);
        card_info->csd.read_blk_misalign  = mmc_unpack_bits(resp,  69 + 8,  1);
        card_info->csd.dsr_imp            = mmc_unpack_bits(resp,  68 + 8,  1);
        if (card_info->csd.csd_structure == 0) {        //V1.0
            card_info->csd.c_size         = mmc_unpack_bits(resp,  54 + 8, 12);
            card_info->csd.vdd_r_curr_min = mmc_unpack_bits(resp,  51 + 8,  3);
            card_info->csd.vdd_r_curr_max = mmc_unpack_bits(resp,  48 + 8,  3);
            card_info->csd.vdd_w_curr_min = mmc_unpack_bits(resp,  45 + 8,  3);
            card_info->csd.vdd_w_curr_max = mmc_unpack_bits(resp,  42 + 8,  3);
            card_info->csd.c_size_mult    = mmc_unpack_bits(resp,  39 + 8,  3);
        } else if (card_info->csd.csd_structure == 1) { //V2.0
            card_info->csd.c_size         = mmc_unpack_bits(resp,  40 + 8, 22); //69~48
        } else if (card_info->csd.csd_structure == 2) { //V3.0
            card_info->csd.c_size         = mmc_unpack_bits(resp,  40 + 8, 28); //75~48
        }
        card_info->csd.erase_blk_en       = mmc_unpack_bits(resp,  38 + 8,  1);
        card_info->csd.sector_size        = mmc_unpack_bits(resp,  31 + 8,  7);
        card_info->csd.wp_grp_size        = mmc_unpack_bits(resp,  24 + 8,  7);
        card_info->csd.wp_grp_enable      = mmc_unpack_bits(resp,  23 + 8,  1);
        card_info->csd.r2w_factor         = mmc_unpack_bits(resp,  18 + 8,  3);
        card_info->csd.write_bl_len       = mmc_unpack_bits(resp,  14 + 8,  4);
        card_info->csd.write_bl_partial   = mmc_unpack_bits(resp,  13 + 8,  1);
        card_info->csd.file_format_grp    = mmc_unpack_bits(resp,   7 + 8,  1);
        card_info->csd.copy               = mmc_unpack_bits(resp,   6 + 8,  1);
        card_info->csd.perm_write_protect = mmc_unpack_bits(resp,   5 + 8,  1);
        card_info->csd.tmp_write_protect  = mmc_unpack_bits(resp,   4 + 8,  1);
        card_info->csd.file_format        = mmc_unpack_bits(resp,   2 + 8,  2);
    }
}

void hal_sdmmc_dump_card_info(struct HAL_SDMMC_CARD_INFO_T *card_info)
{
    if (card_info) {
        TRACE(0, "********************************************");
        TRACE(1, "card sort                : %d(%s)", card_info->cardsort, card_info->cardsort ? "SD card" : "MMC card");
        TRACE(1, "card capacity            : %d(%s)", card_info->high_capacity, card_info->high_capacity ? "high capacity" : "low capacity");
        TRACE(1, "card type                : 0x%X", card_info->cardtype);
        TRACE(1, "card class               : 0x%X", card_info->cardclass);
        TRACE(1, "card relcardAdd          : 0x%X", card_info->relcardadd);
        TRACE(1, "card blocknbr            : 0x%X", card_info->blocknbr);
        TRACE(1, "card blocksize           : 0x%X", card_info->blocksize);
        TRACE(1, "card logblocknbr         : 0x%X", card_info->logblocknbr);
        TRACE(1, "card logllocksize        : 0x%X", card_info->logllocksize);
        TRACE(1, "card ext_csd_rev         : 0x%X", card_info->ext_csd_rev);
        TRACE(1, "card firmware version_h32: 0x%X", card_info->firmware_version_h32);
        TRACE(1, "card firmware version_l32: 0x%X", card_info->firmware_version);
        TRACE(1, "card device version      : 0x%X", card_info->device_version);
        TRACE(1, "card cache size          : 0x%X", card_info->cache_size);
        TRACE(1, "card cache ctrl          : %s", card_info->cache_ctrl ? "1(enabled)" : "0(disable)");
        TRACE(1, "card cache flush policy  : %s", card_info->cache_flush_policy ? "1(fifo mode)" : "0(unknown)");
        TRACE(1, "csd.csd_structure        : 0x%X", card_info->csd.csd_structure);
        TRACE(1, "csd.spec_vers            : 0x%X", card_info->csd.spec_vers);
        TRACE(1, "csd.taac                 : 0x%X", card_info->csd.taac);
        TRACE(1, "csd.nsac                 : 0x%X", card_info->csd.nsac);
        TRACE(1, "csd.tran_speed           : 0x%X", card_info->csd.tran_speed);
        TRACE(1, "csd.ccc                  : 0x%X", card_info->csd.ccc);
        TRACE(1, "csd.read_bl_len          : 0x%X", card_info->csd.read_bl_len);
        TRACE(1, "csd.read_bl_partial      : 0x%X", card_info->csd.read_bl_partial);
        TRACE(1, "csd.write_blk_misalign   : 0x%X", card_info->csd.write_blk_misalign);
        TRACE(1, "csd.read_blk_misalign    : 0x%X", card_info->csd.read_blk_misalign);
        TRACE(1, "csd.dsr_imp              : 0x%X", card_info->csd.dsr_imp);
        TRACE(1, "csd.c_size               : 0x%X", card_info->csd.c_size);
        TRACE(1, "csd.vdd_r_curr_min       : 0x%X", card_info->csd.vdd_r_curr_min);
        TRACE(1, "csd.vdd_r_curr_max       : 0x%X", card_info->csd.vdd_r_curr_max);
        TRACE(1, "csd.vdd_w_curr_min       : 0x%X", card_info->csd.vdd_w_curr_min);
        TRACE(1, "csd.vdd_w_curr_max       : 0x%X", card_info->csd.vdd_w_curr_max);
        TRACE(1, "csd.c_size_mult          : 0x%X", card_info->csd.c_size_mult);
        TRACE(1, "csd.erase_grp_size       : 0x%X", card_info->csd.erase_grp_size);
        TRACE(1, "csd.erase_grp_mult       : 0x%X", card_info->csd.erase_grp_mult);
        TRACE(1, "csd.wp_grp_size          : 0x%X", card_info->csd.wp_grp_size);
        TRACE(1, "csd.wp_grp_enable        : 0x%X", card_info->csd.wp_grp_enable);
        TRACE(1, "csd.default_ecc          : 0x%X", card_info->csd.default_ecc);
        TRACE(1, "csd.r2w_factor           : 0x%X", card_info->csd.r2w_factor);
        TRACE(1, "csd.write_bl_len         : 0x%X", card_info->csd.write_bl_len);
        TRACE(1, "csd.write_bl_partial     : 0x%X", card_info->csd.write_bl_partial);
        TRACE(1, "csd.content_prot_app     : 0x%X", card_info->csd.content_prot_app);
        TRACE(1, "csd.file_format_grp      : 0x%X", card_info->csd.file_format_grp);
        TRACE(1, "csd.copy                 : 0x%X", card_info->csd.copy);
        TRACE(1, "csd.perm_write_protect   : 0x%X", card_info->csd.perm_write_protect);
        TRACE(1, "csd.tmp_write_protect    : 0x%X", card_info->csd.tmp_write_protect);
        TRACE(1, "csd.file_format          : 0x%X", card_info->csd.file_format);
        TRACE(1, "csd.ecc                  : 0x%X", card_info->csd.ecc);

        TRACE(1, "Manufacturer ID          : 0x%X", card_info->cid.mid);
        if (!card_info->cardsort) {
            TRACE(1, "OEM/Application ID       : 0x%X", card_info->cid.oid & 0xFF);
            TRACE(1, "Device/BGA               : 0x%X", card_info->cid.cbx);
            TRACE(1, "Product Name             : %c%c%c%c%c%c", card_info->cid.pnm[0],
                                                                card_info->cid.pnm[1],
                                                                card_info->cid.pnm[2],
                                                                card_info->cid.pnm[3],
                                                                card_info->cid.pnm[4],
                                                                (card_info->cid.pnm[5] & 0x7E) < 0x20 ? ' ' : (card_info->cid.pnm[5] & 0x7E)); //character range
        } else if (card_info->cardsort) {
            TRACE(1, "OEM/Application ID       : 0x%X", card_info->cid.oid);
            TRACE(1, "Product Name             : %c%c%c%c%c",   card_info->cid.pnm[0],
                                                                card_info->cid.pnm[1],
                                                                card_info->cid.pnm[2],
                                                                card_info->cid.pnm[3],
                                                                (card_info->cid.pnm[4] & 0x7E) < 0x20 ? ' ' : (card_info->cid.pnm[4] & 0x7E)); //character range
        }
        TRACE(1, "Product Revision         : %d.%d", (card_info->cid.prv >> 4), (card_info->cid.prv & 0xF));
        TRACE(1, "Product Serial No        : 0x%X", card_info->cid.psn);
        TRACE(1, "Manufacture Date         : %d/%d", 2000 + (card_info->cid.mdt >> 4), card_info->cid.mdt & 0xF);
        POSSIBLY_UNUSED uint32_t val = (uint32_t)(((uint64_t)card_info->blocknbr * card_info->blocksize * 100) / 1073741824);
        TRACE(1, "Card capacity(calc)      : %d.%dGB", val / 100, val % 100);
        TRACE(TR_ATTR_NO_LF, "Card capacity            : 0x%X", (uint32_t)(card_info->capacity >> 32));   //high 4bytes
        TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "%X", (uint32_t)(card_info->capacity));                      //low 4bytes
        TRACE(TR_ATTR_NO_LF, "Card capacity_user       : 0x%X", (uint32_t)(card_info->capacity_user >> 32));
        TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "%X", (uint32_t)(card_info->capacity_user));
        TRACE(TR_ATTR_NO_LF, "Card capacity_boot       : 0x%X", (uint32_t)(card_info->capacity_boot >> 32));
        TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "%X", (uint32_t)(card_info->capacity_boot));
        TRACE(TR_ATTR_NO_LF, "Card capacity_rpmb       : 0x%X", (uint32_t)(card_info->capacity_rpmb >> 32));
        TRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "%X", (uint32_t)(card_info->capacity_rpmb));
        TRACE(0, "********************************************");
        TRACE(0, "  ");
    }
}

enum HAL_SDMMC_ERR hal_sdmmc_get_card_info(enum HAL_SDMMC_ID_T id, struct HAL_SDMMC_CARD_INFO_T *card_info)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    if (card_info) {
        memset((void *)card_info, 0, sizeof(struct HAL_SDMMC_CARD_INFO_T));
        mmc_unpack_cid(host, card_info, host->mmc->cid);
        mmc_unpack_csd(host, card_info, host->mmc->csd);

        card_info->cardsort     = !!(IS_SD(host->mmc));
        card_info->high_capacity = !!(host->mmc->high_capacity);
        card_info->cardtype     = host->mmc->cardtype;
        card_info->cardclass    = card_info->csd.ccc;
        card_info->relcardadd   = host->mmc->rca;
        if (!card_info->high_capacity) {//low capacity card
            card_info->blocknbr = card_info->csd.c_size + 1U;
            card_info->blocknbr *= (1 << ((card_info->csd.c_size_mult & 0x07U) + 2U));
            card_info->blocksize = (1 << (card_info->csd.read_bl_len & 0x0FU));
            card_info->logblocknbr = card_info->blocknbr * (card_info->blocksize / 512U);
            card_info->logllocksize = host->mmc->read_bl_len;
        } else {//high capacity card
            if (IS_MMC(host->mmc)) {
                card_info->blocknbr = (uint32_t)(*(uint32_t *)&host->mmc->ext_csd[212]);
            } else {
                card_info->blocknbr = host->mmc->capacity_user / host->mmc->read_bl_len;
            }
            card_info->blocksize = host->mmc->write_bl_len;
            card_info->logblocknbr = card_info->blocknbr;
            card_info->logllocksize = card_info->blocksize;
        }

        if (IS_MMC(host->mmc)) {
            card_info->ext_csd_rev = host->mmc->ext_csd[192];
            card_info->firmware_version_h32 = (uint32_t)(*(uint32_t *)&host->mmc->ext_csd[258]);
            card_info->firmware_version = (uint32_t)(*(uint32_t *)&host->mmc->ext_csd[254]);
            card_info->device_version = (uint32_t)(*(uint16_t *)&host->mmc->ext_csd[262]);
            card_info->cache_size = host->mmc->cache_size;
            card_info->cache_ctrl = host->mmc->cache_ctrl;
            card_info->cache_flush_policy = host->mmc->ext_csd[240];
        }
        card_info->capacity      = host->mmc->capacity;
        card_info->capacity_user = host->mmc->capacity_user;
        card_info->capacity_boot = host->mmc->capacity_boot;
        card_info->capacity_rpmb = host->mmc->capacity_rpmb;
#ifdef SDMMC_DEBUG
        hal_sdmmc_dump_card_info(card_info);
#endif

        return HAL_SDMMC_ERR_NONE;
    }

    return HAL_SDMMC_INVALID_PARAMETER;
}

enum HAL_SDMMC_ERR hal_sdmmc_cache_control(enum HAL_SDMMC_ID_T id, uint8_t enable_disable)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];
    enum HAL_SDMMC_ERR ret = HAL_SDMMC_ERR_NONE;

    if (host && (host->mmc->cache_size > 0)) {
        enable_disable = !!enable_disable;

        if (host->mmc->cache_ctrl ^ enable_disable) {
            ret = mmc_switch(host->mmc, EXT_CSD_CMD_SET_NORMAL,
                             EXT_CSD_CACHE_CTRL, enable_disable);
            if (ret) {
                HAL_SDMMC_ERROR(1, "%s, emmc cache %s error %d", __func__, enable_disable ? "on" : "off", ret);
            } else {
                host->mmc->cache_ctrl = enable_disable;
            }
        }
    }

    return ret;
}

static bool sdmmc_cache_enabled(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];
    return host->mmc->cache_size > 0 &&
           host->mmc->cache_ctrl & 1;
}

//Flush the internal cache of the eMMC to non-volatile storage
enum HAL_SDMMC_ERR hal_sdmmc_cache_flush(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];
    enum HAL_SDMMC_ERR ret = HAL_SDMMC_ERR_NONE;

    if (sdmmc_cache_enabled(id)) {
        ret = mmc_switch(host->mmc, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_FLUSH_CACHE, 1);
        if (ret) {
            HAL_SDMMC_ERROR(1, "%s, emmc cache flush error %d", __func__, ret);
        }
    }

    return ret;
}

uint32_t hal_sdmmc_get_host_addr(enum HAL_SDMMC_ID_T id)
{
    return (uint32_t)&sdmmc_host[id];
}

uint32_t hal_sdmmc_get_ext_csd(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];
    return (uint32_t)host->mmc->ext_csd;
}

enum HAL_SDMMC_ERR hal_sdmmc_select_card(enum HAL_SDMMC_ID_T id)
{
    int err;
    struct mmc_cmd cmd;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    cmd.cmdidx = MMC_CMD_SELECT_CARD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = host->mmc->rca << 16;
    err = mmc_send_cmd(host->mmc, &cmd, NULL);

    return (enum HAL_SDMMC_ERR)err;
}

enum HAL_SDMMC_ERR hal_sdmmc_deselect_card(enum HAL_SDMMC_ID_T id)
{
    int err;
    struct mmc_cmd cmd;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    cmd.cmdidx = MMC_CMD_DESELECT_CARD;
    cmd.resp_type = MMC_RSP_NONE;
    cmd.cmdarg = 0;
    err = mmc_send_cmd(host->mmc, &cmd, NULL);

    return (enum HAL_SDMMC_ERR)err;
}

static int mmc_can_sleep(struct sdmmc_ip_host *host)
{
    return (host && host->mmc->ext_csd[192] >= 3);
}

#define S_A_TIMEOUT_INDEX_END   0x18
static const uint32_t s_a_timeout_us[S_A_TIMEOUT_INDEX_END] = {
    0, 1, 1, 1, 2, 4, 7, 13, 26, 52, 103, 205, 410, 820, 1639, 3277,
    6554, 13108, 26215, 52429, 104858, 209716, 419431, 838861
};

enum HAL_SDMMC_ERR hal_sdmmc_sleep_card(enum HAL_SDMMC_ID_T id)
{
    int err;
    struct mmc_cmd cmd;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    if (mmc_can_sleep(host)) {
        //deselect card:transfer state to standby state
        hal_sdmmc_deselect_card(id);

        //send cmd5:go to sleep
        cmd.cmdidx = MMC_CMD_SLEEP_AWAKE;
        cmd.resp_type = MMC_RSP_R1b;
        cmd.cmdarg = (host->mmc->rca << 16) | (1 << 15);
        err = mmc_send_cmd(host->mmc, &cmd, NULL);
        if (!err) {
            if (host->mmc->ext_csd[EXT_CSD_S_A_TIMEOUT] && host->mmc->ext_csd[EXT_CSD_S_A_TIMEOUT] < S_A_TIMEOUT_INDEX_END) {
                err = mmc_wait_dat0(host->mmc, 1, s_a_timeout_us[host->mmc->ext_csd[EXT_CSD_S_A_TIMEOUT]]);
                if (err == HAL_SDMMC_ERR_NONE) {
                    host->mmc->has_sleep = 1;
                }
            } else {
                err = HAL_SDMMC_S_A_TIMEOUT_ERR;
                HAL_SDMMC_ERROR(0, "%s, ext_csd[EXT_CSD_S_A_TIMEOUT] error", __func__);
            }
        }
    } else {
        err = HAL_SDMMC_CARD_NOT_SUPPORT;
        HAL_SDMMC_ERROR(0, "%s, not support", __func__);
    }

    return (enum HAL_SDMMC_ERR)err;
}

enum HAL_SDMMC_ERR hal_sdmmc_awake_card(enum HAL_SDMMC_ID_T id)
{
    int err;
    int8_t cnt = 3;
    struct mmc_cmd cmd;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    if (mmc_can_sleep(host)) {
        do {
            //send cmd5:exit sleep
            cmd.cmdidx = MMC_CMD_SLEEP_AWAKE;
            cmd.resp_type = MMC_RSP_R1b;
            cmd.cmdarg = (host->mmc->rca << 16);
            err = mmc_send_cmd(host->mmc, &cmd, NULL);
            if (!err) {
                if (host->mmc->ext_csd[EXT_CSD_S_A_TIMEOUT] && host->mmc->ext_csd[EXT_CSD_S_A_TIMEOUT] < S_A_TIMEOUT_INDEX_END) {
                    err = mmc_wait_dat0(host->mmc, 1, s_a_timeout_us[host->mmc->ext_csd[EXT_CSD_S_A_TIMEOUT]]);
                    if (err) {
                        HAL_SDMMC_ERROR(0, "%s, mmc_wait_dat0 error:%d", __func__, err);
                    }
                } else {
                    err = HAL_SDMMC_S_A_TIMEOUT_ERR;
                    HAL_SDMMC_ERROR(0, "%s, ext_csd[EXT_CSD_S_A_TIMEOUT] error", __func__);
                }
            } else {
                HAL_SDMMC_ERROR(0, "%s, send cmd error:%d, retry:%d", __func__, err, 3 - cnt);
            }
        } while (err && cnt--);

        //select card:standby state to transfer state
        cnt = 3;
        do {
            err = hal_sdmmc_select_card(id);
        } while (err && cnt--);
        if (err) {
            HAL_SDMMC_ERROR(0, "%s, hal_sdmmc_select_card error:%d", __func__, err);
        } else {
            host->mmc->has_sleep = 0;
        }
    } else {
        err = HAL_SDMMC_CARD_NOT_SUPPORT;
        HAL_SDMMC_ERROR(0, "%s, not support", __func__);
    }

    return (enum HAL_SDMMC_ERR)err;
}

enum HAL_SDMMC_ERR hal_sdmmc_get_card_raw_status(enum HAL_SDMMC_ID_T id, uint32_t *raw_status)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    return mmc_send_status(host->mmc, raw_status);
}

enum HAL_SDMMC_ERR hal_sdmmc_get_card_current_state(enum HAL_SDMMC_ID_T id, uint32_t *current_state)
{
    int err;
    uint32_t raw_status = 0;

    err = hal_sdmmc_get_card_raw_status(id, &raw_status);
    if (!err) {
        if (current_state) {
            *current_state = (raw_status >> 9) & 0x0F;
        }
    }

    return err;
}

enum HAL_SDMMC_ERR hal_sdmmc_get_card_sleep_status(enum HAL_SDMMC_ID_T id, uint8_t *sleep_status)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    *sleep_status = host->mmc->has_sleep;
    return HAL_SDMMC_ERR_NONE;
}

enum HAL_SDMMC_ERR hal_sdmmc_send_stop(enum HAL_SDMMC_ID_T id)
{
    int ret;
    struct mmc_cmd cmd;
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_R1b;
    ret = mmc_send_cmd(host->mmc, &cmd, NULL);
    if (ret) {
        HAL_SDMMC_ERROR(0, "%s, mmc fail to send stop cmd", __func__);
    }

    return ret;
}

void hal_sdmmc_dump_reg(enum HAL_SDMMC_ID_T id)
{
    struct sdmmc_ip_host *host = &sdmmc_host[id];

    TRACE(0, "%s", __func__);
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CTRL,       sdmmc_ip_readl(host, SDMMCIP_REG_CTRL));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_PWREN,      sdmmc_ip_readl(host, SDMMCIP_REG_PWREN));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CLKDIV,     sdmmc_ip_readl(host, SDMMCIP_REG_CLKDIV));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CLKSRC,     sdmmc_ip_readl(host, SDMMCIP_REG_CLKSRC));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CLKENA,     sdmmc_ip_readl(host, SDMMCIP_REG_CLKENA));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_TMOUT,      sdmmc_ip_readl(host, SDMMCIP_REG_TMOUT));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CTYPE,      sdmmc_ip_readl(host, SDMMCIP_REG_CTYPE));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_BLKSIZ,     sdmmc_ip_readl(host, SDMMCIP_REG_BLKSIZ));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_BYTCNT,     sdmmc_ip_readl(host, SDMMCIP_REG_BYTCNT));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_INTMASK,    sdmmc_ip_readl(host, SDMMCIP_REG_INTMASK));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CMDARG,     sdmmc_ip_readl(host, SDMMCIP_REG_CMDARG));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CMD,        sdmmc_ip_readl(host, SDMMCIP_REG_CMD));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_RESP0,      sdmmc_ip_readl(host, SDMMCIP_REG_RESP0));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_RESP1,      sdmmc_ip_readl(host, SDMMCIP_REG_RESP1));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_RESP2,      sdmmc_ip_readl(host, SDMMCIP_REG_RESP2));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_RESP3,      sdmmc_ip_readl(host, SDMMCIP_REG_RESP3));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_MINTSTS,    sdmmc_ip_readl(host, SDMMCIP_REG_MINTSTS));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_RINTSTS,    sdmmc_ip_readl(host, SDMMCIP_REG_RINTSTS));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_STATUS,     sdmmc_ip_readl(host, SDMMCIP_REG_STATUS));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_FIFOTH,     sdmmc_ip_readl(host, SDMMCIP_REG_FIFOTH));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_CDETECT,    sdmmc_ip_readl(host, SDMMCIP_REG_CDETECT));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_WRTPRT,     sdmmc_ip_readl(host, SDMMCIP_REG_WRTPRT));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_GPIO,       sdmmc_ip_readl(host, SDMMCIP_REG_GPIO));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_TCMCNT,     sdmmc_ip_readl(host, SDMMCIP_REG_TCMCNT));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_TBBCNT,     sdmmc_ip_readl(host, SDMMCIP_REG_TBBCNT));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_DEBNCE,     sdmmc_ip_readl(host, SDMMCIP_REG_DEBNCE));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_USRID,      sdmmc_ip_readl(host, SDMMCIP_REG_USRID));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_VERID,      sdmmc_ip_readl(host, SDMMCIP_REG_VERID));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_HCON,       sdmmc_ip_readl(host, SDMMCIP_REG_HCON));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_UHS_REG,    sdmmc_ip_readl(host, SDMMCIP_REG_UHS_REG));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_RESET_CARD, sdmmc_ip_readl(host, SDMMCIP_REG_RESET_CARD));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_BMOD,       sdmmc_ip_readl(host, SDMMCIP_REG_BMOD));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_PLDMND,     sdmmc_ip_readl(host, SDMMCIP_REG_PLDMND));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_DBADDR,     sdmmc_ip_readl(host, SDMMCIP_REG_DBADDR));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_IDSTS,      sdmmc_ip_readl(host, SDMMCIP_REG_IDSTS));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_IDINTEN,    sdmmc_ip_readl(host, SDMMCIP_REG_IDINTEN));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_DSCADDR,    sdmmc_ip_readl(host, SDMMCIP_REG_DSCADDR));
    TRACE(0, "read reg[%02X]:0x%X", SDMMCIP_REG_BUFADDR,    sdmmc_ip_readl(host, SDMMCIP_REG_BUFADDR));
}

#endif


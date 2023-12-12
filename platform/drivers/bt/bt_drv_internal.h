/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#ifndef __BT_DRV_INTERNAL_H__
#define __BT_DRV_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

/// Bluetooth technologies version
#define BT40_VERSION                      (6)
#define BT41_VERSION                      (7)
#define BT42_VERSION                      (8)
#define BT50_VERSION                      (9)
#define BT51_VERSION                      (10)
#define BT52_VERSION                      (11)
#define BT53_VERSION                      (12)
#define BT54_VERSION                      (13)

#ifndef ASSERT_ERR
#define ASSERT_ERR(cond)                             { if (!(cond)) { BT_DRV_TRACE(2,"line is %d file is %s", __LINE__, __FILE__); } }
#endif

#define B(byte, feat)     ((FEAT_##feat##_SUPP << B##byte##_##feat##_POS) & B##byte##_##feat##_MSK)
/// Extended feature mask definition page 2 LMP:3.3
#define B1_SEC_CON_CTRL_POS         0
#define B1_SEC_CON_CTRL_MSK         0x01
#define B1_PING_POS                 1
#define B1_PING_MSK                 0x02
#define B1_SAM_POS                  2
#define B1_SAM_MSK                  0x04
#define B1_TRAIN_NUDGING_POS        3
#define B1_TRAIN_NUDGING_MSK        0x08

#define BT_DRIVER_GET_U8_REG_VAL(regAddr)       (*(uint8_t *)(uintptr_t)(regAddr))
#define BT_DRIVER_GET_U16_REG_VAL(regAddr)      (*(uint16_t *)(uintptr_t)(regAddr))
#define BT_DRIVER_GET_U32_REG_VAL(regAddr)      (*(uint32_t *)(uintptr_t)(regAddr))

#define BT_DRIVER_PUT_U8_REG_VAL(regAddr, val)      *(uint8_t *)(uintptr_t)(regAddr) = (val)
#define BT_DRIVER_PUT_U16_REG_VAL(regAddr, val)     *(uint16_t *)(uintptr_t)(regAddr) = (val)
#define BT_DRIVER_PUT_U32_REG_VAL(regAddr, val)     *(uint32_t *)(uintptr_t)(regAddr) = (val)

typedef uint32_t BT_CONTROLER_TRACE_TYPE;
#define BT_CONTROLER_TRACE_TYPE_INTERSYS                0x01
#define BT_CONTROLER_TRACE_TYPE_CONTROLLER              0x02
#define BT_CONTROLER_TRACE_TYPE_LMP_TRACE               0x04
#define BT_CONTROLER_TRACE_TYPE_SPUV_HCI_BUFF           0x08
#define BT_CONTROLER_FILTER_TRACE_TYPE_A2DP_STREAM      0x10
#define BT_CONTROLER_TRACE_TYPE_DUMP_BUFF               0x20
#define BT_CONTROLER_TRACE_TYPE_SPUV_HCI_BUFF_HIGH      0x40
#define BT_CONTROLER_TRACE_TYPE_ACL_PACKET              0x80

#define BT_SUB_SYS_TYPE     0
#define MCU_SYS_TYPE        1
#define BT_EM_AREA_1_TYPE   2
#define BT_EM_AREA_2_TYPE   3

#define BT_FA_INVERT_EN   1
#define BT_FA_INVERT_DISABLE   0

/***************************************************************************
 *multi IBRT config
 ****************************************************************************/
#define MULTI_IBRT_BG_SLICE 24

#define MULTI_IBRT_FG_SLICE 72


#define TX_PWR_16DBM 16
#define TX_PWR_15DBM 15
#define TX_PWR_14DBM 14
#define TX_PWR_13DBM 13
#define TX_PWR_12DBM 12
#define TX_PWR_11DBM 11
#define TX_PWR_10DBM 10
#define TX_PWR_9DBM  9
#define TX_PWR_8DBM  8
#define TX_PWR_7DBM  7
#define TX_PWR_6DBM  6
#define TX_PWR_5DBM  5
#define TX_PWR_4DBM  4
#define TX_PWR_3DBM  3
#define TX_PWR_2DBM  2
#define TX_PWR_1DBM  1
#define TX_PWR_0DBM  0
#define TX_PWR_N1DBM -1
#define TX_PWR_N2DBM -2
#define TX_PWR_N3DBM -3
#define TX_PWR_N4DBM -4
#define TX_PWR_N5DBM -5
#define TX_PWR_N6DBM -6

void btdrv_poweron(uint8_t en);
uint8_t btdrv_rf_init(void);
void btdrv_test_mode_rf_txpwr_init(void);
void btdrv_ins_patch_init(void);
void btdrv_data_patch_init(void);
void btdrv_patch_en(uint8_t en);
void btdrv_config_init(void);
void btdrv_config_end(void);
void btdrv_testmode_config_init(void);
void btdrv_bt_spi_rawbuf_init(void);
void btdrv_bt_spi_xtal_init(void);
void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain);
void btdrv_trace_config(BT_CONTROLER_TRACE_TYPE trace_config);
void btdrv_btc_fault_dump(void);
void btdrv_dump_mem(uint8_t *dump_mem_start, uint32_t dump_length, uint8_t dump_type);
void btdrv_fast_lock_config(bool fastlock_on);
void btdrv_ecc_config(void);
void btdrv_btc_fault_dump(void);
void btdrv_fa_config(void);
void btdrv_hciprocess(void);
void bt_drv_extra_config_after_init(void);
void bt_drv_reg_op_multi_ibrt_sche_dump(void);
void bt_drv_reg_op_dump_rx_record(void);
int8_t btdrv_get_rssi_avg_thr(bool tws_link);
void bt_drv_reg_op_enable_3wire_tports(void);

void bt_drv_heap_init(void);
void btdrv_common_init(void);
void *bt_drv_malloc(unsigned int size);
void bt_drv_free(void *ptr);
void bt_tester_cmd_receive_evt_analyze(const unsigned char *data, unsigned int len);
uint32_t bt_drv_get_btc_sw_version(void);

enum BT_SYNCMODE_REQ_USER_T {
   BT_SYNCMODE_REQ_USER_BT,
   BT_SYNCMODE_REQ_USER_BLE,

   BT_SYNCMODE_REQ_USER_QTY
};
void btdrv_hw_agc_stop_mode(enum BT_SYNCMODE_REQ_USER_T user, bool hw_agc_mode);


inline uint16_t co_read16p(void const *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}

inline uint32_t co_read32p(void const *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = co_read16p((uint16_t *)ptr32);
    addr_h = co_read16p((uint16_t *)ptr32 + 1);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

#ifdef __cplusplus
}
#endif

#endif

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
#include <string.h>
#include "plat_types.h"
#include "bt_drv.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "bt_drv_1502x_internal.h"
#include "hal_chipid.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_internal.h"
#include "pmu.h"
#include "iqcorrect.h"

struct TX_PWR_SELECT_ITEM
{
    float pwr_dbm;
    uint16_t pwr_val;
    uint32_t pa_sel;
};

#define TX_PWR_PA_2P5 1
#define TX_PWR_PA_1P8 0

static const struct TX_PWR_SELECT_ITEM tx_pwr_select_2600ZP[] =
{
    {15, 0x29AB, TX_PWR_PA_2P5},    //15dbm
    {13, 0x29A9, TX_PWR_PA_2P5},    //13dbm
    {14, 0x29AA, TX_PWR_PA_2P5},    //14dbm
    {12, 0x29A8, TX_PWR_PA_2P5},    //12dbm
    {11, 0x8E34, TX_PWR_PA_1P8},    //11dbm
    {10, 0x6834, TX_PWR_PA_1P8},    //10dbm
    {9,  0x6824, TX_PWR_PA_1P8},    //9dbm
    {8,  0x6823, TX_PWR_PA_1P8},    //8dbm
    {7,  0x6923, TX_PWR_PA_1P8},    //7dbm
    {6,  0x6922, TX_PWR_PA_1P8},    //6dbm
    {5,  0x6921, TX_PWR_PA_1P8},    //5dbm
    {4,  0x6920, TX_PWR_PA_1P8},    //4dbm
    {3,  0x4914, TX_PWR_PA_1P8},    //3dbm
    {2,  0x4913, TX_PWR_PA_1P8},    //2dbm
    {1,  0x4912, TX_PWR_PA_1P8},    //1dbm
    {0,  0x4911, TX_PWR_PA_1P8},    //0dbm
    {-1, 0x4910, TX_PWR_PA_1P8},    //-1dbm
    {-2, 0x4905, TX_PWR_PA_1P8},    //-2dbm
    {-3, 0x4904, TX_PWR_PA_1P8},    //-3dbm
    {-4, 0x4903, TX_PWR_PA_1P8},    //-4dbm
};

static const struct TX_PWR_SELECT_ITEM tx_pwr_select_2700H[] =
{
    {15, 0x61AB, TX_PWR_PA_2P5},    //15dbm
    {14, 0x61AA, TX_PWR_PA_2P5},    //13dbm
    {13, 0x61A9, TX_PWR_PA_2P5},    //14dbm
    {12, 0x61A8, TX_PWR_PA_2P5},    //12dbm
    {11, 0x8EA4, TX_PWR_PA_1P8},    //11dbm
    {10, 0x8EA3, TX_PWR_PA_1P8},    //10dbm
    {9,  0x8EA2, TX_PWR_PA_1P8},    //9dbm
    {8,  0x8EA1, TX_PWR_PA_1P8},    //8dbm
    {7,  0x6023, TX_PWR_PA_1P8},    //7dbm
    {6,  0x6022, TX_PWR_PA_1P8},    //6dbm
    {5,  0x6021, TX_PWR_PA_1P8},    //5dbm
    {4,  0x6020, TX_PWR_PA_1P8},    //4dbm
    {3,  0x4015, TX_PWR_PA_1P8},    //3dbm
    {2,  0x4014, TX_PWR_PA_1P8},    //2dbm
    {1,  0x4013, TX_PWR_PA_1P8},    //1dbm
    {0,  0x4012, TX_PWR_PA_1P8},    //0dbm
    {-1, 0x4011, TX_PWR_PA_1P8},    //-1dbm
    {-2, 0x2806, TX_PWR_PA_1P8},    //-2dbm
    {-3, 0x2805, TX_PWR_PA_1P8},    //-3dbm
    {-4, 0x2805, TX_PWR_PA_1P8},    //-4dbm
};


static const struct TX_PWR_SELECT_ITEM tx_pwr_select_2700L[] =
{
    {15, 0x67AB, TX_PWR_PA_2P5},    //15dbm
    {14, 0x67AA, TX_PWR_PA_2P5},    //14dbm
    {13, 0x67A9, TX_PWR_PA_2P5},    //13dbm
    {12, 0x67A8, TX_PWR_PA_2P5},    //12dbm
    {11, 0xBEB0, TX_PWR_PA_1P8},    //11dbm
    {10, 0x6834, TX_PWR_PA_1P8},    //10dbm
    {9,  0x6824, TX_PWR_PA_1P8},    //9dbm
    {8,  0x6823, TX_PWR_PA_1P8},    //8dbm
    {7,  0x7E22, TX_PWR_PA_1P8},    //7dbm
    {6,  0x6822, TX_PWR_PA_1P8},    //6dbm
    {5,  0x6821, TX_PWR_PA_1P8},    //5dbm
    {4,  0x6820, TX_PWR_PA_1P8},    //4dbm
    {3,  0x4813, TX_PWR_PA_1P8},    //3dbm
    {2,  0x4812, TX_PWR_PA_1P8},    //2dbm
    {1,  0x4811, TX_PWR_PA_1P8},    //1dbm
    {0,  0x4810, TX_PWR_PA_1P8},    //0dbm
    {-1, 0x4805, TX_PWR_PA_1P8},    //-1dbm
    {-2, 0x4804, TX_PWR_PA_1P8},    //-2dbm
    {-3, 0x4803, TX_PWR_PA_1P8},    //-3dbm
    {-4, 0x4802, TX_PWR_PA_1P8},    //-4dbm
};

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

#define BT_Rf_REG_PA_SEL 0x162
#define BLE_Rf_REG_PA_SEL 0x161

#ifdef __RF_INIT_XTAL_CAP_FROM_NV__
#include "nvrecord_dev.h"
#endif

#ifndef BT_RF_MAX_XTAL_TUNE_PPB
// Default 10 ppm/bit or 10000 ppb/bit
#define BT_RF_MAX_XTAL_TUNE_PPB                 10000
#endif

#ifndef BT_RF_XTAL_TUNE_FACTOR
// Default 0.2 ppm/bit or 200 ppb/bit
#define BT_RF_XTAL_TUNE_FACTOR                  200
#endif


#define XTAL_FCAP_NORMAL_SHIFT                  0
#define XTAL_FCAP_NORMAL_MASK                   (0x1FF << XTAL_FCAP_NORMAL_SHIFT)
#define XTAL_FCAP_NORMAL(n)                     BITFIELD_VAL(XTAL_FCAP_NORMAL, n)

#define RF_REG_XTAL_FCAP                        0x103

static uint16_t xtal_fcap = DEFAULT_XTAL_FCAP;
static uint16_t init_xtal_fcap = DEFAULT_XTAL_FCAP;

struct RF_SYS_INIT_ITEM
{
    uint16_t reg;
    uint16_t set;
    uint16_t mask;
    uint16_t delay;
};

static const struct RF_SYS_INIT_ITEM rf_sys_init_tbl[] =
{
 //   {0x00E9,0x0000,0x000F,1},
};

const uint16_t rf_init_tbl_1_common[][3] =
{
    //rftx
    {0x0004,0x131a,0},//tx pa on timing
    {0x0049,0x9004,0},//pa sw
    {0x0160,0x040c,0},//pa vdd sel
    {0x015C,0x20A2,0},//pa mode sel
    {0x0147,0x4055,0},//pa i
    {0x002c,0x240e,0},//txf gain
    {0x00c1,0x1A40,0},//pad gain
    {0x00CA,0x8900,0},// pa bias
    {0x004C,0x780f,0}, //drv cap
    {0x0092,0x8390,0},//tmx bias
    {0x00cd,0x0090,0},//tmx phase
    {0x002b,0x0e00,0},//flt max gain

    //rfrx
    {0x0051,0x035e,0},//lna hg
    {0x00bf,0x1002,0},//lna ic
    {0x00ce,0x0074,0},//reduce lna ldo current
    //{0x002c,0x261f,1},//i2v max gain
    {0x0028,0x3e10,0},//i2v_corner
    {0x0027,0x1e04,0},//flt bw
    {0x0081,0x0008,0},//i2v rin

    {0x0012,0x0146,0},//vco cap cal time
    {0x0013,0x8000,0},//Increase VCO current, optimize pulling

    //[rfpll change]
    {0x00A1,0x1174,0},//rfpll setting
    {0x00A2,0x028c,0},
    {0x00A4,0x83A4,0},//reduce rfpll current
    {0x009E,0x1618,0},
    {0x00A0,0x1908,0},
    {0x008F,0xff32,0},//logen div_v setting
    {0x00CF,0xa600,0},
    {0x00D1,0xa200,0},
    {0x00C4,0x5c80,0},

    {0x0041,0x034B,0},//rx0
    {0x0042,0x01D3,0},
    {0x0043,0x01D3,0},
    {0x0044,0x05C3,0},
    {0x0045,0x05D3,0},
    {0x0046,0x05B4,0},
    {0x0047,0x0DB3,0},
    {0x0048,0x0D9A,0},

    {0x0039,0xe038,0},//rx1
    {0x003A,0xC02C,0},
    {0x003B,0xA005,0},
    {0x003C,0xA005,0},
    {0x003D,0x0005,0},
    {0x003E,0x0005,0},
    {0x003F,0x0005,0},
    {0x0040,0x0005,0},

    {0x012c,0x007E,0},//rx2
    {0x012d,0x007E,0},
    {0x012e,0x007E,0},
    {0x012f,0x007E,0},
    {0x0130,0x007E,0},
    {0x0131,0x007E,0},
    {0x0132,0x007E,0},
    {0x0133,0x007E,0},

    {0x0084,0xA449,0},
    {0x0085,0x0024,0},

    {0x00CC,0x803F,0},
    {0x0019,0xF140,0},
    {0x0090,0x3808,0},
    {0x0023,0x28BF,0},        //tx_gain_sel_en BLE Power control enable

    {0x0146,0x401B,0},        //ADC BW

    {0x0001,0x0101,0},
    {0x0008,0x0321,0},
    {0x000a,0x0c01,0},

    //xlc tx pulling
    {0x002d,0x3b20,0},
    {0x00cf,0xaf00,0},
    {0x00d1,0xa200,0},
    {0x00d9,0x0307,0},
    {0x00e6,0xf52b,0},
    {0x00d7,0x0447,0},
    {0x00da,0x0077,0},
    {0x00d8,0x36bf,0},

    {0x0144,0x5025,0},
    {0x009C,0x0041,0},
    {0x009D,0x0404,0},

    //BLE Rx
    {0x005f,0x34B,0},//ble rx0
    {0x0060,0x1D3,0},
    {0x0061,0x1D3,0},
    {0x0062,0x5C3,0},
    {0x0063,0x5D3,0},
    {0x0064,0x5B4,0},
    {0x0065,0xDB3,0},
    {0x0066,0xD9A,0},

    {0x0067,0xe038,0},//ble rx1
    {0x0068,0xC02C,0},
    {0x0069,0xA005,0},
    {0x006a,0xA005,0},
    {0x006b,0x0005,0},
    {0x006c,0x0005,0},
    {0x006d,0x0005,0},
    {0x006e,0x0005,0},

    {0x013c,0x007E,0},//ble rx2
    {0x013d,0x007E,0},
    {0x013e,0x007E,0},
    {0x013f,0x007E,0},
    {0x0140,0x007E,0},
    {0x0141,0x007E,0},
    {0x0142,0x007E,0},
    {0x0143,0x007E,0},

    {0x0082,0xA449,0},
    {0x0083,0x0024,0},
};

#ifdef __HW_AGC__
const uint16_t rf_init_hw_agc_config_t0[][3] =
{
    // RF settings
    {0x0027,0x6604,0},
    {0x0146,0x401B,0},
    {0x0144,0x5025,0},
    // [HWAGC-RF5-D-support nonsignal]
    // HWAGC enable
    {0x027D,0x0078,0},

    // gain table
    {0x021D,0x41AE,0},  //gain 0
    {0x0220,0x41A6,0},
    {0x0223,0x41A6,0},
    {0x0226,0x4196,0},
    {0x0229,0x45B6,0},
    {0x022C,0x45A6,0},
    {0x022F,0x4596,0},
    {0x0232,0x4D96,0},  //gain 7

    {0x021E,0xC036,0},  //gain 0
    {0x0221,0xC017,0},
    {0x0224,0xE008,0},
    {0x0227,0xE008,0},
    {0x022A,0x4007,0},
    {0x022D,0x4007,0},
    {0x0230,0x4007,0},
    {0x0233,0x4007,0},  //gain 7

    {0x021F,0x003E,0},  //gain 0
    {0x0222,0x003E,0},
    {0x0225,0x003E,0},
    {0x0228,0x003E,0},
    {0x022B,0x003E,0},
    {0x022E,0x003E,0},
    {0x0231,0x003E,0},
    {0x0234,0x003E,0},  //gain 7

    //gain offset
    {0x0235,0x2A31,0},
    {0x0236,0x2126,0},
    {0x0237,0x151A,0},
    {0x0238,0x040F,0},

    //jump point
    {0x0239,0xADA9,0},
    {0x023A,0xB7B2,0},
    {0x023B,0xC2BC,0},
    {0x023C,0x00CE,0},

    //mode jump offset
    {0x0219,0x0000,0},
    {0x021A,0xFD09,0},
    {0x021B,0x000A,0},
    {0x021C,0x0000,0},

    //HWAGC settings
    {0x0209,0x811E,0},  // enable fast lock
    {0x0215,0x7F7F,0},
    {0x0216,0x7F7F,0},
    {0x0217,0x4C02,0},

    {0x020A,0x01D4,0},  // wait RF time, 39us
    {0x020B,0x00CC,0},  // BT wait fast rssi time, 17us
    {0x020C,0x0090,0},  // BLE1M wait fast rssi time, 0us
    {0x020D,0x0000,0},  // BLE2M wait fast rssi time, 0us
    {0x020E,0x021C,0},  // LR wait fast rssi time, 0us
    {0x020F,0x0006,0},  // wait APD time锛�0.5us
    {0x0210,0x025A,0},  // wait EDR guard time锛�52.5us

    {0x0049,0x9384,0},  // peak detector threshold锛�0x0049[9:7] = 0x7锛�
    {0x00D2,0x3140,0},  // peak detector threshold,  0x00D2[13:12] = 0x3锛�
};

const uint16_t rf_init_hw_agc_config_t2[][3] =
{
    // RF settings
    {0x0027,0x6604,0},
    {0x0146,0x401B,0},
    {0x0144,0x5025,0},
    // [HWAGC-RF5-ECO]
    // HWAGC enable
    {0x027D,0x0078,0},

    //[HWAGC-RF5-ECO-support nonsignal]
    // enable slow rssi
    {0x0010,0x1000,0},
    // enable force lock
    {0x000E,0x0F02,0},	//0x000E[11:8]=0xF;

    // gain table
    {0x021D,0x41AE,0},        //gain 0
    {0x0220,0x41A6,0},
    {0x0223,0x41A6,0},
    {0x0226,0x4196,0},
    {0x0229,0x45B6,0},
    {0x022C,0x45A6,0},
    {0x022F,0x4596,0},
    {0x0232,0x4D96,0},        //gain 7

    {0x021E,0xC036,0},        //gain 0
    {0x0221,0xC017,0},
    {0x0224,0xE008,0},
    {0x0227,0xE008,0},
    {0x022A,0x4007,0},
    {0x022D,0x4007,0},
    {0x0230,0x4007,0},
    {0x0233,0x4007,0},        //gain 7

    {0x021F,0x003E,0},        //gain 0
    {0x0222,0x003E,0},
    {0x0225,0x003E,0},
    {0x0228,0x003E,0},
    {0x022B,0x003E,0},
    {0x022E,0x003E,0},
    {0x0231,0x003E,0},
    {0x0234,0x003E,0},        //gain 7

    //gain offset
    {0x0235,0x2C33,0},
    {0x0236,0x2329,0},
    {0x0237,0x161B,0},
    {0x0238,0x0510,0},

    //jump point
    {0x0239,0xADA9,0},
    {0x023A,0xB7B2,0},
    {0x023B,0xC2BC,0},
    {0x023C,0x00CE,0},

    //mode jump offset
    {0x0219,0x0000,0},
    {0x021A,0xFD07,0},
    {0x021B,0x000A,0},
    {0x021C,0x0000,0},

    //HWAGC settings
    {0x0209,0x8116,0},        // enable fast lock
    {0x0215,0x0303,0},
    {0x0216,0x0603,0},
    {0x0217,0x4C02,0},

    {0x020A,0x01D4,0},        // wait RF time, 30us
    {0x020B,0x0000,0},        // BT wait fast rssi time, 17us
    {0x020C,0x0000,0},        // BLE1M wait fast rssi time, 12us
    {0x020D,0x0000,0},        // BLE2M wait fast rssi time, 9us
    {0x020E,0x0000,0},        // LR wait fast rssi time, 45us
    {0x020F,0x0006,0},        // wait APD time,0.5us
    {0x0210,0x025A,0},        // wait EDR guard time,52.5us

    {0x0049,0x9384,0},        // peak detector threshold，0x0049[9:7] = 0x7；
    {0x00D2,0x3140,0},        // peak detector threshold,  0x00D2[13:12] = 0x3；
};
#endif

const uint16_t rf_init_tbl_1_packey_type_2700IBP[][3] =
{
    {0x009B, 0x4748, 0},
    {0x00CB, 0x0144, 0}, //pad i
};

const uint16_t rf_init_tbl_1_packey_type_2700H[][3] =
{
    {0x009B, 0x4748, 0},
    {0x00CB, 0x0144, 0}, //pad i
};

const uint16_t rf_init_tbl_1_packey_type_2600ZP[][3] =
{
    {0x009B, 0x4748, 0},
    {0x00CB, 0x01F4, 0}, //pad i
};

const uint16_t rf_init_tbl_1_packey_type_2700L[][3] =
{
    {0x009B, 0x6748, 0},
    {0x00CA, 0xAA00, 0},
    {0x00CB, 0x0144, 0},//pad i
    {0x00CC, 0x802F, 0},
};

void bt_drv_rf_set_bt_hw_agc_enable(bool enable)
{
    uint16_t val_e2 = 0;
    btdrv_read_rf_reg(0xb6,&val_e2);
    if(enable)
    {
        //open rf bt hw agc mode
        val_e2 |= (1<<1);
    }
    else
    {
        //close rf bt hw agc mode
        val_e2 &= ~(1<<1);
    }
    btdrv_write_rf_reg(0xb6,val_e2);
}

void bt_drv_rf_set_ble_hw_agc_enable(bool enable)
{
    uint16_t val_e2 = 0;
    btdrv_read_rf_reg(0xb7,&val_e2);
    if(enable)
    {
        //open rf ble hw agc mode
        val_e2 |= (1<<11);
    }
    else
    {
        //close rf ble hw agc mode
        val_e2 &= ~(1<<11);
    }
    btdrv_write_rf_reg(0xb7,val_e2);
}

void bt_drv_rf_set_afh_monitor_gain(void)
{
    uint16_t val_e2 = 0;
    btdrv_read_rf_reg(0x1e4,&val_e2);//reg_bt_i2v_rin_afh[6:4]
    //i2v_rin_afh = 2 i2v_stb_afh=1
    val_e2 &= ~(7<<4);
    val_e2 |= (2<<4);
    val_e2 &= ~(7<<12); //reg_bt_i2v_stb_afh[14:12]
    val_e2 |= (1<<12); //reg_bt_i2v_stb_afh[14:12]
    btdrv_write_rf_reg(0x1e4,val_e2);


    btdrv_read_rf_reg(0x3cb,&val_e2); //lna_ic = 0 , i2v_gain = 7  lna_ldo_ic = 0xb
    val_e2 &= ~(0xf<<0);//reg_bt_i2v_gain_afh[3:0]
    val_e2 |= (7<<0);

    val_e2 &= ~(3<<4);//reg_bt_adc_vin_sel_afh[5:4]
    val_e2 |= (1<<4);

    val_e2 &= ~(0xF<<8);//reg_bt_lna_ldo_ic_afh[11:8]
    val_e2 |= (0x5<<8);

    val_e2 &= ~(7<<12);// reg_bt_lna_ic_afh[14:12]

    val_e2 &= ~(3<<6);//reg_bt_adc_vin_sel_normal[7:6]
    val_e2 |= (1<<6);

    btdrv_write_rf_reg(0x3cb,val_e2);


    btdrv_read_rf_reg(0x3cc,&val_e2); //flt = 7, hg_en=0, hg=0, rpass=11, rin=00,rfflt=0

    val_e2 = 0x1E0C;

    btdrv_write_rf_reg(0x3cc,val_e2);
}

uint32_t btdrv_rf_get_max_xtal_tune_ppb(void)
{
    return BT_RF_MAX_XTAL_TUNE_PPB;
}

uint32_t btdrv_rf_get_xtal_tune_factor(void)
{
    return BT_RF_XTAL_TUNE_FACTOR;
}

void btdrv_rf_init_xtal_fcap(uint32_t fcap)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
    btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
    init_xtal_fcap = xtal_fcap;
}

uint32_t btdrv_rf_get_init_xtal_fcap(void)
{
    return GET_BITFIELD(init_xtal_fcap, XTAL_FCAP_NORMAL);
}

uint32_t btdrv_rf_get_xtal_fcap(void)
{
    return GET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL);
}

void btdrv_rf_set_xtal_fcap(uint32_t fcap, uint8_t is_direct)
{
}

int btdrv_rf_xtal_fcap_busy(uint8_t is_direct)
{
    return 0;
}

uint32_t btdrv_rf_bit_offset_get(void)
{
    return 0;
}

uint16_t btdrv_rf_bitoffset_get(uint8_t conidx)
{
    return bt_drv_reg_op_bitoff_getf(conidx);
}

void btdrv_rf_log_delay_cal(void)
{
    unsigned short read_value;
    unsigned short write_value;
    BT_DRV_TRACE(1,"%s", __func__);
    BTDIGITAL_REG(0xd0340020) = 0x010e01c0;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );

    btdrv_write_rf_reg(0xd4, 0x000f);
    btdrv_write_rf_reg(0xd5, 0x4000);
    btdrv_write_rf_reg(0xd2, 0x1003);
    btdrv_write_rf_reg(0xa7, 0x004e);
    btdrv_write_rf_reg(0xd4, 0x0000);
    btdrv_write_rf_reg(0xd5, 0x4002);

    BTDIGITAL_REG(0xd0340020) = 0x030e01c1;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );

    btdrv_delay(1);

    btdrv_write_rf_reg(0xd2, 0x5003);

    btdrv_delay(1);

    btdrv_read_rf_reg(0x1e2, &read_value);
    BT_DRV_TRACE(1,"0x1e2 read_value:%x\n",read_value);
    if(read_value == 0xff80)
    {
        btdrv_write_rf_reg(0xd3, 0xffff);
    }
    else
    {
        write_value = ((read_value>>7)&0x0001) | ((read_value & 0x007f)<<1) | ((read_value&0x8000)>>7) | ((read_value&0x7f00)<<1);
        BT_DRV_TRACE(1,"d3 write_value:%x\n",write_value);
        btdrv_write_rf_reg(0xd3, write_value);
    }
    btdrv_delay(1);

    BTDIGITAL_REG(0xd0340020) = 0x010e01c0;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );


    btdrv_write_rf_reg(0xd4, 0x000f);
    btdrv_write_rf_reg(0xd2, 0x1003);
    btdrv_write_rf_reg(0xd5, 0x4000);

}

void btdrv_rf_log_delay_cal_init(void)
{
    unsigned short read_value;
    uint16_t rf_f5_13_7, rf_f5_6_0 = 0;

    BT_DRV_TRACE(1,"%s", __func__);

    BTDIGITAL_REG(0xd0220c00) = 0x000A002A;

    //init
    btdrv_write_rf_reg(0x29c, 0xffb3);
    btdrv_write_rf_reg(0x29d, 0x001f);

    //cal flow
    btdrv_write_rf_reg(0xc2, 0x0);
    //bit9 : reg_rstn_log_cal
    btdrv_read_rf_reg(0xbe, &read_value);
    read_value |= (1 << 9);
    btdrv_write_rf_reg(0xbe, read_value);
    //reg_bt_log_cal_en
    btdrv_read_rf_reg(0x3af, &read_value);
    read_value |= (1 << 0);
    btdrv_write_rf_reg(0x3af, read_value);

    //log_cal_value[14:0]
    btdrv_read_rf_reg(0xf5, &read_value);
    BT_DRV_TRACE(0,"0xf5=0x%x",read_value);
    rf_f5_13_7 = (read_value & 0x3f80) >> 7;
    rf_f5_6_0  = (read_value & 0x7f);

    //0xc2 reg_fdata1[7:1] | 0xc2 reg_fdata2[15:9] | bit0:reg_fdata1_dr | bit8:reg_fdata2_dr
    btdrv_read_rf_reg(0xc2, &read_value);
    read_value = 0x0;
    read_value |= ((rf_f5_6_0 << 1) | (rf_f5_13_7 << 9) | (1<<0) | (1<<8));
    btdrv_write_rf_reg(0xc2, read_value);
    BT_DRV_TRACE(0,"0xc2=0x%x",read_value);

    //cal done
    btdrv_read_rf_reg(0xbe, &read_value);
    read_value &= ~(1 << 9);
    btdrv_write_rf_reg(0xbe, read_value);

    btdrv_read_rf_reg(0x3af, &read_value);
    read_value &= ~(1 << 0);
    btdrv_write_rf_reg(0x3af, read_value);

    BTDIGITAL_REG(0xd0220c00) = 0x0;
}

void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain)
{
    return;
}

//rf Image calib
void btdtv_rf_image_calib(void)
{
    uint16_t read_val = 0;
    //read calibrated val from efuse 0x05 register
    pmu_get_efuse(PMU_EFUSE_PAGE_SW_CFG, &read_val);
    //check if bit 11 has been set
    uint8_t calb_done_flag = ((read_val &0x800)>>11);
    if(calb_done_flag)
    {
        BT_DRV_TRACE(1,"EFUSE REG[5]=%x",read_val);
    }
    else
    {
        BT_DRV_TRACE(0,"EFUSE REG[5] rf image has not been calibrated!");
        return;
    }
    //[bit 12] calib flag
    uint8_t calib_val = ((read_val &0x1000)>>12);
    btdrv_read_rf_reg(0x9b,&read_val);
    read_val&=0xfcff;

    if(calib_val==0)
    {
        read_val|= 1<<8;
    }
    else if(calib_val== 1)
    {
        read_val|= 1<<9;
    }

    BT_DRV_TRACE(1,"write rf image calib val=%x in REG[0x9b]", read_val);
    btdrv_write_rf_reg(0x9b,read_val);
}

#define BT_TX_PWR_PAGE TX_PWR_8DBM
#define BT_TX_PWR_LV3 TX_PWR_15DBM
#define BT_TX_PWR_LV2 TX_PWR_10DBM
#define BT_TX_PWR_LV1 TX_PWR_5DBM
#define BT_TX_PWR_LV0 TX_PWR_0DBM
#define LE_TX_PWR_LV3 TX_PWR_15DBM
#define LE_TX_PWR_LV2 TX_PWR_10DBM
#define LE_TX_PWR_LV1 TX_PWR_5DBM
#define LE_TX_PWR_LV0 TX_PWR_0DBM

struct btdrv_customer_rf_config_t btdrv_rf_customer_config;
void bt_drv_rf_sdk_init(void)
{
    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();

    BT_DRV_TRACE(0,"SDK default rf config");
    btdrv_rf_customer_config.config_xtal_en = false;//get value from NV
    btdrv_rf_customer_config.xtal_cap_val = 0;//get value from NV

    //config TX power in dbm
    btdrv_rf_customer_config.config_tx_pwr_en = true;

    if (metal_id >= HAL_CHIP_METAL_ID_2) {
        btdrv_rf_customer_config.bt_tx_idx3_pwr = BT_TX_PWR_PAGE;//in dbm(mapping to page)

        btdrv_rf_customer_config.bt_tx_idx7_pwr  = BT_TX_PWR_LV3;//in dbm(mapping to idx3)
        btdrv_rf_customer_config.bt_tx_page_pwr = BT_TX_PWR_LV2;//in dbm(mapping to idx2)
        btdrv_rf_customer_config.bt_tx_max_pwr = BT_TX_PWR_LV1;//in dbm(mapping to idx1)
        btdrv_rf_customer_config.bt_tx_idx4_pwr = BT_TX_PWR_LV0;//in dbm(mapping to idx0)
        // ble tx pwr
        btdrv_rf_customer_config.le_tx_idx7_pwr = LE_TX_PWR_LV3;//in dbm(mapping to idx3)
        btdrv_rf_customer_config.le_tx_idx6_pwr = LE_TX_PWR_LV2;//in dbm(mapping to idx2)
        btdrv_rf_customer_config.le_tx_max_pwr = LE_TX_PWR_LV1;//in dbm(mapping to idx1)
        btdrv_rf_customer_config.le_tx_idx4_pwr = LE_TX_PWR_LV0;//in dbm(mapping to idx0)
        //init BLE convert table
        btdrv_txpwr_conv_tbl[0] = -127;     // disable idx0, dont modified
        btdrv_txpwr_conv_tbl[1] = -127;     // disable idx1, dont modified
        btdrv_txpwr_conv_tbl[2] = -127;     // disable idx2, dont modified
        btdrv_txpwr_conv_tbl[3] = -127;     // disable idx3, dont modified
        btdrv_txpwr_conv_tbl[4] = btdrv_rf_customer_config.le_tx_idx4_pwr;
        btdrv_txpwr_conv_tbl[5] = btdrv_rf_customer_config.le_tx_max_pwr;
        btdrv_txpwr_conv_tbl[6] = btdrv_rf_customer_config.le_tx_idx6_pwr;
        btdrv_txpwr_conv_tbl[7] = btdrv_rf_customer_config.le_tx_idx7_pwr;
    } else {
        btdrv_rf_customer_config.config_tx_pwr_en = true;
        btdrv_rf_customer_config.bt_tx_page_pwr = BT_TX_PWR_PAGE;//in dbm

        btdrv_rf_customer_config.bt_tx_idx3_pwr = BT_TX_PWR_LV3;//in dbm
        btdrv_rf_customer_config.bt_tx_idx2_pwr = BT_TX_PWR_LV2;//in dbm
        btdrv_rf_customer_config.bt_tx_idx1_pwr = BT_TX_PWR_LV1;//in dbm
        btdrv_rf_customer_config.bt_tx_idx0_pwr = BT_TX_PWR_LV0;//in dbm

        btdrv_rf_customer_config.le_tx_idx3_pwr = LE_TX_PWR_LV3;//in dbm
        btdrv_rf_customer_config.le_tx_idx2_pwr = LE_TX_PWR_LV2;//in dbm
        btdrv_rf_customer_config.le_tx_idx1_pwr = LE_TX_PWR_LV1;//in dbm
        btdrv_rf_customer_config.le_tx_idx0_pwr = LE_TX_PWR_LV0;//in dbm
        //init BLE convert table
        btdrv_txpwr_conv_tbl[0] = btdrv_rf_customer_config.le_tx_idx0_pwr;
        btdrv_txpwr_conv_tbl[1] = btdrv_rf_customer_config.le_tx_idx1_pwr;
        btdrv_txpwr_conv_tbl[2] = btdrv_rf_customer_config.le_tx_idx2_pwr;
        btdrv_txpwr_conv_tbl[3] = btdrv_rf_customer_config.le_tx_idx3_pwr;
        btdrv_txpwr_conv_tbl[4] = btdrv_rf_customer_config.le_tx_max_pwr;
        btdrv_txpwr_conv_tbl[5] = btdrv_rf_customer_config.le_tx_max_pwr;
        btdrv_txpwr_conv_tbl[6] = btdrv_rf_customer_config.le_tx_max_pwr;
        btdrv_txpwr_conv_tbl[7] = btdrv_rf_customer_config.le_tx_max_pwr;
    }
}

void bt_drv_rf_set_customer_config(struct btdrv_customer_rf_config_t* config)
{
    memset(&btdrv_rf_customer_config, 0, sizeof(struct btdrv_customer_rf_config_t));
    if(config !=NULL)
    {
        btdrv_rf_customer_config = *config;
    }
    else
    {
        bt_drv_rf_sdk_init();
    }
}

static uint16_t bt_rf_txpwr_reg_get(uint8_t idx)
{
    uint16_t rf_reg = 0;
    switch(idx)
    {
        case TX_PWR_IDX_0:
            rf_reg = RF_BT_TX_PWR_IDX0_REG;
            break;
        case TX_PWR_IDX_1:
            rf_reg = RF_BT_TX_PWR_IDX1_REG;
            break;
        case TX_PWR_IDX_2:
            rf_reg = RF_BT_TX_PWR_IDX2_REG;
            break;
        case TX_PWR_IDX_3:
            rf_reg = RF_BT_TX_PWR_IDX3_REG;
            break;
        case TX_PWR_IDX_4:
            rf_reg = RF_BT_TX_PWR_IDX4_REG;
            break;
        case TX_PWR_IDX_5:
            rf_reg = RF_BT_TX_PWR_IDX5_REG;
            break;
        case TX_PWR_IDX_6:
            rf_reg = RF_BT_TX_PWR_IDX6_REG;
            break;
        case TX_PWR_IDX_7:
            rf_reg = RF_BT_TX_PWR_IDX7_REG;
            break;
        default:
            ASSERT(0, "BT_DRV:ERROR tx pwr idx=%d", idx);
            break;
    }

    return rf_reg;
}

void ble_drv_rf_update_fixed_tx_pwr(uint8_t Ble_Tx_Pwr_In_dB)
{

}

void bt_drv_rf_update_fixed_tx_pwr(uint8_t Bt_Tx_Pwr_In_dB)
{

}

uint16_t ble_rf_txpwr_reg_get(uint8_t idx)
{
    uint16_t rf_reg = 0;
    switch(idx)
    {
        case TX_PWR_IDX_0:
            rf_reg = RF_BLE_TX_PWR_IDX0_REG;
            break;
        case TX_PWR_IDX_1:
            rf_reg = RF_BLE_TX_PWR_IDX1_REG;
            break;
        case TX_PWR_IDX_2:
            rf_reg = RF_BLE_TX_PWR_IDX2_REG;
            break;
        case TX_PWR_IDX_3:
            rf_reg = RF_BLE_TX_PWR_IDX3_REG;
            break;
        case TX_PWR_IDX_4:
            rf_reg = RF_BLE_TX_PWR_IDX4_REG;
            break;
        case TX_PWR_IDX_5:
            rf_reg = RF_BLE_TX_PWR_IDX5_REG;
            break;
        case TX_PWR_IDX_6:
            rf_reg = RF_BLE_TX_PWR_IDX6_REG;
            break;
        case TX_PWR_IDX_7:
            rf_reg = RF_BLE_TX_PWR_IDX7_REG;
            break;
        default:
            ASSERT(0, "BT_DRV:ERROR BLE tx pwr idx=%d", idx);
            break;
    }

    return rf_reg;
}

void bt_drv_rf_bt_pa_select(uint8_t idx, uint16_t pu_2p5pa)
{
    uint16_t read_val=0;
    btdrv_read_rf_reg(BT_Rf_REG_PA_SEL, &read_val);
    if (pu_2p5pa){
        read_val |= (1<<(idx+8));
        read_val &= ~((1<<0)<<idx);
        btdrv_write_rf_reg(BT_Rf_REG_PA_SEL, read_val);
    }else{
        read_val |= (1<<idx);
        read_val &= ~((1<<8)<<idx);
        btdrv_write_rf_reg(BT_Rf_REG_PA_SEL, read_val);
    }
}

void bt_drv_rf_ble_pa_select(uint8_t idx, uint16_t pu_2p5pa)
{
    uint16_t read_val=0;
    btdrv_read_rf_reg(BLE_Rf_REG_PA_SEL, &read_val);
    if (pu_2p5pa){
        read_val |= (1<<(idx+8));
        read_val &= ~((1<<0)<<idx);
        btdrv_write_rf_reg(BLE_Rf_REG_PA_SEL, read_val);
    }else{
        read_val |= (1<<idx);
        read_val &= ~((1<<8)<<idx);
        btdrv_write_rf_reg(BLE_Rf_REG_PA_SEL, read_val);
    }
}

int bt_drv_get_packag_type(void)
{
    uint16_t packag_type;

    pmu_get_efuse(PMU_EFUSE_PAGE_RESERVED_13, &packag_type);
    packag_type = (packag_type >> 3) & 0x7;
    if (packag_type != 0) {
        BT_DRV_TRACE(2, "%s PMU_EFUSE_PAGE_SW_CFG val=0x%x",__func__, packag_type);
    } else {
#if (PACKAG_TYPE == BTDRV_PACKAG_2600ZP)
        packag_type = BTDRV_PACKAG_2600ZP;
#elif (PACKAG_TYPE == BTDRV_PACKAG_2700IBP)
        packag_type = BTDRV_PACKAG_2700IBP;
#elif (PACKAG_TYPE == BTDRV_PACKAG_2700H)
        packag_type = BTDRV_PACKAG_2700H;
#elif (PACKAG_TYPE == BTDRV_PACKAG_2700L)
        packag_type = BTDRV_PACKAG_2700L;
#else
        ASSERT(0, "need define PACKAG_TYPE!");
#endif
    }
    return packag_type;
}

static int rf_reg_txpwr_val_get_from_gain_table(int8_t txpwr_dbm, uint16_t *reg_pwr, uint16_t *pa_sel, const struct TX_PWR_SELECT_ITEM *tx_pwr_table, uint16_t tx_pwr_table_size)
{
    uint16_t i = 0;
    uint16_t nRet = -1;

    for (i=0; i < tx_pwr_table_size; i++) {
        if (tx_pwr_table[i].pwr_dbm == txpwr_dbm) {
            *reg_pwr = tx_pwr_table[i].pwr_val;
            *pa_sel = tx_pwr_table[i].pa_sel;
            nRet = 0;
            break;
        }
    }
    return nRet;
}

int rf_reg_txpwr_val_get(int8_t txpwr_dbm, uint16_t *reg_pwr, uint16_t *pa_sel)
{
    uint32_t txpwr_val = 0;
    uint16_t tbl_size;
    uint16_t packag_type;
    uint16_t nRet;

    packag_type = bt_drv_get_packag_type();
    if (packag_type == BTDRV_PACKAG_2700L) {
        tbl_size = sizeof(tx_pwr_select_2700L) / sizeof(tx_pwr_select_2700L[0]);
        nRet = rf_reg_txpwr_val_get_from_gain_table(txpwr_dbm, reg_pwr, pa_sel, tx_pwr_select_2700L, tbl_size);
    } else if (packag_type == BTDRV_PACKAG_2700H || packag_type == BTDRV_PACKAG_2700IBP) {
        tbl_size = sizeof(tx_pwr_select_2700H) / sizeof(tx_pwr_select_2700H[0]);
        nRet = rf_reg_txpwr_val_get_from_gain_table(txpwr_dbm, reg_pwr, pa_sel, tx_pwr_select_2700H, tbl_size);
    } else {
        tbl_size = sizeof(tx_pwr_select_2600ZP) / sizeof(tx_pwr_select_2600ZP[0]);
        nRet = rf_reg_txpwr_val_get_from_gain_table(txpwr_dbm, reg_pwr, pa_sel, tx_pwr_select_2600ZP, tbl_size);
    }

    ASSERT(nRet == 0, "Tx pwr customer config invalid, please use correct tx pwr!");

    return txpwr_val;
}

void bt_drv_ble_rf_reg_txpwr_set(uint16_t rf_reg, uint16_t val)
{
    btdrv_write_rf_reg(rf_reg, val);
}

static void bt_drv_rf_bt_txpwr_set(uint8_t idx, int8_t val_in_dbm)
{
    uint16_t pa_sel = 0;
    uint16_t tx_pwr_val = 0;
    uint16_t rf_reg = bt_rf_txpwr_reg_get(idx);

    rf_reg_txpwr_val_get(val_in_dbm, &tx_pwr_val, &pa_sel);
    btdrv_write_rf_reg(rf_reg, tx_pwr_val);
    bt_drv_rf_bt_pa_select(idx, pa_sel);

    BT_DRV_TRACE(2, "BT:TX pwr reg=0x%x,[in:%d dbm,out:0x%04x]", rf_reg, val_in_dbm, tx_pwr_val);
}

static void bt_drv_rf_ble_txpwr_set(uint8_t idx, int8_t val_in_dbm)
{
    uint16_t pa_sel = 0;
    uint16_t tx_pwr_val = 0;
    uint16_t rf_reg = ble_rf_txpwr_reg_get(idx);

    rf_reg_txpwr_val_get(val_in_dbm, &tx_pwr_val, &pa_sel);
    btdrv_write_rf_reg(rf_reg, tx_pwr_val);
    bt_drv_rf_ble_pa_select(idx, pa_sel);

    BT_DRV_TRACE(2, "BLE:TX pwr reg=0x%x,[in:%d dbm,out:0x%04x]", rf_reg, val_in_dbm, tx_pwr_val);
}

void btdrv_rf_reg_checker(void)
{
    uint16_t read_val = 0;
    uint16_t reg_addr = 0;

    for(int i = 0; i < 8; i++) {
        reg_addr = 0x14C + i;

        btdrv_write_rf_reg(reg_addr, 0x61AB);
        read_val = 0;
        btdrv_read_rf_reg(reg_addr, &read_val);

        if (read_val != 0x61AB) {
            TRACE_IMM(0, "%s error detect 0x%04X val=0x%04X", __func__, reg_addr, read_val);
        }
        btdrv_write_rf_reg(reg_addr, 0);
    }

    for(int i = 0; i < 4; i++) {
        reg_addr = 0x158 + i;

        btdrv_write_rf_reg(reg_addr, 0x61AB);
        read_val = 0;
        btdrv_read_rf_reg(reg_addr, &read_val);

        if (read_val != 0x61AB) {
            TRACE_IMM(0, "%s error detect 0x%04X val=0x%04X", __func__, reg_addr, read_val);
        }
        btdrv_write_rf_reg(reg_addr, 0);
    }
}

void bt_drv_tx_pwr_init(void)
{
    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();

    btdrv_write_rf_reg(BT_Rf_REG_PA_SEL,0x0000);
    btdrv_write_rf_reg(BLE_Rf_REG_PA_SEL,0x0000);
    btdrv_write_rf_reg(0xaf,0xc040);
    btdrv_write_rf_reg(0xb2,0x4806);
    btdrv_write_rf_reg(0xb3,0x1130);

    struct btdrv_customer_rf_config_t cfg = btdrv_rf_customer_config;

    if(cfg.config_tx_pwr_en == true)
    {
        if (metal_id >= HAL_CHIP_METAL_ID_2) {
        //BT
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_0, cfg.bt_tx_idx4_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_1, cfg.bt_tx_idx4_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_2, cfg.bt_tx_idx4_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_3, cfg.bt_tx_idx3_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_4, cfg.bt_tx_idx4_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_5, cfg.bt_tx_max_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_6, cfg.bt_tx_page_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_7, cfg.bt_tx_idx7_pwr);
            //BLE
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_4, cfg.le_tx_idx4_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_5, cfg.le_tx_max_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_6, cfg.le_tx_idx6_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_7, cfg.le_tx_idx7_pwr);
        } else {
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_0, cfg.bt_tx_idx0_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_1, cfg.bt_tx_idx1_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_2, cfg.bt_tx_idx2_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_3, cfg.bt_tx_idx3_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_4, cfg.bt_tx_max_pwr);
            bt_drv_rf_bt_txpwr_set(TX_PWR_IDX_6, cfg.bt_tx_page_pwr);
            //BLE
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_0, cfg.le_tx_idx0_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_1, cfg.le_tx_idx1_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_2, cfg.le_tx_idx2_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_3, cfg.le_tx_idx3_pwr);
            bt_drv_rf_ble_txpwr_set(TX_PWR_IDX_4, cfg.le_tx_max_pwr);
        }
    }
    else
    {
        ASSERT(0, "BT_DRV:pls check TX pwr config");
    }
}

void bt_drv_rf_init_xtal_fcap(void)
{
    uint16_t xtal_fcap_temp = DEFAULT_XTAL_FCAP;
    if(btdrv_rf_customer_config.config_xtal_en == true)
    {
        xtal_fcap_temp = btdrv_rf_customer_config.xtal_cap_val;
        BT_DRV_TRACE(1,"btdrv customer set xtal fcap=0x%x", xtal_fcap_temp);
        btdrv_rf_init_xtal_fcap(xtal_fcap_temp);
    }
    else
    {
#ifdef __RF_INIT_XTAL_CAP_FROM_NV__
        unsigned int xtal_fcap;
        if (!nvrec_dev_get_xtal_fcap(&xtal_fcap))
        {
            btdrv_rf_init_xtal_fcap(xtal_fcap);
            btdrv_delay(1);
            BT_DRV_TRACE(2,"%s XTAL 0x03=0x%x", __func__, xtal_fcap);
        }
        else
#endif
        {
            btdrv_rf_init_xtal_fcap(DEFAULT_XTAL_FCAP);
            BT_DRV_TRACE(1,"%s failed", __func__);
        }
    }
}

void bt_drv_ble_adv_txpwr_via_advhdl(uint8_t adv_hdl, uint8_t idx, int8_t txpwr_dbm)
{

    BT_DRV_TRACE(2,"set adv_txpwr_via_advhdl hdl 0x%02x idx %d dbm %d", adv_hdl, idx, txpwr_dbm);

    if(idx > 5)
    {
        return;
    }

    if(txpwr_dbm <-3)
    {
        txpwr_dbm += 18;
        bt_drv_reg_op_low_txpwr(true, FACTOR_ATTENUATION_18DBM, BLE_ADV_LOW_TXPWR_MODE, adv_hdl);
    }
    else
    {
        bt_drv_reg_op_low_txpwr(false, FACTOR_ATTENUATION_18DBM, BLE_ADV_LOW_TXPWR_MODE, adv_hdl);
    }

    bt_drv_rf_ble_txpwr_set(idx, txpwr_dbm);
}

void btdrv_cap_delay_cal(void)
{
    uint16_t value = 0;;
    uint8_t tx_capdly, rx_capdly = 0;

    tx_capdly = value & 0xF;
    rx_capdly = ((value & 0xF0) >> 4);

    if ((0 == tx_capdly) || (0 == rx_capdly)) {
        BT_DRV_TRACE(0, "invalid efuse ,so use default value");
        tx_capdly = rx_capdly = 0x8;
    }

    BTRF_REG_SET_FIELD(0x295, 0xF, 6, tx_capdly);
    BTRF_REG_SET_FIELD(0x2C5, 0xF, 6, rx_capdly);
}

#define BTRF_DRV_TX_POWER_CALIB_3P5DBM        7
#define BTRF_DRV_TX_POWER_CALIB_3DBM          6
#define BTRF_DRV_TX_POWER_CALIB_2P5DBM        5
#define BTRF_DRV_TX_POWER_CALIB_2DBM          4
#define BTRF_DRV_TX_POWER_CALIB_1P5DBM        3
#define BTRF_DRV_TX_POWER_CALIB_1DBM          2
#define BTRF_DRV_TX_POWER_CALIB_0P5DBM        1
#define BTRF_DRV_TX_POWER_CALIB_0DBM          0
#define BTRF_DRV_TX_POWER_CALIB_N0P5DBM       -1
#define BTRF_DRV_TX_POWER_CALIB_N1DBM         -2
#define BTRF_DRV_TX_POWER_CALIB_N1P5DBM       -3
#define BTRF_DRV_TX_POWER_CALIB_N2DBM         -4
#define BTRF_DRV_TX_POWER_CALIB_N2P5DBM       -5
#define BTRF_DRV_TX_POWER_CALIB_N3DBM         -6
#define BTRF_DRV_TX_POWER_CALIB_N3P5DBM       -7

static void btdrv_set_dig_txpower(uint32_t gsg_nom)
{
    BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 0, gsg_nom);
    BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 10, gsg_nom);
    BTDIGITAL_REG_SET_FIELD(0xD0350344, 0x3FF, 0, gsg_nom);
}

static int32_t btdrv_txpower_calib_get_calib_val_by_efuse(void)
{
    uint16_t read_val;
    int32_t bt_pwr_freql, bt_pwr_freqm, bt_pwr_freqh, bt_pwr_sum;
    int bt_pwr_avg = 0;

    pmu_get_efuse(PMU_EFUSE_PAGE_BT_POWER, &read_val);
    TRACE(2, "%s efuse: 0x%x",__func__, read_val);
    bt_pwr_freql = read_val & 0x7;
    bt_pwr_freqm = (read_val >> 4) & 0x7;
    bt_pwr_freqh = (read_val >> 8) & 0x7;

    bt_pwr_freql = getbit(read_val, 3)  ? (-bt_pwr_freql) : bt_pwr_freql;
    bt_pwr_freqm = getbit(read_val, 7)  ? (-bt_pwr_freqm) : bt_pwr_freqm;
    bt_pwr_freqh = getbit(read_val, 11) ? (-bt_pwr_freqh) : bt_pwr_freqh;

    bt_pwr_sum = bt_pwr_freql + bt_pwr_freqm + bt_pwr_freqh;
    if (bt_pwr_sum >= 0) {
        bt_pwr_avg = (int32_t)((bt_pwr_sum / 3.0f) + 0.5f) ;
    } else {
        bt_pwr_avg = (int32_t)((bt_pwr_sum / 3.0f) - 0.5f) ;
    }

    BT_DRV_TRACE(1, "%s  bt_pwr_avg: %d", __func__, bt_pwr_avg);

    return bt_pwr_avg;
}

static void btdrv_txpower_calib_sel_calib_idx_2600zp(int32_t calib_val)
{
    calib_val = 2 * (2 - calib_val);
    switch (calib_val)
    {
        case BTRF_DRV_TX_POWER_CALIB_1DBM:
            break;
        case BTRF_DRV_TX_POWER_CALIB_2DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_3DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            BTRF_REG_SET_FIELD(0xCC, 0x1F, 4, 0x1);
            break;
        case BTRF_DRV_TX_POWER_CALIB_0DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x16);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N1DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x18);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N2DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x1A);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N3DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x1C);
            break;
        default:
            ASSERT(0, "2600ZP %s write error efuse", __func__);
            break;
    }
}

static void btdrv_txpower_calib_sel_calib_idx_2700L(int32_t calib_val)
{
    BT_DRV_TRACE(1, "%s  calib_val: %d", __func__, calib_val);
    switch (calib_val)
    {
        case BTRF_DRV_TX_POWER_CALIB_3P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            BTRF_REG_SET_FIELD(0xCC, 0x3, 4, 0x3);
            btdrv_set_dig_txpower(0x14);
            break;
        case BTRF_DRV_TX_POWER_CALIB_3DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            BTRF_REG_SET_FIELD(0xCC, 0x3, 4, 0x3);
            btdrv_set_dig_txpower(0x13);
            break;
        case BTRF_DRV_TX_POWER_CALIB_2P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            BTRF_REG_SET_FIELD(0xCC, 0x3, 4, 0x3);
            btdrv_set_dig_txpower(0x12);
            break;
        case BTRF_DRV_TX_POWER_CALIB_2DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            BTRF_REG_SET_FIELD(0xCC, 0x3, 4, 0x3);
            btdrv_set_dig_txpower(0x11);
            break;
        case BTRF_DRV_TX_POWER_CALIB_1P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            BTRF_REG_SET_FIELD(0xCC, 0x3, 4, 0x3);
            btdrv_set_dig_txpower(0x10);
            break;
        case BTRF_DRV_TX_POWER_CALIB_1DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            break;
        case BTRF_DRV_TX_POWER_CALIB_0P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x17);
            break;
        case BTRF_DRV_TX_POWER_CALIB_0DBM:
            // do nothing
            break;
        case BTRF_DRV_TX_POWER_CALIB_N0P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0xC);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N1DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N1P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x7);
            btdrv_set_dig_txpower(0xD);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N2DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            btdrv_set_dig_txpower(0xC);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N2P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x7);
            btdrv_set_dig_txpower(0xC);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N3DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            btdrv_set_dig_txpower(0xC);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N3P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            btdrv_set_dig_txpower(0xB);
            break;
        default:
            ASSERT(0, "2700L %s write error efuse", __func__);
            break;
    }
}

static void btdrv_txpower_calib_sel_calib_idx_2700IBP_2700H(int32_t calib_val)
{
    BT_DRV_TRACE(1, "%s  calib_val: %d", __func__, calib_val);
    switch (calib_val)
    {
        case BTRF_DRV_TX_POWER_CALIB_3P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x14);
            break;
        case BTRF_DRV_TX_POWER_CALIB_3DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x13);
            break;
        case BTRF_DRV_TX_POWER_CALIB_2P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x12);
            break;
        case BTRF_DRV_TX_POWER_CALIB_2DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x11);
            break;
        case BTRF_DRV_TX_POWER_CALIB_1P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            btdrv_set_dig_txpower(0x10);
            break;
        case BTRF_DRV_TX_POWER_CALIB_1DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x1F);
            break;
        case BTRF_DRV_TX_POWER_CALIB_0P5DBM:
            // do nothing
            break;
        case BTRF_DRV_TX_POWER_CALIB_0DBM:
            // do nothing
            break;
        case BTRF_DRV_TX_POWER_CALIB_N0P5DBM:
            btdrv_set_dig_txpower(0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N1DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x10);
            btdrv_set_dig_txpower(0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N1P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0xA);
            btdrv_set_dig_txpower(0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N2DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x8);
            btdrv_set_dig_txpower(0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N2P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x4);
            btdrv_set_dig_txpower(0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N3DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            btdrv_set_dig_txpower(0xE);
            break;
        case BTRF_DRV_TX_POWER_CALIB_N3P5DBM:
            BTRF_REG_SET_FIELD(0xCB, 0x1F, 4, 0x0);
            btdrv_set_dig_txpower(0xC);
            break;
        default:
            ASSERT(0, "2700L %s write error efuse", __func__);
            break;
    }
}

void btdrv_txpower_calib(void)
{
    int16_t calib_val;
    uint16_t packag_type;

    calib_val = btdrv_txpower_calib_get_calib_val_by_efuse();
    packag_type = bt_drv_get_packag_type();

    if (packag_type == BTDRV_PACKAG_2600ZP) {
        btdrv_txpower_calib_sel_calib_idx_2600zp(calib_val);
    } else if (packag_type == BTDRV_PACKAG_2700L) {
        btdrv_txpower_calib_sel_calib_idx_2700L(calib_val);
    } else if (packag_type == BTDRV_PACKAG_2700IBP || packag_type == BTDRV_PACKAG_2700H) {
        btdrv_txpower_calib_sel_calib_idx_2700IBP_2700H(calib_val);
    }
}

static void bt_drv_vco_vres_calib(void)
{
    uint16_t read_val;

    pmu_get_efuse(PMU_EFUSE_PAGE_RESERVED_26, &read_val);

    if (getbit(read_val, 0)) {
        BT_DRV_TRACE(0,"%s, check rx vco", __func__);
    }
}

void btdrv_rf_register_init(const uint16_t rf_init_tbl_p[][3], uint16_t tbl_size)
{
    uint8_t i;
    uint16_t value = 0;

    for (i=0; i< tbl_size; i++) {
        btdrv_write_rf_reg(rf_init_tbl_p[i][0],rf_init_tbl_p[i][1]);
        if(rf_init_tbl_p[i][2] !=0)
            btdrv_delay(rf_init_tbl_p[i][2]);//delay
        btdrv_read_rf_reg(rf_init_tbl_p[i][0],&value);
        BT_DRV_TRACE(2,"reg=%x,v=%x",rf_init_tbl_p[i][0],value);
    }
}

uint8_t btdrv_rf_init(void)
{
    uint16_t value = 0;
    const uint16_t (*rf_init_tbl_p)[3] = NULL;
    uint32_t tbl_size = 0;
    uint8_t i;
    uint16_t packag_type;

#ifndef CHIP_SUBSYS_SENS
    btdrv_rf_reg_checker();
#endif

    packag_type = bt_drv_get_packag_type();

    //system RF register init
    for (i = 0; i < ARRAY_SIZE(rf_sys_init_tbl); i++){
        btdrv_read_rf_reg(rf_sys_init_tbl[i].reg, &value);
        value = (value & ~rf_sys_init_tbl[i].mask) | (rf_sys_init_tbl[i].set & rf_sys_init_tbl[i].mask);
        if (rf_sys_init_tbl[i].delay){
            btdrv_delay(rf_sys_init_tbl[i].delay);
        }
        btdrv_write_rf_reg(rf_sys_init_tbl[i].reg, value);
    }
    //common RF register init
    rf_init_tbl_p = &rf_init_tbl_1_common[0];
    tbl_size = sizeof(rf_init_tbl_1_common)/sizeof(rf_init_tbl_1_common[0]);
    btdrv_rf_register_init(rf_init_tbl_p, tbl_size);

    if (packag_type == BTDRV_PACKAG_2700L) {
        rf_init_tbl_p = &rf_init_tbl_1_packey_type_2700L[0];
        tbl_size = sizeof(rf_init_tbl_1_packey_type_2700L)/sizeof(rf_init_tbl_1_packey_type_2700L[0]);
        btdrv_rf_register_init(rf_init_tbl_p, tbl_size);
    }
    if (packag_type == BTDRV_PACKAG_2600ZP) {
        rf_init_tbl_p = &rf_init_tbl_1_packey_type_2600ZP[0];
        tbl_size = sizeof(rf_init_tbl_1_packey_type_2600ZP)/sizeof(rf_init_tbl_1_packey_type_2600ZP[0]);
        btdrv_rf_register_init(rf_init_tbl_p, tbl_size);
    }
    if (packag_type == BTDRV_PACKAG_2700IBP) {
        rf_init_tbl_p = &rf_init_tbl_1_packey_type_2700IBP[0];
        tbl_size = sizeof(rf_init_tbl_1_packey_type_2700IBP)/sizeof(rf_init_tbl_1_packey_type_2700IBP[0]);
        btdrv_rf_register_init(rf_init_tbl_p, tbl_size);
    }
    if (packag_type == BTDRV_PACKAG_2700H) {
        rf_init_tbl_p = &rf_init_tbl_1_packey_type_2700H[0];
        tbl_size = sizeof(rf_init_tbl_1_packey_type_2700H)/sizeof(rf_init_tbl_1_packey_type_2700H[0]);
        btdrv_rf_register_init(rf_init_tbl_p, tbl_size);
    }

#ifdef __HW_AGC__
    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();

    if (metal_id < HAL_CHIP_METAL_ID_2) {
        rf_init_tbl_p = &rf_init_hw_agc_config_t0[0];
        tbl_size = sizeof(rf_init_hw_agc_config_t0)/sizeof(rf_init_hw_agc_config_t0[0]);
        btdrv_rf_register_init(rf_init_tbl_p, tbl_size);
    } else if (metal_id >= HAL_CHIP_METAL_ID_2) {
        rf_init_tbl_p = &rf_init_hw_agc_config_t2[0];
        tbl_size = sizeof(rf_init_hw_agc_config_t2)/sizeof(rf_init_hw_agc_config_t2[0]);
        btdrv_rf_register_init(rf_init_tbl_p, tbl_size);
    }
#endif

    //customer option
    bt_drv_rf_set_customer_config(NULL);
    //TX power init
    bt_drv_tx_pwr_init();
    //XTAL cap init
    bt_drv_rf_init_xtal_fcap();
    //vco vres calib by efuse only for version D
    bt_drv_vco_vres_calib();
#ifdef __BLE_NEW_SWAGC_MODE__
    bt_drv_reg_op_ble_sync_agc_mode_set(true);
#endif

    return 0;
}

void bt_drv_rf_set_bt_sync_agc_enable(bool enable)
{
    //TODO
    //return;
    uint16_t val = 0;
    btdrv_read_rf_reg(0x22,&val);
    if(enable)
    {
        //open rf new sync agc mode
        val |= (1<<3);
    }
    else
    {
        //close rf new sync agc mode
        val &= ~(1<<3);
    }
    btdrv_write_rf_reg(0x22,val);
}

void bt_drv_rf_set_ble_sync_agc_enable(bool enable)
{
    //TODO
    return;
    uint16_t val = 0;
    btdrv_read_rf_reg(0x36,&val);
    if(enable)
    {
        //open rf new sync agc mode
        val |= (1<<2);
    }
    else
    {
        //close rf new sync agc mode
        val &= ~(1<<2);
    }
    btdrv_write_rf_reg(0x36,val);
}

//switch old swagc and new sync swagc
void bt_drv_select_agc_mode(enum BT_WORK_MODE_T mode)
{
#ifdef __NEW_SWAGC_MODE__
    static uint8_t agc_mode_bak = SWAGC_INIT_MODE;
    uint8_t agc_mode = OLD_SWAGC_MODE;

    switch(mode)
    {
        case BT_A2DP_WORK_MODE:
            agc_mode = NEW_SYNC_SWAGC_MODE;
            break;
        case BT_IDLE_MODE:
        case BT_HFP_WORK_MODE:
            agc_mode = OLD_SWAGC_MODE;
            break;
        default:
            BT_DRV_TRACE(1,"BT_DRV:set error mork mode=%d",mode);
            break;
    }

    if(agc_mode_bak != agc_mode)
    {
        agc_mode_bak = agc_mode;
        BT_DRV_TRACE(1,"BT_DRV:use SW AGC mode=%d[2:OLD,1:NEW sync]",agc_mode);
        bt_drv_reg_op_swagc_mode_set(agc_mode);
    }
#endif
}

//factor=N/512
void btdrv_set_bdr_ble_txpower(uint8_t txpwr_idx, uint16_t n)
{
    uint32_t reg_base = 0xd03500C4;
    uint32_t reg;
    if(txpwr_idx > 7)
    {
        BT_DRV_TRACE(1, "%s txpwr idx err:%d\n", __func__, txpwr_idx);
        return;
    }

    if(n > 1023)
    {
        BT_DRV_TRACE(1, "%s n err:%d\n", __func__, n);
        return;
    }

    reg = (txpwr_idx / 3) * 4 + reg_base;

    BTDIGITAL_REG_SET_FIELD(reg, 0x3ff, (txpwr_idx % 3) * 10, n);
}

#ifdef TX_PULLING_CAL

const uint16_t pulling_cal_rf_set[][2] =
{
    //[tstbuf]
    {0x2b, 0x0700},
    {0x24, 0x0000},
    //{0x51, 0x27f9},
    {0x81, 0x0009},
    {0x95, 0x0031},
    {0xbf, 0x3c32},
    //{0xa6, 0x5e6a},  //turn off tst_buf
    {0x160,0x100c},

    //[ddrpll prepare]
    {0xe7, 0xe04e},
    {0xe8, 0x4030},
    {0xdd, 0x0000},
    {0xde, 0x9800},
    {0xdf, 0x0003},
    {0xe0, 0x0110},

    //[pulling mixer]
    {0xd1, 0x02c8},
    {0xd0, 0x0224},

    //[adc rst]
    {0x0f, 0x0330},
    {0x2b, 0x0710},
    {0x2b, 0x0730},
    {0x23, 0x28BE},
    {0xC8, 0x5B0D},
    {0x27, 0x2A10},
    {0x27, 0x2A30},
    {0x146,0x5FDB},
    {0x2c, 0x120e},
};

const uint16_t pulling_cal_rf_store[][1] =
{
    {0x2b},
    {0x2c},
    {0x24},
    //{0x51},
    {0x81},
    {0x95},
    {0xbf},
    {0xa6},
    {0x160},
    {0xe7},
    {0xe8},
    {0xdd},
    {0xde},
    {0xdf},
    {0xe0},
    {0xd1},
    {0xd0},
    {0x0f},
    {0xc1},
    {0x23},
    {0xC8},
    {0x27},
    {0x15c},
    {0x147},
    {0x146},
    {0x2c}
};

#define BT_RF_DIG_TXPULLING_CALIB_TXPOWER_REDUCE_1P5DBM     4

void btdrv_tx_pulling_cal(void)
{
    int i = 0;
    uint16_t value = 0;
    uint32_t pulling_dig_store[12];
    const uint16_t (*pulling_rf_store_p)[1];
    uint16_t gsg_nom_q;
    uint16_t gsg_nom_i;

    pulling_rf_store_p = &pulling_cal_rf_store[0];
    uint32_t rf_store_tbl_size = ARRAY_SIZE(pulling_cal_rf_store);
    uint16_t pulling_rf_local[rf_store_tbl_size];

    BT_DRV_TRACE(0,"rx reg_store:\n");
    for(i=0; i< rf_store_tbl_size; i++) {
        btdrv_read_rf_reg(pulling_rf_store_p[i][0],&value);
        pulling_rf_local[i] = value;
        BT_DRV_TRACE(2,"rx reg=%x,v=%x",pulling_rf_store_p[i][0],value);
    }

    pulling_dig_store[0] =  BTDIGITAL_REG(0xd0350248);
    pulling_dig_store[1] =  BTDIGITAL_REG(0xd0340020);
    pulling_dig_store[2] =  BTDIGITAL_REG(0xd0220c00);
    pulling_dig_store[3] =  BTDIGITAL_REG(0xD0330020);
    pulling_dig_store[4] =  BTDIGITAL_REG(0xD0350218);
    pulling_dig_store[5] =  BTDIGITAL_REG(0xD0350300);
    pulling_dig_store[6] =  BTDIGITAL_REG(0xD0350308);
    pulling_dig_store[7] =  BTDIGITAL_REG(0xD0350320);
    pulling_dig_store[8] =  BTDIGITAL_REG(0xD035031C);
    pulling_dig_store[9] =  BTDIGITAL_REG(0xD0330058);
    pulling_dig_store[10] = BTDIGITAL_REG(0xD0330060);
    pulling_dig_store[11] = BTDIGITAL_REG(0xD0330064);

    BT_DRV_TRACE(1,"0xd0350248:%x\n",pulling_dig_store[0]);
    BT_DRV_TRACE(1,"0xd0340020:%x\n",pulling_dig_store[1]);
    BT_DRV_TRACE(1,"0xd0220c00:%x\n",pulling_dig_store[2]);
    BT_DRV_TRACE(1,"0xD0330020:%x\n",pulling_dig_store[3]);
    BT_DRV_TRACE(1,"0xD0350218:%x\n",pulling_dig_store[4]);
    BT_DRV_TRACE(1,"0xD0350300:%x\n",pulling_dig_store[5]);
    BT_DRV_TRACE(1,"0xD0350308:%x\n",pulling_dig_store[6]);
    BT_DRV_TRACE(1,"0xD0350320:%x\n",pulling_dig_store[7]);
    BT_DRV_TRACE(1,"0xD035031C:%x\n",pulling_dig_store[8]);
    BT_DRV_TRACE(1,"0xD0330058:%x\n",pulling_dig_store[9]);
    BT_DRV_TRACE(1,"0xD0330060:%x\n",pulling_dig_store[10]);
    BT_DRV_TRACE(1,"0xD0330064:%x\n",pulling_dig_store[11]);

    const uint16_t (*pulling_rf_set_p)[2];
    uint32_t pulling_set_tbl_size = 0;
    pulling_rf_set_p = &pulling_cal_rf_set[0];
    pulling_set_tbl_size = ARRAY_SIZE(pulling_cal_rf_set);
    BT_DRV_TRACE(0,"rx reg_set:\n");
    for(i=0; i< pulling_set_tbl_size; i++) {
        btdrv_write_rf_reg(pulling_rf_set_p[i][0],pulling_rf_set_p[i][1]);
        btdrv_delay(1);
        //BTRF_REG_DUMP(pulling_rf_set_p[i][0]);
    }

    // fix modulation factor
    BTDIGITAL_REG_WR(0xD0350320, 0x00040401);
    BTDIGITAL_REG_WR(0xD035031C, 0x00000000);

    BTRF_REG_SET_FIELD(0x160, 0x1, 7, 1);
    BTRF_REG_SET_FIELD(0x160, 0x3, 9, 3);

    BTDIGITAL_REG_SET_FIELD(0xd0340020, 0x1, 0, 0);
    BTDIGITAL_REG_SET_FIELD(0xd0340020, 0x1, 25, 1);
    BTDIGITAL_REG_SET_FIELD(0xd0350240, 0x1, 12, 1);

    BTDIGITAL_REG_SET_FIELD(0xD0330020, 0x1, 21, 1);
    BTDIGITAL_REG_SET_FIELD(0xD0330020, 0x1, 1, 1);
    BTDIGITAL_REG_WR(0xD0350218, 0x583);
    BTDIGITAL_REG_WR(0xD0350300, 0x3FF00405);

    BTDIGITAL_REG_GET_FIELD(0xD0350308, 0x1f, 0, gsg_nom_q);
    BTDIGITAL_REG_GET_FIELD(0xD0350308, 0x1f, 10, gsg_nom_i);
    gsg_nom_q -= BT_RF_DIG_TXPULLING_CALIB_TXPOWER_REDUCE_1P5DBM;
    gsg_nom_i -= BT_RF_DIG_TXPULLING_CALIB_TXPOWER_REDUCE_1P5DBM;
    BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x1F, 0, gsg_nom_q);
    BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x1F, 10, gsg_nom_i);

    bt_tx_pulling_calib();

    BTRF_REG_SET_FIELD(0x2d, 0x3, 14, 3);

    BT_DRV_TRACE(0,"reg_reset:\n");
    for(i=0; i< rf_store_tbl_size; i++) {
        btdrv_write_rf_reg(pulling_rf_store_p[i][0],pulling_rf_local[i]);
        btdrv_delay(1);
        //BTRF_REG_DUMP(pulling_rf_store_p[i][0]);
    }

    BTDIGITAL_REG_WR(0xd0350248, pulling_dig_store[0]);
    BTDIGITAL_REG_WR(0xd0340020, pulling_dig_store[1]);
    BTDIGITAL_REG_WR(0xd0220c00, pulling_dig_store[2]);
    BTDIGITAL_REG_WR(0xD0330020, pulling_dig_store[3]);
    BTDIGITAL_REG_WR(0xD0350218, pulling_dig_store[4]);
    BTDIGITAL_REG_WR(0xD0350300, pulling_dig_store[5]);
    BTDIGITAL_REG_WR(0xD0350308, pulling_dig_store[6]);
    BTDIGITAL_REG_WR(0xD0350320, pulling_dig_store[7]);
    BTDIGITAL_REG_WR(0xD035031C, pulling_dig_store[8]);
    BTDIGITAL_REG_WR(0xD0330058, pulling_dig_store[9]);
    BTDIGITAL_REG_WR(0xD0330060, pulling_dig_store[10]);
    BTDIGITAL_REG_WR(0xD0330064, pulling_dig_store[11]);

    BT_DRV_TRACE(1,"0xd0350248:%x\n",pulling_dig_store[0]);
    BT_DRV_TRACE(1,"0xd0340020:%x\n",pulling_dig_store[1]);
    BT_DRV_TRACE(1,"0xd0220c00:%x\n",pulling_dig_store[2]);
    BT_DRV_TRACE(1,"0xD0330020:%x\n",pulling_dig_store[3]);
    BT_DRV_TRACE(1,"0xD0350218:%x\n",pulling_dig_store[4]);
    BT_DRV_TRACE(1,"0xD0350300:%x\n",pulling_dig_store[5]);
    BT_DRV_TRACE(1,"0xD0350308:%x\n",pulling_dig_store[6]);
    BT_DRV_TRACE(1,"0xD0350320:%x\n",pulling_dig_store[7]);
    BT_DRV_TRACE(1,"0xD035031C:%x\n",pulling_dig_store[8]);
    BT_DRV_TRACE(1,"0xD0330058:%x\n",pulling_dig_store[9]);
    BT_DRV_TRACE(1,"0xD0330060:%x\n",pulling_dig_store[10]);
    BT_DRV_TRACE(1,"0xD0330064:%x\n",pulling_dig_store[11]);
}
#endif

struct RX_RCCAL_CALIB
{
    uint16_t rf_reg;
    uint16_t mask;
    uint16_t shift;
};

static struct RX_RCCAL_CALIB RX_RCCAL_CALIB_RF_REG_TBL[] =
{
    // flt cap
    {0x0027, 0xFF, 8},
    {0x0144, 0xFF, 0},
    // i2v cap
    {0x021F, 0x7F, 0},
    {0x0222, 0x7F, 0},
    {0x0225, 0x7F, 0},
    {0x0228, 0x7F, 0},
    {0x022b, 0x7F, 0},
    {0x022e, 0x7F, 0},
    {0x0231, 0x7F, 0},
    {0x0234, 0x7F, 0},
    // adc cap
    {0x00C2, 0xFF, 0},
    {0x00C2, 0x3F, 8},
    {0x0144, 0xFF, 8},
    {0x0145, 0x3F, 3},
};

void btdrv_rxbb_rccal(void)
{
    uint16_t value = 0;
    uint16_t rccal_count = 0;
    const uint16_t base_count = 0xA2;
    float quotient = 0.0f;
    float factor;
    uint16_t read_val;
    uint8_t i;

    uint16_t rf_store_104;
    uint16_t rf_store_101;
    uint32_t dig_store;

    dig_store = BTDIGITAL_REG(0xD0220C00);
    btdrv_read_rf_reg(0x104, &rf_store_104);
    btdrv_read_rf_reg(0x101, &rf_store_101);

    BTDIGITAL_REG_WR(0xD0220C00, 0x000A0080);       //rx on
    btdrv_delay(10);

    BTRF_REG_SET_FIELD(0x104, 0x1, 9, 1);           //reg_rcosc_pu
    btdrv_delay(1);
    BTRF_REG_SET_FIELD(0x101, 0x1, 7, 1);           //reg_rccal_en
    btdrv_delay(10);

    btdrv_read_rf_reg(0x117, &value);
    BT_DRV_TRACE(2, "%s rf_103=0x%x", __func__, value);
    //bit12: dig_rccal_finish
    if (getbit(value, 15)) {
        rccal_count = value & 0x7FFF;                //dig_rccal_count[11:0]
        BT_DRV_TRACE(2, "%s rccal_count=0x%x", __func__, rccal_count);
    } else {
        BT_DRV_TRACE(1, "%s dig rccal not finish!!!", __func__);
        goto exit;
    }

    quotient = (float)((float)rccal_count / (float)base_count);
    //TRACE(1, "quotient=%f", (double)quotient);
    if ((quotient > 0.7f) && (quotient < 1.3f)) {
        factor = quotient;
    } else {
        // use efuse calib
        BT_DRV_TRACE(0, "quotient not in range [0.7, 1.3]");
        factor = 1.0f;
    }

    //system RF register init
    for (i = 0; i < ARRAY_SIZE(RX_RCCAL_CALIB_RF_REG_TBL); i++){
        BTRF_REG_GET_FIELD(RX_RCCAL_CALIB_RF_REG_TBL[i].rf_reg, RX_RCCAL_CALIB_RF_REG_TBL[i].mask,
            RX_RCCAL_CALIB_RF_REG_TBL[i].shift, read_val);
        read_val = (uint16_t)((read_val / factor) + 0.5);
        BTRF_REG_SET_FIELD(RX_RCCAL_CALIB_RF_REG_TBL[i].rf_reg, RX_RCCAL_CALIB_RF_REG_TBL[i].mask,
            RX_RCCAL_CALIB_RF_REG_TBL[i].shift, read_val);
    }

exit:
    //reset
    BTDIGITAL_REG_WR(0xD0220C00, dig_store);
    btdrv_write_rf_reg(0x101, rf_store_101);
    btdrv_write_rf_reg(0x104, rf_store_104);
}

__WEAK int pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse)
{
    return 0;
}

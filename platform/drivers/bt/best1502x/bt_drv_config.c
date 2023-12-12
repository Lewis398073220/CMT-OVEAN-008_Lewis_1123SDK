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
#include <besbt_string.h>
#include "plat_types.h"
#include "tgt_hardware.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "bt_drv.h"
#include "hal_timer.h"
#include "hal_chipid.h"
#include "hal_psc.h"
#include "bt_drv_1502x_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_internal.h"
#include "bt_1502x_reg_map.h"
#include "pmu.h"

extern void btdrv_send_cmd(uint16_t opcode,uint8_t cmdlen,const uint8_t *param);
extern void btdrv_write_memory(uint8_t wr_type,uint32_t address,const uint8_t *value,uint8_t length);

typedef struct
{
    uint8_t is_act;
    uint16_t opcode;
    uint8_t parlen;
    const uint8_t *param;

} BTDRV_CFG_TBL_STRUCT;


#define BTDRV_CONFIG_ACTIVE   1
#define BTDRV_CONFIG_INACTIVE 0
#define BTDRV_INVALID_TRACE_LEVEL  0xFF
/*
[0][0] = 63, [0][1] = 0,[0][2] = (-80),           472d
[1][0] = 51, [2][1] = 0,[2][2] = (-80),          472b
[2][0] = 42, [4][1] = 0,[4][2] = (-75),           4722
[3][0] = 36, [6][1] = 0,[6][2] = (-55),           c712
[4][0] = 30, [8][1] = 0,[8][2] = (-40),           c802
[5][0] = 21,[10][1] = 0,[10][2] = 0x7f,         c102
[6][0] = 12,[11][1] = 0,[11][2] = 0x7f,       c142
[7][0] = 3,[13][1] = 0,[13][2] = 0x7f,        c1c2
[8][0] = -3,[14][1] = 0,[14][2] = 0x7f};      c0c2
*/
static uint8_t g_controller_trace_level = BTDRV_INVALID_TRACE_LEVEL;
const int8_t btdrv_rf_env[]=
{
    0x01,0x00,  //rf api
    0x01,   //rf env
    185,     //rf length
    BT_MAX_TX_PWR_IDX,     //txpwr_max
    -1,    ///rssi high thr
    -2,   //rssi low thr
    -100,  //rssi interf thr
    0xf,  //rssi interf gain thr
    2,  //wakeup delay
    -60, //le low rssi thr
    -45, //le high rssi thr
    0xe8,0x3,    //ble agc inv thr
#ifdef __HW_AGC__
    0x1,//hw_sw_agc_flag hw
#else
    0x0,//hw_sw_agc_flag sw
#endif

    0xff,//sw gain set
    0xff,    //sw gain set
    -85,//bt_inq_page_iscan_pscan_dbm
    0x7f,//ble_scan_adv_dbm
    1,      //sw gain reset factor
    1,    //bt sw gain cntl enable
    1,   //ble sw gain cntl en
    1,  //bt interfere  detector en
    0,  //ble interfere detector en

#ifdef __HW_AGC__
    49,0,0,
    43,0,0,
    37,0,0,
    32,0,0,
    27,0,0,
    22,0,0,
    16,0,0,
    5,0,0,  //rx hwgain tbl ptr hw
#else
    0,0,0,
    3,3,12,
    6,6,28,
    9,9,28,
    12,12,28,
    15,15,28,
    18,18,28,
    21,21,28,
#endif

    0x7f,24,0x7f,
    0x7f,27,0x7f,
    0x7f,30,0x7f,
    0x7f,33,0x7f,
    0x7f,36,0x7f,
    0x7f,39,0x7f,
    0x7f,42,0x7f,  //rx hwgain tbl ptr sw

    53,0,-80,
    48,0,-80,
    37,0,-80,
    33,0,-80,
    26,0,-80,
    14,0,-80,
    -1,0,-80,
    -12,0,-80,


    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,  //rx gain tbl ptr

    -87,-87,
    -82,-82,
    -76,-76,
    -72,-72,
    -62,-62,
    -45,-45,
    -34,-34,
    -25,0x7f,


    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //rx gain ths tbl ptr

    0,0,
    0,0,
    0,0,
    0,0,
    0,0,
    0,1,
    0,2,
    0,2,
    0,2,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //flpha filter factor ptr
    -23,-20,-17,-14,-11,-8,-5,-2,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,   //tx pw onv tbl ptr
};

const int8_t btdrv_rxgain_gain_ths_3m[] = {
    -79,-79,
    -74,-74,
    -68,-68,
    -64,-64,
    -54,-54,
    -37,-37,
    -26,-26,

    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //rx gain ths tbl ptr
};

const int8_t btdrv_rxgain_ths_tbl_le[0xf * 2] = {
    -92,-92,
    -81,-81,
    -70,-70,
    -59,-59,
    -53,-53,
    -45,-45,
    -30,-30,
    -25,0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f, //ble rx gain ths tbl ptr
};

const int8_t btdrv_rxgain_ths_tbl_le_2m[0xf * 2] = {
    -92,-92,
    -81,-81,
    -70,-70,
    -59,-59,
    -53,-53,
    -45,-45,
    -30,-30,
    -25,0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f,
    0x7f, 0x7f, //ble rx gain ths tbl ptr
};

//ble txpwr convert
int8_t  btdrv_txpwr_conv_tbl[8] = {
        [0] = -127,
        [1] = -127,
        [2] = -127,
        [3] = -127,
        [4] = -5,
        [5] = 0,
        [6] = 5,
        [7] = 10
};

const int8_t btdrv_afh_env[] =
{
    0x02,0x00,   //afh env
    0x00,      //ignore
    33,          //length
    5,   //nb_reass_chnl
    10,  //win_len
    -70,  //rf_rssi_interf_thr
    10,  //per_thres_bad
    20,  //reass_int
    20,   //n_min
    20,   //afh_rep_intv_max
    96,    //ths_min
    2,   //chnl_assess_report_style
    15,  //chnl_assess_diff_thres
    60, // chnl_assess_interfere_per_thres_bad
    9,  //chnl_assess_stat_cnt_max
    -9,  //chnl_assess_stat_cnt_min
    1,2,3,2,1,   //chnl_assess_stat_cnt_inc_mask[5]
    1,2,3,2,1,    //chnl_assess_stat_cnt_dec_mask
    0xd0,0x7,      //chnl_assess_timer
    -48,        //chnl_assess_min_rssi
    0x64,0,   //chnl_assess_nb_pkt
    0x32,0,     //chnl_assess_nb_bad_pkt
    6,    //chnl_reassess_cnt_val
    0x3c,0,     //chnl_assess_interfere_per_thres_bad
};

const uint8_t lpclk_drift_jitter[] =
{
    0xfa,0x00,  //  drift  250ppm
    0x0a,0x00    //jitter  +-10us

};

const uint8_t  wakeup_timing[] =
{
    0xe8,0x3,   //twrm
    0xe8,0x3,    //twosc
    0xe8,0x3,    //twext
    0x3,           //rwip_prog_delay
    0x2,         //clk_corr
    0x90,0x01,//sleep_algo
    0x1,          //cpu_idle_en
    0xc,//wait_26m_cycle
    0x0,0x0,0x0,0x0//poweroff_flag
};


uint8_t  sleep_param[] =
{
    0,    // sleep_en;
    1,    // exwakeup_en;
    0xd0,0x7,    //  lpo_calib_interval;   lpo calibration interval
    0x32,0,0,0,    // lpo_calib_time;  lpo count lpc times
};

const uint16_t me_bt_default_page_timeout = 0x2000;

const uint8_t  sync_config[] =
{
    1,1,   //sco path config   0:hci  1:pcm
    0,      //sync use max buff length   0:sync data length= packet length 1:sync data length = host sync buff len
    0,        //cvsd bypass     0:cvsd2pcm   1:cvsd transparent
};

//pcm general ctrl
#define PCM_PCMEN_POS            15
#define PCM_LOOPBCK_POS          14
#define PCM_MIXERDSBPOL_POS      11
#define PCM_MIXERMODE_POS        10
#define PCM_STUTTERDSBPOL_POS    9
#define PCM_STUTTERMODE_POS      8
#define PCM_CHSEL_POS            6
#define PCM_MSTSLV_POS           5
#define PCM_PCMIRQEN_POS         4
#define PCM_DATASRC_POS          0


//pcm phy ctrl
#define PCM_LRCHPOL_POS     15
#define PCM_CLKINV_POS      14
#define PCM_IOM_PCM_POS     13
#define PCM_BUSSPEED_LSB    10
#define PCM_SLOTEN_MASK     ((uint32_t)0x00000380)
#define PCM_SLOTEN_LSB      7
#define PCM_WORDSIZE_MASK   ((uint32_t)0x00000060)
#define PCM_WORDSIZE_LSB    5
#define PCM_DOUTCFG_MASK    ((uint32_t)0x00000018)
#define PCM_DOUTCFG_LSB     3
#define PCM_FSYNCSHP_MASK   ((uint32_t)0x00000007)
#define PCM_FSYNCSHP_LSB    0

/// Enumeration of PCM status
enum PCM_STAT
{
    PCM_DISABLE = 0,
    PCM_ENABLE
};

/// Enumeration of PCM channel selection
enum PCM_CHANNEL
{
    PCM_CH_0 = 0,
    PCM_CH_1
};

/// Enumeration of PCM role
enum PCM_MSTSLV
{
    PCM_SLAVE = 0,
    PCM_MASTER
};

/// Enumeration of PCM data source
enum PCM_SRC
{
    PCM_SRC_DPV = 0,
    PCM_SRC_REG
};

/// Enumeration of PCM left/right channel selection versus frame sync polarity
enum PCM_LR_CH_POL
{
    PCM_LR_CH_POL_RIGHT_LEFT = 0,
    PCM_LR_CH_POL_LEFT_RIGHT
};

/// Enumeration of PCM clock inversion
enum PCM_CLK_INV
{
    PCM_CLK_RISING_EDGE = 0,
    PCM_CLK_FALLING_EDGE
};

/// Enumeration of PCM mode selection
enum PCM_MODE
{
    PCM_MODE_PCM = 0,
    PCM_MODE_IOM
};

/// Enumeration of PCM bus speed
enum PCM_BUS_SPEED
{
    PCM_BUS_SPEED_128k = 0,
    PCM_BUS_SPEED_256k,
    PCM_BUS_SPEED_512k,
    PCM_BUS_SPEED_1024k,
    PCM_BUS_SPEED_2048k
};

/// Enumeration of PCM slot enable
enum PCM_SLOT
{
    PCM_SLOT_NONE = 0,
    PCM_SLOT_0,
    PCM_SLOT_0_1,
    PCM_SLOT_0_2,
    PCM_SLOT_0_3
};

/// Enumeration of PCM word size
enum PCM_WORD_SIZE
{
    PCM_8_BITS = 0,
    PCM_13_BITS,
    PCM_14_BITS,
    PCM_16_BITS
};

/// Enumeration of PCM DOUT pad configuration
enum PCM_DOUT_CFG
{
    PCM_OPEN_DRAIN = 0,
    PCM_PUSH_PULL_HZ,
    PCM_PUSH_PULL_0
};

/// Enumeration of PCM FSYNC physical shape
enum PCM_FSYNC
{
    PCM_FSYNC_LF = 0,
    PCM_FSYNC_FR,
    PCM_FSYNC_FF,
    PCM_FSYNC_LONG,
    PCM_FSYNC_LONG_16
};

const uint32_t pcm_setting[] =
{
//pcm_general_ctrl
    (PCM_DISABLE<<PCM_PCMEN_POS) |                      //enable auto
    (PCM_DISABLE << PCM_LOOPBCK_POS)  |                 //LOOPBACK test
    (PCM_DISABLE << PCM_MIXERDSBPOL_POS)  |
    (PCM_DISABLE << PCM_MIXERMODE_POS)  |
    (PCM_DISABLE <<PCM_STUTTERDSBPOL_POS) |
    (PCM_DISABLE <<PCM_STUTTERMODE_POS) |
    (PCM_CH_0<< PCM_CHSEL_POS) |
    (PCM_MASTER<<PCM_MSTSLV_POS) |                      //BT clock
    (PCM_DISABLE << PCM_PCMIRQEN_POS) |
    (PCM_SRC_DPV<<PCM_DATASRC_POS),

//pcm_phy_ctrl
    (PCM_LR_CH_POL_RIGHT_LEFT << PCM_LRCHPOL_POS) |
    (PCM_CLK_FALLING_EDGE << PCM_CLKINV_POS) |
    (PCM_MODE_PCM << PCM_IOM_PCM_POS) |
    (PCM_BUS_SPEED_2048k << PCM_BUSSPEED_LSB) |         //8k sample rate; 2048k = slot_num * sample_rate * bit= 16 * 8k * 16
    (PCM_SLOT_0_1 << PCM_SLOTEN_LSB) |
    (PCM_16_BITS << PCM_WORDSIZE_LSB) |
    (PCM_PUSH_PULL_0 << PCM_DOUTCFG_LSB) |
    (PCM_FSYNC_LF << PCM_FSYNCSHP_LSB),
};

const uint8_t local_feature[] =
{
#if defined(__3M_PACK__)
    0xBF, 0xeE, 0xCD,0xFe,0xdb,0xFd,0x7b,0x87
#else
    0xBF, 0xeE, 0xCD,0xFa,0xdb,0xbd,0x7b,0x87

    //0xBF,0xFE,0xCD,0xFa,0xDB,0xFd,0x73,0x87   // disable simple pairing
#endif
};

const uint8_t local_ex_feature_page1[] =
{
    1,   //page
#if (defined(CTKD_ENABLE)&&defined(IS_CTKD_OVER_BR_EDR_ENABLED))
    0x0F,0,0,0,0,0,0,0,   //page 1 feature
#else
    2,0,0,0,0,0,0,0,   //page 1 feature
#endif
};

const uint8_t local_ex_feature_page2[] =
{
    2,   //page
    0x1f,0x03,0x00,0x00,0x00,0x00,0x00,0x00,   //page 2 feature
};

const uint8_t bt_rf_timing_t0[] =
{
    0x37,// rxpwrupct;
    0x0C,// txpwrdnct;
    0x32,// txpwrupct;
    0x00,// rxpathdly;
    0x10,// txpathdly;
    0x00,// sync_position;
    0x12,// edr_rxgrd_timeout;
};

const uint8_t bt_rf_timing_t1[] =
{
    0x37,// rxpwrupct;
    0x0C,// txpwrdnct;
    0x32,// txpwrupct;
    0x00,// rxpathdly;
    0x10,// txpathdly;
    0x00,// sync_position;
    0x12,// edr_rxgrd_timeout;
};

// LE 1M:uncoded PHY at 1Mbps
// LE 2M:uncoded PHY at 2Mbps
const uint8_t ble_rf_timing_t0[] =
{
    0x00,  //LE 1M syncposition0
    0x37,  //LE 1M rxpwrup0
    0x0C,  //LE 1M txpwrdn0
    0x32,  //LE 1M txpwrup0

    0x00,  //LE 2M syncposition1
    0x37,  //LE 2M rxpwrup1
    0x0C,  //LE 2M txpwrdn1
    0x32,  //LE 2M txpwrup1

    0x00,  //coded PHY at 125kbps and 500kbps syncposition2
    0x41,  //coded PHY at 125kbps and 500kbps rxpwrup2
    0x0C,  //coded PHY at 125kbps txpwrdn2
    0x32,  //coded PHY at 125kbp txpwrup2

    0x0C,  //coded PHY at 500kbps txpwrdn3
    0x32,  //coded PHY at 500kbps txpwrup3

    0x00,  //LE 1M rfrxtmda0
    0x04,  //LE 1M rxpathdly0
    0x0c,  //LE 1M txpathdly0

    0x00,  //LE 2M rfrxtmda1
    0x02,  //LE 2M rxpathdly1
    0x0d,  //LE 2M txpathdly1

    0x14,  //coded PHY at 125kbps rxflushpathdly2
    0xa0,  //coded PHY at 125kbps rfrxtmda2
    0x14,  //coded PHY at 125kbps rxpathdly2
    0x0c,  //coded PHY at 125kbps txpathdly2

    0x26,  //coded PHY at 500kbps rxflushpathdly3
    0x00,  //coded PHY at 500kbps rfrxtmda3
    0x0c,  //coded PHY at 500kbps txpathdly3

    /*   AoA/AoD Registers */
    0x08,  //rxsampstinst01us
    0x18,  //rxswstinst01us
    0x19,  //txswstinst01us

    0x08,  //rxsampstinst02us
    0x18,  //rxswstinst02us
    0x19,  //txswstinst02us

    0x08,  //rxsampstinst11us
    0x18,  //rxswstinst11us
    0x19,  //txswstinst11us

    0x08,  //rxsampstinst12us
    0x18,  //rxswstinst12us
    0x19,  //txswstinst12us

    0x00,  //rxprimidcntlen
    0x00,  //rxprimantid
    0x00,  //txprimidcntlen
    0x00,  //txprimantid
};

const uint8_t ble_rf_timing_t1[] =
{
    0x00,  //LE 1M syncposition0
    0x37,  //LE 1M rxpwrup0
    0x0C,  //LE 1M txpwrdn0
    0x32,  //LE 1M txpwrup0

    0x00,  //LE 2M syncposition1
    0x37,  //LE 2M rxpwrup1
    0x0C,  //LE 2M txpwrdn1
    0x32,  //LE 2M txpwrup1

    0x00,  //coded PHY at 125kbps and 500kbps syncposition2
    0x37,  //coded PHY at 125kbps and 500kbps rxpwrup2
    0x0C,  //coded PHY at 125kbps txpwrdn2
    0x32,  //coded PHY at 125kbp txpwrup2

    0x0C,  //coded PHY at 500kbps txpwrdn3
    0x32,  //coded PHY at 500kbps txpwrup3

    0x00,  //LE 1M rfrxtmda0
    0x04,  //LE 1M rxpathdly0
    0x0c,  //LE 1M txpathdly0

    0x00,  //LE 2M rfrxtmda1
    0x02,  //LE 2M rxpathdly1
    0x0d,  //LE 2M txpathdly1

    0x14,  //coded PHY at 125kbps rxflushpathdly2
    0xa0,  //coded PHY at 125kbps rfrxtmda2
    0x14,  //coded PHY at 125kbps rxpathdly2
    0x0c,  //coded PHY at 125kbps txpathdly2

    0x26,  //coded PHY at 500kbps rxflushpathdly3
    0x00,  //coded PHY at 500kbps rfrxtmda3
    0x0c,  //coded PHY at 500kbps txpathdly3

    /*   AoA/AoD Registers */
    0x08,  //rxsampstinst01us
    0x18,  //rxswstinst01us
    0x19,  //txswstinst01us

    0x08,  //rxsampstinst02us
    0x18,  //rxswstinst02us
    0x19,  //txswstinst02us

    0x08,  //rxsampstinst11us
    0x18,  //rxswstinst11us
    0x19,  //txswstinst11us

    0x08,  //rxsampstinst12us
    0x18,  //rxswstinst12us
    0x19,  //txswstinst12us

    0x00,  //rxprimidcntlen
    0x00,  //rxprimantid
    0x00,  //txprimidcntlen
    0x00,  //txprimantid
};

const uint8_t bt_common_setting_1502x[64] =
{
    0x03, //trace_level
    0x01, //trace_output
    0x00,0x00, //tports_level
    0x01, //power_control_type
    0x00, //close_loopbacken_flag
    0x00, //force_max_slot
    0x06, //wesco_nego
    0x05, //force_max_slot_in_sco
    0x01, //esco_retx_after_establish
    0x00,0x00, //sco_start_delay
    0x01, //msbc_pcmdout_zero_flag
    0x01, //master_2_poll
    0x01, //pca_disable_in_nosync
    BT53_VERSION, //version_major
    0x01, //version_minor
    0x15, //version_build
    0xb0,0x02, //comp_id
    0x00, //ptt_check
    0x00, //address_reset
    0x00,0x00, //hci_timeout_sleep
    0x00, //key_gen_after_reset
    0x01, //intersys_1_wakeup
    0x05,0x00, //lmp_to_before_complete
    0x08, //fastpcm_interval
    0x01, //lm_env_reset_local_name
    0x00, //detach_directly
    0x00, //clk_off_force_even
    0xc8, //seq_error_num
    0x03, //delay_process_lmp_type
    0x0a, //delay_process_lmp_to 10*100 halt slot
    0x01, //ignore_pwr_ctrl_sm_state
    0x00, //lmp_task_trace_via_uart0 (ramrun is support_hiaudio)
    0x01, //pscan_coex_en
    0x01, //iscan_coex_en
    0x00, //drift_limited
    0x00,0x7d,0x00,0x00, //init_lsto
    0x1e,0x00,0x00,0x00, //lmp_rsp_to
    0x01, //hec_error_no_update_last_ts
    0x96, //acl_rssi_avg_nb_pkt
    0xe2,0x04, //sniff_win_lim
    0x01, //iso_host_to_controller_flow
    0x03, //enable_assert
    0x00, //sw_seq_filter_en
    0x00, //ble_aux_adv_ind_update
    0x00, //sco_start_delay_flag
    0x00, //send_evt_when_wakeup
    0x00, //signal_test_en
    0x06, //ble_cis_conn_event_cnt_distance (in connect interval)
    0x0c, //ble_ci_alarm_init_distance (in half slots)
    0x01, //sync_force_max_slot
    0x00,0x08, //max_hdc_adv_dur in slots
};

const uint8_t bt_sche_setting_1502x_t0[37] =
{
    BT_INIT_TX_PWR_IDX, //bt_init_txpwr for ramrun
    0x01, //two_slave_sched_en
    0xff, //music_playing_link
    0x00, //dual_slave_tws_en
    0x00, //reconnecting_flag
    0x08, //default_tpoll
    0x08, //acl_slot_in_ibrt_mode
    0x34, //ibrt_start_estb_interval
    0x9c,0x00, //acl_interv_in_ibrt_sco_mode
    0x68,0x00, //acl_interv_in_ibrt_normal_mode
    0x40,0x06, //acl_switch_to_threshold
    0x90,0x01, //stopbitoffthd
    0x0a, //delay_schd_min
    0x02, //stopthrmin
    0x78, //acl_margin
    0x50, //sco_margin
    0x01, //adv_after_sco_retx
    0x01, //ble_wrong_packet_lantency
    0x60,0x00, //sniff_priority_interv_thd
    0x08, //scan_hslot_in_sco 8 half slot in 2ev3
    0x02, //double_pscan_in_sco
    0x06, //reduce_rext_for_sniff_thd
    0x06, //reduce_att_for_sco_thd
    0x01, //reduce_att_space_adjust
    0x01, //bandwidth_check_ignore_retx
    0x00, //check_acl_ble_en
    0x01, //check_sco_ble_en
#ifdef __BT_RAMRUN__
    0x00, //sch_plan_slave_disable
#else
    0x01, //sch_plan_slave_disable
#endif
    0x0a, //sco_anchor_start_add
    0x02, //ble_slot
    0x02, //sniff_max_frm_time
    0x06, //ble_con_in_sco_duration
};

const uint8_t bt_sche_setting_1502x_t1[37] =
{
    BT_INIT_TX_PWR_IDX_V2, //bt_init_txpwr for ramrun
    0x01, //two_slave_sched_en
    0xff, //music_playing_link
    0x00, //dual_slave_tws_en
    0x00, //reconnecting_flag
    0x08, //default_tpoll
    0x08, //acl_slot_in_ibrt_mode
    0x34, //ibrt_start_estb_interval
    0x9c,0x00, //acl_interv_in_ibrt_sco_mode
    0x68,0x00, //acl_interv_in_ibrt_normal_mode
    0x40,0x06, //acl_switch_to_threshold
    0x90,0x01, //stopbitoffthd
    0x0a, //delay_schd_min
    0x02, //stopthrmin
    0x78, //acl_margin
    0x50, //sco_margin
    0x01, //adv_after_sco_retx
    0x01, //ble_wrong_packet_lantency
    0x60,0x00, //sniff_priority_interv_thd
    0x08, //scan_hslot_in_sco 8 half slot in 2ev3
    0x02, //double_pscan_in_sco
    0x06, //reduce_rext_for_sniff_thd
    0x06, //reduce_att_for_sco_thd
    0x01, //reduce_att_space_adjust
    0x01, //bandwidth_check_ignore_retx
    0x00, //check_acl_ble_en
    0x01, //check_sco_ble_en
#ifdef __BT_RAMRUN__
    0x00, //sch_plan_slave_disable
#else
    0x01, //sch_plan_slave_disable
#endif
    0x0a, //sco_anchor_start_add
    0x02, //ble_slot
    0x02, //sniff_max_frm_time
    0x06, //ble_con_in_sco_duration
};

const uint8_t bt_ibrt_setting_1502x_t0[29] =
{
    0x01, //hci_auto_accept_tws_link_en
    0x00, //send_profile_via_ble
    0x00, //force_rx_error
    0x6c, //sync_win_size hus
    0x16, //magic_cal_bitoff
    0x3f, //role_switch_packet_br
    0x2b, //role_switch_packet_edr
    0x01, //only_support_slave_role
    0x01, //msg_filter_en
    0x00, //relay_sam_info_in_start_snoop
    0x00, //snoop_switch_interval_num
    0x02, //slave_rx_traffic_siam
    0x05, //ibrt_lmp_to
    0x00, //fa_use_twslink_table
    0x00, //fa_use_fix_channel
    0x06, //ibrt_afh_instant_adjust
    0x0c, //ibrt_detach_send_instant
    0x03, //ibrt_detach_receive_instant
    0x03, //ibrt_detach_txcfm_instant
    0x01, //mobile_enc_change
    0x01, //ibrt_auto_accept_sco
    0x01, //ibrt_second_sco_decision
    0x02, //fa_ok_thr_in_sco
    0x02, //fa_ok_thr_for_acl
    0x01, //accep_remote_bt_roleswitch
    0x01, //accept_remote_enter_sniff
    0x0f, //start_ibrt_tpoll
    0x00, //update_all_saved_queue
    BT_MIN_TX_PWR_IDX, //rwip_rf.txpwr_min
};

const uint8_t bt_ibrt_setting_1502x_t1[29] =
{
    0x01, //hci_auto_accept_tws_link_en
    0x00, //send_profile_via_ble
    0x00, //force_rx_error
    0x6c, //sync_win_size hus
    0x16, //magic_cal_bitoff
    0x3f, //role_switch_packet_br
    0x2b, //role_switch_packet_edr
    0x01, //only_support_slave_role
    0x01, //msg_filter_en
    0x00, //relay_sam_info_in_start_snoop
    0x00, //snoop_switch_interval_num
    0x02, //slave_rx_traffic_siam
    0x05, //ibrt_lmp_to
    0x00, //fa_use_twslink_table
    0x00, //fa_use_fix_channel
    0x06, //ibrt_afh_instant_adjust
    0x0c, //ibrt_detach_send_instant
    0x03, //ibrt_detach_receive_instant
    0x03, //ibrt_detach_txcfm_instant
    0x01, //mobile_enc_change
    0x01, //ibrt_auto_accept_sco
    0x01, //ibrt_second_sco_decision
    0x02, //fa_ok_thr_in_sco
    0x02, //fa_ok_thr_for_acl
    0x01, //accep_remote_bt_roleswitch
    0x01, //accept_remote_enter_sniff
    0x0f, //start_ibrt_tpoll
    0x00, //update_all_saved_queue
    BT_MIN_TX_PWR_IDX_V2, //rwip_rf.txpwr_min
};
const uint8_t bt_hw_feat_setting_1502x[39] =
{
    0x01, //rxheader_int_en
    0x00, //rxdone_bt_int_en
    0x00, //txdone_bt_int_en
    0x01, //rxsync_bt_int_en
    0x01, //rxsync_ble_int_en
    0x00, //pscan_hwagc_flag
    0x00, //iscan_hwagc_flag
    0x00, //sniff_hwagc_flag
    0x00, //blescan_hwagc_flag
    0x00, //bleinit_hwagc_flag
#ifdef __NEW_SWAGC_MODE__
    0x01, //bt_sync_swagc_en
#else
    0x00, //bt_sync_swagc_en
#endif
#ifdef __BLE_NEW_SWAGC_MODE__
    0x01, //le_sync_swagc_en
#else
    0x00, //le_sync_swagc_en
#endif
    0x00, //fa_to_en
#ifdef __BES_FA_MODE__
    0x01, //fa_dsb_en
#else
    0x00, //fa_dsb_en
#endif
    0x0F, //fa_to_num
#ifdef __FIX_FA_RX_GAIN___
    0x01, //fa_rxgain
#else
    0xff, //fa_rxgain
#endif
    0x55,0x00,0x00,0x00, //fa_to_type
    0x50,0x00,0x00,0x00, //fa_disable_type
    0xff, //fa_txpwr
    0x00, //ecc_data_flag
    0x01, //softbit_data_flag
    0x00, //rx_noise_chnl_assess_en
    0x14, //rx_noise_thr_good
    0x0a, //rx_noise_thr_bad
    0x00, //snr_chnl_assess_en
    0x14, //snr_good_thr_value
    0x28, //snr_bad_thr_value
    0x00, //rssi_maxhold_record_en
    0x05, //new_agc_adjust_dbm
    0x0f, //ble_rssi_noise_thr
    0x01, //ble_rxgain_adjust_once
    0x00, //trig_open_pcm_flag
    0x00, //sco_sw_mute_en
};

const uint8_t bt_common_setting_1502x_t1[14] =
{
    120,//tws_acl_prio_in_sco
    144,//tws_acl_prio_in_normal
    10,//scan_evt_win_slot_in_a2dp
    32,//pscan_gap_slot_in_a2dp
    20,//page_gap_slot_in_a2dp
    1,//ble_eff_time_config
    0,//send_tws_interval_to_peer
    0,//softbit_enable
    1,//ble_adv_buf_malloc
#ifdef __AFH_ASSESS__
    0,//afh_assess_old
#else
    1,//afh_assess_old
#endif
    0,//lld_ip_ci_bi_compute_params
    1,//cpu_idle_set
    0x40,0x05,//sniff_interval_max
};

const uint8_t bt_txrx_gain_setting_t0[22] =
{
    0x01,//bt_inq_rxgain
    0x01,//bt_page_rxgain
    0x06,//bt_page_txpwr
    0x01,//bt_iscan_rxgain
    0xff,//bt_iscan_txpwr
    0x01,//bt_pscan_rxgain
    0xff,//bt_pscan_txpwr
    0x00,//bt_ibrt_rxgain
    0xff,//ble_adv_txpwr
    0x00,//ble_adv_rxgain
    0xff,//ble_bis_txpwr
    0x00,//ble_bis_rxgain
    0xff,//ble_adv_per_txpwr
    0x00,//ble_adv_per_rxgain
    0xff,//ble_testmode_txpwr
    0x00,//ble_testmode_rxgain
    0xff,//ble_con_txpwr
    0x00,//ble_con_rxgain
    0xff,//ble_con_init_txpwr
    0x00,//ble_con_init_rxgain
    0xff,//ble_scan_txpwr
    0x00,//ble_scan_rxgain
};

const uint8_t bt_txrx_gain_setting_t1[22] =
{
    0x01,//bt_inq_rxgain
    0x01,//bt_page_rxgain
    0x03,//bt_page_txpwr
    0x01,//bt_iscan_rxgain
    0xff,//bt_iscan_txpwr
    0x01,//bt_pscan_rxgain
    0xff,//bt_pscan_txpwr
    0x00,//bt_ibrt_rxgain
    0xff,//ble_adv_txpwr
    0x00,//ble_adv_rxgain
    0xff,//ble_bis_txpwr
    0x00,//ble_bis_rxgain
    0xff,//ble_adv_per_txpwr
    0x00,//ble_adv_per_rxgain
    0xff,//ble_testmode_txpwr
    0x00,//ble_testmode_rxgain
    0xff,//ble_con_txpwr
    0x00,//ble_con_rxgain
    0xff,//ble_con_init_txpwr
    0x00,//ble_con_init_rxgain
    0xff,//ble_scan_txpwr
    0x00,//ble_scan_rxgain
};

bool btdrv_get_accept_new_mobile_enable(void)
{
    bool ret = false;
    return ret;
}

const uint8_t bt_peer_txpwr_dft_thr[]=
{
    0x64,00,//uint16_t rssi_avg_nb_pkt;
    -1,//rssi_high_thr;
    -2,//rssi_low_thr;
    5,//rssi_below_low_thr;
    50,//unused rssi_interf_thr;
};

const struct rssi_txpower_link_thd tws_link_txpwr_thd =
{
    0x14,//uint16_t rssi_avg_nb_pkt;
    -40,//rssi_high_thr;
    -50,//rssi_low_thr;
    5,//rssi_below_low_thr;
    50,//unused rssi_interf_thr;
};

const uint8_t bt_sw_rssi_setting[] =
{
     0,  //.sw_rssi_en = false
    80,00,00,00,//.link_agc_thd_mobile = 80,
    100,00,00,00,//.link_agc_thd_mobile_time = 100,
    80,00,00,00,//.link_agc_thd_tws = 80,
    100,00,00,00,//.link_agc_thd_tws_time = 100,
    3,//.rssi_mobile_step = 3,
    3,//.rssi_tws_step = 3,
    -100,//.rssi_min_value_mobile = -100,
    -100,//.rssi_min_value_tws = -100,

    0,//.ble_sw_rssi_en = 0,
    80,00,00,00,//.ble_link_agc_thd = 80,
    100,00,00,00,//.ble_link_agc_thd_time = 100,//(in BT half-slots)
    3,//.ble_rssi_step = 3,
    -100,//.ble_rssidbm_min_value = -100,

    1,//.bt_no_sync_en = 1,
    -90,//.bt_link_no_sync_rssi= -90,
    80,00,//.bt_link_no_snyc_thd = 0x50,
    200,00,//.bt_link_no_sync_timeout = 200,

    1,//.ble_no_sync_en = 1,
    -90,//.ble_link_no_sync_rssi= -90,
    20,00,//.ble_link_no_snyc_thd = 20,
    0x20,0x03,//.ble_link_no_sync_timeout = 800,
};

const uint8_t dbg_set_pta_para_cmd[]=
{
    3,//bt_prio_thd;
    0,//wifi_freq_start;
    0,//wifi_freq_end;
};

struct bt_cmd_chip_config_t g_bt_drv_btstack_chip_config = {
    HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE,
    HCI_DBG_SET_SCO_SWITCH_CMD_OPCODE,
};

static BTDRV_CFG_TBL_STRUCT  btdrv_cfg_tbl_common[] = {
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_SLEEP_SETTING_CMD_OPCODE,sizeof(sleep_param),sleep_param},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_LOCAL_FEATURE_CMD_OPCODE,sizeof(local_feature),local_feature},
#if (defined(CTKD_ENABLE) && defined(IS_CTKD_OVER_BR_EDR_ENABLED))
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_LOCAL_EX_FEATURE_CMD_OPCODE,sizeof(local_ex_feature_page1),(uint8_t *)&local_ex_feature_page1},
#endif
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_HW_FEAT_SETTING_CMD_OPCODE,sizeof(bt_hw_feat_setting_1502x),bt_hw_feat_setting_1502x},
#ifdef _SCO_BTPCM_CHANNEL_
    {BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE,sizeof(sync_config),(uint8_t *)&sync_config},
    {BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_PCM_SETTING_CMD_OPCODE,sizeof(pcm_setting),(uint8_t *)&pcm_setting},
#endif
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_LOCAL_EX_FEATURE_CMD_OPCODE,sizeof(local_ex_feature_page2),(uint8_t *)&local_ex_feature_page2},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_RSSI_TX_POWER_DFT_THR_CMD_OPCODE,sizeof(bt_peer_txpwr_dft_thr),(uint8_t *)&bt_peer_txpwr_dft_thr},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_BT_COMMON_SETTING_T1_CMD_OPCODE,sizeof(bt_common_setting_1502x_t1),(uint8_t *)&bt_common_setting_1502x_t1},
#ifdef BT_PTA_OUTPUT
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_PTA_PARA_CMD_OPCODE,sizeof(dbg_set_pta_para_cmd),(uint8_t *)dbg_set_pta_para_cmd},
#endif

};

static BTDRV_CFG_TBL_STRUCT btdrv_cfg_tbl_t0[] = {
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_RF_TIMING_CMD_OPCODE,sizeof(bt_rf_timing_t0),(uint8_t *)&bt_rf_timing_t0},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BLE_RF_TIMING_CMD_OPCODE,sizeof(ble_rf_timing_t0),(uint8_t *)&ble_rf_timing_t0},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_BLE_TXRX_GAIN_CMD_OPCODE,sizeof(bt_txrx_gain_setting_t0),(uint8_t *)bt_txrx_gain_setting_t0},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_IBRT_SETTING_CMD_OPCODE,sizeof(bt_ibrt_setting_1502x_t0),bt_ibrt_setting_1502x_t0},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_SCHE_SETTING_CMD_OPCODE,sizeof(bt_sche_setting_1502x_t0),bt_sche_setting_1502x_t0},
};

static BTDRV_CFG_TBL_STRUCT btdrv_cfg_tbl_t1[] = {
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_RF_TIMING_CMD_OPCODE,sizeof(bt_rf_timing_t1),(uint8_t *)&bt_rf_timing_t1},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BLE_RF_TIMING_CMD_OPCODE,sizeof(ble_rf_timing_t1),(uint8_t *)&ble_rf_timing_t1},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_BLE_TXRX_GAIN_CMD_OPCODE,sizeof(bt_txrx_gain_setting_t1),(uint8_t *)bt_txrx_gain_setting_t1},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_IBRT_SETTING_CMD_OPCODE,sizeof(bt_ibrt_setting_1502x_t1),bt_ibrt_setting_1502x_t1},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_SCHE_SETTING_CMD_OPCODE,sizeof(bt_sche_setting_1502x_t1),bt_sche_setting_1502x_t1},
};

void btdrv_fa_txpwrup_timing_setting(uint8_t txpwrup)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL2_ADDR, 0xff, 24, txpwrup);
}

void btdrv_fa_rxpwrup_timig_setting(uint8_t rxpwrup)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL2_ADDR, 0xff, 16, rxpwrup);
}

void btdrv_fa_margin_timig_setting(uint8_t margin)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL2_ADDR, 0x1f, 11, margin);
}

void bt_fa_sync_invert_en_setf(uint8_t faacinven)
{
    BTDIGITAL_REG_WR(BT_BES_FACNTL0_ADDR,
                     (BTDIGITAL_REG(BT_BES_FACNTL0_ADDR) & ~((uint32_t)0x00020000)) | ((uint32_t)faacinven << 17));
}

void btdrv_fast_lock_en_setf(uint8_t rxonextenden)
{
    BTDIGITAL_REG_WR(BT_BES_FACNTL0_ADDR,
                     (BTDIGITAL_REG(BT_BES_FACNTL0_ADDR) & ~((uint32_t)0x40000000)) | ((uint32_t)rxonextenden << 30));
}

void btdrv_fatx_pll_pre_on_en(uint8_t en)
{
    BTDIGITAL_REG_WR(BT_BES_CNTL5_ADDR,
                     (BTDIGITAL_REG(BT_BES_CNTL5_ADDR) & ~((uint32_t)0x80000000)) | ((uint32_t)en << 31));
}

void btdrv_2m_band_wide_sel(uint8_t fa_2m_mode)
{
    BTDIGITAL_REG_WR(BT_BES_FACNTL0_ADDR,
                     (BTDIGITAL_REG(BT_BES_FACNTL0_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)fa_2m_mode << 0));
}

void btdrv_48m_sys_enable(void)
{
    hal_cmu_bt_sys_set_freq(HAL_CMU_FREQ_52M);
    BTDIGITAL_REG(CMU_CLKREG_ADDR)|=(1<<7)|(1<<19);//BT select 52M system
}

void btdrv_24m_sys_enable(void)
{
    BTDIGITAL_REG(CMU_CLKREG_ADDR) &= ~(1<<7);//BT select 52M system
}

void btdrv_fa_corr_mode_setting(bool is_new_corr)
{
    //[1]: corr_preamble
    if(is_new_corr == true)
    {
        BTDIGITAL_REG(0xd0350228) |= (1<<24);//enable new corr
    }
    else
    {
        BTDIGITAL_REG(0xd0350228) &= ~(1<<24);//enable old corr
    }
}

void btdrv_fa_new_mode_corr_thr(uint8_t corr_thr)
{
    BTDIGITAL_REG_SET_FIELD(0xd0350228, 0x7f, 16, (corr_thr & 0x7f));
}

void btdrv_fa_old_mode_corr_thr(uint8_t corr_thr)
{
    BTDIGITAL_REG_SET_FIELD(0xd0350368, 0xf, 4, (corr_thr & 0xf));
}


void btdrv_fa_config_vendor_syncword_en(bool enable)
{
    if(enable)
    {
        BTDIGITAL_REG(BT_BES_FACNTL1_ADDR) |= (1<<31);
    }
    else
    {
        BTDIGITAL_REG(BT_BES_FACNTL1_ADDR) &= ~(1<<31);
    }
}

void btdrv_set_fa_redundancy_time(uint8_t time)
{
    //BTDIGITAL_REG_SET_FIELD(0xd0220488, 0x1f, 21, (time & 0x1f));
}

void btdrv_fa_config_vendor_syncword_content(uint32_t syncword_high, uint32_t syncword_low, uint8_t syncword_len)
{
    if(syncword_low == INVALID_SYNC_WORD || syncword_high == INVALID_SYNC_WORD)
    {
        BT_DRV_TRACE(0,"BT_DRV:vendor syncword invalid");
        return;
    }

    if(syncword_len == FA_SYNCWORD_32BIT)
    {
        BTDIGITAL_REG(BT_BES_FA_SWREG1_ADDR) = syncword_low;
    }
    else if(syncword_len == FA_SYNCWORD_64BIT)
    {
        BTDIGITAL_REG(BT_BES_FA_SWREG1_ADDR) = syncword_high;
        BTDIGITAL_REG(BT_BES_FA_SWREG0_ADDR) = syncword_low;
    }
}

void btdrv_fa_syncword_phy_setting(uint8_t syncword_len)
{
    if (syncword_len == FA_SYNCWORD_32BIT)
    {
        //PHY using 32 bit FA
        BTDIGITAL_REG_SET_FIELD(0xD0350228, 0x3, 29, 0x3);
        BTDIGITAL_REG_SET_FIELD(0xD0350048, 0xf, 24, 0x2);
        BTDIGITAL_REG_SET_FIELD(0xD0350048, 0x1ff, 15, 0x4b);
    }
    else if (syncword_len == FA_SYNCWORD_64BIT)
    {
        //PHY using 64 bit FA
        BTDIGITAL_REG_SET_FIELD(0xD0350228, 0x3, 29, 0x0);
        BTDIGITAL_REG_SET_FIELD(0xD0350048, 0xf, 24, 0xA);
        BTDIGITAL_REG_SET_FIELD(0xD0350048, 0x1ff, 15, 0x12C);
    }
    else
    {
        ASSERT(0, "[%s] len=%d", __func__, syncword_len);
    }
}

void btdrv_fa_config_syncword_mode(uint8_t syncword_len)
{
    if(syncword_len == FA_SYNCWORD_32BIT)
    {
        BTDIGITAL_REG(0xd0220c8c) &= ~(1<<28);//syncword 32bit
    }
    else if(syncword_len == FA_SYNCWORD_64BIT)
    {
        BTDIGITAL_REG(0xd0220c8c) |= (1<<28);//syncword 64bit
    }
}

void btdrv_fa_config_tx_gain(bool ecc_tx_gain_en, uint8_t ecc_tx_gain_idx)//false :disable tx gain
{
    if(ecc_tx_gain_en == true)
    {
        BTDIGITAL_REG(BT_BES_CNTL3_ADDR) |= (1<<29);
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL3_ADDR, 0xf, 25, (ecc_tx_gain_idx & 0xf));
    }
    else
    {
        BTDIGITAL_REG(BT_BES_CNTL3_ADDR) &= ~(1<<29);
    }
}

void btdrv_fa_config_rx_gain(bool ecc_rx_gain_en, uint8_t ecc_rx_gain_idx)//false: disable rx gain
{
    if(ecc_rx_gain_en == true)
    {
        BTDIGITAL_REG(BT_BES_CNTL3_ADDR) |= (1<<30);
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL3_ADDR, 0xf, 21, (ecc_rx_gain_idx & 0xf));
    }
    else
    {
        BTDIGITAL_REG(BT_BES_CNTL3_ADDR) &= ~(1<<30);
    }
}

void btdrv_fa_rx_winsz(uint8_t ecc_rx_winsz)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7f, 24, (ecc_rx_winsz & 0x7f));
}

void btdrv_fa_tx_preamble_enable(bool fa_tx_preamble_en)
{
    if(fa_tx_preamble_en)
    {
        BTDIGITAL_REG(0xD0220C18) |= (1<<16); // fa tx 8 bit preamble
    }
    else
    {
        BTDIGITAL_REG(0xD0220C18) &= ~(1<<16);
    }
}

void btdrv_fa_freq_table_enable(bool fa_freq_table_en)
{
    if(fa_freq_table_en)
    {
        BTDIGITAL_REG(0xD0220C80) |= (1<<14);
    }
    else
    {
        BTDIGITAL_REG(0xD0220C80) &= ~(1<<14);
    }
}

void btdrv_fa_multi_mode0_enable(bool fa_multi_mode0_en)
{
    if(fa_multi_mode0_en)
    {
        BTDIGITAL_REG(0xD0220C18) |= (1<<24);
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7f, 24, (0x66 & 0x7f));
    }
    else
    {
        BTDIGITAL_REG(0xD0220C18) &= ~(1<<24);
    }
}

void btdrv_fa_multi_mode1_enable(bool fa_multi_mode1_en, uint8_t fa_multi_tx_count)
{
    if(fa_multi_mode1_en)
    {
        BTDIGITAL_REG(0xD0220C18) |= (1<<25);
        if(fa_multi_tx_count == 2)
        {
            BTDIGITAL_REG(0xD0220C18) |= 0x00800000;
            BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7f, 24, (0x31 & 0x7f));
        }

        if(fa_multi_tx_count == 3)
        {
            BTDIGITAL_REG(0xD0220C18) |= 0x00c00000;
            BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7f, 24, (0x5b & 0x7f));
        }
    }
    else
    {
        BTDIGITAL_REG(0xD0220C18) &= ~(1<<25);
    }
}

void btdrv_fa_freq_table_config(uint32_t fa_freq_table0,uint32_t fa_freq_table1,uint32_t fa_freq_table2)
{
    BTDIGITAL_REG(0xD0220C40)  = fa_freq_table0; // freq 0~31
    BTDIGITAL_REG(0xD0220C44)  = fa_freq_table1; // freq 32~63
    BTDIGITAL_REG(0xD0220C48) |= fa_freq_table2; // freq 64~78
}

void btdrv_fa_mic_pktlen_sel(bool enable)
{
    BTDIGITAL_REG_SET_FIELD(BT_MIC_PKTLEN_SEL_ADDR, 0x1, 30, enable);
}

void btdrv_fa_basic_config(btdrv_fa_basic_config_t* p_fa_basic_config)
{
    if(p_fa_basic_config != NULL)
    {
        //fast ack TX power gain set
        btdrv_fa_config_tx_gain(p_fa_basic_config->fa_tx_gain_en, p_fa_basic_config->fa_tx_gain_idx);
#ifdef __FIX_FA_RX_GAIN___
        //fix fast ack rx gain
        btdrv_fa_config_rx_gain(p_fa_basic_config->fa_rx_gain_en, p_fa_basic_config->fa_rx_gain_idx);
#endif
        // fa 2M mode select
        btdrv_2m_band_wide_sel(p_fa_basic_config->fa_2m_mode);
        //fa syncword mode
        btdrv_fa_config_syncword_mode(p_fa_basic_config->syncword_len);
        //fa phy setting
        btdrv_fa_syncword_phy_setting(p_fa_basic_config->syncword_len);

        //ECC vendor syncword mode
        if(p_fa_basic_config->fa_vendor_syncword_en)
        {
            btdrv_fa_config_vendor_syncword_en(true);
            btdrv_fa_config_vendor_syncword_content(p_fa_basic_config->syncword_high,
                                                    p_fa_basic_config->syncword_low,
                                                    p_fa_basic_config->syncword_len);
        }
        else
        {
            btdrv_fa_config_vendor_syncword_en(false);
        }
        //fa win size
        btdrv_fa_rx_winsz(p_fa_basic_config->fa_rx_winsz);
        //fa corr mode
        btdrv_fa_corr_mode_setting(p_fa_basic_config->is_new_corr);

        if(p_fa_basic_config->is_new_corr)
        {
            btdrv_fa_new_mode_corr_thr(p_fa_basic_config->new_mode_corr_thr);
        }
        else
        {
            btdrv_fa_old_mode_corr_thr(p_fa_basic_config->old_mode_corr_thr);
        }

        if(p_fa_basic_config->fa_tx_preamble_en)
        {
            btdrv_fa_tx_preamble_enable(true);
        }
        else
        {
            btdrv_fa_tx_preamble_enable(false);
        }

        if(p_fa_basic_config->fa_freq_table_en)
        {
            btdrv_fa_freq_table_enable(true);
            btdrv_fa_freq_table_config(p_fa_basic_config->fa_freq_table0,
                                       p_fa_basic_config->fa_freq_table1,
                                       p_fa_basic_config->fa_freq_table2);
        }
        else
        {
            btdrv_fa_freq_table_enable(false);
        }

        if(p_fa_basic_config->fa_multi_mode0_en)
        {
            btdrv_fa_multi_mode0_enable(true);
        }
        else
        {
            btdrv_fa_multi_mode0_enable(false);
        }

        if(p_fa_basic_config->fa_multi_mode1_en)
        {
            btdrv_fa_multi_mode1_enable(true, p_fa_basic_config->fa_multi_tx_count);
        }
        else
        {
            btdrv_fa_multi_mode1_enable(false,p_fa_basic_config->fa_multi_tx_count);
        }
    }

    //enable fa invert
    bt_fa_sync_invert_en_setf(BT_FA_INVERT_EN);
}

#ifdef __FASTACK_ECC_ENABLE__
//ecc usert data
void btdrv_ecc_config_len_mode_sel(uint8_t ecc_len_mode) //only for ecc 1 block
{
    if(ecc_len_mode == ECC_8_BYTE_MODE)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7, 0, 1);
    }
    else if(ecc_len_mode == ECC_16_BYTE_MODE)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7, 0, 0);
    }
    else if(ecc_len_mode == ECC_12_BYTE_MODE)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7, 0, 2);
    }
}

void btdrv_ecc_config_usr_tx_dat_set(ecc_trx_dat_t* p_ecc_trx_dat)
{
    BTDIGITAL_REG(0xd0220cd0) = p_ecc_trx_dat->trx_dat.dat_arr[0];
    BTDIGITAL_REG(0xd0220cd4) = p_ecc_trx_dat->trx_dat.dat_arr[1];
    BTDIGITAL_REG(0xd0220cd8) = p_ecc_trx_dat->trx_dat.dat_arr[2];
    BTDIGITAL_REG(0xd0220cdc) = p_ecc_trx_dat->trx_dat.dat_arr[3];
}

void btdrv_ecc_config_usr_rx_dat_get(ecc_trx_dat_t* p_ecc_trx_dat)
{
    p_ecc_trx_dat->trx_dat.dat_arr[0] = BTDIGITAL_REG(0xd0220cd0);
    p_ecc_trx_dat->trx_dat.dat_arr[1] = BTDIGITAL_REG(0xd0220cd4);
    p_ecc_trx_dat->trx_dat.dat_arr[2] = BTDIGITAL_REG(0xd0220cd8);
    p_ecc_trx_dat->trx_dat.dat_arr[3] = BTDIGITAL_REG(0xd0220cdc);
}


void btdrv_ecc_disable_spec_pkt_type(uint32_t ptk_type) // 1 -> disable FA, ptk type enum in bt_drv_1600_internal.h
{
    BTDIGITAL_REG(0xd0220c30) |= (1<<20);
    BTDIGITAL_REG(0xd0220c30) = (ptk_type & 0xfffff);
}


void btdrv_ecc_config_blk_mode(uint8_t ecc_blk_mode)
{
    if (ecc_blk_mode == ECC_1BLOCK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7ff, 5, 0xef);
    }
    else if (ecc_blk_mode == ECC_2BLOCK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7ff, 5, 0x1de);
    }
    else if (ecc_blk_mode == ECC_3BLOCK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7ff, 5, 0x2cd);
    }
}

void btdrv_ecc_config_modu_mode_acl(uint8_t ecc_modu_mode_acl)
{
    if(ecc_modu_mode_acl == ECC_8PSK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTLX_ADDR, 3, 20, 3); //ECC 8PSK
    }
    else if(ecc_modu_mode_acl == ECC_DPSK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTLX_ADDR, 3, 20, 2); //ECC DPSK
    }
    else if(ecc_modu_mode_acl == ECC_GFSK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTLX_ADDR, 3, 20, 1); //ECC GFSK
    }
}

void btdrv_ecc_config_modu_mode_sco(uint8_t ecc_modu_mode_sco)
{
    if(ecc_modu_mode_sco == ECC_8PSK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 3, 1, 3); //ECC 8PSK
    }
    else if(ecc_modu_mode_sco == ECC_DPSK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 3, 1, 2); //ECC DPSK
    }
    else if(ecc_modu_mode_sco == ECC_GFSK)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 3, 1, 1); //ECC GFSK
    }
}

void btdrv_sco_ecc_enable(bool enable)
{
    if(enable == true)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 1, 8, 1);//sco ecc flag, disabled by default
    }
    else
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 1, 8, 0);//sco ecc flag, disabled by default
    }
}

void btdrv_acl_ecc_enable(bool enable)
{
    if(enable == true)
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL5_ADDR, 1, 1, 1);//acl ecc flag, disabled by default
    }
    else
    {
        BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL5_ADDR, 1, 1, 0);//acl ecc flag, disabled by default
    }
}


void btdrv_ecc_enable(bool enable)
{
    //ecc all flag, initialize all configurations
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL5_ADDR, 1, 1, 0);//acl ecc flag, disabled by default
    //to enable acl ecc, please use btdrv_acl_ecc_enable later
    BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 1, 8, 0);//sco ecc flag, disabled by default
    //to enable sco ecc, please use btdrv_sco_ecc_enable later

    if(enable == true)
    {
        BTDIGITAL_REG(BT_BES_CNTL5_ADDR) |= 1;//ecc enable
#ifdef __FASTACK_SCO_ECC_ENABLE__
        btdrv_sco_ecc_enable(true);
#endif
#ifdef __FASTACK_ACL_ECC_ENABLE__
        btdrv_acl_ecc_enable(true);
#endif
    }
    else
    {
        BTDIGITAL_REG(BT_BES_CNTL5_ADDR) &= ~1;//disable
    }
}

void btdrv_ecc_basic_config(btdrv_ecc_basic_config_t* p_ecc_basic_config)
{
    if((p_ecc_basic_config != NULL) && (p_ecc_basic_config->ecc_mode_enable == true))
    {
        btdrv_ecc_enable(true);
        btdrv_ecc_config_modu_mode_acl(p_ecc_basic_config->ecc_modu_mode_acl);
        btdrv_ecc_config_modu_mode_sco(p_ecc_basic_config->ecc_modu_mode_sco);
        btdrv_ecc_config_blk_mode(p_ecc_basic_config->ecc_blk_mode);
        btdrv_ecc_config_len_mode_sel(p_ecc_basic_config->ecc_len_mode_sel);
    }
}


void btdrv_bid_ecc_2dh5_enable(void)
{
    BTDIGITAL_REG_SET_FIELD(BT_TRIGREG_ADDR, 1, 15, 1);
}

void btdrv_bid_ecc_cnt_act_1(uint8_t regcnteccact1)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL3_ADDR, 0xff, 0, regcnteccact1);
}

void btdrv_bid_ecc_cnt_act_2(uint8_t regcnteccact2)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL3_ADDR, 0xff, 8, regcnteccact2);
}

void btdrv_bid_ecc_frame_len(uint16_t regeccframelen)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL1_ADDR, 0x7ff, 5, regeccframelen);
}

void btdrv_bid_ecc_bid_ecc_blk_en(uint8_t bideccen)
{
    BTDIGITAL_REG_SET_FIELD(BT_BES_ENHPCM_CNTL_ADDR, 1, 5, bideccen);
}

void btdrv_ecc_bid_enable(void)
{
    BTDIGITAL_REG_SET_FIELD(0xd0220c8c, 1, 29, 1);
}

void btdrv_ecc_bid_pkt_2dh_5_reduce_byte(uint8_t pkt2dh5reducebyte)
{
    BTDIGITAL_REG_SET_FIELD(BT_TRIGREG_ADDR, 0x7f, 18, pkt2dh5reducebyte);
}


void btdrv_ecc_bid_fa_rate_mode_acl(uint8_t regfaratemodeacl)
{

    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTLX_ADDR, 3, 20, regfaratemodeacl);
}


void btdrv_bid_ecc_basic_config(void)
{
    btdrv_bid_ecc_2dh5_enable();
    btdrv_bid_ecc_cnt_act_1(205);
    btdrv_bid_ecc_cnt_act_2(250);
    btdrv_bid_ecc_frame_len(690);
    btdrv_ecc_bid_pkt_2dh_5_reduce_byte(78);
    btdrv_bid_ecc_bid_ecc_blk_en(1);
    btdrv_ecc_bid_fa_rate_mode_acl(3);
    btdrv_ecc_bid_enable();
}




void btdrv_ecc_content_config(void)
{
    btdrv_ecc_basic_config_t ecc_config;
    //ECC  config
    ecc_config.ecc_mode_enable = true;
    ecc_config.ecc_modu_mode_acl = ECC_MODU_MODE;
    ecc_config.ecc_modu_mode_sco = ECC_8PSK;
    ecc_config.ecc_blk_mode = ECC_BLK_MODE;
    ecc_config.ecc_len_mode_sel = ECC_8_BYTE_MODE;
    //setting
    btdrv_ecc_basic_config(&ecc_config);

#if __BT_BID_ECC_EN__
    btdrv_bid_ecc_basic_config();
#endif
}
#endif

void btdrv_fast_ack_config(void)
{
    btdrv_fa_basic_config_t fa_config;
    //fast ack config
    fa_config.fa_2m_mode = false;
    if(fa_config.fa_2m_mode)
    {
        fa_config.syncword_len = FA_SYNCWORD_64BIT;
        btdrv_fa_txpwrup_timing_setting(FA_BW2M_TXPWRUP_TIMING);
        btdrv_fa_rxpwrup_timig_setting(FA_BW2M_RXPWRUP_TIMING);
    }
    else
    {
        #if defined (CTKD_ENABLE)|| defined (__3M_PACK__)||defined(__FASTACK_ECC_ENABLE__)
        fa_config.syncword_len = FA_SYNCWORD_32BIT;
        #else
        fa_config.syncword_len = FA_SYNCWORD_64BIT;
        #endif
        btdrv_fa_txpwrup_timing_setting(FA_TXPWRUP_TIMING);
        btdrv_fa_rxpwrup_timig_setting(FA_RXPWRUP_TIMING);
    }
    fa_config.fa_vendor_syncword_en = false;
    fa_config.syncword_high = INVALID_SYNC_WORD;
    fa_config.syncword_low = INVALID_SYNC_WORD;
    fa_config.is_new_corr = false;

    fa_config.old_mode_corr_thr = FA_OLD_CORR_VALUE;
    fa_config.new_mode_corr_thr = FA_NEW_CORR_VALUE;
    fa_config.fa_tx_gain_en = true;
    fa_config.fa_tx_gain_idx = FA_FIX_TX_GIAN_IDX;
    fa_config.fa_rx_winsz = FA_RX_WIN_SIZE;
#ifdef __FIX_FA_RX_GAIN___
    fa_config.fa_rx_gain_en = true;
    fa_config.fa_rx_gain_idx = FA_FIX_RX_GIAN_IDX;
#endif
    fa_config.enhance_fa_mode_en = false;
    fa_config.fa_tx_preamble_en = true;
    fa_config.fa_freq_table_en = false;
    fa_config.fa_freq_table0 = INVALID_FA_FREQ_TABLE;
    fa_config.fa_freq_table1 = INVALID_FA_FREQ_TABLE;
    fa_config.fa_freq_table2 = INVALID_FA_FREQ_TABLE;
    fa_config.fa_multi_mode0_en = false;
    fa_config.fa_multi_mode1_en = false;
    fa_config.fa_multi_tx_count = FA_MULTI_TX_COUNT;

    //setting
    btdrv_fa_basic_config(&fa_config);
}

void btdrv_fa_timing_init(void)
{
    #if (defined(CTKD_ENABLE)&&defined(IS_CTKD_OVER_BR_EDR_ENABLED))
    enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();
    if (metal_id >= HAL_CHIP_METAL_ID_2)
    {
        btdrv_fa_mic_pktlen_sel(true);
    }
    #endif
    //FA trx pwrup/dn
    btdrv_fa_margin_timig_setting(FA_CNT_PKT_US);
    BTDIGITAL_REG(BT_BES_CNTL5_ADDR) &= ~1;//disable
    BTDIGITAL_REG_SET_FIELD(BT_TRIGREG_ADDR, 7, 15, 0);//disable bid ecc en
    BTDIGITAL_REG_SET_FIELD(BT_TRIGREG_ADDR, 1, 1, 1);//trig_lock_mode
    BTDIGITAL_REG_SET_FIELD(BT_TRIGREG_ADDR, 7, 9, 0);//clear fa mt en

    BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 1, 29, 0);//clear bid ecc en
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL5_ADDR, 1, 8, 0);//clear sbc en
    BTDIGITAL_REG_SET_FIELD(BT_BES_FACNTL0_ADDR, 1, 30, 0);//clear farx always on
    TRACE(0,"fa timing=0x%x",BTDIGITAL_REG(BT_TRIGREG_ADDR));
}

void btdrv_ecc_config(void)
{
    btdrv_fa_timing_init();

    btdrv_fast_ack_config();
#ifdef __FASTACK_ECC_ENABLE__
    btdrv_ecc_content_config();
#endif
}

void btdrv_afh_monitor_config(void)
{
    BTDIGITAL_REG_SET_FIELD(0xD035035C,3,0,1);//afh_avg_window[1:0]
    BTDIGITAL_REG_SET_FIELD(0xD0350240,1,12,1);//cpx_bypass[12]

    BTDIGITAL_REG_SET_FIELD(0xD035034C,0xff,0,0x40);//afh_settle_down0[7:0]
    BTDIGITAL_REG_SET_FIELD(0xD0350350,0xff,0,0x40);//afh_settle_down1[7:0]

    BTDIGITAL_REG_SET_FIELD(0xD0350354,0xff,0,0x80);//The front_end latency time, bit[8:5] which unit is us,and bit[4:0] which unit is 1/overramp_rate
    BTDIGITAL_REG_SET_FIELD(0xD0350358,0xf,0,6);//The number of channel which estimate the RSSI afh_chan_num[3:0]
    //BTDIGITAL_REG_SET_FIELD(0xD0220C04,1,23,1); //bit23 afh en
    //BTDIGITAL_REG_SET_FIELD(0xD0220C04,1,22,1); //bit22 rxgain dr
    //BTDIGITAL_REG_SET_FIELD(0xD0220C04,0xf,12,3); //bit22 rxgain dr

    //BTDIGITAL_REG_SET_FIELD(0xD0350348,0x3ff,0,0x1a4);//afh ramp up delay
    //BTDIGITAL_REG_SET_FIELD(0xD035020C,0xff,0,0x2e);//rx_startupdel[7:0]
    //btdrv_btradiowrupdn_set_rxpwrup(0x5a);
    //btdrv_fa_rxpwrup_timig_setting(0x5a);
}

static POSSIBLY_UNUSED void btdrv_enable_mcu2bt1_isr_en(void)
{
    //[3]:mcu2bt1 isr en
    BTDIGITAL_REG_SET_FIELD(0xD03300a0,1,3,1);
}

static POSSIBLY_UNUSED void btdrv_enable_sens2bt1_isr_en(void)
{
    //[19]:sys2bt_data1 isr en
    BTDIGITAL_REG_SET_FIELD(0xD03300a0,1,19,1);
}

static void btdrv_enable_slave_loop_p_en(void)
{
    //[30]:crash after skipping CS update in slave sniff
    BTDIGITAL_REG_SET_FIELD(0xD0220C38,1,30,1);//enable slave loop avoid controll crash
}

void btdrv_digital_config_init(void)
{
    //[21] tx pwr dr
    BTDIGITAL_REG_SET_FIELD(0xD0220C04,1,21,0);//disable tx power dr
    //[22] swagc gain dr
    BTDIGITAL_REG_SET_FIELD(0xD0220C04,1,22,0);//disable swagc gain dr

    //sync agc guard update en
    BTDIGITAL_REG_SET_FIELD(0xD0220C2C,1,25,1);
    //sync agc guard update time
    BTDIGITAL_REG_SET_FIELD(0xD0220C2C,0x3f,26,23);
#ifdef __HW_AGC__
    //turn BT rxpwr from header to payload for hwagc rssi calculation
    BTDIGITAL_REG_SET_FIELD(0xd0350018, 1, 4, 1);
    BTDIGITAL_REG_SET_FIELD(0xd035001C, 0xffff, 16, 0x0bb8);
#endif

#ifdef BT_LOG_POWEROFF
    hal_psc_bt_enable_auto_power_down();
#endif

#if defined(CHIP_SUBSYS_SENS)
    btdrv_enable_sens2bt1_isr_en();
#else
    btdrv_enable_mcu2bt1_isr_en();
#endif
    //enable slave loop avoid controll crash
    btdrv_enable_slave_loop_p_en();

    BTDIGITAL_REG_SET_FIELD(BLE_RADIOCNTL3_ADDR,3,14,0);
    BTDIGITAL_REG_SET_FIELD(BLE_RADIOCNTL3_ADDR,3,12,0);
    //[4]: soft_rst_rx_corr_new disable [5]:soft_rst_fsync disable
    BTDIGITAL_REG_SET_FIELD(0xD0330058, 0x3, 4, 0);

    //config flush for long range 125k
    BTDIGITAL_REG_SET_FIELD(0xD0220878, 0x1F, 24, 16);
}

void btdrv_ble_modem_config(void)
{
    //add ble modem config here
}

void btdrv_bt_modem_config(void)
{
    uint16_t packag_type;
    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();

    packag_type = bt_drv_get_packag_type();

    //[16]:iqswap
    BTDIGITAL_REG_SET_FIELD(0xd0350214, 0x1, 16, 1);
    //[18]:txqsigned
    BTDIGITAL_REG_SET_FIELD(0xd0350214, 0x1, 18, 0);
    //[19]:txisigned
    BTDIGITAL_REG_SET_FIELD(0xd0350214, 0x1, 19, 0);
    //bit[3]=1'b1 txdac_fifo_bypass
    BTDIGITAL_REG_SET_FIELD(0xd0350324, 0x1, 3, 1);

    BTDIGITAL_REG_SET_FIELD(0xD0350210, 0xFF, 0, 0x20);
    BTDIGITAL_REG_SET_FIELD(0xD0350254, 0x1F, 0, 0x3);

    // iq_swap
    BTDIGITAL_REG_SET_FIELD(0xD0350240, 0x1, 2, 1);
    BTDIGITAL_REG_SET_FIELD(0xD0350240, 0x1, 11, 1);

    BTDIGITAL_REG_SET_FIELD(0xD0350010, 0xffff, 0, 0x0032);
    BTDIGITAL_REG_SET_FIELD(0xD0350010, 0xffff, 16, 0x3710);

    BTDIGITAL_REG_SET_FIELD(0xD0350004, 0xffff, 0, 0x35);
    BTDIGITAL_REG_SET_FIELD(0xD0350004, 0xffff, 16, 0x0291);

    BTDIGITAL_REG_SET_FIELD(0xD035036C, 0xffff, 0, 0x1000);
    BTDIGITAL_REG_SET_FIELD(0xD035036C, 0xffff, 16, 0x0300);
    BTDIGITAL_REG_WR(0xD03502C8, 0x01605866);
    BTDIGITAL_REG_WR(0xD0350300, 0x3FF00405);
    BTDIGITAL_REG_WR(0xD0350340, 0X00000005);

    BTDIGITAL_REG_SET_FIELD(0xD035031C, 0xF, 0, 0x6);        //gsg_dphi_den_bt
    BTDIGITAL_REG_SET_FIELD(0xD035031C, 0xF, 16, 0xA);       //gsg_dphi_den_ble
    BTDIGITAL_REG_SET_FIELD(0xD0350320, 0x1, 10, 0x1);       //gsg_dphi_nom_bt_sel
    BTDIGITAL_REG_SET_FIELD(0xD0350320, 0x3FF, 0, 0x3F);     //gsg_dphi_nom_bt
    BTDIGITAL_REG_SET_FIELD(0xD0350320, 0x1, 17, 0x1);       //gsg_dphi_nom_bt_sel
    BTDIGITAL_REG_SET_FIELD(0xD0350320, 0x3FF, 18, 0x3E2);   //gsg_dphi_nom_ble

    BTDIGITAL_REG_WR(0xd03500c0,0x5);//BLE2M PSD FLT ON
    BTDIGITAL_REG_WR(0xD0350044, 0x03258032);
    BTDIGITAL_REG_WR(0xd03500d0, 0x1aaa2176);               //restore middle freq config for ble 2m sync
    BTDIGITAL_REG_WR(0xD03502C4, 0x08E1FAD9);	// te timeinit bw1m，0xD03502C4[10:0] = 0x2D9;

    if (packag_type == BTDRV_PACKAG_2700H || packag_type == BTDRV_PACKAG_2700IBP) {
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 0, 0xF);     //gsg_nom_i[9:0]
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 10, 0xF);    //gsg_nom_q[19:10]
        BTDIGITAL_REG_SET_FIELD(0xD0350344, 0x3FF, 0, 0xF);     //dsg_nom[9:0]
    } else if (packag_type == BTDRV_PACKAG_2600ZP) {
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 0, 0x11);     //gsg_nom_i[9:0]
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 10, 0x11);    //gsg_nom_q[19:10]
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 20, 1);      //gsg_gain_step[29:20]
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3, 30, 1);        //gsg_den_step[31:30]
        BTDIGITAL_REG_SET_FIELD(0xD0350344, 0x3FF, 0, 0x12);     //dsg_nom[9:0]
    } else {
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 0, 0xF);     //gsg_nom_i[9:0]
        BTDIGITAL_REG_SET_FIELD(0xD0350308, 0x3FF, 10, 0xF);    //gsg_nom_q[19:10]
        BTDIGITAL_REG_SET_FIELD(0xD0350344, 0x3FF, 0, 0xF);     //dsg_nom[9:0]
    }

#ifdef __HW_AGC__
    BTDIGITAL_REG_SET_FIELD(0xD03502C0, 1, 7, 1);           //rrc_engain
    BTDIGITAL_REG_SET_FIELD(0xD03502C0, 1, 20, 1);
    BTDIGITAL_REG_WR(0xD03500E0, 0x007FF887);               //select channel filt out
    BTDIGITAL_REG_WR(0xD03500E8, 0x0D804036);               //BT FAST K=3
    BTDIGITAL_REG_WR(0xD0350004, 0x02910065);
    BTDIGITAL_REG_WR(0xD0350014, 0xC1A1412C);
    BTDIGITAL_REG_SET_FIELD(0xD0350240, 1, 11, 1);          //psd_avg_en_ble
    if (metal_id < HAL_CHIP_METAL_ID_2) {
        BTDIGITAL_REG_WR(0xD0220C0C, 0x28372390);
        BTDIGITAL_REG_WR(0xd035020C, 0x27012727);
        BTDIGITAL_REG_WR(0xD0350104, 0x001441D4);
        BTDIGITAL_REG_WR(0xD0350108, 0x005E8120);
        BTDIGITAL_REG_WR(0xD03500F0, 0x09004055);               //BLE1M FAST K=3
        BTDIGITAL_REG_WR(0xD03500F8, 0x048040AB);               //BLE2M FAST K=1
        BTDIGITAL_REG_WR(0xD0350100, 0x12004000);               //LR FAST K =2
    } else if (metal_id >= HAL_CHIP_METAL_ID_2) {
        BTDIGITAL_REG_WR(0xd035020C, 0x27012727);
        BTDIGITAL_REG_WR(0xD0350104, 0x001441D4);
        BTDIGITAL_REG_WR(0xD0350108, 0x005E8120);
        BTDIGITAL_REG_WR(0xD03500F0, 0x09004055);               // BLE1M FAST K=2
        BTDIGITAL_REG_WR(0xD03500F8, 0x048040AB);               // BLE2M FAST K=1
        BTDIGITAL_REG_WR(0xD0350100, 0x12004000);               // LR FAST K =4
    }
#endif
    btdrv_txpower_calib();

}

const uint32_t data_backup_tbl[] =
{
    0xD0350210,
    0xD0350254,
    0xD0350324,
    0xD0350300,
    0xD0350308,
    0xD0350340,
    0xD0350344,
    0xD0350368,
    0xD0350048,
    0xD035001C,
    0xD0350010,
    0xD0350004,
    0xD035036C,
    0xD0350044,
    0xD03500c0,
    0xD03500E0,
    0xD0350104,
    0xD0350108,
    0xD03500E8,
    0xD03500F0,
    0xD0350100,
    0xD0350040,
    0xD035005C,
    0xD0350054,
    0xD03502C8,
    0xD0350014,
    0xD03500B0,
    0xD03502c0,
    0xD03500C4,
    0xD03500C8,
    0xD0350018,
    0xD03500f8,
    0xd035020C,
    0xD03502C4,
};

void btdrv_config_end(void)
{
    BTDIGITAL_REG(0xD0330024) &= (~(1<<5));
    BTDIGITAL_REG(0xD0330024) |= (1<<18);
#ifdef BT_LOG_POWEROFF
    bt_drv_reg_op_data_bakeup_init();
    bt_drv_reg_op_data_backup_write(&data_backup_tbl[0], sizeof(data_backup_tbl) / sizeof(data_backup_tbl[0]));
#endif
#ifdef BT_ACTIVE_OUTPUT
    hal_iomux_set_bt_active_out();
#endif
#ifdef BT_SYSTEM_52M
    btdrv_48m_sys_enable();
#else
    btdrv_24m_sys_enable();
#endif
}

void btdrv_hciprocess(void)
{
    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();
    BT_DRV_TRACE(1, "%s",  __func__);
    uint32_t btdrv_rf_env_tbl_size = ARRAY_SIZE(btdrv_rf_env);
    uint32_t bt_common_setting_1502x_tbl_size = ARRAY_SIZE(bt_common_setting_1502x);

    // btdrv_rf_env
    if (metal_id >= HAL_CHIP_METAL_ID_2) {

        int8_t *btdrv_rf_env_buf = (int8_t*)bt_drv_malloc(btdrv_rf_env_tbl_size*sizeof(int8_t));

        for (int j = 0; j < btdrv_rf_env_tbl_size; j++) {
            btdrv_rf_env_buf[j] = btdrv_rf_env[j];
        }
        btdrv_rf_env_buf[4] = BT_MAX_TX_PWR_IDX_V2;
        btdrv_send_cmd(HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE, btdrv_rf_env_tbl_size, (uint8_t *)btdrv_rf_env_buf);
        bt_drv_free(btdrv_rf_env_buf);
    } else {
        btdrv_send_cmd(HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE, btdrv_rf_env_tbl_size, (uint8_t *)btdrv_rf_env);
    }

    btdrv_delay(1);


    // bt_common_setting_1502x
    int8_t *bt_common_setting_1502x_buf = (int8_t*)bt_drv_malloc(bt_common_setting_1502x_tbl_size*sizeof(int8_t));

    for (int j = 0; j < bt_common_setting_1502x_tbl_size; j++) {
        bt_common_setting_1502x_buf[j] = bt_common_setting_1502x[j];
    }

    if (btdrv_get_controller_trace_level() != BTDRV_INVALID_TRACE_LEVEL) {
        bt_common_setting_1502x_buf[0] = btdrv_get_controller_trace_level();
    }
    btdrv_send_cmd(HCI_DBG_SET_BT_SETTING_CMD_OPCODE, bt_common_setting_1502x_tbl_size, (uint8_t *)bt_common_setting_1502x_buf);
    bt_drv_free(bt_common_setting_1502x_buf);

    btdrv_delay(1);

    //BT other config
    for (uint8_t i = 0; i < sizeof(btdrv_cfg_tbl_common)/sizeof(btdrv_cfg_tbl_common[0]); i++) {
        if (btdrv_cfg_tbl_common[i].is_act == BTDRV_CONFIG_ACTIVE) {
            btdrv_send_cmd(btdrv_cfg_tbl_common[i].opcode, btdrv_cfg_tbl_common[i].parlen, btdrv_cfg_tbl_common[i].param);
#ifdef NORMAL_TEST_MODE_SWITCH
            btdrv_delay(20);
#else
            btdrv_delay(1);
#endif
        }
    }

    if (metal_id < HAL_CHIP_METAL_ID_2) {
        for (uint8_t i = 0; i < sizeof(btdrv_cfg_tbl_t0)/sizeof(btdrv_cfg_tbl_t0[0]); i++) {
            if (btdrv_cfg_tbl_t0[i].is_act == BTDRV_CONFIG_ACTIVE) {
                btdrv_send_cmd(btdrv_cfg_tbl_t0[i].opcode, btdrv_cfg_tbl_t0[i].parlen, btdrv_cfg_tbl_t0[i].param);
#ifdef NORMAL_TEST_MODE_SWITCH
                btdrv_delay(20);
#else
                btdrv_delay(1);
#endif
            }
        }
    }
    else if (metal_id >= HAL_CHIP_METAL_ID_2) {
        for (uint8_t i = 0; i < sizeof(btdrv_cfg_tbl_t1)/sizeof(btdrv_cfg_tbl_t1[0]); i++) {
            if (btdrv_cfg_tbl_t1[i].is_act == BTDRV_CONFIG_ACTIVE) {
                btdrv_send_cmd(btdrv_cfg_tbl_t1[i].opcode, btdrv_cfg_tbl_t1[i].parlen, btdrv_cfg_tbl_t1[i].param);
#ifdef NORMAL_TEST_MODE_SWITCH
                btdrv_delay(20);
#else
                btdrv_delay(1);
#endif
            }
        }
    }
}

void btdrv_config_init(void)
{
    BT_DRV_TRACE(1,"%s", __func__);

    btdrv_digital_config_init();

    btdrv_bt_modem_config();

    btdrv_ble_modem_config();

    btdrv_ecc_config();
}

bool btdrv_is_ecc_enable(void)
{
    bool ret = false;
#ifdef  __FASTACK_ECC_ENABLE__
    ret = true;
#endif
    return ret;
}

////////////////////////////////////////test mode////////////////////////////////////////////

void btdrv_sleep_config(uint8_t sleep_en)
{
    sleep_param[0] = sleep_en;
    btdrv_send_cmd(HCI_DBG_SET_SLEEP_SETTING_CMD_OPCODE,8,sleep_param);
    btdrv_delay(1);
}

void btdrv_feature_default(void)
{
#ifdef __EBQ_TEST__
    const uint8_t feature[] = {0xBF, 0xFE, 0xCF,0xFe,0xdb,0xFF,0x5b,0x87};
#else
    const uint8_t feature[] = {0xBF, 0xeE, 0xCD,0xFe,0xdb,0xFf,0x7b,0x87};
#endif
    btdrv_send_cmd(HCI_DBG_SET_LOCAL_FEATURE_CMD_OPCODE,8,feature);
    btdrv_delay(1);
}

const uint8_t test_mode_addr[6] = {0x77,0x77,0x77,0x77,0x77,0x77};
void btdrv_test_mode_addr_set(void)
{
    return;

    btdrv_send_cmd(HCI_DBG_SET_BD_ADDR_CMD_OPCODE,sizeof(test_mode_addr),test_mode_addr);
    btdrv_delay(1);
}
void btdrv_set_controller_trace_enable(uint8_t trace_level)
{
    g_controller_trace_level = trace_level;
}

uint8_t btdrv_get_controller_trace_level(void)
{
    return g_controller_trace_level;
}

const struct rssi_txpower_link_thd* btdrv_get_tws_link_txpwr_thd_ptr(void)
{
    return &tws_link_txpwr_thd;
}

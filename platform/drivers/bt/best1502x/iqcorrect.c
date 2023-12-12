#ifndef NOSTD
#if defined (TX_PULLING_CAL)
#include <math.h>
#include <stdio.h>

#include "hal_dma.h"
#include "hal_timer.h"
#include "hal_trace.h"

#include "string.h"
#include "besbt_string.h"
#include "bt_drv.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_btdump.h"
#include "iqcorrect.h"
#include "bt_drv_internal.h"
#include "bt_drv_1502x_internal.h"
#include "pmu.h"
#include "hal_chipid.h"
//#include "bt_drv_iq_common.h"

#define FFTSIZE 1024
//#define PA_BIAS_DEPUG
//#define PULLING_DEBUG

#define bitshift 10
#define pi (3.1415926535898)

volatile int iqimb_dma_status = 0;

const uint16_t win[512] = {
    #include "conj_win_1024_half.txt"
};

typedef struct ComplexInt_
{
    int re;
    int im;
} ComplexInt;
typedef struct Complexflt_
{
    float re;
    float im;
} ComplexFlt;
typedef struct ComplexShort_
{
    short re;
    short im;
} ComplexShort;

// scan +-320k energy(for pa bias calib, log delay calib)
static void Tblgen_iq(ComplexFlt* w0, int len)
{
    for (int i=0; i<len; i++) {
        w0[i].re = (float)(cos(2*pi*i/(1024.0/58)));
        w0[i].im = (float)(sin(2*pi*i/(1024.0/58)));
    }
}

// scan +-16k energy(for log vres calib)
static void Tblgen_iq_1st(ComplexFlt* w0, int len)
{
    for (int i=0; i<len; i++) {
        w0[i].re = (float)(cos(2*pi*i/(1024.0/99)));
        w0[i].im = (float)(sin(2*pi*i/(1024.0/99)));
    }
}


typedef struct TxPullingState_
{
    short *M0data;
    ComplexFlt *Table0;
} TxPullingState;

TxPullingState *TxPullingState_init(int fft_size)
{
    BT_DRV_TRACE(1,"%s, malloc ini",__func__);
    TxPullingState *st = (TxPullingState *)bt_drv_malloc(1 * sizeof(TxPullingState));
    if (st) {
        st->M0data = (short*)bt_drv_malloc((fft_size * 3) * sizeof(short));
        st->Table0 = (ComplexFlt*)bt_drv_malloc((fft_size) * sizeof(ComplexFlt));
        BT_DRV_TRACE(1,"%s malloc ok", __func__);
    }
    return st;
}

int32_t TxPullingState_destroy(TxPullingState *st)
{
    bt_drv_free(st->M0data);
    bt_drv_free(st->Table0);
    bt_drv_free(st);
    return 0;
}

int PullingParameterCalc_ex(const short *M0data, short Cohcnt, ComplexFlt *Table0,int fftsize)
{
    int i,j,k;
    float M0 = 0;
    ComplexFlt tmp0;
    for (j = 0; j < Cohcnt; j++) {
        tmp0.re = 0.0f;
        tmp0.im = 0.0f;
        for (i=0; i<fftsize; i++) {
            if (i < (fftsize / 2)) {
                k = i;
            } else {
                k = fftsize - i - 1;
            }
            tmp0.re = tmp0.re + ((float)M0data[2 * i] * Table0[i].re - (float)M0data[2 * i + 1] * Table0[i].im) * (float)(win[k] / 3276.0);
            tmp0.im = tmp0.im + ((float)M0data[2 * i] * Table0[i].im + (float)M0data[2 * i + 1] * Table0[i].re) * (float)(win[k] / 3276.0);
        }
        tmp0.re = (tmp0.re/ 1024.0);// >> bitshift;
        tmp0.im = (tmp0.im/ 1024.0);// >>bitshift;
        M0 = M0 + tmp0.re*tmp0.re + tmp0.im*tmp0.im;
    }

    return (int)(M0/Cohcnt * 100);
}


void caculate_energy_main_test(TxPullingState *st,int* Energy,int* Energy1,int fftsize)
{
    short Cohcnt = 1;
    *Energy1 = PullingParameterCalc_ex(st->M0data+1024, Cohcnt, st->Table0, fftsize);
}

static struct HAL_DMA_DESC_T iqimb_dma_desc[1];
static uint8_t g_dma_channel = HAL_DMA_CHAN_NONE;

static void iqimb_dma_dout_handler(uint32_t remains, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    if (g_dma_channel != HAL_DMA_CHAN_NONE) {
        hal_audma_free_chan(g_dma_channel);
    }
    hal_btdump_disable();
    iqimb_dma_status = 0;

    return;
}

int bt_iqimb_dma_enable ( short * dma_dst_data, uint16_t size)
{
    int sRet = 0;
    struct HAL_DMA_CH_CFG_T iqimb_dma_cfg;

    iqimb_dma_status = 1;

    sRet = memset_s(&iqimb_dma_cfg, sizeof(iqimb_dma_cfg), 0, sizeof(iqimb_dma_cfg));
    if (sRet){
        TRACE(1, "%s line:%d sRet:%d", __func__, __LINE__, sRet);
    }

    iqimb_dma_cfg.ch = hal_audma_get_chan(HAL_AUDMA_BTDUMP,HAL_DMA_HIGH_PRIO);
    ASSERT((HAL_DMA_CHAN_NONE != iqimb_dma_cfg.ch), "hal_audma_get_chan failed.");
    g_dma_channel = iqimb_dma_cfg.ch;
    iqimb_dma_cfg.dst_bsize = HAL_DMA_BSIZE_16;
    iqimb_dma_cfg.dst_periph = 0; //useless
    iqimb_dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
    iqimb_dma_cfg.handler = (HAL_DMA_IRQ_HANDLER_T)iqimb_dma_dout_handler;
    iqimb_dma_cfg.src = 0; // useless
    iqimb_dma_cfg.src_bsize = HAL_DMA_BSIZE_4;

    iqimb_dma_cfg.src_periph = HAL_AUDMA_BTDUMP;
    iqimb_dma_cfg.src_tsize = size;//1600; //1600*2/26=123us
    iqimb_dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
    iqimb_dma_cfg.try_burst = 1;
    iqimb_dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
    iqimb_dma_cfg.dst = (uintptr_t)(dma_dst_data);

    hal_audma_init_desc(&iqimb_dma_desc[0], &iqimb_dma_cfg, 0, 1);

    hal_audma_sg_start(&iqimb_dma_desc[0], &iqimb_dma_cfg);

    //wait
    for(volatile int i=0; i<5000; i++);
    hal_btdump_enable();

    return 1;

}

void check_mem_data(void* data, int len)
{
    short* share_mem = (short*)data;
    BT_DRV_TRACE(3,"check_mem_data :share_mem= %p, 0x%x, 0x%x",share_mem,share_mem[0],share_mem[1]);

    int32_t i =0;
    int32_t remain = len;

    while(remain > 0)
    {
        for(i=0; i<32; i++)//output two line
        {
            if (remain >16)
            {
                DUMP16("%04X ",share_mem,16);
                share_mem +=16;
                remain -= 16;
            }
            else
            {
                DUMP16("%04X ",share_mem,remain);
                remain =0;
                return;
            }
        }
        //  BT_DRV_TRACE(0,"\n");
        //BT_DRV_TRACE(1,"addr :0x%08x\n",share_mem);
        hal_sys_timer_delay(MS_TO_TICKS(100));
    }
}

//g/p mismatch base addr 0x002E
int bt_rfimb_add_mismatch(int phase_mis)
{
    uint16_t phase_mis_high = phase_mis << 9;
    uint16_t val;
    val = phase_mis_high | phase_mis;
    btdrv_write_rf_reg(0x002E,val);

    return 1;
}

#define TX_PULLING_CAL_CNT                 8

#define TX_PULLING_LOGEN_DELAY_DR   0x40A0

#define TX_PULLING_CALIB_CHL_2405          0
#define TX_PULLING_CALIB_CHL_2415          1
#define TX_PULLING_CALIB_CHL_2425          2
#define TX_PULLING_CALIB_CHL_2435          3
#define TX_PULLING_CALIB_CHL_2445          4
#define TX_PULLING_CALIB_CHL_2455          5
#define TX_PULLING_CALIB_CHL_2465          6
#define TX_PULLING_CALIB_CHL_2475          7

void bt_tx_pulling_write_second_txpwr(const int cnt, const uint16_t phase_low, const uint16_t phase_high)
{
    uint16_t RF_REG1 = 0x164;
    uint16_t RF_REG2 = 0x17c;

    RF_REG1 = 0x164 + cnt;
    BTRF_REG_SET_FIELD(RF_REG1, 0x1FF, 0, phase_low);

    RF_REG2 = 0x17c + (cnt/2);
    if (cnt % 2 == 1) {
        BTRF_REG_SET_FIELD(RF_REG2, 0x7F, 7, phase_high);
    } else {
        BTRF_REG_SET_FIELD(RF_REG2, 0x7F, 0, phase_high);
    }

    TRACE(5, "%s 0x%x=0x%x , 0x%x=0x%x", __func__, RF_REG1, phase_low, RF_REG2, phase_high);
}

void bt_tx_pulling_write_max_txpwr(const int cnt, const uint16_t phase_low, const uint16_t phase_high)
{
    uint16_t RF_REG1 = 0x16C;
    uint16_t RF_REG2 = 0x180;

    RF_REG1 = 0x16C + cnt;
    BTRF_REG_SET_FIELD(RF_REG1, 0x1FF, 0, phase_low);

    RF_REG2 = 0x180 + (cnt/2);
    if (cnt % 2 == 1) {
        BTRF_REG_SET_FIELD(RF_REG2, 0x7F, 7, phase_high);
    } else {
        BTRF_REG_SET_FIELD(RF_REG2, 0x7F, 0, phase_high);
    }

    TRACE(5, "%s 0x%x=0x%x , 0x%x=0x%x", __func__, RF_REG1, phase_low, RF_REG2, phase_high);
}

void tx_pulling_rf_dig_set(const uint16_t bbpll_sdm_freqword, const uint32_t rc_step, const uint32_t test_channel)
{
    TRACE(4, "%s rf_de=0x%x, d0350248=0x%x, d0220c00=0x%x", __func__, bbpll_sdm_freqword, rc_step, test_channel);
    btdrv_write_rf_reg(0xDE, bbpll_sdm_freqword);

    btdrv_write_rf_reg(0xdf, 0x0001);
    hal_sys_timer_delay(MS_TO_TICKS(2));
    btdrv_write_rf_reg(0xdf, 0x0003);

    BTDIGITAL_REG_WR(0xd0350248, rc_step);
    BTDIGITAL_REG_WR(0xd0220c00, test_channel);
    // reset tx farrow filter
    BTDIGITAL_REG_WR(0xD0330058, 0x410000);
    BTDIGITAL_REG_WR(0xD0330060, 0x410000);
    BTDIGITAL_REG_WR(0xD0330064, 0x410000);
    hal_sys_timer_delay(MS_TO_TICKS(2));
}

// calib log vres, optimeze first-order pulling(=- 160k)
void bt_log_vres_calib(void)
{
    const int fftsize = 1024;
    uint16_t rf_local = 0;
    uint32_t dig1, dig2 = 0;
    TxPullingState *st = NULL;
    st = TxPullingState_init(FFTSIZE);
    Tblgen_iq_1st(st->Table0,FFTSIZE);
    int tmp_energy, tmp_energy1, min_energy = 1000000000;

    rf_local = 0x9640 + 3 * 0xA0;
    dig2 = 0x800a0003 + 3 * 0xA;
    dig1 = 0x80bea3f9;
    tx_pulling_rf_dig_set(rf_local, dig1, dig2);
    bt_rfimb_add_mismatch(160);

    int min_vres = 0;

    for(int vres = 0xA; vres <= 0xF; vres++)
    {
        BTRF_REG_SET_FIELD(0xCF, 0xF, 12, vres);
        bt_rfimb_add_mismatch(160); //no mismatch
        bt_iqimb_dma_enable(st->M0data,FFTSIZE+512);
        while(1) {
            if(iqimb_dma_status==0)
                break;
        }
        caculate_energy_main_test(st,&tmp_energy,&tmp_energy1,fftsize);
        if(min_energy > tmp_energy1) {
            min_vres = vres;
            min_energy = tmp_energy1;
        }
        TRACE(0,"%s, energy:%d, vres:%d", __func__, tmp_energy1, vres);
    }

    BTDIGITAL_REG_WR(0xd0220c00, 0x0);      //turn of tx on

    BTRF_REG_SET_FIELD(0xCF, 0xF, 12, min_vres);
    TRACE(0,"best energy:%d, best vres:%d", min_energy, min_vres);
    TxPullingState_destroy(st);
}

uint8_t txPullingPhaseCnt_get(void)
{
    uint16_t rf_c4, rf_2d, rf_2a;
    uint8_t log_cal_value;

    btdrv_read_rf_reg(0xC4, &rf_c4);
    btdrv_read_rf_reg(0x2D, &rf_2d);
    btdrv_read_rf_reg(0x2A, &rf_2a);

    btdrv_write_rf_reg(0xC4, 0x7C89);
    btdrv_write_rf_reg(0x2D, 0x0322);
    BTRF_REG_SET_FIELD(0x2A, 1, 9, 0);
    hal_sys_timer_delay(MS_TO_TICKS(1));
    BTRF_REG_SET_FIELD(0x2A, 1, 9, 1);

    BTDIGITAL_REG_WR(0xD0220C00, 0xA004E);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    BTRF_REG_GET_FIELD(0x58, 0xFF, 0, log_cal_value);
    if (log_cal_value > 235) {
        log_cal_value = 235;
    }

    TRACE_IMM(2, "%s log_cal_value=%d.\n",__func__, log_cal_value);

    //reset
    BTDIGITAL_REG_WR(0xD0220C00, 0);
    btdrv_write_rf_reg(0xC4, rf_c4);
    btdrv_write_rf_reg(0x2D, rf_2d);
    btdrv_write_rf_reg(0x2A, rf_2a);

    return log_cal_value;
}

// pa bias scan range
const uint16_t pa_calib_rf_0xca[] = {
#if (PACKAG_TYPE == BTDRV_PACKAG_2700L)
    0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
#elif (PACKAG_TYPE == BTDRV_PACKAG_2700IBP)
    0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D,
    0x98, 0x99, 0x9A, 0x9B, 0x9C,
    0xA8, 0xA9, 0xAA, 0xAB, 0xAC,
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC,
#else
    0x56, 0x57, 0x58, 0x59, 0x5A, 0x5D, 0x5E, 0x5F,
    0x66, 0x67, 0x68, 0x69, 0x6A, 0x6D, 0x6E, 0x6F,
    0x76, 0x77, 0x78, 0x79, 0x7A, 0x7D, 0x7E, 0x7F,
    0x86, 0x87, 0x88, 0x89, 0x8A, 0x8D, 0x8E, 0x8F,
    0x96, 0x97, 0x98, 0x99, 0x9A, 0x9D, 0x9E, 0x9F,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAD, 0xAE, 0xAF
#endif
};

static int bt_pa_bias_get_energy(uint16_t pa_bias, int phase_val, TxPullingState *st)
{
    int tmp_energy = 1000000000;
    int Energy1 = 1000000000;
    int fftsize = FFTSIZE;

    BTRF_REG_SET_FIELD(0xCA, 0xFF, 8, pa_bias);
    bt_rfimb_add_mismatch(phase_val); //no mismatch
    bt_iqimb_dma_enable(st->M0data, fftsize+512);
    while(1) {
        if(iqimb_dma_status==0)
            break;
    }
    caculate_energy_main_test(st, &Energy1, &tmp_energy, fftsize);
#ifdef PA_BIAS_DEPUG
    TRACE(0,"0xca:0x%x, energy:%d", pa_bias, tmp_energy);
#endif

    return tmp_energy;
}

static int bt_tx_pa_calib_get_best_pa_bias(int calib_chl, int phase_val, TxPullingState *st)
{
    uint16_t rf_local = 0;
    uint32_t dig1, dig2 = 0;
    uint16_t pa_calib_rf_0xca_size = 0;
    uint16_t fdata1_bank2_addr;

    BTRF_REG_SET_FIELD(0x10, 0x3, 0, 3);        //enable 0xCA reg
    int tmp_energy, min_energy = 1000000000;

    rf_local = 0x9640 + calib_chl * 0xA0;
    dig2     = 0x800A0003 + calib_chl * 0xA;

    if(calib_chl <= 2) {
        dig1 = 0x80bb80d6 + calib_chl * 0x10BB6;
    } else if((calib_chl > 2) && (calib_chl < 5)) {
        dig1 = 0x80bea3f9 + (calib_chl - 3) * 0x10BB6;
    } else {
        dig1 = 0x80c0bb66 + (calib_chl - 5) * 0x10BB6;
    }

    tx_pulling_rf_dig_set(rf_local, dig1, dig2);

    int min_num = 0;
    pa_calib_rf_0xca_size = ARRAY_SIZE(pa_calib_rf_0xca);
    for(int ii = 0; ii < pa_calib_rf_0xca_size; ii++)
    {
        tmp_energy = bt_pa_bias_get_energy(pa_calib_rf_0xca[ii], phase_val, st);
        if(min_energy > tmp_energy) {
            min_num = ii;
            min_energy = tmp_energy;
        }
    }
    fdata1_bank2_addr = 0x174 + calib_chl;
    BTRF_REG_SET_FIELD(0xCA, 0xFF, 8, pa_calib_rf_0xca[min_num]);
    BTRF_REG_SET_FIELD(fdata1_bank2_addr, 0xFF, 0, pa_calib_rf_0xca[min_num]);
    BT_DRV_TRACE(0,"%s, energy:%d, best_ca:0x%x", __func__, min_energy, pa_calib_rf_0xca[min_num]);
    BTRF_REG_SET_FIELD(0x10, 0x3, 0, 0);        //restore 0xCA reg
    BTDIGITAL_REG_WR(0xd0220c00, 0x0);

    return tmp_energy;
}

static int bt_tx_pa_calib_t0_energy_get(void)
{
    const int fftsize = FFTSIZE;
    TxPullingState *st = NULL;
    st = TxPullingState_init(fftsize);
    Tblgen_iq(st->Table0,fftsize);
    int min_energy = 1000000000;

    min_energy = bt_tx_pa_calib_get_best_pa_bias(TX_PULLING_CALIB_CHL_2435, TX_PULLING_LOGEN_DELAY_DR, st);
    BTRF_REG_SET_FIELD(0x10, 0x3, 0, 3);        //enable 0xCA reg

    TxPullingState_destroy(st);

    return min_energy;
}

static void bt_tx_pa_calib_t0(void)
{
    int energy = 0;
    uint8_t calib_time = 0;
    uint8_t calib_flag = 0;
    uint16_t logen_vres = 0;

    BTRF_REG_GET_FIELD(0xCF, 0xF, 12, logen_vres);

    if (logen_vres <= 13) {
        calib_flag = 1;
    } else {
        calib_flag = 0;
    }

calib_again:
    energy = bt_tx_pa_calib_t0_energy_get();

    if (energy > 10000) {
        // if logen_vres ±2, energy > 10000, trace warning
        if (calib_time == 2) {
            BT_DRV_TRACE(0,"WARNING: %s energy > 10000", __func__);
            goto exit;
        }

        if (calib_flag) {
            logen_vres += 1;
        } else {
            logen_vres -= 1;
        }

        BTRF_REG_SET_FIELD(0xCF, 0xF, 12, logen_vres);
        calib_time ++;
        goto calib_again;
    }

 exit:
    return;
}

static void bt_tx_pa_calib_t2(void)
{
    const int fftsize = FFTSIZE;
    TxPullingState *st = NULL;
    st = TxPullingState_init(fftsize);
    Tblgen_iq(st->Table0,fftsize);

#ifdef SLIM_PULLINGL_CALIB
    bt_tx_pa_calib_get_best_pa_bias(TX_PULLING_CALIB_CHL_2435, TX_PULLING_LOGEN_DELAY_DR, st);
    BTRF_REG_SET_FIELD(0x10, 0x3, 0, 3);        //enable 0xCA reg
#else
    for (uint8_t calib_chl = 0; calib_chl < TX_PULLING_CAL_CNT; calib_chl++) {
        bt_tx_pa_calib_get_best_pa_bias(calib_chl, TX_PULLING_LOGEN_DELAY_DR, st);
    }
#endif
    TxPullingState_destroy(st);
}

static void bt_tx_pa_calib(void)
{
    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();

    if (metal_id < HAL_CHIP_METAL_ID_2) {
        bt_tx_pa_calib_t0();
    } else {
        bt_tx_pa_calib_t2();
    }
}

uint16_t bt_tx_pulling_get_phase_step(uint16_t phase_tmp)
{
    uint16_t phase_step = 0;

#ifdef SLIM_PULLINGL_CALIB
    if (phase_tmp < 88) {
        phase_step = 16;    //0~79
    } else if (phase_tmp < 124) {
        phase_step = 8;     //96~127
    } else if (phase_tmp < 200) {
        phase_step = 16;
    } else {
        phase_step = 8;
    }
#else
    if (phase_tmp < 64) {
        phase_step = 16;    //0~79
    } else if (phase_tmp < 88) {
        phase_step = 8;     //80~95
    } else if (phase_tmp < 124) {
        phase_step = 4;     //96~127
    } else if (phase_tmp < 200) {
        phase_step = 8;
    } else {
        phase_step = 4;
    }
#endif

    return phase_step;
}


#define TX_PULLING_CAL_IDX_5        4
#define TX_PULLING_CAL_IDX_4        3

static void bt_tx_pulling_algorithm(uint8_t calib_tx_idx)
{
    uint8_t phase_count = 176;
    uint16_t rf_de_value = 0;
    uint32_t rc_step_val, test_chnl = 0;
    uint16_t phase_step = 0;

    TxPullingState *st = NULL;
    st = TxPullingState_init(FFTSIZE);
    Tblgen_iq(st->Table0,FFTSIZE);

    phase_count = txPullingPhaseCnt_get() + 16;
    if (phase_count > 234) {
        phase_count = 234;
    }

    for (int k = 0; k < TX_PULLING_CAL_CNT; k++) {
        rf_de_value = 0x9640 + k * 0xA0;
        test_chnl = 0x800A0003 + k * 0xA;

        if(k<=2) {
            rc_step_val = 0x80bb80d6 + k*0x10BB6;
        } else if((k>2) && (k<5)) {
            rc_step_val = 0x80bea3f9 + (k-3)*0x10BB6;
        } else {
            rc_step_val = 0x80c0bb66 + (k-5)*0x10BB6;
        }

        tx_pulling_rf_dig_set(rf_de_value, rc_step_val, test_chnl);

        int phase_tmp = 0 ;
        int phase_min_val = 0;
        int cur_energy = 1000000000;
        int Energy,Energy1 = 0;

        //Get phase_min_val
        for (phase_tmp = 0; phase_tmp <= phase_count; phase_tmp += phase_step) {
            bt_rfimb_add_mismatch(phase_tmp);
            bt_iqimb_dma_enable(st->M0data,FFTSIZE+512);
            while(1) {
                if(iqimb_dma_status==0)
                    break;
            }
            //check_mem_data((uint8_t *)st->M0data, 3*1024);
            caculate_energy_main_test(st,&Energy,&Energy1,FFTSIZE);

            if (cur_energy > Energy1) {
                phase_min_val = phase_tmp;
                cur_energy = Energy1;
            }
            phase_step = bt_tx_pulling_get_phase_step(phase_tmp);
#ifdef PULLING_DEBUG
            TRACE(0,"%s, phase_tmp:%d, energy:%d, phase_count:%d", __func__, phase_tmp, Energy1, phase_count);
#endif
        }

        BTDIGITAL_REG_WR(0xd0220c00, 0x0);      //turn of tx on

        BT_DRV_TRACE(3, "%s phase_min_val=0x%x, cur_energy=%d.", __func__, phase_min_val, cur_energy);

        if (calib_tx_idx == TX_PULLING_CAL_IDX_5) {
            bt_tx_pulling_write_max_txpwr(k, (phase_min_val & 0x1FF), (phase_min_val & 0x7F));      //calib max tx pwr idx4
        } else if (calib_tx_idx == TX_PULLING_CAL_IDX_4) {
            bt_tx_pulling_write_second_txpwr(k, (phase_min_val & 0x1FF), (phase_min_val & 0x7F));   //calib second tx pwr idx3
        }
    }
    TxPullingState_destroy(st);
}

void bt_tx_pulling_calib(void)
{
    uint32_t time_start = hal_sys_timer_get();
    uint8_t calib_tx_idx;

    enum HAL_CHIP_METAL_ID_T metal_id;
    metal_id = hal_get_chip_metal_id();

    // tx gian logen delay offset enable idx
    if (metal_id < HAL_CHIP_METAL_ID_2) {
        BTRF_REG_SET_FIELD(0x188, 0x7, 3, 3);   // idx 3 use fada1
        BTRF_REG_SET_FIELD(0x188, 0x7, 0, 2);   // idx 0-2 use fada0
    } else {
        BTRF_REG_SET_FIELD(0x188, 0x7, 3, 7);   // idx 7 use fada1
        BTRF_REG_SET_FIELD(0x188, 0x7, 0, 6);   // idx 0-6 use fada0
    }

    //dr tx gain
    BTRF_REG_SET_FIELD(0x24, 0x1, 4, 0x1);      // rf reg 0x24 resetore in func btdrv_tx_pulling_cal

    calib_tx_idx = TX_PULLING_CAL_IDX_5;
    if (metal_id < HAL_CHIP_METAL_ID_2) {
        BTRF_REG_SET_FIELD(0x24, 0x7, 5, 0x3);      //dr tx gain idx3
    } else {
        BTRF_REG_SET_FIELD(0x24, 0x7, 5, 0x7);      //dr tx gain idx7
    }
    if (metal_id < HAL_CHIP_METAL_ID_2) {
        bt_log_vres_calib();
    }
    bt_tx_pa_calib();

    bt_tx_pulling_algorithm(calib_tx_idx);
    calib_tx_idx = TX_PULLING_CAL_IDX_4;
    if (metal_id < HAL_CHIP_METAL_ID_2) {
        BTRF_REG_SET_FIELD(0x24, 0x7, 5, 0x2);      //dr tx gain idx3
    } else {
        BTRF_REG_SET_FIELD(0x24, 0x7, 5, 0x6);      //dr tx gain idx6
    }
    bt_tx_pulling_algorithm(calib_tx_idx);

    BT_DRV_TRACE(1,"use time: %d ms", __TICKS_TO_MS(hal_sys_timer_get()-time_start));
}

#endif
#endif


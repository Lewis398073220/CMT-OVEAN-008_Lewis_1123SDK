/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#include "app_voice_assist_fir_lms.h"
#include "anc_ff_fir_lms.h"

static ANCFFFirLmsSt * fir_st= NULL;

#define BASE_ANC_FILTER_COUNT  (10)
#define MC_FILTER_COUNT        (5)
#define FF_FILTER_COUNT        (8)
#define FB_FILTER_COUNT        (9)
#define FB2EMIC_FILTER_COUNT   (1)

/*
    // low pass filter
    IIR_BIQUARD_LPF,
    // high pass filter
    IIR_BIQUARD_HPF,
    // notch filter
    IIR_BIQUARD_PEAKINGEQ,
    // low shelf filter
    IIR_BIQUARD_LOWSHELF,
    // high shelf filter
    IIR_BIQUARD_HIGHSHELF,
*/
#if (MC_FILTER_COUNT != 0)
#define MC_FILTER_REF_GAIN     (-5.0f)
static FIR_LMS_FILTER_CFG_T  MC_FILT_CFG[MC_FILTER_COUNT] = {
    // {IIR_BIQUARD_LOWSHELF,   -31.0,   12,  0.5,  16000},
    // {IIR_BIQUARD_LOWSHELF,     5.0,  130,  0.7,  16000},
    // {IIR_BIQUARD_PEAKINGEQ,    3.0,  220,  0.5,  16000},
    // {IIR_BIQUARD_PEAKINGEQ,   -5.0,  800,  0.8,  16000},s
    // {IIR_BIQUARD_PEAKINGEQ,  -10.0, 1600,  1.0,  16000},
    // {IIR_BIQUARD_PEAKINGEQ,   -2.0, 2000,  3.0,  16000},
    // {IIR_BIQUARD_PEAKINGEQ,    2.5, 3500,  3.0,  16000},
    // {IIR_BIQUARD_PEAKINGEQ,     15, 5000,  1.5,  16000},
    // {IIR_BIQUARD_HIGHSHELF,   -8.0, 3100,  1.0,  16000},
    // // {IIR_BIQUARD_HIGHSHELF,  -12.0, 2000, 0.9,  16000},

    {IIR_BIQUARD_PEAKINGEQ,   -13.5,   1700,  1.8,  16000},
    {IIR_BIQUARD_PEAKINGEQ,     -6.5,  500,  0.6,  16000},
    {IIR_BIQUARD_PEAKINGEQ,    -4,  1000,  1,  16000},
    {IIR_BIQUARD_LPF,           0,  6000,  1,  16000},
    {IIR_BIQUARD_PEAKINGEQ,     4, 4500,  1,  16000},

};
#endif

#if (FF_FILTER_COUNT != 0)
static float  FF_FILTER_REF_GAIN[BASE_ANC_FILTER_COUNT] = {-2.0f, -2.0f, -2.0f, -2.0f,-2.0f, -2.0f,-2.0f, -2.0f,-2.0f, -2.0f,};
static FIR_LMS_FILTER_CFG_T  FF_FILT_CFG[BASE_ANC_FILTER_COUNT][FF_FILTER_COUNT] = {
{
//mode 0
    // {IIR_BIQUARD_LOWSHELF,  11.5,    40,  0.92, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,    0,   110,   2.0, 16000},
    // {IIR_BIQUARD_LOWSHELF,  -2.4,  570,   0.8, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,    0,   150,   1.5, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,   -4,  2600,   1.5, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,  -23,  3600,   4.0, 16000},
    // {IIR_BIQUARD_HIGHSHELF,  -11,  5600,   1.5, 16000},
    // {IIR_BIQUARD_LOWSHELF,     0,  1300,   2, 16000},
    // {IIR_BIQUARD_LOWSHELF,     0,  500,   0.5, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,    0,  130,   2, 16000},
    // {IIR_BIQUARD_HIGHSHELF,    0,  400,   0.7, 16000},

    {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},

},
//mode1
{
    {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode2
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode3
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode4
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode5
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode6
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode7
{
       {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode8
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
//mode9
{
        {IIR_BIQUARD_PEAKINGEQ,     3,    480,  1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -10,   2400,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     -4,  4500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,   3500,   2, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1.5,  700,   1.5, 16000},
    {IIR_BIQUARD_HIGHSHELF,     1,  480,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -5,  7000,   1.5, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -2,  1500,   1, 16000},
},
};
#endif

#if (FB_FILTER_COUNT != 0)
#define FB_FILTER_REF_GAIN     (5.0f)
static FIR_LMS_FILTER_CFG_T  FB_FILT_CFG[FB_FILTER_COUNT] = {
    // {IIR_BIQUARD_LOWSHELF,    2.0,   100,   1.5, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,  20.0,   130,   0.7, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,   4.5,   500,   1.0, 16000},
    // {IIR_BIQUARD_PEAKINGEQ,  -5.0,  7000,   1.5, 16000},
    // {IIR_BIQUARD_HIGHSHELF,   2.0,   900,   1.1, 16000},
    // {IIR_BIQUARD_PEAKINGEQ, -11.0,  2765,   1.2, 16000},
    // {IIR_BIQUARD_PEAKINGEQ, -12.0,  5305,   1.2, 16000},
    // // {IIR_BIQUARD_HIGHSHELF, -26.0,  18000,   0.7, 16000},
    // {IIR_BIQUARD_LOWSHELF,    1.0,    30,   1.0, 16000},

    {IIR_BIQUARD_HIGHSHELF      -8,   2000,   1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     16,   120,   0.4, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -13,   6000,   1.0, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     -11,  3800,   1, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     6,   700,   1, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.5,  1500,   3, 16000},
    {IIR_BIQUARD_PEAKINGEQ,     5,  300,   1, 16000},
    {IIR_BIQUARD_HIGHSHELF,     2.3,  700,   2, 16000},
    {IIR_BIQUARD_PEAKINGEQ,    -3,    1900,   1.0, 16000},
};
#endif

#if (FB2EMIC_FILTER_COUNT != 0)
#define FB2EMIC_FILTER_REF_GAIN   (0.f)
static FIR_LMS_FILTER_CFG_T  FB2EMIC_FILT_CFG[FB2EMIC_FILTER_COUNT] = {
    {IIR_BIQUARD_PEAKINGEQ,   7.0,   1000,   2.6, 16000},
};
#endif

POSSIBLY_UNUSED const float fir_Sv_coeffs[10] = {1., 0.,};

static ANC_FF_FIR_LMS_CFG_T cfg ={
    .max_cnt = 250,
    .period_cnt = 2,
    .mc_max_cnt = 150,
    .mc_period_cnt = 0,
    .ff_filt_num = BASE_ANC_FILTER_COUNT,
    .mc_filt_cnt = MC_FILTER_COUNT,
    .ff_filt_cnt = FF_FILTER_COUNT,
    .fb_filt_cnt = FB_FILTER_COUNT,
    .fb2emic_filt_cnt = FB2EMIC_FILTER_COUNT,
    .mc_filt_ref_gain = MC_FILTER_REF_GAIN,
    .ff_filt_ref_gain = FF_FILTER_REF_GAIN,
    .fb_filt_ref_gain = FB_FILTER_REF_GAIN,
    .fb2emic_filt_ref_gain = FB2EMIC_FILTER_REF_GAIN,
    .mc_filt_cfg = MC_FILT_CFG,
    .ff_filt_cfg = FF_FILT_CFG[0],
    .fb_filt_cfg = FB_FILT_CFG,
    .fb2emic_filt_cfg = FB2EMIC_FILT_CFG,

};


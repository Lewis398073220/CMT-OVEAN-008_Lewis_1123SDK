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
#include "psap_freq_psap.h"

static float CalibFlt[16]=
{
    0.571316459083046,
    0.183735416895703,
    0.0685002139399859,
    -0.0946331272049063,
    0.00504357840098634,
    -0.0414041097621551,
    -0.127848979480189,
    0.134832979173006,
    0.0657085939167020,
    -0.0830385512439903,
    0.0385443331202435,
    0.0485982315952087,
    -0.0941466886622139,
    0.0383293916640809,
    0.00697331679705550,
    -0.0573107611945590,
};

PsapFreqPsapConfig psap_freq_psap_cfg = {
    .bypass = 0,
    .debug_en = 0,

    .ref_delay = {14,10},
    .firfilter = 0,
    .phase_shift = 0,
    .fre_shift = 0,
    .bf_enable = 0,
    .dehowling_enable = 0,
    .howling_af_enable = 0,
    .ns_enable = 0,
    .wdrc_enable = 0,
    .eq_enable = 0,
    .reverberator_enable = 0,
    .limiter_enable = 0,

    .bf_cfg = {
        .init_enable = 0,
        .dist = 0.020,
        .calib_filter = CalibFlt,
        .calib_filter_len = ARRAY_SIZE(CalibFlt),
    },

    .dehowling_cfg = {
        .onoff = 0,
        .debug_en = 0,
        .power_thd = 0.1,       // energy threshold
        .factor = 100,
        .attact_cnt = 30,       // attact time
        .release_cnt = 5000,    // attact time
    },

    .howling_af_cfg = {
        .delay = 7,
    },

    .ns_cfg = {
        .init_status = 2,
        .ns_power_thd = 0.1,
    },

    .wdrc_cfg = {
        // left
        {
            .total_gain = 6.0,
            .psap_band_num = 13,
            .PSAP_CS = {0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999},
            .PSAP_CT = {-40, -25,-15, -15, -15, -15, -15, -15, -15, -15, -15, -15},
            .PSAP_WS = {0.0, 0.0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1},
            .PSAP_WT = {0.0, 0.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0, -30.0},
            .PSAP_ES = {-0.9999,  -0.9999,  -0.9999,  -0.9999,   -0.9999, -0.9999,  -0.9999,  -0.9999,  -0.9999,  -0.9999,  -0.9999, -0.9999, -0.9999},
            .PSAP_ET = {-85, -85, -85, -85, -85, -85, -85, -85, -85, -85, -85, -85, -85},
            .PSAP_AT = {25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25},
            .PSAP_RT = {25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25},
            .PSAP_TA = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1},
            .PSAP_GAIN = {  0,  6, 15, 16, 18, 22,  22, 22,   20,   15,    8,    0,  0},
            .PSAP_FREQ = { 0, 200,400,600,800,1000,1600,2500, 3750,  4875,5250,  6000, 10000},
        },
        // right
        {
            .total_gain = 0.0,
            .psap_band_num = 13,
            .PSAP_CS = {0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999},
            .PSAP_CT = {-40, -25,-10, -10, -10, -10, -10, -10, -10, -10, -10, -10},
            .PSAP_WS = {0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999, 0.9999},
            .PSAP_WT = {-20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0},
            .PSAP_ES = {-0.9999,  -0.9999,  -0.9999,  -0.9999,   -0.9999, -0.9999,  -0.9999,  -0.9999,  -0.9999,  -0.9999,  -0.9999, -0.9999, -0.9999},
            .PSAP_ET = {-60, -60, -60, -60, -60, -60, -60, -60, -60, -60, -60, -60, -60},
            .PSAP_AT = {25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25},
            .PSAP_RT = {25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25},
            .PSAP_TA = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1},
            .PSAP_GAIN = {  0,    0,   0,   0,   0,    0,   0,   0,    0,    0,     0,    0,    0},
            .PSAP_FREQ = { 375, 750, 1125, 2250, 3000, 3750, 4500, 4875, 5250, 6000, 8000, 10000},
        }
    },

    .eq_cfg = {
        // left
        {
            .bypass = 1,
            .gain   = 0.f,
            .num    = 1,
            .params = {
                {IIR_BIQUARD_PEAKINGEQ, {{600, 0, 0.707f}}},
            },
        },
        // right
        {
            .bypass = 0,
            .gain   = 0.f,
            .num    = 1,
            .params = {
                {IIR_BIQUARD_PEAKINGEQ, {{600, 0, 0.707f}}},
            },
        },
    },

    .reverberator_cfg = {
        //left
        {
            .bypass = 0,
            .reverberance = 50,     //  Degree of reverberation, %
            .hf_damping = 100,      //  High-frequency damping, %, Frequency signal attenuation degree
            .room_scale = 100,      //  Room size, %
            .pre_delay_ms = 10,     //  Predelay, ms, The interval between the direct sound and the first reflection
            .wet_gain_dB = 0,       // Wet gain, db, Control the gain of wet reverberation
            .comb_lengths = { 1116/2, 1188/2, 1277/2, 1356/2, 1422/2, 1491/2, 1557/2, 1617/2},
            .comb_used = 8,
            .allpass_lengths = { 225/2, 341/2, 441/2, 556/2 },
            .allpass_used = 4,
        },
        //right
        {
            .bypass = 0,
            .reverberance = 50,
            .hf_damping = 100,
            .room_scale = 100,
            .pre_delay_ms = 10,
            .wet_gain_dB = 0,
            .comb_lengths = { 1116/2, 1188/2, 1277/2, 1356/2, 1422/2, 1491/2, 1557/2, 1617/2},
            .comb_used = 8,
            .allpass_lengths = { 225/2, 341/2, 441/2, 556/2 },
            .allpass_used = 4,
        },
    },

    .limiter_cfg = {
        .knee = 2,
        .look_ahead_time = 10,
        .threshold = -1,
        .makeup_gain = 0,
        .ratio = 1000,
        .attack_time = 1,
        .release_time = 50,
    },
};

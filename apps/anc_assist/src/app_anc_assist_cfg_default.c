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
#include "plat_types.h"
#include "anc_assist.h"


#if defined (VOICE_ASSIST_CUSTOM_LEAK_DETECT)
const static int custom_leak_detect_pcm_data[] = {
#include "res/ld/tone_peco.h"
};
#else
const static int custom_leak_detect_pcm_data[] = {
   0,0,0
};
#endif
AncAssistConfig anc_assist_cfg = {
    .bypass = 0,
    .debug_en = 0,
    .wind_debug_en = 0,

    .enable_list_cfg = {
        .ff_howling_en  = 0,
        .fb_howling_en  = 0,
        .noise_en   = 0,
        .wind_en    = 0,
        .pilot_en   = 0,
        .pnc_en     = 0,
        .noise_classify_en  = 0,
        .wsd_en     = 0,
        .extern_kws_en     = 0,
        .extern_adaptive_eq_en     = 0,
#if defined(ANC_ASSIST_VPU)
        .vpu_anc_en   = 0,
#endif
    },

    .ff_howling_cfg = {
        .ind0 = 32,         // 62.5Hz per 62.5*32=2k
        .ind1 = 120,        // 62.5*120=7.5k
        .time_thd = 500,    // ff recover time 500*7.5ms=3.75s
        .power_thd = 1e7f,
    },

    .fb_howling_cfg = {
        .ind0 = 32,         // 62.5Hz per 62.5*32=2k
        .ind1 = 120,        // 62.5*120=7.5k
        .time_thd = 500,    // ff recover time 500*7.5ms=3.75s
        .power_thd = 1e8f,
    },

    .noise_cfg = {
        .strong_low_thd = 320,
        .strong_limit_thd = 60,
        .lower_low_thd = 3,
        .lower_mid_thd = 12, 
        .quiet_out_thd = 1,  
        .quiet_thd = 0.6,
        .snr_thd = 100,
        .period = 16,
        .window_size = 5,
        .strong_count = 20,
        .normal_count = 12,
        .band_freq = {100, 400, 1000, 2000},
        .band_weight = {0.5, 0.5, 0},
    },

    .wind_cfg = {
        .scale_size = 8,           // freq range,8/scale=1k
        .to_none_targettime = 500, // time=500*7.5ms=3.75s
        .power_thd = 0.0001, 
        .no_thd = 0.8,
        .small_thd = 0.7,
        .normal_thd = 0.55,
        .strong_thd = 0.4,
        .gain_none = 1.0,
        .gain_small_to_none = 0.7500,
        .gain_small = 0.5,
        .gain_normal_to_small = 0.3750,
        .gain_normal = 0.25,
        .gain_strong_to_normal = 0.1563,
        .gain_strong = 0.0625,
    },

    .pilot_cfg = {
        .dump_en = 0,
        .delay = 310,
#ifdef VOICE_ASSIST_WD_ENABLED
        .cal_period = 25,
#else
        .cal_period = 25,
#endif
        .gain_smooth_ms = 300,

        .adaptive_anc_en = 0,
        .playback_ms = 0,
        .thd_on = {5.6400,  1.5648,  0.6956,  0.4672,  0.4089,  0.2694, 0.1853,0.1373,0.0629,0.0325,0.0043},
        .thd_off = {5.6400,  1.5648,  0.6956,  0.4672,  0.4089,  0.2694, 0.1853,0.1373,0.0629,0.0325,0.0043},

        .wd_en = 0,
        .debug_en = 0,
        .infra_ratio = 0.7,
        .ultrasound_stable_tick = 2,
        .ultrasound_stop_tick = 3,
        .ultrasound_thd = 0.1,
        .inear_thd = 1, //0.1,
        .outear_thd = 0.5, // 0.02,
        .energy_cmp_num = 8, //

        .custom_leak_detect_en = 0,
        .custom_leak_detect_playback_loop_num = 2,
        .custom_pcm_data = custom_leak_detect_pcm_data,
        .custom_pcm_data_len = sizeof(custom_leak_detect_pcm_data) / sizeof(int16_t),
        .gain_local_pilot_signal = 0.0,
    },

    .prompt_cfg = {
        .dump_en = 0,
        .cal_period = 10,
        .curve_num = 6,
        .max_env_energy = 1e9,
        .start_index = 1,
        .end_index = 6,
        .thd1 = {1,0,-2,-4,-6,-8,-10,-12},
        .thd2 = {1.5,-0.5,-2.5,-4.5,-6.5,-8.5,-10.5,-12.5},
        .thd3 = {1,-1,-3,-5,-7,-9,-11,-13}
    },

    .auto_eq_cfg = {
        .delay = 0,
        .cal_period = 500,
        .thd = {0,-2,-4,-6,-8,-10},
    },

    .pnc_cfg = {
        .pnc_lower_bound = 1400*256/16000,
        .pnc_upper_bound = 1500*256/16000,
        .out_lower_bound = 1400*256/16000,
        .out_upper_bound = 1500*256/16000,
        .cal_period = 12,
        .out_thd = 100,
    },

    .noise_classify_cfg = {
        .plane_thd = 700,
        .plane_hold_thd = 350,
        .transport_thd = 500,
        .transport_hold_thd = 250,
        .indoor_thd = 15,
        .indoor_hold_thd = 40,
        .snr_thd = 100,
        .period = 16,
        .window_size = 5,
        .strong_count = 8,
        .normal_count = 12,
        .band_freq = {100, 250, 500, 1000},
        .band_weight = {0.5, 0.5, 0},
    },

    .stereo_wsd_cfg = {
        .dist1 = 0.04,
        .dist2 = 0.18,
        .min_frequency = 150,
        .max_frequency = 900,
        .feat1_threshold = 2,
        .feat2_threshold = 4,
        .feat3_threshold = 0.4,
        .wsd_in_time = 0.05,// 0.02,
        .wsd_out_time = 0.1,
    }
};
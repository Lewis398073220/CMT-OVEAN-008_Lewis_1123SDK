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
#ifndef __PSAP_FREQ_PSAP_H__
#define __PSAP_FREQ_PSAP_H__

#include "plat_types.h"
#include "speech_eq.h"
#include "limiter.h"
#include "reverberator.h"
#include "hal_aud.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSAP_HEARTBEAT_CNT  (6 * 2000)

#define MAX_PLAY_CHANNEL_NUM      (2)

typedef enum {
    PSAP_SW_LOWPASS = 0,
    PSAP_SW_HIGHPASS,
    PSAP_SW_NOTCH,
    PSAP_SW_PEAKING,
} PSAP_SW_FILTER_t;

typedef struct {
    bool        init_enable;
    float       dist;
    float       *calib_filter;
    uint32_t    calib_filter_len;
} psap_freq_bf_cfg_t;

typedef struct {
    int32_t    onoff;
    int32_t    debug_en;
    float      power_thd;
    float      factor;
    int32_t    attact_cnt;
    int32_t    release_cnt;
} psap_freq_dehowling_cfg;

typedef struct {
    int32_t     delay;
} psap_freq_howling_af_cfg_t;

typedef struct {
    int         init_status;
    float       ns_power_thd;
} psap_freq_ns_cfg_t;

#define PSAP_BAND_NUM 17
typedef struct {
    float       total_gain;
    int32_t     psap_band_num;
    float       PSAP_CS[PSAP_BAND_NUM];
    float       PSAP_CT[PSAP_BAND_NUM];
    float       PSAP_WS[PSAP_BAND_NUM];
    float       PSAP_WT[PSAP_BAND_NUM];
    float       PSAP_ES[PSAP_BAND_NUM];
    float       PSAP_ET[PSAP_BAND_NUM];
    float       PSAP_AT[PSAP_BAND_NUM];
    float       PSAP_RT[PSAP_BAND_NUM];
    float       PSAP_TA[PSAP_BAND_NUM];
    float       PSAP_GAIN[PSAP_BAND_NUM];
    float       PSAP_FREQ[PSAP_BAND_NUM - 1];
} psap_freq_wdrc_cfg_t;

typedef EqConfig psap_freq_eq_cfg_t;
typedef LimiterConfig psap_freq_limiter_cfg_t;
typedef ReverberatorConfig psap_freq_reverberator_cfg_t;

typedef struct
{
    float       band_width;
    float       conver_speed;
} psap_freq_adp_notch_cfg_t;

typedef struct {
    int32_t     bypass;
    uint32_t    debug_en;

    uint32_t    bf_enable;
    uint32_t    ref_delay[MAX_PLAY_CHANNEL_NUM];
    uint32_t    firfilter;
    uint32_t    phase_shift;
    uint32_t    fre_shift;
    uint32_t    dehowling_enable;
    uint32_t    howling_af_enable;
    uint32_t    adap_notch_enable;
    uint32_t    ns_enable;
    uint32_t    wdrc_enable;
    uint32_t    eq_enable;
    uint32_t    reverberator_enable;
    uint32_t    limiter_enable;

    psap_freq_bf_cfg_t          bf_cfg;
    psap_freq_dehowling_cfg     dehowling_cfg;
    psap_freq_howling_af_cfg_t  howling_af_cfg;
    psap_freq_ns_cfg_t          ns_cfg;
    psap_freq_wdrc_cfg_t        wdrc_cfg[MAX_PLAY_CHANNEL_NUM];
    psap_freq_eq_cfg_t          eq_cfg[MAX_PLAY_CHANNEL_NUM];
	psap_freq_reverberator_cfg_t reverberator_cfg[MAX_PLAY_CHANNEL_NUM];
    psap_freq_limiter_cfg_t     limiter_cfg;
    psap_freq_adp_notch_cfg_t   adap_notch_cfg;
} PsapFreqPsapConfig;

typedef struct {
    float       *psap_sw_ns_gain;
    float       *psap_sw_wdrc_gain;
} PsapFreqPsapRes;

struct PsapFreqPsapState_;
typedef struct PsapFreqPsapState_ PsapFreqPsapState;

void psap_freq_psap_fadein(bool on);
void psap_freq_psap_fadeout(bool on);

int32_t psap_freq_psap_reset(PsapFreqPsapState *st);
PsapFreqPsapState *psap_freq_psap_create(int sample_rate, int sample_bits, int frame_size, uint32_t play_ch_num, const PsapFreqPsapConfig *cfg);
int32_t  psap_freq_psap_destroy(PsapFreqPsapState *st);
int32_t psap_freq_psap_set_config(PsapFreqPsapState *st, const PsapFreqPsapConfig *cfg);
int32_t psap_freq_psap_process(PsapFreqPsapState *st, int32_t **ff_mic, int32_t **talk_mic, int32_t **ref_mic, int32_t **out1, const PsapFreqPsapConfig *cfg);

int32_t psap_freq_set_total_gain(float *gain);
float *psap_freq_get_total_gain(void);

int32_t psap_freq_psap_set_eq_config(PsapFreqPsapState *st, const psap_freq_eq_cfg_t *cfg, enum AUD_CHANNEL_MAP_T ch);
int32_t psap_freq_set_dehowling_config(PsapFreqPsapState *st, const psap_freq_dehowling_cfg *cfg);
int32_t psap_freq_set_reverb_config(PsapFreqPsapState *st, const psap_freq_reverberator_cfg_t *cfg);

int32_t psap_freq_psap_get_required_mips(PsapFreqPsapState *st);


#ifdef __cplusplus
}
#endif

#endif
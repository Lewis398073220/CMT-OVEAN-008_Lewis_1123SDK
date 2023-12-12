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
#include "stdio.h"
#include "string.h"
#include "hal_trace.h"
#include "psap_freq_psap.h"
#include "psap_sw_app.h"
#include "psap_sw_app_ctrl.h"
#include "stream_dma_rpc.h"

#ifdef PSAP_SW_THIRDPARTY
static psap_sw_cmd_t g_psap_sw_cmd;

static void psap_sw_bth_send_to_dsp(psap_sw_cmd_t *cmd)
{
    if (psap_sw_app_is_opened()) {
        dma_rpc_set_algo_cfg(0, 0, cmd, sizeof(psap_sw_cmd_t));
    } else {
        TRACE(1, "[%s] WARNING: PSAP SW is closed", __func__);
    }
}

void psap_sw_bth_send_cmd_to_dsp(psap_sw_cmd_t *cmd)
{
    memcpy(&g_psap_sw_cmd, cmd, sizeof(psap_sw_cmd_t));
    psap_sw_bth_send_to_dsp(&g_psap_sw_cmd);
}

int32_t psap_sw_app_set_gain(int32_t gain)
{
    psap_sw_cmd_t cfg;

    memset(&cfg, 0, sizeof(psap_sw_cmd_t));
    cfg.mode = PSAP_SW_THIRD_CMD_GAIN;
    cfg.gain = gain;
    psap_sw_bth_send_cmd_to_dsp(&cfg);

    return 0;
}

void psap_sw_app_ctrl_test(const char *cmd)
{
    uint32_t val = 10;

    if (!strncmp(cmd, "gain=", strlen("gain="))) {
        sscanf(cmd, "gain=%d", &val);
        uint32_t gain = val;
        psap_sw_app_set_gain(gain);
    } else {
        TRACE(1, "invalid cmd: %s", cmd);
    }
}
#else
static psap_sw_cmd_t g_psap_sw_cmd = {
    .mode = PSAP_SW_CMD_MODE_QTY,
    .bf_enable = 0,
    .ns_level = 0
};

static const uint32_t g_noise_reduction_table[NOISE_REDUCTION_QTY] = {
    0,
    1,
    2,
    4,
    6
};

static void psap_sw_bth_send_to_dsp(psap_sw_cmd_t *cmd)
{
    if (psap_sw_app_is_opened()) {
        dma_rpc_set_algo_cfg(0, 0, cmd, sizeof(psap_sw_cmd_t));
    } else {
        TRACE(1, "[%s] WARNING: PSAP SW is closed", __func__);
    }
}

void psap_sw_bth_send_cmd_to_dsp(psap_sw_cmd_t *cmd)
{
    memcpy(&g_psap_sw_cmd, cmd, sizeof(psap_sw_cmd_t));
    psap_sw_bth_send_to_dsp(&g_psap_sw_cmd);
}

int32_t psap_sw_set_total_gain(float *total_gain)
{
    psap_sw_cmd_t cmd;

    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_TOTAL_GAIN;

    cmd.total_gain[0] = total_gain[0];
    cmd.total_gain[1] = total_gain[1];

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

int32_t psap_sw_set_band_gain(float *band_gain)
{
    psap_sw_cmd_t cmd;

    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_MODE_WDRC;

    for (uint32_t i = 0; i < PSAP_BAND_NUM; i++) {
        cmd.band_gain[i] = band_gain[i];
    }

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

int32_t psap_sw_set_beamforming(uint32_t enable)
{
    ASSERT(enable < BEAMFORMING_QTY, "[%s] Enable is invalid: %d", __func__, enable);
    TRACE(2, "[%s] enable: %d", __func__, enable);

    psap_sw_cmd_t cmd;
    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_MODE_BF;
    cmd.bf_enable = enable;

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

int32_t psap_sw_set_noise_reduction(uint32_t level)
{
    ASSERT(level < NOISE_REDUCTION_QTY, "[%s] Level is invalid: %d", __func__, level);
    TRACE(2, "[%s] level: %d", __func__, level);

    psap_sw_cmd_t cmd;
    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_MODE_NS;
    cmd.ns_level = g_noise_reduction_table[level];

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

int32_t psap_sw_set_eq(psap_freq_eq_cfg_t *eq_l, psap_freq_eq_cfg_t *eq_r)
{
    psap_sw_cmd_t cmd;
    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_MODE_EQ;
    cmd.eq_config[0] = eq_l;
    cmd.eq_config[1] = eq_r;

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

int32_t psap_sw_set_dehowling(psap_freq_dehowling_cfg *dehowling)
{
    psap_sw_cmd_t cmd;
    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_MODE_DEHOWLING;
    cmd.dehowling_config = *dehowling;

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

psap_freq_reverberator_cfg_t audio_test_reverb_cfg = {
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
};
int32_t psap_sw_set_reverb(float *data)
{
    psap_sw_cmd_t cmd;
    memset(&cmd, 0, sizeof(psap_sw_cmd_t));
    cmd.mode = PSAP_SW_CMD_MODE_REVERB;
    cmd.reverb_cfg = &audio_test_reverb_cfg;
    audio_test_reverb_cfg.reverberance = data[0];
    audio_test_reverb_cfg.hf_damping = data[1];
    audio_test_reverb_cfg.room_scale = data[2];
    audio_test_reverb_cfg.pre_delay_ms = data[3];

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

int32_t psap_sw_set_all_cfg(float *total_gain, float *band_gain, uint32_t bf_enable, uint32_t ns_level)
{
    ASSERT(bf_enable < BEAMFORMING_QTY, "[%s] bf_enable is invalid: %d", __func__, bf_enable);
    ASSERT(ns_level < NOISE_REDUCTION_QTY, "[%s] ns_level is invalid: %d", __func__, ns_level);

    TRACE(6, "[%s] Total Gain L: %d, Total Gain R: %d, band gain[0]: %d, bf_enable: %d, ns_level: %d", __func__,
            (int32_t)total_gain[0], (int32_t)total_gain[1], (int32_t)band_gain[0], bf_enable, ns_level);

    psap_sw_cmd_t cmd;
    memset(&cmd, 0, sizeof(psap_sw_cmd_t));

    cmd.mode = PSAP_SW_CMD_MODE_ALL;
    cmd.total_gain[0] = total_gain[0];
    cmd.total_gain[0] = total_gain[1];
    for (uint32_t i = 0; i < PSAP_BAND_NUM; i++) {
        cmd.band_gain[i] = band_gain[i];
    }

    cmd.bf_enable = bf_enable;
    cmd.ns_level = g_noise_reduction_table[ns_level];

    psap_sw_bth_send_cmd_to_dsp(&cmd);

    return 0;
}

/******************** TEST ********************/
static psap_freq_eq_cfg_t eq_config[4] = {
    {
        .bypass = 0,
        .gain   = 0.f,
        .num    = 6,
        .params = {
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
        },
    },
    {
        .bypass = 0,
        .gain   = 0.f,
        .num    = 6,
        .params = {
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
            {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
        },
    },
    {
        .bypass = 0,
        .gain   = 0.f,
        .num    = 1,
        .params = {
            {IIR_BIQUARD_HPF, {{6000, 0, 0.707f}}},
        },
    },
    {
        .bypass = 0,
        .gain   = 0.f,
        .num    = 1,
        .params = {
            {IIR_BIQUARD_HPF, {{6000, 0, 0.707f}}},
        },
    },
};

static psap_freq_dehowling_cfg dehowling = {
    .onoff = 1,
    .debug_en = 1,
    .power_thd = 0.1,
    .factor = 100,
    .attact_cnt = 30,
    .release_cnt = 5000,
};

static void audio_test_psap_sw_set_eq_index(uint32_t index)
{
    if (index >= ARRAY_SIZE(eq_config) / 2) {
        TRACE(2, "[%s] index is invalid: %d", __FUNCTION__, index);
        return;
    }

    TRACE(2, "[%s] index: %d", __FUNCTION__, index);
    psap_sw_set_eq(&eq_config[2 * index], &eq_config[2 * index + 1]);
}

int msft_250_gain_l  = 0;
int msft_500_gain_l  = 0;
int msft_1000_gain_l = 0;
int msft_2000_gain_l = 0;
int msft_4000_gain_l = 0;
int msft_8000_gain_l = 0;

int msft_250_gain_r  = 0;
int msft_500_gain_r  = 0;
int msft_1000_gain_r = 0;
int msft_2000_gain_r = 0;
int msft_4000_gain_r = 0;
int msft_8000_gain_r = 0;

psap_freq_eq_cfg_t msft_eq_cfg_l = {
    .bypass = 0,
    .gain = 0.f,
    .num = 6,
    .params = {
        {IIR_BIQUARD_PEAKINGEQ, {{250,  0,  0.7}}},//left
        {IIR_BIQUARD_PEAKINGEQ, {{500,  0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{1000, 0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{2000, 0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{4000, 0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{8000, 0,  0.7}}},
    }
};

psap_freq_eq_cfg_t msft_eq_cfg_r = {
    .bypass = 0,
    .gain = 0.f,
    .num = 6,
    .params = {
        {IIR_BIQUARD_PEAKINGEQ, {{250,  0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{500,  0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{1000, 0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{2000, 0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{4000, 0,  0.7}}},
        {IIR_BIQUARD_PEAKINGEQ, {{8000, 0,  0.7}}},
    }
};

void psap_sw_app_ctrl_test(const char *cmd)
{
    uint32_t val = 10;
    if (!strncmp(cmd, "total_gain", strlen("total_gain"))) {
        sscanf(cmd + strlen("total_gain "), "%d", &val);
        float total_gain[2] = {(float)val};
        psap_sw_set_total_gain(total_gain);
    } else if (!strncmp(cmd, "band_gain", strlen("band_gain"))) {
        sscanf(cmd + strlen("band_gain "), "%d", &val);
        float band_gain[17] = {(float)val};
        psap_sw_set_band_gain(band_gain);
    } else if (!strncmp(cmd, "noise_reduction", strlen("noise_reduction"))) {
        sscanf(cmd + strlen("noise_reduction "), "%d", &val);
        psap_sw_set_noise_reduction(val);
    } else if (!strncmp(cmd, "eq", strlen("eq"))) {
        sscanf(cmd + strlen("eq "), "%d", &val);
        audio_test_psap_sw_set_eq_index(val);
    } else if (!strncmp(cmd, "bf", strlen("bf"))){
        sscanf(cmd + strlen("bf "), "%d", &val);
        psap_sw_set_beamforming(val);
    } else if (!strncmp(cmd, "howling", strlen("howling"))) {
        psap_sw_set_dehowling(&dehowling);
    } else if (!strncmp(cmd, "gain", strlen("gain"))) {
        TRACE(0,"CHRIS_DEBUG: %s", cmd + strlen("gain "));
        sscanf(cmd + strlen("gain "), "%d %d %d %d %d %d %d %d %d %d %d %d", &msft_250_gain_l,
                                                            &msft_500_gain_l,
                                                            &msft_1000_gain_l,
                                                            &msft_2000_gain_l,
                                                            &msft_4000_gain_l,
                                                            &msft_8000_gain_l,
                                                            &msft_250_gain_r,
                                                            &msft_500_gain_r,
                                                            &msft_1000_gain_r,
                                                            &msft_2000_gain_r,
                                                            &msft_4000_gain_r,
                                                            &msft_8000_gain_r);

        TRACE(0,"CHRIS_DEBUG, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", msft_250_gain_l,
                                                    msft_500_gain_l,
                                                    msft_1000_gain_l,
                                                    msft_2000_gain_l,
                                                    msft_4000_gain_l,
                                                    msft_8000_gain_l,
                                                    msft_250_gain_r,
                                                    msft_500_gain_r,
                                                    msft_1000_gain_r,
                                                    msft_2000_gain_r,
                                                    msft_4000_gain_r,
                                                    msft_8000_gain_r);

        msft_eq_cfg_l.params[0].design.gain = msft_250_gain_l*1.0;
        msft_eq_cfg_l.params[1].design.gain = msft_500_gain_l*1.0;
        msft_eq_cfg_l.params[2].design.gain = msft_1000_gain_l*1.0;
        msft_eq_cfg_l.params[3].design.gain = msft_2000_gain_l*1.0;
        msft_eq_cfg_l.params[4].design.gain = msft_4000_gain_l*1.0;
        msft_eq_cfg_l.params[5].design.gain = msft_8000_gain_l*1.0;

        msft_eq_cfg_r.params[0].design.gain = msft_250_gain_r*1.0;
        msft_eq_cfg_r.params[1].design.gain = msft_500_gain_r*1.0;
        msft_eq_cfg_r.params[2].design.gain = msft_1000_gain_r*1.0;
        msft_eq_cfg_r.params[3].design.gain = msft_2000_gain_r*1.0;
        msft_eq_cfg_r.params[4].design.gain = msft_4000_gain_r*1.0;
        msft_eq_cfg_r.params[5].design.gain = msft_8000_gain_r*1.0;

        psap_sw_set_eq(&msft_eq_cfg_l, &msft_eq_cfg_r);
    } else {
        TRACE(1, "invalid cmd: %s", cmd);
    }
}
#endif
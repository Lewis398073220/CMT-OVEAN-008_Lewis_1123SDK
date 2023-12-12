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
#ifdef DMA_AUDIO_APP
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "string.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "hal_aud.h"
#include "stream_dma_rpc.h"
#include "dma_audio_host.h"
#include "dma_audio_def.h"
#ifdef CP_AS_SUBSYS
#include "cp_subsys.h"
#endif
#ifdef DMA_RPC_SVR_CORE_M55
#include "dsp_m55.h"
#include "mcu_dsp_m55_app.h"
#endif
#ifdef DMA_RPC_SVR_CORE_HIFI
#include "dsp_m55.h"
#include "mcu_dsp_hifi4_app.h"
#endif
#include "dma_audio_cli.h"
#ifdef DMA_AUDIO_TEST
#include "dma_audio_test.h"
#endif
#ifdef AUDIO_DSP_ACCEL_CP_SUBSYS
#include "mcpp_cmd.h"
#endif

#define BTPCM_STREAM_ID AUD_STREAM_ID_1
#define SCO_STREAM_ID   AUD_STREAM_ID_0

#define DMA_AUDIO_MAX_WAIT_TIME_MS 100
#define DMA_AUDIO_DLY_TIME_MS      1

static int dma_aud_click_state = 0;
static int dsp_core_opened = 0;

#ifdef CP_AS_SUBSYS
static unsigned int rx_handler(const void *data, unsigned int len)
{
#ifdef AUDIO_DSP_ACCEL_CP_SUBSYS
    if (mcpp_check_rpc_cmd(len)) {
        mcpp_baremetal_received_data_handler(data, len);
        return len;
    }
#endif
    unsigned int rlen = len;

    rlen = dma_audio_rx_data_handler(data, len);
    if (rlen != len) {
        TRACE(0, "[%s]: not dma audio msg, len=%d", __func__, len);
    }
    return len;
}

static void tx_handler(const void *data, unsigned int len)
{
#ifdef AUDIO_DSP_ACCEL_CP_SUBSYS
    if (mcpp_check_rpc_cmd(len)) {
        mcpp_baremetal_send_data_done_handler(data, len);
        return;
    }
#endif
    dma_audio_tx_data_handler(data, len);
}

void cli_open_forward_stream(enum AUD_STREAM_T stream)
{
    int i;

    for (i = 0; i < AUD_STREAM_ID_NUM; i++) {
        if (i == DAUD_STREAM_ID || i == BTPCM_STREAM_ID) {
            continue;
        }
        dma_rpcif_forward_stream_open(i, stream);
        TRACE(0, "enable audio forward stream: id=%d, stream=%d", i, stream);
    }
}

void cli_close_forward_stream(enum AUD_STREAM_T stream)
{
    int i;

    for (i = 0; i < AUD_STREAM_ID_NUM; i++) {
        if (i == DAUD_STREAM_ID || i == BTPCM_STREAM_ID) {
            continue;
        }
        dma_rpcif_forward_stream_close(i, AUD_STREAM_CAPTURE);
        TRACE(0, "disable audio forward stream: id=%d, stream=%d", i, stream);
    }
}

static int cli_cp_subsys_open(void)
{
    int r = 0;

    if (!dsp_core_opened) {
        r = cp_subsys_open(rx_handler, tx_handler);
        ASSERT(r == 0, "[%s]: cp_subsys_open failed, r=%d", __func__, r);
#if defined(DMA_AUDIO_USE_ADC1)
        cli_open_forward_stream(AUD_STREAM_CAPTURE);
#endif
        dsp_core_opened = 1;
    }

    return 0;
}

static int cli_cp_subsys_close(void)
{
    int r = 0;

    if (dsp_core_opened) {
        r = cp_subsys_close();
        ASSERT(r == 0, "[%s]: cp_subsys_close failed, r=%d", __func__, r);
#if defined(DMA_AUDIO_USE_ADC1)
        cli_close_forward_stream(AUD_STREAM_CAPTURE);
#endif
        dsp_core_opened = 0;
    }
    return 0;
}
#endif /* CP_AS_SUBSYS */

int dma_audio_cli_open(void)
{
    int r = 0;

    dma_audio_init();

#ifdef CP_AS_SUBSYS
    r = cli_cp_subsys_open();
#endif
    TRACE(0, "[%s]: done, r=%d", __func__, r);
    return r;
}

int dma_audio_cli_close(void)
{
    int r = 0;

#ifdef CP_AS_SUBSYS
    r = cli_cp_subsys_close();
#endif

    TRACE(0, "[%s]: done, r=%d", __func__, r);
    return r;
}

#ifdef DMA_RPC_SVR_CORE_M55
static int cli_dsp_m55_open(void)
{
    volatile int tmcnt = DMA_AUDIO_MAX_WAIT_TIME_MS / DMA_AUDIO_DLY_TIME_MS;

    if (!dsp_core_opened) {
        app_dsp_m55_first_init();
        app_dsp_m55_init(APP_DSP_M55_USER_DMA_AUDIO);

        TRACE(1, "[%s]: wait DSP boot ...", __func__);
        while (!app_dsp_m55_is_running()) {
            osDelay(DMA_AUDIO_DLY_TIME_MS);
            tmcnt--;
            if (tmcnt == 0) {
                TRACE(1, "[%s]: WARNING: wait timeout", __func__);
                return -1;
            }
        }
        //osDelay(10);
        dsp_core_opened = 1;
    }
    TRACE(0, "[%s]: done", __func__);
    return 0;
}

static int cli_dsp_m55_close(void)
{
    if (dsp_core_opened) {
        app_dsp_m55_deinit(APP_DSP_M55_USER_DMA_AUDIO);
        dsp_core_opened = 0;
    }
    TRACE(0, "[%s]: done", __func__);
    return 0;
}
#endif

#ifdef DMA_RPC_SVR_CORE_HIFI
#ifndef APP_RPC_ENABLE
#error "APP_RPC_ENABLE should be defined for DMA_RPC_SVR_CORE_HIFI !!"
#endif
#ifndef DSP_COMBINE_BIN
#error "DSP_COMBINE_BIN should be defined for DMA_RPC_SVR_CORE_HIFI !!"
#endif

static int cli_dsp_hifi_open(void)
{
    if (!dsp_core_opened) {
        app_dsp_hifi4_init(APP_DSP_HIFI4_USER_DMA_AUDIO);
        int32_t wait_cnt = 0;
        while(app_dsp_hifi4_is_running() == false) {
            hal_sys_timer_delay_us(10);
            if (wait_cnt++ > 300000) {    // 3s
                ASSERT(0, "%s: hifi4 is hung", __func__);
            }
        }
        dsp_core_opened = 1;
    }
    TRACE(0, "[%s]: done", __func__);
    return 0;
}

static int cli_dsp_hifi_close(void)
{
    if (dsp_core_opened) {
        app_dsp_hifi4_deinit(APP_DSP_HIFI4_USER_DMA_AUDIO);
        dsp_core_opened = 0;
    }
    TRACE(0, "[%s]: done", __func__);
    return 0;
}
#endif

static void cli_key_click_normal(void)
{
    POSSIBLY_UNUSED int ret = 0;

    dma_aud_click_state ^= 1;

    if (dma_aud_click_state) {
#if defined(DMA_RPC_SVR_CORE_M55)
        ret = cli_dsp_m55_open();
#elif defined(DMA_RPC_SVR_CORE_HIFI)
        ret = cli_dsp_hifi_open();
#endif
        ASSERT(ret == 0, "[%s]: open DSP core failed, ret=%d", __func__, ret);
    }

    dma_audio_on(!!dma_aud_click_state, NULL, NULL);

    if (!dma_aud_click_state) {
#ifdef DMA_RPC_SVR_CORE_M55
        ret = cli_dsp_m55_close();
#elif defined(DMA_RPC_SVR_CORE_HIFI)
        ret = cli_dsp_hifi_close();
#endif
        ASSERT(ret == 0, "[%s]: close DSP core failed, ret=%d", __func__, ret);
    }
}

void dma_audio_cli_key_click(enum DAUD_CLI_KEY_T key)
{
    if (key == DAUD_CLI_KEY_NORMAL) {
        cli_key_click_normal();
    } else {
        TRACE(1, "[%s]: not support key %d", __func__, key);
    }
}

void cli_normal_key_on(void)
{
    POSSIBLY_UNUSED int ret = 0;

#if defined(DMA_RPC_SVR_CORE_M55)
    ret = cli_dsp_m55_open();
#elif defined(DMA_RPC_SVR_CORE_HIFI)
    ret = cli_dsp_hifi_open();
#endif
    ASSERT(ret == 0, "[%s]: open DSP core failed, ret=%d", __func__, ret);

    dma_aud_click_state = 1;
    dma_audio_on(!!dma_aud_click_state, NULL, NULL);
}

void cli_normal_key_off(void)
{
    POSSIBLY_UNUSED int ret = 0;

    dma_aud_click_state = 0;
    dma_audio_on(!!dma_aud_click_state, NULL, NULL);

#if defined(DMA_RPC_SVR_CORE_M55)
    ret = cli_dsp_m55_close();
#elif defined(DMA_RPC_SVR_CORE_HIFI)
    ret = cli_dsp_hifi_close();
#endif
    ASSERT(ret == 0, "[%s]: close DSP core failed, ret=%d", __func__, ret);

}

void dma_audio_cli_key_on(enum DAUD_CLI_KEY_T key)
{
    switch (key) {
    case DAUD_CLI_KEY_NORMAL:
        cli_normal_key_on();
        break;
#ifdef DMA_AUDIO_TEST
    case DAUD_CLI_KEY_TEST_LOOP:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_ADDA_LOOP_ON);
        break;
    case DAUD_CLI_KEY_TEST_PLAY1:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_PLAYBACK1_ON);
        break;
    case DAUD_CLI_KEY_TEST_PLAY2:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_PLAYBACK2_ON);
        break;
    case DAUD_CLI_KEY_TEST_CAP1:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_CAPTURE1_ON);
        break;
#endif
    default:
        TRACE(1, "[%s]: unkonwn key=%d", __func__, key);
        break;
    }
}

void dma_audio_cli_key_off(enum DAUD_CLI_KEY_T key)
{
    switch (key) {
    case DAUD_CLI_KEY_NORMAL:
        cli_normal_key_off();
        break;
#ifdef DMA_AUDIO_TEST
    case DAUD_CLI_KEY_TEST_LOOP:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_ADDA_LOOP_OFF);
        break;
    case DAUD_CLI_KEY_TEST_PLAY1:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_PLAYBACK1_OFF);
        break;
    case DAUD_CLI_KEY_TEST_PLAY2:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_PLAYBACK2_OFF);
        break;
    case DAUD_CLI_KEY_TEST_CAP1:
        dma_audio_app_test_by_cmd(DMA_AUD_TEST_CMD_CAPTURE1_OFF);
        break;
#endif
    default:
        TRACE(1, "[%s]: unkonwn key=%d", __func__, key);
        break;
    }
}

#endif

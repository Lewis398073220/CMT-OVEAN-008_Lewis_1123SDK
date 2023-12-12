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
#ifndef __DMA_AUDIO_TEST_H__
#define __DMA_AUDIO_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"

enum DMA_AUD_TRIG_MODE_T {
    DMA_AUD_TRIG_MODE_PWRKEY,
    DMA_AUD_TRIG_MODE_TIMER,
};

enum DMA_AUD_TEST_CMD_T {
    DMA_AUD_TEST_CMD_APP_OFF,
    DMA_AUD_TEST_CMD_APP_ON,
    DMA_AUD_TEST_CMD_ADDA_LOOP_ON,
    DMA_AUD_TEST_CMD_PLAYBACK1_ON,
    DMA_AUD_TEST_CMD_PLAYBACK1_OFF,
    DMA_AUD_TEST_CMD_PLAYBACK2_ON,
    DMA_AUD_TEST_CMD_PLAYBACK2_OFF,
    DMA_AUD_TEST_CMD_CAPTURE1_ON,
    DMA_AUD_TEST_CMD_CAPTURE1_OFF,
    DMA_AUD_TEST_CMD_ADDA_LOOP_OFF,
    DMA_AUD_TEST_CMD_DELAY_TEST,
    DMA_AUD_TEST_CMD_QTY,
};

void dma_audio_app_test(enum DMA_AUD_TRIG_MODE_T mode, bool open_af);

int dma_audio_app_test_key_handler(uint32_t code, uint8_t event);

void dma_audio_app_test_by_cmd(enum DMA_AUD_TEST_CMD_T cmd);

int playback1_stream_open(bool open);

int playback1_stream_start(bool on);

int playback2_stream_open(bool open);

int playback2_stream_start(bool on);

int capture1_stream_open(bool open);

int capture1_stream_start(bool on);

#ifdef __cplusplus
}
#endif

#endif

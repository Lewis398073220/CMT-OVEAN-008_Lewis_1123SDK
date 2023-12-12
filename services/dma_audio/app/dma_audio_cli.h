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
#ifndef __DMA_AUDIO_CLI_H__
#define __DMA_AUDIO_CLI_H__

#ifdef __cplusplus
extern "C" {
#endif

enum DAUD_CLI_KEY_T {
    DAUD_CLI_KEY_NORMAL,
    DAUD_CLI_KEY_TEST_LOOP,
    DAUD_CLI_KEY_TEST_PLAY1,
    DAUD_CLI_KEY_TEST_PLAY2,
    DAUD_CLI_KEY_TEST_CAP1,
};

int dma_audio_cli_open(void);

int dma_audio_cli_close(void);

void dma_audio_cli_key_click(enum DAUD_CLI_KEY_T key);

void dma_audio_cli_key_on(enum DAUD_CLI_KEY_T key);

void dma_audio_cli_key_off(enum DAUD_CLI_KEY_T key);

#ifdef __cplusplus
}
#endif

#endif

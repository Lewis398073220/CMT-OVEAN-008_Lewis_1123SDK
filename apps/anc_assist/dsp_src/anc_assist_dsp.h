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
#ifndef __ANC_ASSIST_DSP_H__
#define __ANC_ASSIST_DSP_H__

#include "plat_types.h"
#include "app_anc_assist_core.h"
#include "app_voice_assist_dsp.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t anc_assist_dsp_open(void);
int32_t anc_assist_dsp_close(void);
int32_t anc_assist_dsp_register(anc_assist_user_t user, app_voice_assist_dsp_t* anc_assist_dsp, anc_assist_user_fs_t user_fs);
int32_t anc_assist_dsp_send_result_to_bth(anc_assist_user_t user, uint8_t *buf, uint32_t len,  uint32_t cmd);
int32_t anc_assist_dsp_send_result_to_bth_via_fifo(uint8_t user, uint8_t* res_data, uint32_t len, uint8_t cmd);
int32_t anc_assist_dsp_capture_process(float *in_buf[], uint32_t frame_len);
int32_t anc_assist_dsp_capture_cmd(uint8_t *buf, uint32_t len);

// TEST
int32_t anc_assist_dsp_thread_loop_test(void);

#ifdef __cplusplus
}
#endif

#endif
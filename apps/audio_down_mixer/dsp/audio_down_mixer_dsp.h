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
#ifndef __AUDIO_DOWN_MIXER_DSP_H__
#define __AUDIO_DOWN_MIXER_DSP_H__

#ifdef __cplusplus
extern "C" {
#endif

int32_t audio_down_mixer_dsp_register_cb(bool en);

int32_t audio_down_mixer_dsp_init(void);

int32_t audio_down_mixer_dsp_deinit(void);

int32_t audio_down_mixer_dsp_process(uint8_t *dst, uint32_t dst_len, uint8_t *src, uint32_t src_len);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DOWN_MIXER_DSP_H__ */

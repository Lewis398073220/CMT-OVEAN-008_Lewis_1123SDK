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
#ifndef __PSAP_SW_DSP_H__
#define __PSAP_SW_DSP_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t psap_sw_dsp_register_cb(bool en);

int32_t psap_sw_dsp_init(struct DAUD_STREAM_CONFIG_T *cap_cfg, struct DAUD_STREAM_CONFIG_T *play_cfg);

int32_t psap_sw_dsp_deinit(void);

int32_t psap_sw_dsp_process(uint8_t *play_buf, uint32_t play_len, uint8_t *cap_buf, uint32_t cap_len);

#ifdef __cplusplus
}
#endif

#endif /* __PSAP_SW_DSP_H__ */

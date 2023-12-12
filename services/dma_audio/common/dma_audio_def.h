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
#ifndef __DMA_AUDIO_DEF_H__
#define __DMA_AUDIO_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_aud.h"
#include "hal_sysfreq.h"

/* The DMA audio stream ID value */
#define DAUD_STREAM_ID      AUD_STREAM_ID_4

/* The DMA audio stream fade default time */
#define DAUD_STREAM_FADE_MS     20

/* The DMA audio stream skip fade default time */
#define DAUD_STREAM_SKIP_FADE_MS     100

/* The DMA normal stream ID value */
#define DAUD_NORM_STREAM_ID AUD_STREAM_ID_1

/* DMA audio clock user */
#define DAUD_FREQ_USER      (HAL_SYSFREQ_USER_APP_8)

/* DMA audio clock frequency */
#if defined(CHIP_ROLE_CP) || defined(PSAP_SW_APP)
#define DAUD_FREQ_CLOCK     (HAL_CMU_FREQ_208M)
#elif defined(AUDIO_DOWN_MIXER)
#define DAUD_FREQ_CLOCK     (HAL_CMU_FREQ_208M)
#else /* !CHIP_ROLE_CP */
#define DAUD_FREQ_CLOCK     (HAL_CMU_FREQ_104M)
#endif /* #ifdef CHIP_ROLE_CP */

/* DMA audio host clock user */
#define DAUD_HOST_FREQ_USER  (HAL_SYSFREQ_USER_APP_8)

/* DMA audio host clock frequency*/
#ifdef CHIP_ROLE_CP
#define DAUD_HOST_FREQ_CLOCK (HAL_CMU_FREQ_208M)
#else /* !CHIP_ROLE_CP */
#define DAUD_HOST_FREQ_CLOCK (HAL_CMU_FREQ_26M)
#endif /* #ifdef CHIP_ROLE_CP */

#ifdef __cplusplus
}
#endif

#endif


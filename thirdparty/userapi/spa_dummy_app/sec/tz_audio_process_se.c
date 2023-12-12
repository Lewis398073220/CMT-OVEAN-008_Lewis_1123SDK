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
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "hal_codec.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "crc32_c.h"
#include "cmsis.h"
#include "pmu.h"
#include "hal_location.h"

#include "tz_audio_process.h"

/*
    design for ARM_CMSE interface
*/
#ifdef ARM_CMSE
CMSE_API int tz_audio_process_info_init_cmse(void * tz_info)
{
    return tz_audio_process_info_init(tz_info);
}

CMSE_API int tz_audio_process_info_deinit_cmse(void)
{
    return tz_audio_process_info_deinit();
}

CMSE_API void tz_audio_process_demo_func_enable_disable_cmse(bool enable_disable)
{
    tz_audio_process_demo_func_enable_disable(enable_disable);
}

CMSE_API void tz_audio_process_stream_proc_handler_cmse(void * stream_buf, unsigned int stream_len)
{
    tz_audio_process_stream_proc_handler(stream_buf,stream_len);
}
#endif


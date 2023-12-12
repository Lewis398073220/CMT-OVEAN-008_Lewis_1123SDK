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

static bool ree_audio_test_demo_disable = false;

void ree_audio_test_demo_enable_disable(bool enable_disable)
{
    TRACE(1,"%s enable_disable [%d] [%d]",__func__,ree_audio_test_demo_disable,enable_disable);

    ree_audio_test_demo_disable = enable_disable;
}

void ree_audio_test_demo(void * stream_buf, unsigned int stream_len)
{
    if(ree_audio_test_demo_disable == true){
        memset(stream_buf,0,stream_len);
    }
}


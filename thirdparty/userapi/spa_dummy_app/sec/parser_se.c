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

#include "norflash_api.h"
#include "app_flash_api.h"


#include "bin_aes_crypt.h"
#include "parser.h"
#include "sec_mem.h"
#include "bin_aes_crypt.h"


/*
    design for ARM_CMSE interface
*/
#ifdef ARM_CMSE
CMSE_API int sec_decrypted_section_load_init_cmse(void * tz_info)
{
    if(tz_info == NULL){
        ASSERT(0,"%s in para should not be NULL",__func__);
        return -1;
    }

    tz_customer_load_ram_ramx_section_info_t info = *(tz_customer_load_ram_ramx_section_info_t*)tz_info;
    TRACE(0, "FUNC:%s flash_addr 0x%02x", __func__,info.flash_addr);
    TRACE(1,"LR : 0x%02x mem_addr = 0x%02x",(uint32_t)__builtin_return_address(0),(uint32_t)&tz_info);
    return sec_decrypted_section_load_init(tz_info);
}
#endif


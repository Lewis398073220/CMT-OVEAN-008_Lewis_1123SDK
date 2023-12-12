/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef __DSP_LOADER_H__
#define __DSP_LOADER_H__


#ifdef __cplusplus
extern "C" {
#endif

//#define DSP_BOOT_FORM_FLASH     1
#ifndef DSP_LOGIC_SROM_BASE
#ifdef BTH_AS_MAIN_MCU
#define DSP_LOGIC_SROM_BASE     0x34050000
#else
#define DSP_LOGIC_SROM_BASE     0x28050000
#endif
#endif

int dsp_check_and_startup(uint32_t load_base, uint32_t load_size);

int dsp_combine_bin_startup(void);

int dsp_shutdown(void);

int dsp_open(void);

int dsp_close(void);

#ifdef __cplusplus
}
#endif

#endif

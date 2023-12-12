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

#ifndef _vg_lite_ext_h_
#define _vg_lite_ext_h_
#include <vg_lite.h>

void * vg_lite_get_tsbuffer(void);
uint32_t vg_lite_get_tsbuffer_size(void);
void * vg_lite_get_rtbuffer(void);
uint32_t vg_lite_get_rtbuffer_size(void);
void vg_lite_get_cmdbuf_info(uint32_t *start, uint32_t *bytes);
vg_lite_error_t vg_lite_set_multiply_color(vg_lite_color_t color);

#endif
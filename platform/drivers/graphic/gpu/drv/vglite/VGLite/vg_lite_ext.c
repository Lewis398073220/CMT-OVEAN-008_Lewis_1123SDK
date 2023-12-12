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

#include <vg_lite_context.h>
#include <vg_lite_ext.h>
#include <vg_lite_hw.h>
#include <vg_lite_kernel.h>
#include <vg_lite_hal.h>

void * vg_lite_get_tsbuffer(void)
{
    return (void *)s_context.tessbuf.physical_addr;
}


uint32_t vg_lite_get_tsbuffer_size(void)
{
    return s_context.tessbuf.tessbuf_size;
}

void * vg_lite_get_rtbuffer(void)
{
    return (void *)s_context.rtbuffer->memory;
}


uint32_t vg_lite_get_rtbuffer_size(void)
{
    return s_context.rtbuffer->stride * s_context.rtbuffer->height;
}

uint32_t vg_lite_get_vg_count_buffer_size(void)
{
    return s_context.tessbuf.countbuf_size;
}


void vg_lite_get_cmdbuf_info(uint32_t *start, uint32_t *bytes)
{

    ASSERT((start != 0 && bytes != 0),"%s", __func__ );
    *start = (uint32_t)CMDBUF_BUFFER(s_context);
    *bytes = (uint32_t)CMDBUF_OFFSET(s_context) ;
}

vg_lite_error_t vg_lite_set_multiply_color(vg_lite_color_t color)
{
    // s_multiply_color = color;
    // return VG_LITE_SUCCESS;
}
void gpu_enter_lpwr(int enter)
{
    vg_lite_hw_clock_control_t value;
    uint32_t          reset_timer = 1;
    const uint32_t    reset_timer_limit = 1000;

    if (!enter) {

        /* Disable clock gating. */
        value.data = vg_lite_hal_peek(VG_LITE_HW_CLOCK_CONTROL);
        value.control.clock_gate = 0;
        vg_lite_hal_poke(VG_LITE_HW_CLOCK_CONTROL, value.data);
        //vg_lite_hal_delay(1);
        /* Set clock speed. */
        value.control.scale = 64;
        value.control.scale_load = 1;
        vg_lite_hal_poke(VG_LITE_HW_CLOCK_CONTROL, value.data);
        //vg_lite_hal_delay(1);
        value.control.scale_load = 0;
        vg_lite_hal_poke(VG_LITE_HW_CLOCK_CONTROL, value.data);
        //vg_lite_hal_delay(5);

        do {
            vg_lite_hal_delay(reset_timer);
            reset_timer *= 2;
        } while (!VG_LITE_KERNEL_IS_GPU_IDLE());
		vg_lite_hal_poke(0x00520, 0x4);
            /* Enable all interrupts. */
        vg_lite_hal_poke(VG_LITE_INTR_ENABLE, 0xFFFFFFFF);
    }
    else
    {
        while (!VG_LITE_KERNEL_IS_GPU_IDLE() &&
            (reset_timer < reset_timer_limit)   // Force shutdown if timeout.
            ) {
            vg_lite_hal_delay(reset_timer);
            reset_timer *= 2;
        }
        /* Set idle speed. */
        value.data = vg_lite_hal_peek(VG_LITE_HW_CLOCK_CONTROL);
        value.control.scale = 1;
        value.control.scale_load = 1;
        vg_lite_hal_poke(VG_LITE_HW_CLOCK_CONTROL, value.data);
        value.control.scale_load = 0;
        vg_lite_hal_poke(VG_LITE_HW_CLOCK_CONTROL, value.data);
        /* Enable clock gating. */
        value.control.clock_gate = 1;
        vg_lite_hal_poke(VG_LITE_HW_CLOCK_CONTROL, value.data);
    }
}

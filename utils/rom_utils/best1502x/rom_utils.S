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

    .syntax    unified
    .align    2

    #include "rom_utils_v1.S"

    .macro    def_RomFunc   name
    .section .text.\name , "ax", %progbits
    .thumb
    .thumb_func
    .align    2
    .globl    \name
    .type    \name, %function
\name\():
#if 0
    push {r0-r3, r10, lr}
    bl hal_get_chip_metal_id
    cmp r0, 0
    bgt .L0_\name
    pop {r0-r3, r10, lr}
    ldr pc, =\name\()_v1
.L0_\name\():
    pop {r0-r3, r10, lr}
#endif
    ldr pc, =\name\()_v1
    .pool
    .size    \name, . - \name
    .endm

    def_RomFunc rom1_entry
    def_RomFunc set__impure_ptr
    def_RomFunc sbc_bits_for_mono_alloc
    def_RomFunc sbc_bits_for_stereo_alloc
    def_RomFunc sbc_header_process
    def_RomFunc sbc_factors_process
    def_RomFunc sbc_sbsamples_make
    def_RomFunc sbc_factors_calc
    def_RomFunc sbc_header_make
    def_RomFunc sbc_frame_mute
    def_RomFunc sbc_samples_quant
    def_RomFunc sbc_frame_length_get
    def_RomFunc sbc_decoder_init
    def_RomFunc sbc_4band_synth_filter
    def_RomFunc sbc_8band_synth_filter
    def_RomFunc sbc_frames_parser
    def_RomFunc sbc_frames_decode_do
    def_RomFunc sbc_frames_decode
    def_RomFunc sbc_frames_decode_select_channel
    def_RomFunc sbc_frames_decode_out_sbsamples
    def_RomFunc sbc_encoder_init
    def_RomFunc sbc_frames_encode
    def_RomFunc sbc_frames_encode_with_sbsamples
    .end


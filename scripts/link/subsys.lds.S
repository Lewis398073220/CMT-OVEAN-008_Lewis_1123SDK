/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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

#include "plat_addr_map.h"

#define HEAP_SECTION_SIZE			0x400
#if defined(__NuttX__)
#include "nuttx/config.h"
#define NUTTX_HEAP_SECTION_SIZE		0x10000
#define HEAP_SECTION_SIZE			0x10
#define STACK_SECTION_SIZE			CONFIG_IDLETHREAD_STACKSIZE
#else
#define STACK_SECTION_SIZE			0x1100
#endif

#ifndef MAIN_RAM_USE_TCM
#ifndef NO_OVERLAY
#error "Overlay is NOT supported unless MAIN_RAM_USE_TCM defined"
#endif
#ifndef RAMXRET_SIZE
#define RAMXRET_SIZE				RAMRET_SIZE
#endif
#ifndef RAMX_SIZE
#define RAMX_SIZE					RAM_SIZE
#endif
#endif

#ifndef FLASH_BASE
/* Set a fake flash address as the load address */
#define FLASH_BASE					0xF0000000
#endif
#ifndef FLASH_SIZE
#define FLASH_SIZE					0x10000000
#endif

#if !defined(OTA_BOOT_SIZE) && defined(OTA_CODE_OFFSET)
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_CODE_OFFSET)
#elif defined(OTA_BOOT_OFFSET)
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_BOOT_OFFSET)
#else
#define FLASH_REGION_BASE			(FLASH_BASE)
#endif

#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE			(FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif

#define LOAD_REGION					FLASH

#ifdef RAM_NC_BASE
#define REGION_VECTOR				RAM_NC
#else
#define REGION_VECTOR				RAM
#endif

/* Linker script to configure memory regions. */
MEMORY
{
#if defined(RAMRET_SIZE) && (RAMRET_SIZE > 0)
	RAM		(rwx)	: ORIGIN = RAMRET_BASE,			LENGTH = RAMRET_SIZE
	RAMX	(rx)	: ORIGIN = RAMXRET_BASE,		LENGTH = RAMXRET_SIZE
	BUFFER	(rw)	: ORIGIN = RAM_BASE,			LENGTH = RAM_SIZE
#else
	RAM		(rwx)	: ORIGIN = RAM_BASE,			LENGTH = RAM_SIZE
	RAMX	(rx)	: ORIGIN = RAMX_BASE,			LENGTH = RAMX_SIZE
#endif
#ifdef RAM_NC_BASE
	RAM_NC	(rw)	: ORIGIN = RAM_NC_BASE,			LENGTH = RAM_NC_SIZE
#endif
	FLASH 	(rx)	: ORIGIN = FLASH_REGION_BASE,	LENGTH = FLASH_REGION_SIZE
}

/* Library configurations */
#ifndef __NuttX__
GROUP(libgcc.a)
#endif

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __free_ram
 *   __buffer_start__
 *   __buffer_end__
 *   __overlay_text_exec_start__
 *   __overlay_data_start__
 */
ENTRY(Reset_Handler)

SECTIONS
{
	.boot_struct (ORIGIN(FLASH)) :
	{
		__flash_start = .;
		KEEP(*(.boot_struct))
		KEEP(*(.img_desc_subsys))
#ifdef MAIN_RAM_USE_TCM
		__code_seg_map_start = .;
		KEEP(*(.code_seg_map))
		__code_seg_map_end = .;
		__code_seg_map_size = ABSOLUTE(__code_seg_map_end - __code_seg_map_start);
#ifndef NO_OVERLAY
		KEEP(*(.code_overlay_map))
#endif
#endif
		. = ALIGN(4);
	} > FLASH

	.vector_table (ORIGIN(REGION_VECTOR)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
		__vector_table_end__ = .;
	} > REGION_VECTOR

#ifdef RAM_NC_BASE
	.sram_nc_bss (NOLOAD) : AT (ADDR(.sram_nc_bss))
	{
		__sram_nc_bss_start__ = .;
		*(.sram_nc_bss*)
		*(.sync_flags_sram*)
		. = ALIGN(4);
		__sram_nc_bss_end__ = .;
	} > RAM_NC

	__ram_nc_top = ORIGIN(RAM_NC) + LENGTH(RAM_NC);
	__free_ram_nc = __ram_nc_top - __sram_nc_bss_end__;
#endif

#ifdef MAIN_RAM_USE_TCM
	__sram_text_start__ = ALIGN(ORIGIN(RAMX), 32);
#else
#ifdef RAM_NC_BASE
	.dummy_sram_text_start (NOLOAD) :
	{
		. = 32;
	} > RAMX
	__sram_text_start__ = ALIGN(., 32);
#else
	__sram_text_start__ = ALIGN(RAM_TO_RAMX(__vector_table_end__), 32);
#endif
#endif
	__sram_text_data_start__ = RAMX_TO_RAM(__sram_text_start__);
	__sram_text_start_load__ = (LOADADDR(.boot_struct) + SIZEOF(.boot_struct));
	__sram_text_start_load_offset__ = __sram_text_start_load__ - __flash_start;

	.sram_text (__sram_text_start__) : AT (__sram_text_start_load__)
	{
		_stext = .;
		*(.reset_handler_subsys)
#ifdef NO_OVERLAY
		*(.text*)
#endif
		*(.boot_text_sram*)
		*(.boot_text_flash*)
		*(.sram_text*)
		*(.fast_text_sram*)
		*(.flash_text*)

#ifndef NOSTD
		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)
#endif
		. = ALIGN(4);
	} > RAMX

	.sram_end (.) : AT (LOADADDR(.sram_text) + SIZEOF(.sram_text))
	{
		. = ALIGN(32);
		__sram_text_end__ = .;
	} > RAMX

	__sram_text_seg_size__ = __sram_text_end__ - __sram_text_start__;

#ifdef NO_OVERLAY

	__text_start__ = __sram_text_end__;
	__text_load_start__ = LOADADDR(.sram_end) + SIZEOF(.sram_end);

#else

	__overlay_text_start__ = RAMX_TO_RAM(.);
	__overlay_text_exec_start__ = .;
	__overlay_load_text_start__ = LOADADDR(.sram_text) + SIZEOF(.sram_text);

	OVERLAY (__overlay_text_exec_start__) : NOCROSSREFS AT (__overlay_load_text_start__)
	{
		.overlay_text0
		{
			LONG(0);
#if defined(A2DP_DECODER_CROSS_CORE)
			*:a2dp_decoder_cc_sbc.o(.text*)
			*:sbc.o(.text*)
#endif
			*(.overlay_text0)
			. = ALIGN(4);
		}
		.overlay_text1
		{
			LONG(0);
#if defined(A2DP_DECODER_CROSS_CORE) && defined(A2DP_AAC_ON)
			*:a2dp_decoder_cc_aac.o(.text*)
#if defined(AAC_REDUCE_SIZE)
			*:fdkaac_decoder.o(.text*)
#else
			*:block.o(.text*.*CBlock_GetEscape*)
			*:tpdec_lib.o(.text*.*transportDec_GetBitstream*)
			*:tpdec_lib.o(.text*.*transportDec_GetAuBitsRemaining*)
			*:tpdec_latm.o(.text*.*CLatmDemux_GetValue*)
			*:tpdec_latm.o(.text*.*CLatmDemux_Read*)
			*:tpdec_lib.o(.text*.*transportDec_FillData*)
			*:tpdec_lib.o(.text*.*transportDec_ReadAccessUnit*)
			*:tpdec_lib.o(.text*.*transportDec_readHeader*)
			*:block.o(.text*.*ApplyTools*)
			*:aacdec_pns.o(.text*.*CPns_Apply*)
			*:aacdec_tns.o(.text*.*CTns_Apply*)
			*:aacdec_tns.o(.text*.*CTns_Reset*)
			*:aacdec_pns.o(.text*.*CPns_ResetData*)
			*:pulsedata.o(.text*.*CPulseData_Read*)
#if !defined(AAC_TEXT_PARTIAL_IN_FLASH)
			*:channel.o(.text*.*CChannelElement_Read*)
			*:tpdec_asc.o(.text*.*CProgramConfig_Reset*)
			*:tpdec_lib.o(.text*.*transportDec_CrcCheck*)
			*:block.o(.text*.*CBlock_FrequencyToTime*)
			*:block.o(.text*.*CBlock_ReadSectionData*)
			*:channel.o(.text*.*CChannelElement_Decode*)
			*:tpdec_lib.o(.text*.*transportDec_CrcEndReg*)
			*:aacdecoder.o(.text*.*CAacDecoder_DecodeFrame*)
			*:aac_mdct.o(.text*)
#endif

#if !defined(__SBC_FUNC_IN_ROM_VBEST2000__) && !defined(AAC_TEXT_PARTIAL_IN_FLASH)
			*:block.o(.text*.*CBlock_ReadSpectralData*)
			*:block.o(.text*.*CBlock_ScaleSpectralData*)
			*:aacdec_tns.o(.text*.*CTns_ReadDataPresentFlag*)
			*:tpdec_lib.o(.text*.*transportDec_CrcStartReg*)
			*:block.o(.text*.*CBlock_ReadScaleFactorData*)
			*:tpdec_lib.o(.text*.*transportDec_EndAccessUnit*)
			*:tpdec_latm.o(.text*.*CLatmDemux_GetNrOfSubFrames*)
			*:tpdec_lib.o(.text*.*transportDec_GetAuBitsTotal*)
			*:tpdec_asc.o(.text*.*CProgramConfig_LookupElement*)
			*:tpdec_latm.o(.text*.*CLatmDemux_GetFrameLengthInBits*)
			*:tpdec_latm.o(.text*.*CLatmDemux_ReadPayloadLengthInfo*)
			*:block.o(.text*.*CBlock_InverseQuantizeSpectralData*)
			*:channelinfo.o(.text*.*IcsRead*)
			*:aacdec_tns.o(.text*.*CTns_Read*)
			*:tpdec_lib.o(.text*.*transportDec_AdjustEndOfAccessUnit*)
			*:pulsedata.o(.text*.*CPulseData_Apply*)
			*:tpdec_asc.o(.text*.*AudioSpecificConfig_Init*)
			*:tpdec_asc.o(.text*.*AudioSpecificConfig_Parse*)
			*:tpdec_latm.o(.text*.*CLatmDemux_ReadStreamMuxConfig*)
			*:tpdec_asc.o(.text*.*getSampleRate*)
			*:tpdec_asc.o(.text*.*getAOT*)
			*:aacdecoder.o(.text*.*CAacDecoder_Init*)
			*:tpdec_asc.o(.text*.*CProgramConfig_Init*)
			*:aacdecoder_lib.o(.text*.*aacDecoder_ConfigCallback*)
			*:aacdecoder.o(.text*.*CAacDecoder_ExtPayloadParse*)
			*:stereo.o(.text*.*CJointStereo_Read*)
#endif			
			*:aac_com.o(.text*)
			*:aac_enc.o(.text*)
			*:aac_dec.o(.text*)
#endif
#endif
			*(.overlay_text1)
			. = ALIGN(4);
		}
		.overlay_text2
		{
			LONG(0);
#if defined(A2DP_DECODER_CROSS_CORE) && defined(A2DP_SCALABLE_ON)
			*:rc_decode.o(.text*)
			*:audio_int32.o(.text*)
			*:ssc_ifft.o(.text*)
			*:vector_quant.o(.text*)
			*:ssc_quant.o(.text*)
			*:ssc_pulsealloc.o(.text*)
			*:audio.o(.text*)
			*:set_sbm.o(.text*)
			*:rc_code.o(.text*)
			*:SBM_pitchBased.o(.text*)
			*:ssc_imdct.o(.text*)
			*:audio_decoder.o(.text*)
			*:SoundBooster_DRC_classic.o(.text*)
			*:ssc_cwrs.o(.text*)
			*:ssc_mode.o(.text*)
			*:ssc_math.o(.text*)
			*:ssc_lap.o(.text*)
			*:SeamlessBufferManagement.o(.text*)
			*:a2dp_decoder_cc_scalable.o(.text*)
#endif
			*(.overlay_text2)
			. = ALIGN(4);
		}
		.overlay_text3
		{
			LONG(0);
#if 0 // LC3 work together with call algo and need to overlay Call algo firstly
		    *:lc3_com.o(.text*)
		    *:lc3_dec.o(.text*)
		    *:lc3_enc.o(.text*)
#endif
			*(.overlay_text3)
			. = ALIGN(4);
		}
		.overlay_text4
		{
			LONG(0);
			*(.overlay_text4)
			. = ALIGN(4);
		}
		.overlay_text5
		{
			LONG(0);
			*(.overlay_text5)
			. = ALIGN(4);
		}
		.overlay_text6
		{
			LONG(0);
			*(.overlay_text6)
			. = ALIGN(4);
		}
		.overlay_text7
		{
			LONG(0);
			*(.overlay_text7)
			. = ALIGN(4);
		}
		.overlay_text8
		{
			LONG(0);
			*(.overlay_text8)
			. = ALIGN(4);
		}
		.overlay_text9
		{
			LONG(0);
			*(.overlay_text9)
			. = ALIGN(4);
		}
		.overlay_text10
		{
			LONG(0);
			*(.overlay_text10)
			. = ALIGN(4);
		}
		.overlay_text11
		{
			LONG(0);
			*(.overlay_text11)
			. = ALIGN(4);
		}
		.overlay_text_last
		{
			LONG(0);
			. = ALIGN(4);
		}
	} > RAMX

	__overlay_text0_load_offset__ = __load_start_overlay_text0 - __flash_start;
	__overlay_text1_load_offset__ = __load_start_overlay_text1 - __flash_start;
	__overlay_text2_load_offset__ = __load_start_overlay_text2 - __flash_start;
	__overlay_text3_load_offset__ = __load_start_overlay_text3 - __flash_start;
	__overlay_text4_load_offset__ = __load_start_overlay_text4 - __flash_start;
	__overlay_text5_load_offset__ = __load_start_overlay_text5 - __flash_start;
	__overlay_text6_load_offset__ = __load_start_overlay_text6 - __flash_start;
	__overlay_text7_load_offset__ = __load_start_overlay_text7 - __flash_start;

	__overlay_text0_size__ = __load_stop_overlay_text0 - __load_start_overlay_text0;
	__overlay_text1_size__ = __load_stop_overlay_text1 - __load_start_overlay_text1;
	__overlay_text2_size__ = __load_stop_overlay_text2 - __load_start_overlay_text2;
	__overlay_text3_size__ = __load_stop_overlay_text3 - __load_start_overlay_text3;
	__overlay_text4_size__ = __load_stop_overlay_text4 - __load_start_overlay_text4;
	__overlay_text5_size__ = __load_stop_overlay_text5 - __load_start_overlay_text5;
	__overlay_text6_size__ = __load_stop_overlay_text6 - __load_start_overlay_text6;
	__overlay_text7_size__ = __load_stop_overlay_text7 - __load_start_overlay_text7;

	.dummy_overlay_text_end (.) (NOLOAD) : AT (ADDR(.dummy_overlay_text_end))
	{
		__overlay_text_exec_end__ = .;
	} > RAMX

	__overlay_text_end__ = RAMX_TO_RAM(__overlay_text_exec_end__);
	__text_start__ = ALIGN(., 32);
	__text_load_start__ = __load_stop_overlay_text_last;

#endif /* !NO_OVERLAY */

	__text_data_start__ = RAMX_TO_RAM(__text_start__);
	__text_start_load_offset__ = __text_load_start__ -  __flash_start;

	.text (__text_start__) : AT (__text_load_start__)
	{
#ifndef NO_OVERLAY
		*(.text*)
#endif
#if (RAMCP_SIZE == 0)
		*(.cp_text*)
#endif
	} > RAMX

	.ARM.extab (.) : AT (LOADADDR(.text) + SIZEOF(.text))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > RAMX

	/* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
	.ARM.exidx (.) : AT (LOADADDR(.ARM.extab) + SIZEOF(.ARM.extab))
	{
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > RAMX

	.ramx_end (.) : AT (LOADADDR(.ARM.exidx) + SIZEOF(.ARM.exidx))
	{
		. = ALIGN(32);
		__ramx_end = .;
	} > RAMX

	/* For MPU config */
	__fast_sram_text_exec_start__ = __text_start__;
	__fast_sram_text_exec_end__ = __ramx_end;

#ifdef MAIN_RAM_USE_TCM
	__text_seg_size__ = __ramx_end - __text_start__;
	__sram_rodata_start__ = ALIGN(__vector_table_end__, 32);
	__sram_rodata_start_load_offset__ = __sram_rodata_start_load__ - __flash_start;
#else
	__sram_rodata_start__ = RAMX_TO_RAM(.);
#endif
	__sram_rodata_start_load__ = LOADADDR(.ramx_end) + SIZEOF(.ramx_end);

	.sram_rodata (__sram_rodata_start__) : AT (__sram_rodata_start_load__)
	{
		*(.boot_rodata_sram*)
		*(.sram_rodata*)
		*(.boot_rodata_flash*)
		*(.flash_rodata*)
#ifdef NO_OVERLAY
		*(.rodata*)
#endif

		. = ALIGN(4);

#ifndef NOSTD
		KEEP(*(.eh_frame*))
#endif
		*(.note.gnu.build-id)
		. = ALIGN(32);
	} > RAM

	__etext = .;
	_etext = .;

	.sram_data (.) : AT (LOADADDR(.sram_rodata) + SIZEOF(.sram_rodata))
	{
		__sram_data_start__ = .;
		_sdata = .;
#ifdef NO_OVERLAY
		*(.data*)
#endif
		*(.boot_data*)
		*(.sram_data*)
		. = ALIGN(4);

#ifndef NOSTD
		*(vtable)

		. = ALIGN(4);
		/* preinit data */
		_sinit = ABSOLUTE(.);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);
		_einit = ABSOLUTE(.);
		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);


		KEEP(*(.jcr*))
		. = ALIGN(4);
#endif

		__core_bridge_task_cmd_table_start = .;
		KEEP(*(.core_bridge_task_cmd_table))
		__core_bridge_task_cmd_table_end = .;
		. = ALIGN(4);
		__core_bridge_instant_cmd_table_start = .;
		KEEP(*(.core_bridge_instant_cmd_table))
		__core_bridge_instant_cmd_table_end = .;
                . = ALIGN(4);
		__m55_core_bridge_task_cmd_table_start = .;
		KEEP(*(.m55_core_bridge_task_cmd_table))
		__m55_core_bridge_task_cmd_table_end = .;
		. = ALIGN(4);
		__m55_core_bridge_instant_cmd_table_start = .;
		KEEP(*(.m55_core_bridge_instant_cmd_table))
		__m55_core_bridge_instant_cmd_table_end = .;
		/* All data end */
		. = ALIGN(4);
		__sram_data_end__ = .;
		_edata = .;
	} > RAM

#ifdef MAIN_RAM_USE_TCM
	__sram_data_seg_size__ = . - __sram_rodata_start__;
#endif

#ifdef NO_OVERLAY

	__data_seg_start_load__ = LOADADDR(.sram_data) + SIZEOF(.sram_data);

#else

	__overlay_data_start__ = .;
	__load_start_overlay_data_start = LOADADDR(.sram_data) + SIZEOF(.sram_data);

	OVERLAY (__overlay_data_start__) : NOCROSSREFS AT (__load_start_overlay_data_start)
	{
		.overlay_data0
		{
            LONG(0);
#if defined(A2DP_DECODER_CROSS_CORE)
			*:a2dp_decoder_cc_sbc.o(.data* .rodata* .bss*)
#endif
			. = ALIGN(4);
		}
		.overlay_data1
		{
			LONG(0);
#if defined(A2DP_DECODER_CROSS_CORE) && defined(A2DP_AAC_ON)
			*:a2dp_decoder_cc_aac.o(.data* .rodata* .bss*)
#if defined(AAC_REDUCE_SIZE)
			*:fdkaac_decoder.o(.data* .rodata*)
#else
			*:aac_com.o(.data .rodata*)
			*:aac_dec.o(.data .rodata*)
			*:aac_enc.o(.data .rodata*)
			*:block.o(.data* .rodata*)
			*:tpdec_lib.o(.data* .rodata*)
			*:tpdec_latm.o(.data* .rodata*)
			*:aacdec_pns.o(.data* .rodata*)
			*:aacdec_tns.o(.data* .rodata*)
			*:pulsedata.o(.data* .rodata*)
			*:channel.o(.data* .rodata*)
			*:tpdec_asc.o(.data* .rodata*)
			*:aacdecoder.o(.data* .rodata*)
			*:channelinfo.o(.data* .rodata*)
			*:aacdecoder_lib.o(.data* .rodata*)
			*:stereo.o(.data* .rodata*)
			*:fixpoint_math.o(.data* .rodata*)
			*:FDK_bitbuffer.o(.data* .rodata*)
#endif
#endif
			. = ALIGN(4);
		}
		.overlay_data2
		{
			LONG(0);
#if defined(A2DP_DECODER_CROSS_CORE) && defined(A2DP_SCALABLE_ON)
			*:rc_decode.o(.data* .rodata*)
			*:audio_int32.o(.data* .rodata*)
			*:ssc_ifft.o(.data* .rodata*)
			*:vector_quant.o(.data* .rodata*)
			*:ssc_quant.o(.data* .rodata*)
			*:ssc_pulsealloc.o(.data* .rodata*)
			*:audio.o(.data* .rodata*)
			*:set_sbm.o(.data* .rodata*)
			*:rc_code.o(.data* .rodata*)
			*:SBM_pitchBased.o(.data* .rodata*)
			*:ssc_imdct.o(.data* .rodata*)
			*:audio_decoder.o(.data* .rodata*)
			*:SoundBooster_DRC_classic.o(.data* .rodata*)
			*:ssc_cwrs.o(.data* .rodata*)
			*:ssc_mode.o(.data* .rodata*)
			*:ssc_math.o(.data* .rodata*)
			*:ssc_lap.o(.data* .rodata*)
			*:SeamlessBufferManagement.o(.data* .rodata*)
			*:a2dp_decoder_cc_scalable.o(.data* .rodata* .bss*)
#endif
			. = ALIGN(4);
		}
		.overlay_data3
		{
			LONG(0);
#if 0 // LC3 work together with call algo and need to overlay Call algo firstly
			*:lc3_com.o(.data* .rodata* .bss*)
			*:lc3_dec.o(.data* .rodata* .bss*)
			*:lc3_enc.o(.data* .rodata* .bss*)
#endif
			. = ALIGN(4);
		}
		.overlay_data4
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data5
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data6
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data7
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data8
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data9
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data10
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data11
		{
			LONG(0);
			. = ALIGN(4);
		}
		.overlay_data_last
		{
			LONG(0);
			. = ALIGN(4);
		}
	} > RAM

	__overlay_data0_load_offset__ = __load_start_overlay_data0 - __flash_start;
	__overlay_data1_load_offset__ = __load_start_overlay_data1 - __flash_start;
	__overlay_data2_load_offset__ = __load_start_overlay_data2 - __flash_start;
	__overlay_data3_load_offset__ = __load_start_overlay_data3 - __flash_start;
	__overlay_data4_load_offset__ = __load_start_overlay_data4 - __flash_start;
	__overlay_data5_load_offset__ = __load_start_overlay_data5 - __flash_start;
	__overlay_data6_load_offset__ = __load_start_overlay_data6 - __flash_start;
	__overlay_data7_load_offset__ = __load_start_overlay_data7 - __flash_start;

	__overlay_data0_size__ = __load_stop_overlay_data0 - __load_start_overlay_data0;
	__overlay_data1_size__ = __load_stop_overlay_data1 - __load_start_overlay_data1;
	__overlay_data2_size__ = __load_stop_overlay_data2 - __load_start_overlay_data2;
	__overlay_data3_size__ = __load_stop_overlay_data3 - __load_start_overlay_data3;
	__overlay_data4_size__ = __load_stop_overlay_data4 - __load_start_overlay_data4;
	__overlay_data5_size__ = __load_stop_overlay_data5 - __load_start_overlay_data5;
	__overlay_data6_size__ = __load_stop_overlay_data6 - __load_start_overlay_data6;
	__overlay_data7_size__ = __load_stop_overlay_data7 - __load_start_overlay_data7;

	.dummy_overlay_data_end (.) (NOLOAD) : AT (ADDR(.dummy_overlay_data_end))
	{
		__overlay_data_end__ = .;
	} > RAM

	__data_seg_start_load__ = __load_stop_overlay_data_last;

#endif /* !NO_OVERLAY */

	__data_start_load_offset__ = __data_seg_start_load__ - __flash_start;

	.data (.) : AT (__data_seg_start_load__)
	{
		__data_start__ = .;
#ifndef NO_OVERLAY
		*(.data*)
		*(.rodata*)
#endif
#if (RAMCP_SIZE == 0)
		*(.cp_data*)
#endif
		__data_end__ = .;
	}

	.build_info (.) : AT (LOADADDR(.data) + SIZEOF(.data))
	{
		KEEP(*(.build_info))
		. = ALIGN(4);
	} > RAM = 0x00000000

#ifdef MAIN_RAM_USE_TCM
	__data_seg_size__ = . - __data_start__;
#endif

	/* The following section be the last loaded section */
	.code_start_addr (LOADADDR(.build_info) + SIZEOF(.build_info)):
	{
		LONG(ABSOLUTE(__flash_start));
	} > FLASH

	__flash_end = .;
	__flash_image_size = __flash_end - __flash_start;

	.boot_bss (NOLOAD) :
	{
		. = ALIGN(4);
		__boot_bss_sram_start__ = .;
		*(.boot_bss*)
		. = ALIGN(4);
		__boot_bss_sram_end__ = .;
	} > RAM

	.bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		_sbss = .;
		*(.bss*)
#if (RAMCP_SIZE == 0)
		*(.cp_bss*)
#endif
		*(.reboot_param*)
		*(.boot_bss*)
		*(.sram_bss*)
		*(COMMON)
#if defined(CONFIG_SMP)
		. = ALIGN(64);
		__cp_stack_limit = .;
		. += 1024;	/* just used for system_cp_reset_handler's stack */
		__cp_stack_top = .;
#endif
		. = ALIGN(4);
		__bss_end__ = .;
		_ebss = .;
	} > RAM

#if defined(__NuttX__) && defined(CONFIG_USE_BES_BT_STACK)
	.nuttx_heap (.) (NOLOAD) :
	{
		_nuttx_heap_start = .;
		. = ALIGN(8);
		. += NUTTX_HEAP_SECTION_SIZE;
		. = ALIGN(8);
		_nuttx_heap_end = .;
	} > RAM
#endif
	.heap (.) (NOLOAD) :
	{
		. = ALIGN(8);
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		. += HEAP_SECTION_SIZE;
		. = ALIGN(8);
		__HeapLimit = .;
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (.) (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

#if defined(RAMRET_SIZE) && (RAMRET_SIZE > 0)
	.buffer (ORIGIN(BUFFER)) (NOLOAD) :
	{
		__buffer_start__ = .;
		*(.buffer_section)
		__buffer_end__ = LENGTH(BUFFER);
	} > BUFFER
#else
	__buffer_start__ = .;
	__buffer_end__ = __buffer_start__;
#endif

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);
	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	__free_ram = __StackLimit - __HeapLimit;

	__mem_pool_start__ = __HeapLimit;
	__mem_pool_end__ = __StackLimit;

#if defined(__NuttX__) && !defined(CONFIG_USE_BES_BT_STACK)
	_nuttx_heap_start = __mem_pool_start__;
	_nuttx_heap_end = __mem_pool_end__ - 4;
#endif

#ifdef MAIN_RAM_USE_TCM
	__ramx_top = ORIGIN(RAMX) + LENGTH(RAMX);
	__free_ramx = __ramx_top - __ramx_end;
#endif
	__free_flash = ORIGIN(FLASH) + LENGTH(FLASH) - __flash_end;
}

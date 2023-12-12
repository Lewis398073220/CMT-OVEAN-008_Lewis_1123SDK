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

#include "plat_addr_map.h"

#define HEAP_SECTION_SIZE			0x40
#define STACK_SECTION_SIZE			0x1000

#define CODE_MSG_OVERHEAD			8

#ifdef OTA_BOOT_TO_DUAL_CHIP
#define OTA_SIZE_FACTOR				2
#else
#define OTA_SIZE_FACTOR				1
#endif

/* TODO: Add FLASH_REGION_OFFSET if there are multiple levels of boot loaders */
#if !defined(OTA_BOOT_SIZE) && defined(OTA_CODE_OFFSET)
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_CODE_OFFSET)
#else
#define FLASH_REGION_BASE			(FLASH_BASE)
#endif

#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE			(FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif
#define FLASH_NC_REGION_BASE		FLASH_C_TO_NC(FLASH_REGION_BASE)
#define FLASHX_REGION_BASE			FLASH_TO_FLASHX(FLASH_REGION_BASE)

#if (FLASH_BASE == FLASHX_BASE)
#define FLASHX						FLASH
#endif

/* Linker script to configure memory regions. */
MEMORY
{
	ROM    (rx)  : ORIGIN = ROM_BASE,    LENGTH = ROM_SIZE	/* see plat_addr_map.h and common.mk */
	FLASH 		(r)		: ORIGIN = FLASH_REGION_BASE,		LENGTH = FLASH_REGION_SIZE
	FLASH_NC	(r)		: ORIGIN = FLASH_NC_REGION_BASE,	LENGTH = FLASH_REGION_SIZE
#ifndef FLASHX
	FLASHX		(rx)	: ORIGIN = FLASHX_REGION_BASE,		LENGTH = FLASH_REGION_SIZE
#endif
	RAM    (rwx) : ORIGIN = RAM_BASE,    LENGTH = RAM_SIZE
	RAMX   (rx)  : ORIGIN = RAMX_BASE,   LENGTH = RAM_SIZE
}

/* Library configurations */
GROUP(libgcc.a libc.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exec_struct_start
 *   __cust_cmd_init_tbl_start
 *   __cust_cmd_init_tbl_end
 *   __cust_cmd_hldr_tbl_start
 *   __cust_cmd_hldr_tbl_end
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
 */

ENTRY(Boot_Loader)

SECTIONS
{
	__export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE);

	.boot_struct (ORIGIN(FLASH)) :
	{
		__flash_start = .;
		KEEP(*(.boot_struct))
		. = ALIGN(4);
	} > FLASH

	. = FLASH_TO_FLASHX(.);

	.boot_text_flash (.) : AT (FLASHX_TO_FLASH(ADDR(.boot_text_flash)))
	{
		*(.boot_loader)
		*(.boot_text_flash*)
		*(.boot_rodata_flash*)
		*(.flash_text*)
		*(.flash_rodata*)
		*:main.o(.text* .rodata*)
		*:cmsis_nvic.o(.text.NVIC_DisableAllIRQs .rodata.NVIC_DisableAllIRQs)
		. = ALIGN(4);
	} > FLASHX

	. = FLASHX_TO_FLASH(.);

	.got_info (.) :
	{
		__got_info_start = .;
		__got_plt_start = .;
		*(.got.plt)
		__got_plt_end = .;
		__got_start = .;
		*(.got)
		__got_end = .;
		. = ALIGN(4);
	} > FLASH

	.vector_table (ORIGIN(RAM)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.reboot_param (.) (NOLOAD) :
	{
		KEEP(*(.reboot_param))
		. = REBOOT_PARAM_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.burn_buffer (.) (NOLOAD) :
	{
		/* The size of .burn_buffer should be greater than __rom_HeapLimit. In most cases 8K is enough. */
		KEEP(*(.burn_buffer))
		. = ALIGN(4);
	} > RAM

/*
	.code_dummy (.) (NOLOAD) :
	{
		. = CODE_MSG_OVERHEAD;
		. = ALIGN(4);
	} > RAM
*/

	__boot_sram_start_flash__ = LOADADDR(.got_info) + SIZEOF(.got_info);
#ifndef MAIN_RAM_USE_TCM
	__boot_sram_start__ = .;
#endif

	.text
#ifndef MAIN_RAM_USE_TCM
		(RAM_TO_RAMX(__boot_sram_start__))
#endif
		: AT (__boot_sram_start_flash__)
	{
		__boot_text_sram_start__ = .;
		_stext = __boot_text_sram_start__;
		*(.text*)
		. = ALIGN(4);
		__boot_text_sram_end__ = .;
	} > RAMX

#ifdef MAIN_RAM_USE_TCM
	__ramx_end__ = __boot_text_sram_end__;
	__ramx_top = ORIGIN(RAMX) + LENGTH(RAMX);
	__free_ramx = __ramx_top - __ramx_end__;
#endif
	__boot_text_sram_end_flash__ = __boot_sram_start_flash__ + SIZEOF(.text);
	__boot_data_sram_start_flash__ = __boot_text_sram_end_flash__;

	.rodata
#ifndef MAIN_RAM_USE_TCM
		(RAMX_TO_RAM(ADDR(.text) + SIZEOF(.text)))
#endif
		: AT (__boot_data_sram_start_flash__)
	{
		__boot_data_sram_start__ = .;

		*(.rodata*)
		*(.boot_rodata_sram*)

		. = ALIGN(4);
		__cust_cmd_init_tbl_start = .;
		KEEP(*(.cust_cmd_init_tbl))
		__cust_cmd_init_tbl_end = .;

		. = ALIGN(4);
		__cust_cmd_hldr_tbl_start = .;
		KEEP(*(.cust_cmd_hldr_tbl))
		__cust_cmd_hldr_tbl_end = .;

		*(.note.gnu.build-id)
		. = ALIGN(4);
	} > RAM

	__boot_sram_end__ = .;
	__boot_sram_end_flash__ = LOADADDR(.rodata) + SIZEOF(.rodata);
	__etext = __boot_sram_end_flash__;

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(.data*)

		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	. = FLASH_TO_FLASHX(LOADADDR(.data) + SIZEOF(.data));

	.ARM.extab (.) : AT (FLASHX_TO_FLASH(ADDR(.ARM.extab)))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASHX

	__exidx_start = .;
	/* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
	.ARM.exidx (.) : AT (FLASHX_TO_FLASH(ADDR(.ARM.exidx)))
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASHX
	__exidx_end = .;

	. = FLASHX_TO_FLASH(.);

	.build_info (LOADADDR(.data) + SIZEOF(.data)) : AT (LOADADDR(.data) + SIZEOF(.data))
	{
		KEEP(*(.build_info))
		. = ALIGN(4);
	} > FLASH = 0x00000000

	/* The following section be the last loaded section */
	.code_start_addr (.) :
	{
		__app_entry_address__ = .;
		LONG(APP_ENTRY_ADDRESS);
		__download_uart_bandrate__ = .;
		LONG(DOWNLOAD_UART_BANDRATE);
		LONG(BUILD_INFO_MAGIC);
		LONG(ABSOLUTE(__flash_start));
	} > FLASH

	__flash_end = .;

	.bss (ADDR(.data)+SIZEOF(.data)) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

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
	__boot_bss_sram_start__ = .;
	__boot_bss_sram_end__ = .;
	__sram_text_data_start_flash__ = .;
	__sram_text_end_flash__ = .;
	__sram_data_start_flash__ = .;
	__sram_text_data_end_flash__ = .;
	__sram_text_data_start__ = .;
	__sram_text_start__ = .;
	__sram_text_end__ = .;
	__sram_data_start__ = .;
	__sram_data_end__ = .;
	__sram_bss_start__ = .;
	__sram_bss_end__ = .;
	__fast_sram_text_data_start_flash__ = .;
	__fast_sram_text_data_end_flash__ = .;
	__fast_sram_text_data_start__ = .;

#ifdef OTA_BOOT_SIZE
#if (OTA_BOOT_SIZE > FLASH_REGION_SIZE)
#error "OTA_BOOT_SIZE should <= FLASH_REGION_SIZE"
#endif
#ifndef OTA_CODE_OFFSET
#error "OTA_CODE_OFFSET should be defined along with OTA_BOOT_SIZE"
#endif
#if defined(OTA_BOOT_OFFSET) && ((OTA_BOOT_OFFSET + OTA_BOOT_SIZE) * OTA_SIZE_FACTOR > OTA_CODE_OFFSET)
#error "(OTA_BOOT_OFFSET + OTA_BOOT_SIZE) * OTA_SIZE_FACTOR should <= OTA_CODE_OFFSET"
#elif ((OTA_BOOT_SIZE) * OTA_SIZE_FACTOR > OTA_CODE_OFFSET)
#error "(OTA_BOOT_SIZE) * OTA_SIZE_FACTOR should <= OTA_CODE_OFFSET"
#endif
#endif

#ifdef OTA_REMAP_OFFSET
#ifndef OTA_CODE_OFFSET
#error "OTA_CODE_OFFSET should be set along with OTA_REMAP_OFFSET"
#endif
#ifndef OTA_BOOT_SIZE
	/* This is not an OTA booting code, but a normal working code */
	ASSERT((FLASH_BASE & 0x03FFFFFF) == 0, "bad FLASH_BASE")
#ifdef CHIP_BEST2300P
	ASSERT((FLASH_SIZE & (FLASH_SIZE - 1)) == 0, "bad FLASH_SIZE")
	ASSERT(__flash_end <= FLASH_BASE + OTA_REMAP_OFFSET, "flash code size too large to remap")
	ASSERT(__flash_start - FLASH_BASE >= FLASH_SIZE / (1 << (4 + 1)), "flash code cannot remap within 4 regions (max code size is half of flash size)")
	ASSERT((__flash_start - FLASH_BASE) % (FLASH_SIZE / (1 << (4 + 1))) == 0, "flash code start address not aligned with remap region boundary")
#else
	ASSERT(((OTA_REMAP_OFFSET) & (0x8000 - 1)) == 0, "OTA_REMAP_OFFSET not aligned with 32K sector boundary")
	ASSERT(__flash_end <= FLASH_BASE + (OTA_CODE_OFFSET + OTA_REMAP_OFFSET), "flash code size too large to remap")
#endif
#endif
#endif

#ifdef OTA_BOOT_INFO_OFFSET
#ifdef OTA_BOOT_SIZE
#if (OTA_CODE_OFFSET < (OTA_BOOT_INFO_OFFSET + (OTA_BOOT_INFO_SECTION_SIZE) * 2))
#error "OTA_CODE_OFFSET should >= (OTA_BOOT_INFO_OFFSET + (OTA_BOOT_INFO_SECTION_SIZE) * 2)"
#endif
	.ota_boot_info (FLASH_NC_BASE + OTA_BOOT_INFO_OFFSET) (NOLOAD) :
	{
		__ota_boot_info_start = .;
		*(.ota_boot_info)
		. = OTA_BOOT_INFO_SECTION_SIZE;
		__ota_boot_info_bak_start = .;
		*(.ota_boot_info_bak)
		. = (OTA_BOOT_INFO_SECTION_SIZE) * 2;
	} > FLASH_NC

#ifdef OTA_BOOT_OFFSET
	__ota_boot_code_end = FLASH_NC_BASE + (OTA_BOOT_OFFSET + OTA_BOOT_SIZE) * OTA_SIZE_FACTOR;
#else
	__ota_boot_code_end = FLASH_NC_BASE + (OTA_BOOT_SIZE) * OTA_SIZE_FACTOR;
#endif
	ASSERT(__ota_boot_info_start >= __ota_boot_code_end, ".ota_boot_info section overlaps with ota boot codes")
#endif
#endif
}

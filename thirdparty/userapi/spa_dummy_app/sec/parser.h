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

#ifndef __SEC_CRYPT_PARSER_H__
#define __SEC_CRYPT_PARSER_H__

#define NORMAL_MAGIC_CODE 0x55aabbcc

typedef struct{
/*
    should be 0x55aabbcc
*/
    unsigned int magic_code;
/*
    reserved = 0;
*/
    unsigned int start_offset;
/*
    bin length
*/
    unsigned int length;
/*
    text section offset , the absolute addr
*/
    unsigned int text_start_offset;
/*
    text section length
*/
    unsigned int text_length;
/*
    data section offset, the absolute addr
*/
    unsigned int data_start_offset;
/*
    data section length
*/
    unsigned int data_length;
/*
    the body crc val
*/
    unsigned int crc_val;
}encrypted_sec_header_t;

typedef struct{
    unsigned int flash_addr;
    unsigned int dst_text_sramx_addr_start;
    unsigned int dst_text_sramx_addr_end;
    unsigned int dst_data_sram_addr_start;
    unsigned int dst_data_sram_addr_end;
}tz_customer_load_ram_ramx_section_info_t;

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t __customer_load_ram_data_start__[];
extern uint32_t __customer_load_ram_data_end__[];
extern uint32_t __customer_load_ram_data_start_flash__[];
extern uint32_t __customer_load_ram_data_end_flash__[];

extern uint32_t __customer_load_sram_text_start__[];
extern uint32_t __customer_load_sram_text_end__[];
extern uint32_t __customer_load_sram_text_start_flash__[];
extern uint32_t __customer_load_sram_text_end_flash__[];

extern uint32_t inc_enc_bin_sec_start[];

#ifdef CMSE_RAM_RAMX_LEND_NSE
/* text flash */
extern uint32_t __cmse_ramx_lend_text_section_start_flash__[];
extern uint32_t __cmse_ramx_lend_text_section_end_flash__[];
/* text exec */
extern uint32_t __cmse_ramx_lend_text_section_ram_start__[];
extern uint32_t __cmse_ramx_lend_text_section_ram_end__[];

/* data flash */
extern uint32_t __cmse_ram_lend_data_section_start_flash__[];
extern uint32_t __cmse_ram_ramx_data_text_end_flash__[];
/* data exec*/
extern uint32_t __cmse_ram_lend_data_section_exec_start__[];
extern uint32_t __cmse_ram_lend_data_section_exec_end__[];
#endif


/*
    flash section for decrypted data
*/
extern uint32_t inc_dec_bin_sec_start[];
extern uint32_t inc_dec_bin_sec_end[];

#ifndef ARM_CMNS
int sec_decrypted_section_load_init(void *info);
int cmse_flash_load_into_sram_if(uint32_t flash_start,uint32_t flash_end,uint32_t sram_start , uint32_t sram_end);

int cmse_flash_load_into_sram(uint32_t flash_start,uint32_t flash_end,uint32_t sram_start , uint32_t sram_end,uint8_t * cache_buf,uint32_t cache_buf_len);

#else
//#define sec_decrypted_section_load_init sec_decrypted_section_load_init_cmse
int sec_decrypted_section_load_init(void *info);
#endif

#if defined(ARM_CMSE) || defined(ARM_CMNS)
int sec_decrypted_section_load_init_cmse(void *info);
#endif

#ifdef __cplusplus
}
#endif

#endif


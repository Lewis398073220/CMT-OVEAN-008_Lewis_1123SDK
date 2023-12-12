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

#include "parser.h"

#include "sec_mem.h"
#include "bin_aes_crypt.h"

#include "cmsis.h"
#include "pmu.h"

#include "norflash_api.h"

#include "hal_location.h"

#include "app_flash_api.h"
#include "bin_aes_crypt.h"

#define PARSER_MAX_SIZE (4*1024)

#if (AES_CBC_SRC_BUF_MAX_SIZE < PARSER_MAX_SIZE)
#error "PARSER_MAX_SIZE should > PARSER_MAX_SIZE"
#endif

typedef enum{
    LOAD_SRAM_SECTION_TYPE_TEXT,
    LOAD_SRAM_SECTION_TYPE_DATA,
    LOAD_SRAM_SECTION_TYPE_TEXT_DATA,
}load_sram_section_type_e;

static unsigned char * crypt_cache_ptr =NULL;
static unsigned int crypt_cache_len = 0;

#ifdef ARM_CMSE
static unsigned char * cmse_crypt_in_file_cache_ptr =NULL;
static unsigned int cmse_crypt_in_file_cache_len = 0;
encrypted_sec_header_t sec_header;
#endif

#ifdef SPA_AUDIO_SEC_TEST

#if (PARSER_MAX_SIZE > (4*1024))
#error "flash sector operation should = 4KB"
#endif

#define SRAM_TEST_MAX_SIZE  (50*1024)

static uint8_t sram_code_text_buf[SRAM_TEST_MAX_SIZE];
static uint16_t sram_codec_text_size = 0;
static uint8_t sram_code_data_buf[SRAM_TEST_MAX_SIZE];
static uint16_t sram_codec_data_size = 0;
static uint8_t sram_bin_text_buf[SRAM_TEST_MAX_SIZE];
static uint16_t sram_bin_text_size = 0;
static uint8_t sram_bin_data_buf[SRAM_TEST_MAX_SIZE];
static uint16_t sram_bin_data_size = 0;

static void sram_buf_init()
{
    memset(sram_code_text_buf,0,SRAM_TEST_MAX_SIZE);
    memset(sram_code_data_buf,0,SRAM_TEST_MAX_SIZE);
    memset(sram_bin_text_buf,0,SRAM_TEST_MAX_SIZE);
    memset(sram_bin_data_buf,0,SRAM_TEST_MAX_SIZE);
}
static void sram_buf_check_write(uint8_t type,uint8_t *buf,uint32_t len)
{
    uint8_t * cache_ram_p = NULL;

    if(type == 0){
        cache_ram_p = sram_code_text_buf;
        sram_codec_text_size = len;
    }else if(type == 1){
        cache_ram_p = sram_code_data_buf;
        sram_codec_data_size = len;
    }else if (type == 2){
        cache_ram_p = sram_bin_text_buf;
        sram_bin_text_size = len;
    }else if(type == 3){
        cache_ram_p = sram_bin_data_buf;
        sram_bin_data_size = len;
    }else{
        ASSERT(0,"exception type = %d",type);
    }

    if(len > SRAM_TEST_MAX_SIZE){
        ASSERT(0,"exception type = %d size = %d",type,len);
    }

    if(cache_ram_p){
        memcpy(cache_ram_p,buf,len);
    }
}

static void sram_buf_check_dump(char * str,uint8_t * buf,uint32_t len)
{
    TRACE(0,"%s %s",__func__,str);
    int remain = len / 16;
    uint32_t i = 0;
    for(i = 0;i<remain;i++){
        DUMP8("%02x ",&buf[i*16],16);
        hal_trace_flush_buffer();
    }
    remain = len % 16;
    DUMP8("%02x ",&buf[i*16],remain);
}

static void sram_buf_check_handler(void)
{
    sram_buf_init();

    uint8_t * start = (uint8_t *)inc_enc_bin_sec_start;

    encrypted_sec_header_t *header =  (encrypted_sec_header_t *)(inc_enc_bin_sec_start);

    sram_buf_check_write(0,(uint8_t*)__customer_load_sram_text_start_flash__,(__customer_load_sram_text_end_flash__-__customer_load_sram_text_start_flash__) * sizeof(int));
    sram_buf_check_write(1,(uint8_t*)__customer_load_ram_data_start_flash__,(__customer_load_ram_data_end_flash__-__customer_load_ram_data_start_flash__) * sizeof(int));

    sram_buf_check_write(2,(uint8_t*)(start + header->text_start_offset),header->text_length);
    sram_buf_check_write(3,(uint8_t*)(start + header->data_start_offset),header->data_length);

    if(memcmp(sram_code_text_buf,sram_bin_text_buf,SRAM_TEST_MAX_SIZE)){
        TRACE(0,"text not match");
        sram_buf_check_dump("sram_code_text_buf",sram_code_text_buf,sram_codec_text_size);
        sram_buf_check_dump("sram_bin_text_buf",sram_bin_text_buf,sram_bin_text_size);
    }

    if(memcmp(sram_code_data_buf,sram_bin_data_buf,SRAM_TEST_MAX_SIZE)){
        TRACE(0,"data not match");
        sram_buf_check_dump("sram_code_data_buf",sram_code_data_buf,sram_codec_data_size);
        sram_buf_check_dump("sram_bin_data_buf",sram_bin_data_buf,sram_bin_data_size);
    }

    uint32_t text_s_addr = 0;
    uint32_t text_e_addr = 0;
    uint32_t data_s_addr = 0;
    uint32_t data_e_addr = 0;

    text_s_addr = (uint32_t)__customer_load_sram_text_start_flash__;
    text_e_addr = (uint32_t)__customer_load_sram_text_end_flash__;
    data_s_addr = (uint32_t)__customer_load_ram_data_start_flash__;
    data_e_addr = (uint32_t)__customer_load_ram_data_end_flash__;

    TRACE(0,"info addr  : %x %x %x %x",text_s_addr,text_e_addr,data_s_addr,data_e_addr);
    TRACE(0,"length 0x%x 0x%x",(__customer_load_sram_text_end_flash__-__customer_load_sram_text_start_flash__),(__customer_load_ram_data_end_flash__-__customer_load_ram_data_start_flash__));

    text_s_addr = (uint32_t)(start + header->text_start_offset);
    text_e_addr = (uint32_t)(start + header->text_start_offset + header->text_length);
    data_s_addr = (uint32_t)(start + header->data_start_offset);
    data_e_addr = (uint32_t)(start + header->data_start_offset + header->data_length);

    TRACE(0,"info addr  : %x %x %x %x",text_s_addr,text_e_addr,data_s_addr,data_e_addr);
    TRACE(0,"length 0x%x 0x%x",header->text_length,header->data_length);

    TRACE(0,"size = %d %d %d %d",sram_codec_text_size,sram_codec_data_size,sram_bin_text_size,sram_bin_data_size);
}

/*
    used to load normal bin in dedicated sram/ram text/data section
*/
static int sram_buf_check_section_load_init(uint32_t flash_addr ,
                                                    uint32_t dst_text_sramx_addr_start , uint32_t dst_text_sramx_addr_end,
                                                    uint32_t dst_data_sram_addr_start,uint32_t dst_data_sram_addr_end)
{
    flash_addr = (uint32_t)inc_enc_bin_sec_start;

    encrypted_sec_header_t *header =  (encrypted_sec_header_t *)(flash_addr);

    if(header->magic_code != NORMAL_MAGIC_CODE){
        TRACE(0,"%s magic_code = %x <- addr = %x",__func__,header->magic_code,flash_addr);
        DUMP8("0x%02x ",(uint8_t*)flash_addr,16);
        return -1;
    }

    uint8_t * start = (uint8_t *)flash_addr;
    start += header->start_offset + sizeof(encrypted_sec_header_t);

    uint32_t length = header->length ;
    uint32_t crc = header->crc_val;

    uint32_t cal_crc = crc32_c(0, (uint8_t*)start , length);

    if(cal_crc != crc){
        TRACE(0,"%s crc 0x%02x != 0x%02x info : 0x%02x 0x%02x",__func__,cal_crc,crc,(uint32_t)start,length);
        return -1;
    }
    start = (uint8_t *)addr;

    uint32_t text_s_addr = 0;
    uint32_t text_e_addr = 0;
    uint32_t data_s_addr = 0;
    uint32_t data_e_addr = 0;

    text_s_addr = (uint32_t)(start + header->text_start_offset);
    text_e_addr = (uint32_t)(start + header->text_start_offset + header->text_length);
    data_s_addr = (uint32_t)(start + header->data_start_offset);
    data_e_addr = (uint32_t)(start + header->data_start_offset + header->data_length);

    customer_load_sram_ram_text_data_init((uint32_t*)text_s_addr,(uint32_t*)text_e_addr,
        (uint32_t*)data_s_addr,(uint32_t*)data_e_addr);

    sram_buf_check_handler();

    return 0;
}

/*write decrypt to flash*/
#define FLASH_OP_ITERATE_SIZE   (4*1024)

static uint32_t decrypt_to_flash_offset = 0;
static uint8_t * decrypt_to_flash_cache_ptr = NULL;
static uint32_t decrypt_to_flash_cache_offset = 0;
static void app_decrypt_to_flash_env_reset(void)
{
    decrypt_to_flash_offset = 0;
    decrypt_to_flash_cache_ptr = NULL;
    decrypt_to_flash_cache_offset = 0;
}

static void app_register_decrypt_to_flash_user(void)
{
    TRACE(0,"%s",__func__);
    app_flash_register_module(NORFLASH_API_MODULE_ID_TEST1, norflash_api_get_dev_id_by_addr((uint32_t)inc_dec_bin_sec_start),(uint32_t)inc_dec_bin_sec_start, 
                            (uint32_t)inc_dec_bin_sec_end - (uint32_t)inc_dec_bin_sec_start, 0);

    sec_mem_buf_alloc(&decrypt_to_flash_cache_ptr,FLASH_OP_ITERATE_SIZE);
}

static void app_decrypt_to_flash_flush(void)
{
    if(decrypt_to_flash_cache_offset > 0){
//        TRACE(1,"size = %d addr = 0x%02x offset = 0x%02x op_addr = 0x%02x",decrypt_to_flash_cache_offset,(uint32_t)inc_dec_bin_sec_start,
//            decrypt_to_flash_offset,(uint32_t)((uint32_t)inc_dec_bin_sec_start + decrypt_to_flash_offset));

//        DUMP8("%02x ",decrypt_to_flash_cache_ptr,decrypt_to_flash_cache_offset);
#if 0
        app_flash_program(NORFLASH_API_MODULE_ID_TEST1,
                          (uint32_t)((uint32_t)inc_dec_bin_sec_start + decrypt_to_flash_offset),
                          decrypt_to_flash_cache_ptr,
                          decrypt_to_flash_cache_offset,
                          true);
#else
        uint32_t lock ;
        lock = int_lock_global();
        norflash_api_flush_all(); //Ensure that the flash is in an operational state
        pmu_flash_write_config();

        enum NORFLASH_API_RET_T ret = norflash_api_erase(NORFLASH_API_MODULE_ID_TEST1,(uint32_t)((uint32_t)inc_dec_bin_sec_start + decrypt_to_flash_offset),FLASH_OP_ITERATE_SIZE,false);
        ASSERT(ret == NORFLASH_API_OK,"%s: erase failed! ret = %d len = %d .",__FUNCTION__,ret,decrypt_to_flash_cache_offset);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_TEST1,(uint32_t)((uint32_t)inc_dec_bin_sec_start + decrypt_to_flash_offset),(uint8_t *)decrypt_to_flash_cache_ptr,decrypt_to_flash_cache_offset,false);
        ASSERT(ret == NORFLASH_API_OK,"%s: write failed! ret = %d len = %d .",__FUNCTION__,ret,decrypt_to_flash_cache_offset);

        pmu_flash_read_config();
        int_unlock_global(lock);

#endif
//        DUMP8("%02x ",(uint8_t*)((uint32_t)inc_dec_bin_sec_start + decrypt_to_flash_offset),32);

        decrypt_to_flash_offset += decrypt_to_flash_cache_offset;
    }

}

static void app_decrypt_to_flash_write(uint8_t *ptr, uint32_t len)
{
    uint32_t cache_already_have_len = decrypt_to_flash_cache_offset;
    uint32_t cache_remain_len = FLASH_OP_ITERATE_SIZE - decrypt_to_flash_cache_offset;
    uint32_t cache_in_total_len = cache_already_have_len + len;
    uint32_t buf_op_in_remain_len = len;
    uint32_t buf_op_in_op_len = 0;

    uint8_t * cache_op_ptr = decrypt_to_flash_cache_ptr + cache_already_have_len;

    if(cache_in_total_len >= FLASH_OP_ITERATE_SIZE){
        memcpy(cache_op_ptr,ptr,cache_remain_len);

        decrypt_to_flash_cache_offset = FLASH_OP_ITERATE_SIZE;

        buf_op_in_remain_len -= cache_remain_len;

        buf_op_in_op_len = cache_remain_len;

        app_decrypt_to_flash_flush();

        decrypt_to_flash_cache_offset = 0;
    }

    if(buf_op_in_remain_len){
        memcpy(cache_op_ptr , ptr + buf_op_in_op_len,buf_op_in_remain_len);
        decrypt_to_flash_cache_offset += buf_op_in_remain_len;
    }

}

#endif

static POSSIBLY_UNUSED int sec_decypt_section_handler(load_sram_section_type_e section,const char * infile,
                                        uint32_t infile_len,const char * dst_start,const char * dst_end)
{
    uint32_t decrypt_len = 0;

    uint32_t i = 0;
    uint32_t crypt_iter_op_in_len = 0;
    uint32_t crypt_iter_op_size = 0;
    uint32_t crypt_iter_op_remain = 0;

    unsigned char *outbuf;

    char * op_file_pos = (char *)infile;
    char * op_file_in_buf = NULL;

    outbuf = crypt_cache_ptr;

    if(outbuf == NULL){
        ASSERT(0,"%s buf error",__func__);
    }

    crypt_iter_op_size = crypt_cache_len;
    crypt_iter_op_remain = infile_len;

    customer_load_sram_ram_text_data_init_offset_reset();

    for(i = 0; i < infile_len; i+= crypt_iter_op_in_len){
        decrypt_len = 0;

        crypt_iter_op_in_len = crypt_iter_op_size;

        if(crypt_iter_op_remain < PARSER_MAX_SIZE){
            crypt_iter_op_in_len = crypt_iter_op_remain;
        }
#ifdef ARM_CMSE
        hal_norflash_read(HAL_FLASH_ID_0,(uint32_t)op_file_pos,cmse_crypt_in_file_cache_ptr,crypt_iter_op_in_len);
        op_file_in_buf = (char *)cmse_crypt_in_file_cache_ptr;
#else
        op_file_in_buf = op_file_pos;
#endif
        bin_aes_crypt_handler_if(MODE_DECRYPT,op_file_in_buf,crypt_iter_op_in_len,(char*)outbuf,&decrypt_len);
//        TRACE(1,"[%d ]in_len %d  remain %d iter_len %d decrypted %d \n",i,crypt_iter_op_in_len,crypt_iter_op_remain,PARSER_MAX_SIZE,decrypt_len);

//        DUMP8("%x ",op_file_pos,16);
//        DUMP8("%x ",outbuf,16);

        op_file_pos += crypt_iter_op_in_len;

#if 0
//#if !defined(ARM_CMNS)
        if(section == LOAD_SRAM_SECTION_TYPE_TEXT){
            customer_load_sram_ram_text_init_with_offset((uint32_t*)outbuf,(uint32_t*)(outbuf + decrypt_len),decrypt_len/sizeof(int));
        }else if(section == LOAD_SRAM_SECTION_TYPE_DATA){
            customer_load_sram_ram_data_init_with_offset((uint32_t*)outbuf,(uint32_t*)(outbuf + decrypt_len),decrypt_len/sizeof(int));
        }
#else
        customer_load_sram_ram_text_memcpy_with_offset((uint32_t*)outbuf,(uint32_t*)(outbuf + decrypt_len),decrypt_len/sizeof(int),(uint32_t*)dst_start,(uint32_t*)dst_end);
#endif
#ifdef SPA_AUDIO_SEC_TEST
        app_decrypt_to_flash_write(outbuf,decrypt_len);
#endif
        crypt_iter_op_remain -= crypt_iter_op_in_len;
    }


    return 0;
}

static POSSIBLY_UNUSED int sec_decrypted_section_load_init_handler(uint32_t flash_addr ,
                                                    uint32_t dst_text_sramx_addr_start , uint32_t dst_text_sramx_addr_end,
                                                    uint32_t dst_data_sram_addr_start,uint32_t dst_data_sram_addr_end)
{
    TRACE(0,"%s start",__func__);

    encrypted_sec_header_t *header =  (encrypted_sec_header_t *)(flash_addr);

#ifdef ARM_CMSE
    hal_norflash_read(HAL_FLASH_ID_0,(uint32_t)flash_addr,(uint8_t *)&sec_header,sizeof(encrypted_sec_header_t));
    header = &sec_header;
//    TRACE(1,"addr 0x%02x",(uint32_t)sys_build_info);
    DUMP8("%02x ",(uint8_t*)&sec_header,24);
#endif

    if(header->magic_code != NORMAL_MAGIC_CODE){
        TRACE(0,"%s magic_code = %x <- addr = %x",__func__,header->magic_code,flash_addr);
        DUMP8("0x%02x ",(uint8_t*)flash_addr,16);
        return -1;
    }

    uint8_t * start = (uint8_t *)flash_addr;
    start += header->start_offset + sizeof(encrypted_sec_header_t);

#if !defined(ARM_CMSE)
    uint32_t length = header->length ;
    uint32_t crc = header->crc_val;

    uint32_t cal_crc = crc32_c(0, (uint8_t*)start , length);

    if(cal_crc != crc){
        TRACE(0,"%s crc 0x%02x != 0x%02x info : 0x%02x 0x%02x",__func__,cal_crc,crc,(uint32_t)start,length);
        return -1;
    }
#else
    sec_mem_total_init();

    cmse_crypt_in_file_cache_len = PARSER_MAX_SIZE;

    sec_mem_buf_alloc(&cmse_crypt_in_file_cache_ptr,cmse_crypt_in_file_cache_len);

    uint32_t infile_len = header->length;
    uint32_t cal_crc = 0;

    uint32_t op_len = 0;
    uint32_t op_remain_len = infile_len;
    uint32_t op_offset = 0;

    for( ;op_offset < infile_len ; ){
        op_len = cmse_crypt_in_file_cache_len;

        if(op_len > op_remain_len){
            op_len = op_remain_len;
        }

        hal_norflash_read(HAL_FLASH_ID_0,(uint32_t)(start + op_offset),cmse_crypt_in_file_cache_ptr,op_len);

        cal_crc = crc32_c(cal_crc, (uint8_t*)cmse_crypt_in_file_cache_ptr , op_len);

#if 0
    {
        TRACE(1,"[start 0x%02x][op_off 0x%02x][in 0x%02x] [op_len 0x%02x ][remain 0x%02x] crc = 0x%02x",(uint32_t)start,op_offset,
                (uint32_t)(start+op_offset),op_len,op_remain_len,cal_crc);
        uint32_t block = cmse_crypt_in_file_cache_len / 16;
        uint32_t remain = cmse_crypt_in_file_cache_len %16;
        uint32_t i = 0;

        TRACE(1,"block = %d remain = %d",block,remain);

        for(i=0;i<block;i++){
            DUMP8("%02x ",cmse_crypt_in_file_cache_ptr + i*16,16);
        }
        DUMP8("%02x ",cmse_crypt_in_file_cache_ptr + i*16,remain);
    }
#endif

        op_remain_len -= op_len;
        op_offset += op_len;

    }

    if(cal_crc != header->crc_val){
        TRACE(0,"crc 0x%02x != 0x%02x info : 0x%02x 0x%02x",cal_crc,header->crc_val,(uint32_t)flash_addr,infile_len);
        return -1;
    }
#endif

#if defined(SPA_AUDIO_SEC)
    char *  demo_aes_cbc_key = "1F8199004CB7FBAF070C4864495399A0";
    char *  demo_aes_cbc_iv = "3FA8DC149F54D369F71E7386BD37EE50";

    uint32_t demo_aes_cbc_key_len = strlen(demo_aes_cbc_key);
    uint32_t demo_aes_cbc_iv_len = strlen(demo_aes_cbc_iv);

    start = (uint8_t *)flash_addr;

    sec_mem_total_init();

#ifdef SPA_AUDIO_SEC_TEST
    app_decrypt_to_flash_env_reset();
    app_register_decrypt_to_flash_user();
#endif

    crypt_cache_len = PARSER_MAX_SIZE;

    sec_mem_buf_alloc(&crypt_cache_ptr,crypt_cache_len);

#if defined(ARM_CMSE)
    cmse_crypt_in_file_cache_len = PARSER_MAX_SIZE;
    sec_mem_buf_alloc(&cmse_crypt_in_file_cache_ptr,cmse_crypt_in_file_cache_len);
#endif

    bin_aes_crypt_init_if(demo_aes_cbc_key,demo_aes_cbc_key_len,demo_aes_cbc_iv,demo_aes_cbc_iv_len);

    /* decrypt text section */
    TRACE(0,"start to decrypt text ");

    customer_load_sram_ram_text_data_init_offset_reset();

//    TRACE(1,"addr 0x%02x offset = 0x%02x 0x%02x length = 0x%02x",(uint32_t)start,(uint32_t)(header->text_start_offset),
//        (uint32_t)(start+header->text_start_offset),(uint32_t)(header->text_length));

//    DUMP8("%02x ",start,32);

    sec_decypt_section_handler(LOAD_SRAM_SECTION_TYPE_TEXT,(const char *)(start + header->text_start_offset),header->text_length,(const char *)dst_text_sramx_addr_start,(const char *)dst_text_sramx_addr_end);

    customer_load_sram_ram_text_data_init_offset_reset();

    /* decrypt data section */
    TRACE(1,"start to decrypt data section");
    sec_decypt_section_handler(LOAD_SRAM_SECTION_TYPE_DATA,(const char *)(start + header->data_start_offset),header->data_length,(const char *)dst_data_sram_addr_start,(const char *)dst_data_sram_addr_end);

#ifdef SPA_AUDIO_SEC_TEST
        app_decrypt_to_flash_flush();
#endif

    TRACE(1,"end of text data section decrypt");

    bin_aes_crypt_deinit_if();
#endif
    //    TRACE(0,"[%s] info : 0x%02x 0x%02x 0x%02x 0x%02x",__func__,header->text_start_offset,header->text_length,header->data_start_offset,header->data_length);

    TRACE(1,"%s success",__func__);

    return 0;
}

int sec_decrypted_section_load_init(void * in_info)
{
    if(in_info == NULL){
        ASSERT(0,"%s in para should not be NULL",__func__);
        return -1;
    }

    tz_customer_load_ram_ramx_section_info_t info = *(tz_customer_load_ram_ramx_section_info_t*)in_info;

    TRACE(0, "FUNC:%s flash_addr 0x%02x", __func__,info.flash_addr);
    TRACE(1,"LR : 0x%02x mem_addr = 0x%02x",(uint32_t)__builtin_return_address(0),(uint32_t)&in_info);

#if defined(SPA_AUDIO_SEC_TEST)
    return sram_buf_check_section_load_init(info.flash_addr,info.dst_text_sramx_addr_start,info.dst_text_sramx_addr_end,info.dst_data_sram_addr_start,info.dst_data_sram_addr_end);
#elif defined(SPA_AUDIO_SEC)
    return sec_decrypted_section_load_init_handler(info.flash_addr,info.dst_text_sramx_addr_start,info.dst_text_sramx_addr_end,info.dst_data_sram_addr_start,info.dst_data_sram_addr_end);
#else
    ASSERT(0,"%s addr = 0x%02x LR = %p",__func__,info.flash_addr,__builtin_return_address(0));
    return 0;
#endif

}


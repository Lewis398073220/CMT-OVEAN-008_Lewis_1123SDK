/**
 * @copyright Copyright (c) 2015-20223 BES Technic.
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
 */
#ifndef APP_BIS_TRANSPOND_BUFFER_H_
#define APP_BIS_TRANSPOND_BUFFER_H_


/*****************************header include********************************/


/*********************external function declaration*************************/


/************************private macro defination***************************/

/************************private type defination****************************/
typedef struct
{
    uint8_t   pack_num;
    uint16_t  pack_size;
    void      *list_info;
}  app_bis_tran_buff_list_t;

/**********************private function declaration*************************/

/************************private variable defination************************/

void app_bis_tran_buffer_init();

void app_bis_tran_buffer_deinit();

void app_bis_tran_buffer_free(void *ptr);

uint8_t *app_bis_tran_buffer_malloc(uint32_t size);

void app_bis_tran_buf_list_creat(app_bis_tran_buff_list_t *list);

void app_bis_tran_buf_list_destroy(    app_bis_tran_buff_list_t      *list);

uint8_t *app_bis_tran_buf_list_get_data_packet(app_bis_tran_buff_list_t *list, uint16_t *data_len);

uint8_t *app_bis_tran_buf_list_get_free_packet(app_bis_tran_buff_list_t *list);

void app_bis_tran_buf_list_push_data_packet(app_bis_tran_buff_list_t *list, uint8_t *data, uint16_t data_len);

void app_bis_tran_buf_list_push_free_packet(app_bis_tran_buff_list_t *list, uint8_t* data);
#endif //APP_BIS_TRANSPOND_BUFFER_H_
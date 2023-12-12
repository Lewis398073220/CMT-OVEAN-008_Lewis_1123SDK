/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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

#ifndef __CFIFO_H__
#define __CFIFO_H__

#ifdef __cplusplus
extern "C"{
#endif
/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************type defination******************************/
enum {
    FIFO_OK = 0,
    FIFO_ERR = 1,
};

typedef struct{
    uint8_t *base;
    uint32_t size;
    uint32_t write;
    uint32_t read;
} cfifo;
/****************************function declaration***************************/

void cfifo_init(cfifo *fifo, uint8_t *buffer, uint32_t buf_len);
uint32_t cfifo_put(cfifo *fifo, uint8_t *buffer, uint32_t buf_len);
uint32_t cfifo_pop(cfifo *fifo, uint8_t *buffer, uint32_t buf_len);
uint32_t cfifo_peek(cfifo *fifo, uint32_t len_want, uint8_t **buf1, uint32_t *len1, uint8_t **buf2, uint32_t *len2);
uint32_t cfifo_peek_to_buf(cfifo *fifo, uint8_t *buffer, uint32_t buf_len);
uint32_t cfifo_len(cfifo *fifo);
uint32_t cfifo_free_space(cfifo *fifo);

#ifdef __cplusplus
}
#endif

#endif /// __CFIFO_H__
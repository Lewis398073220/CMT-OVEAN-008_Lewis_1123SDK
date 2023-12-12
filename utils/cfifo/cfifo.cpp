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
#include <stdio.h>
#include <string.h>
#include "hal_trace.h"
#include "cfifo.h"

/************************private macro defination***************************/

/*********************private variable defination***************************/

/**********************private function declaration*************************/

void cfifo_init(cfifo *fifo, uint8_t *buffer, uint32_t buf_len)
{
    ASSERT(buf_len != 0, "%s buf len can't be 0", __func__);
    fifo->size = buf_len;
    fifo->base = buffer;
    fifo->write = 0;
    fifo->read = 0;
    TRACE(0, "%s size:%d base:%p", __func__, buf_len, buffer);
}

uint32_t cfifo_put(cfifo *fifo, uint8_t *buffer, uint32_t buf_len)
{
    if (buf_len > cfifo_free_space(fifo)) {
        return FIFO_ERR;
    }

    __sync_synchronize();
    uint32_t write = fifo->write % fifo->size;

    if (buffer != NULL)
    {
        uint32_t bytesToTheEnd = fifo->size - write;
        if (bytesToTheEnd >= buf_len)
        {
            memcpy(fifo->base + write, buffer, buf_len);
        }
        else
        {
            memcpy(fifo->base + write, buffer, bytesToTheEnd);
            memcpy(fifo->base, buffer + bytesToTheEnd, buf_len - bytesToTheEnd);
        }
    }

    __sync_synchronize();
    fifo->write += buf_len;

    return FIFO_OK;
}

uint32_t cfifo_pop(cfifo *fifo, uint8_t *buffer, uint32_t buf_len)
{
    if (buf_len > cfifo_len(fifo)) {
        return FIFO_ERR;
    }

    __sync_synchronize();
    uint32_t read = fifo->read % fifo->size;

    if (buffer != NULL)
    {
        uint32_t bytesToTheEnd = fifo->size - read;
        if (bytesToTheEnd >= buf_len)
        {
            memcpy(buffer, fifo->base + read, buf_len);
        }
        else
        {
            memcpy(buffer, fifo->base + read, bytesToTheEnd);
            memcpy(buffer + bytesToTheEnd, fifo->base, buf_len - bytesToTheEnd);
        }
    }

    __sync_synchronize();
    fifo->read += buf_len;

    return FIFO_OK;
}

uint32_t cfifo_peek(cfifo *fifo, uint32_t len_want, uint8_t **buf1, uint32_t *len1, uint8_t **buf2, uint32_t *len2)
{
    if (len_want > cfifo_len(fifo)) {
        return FIFO_ERR;
    }

    __sync_synchronize();
    uint32_t read = fifo->read % fifo->size;
    *buf1 = fifo->base + read;
    if (fifo->size - read >= len_want)
    {
        *len1 = len_want;
        *buf2 = NULL;
        *len2 = 0;
    }
    else
    {
        *len1 = fifo->size - read;
        *buf2 = fifo->base;
        *len2 = len_want - *len1;
    }

    return FIFO_OK;
}

uint32_t cfifo_peek_to_buf(cfifo *fifo, uint8_t *buffer, uint32_t buf_len)
{
    if (buf_len > cfifo_len(fifo)) {
        return FIFO_ERR;
    }

    __sync_synchronize();
    uint32_t read = fifo->read % fifo->size;
    uint32_t bytesToTheEnd = fifo->size - read;
    if (bytesToTheEnd >= buf_len)
    {
        memcpy(buffer, fifo->base + read, buf_len);
    }
    else
    {
        memcpy(buffer, fifo->base + read, bytesToTheEnd);
        memcpy(buffer + bytesToTheEnd, fifo->base, buf_len - bytesToTheEnd);
    }

    return FIFO_OK;
}

uint32_t cfifo_len(cfifo *fifo)
{
    return (fifo->write - fifo->read);
}

uint32_t cfifo_free_space(cfifo *fifo)
{
    return (fifo->size - cfifo_len(fifo));
}

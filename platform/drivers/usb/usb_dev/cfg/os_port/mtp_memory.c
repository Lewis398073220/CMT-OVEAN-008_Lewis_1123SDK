/***************************************************************************
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
 ***************************************************************************/
#ifdef RTOS
#include "stddef.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "heap_api.h"
#include "mtp_log.h"
#include "list.h"
#include "heap_api.h"

#define MTP_STORAGE_HEAP_BUFF_SIZE 1024*10
static multi_heap_handle_t g_mtp_heap_handle;
static uint32_t *g_mtp_heap_buff;//[MTP_STORAGE_HEAP_BUFF_SIZE];

#define MTP_STORAGE_LIST_NODE_BUFF_SIZE 1024*8
static multi_heap_handle_t g_mtp_list_node_handle;
static uint32_t *g_mtp_list_node_heap_buff;//[MTP_STORAGE_LIST_NODE_BUFF_SIZE];

#define MTP_STORAGE_DBENTRY_BUFF_SIZE 1024*5
static multi_heap_handle_t g_mtp_dbentry_handle;
static uint32_t *g_mtp_dbentry_heap_buff;//[MTP_STORAGE_DBENTRY_BUFF_SIZE];

void *mtp_storage_zalloc(size_t size)
{
    void *ptr = NULL;
    ptr = heap_malloc(g_mtp_heap_handle, size);
    if (ptr == NULL) {
        MTP_TR_ERROR(0, "%s: malloc memory error", __FUNCTION__);
        return NULL;
    }
    memset(ptr, 0x00, size);
    return ptr;
}

void mtp_storage_free(void *ptr)
{
    heap_free(g_mtp_heap_handle, ptr);
}

void *mtp_list_node_zalloc(size_t size)
{
    void *ptr = NULL;
    ptr = heap_malloc(g_mtp_list_node_handle, size);
    if (ptr == NULL) {
        MTP_TR_ERROR(0, "%s: malloc memory error", __FUNCTION__);
        return NULL;
    }
    memset(ptr, 0x00, size);
    return ptr;
}

void mtp_list_node_free(void *ptr)
{
    heap_free(g_mtp_list_node_handle, ptr);
}

void *mtp_databse_zalloc(size_t size)
{
    void *ptr = NULL;
    ptr = heap_malloc(g_mtp_dbentry_handle, size);
    if (ptr == NULL) {
        MTP_TR_ERROR(0, "%s: malloc memory err", __FUNCTION__);
        return NULL;
    }
    memset(ptr, 0x00, size);
    return ptr;
}

void mtp_databse_free(void *ptr)
{
    heap_free(g_mtp_dbentry_handle, ptr);
}

int32_t mtp_storage_heap_init()
{
    static uint32_t g_mtp_heap_init = 0;

    if (g_mtp_heap_init == 1) {
        return 0;
    }

    syspool_init();
    list_init();

    if( g_mtp_heap_buff ==NULL){
        syspool_get_buff((uint8_t **)&g_mtp_heap_buff, MTP_STORAGE_HEAP_BUFF_SIZE*sizeof(uint32_t));
        ASSERT(g_mtp_heap_buff !=NULL, "[%s]: malloc g_mtp_heap_buff failed", __func__);
    }
    g_mtp_heap_handle = heap_register((void*)g_mtp_heap_buff, MTP_STORAGE_HEAP_BUFF_SIZE*sizeof(uint32_t));//sizeof(g_mtp_heap_buff));
    if (g_mtp_heap_handle == NULL) {
        MTP_TR_ERROR(0, "%s: heap_register error", __FUNCTION__);
        return -1;
    }
    if( g_mtp_list_node_heap_buff ==NULL){
        syspool_get_buff((uint8_t **)&g_mtp_list_node_heap_buff, MTP_STORAGE_LIST_NODE_BUFF_SIZE*sizeof(uint32_t));
        ASSERT(g_mtp_list_node_heap_buff !=NULL, "[%s]: malloc g_mtp_list_node_heap_buff failed", __func__);
    }
    g_mtp_list_node_handle = heap_register((void*)g_mtp_list_node_heap_buff, MTP_STORAGE_LIST_NODE_BUFF_SIZE*sizeof(uint32_t));//sizeof(g_mtp_list_node_heap_buff));
    if (g_mtp_list_node_handle == NULL) {
        MTP_TR_ERROR(0, "%s: heap_register error", __FUNCTION__);
        return -1;
    }
    if( g_mtp_dbentry_heap_buff ==NULL){
        syspool_get_buff((uint8_t **)&g_mtp_dbentry_heap_buff, MTP_STORAGE_DBENTRY_BUFF_SIZE*sizeof(uint32_t));
        ASSERT(g_mtp_dbentry_heap_buff!=NULL, "[%s]: malloc g_mtp_dbentry_heap_buff failed", __func__);
    }
    g_mtp_dbentry_handle = heap_register((void*)g_mtp_dbentry_heap_buff, MTP_STORAGE_DBENTRY_BUFF_SIZE*sizeof(uint32_t));//sizeof(g_mtp_dbentry_heap_buff));
    if (g_mtp_dbentry_handle == NULL) {
        MTP_TR_ERROR(0, "%s: heap_register error", __FUNCTION__);
        return -1;
    }
    g_mtp_heap_init = 1;
    return 0;
}
#endif


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
#ifndef MTP_STORAGE_MEMORY_H
#define MTP_STORAGE_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

/** use dynamic memory to init first 
 *
 *  @param void
 *
 *  @returns
 *    0 success  -1 error
 */
int32_t mtp_storage_heap_init();


/** free dynamic memory
*
*  @param ptr is a point  from mtp_storage_zalloc returned
*
*  @returns
*/
void mtp_storage_free(void *ptr);


/** malloc dynamic memory and Initialize to 0
*
*  @param size: memory size
*
*  @returns
*    ptr success  NULL error

*/
void *mtp_storage_zalloc(size_t size);


/** free dynamic memory
*
*  @param ptr is a point  from mtp_list_node_zalloc returned
*
*  @returns
*/
void mtp_list_node_free(void *ptr);


/** malloc dynamic memory and Initialize to 0
*
*  @param size: memory size 
*
*  @returns
*    ptr success  NULL error

*/
void *mtp_list_node_zalloc(size_t size);

/** free dynamic memory
*
*  @param ptr is a point  from mtp_databse_zalloc returned
*
*  @returns
*/
void mtp_databse_free(void *ptr);


/** malloc dynamic memory and Initialize to 0
*
*  @param size: memory size 
*
*  @returns
*    ptr success  NULL error

*/
void *mtp_databse_zalloc(size_t size);

#define MTP_STORAGE_FREE(ptr)            mtp_storage_free(ptr)
#define MTP_STORAGE_FREE_FP              mtp_storage_free
#define MTP_STORAGE_ZALLOC(size)         mtp_storage_zalloc(size)
#define MTP_STORAGE_ZALLOC_FP            mtp_storage_zalloc

#define MTP_OBJ_ZALLOC(size)             mtp_databse_zalloc(size)
#define MTP_OBJ_FREE(ptr)                mtp_databse_free(ptr)
#define MTP_OBJ_FREE_FP                  mtp_databse_free

#define ZALLOC_LIST(size)                MTP_STORAGE_ZALLOC(size)
#define FREE_LIST(list)                  MTP_STORAGE_FREE(list)
#define ZALLOC_LIST_NODE(size)           mtp_list_node_zalloc(size)
#define FREE_LIST_NODE(node)             mtp_list_node_free(node)
#define ZALLOC_LIST_NODE_FP              mtp_list_node_zalloc
#define FREE_LIST_NODE_FP                mtp_list_node_free

#define MTP_STORAGE_HEAP_INIT()          mtp_storage_heap_init()

#ifdef __cplusplus
}
#endif

#endif

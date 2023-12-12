/**
 ****************************************************************************************
 *
 * @file ke_mem.h
 *
 * @brief API for the heap management module.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef _KE_MEM_H_
#define _KE_MEM_H_

#include "rwip_config.h"     // IP configuration
#include <stdint.h>          // standard integer
#include <stdbool.h>         // standard includes

#define TRACE_KE_MEM_USAGE_ENABLEDx
#if defined(__cplusplus)
extern "C" {
#endif

/**
 ****************************************************************************************
 * @defgroup MEM Memory
 * @ingroup KERNEL
 * @brief Heap management module.
 *
 * This module implements heap management functions that allow initializing heap,
 * allocating and freeing memory.
 *
 * @{
 ****************************************************************************************
 */

// forward declarations
struct mblock_free;

/**
 ****************************************************************************************
 * @brief Heap initialization.
 *
 * This function performs the following operations:
 * - sanity checks
 * - check memory allocated is at least large enough to hold two block descriptors to hold
 * start and end
 * - initialize the first and last descriptors
 * - save heap into kernel environment variable.
 *
 * @param[in]     type      Memory type.
 * @param[in|out] heap      Heap pointer
 * @param[in]     heap_size Size of the heap
 *
 *
 ****************************************************************************************
 */
void ke_mem_init(uint8_t type, uint8_t* heap, uint16_t heap_size);

/**
 ****************************************************************************************
 * @brief Allocation of a block of memory.
 *
 * Allocates a memory block whose size is size; if no memory is available return NULL
 *
 * @param[in] size Size of the memory area that need to be allocated.
 * @param[in] type Type of memory block
 *
 * @return A pointer to the allocated memory area.
 *
 ****************************************************************************************
 */
#ifdef TRACE_KE_MEM_USAGE_ENABLED
void *ke_malloc_generic(uint32_t size, uint8_t type, const char* func,const uint32_t line);
#define ke_malloc(size, type)  ke_malloc_generic((size), (type), __FUNCTION__, __LINE__)
#elif __RECORD_KE_MEM_USAGE_ENABLED__
void *ke_malloc_generic(uint32_t size, uint8_t type, const uint32_t func,const uint32_t line);
#define ke_malloc(size, type)  ke_malloc_generic((size), (type), (uint32_t)__builtin_return_address(0), __LINE__)
#else
void *ke_malloc(uint32_t size, uint8_t type);
#endif

/**
 ****************************************************************************************
 * @brief Check if it's possible to allocate a block of memory with a specific size.
 *
 * @param[in] size Size of the memory area that need to be allocated.
 * @param[in] type Type of memory block
 *
 * @return True if memory block can be allocated, False else.
 *
 ****************************************************************************************
 */
bool ke_check_malloc(uint32_t size, uint8_t type);

/**
 ****************************************************************************************
 * @brief Freeing of a block of memory.
 *
 * Free the memory area pointed by mem_ptr : mark the block as free and insert it in
 * the pool of free block.
 *
 * @param[in] mem_ptr Pointer to the memory area that need to be freed.
 *
 ****************************************************************************************
 */
#ifdef TRACE_KE_MEM_USAGE_ENABLED
void ke_free_generic(void *mem_ptr, const char* func,const uint32_t line);
#define ke_free(mem_ptr)  ke_free_generic((mem_ptr), __func__, __LINE__)
#else
void ke_free(void *mem_ptr);
#endif


/**
 ****************************************************************************************
 * @brief Check if current heap is empty or not (not used)
 *
 * @param[in] type Type of memory heap block
 *
 * @return true if heap not used, false else.
 ****************************************************************************************
 */
bool ke_mem_is_empty(uint8_t type);


/**
 ****************************************************************************************
 * @brief Check if current pointer is free or not
 *
 * @param[in] mem_ptr pointer to a memory block
 *
 * @return true if already free, false else.
 ****************************************************************************************
 */
bool ke_is_free(void* mem_ptr);

#if (KE_PROFILING)

/**
 ****************************************************************************************
 * @brief Retrieve memory usage of selected heap.
 *
 * @param[in] type Type of memory heap block
 *
 * @return current memory usage of current heap.
 ****************************************************************************************
 */
uint16_t ke_get_mem_usage(uint8_t type);


/**
 ****************************************************************************************
 * @brief Retrieve max memory usage of all heap.
 * This command also resets max measured value.
 *
 * @return max memory usage of all heap.
 ****************************************************************************************
 */
uint32_t ke_get_max_mem_usage(void);

#endif // (KE_PROFILING)

#if defined(__cplusplus)
}
#endif

///@} MEM
#endif // _KE_MEM_H_


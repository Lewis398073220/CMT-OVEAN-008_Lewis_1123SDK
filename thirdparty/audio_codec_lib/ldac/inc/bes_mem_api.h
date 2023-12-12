#ifndef _BES_MEM_API_H
#define _BES_MEM_API_H

#ifndef LDACBT_API
#define LDACBT_API
#endif /* LDACBT_API  */

#ifdef __cplusplus
extern "C" {
#endif

void ldac_cc_heap_init(void *begin_addr, uint32_t size);
void *ldac_cc_heap_malloc(uint32_t size);
void *bes_cc_heap_cmalloc(size_t __nmemb, uint32_t size);
void ldac_cc_heap_free(void *rmem);
LDACBT_API void ldac_cc_mem_init(void);

typedef void* (*private_malloc_cb)(size_t);
typedef void* (*private_calloc_cb)(size_t __nmemb, size_t __size);
typedef void (*private_free_cb)(void *__ptr);

void private_malloc_callback_register(private_malloc_cb cb);
void private_calloc_callback_register(private_calloc_cb cb);
void private_free_callback_register(private_free_cb cb);

/**************************************************
* note:
* ldac lib use private_malloc/private_calloc/private_free replace malloc/calloc/free
***************************************************/
void *private_malloc(size_t __size);
void *private_calloc(size_t __nmemb, size_t __size);
void private_free(void *__ptr);

#ifdef __cplusplus
}
#endif
#endif
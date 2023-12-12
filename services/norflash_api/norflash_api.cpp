#include "stdio.h"
#include "string.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#else
#include "hal_timer.h"
#endif
#include "pmu.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "hal_cache.h"

#if 0
#define NORFLASH_API_TRACE TRACE
#else
#define NORFLASH_API_TRACE(level,...)
#endif

#if defined(FLASH_API_SIMPLE)
#define NORFLASH_API_SYS_BUFFER_NUM         (0)
#endif
#if defined(FLASH_API_HIGHPERFORMANCE)
#define NORFLASH_API_SYS_BUFFER_NUM         (4)
#endif
#if defined(FLASH_API_NORMAL)
#define NORFLASH_API_SYS_BUFFER_NUM         (1)
#endif
#define NORFLASH_API_SYS_OPRA_NUM      ((NORFLASH_API_SYS_BUFFER_NUM + 1) * 3)

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
#define NORFLASH_API_DATA_BUFFER_NUM    (2)
#define NORFLASH_API_DATA_OPRA_NUM     ((NORFLASH_API_DATA_BUFFER_NUM + 1) * 3)
#endif

#define NORFLASH_API_REMAP_ID               HAL_NORFLASH_REMAP_ID_0
#define IS_ALIGN(v,size)                (((v/size)*size) == v)
#define IN_ONE_SECTOR(addr,len,sec_size) \
    (((addr & HAL_NORFLASH_ADDR_MASK) / sec_size) ==\
    (((addr & HAL_NORFLASH_ADDR_MASK) + len-1) / sec_size))

#define ADDR_IS_VALIDE(mod_base_addr, mod_len, start_addr, len) \
    ((start_addr & HAL_NORFLASH_ADDR_MASK) >= (mod_base_addr & HAL_NORFLASH_ADDR_MASK) && \
     (start_addr & HAL_NORFLASH_ADDR_MASK) + len <= (mod_base_addr & HAL_NORFLASH_ADDR_MASK) + mod_len)

typedef struct _opera_info
{
    enum NORFLASH_API_OPRATION_TYPE type;
    uint32_t addr;
    uint32_t len;
    uint32_t w_offs;
    uint32_t w_len;
    uint8_t *buff;
    bool lock;
    struct _opera_info *next;
}OPRA_INFO;

typedef struct
{
    bool is_registered;
    enum HAL_FLASH_ID_T dev_id;
    enum NORFLASH_API_MODULE_ID_T mod_id;
    uint32_t mod_base_addr;
    uint32_t mod_len;
    uint32_t mod_block_len;
    uint32_t mod_sector_len;
    uint32_t mod_page_len;
    uint32_t buff_len;
    NORFLASH_API_OPERA_CB cb_func;
    OPRA_INFO *opera_info;
    OPRA_INFO *cur_opera_info;
    enum NORFLASH_API_STATE state;
}MODULE_INFO;

typedef struct
{
    bool is_inited;
    MODULE_INFO mod_info[NORFLASH_API_MODULE_ID_COUNT];
    enum NORFLASH_API_MODULE_ID_T cur_mod_id;
    MODULE_INFO* cur_mod;
    NOFLASH_API_FLUSH_ALLOWED_CB allowed_cb[NORFLASH_API_USER_COUNTS];
}NORFLASH_API_INFO;

typedef struct
{
    bool is_used;
    OPRA_INFO opera_info;
}NORFLASH_OPERA;

typedef struct
{
    bool is_used;
    uint8_t buffer[NORFLASH_API_SECTOR_MAX_SIZE];
}NORFLASH_BUFFER;

typedef struct
{
    NORFLASH_API_HOOK_HANDLE hook_handle;
}NORFLASH_API_HOOK_T;


static NORFLASH_API_INFO norflash_api_info = {false,};
static NORFLASH_API_HOOK_T _norflash_api_hook[NORFLASH_API_HOOK_USER_QTY];

static NORFLASH_OPERA _sys_opera_list[NORFLASH_API_SYS_OPRA_NUM];
static NORFLASH_BUFFER _sys_buffer_list[NORFLASH_API_SYS_BUFFER_NUM];
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
static NORFLASH_OPERA _data_opera_list[NORFLASH_API_DATA_OPRA_NUM];
static NORFLASH_BUFFER _data_buffer_list[NORFLASH_API_DATA_BUFFER_NUM];
#endif

#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
static const osMutexAttr_t MutexAttr_norflash = {
    .name = "NORFLASH_API_MUTEX",
    .attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust,
    .cb_mem = NULL,
    .cb_size = 0U,
};
static osMutexId_t norflash_api_mutex_id = NULL;
#endif
static uint32_t _norflash_api_lock = 0;
static int suspend_number = 0;

static uint32_t flash_total_size[HAL_FLASH_ID_NUM];
static uint32_t flash_block_size[HAL_FLASH_ID_NUM];
static uint32_t flash_sector_size[HAL_FLASH_ID_NUM];
static uint32_t flash_page_size[HAL_FLASH_ID_NUM];

static int _norflash_api_exec_flush_hook(void);

static void* norflash_memset(void* s, uint8_t c, size_t n)
{
	uint32_t i;
	char *ss = (char*)s;

	for (i=0;i<n;i++) ss[i] = c;
	return s;
}

static void* norflash_memcpy(void* __dest, __const void* __src,
			    size_t __n)
{
	uint32_t i;
	char *d = (char *)__dest, *s = (char *)__src;
	for (i=0;i<__n;i++) d[i] = s[i];
	return __dest;
}

#ifdef FLASH_REMAP
//when remap enable, must keep function can't locate in the flash
#define PMU_FLASH_WRITE_CONFIG_FUNC()
#define PMU_FLASH_READ_CONFIG_FUNC()
#else
#define PMU_FLASH_WRITE_CONFIG_FUNC() pmu_flash_write_config()
#define PMU_FLASH_READ_CONFIG_FUNC()  pmu_flash_read_config()
#endif

static void _cache_invalid(enum HAL_FLASH_ID_T dev_id, uint32_t start_addr, uint32_t len)
{
    bool is_cache_addr = false;
    uint32_t mark = ~HAL_NORFLASH_ADDR_MASK;

    //TRACE(1, "%s: id = %d, start_addr = 0x%x, mark = 0x%x", __func__, dev_id, start_addr, mark);
    if(dev_id == HAL_FLASH_ID_0)
    {
        if((mark & start_addr) == (mark & FLASH_BASE))
        {
            //TRACE(1, "%s: is cache address, start_addr = 0x%x, len = 0x%x", __func__, start_addr, len);
            is_cache_addr = true;
        }
    }
#ifdef FLASH1_CTRL_BASE
    else if(dev_id == HAL_FLASH_ID_1)
    {
        if((mark & start_addr) == (mark & FLASH1_BASE))
        {
            //TRACE(1, "%s: is cache address, start_addr = 0x%x, len = 0x%x", __func__, start_addr, len);
            is_cache_addr = true;
        }
    }
#endif
    else
    {
        ASSERT(0, "%s: ERROR ID: %d", __func__, dev_id);
    }

    if(is_cache_addr)
    {
        hal_cache_invalidate(HAL_CACHE_ID_I_CACHE, start_addr, len);
        hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, start_addr, len);
    }
}

static void* _norflash_api_malloc(enum HAL_FLASH_ID_T dev_id, uint32_t size)
{
    uint32_t i;
    NORFLASH_OPERA *opera;
    NORFLASH_BUFFER *buff;
    uint32_t opera_num;
    uint32_t buff_num;

    if(dev_id == HAL_FLASH_ID_0)
    {
        opera = (NORFLASH_OPERA*)_sys_opera_list;
        buff = (NORFLASH_BUFFER*)_sys_buffer_list;
        opera_num = ARRAY_SIZE(_sys_opera_list);
        buff_num = ARRAY_SIZE(_sys_buffer_list);
    }
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    else if(dev_id == HAL_FLASH_ID_1)
    {
        opera = (NORFLASH_OPERA*)_data_opera_list;
        buff = (NORFLASH_BUFFER*)_data_buffer_list;
        opera_num = ARRAY_SIZE(_data_opera_list);
        buff_num = ARRAY_SIZE(_data_buffer_list);
    }
#endif
    else
    {
        ASSERT(0, "%s: dev_id(%d) ERROR!", __func__, dev_id);
    }

    if(size == sizeof(OPRA_INFO))
    {
        for(i = 0; i < opera_num; i++)
        {
            if(opera[i].is_used == false)
            {
                opera[i].is_used = true;
                return (void*)&opera[i].opera_info;
            }
        }
        return NULL;
    }
    else if(size == flash_sector_size[dev_id])
    {
        for(i = 0; i < buff_num; i++)
        {
            if(buff[i].is_used == false)
            {
                buff[i].is_used = true;
                return (void*)buff[i].buffer;
            }
        }
        return NULL;
    }
    else
    {
        ASSERT(0,"%s: size(0x%x) error!", __func__, size);
    }
}

static void _norflash_api_free(enum HAL_FLASH_ID_T dev_id, void *p)
{
    uint32_t i;
    NORFLASH_OPERA *opera;
    NORFLASH_BUFFER *buff;
    uint32_t opera_num;
    uint32_t buff_num;

    if(dev_id == HAL_FLASH_ID_0)
    {
        opera = (NORFLASH_OPERA*)_sys_opera_list;
        buff = (NORFLASH_BUFFER*)_sys_buffer_list;
        opera_num = ARRAY_SIZE(_sys_opera_list);
        buff_num = ARRAY_SIZE(_sys_buffer_list);
    }
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    else if(dev_id == HAL_FLASH_ID_1)
    {
        opera = (NORFLASH_OPERA*)_data_opera_list;
        buff = (NORFLASH_BUFFER*)_data_buffer_list;
        opera_num = ARRAY_SIZE(_data_opera_list);
        buff_num = ARRAY_SIZE(_data_buffer_list);
    }
#endif
    else
    {
        ASSERT(0, "%s: dev_id(%d) ERROR!", __func__, dev_id);
    }

    for(i = 0; i < opera_num; i++)
    {
        if((uint8_t*)&opera[i].opera_info == p)
        {
            opera[i].is_used = false;
            return;
        }
    }

    for(i = 0; i < buff_num; i++)
    {
        if(buff[i].buffer == p)
        {
            buff[i].is_used = false;
            return;
        }
    }

    ASSERT(0,"%s: p(%p) error!", __func__, p);
}

static uint32_t _norflash_api_opera_num(enum HAL_FLASH_ID_T dev_id)
{
    uint32_t opera_num = 0;

    if(dev_id == HAL_FLASH_ID_0)
    {
        opera_num = ARRAY_SIZE(_sys_opera_list);
    }
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    else if(dev_id == HAL_FLASH_ID_1)
    {
        opera_num = ARRAY_SIZE(_data_opera_list);
    }
#endif
    else
    {
        ASSERT(0, "%s: dev_id(%d) error!", __func__, dev_id);
    }

    return opera_num;
}

static uint32_t _norflash_api_buffer_num(enum HAL_FLASH_ID_T dev_id)
{
    uint32_t buff_num = 0;
    if(dev_id == HAL_FLASH_ID_0)
    {
        buff_num = ARRAY_SIZE(_sys_buffer_list);
    }
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    else if(dev_id == HAL_FLASH_ID_1)
    {
        buff_num = ARRAY_SIZE(_data_buffer_list);
    }
#endif
    else
    {
        ASSERT(0, "%s: dev_id(%d) error!", __func__, dev_id);
    }

    return buff_num;
}

static inline void _norflash_api_mutex_init(void)
{
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
    norflash_api_mutex_id = osMutexNew(&MutexAttr_norflash);
    ASSERT(norflash_api_mutex_id, "cannot create norflash api mutex");
#endif
}

static inline void _norflash_api_mutex_wait(void)
{
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
    if(norflash_api_mutex_id && !in_int_locked())
    {
        osMutexAcquire(norflash_api_mutex_id, osWaitForever);
    }
#endif
}

static inline void _norflash_api_mutex_release(void)
{
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
    if(norflash_api_mutex_id && !in_int_locked())
    {
        osMutexRelease(norflash_api_mutex_id);
    }
#endif
}

static inline void _norflash_api_int_lock(enum HAL_FLASH_ID_T dev_id)
{
    if(dev_id == HAL_FLASH_ID_0)
    {
        _norflash_api_lock = int_lock_global();
    }
}

static inline void _norflash_api_int_unlock(enum HAL_FLASH_ID_T dev_id)
{
    if(dev_id == HAL_FLASH_ID_0)
    {
        int_unlock_global(_norflash_api_lock);
    }
}

static MODULE_INFO* _get_module_info(enum NORFLASH_API_MODULE_ID_T mod_id)
{
    return &norflash_api_info.mod_info[mod_id];
}

static OPRA_INFO* _get_tail(MODULE_INFO *mod_info,bool is_remove)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *pre_node = NULL;
    OPRA_INFO *tmp;

    pre_node = mod_info->opera_info;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;
        if(tmp)
        {
            pre_node = opera_node;
        }
    }
    if(is_remove)
    {
        if(pre_node)
        {
            pre_node->next = NULL;
        }
    }
    if(opera_node)
    {
        opera_node->lock = true;
    }
    return opera_node;
}

static void _opera_del(MODULE_INFO *mod_info,OPRA_INFO *node)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *pre_node = NULL;
    OPRA_INFO *tmp;

    pre_node = mod_info->opera_info;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        if(opera_node == node)
        {
            if(mod_info->opera_info == opera_node)
            {
                mod_info->opera_info = NULL;
            }
            else
            {
                pre_node->next = NULL;
            }
            if(node->buff)
            {
                _norflash_api_free(mod_info->dev_id, node->buff);
            }
            _norflash_api_free(mod_info->dev_id, node);
            break;
        }
        tmp = opera_node->next;
        if(tmp)
        {
            pre_node = opera_node;
        }
    }
}

static uint32_t _get_ew_count(MODULE_INFO *mod_info)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *tmp;
    uint32_t count = 0;

    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        count ++;
        tmp = opera_node->next;
    }
    return count;
}

static uint32_t _get_w_count(MODULE_INFO *mod_info)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *tmp;
    uint32_t count = 0;

    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        if(opera_node->type == NORFLASH_API_WRITTING)
        {
            count ++;
        }
        tmp = opera_node->next;
    }
    return count;
}

static uint32_t _get_e_count(MODULE_INFO *mod_info)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *tmp;
    uint32_t count = 0;

    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        if(opera_node->type == NORFLASH_API_ERASING)
        {
            count ++;
        }
        tmp = opera_node->next;
    }
    return count;
}

static MODULE_INFO* _get_cur_mod(void)
{
    uint32_t i;
    MODULE_INFO *mod_info;
    uint32_t tmp_mod_id = NORFLASH_API_MODULE_ID_COUNT;

    if(norflash_api_info.cur_mod)
    {
        return norflash_api_info.cur_mod;
    }

    tmp_mod_id = norflash_api_info.cur_mod_id;
    for(i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++)
    {
        tmp_mod_id =  tmp_mod_id + 1 >= NORFLASH_API_MODULE_ID_COUNT ? 0 : tmp_mod_id + 1;
        mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)tmp_mod_id);
        if(mod_info->is_registered)
        {
            if(_get_ew_count(mod_info) > 0)
            {
                return mod_info;
            }
        }
    }
    return NULL;
}

static enum NORFLASH_API_MODULE_ID_T _get_mod_id(MODULE_INFO *mod_info)
{
    uint32_t i;
    enum NORFLASH_API_MODULE_ID_T mod_id = NORFLASH_API_MODULE_ID_COUNT;
    MODULE_INFO *tmp_mod_info;

    for(i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++)
    {
        tmp_mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
        if(tmp_mod_info == mod_info)
        {
            mod_id = (enum NORFLASH_API_MODULE_ID_T)i;
            break;
        }
    }
    return mod_id;
}

#ifdef FLASH_REMAP
static void _flash_remap_start(enum HAL_FLASH_ID_T id,uint32_t addr, uint32_t len)
{
    uint32_t remap_addr;
    uint32_t remap_len;
    uint32_t start_addr;
    enum HAL_NORFLASH_RET_T ret;

    start_addr = addr & HAL_NORFLASH_ADDR_MASK;
    remap_addr = OTA_CODE_OFFSET & HAL_NORFLASH_ADDR_MASK;
    remap_len = OTA_REMAP_OFFSET;

    // NORFLASH_API_TRACE(3,"%s: id = %d,addr = 0x%x,len = 0x%x.", __func__,id,addr,len);
    if(start_addr + len <= remap_addr ||
       start_addr >= remap_addr + 2*remap_len)
    {
        // NORFLASH_API_TRACE(3,"%s: Not in the remap area.",__func__);
        return;
    }

    if((start_addr < remap_addr &&
       start_addr + len > remap_addr)   ||
       (start_addr < remap_addr + remap_len &&
       start_addr + len > remap_addr + remap_len) ||
       ((start_addr  >= remap_addr + remap_len) &&
       (start_addr < remap_addr + 2*remap_len)))
    {
        ASSERT(0,"%s: Address ranges bad!start_addr = 0x%x, len=0x%x,remap_addr=0x%x,remap_len=0x%x",
            __func__, start_addr, len, remap_addr, remap_len);
    }

    if(!hal_norflash_get_remap_status(id, NORFLASH_API_REMAP_ID))
    {
        NORFLASH_API_TRACE(3,"%s: Unremap to enable remap.",__func__);
        ret = hal_norflash_enable_remap(id, NORFLASH_API_REMAP_ID);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to enable remap, ret = %d",
            __func__, ret);
    }
    else
    {
        NORFLASH_API_TRACE(3,"%s: Remaped to disable remap.",__func__);
        ret = hal_norflash_disable_remap(id, NORFLASH_API_REMAP_ID);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to disable remap, ret = %d",
            __func__, ret);
    }
}

static void _flash_remap_done(enum HAL_FLASH_ID_T id,uint32_t addr, uint32_t len)
{
    uint32_t remap_addr;
    uint32_t remap_len;
    uint32_t start_addr;
    enum HAL_NORFLASH_RET_T ret;

    start_addr = addr & HAL_NORFLASH_ADDR_MASK;
    remap_addr = OTA_CODE_OFFSET & HAL_NORFLASH_ADDR_MASK;
    remap_len = OTA_REMAP_OFFSET;

    // NORFLASH_API_TRACE(3, "%s: id = %d,addr = 0x%x,len = 0x%x.", __func__,id,addr,len);
    if(start_addr + len <= remap_addr ||
       start_addr >= remap_addr + remap_len)
    {
        // NORFLASH_API_TRACE(3,"%s: Not in the remap area.",__func__);
        return;
    }

    if(((start_addr  < remap_addr) &&
       (start_addr + len > remap_addr)) ||
       ((start_addr  < remap_addr + remap_len)  &&
       (start_addr + len > remap_addr + remap_len)) ||
       ((start_addr  >= remap_addr + remap_len) &&
       (start_addr < remap_addr + 2*remap_len)))
    {
        ASSERT(0,"%s: Address ranges bad!addr = 0x%x, len=0x%x,remap_addr=0x%x,remap_len=0x%x",
            __func__, start_addr, len, remap_addr, remap_len);
    }

    if(!hal_norflash_get_remap_status(id, NORFLASH_API_REMAP_ID))
    {
        NORFLASH_API_TRACE(3, "%s: Unremap to enable remap.",__func__);
        ret = hal_norflash_enable_remap(id, NORFLASH_API_REMAP_ID);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to enable remap, ret = %d",
            __func__, ret);
    }
    else
    {
        NORFLASH_API_TRACE(3, "%s: Remaped to disable remap.",__func__);
        ret = hal_norflash_disable_remap(id, NORFLASH_API_REMAP_ID);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to disable remap, ret = %d",
            __func__, ret);
    }
}

static void _flash_remap_config(enum HAL_FLASH_ID_T id)
{
    uint32_t remap_addr;
    uint32_t remap_len;
    uint32_t remap_offset;
    enum HAL_NORFLASH_RET_T ret;

    remap_addr = OTA_CODE_OFFSET;
    remap_len = OTA_REMAP_OFFSET;
    remap_offset = OTA_REMAP_OFFSET;

    if(!hal_norflash_get_remap_status(id, NORFLASH_API_REMAP_ID))
    {
        NORFLASH_API_TRACE(3,"%s: Unremap to enable remap.",__func__);
        ret = hal_norflash_config_remap(id, NORFLASH_API_REMAP_ID, remap_addr, remap_len, remap_offset);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to config remap(0x%x,0x%x,0x%x),0x%x ret = %d",
               __func__, remap_addr, remap_len, remap_offset,
               hal_norflash_get_flash_total_size(id),
               ret);
        NORFLASH_API_TRACE(3,"%s: remap(0x%x,0x%x,0x%x) done",
            __func__, remap_addr, remap_len, remap_offset);
    }
}

POSSIBLY_UNUSED
static uint32_t _get_real_addr_in_remap(enum HAL_FLASH_ID_T id, uint32_t addr, uint32_t len)
{
    uint32_t remap_addr;
    uint32_t remap_len;
    uint32_t start_addr;

    start_addr = addr & HAL_NORFLASH_ADDR_MASK;
    remap_addr = OTA_CODE_OFFSET & HAL_NORFLASH_ADDR_MASK;
    remap_len = OTA_REMAP_OFFSET;

    // NORFLASH_API_TRACE(3,"%s: id = %d,addr = 0x%x,len = 0x%x.", __func__,id,addr,len);
    if(start_addr + len <= remap_addr ||
       start_addr >= remap_addr + 2*remap_len)
    {
        // NORFLASH_API_TRACE(3,"%s: Not in the remap area.",__func__);
        return start_addr;
    }

    if(((start_addr  < remap_addr) &&
       (start_addr + len > remap_addr)) ||
       ((start_addr  < remap_addr + remap_len)  &&
       (start_addr + len > remap_addr + remap_len)) ||
       ((start_addr  >= remap_addr + remap_len) &&
       (start_addr < remap_addr + 2*remap_len)))
    {
        ASSERT(0,"%s: start_addr = 0x%x, len=0x%x,remap_addr=0x%x,remap_len=0x%x",
            __func__, start_addr, len, remap_addr, remap_len);
    }

    if(!hal_norflash_get_remap_status(id, NORFLASH_API_REMAP_ID))
    {
        NORFLASH_API_TRACE(3,"%s: Unremap, return B address: 0x%x.", __func__, start_addr + remap_len);
        return start_addr + remap_len;
    }
    else
    {
        NORFLASH_API_TRACE(3,"%s: Remaped, return A address: 0x%x.", __func__, start_addr);
        return start_addr;
    }
}

#define FLASH_REMAP_START _flash_remap_start
#define FLASH_REMAP_DONE _flash_remap_done
#define FLASH_REMAP_CONFIG _flash_remap_config
#define FLASH_REMAP_GET_REAL_ADDR _get_real_addr_in_remap
#else // FLASH_REMAP
#define FLASH_REMAP_START(...)
#define FLASH_REMAP_DONE(...)
#define FLASH_REMAP_CONFIG(...)
#endif // FLASH_REMAP

#ifdef FLASH_PROTECTION

#define FLASH_BP_NONE       0x0000
#define FLASH_BP_ALL        0x007C

#define FLASH_1KBYTE        1024

static FLASH_BP_MAP_T flash_bp_map[FLASH_BP_MAP_LEN_MAX];
static uint32_t flash_bp_numer = 0;
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
static FLASH_BP_MAP_T flash1_bp_map[FLASH_BP_MAP_LEN_MAX];
static uint32_t flash1_bp_numer = 0;
#endif
static int flash_chip_dual_enable[HAL_FLASH_ID_NUM];

static bool _flash_chip_dual_enable(enum HAL_FLASH_ID_T id)
{
    return flash_chip_dual_enable[id] == 0 ? false : true;
}

static void _bp_init_2m_128m(enum HAL_FLASH_ID_T id, uint32_t total_size)
{
    uint32_t prot_size, unprot_size;
    uint32_t ind = 0;
    uint32_t multi;
    uint32_t n,i;
    FLASH_BP_MAP_T *bp_map;

    multi = 0x1 << ((total_size/(0x800000)) % 0x2000000);
    if(_flash_chip_dual_enable(id))
    {
        multi = multi >> (total_size/0x2000000);
    }
    // NORFLASH_API_TRACE(1, "%s: total_size = 0x%x, mults = 0x%x", __func__, total_size, mults);
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
    }
    else
    {
        bp_map = (FLASH_BP_MAP_T*)flash1_bp_map;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
#endif
    // bp 0: none.
    bp_map[ind].bp = FLASH_BP_NONE;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = 0;
    ind ++;

    // bp 0: All.
    bp_map[ind].bp = FLASH_BP_ALL;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = total_size-1;
    ind ++;

    // CMP = 0
    // Upper
    i = 0;
    prot_size = 0;
    while(1)
    {
        n = (64<<i);
        prot_size = (n*multi*FLASH_1KBYTE);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = total_size - prot_size;
        bp_map[ind].end_addr =  total_size - 1;
        ind ++;
        i ++;
    }

    // Lower
    prot_size = 0;
    i = 0;
    while(1)
    {
        n = (64<<i);
        prot_size = (n*multi*FLASH_1KBYTE);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+9)<<2); // max bp = 0xe
        bp_map[ind].start_addr = 0x0;
        bp_map[ind].end_addr = prot_size - 1;
        ind ++;
        i ++;
    }

    // Top
    prot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        prot_size = (n*FLASH_1KBYTE);
        if(prot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | (((0x1<<4)|(i+1))<<2); // max bp = 0x14
        bp_map[ind].start_addr = total_size - prot_size;
        bp_map[ind].end_addr = total_size - 1;
        ind ++;
        i ++;
    };

    // Botton
    prot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        prot_size = (n*FLASH_1KBYTE);
        if(prot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | (((0x1<<4)|(i+9))<<2); // max bp = 0x1c
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr = prot_size - 1;
        ind ++;
        i ++;
    }

    // CMP = 1
    // Lower
    unprot_size = 0;
    i = 0;
    while(1)
    {
        n = (64<<i);
        unprot_size = (n*multi*FLASH_1KBYTE);
        if(unprot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr =  total_size - unprot_size - 1;
        ind ++;
        i ++;
    }

    // Upper
    unprot_size = 0;
    i = 0;
    while(1)
    {
        n = (64<<i);
        unprot_size = (n*multi*FLASH_1KBYTE);
        if(unprot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | ((i+9)<<2); // max bp = 0xe
        bp_map[ind].start_addr = unprot_size;
        bp_map[ind].end_addr = total_size - 1;
        ind ++;
        i ++;
    }

    // L-
    unprot_size = 0;
    i = 0;

    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        unprot_size = (n*FLASH_1KBYTE);
        if(unprot_size > 32*FLASH_1KBYTE)
        {
           break;
        }
        bp_map[ind].bp = 0x4000 | (((0x1<<4)|(i+1))<<2); // max bp = 0x1c
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr = total_size - unprot_size - 1;
        ind ++;
        i ++;
    }

    // U -
    unprot_size = total_size;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        unprot_size = (n*FLASH_1KBYTE);
        if(unprot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | (((0x1<<4)|(i+9))<<2); // max bp = 0x1c
        bp_map[ind].start_addr = unprot_size;
        bp_map[ind].end_addr = total_size - 1;
        ind ++;
        i ++;
    }

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        flash_bp_numer = ind;
    }
    else
    {
        flash1_bp_numer = ind;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    flash_bp_numer = ind;
#endif
}

// GD25LE255E BP without CMP.
static void _bp_init_256m(enum HAL_FLASH_ID_T id, uint32_t total_size)
{
    uint32_t prot_size;
    uint32_t ind = 0;
    uint32_t multi;
    uint32_t n,i;
    uint32_t base_shift;
    FLASH_BP_MAP_T *bp_map;
    //uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    base_shift = 0x2;

    multi = 0x1 << ((total_size/(0x800000)) % 2000000);

    // NORFLASH_API_TRACE(1, "%s: total_size = 0x%x, mults = 0x%x", __func__, total_size, mults);
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
    }
    else
    {
        bp_map = (FLASH_BP_MAP_T*)flash1_bp_map;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
#endif
    // bp 0: none.
    bp_map[ind].bp = FLASH_BP_NONE;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = 0;
    ind ++;

    // bp 0: All.
    bp_map[ind].bp = FLASH_BP_ALL;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = total_size-1;
    ind ++;

    // CMP = 0
    // Upper
    i = 0;
    prot_size = 0;
    while(1)
    {
       if(_flash_chip_dual_enable(id))
        {
            n = (base_shift<<i);
        }
        else
        {
            n = ((base_shift*2)<<i);
        }

        prot_size = (n*multi*FLASH_1KBYTE);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = total_size - prot_size;
        bp_map[ind].end_addr =  total_size - 1;
        ind ++;
        i ++;
    }

    // Lower
    prot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (base_shift<<i);
        }
        else
        {
            n = ((base_shift*2)<<i);
        }

        prot_size = (n*multi*FLASH_1KBYTE);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+9)<<2); // max bp = 0xe
        bp_map[ind].start_addr = 0x0;
        bp_map[ind].end_addr = prot_size - 1;
        ind ++;
        i ++;
    }

    //hal_norflash_get_id(id, flash_id, ARRAY_SIZE(flash_id));
    // GD25LE255 without CMP bit.
    //if(!(flash_id[0] == 0xC8 && flash_id[1] == 0x60 && flash_id[2] == 0x19))

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        flash_bp_numer = ind;
    }
    else
    {
        flash1_bp_numer = ind;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    flash_bp_numer = ind;
#endif
}

// GD25LQ255E BP with CMP
static void _bp_init_256m_with_bmp(enum HAL_FLASH_ID_T id, uint32_t total_size)
{
    uint32_t prot_size;
    uint32_t ind = 0;
    uint32_t multi;
    uint32_t n,i;
    uint32_t base_shift;
    FLASH_BP_MAP_T *bp_map;
    uint32_t unprot_size;

    base_shift = 0x10;
    multi = 0x1 << ((total_size/(0x800000)) % 2000000);

    // NORFLASH_API_TRACE(1, "%s: total_size = 0x%x, mults = 0x%x", __func__, total_size, mults);
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
    }
    else
    {
        bp_map = (FLASH_BP_MAP_T*)flash1_bp_map;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
#endif
    // bp 0: none.
    bp_map[ind].bp = FLASH_BP_NONE;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = 0;
    ind ++;

    // bp 0: All.
    bp_map[ind].bp = FLASH_BP_ALL;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = total_size-1;
    ind ++;

    // CMP = 0
    // Upper
    i = 0;
    prot_size = 0;
    while(1)
    {
       if(_flash_chip_dual_enable(id))
        {
            n = (base_shift<<i);
        }
        else
        {
            n = ((base_shift*2)<<i);
        }

        prot_size = (n*multi*FLASH_1KBYTE);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = total_size - prot_size;
        bp_map[ind].end_addr =  total_size - 1;
        ind ++;
        i ++;
    }

    // Lower
    prot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (base_shift<<i);
        }
        else
        {
            n = ((base_shift*2)<<i);
        }

        prot_size = (n*multi*FLASH_1KBYTE);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+9)<<2); // max bp = 0xe
        bp_map[ind].start_addr = 0x0;
        bp_map[ind].end_addr = prot_size - 1;
        ind ++;
        i ++;
    }

    // Top
    prot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        prot_size = (n*FLASH_1KBYTE);
        if(prot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | (((0x1<<4)|(i+1))<<2); // max bp = 0x14
        bp_map[ind].start_addr = total_size - prot_size;
        bp_map[ind].end_addr = total_size - 1;
        ind ++;
        i ++;
    };

    // Botton
    prot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        prot_size = (n*FLASH_1KBYTE);
        if(prot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | (((0x1<<4)|(i+9))<<2); // max bp = 0x1c
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr = prot_size - 1;
        ind ++;
        i ++;
    }

    // CMP = 1
    // Lower
    unprot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (base_shift<<i);
        }
        else
        {
            n = ((base_shift*2)<<i);
        }
        unprot_size = (n*multi*FLASH_1KBYTE);
        if(unprot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr =  total_size - unprot_size - 1;
        ind ++;
        i ++;
    }

    // Upper
    unprot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (base_shift<<i);
        }
        else
        {
            n = ((base_shift*2)<<i);
        }
        unprot_size = (n*multi*FLASH_1KBYTE);
        if(unprot_size*2 > total_size)
        {
            break;
        }

        bp_map[ind].bp = 0x4000 | ((i+9)<<2);

        bp_map[ind].start_addr = unprot_size;
        bp_map[ind].end_addr = total_size - 1;
        ind ++;
        i ++;
    }


    unprot_size = 0;
    i = 0;

    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (8<<i);
        }
        else
        {
            n = (4<<i);
        }

        // L-
        unprot_size = (n*FLASH_1KBYTE);
        if(unprot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | (((0x1<<4)|(i+1))<<2); // max bp = 0x1c
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr = total_size - unprot_size - 1;
        ind ++;
        i ++;
        }

        // U -
        unprot_size = total_size;
        i = 0;
        while(1)
        {
            if(_flash_chip_dual_enable(id))
            {
                n = (8<<i);
            }
            else
            {
                n = (4<<i);
            }

        unprot_size = (n*FLASH_1KBYTE);
        if(unprot_size > 32*FLASH_1KBYTE)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | (((0x1<<4)|(i+9))<<2); // max bp = 0x1c
        bp_map[ind].start_addr = unprot_size;
        bp_map[ind].end_addr = total_size - 1;
        ind ++;
        i ++;
    }

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        flash_bp_numer = ind;
    }
    else
    {
        flash1_bp_numer = ind;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    flash_bp_numer = ind;
#endif
}


static void _bp_init_gd25d20_40(enum HAL_FLASH_ID_T id, uint32_t total_size)
{
    uint32_t unprot_size;
    uint32_t ind = 0;
    uint32_t multi;
    uint32_t n,i;
    FLASH_BP_MAP_T *bp_map;

    multi = 0x1 << ((total_size/(0x800000)) % 2000000);

    // NORFLASH_API_TRACE(1, "%s: total_size = 0x%x, mults = 0x%x", __func__, total_size, mults);
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
    }
    else
    {
        bp_map = (FLASH_BP_MAP_T*)flash1_bp_map;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
#endif
    // bp 0: none.
    bp_map[ind].bp = FLASH_BP_NONE;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = 0;
    ind ++;

    // bp 0: All.
    bp_map[ind].bp = FLASH_BP_ALL;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = total_size-1;
    ind ++;

    unprot_size = 0;
    i = 0;
    while(1)
    {
        if(_flash_chip_dual_enable(id))
        {
            n = (16<<i);
        }
        else
        {
            n = (8<<i);
        }
        unprot_size = (n*multi*1024);
        if(unprot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = ((i+1)<<2);
        bp_map[ind].start_addr = 0;
        bp_map[ind].end_addr = total_size - unprot_size - 1;
        ind ++;
        i ++;
    }


#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        flash_bp_numer = ind;
    }
    else
    {
        flash1_bp_numer = ind;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    flash_bp_numer = ind;
#endif
}

static void _bp_init_xt25q08b(enum HAL_FLASH_ID_T id, uint32_t total_size)
{
    uint32_t prot_size;
    uint32_t ind = 0;
    uint32_t multi;
    uint32_t n,i;
    FLASH_BP_MAP_T *bp_map;

    multi = 0x1 << ((total_size/(0x800000)) % 2000000);

    // NORFLASH_API_TRACE(1, "%s: total_size = 0x%x, mults = 0x%x", __func__, total_size, mults);
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
    }
    else
    {
        bp_map = (FLASH_BP_MAP_T*)flash1_bp_map;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
#endif
    // bp 0: none.
    bp_map[ind].bp = FLASH_BP_NONE;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = 0;
    ind ++;

    // bp 0: All.
    bp_map[ind].bp = FLASH_BP_ALL;
    bp_map[ind].start_addr = 0;
    bp_map[ind].end_addr = total_size-1;
    ind ++;

    // CMP = 0
    // Upper
    i = 0;
    prot_size = 0;
    while(1)
    {
        n = (64<<i);
        prot_size = (n*multi*1024);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x0000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = total_size - prot_size;
        bp_map[ind].end_addr =  total_size - 1;
        ind ++;
        i ++;
    }

    // CMP = 1
    // Lower
    prot_size = 0;
    i = 0;
    while(1)
    {
        n = (64<<i);
        prot_size = (n*multi*1024);
        if(prot_size*2 > total_size)
        {
            break;
        }
        bp_map[ind].bp = 0x4000 | ((i+1)<<2); // max bp = 0x6
        bp_map[ind].start_addr = 0x0;
        bp_map[ind].end_addr = prot_size - 1;
        ind ++;
        i ++;
    }

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        flash_bp_numer = ind;
    }
    else
    {
        flash1_bp_numer = ind;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    flash_bp_numer = ind;
#endif
}

void norflash_api_protection_bp_init(enum HAL_FLASH_ID_T id, uint32_t total_size)
{
    bool protection_enable = false;
    uint8_t flash_id[3];
#ifdef FLASH_GD256M_SR_WHIT_CMP
    bool with_bmp = true;
#else
    bool with_bmp = false;
#endif

    if(_flash_chip_dual_enable(id))
    {
        if(total_size >= 0x20000 && total_size <= 0x2000000)
        {
            protection_enable = true;
            _bp_init_2m_128m(id, total_size);
        }
    }
    else
    {
        hal_norflash_get_id(id, flash_id, ARRAY_SIZE(flash_id));
        if(flash_id[0] == 0xC8 && flash_id[1] == 0x40 && (flash_id[2] == 0x12 || flash_id[2] == 0x13))
        {
            protection_enable = true;
            _bp_init_gd25d20_40(id, total_size);
        }
        if(flash_id[0] == 0x0B && flash_id[1] == 0x60 && flash_id[2] == 0x14)
        {
            protection_enable = true;
            _bp_init_xt25q08b(id, total_size);
        }
        else
        {
            if(total_size >= 0x10000 && total_size < 0x2000000)
            {
                protection_enable = true;
                _bp_init_2m_128m(id, total_size);
            }
            else if(total_size == 0x2000000)
            {
                protection_enable = true;
                if(with_bmp)
                {
                    _bp_init_256m_with_bmp(id, total_size);
                }
                else
                {
                    _bp_init_256m(id, total_size);
                }
            }
        }
    }

    ASSERT(protection_enable,"%s: Unkown protection BP defining! total_size = 0x%x", __func__, total_size);

}

#ifdef FLASH_PROTECTION_BOOT_SECTION_FIRST
static bool _with_boot_section(enum HAL_FLASH_ID_T id, uint32_t start_addr, uint32_t end_addr)
{
#ifdef OTA_CODE_OFFSET
    uint32_t boot_start;
    uint32_t boot_end;

    boot_start = 0x0;
    boot_end = OTA_CODE_OFFSET & HAL_NORFLASH_ADDR_MASK;

    if(id == HAL_FLASH_ID_0)
    {
        if(start_addr <= boot_start && end_addr >= boot_end)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
#endif
    {
        return true;
    }
}
#endif

static void _protection_part(enum HAL_FLASH_ID_T id, uint32_t start_addr, uint32_t size)
{
    enum HAL_NORFLASH_RET_T result;
    uint32_t i;
    uint32_t bp = FLASH_BP_ALL;
    uint32_t real_addr;
    uint32_t addr;
    uint32_t tmp_size;
    uint32_t max_size = 0;
    uint32_t bp_i = 0;
    FLASH_BP_MAP_T *bp_map;
    uint32_t bp_map_number;
#ifdef FLASH_REMAP
    real_addr = _get_real_addr_in_remap(id, start_addr, size);
    addr = (real_addr & HAL_NORFLASH_ADDR_MASK);
#else
    real_addr = start_addr;
    addr = (real_addr & HAL_NORFLASH_ADDR_MASK);
#endif

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
        bp_map_number = flash_bp_numer;
    }
    else
    {
        bp_map = (FLASH_BP_MAP_T*)flash1_bp_map;
        bp_map_number = flash1_bp_numer;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    bp_map = (FLASH_BP_MAP_T*)flash_bp_map;
    bp_map_number = flash_bp_numer;
#endif

    for(i = 0; i < bp_map_number; i++)
    {
        if((addr >= bp_map[i].end_addr
            || addr + size <= bp_map[i].start_addr)
#ifdef FLASH_PROTECTION_BOOT_SECTION_FIRST
            && _with_boot_section(id, bp_map[i].start_addr,bp_map[i].end_addr)
#endif
            )
        {
            tmp_size = bp_map[i].end_addr - bp_map[i].start_addr;
            if(tmp_size > max_size)
            {
                max_size = tmp_size;
                bp = bp_map[i].bp;
                bp_i = i;
                bp_i = bp_i;
            }
        }
    }

    NORFLASH_API_TRACE(2,"%s: w: (0x%x -- 0x%x) bp: 0x%x(0x%x -- 0x%x).",
           __func__,
           addr, addr + size,
           bp,
           bp_map[bp_i].start_addr, bp_map[bp_i].end_addr);
    ASSERT(bp != FLASH_BP_ALL, "%s: protection area undefined! addr:0x%x -- 0x%x",
           __func__, addr, addr + size );

    result = hal_norflash_set_protection(id, bp);
    ASSERT(result == HAL_NORFLASH_OK,
            "%s: set protection fail! ret = %d",
            __func__, result);

}

static void _protection_all(enum HAL_FLASH_ID_T id)
{
    enum HAL_NORFLASH_RET_T result;
    uint32_t bp = FLASH_BP_ALL;

    result = hal_norflash_set_protection(id, bp);
    ASSERT(result == HAL_NORFLASH_OK,
            "%s: set protection fail! ret = %d",
            __func__, result);
}
void norflash_api_get_pb_map(enum HAL_FLASH_ID_T id,
                        FLASH_BP_MAP_T **pb_map,
                        uint32_t       *pb_count)
{

#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(id == HAL_FLASH_ID_0)
    {
        *pb_map = (FLASH_BP_MAP_T*)flash_bp_map;
        *pb_count = flash_bp_numer;
    }
    else
    {
        *pb_map = (FLASH_BP_MAP_T*)flash1_bp_map;
        *pb_count = flash1_bp_numer;
    }
#else
    ASSERT(id == HAL_FLASH_ID_0, "%s: Invalid id: 0x%x ", __func__, id);
    *pb_map = (FLASH_BP_MAP_T*)flash_bp_map;
    *pb_count = flash_bp_numer;
#endif
}

#else // FLASH_PROTECTION

static void _protection_part(enum HAL_FLASH_ID_T id, uint32_t start_addr, uint32_t size){}
static void _protection_all(enum HAL_FLASH_ID_T id){}

#endif // FLASH_PROTECTION

#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)

#define NORFLASH_API_MESSAGE_ID    0
#define NORFLASH_API_MAILBOX_MAX   2

typedef struct
{
    uint32_t id;
    uint32_t param;
} NORFLASH_API_MESSAGE_T;


static void _norflash_api_thread(void const *argument);
static osMemoryPoolId_t   mail_box_mp_id;
static osMessageQueueId_t mail_box_mq_id;
static uint8_t _norflash_api_mailbox_cnt = 0;

static uint64_t os_thread_def_stack [ROUND_UP(4096, 8) / sizeof(uint64_t)];
static const osThreadAttr_t ThreadAttr_norflash = {
    .name = (char *)"norflash_api_thread",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0U,
    .stack_mem = os_thread_def_stack,
    .stack_size = ROUND_UP(4096, 8),
    .priority = osPriorityLow,
    .tz_module = 1U,                  // indicate calls to secure mode
    .reserved = 0U,
};

static const osMemoryPoolAttr_t MPAttr_norflash = {
    .name = NULL,
    .attr_bits = 0,
    .cb_mem = NULL,
    .cb_size = 0,
    .mp_mem = NULL,
    .mp_size = 0,
};
static const osMessageQueueAttr_t MQAttr_norflash = {
    .name = "_norflash_api_mailbox",
    .attr_bits = 0,
    .cb_mem = NULL,
    .cb_size = 0,
    .mq_mem = NULL,
    .mq_size = 0,
};

static int _norflash_api_mailbox_init(void)
{
    // NORFLASH_API_TRACE(1, "%s,%d",__func__,__LINE__);
    mail_box_mp_id = osMemoryPoolNew(NORFLASH_API_MAILBOX_MAX,
                                    sizeof(NORFLASH_API_MESSAGE_T), &MPAttr_norflash);
    mail_box_mq_id = osMessageQueueNew(NORFLASH_API_MAILBOX_MAX,
                                    sizeof(void *), &MQAttr_norflash);
    if (mail_box_mp_id == NULL || mail_box_mq_id == NULL)  {
        NORFLASH_API_TRACE(1, "Failed to Create _norflash_api_mailbox\n");
        return -1;
    }
    _norflash_api_mailbox_cnt = 0;
    return 0;
}

static int _norflash_api_mailbox_put(NORFLASH_API_MESSAGE_T* msg_src)
{
    osStatus_t status;
    NORFLASH_API_MESSAGE_T *msg_p = NULL;

    if(_norflash_api_mailbox_cnt >= NORFLASH_API_MAILBOX_MAX)
    {
        NORFLASH_API_TRACE(1, "%s, _norflash_api_mailbox_cnt  = %d.",
        __func__, _norflash_api_mailbox_cnt);
        return 0;
    }
    msg_p = (NORFLASH_API_MESSAGE_T*)osMemoryPoolAlloc(mail_box_mp_id, 0);
    ASSERT(msg_p, "osMailAlloc error");
    msg_p->id = msg_src->id;
    msg_p->param = msg_src->param;

    status = osMessageQueuePut(mail_box_mq_id, &msg_p, 0U, 0U);
    if (osOK == status)
    {
        _norflash_api_mailbox_cnt++;
    }
    // NORFLASH_API_TRACE(1,"%s,_norflash_api_mailbox_cnt = %d.", __func__, _norflash_api_mailbox_cnt);
    return (int)status;
}

static int _norflash_api_mailbox_free(NORFLASH_API_MESSAGE_T* msg_p)
{
    osStatus_t status;

    status = osMemoryPoolFree(mail_box_mp_id, msg_p);
    if (osOK == status)
    {
        _norflash_api_mailbox_cnt--;
    }
    NORFLASH_API_TRACE(1,"%s,_norflash_api_mailbox_cnt = %d.", __func__, _norflash_api_mailbox_cnt);
    return (int)status;
}

static int _norflash_api_mailbox_get(NORFLASH_API_MESSAGE_T **msg_p)
{
    osStatus_t status;
    void *mail = NULL;
    status = osMessageQueueGet(mail_box_mq_id, &mail, NULL, osWaitForever);
    if (status == osOK) {
        *msg_p = (NORFLASH_API_MESSAGE_T*)mail;
        return 0;
    }
    return -1;
}

static void _norflash_api_thread(void const *argument)
{
    while(1){
        NORFLASH_API_MESSAGE_T *msg_p = NULL;
        if (!_norflash_api_mailbox_get(&msg_p)) {
            // TRACE(1,"%s: id = 0x%x, ptr = 0x%x,param = 0x%x.",
            //     __func__, msg_p->id, msg_p->ptr, msg_p->param);
            if(msg_p->id == NORFLASH_API_MESSAGE_ID)
            {
                _norflash_api_exec_flush_hook();
            }
            // TRACE(1,"%s: hook done.", __func__);
            _norflash_api_mailbox_free(msg_p);
        }
    }
}

static int norflash_api_thread_init(void)
{
    osThreadId_t tid;

    if (_norflash_api_mailbox_init()) {
        TRACE(1,"%s: _norflash_api_mailbox_init failed!", __func__);
        return -1;
    }
    tid = osThreadNew((osThreadFunc_t)_norflash_api_thread, NULL, &ThreadAttr_norflash);
    if (tid == NULL) {
        TRACE(1,"Failed to Create _flush_thread\n");
        return -2;
    }
    return 0;
}


static int _norflash_api_hook_activate(void)
{
    NORFLASH_API_MESSAGE_T msg;
    int32_t ret;

    msg.id = NORFLASH_API_MESSAGE_ID;
    msg.param = 0;
    ret = _norflash_api_mailbox_put(&msg);
    return ret;
}

#endif

int norflash_api_hook_activate(void)
{
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
    return _norflash_api_hook_activate();
#endif
    return 0;
}

void norflash_api_set_hook(enum NORFLASH_API_HOOK_USER_T user_id , NORFLASH_API_HOOK_HANDLE hook_handle)
{
    ASSERT(user_id < NORFLASH_API_HOOK_USER_QTY, "%s: user_id %d invalid.", __func__, user_id)
    _norflash_api_hook[user_id].hook_handle = hook_handle;
}

uint32_t noflash_api_to_nc_addr(enum HAL_FLASH_ID_T id, uint32_t addr)
{
    uint32_t nc_addr = 0;

    if(id == HAL_FLASH_ID_0)
    {
        nc_addr = (((addr) & HAL_NORFLASH_ADDR_MASK) | FLASH_NC_BASE);
    }
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    else if(id == HAL_FLASH_ID_1)
    {
        nc_addr = (((addr) & HAL_NORFLASH_ADDR_MASK) | FLASH1_NC_BASE);
    }
#endif
    else
    {
        ASSERT(0, "%s: id(%d) invalid!!!", __func__, id);
    }
    return nc_addr;
}


static int32_t _opera_read(MODULE_INFO *mod_info,
               uint32_t addr,
               uint8_t *buff,
               uint32_t len)
{
    OPRA_INFO *opera_node;
    OPRA_INFO *e_node = NULL;
    OPRA_INFO *w_node = NULL;
    OPRA_INFO *tmp;
    uint32_t r_offs;
    uint32_t sec_start;
    uint32_t sec_len;

    sec_len = mod_info->mod_sector_len;
    sec_start = (addr/sec_len)*sec_len;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;
        if(opera_node->addr == sec_start)
        {
            if(opera_node->type == NORFLASH_API_WRITTING)
            {
                w_node = opera_node;
                break;
            }
            else
            {
                e_node = opera_node;
                break;
            }
        }
    }

    if(w_node)
    {
        r_offs = addr - sec_start;
        norflash_memcpy(buff,w_node->buff + r_offs,len);
    }
    else
    {
        if(e_node)
        {
            norflash_memset(buff,0xff,len);
        }
        else
        {
            FLASH_REMAP_START(mod_info->dev_id,addr,len);
#ifdef NO_FLASH1_ADDR_ACCESS
            if(mod_info->dev_id > HAL_FLASH_ID_0)
            {
                hal_norflash_read(mod_info->dev_id, addr, buff, len);
            }
            else
#endif
            {
                norflash_memcpy(buff,(uint8_t*)addr,len);
            }
            FLASH_REMAP_DONE(mod_info->dev_id,addr,len);
            /*
            HAL_NORFLASH_RET_T result;
            result = hal_norflash_read(mod_info->dev_id,addr,buff,len);
            if(result != HAL_NORFLASH_OK)
            {
                NORFLASH_API_TRACE(2,"%s: hal_norflash_read failed,result = %d.",
                        __func__,result);
                return result;
            }
            */
        }
    }
    return 0;
}

static int32_t _e_opera_add(MODULE_INFO *mod_info,
               uint32_t addr,
               uint32_t len
               )
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *pre_node = NULL;
    OPRA_INFO *tmp;
    int32_t ret = 0;

    // delete opera nodes with the same address when add the erase opera node.
    pre_node = mod_info->opera_info;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;
        if((opera_node->addr >= addr) && ((opera_node->addr + opera_node->len) <= (addr + len)))
        {
            if(opera_node->lock == false)
            {
                NORFLASH_API_TRACE(3,"%s:Opera is merged! ",__func__);
                if(opera_node == mod_info->opera_info)
                {
                    mod_info->opera_info = tmp;
                }
                else
                {
                    pre_node->next = tmp;
                }
                if(opera_node->type == NORFLASH_API_WRITTING)
                {
                    if(opera_node->buff)
                    {
                        _norflash_api_free(mod_info->dev_id, opera_node->buff);
                    }
                }
                _norflash_api_free(mod_info->dev_id, opera_node);
            }
            else
            {
                if(opera_node->type == NORFLASH_API_ERASING)
                {
                    NORFLASH_API_TRACE(3,"%s: erase opera is merged! addr = 0x%x,len = 0x%x.",
                        __func__,
                        opera_node->addr,
                        opera_node->len);
                    ret = 0;
                    goto _func_end;
                }
                else
                {
                    pre_node = opera_node;
                }
            }
        }
        else
        {
            pre_node = opera_node;
        }
    }

    // add new node to header.
    opera_node = (OPRA_INFO*)_norflash_api_malloc(mod_info->dev_id, sizeof(OPRA_INFO));
    if(opera_node == NULL)
    {
        NORFLASH_API_TRACE(3,"%s:%d,_norflash_api_malloc failed! size = %d.",
                __func__,__LINE__,sizeof(OPRA_INFO));
        ret = 1;
        goto _func_end;
    }
    opera_node->type = NORFLASH_API_ERASING;
    opera_node->addr = addr;
    opera_node->len = len;
    opera_node->w_offs = 0;
    opera_node->w_len = 0;
    opera_node->buff = NULL;
    opera_node->lock = false;
    opera_node->next = mod_info->opera_info;
    mod_info->opera_info = opera_node;
    ret = 0;
_func_end:
    return ret;
}

static int32_t _w_opera_add(MODULE_INFO *mod_info,
               uint32_t addr,
               uint32_t len,
               uint8_t *buff)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *e_node = NULL;
    OPRA_INFO *w_node = NULL;
    OPRA_INFO *tmp;
    uint32_t w_offs;
    uint32_t w_len;
    uint32_t sec_start;
    uint32_t sec_len;
    uint32_t w_end1;
    uint32_t w_end2;
    uint32_t w_start;
    uint32_t w_end;
    uint32_t w_len_new;
    int32_t ret = 0;

    sec_len = mod_info->mod_sector_len;
    sec_start = (addr/sec_len)*sec_len;
    w_offs = addr - sec_start;
    w_len = len;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;

        if(opera_node->addr == sec_start)
        {
            if(opera_node->type == NORFLASH_API_WRITTING)
            {
                if(!opera_node->lock)
                {
                    // select the first w_node in the list.
                    w_node = opera_node;
                    break;
                }
            }
            else
            {
                e_node = opera_node;
                break;
            }
        }
    }

    if(w_node)
    {
        norflash_memcpy(w_node->buff + w_offs,buff,w_len);
        w_start = w_node->w_offs <= w_offs ? w_node->w_offs:w_offs;
        w_end1 = w_node->w_offs + w_node->w_len;
        w_end2 = w_offs + w_len;
        w_end = w_end1 >= w_end2 ? w_end1 : w_end2;
        w_len_new = w_end - w_start;
        w_node->w_offs = w_start;
        w_node->w_len = w_len_new;
        opera_node = w_node;
        ret = 0;
    }
    else
    {
        opera_node = (OPRA_INFO*)_norflash_api_malloc(mod_info->dev_id, sizeof(OPRA_INFO));
        if(opera_node == NULL)
        {
            NORFLASH_API_TRACE(3,"%s:%d,_norflash_api_malloc failed! size = %d.",
                    __func__,__LINE__,sizeof(OPRA_INFO));
             ret = 1;
             goto _func_end;
        }
        opera_node->type = NORFLASH_API_WRITTING;
        opera_node->addr = sec_start;
        opera_node->len = sec_len;
        opera_node->w_offs = w_offs;
        opera_node->w_len = w_len;
        opera_node->buff = (uint8_t*)_norflash_api_malloc(mod_info->dev_id, opera_node->len);
        if(opera_node->buff == NULL)
        {
            _norflash_api_free(mod_info->dev_id, opera_node);
            NORFLASH_API_TRACE(3,"%s:%d,_norflash_api_malloc failed! size = %d.",
                    __func__,__LINE__,opera_node->len);
             ret = 1;
             goto _func_end;
        }
        if(e_node)
        {
            norflash_memset(opera_node->buff,0xff,opera_node->len);
        }
        else
        {
            if (w_len != opera_node->len)
            {
                FLASH_REMAP_START(mod_info->dev_id,opera_node->addr,opera_node->len);
#ifdef NO_FLASH1_ADDR_ACCESS
                if(mod_info->dev_id > HAL_FLASH_ID_0)
                {
                    hal_norflash_read(mod_info->dev_id, opera_node->addr, opera_node->buff, opera_node->len);
                }
                else
#endif
                {
                    norflash_memcpy(opera_node->buff,(uint8_t*)opera_node->addr,opera_node->len);
                }
                FLASH_REMAP_DONE(mod_info->dev_id,opera_node->addr,opera_node->len);
            }
        }
        norflash_memcpy(opera_node->buff + w_offs,buff,w_len);
        opera_node->lock = false;
        opera_node->next = mod_info->opera_info;
        mod_info->opera_info = opera_node;
        ret = 0;
    }

_func_end:
    return ret;
}

static bool _opera_flush(MODULE_INFO *mod_info,bool nosuspend)
{
    OPRA_INFO *cur_opera_info;
    enum HAL_NORFLASH_RET_T result;
    bool opera_is_completed = false;
    NORFLASH_API_OPERA_RESULT opera_result;
    bool ret = false;
    bool suspend;

#if defined(FLASH_SUSPEND)
    suspend = true;
#else
    suspend = false;
#endif
    suspend = nosuspend == true ? false: suspend;

    if(!mod_info->cur_opera_info)
    {
        mod_info->cur_opera_info = _get_tail(mod_info, false);
    }

    if(!mod_info->cur_opera_info)
    {
        return false;
    }

    ret = true;
    cur_opera_info = mod_info->cur_opera_info;
    if(cur_opera_info->type == NORFLASH_API_WRITTING)
    {
        if(mod_info->state == NORFLASH_API_STATE_IDLE)
        {
            suspend_number = 0;
            if(cur_opera_info->w_len > 0)
            {
                NORFLASH_API_TRACE(5,"%s: %d,hal_norflash_write_suspend,addr = 0x%x,len = 0x%x,suspend = %d.",
                                __func__,__LINE__,
                                cur_opera_info->addr + cur_opera_info->w_offs,
                                cur_opera_info->w_len,
                                suspend);

                _protection_part(mod_info->dev_id,cur_opera_info->addr + cur_opera_info->w_offs,cur_opera_info->w_len);
                FLASH_REMAP_START(mod_info->dev_id,cur_opera_info->addr + cur_opera_info->w_offs,cur_opera_info->w_len);
                PMU_FLASH_WRITE_CONFIG_FUNC();
                result = hal_norflash_write_suspend(mod_info->dev_id,
                             cur_opera_info->addr + cur_opera_info->w_offs,
                             cur_opera_info->buff + cur_opera_info->w_offs,
                             cur_opera_info->w_len,
                             suspend);
                PMU_FLASH_READ_CONFIG_FUNC();
                FLASH_REMAP_DONE(mod_info->dev_id,cur_opera_info->addr + cur_opera_info->w_offs,cur_opera_info->w_len);
            }
            else
            {
                result = HAL_NORFLASH_OK;
            }

            if(result == HAL_NORFLASH_OK)
            {
                _protection_all(mod_info->dev_id);
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_WRITTING_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, hal_norflash_write_suspend failed,result = %d",__func__,__LINE__,result);
            }
        }
        else if(mod_info->state == NORFLASH_API_STATE_WRITTING_SUSPEND)
        {
            suspend_number ++;

            FLASH_REMAP_START(mod_info->dev_id,cur_opera_info->addr + cur_opera_info->w_offs,cur_opera_info->w_len);
            PMU_FLASH_WRITE_CONFIG_FUNC();
            result = hal_norflash_write_resume(mod_info->dev_id, suspend);
            PMU_FLASH_READ_CONFIG_FUNC();
            FLASH_REMAP_DONE(mod_info->dev_id,cur_opera_info->addr + cur_opera_info->w_offs,cur_opera_info->w_len);
            if(result == HAL_NORFLASH_OK)
            {
                _protection_all(mod_info->dev_id);
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_WRITTING_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, write resume failed! r = %d, addr: 0x%x, len: 0x%x",
                       __func__,__LINE__, result,
                       cur_opera_info->addr + cur_opera_info->w_offs, cur_opera_info->w_len);
            }
        }
        else
        {
            ASSERT(0, "%s: %d, mod_info->state error,state = %d",__func__,__LINE__,mod_info->state);
        }
    }
    else
    {
        if(mod_info->state == NORFLASH_API_STATE_IDLE)
        {
            suspend_number = 0;
            NORFLASH_API_TRACE(5,"%s: %d,hal_norflash_erase_suspend,addr = 0x%x,len = 0x%x,suspend = %d.",
                                __func__,__LINE__,
                                cur_opera_info->addr,
                                cur_opera_info->len,
                                suspend);
            _protection_part(mod_info->dev_id, cur_opera_info->addr, cur_opera_info->len);
            FLASH_REMAP_START(mod_info->dev_id, cur_opera_info->addr, cur_opera_info->len);
            PMU_FLASH_WRITE_CONFIG_FUNC();
            result = hal_norflash_erase_suspend(mod_info->dev_id,
                         cur_opera_info->addr,
                         cur_opera_info->len,
                         suspend);
            PMU_FLASH_READ_CONFIG_FUNC();
            FLASH_REMAP_DONE(mod_info->dev_id, cur_opera_info->addr, cur_opera_info->len);
            if(result == HAL_NORFLASH_OK)
            {
                _protection_all(mod_info->dev_id);
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_ERASE_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, hal_norflash_erase_suspend failed,result = %d",__func__,__LINE__,result);
            }
        }
        else if(mod_info->state == NORFLASH_API_STATE_ERASE_SUSPEND)
        {
            suspend_number ++;
            FLASH_REMAP_START(mod_info->dev_id, cur_opera_info->addr, cur_opera_info->len);
            PMU_FLASH_WRITE_CONFIG_FUNC();
            result = hal_norflash_erase_resume(mod_info->dev_id,
                         suspend);
            PMU_FLASH_READ_CONFIG_FUNC();
            FLASH_REMAP_DONE(mod_info->dev_id, cur_opera_info->addr, cur_opera_info->len);

            if(result == HAL_NORFLASH_OK)
            {
                _protection_all(mod_info->dev_id);
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_ERASE_SUSPEND;
            }
            else
            {
                 ASSERT(0, "%s: %d, erase resume failed! r = %d, addr: 0x%x, len: 0x%x",
                       __func__,__LINE__,result,cur_opera_info->addr,cur_opera_info->len);
            }
        }
        else
        {
            ASSERT(0, "%s: %d, mod_info->state error,state = %d",
                __func__,__LINE__,mod_info->state);
        }
    }

__opera_is_completed:

    if(opera_is_completed)
    {
        mod_info->state = NORFLASH_API_STATE_IDLE;
        if(!nosuspend
           && mod_info->cb_func
           && ((cur_opera_info->w_len > 0 && cur_opera_info->type == NORFLASH_API_WRITTING)
               || (cur_opera_info->len > 0 && cur_opera_info->type == NORFLASH_API_ERASING))
           )
        {
            NORFLASH_API_TRACE(6,"%s: w/e done.type:%d,addr:0x%x,w_len:0x%x,len:0x%x,suspend_num:%d.",
                __func__,
                cur_opera_info->type,
                cur_opera_info->addr + cur_opera_info->w_offs,
                cur_opera_info->w_len,
                cur_opera_info->len,
                suspend_number);
            if(cur_opera_info->type == NORFLASH_API_WRITTING)
            {
                opera_result.addr = cur_opera_info->addr + cur_opera_info->w_offs;
                opera_result.len = cur_opera_info->w_len;
            }
            else
            {
                opera_result.addr = cur_opera_info->addr;
                opera_result.len = cur_opera_info->len;
            }
            _cache_invalid(mod_info->dev_id, opera_result.addr, opera_result.len);
            opera_result.type = cur_opera_info->type;
            opera_result.result = NORFLASH_API_OK;
            opera_result.remain_num = _get_ew_count(mod_info) - 1;
            opera_result.suspend_num = suspend_number;
            mod_info->cb_func(&opera_result);
        }
        _opera_del(mod_info,cur_opera_info);
        mod_info->cur_opera_info = NULL;
    }

    return ret;
}

static void _flush_disable(enum NORFLASH_API_USER user_id,uint32_t cb)
{
    norflash_api_info.allowed_cb[user_id] = (NOFLASH_API_FLUSH_ALLOWED_CB)cb;
}

static void _flush_enable(enum NORFLASH_API_USER user_id)
{
    norflash_api_info.allowed_cb[user_id] = NULL;
}

static bool _flush_is_allowed(void)
{
    bool ret = true;
    uint32_t user_id;

    for(user_id = NORFLASH_API_USER_CP; user_id < NORFLASH_API_USER_COUNTS; user_id ++)
    {
        if(norflash_api_info.allowed_cb[user_id])
        {
            if(!norflash_api_info.allowed_cb[user_id]())
            {
                ret = false;
            }
            else
            {
                norflash_api_info.allowed_cb[user_id] = NULL;
            }
        }
    }
    return ret;
}

static int _norflash_api_flush(void)
{
    enum NORFLASH_API_MODULE_ID_T mod_id = NORFLASH_API_MODULE_ID_COUNT;
    MODULE_INFO *mod_info;
    int ret;

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return 0;
    }
#if defined(FLASH_API_SIMPLE)
    return 0;
#endif

    if(!_flush_is_allowed())
    {
        return 0;
    }

    mod_info = _get_cur_mod();
    if(!mod_info)
    {
        return 0;
    }
    _norflash_api_int_lock(mod_info->dev_id);
    mod_id = _get_mod_id(mod_info);

    norflash_api_info.cur_mod_id = mod_id;
    norflash_api_info.cur_mod = mod_info;
    if(!_opera_flush(mod_info,false))
    {
        norflash_api_info.cur_mod = NULL;
    }

    if(!_get_cur_mod())
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    _norflash_api_int_unlock(mod_info->dev_id);

    return ret;
}

//-------------------------------------------------------------------
// APIS Function.
//-------------------------------------------------------------------
enum NORFLASH_API_RET_T norflash_api_init(void)
{
    uint32_t i;
    uint32_t total_size;
    bool protection_enable = false;
    enum HAL_NORFLASH_RET_T ret;

    norflash_memset((void*)&norflash_api_info, 0, sizeof(NORFLASH_API_INFO));
    norflash_api_info.cur_mod_id = NORFLASH_API_MODULE_ID_COUNT;
    norflash_api_info.is_inited = true;
    norflash_api_info.cur_mod = NULL;
    for(i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++)
    {
        norflash_api_info.mod_info[i].state = NORFLASH_API_STATE_UNINITED;
    }
    _norflash_api_mutex_init();
    norflash_api_set_hook(NORFLASH_API_HOOK_USER_0, norflash_api_flush);
#if !defined(FLASH_API_SIMPLE)
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
    norflash_api_thread_init();
#else
#if defined(FLASH_SUSPEND)
    hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_NORFLASH_API, _norflash_api_exec_flush_hook);
#else //FLASH_SUSPEND
    hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_NORFLASH_API, _norflash_api_exec_flush_hook);
#endif // FLASH_SUSPEND
#endif
#endif // FLASH_API_SIMPLE

    for(i = HAL_FLASH_ID_0; i < HAL_FLASH_ID_NUM; i++)
    {
        if(i > HAL_FLASH_ID_0)
        {
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
            hal_norflash_init((enum HAL_FLASH_ID_T)i);
#else
            continue;
#endif
        }

        ret = hal_norflash_get_size((enum HAL_FLASH_ID_T)i, &flash_total_size[i], &flash_block_size[i], &flash_sector_size[i], &flash_page_size[i]);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: get size fail!, ret = %d", __func__, (int)ret);

#ifdef FLASH_PROTECTION
        ret = hal_norflash_get_dual_chip_mode((enum HAL_FLASH_ID_T)i, &flash_chip_dual_enable[i], NULL);
        ASSERT(ret == HAL_NORFLASH_OK, "%s: get dual chip mode fail!, ret = %d", __func__, (int)ret);
        total_size = hal_norflash_get_flash_total_size((enum HAL_FLASH_ID_T)i);
        if(_flash_chip_dual_enable((enum HAL_FLASH_ID_T)i))
        {
            if(total_size >= 0x40000 && total_size <= 0x2000000)
            {
                protection_enable = true;
            }
        }
        else
        {
            if(total_size >= 0x20000 && total_size <= 0x2000000)
            {
                protection_enable = true;
            }
        }

        ASSERT(protection_enable, "PROTECTION NOT SUPPORT AT THIS SIZE! flash id = %d, total size: 0x%x",i, total_size);
        norflash_api_protection_bp_init((enum HAL_FLASH_ID_T)i, total_size);
#else
        total_size = 0;
        total_size = total_size;
        protection_enable = false;
        protection_enable = protection_enable;
#endif

#ifdef FLASH_REMAP
        FLASH_REMAP_CONFIG((enum HAL_FLASH_ID_T)i);
#endif
    }

    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_register(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                enum HAL_FLASH_ID_T dev_id,
                uint32_t mod_base_addr,
                uint32_t mod_len,
                uint32_t mod_block_len,
                uint32_t mod_sector_len,
                uint32_t mod_page_len,
                uint32_t buffer_len,
                NORFLASH_API_OPERA_CB cb_func
                )
{
    MODULE_INFO *mod_info;

    NORFLASH_API_TRACE(5,"mod_id = %d,dev_id = %d,base_addr = 0x%x,mod_len = 0x%x",
            mod_id,dev_id,mod_base_addr,mod_len);
    NORFLASH_API_TRACE(4,"mod_block_len = 0x%x,mod_sector_len = 0x%x,mod_page_len = 0x%x,buffer_len = 0x%x.",
            mod_block_len,mod_sector_len,mod_page_len,buffer_len);
    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(2,"%s: %d, norflash_api uninit!",__func__,__LINE__);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    if(dev_id >= HAL_FLASH_ID_NUM)
    {
        NORFLASH_API_TRACE(2,"%s : dev_id error! mod_id = %d.",__func__,dev_id);
        return NORFLASH_API_BAD_DEV_ID;
    }

    if(buffer_len < mod_sector_len || !IS_ALIGN(buffer_len,mod_sector_len))
    {
        NORFLASH_API_TRACE(2,"%s : buffer_len error buffer_len = %d.",__func__, buffer_len);
        return NORFLASH_API_BAD_BUFF_LEN;
    }
    mod_info = _get_module_info(mod_id);
    if(mod_info->is_registered)
    {
        NORFLASH_API_TRACE(1,"%s: %d, norflash_async[%d] has registered!",__func__,__LINE__,mod_id);
        return NORFLASH_API_ERR_REGISTRATION;
    }
    _norflash_api_mutex_wait();
    mod_info->dev_id = dev_id;
    mod_info->mod_id = mod_id;
    mod_info->mod_base_addr = mod_base_addr;
    mod_info->mod_len = mod_len;
    mod_info->mod_block_len = mod_block_len;
    mod_info->mod_sector_len = mod_sector_len;
    mod_info->mod_page_len = mod_page_len;
    mod_info->buff_len = buffer_len;
    mod_info->cb_func = cb_func;
    mod_info->opera_info = NULL;
    mod_info->cur_opera_info = NULL;
    mod_info->state = NORFLASH_API_STATE_IDLE;
    mod_info->is_registered = true;
    _norflash_api_mutex_release();
    return NORFLASH_API_OK;
}

enum HAL_FLASH_ID_T norflash_api_get_dev_id_by_addr(uint32_t addr)
{
    uint32_t tmp;

    tmp = addr & FLASH_BASE;
    if (tmp == FLASH_BASE)
    {
        return HAL_FLASH_ID_0;
    }
    tmp = addr & FLASH_NC_BASE;
    if (tmp == FLASH_NC_BASE)
    {
        return HAL_FLASH_ID_0;
    }
#if defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    tmp = addr & FLASH1_BASE;
    if (tmp == FLASH1_BASE)
    {
        return HAL_FLASH_ID_1;
    }
    tmp = addr & FLASH1_NC_BASE;
    if (tmp == FLASH1_NC_BASE)
    {
        return HAL_FLASH_ID_1;
    }
#endif
#if defined(FLASH2_CTRL_BASE) && defined(USE_MULTI_FLASH)
    tmp = addr & FLASH2_BASE;
    if (tmp == FLASH2_BASE)
    {
        return HAL_FLASH_ID_2;
    }
    tmp = addr & FLASH2_NC_BASE;
    if (tmp == FLASH2_NC_BASE)
    {
        return HAL_FLASH_ID_2;
    }
#endif
    else
    {
        ASSERT(0, "get flash id fail! addr = 0x%x", addr);
    }
}

enum NORFLASH_API_RET_T norflash_api_read(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                uint8_t *buffer,
                uint32_t len
                )
{
    MODULE_INFO *mod_info;
    int32_t result;
    enum NORFLASH_API_RET_T ret;

    NORFLASH_API_TRACE(4,"%s:mod_id = %d,start_addr = 0x%x,len = 0x%x",
                __func__,mod_id, start_addr, len);
    ASSERT(buffer,"%s:buffer is null! ",
                __func__);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(!ADDR_IS_VALIDE(mod_info->mod_base_addr, mod_info->mod_len, start_addr, len))
    {
        NORFLASH_API_TRACE(3,"%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__, start_addr, len);
        return NORFLASH_API_BAD_ADDR;
    }

    if((len == 0)
       || !IN_ONE_SECTOR(start_addr, len, flash_sector_size[mod_info->dev_id]))
    {
        NORFLASH_API_TRACE(2,"%s : len error! start_addr = 0x%x, len = %d.",
                   __func__, start_addr, len);
        return NORFLASH_API_BAD_LEN;
    }

    _norflash_api_mutex_wait();
    _norflash_api_int_lock(mod_info->dev_id);

    result = _opera_read(mod_info,start_addr,(uint8_t*)buffer,len);

    if(result)
    {
        ret =  NORFLASH_API_ERR;
    }
    else
    {
        ret = NORFLASH_API_OK;
    }
    _norflash_api_int_unlock(mod_info->dev_id);
    _norflash_api_mutex_release();
    NORFLASH_API_TRACE(2,"%s: done. ret = %d.",__func__,ret);
    return ret;
}

enum NORFLASH_API_RET_T norflash_sync_read(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                uint8_t *buffer,
                uint32_t len
                )
{
    MODULE_INFO *mod_info;

    NORFLASH_API_TRACE(4,"%s:mod_id = %d,start_addr = 0x%x,len = 0x%x",
                __func__,mod_id,start_addr,len);
    ASSERT(buffer,"%s:%d,buffer is null! ",
                __func__,__LINE__);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!", __func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.", __func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(!ADDR_IS_VALIDE(mod_info->mod_base_addr, mod_info->mod_len, start_addr, len))
    {
        NORFLASH_API_TRACE(3,"%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__, start_addr, len);
        return NORFLASH_API_BAD_ADDR;
    }

    if((len == 0)
        || !IN_ONE_SECTOR(start_addr, len, flash_sector_size[mod_info->dev_id]))
    {
        NORFLASH_API_TRACE(2,"%s : len error! start_addr = 0x%x, len = %d.",
                   __func__, start_addr, len);
        return NORFLASH_API_BAD_LEN;
    }
    _norflash_api_mutex_wait();
    _norflash_api_int_lock(mod_info->dev_id);
    FLASH_REMAP_START(mod_info->dev_id,start_addr, len);
#ifdef NO_FLASH1_ADDR_ACCESS
    if(mod_info->dev_id > HAL_FLASH_ID_0)
    {
        hal_norflash_read(mod_info->dev_id, start_addr, buffer, len);
    }
    else
#endif
    {
        norflash_memcpy(buffer,(uint8_t*)start_addr,len);
    }
    FLASH_REMAP_DONE(mod_info->dev_id,start_addr, len);
    _norflash_api_int_unlock(mod_info->dev_id);
    _norflash_api_mutex_release();
    NORFLASH_API_TRACE(1,"%s: done.",__func__);
    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_erase(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                uint32_t len,
                bool async
                )
{
    MODULE_INFO *mod_info;
    MODULE_INFO *cur_mod_info;
    int32_t result;
    bool bresult = 0;
    enum NORFLASH_API_RET_T ret;

    NORFLASH_API_TRACE(5,"%s: mod_id = %d,start_addr = 0x%x,len = 0x%x,async = %d.",
                __func__,mod_id,start_addr,len,async);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : invalid mod_id! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(!ADDR_IS_VALIDE(mod_info->mod_base_addr, mod_info->mod_len, start_addr, len))
    {
        NORFLASH_API_TRACE(3,"%s : erase out of range! start_address = 0x%x,len = 0x%x.",
                    __func__,start_addr,len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(
#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
        !IS_ALIGN(start_addr,mod_info->mod_page_len) &&
#endif
        !IS_ALIGN(start_addr,mod_info->mod_sector_len))
    {
        NORFLASH_API_TRACE(2,"%s : start_address no alignment! start_address = %d.",
                   __func__, start_addr);
        return NORFLASH_API_BAD_ADDR;
    }

    if(
#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
       len != mod_info->mod_page_len &&
#endif
       len != mod_info->mod_sector_len &&
       len != mod_info->mod_block_len)
    {
        NORFLASH_API_TRACE(2,"%s : len error. len = %d!",
                   __func__, len);
        return NORFLASH_API_BAD_LEN;
    }
    _norflash_api_mutex_wait();
#if defined(FLASH_API_SIMPLE)
    async = false;
#endif
    if(async)
    {
        _norflash_api_int_lock(mod_info->dev_id);
        // add to opera_info chain header.
        result = _e_opera_add(mod_info,start_addr,len);
        if(result == 0)
        {
            ret = NORFLASH_API_OK;
        }
        else
        {
            ret = NORFLASH_API_BUFFER_FULL;
        }
        _norflash_api_int_unlock(mod_info->dev_id);
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
        _norflash_api_hook_activate();
#endif
        NORFLASH_API_TRACE(4,"%s: _e_opera_add done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    else
    {
        _norflash_api_int_lock(mod_info->dev_id);
        // Handle the suspend operation.
        if(norflash_api_info.cur_mod != NULL
           && mod_info != norflash_api_info.cur_mod)
        {
            cur_mod_info = norflash_api_info.cur_mod;
            while(cur_mod_info->state != NORFLASH_API_STATE_IDLE)
            {
                if(!_flush_is_allowed())
                {
                    continue;
                }
                bresult = _opera_flush(cur_mod_info,true);
                if(!bresult)
                {
                    norflash_api_info.cur_mod = NULL;
                }
            }
        }

        // flush all of cur module opera.
        // norflash_api_info.cur_mod_id = mod_id;
        do{
            if(!_flush_is_allowed())
            {
                continue;
            }
            bresult = _opera_flush(mod_info,true);
        }while(bresult);

        _protection_part(mod_info->dev_id,start_addr,len);
        FLASH_REMAP_START(mod_info->dev_id,start_addr,len);
        PMU_FLASH_WRITE_CONFIG_FUNC();
        result = hal_norflash_erase(mod_info->dev_id,start_addr, len);
        PMU_FLASH_READ_CONFIG_FUNC();
        FLASH_REMAP_DONE(mod_info->dev_id,start_addr,len);
        _protection_all(mod_info->dev_id);
        if(result == HAL_NORFLASH_OK)
        {
            ret = NORFLASH_API_OK;
        }
        else if(result == HAL_NORFLASH_BAD_ADDR)
        {
            ret = NORFLASH_API_BAD_ADDR;
        }
        else if(result == HAL_NORFLASH_BAD_LEN)
        {
            ret = NORFLASH_API_BAD_LEN;
        }
        else
        {
            ret = NORFLASH_API_ERR;
        }
        _cache_invalid(mod_info->dev_id, start_addr, len);
        _norflash_api_int_unlock(mod_info->dev_id);
        NORFLASH_API_TRACE(4,"%s: hal_norflash_erase done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    _norflash_api_mutex_release();
    //NORFLASH_API_TRACE(2,"%s: done.ret = %d.",__func__, ret);
    return ret;
}

enum NORFLASH_API_RET_T norflash_api_write(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                const uint8_t *buffer,
                uint32_t len,
                bool async
                )
{
    MODULE_INFO *mod_info;
    MODULE_INFO *cur_mod_info;
    int32_t result;
    bool bresult = 0;
    enum NORFLASH_API_RET_T ret;

    NORFLASH_API_TRACE(4,"%s: mod_id = %d,start_addr = 0x%x,len = 0x%x.",
                __func__,mod_id,start_addr,len);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s :module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(!ADDR_IS_VALIDE(mod_info->mod_base_addr, mod_info->mod_len, start_addr, len))
    {
        NORFLASH_API_TRACE(2,"%s : writting out of range! start_address = 0x%x,len = 0x%x.",
                    __func__,mod_info->mod_base_addr,mod_info->mod_len);
        return NORFLASH_API_BAD_ADDR;
    }

    if((len == 0)
        || !IN_ONE_SECTOR(start_addr,len, flash_sector_size[mod_info->dev_id]))
    {
        NORFLASH_API_TRACE(2,"%s : len error! start_addr = 0x%x, len = %d, flash_sector_size:%d",
                   __func__, start_addr, len, flash_sector_size[mod_info->dev_id]);
        return NORFLASH_API_BAD_LEN;
    }
    _norflash_api_mutex_wait();
#if defined(FLASH_API_SIMPLE)
    async = false;
#endif
    if(async)
    {
        // add to opera_info chain header.
        _norflash_api_int_lock(mod_info->dev_id);
        result = _w_opera_add(mod_info,start_addr,len,(uint8_t*)buffer);
        if(result == 0)
        {
            ret = NORFLASH_API_OK;
        }
        else
        {
            ret = NORFLASH_API_BUFFER_FULL;
        }
        _norflash_api_int_unlock(mod_info->dev_id);
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
        _norflash_api_hook_activate();
#endif
        NORFLASH_API_TRACE(4,"%s: _w_opera_add done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    else
    {
        _norflash_api_int_lock(mod_info->dev_id);

        // flush the opera of currently being processed.
        if(norflash_api_info.cur_mod != NULL
            && mod_info != norflash_api_info.cur_mod)
        {
            cur_mod_info = norflash_api_info.cur_mod;
            while(cur_mod_info->state != NORFLASH_API_STATE_IDLE)
            {
                if(!_flush_is_allowed())
                {
                    continue;
                }
                bresult = _opera_flush(cur_mod_info,true);
                if(!bresult)
                {
                    norflash_api_info.cur_mod = NULL;
                }
            }
        }

        // flush all of cur module opera.
        do{
            if(!_flush_is_allowed())
            {
                continue;
            }
            bresult = _opera_flush(mod_info,true);
        }while(bresult);
        _protection_part(mod_info->dev_id,start_addr,len);
        FLASH_REMAP_START(mod_info->dev_id,start_addr,len);
        PMU_FLASH_WRITE_CONFIG_FUNC();
        result = hal_norflash_write(mod_info->dev_id,start_addr, buffer, len);
        PMU_FLASH_READ_CONFIG_FUNC();
        FLASH_REMAP_DONE(mod_info->dev_id,start_addr,len);
        _protection_all(mod_info->dev_id);
        if(result == HAL_NORFLASH_OK)
        {
            ret = NORFLASH_API_OK;
        }
        else if(result == HAL_NORFLASH_BAD_ADDR)
        {
            ret = NORFLASH_API_BAD_ADDR;
        }
        else if(result == HAL_NORFLASH_BAD_LEN)
        {
            ret = NORFLASH_API_BAD_LEN;
        }
        else
        {
            ret = NORFLASH_API_ERR;
        }
        _cache_invalid(mod_info->dev_id, start_addr, len);
        _norflash_api_int_unlock(mod_info->dev_id);
        NORFLASH_API_TRACE(4,"%s: hal_norflash_write done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    _norflash_api_mutex_release();
    return ret;
}

// 0:all pending flash op flushed, 1:still pending flash op to be flushed
int norflash_api_flush(void)
{
    int ret;

    _norflash_api_mutex_wait();
    ret = _norflash_api_flush();
    _norflash_api_mutex_release();
    return ret;
}

// 0:all pending flash op flushed, 1:still pending flash op to be flushed
static int _norflash_api_exec_flush_hook(void)
{
    uint32_t i;
    int ret;
    bool hook_done;

    do
    {
        hook_done = true;
        for(i = 0; i < NORFLASH_API_HOOK_USER_QTY; i++)
        {
            if(_norflash_api_hook[i].hook_handle)
            {
                ret = _norflash_api_hook[i].hook_handle();
                if(ret)
                {
#if !(defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK))
                    break;
#endif
                    hook_done = false;
                }
            }
        }
    }while(!hook_done);

    return hook_done ? 0 : 1;
}

bool norflash_api_buffer_is_free(
                enum NORFLASH_API_MODULE_ID_T mod_id)
{
    MODULE_INFO *mod_info;
    uint32_t count;

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }
    _norflash_api_mutex_wait();
    count = _get_ew_count(mod_info);
    _norflash_api_mutex_release();
    if(count > 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

uint32_t norflash_api_get_used_buffer_count(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                enum NORFLASH_API_OPRATION_TYPE type
                )
{
    MODULE_INFO *mod_info;
    uint32_t count = 0;

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }
    _norflash_api_mutex_wait();
    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }
    if(type & NORFLASH_API_WRITTING)
    {
        count = _get_w_count(mod_info);
    }

    if(type & NORFLASH_API_ERASING)
    {
        count += _get_e_count(mod_info);
    }
    _norflash_api_mutex_release();
    return count;
}

uint32_t norflash_api_get_free_buffer_count(
                enum HAL_FLASH_ID_T dev_id,
                enum NORFLASH_API_OPRATION_TYPE type
                )
{
    MODULE_INFO *mod_info;
    uint32_t i;
    uint32_t used_count = 0;
    uint32_t free_count = 0;
    uint32_t total_count;

    _norflash_api_mutex_wait();
    if(type & NORFLASH_API_WRITTING)
    {
        for(i = NORFLASH_API_MODULE_ID_LOG_DUMP; i < NORFLASH_API_MODULE_ID_COUNT; i ++)
        {
            mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
            if(mod_info->is_registered && mod_info->dev_id == dev_id)
            {
                used_count += _get_w_count(mod_info);
            }
        }
        total_count = _norflash_api_opera_num(dev_id);
        ASSERT(used_count <= total_count,"writting opra count error!");
        free_count += (total_count - used_count);
    }

    if(type & NORFLASH_API_ERASING)
    {
        for(i = NORFLASH_API_MODULE_ID_LOG_DUMP; i < NORFLASH_API_MODULE_ID_COUNT; i ++)
        {
            mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
            if(mod_info->is_registered && mod_info->dev_id == dev_id)
            {
                used_count += _get_e_count(mod_info);
            }
        }
        total_count = _norflash_api_buffer_num(dev_id);
        ASSERT(used_count <= total_count,"erase opra count error!");
        free_count += (total_count - used_count);
    }
    _norflash_api_mutex_release();
    return free_count;
}


void norflash_api_flush_all(void)
{
    int ret;
    int cnt = 0;

    norflash_api_flush_enable_all();
    do
    {
        ret = norflash_api_flush();
        if(ret == 1)
        {
            cnt ++;
        }
    } while (1 == ret);

    NORFLASH_API_TRACE(2,"%s: done. cnt = %d.",__func__,cnt);
}

void norflash_api_flush_disable(enum NORFLASH_API_USER user_id,uint32_t cb)
{
    if(!norflash_api_info.is_inited)
    {
        return;
    }
    ASSERT(user_id < NORFLASH_API_USER_COUNTS, "%s: user_id(%d) error!", __func__, user_id);
    _flush_disable(user_id,cb);
}

void norflash_api_flush_enable(enum NORFLASH_API_USER user_id)
{
    if(!norflash_api_info.is_inited)
    {
        return;
    }
    ASSERT(user_id < NORFLASH_API_USER_COUNTS, "%s: user_id(%d) too large!", __func__, user_id);
    _flush_enable(user_id);
}

void norflash_api_flush_enable_all(void)
{
    uint32_t user_id;

    if(!norflash_api_info.is_inited)
    {
        return;
    }
    for(user_id = NORFLASH_API_USER_CP; user_id < NORFLASH_API_USER_COUNTS; user_id ++)
    {
        _flush_enable((enum NORFLASH_API_USER)user_id);
    }
}


enum NORFLASH_API_STATE norflash_api_get_state(enum NORFLASH_API_MODULE_ID_T mod_id)
{
    ASSERT(mod_id < NORFLASH_API_MODULE_ID_COUNT,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    return norflash_api_info.mod_info[mod_id].state;
}

void norflash_flush_all_pending_op(void)
{
    norflash_api_flush_all();
}

enum NORFLASH_API_RET_T norflash_api_get_base_addr(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t *addr)
{
    ASSERT(addr, "null pointer received in %s", __func__);
    *addr = 0;

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(2,"%s: %d, norflash_api uninit!",__func__,__LINE__);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    MODULE_INFO *mod_info =_get_module_info(mod_id);
    if(mod_info->is_registered)
    {
        *addr = mod_info->mod_base_addr;
        return NORFLASH_API_OK;
    }
    else
    {
        NORFLASH_API_TRACE(2,"%s: %d, norflash_api uninit!",__func__,__LINE__);
        return NORFLASH_API_ERR_UNINIT;
    }
}

enum NORFLASH_API_RET_T norflash_api_get_dev_id(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                enum HAL_FLASH_ID_T *dev_id)
{
    ASSERT(dev_id, "null pointer received in %s", __func__);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(2,"%s: %d, norflash_api uninit!",__func__,__LINE__);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    MODULE_INFO *mod_info =_get_module_info(mod_id);
    if(mod_info->is_registered)
    {
        *dev_id = mod_info->dev_id;
        return NORFLASH_API_OK;
    }
    else
    {
        NORFLASH_API_TRACE(2,"%s: %d, norflash_api uninit!",__func__,__LINE__);
        return NORFLASH_API_ERR_UNINIT;
    }
}

uint32_t norflash_api_get_total_size(enum HAL_FLASH_ID_T dev_id)
{
    return flash_total_size[dev_id];
}

uint32_t norflash_api_get_block_size(enum HAL_FLASH_ID_T dev_id)
{
    return flash_block_size[dev_id];
}

uint32_t norflash_api_get_sector_size(enum HAL_FLASH_ID_T dev_id)
{
    return flash_sector_size[dev_id];
}

uint32_t norflash_api_get_page_size(enum HAL_FLASH_ID_T dev_id)
{
    return flash_page_size[dev_id];
}

enum NORFLASH_API_RET_T norflash_api_remap_start(
                         enum NORFLASH_API_MODULE_ID_T mod_id,
                         uint32_t start_addr,
                         uint32_t len)
{
    MODULE_INFO *mod_info;

    NORFLASH_API_TRACE(4,"%s:mod_id = %d,start_addr = 0x%x,len = 0x%x",
                __func__,mod_id, start_addr, len);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(!ADDR_IS_VALIDE(mod_info->mod_base_addr, mod_info->mod_len, start_addr, len))
    {
        NORFLASH_API_TRACE(3,"%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__, start_addr, len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(len == 0)
    {
        NORFLASH_API_TRACE(2,"%s : len error! len = %d.",
                   __func__, len);
        return NORFLASH_API_BAD_LEN;
    }
    FLASH_REMAP_START(mod_info->dev_id, start_addr, len);
    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_remap_done(
                         enum NORFLASH_API_MODULE_ID_T mod_id,
                         uint32_t start_addr,
                         uint32_t len)
{
    MODULE_INFO *mod_info;

    NORFLASH_API_TRACE(4,"%s:mod_id = %d,start_addr = 0x%x,len = 0x%x",
                __func__,mod_id, start_addr, len);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(!ADDR_IS_VALIDE(mod_info->mod_base_addr, mod_info->mod_len, start_addr, len))
    {
        NORFLASH_API_TRACE(3,"%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__, start_addr, len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(len == 0)
    {
        NORFLASH_API_TRACE(2,"%s : len error! len = %d.",
                   __func__, len);
        return NORFLASH_API_BAD_LEN;
    }

    FLASH_REMAP_DONE(mod_info->dev_id, start_addr, len);
    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_get_remap_info(
                         enum NORFLASH_API_MODULE_ID_T mod_id,
                         uint8_t *remap_opened,
                         uint8_t *remap_status,
                         uint32_t *remap_addr,
                         uint32_t *remap_len)
{
    MODULE_INFO *mod_info;

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE(1,"%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE(2,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_registered)
    {
        NORFLASH_API_TRACE(2,"%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }
#ifdef FLASH_REMAP
    *remap_opened = 1;
    uint8_t status = hal_norflash_get_remap_status(mod_info->dev_id, NORFLASH_API_REMAP_ID);
    if(status == 0 || status == 1)
    {
        *remap_status = status;

    }
    else
    {
        NORFLASH_API_TRACE(2,"%s : get remap status fail! mod_id = %d, remap_status = %d.",
                          __func__, mod_id, status);
        return NORFLASH_API_ERR_REMAP;
    }
    *remap_addr = OTA_CODE_OFFSET;
    *remap_len = OTA_REMAP_OFFSET;
#else
    *remap_opened = 0;
#endif
    return NORFLASH_API_OK;
}

void norflash_api_flash_operation_start(uint32_t addr)
{
#if defined(RTOS) && defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(norflash_api_get_dev_id_by_addr(addr) == HAL_FLASH_ID_1)
    {
        _norflash_api_mutex_wait();
    }
    else
#endif
    {
        _norflash_api_int_lock(norflash_api_get_dev_id_by_addr(addr));
    }
}

void norflash_api_flash_operation_end(uint32_t addr)
{
#if defined(RTOS) && defined(FLASH1_CTRL_BASE) && defined(USE_MULTI_FLASH)
    if(norflash_api_get_dev_id_by_addr(addr) == HAL_FLASH_ID_1)
    {
        _norflash_api_mutex_release();
    }
    else
#endif
    {
        _norflash_api_int_unlock(norflash_api_get_dev_id_by_addr(addr));
    }
}

bool norflash_api_flush_is_in_task(void)
{
#if defined(RTOS) && !defined(NORFLASH_API_FLUSH_IN_SLEEP_HOOK)
    return true;
#else
    return false;
#endif
}

#ifdef FLASH_SECURITY_REGISTER
static inline enum NORFLASH_API_RET_T RET_VALUE(enum HAL_NORFLASH_RET_T err_code)
{
	enum NORFLASH_API_RET_T ret;

	switch(err_code)
	{
		case HAL_NORFLASH_OK:
			ret = NORFLASH_API_OK;
			break;
		case HAL_NORFLASH_BAD_OP:
			ret = NORFLASH_API_ERR_REGISTRATION;
			break;
		case HAL_NORFLASH_BAD_ADDR:
			ret = NORFLASH_API_BAD_ADDR;
			break;
		case NORFLASH_API_BAD_LEN:
			ret = NORFLASH_API_BAD_LEN;
			break;
		default:
			ret = NORFLASH_API_ERR;
			break;
	}
	return ret;
}

enum NORFLASH_API_RET_T norflash_api_security_register_lock(enum HAL_FLASH_ID_T id, uint32_t start_address, uint32_t len)
{
	uint32_t lock;
	enum HAL_NORFLASH_RET_T err_code;
	enum NORFLASH_API_RET_T ret;

	lock = int_lock_global();
	err_code = hal_norflash_security_register_lock(id, start_address, len);
	int_unlock_global(lock);

	ret = RET_VALUE(err_code);
	return ret;
}

enum NORFLASH_API_RET_T norflash_api_security_register_erase(enum HAL_FLASH_ID_T id, uint32_t start_address, uint32_t len)
{
	uint32_t lock;
	enum HAL_NORFLASH_RET_T err_code;
	enum NORFLASH_API_RET_T ret;

	lock = int_lock_global();
	err_code = hal_norflash_security_register_erase(id, start_address, len);
	int_unlock_global(lock);

	ret = RET_VALUE(err_code);
	return ret;
}

enum NORFLASH_API_RET_T norflash_api_security_register_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
	uint32_t lock;
	enum HAL_NORFLASH_RET_T err_code;
	enum NORFLASH_API_RET_T ret;

	lock = int_lock_global();
	err_code = hal_norflash_security_register_write(id, start_address, buffer, len);
	int_unlock_global(lock);

	ret = RET_VALUE(err_code);
	return ret;
}

enum NORFLASH_API_RET_T norflash_api_security_register_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
	uint32_t lock;
	enum HAL_NORFLASH_RET_T err_code;
	enum NORFLASH_API_RET_T ret;

	lock = int_lock_global();
	err_code = hal_norflash_security_register_read(id, start_address, buffer, len);
	int_unlock_global(lock);

	ret = RET_VALUE(err_code);
	return ret;
}
#endif



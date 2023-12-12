#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_audio.h"
#include "app_through_put.h"
#include "bluetooth_bt_api.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#include "dip_api.h"
#include "app_dip.h"
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"
#include "app_ai_if_config.h"
#include "app_ai_if_thirdparty.h"
#include "app_ai_if_custom_ui.h"
#include "app_bt.h"

#ifdef __IAG_BLE_INCLUDE__
#include "bluetooth_ble_api.h"
#endif

#ifdef __AI_VOICE__
#ifdef __IAG_BLE_INCLUDE__
#include "app_ai_ble.h"
#endif
#include "spp_api.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#include "app_fp_rfcomm.h"
#include "ai_spp.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_bt.h"
#endif

#ifdef IBRT
#include "app_ibrt_internal.h"
#endif

#define CASE_S(s) case s:return "["#s"]";
#define CASE_D()  default:return "[INVALID]";

AI_CAPTURE_BUFFER_T app_ai_if_capture_buf;

const char *ai_spec_type2str(AI_SPEC_TYPE_E ai_spec)
{
    switch(ai_spec) {
        CASE_S(AI_SPEC_INIT)
        CASE_S(AI_SPEC_GSOUND)
        CASE_S(AI_SPEC_AMA)
        CASE_S(AI_SPEC_BES)
        CASE_S(AI_SPEC_BAIDU)
        CASE_S(AI_SPEC_TENCENT)
        CASE_S(AI_SPEC_ALI)
        CASE_S(AI_SPEC_COMMON)
        CASE_D()
    }
}

bool ai_if_is_ai_stream_mic_open(void)
{
    bool mic_open = false;

#ifdef __AI_VOICE__
    mic_open = app_ai_voice_is_mic_open();
#endif

    return mic_open;
}

uint8_t app_ai_if_get_ble_connection_index(void)
{
    uint8_t ble_connection_index = 0xFF;

#if defined(__AI_VOICE__) && defined(__IAG_BLE_INCLUDE__)
    ble_connection_index = app_ai_ble_get_connection_index(ai_manager_get_foreground_ai_conidx());
#endif

    return ble_connection_index;
}

static void app_ai_if_set_ai_spec(uint32_t *ai_spec, uint32_t set_spec)
{
    if(*ai_spec == AI_SPEC_INIT)
    {
        *ai_spec = set_spec;
    }
    else
    {
        *ai_spec = (*ai_spec << 8) | set_spec;
        TRACE(2, "%s, AI SPEC = %04x", __func__, *ai_spec);
    }
}

uint32_t app_ai_if_get_ai_spec(void)
{
    uint32_t ai_spec = AI_SPEC_INIT;

    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_INIT);//Avoid not using this function in some cases, resulting in compilation failure

#ifdef __AMA_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_AMA);
#endif
#ifdef __DMA_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_BAIDU);
#endif
#ifdef __SMART_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_BES);
#endif
#ifdef __TENCENT_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_TENCENT);
#endif
#ifdef __GMA_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_ALI);
#endif
#ifdef __CUSTOMIZE_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_COMMON);
#endif
#ifdef DUAL_MIC_RECORDING
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_RECORDING);
#endif
    //TRACE(2, "%s, AI SPEC = %04x", __func__, ai_spec);
    return ai_spec;
}

#define     AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE           (0x00)
static uint32_t ai_capture_mempool_user_addr[AI_CAPTURE_MEMPOOL_USER_MAX_COUNT] = {
    [AI_CAPTURE_MEMPOOL_USER_INVALID]                       = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_AUDIOFLINGER]                  = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_PCM_CACHE]                     = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_HOTWORD_ENGINE]                = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_VAD]                           = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ANC_ASSIST]                    = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_STORAGE]                = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_ONE_FRAME]              = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_STEREO_M_STORAGE]       = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_STEREO_S_STORAGE]       = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_STEREO_PROCESS_HEAP]    = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_OPUS_HEAP]              = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ENCODE_OPUS_STACK]             = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_ANC_ASSIST_RESAMPLE]           = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
    [AI_CAPTURE_MEMPOOL_USER_HOTWORD_MODELFILE]             = AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE,
};

static POSSIBLY_UNUSED void app_ai_if_mempool_init(void)
{
    uint8_t *buf = NULL;
    app_capture_audio_mempool_get_buff(&buf, APP_CAPTURE_AUDIO_BUFFER_SIZE);

    memset((uint8_t *)buf, 0, APP_CAPTURE_AUDIO_BUFFER_SIZE);

    app_ai_if_capture_buf.buff = buf;
    app_ai_if_capture_buf.buff_size_total = APP_CAPTURE_AUDIO_BUFFER_SIZE;
    app_ai_if_capture_buf.buff_size_used = 0;
    app_ai_if_capture_buf.buff_size_free = APP_CAPTURE_AUDIO_BUFFER_SIZE;

    memset(ai_capture_mempool_user_addr, AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE, sizeof(ai_capture_mempool_user_addr));
    TRACE(3, "%s buf %p size %d", __func__, buf, APP_CAPTURE_AUDIO_BUFFER_SIZE);
}

void app_ai_if_mempool_deinit(void)
{
    TRACE(2, "%s size %d", __func__, app_ai_if_capture_buf.buff_size_total);

    memset((uint8_t *)app_ai_if_capture_buf.buff, 0, app_ai_if_capture_buf.buff_size_total);

    app_ai_if_capture_buf.buff_size_used = 0;
    app_ai_if_capture_buf.buff_size_free = app_ai_if_capture_buf.buff_size_total;
    memset(ai_capture_mempool_user_addr, AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE, sizeof(ai_capture_mempool_user_addr));
}

void app_ai_if_mempool_get_buff(uint8_t **buff, uint32_t size, uint8_t user_index)
{
    uint32_t buff_size_free;

    buff_size_free = app_ai_if_capture_buf.buff_size_free;

    if (user_index >= AI_CAPTURE_MEMPOOL_USER_MAX_COUNT)
    {
        ASSERT(0, "Invalid user request to ai mempool");
    }
    if (ai_capture_mempool_user_addr[user_index] != AI_CAPTURE_MEMPOOL_ADDR_INVALID_VALUE)
    {
        *buff = (uint8_t *)ai_capture_mempool_user_addr[user_index];
        TRACE(2, "user(%d) has allocated mem:%p", user_index, *buff);
        return;
    }
    if (size % 4){
        size = size + (4 - size % 4);
    }

    TRACE(3,"%s free %d to allocate %d", __func__, buff_size_free, size);

    ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__, size, buff_size_free);

    *buff = app_ai_if_capture_buf.buff + app_ai_if_capture_buf.buff_size_used;

    app_ai_if_capture_buf.buff_size_used += size;
    app_ai_if_capture_buf.buff_size_free -= size;

    ai_capture_mempool_user_addr[user_index] = (uint32_t)(*buff);

    TRACE(5, "AI allocate %d, now used %d left %d user(%d) addr:0x%08x", size,
          app_ai_if_capture_buf.buff_size_used,
          app_ai_if_capture_buf.buff_size_free, user_index, ai_capture_mempool_user_addr[user_index]);
}

void app_ai_if_mobile_connect_handle(void *addr)
{
    POSSIBLY_UNUSED bt_bdaddr_t * _addr = (bt_bdaddr_t *)addr;

#ifdef BISTO_ENABLED
    gsound_custom_bt_link_connected_handler(_addr->address);
#endif

#ifdef BT_DIP_SUPPORT
    POSSIBLY_UNUSED MOBILE_CONN_TYPE_E type = MOBILE_CONNECT_IDLE;
    bt_dip_pnp_info_t pnp_info;
    pnp_info.vend_id = 0;

    bool isExsit = nv_record_get_pnp_info(_addr, &pnp_info);
    TRACE(3, "%s vend_id 0x%x vend_id_source 0x%x", __func__, 
        pnp_info.vend_id, pnp_info.vend_id_source);
    if (isExsit)
    {
        type = btif_dip_check_is_ios_by_vend_id(pnp_info.vend_id, pnp_info.vend_id_source)?MOBILE_CONNECT_IOS:MOBILE_CONNECT_ANDROID;
#ifdef __AI_VOICE__
        app_ai_mobile_connect_handle(type, _addr->address);
#endif
#ifdef BISTO_ENABLED
        gsound_mobile_type_get_callback(type);
#endif
    }
#endif

#ifdef __IAG_BLE_INCLUDE__
    bes_ble_gap_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
}

void app_ai_if_hfp_connected_handler(uint8_t device_id)
{
#ifdef __AI_VOICE__
    uint8_t ai_index = 0;

    for (uint8_t ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        ai_index = app_ai_get_ai_index_from_connect_index(ai_connect_index);

        if(ai_index)
        {
            AI_connect_info *ai_info = app_ai_get_connect_info(ai_connect_index);
            if(ai_info) {
                uint8_t dest_id = app_ai_get_dest_id(ai_info);
                ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0, ai_index, dest_id);
            }
        }
    }
#endif
}

void app_ai_voice_stay_active(uint8_t aiIndex)
{
#ifdef __AI_VOICE__
#ifdef IBRT
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        if (app_tws_ibrt_tws_link_connected() && \
            (p_ibrt_ctrl->nv_role == IBRT_MASTER) && \
            p_ibrt_ctrl->p_tws_remote_dev)
        {
            btif_me_stop_sniff(p_ibrt_ctrl->p_tws_remote_dev);
        }
    
        uint8_t* mobileAddr;
        struct BT_DEVICE_T* pBtdev = app_bt_get_device(app_ai_get_device_id_from_index(aiIndex));
        if (pBtdev && (pBtdev->acl_is_connected))
        {
            mobileAddr = pBtdev->remote.address;
            app_ibrt_if_prevent_sniff_set(mobileAddr, AI_VOICE_RECORD);
        }

#else
        app_bt_active_mode_set(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
#endif
#endif
}

void app_ai_voice_resume_sleep(uint8_t aiIndex)
{
#ifdef __AI_VOICE__
#ifdef IBRT
    uint8_t* mobileAddr;
    struct BT_DEVICE_T* pBtdev = app_bt_get_device(app_ai_get_device_id_from_index(aiIndex));
    if (pBtdev && (pBtdev->acl_is_connected))
    {
        mobileAddr = pBtdev->remote.address;
        app_ibrt_if_prevent_sniff_clear(mobileAddr, AI_VOICE_RECORD);
    }

#else
    app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
#endif
#endif
}

void app_ai_key_event_handle(void *param1, uint32_t param2)
{
}

#if defined(AI_KWS_ENGINE_OVERLAY)
static share_mem_pool_monolized_info_t monlized_info;
static ai_kws_engine_overlay_buffer_info_t kws_overlay_buffer_info;
static uint8_t ai_kws_engine_overlay_buffer[APP_AI_SHARE_MEM_POOL_MAXIAM_SIZE];

osMutexDef(app_ai_share_mem_op_mutex);
static osMutexId app_ai_share_mem_op_mutex_id = NULL;

static inline void app_ai_share_mem_pool_monolized_info_op_mutex_alloc(void)
{
    if (app_ai_share_mem_op_mutex_id == NULL) {
        app_ai_share_mem_op_mutex_id = osMutexCreate(osMutex(app_ai_share_mem_op_mutex));
    }
}

static inline void app_ai_share_mem_pool_monolized_info_op_mutex_req(void)
{
    osMutexWait(app_ai_share_mem_op_mutex_id, osWaitForever);
}

static inline void app_ai_share_mem_pool_monolized_info_op_mutex_rls(void)
{
    osMutexRelease(app_ai_share_mem_op_mutex_id);
}

void app_ai_share_mem_pool_info_print(void)
{
    TRACE(0,"%s info :",__func__);
    TRACE(3,"%s ai share mem:%d %d ",__func__,app_ai_share_mem_pool_total_size(),app_ai_share_mem_pool_free_size());
}

void app_ai_share_mem_pool_monolized_info_print(void)
{
    TRACE(2,"%s cur monolized user = %s",__func__,ai_spec_type2str(monlized_info.monolized_user));
    for(uint8_t i = AI_SPEC_INIT;i<AI_SPEC_COUNT;i++){
        if(monlized_info.user[i].used == true){
            TRACE(4,"%s %p",ai_spec_type2str(monlized_info.user[i].spec),monlized_info.user[i].callback);
            DUMP32("%d ",&monlized_info.user[i].size_cost_info,sizeof(ai_spec_size_cost_t)/sizeof(uint32_t));
        }
    }
}

void app_ai_share_mem_pool_monilized_info_init(void)
{
    app_ai_share_mem_pool_monolized_info_op_mutex_alloc();

    monlized_info.monolized_user = AI_SPEC_COUNT;
    for(uint8_t i = AI_SPEC_INIT;i<AI_SPEC_COUNT;i++){
        monlized_info.user[i].spec = (AI_SPEC_TYPE_E)i;
        memset(&monlized_info.user[i].size_cost_info,0,sizeof(ai_spec_size_cost_t));
        monlized_info.user[i].used = 0;
    }
}

/*
    if user switch, notify the previous app layer & current app layer
    record the share mem usage info
*/
static void app_ai_share_mem_pool_monolized_info_update(AI_SPEC_TYPE_E user,uint32_t size)
{
    AI_SPEC_TYPE_E cur_user = monlized_info.monolized_user;

    TRACE(1,"%s",__func__);

    if(user >= AI_SPEC_COUNT){
        TRACE(4,"info %d %s %s",size,ai_spec_type2str(user),ai_spec_type2str(cur_user));
        return;
    }

    if((user != monlized_info.monolized_user) && (cur_user < AI_SPEC_COUNT)){
        monlized_info.user[cur_user].used = false;
        memset(&monlized_info.user[cur_user].size_cost_info,0,sizeof(ai_spec_size_cost_t));
        if(monlized_info.user[cur_user].callback){
            TRACE(0,"switch...");
            monlized_info.user[cur_user].callback(user,cur_user);
        }
    }

    if(monlized_info.user[user].callback){
        monlized_info.user[user].callback(user,cur_user);
    }

    TRACE(0,"switch done...");

    cur_user = user;
    monlized_info.monolized_user = cur_user;
    monlized_info.user[cur_user].used = true;

    //size turn round.
    if(monlized_info.user[cur_user].size_cost_info.size_cost_phase_1 == 0){
        monlized_info.user[cur_user].size_cost_info.size_cost_phase_1 = size;
    }else if(monlized_info.user[cur_user].size_cost_info.size_cost_phase_2 == 0){
        monlized_info.user[cur_user].size_cost_info.size_cost_phase_2 = size;
    }else if(monlized_info.user[cur_user].size_cost_info.size_cost_phase_3 == 0){
        monlized_info.user[cur_user].size_cost_info.size_cost_phase_2 = size;
    }else if(monlized_info.user[cur_user].size_cost_info.size_cost_phase_4 == 0){
        monlized_info.user[cur_user].size_cost_info.size_cost_phase_2 = size;
    }else{
        monlized_info.user[cur_user].size_cost_info.size_cost_phase_1 = size;
    } 

}

void app_ai_share_mem_pool_init(void)
{
    app_ai_share_mem_pool_monolized_info_op_mutex_req();

    memset(ai_kws_engine_overlay_buffer,0,APP_AI_SHARE_MEM_POOL_MAXIAM_SIZE);
    memset((uint8_t*)&(kws_overlay_buffer_info),0,sizeof(ai_kws_engine_overlay_buffer_info_t));
    kws_overlay_buffer_info.buffer_ptr = (uint32_t)ai_kws_engine_overlay_buffer;
    kws_overlay_buffer_info.total_bytes = APP_AI_SHARE_MEM_POOL_MAXIAM_SIZE;

    app_ai_share_mem_pool_monolized_info_op_mutex_rls();
}

uint32_t app_ai_share_mem_pool_total_size(void)
{
    return kws_overlay_buffer_info.total_bytes;
}

uint32_t app_ai_share_mem_pool_free_size(void)
{
    uint32_t free_bytes = 0;
    app_ai_share_mem_pool_monolized_info_op_mutex_req();

    free_bytes = kws_overlay_buffer_info.total_bytes - kws_overlay_buffer_info.cost_bytes;

    app_ai_share_mem_pool_monolized_info_op_mutex_rls();

    return free_bytes;
}

void app_ai_share_pool_rebuild(void)
{
    app_ai_share_mem_pool_init();
}

bool app_ai_share_mem_pool_monolize(AI_SPEC_TYPE_E user,uint8_t**buf,uint32_t size)
{
    bool ret = false;

    uint32_t free_size = app_ai_share_mem_pool_free_size();

    size = (size + 3) & ~3;


    app_ai_share_mem_pool_monolized_info_op_mutex_req();

    if(free_size >= size){
        app_ai_share_mem_pool_monolized_info_update(user,size);
        kws_overlay_buffer_info.cost_bytes += size;
        kws_overlay_buffer_info.buffer_ptr += size;
        ret = true;
    }
    app_ai_share_mem_pool_monolized_info_op_mutex_rls();

    return ret;
}

bool app_ai_share_mem_pool_monolize_force(AI_SPEC_TYPE_E user,uint8_t**buf,uint32_t size)
{
    bool force = false;
    if(app_ai_share_mem_pool_total_size() < size){
        ASSERT(0,"%s total[%d] < size[%d] ",__func__,app_ai_share_mem_pool_total_size(),size);
    }

    uint32_t free_size = app_ai_share_mem_pool_free_size();

    size = (size + 3) & ~3;

    app_ai_share_mem_pool_monolized_info_op_mutex_req();

    if(free_size < size){
        TRACE(1,"%s rebuild %d %d",__func__,free_size,size);
        app_ai_share_pool_rebuild();
        force = true;
    }

    app_ai_share_mem_pool_monolized_info_update(user,size);
    *buf = (uint8_t*)(kws_overlay_buffer_info.buffer_ptr);
    kws_overlay_buffer_info.buffer_ptr += size;
    kws_overlay_buffer_info.cost_bytes += size;

    app_ai_share_mem_pool_monolized_info_op_mutex_rls();

    return force;
}

void app_ai_share_mem_pool_monolized_info_update_notify_callback_reg(AI_SPEC_TYPE_E user,uint32_t func)
{
    monlized_info.user[user].callback = (ai_share_mem_monolized_user_update_callback_t)func;
}

#endif

void app_ai_init(void)
{
#ifdef __AI_VOICE__
    app_ai_manager_init();
#if defined(AI_KWS_ENGINE_OVERLAY)
    app_ai_share_mem_pool_init();
    app_ai_share_mem_pool_monilized_info_init();
    app_ai_share_mem_pool_info_print();
#endif

#ifdef BISTO_ENABLED
    bool gsound_enable = true;
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (app_ai_manager_is_in_multi_ai_mode())
    {
        if ((app_ai_manager_get_current_spec() == AI_SPEC_AMA))
        {
            TRACE(1,"%s set gsound false", __func__);
            gsound_enable = false;
        }
    }
#endif    
    gsound_custom_init(gsound_enable);
#endif
#ifdef __AMA_VOICE__
    ai_open_specific_ai(AI_SPEC_AMA);
#endif
#ifdef __DMA_VOICE__
    ai_open_specific_ai(AI_SPEC_BAIDU);
#endif
#ifdef __SMART_VOICE__
    ai_open_specific_ai(AI_SPEC_BES);
#endif
#ifdef __TENCENT_VOICE__
    ai_open_specific_ai(AI_SPEC_TENCENT);
#endif
#ifdef __GMA_VOICE__
    ai_open_specific_ai(AI_SPEC_ALI);
#endif
#ifdef __CUSTOMIZE_VOICE__
    ai_open_specific_ai(AI_SPEC_COMMON);
#endif
#ifdef DUAL_MIC_RECORDING
    ai_open_specific_ai(AI_SPEC_RECORDING);
#endif
#ifdef __BIXBY_VOICE__
    ai_open_specific_ai(AI_SPEC_BIXBY);
#endif

    app_ai_tws_init();
    app_ai_if_custom_init();

#ifdef __THROUGH_PUT__
    app_throughput_test_init();
#endif

    app_ai_voice_thread_init();

    app_capture_audio_mempool_init();

#ifdef __THIRDPARTY
    app_ai_if_thirdparty_init();
#endif

    app_ai_if_mempool_init();
#endif
}

uint8_t app_ai_get_codec_type(uint8_t spec)
{
    /// init as invalid value
    uint8_t codec = 0;

    if (0)
    {
    }
#ifdef __AMA_VOICE__
    else if (AI_SPEC_AMA == spec)
    {
#if defined(AMA_ENCODE_USE_SBC)
        codec = VOC_ENCODE_SBC;
#else
        codec = VOC_ENCODE_OPUS;
#endif
    }
#endif
#ifdef __DMA_VOICE__
    else if (AI_SPEC_BAIDU == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __SMART_VOICE__
    else if (AI_SPEC_BES == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __TENCENT_VOICE__
    else if (AI_SPEC_TENCENT == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __GMA_VOICE__
    else if (AI_SPEC_ALI == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __CUSTOMIZE_VOICE__
    else if (AI_SPEC_COMMON == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef DUAL_MIC_RECORDING
    else if (AI_SPEC_RECORDING == spec)
    {
#ifdef RECORDING_USE_OPUS
    codec = VOC_ENCODE_OPUS;
#else
    codec = VOC_ENCODE_SCALABLE;
#endif
    }
#endif
#ifdef __BIXBY_VOICE__
    else if (AI_SPEC_BIXBY == spec)
    {
        codec = VOC_ENCODE_SBC;
    }
#endif

    return codec;
}

#ifdef IS_MULTI_AI_ENABLED
static AI_OPEN_MIC_USER_E g_ai_open_mic_user = AI_OPEN_MIC_USER_NONE;

void app_ai_open_mic_user_set(AI_OPEN_MIC_USER_E user)
{
    g_ai_open_mic_user = user;
}

AI_OPEN_MIC_USER_E app_ai_open_mic_user_get()
{
    return g_ai_open_mic_user;
}
#endif

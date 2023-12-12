/**
 * @file gaf_media_common.h
 * @author BES AI team
 * @version 0.1
 * @date 2020-08-31
 *
 * @copyright Copyright (c) 2015-2021 BES Technic.
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


#ifndef __GAF_MEDIA_COMMON_H__
#define __GAF_MEDIA_COMMON_H__

/*****************************header include********************************/
#include "cmsis_os.h"
#include "gaf_media_pid.h"
#include "bluetooth_bt_api.h"
#include "gaf_codec_common.h"
#include "cc_stream_common.h"

/******************************macro defination*****************************/
#ifdef LEA_FOUR_CIS_ENABLED
#define GAF_MAXIMUM_CONNECTION_COUNT                        (4)
#else
#define GAF_MAXIMUM_CONNECTION_COUNT                        (2)
#endif

#define GAF_INVALID_ASE_INDEX                               (0xFF)
#define GAF_INVALID_CONNECTION_IDX                          (0xFF)
#define GAF_AUDIO_DFT_PLAYBACK_LIST_IDX                     (0)
// Every stream direction per conn that used ase number cnt max
#define GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT                  (2)

#define GAF_AUDIO_MEDIA_DATA_PACKET_NUM_LIMITER             (15)
#define GAF_AUDIO_MAX_CHANNELS                              (2)
#define GAF_AUDIO_CONTROLLER_2_HOST_LATENCY_US              (2000)
#define GAF_AUDIO_CAPTURE_TRIGGER_GUARD_TIME_US             (2000)
#define GAF_AUDIO_CAPTURE_TRIGGER_DELAY_IN_US               (20000)
#define GAF_AUDIO_DMA_IRQ_HAPPENING_TIME_GAP_TOLERANCE_US   (600)
#define GAF_AUDIO_INVALID_ISO_CHANNEL                       (0xFF)
#define GAF_AUDIO_INVALID_ISO_HANDLE                        (0xFFFF)
#define GAF_AUDIO_INVALID_SEQ_NUMBER                        (0xFFFF)
#define GAF_INVALID_SINK_PLAY_DELAY                         (0xFFFFFFFF)
#define GAF_AUDIO_ALPHA_PRAMS_1                             (3)
#define GAF_AUDIO_ALPHA_PRAMS_2                             (4)
// Max allowed different bt time (2000us)
#define GAF_AUDIO_MAX_DIFF_BT_TIME                          (2000)
// Min need time to set dma (7000us)
#define GAF_AUDIO_DMA_SETUP_MIN_TIME                        (7000)

#define APP_BAP_MAX_ASCS_NB_ASE_CHAR                        (3)

#ifdef AOB_MOBILE_ENABLED
#define GAF_AUDIO_ASE_TOTAL_COUNT                           (APP_BAP_MAX_ASCS_NB_ASE_CHAR * BLE_AUDIO_CONNECTION_CNT)
#define GAF_AUDIO_FIXED_MOBILE_SIDE_PRESENT_LATENCY_US      (30000)
#else
#define GAF_AUDIO_ASE_TOTAL_COUNT                           (APP_BAP_MAX_ASCS_NB_ASE_CHAR)
#endif

#if defined(BLE_AUDIO_CENTRAL_APP_ENABLED) || defined(AOB_LOW_LATENCY_MODE)
#define GAF_MARGIN_BETWEEN_TRIGGER_TIME_AND_CURRENT_TIME_US 5000
#else
#define GAF_MARGIN_BETWEEN_TRIGGER_TIME_AND_CURRENT_TIME_US 10000
#endif

/// Maximum diff value of a 1us Bluetooth clock
#define GAF_AUDIO_MAX_DIFF_TIME                             ((1L<<16) - 1)
#define GAF_AUDIO_CLK_SUB(clock_a, clock_b)                 ((uint32_t)(((clock_a) - (clock_b)) & GAF_AUDIO_MAX_DIFF_TIME))
#define GAF_AUDIO_ABS(value)                                ((value) > 0?(value):(-value))
#define GAF_AUDIO_CLK_DIFF(clock_a, clock_b)                ((GAF_AUDIO_CLK_SUB((clock_b), (clock_a)) > (uint32_t)((GAF_AUDIO_MAX_DIFF_TIME + 1) >> 1)) ? \
                                                             ((int32_t)((-GAF_AUDIO_CLK_SUB((clock_a), (clock_b))))) : ((int32_t)((GAF_AUDIO_CLK_SUB((clock_b), (clock_a))))))

#define GAF_AUDIO_MAX_32_BIT_DIFF_TIME                      (0xFFFFFFFF)
#define GAF_AUDIO_CLK_32_BIT_SUB(clock_a, clock_b)          ((uint64_t)(((clock_a) - (clock_b)) & GAF_AUDIO_MAX_32_BIT_DIFF_TIME))
#define GAF_AUDIO_CLK_32_BIT_DIFF(clock_a, clock_b)         ((GAF_AUDIO_CLK_32_BIT_SUB((clock_b), (clock_a)) > (uint64_t)((GAF_AUDIO_MAX_32_BIT_DIFF_TIME + 1) >> 1)) ? \
                                                             ((int32_t)((-GAF_AUDIO_CLK_32_BIT_SUB((clock_a), (clock_b))))) : ((int32_t)((GAF_AUDIO_CLK_32_BIT_SUB((clock_b), (clock_a))))))


/******************************type defination******************************/
typedef enum
{
    GAF_TIME_STAMP_CHECK_MATCH_EXPECED        = 1 << 0,
    GAF_TIME_STAMP_CHECK_LATER_THAN_EXPECED   = 1 << 1,
    GAF_TIME_STAMP_CHECK_EARLIER_THAN_EXPECED = 1 << 2,
} GAF_TIME_CHECK_RESULT_T;

/// CIS Direction
typedef enum
{
    /// Sink direction
    GAF_LOCAL_DIRECTION_SINK = (1<<0),
    /// Source direction
    GAF_LOCAL_DIRECTION_SRC  = (1<<1),
    /// Sink and Source direction
    GAF_LOCAL_DIRECTION_BOTH  = GAF_LOCAL_DIRECTION_SINK|GAF_LOCAL_DIRECTION_SRC,
} GAF_LOCAL_DIRECTION;

typedef enum
{
    GAF_AUDIO_STREAM_CONTEXT_TYPE_MIN = 0,
    GAF_AUDIO_STREAM_CONTEXT_TYPE_MEDIA         = GAF_AUDIO_STREAM_CONTEXT_TYPE_MIN,
    // FLEXIBLE type means that both playback and capture streams could be
    // started and stopped in the run-time across the context life span
    GAF_AUDIO_STREAM_CONTEXT_TYPE_FLEXIBLE      = 1,
#ifdef AOB_MOBILE_ENABLED
    GAF_AUDIO_STREAM_CONTEXT_TYPE_CALL          = 2,
    GAF_AUDIO_STREAM_CONTEXT_TYPE_AI            = 3,
#endif

    GAF_AUDIO_STREAM_CONTEXT_TYPE_BIS           = 4,

    GAF_AUDIO_CONTEXT_NUM_MAX
} GAF_AUDIO_STREAM_CONTEXT_TYPE_E;

/// HCI ISO_Data_Load - Packet Status Flag
typedef enum
{
    GAF_STREAM_PLAYBACK = (1<<0),
    GAF_STREAM_CAPTURE  = (1<<1),
} GAF_STREAM_TYPE_E;

typedef enum
{
    GAF_STREAM_PLAYBACK_STATUS_IDLE,
    GAF_STREAM_PLAYBACK_STATUS_BUSY,
} GAF_STREAM_PLAYBACK_STATUS_E;

typedef struct
{
    uint32_t sample_rate;
    int32_t num_channels;
    int32_t bits_depth;
    uint32_t frame_samples;
} GAF_AUDIO_INPUT_CONFIG_T;

typedef struct
{
    uint32_t    bypass;
    uint32_t    frame_len;
    uint32_t    algo_frame_len;
    uint32_t    sample_rate;
    uint32_t    channel_num;
    uint32_t    bits;
} GAF_AUDIO_TX_ALGO_CFG_T;

typedef int (*GAF_AUDIO_START_STREAM_FUNC)(void* pStreamEnv);
typedef void (*GAF_AUDIO_INIT_STREAM_BUF_FUNC)(void* pStreamEnv);
typedef int (*GAF_AUDIO_STOP_STREAM_FUNC)(void* pStreamEnv);
typedef uint32_t (*GAF_AUDIO_STREAM_DMA_IRQ_HANDLER)(uint8_t* ptrData, uint32_t dataLength);
typedef void (*GAF_AUDIO_DEINIT_STREAM_BUF_FUNC)(void* pStreamEnv);

typedef void (*GAF_AUDIO_STREAM_RETRIGGER)(void* pStreamEnv, uint32_t streamType);

/*************************************GAF CUSTOM*****************************************/
/**
 * @brief call after a packet received, input received packet ptr
 *
 */
typedef uint8_t (*gaf_stream_recv_data_custom_cb)(const gaf_media_data_t* header);

/**
 * @brief call after a packet decoded, input raw data ptr and len
 *
 */
typedef uint8_t (*gaf_stream_playback_dma_irq_custom_cb)(const gaf_media_data_t* header,
                                                         const uint8_t* raw_data, uint32_t len);

/**
 * @brief call before sending out an encoded packet, input packet encoded data ptr and len
 *
 */
typedef uint8_t (*gaf_stream_send_data_custom_cb)(uint8_t* encoded_data, uint32_t len);

/**
 * @brief call before encoding pcm raw data, input raw data and len
 *
 */
typedef uint8_t (*gaf_stream_capture_dma_irq_custom_cb)(uint8_t* raw_data, uint32_t len);

typedef enum
{
    GAF_AUDIO_UPDATE_STREAM_INFO_TO_START   = 0,
    GAF_AUDIO_UPDATE_STREAM_INFO_TO_STOP    = 1,
} GAF_AUDIO_UPDATE_STREAM_INFO_PURPOSE_E;

typedef enum
{
    GAF_PLAYBACK_STREAM_IDLE                = 0,
    GAF_PLAYBACK_STREAM_INITIALIZING          = 1,
    GAF_PLAYBACK_STREAM_BUF_INITIALIZED     = 2,
    GAF_PLAYBACK_STREAM_INITIALIZED         = 3,
    GAF_PLAYBACK_STREAM_START_TRIGGERING    = 4,
    GAF_PLAYBACK_STREAM_STREAMING_TRIGGERED = 5,
} GAF_PLAYBACK_STREAM_STATE_E;

typedef enum
{
    GAF_CAPTURE_STREAM_IDLE                 = 0,
    GAF_CAPTURE_STREAM_INITIALIZING           = 1,
    GAF_CAPTURE_STREAM_BUF_INITIALIZED      = 2,
    GAF_CAPTURE_STREAM_INITIALIZED          = 3,
    GAF_CAPTURE_STREAM_START_TRIGGERING     = 4,
    GAF_CAPTURE_STREAM_STREAMING_TRIGGERED  = 5,
} GAF_CAPTURE_STREAM_STATE_E;

typedef enum
{
    // music (playback only) or call (playback + capture)
    GAF_AUDIO_TRIGGER_BY_PLAYBACK_STREAM = 0,

    // AI (capture only)
    GAF_AUDIO_TRIGGER_BY_CAPTURE_STREAM  = 1,

    // Game or Live (peripheral role) or Call (Central role)
    GAF_AUDIO_TRIGGER_BY_ISOLATE_STREAM = 2,

} GAF_AUDIO_TRIGGER_STREAM_TYPE_E;

typedef enum
{
    GAF_AUDIO_STREAM_TYPE_PLAYBACK = (1 << 0),
    GAF_AUDIO_STREAM_TYPE_CAPTURE  = (1 << 1),
    GAF_AUDIO_STREAM_TYPE_FLEXIBLE = (1 << 2),
} GAF_AUDIO_STREAM_TYPE_E;

typedef enum
{
    GAF_STREAM_USER_CASE_MIN,
    GAF_STREAM_USER_CASE_UC_SRV = GAF_STREAM_USER_CASE_MIN,
#ifdef AOB_MOBILE_ENABLED
    GAF_STREAM_USER_CASE_UC_CLI,
#endif
    GAF_STREAM_USER_CASE_MAX,
} GAF_STREAM_USER_CASE_E;

typedef struct
{
    uint8_t instance_handle;
    uint32_t core_type;
    INSTANCE_STATUS instance_status;
} CODEC_CORE_INFO_T;

typedef struct
{
    uint16_t    ase_handle;// CIS handle range 0x100~0x200,BIS handle range 0x200~0x300
    uint8_t     iso_channel_hdl;
    uint32_t    allocation_bf;
    CODEC_CORE_INFO_T codec_core_info;
} GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T;

typedef struct
{
    uint16_t    iso_channel_hdl;// CIS handle range 0x100~0x200,BIS handle range 0x200~0x300
    uint16_t    blocks_size;
    uint8_t     select_ch_map;
    uint16_t    encoded_frame_size;
    uint8_t     sdu_num;
    float       frame_ms;
    CODEC_CORE_INFO_T codec_core_info;
} GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T;

typedef struct
{
    uint32_t    sample_rate;
    uint8_t     num_channels;
    uint8_t     bits_depth;
    float       frame_ms;

    uint8_t*    storedDmaBufPtr;
    uint8_t*    dmaBufPtr;
    uint32_t    dmaChunkSize;
    uint32_t    dmaChunkIntervalUs;
} GAF_AUDIO_DMA_INFO_T;

typedef struct
{
    /*
     * Used to record the info of ASE in this scenario.
     * Currently, it is mainly used on the mobile
     */
    GAF_AUDIO_CIS_STREAM_CHANNEL_INFO_T aseChInfo[GAF_AUDIO_ASE_TOTAL_COUNT];
    GAF_AUDIO_BIS_STREAM_CHANNEL_INFO_T bisChInfo[2];

    CODEC_INFO_T codec_info;
    GAF_AUDIO_DMA_INFO_T dma_info;

    uint16_t    maxCachedFrameCount;
    uint32_t    maxFrameSize;

    uint32_t    presDelayUs;
    uint32_t    cigSyncDelayUs;
    uint32_t    isoIntervalUs;
    uint32_t    bnM2S;
    uint32_t    bnS2M;

    uint8_t     trigger_stream_lid;
    uint8_t*    decode_buf;
} GAF_AUDIO_STREAM_COMMON_INFO_T;

typedef struct
{
    // playback stream context
    GAF_PLAYBACK_STREAM_STATE_E playback_stream_state;
    uint16_t                    lastestPlaybackSeqNum[GAF_AUDIO_ASE_TOTAL_COUNT];
    uint32_t                    lastPlaybackDmaIrqTimeUs;
    uint32_t                    playbackTriggerStartTicks;
    uint8_t                     playbackTriggerChannel;

    gaf_media_pid_t             playback_pid_env;
    osTimerId                   playback_trigger_supervisor_timer_id;
    bool                        playback_retrigger_onprocess;

    cfifo*                      playback_frame_fifo[PLAYBACK_INSTANCE_MAX];
    cfifo*                      playback_pcm_fifo[PLAYBACK_INSTANCE_MAX];
    uint8_t*                    playback_frame_buf[PLAYBACK_INSTANCE_MAX];
    uint8_t*                    playback_frame_cache;
    uint8_t*                    playback_pcm_cache;
    bool*                       isDecoding[PLAYBACK_INSTANCE_MAX];

    // capture stream context
    GAF_CAPTURE_STREAM_STATE_E  capture_stream_state;
    uint16_t                    latestCaptureSeqNum;
    uint32_t                    lastCaptureDmaIrqTimeUs;
    uint32_t                    captureTriggerStartTicks;
    uint32_t                    lastCaptureDmaIrqTimeUsInTriggerPoint;
    bool                        isUsSinceLatestAnchorPointConfigured;
    int32_t                     usSinceLatestAnchorPoint;
    int32_t                     captureAverageDmaChunkIntervalUs;
    gaf_media_pid_t             capture_pid_env;
    uint8_t                     captureTriggerChannel;
    // trigger time in us = us converted from master_trigger_base_bt_clk_cnt + trigger delay in us
    uint32_t                    master_trigger_base_bt_clk_cnt;

    // when the captured seq number reaches this value, the upstreaming will be started
    // which will start transmitting from the first sequence packet
    uint32_t                    capturedSeqNumToStartUpStreaming;
    bool                        isUpStreamingStarted;
    uint32_t                    usStorePcmToFetchFrame;
    osTimerId                   capture_trigger_supervisor_timer_id;
    bool                        capture_retrigger_onprocess;

    cfifo*                      capture_frame_fifo[CAPTURE_INSTANCE_MAX];
    cfifo*                      capture_pcm_fifo[CAPTURE_INSTANCE_MAX];
    uint8_t*                    capture_frame_cache;
    uint8_t*                    capture_pcm_cache;
    bool*                       isEncoding[CAPTURE_INSTANCE_MAX];
} GAF_AUDIO_STREAM_CONTEXT_T;

typedef struct
{
    uint8_t codec_type;
    uint8_t con_lid;
    bool is_mobile;
    bool is_bis;
    void* gaf_playback_status_mutex[GAF_AUDIO_ASE_TOTAL_COUNT];

    uint16_t bap_contextType;
    GAF_AUDIO_STREAM_CONTEXT_TYPE_E contextType;
    // see @GAF_AUDIO_STREAM_TYPE_E

    GAF_AUDIO_STREAM_COMMON_INFO_T  playbackInfo;
    GAF_AUDIO_STREAM_COMMON_INFO_T  captureInfo;
    GAF_AUDIO_TX_ALGO_CFG_T         tx_algo_cfg;
} GAF_AUDIO_STREAM_INFO_T;

typedef struct
{
    GAF_AUDIO_START_STREAM_FUNC     start_stream_func;
    GAF_AUDIO_INIT_STREAM_BUF_FUNC  init_stream_buf_func;
    GAF_AUDIO_STOP_STREAM_FUNC      stop_stream_func;

    GAF_AUDIO_STREAM_DMA_IRQ_HANDLER  playback_dma_irq_handler_func;
    GAF_AUDIO_STREAM_DMA_IRQ_HANDLER  capture_dma_irq_handler_func;
    GAF_AUDIO_DEINIT_STREAM_BUF_FUNC  deinit_stream_buf_func;

    GAF_AUDIO_START_STREAM_FUNC     playback_start_stream_func;
    GAF_AUDIO_INIT_STREAM_BUF_FUNC  playback_init_stream_buf_func;
    GAF_AUDIO_STOP_STREAM_FUNC      playback_stop_stream_func;
    GAF_AUDIO_DEINIT_STREAM_BUF_FUNC  playback_deinit_stream_buf_func;

    GAF_AUDIO_START_STREAM_FUNC     capture_start_stream_func;
    GAF_AUDIO_INIT_STREAM_BUF_FUNC  capture_init_stream_buf_func;
    GAF_AUDIO_STOP_STREAM_FUNC      capture_stop_stream_func;
    GAF_AUDIO_DEINIT_STREAM_BUF_FUNC  capture_deinit_stream_buf_func;

} GAF_AUDIO_STREAM_FUNC_LIST_T;

typedef struct
{
    GAF_AUDIO_STREAM_FUNC_LIST_T    stream_func_list;
    const GAF_DECODER_FUNC_LIST_T*  decoder_func_list;
    const GAF_ENCODER_FUNC_LIST_T*  encoder_func_list;
} GAF_AUDIO_FUNC_LIST_T;

typedef struct
{
    // changing value along with streaming
    GAF_AUDIO_STREAM_CONTEXT_T  stream_context;

    // constant value configured when starting stream
    GAF_AUDIO_STREAM_INFO_T     stream_info;

    GAF_AUDIO_FUNC_LIST_T*      func_list;

} GAF_AUDIO_STREAM_ENV_T;

typedef struct
{
    // Number sink ase - 1
    uint8_t playback_ase_id[GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT];
    uint8_t capture_ase_id[GAF_AUDIO_USED_ASE_PER_DIR_MAX_CNT];

    // from GAF_AUDIO_STREAM_TYPE_E
    // when startedStreamTypes becomes the same as included_stream_types,
    // start the context
    uint8_t startedStreamTypes;
} GAF_MEDIA_DWELLING_INFO_T;

typedef struct
{
    // from GAF_AUDIO_STREAM_TYPE_E
    uint8_t included_stream_type;
    // from GAF_AUDIO_TRIGGER_STREAM_TYPE_E
    uint8_t trigger_stream_type;

    uint8_t playback_ase_count;
    uint8_t capture_ase_count;
} GAF_MEDIA_STREAM_TYPE_OPERATION_RULE_T;

typedef void (*GAF_ADUIO_STREAM_INFO_GET)(GAF_AUDIO_STREAM_ENV_T **pStreamEnv);

typedef struct
{
    /// After SINK received a ** codec packet
    gaf_stream_recv_data_custom_cb encoded_packet_recv_cb;
    /// After SINK decode a ** codec packet to raw pcm data
    gaf_stream_playback_dma_irq_custom_cb decoded_raw_data_cb;
    /// Before SRC prepared for sending a ** codec packet
    gaf_stream_send_data_custom_cb encoded_packet_send_cb;
    /// Before SRC prepared for encoding a pcm to ** codec packet
    gaf_stream_capture_dma_irq_custom_cb raw_pcm_data_cb;
} GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T ;

/****************************function declaration***************************/
#ifdef __cplusplus
extern "C"{
#endif

void gaf_stream_register_running_stream_ref(uint8_t con_lid, GAF_AUDIO_STREAM_ENV_T* pStreamEnv);
void gaf_stream_register_retrigger_callback(GAF_AUDIO_STREAM_RETRIGGER retrigger_cb);
void gaf_mobile_stream_register_retrigger_callback(GAF_AUDIO_STREAM_RETRIGGER retrigger_cb);
void gaf_stream_heap_init(void *begin_addr, uint32_t size);
void *gaf_stream_heap_malloc(uint32_t size);
void *gaf_stream_heap_cmalloc(uint32_t size);
void *gaf_stream_heap_realloc(void *rmem, uint32_t newsize);
void gaf_stream_heap_free(void *rmem);
void gaf_stream_data_free(void *packet);
void *gaf_stream_data_frame_malloc(uint32_t packet_len);
int gaf_playback_status_mutex_lock(void *mutex);
int gaf_playback_status_mutex_unlock(void *mutex);
uint32_t gaf_stream_common_sample_freq_parse(uint8_t sample_freq);
float gaf_stream_common_frame_duration_parse(uint8_t frame_duration);
const char* gaf_stream_common_print_code_type(uint8_t codec_id);
const char* gaf_stream_common_print_context(uint16_t context_bf);
void gaf_stream_common_clr_trigger(uint8_t triChannel);
void gaf_stream_reset_retrigger_counter(void);
void gaf_stream_common_update_playback_stream_state(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, GAF_PLAYBACK_STREAM_STATE_E newState);
void gaf_stream_common_update_capture_stream_state(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, GAF_CAPTURE_STREAM_STATE_E newState);
const char* gaf_stream_common_get_capture_stream_state(GAF_CAPTURE_STREAM_STATE_E capture_stream_state);
void gaf_stream_common_set_playback_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t tg_tick);
void gaf_stream_common_set_playback_trigger_time_generic(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint8_t dstStreamType, uint32_t tg_tick);
void gaf_stream_common_set_capture_trigger_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t tg_tick);
void gaf_stream_common_set_capture_trigger_time_generic(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint8_t srcStreamType, uint32_t tg_tick);
void gaf_stream_common_set_capture_trigger_info(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t tg_tick);
void gaf_stream_common_capture_timestamp_checker(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs);
void gaf_stream_common_playback_timestamp_checker(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs);
bool gaf_stream_common_get_packet(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, CC_PLAYBACK_DATA_T *out_frame,
        uint8_t instance_handle, uint32_t dmaIrqHappeningTimeUs, uint32_t sink_play_delay, bool pidAdjust);
void gaf_stream_common_updated_expeceted_playback_seq_and_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint8_t list_idx, uint32_t dmaIrqHappeningTimeUs);
void gaf_stream_common_store_packet(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, CC_PLAYBACK_DATA_T *frame, CODEC_CORE_INFO_T *coreInfo);
bool gaf_stream_common_store_pcm(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint32_t dmairqhappentime,
            uint8_t* ptrBuf, uint32_t length, uint32_t frameLen, CODEC_CORE_INFO_T *coreInfo);
void gaf_stream_common_register_func_list(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, GAF_AUDIO_FUNC_LIST_T* funcList);
void gaf_stream_common_trigger_sync_capture(
    GAF_AUDIO_STREAM_CONTEXT_TYPE_E streamContext, uint32_t master_clk_cnt,
    int32_t usSinceLatestAnchorPoint, uint32_t triggertimeUs);
void gaf_stream_common_start_sync_capture(GAF_AUDIO_STREAM_ENV_T* pStreamEnv);
void gaf_stream_common_sync_us_since_latest_anchor_point(GAF_AUDIO_STREAM_ENV_T* pStreamEnv);
uint8_t gaf_stream_common_get_allocation_lr_cnt(uint32_t audio_allocation_bf);
bool gaf_stream_get_prefill_status(void);
void gaf_stream_set_prefill_status(bool is_doing);
uint32_t gaf_media_common_get_latest_tx_iso_evt_timestamp(GAF_AUDIO_STREAM_ENV_T* pStreamEnv);
uint32_t gaf_media_common_get_latest_rx_iso_evt_timestamp(GAF_AUDIO_STREAM_ENV_T* pStreamEnv);
uint8_t gaf_media_common_get_ase_chan_lid_from_iso_channel(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                                        uint8_t direction, uint8_t iso_channel);
bool gaf_stream_common_fetch_multi_pcm(GAF_AUDIO_STREAM_ENV_T *pStreamEnv,
        uint8_t* ptrData, uint32_t dataLen, uint32_t dmaIrqHappeningTimeUs);
bool gaf_stream_common_fetch_pcm(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t* ptrData,
    uint32_t dataLen, uint8_t instance_handle, uint32_t dmaIrqHappeningTimeUs, uint32_t sink_play_delay, bool pidAdjust);
void* gaf_bis_stream_get_codec_core_info(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                                uint8_t direction, uint8_t iso_channel);
bool gaf_bis_playback_fetch_multi_pcm_data(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t* ptrBuf,
        uint32_t dataLen, uint32_t audio_play_delay, uint32_t dmaIrqHappeningTimeUs);
bool gaf_stream_common_fetch_frame(GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint8_t* ptrData,
            uint32_t dataLen, uint32_t currentDmaIrqTimeUs, uint8_t instance_handle);
void *gaf_stream_common_buf_alloc(void *pool, uint32_t size);
void gaf_playback_decoder_init(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t ase_lid);
void gaf_playback_decoder_deinit_by_ase(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, uint8_t ase_lid, bool isRetrigger);
void gaf_playback_decoder_deinit_all_ase(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, bool isRetrigger);
void gaf_capture_encoder_init(GAF_AUDIO_STREAM_ENV_T *pStreamEnv);
void gaf_capture_encoder_deinit(GAF_AUDIO_STREAM_ENV_T *pStreamEnv, bool isRetrigger);
void gaf_stream_common_register_cc_func_list(void);

void gaf_media_stream_boost_freq(uint32_t stayingMs);
bool gaf_stream_is_capture_stream_iso_created(void *pStreamEnv);
bool gaf_stream_is_playback_stream_iso_created(void *pStreamEnv);

void gaf_bis_stream_common_update_multi_channel_expect_seq_and_time(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, uint32_t dmaIrqHappeningTimeUs);
bool gaf_bis_stream_common_get_combined_packet_from_multi_channels(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, CC_PLAYBACK_DATA_T *out_frame, uint32_t dmaIrqHappeningTimeUs, uint32_t sink_play_delay);
#if defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
bool gaf_stream_common_get_combined_packet_from_multi_channels(
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv, CC_PLAYBACK_DATA_T *out_frame, uint32_t dmaIrqHappeningTimeUs);
void gaf_stream_common_update_multi_channel_expect_seq_and_time(GAF_AUDIO_STREAM_ENV_T* pStreamEnv,
                                                                   uint32_t dmaIrqHappeningTimeUs);
#endif

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
typedef void (*gaf_stream_common_plc_packet_occurs_cb)(uint16_t pkt_seq_nb,
    GAF_ISO_PKT_STATUS_E pkt_status, uint32_t time_stamp, uint32_t dmaIrqHappenTime);

typedef void (*gaf_stream_common_get_packet_cb)(uint8_t con_lid, uint16_t pkt_seq_nb,
        GAF_ISO_PKT_STATUS_E pkt_status, uint32_t time_stamp, uint32_t dmaIrqHappenTime);
typedef void (*gaf_stream_common_dma_irq_happens_cb)(uint8_t con_lid, uint32_t dmaChunkIntervalUs);

typedef void(*gaf_stream_dma_irq_cb) (uint8_t con_lid, uint32_t dmaIrqHappeningTimeUs, uint16_t seq_num);

void gaf_stream_common_register_stream_plc_packet_occurs_cb(gaf_stream_common_plc_packet_occurs_cb func);
void gaf_stream_common_register_stream_get_packet_cb(gaf_stream_common_get_packet_cb func);
void gaf_stream_common_register_dma_irq_happens_cb(gaf_stream_common_dma_irq_happens_cb func);

#endif   //IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED

uint8_t gaf_stream_common_get_ase_idx_in_ase_lid_list(uint8_t* ase_lid_list, uint8_t ase_lid);
uint8_t gaf_stream_common_get_valid_idx_in_ase_lid_list(uint8_t* ase_lid_list);
/**********************************GAF CUSTOM**********************************/
/**
 * @brief Set custom data callback handler
 *
 * @param gaf_user_case
 * @param func_list
 */
void gaf_stream_common_set_custom_data_handler(GAF_STREAM_USER_CASE_E gaf_user_case,
                                               const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T *func_list);
/**
* @brief Get custom data callback handler
*
* @param gaf_user_case
* @return const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T*
*/
const GAF_STREAM_COMMON_CUSTOM_DATA_HANDLER_FUNC_T
     *gaf_stream_common_get_custom_data_handler(GAF_STREAM_USER_CASE_E gaf_user_case);

/**
 * @brief Set custom presDelay
 *
 * @param presDelayUs presentation delay in us
 */
void gaf_stream_common_set_custom_presdelay_us(uint32_t presDelayUs);

/**
 * @brief Get custom set presDelay in us
 *
 * @return uint32_t
 */
uint32_t gaf_stream_common_get_custom_presdelay_us(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __GAF_MEDIA_COMMON_H__ */

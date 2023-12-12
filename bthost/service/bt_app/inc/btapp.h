/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#ifndef __BTAPP_H__
#define __BTAPP_H__
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "bluetooth.h"
#include "app_key.h"
#include "hfp_api.h"
#include "a2dp_api.h"
#include "app_a2dp_codecs.h"
#include "app_audio_focus_control.h"
#include "app_audio_control.h"

#ifdef BT_HID_DEVICE
#include "app_bt_hid.h"
#endif

#if defined(BT_MAP_SUPPORT)
#include "app_map.h"
#endif

#if defined(BT_PBAP_SUPPORT)
#include "app_pbap.h"
#endif

#if defined(BT_OPP_SUPPORT)
#include "app_opp.h"
#endif

#if defined(BT_PAN_SUPPORT)
#include "app_pan.h"
#endif

//#define __EARPHONE_STAY_BOTH_SCAN__

/* bt config */
#ifdef _SCO_BTPCM_CHANNEL_
#define SYNC_CONFIG_PATH (0<<8|1<<4|1<<0) /* all links use hci */
#else
#define SYNC_CONFIG_PATH (0<<8|0<<4|0<<0) /* all links use hci */
#endif
#define SYNC_CONFIG_MAX_BUFFER (0) /* (e)sco use Packet size */
#ifdef CVSD_BYPASS
#define SYNC_CONFIG_CVSD_BYPASS (1) /* use pcm over hci */
#else
#define SYNC_CONFIG_CVSD_BYPASS (0) /* use pcm over hci */
#endif
#define INQ_EXT_RSP_LEN 240
///a2dp app include
/* a2dp */
/* Default SBC codec configuration */
/* data type for the SBC Codec Information Element*/
/*****************************************************************************
**  Constants
*****************************************************************************/
/* the length of the SBC Media Payload header. */
#define A2D_SBC_MPL_HDR_LEN         1

/* the LOSC of SBC media codec capabilitiy */
#define A2D_SBC_INFO_LEN            6

/* for Codec Specific Information Element */
#if defined(A2DP_SCALABLE_ON) || defined(A2DP_LHDCV5_ON)
#define A2D_SCALABLE_IE_SAMP_FREQ_MSK    0xFF    /* b7-b0 sampling frequency */
#endif

#if defined(A2DP_LHDC_ON)
#define A2D_LHDC__IE_SAMP_FREQ_MSK    0xFF    /* b7-b0 sampling frequency */
#endif


#define A2D_STREAM_SAMP_FREQ_MSK    0xFF    /* b7-b4 sampling frequency */

#define A2D_SBC_IE_SAMP_FREQ_MSK    0xF0    /* b7-b4 sampling frequency */

#define A2D_SBC_IE_SAMP_FREQ_16     0x80    /* b7:16  kHz */
#define A2D_SBC_IE_SAMP_FREQ_32     0x40    /* b6:32  kHz */
#define A2D_SBC_IE_SAMP_FREQ_44     0x20    /* b5:44.1kHz */
#define A2D_SBC_IE_SAMP_FREQ_48     0x10    /* b4:48  kHz */

#ifdef A2DP_SCALABLE_ON
#define A2D_SBC_IE_SAMP_FREQ_96     0x08    /* b4:48  kHz */
#endif

#if defined(A2DP_LHDC_ON) || defined(A2DP_LHDCV5_ON)
#ifndef A2D_SBC_IE_SAMP_FREQ_96
#define A2D_SBC_IE_SAMP_FREQ_96     0x08    /* b3:96  kHz */
#endif
#endif

#ifdef A2DP_LHDCV5_ON
#ifndef A2D_SBC_IE_SAMP_FREQ_192
#define A2D_SBC_IE_SAMP_FREQ_192    0x04    /* b2:192  kHz */
#endif
#endif

#ifdef A2DP_LC3_ON
#ifndef A2D_SBC_IE_SAMP_FREQ_96
#define A2D_SBC_IE_SAMP_FREQ_96     0x08    /* b4:48  kHz */
#endif
#endif

#ifdef A2DP_LDAC_ON
#ifndef A2DP_LDAC_OCTET_NUMBER
#define A2DP_LDAC_OCTET_NUMBER                     (8)
#endif

#ifndef A2D_SBC_IE_SAMP_FREQ_96
#define A2D_SBC_IE_SAMP_FREQ_96     0x08    /* b4:96  kHz */
#endif
#ifndef A2D_SBC_IE_SAMP_FREQ_88
#define A2D_SBC_IE_SAMP_FREQ_88     0x04    /* b4:88.2  kHz */
#endif
#endif

#define A2D_SBC_IE_BIT_NUM_16       0x16
#define A2D_SBC_IE_BIT_NUM_24       0x24

#define A2D_SBC_IE_CH_MD_MSK        0x0F    /* b3-b0 channel mode */
#define A2D_SBC_IE_CH_MD_MONO       0x08    /* b3: mono */
#define A2D_SBC_IE_CH_MD_DUAL       0x04    /* b2: dual */
#define A2D_SBC_IE_CH_MD_STEREO     0x02    /* b1: stereo */
#define A2D_SBC_IE_CH_MD_JOINT      0x01    /* b0: joint stereo */

#define A2D_SBC_IE_BLOCKS_MSK       0xF0    /* b7-b4 number of blocks */
#define A2D_SBC_IE_BLOCKS_4         0x80    /* 4 blocks */
#define A2D_SBC_IE_BLOCKS_8         0x40    /* 8 blocks */
#define A2D_SBC_IE_BLOCKS_12        0x20    /* 12blocks */
#define A2D_SBC_IE_BLOCKS_16        0x10    /* 16blocks */

#define A2D_SBC_IE_SUBBAND_MSK      0x0C    /* b3-b2 number of subbands */
#define A2D_SBC_IE_SUBBAND_4        0x08    /* b3: 4 */
#define A2D_SBC_IE_SUBBAND_8        0x04    /* b2: 8 */

#define A2D_SBC_IE_ALLOC_MD_MSK     0x03    /* b1-b0 allocation mode */
#define A2D_SBC_IE_ALLOC_MD_S       0x02    /* b1: SNR */
#define A2D_SBC_IE_ALLOC_MD_L       0x01    /* b0: loundess */

#define A2D_SBC_IE_MIN_BITPOOL      2
#define A2D_SBC_IE_MAX_BITPOOL      250

#ifdef __cplusplus
extern "C" {
#endif

//extern A2dpStream a2dp_stream;
//extern btif_avdtp_codec_t a2dp_avdtpcodec;
extern const unsigned char a2dp_codec_elements[];
//extern enum AUD_SAMPRATE_T a2dp_sample_rate;

#define HFP_KEY_ANSWER_CALL             8
#define HFP_KEY_HANGUP_CALL             9
#define HFP_KEY_REDIAL_LAST_CALL        10
#define HFP_KEY_CHANGE_TO_PHONE         11
#define HFP_KEY_ADD_TO_EARPHONE         12
#define HFP_KEY_MUTE                    13
#define HFP_KEY_CLEAR_MUTE              14
//3way calls oper
#define HFP_KEY_THREEWAY_HOLD_AND_ANSWER              15
#define HFP_KEY_THREEWAY_HANGUP_AND_ANSWER            16
#define HFP_KEY_THREEWAY_HOLD_REL_INCOMING            17
#define HFP_KEY_THREEWAY_HOLD_ADD_HELD_CALL           18

#define HFP_KEY_DUAL_HF_HANGUP_ANOTHER                19
#define HFP_KEY_DUAL_HF_HANGUP_CURR_ANSWER_ANOTHER    20
#define HFP_KEY_DUAL_HF_HOLD_CURR_ANSWER_ANOTHER      21
#define HFP_KEY_DUAL_HF_CHANGETOPHONE_ANSWER_ANOTHER  22
#define HFP_KEY_DUAL_HF_CHANGETOPHONE_ANOTHER_ADDTOEARPHONE 23
#define HFP_KEY_DUAL_HF_HANGUP_ANOTHER_ADDTOEARPHONE 24
#define HFP_KEY_DUAL_HF_CHANGETOPHONE_ANSWER_CURR 25

//hsp
#define HSP_KEY_CKPD_CONTROL     	21
#define HSP_KEY_ADD_TO_EARPHONE 	22
#define HSP_KEY_CHANGE_TO_PHONE	23

typedef enum
{
    HFCALL_MACHINE_CURRENT_IDLE = 0,                                //0
    HFCALL_MACHINE_CURRENT_INCOMMING,                               //1
    HFCALL_MACHINE_CURRENT_OUTGOING,                                //2
    HFCALL_MACHINE_CURRENT_CALLING,                                 //3
    HFCALL_MACHINE_CURRENT_3WAY_INCOMMING,                          //4
    HFCALL_MACHINE_CURRENT_3WAY_HOLD_CALLING,                       //5
    HFCALL_MACHINE_CURRENT_IDLE_ANOTHER_IDLE,                       //6
    HFCALL_MACHINE_CURRENT_INCOMMING_ANOTHER_IDLE,                  //7
    HFCALL_MACHINE_CURRENT_OUTGOING_ANOTHER_IDLE,                   //8
    HFCALL_MACHINE_CURRENT_CALLING_ANOTHER_IDLE,                    //9
    HFCALL_MACHINE_CURRENT_3WAY_INCOMMING_ANOTHER_IDLE,             //10
    HFCALL_MACHINE_CURRENT_3WAY_HOLD_CALLING_ANOTHER_IDLE,          //11
    HFCALL_MACHINE_CURRENT_INCOMMING_ANOTHER_CALLING,             //12
    HFCALL_MACHINE_CURRENT_INCOMMING_ANOTHER_INCOMMING,               //13
    HFCALL_MACHINE_CURRENT_CALLING_ANOTHER_CHANGETOPHONE,           //14
    HFCALL_MACHINE_CURRENT_INCOMMING_ANOTHER_OUTGOING,              //15
    HFCALL_MACHINE_CURRENT_CALLING_ANOTHER_HOLD,                    //16
    HFCALL_MACHINE_CURRENT_OUTGOING_ANOTHER_CALLING,                //17
    HFCALL_MACHINE_NUM
} HFCALL_MACHINE_ENUM;

typedef enum
{
    HFCALL_NEXT_STA_NULL = 0,                                //0
    HFCALL_NEXT_STA_ANOTHER_ANSWER,                               //1
    HFCALL_NEXT_STA_ANOTHER_ADDTOEARPHONE,                               //2
    HFCALL_NEXT_STA_CURR_ANSWER,                              //3
    HFCALL_NEXT_STA_NUM
} HFCALL_NEXT_STA_ENUM;

#define APP_REPORT_SPEAKER_VOL_CMD 	0x01
#define APP_CPKD_CMD				0x02
#define SPP_MAX_TX_PACKET_NUM	5

typedef enum bt_profile_reconnect_mode
{
    bt_profile_reconnect_null,
    bt_profile_reconnect_openreconnecting,
    bt_profile_reconnect_reconnecting,
    bt_profile_reconnect_reconnect_pending,
} bt_profile_reconnect_mode;

typedef enum bt_profile_connect_status
{
    bt_profile_connect_status_unknow,
    bt_profile_connect_status_success,
    bt_profile_connect_status_failure,
} bt_profile_connect_status;

typedef enum
{
    APP_BT_IDLE_STATE = 0,
    APP_BT_IN_CONNECTING_PROFILES_STATE // acl link is created and in the process of connecting profiles
} APP_BT_CONNECTING_STATE_E;

struct app_bt_profile_manager
{
    bool profile_connected;
    bool remote_support_hfp;
    bool remote_support_a2dp;
    bt_bdaddr_t rmt_addr;
    bt_profile_connect_status hfp_connect;
    bt_profile_connect_status a2dp_connect;
    bt_profile_reconnect_mode reconnect_mode;
    a2dp_stream_t *stream;
    btif_hf_channel_t* chan;
    uint16_t reconnect_cnt;
    osTimerId reconnect_timer;
    void (*connect_timer_cb)(void const *);
    APP_BT_CONNECTING_STATE_E connectingState;
};

bool app_bt_source_is_enabled(void);
bool app_bt_sink_is_enabled(void);

#define BT_AVDTP_CP_VALUE_SIZE 10

typedef uint8_t a2dp_sync_status_recheck;
typedef uint8_t hfp_sync_status_recheck;

#define A2DP_STATUS_SYNC_A2DP_STATE    0x01
#define A2DP_STATUS_SYNC_AVRCP_STATE   0x02
#define A2DP_STATUS_SYNC_CODEC_STATE   0x04
#define A2DP_STATUS_ALL_RECHECK A2DP_STATUS_SYNC_A2DP_STATE|A2DP_STATUS_SYNC_AVRCP_STATE|A2DP_STATUS_SYNC_CODEC_STATE

#define HFP_STATUS_SYNC_CALL_STATE        0x01
#define HFP_STATUS_SYNC_CALLSETUP_STATE   0x02
#define HFP_STATUS_SYNC_CALLHOLD_STATE    0x04
#define HFP_STATUS_ALL_RECHECK HFP_STATUS_SYNC_CALL_STATE|HFP_STATUS_SYNC_CALLSETUP_STATE|HFP_STATUS_SYNC_CALLHOLD_STATE

#define APP_BT_COD_MISCELLANEOUS       0x00
#define APP_BT_COD_COMPUTER            0x01
#define APP_BT_COD_PHONE               0x02
#define APP_BT_COD_LAN_ACCESS_POINT    0x03
#define APP_BT_COD_AUDIO               0x04
#define APP_BT_COD_PERIPHERAL          0x05
#define APP_BT_COD_IMAGING             0x06

struct BT_DEVICE_T {
    bt_bdaddr_t remote;
    bool acl_is_connected;
    uint16_t acl_conn_hdl;
    uint8_t device_id;
    bool profiles_connected_before;
    btif_remote_device_t *btm_conn;
    struct app_bt_profile_manager profile_mgr;

    btif_a2dp_stream_t *btif_a2dp_stream;
    a2dp_stream_t *a2dp_connected_stream;

    uint8_t a2dp_lhdc_llc;
    uint8_t a2dp_non_type;
    uint8_t lhdc_ext_flags;
    uint8_t channel_mode;
    uint8_t sample_rate;
    uint8_t sample_bit;
    uint8_t vbr_support;
    uint8_t a2dp_channel_num;
    uint8_t a2dp_conn_flag;
    uint8_t a2dp_streamming;
    uint8_t isUseLocalSBM;
    uint8_t a2dp_session;
    uint8_t a2dp_play_pause_flag;
    uint8_t a2dp_need_resume_flag;
    uint8_t a2dp_disc_on_process;
    btif_avdtp_codec_type_t codec_type;
    btif_avdtp_codec_type_non_t codec_type_non_type;
    unsigned int a2dp_paused_time;
    unsigned int sco_disconnect_time;
    bool this_is_delay_reconnect_a2dp;
    bool this_is_paused_bg_a2dp;
    bool this_is_closed_bg_a2dp;
    bool this_is_curr_playing_a2dp_and_paused;
    bool a2dp_is_auto_paused_by_phone;
    bool auto_make_remote_play;
    bool this_sco_wait_to_play;
    uint8_t remember_interrupted_a2dp_for_a_while;
    bool ibrt_slave_force_disc_a2dp;
    bool mock_a2dp_after_force_disc;
    bool ibrt_disc_a2dp_profile_only;
    uint8_t a2dp_initial_volume;
    uint8_t a2dp_default_abs_volume;
    uint8_t a2dp_current_abs_volume;
	uint8_t a2dp_lc3_frame_dms;
    uint16_t a2dp_lc3_bitrate;
    uint32_t a2dp_stream_start_time;
    uint32_t a2dp_stream_suspend_time;
    uint32_t a2dp_stream_start_repeat_count;
    uint32_t a2dp_stream_suspend_repeat_count;

#ifdef __A2DP_AVDTP_CP__
    bool avdtp_cp;
    btif_avdtp_content_prot_t a2dp_avdtp_cp;
    uint8_t a2dp_avdtp_cp_security_data[BT_AVDTP_CP_VALUE_SIZE];
#endif

    btif_avrcp_channel_t *avrcp_channel;
    avrcp_media_status_t avrcp_playback_status;
    bool play_status_notify_registered;
    bool avrcp_remote_support_playback_status_change_event;
    uint8_t avrcp_conn_flag;
    uint8_t volume_report;
    uint8_t track_changed;
    uint8_t avrcp_connect_try_times;
    uint8_t a2dp_stream_recheck_context;
    bool avrcp_play_status_wait_to_handle;
    bool filter_avrcp_pause_play_quick_switching;
    bool a2dp_disc_timer_enable;
    osTimerId avrcp_pause_play_quick_switch_filter_timer;
    osTimerId avrcp_play_status_wait_timer;
    osTimerId a2dp_stream_recheck_timer;
    osTimerId hfp_delay_send_hold_timer;
#ifndef IBRT
    osTimerId avrcp_reconnect_timer;
#endif

    bool rsv_avdtp_start_signal;
    bool ibrt_slave_force_disc_avrcp;
    bool mock_avrcp_after_force_disc;
    bool ibrt_slave_force_disc_hfp;
    bool mock_hfp_after_force_disc;

    uint32_t acl_conn_prio;
    uint32_t a2dp_audio_prio;
    uint32_t a2dp_conn_prio;
    uint32_t pause_a2dp_resume_prio;
    uint32_t close_a2dp_resume_prio;
    uint32_t sco_audio_prio;
    uint32_t hfp_conn_prio;
    uint32_t hfp_call_setup_prio;
    uint32_t hfp_call_active_prio;
    uint32_t hfp_call_active_time;

    btif_hf_channel_t* hf_channel;
    btif_hf_call_setup_t hfchan_callSetup;
    btif_hf_call_active_t hfchan_call;
    btif_audio_state_t hf_audio_state;
    btif_hf_call_held_state hf_callheld;
    bool is_accepting_sco_request;
    bool disc_sco_when_it_connected;
    uint8_t hf_conn_flag;
    uint8_t switch_sco_to_earbud;
    uint8_t battery_level;

    uint16_t media_active;
    uint16_t app_audio_manage_scocodecid;

    osTimerId hfp_delay_conn_timer;
    osTimerId a2dp_delay_conn_timer;
    osTimerId avrcp_delay_conn_timer;
    osTimerId hfp_delay_disc_timer;
    osTimerId a2dp_delay_disc_timer;
    osTimerId avrcp_delay_disc_timer;

#ifdef BT_HID_DEVICE
    hid_channel_t hid_channel;
    osTimerId capture_wait_timer_id;
    osTimerId hid_wait_disc_timer_id;
    uint8_t hid_conn_flag;
    uint8_t wait_send_capture_key;
#endif

#if defined(BT_PBAP_SUPPORT)
    struct btif_pbap_channel_t *pbap_channel;
#endif

#if defined(BT_OPP_SUPPORT)
    struct btif_opp_channel_t *opp_channel;
    struct btif_opp_channel_t *opp_server_channel;
#endif

#if defined(BT_PAN_SUPPORT)
    struct btif_pan_channel_t *pan_channel;
#endif

    int8_t a2dp_audio_focus;
    int8_t call_audio_focus;
    int8_t ring_audio_focus;
    bool a2dp_streaming_available;
    bool sco_streaming_available;
    bool this_is_bg_a2dp;          /* This device maybe paused/mute/disconected by differenct configruation. */
    on_audio_focus_change_listener a2dp_focus_changed_listener;
    on_audio_focus_change_listener sco_focus_changed_listener;
    on_audio_focus_change_listener ring_focus_changed_listener;
    bool ignore_ring_and_play_tone_self;
    osTimerId self_ring_tone_play_timer;
    a2dp_sync_status_recheck a2dp_status_recheck;
    osTimerId abandon_focus_timer;
    bool waiting_pause_suspend;
    int8_t tx_power_idx;
    hfp_sync_status_recheck hfp_status_recheck;
    osTimerId check_hf_pause_a2dp_timer;
    osTimerId clcc_timer;
    bool is_hf_pause_its_a2dp;
    osTimerId check_a2dp_restreaming_timer;

    void *pcustom_param;
    osTimerId delay_play_a2dp_timer;
};

struct BT_DEVICE_RECONNECT_T {
    list_entry_t node;
    bt_bdaddr_t rmt_addr;
    uint8_t inuse;
    bool for_source_device;
    bt_profile_reconnect_mode reconnect_mode;
    uint16_t acl_reconnect_cnt;
    osTimerId acl_reconnect_timer;
};

struct app_bt_config {
    uint8_t a2dp_default_abs_volume;
    uint8_t a2dp_force_use_the_codec;
    bool a2dp_force_use_prev_codec;
    bool hid_capture_non_invade_mode;
    bool a2dp_prompt_play_mode; //new a2dp disruppt old a2dp automatically
    bool sco_prompt_play_mode; //new sco disruppt old sco automatically
    bool dont_auto_play_bg_a2dp;
    uint8_t second_sco_handle_mode;
    bool a2dp_prompt_play_only_when_avrcp_play_received;
    uint32_t a2dp_prompt_delay_ms;
    bool a2dp_delay_prompt_play;
    bool mute_a2dp_stream;
    bool pause_a2dp_stream;
    bool close_a2dp_stream;
    bool keep_only_one_stream_close_connected_a2dp;
    bool pause_a2dp_when_call_exist;
    bool reconn_sco_if_fast_disc_after_call_active_for_iphone_auto_mode;
    uint8_t hfp_default_volume;
    bool host_reject_unexcept_sco_packet;
    uint8_t second_sco_bg_action;
    bool allow_duck_ringtone;
    uint8_t virtual_call_handle;
    bool pc_second_sco_discon_acl;
    bool prompt_tone_cant_preempt_music;
};

#ifdef IBRT
struct app_bt_tws_conn_info {
    bt_bdaddr_t remote;
    uint16_t acl_conn_hdl;
    bool acl_is_connected;
    int8_t tx_power_idx;
    btif_remote_device_t *btm_conn;
};
#endif

struct BT_DEVICE_MANAGER_T {
    struct BT_DEVICE_T bt_devices[BT_DEVICE_NUM];
#if defined(BT_SOURCE)
    struct BT_DEVICE_T source_base_devices[BT_SOURCE_DEVICE_NUM];
#endif
    uint8_t curr_hf_channel_id;
    uint8_t curr_a2dp_stream_id;
    uint8_t curr_playing_sco_id;
    uint8_t curr_playing_a2dp_id;
    uint8_t interrupted_a2dp_id;
    uint8_t a2dp_last_paused_device;
    bool sco_trigger_a2dp_replay;
    bool a2dp_stream_play_recheck;

    bool trigger_a2dp_switch;
    uint8_t trigger_sco_device_id;
    uint8_t a2dp_switch_trigger_device;
    uint32_t a2dp_switch_trigger_btclk;
    uint32_t sco_switch_trigger_btclk;
    bool trigger_a2dp_cis_toggle;
    uint8_t a2dp_cis_toggle_trigger_device;
    uint32_t a2dp_cis_toggle_trigger_btclk;
    osTimerId wait_sco_connected_timer;
    uint8_t wait_sco_connected_device_id;

    uint32_t audio_prio_seed;
    uint16_t current_a2dp_conhdl;
    uint8_t prev_active_audio_link;
    uint8_t device_routed_sco_to_phone;
    uint8_t device_routing_sco_back;
    uint8_t hf_tx_mute_flag;

    uint8_t hf_call_next_state;
    uint8_t hfp_key_handle_curr_id;
    uint8_t hfp_key_handle_another_id;

    list_entry_t linkloss_reconnect_list;
    list_entry_t poweron_reconnect_list;
    struct BT_DEVICE_RECONNECT_T reconnect_node[BT_DEVICE_NUM+BT_SOURCE_DEVICE_NUM];

    struct app_bt_config config;

#ifdef IBRT
    struct app_bt_tws_conn_info tws_conn;
#endif
};

void app_bt_manager_init(void);

struct BT_DEVICE_T* app_bt_get_device(int i);
extern struct BT_DEVICE_MANAGER_T app_bt_manager;

/////app key handle include
void a2dp_handleKey(uint8_t a2dp_key);
void hfp_handle_key(uint8_t hfp_key);
void hsp_handle_key(uint8_t hsp_key);
void btapp_a2dp_report_speak_gain(void);

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
#define   BTAPP_FUNC_KEY			APP_KEY_CODE_FN1
#define   BTAPP_VOLUME_UP_KEY		APP_KEY_CODE_FN2
#define   BTAPP_VOLUME_DOWN_KEY		APP_KEY_CODE_FN3
#ifdef SUPPORT_SIRI
#define   BTAPP_RELEASE_KEY			APP_KEY_CODE_NONE
#endif
#else
#define   BTAPP_FUNC_KEY			APP_KEY_CODE_PWR
#define   BTAPP_VOLUME_UP_KEY		APP_KEY_CODE_FN1
#define   BTAPP_VOLUME_DOWN_KEY		APP_KEY_CODE_FN2
#ifdef SUPPORT_SIRI
#define   BTAPP_RELEASE_KEY			APP_KEY_CODE_NONE
#endif
#endif
void bt_key_init(void);
void bt_key_send(APP_KEY_STATUS *status);
void bt_key_handle(void);
void bt_key_handle_music_playback(void);
void bt_key_handle_call(CALL_STATE_E call_state);
void bt_key_handle_up_key(enum APP_KEY_EVENT_T event);
void bt_key_handle_down_key(enum APP_KEY_EVENT_T event);
void bt_key_handle_source_func_key(enum APP_KEY_EVENT_T event);
void bt_key_handle_bt_func_click(void);
void bt_drv_accessmode_switch_test(void);
void bt_key_handle_siri_key(enum APP_KEY_EVENT_T event);

void a2dp_callback(uint8_t device_id, a2dp_stream_t *Stream, const a2dp_callback_parms_t *Info);
void avrcp_init(void);

void a2dp_volume_set(int id, uint8_t bt_volume);
void a2dp_volume_set_local_vol(int id, uint8_t local_volume);
uint8_t a2dp_volume_get(int id);
void a2dp_volume_local_set(int id, uint8_t vol);
void a2dp_update_local_volume(int id, uint8_t localVol);

bool avrcp_get_tg_play_status_play_pause(void);
void a2dp_volume_local_set(int id, uint8_t vol);
uint8_t a2dp_volume_local_get(int id);

void a2dp_abs_volume_set(int id, uint8_t vol);
uint8_t a2dp_abs_volume_get(int id);

uint8_t a2dp_convert_local_vol_to_bt_vol(uint8_t localVol);
uint8_t a2dp_convert_bt_vol_to_local_vol(uint8_t btVol);

void hfp_volume_set(int id, uint8_t btVol);
uint8_t hfp_volume_get(int id);
void hfp_update_local_volume(int id, uint8_t localVol);
uint8_t hfp_volume_local_get(int id);

uint8_t hfp_convert_local_vol_to_bt_vol(uint8_t localVol);
uint8_t hfp_convert_bt_vol_to_local_vol(uint8_t btVol);
void app_bt_reset_rssi_collector(void);
int32_t app_bt_tx_rssi_analyzer(int8_t rssi);
/**
 * Convert BES BD_ADDR to virtual
 * BES Device ID
 */
bool a2dp_id_from_bdaddr(bt_bdaddr_t *bd_addr, uint8_t *id);

void bt_key_handle_func_click();
void bt_key_handle_func_doubleclick();
void bt_key_handle_func_tripleclick();
void bt_key_handle_func_longpress();

void bt_key_handle_customer_doubleclick();
void bt_key_handle_customer_volume();

void app_bt_print_buff_status(void);

void bt_sbc_player_set_codec_type(uint8_t type);
uint8_t bt_sbc_player_get_codec_type(void);
uint8_t bt_sbc_player_get_sample_bit(void);
#if defined(A2DP_LDAC_ON)
int bt_ldac_player_get_channelmode(void);
int bt_get_ladc_sample_rate(void);
#endif

uint8_t app_bt_avrcp_get_volume_change_trans_id(uint8_t device_id);
void app_bt_avrcp_set_volume_change_trans_id(uint8_t device_id, uint8_t trans_id);
uint8_t app_bt_avrcp_get_ctl_trans_id(uint8_t device_id);
void app_bt_avrcp_set_ctl_trans_id(uint8_t device_id, uint8_t trans_id);

void app_bt_profile_connect_manager_a2dp(int id, a2dp_stream_t *Stream, const a2dp_callback_parms_t *Info);
#ifdef _SUPPORT_REMOTE_COD_
bool app_bt_device_is_computer(uint8_t device_id);
#endif
void app_bt_multi_ibrt_music_config(uint8_t* link_id, uint8_t* active, uint8_t num);

#ifdef CUSTOM_BITRATE
uint8_t app_audio_a2dp_player_playback_delay_mtu_get(uint16_t codec_type);
void app_audio_dynamic_update_dest_packet_mtu_set(uint8_t codec_index, uint8_t packet_mtu, uint8_t user_configure);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BTAPP_H__ */

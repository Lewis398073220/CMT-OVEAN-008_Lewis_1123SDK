/***************************************************************************
 *
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
 *
 ****************************************************************************/
#ifndef __BES_AOB_API_H__
#define __BES_AOB_API_H__
#include "ble_aob_common.h"
#include "nvrecord_extension.h"
#ifdef BLE_HOST_SUPPORT
#ifdef __cplusplus
extern "C" {
#endif

#if BLE_AUDIO_ENABLED

#define BES_LEA_CODEC_ID_LC3          (&codec_id_lc3)

typedef enum
{
    BES_BLE_AUDIO_TWS_MASTER,
    BES_BLE_AUDIO_TWS_SLAVE,
    BES_BLE_AUDIO_MOBILE,
    BES_BLE_AUDIO_ROLE_UNKNOW,
} BES_BLE_AUDIO_TWS_ROLE_E;

/// ASE Direction, sync @see app_gaf_direction
typedef enum bes_gaf_direction
{
    /// Sink direction
    BES_BLE_GAF_DIRECTION_SINK = 0,
    /// Source direction
    BES_BLE_GAF_DIRECTION_SRC,

    BES_BLE_GAF_DIRECTION_MAX,
} bes_gaf_direction_t;

// Frame_Duration   #app_gaf_bap_frame_dur
enum bes_ble_gaf_bap_frame_duration
{
    BES_BLE_GAF_BAP_FRAME_DURATION_7_5MS    = 0x00,
    BES_BLE_GAF_BAP_FRAME_DURATION_10MS     = 0x01,
    BES_BLE_GAF_BAP_FRAME_DURATION_5MS      = 0x02,
    BES_BLE_GAF_BAP_FRAME_DURATION_2_5MS    = 0x03,
    BES_BLE_GAF_BAP_FRAME_DURATION_MAX,
};

/// Audio Location Bitfield
typedef enum
{
    BES_BLE_LOC_FRONT_LEFT               = 0x00000001,
    BES_BLE_LOC_FRONT_RIGHT              = 0x00000002,
    BES_BLE_LOC_FRONT_CENTER             = 0x00000004,
    BES_BLE_LOC_LOW_FREQ_EFFECTS_1       = 0x00000008,
    BES_BLE_LOC_BACK_LEFT                = 0x00000010,
    BES_BLE_LOC_BACK_RIGHT               = 0x00000020,
    BES_BLE_LOC_FRONT_LEFT_OF_CENTER     = 0x00000040,
    BES_BLE_LOC_FRONT_RIGHT_OF_CENTER    = 0x00000080,
    BES_BLE_LOC_BACK_CENTER              = 0x00000100,
    BES_BLE_LOC_LOW_FREQ_EFFECTS_2       = 0x00000200,
    BES_BLE_LOC_SIDE_LEFT                = 0x00000400,
    BES_BLE_LOC_SIDE_RIGHT               = 0x00000800,
    BES_BLE_LOC_TOP_FRONT_LEFT           = 0x00001000,
    BES_BLE_LOC_TOP_FRONT_RIGHT          = 0x00002000,
    BES_BLE_LOC_TOP_FRONT_CENTER         = 0x00004000,
    BES_BLE_LOC_TOP_CENTER               = 0x00008000,
    BES_BLE_LOC_TOP_BACK_LEFT            = 0x00010000,
    BES_BLE_LOC_TOP_BACK_RIGHT           = 0x00020000,
    BES_BLE_LOC_TOP_SIDE_LEFT            = 0x00040000,
    BES_BLE_LOC_TOP_SIDE_RIGHT           = 0x00080000,
    BES_BLE_LOC_TOP_BACK_CENTER          = 0x00100000,
    BES_BLE_LOC_BOTTOM_FRONT_CENTER      = 0x00200000,
    BES_BLE_LOC_BOTTOM_FRONT_LEFT        = 0x00400000,
    BES_BLE_LOC_BOTTOM_FRONT_RIGHT       = 0x00800000,
    BES_BLE_LOC_FRONT_LEFT_WIDE          = 0x01000000,
    BES_BLE_LOC_FRONT_RIGHT_WIDE         = 0x02000000,
    BES_BLE_LOC_LEFT_SURROUND            = 0x04000000,
    BES_BLE_LOC_RIGHT_SURROUND           = 0x08000000,

    BES_BLE_LOC_RFU                      = 0xF0000000,
} BES_BLE_GAF_LOCATION_BF_BIT_E;

typedef enum
{
    BES_BLE_GAF_SAMPLE_FREQ_8000    = 0x01,
    BES_BLE_GAF_SAMPLE_FREQ_11025,
    BES_BLE_GAF_SAMPLE_FREQ_16000,
    BES_BLE_GAF_SAMPLE_FREQ_22050,
    BES_BLE_GAF_SAMPLE_FREQ_24000,
    BES_BLE_GAF_SAMPLE_FREQ_32000,
    BES_BLE_GAF_SAMPLE_FREQ_44100,
    BES_BLE_GAF_SAMPLE_FREQ_48000,
    BES_BLE_GAF_SAMPLE_FREQ_88200,
    BES_BLE_GAF_SAMPLE_FREQ_96000,
    BES_BLE_GAF_SAMPLE_FREQ_176400,
    BES_BLE_GAF_SAMPLE_FREQ_192000,
    BES_BLE_GAF_SAMPLE_FREQ_384000,
} BES_BLE_GAF_SAMPLE_FREQ_E;

/// Frame Duration values
typedef enum
{
    /// Use 7.5ms Codec frames
    BES_BLE_GAF_FRAME_DUR_7_5MS = 0,
    /// Use 10ms Codec frames
    BES_BLE_GAFFRAME_DUR_10MS,
    /// Use 5ms Codec frames
    BES_BLE_GAF_FRAME_DUR_5MS,
    /// Use 2.5ms Codec frames
    BES_BLE_GAF_FRAME_DUR_2_5MS,
    /// Maximum value
    BES_BLE_GAF_FRAME_DUR_MAX
} BES_BLE_GAF_FRAME_DUR_E;

typedef enum
{
    // 8K__7_5MS
    BES_BLE_GAF_CODEC_FRAME_26     = 26,
    // 8K__10MS
    // 16K__7_5MS
    BES_BLE_GAF_CODEC_FRAME_30     = 30,
    // 16K__10MS
    BES_BLE_GAF_CODEC_FRAME_40     = 40,
    // 24K__7_5MS
    BES_BLE_GAF_CODEC_FRAME_45     = 45,
    // 24K__10MS
    // 32K__7_5MS
    BES_BLE_GAF_CODEC_FRAME_60     = 60,
    // 32K__10MS
    BES_BLE_GAF_CODEC_FRAME_80     = 80,
    // 44.1K__7_5MS
    BES_BLE_GAF_CODEC_FRAME_97     = 97,
    // 44.1K__10MS
    BES_BLE_GAF_CODEC_FRAME_130    = 130,
    // 48K__7_5MS
    BES_BLE_GAF_CODEC_FRAME_75     = 75,
    BES_BLE_GAF_CODEC_FRAME_90     = 90,
    BES_BLE_GAF_CODEC_FRAME_117    = 117,
    // 48K__10MS
    BES_BLE_GAF_CODEC_FRAME_100    = 100,
    BES_BLE_GAF_CODEC_FRAME_120    = 120,
    BES_BLE_GAF_CODEC_FRAME_155    = 155,
} BES_BLE_GAF_CODEC_FRAME_E;

/// Context type bit field meaning
typedef enum
{
    /// Unspecified - Position
    BES_BLE_GAF_CONTEXT_TYPE_UNSPECIFIED = 0,
    BES_BLE_GAF_CONTEXT_TYPE_UNSPECIFIED_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_UNSPECIFIED),
    /// Conversation between humans as, for example, in telephony or video calls
    BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL = 1,
    BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL),
    /// Media as, for example, in music, public radio, podcast or video soundtrack.
    BES_BLE_GAF_CONTEXT_TYPE_MEDIA = 2,
    BES_BLE_GAF_CONTEXT_TYPE_MEDIA_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_MEDIA),
    /// Audio associated with video gaming, for example gaming media, gaming effects, music and in-game voice chat
    BES_BLE_GAF_CONTEXT_TYPE_GAME = 3,
    BES_BLE_GAF_CONTEXT_TYPE_GAME_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_GAME),
    /// Instructional audio as, for example, in navigation, traffic announcements or user guidance
    BES_BLE_GAF_CONTEXT_TYPE_INSTRUCTIONAL = 4,
    BES_BLE_GAF_CONTEXT_TYPE_INSTRUCTIONAL_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_INSTRUCTIONAL),
    /// Man machine communication as, for example, with voice recognition or virtual assistant
    BES_BLE_GAF_CONTEXT_TYPE_MAN_MACHINE = 5,
    BES_BLE_GAF_CONTEXT_TYPE_MAN_MACHINE_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_MAN_MACHINE),
    /// Live audio as from a microphone where audio is perceived both through a direct acoustic path and through
    /// an LE Audio Stream
    BES_BLE_GAF_CONTEXT_TYPE_LIVE = 6,
    BES_BLE_GAF_CONTEXT_TYPE_LIVE_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_LIVE),
    /// Sound effects including keyboard and touch feedback;
    /// menu and user interface sounds; and other system sounds
    BES_BLE_GAF_CONTEXT_TYPE_SOUND_EFFECTS = 7,
    BES_BLE_GAF_CONTEXT_TYPE_SOUND_EFFECTS_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_SOUND_EFFECTS),
    /// Attention seeking audio as, for example, in beeps signalling arrival of a message or keyboard clicks
    BES_BLE_GAF_CONTEXT_TYPE_ATTENTION_SEEKING = 8,
    BES_BLE_GAF_CONTEXT_TYPE_ATTENTION_SEEKING_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_ATTENTION_SEEKING),
    /// Ringtone as in a call alert
    BES_BLE_GAF_CONTEXT_TYPE_RINGTONE = 9,
    BES_BLE_GAF_CONTEXT_TYPE_RINGTONE_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_RINGTONE),
    /// Immediate alerts as, for example, in a low battery alarm, timer expiry or alarm clock.
    BES_BLE_GAF_CONTEXT_TYPE_IMMEDIATE_ALERT = 10,
    BES_BLE_GAF_CONTEXT_TYPE_IMMEDIATE_ALERT_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_IMMEDIATE_ALERT),
    /// Emergency alerts as, for example, with fire alarms or other urgent alerts
    BES_BLE_GAF_CONTEXT_TYPE_EMERGENCY_ALERT = 11,
    BES_BLE_GAF_CONTEXT_TYPE_EMERGENCY_ALERT_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_EMERGENCY_ALERT),
    /// TV - Position
    /// Audio associated with a television program and/or with metadata conforming to the Bluetooth Broadcast TV
    /// profile
    BES_BLE_GAF_CONTEXT_TYPE_TV = 12,
    BES_BLE_GAF_CONTEXT_TYPE_TV_BIT = CO_BIT_MASK(BES_BLE_GAF_CONTEXT_TYPE_TV),
} BES_BLE_GAF_CONTEXT_E;

/// ASE State values
enum bes_ble_gaf_bap_uc_ase_state
{
    /// Idle
    BES_BLE_GAF_ASCS_ASE_STATE_IDLE = 0,
    /// Codec configured
    BES_BLE_GAF_ASCS_ASE_STATE_CODEC_CONFIGURED,
    /// QoS configured
    BES_BLE_GAF_ASCS_ASE_STATE_QOS_CONFIGURED,
    /// Enabling
    BES_BLE_GAF_ASCS_ASE_STATE_ENABLING,
    /// Streaming
    BES_BLE_GAF_ASCS_ASE_STATE_STREAMING,
    /// Disabling
    BES_BLE_GAF_ASCS_ASE_STATE_DISABLING,
    /// Releasing
    BES_BLE_GAF_ASCS_ASE_STATE_RELEASING,

    BES_BLE_GAF_ASCS_ASE_STATE_MAX,
};

/// Codec Type values
typedef enum bes_ble_gaf_codec_type
{
    /// LC3 Codec
    BES_BLE_GAF_CODEC_TYPE_LC3    = 0x06,
    /// ULL
    BES_BLE_GAF_CODEC_TYPE_ULL    = 0x08,
    /// Maximum SIG Codec
    BES_BLE_GAF_CODEC_TYPE_SIG_MAX,
    /// Vendor Specific Codec
    BES_BLE_GAF_CODEC_TYPE_VENDOR = 0xFF,
} BES_BLE_GAF_CODEC_TYPE_T;

/// SDU Buffer structure
typedef struct
{
    /// Time_Stamp
    uint32_t        time_stamp;
    /// Packet Sequence Number
    uint16_t        pkt_seq_nb;
    /// length of the ISO SDU (in bytes)
    uint16_t        sdu_length;
    /// Reception status (@see enum hci_iso_pkt_stat_flag)
    uint8_t         status;
    /// SDU
    uint8_t         *sdu;
} bes_ble_dp_itf_iso_buffer_t;

/// ASCS ASE Structure @ see bes_ble_bap_ascs_ase_t
typedef struct bes_ble_bap_ascs_ase
{
    /// ASE local index
    uint8_t ase_lid;
    /// Connection local index
    uint8_t con_lid;
    /// ASE Instance local index
    uint8_t ase_id;
    /// ASE Direction
    bes_gaf_direction_t direction;
    /// ASE State
    uint8_t ase_state;
    /// Codec ID
    AOB_CODEC_ID_T codec_id;
    /// Pointer to Codec Configuration structure
    AOB_BAP_CFG_T *p_cfg;
    /// QoS Requirements
    AOB_BAP_QOS_REQ_T qos_req;
    /// CIG ID
    uint8_t cig_id;
    /// CIS ID
    uint8_t cis_id;
    /// CIS Connection Handle
    uint16_t cis_hdl;
    /// QoS Configuration structure
    AOB_BAP_QOS_CFG_T qos_cfg;
    /// Pointer to Metadata structure
    AOB_BAP_CFG_METADATA_T *p_metadata;
    /// CIG sync delay in us
    uint32_t cig_sync_delay;
    /// CIS sync delay in us
    uint32_t cis_sync_delay;
    /// iso interval in us
    uint32_t iso_interval_us;
    /// bn count from master to slave
    uint32_t bn_m2s;
    /// bn count from slave to master
    uint32_t bn_s2m;
    /// context type bring by enable req
    uint16_t init_context_bf;
} bes_ble_bap_ascs_ase_t;

/// QoS Configuration structure (short)
typedef struct bes_ble_bap_ascc_qos_cfg
{
    /// PHY
    uint8_t phy;
    /// Maximum number of retransmissions for each CIS Data PDU
    /// From 0 to 15
    uint8_t retx_nb;
    /// Maximum SDU size
    /// From 0 to 4095 bytes (0xFFF)
    uint16_t max_sdu_size;
    /// Presentation delay in microseconds
    uint32_t pres_delay_us;
} bes_ble_bap_ascc_qos_cfg_t;

/// ASCC ASE Information structure
typedef struct bes_ble_bap_ascc_ase
{
    /// ASE local index
    uint8_t ase_lid;
    /// ASE State
    uint8_t ase_state;
    /// Connection local index
    uint8_t con_lid;
    /// ASE instance index
    uint8_t ase_instance_idx;
    /// ASE Direction
    bes_gaf_direction_t direction;
    /// Codec ID
    AOB_CODEC_ID_T codec_id;
    /// Pointer to Codec Configuration
    AOB_BAP_CFG_T *p_cfg;
    /// QoS configuration
    bes_ble_bap_ascc_qos_cfg_t qos_cfg;
    /// Pointer to Metadata structure
    AOB_BAP_CFG_METADATA_T *p_metadata;
    /// CIS index
    uint8_t cis_id;
    /// CIS Connection Handle
    uint16_t cis_hdl;
    /// current audio stream type, @see gaf_bap_context_type_bf
    //uint16_t context_bf;
    /// CIG sync delay in us
    uint32_t cig_sync_delay;
    /// CIS sync delay in us
    uint32_t cis_sync_delay;
    /// iso interval in us
    uint32_t iso_interval_us;
    /// bn count from master to slave
    uint32_t bn_m2s;
    /// bn count from slave to master
    uint32_t bn_s2m;
} bes_ble_bap_ascc_ase_t;

/// BIG Info Report
typedef struct
{
    /// Value of the SDU interval in microseconds (Range 0x0000FF-0x0FFFFF)
    uint32_t  sdu_interval;
    /// Value of the ISO Interval (1.25 ms unit)
    uint16_t  iso_interval;
    /// Value of the maximum PDU size (Range 0x0000-0x00FB)
    uint16_t  max_pdu;
    /// VValue of the maximum SDU size (Range 0x0000-0x0FFF)
    uint16_t  max_sdu;
    /// Number of BIS present in the group (Range 0x01-0x1F)
    uint8_t   num_bis;
    /// Number of sub-events (Range 0x01-0x1F)
    uint8_t   nse;
    /// Burst number (Range 0x01-0x07)
    uint8_t   bn;
    /// Pre-transmit offset (Range 0x00-0x0F)
    uint8_t   pto;
    /// Initial retransmission count (Range 0x01-0x0F)
    uint8_t   irc;
    /// PHY used for transmission (0x01: 1M, 0x02: 2M, 0x03: Coded, All other values: RFU)
    uint8_t   phy;
    /// Framing mode (0x00: Unframed, 0x01: Framed, All other values: RFU)
    uint8_t   framing;
    /// True if broadcast isochronous group is encrypted, False otherwise
    bool      encrypted;
} bes_ble_bap_bc_big_info_t;

/// sync see@aob_bis_src_big_param_t
typedef struct
{
    uint8_t                    bcast_id[3];
    // Indicate if streams are encrypted (!= 0) or not
    uint8_t                    encrypted;
    // Broadcast code. Meaningful only if encrypted parameter indicates that streams are encrypted
    uint8_t                    bcast_code[16];
    /// Number of Streams in the Broadcast Group. Cannot be 0
    uint8_t                    nb_streams;
    /// Number of Subgroups in the Broadcast Group. Cannot be 0
    uint8_t                    nb_subgroups;
    /// SDU interval in microseconds
    /// From 256us (0x00000100) to 1.048575s (0x000FFFFF)
    uint32_t                   sdu_intv_us;
    /// Maximum size of an SDU
    /// From 1 to 4095 bytes
    uint16_t                   max_sdu;
    // Audio output presentation delay in microseconds
    uint32_t                   pres_delay_us;
    // creat test big hci cmd, 0:creat BIG cmd, 1:creat BIG test cmd
    uint8_t                    test;

    //// test = 0, set this param
    struct {
        /// Maximum time (in milliseconds) between the first transmission of an SDU to the end of the last transmission
        /// of the same SDU
        /// From 0ms to 4.095s (0x0FFF)
        uint16_t                   max_tlatency_ms;
        /// Number of times every PDU should be transmitted
        /// From 0 to 15
        uint8_t                    rtn;
    } big_param;

    //// test = 1, set this param
    struct {
        /// ISO interval in multiple of 1.25ms. From 0x4 (5ms) to 0xC80 (4s)
        uint16_t                   iso_intv_frame;
        /// Number of subevents in each interval of each stream in the group
        uint8_t                    nse;
        /// Maximum size of a PDU
        uint8_t                    max_pdu;
        /// Burst number (number of new payload in each interval). From 1 to 7
        uint8_t                    bn;
        /// Number of times the scheduled payload is transmitted in a given event. From 0x1 to 0xF
        uint8_t                    irc;
        /// Isochronous Interval spacing of payloads transmitted in the pre-transmission subevents.
        /// From 0x00 to 0x0F
        uint8_t                    pto;
    } test_big_param;
}bes_ble_bis_src_big_param_t;

/// sync see @aob_bis_src_subgrp_param_t
typedef struct
{
    /// Subgroup local identifier
    uint8_t                  sgrp_lid;
    /// Codec ID
    uint8_t                  codec_id[AOB_COMMON_CODEC_ID_LEN];

    ///LTV cfg info
    // audio location bf, bit see@BES_BLE_GAF_LOCATION_BF_BIT_E
    uint32_t                 location_bf;
    /// Length of a codec frame in octets
    uint16_t                 frame_octet;
    /// Sampling Frequency (see #BES_BLE_GAF_SAMPLE_FREQ_E enumeration)
    uint8_t                  sampling_freq;
    /// Frame Duration (see #BES_BLE_GAF_FRAME_DUR_E enumeration)
    uint8_t                  frame_dur;
    /// Number of blocks of codec frames that shall be sent or received in a single SDU
    uint8_t                  frames_sdu;

    ///LTV media data
    /// Streaming Audio Contexts bit field (see #enum bap_context_type_bf enumeration)
    uint16_t                 context_bf;
} bes_ble_bis_src_subgrp_param_t;

/// sync see@aob_bis_src_stream_param_t
typedef struct
{
    /// Stream local identifier
    uint8_t                  stream_lid;
    /// Subgroup local identifier
    uint8_t                  sgrp_lid;
    /// LTV cfg param
    /// When received, 0 shall be interpreted as a single channel with no specified Audio Location
    /// (see @BES_BLE_GAF_LOCATION_BF_BIT_E)
    uint32_t                 location_bf;
    /// Length of a codec frame in octets
    uint16_t                 frame_octet;
    /// Sampling Frequency (see #BES_BLE_GAF_SAMPLE_FREQ_E enumeration)
    uint8_t                  sampling_freq;
    /// Frame Duration (see #BES_BLE_GAF_FRAME_DUR_E enumeration)
    uint8_t                  frame_dur;
    /// Number of blocks of codec frames that shall be sent or received in a single SDU
    uint8_t                  frames_sdu;
} bes_ble_bis_src_stream_param_t;

typedef struct
{
    uint32_t big_trans_latency;
} bes_ble_bis_src_started_param_t;

/// sync see@aob_bis_src_stream_param_t
typedef struct
{
    void (*bis_stream_get_data)(uint8_t stream_id, uint8_t **data, uint16_t *data_len, uint8_t cache_num);
    void (*bis_stream_get_buf_free)(uint8_t stream_id, uint8_t *data);
    void (*bis_stream_start_ind)(uint8_t stream_id, bes_ble_bis_src_started_param_t *param);
    void (*bis_stream_stop_ind)(uint8_t stream_id);
} bes_ble_bis_src_start_param_t;

/// sync see@aob_bis_sink_start_param_t
typedef struct
{
    uint32_t ch_bf;
    uint8_t *bc_id;
    uint8_t *bc_code;
    struct
    {
        void (*bis_sink_started_callback)(uint8_t grp_lid);
        void (*bis_sink_stoped_callback)(uint8_t grp_lid);
        void (*bis_sink_private_data_callback)(uint8_t *buf, uint16_t buf_len);
    } event_callback;
} bes_ble_bis_sink_start_param_t;

/// sync see@app_gaf_codec_id_t
typedef struct
{
    /// Codec ID value
    uint8_t codec_id[5];
} bes_gaf_codec_id_t;

/// sync see@AOB_MEDIA_ASE_CFG_INFO_T
typedef struct
{
    uint16_t sample_rate;
    uint16_t frame_octet;
    bes_gaf_direction_t direction;
    const bes_gaf_codec_id_t *codec_id;
    BES_BLE_GAF_CONTEXT_E context_type;
} bes_lea_ase_cfg_param_t;

#ifdef AOB_MOBILE_ENABLED
void bes_ble_mobile_connect_failed(bool is_failed);

void bes_ble_audio_mobile_core_register_event_cb(const BLE_AUD_MOB_CORE_EVT_CB_T *cb);

void bes_ble_audio_mobile_conn_next_paired_dev(ble_bdaddr_t* bdaddr);

bool bes_ble_audio_mobile_reconnect_dev(ble_bdaddr_t* bdaddr);

bool bes_ble_mobile_is_connect_failed(void);

uint8_t* bes_ble_audio_mobile_conn_get_connecting_dev(void);

void bes_lea_mobile_stream_start(uint8_t con_lid, bes_lea_ase_cfg_param_t *cfg);
#endif

void bes_ble_audio_common_init(void);

void bes_ble_audio_common_deinit(void);

bool bes_ble_audio_make_new_le_core_sm(uint8_t conidx, uint8_t *peer_bdaddr);

uint16_t bes_ble_bis_src_get_bis_hdl_by_big_idx(uint8_t big_idx);

const AOB_CODEC_ID_T *bes_ble_bis_src_get_codec_id_by_big_idx(uint8_t big_idx, uint8_t subgrp_idx);

const AOB_BAP_CFG_T *bes_ble_bis_src_get_codec_cfg_by_big_idx(uint8_t big_idx);

uint32_t bes_ble_bis_src_get_iso_interval_ms_by_big_idx(uint8_t big_idx);

void bes_ble_bis_sink_private_data_callback(uint8_t *buf, uint8_t buf_len);

uint8_t bes_ble_bis_src_send_iso_data_to_all_channel(uint8_t **payload, uint16_t payload_len, uint32_t ref_time);

void bes_ble_bis_src_set_big_param(uint8_t big_idx, bes_ble_bis_src_big_param_t *p_big_param);

void bes_ble_bis_src_set_subgrp_param(uint8_t big_idx, bes_ble_bis_src_subgrp_param_t *p_subgrp_param);

void bes_ble_bis_src_set_stream_param(uint8_t big_idx, bes_ble_bis_src_stream_param_t *p_stream_param);

uint32_t bes_ble_bis_src_get_stream_current_timestamp(uint8_t big_idx, uint8_t stream_lid);

void bes_ble_bis_src_start(uint8_t big_idx, bes_ble_bis_src_start_param_t *start_bis_info);

void bes_ble_bis_src_stop(uint8_t big_idx);

void bes_ble_update_tws_nv_role(uint8_t role);

void bes_ble_bap_set_activity_type(gaf_bap_activity_type_e type);

bool bes_ble_gap_is_remote_mobile_connected(const ble_bdaddr_t *p_addr);

bool bes_ble_aob_conn_start_adv(bool br_edr_support, bool in_paring);

bool bes_ble_aob_conn_stop_adv(void);

void bes_ble_audio_mobile_req_disconnect(ble_bdaddr_t *addr);

bool bes_ble_audio_is_mobile_link_connected(ble_bdaddr_t *addr);

void bes_ble_audio_disconnect_all_connection(void);

bool bes_ble_audio_is_any_mobile_connnected(void);

bool bes_ble_audio_is_ux_mobile(void);

void bes_ble_mobile_start_connect(void);

uint8_t bes_ble_audio_get_mobile_lid_by_pub_address(uint8_t *pub_addr);

void bes_ble_audio_mobile_disconnect_device(uint8_t conidx);

bool bes_ble_aob_csip_is_use_custom_sirk(void);

void bes_ble_aob_gattc_rebuild_cache(GATTC_NV_SRV_ATTR_t *record);

void bes_ble_aob_csip_if_use_temporary_sirk();

void bes_ble_aob_csip_if_refresh_sirk(uint8_t *sirk);

bool bes_ble_aob_csip_sirk_already_refreshed();

bool bes_ble_aob_csip_if_get_sirk(uint8_t *sirk);

void bes_ble_aob_conn_dump_state_info(void);

void bes_ble_aob_bis_tws_sync_state_req(void);

void bes_ble_audio_core_register_event_cb(const BLE_AUD_CORE_EVT_CB_T *cb);

void bes_ble_aob_call_if_outgoing_dial(uint8_t conidx, uint8_t *uri, uint8_t uriLen);

ble_bdaddr_t *bes_ble_aob_conn_get_remote_address(uint8_t con_lid);

void bes_ble_aob_media_prev(uint8_t con_lid);

void bes_ble_aob_media_next(uint8_t con_lid);

void bes_ble_aob_call_if_retrieve_call(uint8_t conidx, uint8_t call_id);

void bes_ble_aob_call_if_terminate_call(uint8_t conidx, uint8_t call_id);

void bes_ble_aob_call_if_accept_call(uint8_t conidx, uint8_t call_id);

void bes_ble_aob_media_play(uint8_t con_lid);

void bes_ble_aob_media_pause(uint8_t con_lid);

void bes_ble_aob_vol_down(void);

void bes_ble_aob_vol_up(void);

uint8_t bes_ble_aob_convert_local_vol_to_le_vol(uint8_t bt_vol);

BLE_AUDIO_POLICY_CONFIG_T* bes_ble_audio_get_policy_config();

uint8_t bes_ble_audio_get_mobile_sm_index_by_addr(ble_bdaddr_t *addr);

void bes_ble_audio_sink_streaming_handle_event(uint8_t con_lid, uint8_t data,
                                                               bes_gaf_direction_t direction, app_ble_audio_event_t event);


void bes_ble_audio_dump_conn_state(void);

uint8_t bes_ble_audio_get_mobile_addr(uint8_t deviceId, uint8_t *addr);

int bes_ble_aob_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);

uint8_t bes_ble_aob_get_call_id_by_conidx_and_type(uint8_t device_id, uint8_t call_state);

uint8_t bes_ble_aob_get_call_id_by_conidx(uint8_t device_id);

bool bes_ble_aob_get_acc_bond_status(uint8_t conidx, uint8_t type);

uint8_t bes_ble_audio_get_mobile_connected_lid(uint8_t con_lid[]);

void bes_ble_bap_capa_srv_get_ava_context_bf(uint8_t con_lid, uint16_t *context_bf_ava_sink, uint16_t *context_bf_ava_src);

void bes_ble_bap_capa_srv_set_ava_context_bf(uint8_t con_lid, uint16_t context_bf_ava_sink, uint16_t context_bf_ava_src);

// sink api
void bes_ble_bis_sink_start(bes_ble_bis_sink_start_param_t *param);
void bes_ble_bis_sink_stop();

void bes_ble_bap_start_discovery(uint8_t con_lid);

gaf_bap_activity_type_e bes_ble_bap_get_actv_type(void);

uint8_t bes_ble_bap_get_device_num_to_be_connected(void);

const bes_ble_bap_ascs_ase_t *bes_ble_get_ascs_ase_info(uint8_t ase_lid);

uint32_t bes_ble_bap_capa_get_location_bf(bes_gaf_direction_t direction);

uint8_t bes_ble_bap_ascs_get_streaming_ase_lid_list(uint8_t con_lid, uint8_t *ase_lid_list);

void bes_ble_bap_ascs_send_ase_enable_rsp(uint8_t ase_lid, bool accept);

void bes_ble_bap_ascs_disable_ase_req(uint8_t ase_lid);

void bes_ble_bap_ascs_release_ase_req(uint8_t ase_lid);

void bes_ble_bap_dp_itf_data_come_callback_register(void *callback);

void bes_ble_bap_dp_itf_data_come_callback_deregister(void);

void bes_ble_bap_iso_dp_send_data(uint16_t conhdl, uint16_t seq_num, uint8_t *payload, uint16_t payload_len, uint32_t ref_time);

uint8_t bes_ble_audio_get_tws_nv_role(void);

uint8_t bes_ble_audio_get_location_fs_l_r_cnt(uint32_t audio_location_bf);

uint8_t bes_ble_arc_get_mic_state(uint8_t con_lid);

uint8_t bes_ble_arc_convert_le_vol_to_local_vol(uint8_t le_vol);

uint8_t bes_ble_arc_vol_get_real_time_volume(uint8_t con_lid);

void bes_ble_arc_mobile_set_abs_vol(uint8_t con_lid, uint8_t local_vol);

void *bes_ble_bap_dp_itf_get_rx_data(uint16_t iso_hdl, bes_ble_dp_itf_iso_buffer_t *p_iso_buffer);

int bes_ble_bap_get_free_iso_packet_num(void);

void bes_ble_bap_dp_tx_iso_stop(uint16_t iso_hdl);

void bes_ble_bap_dp_rx_iso_stop(uint16_t iso_hdl);

bool bes_ble_ccp_call_is_device_call_active(uint8_t con_lid);

void bes_ble_start_gaf_discovery(uint8_t con_lid);

#ifdef AOB_MOBILE_ENABLED
const bes_ble_bap_ascc_ase_t *bes_ble_bap_ascc_get_ase_info(uint8_t ase_lid);

uint8_t bes_ble_bap_ascc_get_specific_state_ase_lid_list(uint8_t con_lid, uint8_t direction, uint8_t ase_state, uint8_t *ase_lid_list);

bool bes_ble_bap_pacc_is_peer_support_stereo_channel(uint8_t con_lid, uint8_t direction);

void bes_ble_bap_ascc_configure_codec_by_ase_lid(uint8_t ase_lid, uint8_t cis_id, const AOB_CODEC_ID_T *codec_id,
                                                        uint16_t sampleRate, uint16_t frame_octet);

void bes_ble_bap_ascc_ase_qos_cfg_by_ase_lid(uint8_t ase_lid, uint8_t grp_lid);

void bes_ble_bap_ascc_link_create_group_req(uint8_t cig_lid);

void bes_ble_bap_ascc_link_remove_group_req(uint8_t grp_lid);

void bes_ble_bap_ascc_ase_enable_by_ase_lid(uint8_t ase_lid, uint16_t context_bf);

void bes_ble_bap_ascc_ase_release_by_ase_lid(uint8_t ase_lid);

void bes_ble_bap_ascc_ase_disable_by_ase_lid(uint8_t ase_lid);

void bes_ble_mcp_mcs_action_control(uint8_t media_lid, uint8_t action);

void bes_ble_mcp_mcs_track_changed(uint8_t media_lid, uint32_t duration_10ms, uint8_t *title, uint8_t title_len);

#endif /// AOB_MOBILE_ENABLED

#endif /* BLE_AUDIO_ENABLED */

#ifdef __cplusplus
}
#endif
#endif /* BLE_HOST_SUPPORT */
#endif /* __BES_AOB_API_H__ */

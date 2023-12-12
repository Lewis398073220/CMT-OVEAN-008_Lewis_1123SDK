/**
 ****************************************************************************************
 *
 * @file bap.h
 *
 * @brief Basic Audio Profile - Header file
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef BAP_H_
#define BAP_H_

/**
 ****************************************************************************************
 * @defgroup BAP Basic Audio Profile (BAP)
 * @ingroup GAF
 * @brief Description of Basic Audio Profile block
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_COMMON Common
 * @ingroup BAP
 * @brief Description of common API for Basic Audio Profile block
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_ENUM Enumerations
 * @ingroup BAP_COMMON
 * @brief Enumerations for Basic Audio Profile block
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_STRUCT Structures
 * @ingroup BAP_COMMON
 * @brief Structures for Basic Audio Profile block
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_NATIVE_API Native API
 * @ingroup BAP_COMMON
 * @brief Description of Native API for Basic Audio Profile block
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf_cfg.h"         // GAF Configuration
#include "gaf.h"             // Code definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup BAP_ENUM
/// @{

/// Module type values for Basic Audio Profile block
enum bap_module_type
{
    /// Common module
    BAP_MODULE_COMMON = 0,
    /// Basic Audio Profile Capabilities Server module
    BAP_MODULE_CAPA_SRV = 1,
    /// Basic Audio Profile Capabilities Client module
    BAP_MODULE_CAPA_CLI = 2,
    /// Basic Audio Profile Unicast Server module
    BAP_MODULE_UC_SRV = 3,
    /// Basic Audio Profile Unicast Client module
    BAP_MODULE_UC_CLI = 4,
    /// Basic Audio Profile Broadcast Source module
    BAP_MODULE_BC_SRC = 5,
    /// Basic Audio Profile Broadcast Sink module
    BAP_MODULE_BC_SINK = 6,
    /// Basic Audio Profile Broadcast Scan module
    BAP_MODULE_BC_SCAN = 7,
    /// Basic Audio Profile Broadcast Assistant module
    BAP_MODULE_BC_ASSIST = 8,
    /// Basic Audio Profile Broadcast Delegator module
    BAP_MODULE_BC_DELEG = 9,

    /// Maximum value
    BAP_MODULE_MAX,
};

/// LTV structure format
enum bap_ltv_fmt
{
    /// Length
    BAP_LTV_LENGTH_POS = 0,
    /// Type
    BAP_LTV_TYPE_POS,
    /// Value
    BAP_LTV_VALUE_POS,
    ///
    BAP_VENDOR_LTV_COMPANY_POS_1 = BAP_LTV_VALUE_POS,

    BAP_VENDOR_LTV_COMPANY_POS_2,

    /// Minimal length of LTV structure
    BAP_LTV_LENGTH_MIN = 1,

    /// Minimal length of Vendor Specific LTV structure
    BAP_VENDOR_LTV_LENGTH_MIN = 3,
};

/// Codec Specific Capabilities Types values
enum bap_capa_type
{
    /// Supported Sampling Frequencies
    BAP_CAPA_TYPE_SAMP_FREQ = 1,
    /// Supported Frame Durations
    BAP_CAPA_TYPE_FRAME_DUR,
    /// Audio Channel Counts
    BAP_CAPA_TYPE_CHNL_CNT,
    /// Supported Octets per Codec Frame
    BAP_CAPA_TYPE_OCTETS_FRAME,
    /// Maximum Supported Codec Frames per SDU
    BAP_CAPA_TYPE_FRAMES_SDU,
};

/// Metadata Types values
enum bap_metadata_type
{
    /// Preferred Audio Contexts
    BAP_METADATA_TYPE_PREF_CONTEXTS = 1,
    /// Streaming Audio Contexts
    BAP_METADATA_TYPE_STREAM_CONTEXTS,

    /// Extended
    BAP_METADATA_TYPE_EXTENDED = 0xFE,

    /// Vendor Specific
    BAP_METADATA_TYPE_VENDOR = 0xFF,
};

/// Minimal value of length field for Codec Specific Capabilities LTV structure
enum bap_capa_length
{
    /// Supported Sampling Frequencies
    BAP_CAPA_LENGTH_SAMP_FREQ = 3,
    /// Supported Frame Durations
    BAP_CAPA_LENGTH_FRAME_DUR = 2,
    /// Audio Channel Counts
    BAP_CAPA_LENGTH_CHNL_CNT = 2,
    /// Supported Octets per Codec Frame
    BAP_CAPA_LENGTH_OCTETS_FRAME = 5,
    /// Maximum Supported Codec Frames per SDU
    BAP_CAPA_LENGTH_FRAMES_SDU = 2,
};

/// Minimal value of length field for Metadata LTV structure
enum bap_metadata_length
{
    /// Preferred Audio Contexts
    BAP_METADATA_LENGTH_PREF_CONTEXTS = 3,
    /// Streaming Audio Contexts
    BAP_METADATA_LENGTH_STREAM_CONTEXTS = 3,
};

/// Codec Specific Configuration Types values
enum bap_cfg_type
{
    /// Sampling Frequencies
    BAP_CFG_TYPE_SAMP_FREQ = 1,
    /// Frame Duration
    BAP_CFG_TYPE_FRAME_DUR,
    /// Audio Channel Allocation
    BAP_CFG_TYPE_CHNL_LOCATION,
    /// Octets per Codec Frame
    BAP_CFG_TYPE_OCTETS_FRAME,
    /// Codec Frame Blocks Per SDU
    BAP_CFG_TYPE_FRAMES_SDU,
};

/// Minimal value of length field for Codec Specific Configuration LTV structure
enum bap_cfg_length
{
    /// Sampling Frequencies
    BAP_CFG_LENGTH_SAMP_FREQ = 2,
    /// Frame Duration
    BAP_CFG_LENGTH_FRAME_DUR = 2,
    /// Audio Channel Allocation
    BAP_CFG_LENGTH_CHNL_LOCATION = 5,
    /// Octets per Codec Frame
    BAP_CFG_LENGTH_OCTETS_FRAME = 3,
    /// Codec Frame Blocks Per SDU
    BAP_CFG_LENGTH_FRAMES_SDU = 2,
};

/// Supported Roles bit field meaning
enum bap_role_bf
{
    /// Capabilities Server supported
    BAP_ROLE_SUPP_CAPA_SRV_POS = 0,
    BAP_ROLE_SUPP_CAPA_SRV_BIT = CO_BIT(BAP_ROLE_SUPP_CAPA_SRV_POS),
    /// Capabilities Client supported
    BAP_ROLE_SUPP_CAPA_CLI_POS = 1,
    BAP_ROLE_SUPP_CAPA_CLI_BIT = CO_BIT(BAP_ROLE_SUPP_CAPA_CLI_POS),
    /// Unicast Server supported
    BAP_ROLE_SUPP_UC_SRV_POS = 2,
    BAP_ROLE_SUPP_UC_SRV_BIT = CO_BIT(BAP_ROLE_SUPP_UC_SRV_POS),
    /// Unicast Client supported
    BAP_ROLE_SUPP_UC_CLI_POS = 3,
    BAP_ROLE_SUPP_UC_CLI_BIT = CO_BIT(BAP_ROLE_SUPP_UC_CLI_POS),
    /// Broadcast Source supported
    BAP_ROLE_SUPP_BC_SRC_POS = 4,
    BAP_ROLE_SUPP_BC_SRC_BIT = CO_BIT(BAP_ROLE_SUPP_BC_SRC_POS),
    /// Broadcast Sink supported
    BAP_ROLE_SUPP_BC_SINK_POS = 5,
    BAP_ROLE_SUPP_BC_SINK_BIT = CO_BIT(BAP_ROLE_SUPP_BC_SINK_POS),
    /// Broadcast Scan supported
    BAP_ROLE_SUPP_BC_SCAN_POS = 6,
    BAP_ROLE_SUPP_BC_SCAN_BIT = CO_BIT(BAP_ROLE_SUPP_BC_SCAN_POS),
    /// Broadcast Scan Assistant supported
    BAP_ROLE_SUPP_BC_ASSIST_POS = 7,
    BAP_ROLE_SUPP_BC_ASSIST_BIT = CO_BIT(BAP_ROLE_SUPP_BC_ASSIST_POS),
    /// Broadcast Scan Delegator supported
    BAP_ROLE_SUPP_BC_DELEG_POS  = 8,
    BAP_ROLE_SUPP_BC_DELEG_BIT  = CO_BIT(BAP_ROLE_SUPP_BC_DELEG_POS),
};

/// Sampling Frequency values
enum bap_sampling_freq
{
    BAP_SAMPLING_FREQ_MIN = 1,
    /// 8000 Hz
    BAP_SAMPLING_FREQ_8000HZ = BAP_SAMPLING_FREQ_MIN,
    /// 11025 Hz
    BAP_SAMPLING_FREQ_11025HZ,
    /// 16000 Hz
    BAP_SAMPLING_FREQ_16000HZ,
    /// 22050 Hz
    BAP_SAMPLING_FREQ_22050HZ,
    /// 24000 Hz
    BAP_SAMPLING_FREQ_24000HZ,
    /// 32000 Hz
    BAP_SAMPLING_FREQ_32000HZ,
    /// 44100 Hz
    BAP_SAMPLING_FREQ_44100HZ,
    /// 48000 Hz
    BAP_SAMPLING_FREQ_48000HZ,
    /// 88200 Hz
    BAP_SAMPLING_FREQ_88200HZ,
    /// 96000 Hz
    BAP_SAMPLING_FREQ_96000HZ,
    /// 176400 Hz
    BAP_SAMPLING_FREQ_176400HZ,
    /// 192000 Hz
    BAP_SAMPLING_FREQ_192000HZ,
    /// 384000 Hz
    BAP_SAMPLING_FREQ_384000HZ,

    /// Maximum value
    BAP_SAMPLING_FREQ_MAX
};

/// Supported Sampling Frequencies bit field meaning
enum bap_sampling_freq_bf
{
    /// 8000 Hz - Position
    BAP_SAMPLING_FREQ_8000HZ_POS = 0,
    /// 8000 Hz - Bit
    BAP_SAMPLING_FREQ_8000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_8000HZ_POS),

    /// 11025 Hz - Position
    BAP_SAMPLING_FREQ_11025HZ_POS = 1,
    /// 11025 Hz - Bit
    BAP_SAMPLING_FREQ_11025HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_11025HZ_POS),

    /// 16000 Hz - Position
    BAP_SAMPLING_FREQ_16000HZ_POS = 2,
    /// 16000 Hz - Bit
    BAP_SAMPLING_FREQ_16000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_16000HZ_POS),

    /// 22050 Hz - Position
    BAP_SAMPLING_FREQ_22050HZ_POS = 3,
    /// 22050 Hz - Bit
    BAP_SAMPLING_FREQ_22050HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_22050HZ_POS),

    /// 24000 Hz - Position
    BAP_SAMPLING_FREQ_24000HZ_POS = 4,
    /// 24000 Hz - Bit
    BAP_SAMPLING_FREQ_24000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_24000HZ_POS),

    /// 32000 Hz - Position
    BAP_SAMPLING_FREQ_32000HZ_POS = 5,
    /// 32000 Hz - Bit
    BAP_SAMPLING_FREQ_32000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_32000HZ_POS),

    /// 44100 Hz - Position
    BAP_SAMPLING_FREQ_44100HZ_POS = 6,
    /// 44100 Hz - Bit
    BAP_SAMPLING_FREQ_44100HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_44100HZ_POS),

    /// 48000 Hz - Position
    BAP_SAMPLING_FREQ_48000HZ_POS = 7,
    /// 48000 Hz - Bit
    BAP_SAMPLING_FREQ_48000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_48000HZ_POS),

    /// 88200 Hz - Position
    BAP_SAMPLING_FREQ_88200HZ_POS = 8,
    /// 88200 Hz - Bit
    BAP_SAMPLING_FREQ_88200HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_88200HZ_POS),

    /// 96000 Hz - Position
    BAP_SAMPLING_FREQ_96000HZ_POS = 9,
    /// 96000 Hz - Bit
    BAP_SAMPLING_FREQ_96000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_96000HZ_POS),

    /// 176400 Hz - Position
    BAP_SAMPLING_FREQ_176400HZ_POS = 10,
    /// 176400 Hz - Bit
    BAP_SAMPLING_FREQ_176400HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_176400HZ_POS),

    /// 192000 Hz - Position
    BAP_SAMPLING_FREQ_192000HZ_POS = 11,
    /// 192000 Hz - Bit
    BAP_SAMPLING_FREQ_192000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_192000HZ_POS),

    /// 384000 Hz - Position
    BAP_SAMPLING_FREQ_384000HZ_POS = 12,
    /// 384000 Hz - Bit
    BAP_SAMPLING_FREQ_384000HZ_BIT = CO_BIT(BAP_SAMPLING_FREQ_384000HZ_POS),
};

/// Frame Duration values
enum bap_frame_dur
{
    /// Use 7.5ms Codec frames
    BAP_FRAME_DUR_7_5MS = 0,
    /// Use 10ms Codec frames
    BAP_FRAME_DUR_10MS,
#if defined(LC3PLUS_SUPPORT) || defined(AOB_LOW_LATENCY_MODE) || defined(HID_ULL_ENABLE)
    /// Use 5ms Codec frames
    BAP_FRAME_DUR_5MS,
    /// Use 2.5ms Codec frames
    BAP_FRAME_DUR_2_5MS,
#endif
    /// Maximum value
    BAP_FRAME_DUR_MAX
};

#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
/// Supoorted IMU Interval bit field meaning
enum bap_imu_interval_bf
{
    /// 7.5ms IMU Interval is supported - Position
    IMU_INTERVAL_7_5MS_POS = 0,
    /// 7.5ms IMU Interval is supported - Bit
    IMU_INTERVAL_7_5MS_BIT = CO_BIT(IMU_INTERVAL_7_5MS_POS),

    /// 10ms IMU interval is supported - Position
    IMU_INTERVAL_10MS_POS = 1,
    /// 10ms IMU interval is supported - Bit
    IMU_INTERVAL_10MS_BIT = CO_BIT(IMU_INTERVAL_10MS_POS),

    /// Bit2 and Bit3 is for future

    /// 7.5ms IMU Interval is preferred - Position
    IMU_INTERVAL_7_5MS_PREF_POS = 4,
    /// 7.5ms frame duration is preferred - Bit
    IMU_INTERVAL_7_5MS_PREF_BIT = CO_BIT(IMU_INTERVAL_7_5MS_PREF_POS),

    /// 10ms IMU interval is preferred - Position
    IMU_INTERVAL_10MS_PREF_POS = 5,
    /// 10ms IMU interval is preferred - Bit
    IMU_INTERVAL_10MS_PREF_BIT = CO_BIT(IMU_INTERVAL_10MS_PREF_POS),

    /// Bit6 and Bit7 is for future
};

/// Selected Coordinated system bit field meaning
enum bap_select_coordinated_system_bt
{
    /// Quat-X coordinated system is supported - Position
    COORDINATED_SYSTEM_QUAT_X_POS = 0,
    /// Quat-X coordinated system is supported - Bit
    COORDINATED_SYSTEM_QUAT_X_BIT = CO_BIT(COORDINATED_SYSTEM_QUAT_X_POS),

    /// Quat-Y coordinated system is supported - Position
    COORDINATED_SYSTEM_QUAT_Y_POS = 1,
    /// Quat-Y coordinated system is supported - Bit
    COORDINATED_SYSTEM_QUAT_Y_BIT = CO_BIT(COORDINATED_SYSTEM_QUAT_Y_POS),

    /// Quat-Z coordinated system is supported - Position
    COORDINATED_SYSTEM_QUAT_Z_POS = 2,
    /// Quat-Z coordinated system is supported - Bit
    COORDINATED_SYSTEM_QUAT_Z_BIT = CO_BIT(COORDINATED_SYSTEM_QUAT_Z_POS),

    /// Quat-W coordinated system is supported - Position
    COORDINATED_SYSTEM_QUAT_W_POS = 3,
    /// Quat-Z coordinated system is supported - Bit
    COORDINATED_SYSTEM_QUAT_W_BIT = CO_BIT(COORDINATED_SYSTEM_QUAT_W_POS),

    /// Acc-X coordinated system is supported - Position
    COORDINATED_SYSTEM_ACC_X_POS = 4,
    /// Acc-X coordinated system is supported - Bit
    COORDINATED_SYSTEM_ACC_X_BIT = CO_BIT(COORDINATED_SYSTEM_ACC_X_POS),

    /// Acc-Y coordinated system is supported - Position
    COORDINATED_SYSTEM_ACC_Y_POS = 5,
    /// Acc-Y coordinated system is supported - Bit
    COORDINATED_SYSTEM_ACC_Y_BIT = CO_BIT(COORDINATED_SYSTEM_ACC_Y_POS),

    /// Acc-Z coordinated system is supported - Position
    COORDINATED_SYSTEM_ACC_Z_POS = 6,
    /// Acc-Z coordinated system is supported - Bit
    COORDINATED_SYSTEM_ACC_Z_BIT = CO_BIT(COORDINATED_SYSTEM_ACC_Z_POS),

    /// Bit7 to Bit15 for the future.
};
#endif

/// Supported Frame Durations bit field meaning
enum bap_frame_dur_bf
{
    /// 7.5ms frame duration is supported - Position
    BAP_FRAME_DUR_7_5MS_POS = 0,
    /// 7.5ms frame duration is supported - Bit
    BAP_FRAME_DUR_7_5MS_BIT = CO_BIT(BAP_FRAME_DUR_7_5MS_POS),

    /// 10ms frame duration is supported - Position
    BAP_FRAME_DUR_10MS_POS = 1,
    /// 10ms frame duration is supported - Bit
    BAP_FRAME_DUR_10MS_BIT = CO_BIT(BAP_FRAME_DUR_10MS_POS),

#if defined(LC3PLUS_SUPPORT) || defined(AOB_LOW_LATENCY_MODE) || defined(HID_ULL_ENABLE)
    /// 7.5ms frame duration is supported - Position
    BAP_FRAME_DUR_5MS_POS  = 2,
    /// 7.5ms frame duration is supported - Bit
    BAP_FRAME_DUR_5MS_BIT = CO_BIT(BAP_FRAME_DUR_5MS_POS),

    /// 10ms frame duration is supported - Position
    BAP_FRAME_DUR_2_5MS_POS = 3,
    /// 10ms frame duration is supported - Bit
    BAP_FRAME_DUR_2_5MS_BIT = CO_BIT(BAP_FRAME_DUR_2_5MS_POS),
#endif

    /// 7.5ms frame duration is preferred - Position
    BAP_FRAME_DUR_7_5MS_PREF_POS = 4,
    /// 7.5ms frame duration is preferred - Bit
    BAP_FRAME_DUR_7_5MS_PREF_BIT = CO_BIT(BAP_FRAME_DUR_7_5MS_PREF_POS),

    /// 10ms frame duration is preferred - Position
    BAP_FRAME_DUR_10MS_PREF_POS = 5,
    /// 10ms frame duration is preferred - Bit
    BAP_FRAME_DUR_10MS_PREF_BIT = CO_BIT(BAP_FRAME_DUR_10MS_PREF_POS),

#if defined(LC3PLUS_SUPPORT) || defined(AOB_LOW_LATENCY_MODE) || defined(HID_ULL_ENABLE)
    /// 7.5ms frame duration is supported - Position
    BAP_FRAME_DUR_5MS_PREF_POS = 6,
    /// 7.5ms frame duration is supported - Bit
    BAP_FRAME_DUR_5MS_PREF_BIT = CO_BIT(BAP_FRAME_DUR_5MS_PREF_POS),

    /// 10ms frame duration is supported - Position
    BAP_FRAME_DUR_2_5MS_PREF_POS = 7,
    /// 10ms frame duration is supported - Bit
    BAP_FRAME_DUR_2_5MS_PREF_BIT = CO_BIT(BAP_FRAME_DUR_2_5MS_PREF_POS),
#endif

};

/// Context type bit field meaning
enum bap_context_type_bf
{
    /// Unspecified - Position
    BAP_CONTEXT_TYPE_UNSPECIFIED_POS = 0,
    /// Unspecified - Bit
    BAP_CONTEXT_TYPE_UNSPECIFIED_BIT = CO_BIT(BAP_CONTEXT_TYPE_UNSPECIFIED_POS),

    /// Conversational - Position
    BAP_CONTEXT_TYPE_CONVERSATIONAL_POS = 1,
    /// Conversational - Bit\n
    /// Conversation between humans as, for example, in telephony or video calls
    BAP_CONTEXT_TYPE_CONVERSATIONAL_BIT = CO_BIT(BAP_CONTEXT_TYPE_CONVERSATIONAL_POS),

    /// Media - Position
    BAP_CONTEXT_TYPE_MEDIA_POS = 2,
    /// Media - Bit\n
    /// Media as, for example, in music, public radio, podcast or video soundtrack.
    BAP_CONTEXT_TYPE_MEDIA_BIT = CO_BIT(BAP_CONTEXT_TYPE_MEDIA_POS),

    /// Game - Position
    BAP_CONTEXT_TYPE_GAME_POS = 3,
    /// Game - Bit\n
    /// Audio associated with video gaming, for example gaming media, gaming effects, music and in-game voice chat
    /// between participants; or a mix of all the above
    BAP_CONTEXT_TYPE_GAME_BIT = CO_BIT(BAP_CONTEXT_TYPE_GAME_POS),

    /// Instructional - Position
    BAP_CONTEXT_TYPE_INSTRUCTIONAL_POS = 4,
    /// Instructional - Bit\n
    /// Instructional audio as, for example, in navigation, traffic announcements or user guidance
    BAP_CONTEXT_TYPE_INSTRUCTIONAL_BIT = CO_BIT(BAP_CONTEXT_TYPE_INSTRUCTIONAL_POS),

    /// Man Machine - Position
    BAP_CONTEXT_TYPE_MAN_MACHINE_POS = 5,
    /// Man Machine - Bit\n
    /// Man machine communication as, for example, with voice recognition or virtual assistant
    BAP_CONTEXT_TYPE_MAN_MACHINE_BIT = CO_BIT(BAP_CONTEXT_TYPE_MAN_MACHINE_POS),

    /// Live - Position
    BAP_CONTEXT_TYPE_LIVE_POS = 6,
    /// Live - Bit\n
    /// Live audio as from a microphone where audio is perceived both through a direct acoustic path and through
    /// an LE Audio Stream
    BAP_CONTEXT_TYPE_LIVE_BIT = CO_BIT(BAP_CONTEXT_TYPE_LIVE_POS),

    /// Sound Effects - Position
    BAP_CONTEXT_TYPE_SOUND_EFFECTS_POS = 7,
    /// Sound Effects - Bit\n
    /// Sound effects including keyboard and touch feedback;
    /// menu and user interface sounds; and other system sounds
    BAP_CONTEXT_TYPE_SOUND_EFFECTS_BIT = CO_BIT(BAP_CONTEXT_TYPE_SOUND_EFFECTS_POS),

    /// Attention Seeking - Position
    BAP_CONTEXT_TYPE_ATTENTION_SEEKING_POS = 8,
    /// Attention Seeking - Bit\n
    /// Attention seeking audio as, for example, in beeps signalling arrival of a message or keyboard clicks
    BAP_CONTEXT_TYPE_ATTENTION_SEEKING_BIT = CO_BIT(BAP_CONTEXT_TYPE_ATTENTION_SEEKING_POS),

    /// Ringtone - Position
    BAP_CONTEXT_TYPE_RINGTONE_POS = 9,
    /// Ringtone - Bit\n
    /// Ringtone as in a call alert
    BAP_CONTEXT_TYPE_RINGTOME_BIT = CO_BIT(BAP_CONTEXT_TYPE_RINGTONE_POS),

    /// Immediate Alert - Position
    BAP_CONTEXT_TYPE_IMMEDIATE_ALERT_POS = 10,
    /// Immediate Alert - Bit\n
    /// Immediate alerts as, for example, in a low battery alarm, timer expiry or alarm clock.
    BAP_CONTEXT_TYPE_IMMEDIATE_ALERT_BIT = CO_BIT(BAP_CONTEXT_TYPE_IMMEDIATE_ALERT_POS),

    /// Emergency Alert - Position
    BAP_CONTEXT_TYPE_EMERGENCY_ALERT_POS = 11,
    /// Emergency Alert - Bit\n
    /// Emergency alerts as, for example, with fire alarms or other urgent alerts
    BAP_CONTEXT_TYPE_EMERGENCY_ALERT_BIT = CO_BIT(BAP_CONTEXT_TYPE_EMERGENCY_ALERT_POS),

    /// TV - Position
    BAP_CONTEXT_TYPE_TV_POS = 12,
    /// TV - Bit\n
    /// Audio associated with a television program and/or with metadata conforming to the Bluetooth Broadcast TV
    /// profile
    BAP_CONTEXT_TYPE_TV_BIT = CO_BIT(BAP_CONTEXT_TYPE_TV_POS),
};

/// @} BAP_ENUM

/*
 * TYPE DEFINITION
 ****************************************************************************************
 */

/// @addtogroup BAP_STRUCT
/// @{

/// Configuration structure for BAP Capabilities Server module
typedef struct bap_capa_srv_cfg
{
    /// Number of PAC Groups for Sink direction
    uint8_t nb_pacs_sink;
    /// Number of PAC Groups for Source direction
    uint8_t nb_pacs_src;
    /// Configuration bit field (see #bap_capa_srv_cfg_bf enumeration)
    uint8_t cfg_bf;
    /// Preferred MTU
    /// Values from 0 to 63 are equivalent to 64
    uint16_t pref_mtu;
    /// Required start handle
    /// If set to GATT_INVALID_HDL, the start handle will be automatically chosen
    uint16_t shdl;
    /// Supported Audio Locations bit field for Sink direction
    /// Meaningful only if nb_pac_sink is different than 0
    uint32_t location_bf_sink;
    /// Supported Audio Locations bit field for Source direction
    /// Meaningful only if nb_pac_src is different than 0
    uint32_t location_bf_src;
    /// Supported Audio Contexts bit field for Sink direction
    /// Meaningful only if nb_pac_sink is different than 0
    uint16_t supp_context_bf_sink;
    /// Supported Audio Contexts bit field for Source direction
    /// Meaningful only if nb_pac_src is different than 0
    uint16_t supp_context_bf_src;
} bap_capa_srv_cfg_t;

/// Configuration structure for BAP Capabilities Client module
typedef struct bap_capa_cli_cfg
{
    /// Preferred MTU
    /// Values from 0 to 63 are equivalent to 64
    uint16_t pref_mtu;
} bap_capa_cli_cfg_t;

/// Configuration structure for BAP Unicast Server module
typedef struct bap_uc_srv_cfg
{
    /// Number of Sink ASE characteristic instances
    /// Shall be in the range [0, 15]
    /// Total number of Sink/Source ASE characteristics cannot be higher than 15
    /// At least one Sink/Source ASE characteristic shall be supported
    uint8_t nb_ase_chars_sink;
    /// Number of Source ASE characteristic instances
    /// Shall be in the range [0, 15]
    /// Total number of Sink/Source ASE characteristics cannot be higher than 15
    /// At least one Sink/Source ASE characteristic shall be supported
    uint8_t nb_ase_chars_src;
    /// Number of ASE configurations that can be maintained
    /// Shall be at least equal to nb_ase_chars_sink + nb_ase_chars_src
    /// Should be a multiple of nb_ase_chars_sink + nb_ase_chars_src
    /// Shall not be larger than (nb_ase_chars_sink + nb_ase_chars_src) * HOST_CONNECTION_MAX
    /// Cannot be higher than 32
    uint8_t nb_ases_cfg;
    /// Configuration bit field (see #bap_uc_srv_cfg_bf enumeration)
    uint8_t cfg_bf;
    /// Preferred MTU
    /// Values from 0 to 63 are equivalent to 64
    uint16_t pref_mtu;
    /// Required start handle
    /// If set to GATT_INVALID_HDL, the start handle will automatically chosen
    uint16_t shdl;
} bap_uc_srv_cfg_t;

/// Configuration structure for BAP Unicast Client module
typedef struct bap_uc_cli_cfg
{
    /// Configuration bit field
    uint8_t cfg_bf;
    /// Number of ASE configurations that can be maintained\n
    /// Shall be at larger than 0
    uint8_t nb_ases_cfg;
    /// Preferred MTU\n
    /// Values from 0 to 63 are equivalent to 64
    uint16_t pref_mtu;
    /// Timeout duration in seconds for reception of notification for ASE Control Point characteristic and for
    /// some notifications of ASE characteristic\n
    /// From 1s to 5s, 0 means 1s
    uint8_t timeout_s;
} bap_uc_cli_cfg_t;

/// Broadcast Scanner configuration structure
typedef struct bap_bc_scan_cfg
{
    /// Number of Broadcast Source information that may be stored in the cache
    uint8_t cache_size;
    /// Number of Periodic Synchronizations that may be established in parallel
    uint8_t nb_sync;
} bap_bc_scan_cfg_t;

/// Broadcast Assistant configuration structure
typedef struct bap_bc_assist_cfg
{
    /// Configuration bit field (see #bap_bc_assist_cfg_bf enumeration)
    uint8_t cfg_bf;
    /// Preferred MTU - Values from 0 to 63 are equivalent to 64
    uint16_t pref_mtu;
} bap_bc_assist_cfg_t;

/// Broadcast Delegator configuration structure
typedef struct bap_bc_deleg_cfg
{
    /// Number of supported Broadcast Sources Up to 15
    uint8_t nb_srcs;
    /// Configuration bit field (see #bap_bc_deleg_cfg_bf enumeration)
    uint8_t cfg_bf;
    /// Required start handle - If set to GATT_INVALID_HDL, the start handle will be automatically chosen
    uint16_t shdl;
    /// Preferred MTU - Values from 0 to 63 are equivalent to 64
    uint16_t pref_mtu;
} bap_bc_deleg_cfg_t;

/// Broadcast Group Parameters structure
typedef struct bap_bc_grp_param
{
    /// SDU interval in microseconds
    /// From 256us (0x00000100) to 1.048575s (0x000FFFFF)
    uint32_t sdu_intv_us;
    /// Maximum size of an SDU
    /// From 1 to 4095 bytes
    uint16_t max_sdu;
    /// Sequential or Interleaved scheduling (see TODO [LT])
    uint8_t packing;
    /// Unframed or framed mode (see TODO [LT])
    uint8_t framing;
    /// Bitfield indicating PHYs that can be used by the controller for transmission of SDUs (see TODO [LT])
    uint8_t phy_bf;
    // creat test big hci cmd, 0:creat BIG cmd, 1:creat BIG test cmd
    uint8_t test;

     //// test = 0, set this param
    /// Maximum time (in milliseconds) between the first transmission of an SDU to the end of the last transmission
    /// of the same SDU
    /// From 0ms to 4.095s (0x0FFF)
    uint16_t max_tlatency_ms;
    /// Number of times every PDU should be transmitted
    /// From 0 to 15
    uint8_t rtn;

    //// test = 1, set this param
    /// ISO interval in multiple of 1.25ms. From 0x4 (5ms) to 0xC80 (4s)
    uint16_t iso_intv_frame;
    /// Number of subevents in each interval of each stream in the group
    uint8_t  nse;
    /// Maximum size of a PDU
    uint8_t  max_pdu;
    /// Burst number (number of new payload in each interval). From 1 to 7
    uint8_t  bn;
    /// Number of times the scheduled payload is transmitted in a given event. From 0x1 to 0xF
    uint8_t  irc;
    /// Isochronous Interval spacing of payloads transmitted in the pre-transmission subevents.
    /// From 0x00 to 0x0F
    uint8_t  pto;
} bap_bc_grp_param_t;

/// Advertising Parameters structure
typedef struct bap_bc_adv_param
{
    /// Minimum advertising interval in multiple of 0.625ms
    /// From 20ms (0x00000020) to 10485.759375s (0x00FFFFFF)
    uint32_t adv_intv_min_slot;
    /// Maximum advertising interval in multiple of 0.625ms
    /// From 20ms (0x00000020) to 10485.759375s (0x00FFFFFF)
    uint32_t adv_intv_max_slot;
    /// Channel Map (@see TODO [LT])
    uint8_t chnl_map;
    /// PHY for primary advertising (see #gap_phy_val enumeration)
    /// Only LE 1M and LE Codec PHYs are allowed
    uint8_t phy_prim;
    /// PHY for secondary advertising (see #gap_phy_val enumeration)
    uint8_t phy_second;
    /// Advertising SID
    /// From 0x00 to 0x0F
    uint8_t adv_sid;
} bap_bc_adv_param_t;

/// Periodic Advertising Parameters structure
typedef struct bap_bc_per_adv_param
{
    /// Minimum Periodic Advertising interval in multiple of 1.25ms
    /// Must be higher than 7.5ms (0x0006)
    uint16_t adv_intv_min_frame;
    /// Maximum Periodic Advertising interval in multiple of 1.25ms
    /// Must be higher than 7.5ms (0x0006)
    uint16_t adv_intv_max_frame;
} bap_bc_per_adv_param_t;

/// Advertising identification structure
typedef gap_per_adv_bdaddr_t bap_adv_id_t;

/// Codec Capabilities parameters structure
typedef struct bap_capa_param
{
    /// Supported Sampling Frequencies bit field (see #bap_sampling_freq_bf enumeration)\n
    /// 0 means that the field is not part of the Codec Specific Capabilities\n
    /// Mandatory for LC3
    uint16_t sampling_freq_bf;
    /// Supported Frame Durations bit field (see #bap_frame_dur_bf enumeration)\n
    /// 0 means that the field is not part of the Codec Specific Capabilities\n
    /// Mandatory for LC3
    uint8_t frame_dur_bf;
    /// Supported Audio Channel Counts\n
    /// 0 means that the field is not part of the Codec Specific Capabilities\n
    /// For LC3, absence in the Codec Specific Capabilities is equivalent to 1 channel supported (forced to 0x01
    /// on reception side)
    uint8_t chan_cnt_bf;
    /// Supported Octets Per Codec Frame - Minimum\n
    /// Not part of the Codec Specific Capabilities is equal to 0 and frame_octet_max also equal to 0\n
    /// Mandatory for LC3
    uint16_t frame_octet_min;
    /// Supported Octets Per Codec Frame - Maximum\n
    /// Not part of the Codec Specific Capabilities is equal to 0 and frame_octet_min also equal to 0\n
    /// Mandatory for LC3
    uint16_t frame_octet_max;
    /// Supported Maximum Codec Frames Per SDU\n
    /// 0 means that the field is not part of the Codec Specific Capabilities\n
    /// For LC3, absence in the Codec Specific Capabilities is equivalent to 1 Frame Per SDU (forced to 1 on
    /// reception side)
    uint8_t max_frames_sdu;
} bap_capa_param_t;

/// Codec Capabilities structure
typedef struct bap_capa
{
    /// Parameters structure
    bap_capa_param_t param;
    /// Additional Codec Capabilities (in LTV format)
    gaf_ltv_t add_capa;
} bap_capa_t;

/// Codec Capabilities structure (Additional Codec Capabilities provided as a pointer)
typedef struct bap_capa_ptr
{
    /// Parameters structure
    bap_capa_param_t param;
    /// Pointer to Additional Codec Capabilities (in LTV format)
    const gaf_ltv_t* p_add_capa;
} bap_capa_ptr_t;

/// Codec Capabilities Metadata parameters structure
typedef struct bap_capa_metadata_param
{
    /// Preferred Audio Contexts bit field (see #bap_context_type_bf enumeration)
    uint16_t context_bf;
} bap_capa_metadata_param_t;

/// Codec Capabilities Metadata structure
typedef struct bap_capa_metadata
{
    /// Parameters structure
    bap_capa_metadata_param_t param;
    /// Additional Metadata (in LTV format)
    gaf_ltv_t add_metadata;
} bap_capa_metadata_t;

/// Codec Capabilities Metadata structure (Additional Metadata provided as a pointer)
typedef struct bap_capa_metadata_ptr
{
    /// Parameters structure
    bap_capa_metadata_param_t param;
    /// Pointer to Additional Codec Capabilities (in LTV format)
    const gaf_ltv_t* p_add_metadata;
} bap_capa_metadata_ptr_t;

/// Codec Configuration parameters structure
typedef struct bap_cfg_param
{
    /// Audio Locations of the Audio Channels being configured for the codec (i.e the number of codec frames per
    /// block) and their ordering within a single block of codec frames
    /// When transmitted, part of Codec Specific Configuration only if not equal to 0
    /// When received, 0 shall be interpreted as a single channel with no specified Audio Location
    uint32_t location_bf;
    /// Length of a codec frame in octets
    uint16_t frame_octet;
    /// Sampling Frequency (see #bap_sampling_freq enumeration)
    uint8_t sampling_freq;
    /// Frame Duration (see #bap_frame_dur enumeration)
    uint8_t frame_dur;
    /// Number of blocks of codec frames that shall be sent or received in a single SDU
    uint8_t frames_sdu;
} bap_cfg_param_t;

/// Codec Configuration structure
typedef struct bap_cfg
{
    /// Parameters structure
    bap_cfg_param_t param;
    /// Additional Codec Configuration (in LTV format)
    gaf_ltv_t add_cfg;
} bap_cfg_t;

/// Codec Configuration structure (Additional Codec Configuration provided as a pointer)
typedef struct bap_cfg_ptr
{
    /// Parameters structure
    bap_cfg_param_t param;
    /// Pointer to Additional Codec Configuration (in LTV format)
    const gaf_ltv_t* p_add_cfg;
} bap_cfg_ptr_t;

/// Codec Configuration Metadata parameters structure
typedef struct bap_cfg_metadata_param
{
    /// Streaming Audio Contexts bit field (see #bap_context_type_bf enumeration)
    uint16_t context_bf;
} bap_cfg_metadata_param_t;

/// Codec Configuration Metadata structure
typedef struct bap_cfg_metadata
{
    /// Parameters structure
    bap_cfg_metadata_param_t param;
    /// Additional Metadata value (in LTV format)
    gaf_ltv_t add_metadata;
} bap_cfg_metadata_t;

///  Codec Configuration Metadata structure (with additional Metadata provided as pointer)
typedef struct bap_cfg_metadata_ptr
{
    /// Parameters structure
    bap_cfg_metadata_param_t param;
    /// Pointer to additional Metadata value (in LTV format)
    const gaf_ltv_t* p_add_metadata;
} bap_cfg_metadata_ptr_t;

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

typedef struct bap_capa_srv_cb bap_capa_srv_cb_t;
typedef struct bap_capa_cli_cb bap_capa_cli_cb_t;
typedef struct bap_uc_srv_cb bap_uc_srv_cb_t;
typedef struct bap_uc_cli_cb  bap_uc_cli_cb_t;
typedef struct bap_bc_src_cb bap_bc_src_cb_t;
typedef struct bap_bc_sink_cb   bap_bc_sink_cb_t;
typedef struct bap_bc_scan_cb bap_bc_scan_cb_t;
typedef struct bap_bc_assist_cb bap_bc_assist_cb_t;
typedef struct bap_bc_deleg_cb  bap_bc_deleg_cb_t;

/// Basic Audio Profile callback set for each roles
typedef struct bap_cb
{
    /// Capabilities Server callback functions
    const bap_capa_srv_cb_t* p_capa_srv_cb;
    /// Capabilities Client callback functions
    const bap_capa_cli_cb_t* p_capa_cli_cb;
    /// Unicast Server callback functions
    const bap_uc_srv_cb_t* p_uc_srv_cb;
    /// Unicast Client callbacks
    const bap_uc_cli_cb_t* p_uc_cli_cb;
    /// Broadcast Source callback functions
    const bap_bc_src_cb_t* p_bc_src_cb;
    /// Broadcast Sink callback functions
    const bap_bc_sink_cb_t* p_bc_sink_cb;
    /// Broadcast Scan callback functions
    const bap_bc_scan_cb_t* p_bc_scan_cb;
    /// Broadcast Scan Assistant callbacks
    const bap_bc_assist_cb_t* p_bc_assist_cb;
    /// Broadcast Scan Delegator callbacks
    const bap_bc_deleg_cb_t* p_bc_deleg_cb;
} bap_cb_t;

/// @} BAP_STRUCT

/*
 * API FUNCTION DEFINITION
 ****************************************************************************************
 */

/// @addtogroup BAP_NATIVE_API
/// @{

/**
 ****************************************************************************************
 * @brief Configure Basic Audio profile
 *
 * @param[in] role_bf           Supported role bit field (see #bap_role_bf enumeration)
 * @param[in] p_capa_srv_cfg    Pointer to Capabilities Server configuration
 * @param[in] p_capa_cli_cfg    Pointer to Capabilities Client configuration
 * @param[in] p_uc_srv_cfg      Pointer to Unicast Server configuration
 * @param[in] p_uc_cli_cfg      Pointer to Unicast Client configuration
 * @param[in] p_bc_scan_cfg     Pointer to Broadcast Scan configuration
 * @param[in] p_bc_deleg_cfg    Pointer to Broadcast Delegator configuration
 * @param[in] p_bc_assist_cfg   Pointer to Broadcast Assistant configuration
 * @param[in] p_cb              Pointer to callback structure
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_configure(uint32_t role_bf, bap_capa_srv_cfg_t* p_capa_srv_cfg, bap_capa_cli_cfg_t* p_capa_cli_cfg,
                       bap_uc_srv_cfg_t* p_uc_srv_cfg, bap_uc_cli_cfg_t* p_uc_cli_cfg,
                       bap_bc_scan_cfg_t* p_bc_scan_cfg, bap_bc_deleg_cfg_t* p_bc_deleg_cfg,
                       bap_bc_assist_cfg_t* p_bc_assist_cfg, const bap_cb_t* p_cb);

/// @} BAP_NATIVE_API

#endif // BAP_H_

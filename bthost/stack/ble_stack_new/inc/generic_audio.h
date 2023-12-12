/***************************************************************************
 *
 * Copyright (c) 2015-2023 BES Technic
 *
 * Authored by BES CD team (Blueelf Prj).
 *
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
#ifndef __GEN_AUD__
#define __GEN_AUD__

#include "bluetooth.h"
#include "bap_service.h"
#include "gaf_cfg.h"
/*Defination*/

#define GEN_AUD_CODEC_ID_LEN                    (5)

#define GEN_AUD_INVALID_CONTEXT                 (0xFFFF)

#define GEN_AUD_STREAM_LANG_LEN                 (3)

#define gen_aud_co_max(a, b)                    ((a) > (b) ? (a) : (b))

/*Enumeartion*/

/// Codec Type values
enum gen_aud_codec_type
{
    /// LC3 Codec
    GEN_AUD_CODEC_TYPE_LC3 = 0x06,
    /// Maximum SIG Codec
    GEN_AUD_CODEC_TYPE_SIG_MAX,
    /// Vendor Specific Codec
    GEN_AUD_CODEC_TYPE_VENDOR = 0xFF,
};

/// Codec Specific Capabilities Types values
enum generic_audio_codec_capa_type
{
    /// Supported Sampling Frequencies
    GEN_AUD_CAPA_TYPE_SUPP_SAMP_FREQ = 1,
    /// Supported Frame Durations
    GEN_AUD_CAPA_TYPE_SUPP_FRAME_DUR,
    /// Audio Channel Counts
    GEN_AUD_CAPA_TYPE_SUPP_CHNL_CNT,
    /// Supported Octets per Codec Frame
    GEN_AUD_CAPA_TYPE_SUPP_OCTS_FRAME,
    /// Maximum Supported Codec Frames per SDU
    GEN_AUD_CAPA_TYPE_SUPP_FRAMES_SDU,
};

/// Codec Specific Configuration Types values
enum generic_audio_codec_cfg_type
{
    /// Sampling Frequencies
    GEN_AUD_CC_TYPE_SAMP_FREQ = 1,
    /// Frame Duration
    GEN_AUD_CC_TYPE_FRAME_DUR,
    /// Audio Channel Allocation
    GEN_AUD_CC_TYPE_CHNL_ALLOC,
    /// Octets per Codec Frame
    GEN_AUD_CC_TYPE_OCTETS_FRAME,
    /// Codec Frame Blocks Per SDU
    GEN_AUD_CC_TYPE_FRAME_BLOCKS_SDU,
};

/// Metadata Types values
enum generic_audio_metadata_type
{
    /// Preferred Audio Contexts
    GEN_AUD_METADATA_TYPE_PREF_CONTEXTS = 1,
    /// Streaming Audio Contexts
    GEN_AUD_METADATA_TYPE_STREAM_CONTEXTS,
    /// UTF-8 encoded title or summary of stream content
    GEN_AUD_METADATA_TYPE_PROGRAM_INFO,
    /// Stream language
    GEN_AUD_METADATA_TYPE_STREAM_LANG,
    /// Array of 8-bit CCID values
    GEN_AUD_METADATA_TYPE_CCID_LIST,
    /// Parental rating @see generic_audio_parental_rating
    GEN_AUD_METADATA_TYPE_PARENTAL_RATING,
    /// UTF-8 encoded URI for additional Program information
    GEN_AUD_METADATA_TYPE_PROGRAM_INFO_URI,
    /// Audio_Active_State
    GEN_AUD_METADATA_TYPE_AUD_ACTIVE_STATE,
    /// Broadcast_Audio_Immediate_Rendering_Flag
    GEN_AUD_METADATA_TYPE_BCAST_AUD_IMME_REND_FLAG = 9,
    /// Extended
    GEN_AUD_METADATA_TYPE_EXTENDED = 0xFE,
    /// Vendor Specific
    GEN_AUD_METADATA_TYPE_VENDOR = 0xFF,
};

/// Sampling Frequency values
typedef enum generic_audio_sampling_freq
{
    GEN_AUD_SAMPLING_FREQ_MIN = 1,
    /// 8000 Hz
    GEN_AUD_SAMPLING_FREQ_8000HZ = GEN_AUD_SAMPLING_FREQ_MIN,
    /// 11025 Hz
    GEN_AUD_SAMPLING_FREQ_11025HZ,
    /// 16000 Hz
    GEN_AUD_SAMPLING_FREQ_16000HZ,
    /// 22050 Hz
    GEN_AUD_SAMPLING_FREQ_22050HZ,
    /// 24000 Hz
    GEN_AUD_SAMPLING_FREQ_24000HZ,
    /// 32000 Hz
    GEN_AUD_SAMPLING_FREQ_32000HZ,
    /// 44100 Hz
    GEN_AUD_SAMPLING_FREQ_44100HZ,
    /// 48000 Hz
    GEN_AUD_SAMPLING_FREQ_48000HZ,
    /// 88200 Hz
    GEN_AUD_SAMPLING_FREQ_88200HZ,
    /// 96000 Hz
    GEN_AUD_SAMPLING_FREQ_96000HZ,
    /// 176400 Hz
    GEN_AUD_SAMPLING_FREQ_176400HZ,
    /// 192000 Hz
    GEN_AUD_SAMPLING_FREQ_192000HZ,
    /// 384000 Hz
    GEN_AUD_SAMPLING_FREQ_384000HZ,

    /// Maximum value
    GEN_AUD_SAMPLING_FREQ_MAX
} gen_aud_sampling_freq_e;

/// Supported Sampling Frequencies bit field meaning
typedef enum generic_audio_sampling_freq_bf
{
    /// 8000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_8000HZ_BIT    = 0x0001,
    /// 11025 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_11025HZ_BIT   = 0x0002,
    /// 16000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_16000HZ_BIT   = 0x0004,
    /// 22050 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_22050HZ_BIT   = 0x0008,
    /// 24000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_24000HZ_BIT   = 0x0010,
    /// 32000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_32000HZ_BIT   = 0x0020,
    /// 44100 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_44100HZ_BIT   = 0x0040,
    /// 48000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_48000HZ_BIT   = 0x0080,
    /// 88200 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_88200HZ_BIT   = 0x0100,
    /// 96000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_96000HZ_BIT   = 0x0200,
    /// 176400 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_176400HZ_BIT  = 0x0400,
    /// 192000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_192000HZ_BIT  = 0x0800,
    /// 384000 Hz - Bit
    GEN_AUD_SAMPLING_FREQ_384000HZ_BIT  = 0x1000,
} gen_aud_sampling_freq_bf_e;

/// Frame Duration values
typedef enum generic_audio_frame_dur
{
    /// Use 7.5ms Codec frames
    GEN_AUD_FRAME_DUR_7_5MS = 0,
    /// Use 10ms Codec frames
    GEN_AUD_FRAME_DUR_10MS,
    /// Use 5ms Codec frames
    GEN_AUD_FRAME_DUR_5MS,
    /// Use 2.5ms Codec frames
    GEN_AUD_FRAME_DUR_2_5MS,
    /// Maximum value
    GEN_AUD_FRAME_DUR_MAX
} gen_aud_frame_dur_e;

/// Supported Frame Durations bit field meaning
typedef enum generic_audio_frame_dur_bf
{
    /// 7.5ms frame duration is supported - Bit
    GEN_AUD_FRAME_DUR_7_5MS_BIT         = 0x01,
    /// 10ms frame duration is supported - Bit
    GEN_AUD_FRAME_DUR_10MS_BIT          = 0x02,
    /// 5ms frame duration is supported - Bit
    GEN_AUD_FRAME_DUR_5MS_BIT           = 0x04,
    /// 2.5ms frame duration is supported - Bit
    GEN_AUD_FRAME_DUR_2_5MS_BIT         = 0x08,
    /// 7.5ms frame duration is preferred - Bit
    GEN_AUD_FRAME_DUR_7_5MS_PREF_BIT    = 0x10,
    /// 10ms frame duration is preferred - Bit
    GEN_AUD_FRAME_DUR_10MS_PREF_BIT     = 0x20,
    /// 5ms frame duration is supported - Bit
    GEN_AUD_FRAME_DUR_5MS_PREF_BIT      = 0x40,
    /// 2.5ms frame duration is supported - Bit
    GEN_AUD_FRAME_DUR_2_5MS_PREF_BIT    = 0x80,
} gen_aud_frame_dur_bf_e;

/// Context type bit field meaning
typedef enum generic_audio_context_type_bf
{
    /// Unspecified - Bit
    GEN_AUD_CONTEXT_TYPE_UNSPECIFIED_BIT       = 0x0001,
    /// Conversation between humans as, for example, in telephony or video calls
    GEN_AUD_CONTEXT_TYPE_CONVERSATIONAL_BIT    = 0x0002,
    /// Media as, for example, in music, public radio, podcast or video soundtrack.
    GEN_AUD_CONTEXT_TYPE_MEDIA_BIT             = 0x0004,
    /// Audio associated with video gaming, for example gaming media, gaming effects, music and in-game voice chat
    /// between participants; or a mix of all the above
    GEN_AUD_CONTEXT_TYPE_GAME_BIT              = 0x0008,
    /// Instructional audio as, for example, in navigation, traffic announcements or user guidance
    GEN_AUD_CONTEXT_TYPE_INSTRUCTIONAL_BIT     = 0x0010,
    /// Man machine communication as, for example, with voice recognition or virtual assistant
    GEN_AUD_CONTEXT_TYPE_MAN_MACHINE_BIT       = 0x0020,
    /// Live audio as from a microphone where audio is perceived both through a direct acoustic path and through
    /// an LE Audio Stream
    GEN_AUD_CONTEXT_TYPE_LIVE_BIT              = 0x0040,
    /// Sound effects including keyboard and touch feedback;
    /// menu and user interface sounds; and other system sounds
    GEN_AUD_CONTEXT_TYPE_SOUND_EFFECTS_BIT     = 0x0080,
    /// Attention seeking audio as, for example, in beeps signalling arrival of a message or keyboard clicks
    GEN_AUD_CONTEXT_TYPE_ATTENTION_SEEKING_BIT = 0x0100,
    /// Ringtone as in a call alert
    GEN_AUD_CONTEXT_TYPE_RINGTOME_BIT          = 0x0200,
    /// Immediate alerts as, for example, in a low battery alarm, timer expiry or alarm clock.
    GEN_AUD_CONTEXT_TYPE_IMMEDIATE_ALERT_BIT   = 0x0400,
    /// Emergency alerts as, for example, with fire alarms or other urgent alerts
    GEN_AUD_CONTEXT_TYPE_EMERGENCY_ALERT_BIT   = 0x0800,
    /// Audio associated with a television program and/or with metadata conforming to the Bluetooth Broadcast TV
    /// profile
    GEN_AUD_CONTEXT_TYPE_TV_BIT                = 0x1000,
} gen_aud_context_type_bf_e;

/// Supported Audio Location Bitfield
typedef enum generic_audio_supp_audio_locations_bf
{
    GEN_AUD_AUDIO_LOCATION_FRONT_LEFT               = 0x00000001,
    GEN_AUD_AUDIO_LOCATION_FRONT_RIGHT              = 0x00000002,
    GEN_AUD_AUDIO_LOCATION_FRONT_CENTER             = 0x00000004,
    GEN_AUD_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1       = 0x00000008,
    GEN_AUD_AUDIO_LOCATION_BACK_LEFT                = 0x00000010,
    GEN_AUD_AUDIO_LOCATION_BACK_RIGHT               = 0x00000020,
    GEN_AUD_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER     = 0x00000040,
    GEN_AUD_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER    = 0x00000080,
    GEN_AUD_AUDIO_LOCATION_BACK_CENTER              = 0x00000100,
    GEN_AUD_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2       = 0x00000200,
    GEN_AUD_AUDIO_LOCATION_SIDE_LEFT                = 0x00000400,
    GEN_AUD_AUDIO_LOCATION_SIDE_RIGHT               = 0x00000800,
    GEN_AUD_AUDIO_LOCATION_TOP_FRONT_LEFT           = 0x00001000,
    GEN_AUD_AUDIO_LOCATION_TOP_FRONT_RIGHT          = 0x00002000,
    GEN_AUD_AUDIO_LOCATION_TOP_FRONT_CENTER         = 0x00004000,
    GEN_AUD_AUDIO_LOCATION_TOP_CENTER               = 0x00008000,
    GEN_AUD_AUDIO_LOCATION_TOP_BACK_LEFT            = 0x00010000,
    GEN_AUD_AUDIO_LOCATION_TOP_BACK_RIGHT           = 0x00020000,
    GEN_AUD_AUDIO_LOCATION_TOP_SIDE_LEFT            = 0x00040000,
    GEN_AUD_AUDIO_LOCATION_TOP_SIDE_RIGHT           = 0x00080000,
    GEN_AUD_AUDIO_LOCATION_TOP_BACK_CENTER          = 0x00100000,
    GEN_AUD_AUDIO_LOCATION_BOTTOM_FRONT_CENTER      = 0x00200000,
    GEN_AUD_AUDIO_LOCATION_BOTTOM_FRONT_LEFT        = 0x00400000,
    GEN_AUD_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT       = 0x00800000,
    GEN_AUD_AUDIO_LOCATION_FRONT_LEFT_WIDE          = 0x01000000,
    GEN_AUD_AUDIO_LOCATION_FRONT_RIGHT_WIDE         = 0x02000000,
    GEN_AUD_AUDIO_LOCATION_LEFT_SURROUND            = 0x04000000,
    GEN_AUD_AUDIO_LOCATION_RIGHT_SURROUND           = 0x08000000,

    GEN_AUD_AUDIO_LOCATION_RFU                      = 0xF0000000,
} gen_aud_supp_loc_bf_e;

/**
 * @brief Parental rating defined by the assigned numbers
 *
 * The numbering scheme is aligned with Annex F of EN 300 707 v1.2.1 which
 * defined parental rating for viewing.
 */
enum generic_audio_parental_rating
{
    GEN_AUDIO_PARENTAL_RATING_NO_RATING        = 0x00,
    GEN_AUDIO_PARENTAL_RATING_AGE_ANY          = 0x01,
    GEN_AUDIO_PARENTAL_RATING_AGE_5_OR_ABOVE   = 0x02,
    GEN_AUDIO_PARENTAL_RATING_AGE_6_OR_ABOVE   = 0x03,
    GEN_AUDIO_PARENTAL_RATING_AGE_7_OR_ABOVE   = 0x04,
    GEN_AUDIO_PARENTAL_RATING_AGE_8_OR_ABOVE   = 0x05,
    GEN_AUDIO_PARENTAL_RATING_AGE_9_OR_ABOVE   = 0x06,
    GEN_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE  = 0x07,
    GEN_AUDIO_PARENTAL_RATING_AGE_11_OR_ABOVE  = 0x08,
    GEN_AUDIO_PARENTAL_RATING_AGE_12_OR_ABOVE  = 0x09,
    GEN_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE  = 0x0A,
    GEN_AUDIO_PARENTAL_RATING_AGE_14_OR_ABOVE  = 0x0B,
    GEN_AUDIO_PARENTAL_RATING_AGE_15_OR_ABOVE  = 0x0C,
    GEN_AUDIO_PARENTAL_RATING_AGE_16_OR_ABOVE  = 0x0D,
    GEN_AUDIO_PARENTAL_RATING_AGE_17_OR_ABOVE  = 0x0E,
    GEN_AUDIO_PARENTAL_RATING_AGE_18_OR_ABOVE  = 0x0F
};

/// Minimal value of length field for Codec Specific Capabilities LTV structure
enum generic_audio_capa_length
{
    /// Supported Sampling Frequencies
    GEN_AUD_CAPA_LENGTH_SAMP_FREQ = 3,
    /// Supported Frame Durations
    GEN_AUD_CAPA_LENGTH_FRAME_DUR = 2,
    /// Audio Channel Counts
    GEN_AUD_CAPA_LENGTH_CHNL_CNT = 2,
    /// Supported Octets per Codec Frame
    GEN_AUD_CAPA_LENGTH_OCTETS_FRAME = 5,
    /// Maximum Supported Codec Frames per SDU
    GEN_AUD_CAPA_LENGTH_FRAMES_SDU = 2,
};

/// LTV structure format
enum gen_aud_ltv_fmt
{
    /// Length
    GEN_AUD_LTV_LENGTH_POS = 0,
    /// Type
    GEN_AUD_LTV_TYPE_POS,
    /// Value
    GEN_AUD_LTV_VALUE_POS,

    /// Minimal length of LTV structure
    GEN_AUD_LTV_LENGTH_MIN = 1,
};

/// Minimal value of length field for Codec Specific Configuration LTV structure
enum generic_audio_codec_cfg_length
{
    /// Sampling Frequencies
    GEN_AUD_CC_LENGTH_SAMP_FREQ = 2,
    /// Frame Duration
    GEN_AUD_CC_LENGTH_FRAME_DUR = 2,
    /// Audio Channel Allocation
    GEN_AUD_CC_LENGTH_CHNL_LOCATION = 5,
    /// Octets per Codec Frame
    GEN_AUD_CC_LENGTH_OCTETS_FRAME = 3,
    /// Codec Frame Blocks Per SDU
    GEN_AUD_CC_LENGTH_FRAMES_SDU = 2,
};

/*Type Define*/
/*****************************************************************************************************
 *                                                                                                   *
 *                                          CAPA                                                     *
 *                                                                                                   *
 *                                                                                                   *
 *****************************************************************************************************/
/*****************************CODEC SPECIFIC CAPABILTY STRUCTURE PACKED*******************************/
typedef struct generic_audio_codec_capa_samp_freq_packed
{
    uint8_t len;
    uint8_t type;
    uint16_t samp_freq_bf;
} __attribute__((packed)) gen_aud_capa_samp_freq_p_t;

typedef struct generic_audio_codec_capa_frame_dur_packed
{
    uint8_t len;
    uint8_t type;
    uint8_t frame_dur_bf;
} __attribute__((packed)) gen_aud_capa_frame_dur_p_t;

typedef struct generic_audio_codec_capa_chn_cnt_packed
{
    uint8_t len;
    uint8_t type;
    uint8_t chan_cnt_bf;
} __attribute__((packed)) gen_aud_capa_chan_cnt_p_t;

typedef struct generic_audio_codec_capa_supp_octs_frame_packed
{
    uint8_t len;
    uint8_t type;
    uint16_t octs_min;
    uint16_t octs_max;
} __attribute__((packed)) gen_aud_capa_supp_octs_p_t;

typedef struct generic_audio_codec_capa_frames_sdu_packed
{
    uint8_t len;
    uint8_t type;
    uint8_t frames_per_sdu;
} __attribute__((packed)) gen_aud_capa_frames_sdu_p_t;

typedef struct generic_audio_metadata_context_packed
{
    uint8_t len;
    uint8_t type;
    uint16_t context_bf;
} __attribute__((packed)) gen_aud_metadata_context_p_t;

/************************************************CODEC SPECIFIC CAPABLITY PARAMETER**********************************************/
typedef struct gen_aud_codec_capa_param
{
    /// Supported Sampling Frequencies bit field (see #gen_aud_sampling_freq_bf enumeration)\n
    /// 0 means that the field is not part of the Codec Specific Capabilities\n
    /// Mandatory for LC3
    uint16_t sampling_freq_bf;
    /// Supported Frame Durations bit field (see #gen_aud_frame_dur_bf enumeration)\n
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
} gen_aud_codec_capa_param_t;

/*****************************************************************************************************
 *                                                                                                   *
 *                                          CODEC CFG                                                *
 *                                                                                                   *
 *                                                                                                   *
 *****************************************************************************************************/
/*****************************CODEC SPECIFIC CONFIGUREATION STRUCTURE PACKED*******************************/
typedef struct generic_audio_codec_cfg_samp_freq_packed
{
    uint8_t len;
    uint8_t type;
    uint8_t samp_freq;
} __attribute__((packed)) gen_aud_cc_samp_freq_p_t;

typedef struct generic_audio_codec_cfg_frame_dur_packed
{
    uint8_t len;
    uint8_t type;
    uint8_t frame_dur;
} __attribute__((packed)) gen_aud_cc_frame_dur_p_t;

typedef struct generic_audio_codec_cfg_audio_allocation_packed
{
    uint8_t len;
    uint8_t type;
    uint32_t audio_chan_alloc;
} __attribute__((packed)) gen_aud_cc_aud_chan_alloc_p_t;

typedef struct generic_audio_codec_cfg_octs_per_frame_packed
{
    uint8_t len;
    uint8_t type;
    uint16_t octs_per_frame;
} __attribute__((packed)) gen_aud_cc_frame_octs_p_t;

typedef struct generic_audio_codec_cfg_frames_sdu_packed
{
    uint8_t len;
    uint8_t type;
    uint8_t frame_blocks_per_sdu;
} __attribute__((packed)) gen_aud_cc_frame_blocks_sdu_p_t;

/*****************************CODEC SPECIFIC CONFIGURATION PARAMETER*********************************/
typedef struct gen_aud_codec_configuration_param
{
    /// Audio Channel Allocation
    uint32_t audio_chan_allocation_bf;
    /// Octets Per Codec Frame
    uint16_t frame_octets;
    /// Sampling Frequencies
    uint8_t sampling_freq;
    /// Frame Duration
    uint8_t frame_dur;
    /// Frame blocks per sud
    uint8_t frame_blocks_per_sdu;
} gen_aud_codec_cfg_param_t;

/*****************************************************************************************************
 *                                                                                                   *
 *                                          METADATA                                                 *
 *                                                                                                   *
 *                                                                                                   *
 *****************************************************************************************************/
/***********************************METADATA************************************/

typedef struct generic_audio_variable_list
{
    /// len of data
    uint8_t len;
    /// data
    uint8_t data[GAF_ARRAY_EMPTY];
} gen_aud_var_info_t;

typedef struct generic_audio_ltv
{
    /// len of data
    uint8_t len;
    /// Type
    uint8_t type;
    /// data
    uint8_t data[GAF_ARRAY_EMPTY];
} gen_aud_ltv_t;

typedef struct generic_audio_metadata_ltv_ptr
{
    /// Array of 8-bit CCID values
    const gen_aud_ltv_t *p_ccid_list;
    /// UTF-8 encoded title or summary of stream content
    const gen_aud_ltv_t *p_program_info;
    /// UTF-8 encoded URI for additional Program information
    const gen_aud_ltv_t *p_add_prg_info;
    /// Extended
    const gen_aud_ltv_t *p_extended_md;
    /// Vendor
    const gen_aud_ltv_t *p_vendor_info;
} gen_aud_metadata_ltv_ptr_t;

typedef struct generic_audio_metadata_param
{
    /// Preferred Audio Contexts
    uint16_t preferred_audio_context;
    /// Streaming Audio Contexts
    uint16_t streaming_audio_context;
    /// Stream language
    uint8_t stream_language[GEN_AUD_STREAM_LANG_LEN];
    /// Parental rating @see generic_audio_parental_rating
    uint8_t parental_rating;
    /// Audio Active State
    uint8_t audio_active_state;
    /// Broadcast audio immediate rendering flag
    uint8_t bcast_imme_rend_flag_present;
} gen_aud_metadata_param_t;

extern __CONST gen_aud_metadata_param_t init_baisc_md;

/*PACS CAPA*/
typedef struct generic_audio_capa
{
    gen_aud_codec_capa_param_t basic_capa_param;
    gen_aud_var_info_t add_capa_param;
} gen_aud_capa_t;

typedef struct generic_audio_capa_ptr
{
    gen_aud_codec_capa_param_t basic_capa_param;
    const gen_aud_var_info_t *p_add_capa_param;
} gen_aud_capa_ptr_t;

/*ASCS CC*/
typedef struct generic_audio_codec_cfg
{
    gen_aud_codec_cfg_param_t basic_cc_param;
    gen_aud_var_info_t add_cc_param;
} gen_aud_cc_t;

typedef struct generic_audio_codec_cfg_ptr
{
    gen_aud_codec_cfg_param_t basic_cc_param;
    const gen_aud_var_info_t *p_add_cc_param;
} gen_aud_cc_ptr_t;

/*Metadata*/
typedef struct generic_audio_metadata
{
    /// Baisc metadata for parsed value
    gen_aud_metadata_param_t basic_metadata;
    /// Additonal metadata
    gen_aud_var_info_t add_metadata;
} gen_aud_metadata_t;

typedef struct generic_audio_metadata_ptr
{
    /// Parsed metatdaa
    gen_aud_metadata_param_t basic_metadata;
    /// Parsed LTV Metatdata pointer
    gen_aud_metadata_ltv_ptr_t parsed_metadata_ptr;
} gen_aud_metadata_ptr_t;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check if a list of values in LTV format is properly formatted
 *
 * @param[in] p_ltv_data      Pointer to LTV data structure
 *
 * @return An error status
 *  - BT_STS_SUCCESS if data is properly formatted
 *  - BT_STS_INVALID_PARM if data is not properly formatted
 ****************************************************************************************
 */
int gen_aud_ltv_check(const gen_aud_var_info_t *p_ltv_data);

#if (BAP_ASCS_ENABLE || BAP_ASCC_ENABLE)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_cfg_check(const uint8_t *p_codec_id, const gen_aud_cc_t *p_cc,
                            const gen_aud_metadata_t *p_metadata, uint8_t *p_cc_ltv_len,
                            uint8_t *p_metadata_ltv_len);
#endif //(BAP_ASCS_ENABLE || BAP_ASCC_ENABLE)

#if (BAP_ASCS_ENABLE || BAP_ASCC_ENABLE || BAP_BC_SRC_ENABLE)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_cfg_pack(uint8_t *p_data, const gen_aud_cc_t *p_cc, bool with_add_cfg);
#endif //(BAP_ASCS_ENABLE || BAP_ASCC_ENABLE || BAP_BC_SRC_ENABLE)

#if (BAP_ASCS_ENABLE || BAP_ASCC_ENABLE)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_cfg_unpack(const uint8_t *p_codec_id, gen_aud_cc_ptr_t *p_cc,
                             gen_aud_var_info_t *p_capa_ltv);
#endif //(BAP_ASCS_ENABLE || BAP_ASCC_ENABLE)

#if (BAP_PACS_ENABLE || BAP_PACC_ENABLE)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_capa_check(const uint8_t *p_codec_id, const gen_aud_capa_t *p_capa,
                             const gen_aud_metadata_t *p_metadata, uint16_t *p_ltv_len);
#endif //(BAP_PACS_ENABLE || BAP_PACC_ENABLE)

#if (BAP_PACS_ENABLE)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_capa_pack(uint8_t *p_data, const gen_aud_capa_t *p_capa);

#endif //(BAP_PACS_ENABLE)

#if (BAP_PACC_ENABLE)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_capa_unpack(const uint8_t *p_codec_id, gen_aud_capa_ptr_t *p_capa,
                              gen_aud_var_info_t *p_capa_ltv);
#endif //(BAP_PACC_ENABLE)

#if (BAP_ASCS_ENABLE || BAP_ASCC_ENABLE || GAF_BAP_BC_DELEG || BAP_BROADCAST_ASSIST)
/**
 ****************************************************************************************
 * @brief Init gen_aud_metadata_t
 ****************************************************************************************
 */
void gen_aud_init_metadata(gen_aud_metadata_t *p_metadata);

/**
 ****************************************************************************************
 * @brief Init gen_aud_metadata_ptr_t
 ****************************************************************************************
 */
void gen_aud_init_metadata_ptr(gen_aud_metadata_ptr_t *p_metadata_ptr);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_pack_metadata(uint8_t *p_data, const gen_aud_metadata_t *p_metadata);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_calc_basic_metadata_ltv_value_len(const gen_aud_metadata_param_t *p_basic_metadata);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_calc_metadata_ltv_list_value_len(const gen_aud_metadata_ltv_ptr_t *p_metadata_ltv);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_memcpy_s_metadata_ltv_list_value(const gen_aud_metadata_ltv_ptr_t *p_metadata_src,
                                                   uint8_t *dest_mem, uint8_t dest_mem_len);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_unpack_metadata_with_err_type_ret(const uint8_t *p_codec_id, gen_aud_metadata_ptr_t *p_metadata,
                                              gen_aud_var_info_t *p_metadata_ltv, uint8_t *p_err_md_type_ret);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
int gen_aud_codec_unpack_metadata(const uint8_t *p_codec_id, gen_aud_metadata_ptr_t *p_metadata,
                                  gen_aud_var_info_t *p_metadata_ltv);
#endif //(BAP_ASCS_ENABLE || BAP_ASCC_ENABLE || GAF_BAP_BC_DELEG || BAP_BROADCAST_ASSIST)

/**
 ****************************************************************************************
 * @brief Check if a Codec ID is Codec ID for LC3
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is for LC3, else false
 ****************************************************************************************
 */
bool gen_aud_codec_is_lc3(const uint8_t *p_codec_id);

/**
 ****************************************************************************************
 * @brief Check if a Codec ID is Codec ID for vendor specific data
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is for vendor, else false
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check if a Codec ID is Codec ID for LC3plus
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is for LC3plus, else false
 ****************************************************************************************
 */
bool gen_aud_codec_is_lc3plus(const uint8_t *p_codec_id);

/**
 ****************************************************************************************
 * @brief Check if a Codec ID is valid
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is valid else false
 ****************************************************************************************
 */
bool gen_aud_codec_is_id_valid(const uint8_t *p_codec_id);

#if (BAP_ASCS_ENABLE || BAP_ASCC_ENABLE || GEN_AUD_BC_DELEG || GEN_AUD_BC_ASSIST)
/**
 ****************************************************************************************
 * @brief Print Codec ID
 ****************************************************************************************
 */
void gen_aud_codec_id_print(const uint8_t *p_codec_id);

/**
 ****************************************************************************************
 * @brief Print Metadata
 ****************************************************************************************
 */
void gen_aud_metadata_print(const gen_aud_metadata_t *p_metadata);

/**
 ****************************************************************************************
 * @brief Print Codec Specific configuration
 ****************************************************************************************
 */
void gen_aud_codec_cfg_print(const uint8_t *p_codec_id, const gen_aud_cc_t *p_codec_cfg,
                             const gen_aud_metadata_t *p_metadata);

/**
 ****************************************************************************************
 * @brief Print Codec capability
 ****************************************************************************************
 */
void gen_aud_codec_capa_print(const uint8_t *p_codec_id, const gen_aud_capa_t *p_capa,
                              const gen_aud_metadata_t *p_metadata);
#endif // (BAP_ASCS_ENABLE || BAP_ASCC_ENABLE || GEN_AUD_BC_DELEG || GEN_AUD_BC_ASSIST)

#endif /// __GEN_AUD__
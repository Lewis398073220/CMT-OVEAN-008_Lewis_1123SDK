/**
 ****************************************************************************************
 *
 * @file bap_capa_pacs_int.h
 *
 * @brief Basic Audio Profile - Capabilities - Published Audio Capabilities Service Internal Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef BAP_CAPA_PACS_INT_H_
#define BAP_CAPA_PACS_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "bap.h"            // Basic Audio Profile Definitions
#include "gaf_inc.h"        // Generic Audio Framework Included Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Position of fields in Sink/Source PAC characteristic value
enum bap_capa_pac_pos
{
    /// Number of PAC record
    BAP_CAPA_PAC_NB_RECORDS_POS = 0,

    BAP_CAPA_PAC_LEN_MIN,

    /// Codec ID
    BAP_CAPA_PAC_CODEC_ID_POS = 0,

    /// Length of Codec Specific Capabilities
    BAP_CAPA_PAC_CODEC_CAPA_LEN_POS = 0,
    /// Codec Specific Capabilities
    BAP_CAPA_PAC_CODEC_CAPA_POS,

    /// Length of Metadata
    BAP_CAPA_PAC_METADATA_LEN_POS = 0,
    /// Metadata
    BAP_CAPA_PAC_METADATA_POS,

    BAP_CAPA_PAC_RECORD_LEN_MIN = 2 + GAF_CODEC_ID_LEN,
};

/// Position of fields in Available Audio Contexts and Supported Audio Contexts characteristic value
enum bap_capa_context_pos
{
    /// Length of context field for a direction
    BAP_CAPA_CONTEXT_DIR_LEN = 2,

    /// Contexts for Sink direction
    BAP_CAPA_CONTEXT_SINK_POS = 0,
    /// Contexts for Source direction
    BAP_CAPA_CONTEXT_SRC_POS = BAP_CAPA_CONTEXT_SINK_POS + BAP_CAPA_CONTEXT_DIR_LEN,

    /// Length of characteristic value
    BAP_CAPA_CONTEXT_LEN = BAP_CAPA_CONTEXT_SRC_POS + BAP_CAPA_CONTEXT_DIR_LEN,
};

/// Position of fields in Sink/Source Audio Locations characteristic value
enum bap_capa_audio_loc_pos
{
    /// Length of Sink/Source characteristic value
    BAP_CAPA_LOCATION_LEN = 4,

    /// Location
    BAP_CAPA_LOCATION_POS = 0,
};

/// Descriptor type values for Published Audio Capabilities Service
enum bap_pacs_desc_type
{
    /// Available Audio Contexts characteristic
    BAP_CAPA_DESC_TYPE_CCC_CONTEXT_AVA = 0,
    /// Supported Audio Contexts characteristic
    BAP_CAPA_DESC_TYPE_CCC_CONTEXT_SUPP,
    /// Sink Audio Locations characteristic
    BAP_CAPA_DESC_TYPE_CCC_LOC_SINK,
    /// Source Audio Locations characteristic
    BAP_CAPA_DESC_TYPE_CCC_LOC_SRC,
    /// Sink PAC characteristic
    BAP_CAPA_DESC_TYPE_CCC_PAC,

    BAP_CAPA_DESC_TYPE_MAX,
};

#endif // BAP_CAPA_H_

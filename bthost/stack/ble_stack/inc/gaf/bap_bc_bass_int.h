/**
 ****************************************************************************************
 *
 * @file bap_bc_bass_int.h
 *
 * @brief Basic Audio Profile - Broadcast Audio Scan Service Internals
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef BAP_BC_BASS_INT_H_
#define BAP_BC_BASS_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "bap.h"            // Basic Audio Profile Definitions
#include "gaf_inc.h"        // Generic Audio Framework Included Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum length supported for Metadata
#define BAP_BC_METADATA_LEN_MAX       (255)
/// Minimal length of Broadcast Audio Scan Control Point characteristic value
#define BAP_BC_CP_LEN_MIN             (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Position of fields in Broadcast Audio Scan Control Point characteristic
enum bap_bc_cp_pos
{
    /// Operation Code
    BAP_BC_CP_OPCODE_POS = 0,
};

/// Position of fields in Broadcast Audio Scan Control Point characteristic value for Add Source operation
enum bap_bc_cp_add_source_pos
{
    /// Operation Code
    BAP_BC_CP_ADD_SRC_OPCODE_POS = 0,
    /// Advertiser Address Type
    BAP_BC_CP_ADD_SRC_ADV_ADDR_TYPE_POS,
    /// Advertiser Address
    BAP_BC_CP_ADD_SRC_ADV_ADDR_POS,
    /// Advertising SID
    BAP_BC_CP_ADD_SRC_ADV_SID_POS = BAP_BC_CP_ADD_SRC_ADV_ADDR_POS + 6,
    /// Broadcast ID
    BAP_BC_CP_ADD_SRC_BROADCAST_ID,
    /// PA Sync
    BAP_BC_CP_ADD_SRC_PA_SYNC_POS = BAP_BC_CP_ADD_SRC_BROADCAST_ID + 3,
    /// PA Interval
    BAP_BC_CP_ADD_SRC_PA_INTV_POS,
    /// Number of Subgroups
    BAP_BC_CP_ADD_SRC_NB_SUBGROUPS_POS = BAP_BC_CP_ADD_SRC_PA_INTV_POS + 2,

    /// Subgroup information
    BAP_BC_CP_ADD_SRC_SUBGROUP_INFO_POS,
    /// BIS Sync
    BAP_BC_CP_ADD_SRC_BIS_SYNC_POS = 0,
    /// Metadata length
    BAP_BC_CP_ADD_SRC_METADATA_LEN_POS = BAP_BC_CP_ADD_SRC_BIS_SYNC_POS + 4,
    /// Metadata
    BAP_BC_CP_ADD_SRC_METADATA_POS,

    /// Minimal length of Broadcast Audio Scan Control Point characteristic value for Add Source operation
    BAP_BC_CP_ADD_SRC_MIN_LEN = BAP_BC_CP_ADD_SRC_SUBGROUP_INFO_POS,
    /// Minimal length of Subgroup information part
    BAP_BC_CP_ADD_SRC_SUBGROUP_MIN_LEN = BAP_BC_CP_ADD_SRC_METADATA_POS,
};

/// Position of fields in Broadcast Audio Scan Control Point characteristic value for Modify Source operation
enum bap_bc_cp_modify_source_pos
{
    /// Operation Code
    BAP_BC_CP_MODIFY_SRC_OPCODE_POS = 0,
    /// Source ID
    BAP_BC_CP_MODIFY_SRC_SRC_ID_POS,
    /// PA Sync
    BAP_BC_CP_MODIFY_SRC_PA_SYNC_POS,
    /// PA Interval
    BAP_BC_CP_MODIFY_SRC_PA_INTV_POS,
    /// Number of Subgroups
    BAP_BC_CP_MODIFY_SRC_NB_SUBGROUPS_POS = BAP_BC_CP_MODIFY_SRC_PA_INTV_POS + 2,

    /// Subgroup information
    BAP_BC_CP_MODIFY_SRC_SUBGROUP_INFO_POS,
    /// BIS Sync
    BAP_BC_CP_MODIFY_SRC_BIS_SYNC_POS = 0,
    /// Metadata length
    BAP_BC_CP_MODIFY_SRC_METADATA_LEN_POS = BAP_BC_CP_MODIFY_SRC_BIS_SYNC_POS + 4,
    /// Metadata
    BAP_BC_CP_MODIFY_SRC_METADATA_POS,

    /// Length of Broadcast Audio Scan Control Point characteristic value for Modify Source operation
    BAP_BC_CP_MODIFY_SRC_MIN_LEN = BAP_BC_CP_MODIFY_SRC_SUBGROUP_INFO_POS,
    /// Minimal length of Subgroup information part
    BAP_BC_CP_MODIFY_SRC_SUBGROUP_MIN_LEN = BAP_BC_CP_MODIFY_SRC_METADATA_POS,
};

/// Position of fields in Broadcast Audio Scan Control Point characteristic value for Set Broadcast
/// Code operation
enum bap_bc_cp_set_bcast_code
{
    /// Operation Code
    BAP_BC_CP_SET_BROADCAST_CODE_OPCODE_POS = 0,
    /// Source ID
    BAP_BC_CP_SET_BROADCAST_CODE_SRC_ID_POS = 1,
    /// Broadcast Code
    BAP_BC_CP_SET_BROADCAST_CODE_CODE_POS = 2,

    /// Length of Broadcast Audio Scan Control Point characteristic value for Set Broadcast
    /// Code operation
    BAP_BC_CP_SET_BROADCAST_CODE_LEN = 18,
};

/// Position of fields in Broadcast Audio Scan Control Point characteristic value for Remove Source
/// operation
enum bap_bc_cp_remove_source
{
    /// Operation Code
    BAP_BC_CP_REMOVE_SRC_OPCODE_POS = 0,
    /// Source ID
    BAP_BC_CP_REMOVE_SRC_SRC_ID_POS = 1,

    /// Length of Broadcast Audio Scan Control Point characteristic value for Remove Source opearation
    BAP_BC_CP_REMOVE_SRC_LEN = 2,
};

/// Values for PA Sync field in Broadcast Audio Scan Control Point characteristic value
enum bap_bc_pa_sync
{
    /// Do not synchronize to PA
    BAP_BC_PA_SYNC_NO_SYNC = 0,
    /// Synchronize to PA, PAST on client
    BAP_BC_PA_SYNC_SYNC_PAST,
    /// Synchronize to PA, no PAST on client
    BAP_BC_PA_SYNC_SYNC_NO_PAST,

    BAP_BC_PA_SYNC_MAX
};

/// Position of fields in Broadcast Receive State characteristic value
enum bap_bc_rx_state_pos
{
    /// Source ID
    BAP_BC_RX_STATE_SRC_ID_POS = 0,
    /// Source Address Type
    BAP_BC_RX_STATE_SRC_ADDR_TYPE_POS,
    /// Source Address
    BAP_BC_RX_STATE_SRC_ADDR_POS,
    /// Source Advertising SID
    BAP_BC_RX_STATE_SRC_ADV_SID_POS = BAP_BC_RX_STATE_SRC_ADDR_POS + 6,
    /// Broadcast ID
    BAP_BC_RX_STATE_BCAST_ID_POS,
    /// PA Sync State
    BAP_BC_RX_STATE_PA_SYNC_STATE_POS = BAP_BC_RX_STATE_BCAST_ID_POS + 3,
    /// BIG Encryption
    BAP_BC_RX_STATE_BIG_ENCRYPTION_POS,
    /// Bad Code
    BAP_BC_RX_STATE_BAD_CODE_POS,

    /// BIS Sync State
    BAP_BC_RX_STATE_BIS_SYNC_STATE_POS = 0,
    /// Metadata length
    BAP_BC_RX_STATE_METADATA_LEN_POS = BAP_BC_RX_STATE_BIS_SYNC_STATE_POS + 4,
    /// Metadata
    BAP_BC_RX_STATE_METADATA_POS,

    /// Length of Broadcast Receive State characteristic value (+ 1 for Number of Subgroups field)
    BAP_BC_RX_STATE_MIN_LEN = BAP_BC_RX_STATE_BAD_CODE_POS + 1,
    /// Minimum length of Subgroup information
    BAP_BC_RX_STATE_SGRP_MIN_LEN = BAP_BC_RX_STATE_METADATA_POS,
};

/// Source Address Type values // TODO should use the GAP define instead
enum bap_bc_src_addr_type
{
    /// Public Device Address
    BAP_BC_SRC_ADDR_TYPE_PUBLIC = 0,
    /// Random Device Address
    BAP_BC_SRC_ADDR_TYPE_RANDOM,

    /// Maximum allowed value
    BAP_BC_SRC_ADDR_TYPE_MAX,
};

/// Operation codes for Broadcast Audio Scan Control Point characteristic
enum bap_bc_opcodes
{
    /// Remote Scan Stop
    BAP_BC_OPCODE_REMOTE_SCAN_STOPPED = 0,
    /// Remote Scan Start
    BAP_BC_OPCODE_REMOTE_SCAN_STARTED,
    /// Add Source
    BAP_BC_OPCODE_ADD_SOURCE,
    /// Modify Source
    BAP_BC_OPCODE_MODIFY_SOURCE,
    /// Set Broadcast Code
    BAP_BC_OPCODE_SET_BROADCAST_CODE,
    /// Remove Source
    BAP_BC_OPCODE_REMOVE_SOURCE,

    BAP_BC_OPCODE_MAX,
};

#endif // BAP_BC_BASS_INT_H_

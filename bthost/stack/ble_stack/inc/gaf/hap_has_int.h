/**
 ****************************************************************************************
 *
 * @file hap_has_int.h
 *
 * @brief Hearing Aid Profile - Hearing Aid Service Internal Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef HAP_HAS_INT_H_
#define HAP_HAS_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hap.h"         // Hearing Aid Profile Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Operation codes for Preset Control Point characteristic
enum hap_has_cp_opcode
{
    /// Write Preset Name
    HAP_HAS_CP_OPCODE_WRITE_PRESET_NAME = 0,
    /// Set Preset
    HAP_HAS_CP_OPCODE_SET_PRESET,
    /// Set Next Preset
    HAP_HAS_CP_OPCODE_SET_NEXT_PRESET,
    /// Set Previous Preset
    HAP_HAS_CP_OPCODE_SET_PREV_PRESET,
    /// Set Preset - Coordinated
    HAP_HAS_CP_OPCODE_SET_PRESET_COORD,
    /// Set Next Preset - Coordinated
    HAP_HAS_CP_OPCODE_SET_NEXT_PRESET_COORD,
    /// Set Previous Preset - Coordinated
    HAP_HAS_CP_OPCODE_SET_PREV_PRESET_COORD,

    HAP_HAS_CP_OPCODE_MAX,
};

/// Preset Record formatting
enum hap_has_preset_fmt
{
    /// Index field
    HAP_HAS_PRESET_INDEX_POS = 0,
    /// Read only field
    HAP_HAS_PRESET_READ_ONLY_POS,
    /// Length
    HAP_HAS_PRESET_LENGTH_POS,
    /// Name
    HAP_HAS_PRESET_NAME_POS,

    /// Minimal length of a Preset Record
    HAP_HAS_PRESET_LEN_MIN = HAP_HAS_PRESET_NAME_POS,
};

/// Active Preset characteristic value formatting
enum hap_has_active_preset_fmt
{
    /// Active preset index field
    HAP_HAS_ACTIVE_PRESET_INDEX_POS = 0,

    /// Length of Active Preset characteristic value
    HAP_HAS_ACTIVE_PRESET_LEN,
};

/// Flags characteristic value formatting
enum hap_has_flags_fmt
{
    /// Flags fields
    HAP_HAS_FLAGS_FLAGS_POS = 0,

    /// Length of Flags characteristic value
    HAP_HAS_FLAGS_LEN,
};

/// Preset Control Point characteristic value formatting
enum hap_has_preset_cp_fmt
{
    /// Opcode fields
    HAP_HAS_PRESET_CP_OPCODE_POS = 0,
    /// Preset Index
    HAP_HAS_PRESET_CP_INDEX_POS,
    /// Name
    HAP_HAS_PRESET_CP_NAME_POS,

    /// Minimal length of Preset Control Point characteristic value
    HAP_HAS_PRESET_CP_LEN_MIN = HAP_HAS_PRESET_CP_INDEX_POS,
    /// Maximal length of Preset Control Point characteristic value
    HAP_HAS_PRESET_CP_LEN_MAX = HAP_HAS_PRESET_CP_LEN_MIN + HAP_HAS_PRESET_NAME_LEN_MAX,
    /// Minimal length of Preset Control Point characteristic value when index is provided
    HAP_HAS_PRESET_CP_LEN_MIN_INDEX = HAP_HAS_PRESET_CP_NAME_POS,
};

#endif // HAP_HAS_INT_H_


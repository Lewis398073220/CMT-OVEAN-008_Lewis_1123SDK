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


#ifndef _AMSC_H_
#define _AMSC_H_

/**
 ****************************************************************************************
 * @addtogroup AMSC.
 * @ingroup AMS
 * @brief AMS - Client Role.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_AMS_CLIENT)
#include "ams_common.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of AMSC task instances
#define AMSC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the amsc task
enum amsc_states
{
    /// Idle state
    AMSC_FREE,
    /// Connected state
    AMSC_IDLE,
    /// Discovery
    AMSC_DISCOVERING,
    /// Number of defined states.
    AMSC_STATE_MAX
};

/// AMS Characteristics
enum amsc_chars
{
    /// Remote Command
    AMSC_REMOTE_COMMAND_CHAR = 0,
    /// Entity Update
    AMSC_ENTITY_UPDATE_CHAR,
    /// Entity Attribute
    AMSC_ENTITY_ATTRIBUTE_CHAR,
    /// Number of defined states.
    AMSC_CHAR_MAX,
};

/// AMS Characteristic Descriptors
enum amsc_descs
{
    /// Remote Command - Client Characteristic Configuration
    AMSC_DESC_REMOTE_CMD_CL_CFG,
    /// Entity Update - Client Characteristic Configuration
    AMSC_DESC_ENTITY_UPDATE_CL_CFG,

    AMSC_DESC_MAX,

    AMSC_DESC_MASK = 0x10,
};

/// Pointer to the connection clean-up function
#define AMSC_CLEANUP_FNCT       (amsc_cleanup)

#define AMSC_INVALID_STATE      0xA0
#define AMSC_INVALID_COMMAND    0xA1
#define AMSC_ABSENT_ATTRIBUTE   0xA2

/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Apple Media Service
 */
struct amsc_ams_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Remote Command
    ///  - Entity Update
    ///  - Entity Attribute
    prf_char_t chars[AMSC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Remote Command Client Char Cfg
    ///  - Entity Update Client Char Cfg
    prf_desc_t descs[AMSC_DESC_MAX];
};

struct amsc_cnx_env
{
    /// Current Operation
    void *operation;

    /// Provide an indication about the last operation
    uint16_t last_req;
    /// Last characteristic code discovered
    uint8_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// AMS Characteristics
    struct amsc_ams_content ams;
};

/// AMS Client environment variable
typedef PRF_ENV_TAG(amsc)
{
    /// profile environment
    prf_hdr_t prf_env;
    /// Service Start Handle
    uint16_t shdl;
    /// flag to mark whether notification or indication is enabled
    uint16_t ntfIndEnableFlag[AMSC_IDX_MAX];

    ams_defer_cfm_info_t rdInfo;
    ams_defer_cfm_info_t wrInfo[AMSC_IDX_MAX];

    /// Environment variable pointer for each connections
    struct amsc_cnx_env* env[AMSC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[AMSC_IDX_MAX];
    /// GATT User local index
    uint8_t user_lid;
} PRF_ENV_T(amsc);

enum amsc_remote_command_id
{
    RemoteCommandIDPlay                           = 0,
    RemoteCommandIDPause                          = 1,
    RemoteCommandIDTogglePlayPause                = 2,
    RemoteCommandIDNextTrack                      = 3,
    RemoteCommandIDPreviousTrack                  = 4,
    RemoteCommandIDVolumeUp                       = 5,
    RemoteCommandIDVolumeDown                     = 6,
    RemoteCommandIDAdvanceRepeatMode              = 7,
    RemoteCommandIDAdvanceShuffleMode             = 8,
    RemoteCommandIDSkipForward                    = 9,
    RemoteCommandIDSkipBackward                   = 10,
    RemoteCommandIDLikeTrack                      = 11,
    RemoteCommandIDDislikeTrack                   = 12,
    RemoteCommandIDBookmarkTrack                  = 13,
    RemoteCommandIDReserved                       = 14,
};

enum amsc_entity_id
{
    EntityIDPlayer        = 0,
    EntityIDQueue         = 1,
    EntityIDTrack         = 2,
    EntityIDReserved      = 3,
};

enum amsc_entity_update_flags
{
    EntityUpdateFlagTruncated     = 1 << 0,
    EntityFlagsReserved           = 1 << 1,
};

enum amsc_play_attribute_id
{
    PlayerAttributeIDName            = 0,
    PlayerAttributeIDPlaybackInfo    = 1,
    PlayerAttributeIDVolume          = 2,
    PlayerAttributeReserved          = 3,
};

enum amsc_queue_attribute_id
{
    QueueAttributeIDIndex            = 0,
    QueueAttributeIDCount            = 1,
    QueueAttributeIDShuffleMode      = 2,
    QueueAttributeIDRepeatMode       = 3,
    QueueAttributeReserved           = 4,
};

enum amsc_track_attribute_id
{
    TrackAttributeIDArtist       = 0,
    TrackAttributeIDAlbum        = 1,
    TrackAttributeIDTitle        = 2,
    TrackAttributeIDDuration     = 3,
    TrackReserved                = 4,
};

enum amsc_shuffle_mode
{
    ShuffleModeOff         = 0,
    ShuffleModeOne         = 1,
    ShuffleModeAll         = 2,
    ShuffleReserved        = 3,
};

enum amsc_repeat_mode
{
    RepeateModeOff        = 0,
    RepeatModeOne         = 1,
    RepeatModeAll         = 2,
    RepeatReserved        = 3,
};

typedef struct {
    uint8_t entityid;
    uint8_t attributeid;
    uint8_t entityupdateflags;
    uint8_t value[0];
}amsc_entity_info;

typedef struct {
    uint8_t entityid;
    uint8_t attributeid[3];
}amsc_entity_player_cmd;

typedef struct {
    uint8_t entityid;
    uint8_t attributeid[4];
}amsc_entity_queue_cmd;

typedef struct {
    uint8_t entityid;
    uint8_t attributeid[4];
}amsc_entity_track_cmd;
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
void amsc_enable_rsp_send(PRF_ENV_T(amsc) *amsc_env, uint8_t conidx, uint8_t status);
void amsc_task_init(struct ke_task_desc *task_desc, void* p_env);

#endif //(BLE_AMS_CLIENT)

/// @} AMSC

#endif //(_AMSC_H_)

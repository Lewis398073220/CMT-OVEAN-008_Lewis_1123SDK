/**
 ****************************************************************************************
 * @file mhdt_le_int.h
 *
 * @brief  MHDT_LE Internal use api
 *
 * Copyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup mHDT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#ifndef _MHDT_LE_INT_
#define _MHDT_LE_INT_
#include "rwip_config.h"
#if (mHDT_LE_SUPPORT)

#if (HOST_MSG_API)
#include "ke_msg.h"                 // KE Message handler
#include "ke_task.h"                // KE Task handler
#endif // (HOST_MSG_API)

#include "mhdt_le_msg.h"
/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */

/*
 * ENUMERATION
 ****************************************************************************************
 */
enum mhdt_tci_read_proprietary_feat_sub_opcode
{
    TCI_LE_RD_LOCAL_PROPRIETARY_FEAT_SUBCODE   = 0x07,
    TCI_LE_RD_REMOTE_PROPRIETARY_FEAT_SUBCODE  = 0x08,
};


struct mhdt_le_con_t
{
    uint32_t mhdt_le_remote_feature;
};

typedef struct mhdt_le_con_t mhdt_le_con_t;

struct mhdt_le_env
{
    ke_task_id_t dest_task_id;

    mhdt_le_con_t *p_con[HOST_CONNECTION_MAX];

    uint32_t mhdt_le_local_feature;

    mhdt_mabr_dft_label_params_t dft_label_params;
};

typedef struct mhdt_le_env mhdt_le_env_t;
/*
 * DEFINES
 ****************************************************************************************
 */

#endif
#endif
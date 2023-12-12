/**
 ****************************************************************************************
 *
 * @file mhdt_le.h
 *
 * @brief mhdt_le - LE mhdt data
 *
 * Copyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

#ifndef _MHDT_LE_H_
#define _MHDT_LE_H_
/**
 ****************************************************************************************
 * @addtogroup mHDT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************/
#include "rwip_config.h"

#if (mHDT_LE_SUPPORT)

#if (HOST_MSG_API)
#include "ke_msg.h"                 // KE Message handler
#include "ke_task.h"                // KE Task handler
#include "mhdt_le_msg.h"
#endif // (HOST_MSG_API)

#include "mhdt_le_int.h"
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

void mhdt_init(uint8_t init_type);
void mhdt_con_create(uint8_t conidx);
void mhdt_con_cleanup(uint8_t conidx);
mhdt_le_con_t *mhdt_le_con_get(uint8_t conidx);
uint16_t mhdt_le_set_mabr_dft_label_params_cfg(void *params);
bool mhdt_le_is_feat_support(uint8_t conidx, uint8_t feature);
void mhdt_tci_le_rd_local_proprietary_feat_cmd();
void mhdt_tci_le_rd_remote_proprietary_feat_cmd(uint8_t conidx);
void mhdt_tci_set_default_label_params_cmd_send(uint8_t group_lid);

#endif

/// @} mHDT
#endif
/**
 ****************************************************************************************
 *
 * @file iap_proc.h
 *
 * @brief Isochronous Access Profile - Header file for Procedure Manager
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef IAP_PROC_H_
#define IAP_PROC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "iap_int.h"        // IAP Internal Definitions

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Start a new procedure for a group
 *
 * @param[in] p_group_info      Pointer to group information structure
 * @param[in] cmd_code          Command code for which procedure is started
 * @param[in] stream_lid        Stream local index if procedure is started for a specific
 * stream
 ****************************************************************************************
 */
void iap_proc_start(iap_group_info_t* p_group_info, uint16_t cmd_code, uint8_t stream_lid);

/**
 ****************************************************************************************
 * @brief Handler of procedure execution deferred in background
 *
 * @param[in] p_djob  Pointer to Delayed job structure.
 ****************************************************************************************
 */
void iap_proc_cb_djob(co_djob_t* p_djob);

/**
 ****************************************************************************************
 * @brief End a group procedure
 *
 * @param[in] p_group_info      Pointer to group information structure
 * @param[in] status            Procedure status
 ****************************************************************************************
 */
void iap_proc_end(iap_group_info_t* p_group_info, uint16_t status);

/**
 ****************************************************************************************
 * @brief Return if a procedure is currently pending for a group
 *
 * @param[in] p_group_info      Pointer to group information structure
 ****************************************************************************************
 */
bool iap_proc_is_busy(iap_group_info_t* p_group_info);

/**
 ****************************************************************************************
 * @brief End a group procedure with a delay
 *
 * @param[in] p_group_info      Pointer to group information structure
 ****************************************************************************************
 */
void iap_proc_end_delay(iap_group_info_t* p_group_info, iap_proc_cb cb);

#endif // IAP_PROC_H_

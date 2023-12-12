/**
 ****************************************************************************************
 *
 * @file atc_csism_int.h
 *
 * @brief Audio Topology Control - Coordinated Set Identification Set Member - Internal Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ATC_CSISM_INT_H_
#define ATC_CSISM_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "atc_csism.h"              // Coordinated Set Identification Set Member Definitions

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function called when a new RSI has been generated for the first Coordinated Set
 *
 * @param[in] p_rsi             Pointer to generated RSI value
 ****************************************************************************************
 */
typedef void (*atc_csism_cb_int_rsi)(const csis_rsi_t* p_rsi);

/**
 ****************************************************************************************
 * @brief Callback function called when Update RSI procedure has been completed for the first Coordinated Set
 *
 * @param[in] status        Status
 ****************************************************************************************
 */
typedef void (*atc_csism_cb_rsi_cmp_evt)(uint16_t status);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of internal callback functions for Coordinated Set Identification Service Set Member
typedef struct atc_csism_cb_int
{
    /// Callback function called when a new RSI has been generated for the first Coordinated Set
    atc_csism_cb_int_rsi cb_rsi;
    /// Callback function called when Update RSI procedure has been completed for the first Coordinated Set
    atc_csism_cb_rsi_cmp_evt cb_rsi_cmp_evt;
} atc_csism_cb_int_t;

#if (GAF_ATC_CSISM)

/*
 * INTERNAL FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_ADV)
/**
 ****************************************************************************************
 * @brief Bind with ATC CSISM module for update of RSI
 *
 * @param[in] p_cb_int      Pointer to set of callback functions for internal communication
 ****************************************************************************************
 */
void atc_csism_register(const atc_csism_cb_int_t* p_cb_int);

/**
 ****************************************************************************************
 * @brief Block use of Set SIRK procedure by an Upper Layer so that RSI value cannot be updated unexpectedly
 * during an internal procedure

 * @return An error value
 ****************************************************************************************
 */
uint16_t atc_csism_block_sirk();

/**
 ****************************************************************************************
 * @brief Unblock use of Set SIRK procedure by an Upper Layer
 ****************************************************************************************
 */
void atc_csism_unblock_sirk();

/**
 ****************************************************************************************
 * @brief Update RSI for the first Coordinated Set
 *
 * @return An error value
 ****************************************************************************************
 */
uint16_t atc_csism_update_rsi_local(void);
#endif //(GAF_ADV)

#endif //(GAF_ATC_CSISM)

#endif // ATC_CSISM_INT_H_

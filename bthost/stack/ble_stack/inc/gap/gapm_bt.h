/**
 ****************************************************************************************
 *
 * @file gapm_bt.h
 *
 * @brief Generic Access Profile Manager - BT-Classic Activities
 *
 * Copyright (C) RivieraWaves 2009-2021
 *
 ****************************************************************************************
 */


#ifndef GAPM_BT_H_
#define GAPM_BT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gapm.h"

/**
 ****************************************************************************************
 * @addtogroup GAPM_BT_API BT-Classic
 *
 * @ingroup GAPM_API
 *
 * @brief Set of functions and interfaces required to create and manage a BT-Classic activity.
 *
 * @{
 * @}
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GAPM_BT_KEY_MGR_API Link-Key Manager
 *
 * @ingroup GAPM_BT_API
 *
 * @brief Set of functions and interfaces required for Link Key Manager
 *
 * @{
 * @}
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GAPM_BT_SEC_API Security toolbox
 * @ingroup GAPM_BT_API
 * @brief OOB data
 *
 * @{
 * @}
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */
/// @addtogroup GAPM_ENUM_API
/// @{

/// Inquiry or Inquiry Scan type
enum gapm_inquiry_type
{
    /// General discoverable or discovery mode
    GAPM_INQUIRY_GENERAL,
    /// Limited discoverable or discovery mode
    GAPM_INQUIRY_LIMITED,
};

/// @} GAPM_ENUM_API

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * INTERFACES
 ****************************************************************************************
 */
/// @addtogroup GAPM_BT_KEY_MGR_API
/// @{

/// Callback structure that must be followed by a \glos{BT} link key manager
typedef struct gapm_link_key_manager_itf
{
    /**
     ****************************************************************************************
     * @brief Associate a link key with a BT classic address
     *
     * @param[in] p_addr         Pointer to the BD address
     * @param[in] p_link_key     Pointer to the BT Classic link key to store
     * @param[in] sec_lvl        security level associated with the key  (see enum #gap_sec_lvl).
     *                           At least GAP_SEC_UNAUTH
     *
     * @return Execution status (see enum #hl_err).
     ****************************************************************************************
     */
    uint16_t (*link_key_store)(const gap_bdaddr_t* p_addr, const gap_sec_key_t* p_link_key, uint8_t sec_lvl);

    /**
     ****************************************************************************************
     * @brief Remove a link key associated with a BT classic address
     *
     * @param[in] p_addr         Pointer to the BD address
     *
     * @return Execution status (see enum #hl_err).
     ****************************************************************************************
     */
    uint16_t (*link_key_remove)(const gap_bdaddr_t* p_addr);

    /**
     ****************************************************************************************
     * @brief Remove a all link key associated with any BT classic address
     *
     * @return Execution status (see enum #hl_err).
     ****************************************************************************************
     */
    uint16_t (*link_key_remove_all)(void);

    /**
     ****************************************************************************************
     * @brief Retrieve link key associated with a BT classic address
     *
     * @param[in]  p_addr        Pointer to the BD address
     * @param[out] p_link_key    Pointer to link key to fill
     * @param[out] p_sec_lvl     Pointer to link key security level (see enum #gap_sec_lvl).
     *
     * @return Execution status (see enum #hl_err).
     ****************************************************************************************
     */
    uint16_t (*link_key_get)(const gap_bdaddr_t* p_addr, gap_sec_key_t* p_link_key, uint8_t* p_sec_lvl);
} gapm_link_key_manager_itf_t;

/// @} GAPM_BT_KEY_MGR_API

/// @addtogroup GAPM_BT_SEC_API
/// @{

/**
 ***************************************************************************************
 * @brief Function executed when procedure execution is over.
 *
 * @param[in] dummy           Dummy parameter provided by upper layer application
 * @param[in] status          Procedure execution status  (see enum #hl_err)
 * @param[in] p_oob_192       Pointer to generated P-192 OOB data (NULL if status != GAP_ERR_NO_ERROR)
 * @param[in] p_oob_256       Pointer to generated P-256 OOB data (NULL if status != GAP_ERR_NO_ERROR)
 ***************************************************************************************
 */
typedef void (*gapm_bt_oob_cb)(uint32_t dummy, uint16_t status, const gap_oob_t* p_oob_192, const gap_oob_t* p_oob_256);
/// @} GAPM_BT_SEC_API

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/// @addtogroup GAPM_PROC_API
/// @{

/**
 ****************************************************************************************
 * @brief Get next available service record handle - shall be use only by BT Classic profiles
 *
 * @return Next available service record handle
 ****************************************************************************************
 */
uint32_t gapm_sdp_get_next_service_record_handle(void);

/// @} GAPM_PROC_API

/// @addtogroup GAPM_BT_SEC_API
/// @{

/**
 ***************************************************************************************
 * @brief Generate BT Classic OOB data using
 *
 *        OOB data must be conveyed to peer device through an out of band method.
 *
 * @param[in] dummy     Dummy parameter provided by upper layer application
 * @param[in] res_cb    Function called when procedure is over
 *
 * @return Execution status (see enum #hl_err).
 *         If returns GAP_ERR_NO_ERROR, upper layer SW shall wait for #gapm_bt_oob_cb callback execution
 ***************************************************************************************
 */
uint16_t gapm_generate_bt_oob_data(uint32_t dummy, gapm_bt_oob_cb res_cb);
/// @} GAPM_BT_SEC_API


/// @addtogroup GAPM_BT_KEY_MGR_API
/// @{
/*
 * Link Key Manager
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Associate a link key with a BT classic address
 *
 * @param[in] p_addr         Pointer to the BD address
 * @param[in] p_link_key     Pointer to the BT Classic link key to store
 * @param[in] sec_lvl        security level associated with the key  (see enum #gap_sec_lvl).
 *                           At least GAP_SEC_UNAUTH
 *
 * @return Execution status (see enum #hl_err).
 ****************************************************************************************
 */
uint16_t gapm_link_key_store(const gap_bdaddr_t* p_addr, const gap_sec_key_t* p_link_key, uint8_t sec_lvl);

/**
 ****************************************************************************************
 * @brief Retrieve link key associated with a BT classic address
 *
 * @param[in]  p_addr        Pointer to the BD address
 * @param[out] p_link_key    Pointer to link key to fill
 * @param[out] p_sec_lvl     Pointer to link key security level (see enum #gap_sec_lvl).
 *
 * @return Execution status (see enum #hl_err).
 ****************************************************************************************
 */
uint16_t gapm_link_key_get(const gap_bdaddr_t* p_addr, gap_sec_key_t* p_link_key, uint8_t* p_sec_lvl);

/**
 ****************************************************************************************
 * @brief Remove a link key associated with a BT classic address
 *
 * @param[in] p_addr         Pointer to the BD address
 *
 * @return Execution status (see enum #hl_err).
 ****************************************************************************************
 */
uint16_t gapm_link_key_remove(const gap_bdaddr_t* p_addr);

/**
 ****************************************************************************************
 * @brief Remove a all link key associated with any BT classic address
 *
 * @return Execution status (see enum #hl_err).
 ****************************************************************************************
 */
uint16_t gapm_link_key_remove_all(void);

/**
 ***************************************************************************************
 * @brief BT-Classic only, setup the link key manager. By default, if compiled, a simple key manager is configured.
 *        If no key manager present, it's not possible to perform a BT classic pairing.
 *
 * @param[in] p_itf     Pointer to the BT Classic link key manager
 *
 * @return Execution status (see enum #hl_err).
 ***************************************************************************************
 */
uint16_t gapm_set_link_key_manager(const gapm_link_key_manager_itf_t* p_itf);

/// @} GAPM_BT_KEY_MGR_API
#endif /* GAPM_BT_H_ */

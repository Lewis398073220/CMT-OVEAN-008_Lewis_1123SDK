/**
 ****************************************************************************************
 *
 * @file iap_sm.h
 *
 * @brief Isochronous Access Profile - Header file for Stream Manager
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef IAP_SM_H_
#define IAP_SM_H_

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
 * @brief Add a group
 *
 * @param[out] pp_group_info        Pointer used to return pointer to allocated structure for
 * the new group
 * @param[in] id                    Group idenfier (CIS ID for an unicast group, BIG Handle
 * for a Broadcast Group)
 * @param[in] type                  Group type (see #iap_group_types enumeration)
 * @param[in] role                  Role (see #iap_roles enumeration)
 * @param[in] test                  True if test parameters are used for group configuration,
 * else false
 * @param[in] intf_lid              Interface local index for communication with upper layer
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_sm_group_add(iap_group_info_t** pp_group_info, uint8_t id,
                          uint8_t type, uint8_t role, bool test, uint8_t intf_lid);

/**
 ****************************************************************************************
 * @brief Get group information structure
 *
 * @param[in] group_lid             Group local index
 * @param[out] pp_group_info        Pointer used to return pointer to group information
 * structure
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_sm_group_get(uint8_t group_lid, iap_group_info_t** pp_group_info);

/**
 ****************************************************************************************
 * @brief Remove a group
 *
 * @param[in] p_group_info          Pointer to group information structure
 ****************************************************************************************
 */
void iap_sm_group_remove(iap_group_info_t* p_group_info);

/**
 ****************************************************************************************
 * @brief Seek for a group using its identifier
 *
 * @param[in] type              Group type
 * @param[in] id                Group identifier
 *
 * @return Pointer to found group information structure, NULL if no group with provided identifier
 * is found
 ****************************************************************************************
 */
iap_group_info_t* iap_sm_group_seek_id(uint8_t type, uint8_t id);

/**
 ****************************************************************************************
 * @brief Get group role
 *
 * @param[in] p_group_info          Pointer to group information structure
 ****************************************************************************************
 */
uint8_t iap_sm_group_role_get(iap_group_info_t* p_group_info);

/**
 ****************************************************************************************
 * @brief Get group type
 *
 * @param[in] p_group_info          Pointer to group information structure
 ****************************************************************************************
 */
uint8_t iap_sm_group_type_get(iap_group_info_t* p_group_info);

/**
 ****************************************************************************************
 * @brief Get test status of a group
 *
 * @param[in] p_group_info          Pointer to group information structure
 ****************************************************************************************
 */
bool iap_sm_group_test_get(iap_group_info_t* p_group_info);

/**
 ****************************************************************************************
 * @brief Add a stream to a group
 *
 * @param[out] group_lid            Group local index
 * @param[out] pp_group_info        Pointer used to return pointer to allocated structure for
 * the new stream
 * @param[in] id_pos                CIS ID for an unicast stream, position in the group for a
 * broadcast stream
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_sm_stream_add(uint8_t group_lid, iap_stream_info_t** pp_stream_info, uint8_t id_pos,
                           iap_disable_cb cb_disable);

/**
 ****************************************************************************************
 * @brief Get stream information structure
 *
 * @param[in] stream_lid            Stream local index
 * @param[out] pp_stream_info       Pointer used to return pointer to stream information
 * structure
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_sm_stream_get(uint8_t stream_lid, iap_stream_info_t** pp_stream_info);

/**
 ****************************************************************************************
 * @brief Remove a stream
 *
 * @param[in] p_stream_info     Pointer to Stream information structure
 ****************************************************************************************
 */
void iap_sm_stream_remove(iap_stream_info_t* p_stream_info);

/**
 ****************************************************************************************
 * @brief Get both group and stream information structure
 *
 * @param[in] stream_lid            Stream local index
 * @param[out] pp_stream_info       Pointer used to return pointer to stream information
 * structure
 * @param[out] pp_group_info        Pointer used to return pointer to group information
 * structure
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_sm_stream_group_get(uint8_t stream_lid,
                                 iap_stream_info_t** pp_stream_info, iap_group_info_t** pp_group_info);

/**
 ****************************************************************************************
 * @brief Seek for an unicast stream based on its connection handle
 *
 * @param[in] con_hdl           Connection handle
 * @param[out] pp_group_info    Pointer at which pointer to group information structure will be
 * returned
 *
 * @return Stream information structure
 ****************************************************************************************
 */
iap_stream_info_t* iap_sm_stream_seek_conhdl(uint16_t con_hdl, iap_group_info_t** pp_group_info);

/**
 ****************************************************************************************
 * @brief Inform that stream has been enabled
 *
 * @param[in] p_stream_info         Pointer to Stream information structure
 * @param[in] enable                True if stream has been enabled, else false
 ****************************************************************************************
 */
void iap_sm_stream_enable(iap_stream_info_t* p_stream_info, uint8_t enable);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
void iap_sm_stream_tm_used(iap_stream_info_t* p_stream_info, uint8_t used);

/**
 ****************************************************************************************
 * @brief Return if a stream is enabled
 *
 * @param[in] stream_lid            Stream local index
 ****************************************************************************************
 */
bool iap_sm_stream_is_enabled(iap_stream_info_t* p_stream_info);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
bool iap_sm_stream_is_tm_used(iap_stream_info_t* p_stream_info);

/**
 ****************************************************************************************
 * @brief Set connection handle allocated by the controller for a stream
 *
 * @param[in] p_stream_info         Pointer to Stream information structure
 * @param[in] con_hdl               Allocated connection handle
 ****************************************************************************************
 */
void iap_sm_stream_conhdl_set(iap_stream_info_t* p_stream_info, uint16_t con_hdl);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
void iap_sm_stream_tm_disable(iap_stream_info_t* p_stream_info, bool local);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
void iap_sm_stream_tm_disabled(iap_stream_info_t* p_stream_info);

#endif // IAP_SM_H_

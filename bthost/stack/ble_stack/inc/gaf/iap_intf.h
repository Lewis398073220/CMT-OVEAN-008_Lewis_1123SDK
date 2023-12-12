/**
 ****************************************************************************************
 *
 * @file iap_intf.h
 *
 * @brief Isochronous Access Profile - Header file for Interface Manager
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef IAP_INTF_H_
#define IAP_INTF_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ke_task.h"            // Kernel Task Definitions
#include "iap_int.h"            // IAP Internal Definitions

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Get set of callback functions for an interface
 *
 * @param[in] intf_lid          Interface local index
 * @param[out] pp_cb            Pointer at which pointer to set of callback functions will
 * be returned
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_intf_cb_get(uint8_t intf_lid, const iap_cb_t** pp_cb);

#if (GAF_API_MSG)
/**
 ****************************************************************************************
 * @brief Get task ID of task that has registered an interface using kernel messages
 *
 * @param[in] intf_lid      Interface local index
 *
 * @return Task ID
 ****************************************************************************************
 */
ke_task_id_t iap_intf_task_id_get(uint8_t intf_lid);

/**
 ****************************************************************************************
 * @brief Seek for an interface using a provided task identifier and return it local index
 *
 * @param[in] task_id           Task identifier
 * @param[out] p_intf_lid       Pointer at which found local index is returned
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t iap_intf_lid_get(ke_task_id_t task_id, uint8_t* p_intf_lid);

/**
 ****************************************************************************************
 * @brief Set task ID of task that on an interface which use kernel messages
 *
 * @param[in] intf_lid      Interface local index
 * @param[in] task_id       Task identifier
 *
 * @return Task ID
 ****************************************************************************************
 */
void iap_intf_task_id_set(uint8_t intf_lid, ke_task_id_t task_id);
#endif //(GAF_API_MSG)

/**
 ****************************************************************************************
 * @brief Call complete event callback function for an interface
 *
 * @param[in] intf_lid      Interface local index
 * @param[in] cmd_code      Command code
 * @param[in] status        Command status
 * @param[in] group_lid     Group local index
 * @param[in] stream_lid    Stream local index
 ****************************************************************************************
 */
void iap_intf_cb_cmp_evt(uint8_t intf_lid, uint16_t cmd_code, uint16_t status,
                        uint8_t group_lid, uint8_t stream_lid);

#if (GAF_UNICAST_SUPP)
/**
 ****************************************************************************************
 * @brief Call unicast stream enabled callback function for an interface
 *
 * @param[in] intf_lid      Interface local index
 * @param[in] stream_lid    Stream local index
 ****************************************************************************************
 */
void iap_intf_cb_us_enabled(uint8_t intf_lid, uint8_t group_lid, uint8_t stream_lid,
                            iap_ug_config_t* p_ug_cfg, iap_us_config_t* p_us_cfg);

/**
 ****************************************************************************************
 * @brief Call unicast stream disabled callback function for an interface
 *
 * @param[in] intf_lid      Interface local index
 * @param[in] stream_lid    Stream local index
 * @param[in] reason        Reason why stream has been disabled (see #iap_reason enumeration)
 ****************************************************************************************
 */
void iap_intf_cb_us_disabled(uint8_t intf_lid, uint8_t stream_lid,uint8_t status, uint8_t reason);
#endif //(GAF_UNICAST_SUPP)

#if (GAF_BROADCAST_SLV_SUPP)
/**
 ****************************************************************************************
 * @brief Call Broadcast Group synchronization status callback function for an interface
 *
 * @param[in] intf_lid          Interface local index
 * @param[in] group_lid         Group local index
 * @param[in] status            Status
 * @param[in] p_cfg             Pointer to Broadcast Group Configuration structure
 * @param[in] nb_bis            Number of BISes synchronization has been established with\n
 *                              Meaningful only if synchronization has been established
 * @param[in] p_conhdl          Pointer to list of Connection Handle values provided by the Controller\n
 *                              List of nb_bis values
 ****************************************************************************************
 */
void iap_intf_cb_bg_sync_status(uint8_t intf_lid, uint8_t group_lid, uint8_t state, iap_bg_sync_config_t* p_cfg,
                                uint8_t nb_bis, const uint16_t* p_conhdl);
#endif //(GAF_BROADCAST_SLV_SUPP)

#if (GAF_BROADCAST_MST_SUPP)
/**
 ****************************************************************************************
 * @brief Call Broadcast Group created callback function for an interface
 *
 * @param[in] intf_lid          Interface local index
 * @param[in] group_lid         Group local index
 * @param[in] p_cfg             Pointer to Broadcast Group Configuration structure
 * @param[in] nb_bis            Number of BISes synchronization has been established with\n
 *                              Meaningful only if synchronization has been established
 * @param[in] p_conhdl          Pointer to list of Connection Handle values provided by the Controller\n
 *                              List of nb_bis values
 ****************************************************************************************
 */
void iap_intf_cb_bg_created(uint8_t intf_lid, uint8_t group_lid, iap_bg_config_t* p_cfg,
                            uint8_t nb_bis, const uint16_t* p_conhdl);
#endif //(GAF_BROADCAST_MST_SUPP)

/**
 ****************************************************************************************
 * @brief Call data path update callback function for an interface
 *
 * @param[in] intf_lid      Interface local index
 * @param[in] group_lid     Group local index
 * @param[in] stream_lid    Stream local index
 * @param[in] dp_update     Data path update type
 * @param[in] direction     Direction for setup update, direction bit field for remove update
 * @param[in] status        Update status
 ****************************************************************************************
 */
void iap_intf_cb_dp_update(uint8_t intf_lid, uint8_t group_lid, uint8_t stream_lid, uint8_t dp_update,
                           uint8_t direction, uint16_t status);

/**
 ****************************************************************************************
 * @brief Call test mode counter callback function for an interface
 *
 * @param[in] intf_lid          Interface local index
 * @param[in] stream_lid        Stream local index
 * @param[in] nb_rx             Number of received packets
 * @param[in] nb_missed         Number of missed packets
 * @param[in] nb_failed         Number of failed packets
 ****************************************************************************************
 */
void iap_intf_cb_tm_cnt(uint8_t intf_lid, uint8_t stream_lid, uint32_t nb_rx, uint32_t nb_missed,
                               uint32_t nb_failed);

/**
 ****************************************************************************************
 * @brief Call get link quality command complete event callback function for an interface
 *
 * @param[in] intf_lid          Interface local index
 * @param[in] stream_lid        Stream local index
 ****************************************************************************************
 */
void iap_intf_cb_quality_cmp_evt(uint8_t intf_lid, uint16_t status, uint8_t stream_lid,
                                 uint32_t tx_unacked_packets, uint32_t tx_flushed_packets,
                                 uint32_t tx_last_subevent_packets, uint32_t retransmitted_packets,
                                 uint32_t crc_error_packets, uint32_t rx_unreceived_packets,
                                 uint32_t duplicate_packets);

void iap_intf_cb_iso_tx_sync_cmp_evt(uint8_t intf_lid, uint16_t status, uint8_t stream_lid,
                                 uint16_t con_hdl, uint16_t packet_sequence_number,
                                 uint32_t tx_time_stamp, uint32_t time_offset);
/**
 ****************************************************************************************
 * @brief CIS rejected callback function for an interface
 *
 * @param[in] intf_lid          Interface local index
 * @param[in] con_hdl           Connection handle of Connected Isochronous Stream
 * @param[in] error             Reject reason
 ****************************************************************************************
 */
void iap_intf_cb_cis_rejected_evt(uint8_t intf_lid, uint16_t con_hdl, uint8_t error);

/**
 ****************************************************************************************
 * @brief CIS terminated callback function for an interface
 *
 * @param[in] intf_lid      Interface local index
 * @param[in] cig_id        CIG index
 * @param[in] group_lid     Group local index
 * @param[in] stream_lid    Stream local index
 ****************************************************************************************
 */
void iap_intf_cb_cig_terminated_evt(uint8_t intf_lid, uint8_t cig_id, uint8_t group_lid,
                                    uint8_t stream_lid, uint8_t reason);

#endif // IAP_INTF_H_

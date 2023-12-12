/**
 ****************************************************************************************
 *
 * @file gaf_msg_int.h
 *
 * @brief Generic Audio Framework - Definition of Kernel Messages (Internal)
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef GAF_MSG_INT_H_
#define GAF_MSG_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf_msg.h"        // GAF Definitions

#if (GAF_API_MSG)

/*
 * DEFINES
 ****************************************************************************************
 */

/// Default module type
#define GAF_MSG_MODULE_DFLT        (0x0F)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure of callback for GAF module message handlers.
typedef struct gaf_msg_cbs
{
    /**
     * Message handler module initialization
     *
     * @param[in] init_type initialization type (see #rwip_init_type enumeration)
     * @param[in] src_id Source task identifier
     */
    void (*cb_init)(uint8_t init_type);
    /**
     * Handler of request message received from upper layer application
     *
     * @param[in] p_req Pointer to request parameters
     * @param[in] src_id Source task identifier
     */
    void (*cb_req_msg_handler)(gaf_req_t const* p_req, ke_task_id_t const src_id);
    /**
     * Handler of command message received from upper layer application
     *
     * @param[in] p_cmd Pointer to command parameters
     * @param[in] src_id Source task identifier
     */
    void (*cb_cmd_msg_handler)(gaf_cmd_t const* p_cmd, ke_task_id_t const src_id);
    /**
     * Handler of request message received from upper layer application
     *
     * @param[in] p_cfm Pointer to confirmation parameters
     * @param[in] src_id Source task identifier
     */
    void (*cb_cfm_msg_handler)(gaf_cfm_t const* p_cfm, ke_task_id_t const src_id);
} gaf_msg_cbs_t;


/**
 ****************************************************************************************
 * Get message interface for module callback
 *
 * @return message callback structure for a given module
 ****************************************************************************************
 */
typedef const gaf_msg_cbs_t* (*gaf_msg_itf_t) (void);


/**
 ****************************************************************************************
 * Get message interface for specific module in a layer
 *
 * @param[in] module Module identifier
 *
 * @return message callback structure for a given module
 ****************************************************************************************
 */
typedef const gaf_msg_cbs_t* (*gaf_msg_module_itf_t) (uint8_t module);

/*
 * INTERNAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Send a basic response message
 *
 * @param[in] req_code          Request code
 * @param[in] status            Status
 * @param[in] msg_id            Message ID
 * @param[in] dest_id           Message destination ID
 ****************************************************************************************
 */
void gaf_msg_rsp_send(uint16_t req_code, uint16_t status, ke_msg_id_t msg_id, ke_task_id_t dest_id);

/**
 ****************************************************************************************
 * @brief Send a Command Complete Event message containing only the command code and an error status
 *
 * @param[in] cmd_code          Command code
 * @param[in] status            Status
 * @param[in] dest_id           Message Destination ID
 * @param[in] size              Size of Command Complete Event message
 ****************************************************************************************
 */
void gaf_msg_cmp_evt_send(uint16_t cmd_code, uint16_t status, ke_task_id_t dest_id, uint16_t size);

#endif //(GAF_API_MSG)

#endif // GAF_MSG_H_

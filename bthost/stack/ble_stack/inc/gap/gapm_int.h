/**
 ****************************************************************************************
 *
 * @file gapm_int.h
 *
 * @brief Generic Access Profile Manager Internal Header.
 *
 * Copyright (C) RivieraWaves 2009-2016
 *
 ****************************************************************************************
 */


#ifndef _GAPM_INT_H_
#define _GAPM_INT_H_

/**
 ****************************************************************************************
 * @addtogroup GAPM_INT Generic Access Profile Manager Internal
 * @ingroup GAPM
 * @brief defines for internal GAPM usage
 *
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gapm.h"
#include "co_bt.h"
#include "co_list.h"
#include "co_djob.h"
#include "co_time.h"
#include "gap_int.h"
#include "gapm_proc.h" // GAP Procedures
#include "gapm_actv.h" // GAP Activities
#if(BT_HOST_PRESENT)
#include "gapm_bt.h"
#endif // (BT_HOST_PRESENT)

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of GAP Manager process
#define GAPM_IDX_MAX                                 BLE_CONNECTION_MAX

/// Scan filter size
#define GAPM_SCAN_FILTER_SIZE   10

/// check if current role is supported by configuration
#define GAPM_IS_ROLE_SUPPORTED(role_type)\
    ((gapm_env.role & (role_type)) == (role_type))

/// Maximum number of advertising reports from different advertisers that can be reassembled in parallel
#define GAPM_REPORT_NB_MAX            (5)


/*
 * INTERNAL API TYPES
 ****************************************************************************************
 */


/// Retrieve information about memory usage
struct gapm_dbg_get_mem_info_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_DBG_GET_MEM_INFO: Get memory usage
    uint8_t operation;
};

/// Indication containing information about memory usage.
/*@TRACE*/
struct gapm_dbg_mem_info_ind
{
    /// peak of memory usage measured
    uint32_t max_mem_used;
    /// memory size currently used into each heaps.
    uint16_t mem_used[KE_MEM_BLOCK_MAX];
};

/// Parameters of #GAPM_DBG_STATS_IND message structure
/*@TRACE*/
struct gapm_dbg_stats_ind
{
    /// Max message sent
    uint32_t max_msg_sent;
    /// Max message saved
    uint32_t max_msg_saved;
    /// Max timer used
    uint32_t max_timer_used;
    /// Max heap used
    uint32_t max_heap_used;
    ///Max stack used
    uint32_t max_stack_used;
};

/// Procedure type
enum gapm_proc_type
{
    /// Configuration Procedure
    GAPM_PROC_CFG         = 0x00,
    /// Air mode procedure (scanning, advertising, connection establishment)
    /// Note: Restriction, only one air procedure supported.
    GAPM_PROC_AIR         = 0x01,
    /// Procedure used for ECDH computation
    /// TODO [NATIVE API] could be decide to support only one ECDH computation other rejected until first one finished...
    GAPM_PROC_ECDH        = 0x02,
    /// Max number of procedures type
    GAPM_PROC_MAX,
};

/*
 * MACROS
 ****************************************************************************************
 */



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// GAP Manager environment structure
typedef struct gapm_env_
{
    #if (BT_HOST_PRESENT)
    const gapm_link_key_manager_itf_t* p_link_key_mgr;
    #endif // (BT_HOST_PRESENT)

    /// Pointer to procedure under execution
    hl_proc_queue_t proc_queue[GAPM_PROC_MAX];
    /// Pointer to the device name - if NULL, in LE directly ask application
    const uint8_t*   p_name;

    #if (HL_LE_CENTRAL || HL_LE_PERIPHERAL)
    /// Generated OOB Data
    gapm_oob_data_t* p_oob;
    /// Bit field use to know if periodic ADV sync transfer establishment on-going onto a specific connection index
    uint32_t         past_estab_bf;
    #endif // (HL_LE_CENTRAL || HL_LE_PERIPHERAL)

    /// token number counter to ensure a request and confirmation have an unique identifier.
    uint16_t        token_id_cnt;

    #if (HL_LE_BROADCASTER)
    /// Maximum length of advertising data that can be pushed to the controller
    uint16_t        max_adv_data_len;
    #endif //(HL_LE_BROADCASTER)

    /// Duration before regenerate device address when privacy is enabled. - in seconds
    uint16_t        renew_dur;
    /// Device IRK used for resolvable random BD address generation (MSB -> LSB)
    gap_sec_key_t   irk;

    /// Current device Address
    gap_addr_t      addr;
    gap_addr_t      addr_rpa;
    gap_addr_t      connectedAddr[BLE_CONNECTION_MAX];
    /// Privacy Configuration (@see gapm_priv_cfg)
    uint8_t         priv_cfg;
    /// Device Role
    uint8_t         role;
    /// Number of BLE connection
    uint8_t         connections;
    /// Pairing mode authorized (see enum #gapm_pairing_mode)
    uint8_t         pairing_mode;


    /// Number of devices in the white list
    uint8_t         nb_dev_wl;
    /// Used to know if Host is configured // TODO [NATIVE API] create a bit field + for procedure creation, do an automatic reject
    bool            configured;
    /// Used to know if Host has been reset
    bool            reset;
    /// Length of device name (@see gapm_env_t.p_name)
    uint8_t         name_len;


    /// Array of pointers to the allocated activities
    gapm_actv_t*    p_actvs[GAPM_ACTV_NB];
    #if(HL_LE_PERIPHERAL)
    /// temporary buffer to keep connection information
    co_buf_t*       p_adv_connection_info;
    #endif // (HL_LE_PERIPHERAL)

    /// Timer used for address renewal
    #ifdef BLE_STACK_PORTING_CHANGES
    osTimerId       timerId;
    #else
    co_time_timer_t addr_renew_timer;
    #endif
    /// Mark if activity is busy or not
    uint32_t        actv_busy_bf;
    /// Bit field that contains list of created activities
    uint32_t        actv_bf;
    #if (BLE_HOST_PRESENT)
    #if (HL_LE_BROADCASTER)
    /// Number of advertising sets supported by controller
    uint8_t         max_adv_set;
    /// Number of created advertising activities
    uint8_t         nb_adv_actv;
    #endif //(HL_LE_BROADCASTER)
    #if (HL_LE_OBSERVER)
    /// Last generated Random Address
    gap_addr_t      scan_init_rand_addr;
    /// Address type used by scan or init activity (see enum #gapm_own_addr)
    uint8_t         scan_init_own_addr_type;
    /// Activity identifier for currently started scanning activity
    uint8_t         scan_actv_idx;
    #endif //(HL_LE_OBSERVER)
    #if (HL_LE_CENTRAL)
    /// Activity identifier for currently started initiating activity
    uint8_t         init_actv_idx;
    #endif //(HL_LE_CENTRAL)

    /// Activity index of LE test mode
    uint8_t         test_actv_idx;
    #endif // (BLE_HOST_PRESENT)

    #if (HL_LE_OBSERVER)
    /// Activity in Periodic sync establishment state
    uint8_t         per_sync_estab_actv_idx;
    /// Used to know if transmission of advertising reports to application must be stopped
    /// When enabled, advertising reports are dropped without informing host.
    bool            adv_report_flow_off;
    #endif //(HL_LE_OBSERVER)

    /// Number of created activities
    uint8_t         nb_created_actvs;
    /// Number of started activities
    uint8_t         nb_started_actvs;
    /// Number of started activities that can lead to connection establishment (connectable
    /// advertising or initiating)
    uint8_t         nb_connect_actvs;

    /// Address renew information bit field (enum gapm_addr_info_bf)
    uint8_t         addr_info_bf;

    #if(BT_HOST_PRESENT)
    /// Activity index of started inquiry structure
    uint8_t         inquiry_actv_idx;
    /// Activity index of started inquiry scan structure
    uint8_t         inquiry_scan_actv_idx;
    /// Activity index of started page scan structure
    uint8_t         page_scan_actv_idx;
    /// Activity index of started page structure
    uint8_t         page_actv_idx;
    /// Counter of SDP Record handle.
    uint8_t         sdp_rec_hdl_cnt;
    #endif // (BT_HOST_PRESENT)
} gapm_env_t;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
/// GAP Manager environment variable.
extern gapm_env_t gapm_env;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Checks validity of the Data Length Suggested values
 *
 * @param[in] sugg_oct   Suggested octets
 * @param[in] sugg_time  Suggested time
 ****************************************************************************************
 */
uint8_t gapm_dle_val_check(uint16_t sugg_oct, uint16_t sugg_time);

#if (BT_HOST_PRESENT)
#if (HOST_KEY_MANAGER_PRESENT)
/**
 ****************************************************************************************
 * @brief Initialize default key manager
 *
 * @param[in] init_type  Type of initialization (see enum #rwip_init_type)
 ****************************************************************************************
 */
void gapm_link_key_mgr_initialize(uint8_t init_type);
#endif // (HOST_KEY_MANAGER_PRESENT)
#endif // (BT_HOST_PRESENT)

#if (HOST_MSG_API)
/**
 ****************************************************************************************
 * @brief Initialize Message API module
 *
 * @param[in] init_type  Type of initialization (see enum #rwip_init_type)
 ****************************************************************************************
 */
void gapm_msg_initialize(uint8_t init_type);
#endif // (HOST_MSG_API)

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

/// @} GAPM_INT

#endif /* _GAPM_INT_H_ */

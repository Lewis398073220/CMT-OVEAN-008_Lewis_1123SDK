/**
 ****************************************************************************************
 *
 * @file gapc_le_con.h
 *
 * @brief Generic Access Profile Controller Internal Header - BT Classic Connection
 *
 * Copyright (C) RivieraWaves 2009-2021
 *
 ****************************************************************************************
 */
#ifndef _GAPC_BT_CON_H_
#define _GAPC_BT_CON_H_

/**
 ****************************************************************************************
 * @addtogroup GAPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BT_HOST_PRESENT)
#include "gapc_int.h"

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * TYPE DECLARATIONS
 ****************************************************************************************
 */


/// GAP controller connection variable structure.
typedef struct gapc_bt_con
{
    /// base connection information
    gapc_con_t          hdr;
    /// peer BD Address used for the link that should be kept
    gap_addr_t          peer_addr;
    /// pairing state
    uint8_t             sec_state;
    /// Store IOCAP to automatically accept numeric value if Display Yes/No not present
    uint8_t             iocap;
    /// True if a pairing is bonding devices, false otherwise -- TODO use information bit field
    bool                do_bonding;
    /// True if a pairing procedure is initiated -- TODO use information bit field
    bool                do_pairing;
    /// True if local device is pairing responder -- TODO use information bit field
    bool                is_pairing_responder;
    /// True to indicate that authentication information must be triggered to application
    bool                indicate_auth_info;

    #if (HOST_MSG_API)
    /// keep pin code for pairing
    uint8_t             pin_code[GAP_KEY_LEN];
    #endif // (HOST_MSG_API)
} gapc_bt_con_t;


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve BT Classic Connection environment
 *
 * @param[in] conidx    Connection index
 *
 * @return Pointer to connection structure if BT classic connection
 ****************************************************************************************
 */
__INLINE gapc_bt_con_t* gapc_bt_con_env_get(uint8_t conidx)
{
    gapc_con_t* p_con = gapc_con_env_get(conidx);
    // Only return BT classic environment
    return ((p_con != NULL) && !GETB(p_con->info_bf, GAPC_LE_CON_TYPE) ? (gapc_bt_con_t*) p_con : NULL);
}


/// Provide passkey for a BT Classic pairing
uint16_t gapc_bt_pairing_provide_passkey(uint8_t conidx, bool accept, uint32_t passkey);
/// Provide number comparison for a BT Classic pairing
uint16_t gapc_bt_pairing_numeric_compare_rsp(uint8_t conidx, bool accept);


#endif // (BT_HOST_PRESENT)
/// @} GAPC

#endif /* _GAPC_BT_CON_H_ */

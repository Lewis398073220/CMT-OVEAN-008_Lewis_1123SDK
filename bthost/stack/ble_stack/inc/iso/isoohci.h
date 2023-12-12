/**
 ****************************************************************************************
 *
 * @file isoohci.h
 *
 * @brief Declaration of the functions used for Isochronous Payload over HCI Transport layer
 *
 * Copyright (C) RivieraWaves 2009-2017
 *
 *
 ****************************************************************************************
 */

#ifndef ISOOHCI_H_
#define ISOOHCI_H_

/**
 ****************************************************************************************
 * @addtogroup ISOOHCI
 * @ingroup ISO
 * @brief Isochronous Payload over HCI Transport Layer
 *
 * This module implements the primitives used for Isochronous Payload over HCI Transport layer
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_ISOOHCI)

#include <stdint.h>
#include <stdbool.h>
#include "co_hci.h"

#include "data_path.h"

/*
 * MACROS
 ****************************************************************************************
 */
/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


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
 * @brief Initialize Isochronous over HCI TL module
 *
 * @param[in] init_type  Type of initialization (@see enum rwip_init_type)
 ****************************************************************************************
 */
void isoohci_init(uint8_t init_type);

/**
 ****************************************************************************************
 * @brief Retrieve the isochronous data path interface
 *
 * @param[in] Data path direction (@see enum iso_rx_tx_select)
 *
 * @return isochronous data path interface
 ****************************************************************************************
 */
const struct data_path_itf* isoohci_itf_get(uint8_t  direction);

/**
 ****************************************************************************************
 * @brief Release the isochronous out buffer
 *
 * @param[in] iso_sdu_ptr   Isochronous SDU pointer
 ****************************************************************************************
 */
void isoohci_out_buf_release(uint8_t* iso_sdu_ptr);


#endif //(BLE_ISOOHCI)

/// @} ISOOHCI

#endif // ISOOHCI_H_

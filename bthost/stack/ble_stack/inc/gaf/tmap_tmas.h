/**
 ****************************************************************************************
 *
 * @file tmap_tmas.h
 *
 * @brief Telephony and Media Audio Profile - Telephony and Media Audio Service Server - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef TMAP_TMAS_H_
#define TMAP_TMAS_H_

/**
 ****************************************************************************************
 * @defgroup TMAP_TMAS Telephony and Media Audio Service Server
 * @ingroup TMAP
 * @brief Description of Telephony and Media Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup TMAP_TMAS_NATIVE_API Native API
 * @ingroup TMAP_TMAS
 * @brief Description of Native API for Telephony and Media Service Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "tmap.h"               // Telephony and Media Audio Profile Definitions

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_TMAP_TMAS)

/// @addtogroup TMAP_TMAS_NATIVE_API
/// @{

/**
 ****************************************************************************************
 * @return If use of Server Role for Telephony and Media Audio Service has been configured
 ****************************************************************************************
 */
bool tmap_tmas_is_configured();

/// @} TMAP_TMAS_NATIVE_API
#endif //(GAF_TMAP_TMAS)

#endif // TMAP_TMAS_H_

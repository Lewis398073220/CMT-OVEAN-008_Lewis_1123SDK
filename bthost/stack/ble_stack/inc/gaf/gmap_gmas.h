/**
 ****************************************************************************************
 *
 * @file gmap_gmas.h
 *
 * @brief Gaming Audio Profile - Gaming Audio Service Server - Definitions
 *
 * GmapCopyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

#ifndef GMAP_GMAS_H_
#define GMAP_GMAS_H_

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAS Gaming Audio Service Server
 * @ingroup GMAP
 * @brief Description of Telephony and Media Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAS_NATIVE_API Native API
 * @ingroup GMAP_GMAS
 * @brief Description of Native API for Telephony and Media Service Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gmap.h"               // Gaming Audio Profile Definitions

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_GMAP_GMAS)

/// @addtogroup GMAP_GMAS_NATIVE_API
/// @{

/**
 ****************************************************************************************
 * @return If use of Server Role for Gaming Audio Service has been configured
 ****************************************************************************************
 */
bool gmap_gmas_is_configured();

/// @} GMAP_GMAS_NATIVE_API
#endif //(GAF_GMAP_GMAS)

#endif // GMAP_GMAS_H_

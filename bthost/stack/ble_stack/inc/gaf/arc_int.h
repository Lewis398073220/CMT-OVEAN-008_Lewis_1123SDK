/**
 ****************************************************************************************
 *
 * @file arc_int.h
 *
 * @brief Audio Rendering Control - Internal Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ARC_INT_H_
#define ARC_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf_int.h"            // Generic Audio Framework Internal Definitions

/*
 * INTERNAL FUNCTIONS
 ****************************************************************************************
 */

#if (GAF_ARC_VOCS)
void arc_srv_vocs_get(uint8_t* p_nb_vocs, uint16_t** pp_vocs_shdl);
#endif //(GAF_ARC_VOCS)

#if (GAF_ARC_AICS)
uint16_t arc_srv_aics_get(uint8_t input_lid, uint16_t* p_shdl);
#endif //(GAF_ARC_AICS)

#endif // ARC_INT_H_

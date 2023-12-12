#pragma once
//
// Copyright 2020 BES Technic
//
// Description: beco coprocessor instructions bindings
//

typedef enum
{
    BECO_OK             =  0,        /**< No error */
    BECO_ARGUMENT_ERROR = -1,        /**< One or more arguments are incorrect */
    BECO_LENGTH_ERROR   = -2,        /**< Length of data buffer is incorrect */
    BECO_SIZE_MISMATCH  = -3,        /**< Size of matrices is not compatible with the operation */
    BECO_NANINF         = -4,        /**< Not-a-number (NaN) or infinity is generated */
    BECO_SINGULAR       = -5,        /**< Input matrix is singular and cannot be inverted */
} beco_state;

#ifndef __arm__
#  define BECO_SIM
#endif

#ifdef BECO_SIM
#  include "beco-sim-if.h"
#else
#  include "beco-if.h"
#endif


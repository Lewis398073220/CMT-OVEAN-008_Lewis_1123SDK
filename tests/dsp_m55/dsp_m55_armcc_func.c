/***************************************************************************
 *
 * Copyright 2015-2022 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
/**
 * Customer use armcc compiler to generate their lib and integrate into sdk.
 * We use gcc to build sdk, perhaps you can find the build error as following: "undefined reference to `__hardfp_logf'"
 * The reason is gcc and armcc generate different names for standard math lib. So we need to do a wrap for armcc function
 */
#include "math.h"
float __hardfp_logf (float x) { return logf(x); }
float __hardfp_expf (float x) { return expf(x); }
float __hardfp_exp2 (float x) { return expf(x); }
float __hardfp_exp2f (float x) { return expf(x); }
float __hardfp_powf (float x, float y) { return powf(x, y); }
// Add more math function

// Add other module

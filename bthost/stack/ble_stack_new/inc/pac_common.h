/***************************************************************************
 *
 * Copyright (c) 2015-2023 BES Technic
 *
 * Authored by BES CD team (Blueelf Prj).
 *
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
#ifndef __BAP_PAC_COMMON__
#define __BAP_PAC_COMMON__

#include "bluetooth.h"

#include "gaf_cfg.h"
#include "generic_audio.h"

/// Characteristic type values for Published Audio Capabilities Service
enum pacs_char_type
{
    /// Available Audio Contexts characteristic
    PACS_CHAR_TYPE_CONTEXT_AVA = 0,
    /// Supported Audio Contexts characteristic
    PACS_CHAR_TYPE_CONTEXT_SUPP,
    /// Sink Audio Locations characteristic
    PACS_CHAR_TYPE_LOC_SINK,
    /// Source Audio Locations characteristic
    PACS_CHAR_TYPE_LOC_SRC,
    /// SINK PAC characteristic
    PACS_CHAR_TYPE_SINK_PAC,
    /// SRC PAC characteristic
    PACS_CHAR_TYPE_SRC_PAC,
    /// MAY BE MORE THAN ONE PAC
    PACS_CHAR_TYPE_MAX,
};

#endif /// __BAP_PAC_COMMON__

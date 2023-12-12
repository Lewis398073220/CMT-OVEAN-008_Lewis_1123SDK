/***************************************************************************
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
 ***************************************************************************/
#ifndef MTP_STORAGE_LOG_H
#define MTP_STORAGE_LOG_H

#include "hal_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MTP_TR_CRITICAL(attr, str, ...)         NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_CRITICAL), \
                                                        str, ##__VA_ARGS__)
#define MTP_TR_ERROR(attr, str, ...)            NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_ERROR), \
                                                        str, ##__VA_ARGS__)
#define MTP_TR_WARN(attr, str, ...)             NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_WARN), \
                                                        str, ##__VA_ARGS__)
#define MTP_TR_NOTIF(attr, str, ...)            NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_NOTIF), \
                                                        str, ##__VA_ARGS__)
#define MTP_TR_INFO(attr, str, ...)             NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_INFO), \
                                                        str, ##__VA_ARGS__)
#define MTP_TR_DEBUG(attr, str, ...)            NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_DEBUG), \
                                                        str, ##__VA_ARGS__)
#define MTP_TR_VERBOSE(attr, str, ...)          NORM_LOG(((attr) & ~TR_ATTR_LEVEL_MASK) | TR_ATTR_LEVEL(TR_LEVEL_VERBOSE), \
                                                        str, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif


/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#ifndef __BT_STACK_MEM_H__
#define __BT_STACK_MEM_H__

// only used to malloc global variables that exist all the time after reboot
void *bt_stack_malloc(uint32_t size); 



#endif /* __BT_STACK_MEM_H__ */

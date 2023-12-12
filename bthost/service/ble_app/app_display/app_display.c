/**
 ****************************************************************************************
 *
 * @file app_display.c
 *
 * @brief Application Display entry point
 *
 * Copyright (C) RivieraWaves 2019-2019
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */
#include "rwip_config.h"     // SW Configuration

#if (BLE_APP_PRESENT)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_display.h"

#if (BLE_APP_MESH)
#include "app_display_mesh.h"
#elif (BLE_APP_PRF)
#include "app_display_prf.h"
#endif //(BLE_APP_MESH)

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_display_init(uint8_t* p_dev_name, uint8_t demo_type)
{
    #if (BLE_APP_MESH)
    app_display_mesh_init(p_dev_name, demo_type);
    #elif (BLE_APP_PRF)
    app_display_prf_init();
    #endif //(BLE_APP_MESH)
}
#endif //(BLE_APP_PRESENT)

/// @} APP


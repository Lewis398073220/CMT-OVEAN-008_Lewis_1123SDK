/**
 ****************************************************************************************
 *
 * @file app_display_mesh.c
 *
 * @brief Application Display Mesh entry point
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "display.h"

#if (BLE_APP_MESH)

#include <stdio.h>
#include <string.h>
#include "app_display_mesh.h"
#include "app_display.h"
#include "app.h"
#include "app_task.h"
#include "ke_timer.h"
#include "co_utils.h"
#include "app_mesh.h"
#include "dbg_swdiag.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Length of the external led binary value
#define MESH_EXTERNAL_LED_BITS_LEN              (6)
/// Invalid screen identifier
#define APP_DISPLAY_MESH_INVALID_SCREEN_ID      (0xFF)

/*
 * TYPE DEFINTIIONS
 ****************************************************************************************
 */

/// Structure for mesh display environment
typedef struct app_display_mesh_env
{
    /* ******************************** */
    /* *       MESH DEMO BASIC        * */
    /* ******************************** */

    /// Mesh model screen
    uint8_t screen_id_mm;
    /// Information screen
    uint8_t screen_id_info;
    /// Name screen
    uint8_t screen_id_name;
    /// Attention screen
    uint8_t screen_id_attention;
    /// IV Index screen
    uint8_t screen_id_iv;
    /// Unicast Address screen
    uint8_t screen_id_addr;
    /// Provisioning State screen
    uint8_t screen_id_prov;
    /// Connection State screen
    uint8_t screen_id_con;
    /// Save screen
    uint8_t screen_id_save;
    /// Remove screen
    uint8_t screen_id_remove;
    /// Screen when attention screen is shown
    uint8_t prev_screen_id;

    /// Remaining attention timer duration
    uint32_t attention_time_s;
    /// Demonstration type
    uint8_t mesh_demo_type;
    /// Current connection state (true = connected, false = idle)
    bool connected;
    /// Current provisioning (true = provisioned, false = unprovisioned)
    bool provisioned;
    /// Indicate if saving mesh information
    bool saving;
    /// Indicate if removing mesh specific storage
    bool removing;

    /// Transition time step duration (in milliseconds)
    uint32_t trans_step_ms;
    /// Remaining number of steps for transition
    uint32_t trans_step_nb;
    /// Transition steps (power percentage)
    int8_t trans_step;
    /// Power percentage
    uint8_t power_percent;
    /// Power percentage target
    uint8_t power_percent_tgt;

    /// Pointer to callback functions provided by mesh module
    app_display_mesh_cb_t* p_mesh_cb;

    /* ******************************** */
    /* *    GENERIC SERVER ONOFF      * */
    /* ******************************** */

    /// Generic Server On Off State screen
    uint8_t screen_id_gens_onoff;

    /* ******************************** */
    /* *    GENERIC CLIENT ONOFF      * */
    /* ******************************** */

    /// Generic Client On Off State screen
    uint8_t screen_id_genc_onoff;
    /// Group Address screen for Generic Client On Off
    uint8_t screen_id_group_addr;

    /* ******************************** */
    /* *       LIGHTNESS SERVER       * */
    /* ******************************** */

    /// Lightness Server Lightness Screen
    uint8_t screen_id_lights_ln;

    /* ******************************** */
    /* *       LIGHTNESS CLIENT       * */
    /* ******************************** */

    /// Lightness Client Lightness Screen
    uint8_t screen_id_lightc_ln;
    /// Indicate if screen showing Light Lightness state (Client) is selected
    bool lightc_ln_select;
} app_display_mesh_env_t;

/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */

/// Application display environment
__STATIC app_display_mesh_env_t env;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Update SW diagport value in order to provide lightness value to an external device
 *
 * @param[in] lightness    Lightness
 ****************************************************************************************
 */
__STATIC void app_display_mesh_export_lightness(void)
{
    // Compute the lightness
    uint16_t lightness = ((uint32_t)env.power_percent * 65535) / 100;
    // Output
    uint8_t led_output[MESH_EXTERNAL_LED_BITS_LEN] = {0};
    // Counter
    int cnt = 0;
    // Scales an integer from 0 to 65535 to an integer from 0 to 63 to facilitate conversion to a 6-bit
    // binary number
    uint16_t temp = ((lightness + 1) / 1024) - 1;

    if (lightness == 0)
    {
        temp = 0;
    }

    // Convert a short integer to a 6-bit binary number
    while (temp != 0)
    {
        led_output[cnt] = (temp % 2);
        cnt++;
        temp = (temp / 2);
    }

    // Set the SW diagports to send 6-bit binary number
    DBG_SWDIAG(LIGHT, LIGHTNESS0, led_output[0]);
    DBG_SWDIAG(LIGHT, LIGHTNESS1, led_output[1]);
    DBG_SWDIAG(LIGHT, LIGHTNESS2, led_output[2]);
    DBG_SWDIAG(LIGHT, LIGHTNESS3, led_output[3]);
    DBG_SWDIAG(LIGHT, LIGHTNESS4, led_output[4]);
    DBG_SWDIAG(LIGHT, LIGHTNESS5, led_output[5]);
}

/**
 ****************************************************************************************
 * @brief Update content of the screen showing Generic OnOff state (Server)
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC void app_display_mesh_upd_screen_gens_onoff(void)
{
    char string[DISPLAY_LINE_SIZE + 1];

    // Number of spaces depends on displayed percentage
    if (env.power_percent == 0)
    {
        sprintf(string, "< OnOff    0p  s");
    }
    else if (env.power_percent == 100)
    {
        sprintf(string, "< OnOff  100p  s");
    }
    else
    {
        sprintf(string, "< OnOff   %dp  s", env.power_percent);
    }

    display_screen_update(env.screen_id_gens_onoff, 0, string);

    if (env.power_percent)
    {
        display_screen_update(env.screen_id_gens_onoff, 1, "->ON        OFF");
    }
    else
    {
        display_screen_update(env.screen_id_gens_onoff, 1, "  ON      ->OFF");
    }
}

/**
 ****************************************************************************************
 * @brief Update content of the screen showing Generic OnOff state (Client)
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC void app_display_mesh_upd_screen_genc_onoff(void)
{
    char string[DISPLAY_LINE_SIZE + 1];

    // Number of spaces depends on displayed percentage
    if (env.power_percent == 0)
    {
        sprintf(string, "< OnOff    0p  s");
    }
    else if (env.power_percent == 100)
    {
        sprintf(string, "< OnOff  100p  s");
    }
    else
    {
        sprintf(string, "< OnOff   %dp  s", env.power_percent);
    }

    display_screen_update(env.screen_id_genc_onoff, 0, string);

    if (env.power_percent)
    {
        display_screen_update(env.screen_id_genc_onoff, 1, "->ON        OFF");
    }
    else
    {
        display_screen_update(env.screen_id_genc_onoff, 1, "  ON      ->OFF");
    }
}

/**
 ****************************************************************************************
 * @brief Update content of the screen showing Light Lightness state (Server)
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC void app_display_mesh_upd_screen_lights_ln(void)
{
    char string[DISPLAY_LINE_SIZE + 1];

    // Number of spaces depends on displayed percentage
    if (env.power_percent == 0)
    {
        sprintf(string, "          0p");
    }
    else if (env.power_percent == 100)
    {
        sprintf(string, "        100p");
    }
    else
    {
        sprintf(string, "         %dp", env.power_percent);
    }

    // Update content of the lightness screen
    display_screen_update(env.screen_id_lights_ln, 0, "< Lightness");
    display_screen_update(env.screen_id_lights_ln, 1, string);
}

/**
 ****************************************************************************************
 * @brief Update content of the screen showing Light Lightness state (Client)
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC void app_display_mesh_upd_screen_lightc_ln(void)
{
    char string[DISPLAY_LINE_SIZE + 1];

    // Number of spaces depends on displayed percentage
    if (env.power_percent == 0)
    {
        sprintf(string, "          0p");
    }
    else if (env.power_percent == 100)
    {
        sprintf(string, "        100p");
    }
    else
    {
        sprintf(string, "         %dp", env.power_percent);
    }

    // Update content of the lightness screen
    if (env.lightc_ln_select)
    {
        display_screen_update(env.screen_id_lightc_ln, 0, "< Lightness    o");
    }
    else
    {
        display_screen_update(env.screen_id_lightc_ln, 0, "< Lightness    s");
    }
    display_screen_update(env.screen_id_lightc_ln, 1, string);
}

/**
 ****************************************************************************************
 * @brief Handles expiration of attention timer
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int app_display_mesh_attention_timer_handler(ke_msg_id_t const msgid, void const* p_param,
                                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Decrease remaining attention timer duration
    env.attention_time_s--;

    if (env.attention_time_s)
    {
        char string[DISPLAY_LINE_SIZE];

        // Restart the timer for 1 second
        ke_timer_set(APP_MESH_ATTENTION_TIMER, TASK_APP, 1000);

        // String creation
        sprintf(string, "       %lds", env.attention_time_s);

        // Update attention screen in order to display remaining attention duration
        display_screen_update(env.screen_id_attention, 1, string);
    }
    else
    {
        // Jump to the screen displayed before attention timer was launched
        display_goto_screen(env.prev_screen_id);
    }

    // Refresh current screen
    display_refresh();

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles expiration of saving timer
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int app_display_mesh_saving_timer_handler(ke_msg_id_t const msgid, void const* p_param,
                                                   ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Update saving state
    env.saving = false;

    // Update the screen allowing to store mesh information
    display_screen_update(env.screen_id_save, 0, "< Save         s");
    display_screen_update(env.screen_id_save, 1, "      Ready     ");

    // Refresh current screen
    display_refresh();

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles expiration of removing timer
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

__STATIC int app_display_mesh_removing_timer_handler(ke_msg_id_t const msgid, void const* p_param,
                                                     ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Update removing state
    env.removing = false;

    // Update the screen allowing to remove stored mesh information
    display_screen_update(env.screen_id_remove, 1, "      Ready     ");

    // Refresh current screen
    display_refresh();

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles expiration of transition timer
 *
 * @param[in] msgid     ID of the message received.
 * @param[in] p_param   Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int app_display_mesh_transition_timer_handler(ke_msg_id_t const msgid, void const* p_param,
                                                       ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Decrease remaining number of steps
    env.trans_step_nb--;

    // Update power percentage
    if (env.trans_step_nb)
    {
        env.power_percent += env.trans_step;

        // Restart the transition timer
        ke_timer_set(APP_MESH_TRANSITION_TIMER, TASK_APP,  env.trans_step_ms);
    }
    else
    {
        env.power_percent = env.power_percent_tgt;
    }

    // Update screen showing Generic OnOff state
    app_display_mesh_upd_screen_gens_onoff();

    // Update screen showing Light Lightness state
    app_display_mesh_upd_screen_lights_ln();

    // Display on external light bulb
    app_display_mesh_export_lightness();

    // Refresh current screen
    display_refresh();

    // Message can be consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles user input from display in the saving configuration screen
 *
 * @param[in] input     User input from display
 ****************************************************************************************
 */
__STATIC void app_display_mesh_cb_save(uint8_t input)
{
    if (input == DISPLAY_INPUT_SELECT)
    {
        // Ensure that another save procedure is not pending
        if (env.saving == false)
        {
            // Update screen content - Screen is not more selectable
            display_screen_update(env.screen_id_save, 0, "< Save          ");
            display_screen_update(env.screen_id_save, 1, "    Saving...   ");

            // Keep in mind that a save procedure is in progress
            env.saving = true;

            // Inform mesh module about user request
            env.p_mesh_cb->cb_save();

            // Refresh information displayed on screen
            display_refresh();
        }
    }
}

/**
 ****************************************************************************************
 * @brief Handles user input from display in the removing mesh specific storage screen
 *
 * @param[in] input     User input from display
 ****************************************************************************************
 */
__STATIC void app_display_mesh_cb_remove(uint8_t input)
{
    if (input == DISPLAY_INPUT_SELECT)
    {
        // Ensure that another remove procedure is not pending
        if (env.removing == false)
        {
            // Keep in mind that a remove procedure is in progress
            env.removing = true;

            // Inform mesh module about user request
            env.p_mesh_cb->cb_remove();

            // Update screen content - Screen is not more selectable
            display_screen_update(env.screen_id_remove, 0, "< Remove        ");
            display_screen_update(env.screen_id_remove, 1, "     Removing   ");

            // Refresh information displayed on screen
            display_refresh();

            // Launch removing timer for 5 seconds
            ke_timer_set(APP_MESH_REMOVING_TIMER, TASK_APP, 5000);
        }
    }
}


/**
 ****************************************************************************************
 * @brief Handles user input from display in the generic server onoff screen
 *
 * @param[in] input     User input from display
 ****************************************************************************************
 */

__STATIC void app_display_mesh_cb_gens_onoff(uint8_t input)
{
    // Do not authorize this action before end of provisioning
    if (env.provisioned
            && (input == DISPLAY_INPUT_SELECT))
    {
        if (env.power_percent == 0)
        {
            // Switch to On state
            env.power_percent = 100;
        }
        else
        {
            // Switch to Off state
            env.power_percent = 0;
        }

        // Provide the new lightness value to the external device
        app_display_mesh_export_lightness();

        // Update screen content
        app_display_mesh_upd_screen_gens_onoff();

        // Inform the mesh stack about the state
        env.p_mesh_cb->cb_gens_onoff((env.power_percent == 0) ? 0 : 1);

        // Refresh current screen
        display_refresh();
    }
}


/**
 ****************************************************************************************
 * @brief Handles user input from display in the generic client onoff screen
 *
 * @param[in] input     User input from display
 ****************************************************************************************
 */
__STATIC void app_display_mesh_cb_genc_onoff(uint8_t input)
{
    // Do not authorize this action before end of provisioning
    if (env.provisioned
            && (input == DISPLAY_INPUT_SELECT))
    {
        if (env.power_percent == 0)
        {
            // Switch to On state
            env.power_percent = 100;
        }
        else
        {
            // Switch to Off state
            env.power_percent = 0;
        }

        // Update screen showing the Generic OnOff state
        app_display_mesh_upd_screen_genc_onoff();

        // Inform the mesh stack about the required transition
        env.p_mesh_cb->cb_genc_onoff((env.power_percent == 0) ? 0 : 1);

        if (env.screen_id_lightc_ln != APP_DISPLAY_MESH_INVALID_SCREEN_ID)
        {
            // Update screen showing the Light Lightness state
            app_display_mesh_upd_screen_lightc_ln();
        }

        // Refresh current screen
        display_refresh();
    }
}

/**
 ****************************************************************************************
 * @brief Handles user input from display in the Light Client Lightness screen
 *
 * @param[in] input     User input from display
 ****************************************************************************************
 */
__STATIC void app_display_mesh_cb_lightc_ln(uint8_t input)
{
    // Do not authorize this action before end of provisioning
    if (env.provisioned)
    {
        if (input == DISPLAY_INPUT_SELECT)
        {
            env.lightc_ln_select = true;
        }
        else if (input == DISPLAY_INPUT_LEFT)
        {
            // Decrease the lightness value by 10%
            if ((env.power_percent - 10) >= 0)
            {
                env.power_percent -= 10;
            }
            else
            {
                env.power_percent = 0;
            }

            // Inform the mesh stack about the required transition
            env.p_mesh_cb->cb_lightc_lightness(((uint32_t)env.power_percent * 65535) / 100);
        }
        else if (input == DISPLAY_INPUT_RIGHT)
        {
            // Increase the lightness value by 10%
            if (env.power_percent <= (100 - 10))
            {
                env.power_percent += 10;
            }
            else
            {
                env.power_percent = 100;
            }

            // Inform the mesh stack about the required transition
            env.p_mesh_cb->cb_lightc_lightness(((uint32_t)env.power_percent * 65535) / 100);
        }
        else if (input == DISPLAY_INPUT_DESELECT)
        {
            env.lightc_ln_select = false;
        }

        // Update screen showing the Light Lightness state
        app_display_mesh_upd_screen_lightc_ln();

        // Update screen showing the Generic OnOff state
        app_display_mesh_upd_screen_genc_onoff();

        // Refresh current screen
        display_refresh();
    }
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_display_mesh_init(uint8_t* p_dev_name, uint8_t mesh_demo_type)
{
    // Level 2 Mesh Model Top Screen
    uint8_t s_mdl_top;

    // Keep the provided demonstration type
    env.mesh_demo_type = mesh_demo_type;

    // Initialize screen identifiers for optional screen
    env.screen_id_gens_onoff = APP_DISPLAY_MESH_INVALID_SCREEN_ID;
    env.screen_id_genc_onoff = APP_DISPLAY_MESH_INVALID_SCREEN_ID;
    env.screen_id_lights_ln = APP_DISPLAY_MESH_INVALID_SCREEN_ID;
    env.screen_id_lightc_ln = APP_DISPLAY_MESH_INVALID_SCREEN_ID;
    env.screen_id_group_addr = APP_DISPLAY_MESH_INVALID_SCREEN_ID;

    /* ***********************************************************************/
    /*                              Screen Allocation                        */
    /* ***********************************************************************/

    // ------------------------------- Level 1 -------------------------------

    // Mesh Model
    env.screen_id_mm = display_screen_alloc();
    // Information
    env.screen_id_info = display_screen_alloc();

    // ------------------------- Level 2 - Mesh Model ------------------------

    if (env.mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
    {
        env.screen_id_gens_onoff = display_screen_alloc();
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
    {
        env.screen_id_genc_onoff = display_screen_alloc();
        env.screen_id_group_addr = display_screen_alloc();
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN)
    {
        env.screen_id_lights_ln = display_screen_alloc();
        env.screen_id_gens_onoff = display_screen_alloc();
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN)
    {
        env.screen_id_lightc_ln = display_screen_alloc();
        env.screen_id_genc_onoff = display_screen_alloc();
        env.screen_id_group_addr = display_screen_alloc();
    }
    env.power_percent = 0;

    // ------------------------- Level 2 - Information -----------------------

    env.screen_id_name = display_screen_alloc();
    env.screen_id_attention = display_screen_alloc();
    env.screen_id_iv = display_screen_alloc();
    env.screen_id_addr = display_screen_alloc();
    env.screen_id_prov = display_screen_alloc();
    env.screen_id_con = display_screen_alloc();
    env.screen_id_save = display_screen_alloc();
    env.screen_id_remove = display_screen_alloc();
    env.attention_time_s = 0;
    env.connected = false;
    env.provisioned = false;
    env.saving = false;
    env.removing = false;

    /* ***********************************************************************/
    /*                              Screen Insertion                         */
    /* ***********************************************************************/

    // ------------------------------- Level 1 -------------------------------

    display_screen_insert(env.screen_id_info, env.screen_id_mm);

    // ------------------------- Level 2 - Information -----------------------

    display_screen_insert(env.screen_id_prov, env.screen_id_name);
    display_screen_insert(env.screen_id_con, env.screen_id_name);
    display_screen_insert(env.screen_id_iv, env.screen_id_name);
    display_screen_insert(env.screen_id_addr, env.screen_id_name);

    // Group address displayed for client demonstration
    if ((env.mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
             || (env.mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN))
    {
        display_screen_insert(env.screen_id_group_addr, env.screen_id_name);
    }

    display_screen_insert(env.screen_id_save, env.screen_id_name);
    display_screen_insert(env.screen_id_remove, env.screen_id_name);

    // ------------------------- Level 2 - Mesh Model ------------------------

    if (env.mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
    {
        s_mdl_top = env.screen_id_gens_onoff;
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
    {
        s_mdl_top = env.screen_id_genc_onoff;
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN)
    {
        s_mdl_top = env.screen_id_lights_ln;
        display_screen_insert(env.screen_id_gens_onoff, s_mdl_top);
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN)
    {
        s_mdl_top = env.screen_id_lightc_ln;
        display_screen_insert(env.screen_id_genc_onoff, s_mdl_top);
    }
    else // if (env.mesh_demo_type == APP_MESH_DEMO_PROXY)
    {
        s_mdl_top = display_screen_alloc();
        display_screen_set(s_mdl_top, NULL, "< No Models", "");
    }

    /* ***********************************************************************/
    /*                              Screen List Link                         */
    /* ***********************************************************************/

    // Link Level 1 and Level 2 - Mesh Model
    display_screen_link(env.screen_id_mm, s_mdl_top);

    // Link Level 1 and Level 2 - Information
    display_screen_link(env.screen_id_info, env.screen_id_name);

    /* ***********************************************************************/
    /*                              Screen Set Content                       */
    /* ***********************************************************************/

    // ------------------------------ Attention ------------------------------

    display_screen_set(env.screen_id_attention, NULL, "!! ATTENTION !!", "");

    // ------------------------------- Level 1 -------------------------------

    display_screen_update(env.screen_id_mm, 0, "  Mesh Model   >");

    if (env.mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
    {
        display_screen_update(env.screen_id_mm, 1, "        (OO SRV)");
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
    {
        display_screen_update(env.screen_id_mm, 1, "        (OO CLI)");
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN)
    {
        display_screen_update(env.screen_id_mm, 1, "        (LN SRV)");
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN)
    {
        display_screen_update(env.screen_id_mm, 1, "        (LN CLI)");
    }
    else // if (env.mesh_demo_type == APP_MESH_DEMO_PROXY)
    {
        display_screen_update(env.screen_id_mm, 1, "         (PROXY)");
    }

    display_screen_set(env.screen_id_info, NULL, "  Information  >", "");

    // ------------------------- Level 2 - Information -----------------------

    display_screen_set(env.screen_id_name, NULL, "< Device Name", (char*)p_dev_name);
    display_screen_set(env.screen_id_iv, NULL, "< IV", "    UNKNOWN");
    display_screen_set(env.screen_id_addr, NULL, "< Unicast Addr", "    UNKNOWN");
    display_screen_set(env.screen_id_prov, NULL, "< Provisioned", "      NO");
    display_screen_set(env.screen_id_con, NULL, "< Connected", "      NO");
    display_screen_set(env.screen_id_save, &app_display_mesh_cb_save, "< Save         s", "      Ready");
    display_screen_set(env.screen_id_remove, &app_display_mesh_cb_remove, "< Remove       s", "      Ready");
    display_select_type_set(env.screen_id_save, DISPLAY_SELECT_TYPE_PUSH);
    display_select_type_set(env.screen_id_remove, DISPLAY_SELECT_TYPE_PUSH);

    if ((env.mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
            || (env.mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN))
    {
        display_screen_set(env.screen_id_group_addr, NULL, "< Group Addr", "    UNKNOWN    ");
    }

    // ------------------------- Level 2 - Mesh Model ------------------------

    if (env.mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
    {
        display_screen_set(env.screen_id_gens_onoff, &app_display_mesh_cb_gens_onoff, "", "");
        app_display_mesh_upd_screen_gens_onoff();
        display_select_type_set(env.screen_id_gens_onoff, DISPLAY_SELECT_TYPE_PUSH);
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_GENC_ONOFF)
    {
        display_screen_set(env.screen_id_genc_onoff, &app_display_mesh_cb_genc_onoff, "", "");
        app_display_mesh_upd_screen_genc_onoff();
        display_select_type_set(env.screen_id_genc_onoff, DISPLAY_SELECT_TYPE_PUSH);
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN)
    {
        display_screen_set(env.screen_id_gens_onoff, &app_display_mesh_cb_gens_onoff, "", "");
        app_display_mesh_upd_screen_gens_onoff();
        app_display_mesh_upd_screen_lights_ln();
    }
    else if (env.mesh_demo_type == APP_MESH_DEMO_LIGHTC_LN)
    {
        display_screen_set(env.screen_id_genc_onoff, &app_display_mesh_cb_genc_onoff, "", "");
        display_screen_set(env.screen_id_lightc_ln, &app_display_mesh_cb_lightc_ln, "", "");
        app_display_mesh_upd_screen_genc_onoff();
        app_display_mesh_upd_screen_lightc_ln();
        display_select_type_set(env.screen_id_genc_onoff, DISPLAY_SELECT_TYPE_PUSH);
    }

    if ((env.mesh_demo_type == APP_MESH_DEMO_GENS_ONOFF)
            || (env.mesh_demo_type == APP_MESH_DEMO_LIGHTS_LN))
    {
        // Initialize lightness on external device
        app_display_mesh_export_lightness();
    }

    // Start with Mesh Model screen
    display_start(env.screen_id_mm);
}

void app_display_register_cb_mesh(const app_display_mesh_cb_t* p_mesh_cb)
{
    // Keep provided pointer
    env.p_mesh_cb = (app_display_mesh_cb_t*)p_mesh_cb;
}

void app_display_mesh_set_con(bool state)
{
    // Update content of connection screen
    display_screen_update(env.screen_id_con, 1, (state) ? "      YES" : "      NO");

    // Refresh current screen
    display_refresh();
}

void app_display_mesh_set_prov(bool state)
{
    // Keep provided state
    env.provisioned = state;

    // Update content of provisioning screen
    display_screen_update(env.screen_id_prov, 1, (state) ? "      YES" : "      NO");

    // Refresh current screen
    display_refresh();

    if (state)
    {
        // Set IV Index and Unicast Address
        app_display_mesh_set_iv(0);
        app_display_mesh_set_unicast_addr(0x0001);
    }
}

void app_display_mesh_set_iv(uint32_t iv)
{
    char string[DISPLAY_LINE_SIZE];

    // Create string containing the IV
    sprintf(string, "   0x%08lX", iv);

    // Update content of IV screen
    display_screen_update(env.screen_id_iv, 1, string);

    // Refresh current screen
    display_refresh();
}

void app_display_mesh_set_unicast_addr(uint16_t addr)
{
    char string[DISPLAY_LINE_SIZE];

    // Create string containing the unicast address
    sprintf(string, "       0x%04X", addr);

    // Update content of unicast address screen
    display_screen_update(env.screen_id_addr, 1, string);

    // Refresh current screen
    display_refresh();
}

void app_display_mesh_set_group_addr(uint16_t group_addr)
{
    // Check if screen showing group address is used
    if (env.screen_id_group_addr != APP_DISPLAY_MESH_INVALID_SCREEN_ID)
    {
        char string[DISPLAY_LINE_SIZE];

        // Create string containing the group address
        sprintf(string, "       0x%04X", group_addr);

        // Update content of group address screen
        display_screen_update(env.screen_id_group_addr, 1, string);

        // Refresh current screen
        display_refresh();
    }
}

void app_display_mesh_set_lightness(uint32_t trans_time_ms, uint16_t lightness)
{
    // Check if screen showing Light Lightness state is used
    if (env.screen_id_lights_ln != APP_DISPLAY_MESH_INVALID_SCREEN_ID)
    {
        // Clear transition timer
        ke_timer_clear(APP_MESH_TRANSITION_TIMER, TASK_APP);

        if (trans_time_ms)
        {
            // Initialize transition time step duration to one tenth of transition time
            env.trans_step_ms = trans_time_ms / 10;
            // Initialize remaining number of steps
            env.trans_step_nb = 10;
            // Initialize target state
            env.power_percent_tgt = ((uint32_t)lightness) * 100 / 65535;
            // Power step
            env.trans_step = (env.power_percent_tgt - env.power_percent) / 10;

            // Set transition timer
            ke_timer_set(APP_MESH_TRANSITION_TIMER, TASK_APP, env.trans_step_ms);
        }
        else
        {
            // Update power percentage value
            env.power_percent = ((uint32_t)lightness * 100) / 65535;

            // Forward the received value to the external bulb
            app_display_mesh_export_lightness();

            // Update screen showing the Light Lightness state
            app_display_mesh_upd_screen_lights_ln();

            // Update screen showing the Generic OnOff state
            app_display_mesh_upd_screen_gens_onoff();

            // Refresh current screen
            display_refresh();
        }
    }
}

void app_display_mesh_set_onoff(uint32_t trans_time_ms, bool state)
{
    // Check if screen showing Generic OnOff state is used
    if (env.screen_id_gens_onoff != APP_DISPLAY_MESH_INVALID_SCREEN_ID)
    {
        // Clear transition timer
        ke_timer_clear(APP_MESH_TRANSITION_TIMER, TASK_APP);

        // Check if transition is immediate or not
        if (trans_time_ms)
        {
            // Initialize transition time step duration to one tenth of transition time
            env.trans_step_ms = trans_time_ms / 10;
            // Initialize remaining number of steps
            env.trans_step_nb = 10;
            // Initialize target Generic OnOff state
            env.power_percent_tgt = (state) ? 0 : 100;
            // Power step
            env.trans_step = (env.power_percent_tgt - env.power_percent) / 10;

            // Set transition timer
            ke_timer_set(APP_MESH_TRANSITION_TIMER, TASK_APP, env.trans_step_ms);
        }
        else
        {
            // Update generic OnOff state and update the screen
            env.power_percent = (state) ? 100 : 0;

            // Forward the received value to the external bulb
            app_display_mesh_export_lightness();

            // Update screen showing the Generic OnOff state
            app_display_mesh_upd_screen_gens_onoff();

            // Refresh current screen
            display_refresh();
        }
    }
}

void app_display_mesh_set_attention(uint32_t duration_s)
{
    if (duration_s != 0)
    {
        char string[DISPLAY_LINE_SIZE];

        // Keep provided duration
        env.attention_time_s = duration_s;

        // Keep current screen index
        env.prev_screen_id = display_id_get();

        // Start timer allowing to monitor attention timer duration
        ke_timer_set(APP_MESH_ATTENTION_TIMER, TASK_APP, 1000);

        // String creation
        sprintf(string, "       %lds", duration_s);

        // Update attention screen in order to display remaining attention duration
        display_screen_update(env.screen_id_attention, 1, string);

        // Display the attention screen
        display_goto_screen(env.screen_id_attention);
    }
    else
    {
        // Jump to the screen showing the provisioning state
        display_goto_screen(env.prev_screen_id);

        // Clean the attention timer
        ke_timer_clear(APP_MESH_ATTENTION_TIMER, TASK_APP);
    }

    // Refresh current screen
    display_refresh();
}

void app_display_mesh_save_end(void)
{
    // Update save screen content
    display_screen_update(env.screen_id_save, 1, "      OK");

    // Refresh current screen
    display_refresh();

    // Start saving timer for 5 seconds
    ke_timer_set(APP_MESH_SAVING_TIMER, TASK_APP, 5000);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

const struct ke_msg_handler app_display_mesh_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel
    {APP_MESH_ATTENTION_TIMER, (ke_msg_func_t)app_display_mesh_attention_timer_handler},
    {APP_MESH_SAVING_TIMER, (ke_msg_func_t)app_display_mesh_saving_timer_handler},
    {APP_MESH_REMOVING_TIMER, (ke_msg_func_t)app_display_mesh_removing_timer_handler},
    {APP_MESH_TRANSITION_TIMER, (ke_msg_func_t)app_display_mesh_transition_timer_handler},
};

const struct app_subtask_handlers app_display_mesh_handlers = APP_HANDLERS(app_display_mesh);

#endif //(BLE_APP_MESH)

/// @} APP


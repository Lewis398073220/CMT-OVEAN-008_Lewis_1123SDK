/**
 ****************************************************************************************
 *
 * @file app_hid.c
 *
 * @brief HID Application Module entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"            // SW configuration

#include <stdio.h>
#include <string.h>

#if (BLE_APP_HID)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app.h"                    // Application Definitions
#include "app_sec.h"                // Application Security Module API
#include "app_task.h"               // Application task definitions
#include "app_hid.h"                // HID Application Module Definitions
#include "hogpd_msg.h"             // HID Over GATT Profile Device Role Functions
#include "prf_types.h"              // Profile common types Definition
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "ke_timer.h"
#include "gapm_msg.h"
#include "ble_app_dbg.h"

#if (NVDS_SUPPORT)
#include "nvds.h"                   // NVDS Definitions
#endif //(NVDS_SUPPORT)

#if (DISPLAY_SUPPORT)
#include "app_display.h"            // Application Display Module
#endif //(DISPLAY_SUPPORT)

#include "co_utils.h"               // Common functions

#if (KE_PROFILING)
#include "ke_mem.h"
#endif //(KE_PROFILING)

//#define HID_VOL_TEST

#if defined(HID_VOL_TEST)
uint8_t key_define = HID_KEY_IDX_PP;
#endif 

/*
 * DEFINES
 ****************************************************************************************
 */

/// Length of the HID Mouse Report
#define APP_HID_MOUSE_REPORT_LEN       (6)
/// Length of the Report Descriptor for an HID Mouse
#define APP_HID_MOUSE_REPORT_MAP_LEN   (sizeof(app_hid_mouse_report_map))

/// Duration before connection update procedure if no report received (mouse is silent) - 20s
#define APP_HID_SILENCE_DURATION_1     (2000)
/// Duration before disconnection if no report is received after connection update - 60s
#define APP_HID_SILENCE_DURATION_2     (6000)

/// Number of reports that can be sent
#define APP_HID_NB_SEND_REPORT         (10)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// States of the Application HID Module
enum app_hid_states
{
    /// Module is disabled (Service not added in DB)
    APP_HID_DISABLED,
    /// Module is idle (Service added but profile not enabled)
    APP_HID_IDLE,
    /// Module is enabled (Device is connected and the profile is enabled)
    APP_HID_ENABLED,
    /// The application can send reports
    APP_HID_READY,
    /// Waiting for a report
    APP_HID_WAIT_REP,

    APP_HID_STATE_MAX,
};

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// HID Application Module Environment Structure
static struct app_hid_env_tag app_hid_env;

#ifdef HOGP_HEAD_TRACKER_PROTOCOL
static const uint8_t hid_consumer_report_map[] =
{
    HID_REPORT_MODE_SENSOR_DESCRIPTOR_DATA,
};
#else
#if defined(HID_MOUSE)
/// HID Mouse Report Descriptor
static const uint8_t app_hid_mouse_report_map[] =
{
    /**
     *  --------------------------------------------------------------------------
     *  Bit      |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *  --------------------------------------------------------------------------
     *  Byte 0   |               Not Used                | Middle| Right | Left  |
     *  --------------------------------------------------------------------------
     *  Byte 1   |                     X Axis Relative Movement                  |
     *  --------------------------------------------------------------------------
     *  Byte 2   |                     Y Axis Relative Movement                  |
     *  --------------------------------------------------------------------------
     *  Byte 3   |                     Wheel Relative Movement                   |
     *  --------------------------------------------------------------------------
     */

    0x05, 0x01,     /// USAGE PAGE (Generic Desktop)
    0x09, 0x02,     /// USAGE (Mouse)
    0xA1, 0x01,     /// COLLECTION (Application)
    0x85, 0x01,     /// REPORT ID (1) - MANDATORY
    0x09, 0x01,     ///     USAGE (Pointer)
    0xA1, 0x00,     ///     COLLECTION (Physical)

    /**
     * ----------------------------------------------------------------------------
     * BUTTONS
     * ----------------------------------------------------------------------------
     */
    0x05, 0x09,     ///         USAGE PAGE (Buttons)
    0x19, 0x01,     ///         USAGE MINIMUM (1)
    0x29, 0x08,     ///         USAGE MAXIMUM (8)
    0x15, 0x00,     ///         LOGICAL MINIMUM (0)
    0x25, 0x01,     ///         LOGICAL MAXIMUM (1)
    0x75, 0x01,     ///         REPORT SIZE (1)
    0x95, 0x08,     ///         REPORT COUNT (8)
    0x81, 0x02,     ///         INPUT (Data, Variable, Absolute)

    /**
     * ----------------------------------------------------------------------------
     * MOVEMENT DATA
     * ----------------------------------------------------------------------------
     */
    0x05, 0x01,     ///         USAGE PAGE (Generic Desktop)
    0x16, 0x08, 0xFF, ///       LOGICAL MINIMUM (-255)
    0x26, 0xFF, 0x00, ///       LOGICAL MAXIMUM (255)
    0x75, 0x10,     ///         REPORT SIZE (16)
    0x95, 0x02,     ///         REPORT COUNT (2)
    0x09, 0x30,     ///         USAGE (X)
    0x09, 0x31,     ///         USAGE (Y)
    0x81, 0x06,     ///         INPUT (Data, Variable, Relative)

    0x15, 0x81,     ///         LOGICAL MINIMUM (-127)
    0x25, 0x7F,     ///         LOGICAL MAXIMUM (127)
    0x75, 0x08,     ///         REPORT SIZE (8)
    0x95, 0x01,     ///         REPORT COUNT (1)
    0x09, 0x38,     ///         USAGE (Wheel)
    0x81, 0x06,     ///         INPUT (Data, Variable, Relative)

    0xC0,           ///     END COLLECTION (Physical)
    0xC0            /// END COLLECTION (Application)
};
#else
static const uint8_t hid_consumer_report_map[] =
{
    0x05,0x0c,
    0x09,0x01,
    0xA1,0x01,
    0x85,0x02,
    0x09,0xCD,    //usage playpause
    0x09,0xB7,    //usage stop
    0x09,0xB6,    //usage previous
    0x09,0xB5,    //usage next
    0x09,0xE9,    //usage vol+
    0x09,0xEA,    //usage vol-
    0x0A,0xB1,0x01, // usage screen saver
    0x09,0x30,    //usage power
    0x95,0x01,
    0x75,0x10,
    0x15,0x01,
    0x26,0x9C,0x02,
    0x19,0x01,
    0x2A,0x9C,0x02,
    0x81,0x00,
    0xC0,
    0x05,0x01,
    0x09,0x06,
    0xA1,0x01,
    0x85,0x03,
    0x05,0x07,
    0x19,0xE0,
    0x29,0xE7,
    0x15,0x00,
    0x25,0x01,
    0x75,0x01,
    0x95,0x08,
    0x81,0x02,
    0x75,0x08,
    0x95,0x01,
    0x15,0x00,
    0x25,0xF4,
    0x05,0x07,
    0x19,0x00,
    0x29,0xF4,
    0x81,0x00,
    0xC0
};
#endif
#endif

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#define NVDS_LEN_MOUSE_TIMEOUT 100

#ifdef HOGP_HEAD_TRACKER_PROTOCOL
#define HOGP_REPORT_INDEX_SENSOR_DESCR_FEATURE  0
#define HOGP_REPORT_INDEX_SENSOR_STATE_FEATURE  1
#define HOGP_REPORT_INDEX_SENSOR_VALUE_INPUT    2

static const uint8_t hid_report_id[] = {
        HID_SENSOR_DESCR_FEATURE_REPORT_ID,
        HID_SENSOR_STATE_FEATURE_REPORT_ID,
        HID_SENSOR_VALUE_INPUT_REPORT_ID,
    };

static const uint8_t hid_report_type[] = {
        HOGPD_CFG_REPORT_FEAT,
        HOGPD_CFG_REPORT_FEAT,
        HOGPD_CFG_REPORT_IN,
    };
#else
static const uint8_t hid_report_id[] = {2};
static const uint8_t hid_report_type[] = {HOGPD_CFG_REPORT_IN};
#endif

void app_hid_init(void)
{
    // Reset the environment
    memset(&app_hid_env, 0, sizeof(app_hid_env));

    app_hid_env.num_report = APP_HID_NB_SEND_REPORT;

    app_hid_env.report_map = (uint8_t *)hid_consumer_report_map;
    app_hid_env.report_map_len = sizeof(hid_consumer_report_map);

    app_hid_env.report_nb = sizeof(hid_report_id)/sizeof(uint8_t);
    app_hid_env.report_id = (uint8_t *)hid_report_id;
    app_hid_env.report_type = (uint8_t *)hid_report_type;

    /*
     * Get the timeout value from the NVDS - This value is used each time a report is received
     * from the PS2 driver, store it in the environment.
     */
    #if (NVDS_SUPPORT)
    // Length of the mouse timeout value
    uint8_t length = NVDS_LEN_MOUSE_TIMEOUT;
    if (nvds_get(NVDS_TAG_MOUSE_TIMEOUT, &length, (uint8_t *)&app_hid_env.timeout) != NVDS_OK)
    {
        app_hid_env.timeout = APP_HID_SILENCE_DURATION_1;
    }
    #endif
}


/*
 ****************************************************************************************
 * @brief Function called when get GAP manager command complete events.
 *
 ****************************************************************************************
 */
void app_hid_start_mouse(void)
{
    /*-----------------------------------------------------------------------------------
     * CONFIGURE THE MOUSE
     *----------------------------------------------------------------------------------*/
    #if (PS2_SUPPORT)
    // Default mouse rate (200 report/s)
    uint8_t rate = 200;

    #if (NVDS_SUPPORT)
    uint8_t length = NVDS_LEN_MOUSE_SAMPLE_RATE;

    // Get sample rate from NVDS
    if (nvds_get(NVDS_TAG_MOUSE_SAMPLE_RATE, &length, &rate) == NVDS_OK)
    {
        // Check if value is among supported set
        if ((rate != 10) && (rate != 20) && (rate != 40) && (rate != 60) &&
            (rate != 80) && (rate != 100) && (rate != 200) )
        {
            // Default value
            rate = 200;
        }
    }
    #endif //(NVDS_SUPPORT)

    // Start PS2_mouse
    ps2_mouse_start(rate, &app_hid_send_mouse_report);
    #endif //(PS2_SUPPORT)
}

void app_hid_add_hids(void)
{
    struct hogpd_db_cfg *db_cfg;
    // Prepare the HOGPD_CREATE_DB_REQ message
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                   TASK_GAPM, TASK_APP,
                                                   gapm_profile_task_add_cmd, sizeof(struct hogpd_db_cfg));

    // Fill message
    req->operation   = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl     =  (uint8_t)(SVC_SEC_LVL(AUTH)| ATT_UUID(128));//PERM(SVC_AUTH, AUTH);| ATT_UUID(128)
    req->prf_api_id = TASK_ID_HOGPD;  //req->prf_task_id 
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    // Set parameters
    db_cfg = (struct hogpd_db_cfg* ) req->param;

    // Only one HIDS instance is useful
    db_cfg->hids_nb = 1;

    // The device is a mouse HOGPD_CFG_MOUSE_BIT;
    //The device is a KEYBOARD
    db_cfg->cfg[0].svc_features =HOGPD_CFG_PROTO_MODE_BIT;

    // Only one Report Characteristic is requested
    db_cfg->cfg[0].report_nb    = app_hid_env.report_nb;
    memcpy(db_cfg->cfg[0].report_id, app_hid_env.report_id, app_hid_env.report_nb);
    memcpy(db_cfg->cfg[0].report_char_cfg, app_hid_env.report_type, app_hid_env.report_nb);

    TRACE(0, "hogp app add hids: report_nb %d report id %d %d %d report type %d %d %d",
        db_cfg->cfg[0].report_nb,
        db_cfg->cfg[0].report_id[0], db_cfg->cfg[0].report_id[1], db_cfg->cfg[0].report_id[2],
        db_cfg->cfg[0].report_char_cfg[0], db_cfg->cfg[0].report_char_cfg[1], db_cfg->cfg[0].report_char_cfg[2]);

    // HID Information
    db_cfg->cfg[0].hid_info.bcdHID       = 0x0111;         // HID Version 1.11
    db_cfg->cfg[0].hid_info.bCountryCode = 0x00;
    db_cfg->cfg[0].hid_info.flags        = HIDS_REMOTE_WAKE_CAPABLE| HIDS_NORM_CONNECTABLE;

    // Send the message
    ke_msg_send(req);
}

/*
 ****************************************************************************************
 * @brief Function called when get connection complete event from the GAP
 *
 ****************************************************************************************
 */
void app_hid_enable_prf(uint8_t conidx, bool open)
{
    TRACE(0, "hogp app enable prf: conidx %d open %d", conidx, open);

    if (open == false)
    {
        return;
    }

    if(APP_HID_ENABLED<=app_hid_env.state)
        return;
    // Requested connection parameters
    //struct gapc_conn_param conn_param;

    uint16_t ntf_cfg;

    // Store the connection handle
    app_hid_env.conidx = conidx;

    // Allocate the message
    struct hogpd_enable_req * req = KE_MSG_ALLOC(HOGPD_ENABLE_REQ,
                                                 KE_BUILD_ID(prf_get_task_from_id(TASK_ID_HOGPD),conidx),
                                                 TASK_APP,
                                                 hogpd_enable_req);

    // Fill in the parameter structure
    req->conidx     = conidx;
    // Notifications are disabled
    ntf_cfg         = 0;

    // Go to Enabled state
    app_hid_env.state = APP_HID_ENABLED;

    // If first connection with the peer device
    if (app_sec_get_bond_status())
    {
        #if (NVDS_SUPPORT)
        // Length of the value read in NVDS
        uint8_t length   = NVDS_LEN_MOUSE_NTF_CFG;
        // Notification configuration

        if (nvds_get(NVDS_TAG_MOUSE_NTF_CFG, &length, (uint8_t *)&ntf_cfg) != NVDS_OK)
        {
            // If we are bonded this information should be present in the NVDS
            ASSERT_ERR(0);
        }
        #endif //(NVDS_SUPPORT)
        
        ntf_cfg = 0x40;
        // CCC enable notification
        if ((ntf_cfg & HOGPD_CFG_REPORT_NTF_EN ) != 0)
        {
            // The device is ready to send reports to the peer device
            app_hid_env.state = APP_HID_READY;
            app_hid_env.report_map_len = sizeof(hid_consumer_report_map);
            
            // Restart the mouse timeout timer if needed
            if (app_hid_env.timeout != 0)
            {
                ke_timer_set(APP_HID_MOUSE_TIMEOUT_TIMER, TASK_APP, (uint16_t)(app_hid_env.timeout));
                app_hid_env.timer_enabled = true;
            }
        }
    }
    else
    {
        TRACE(0, "hogp app enable prf: bond_status %x ntf_cfg %x", app_sec_get_bond_status(), ntf_cfg);
    }

    req->ntf_cfg[conidx] = ntf_cfg;

    /*
     * Requested connection interval: 10ms
     * Latency: 25
     * Supervision Timeout: 2s
     */
  /*conn_param.intv_min = 8;
    conn_param.intv_max = 8;
    conn_param.latency  = 25;
    conn_param.time_out = 200;

    appm_update_param(conidx,&conn_param);*/
    // Send the message
    ke_msg_send(req);
}

void app_hid_send_report(uint8_t * report_buf, uint16_t len, uint8_t reportidx)
{
    app_hid_env.state = APP_HID_READY;
    if (app_hid_env.state == APP_HID_READY)
    {
       // Allocate the HOGPD_REPORT_UPD_REQ message
       struct hogpd_report_upd_req * req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_UPD_REQ,
                                                            KE_BUILD_ID(prf_get_task_from_id(TASK_ID_HOGPD),app_hid_env.conidx),
                                                            TASK_APP,
                                                            hogpd_report_upd_req,
                                                            len);
        req->conidx  = app_hid_env.conidx;
        //now fill report
        req->hid_idx  = app_hid_env.conidx;
        req->report_type     = HOGPD_REPORT; //HOGPD_BOOT_MOUSE_INPUT_REPORT;//
        req->report_idx = reportidx;
        req->report.length   = len;
        memcpy(&req->report.value[0], report_buf, len);
        ke_msg_send(req);
    }
    else
    {
        TRACE(0, "app_hid_send_report: not ready %d len %d report_idx %d",
            app_hid_env.state, len, reportidx);
    }
}

void app_hid_send_consumer_report(uint8_t keyindx)
{
    uint8_t report_buf[2]={0};
    report_buf[0] = keyindx;
    app_hid_send_report(report_buf,2,0);
    ke_timer_set(APP_HID_RELEASE_TIMER, TASK_APP, 100);
}

#ifdef HOGP_HEAD_TRACKER_PROTOCOL
bt_status_t ble_hogp_send_sensor_report(const struct bt_hid_sensor_report_t *report)
{
    app_hid_env.sensor_report = *report;
    app_hid_send_report((uint8_t *)report, sizeof(struct bt_hid_sensor_report_t), HOGP_REPORT_INDEX_SENSOR_VALUE_INPUT);
    return BT_STS_SUCCESS;
}
#endif

static int app_hid_key_release(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    LOG_I("%s",__func__);
    uint8_t report_buf[2]={HID_KEY_IDX_RELEASE,0};

    app_hid_send_report(report_buf,2,0);
    return (KE_MSG_CONSUMED);
}


/*
 ****************************************************************************************
 * @brief Function called from PS2 driver
 *
 ****************************************************************************************
 */
#if 0
void app_hid_send_mouse_report(struct ps2_mouse_msg report)
{

//    app_display_update_hid_value_screen(report.b, report.x, report.y, report.w);

    switch (app_hid_env.state)
    {
        case (APP_HID_READY):
        {
            // Check if the report can be sent
            if (app_hid_env.num_report)
            {
                // Buffer used to create the Report
                uint8_t report_buff[APP_HID_MOUSE_REPORT_LEN];
                // X, Y and wheel relative movements
                int16_t x;
                int16_t y;

                #if (DISPLAY_SUPPORT)
                // Display all the received information on the LCD screen
                //app_display_update_hid_value_screen(report.b, report.x, report.y, report.w);
                #endif //(DISPLAY_SUPPORT)

                // Clean the report buffer
                memset(&report_buff[0], 0, APP_HID_MOUSE_REPORT_LEN);

                // Set the button states
                report_buff[0] = (report.b & 0x07);

                // If X value is negative
                if (report.b & 0x10)
                {
                    report.x = ~report.x;
                    report.x += 1;
                    x = (int16_t)report.x;
                    x *= (-1);
                }
                else
                {
                    x = (int16_t)report.x;
                }

                // If Y value is negative
                if (report.b & 0x20)
                {
                    report.y = ~report.y;
                    report.y += 1;
                    y = (int16_t)report.y;
                }
                else
                {
                    y = (int16_t)report.y;
                    y *= (-1);
                }


                // Set the X and Y movement value in the report
                co_write16p(&report_buff[1], x);
                co_write16p(&report_buff[3], y);
                report_buff[5] =(signed char) (-1) * report.w;

                // Allocate the HOGPD_REPORT_UPD_REQ message
                struct hogpd_report_upd_req * req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_UPD_REQ,
                                                                  prf_get_task_from_id(TASK_ID_HOGPD),
                                                                  TASK_APP,
                                                                  hogpd_report_upd_req,
                                                                  APP_HID_MOUSE_REPORT_LEN);

                req->conidx  = app_hid_env.conidx;
                //now fill report
                req->report.hid_idx  = app_hid_env.conidx;
                req->report.type     = HOGPD_REPORT; //HOGPD_BOOT_MOUSE_INPUT_REPORT;//
                req->report.idx      = 0; //0 for boot reports and report map
                req->report.length   = APP_HID_MOUSE_REPORT_LEN;
                memcpy(&req->report.value[0], &report_buff[0], APP_HID_MOUSE_REPORT_LEN);

                ke_msg_send(req);

                app_hid_env.num_report--;

                // Restart the mouse timeout timer if needed
                if (app_hid_env.timeout != 0)
                {
                    ke_timer_set(APP_HID_MOUSE_TIMEOUT_TIMER, TASK_APP, (uint16_t)(app_hid_env.timeout));
                    app_hid_env.timer_enabled = true;
                }
            }
        } break;

        case (APP_HID_WAIT_REP):
        {
            // Requested connection parameters
            struct gapc_conn_param conn_param;

            /*
             * Requested connection interval: 10ms
             * Latency: 25
             * Supervision Timeout: 2s
             */
            conn_param.intv_min = 8;
            conn_param.intv_max = 8;
            conn_param.latency  = 25;
            conn_param.time_out = 200;

            app_update_param(&conn_param);

            // Restart the mouse timeout timer if needed
            if (app_hid_env.timeout != 0)
            {
                ke_timer_set(APP_HID_MOUSE_TIMEOUT_TIMER, TASK_APP, (uint16_t)(app_hid_env.timeout));
                app_hid_env.timer_enabled = true;
            }

            // Go back to the ready state
            app_hid_env.state = APP_HID_READY;
        } break;

        case (APP_HID_IDLE):
        {
            // Try to restart advertising if needed
            app_update_adv_state(true);
        } break;

        // DISABLE and ENABLED states
        default:
        {
            // Drop the message
        } break;
    }

    #if (KE_PROFILING)
//    app_display_hdl_env_size(0xFFFF, ke_get_mem_usage(KE_MEM_ENV));
//    app_display_hdl_db_size(0xFFFF, ke_get_mem_usage(KE_MEM_PROFILE));
//    app_display_hdl_msg_size((uint16_t)ke_get_max_mem_usage(), ke_get_mem_usage(KE_MEM_KE_MSG));
    #endif //(KE_PROFILING)
}
#endif

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */


static int hogpd_ctnl_pt_ind_handler(ke_msg_id_t const msgid,
                                     struct hogpd_ctnl_pt_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    TRACE(0, "hogp app ctrl pt ind: connidx %d hid_ctnl_pt %d", param->conidx, param->hid_ctnl_pt);

    if (param->conidx == app_hid_env.conidx)
    {
        //make use of param->hid_ctnl_pt
       struct hogpd_report_write_req_ind  *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_WRITE_CFM,
                                                                   dest_id,
                                                                   src_id,/*  prf_get_task_from_id(TASK_ID_HOGPD)*/
                                                                   hogpd_report_write_req_ind,
                                                                   app_hid_env.report_map_len);

        req->conidx = param->conidx;
        /// HIDS Instance
        req->hid_idx = 0;
        /// type of report (@see enum hogpd_report_type)
        req->report_type = 1;//outside 
        /// Report Length (uint8_t)
        req->report.length = 0;
        /// Report Instance - 0 for boot reports and report map
        req->report_idx = 0;
        /// Report data
        // Send the message
        ke_msg_send(req);
    }
    return (KE_MSG_CONSUMED);
}

static int hogpd_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                     struct hogpd_ntf_cfg_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    TRACE(0, "hogp app ntf cfg ind: conidx %d ntf_cfg %d", param->conidx, param->ntf_cfg[param->conidx]);

    if (app_hid_env.conidx == param->conidx)
    {
        if ((param->ntf_cfg[param->conidx] & HOGPD_CFG_REPORT_NTF_EN ) != 0)
        {
            // The device is ready to send reports to the peer device
            app_hid_env.state = APP_HID_READY;
        }
        else
        {
            // Come back to the Enabled state
            if (app_hid_env.state == APP_HID_READY)
            {
                app_hid_env.state = APP_HID_ENABLED;
            }
        }
        #if (NVDS_SUPPORT)
        // Store the notification configuration in the database
        if (nvds_put(NVDS_TAG_MOUSE_NTF_CFG, NVDS_LEN_MOUSE_NTF_CFG,
                     (uint8_t *)&param->ntf_cfg[param->conidx]) != NVDS_OK)
        {
            // Should not happen
            ASSERT_ERR(0);
        }
        #endif
    }

    return (KE_MSG_CONSUMED);
}

//hogpd_report_req_ind
static int hogpd_report_read_ind_handler(ke_msg_id_t const msgid,
                                        struct hogpd_report_read_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    if (ke_state_get(TASK_APP) != APPM_ENCRYPTED)
    {
        TRACE(0, "hogp app read req handler: not encrypted %d", ke_state_get(TASK_APP));
        return KE_MSG_SAVED;
    }

    TRACE(0, "hogp app read req handler: conidx %d token %d type %d report_idx %d",
        param->conidx, param->token, param->report_type, param->report_idx);

    if (param->report_type == HOGPD_REPORT_MAP)
    {
        struct hogpd_report_read_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_READ_CFM,
                                                             src_id,  /// prf_get_task_from_id(TASK_ID_HOGPD),/* src_id */
                                                             dest_id, /// TASK_APP,
                                                             hogpd_report_read_cfm,
                                                             app_hid_env.report_map_len);
        req->conidx = app_hid_env.conidx;
        req->status = GAP_ERR_NO_ERROR;
        req->token = param->token;
        req->tot_length = app_hid_env.report_map_len + 2;
        req->report.length = app_hid_env.report_map_len;
        memcpy(&req->report.value[0], &app_hid_env.report_map[0], app_hid_env.report_map_len);

        ke_msg_send(req);
    }
    else if (param->report_type == HOGPD_REPORT)
    {
#ifdef HOGP_HEAD_TRACKER_PROTOCOL
        if (param->report_idx == HOGP_REPORT_INDEX_SENSOR_DESCR_FEATURE)
        {
            uint8_t report_data_len = 23 + 10 + 6;
            struct bdaddr_t remote = {{0}};
            struct hogpd_report_read_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_READ_CFM,
                                                             src_id,
                                                             dest_id,
                                                             hogpd_report_read_cfm,
                                                             report_data_len);
            req->conidx = app_hid_env.conidx;
            req->status = GAP_ERR_NO_ERROR;
            req->token = param->token;
            req->tot_length = report_data_len + 2;
            req->report.length = report_data_len;

            uint8_t *dest_remote = req->report.value+23+10;
            app_bt_get_local_device_address(&remote);
            memcpy(req->report.value, "#AndroidHeadTracker#1.0", 23);
            memcpy(req->report.value+23, "\0\0\0\0\0\0\0\0BT", 10);
            dest_remote[0] = remote.address[5]; // copy bd_addr to report from MSB to LSB
            dest_remote[1] = remote.address[4];
            dest_remote[2] = remote.address[3];
            dest_remote[3] = remote.address[2];
            dest_remote[4] = remote.address[1];
            dest_remote[5] = remote.address[0];

            ke_msg_send(req);
        }
        else if (param->report_idx == HOGP_REPORT_INDEX_SENSOR_STATE_FEATURE)
        {
            uint8_t report_data_len = 1;
            struct hogpd_report_read_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_READ_CFM,
                                                             src_id,
                                                             dest_id,
                                                             hogpd_report_read_cfm,
                                                             report_data_len);
            req->conidx = app_hid_env.conidx;
            req->status = GAP_ERR_NO_ERROR;
            req->token = param->token;
            req->tot_length = report_data_len + 2;
            req->report.length = report_data_len;
            req->report.value[0] = app_hid_env.sensor_state_feature;

            ke_msg_send(req);
        }
        else if (param->report_idx == HOGP_REPORT_INDEX_SENSOR_VALUE_INPUT)
        {
            uint8_t report_data_len = sizeof(struct bt_hid_sensor_report_t);
            struct hogpd_report_read_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_READ_CFM,
                                                             src_id,
                                                             dest_id,
                                                             hogpd_report_read_cfm,
                                                             report_data_len);
            req->conidx = app_hid_env.conidx;
            req->status = GAP_ERR_NO_ERROR;
            req->token = param->token;
            req->tot_length = report_data_len + 2;
            req->report.length = report_data_len;
            memcpy(&req->report.value[0], &app_hid_env.sensor_report, report_data_len);

            ke_msg_send(req);
        }
        else
#endif
        {
            struct hogpd_report_read_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_READ_CFM,
                                                             src_id,
                                                             dest_id,
                                                             hogpd_report_read_cfm,
                                                             0);
            req->conidx = app_hid_env.conidx;
            req->status = GAP_ERR_NO_ERROR;
            req->token = param->token;
            req->tot_length = 2;
            req->report.length = 0;

            ke_msg_send(req);
        }
    }
#if 0
    else
    {
        if (param->report.type == HOGPD_BOOT_MOUSE_INPUT_REPORT)
        { //request of boot mouse report
            struct hogpd_report_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_CFM,
                                                            prf_get_task_from_id(TASK_ID_HOGPD),/* src_id */
                                                            TASK_APP,
                                                            hogpd_report_cfm,
                                                            0/*param->report.length*/);

            req->conidx = param->conidx; ///app_hid_env.conidx; ///???
            /// Operation requested (read/write @see enum hogpd_op)
            req->operation = HOGPD_OP_REPORT_READ;
            /// Status of the request
            req->status = GAP_ERR_NO_ERROR;  ///???
            /// HIDS Instance
            req->report.hid_idx = app_hid_env.conidx; ///???
            /// type of report (@see enum hogpd_report_type)
            req->report.type = param->report.type;//-1;//outside 
            /// Report Length (uint8_t)
            req->report.length = 0; //param->report.length;
            /// Report Instance - 0 for boot reports and report map
            req->report.idx = param->report.idx; //0;
            /// Report data

            // Send the message
            ke_msg_send(req);
        }
        else if (param->report.type == HOGPD_REPORT)
        { //request of mouse report
            struct hogpd_report_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_CFM,
                                                            prf_get_task_from_id(TASK_ID_HOGPD),/* src_id */
                                                            TASK_APP,
                                                            hogpd_report_cfm,
                                                            8/*param->report.length*/);

            req->conidx = param->conidx; ///app_hid_env.conidx; ///???
            /// Operation requested (read/write @see enum hogpd_op)
            req->operation = HOGPD_OP_REPORT_READ;
            /// Status of the request
            req->status = GAP_ERR_NO_ERROR;  ///???
            /// Report Info
            //req->report;
            /// HIDS Instance
            req->report.hid_idx = app_hid_env.conidx; ///???
            /// type of report (@see enum hogpd_report_type)
            req->report.type = param->report.type;//-1;//outside 
            /// Report Length (uint8_t)
            req->report.length = 8; //param->report.length;
            /// Report Instance - 0 for boot reports and report map
            req->report.idx = param->report.idx; //0;
            /// Report data
            memset(&req->report.value[0], 0, 8); //???
            req->report.value[0] = param->report.hid_idx;    /// HIDS Instance
            req->report.value[1] = param->report.type;    /// type of report (@see enum hogpd_report_type)
            req->report.value[2] = param->report.length;    /// Report Length (uint8_t)
            req->report.value[3] = param->report.idx;    /// Report Instance - 0 for boot reports and report map

            // Send the message
            ke_msg_send(req);
        }
        else
        {
            struct hogpd_report_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_REPORT_CFM,
                                                            prf_get_task_from_id(TASK_ID_HOGPD),/* src_id */
                                                            TASK_APP,
                                                            hogpd_report_cfm,
                                                            8/*param->report.length*/);

            req->conidx = param->conidx; ///app_hid_env.conidx; ///???
            /// Operation requested (read/write @see enum hogpd_op)
            req->operation = HOGPD_OP_REPORT_READ;
            /// Status of the request
            req->status = GAP_ERR_NO_ERROR;  ///???
            /// Report Info
            //req->report;
            /// HIDS Instance
            req->report.hid_idx = app_hid_env.conidx; ///???
            /// type of report (@see enum hogpd_report_type)
            req->report.type = param->report.type;//-1;//outside 
            /// Report Length (uint8_t)
            req->report.length = 8; //param->report.length;
            /// Report Instance - 0 for boot reports and report map
            req->report.idx = param->report.idx; //0;
            /// Report data
            memset(&req->report.value[0], 0, 8); //???
            req->report.value[0] = param->report.hid_idx;    /// HIDS Instance
            req->report.value[1] = param->report.type;    /// type of report (@see enum hogpd_report_type)
            req->report.value[2] = param->report.length;    /// Report Length (uint8_t)
            req->report.value[3] = param->report.idx;    /// Report Instance - 0 for boot reports and report map

            // Send the message
            ke_msg_send(req);
        }
        
    }
    #endif
    #if defined(HID_VOL_TEST)
    ke_timer_set(APP_HID_MOUSE_TIMEOUT_TIMER, TASK_APP, 1000);
    app_hid_env.timer_enabled = true;
    LOG_I("%s","start timer");
    #endif
    return (KE_MSG_CONSUMED);
}

static int hogpd_report_write_ind_handler(ke_msg_id_t const msgid,
                                        struct hogpd_report_write_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    struct hogpd_report_write_cfm *write_cfm = NULL;

    if (ke_state_get(TASK_APP) != APPM_ENCRYPTED)
    {
        TRACE(0, "hogp app write req handler: not encrypted %d", ke_state_get(TASK_APP));
        return KE_MSG_SAVED;
    }

    TRACE(0, "hogp app write req handler: conidx %d token %d type %d report_idx %d len %d",
        param->conidx, param->token, param->report_type, param->report_idx, param->report.length);

#ifdef HOGP_HEAD_TRACKER_PROTOCOL
    if (param->report_idx == HOGP_REPORT_INDEX_SENSOR_STATE_FEATURE && param->report.length)
    {
        uint8_t sensor_state = param->report.value[0];
        uint8_t sensor_power_state = (sensor_state & 0x02) ? true : false;
        uint8_t sensor_reporting_state = (sensor_state & 0x01) ? true : false;
        uint8_t sensor_interval = 10 + ((sensor_state & 0xfc) >> 2);

        app_hid_env.sensor_state_feature = sensor_state;

        TRACE(0, "hogp app write req handler: sesnor power %d reporting %d interval %d ms",
            sensor_power_state, sensor_reporting_state, sensor_interval);
    }
#endif

    write_cfm = KE_MSG_ALLOC_DYN(HOGPD_REPORT_READ_CFM, src_id, dest_id, hogpd_report_write_cfm, 0);
    write_cfm->conidx = app_hid_env.conidx;
    write_cfm->status = GAP_ERR_NO_ERROR;
    write_cfm->token = param->token;

    ke_msg_send(write_cfm);
    return (KE_MSG_CONSUMED);
};

static int hogpd_proto_mode_req_ind_handler(ke_msg_id_t const msgid,
                                        struct hogpd_proto_mode_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    TRACE(0, "hogp app proto mode req: condix %d mode %d", param->conidx, param->proto_mode);

    if ((param->conidx == app_hid_env.conidx))//&& (param->operation == HOGPD_OP_PROT_UPDATE))
    {

        //make use of param->proto_mode
        struct hogpd_proto_mode_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_PROTO_MODE_CFM,
                                                        KE_BUILD_ID(prf_get_task_from_id(TASK_ID_HOGPD),app_hid_env.conidx),/* src_id */
                                                        TASK_APP,
                                                        hogpd_proto_mode_cfm,
                                                        0);
        /// Connection Index
        req->conidx = app_hid_env.conidx;
        /// Status of the request
        req->status = GAP_ERR_NO_ERROR;
        /// HIDS Instance
        req->hid_idx = app_hid_env.conidx;
        /// New Protocol Mode Characteristic Value
        req->proto_mode = param->proto_mode;
        // Send the message
        ke_msg_send(req);
    }
    else
    {
        struct hogpd_proto_mode_cfm *req = KE_MSG_ALLOC_DYN(HOGPD_PROTO_MODE_CFM,
                                                        KE_BUILD_ID(prf_get_task_from_id(TASK_ID_HOGPD),app_hid_env.conidx),/* src_id */
                                                        TASK_APP,
                                                        hogpd_proto_mode_cfm,
                                                        0);
        /// Status of the request
        req->status = ATT_ERR_APP_ERROR;

        /// Connection Index
        req->conidx = app_hid_env.conidx;
        /// HIDS Instance
        req->hid_idx = app_hid_env.conidx;
        /// New Protocol Mode Characteristic Value
        req->proto_mode = param->proto_mode;
        // Send the message
        ke_msg_send(req);
    }
    return (KE_MSG_CONSUMED);
}


static int hogpd_report_upd_handler(ke_msg_id_t const msgid,
                                   struct hogpd_report_upd_rsp const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    LOG_I("%s app_hid_env.conidx:%x param->conidx:%x,param->status:%x",__func__,
                                        app_hid_env.conidx,
                                        param->conidx,param->status);
    if (app_hid_env.conidx == param->conidx)
    {
        if (GAP_ERR_NO_ERROR == param->status)
        {
            if (app_hid_env.num_report < APP_HID_NB_SEND_REPORT)
            {
                app_hid_env.num_report++;
            }
        }
        else
        {
            // we get this message if error occur while sending report
            // most likely - disconnect
            // Go back to the ready state
            app_hid_env.state = APP_HID_IDLE;
            // change mode
            // restart adv
            // Try to restart advertising if needed
            // appm_update_actv_state(app_hid_env.conidx,true);

            //report was not success - need to restart???
        }
    }
    return (KE_MSG_CONSUMED);
}

static int hogpd_enable_rsp_handler(ke_msg_id_t const msgid,
                                     struct hogpd_enable_rsp const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    LOG_I("%s,conidx:%x,status:%x",__func__,param->conidx,param->status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Function called when the APP_HID_MOUSE_TIMEOUT_TIMER expires.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_hid_mouse_timeout_timer_handler(ke_msg_id_t const msgid,
                                               void const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    LOG_I("%s", __func__);
    app_hid_env.timer_enabled = false;
    
    #if defined(HID_VOL_TEST)
    LOG_I("key_define:%x",key_define);
    app_hid_send_consumer_report(key_define);
    key_define++;
    if(key_define > HID_KEY_IDX_POWER)
    {
       key_define = HID_KEY_IDX_PP;
    }
    // Relaunch the timer
    ke_timer_set(APP_HID_MOUSE_TIMEOUT_TIMER, TASK_APP, 2000);
    app_hid_env.timer_enabled = true;
    #endif
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_hid_msg_dflt_handler(ke_msg_id_t const msgid,
                                    void const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Drop the message
    LOG_I("%s",__func__);
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Set the value of the Report Map Characteristic in the database
 ****************************************************************************************
 */
void app_hid_set_report_map(void);

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_hid_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_hid_msg_dflt_handler},
    {HOGPD_ENABLE_RSP,              (ke_msg_func_t)hogpd_enable_rsp_handler},
    {HOGPD_NTF_CFG_IND,             (ke_msg_func_t)hogpd_ntf_cfg_ind_handler},
    {HOGPD_REPORT_READ_REQ_IND,     (ke_msg_func_t)hogpd_report_read_ind_handler},
    {HOGPD_REPORT_WRITE_REQ_IND,    (ke_msg_func_t)hogpd_report_write_ind_handler},
    {HOGPD_PROTO_MODE_REQ_IND,      (ke_msg_func_t)hogpd_proto_mode_req_ind_handler},
    {HOGPD_CTNL_PT_IND,             (ke_msg_func_t)hogpd_ctnl_pt_ind_handler},
    {HOGPD_REPORT_UPD_RSP,          (ke_msg_func_t)hogpd_report_upd_handler},
    {APP_HID_RELEASE_TIMER,         (ke_msg_func_t)app_hid_key_release},
    {APP_HID_MOUSE_TIMEOUT_TIMER,   (ke_msg_func_t)app_hid_mouse_timeout_timer_handler},
};

const struct app_subtask_handlers app_hid_handlers = APP_HANDLERS(app_hid);

#endif //(BLE_APP_HID)

/// @} APP

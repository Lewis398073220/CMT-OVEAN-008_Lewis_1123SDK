#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "pts_sm.h"
#include "nvrecord_extension.h"
#include "nvrecord_ble.h"

enum{
    SEC_ATT_SVC,
    SEC_ATT_CHAR,
    SEC_ATT_VAL,
    SEC_ATT_NUM,
};

static uint8_t gatt_info = 0b00000101;

struct gatt_att_desc pts_sec_att[SEC_ATT_NUM] =
{
    /// Service Declaration
    [SEC_ATT_SVC] = {{0x00, 0x28},PROP(RD), 0},

    /// Characteristic Declaration
    [SEC_ATT_CHAR] = {{0x03, 0x28}, PROP(RD), 0},
    /// Characteristic Value
    [SEC_ATT_VAL] = {{0xC7, 0x2B}, PROP(RD)|PROP(WC)|PROP(WR), GATT_ATT_WRITE_MAX_SIZE_MASK},
};

static void pts_sec_add_svc_cb(const void *data)
{
    uint16_t uuid = 0x5566;
    pts_gatt_svc_add(gatt_info, (uint8_t *)&uuid, SEC_ATT_NUM, SEC_ATT_NUM, pts_sec_att);
}

static void pts_sec_reg_gatt(uint8_t role)
{
    //reg gatt and add svc db
    osTimerId TimerID;
    osTimerDef (PTS_SEC_TIMER, pts_sec_add_svc_cb);

    pts_gatt_reg(role);
    if(role == PTS_GATT_ROLE_SERVER)
    {
        TimerID = osTimerCreate(osTimer(PTS_SEC_TIMER), false, NULL);
        osTimerStart(TimerID, 10);
    }
}

static void pts_sec_start_adv()
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->adv_data.len = adv_para->adv_data.data[0] + 1;

    pts_adv_start();  
}

static void pts_gap_sec_sem_bv_11_c(char *para_buf, uint32_t str_len)
{
    gatt_info = 0b00000001;
    //set pairing para
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 9);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);


    //reg gatt svc
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //start ADV
    pts_sec_start_adv();
}

static void pts_gap_sec_sem_bv_21_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    uint16_t gatt_read_handle = 0x0009;
    //int gatt_read_handle;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else if(steps == 3){
        pts_sm_send_security_req(0, 8);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }

}


static void pts_gap_sec_sem_bv_22_c(char *para_buf, uint32_t str_len)
{
    //reg gatt svc
    gatt_info = 0b00000010;
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //start ADV
    pts_sec_start_adv();
}

static void pts_gap_sec_sem_bv_23_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    uint16_t gatt_read_handle = 0x0009;

    //int gatt_read_handle;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else if(steps == 3){
        pts_sm_send_security_req(0, 8);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

//need close check pairing security reached
// gapc_le_smp_central_pairing_start_proc_transition
// //smp_status = gapc_le_check_pairing_security_reached(p_proc);
static void pts_gap_sec_sem_bv_24_c(char *para_buf, uint32_t str_len)
{
    //reg gatt svc
    gatt_info = 0b00000001;
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //start ADV
    pts_sec_start_adv();
}

// need close check pairing security reached
// gapc_le_smp_periph_pairing_start_proc_transition
// smp_status = gapc_le_check_pairing_security_reached(p_proc);
static void pts_gap_sec_sem_bv_29_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,", &steps);
    }

    if(steps == 1){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 0);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);


                //reg gatt svc
        gatt_info = 0b00000001;
        pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

        //connect dongle
        pts_conn_start();
    }else if(steps == 2){
        uint8_t* deviceSecurityInfo = NULL;

        deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(pts_get_peer_addr());
        if (deviceSecurityInfo)
        {
            pts_sm_send_encrypt_req(0, deviceSecurityInfo);
        }
    }
}

static void pts_gap_sec_sem_bv_37_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    uint16_t gatt_read_handle = 0x0009;

    //int gatt_read_handle;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else if(steps == 3){
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL,GAP_SEC1_NOAUTH_PAIR_ENC);
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 9);
        pts_sm_send_security_req(0, 9);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }

}

static void pts_gap_sec_sem_bv_38_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    uint16_t gatt_read_handle = 0x0009;

    //int gatt_read_handle;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else if(steps == 3){
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL,GAP_SEC1_AUTH_PAIR_ENC);
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, GAP_AUTH_REQ_SEC_CON_BOND);
        pts_sm_send_security_req(0, GAP_AUTH_REQ_SEC_CON_BOND);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }

}


static void pts_gap_sec_sem_bv_39_c(char *para_buf, uint32_t str_len)
{
    //set pairing para
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 9);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 1);


    //reg gatt svc
    gatt_info = 0b00000001;
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //start ADV
    pts_sec_start_adv();
}

static void pts_gap_sec_sem_bv_40_c(char *para_buf, uint32_t str_len)
{
    //reg gatt svc
    gatt_info = 0b00000011;
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //start ADV
    pts_sec_start_adv();

}

static void pts_gap_sec_sem_bv_41_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int gatt_read_handle;
    
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    if(steps == 1){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);
        
        //connect dongle
        pts_conn_start();
    }else if(steps == 2){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 9);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 1);

        pts_sm_pair_req();
    }else if(steps == 3){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);  
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}


static void pts_gap_sec_sem_bv_42_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int gatt_read_handle;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    if((steps == 1) || (steps == 3)){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //connect dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_pair_req();
    }else if(steps == 4){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_sec_sem_bv_43_c(char *para_buf, uint32_t str_len)
{
    //set pairing para
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 9);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 1);


    //reg gatt svc
    gatt_info = 0b00000001;
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);
    
    //connect dongle
    pts_conn_start();
}


static void pts_gap_sec_sem_bv_44_c(char *para_buf, uint32_t str_len)
{
    //reg gatt svc
    gatt_info = 0b00000001;
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //connect dongle
    pts_conn_start();
}

static void pts_gap_sec_sem_bv_45_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int gatt_read_handle;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    if((steps == 1) || (steps == 3)){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //connect dongle
        pts_conn_start();
    }else if((steps == 2) || (steps == 5)){
        //set pairing para
         pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
         pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
         pts_sm_pair_req();
    }else if(steps == 4){  //click PTS "Cancel"
    /*
        uint8_t* deviceSecurityInfo = NULL;
        
        deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(pts_get_peer_addr());
        if (deviceSecurityInfo)
        {
            pts_sm_send_encrypt_req(0, deviceSecurityInfo);
        }

    */
    }else if(steps == 6){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }
}

static void pts_gap_sec_aut_bv_11_c(char *para_buf, uint32_t str_len)
{
    //set pairing para
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

    //reg gatt svc
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //start ADV
    pts_sec_start_adv();
}

static void pts_gap_sec_aut_bv_12_c(char *para_buf, uint32_t str_len)
{
    //set pairing para
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 0);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

    //reg gatt svc
    pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

    //connect dongle
    pts_conn_start();
}

static void pts_gap_sec_aut_bv_13_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_sm_set_pairing_para(PTS_PAIRING_NV_GET_KEY, false);

        //reg gatt svc
        gatt_info = 0b00000010;
        pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

        //connect dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 5);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 2);
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_sec_aut_bv_14_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_sm_set_pairing_para(PTS_PAIRING_NV_GET_KEY, false); 

        //reg gatt svc
        gatt_info = 0b00000010;
        pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 5);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 2);
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_sec_aut_bv_17_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int gatt_read_handle = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    if(steps == 1){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);
        //connect dongle
        pts_conn_start();
    }else if((steps == 2) || (steps == 4)){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else if(steps == 3){
        //set pairing para;
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_sm_pair_req();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_sec_aut_bv_18_c(char *para_buf, uint32_t str_len)
{


}

//NOte:Windows PTS software cleanup key
static void pts_gap_sec_aut_bv_19_c(char *para_buf, uint32_t str_len)
{
    int steps = 0, gatt_read_handle = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    switch(steps){
        case 1:
            {
                //reg gatt svc
                pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);
                //connect dongle
                pts_conn_start();
                break;
            }
        case 2:
            {
                 //set pairing para
                 pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
                 pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);


                 pts_sm_pair_req();
                 break;
            }
        case 3:
            {
                pts_conn_start();
                break;
            }
        case 4:
        case 6:
            {
                pts_gatt_cli_read(gatt_read_handle, 0, 1);
                break;
            }
        case 5:
            {
                 pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
                 pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 1);

                 pts_sm_pair_req();
                 break;
            }
        case 7:
            {
                pts_disconnect();
                break;
            }
        default:
            {
                TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
                break;
            }
    }
}

static void pts_gap_sec_aut_bv_20_c(char *para_buf, uint32_t str_len)
{


}

static void pts_gap_sec_aut_bv_21_c(char *para_buf, uint32_t str_len)
{


}

static void pts_gap_sec_aut_bv_22_c(char *para_buf, uint32_t str_len)
{


}

static void pts_gap_sec_aut_bv_23_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        uint8_t* deviceSecurityInfo = NULL;

        deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(pts_get_peer_addr());
        if (deviceSecurityInfo)
        {
            pts_sm_send_encrypt_req(0, deviceSecurityInfo);
        }
    }

}

static void pts_gap_sec_aut_bv_24_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if((steps == 1) ||(steps == 3)){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_SERVER);

        //start ADV
        pts_conn_start();
    }else if(steps == 2){
        pts_disconnect();
    }else if(steps == 4){
        uint8_t* deviceSecurityInfo = NULL;
        
        deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(pts_get_peer_addr());
        if (deviceSecurityInfo)
        {
            pts_sm_send_encrypt_req(0, deviceSecurityInfo);
        }
    }
}

static void pts_gap_sec_aut_bv_25_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int gatt_read_handle = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    if((steps == 1)||(steps == 3)){
        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //start ADV
        pts_conn_start();
    }else if((steps == 2) ||(steps == 5)){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        pts_sm_pair_req();
    }else if((steps == 4) || (steps == 6)){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else {
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_sec_aut_bv_26_28_c(char *para_buf, uint32_t str_len)
{
    int steps = 0, gatt_read_handle = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    switch(steps){
        case 1:
            {
                //reg gatt svc
                pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);
                //start ADV
                pts_sec_start_adv();

                break;
            }
        case 2:
            {
                //set pairing para
                pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
                pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
                pts_sm_set_pairing_para(PTS_PAIRING_NV_GET_KEY, false);
                //send security req
                pts_sm_send_security_req(0, GAP_AUTH_REQ_NO_MITM_BOND);
                break;
            }
        case 3:
        case 4:
            {
                pts_gatt_cli_read(gatt_read_handle, 0, 1);
                break;
            }
        default:
            {
                TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
                break;
            }
    }
}

static void pts_gap_sec_aut_bv_27_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int gatt_read_handle = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x", &steps, &gatt_read_handle);
    }

    if(steps == 1){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        //reg gatt svc
        pts_sec_reg_gatt(PTS_GATT_ROLE_CLIENT);

        //start ADV
        pts_sec_start_adv();
    }else if(steps == 2){
        pts_disconnect();
    }else if(steps == 3){
        pts_gatt_cli_read(gatt_read_handle, 0, 1);
    }else if(steps == 4){
        uint8_t* deviceSecurityInfo = NULL;

        deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(pts_get_peer_addr());
        if (deviceSecurityInfo)
        {
            pts_sm_send_encrypt_req(0, deviceSecurityInfo);
        }
    }else {
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

static pts_case_num_t sec_sem_bv_num[] = {
{11, pts_gap_sec_sem_bv_11_c},
{21, pts_gap_sec_sem_bv_21_c},
{22, pts_gap_sec_sem_bv_22_c},
{23, pts_gap_sec_sem_bv_23_c},
{24, pts_gap_sec_sem_bv_24_c},
{29, pts_gap_sec_sem_bv_29_c},
{37, pts_gap_sec_sem_bv_37_c},
{38, pts_gap_sec_sem_bv_38_c},
{39, pts_gap_sec_sem_bv_39_c},
{40, pts_gap_sec_sem_bv_40_c},
{41, pts_gap_sec_sem_bv_41_c},
{42, pts_gap_sec_sem_bv_42_c},
{43, pts_gap_sec_sem_bv_43_c},
{44, pts_gap_sec_sem_bv_44_c},
{45, pts_gap_sec_sem_bv_45_c},
{0xFF, NULL},
};

static pts_case_num_t sec_aut_bv_num[] = {
{11, pts_gap_sec_aut_bv_11_c},
{12, pts_gap_sec_aut_bv_12_c},
{13, pts_gap_sec_aut_bv_13_c},
{14, pts_gap_sec_aut_bv_14_c},
{17, pts_gap_sec_aut_bv_17_c},
{18, pts_gap_sec_aut_bv_18_c},
{19, pts_gap_sec_aut_bv_19_c},
{20, pts_gap_sec_aut_bv_20_c},
{21, pts_gap_sec_aut_bv_21_c},
{22, pts_gap_sec_aut_bv_22_c},
{23, pts_gap_sec_aut_bv_23_c},
{24, pts_gap_sec_aut_bv_24_c},
{25, pts_gap_sec_aut_bv_25_c},
{26, pts_gap_sec_aut_bv_26_28_c},
{27, pts_gap_sec_aut_bv_27_c},
{28, pts_gap_sec_aut_bv_26_28_c},
{0xFF, NULL},
};

pts_module_case_t pts_sec_case[] = {
{"SEM", sec_sem_bv_num, NULL},
{"AUT", sec_aut_bv_num, NULL},
{"CSIGN", NULL, NULL},
{NULL, NULL, NULL},
};



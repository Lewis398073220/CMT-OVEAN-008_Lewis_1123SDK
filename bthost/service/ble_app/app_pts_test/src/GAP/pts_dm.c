#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "nvrecord_extension.h"
#include "nvrecord_ble.h"


static void pts_dm_start_adv()
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->adv_data.len = adv_para->adv_data.data[0] + 1;

    pts_adv_start();  

}

static void pts_gap_dm_ncon_bv_01_c(char *para_buf, uint32_t str_len)
{

    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK;

    pts_adv_start();
}

static void pts_gap_dm_con_bv_01_c(char *para_buf, uint32_t str_len)
{  
    pts_adv_start();
}

static void pts_gap_dm_bon_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        //classic BT send conn req
        //cmd: best_test:create_service_link
        
    }
    else if(steps == 2){
        //classic BT send disconn req
        //cmd: best_test:bt_pts_disc_acl_link
    }
    else if((steps == 3) ||(steps == 4)){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, GAP_SEC1_NOAUTH_PAIR_ENC);
        pts_sm_set_pairing_para(PTS_PAIRING_NV_GET_KEY, false);

        pts_conn_start();
    }
    else if(steps == 5){
        uint8_t* deviceSecurityInfo = NULL;
        
        deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(pts_get_peer_addr());
        if (deviceSecurityInfo)
        {
            pts_sm_send_encrypt_req(0, deviceSecurityInfo);
        }
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_dm_nad_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        pts_conn_start();
    }
    else if(steps == 2){
        pts_get_peer_dev_info(GAPC_GET_PEER_NAME);
    }
    else if(steps == 3){
        pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static void pts_gap_dm_lep_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    pts_dm_start_adv();
}

static void pts_gap_dm_lep_bv_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

}

static void pts_gap_dm_lep_bv_06_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

}

static void pts_gap_dm_lep_bv_07_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    pts_dm_start_adv();
}
static void pts_gap_dm_lep_bv_08_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_dm_start_adv();
}
static void pts_gap_dm_lep_bv_09_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(steps == 1){
        pts_conn_start();
    }
    else if(steps == 2){
        //classic BT send link req
        //cmd: best_test:create_service_link
    }
    else if(steps == 3){
        pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}


static void pts_gap_dm_lep_bv_11_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        pts_conn_start();
    }
    else if(steps == 2){
        pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static pts_case_num_t dm_ncon_bv_num[] = {
{1,  pts_gap_dm_ncon_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t dm_con_bv_num[] = {
{1,  pts_gap_dm_con_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t dm_bon_bv_num[] = {
{1,  pts_gap_dm_bon_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t dm_nad_bv_num[] = {
{2,  pts_gap_dm_nad_bv_02_c},
{0xFF, NULL},
};


static pts_case_num_t dm_lep_bv_num[] = {
{1,  pts_gap_dm_lep_bv_01_c},
{4,  pts_gap_dm_lep_bv_04_c},
{6,  pts_gap_dm_lep_bv_06_c},
{7,  pts_gap_dm_lep_bv_07_c},
{8,  pts_gap_dm_lep_bv_08_c},
{9,  pts_gap_dm_lep_bv_09_c},
{11, pts_gap_dm_lep_bv_11_c},
{0xFF, NULL},
};

pts_module_case_t pts_dm_case[] = {
{"NCON", dm_ncon_bv_num, NULL},
{"CON",  dm_con_bv_num, NULL},
{"BON",  dm_bon_bv_num, NULL},
{"NAD",  dm_nad_bv_num, NULL},
{"LEP",  dm_lep_bv_num, NULL},
{NULL, NULL, NULL},
};

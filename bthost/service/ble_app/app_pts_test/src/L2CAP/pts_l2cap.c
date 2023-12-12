#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

static void pts_l2cap_start_adv()
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->adv_data.len = adv_para->adv_data.data[0] + 1;

    pts_adv_start();  
}

static void pts_l2cap_le_cpu_bv01_bi02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    
    if(steps == 1){
        pts_l2cap_start_adv();
     }
     else if (steps == 2){
        struct gapc_conn_param conn_para;

        conn_para.intv_min = 50;
        conn_para.intv_max = 100;
        conn_para.time_out = 2000;
        conn_para.latency = 0;

        pts_update_param(0, &conn_para);
     }
     else{
         TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
     }
}

static void pts_l2cap_le_cpu_bv02_bi01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    
    if(steps == 1){
        pts_conn_start();
     }
     else if (steps == 2){
        pts_disconnect();
     }
     else{
         TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
     }
}


static void pts_l2cap_le_rej_bi_01_c(char *para_buf, uint32_t str_len)
{
    pts_l2cap_start_adv();
}

static void pts_l2cap_le_cid_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        pts_l2cap_start_adv();
    }
    else if (steps == 2){ //creat L2CAP
        pts_l2cap_channel_creat(0x90);
    }
    else{
         TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}


static pts_case_num_t le_cpu_bv_num[] = {
{1, pts_l2cap_le_cpu_bv01_bi02_c},
{2, pts_l2cap_le_cpu_bv02_bi01_c},
{0xFF, NULL},
};

static pts_case_num_t le_cpu_bi_num[] = {
{1, pts_l2cap_le_cpu_bv02_bi01_c},
{2, pts_l2cap_le_cpu_bv01_bi02_c},
{0xFF, NULL},
};

static pts_case_num_t le_rej_bi_num[] = {
{1, pts_l2cap_le_rej_bi_01_c},
{0xFF, NULL},
};

static pts_case_num_t le_cid_bi_num[] = {
{1, pts_l2cap_le_cid_bi_01_c},
{0xFF, NULL},
};

pts_module_case_t pts_le_case[] = {
{"CPU", le_cpu_bv_num, le_cpu_bi_num},
{"REJ", NULL, le_rej_bi_num},
{"CID", NULL, le_cid_bi_num},
{NULL, NULL, NULL},
};

pts_sub_module_t pts_l2cap_sub_modele[] = {
{"LE",  pts_le_case},
{NULL, NULL}
};

void pts_l2cap_module_reg()
{
    ble_host_pts_module_reg("L2CAP", pts_l2cap_sub_modele);
}


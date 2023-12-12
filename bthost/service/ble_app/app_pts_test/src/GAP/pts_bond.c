#include <stdio.h>
#include <cmsis_os.h>
#include "gap.h"
#include "gapm_le.h"
#include "gapc_msg.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "pts_sm.h"

static void pts_bond_start_adv()
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->adv_data.len = adv_para->adv_data.data[0] + 1;

    pts_adv_start();
}

//NOTE:The model test erased flash every time it burned
static void pts_gap_bond_nbon_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 0);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_conn_start();
    }
    else if(steps == 2){
        pts_conn_start();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

static void pts_gap_bond_nbon_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 0);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_conn_start();
    }
    else if(steps == 2){
        pts_sm_pair_req();
    }
    else if(steps == 3){
        pts_conn_start();
    }
    else if(steps == 4){
        pts_sm_pair_req();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }


}

static void pts_gap_bond_nbon_bv_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 0);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
    pts_bond_start_adv();

}

static void pts_gap_bond_bon_bv_01_c(char *para_buf, uint32_t str_len)
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

        //start ADV
        pts_bond_start_adv();
    }
    else if(steps == 2){
        pts_sm_send_security_req(0, GAP_AUTH_REQ_NO_MITM_BOND);
    }
    else if(steps == 3){
        //start ADV
        pts_bond_start_adv();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_bond_bon_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if((steps == 1) || (steps == 3)){
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_conn_start();
    }
    else if(steps == 2){
        pts_sm_pair_req();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_bond_bon_bv_03_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        //start ADV
        pts_bond_start_adv();
    }
    else if(steps == 2){
        pts_bond_start_adv();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

static void pts_gap_bond_bon_bv_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if((steps == 1) || (steps == 3)){
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_conn_start();
    }
    else if(steps == 2){
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static pts_case_num_t bond_nbon_bv_num[] = {
{1, pts_gap_bond_nbon_bv_01_c},
{2, pts_gap_bond_nbon_bv_02_c},
{3, pts_gap_bond_nbon_bv_03_c},
{0xFF, NULL},
};

static pts_case_num_t bond_bon_bv_num[] = {
{1, pts_gap_bond_bon_bv_01_c},
{2, pts_gap_bond_bon_bv_02_c},
{3, pts_gap_bond_bon_bv_03_c},
{4, pts_gap_bond_bon_bv_04_c},
{0xFF, NULL},
};

pts_module_case_t pts_bond_case[] = {
{"NBON", bond_nbon_bv_num, NULL},
{"BON", bond_bon_bv_num, NULL},
{NULL, NULL, NULL},
};




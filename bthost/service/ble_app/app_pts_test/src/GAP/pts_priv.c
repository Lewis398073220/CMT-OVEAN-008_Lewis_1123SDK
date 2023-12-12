#include <stdio.h>
#include "gap.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

//gapm_own_addr
void pts_priv_start_adv(uint8_t addr_type)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.own_addr_type = addr_type;

    if(addr_type == GAPM_GEN_RSLV_ADDR)
    {
        adv_para->para.adv_param.peer_addr.addr_type = GAPM_STATIC_ADDR;
        memcpy(adv_para->para.adv_param.peer_addr.addr, dongle_addr.addr, GAP_BD_ADDR_LEN);
    }

    pts_adv_start();
}

static void pts_gap_priv_conn_bv_10_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    //set pairing para
    if(steps == 1){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_priv_start_adv(GAPM_STATIC_ADDR);
    }
    else if((steps == 2) ||(steps == 4)){
        pts_disconnect();
        osDelay(10);
        pts_adv_delete();
    }
    else if(steps == 3){
        pts_priv_start_adv(GAPM_GEN_RSLV_ADDR);
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

static void pts_gap_priv_conn_bv_11_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    
    if(steps == 1){
        pts_conn_para_t *conn_para = NULL;
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        conn_para = pts_conn_get_para_index();
        memcpy(conn_para->para.peer_addr.addr, dongle_addr.addr, 6);
        conn_para->para.peer_addr.addr_type = 0;
        pts_conn_start();
     }
     else if (steps == 2){
        pts_conn_para_t *conn_para = NULL;
        conn_para = pts_conn_get_para_index();
        conn_para->para.peer_addr.addr_type = 1;
        pts_conn_start();
     }
     else{
         TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
     }
}

// PTS Use public ADDR the first time, and initiate a connection the second time using an unresolvable address
static void pts_gap_priv_conn_bv_12_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if((steps == 1) || (steps == 3)){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_priv_start_adv(GAPM_STATIC_ADDR);

    }
    else if((steps == 2) ||(steps == 4)){
        pts_disconnect();
        osDelay(10);
        pts_adv_delete();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static pts_case_num_t priv_conn_bv_num[] = {
{10, pts_gap_priv_conn_bv_10_c},
{11, pts_gap_priv_conn_bv_11_c},
{12, pts_gap_priv_conn_bv_12_c},
{0xFF, NULL},
};

pts_module_case_t pts_priv_case[] = {
{"CONN", priv_conn_bv_num, NULL},
{NULL, NULL, NULL},
};






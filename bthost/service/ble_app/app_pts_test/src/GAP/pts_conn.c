#include <stdio.h>
#include "gap.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "app.h"
#include "gapc_le_msg.h"

#define PTS_ADV_TYPE_NAME "PTS-GAP-6947"


void pts_conn_start_adv(uint8_t addr_type, uint16_t prop)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.own_addr_type = addr_type;
    adv_para->para.adv_param.prop = prop;
    if(GETB(adv_para->para.adv_param.prop, GAPM_ADV_PROP_DIRECTED))
    {
        adv_para->para.adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
    }
    if(addr_type == GAPM_GEN_RSLV_ADDR)
    {
        adv_para->para.adv_param.peer_addr.addr_type = GAPM_STATIC_ADDR;
        memcpy(adv_para->para.adv_param.peer_addr.addr, dongle_addr.addr, GAP_BD_ADDR_LEN);
    }

    pts_adv_start();
}




static void pts_gap_conn_to_priv_addr_dongle(char *para_buf, uint32_t str_len)
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
        pts_disconnect();
    }
    else if (steps == 3){
        pts_conn_para_t *conn_para = NULL;
        conn_para = pts_conn_get_para_index();
        memcpy(conn_para->para.peer_addr.addr, dongle_addr.addr, 6);
        conn_para->para.peer_addr.addr_type = 1;
        pts_conn_start();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static void pts_gap_conn_ncon_bv_01_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK;

    pts_adv_start();
}

static void pts_gap_conn_ncon_bv_02_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK;

    pts_adv_start();
}


static void pts_gap_conn_ncon_bv_03_c(char *para_buf, uint32_t str_len)
{
        struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.len = 3;
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x19;
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK;

    pts_adv_start();
}


static void pts_gap_conn_dcon_bv_01_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.type = GAPM_ADV_TYPE_LEGACY;
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_DIR_CONN_LDC_MASK;
    adv_para->para.adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
    adv_para->para.own_addr_type = GAPM_STATIC_ADDR;
    adv_para->adv_data.len = 3;
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;
    adv_para->para.adv_param.peer_addr = dongle_addr;

    pts_adv_start();
}

static void pts_gap_conn_dcon_bv_04_05_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1)
    {
        pts_conn_start_adv(GAPM_STATIC_ADDR, GAPM_ADV_PROP_UNDIR_CONN_MASK);
    }
    else if((steps == 2) || (steps == 3))
    {
        pts_disconnect();
        osDelay(10);
        pts_adv_delete();
        osDelay(10);
        pts_conn_start_adv(GAPM_GEN_RSLV_ADDR, GAPM_ADV_PROP_DIR_CONN_MASK);
    }
    else
    {
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_conn_ucon_bv_01_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
    adv_para->adv_data.len = 3;
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x18;

    pts_adv_start();
}

static void pts_gap_conn_ucon_bv_02_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
    adv_para->adv_data.len = 3;
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;

    pts_adv_start();

}

static void pts_gap_conn_ucon_bv_03_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
    adv_para->adv_data.len = 3;
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x19;

    pts_adv_start();

}

static void pts_gap_conn_ucon_bv_06_c(char *para_buf, uint32_t str_len)
{

}



static void pts_gap_conn_acep_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}




static void pts_gap_conn_gcep_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_conn_gcep_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static void pts_gap_conn_scep_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}



static void pts_gap_conn_dcep_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_conn_dcep_bv_03_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_conn_cpup_bv_06_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
        //Update l2cap para
        struct gapc_conn_param update_param;

        update_param.intv_min = 0x0032;
        update_param.intv_max = 0x0046;
        update_param.time_out = 0x01F4;
        update_param.latency  = 0x0001;
        pts_update_param(0, &update_param);
    }
    else if(3 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_conn_cpup_bv_08_c(char *para_buf, uint32_t str_len)
{
    struct pts_adv_para *adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
    adv_para->adv_data.len = 3;
    adv_para->adv_data.data[0] = 0x02;
    adv_para->adv_data.data[1] = 0x01;
    adv_para->adv_data.data[2] = 0x1A;

    pts_adv_start();

}

static void pts_gap_conn_cpup_bv_10_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        struct pts_adv_para *adv_para = NULL;

        adv_para = pts_adv_get_para_index();
        adv_para->para.adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
        adv_para->adv_data.len = 3;
        adv_para->adv_data.data[0] = 0x02;
        adv_para->adv_data.data[1] = 0x01;
        adv_para->adv_data.data[2] = 0x1A;

        pts_adv_start();
    }
    else if(2 == steps){
        //Update l2cap para
        struct gapc_conn_param update_param;

        update_param.intv_min = 0x0032;
        update_param.intv_max = 0x0046;
        update_param.time_out = 0x01F4;
        update_param.latency  = 0x0001;

        pts_update_param(0, &update_param);
    }
    else if(3 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static void pts_gap_conn_term_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        struct pts_adv_para *adv_para = NULL;

        adv_para = pts_adv_get_para_index();
        adv_para->para.adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
        adv_para->adv_data.len = 3;
        adv_para->adv_data.data[0] = 0x02;
        adv_para->adv_data.data[1] = 0x01;
        adv_para->adv_data.data[2] = 0x1A;

        pts_adv_start();
    }

    else if(2 == steps){
       //Disconnect the dongle
       pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}


static void pts_gap_conn_prda_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    //set pairing para
    if((steps == 1) || (steps == 3) || (steps == 5)){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_conn_start_adv(GAPM_STATIC_ADDR, GAPM_ADV_PROP_UNDIR_CONN_MASK);
    }
    else if((steps == 2) || (steps == 4) || (steps == 6)){
        pts_disconnect();
        osDelay(10);
        pts_adv_delete();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void priv_scan_report_cb(struct gapm_ext_adv_report_ind *param)
{
    int i;
    for(i=0; i < param->length;)
    {
        if(param->data[i+1] == GAP_AD_TYPE_COMPLETE_NAME)
        {
            DUMP8("%02x ", &param->data[i], param->data[i]+1);
            if(!strncmp((char *)&param->data[i+2], PTS_ADV_TYPE_NAME, strlen(PTS_ADV_TYPE_NAME)))
            {
                pts_conn_para_t *conn_para = NULL;

                pts_scan_disenable();
                conn_para = pts_conn_get_para_index();
                memcpy(conn_para->para.peer_addr.addr, param->trans_addr.addr, 6);
                conn_para->para.peer_addr.addr_type = GAPM_GEN_NON_RSLV_ADDR;
                pts_conn_start();
            }
            DUMP8("%02x ", param->trans_addr.addr, GAP_BD_ADDR_LEN);
            return;
        }
        i += (param->data[i] + 1);
    }
    DUMP8("%02x ", param->data, param->length);
}

//becuase the dongle may has bugs that don't let irkey exchange ,so we use an unexpend way to pass this case.(chinese:zuo bi)
static void pts_gap_conn_prda_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;
    int addr[6] = {0};
    uint8_t char_addr[6] = {0};
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d,%x-%x-%x-%x-%x-%x", &steps, &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);
    }

    char_addr[0] = addr[5];
    char_addr[1] = addr[4];
    char_addr[2] = addr[3];
    char_addr[3] = addr[2];
    char_addr[4] = addr[1];
    char_addr[5] = addr[0];

    if(steps == 1){
        pts_conn_para_t *conn_para = NULL;
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);

        conn_para = pts_conn_get_para_index();
        memcpy(conn_para->para.peer_addr.addr, char_addr, 6);
        conn_para->para.peer_addr.addr_type = GAPM_GEN_NON_RSLV_ADDR;
        pts_conn_start();
     }
     else if(steps == 2){
        pts_scan_start(0, priv_scan_report_cb);
     }
     else if(steps == 3){
        pts_scan_start(0, priv_scan_report_cb);
     }
     else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
     }

}

static pts_case_num_t conn_ncon_bv_num[] = {
{1, pts_gap_conn_ncon_bv_01_c},
{2, pts_gap_conn_ncon_bv_02_c},
{3, pts_gap_conn_ncon_bv_03_c},
{0xFF, NULL},
};

static pts_case_num_t conn_dcon_bv_num[] = {
{1, pts_gap_conn_dcon_bv_01_c},
{4, pts_gap_conn_dcon_bv_04_05_c},
{5, pts_gap_conn_dcon_bv_04_05_c},
{0xFF, NULL},
};


static pts_case_num_t conn_ucon_bv_num[] = {
{1, pts_gap_conn_ucon_bv_01_c},
{2, pts_gap_conn_ucon_bv_02_c},
{3, pts_gap_conn_ucon_bv_03_c},
{6, pts_gap_conn_ucon_bv_06_c},
{0xFF, NULL},
};

static pts_case_num_t conn_acep_bv_num[] = {
{1, pts_gap_conn_acep_bv_01_c},
{3, pts_gap_conn_to_priv_addr_dongle},
{4, pts_gap_conn_to_priv_addr_dongle},
{0xFF, NULL},
};

static pts_case_num_t conn_gcep_bv_num[] = {
{1, pts_gap_conn_gcep_bv_01_c},
{2, pts_gap_conn_gcep_bv_02_c},
{5, pts_gap_conn_to_priv_addr_dongle},
{6, pts_gap_conn_to_priv_addr_dongle},
{0xFF, NULL},
};

static pts_case_num_t conn_scep_bv_num[] = {
{1, pts_gap_conn_scep_bv_01_c},
{3, pts_gap_conn_to_priv_addr_dongle},
{0xFF, NULL},
};

static pts_case_num_t conn_dcep_bv_num[] = {
{1, pts_gap_conn_dcep_bv_01_c},
{3, pts_gap_conn_dcep_bv_03_c},
{5, pts_gap_conn_to_priv_addr_dongle},
{6, pts_gap_conn_to_priv_addr_dongle},
{0xFF, NULL},
};

static pts_case_num_t conn_cpup_bv_num[] = {
{6, pts_gap_conn_cpup_bv_06_c},
{8, pts_gap_conn_cpup_bv_08_c},
{10, pts_gap_conn_cpup_bv_10_c},
{0xFF, NULL},
};

static pts_case_num_t conn_term_bv_num[] = {
{1, pts_gap_conn_term_bv_01_c},
{0xFF, NULL},
};


static pts_case_num_t conn_prda_bv_num[] = {
{1, pts_gap_conn_prda_bv_01_c},
{2, pts_gap_conn_prda_bv_02_c},
{0xFF, NULL},
};

pts_module_case_t pts_conn_case[] = {
{"NCON", conn_ncon_bv_num, NULL},
{"DCON", conn_dcon_bv_num, NULL},
{"UCON", conn_ucon_bv_num, NULL},
{"ACEP", conn_acep_bv_num, NULL},
{"GCEP", conn_gcep_bv_num, NULL},
{"SCEP", conn_scep_bv_num, NULL},
{"DCEP", conn_dcep_bv_num, NULL},
{"CPUP", conn_cpup_bv_num, NULL},
{"TERM", conn_term_bv_num, NULL},
{"PRDA", conn_prda_bv_num, NULL},

{NULL, NULL, NULL},
};

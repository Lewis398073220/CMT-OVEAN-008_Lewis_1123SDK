#include <stdio.h>
#include "gap.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

static void pts_sm_per_start(uint8_t pair_iocap, uint8_t pair_auth ,uint8_t pair_kist)
{
    //set paring param
    pts_sm_set_pairing_para(PTS_PAIRING_IOCAP, pair_iocap);
    pts_sm_set_pairing_para(PTS_PAIRING_AUTH, pair_auth);
    pts_sm_set_pairing_para(PTS_PAIRING_KEY_DIST, pair_kist);
    pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, GAP_AUTH_REQ_NO_MITM_NO_BOND);
    //start adv to let dongle connect iut
    pts_adv_start();
}


static void pts_sm_per_prot_bv_02_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0d, GAP_KDIST_NONE);
}

static void pts_sm_per_jw_bv_02_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0d, GAP_KDIST_NONE);
}

static void pts_sm_per_jw_bi_02_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0c, GAP_KDIST_NONE);
}

static void pts_sm_per_jw_bi_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0d, GAP_KDIST_NONE);
}

static void pts_sm_per_pke_bv_02_05c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0c, GAP_KDIST_NONE);
}

static void pts_sm_per_pke_bi_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0c, GAP_KDIST_NONE);
}

static void pts_sm_per_oob_bv_06_08_c(char *para_buf, uint32_t str_len)
{
    pts_sm_set_pairing_para(PTS_PAIRING_OOB, 0X00);
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0c, GAP_KDIST_NONE);
}

static void pts_sm_per_eks_bv_02_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0c, GAP_KDIST_NONE);
}

static void pts_sm_per_eks_bi_02_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0d, GAP_KDIST_NONE);
}

static void pts_sm_per_kdu_bv_01_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0d, GAP_KDIST_ENCKEY);
}

static void pts_sm_per_kdu_bv_02_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0d, GAP_KDIST_IDKEY);
}

static void pts_sm_per_kdu_bv_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x0d, GAP_KDIST_SIGNKEY);
}

static void pts_sm_per_kdu_bv_07_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0d, GAP_KDIST_ENCKEY);
}

static void pts_sm_per_kdu_bv_08_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0d, GAP_KDIST_IDKEY);
}

static void pts_sm_per_kdu_bv_09_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0d, GAP_KDIST_SIGNKEY);
}

static void pts_sm_per_kdu_bi_01_c(char *para_buf, uint32_t str_len)
{
    pts_sm_set_pairing_para(PTS_PAIRING_KEY_SIZE, 7);
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0d, GAP_KDIST_NONE);
}

static void pts_sm_per_kdu_bi_02_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x0d, GAP_KDIST_IDKEY);
}

static void pts_sm_per_sip_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        pts_sm_per_start(GAP_IO_CAP_KB_ONLY, 0x08, GAP_KDIST_NONE);
    }else if(2 == steps){
       //start a security req
        pts_sm_send_security_req(0, 8);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_per_sie_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x09, GAP_KDIST_ENCKEY);
    }else if(2 == steps){
        //start adv to let dongle connect iut
        pts_adv_start();
    }else if(3 == steps){
       //start a security req
        pts_sm_send_security_req(0, 9);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_per_scjw_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x08, GAP_KDIST_NONE);
    }else if(2 == steps){
        pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x08, GAP_KDIST_NONE);
    }else if(3 == steps){
        pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x08, GAP_KDIST_NONE);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_per_scjw_bv_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x08, GAP_KDIST_NONE);
}

static void pts_sm_per_scjw_bi_02_c(char *para_buf, uint32_t str_len)
{
   pts_sm_per_start(GAP_IO_CAP_NO_INPUT_NO_OUTPUT, 0x08, GAP_KDIST_NONE);
}

static void pts_sm_per_scpk_bv_02_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_ONLY, 0x08, GAP_KDIST_NONE);
}

static void pts_sm_per_scpk_bi_03_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_ONLY, 0x08, GAP_KDIST_NONE);
}

static void pts_sm_per_scpk_bi_04_c(char *para_buf, uint32_t str_len)
{
    pts_sm_per_start(GAP_IO_CAP_KB_ONLY, 0x0c, GAP_KDIST_NONE);
}

static void pts_sm_per_scct_bv_08_10_c(char *para_buf, uint32_t str_len)
{
    pts_sm_set_pairing_para(PTS_PAIRING_KEY_SIZE, 7);
    pts_sm_per_start(GAP_IO_CAP_DISPLAY_YES_NO, 0x29, GAP_KDIST_LINKKEY);
}

static pts_case_num_t per_prot_bv_num[] = {
{2, pts_sm_per_prot_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t per_jw_bv_num[] = {
{2, pts_sm_per_jw_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t per_jw_bi_num[] = {
{2, pts_sm_per_jw_bi_02_c},
{3, pts_sm_per_jw_bi_03_c},
{0xFF, NULL},
};

static pts_case_num_t per_pke_bv_num[] = {
{2, pts_sm_per_pke_bv_02_05c},
{5, pts_sm_per_pke_bv_02_05c},
{0xFF, NULL},
};

static pts_case_num_t per_pke_bi_num[] = {
{3, pts_sm_per_pke_bi_03_c},
{0xFF, NULL},
};

static pts_case_num_t per_oob_bv_num[] = {
{6, pts_sm_per_oob_bv_06_08_c},
{8, pts_sm_per_oob_bv_06_08_c},
{0xFF, NULL},
};

static pts_case_num_t per_eks_bv_num[] = {
{2, pts_sm_per_eks_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t per_eks_bi_num[] = {
{2, pts_sm_per_eks_bi_02_c},
{0xFF, NULL},
};

static pts_case_num_t per_kdu_bv_num[] = {
{1, pts_sm_per_kdu_bv_01_c},
{2, pts_sm_per_kdu_bv_02_c},
{3, pts_sm_per_kdu_bv_03_c},
{7, pts_sm_per_kdu_bv_07_c},
{8, pts_sm_per_kdu_bv_08_c},
{9, pts_sm_per_kdu_bv_09_c},
{0xFF, NULL},
};

static pts_case_num_t per_kdu_bi_num[] = {
{1, pts_sm_per_kdu_bi_01_c},
{2, pts_sm_per_kdu_bi_02_03_c},
{3, pts_sm_per_kdu_bi_02_03_c},
{0xFF, NULL},
};

static pts_case_num_t per_sip_bv_num[] = {
{1, pts_sm_per_sip_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t per_sie_bv_num[] = {
{1, pts_sm_per_sie_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t per_scjw_bv_num[] = {
{2, pts_sm_per_scjw_bv_02_c},
{3, pts_sm_per_scjw_bv_03_c},
{0xFF, NULL},
};

static pts_case_num_t per_scjw_bi_num[] = {
{2, pts_sm_per_scjw_bi_02_c},
{0xFF, NULL},
};

static pts_case_num_t per_scpk_bv_num[] = {
{2, pts_sm_per_scpk_bv_02_03_c},
{3, pts_sm_per_scpk_bv_02_03_c},
{0xFF, NULL},
};

static pts_case_num_t per_scpk_bi_num[] = {
{3, pts_sm_per_scpk_bi_03_c},
{4, pts_sm_per_scpk_bi_04_c},
{0xFF, NULL},
};

static pts_case_num_t per_scct_bv_num[] = {
{8, pts_sm_per_scct_bv_08_10_c},
{10, pts_sm_per_scct_bv_08_10_c},
{0xFF, NULL},
};

pts_module_case_t pts_per_case[] = {
{"PROT", per_prot_bv_num, NULL},
{"JW", per_jw_bv_num, per_jw_bi_num},
{"PKE", per_pke_bv_num, per_pke_bi_num},
{"OOB", per_oob_bv_num, NULL},
{"EKS", per_eks_bv_num, per_eks_bi_num},
{"KDU", per_kdu_bv_num, per_kdu_bi_num},
{"SIP", per_sip_bv_num, NULL},
{"SIE", per_sie_bv_num, NULL},
{"SCJW", per_scjw_bv_num, per_scjw_bi_num},
{"SCPK", per_scpk_bv_num, per_scpk_bi_num},
{"SCCT", per_scct_bv_num, NULL},
{NULL, NULL, NULL},
};


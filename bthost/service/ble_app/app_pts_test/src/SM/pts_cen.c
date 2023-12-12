#include <stdio.h>
#include "gap.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

static void pts_sm_cen_start_pair(uint8_t pair_iocap, uint8_t pair_auth ,uint8_t pair_kist, uint8_t pair_level)
{
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_IOCAP, pair_iocap);
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, pair_auth);
        pts_sm_set_pairing_para(PTS_PAIRING_KEY_DIST, pair_kist);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, pair_level);
        //start paring
        pts_sm_pair_req();
}
static void pts_sm_cen_prot_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_YES_NO,0X09,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,0x01);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_jw_bv_05_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if((1 == steps)||(4 == steps)||(7 == steps)||(10 == steps))
    {
        //Connect the dongle
        pts_conn_start();
    }else if((2 == steps)||(5 == steps)||(8 == steps)||(11 == steps))
    {
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_YES_NO,0X09,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,0x01);
    }else if((3 == steps)||(6 == steps)||(9 == steps)||(12 == steps))
    {
       //Disconnect the dongle
       pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_jw_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        //set pairing para
        pts_sm_cen_start_pair(GAP_IO_CAP_NO_INPUT_NO_OUTPUT,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_jw_bi_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_NO_INPUT_NO_OUTPUT,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps)
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_pke_bv_01_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps)
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_pke_bi_01_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps)
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_oob_bv_05_07_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_OOB, 0X00);
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps)
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_eks_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps)
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_eks_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X00,GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps)
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_sign_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if((1 == steps) || (4 == steps))
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        //set pairing para
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X01,GAP_KDIST_SIGNKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if((3 == steps) || (6 == steps))
    {
        //Disconnect the dongle
        pts_disconnect();
    }else if(5 == steps){
        //reg gatt cli
        pts_gatt_reg(GATT_ROLE_CLIENT);
        //write singe
        pts_gatt_cli_write(GATT_WRITE_SIGNED,6,0,0,NULL);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_sign_bv_03_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if((1 == steps)||(4 == steps))
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X01,GAP_KDIST_SIGNKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }
    else if((3 == steps)||(5 == steps))
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_sign_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if((1 == steps)||(4 == steps))
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X01,GAP_KDIST_SIGNKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }
    else if((3 == steps)||(5 == steps))
    {
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}


static void pts_sm_cen_kdu_bv_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X01,GAP_KDIST_SIGNKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_kdu_bv_05_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X01,GAP_KDIST_IDKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_kdu_bv_06_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        //set pairing para
        pts_sm_set_pairing_para(PTS_PAIRING_IOCAP, GAP_IO_CAP_DISPLAY_ONLY);
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 0x01);
        pts_sm_set_pairing_para(PTS_PAIRING_KEY_DIST, GAP_KDIST_ENCKEY);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, GAP_AUTH_REQ_NO_MITM_NO_BOND);
        //start paring
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X01,GAP_KDIST_ENCKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_kdu_bv_10_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X09,GAP_KDIST_IDKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_kdu_bv_11_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X09,GAP_KDIST_SIGNKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}


static void pts_sm_cen_kdu_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(2 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_kdu_bi_02_03_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X09,GAP_KDIST_IDKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_sip_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        pts_conn_start();
        pts_sm_cen_start_pair(GAP_IO_CAP_KB_ONLY,0X00,GAP_KDIST_NONE,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(2 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_scjw_bv_01_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_KB_ONLY,0X08,GAP_KDIST_NONE,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_scjw_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if((1 == steps) || (4 == steps) || (7 == steps) || (10 == steps) || (13 == steps))
    {
        //Connect the dongle
        pts_conn_start();
    }else if((steps == 2) || (5 == steps) || (8 == steps) || (11 == steps))
    {
        pts_sm_cen_start_pair(GAP_IO_CAP_KB_ONLY,0X08,GAP_KDIST_NONE,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if((3 == steps) || (6 == steps) || (9 == steps) || (12 == steps) || (15 == steps))
        {
        //Disconnect the dongle
        pts_disconnect();
    }else if(steps == 14){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_YES_NO,0X18,GAP_KDIST_NONE,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_scpk_bv_01_04_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X18,GAP_KDIST_NONE,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_scpk_bi_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X18,GAP_KDIST_NONE,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_scpk_bi_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_cen_start_pair(GAP_IO_CAP_DISPLAY_ONLY,0X19,GAP_KDIST_IDKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static void pts_sm_cen_scct_bv_07_09_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    if(1 == steps)
    {
        //set dongle address
        //Connect the dongle
        pts_conn_start();
    }else if(steps == 2){
        pts_sm_set_pairing_para(PTS_PAIRING_KEY_SIZE, 7);
        pts_sm_cen_start_pair(GAP_IO_CAP_KB_DISPLAY,0X29,GAP_KDIST_LINKKEY,GAP_AUTH_REQ_NO_MITM_NO_BOND);
    }else if(3 == steps){
        //Disconnect the dongle
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]ERROR:step is NULL!", __FUNCTION__, __LINE__);
    }
}

static pts_case_num_t cen_prot_bv_num[] = {
{1, pts_sm_cen_prot_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t cen_jw_bv_num[] = {
{5, pts_sm_cen_jw_bv_05_c},
{0xFF, NULL},
};

static pts_case_num_t cen_jw_bi_num[] = {
{1, pts_sm_cen_jw_bi_01_c},
{4, pts_sm_cen_jw_bi_04_c},
{0xFF, NULL},
};

static pts_case_num_t cen_pke_bv_num[] = {
{1, pts_sm_cen_pke_bv_01_04_c},
{4, pts_sm_cen_pke_bv_01_04_c},
{0xFF, NULL},
};

static pts_case_num_t cen_pke_bi_num[] = {
{1, pts_sm_cen_pke_bi_01_02_c},
{2, pts_sm_cen_pke_bi_01_02_c},
{0xFF, NULL},
};

static pts_case_num_t cen_oob_bv_num[] = {
{5, pts_sm_cen_oob_bv_05_07_c},
{7, pts_sm_cen_oob_bv_05_07_c},
{0xFF, NULL},
};

static pts_case_num_t cen_eks_bv_num[] = {
{1, pts_sm_cen_eks_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t cen_eks_bi_num[] = {
{1, pts_sm_cen_eks_bi_01_c},
{0xFF, NULL},
};

static pts_case_num_t cen_sign_bv_num[] = {
{1, pts_sm_cen_sign_bv_01_c},
{3, pts_sm_cen_sign_bv_03_c},
{0xFF, NULL},
};


static pts_case_num_t cen_sign_bi_num[] = {
{1, pts_sm_cen_sign_bi_01_c},
{0xFF, NULL},
};

static pts_case_num_t cen_kdu_bv_num[] = {
{4, pts_sm_cen_kdu_bv_04_c},
{5, pts_sm_cen_kdu_bv_05_c},
{6, pts_sm_cen_kdu_bv_06_c},
{10, pts_sm_cen_kdu_bv_10_c},
{11, pts_sm_cen_kdu_bv_11_c},
{0xFF, NULL},
};

static pts_case_num_t cen_kdu_bi_num[] = {
{1, pts_sm_cen_kdu_bi_01_c},
{2, pts_sm_cen_kdu_bi_02_03_c},
{3, pts_sm_cen_kdu_bi_02_03_c},
{0xFF, NULL},
};

static pts_case_num_t cen_sip_bv_num[] = {
{2, pts_sm_cen_sip_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t cen_scjw_bv_num[] = {
{1, pts_sm_cen_scjw_bv_01_04_c},
{4, pts_sm_cen_scjw_bv_01_04_c},
{0xFF, NULL},
};

static pts_case_num_t cen_scjw_bi_num[] = {
{1, pts_sm_cen_scjw_bi_01_c},
{0xFF, NULL},
};

static pts_case_num_t cen_scpk_bv_num[] = {
{1, pts_sm_cen_scpk_bv_01_04_c},
{4, pts_sm_cen_scpk_bv_01_04_c},
{0xFF, NULL},
};

static pts_case_num_t cen_scpk_bi_num[] = {
{1, pts_sm_cen_scpk_bi_01_c},
{2, pts_sm_cen_scpk_bi_02_c},
{0xFF, NULL},
};

static pts_case_num_t cen_scct_bv_num[] = {
{7, pts_sm_cen_scct_bv_07_09_c},
{9, pts_sm_cen_scct_bv_07_09_c},
{0xFF, NULL},
};


pts_module_case_t pts_cen_case[] = {
{"PROT", cen_prot_bv_num, NULL},
{"JW", cen_jw_bv_num, cen_jw_bi_num},
{"PKE", cen_pke_bv_num, cen_pke_bi_num},
{"OOB", cen_oob_bv_num, NULL},
{"EKS", cen_eks_bv_num, cen_eks_bi_num},
{"SIGN", cen_sign_bv_num, cen_sign_bi_num},
{"KDU", cen_kdu_bv_num, cen_kdu_bi_num},
{"SIP", cen_sip_bv_num, NULL},
{"SCJW", cen_scjw_bv_num, cen_scjw_bi_num},
{"SCPK", cen_scpk_bv_num, cen_scpk_bi_num},
{"SCCT", cen_scct_bv_num, NULL},
{NULL, NULL, NULL},
};


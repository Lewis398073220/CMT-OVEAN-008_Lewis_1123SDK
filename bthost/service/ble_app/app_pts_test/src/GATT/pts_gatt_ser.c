#include <stdio.h>
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "pts_sm.h"

enum pts_bc_att
{
    ATT_SVC,
    ATT_CP_CHAR,
    ATT_CP_VAL,
    ATT_RX_STATE_CHAR,
    ATT_RX_STATE_VAL,
    ATT_RX_STATE_CFG,
    ATT_MAX_NB,
};

struct gatt_att_desc pts_att[ATT_MAX_NB] =
{
    /// Service Declaration
    [ATT_SVC] = {{0x00, 0x28},PROP(RD), 0},
    /// Characteristic Declaration
    [ATT_CP_CHAR] = {{0x03, 0x28}, PROP(RD), 0},
    /// Characteristic Value
    [ATT_CP_VAL] = {{0xC7, 0x2B}, PROP(WC) | PROP(WR), GATT_ATT_WRITE_MAX_SIZE_MASK},
    /// Receive State Characteristic Declaration  
    [ATT_RX_STATE_CHAR] = {{0x03, 0x28}, PROP(RD), 0},
    /// Receive State Characteristic Value
    [ATT_RX_STATE_VAL] = {{0xC8, 0x2B}, PROP(RD) | PROP(N), 0},
    /// Receive State Characteristic Client Characteristic Configuration Descriptor
    [ATT_RX_STATE_CFG] = {{0x02, 0x29}, PROP(RD) | PROP(WR), OPT(NO_OFFSET)},
};

extern gatt_para_t gatt_para;
void pts_gatt_ser_init(uint8_t info, uint8_t *uuid ){
    if(gatt_para.pts_server.pts_user_lid ==0xFF){
        pts_gatt_reg(GATT_ROLE_SERVER);
        osDelay(10);
    }
    if(gatt_para.pts_server.server_att_para[0].start_hdl ==0){//ATT not add
        pts_gatt_svc_add(info, uuid, ATT_MAX_NB, ATT_MAX_NB, (gatt_att_desc_t *)&pts_att);
    }else{
        pts_gatt_svc_remove(0);
        osDelay(50);
        pts_gatt_svc_add(info, uuid, ATT_MAX_NB, ATT_MAX_NB, (gatt_att_desc_t *)&pts_att);//UPdate info
    }
}

void pts_gatt_ser_att_add(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }
}

void pts_gatt_ser_gar_bi_03(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;//SVC_SEC_LVL(NOT_ENC)|SVC_UUID(16);
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        osDelay(10);
        pts_gatt_svc_ctrl(0, 0, 1);
        pts_adv_start();
    }
}

void pts_gatt_ser_gar_bi_04(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = SVC_SEC_LVL(SECURE_CON)|SVC_UUID(16);
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }
}

void pts_gatt_ser_gar_bi_05(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = SVC_SEC_LVL(NO_AUTH)|SVC_UUID(16);
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    if(test_step == 1){
        SETB(info, GATT_SVC_EKS,1);
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_sm_set_pairing_para(PTS_PAIRING_KEY_SIZE,7);
        pts_adv_start();
    }
}

void pts_gatt_ser_gar_bv_04(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 45);
        pts_adv_start();
    }
}
void pts_gatt_ser_gar_bv_07(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }

    pts_att[ATT_RX_STATE_CFG].ext_info = GATT_ATT_WRITE_MAX_SIZE_MASK;
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 45);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_att_add(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bi_04(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;//SVC_SEC_LVL(NOT_ENC)|SVC_UUID(16);
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_RX_STATE_VAL].info|=(PROP(WC) | PROP(WR));
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        osDelay(10);
        pts_gatt_svc_ctrl(0, 0, 1);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bi_05(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = SVC_SEC_LVL(SECURE_CON)|SVC_UUID(16);
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_RX_STATE_VAL].info|=(PROP(WC) | PROP(WR));
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bi_06(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = SVC_SEC_LVL(NO_AUTH)|SVC_UUID(16);
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_RX_STATE_VAL].info|=(PROP(WC) | PROP(WR));
    if(test_step == 1){
        SETB(info, GATT_SVC_EKS,1);
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_sm_set_pairing_para(PTS_PAIRING_KEY_SIZE,7);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bv_05(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 45);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bi_09(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    pts_att[ATT_CP_VAL].ext_info = 66;
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 66);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bi_32(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    pts_att[ATT_CP_VAL].ext_info = 10; //att_lenth <MTU-3
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 10);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bi_33(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    pts_att[ATT_CP_VAL].ext_info = 40; //att_lenth >MTU-3
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 40);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bv_08(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }

    pts_att[ATT_SVC].uuid[0] = 0x01;
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    pts_att[ATT_CP_VAL].ext_info = 66;
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 66);
        osDelay(50);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bv_09(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }

    pts_att[ATT_RX_STATE_CFG].uuid[0] = 0x01;
    pts_att[ATT_RX_STATE_CFG].ext_info = 45;
    pts_att[ATT_CP_VAL].info|= PROP(RD);

    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 45);
        pts_adv_start();
    }
}

void pts_gatt_ser_gaw_bv_10(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    uint16_t uuid_1 = 0x018F;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }

    pts_att[ATT_SVC].uuid[0] = 0x01;
    pts_att[ATT_CP_VAL].info|= PROP(RD);
    pts_att[ATT_CP_VAL].ext_info = 66;
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_gatt_set_ser_att_datasize(0, 66);
        pts_gatt_set_ser_att_datasize(1, 66);
        osDelay(50);
        pts_gatt_svc_add(info, (uint8_t *)&uuid_1, ATT_MAX_NB, ATT_MAX_NB, (gatt_att_desc_t *)&pts_att);
        pts_adv_start();
    }
}

void pts_gatt_ser_gan_bv_01(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0; int hdl = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x", &test_step, &hdl);
    }
    pts_att[ATT_CP_VAL].info|=(PROP(RD)|PROP(N));
    uint8_t evt_type = GATT_NOTIFY; uint8_t value_length = 1;uint8_t value=63;
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }else if(test_step == 2){
         pts_gatt_srv_event_send(hdl, evt_type, value_length, &value);
    }
}

void pts_gatt_ser_gai_bv_01(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0; int hdl = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x", &test_step, &hdl);
    }
    pts_att[ATT_CP_VAL].info|=(PROP(RD)|PROP(N)|PROP(I));
    uint8_t evt_type = GATT_INDICATE; uint8_t value_length = 1;uint8_t value=63;
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }else if(test_step == 2){
         pts_gatt_srv_event_send(hdl, evt_type, value_length, &value);
    }
}

void pts_gatt_ser_gas_bv_01(char *para_buf, uint32_t str_len){
    int test_step=0 ;uint8_t info = 0;
    uint16_t uuid = GATT_SVC_BCAST_AUDIO_SCAN;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d", &test_step);
    }
    if(test_step == 1){
        pts_gatt_ser_init(info,(uint8_t *)&uuid);
        pts_adv_start();
    }else if(test_step == 2){
        if(gatt_para.pts_server.server_att_para[0].start_hdl ==0){//ATT not add
            pts_gatt_svc_add(info, (uint8_t *)& uuid, ATT_MAX_NB, ATT_MAX_NB, (gatt_att_desc_t *)&pts_att);
        }else{
            pts_gatt_svc_remove(0);}
    }
}

static pts_case_num_t le_gatt_sr_gad_bv_num[] = {
{1, pts_gatt_ser_att_add},
{2, pts_gatt_ser_att_add},
{3, pts_gatt_ser_att_add},
{4, pts_gatt_ser_att_add},
{5, pts_gatt_ser_att_add},
{6, pts_gatt_ser_att_add},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gar_bv_num[] = {
{1, pts_gatt_ser_att_add},
{3, pts_gatt_ser_att_add},
{4, pts_gatt_ser_gar_bv_04},
{5, pts_gatt_ser_att_add},
{6, pts_gatt_ser_att_add},
{7, pts_gatt_ser_att_add},
{8, pts_gatt_ser_att_add},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gar_bi_num[] = {
{1, pts_gatt_ser_att_add},
{2, pts_gatt_ser_att_add},
{3, pts_gatt_ser_gar_bi_03},
{4, pts_gatt_ser_gar_bi_04},
{5, pts_gatt_ser_gar_bi_05},
{6, pts_gatt_ser_att_add},
{7, pts_gatt_ser_att_add},
{8, pts_gatt_ser_att_add},
{9, pts_gatt_ser_gar_bi_03},
{10, pts_gatt_ser_gar_bi_04},
{11, pts_gatt_ser_gar_bi_05},
{12, pts_gatt_ser_att_add},
{13, pts_gatt_ser_gar_bv_04},
{14, pts_gatt_ser_att_add},
{15, pts_gatt_ser_gar_bi_03},
{16, pts_gatt_ser_gar_bi_04},
{17, pts_gatt_ser_gar_bi_05},
{18, pts_gatt_ser_att_add},
{19, pts_gatt_ser_att_add},
{20, pts_gatt_ser_gar_bi_03},
{21, pts_gatt_ser_gar_bi_04},
{22, pts_gatt_ser_gar_bi_05},
{45, pts_gatt_ser_att_add},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gaw_bv_num[] = {
{1, pts_gatt_ser_gaw_att_add},
{3, pts_gatt_ser_gaw_att_add},
{5, pts_gatt_ser_gaw_bv_05},
{6, pts_gatt_ser_gaw_att_add},
{7, pts_gatt_ser_gaw_bv_08},
{8, pts_gatt_ser_gaw_bv_08},
{9, pts_gatt_ser_gaw_bv_09},
{10, pts_gatt_ser_gaw_bv_10},
{11, pts_gatt_ser_gaw_bv_05},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gaw_bi_num[] = {
{2, pts_gatt_ser_gaw_att_add},
{3, pts_gatt_ser_gaw_att_add},
{4, pts_gatt_ser_gaw_bi_04},
{5, pts_gatt_ser_gaw_bi_05},
{6, pts_gatt_ser_gaw_bi_06},
{7, pts_gatt_ser_gaw_att_add},
{8, pts_gatt_ser_gaw_att_add},
{9, pts_gatt_ser_gaw_bi_09},
{11, pts_gatt_ser_gaw_bi_04},
{12, pts_gatt_ser_gaw_bi_05},
{13, pts_gatt_ser_gaw_bi_06},
{32, pts_gatt_ser_gaw_bi_32},
{33, pts_gatt_ser_gaw_bi_33},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gan_bv_num[] = {
{1, pts_gatt_ser_gan_bv_01},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gai_bv_num[] = {
{1, pts_gatt_ser_gai_bv_01},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gas_bv_num[] = {
{1, pts_gatt_ser_gas_bv_01},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_gat_bv_num[] = {
{1, pts_gatt_ser_gai_bv_01},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_sr_uns_bi_num[] = {
{1, pts_gatt_ser_gaw_att_add},
{2, pts_gatt_ser_gaw_att_add},
{0xFF, NULL},
};

pts_module_case_t pts_gatt_sr_case[] = {
{"GAD", le_gatt_sr_gad_bv_num, NULL},
{"GAR", le_gatt_sr_gar_bv_num, le_gatt_sr_gar_bi_num},
{"GAW", le_gatt_sr_gaw_bv_num, le_gatt_sr_gaw_bi_num},
{"GAN", le_gatt_sr_gan_bv_num, NULL},
{"GAI", le_gatt_sr_gai_bv_num, NULL},
{"GAS", le_gatt_sr_gas_bv_num, NULL},
{"GAT", le_gatt_sr_gat_bv_num, NULL},
{"UNS", NULL, le_gatt_sr_uns_bi_num},
{NULL, NULL, NULL},
};


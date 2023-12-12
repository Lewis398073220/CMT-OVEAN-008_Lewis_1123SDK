#ifndef __PTS_ADV_H__
#define __PTS_ADV_H__
#include "pts_ble_host_app.h"

extern pts_module_case_t pts_brob_case[];
extern pts_module_case_t pts_disc_case[];
extern pts_module_case_t pts_idle_case[];
extern pts_module_case_t pts_conn_case[];
extern pts_module_case_t pts_bond_case[];
extern pts_module_case_t pts_sec_case[];
extern pts_module_case_t pts_priv_case[];
extern pts_module_case_t pts_adv_case[];
extern pts_module_case_t pts_padv_case[];
extern pts_module_case_t pts_dm_case[];
extern pts_module_case_t pts_bis_case[];

void pts_gap_module_reg();


#endif

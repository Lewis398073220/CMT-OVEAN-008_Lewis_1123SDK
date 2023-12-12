#include <stdio.h>
#include <string.h>
#include "ke_task.h"
#include "gapm_le.h"
#include "pts_gap.h"
#include "pts_sm.h"
#include "pts_bap.h"
#include "pts_l2cap.h"
#include "pts_ble_host_interface.h"
#include "pts_gatt.h"

/* Defines the place holder for the states of all the task instances. */
extern const struct ke_task_desc TASK_BLE_PTS_APP;

gap_bdaddr_t dongle_addr = {
    .addr = {0x69, 0x47, 0x94, 0x71, 0xf8, 0xd0},
    .addr_type = GAPM_STATIC_ADDR
};

pts_module_t ble_host_pts_module[10] = {0};

void ble_host_pts_init()
{
    static bool init_flag = 0;

    if(init_flag == 0){
        // Create APP task
        ke_task_create(TASK_BLE_PTS, &TASK_BLE_PTS_APP);
        // Initialize Task state
        ke_state_set(TASK_BLE_PTS, 0);

        init_flag = 1;
    }
    //init sec
    pts_sm_init();

    //reg GAP module
    pts_gap_module_reg();

    //reg L2CAP module
    pts_l2cap_module_reg();

    //reg L2CAP module
    pts_gatt_module_reg();

    //reg SM module
    pts_sm_module_reg();

    //reg BAP module
    pts_bap_module_reg();
}

void ble_host_pts_module_reg(char *module_name, pts_sub_module_t *sub_module)
{
    static int module_num=0;
    if((module_name) && (sub_module))
    {
        strcpy(ble_host_pts_module[module_num].module_name, module_name);
        ble_host_pts_module[module_num].sub_module = sub_module;

        module_num++;
    }
    else
    {
        TRACE(0, "[%s][%d][ERROR]: reg pts module Parameter is invalid!", __FUNCTION__, __LINE__);
    }
}

void ble_host_pts_cmd_handle(uint32_t cmd_index, uint32_t str_len)
{
    int i=0;
    char *cmd_buf = (char *)cmd_index;
    pts_case_num_t *case_num = NULL;
    pts_module_case_t *test_case = NULL;
    pts_sub_module_t *sub_mod = NULL;
    TRACE(0, "[%s][%d]:%s", __FUNCTION__, __LINE__, cmd_buf);

    //find ble host pts module
    for(i=0; ble_host_pts_module[i].module_name[0] != 0; ++i)
    {
        if (cmd_buf == strstr(cmd_buf, ble_host_pts_module[i].module_name))
        {
            sub_mod = ble_host_pts_module[i].sub_module;
            cmd_buf += (strlen(ble_host_pts_module[i].module_name) +1);
            break;
        }
    }

    //find ble host pts sub_module
    if(sub_mod)
    {
        for(i=0; sub_mod[i].sub_module_name != NULL; ++i)
        {
            //parse sub module
            if(cmd_buf == strstr(cmd_buf, sub_mod[i].sub_module_name))
            {
                test_case = sub_mod[i].test_case;
                cmd_buf += (strlen(sub_mod[i].sub_module_name) +1);
                break;
            }
        }
    }else{
        TRACE(0, "[%s][%d]ERROR: no find module!", __FUNCTION__, __LINE__);
    }

    //find ble host pts case
    if(test_case)
    {
        for(i=0; test_case[i].case_name != NULL; ++i)
        {
            //Some items do not have cast-level catalogss
            if(strstr(test_case[i].case_name, BLE_HOST_TEST_NO_CASE))
            {
                break;
            }

            //parse sub module
            if(cmd_buf == strstr(cmd_buf, test_case[i].case_name))
            {
                cmd_buf += (strlen(test_case[i].case_name) +1);
                break;
            }
        }

        if(strstr(cmd_buf, "BV-"))
        {
           case_num = test_case[i].case_bv;
        }
        else if(strstr(cmd_buf, "BI-"))
        {
            case_num = test_case[i].case_bi;
        }
    }else {
        TRACE(0, "[%s][%d]ERROR: no find sub_module!", __FUNCTION__, __LINE__);
    }

    //find ble host pts case num
    if(case_num)
    {
        int num = 0;
        cmd_buf += 3;

        sscanf(cmd_buf, "%d", &num);
        for(i=0; case_num[i].number != 0xFF; ++i)
        {
            if(case_num[i].number == num)
            {
                if(strstr(cmd_buf, " "))
                {
                    cmd_buf = strstr(cmd_buf, " ") + 1;
                    str_len = str_len - (cmd_buf - (char *)cmd_index);
                }
                else
                {
                    cmd_buf = NULL;
                    str_len = 0;
                }

                case_num[i].case_handle(cmd_buf, str_len);
                break;
            }
        }

        if(case_num[i].number == 0xFF)
        {
            TRACE(0, "[%s][%d]ERROR: no find case num!", __FUNCTION__, __LINE__);
        }
    }else {
        TRACE(0, "[%s][%d]ERROR: no find case!", __FUNCTION__, __LINE__);
    }

}


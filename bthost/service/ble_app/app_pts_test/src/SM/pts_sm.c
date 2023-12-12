#include <stdio.h>
#include "pts_ble_host_app.h"
#include "pts_sm.h"

pts_sub_module_t pts_sm_sub_modele[] = {
{"CEN" , pts_cen_case },
{"PER" , pts_per_case },
{NULL, NULL},
};

void pts_sm_module_reg()
{
    ble_host_pts_module_reg("SM", pts_sm_sub_modele);
}

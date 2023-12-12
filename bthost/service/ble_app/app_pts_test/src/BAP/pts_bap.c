#include <stdio.h>
#include "pts_ble_host_app.h"
#include "pts_bap.h"

pts_sub_module_t pts_bap_sub_modele[] = {
{"BSRC", pts_bsrc_case},
{NULL, NULL},
};

void pts_bap_module_reg()
{
    ble_host_pts_module_reg("BAP", pts_bap_sub_modele);
}

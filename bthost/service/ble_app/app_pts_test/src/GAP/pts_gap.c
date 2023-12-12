#include <stdio.h>
#include "pts_ble_host_app.h"
#include "pts_gap.h"

pts_sub_module_t pts_gap_sub_modele[] = {
{"BROB", pts_brob_case},
{"DISC", pts_disc_case},
{"IDLE", pts_idle_case},
{"CONN", pts_conn_case},
{"BOND", pts_bond_case},
{"SEC" , pts_sec_case },
{"PRIV", pts_priv_case},
{"ADV" , pts_adv_case },
{"PADV", pts_padv_case},
{"DM"  , pts_dm_case  },
{"BIS" , pts_bis_case },
{NULL, NULL},
};

void pts_gap_module_reg()
{
    ble_host_pts_module_reg("GAP", pts_gap_sub_modele);
}

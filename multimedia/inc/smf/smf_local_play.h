#pragma once
#ifndef __SMF_LOCAL_PLAYER_H__
#define __SMF_LOCAL_PLAYER_H__
// #include "smf_api.h"
// #include "smf_debug.h"

#ifndef EXTERNC
#ifndef __cplusplus
#define EXTERNC
#else
#define EXTERNC extern "C"
#endif
#endif
// enum {
//     default = 0
//     ,loop
//     ,single_loop
//     ,random
//     ,single
//     ,restart
// }playerMode;
/** config player resource
 * @return void 
 * @note must be executed only once at startup Demo
 */
EXTERNC void smf_local_player_config();

/** playback start
 * @param mode[in]: play mode
 * @return true/false
 */
EXTERNC bool smf_local_player_start(uint32_t mode);

/** playback stop
 * @param mode[in]: play mode
 *	0:stop
 *	1:pause
 * @return true/false
 */
EXTERNC bool smf_local_player_stop(uint32_t mode);

#endif


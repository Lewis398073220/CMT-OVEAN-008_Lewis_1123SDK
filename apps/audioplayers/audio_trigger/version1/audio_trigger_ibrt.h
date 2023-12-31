#ifndef __AUDIO_TRIGGER_IBRT_H__
#define __AUDIO_TRIGGER_IBRT_H__


#ifdef __cplusplus
extern "C" {
#endif

void app_bt_stream_ibrt_auto_synchronize_stop(void);
int app_bt_stream_ibrt_audio_master_detect_next_packet_start(void);
int app_bt_stream_ibrt_audio_slave_detect_next_packet_start(int need_autotrigger);
bool a2dp_player_playback_waterline_is_enalbe(void);
void a2dp_player_playback_waterline_enable_set(bool enable);

#ifdef __cplusplus
}
#endif

#endif

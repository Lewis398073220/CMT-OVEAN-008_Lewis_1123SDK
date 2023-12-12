#ifndef __ANC_ASSIST_H__
#define __ANC_ASSIST_H__

#include <stdint.h>
#include <stdbool.h>
#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ANC_DUAL_CHANNEL)&& !defined(FREEMAN_ENABLED_STERO)
#define MAX_FF_CHANNEL_NUM   (1)
#define MAX_FB_CHANNEL_NUM   (1)
#define MAX_TALK_CHANNEL_NUM (1)
#define MAX_REF_CHANNEL_NUM  (1)
#else
#define MAX_FF_CHANNEL_NUM   (2)
#define MAX_FB_CHANNEL_NUM   (2)
#define MAX_TALK_CHANNEL_NUM (2)
#define MAX_REF_CHANNEL_NUM  (2)
#endif

// NOTE: This format is used to extend.
// e.g. Two algos control same gain.
// TODO: Just support one adaptive anc algo
typedef enum {
    ANC_ASSIST_ALGO_ID_NONE 			= 0,
    ANC_ASSIST_ALGO_ID_FF_HOWLING		= (0x1 << 0),
    ANC_ASSIST_ALGO_ID_FB_HOWLING 		= (0x1 << 1),
    ANC_ASSIST_ALGO_ID_HYBRID_HOWLING 	= (0x1 << 2),
    ANC_ASSIST_ALGO_ID_NOISE 			= (0x1 << 3),
    ANC_ASSIST_ALGO_ID_WIND 			= (0x1 << 4),
    ANC_ASSIST_ALGO_ID_PILOT 			= (0x1 << 5),
    ANC_ASSIST_ALGO_ID_NOISE_CLASSIFY 	= (0x1 << 6),
    ANC_ASSIST_ALGO_ID_PNC_SELECT 		= (0x1 << 7),
    ANC_ASSIST_ALGO_ID_PNC 				= (0x1 << 8),
    ANC_ASSIST_ALGO_ID_WSD 				= (0x1 << 9),
    ANC_ASSIST_ALGO_ID_EXTERN_WD 		= (0x1 << 10),
    ANC_ASSIST_ALGO_ID_FFT              = (0x1 << 11),
} anc_assist_algo_id_t;

extern const char *howling_status_str[];
extern const char *noise_status_str[];
extern const char *wind_status_str[];
extern const char *noise_classify_status_str[];

// ---------- Assist Status ----------

typedef enum {
    ASSIST_CTRL_CFG_SYNC = 0,
    ASSIST_CTRL_CUSTOM_LEAK_CTRL_ON,
    ASSIST_CTRL_CUSTOM_LEAK_CTRL_OFF,
    ASSIST_CTRL_CUSTOM_LEAK_CTRL_RESET,

    ASSIST_CTRL_M55_CFG_QTY
} assist_ctrl_m55_t;

typedef enum {
    ANC_ASSIST_ALGO_STATUS_NO_CHANGED = 0,
    ANC_ASSIST_ALGO_STATUS_CHANGED
} anc_assist_algo_status_t;

typedef enum {
    HOWLING_STATUS_INVALID = 0,
    HOWLING_STATUS_NORMAL,
    HOWLING_STATUS_HOWLING,
    HOWLING_STATUS_DUALPEAK,
} howling_status_t;

typedef enum {
    NOISE_STATUS_INVALID = 0,
    NOISE_STATUS_QUIET_ANC,
    NOISE_STATUS_LOWER_ANC,
    NOISE_STATUS_MIDDLE_ANC,
    NOISE_STATUS_STRONG_ANC,
} noise_status_t;

typedef enum {
    // NOISE_STATUS_INVALID = 0,
    NOISE_STATUS_INDOOR_ANC = 1,
    NOISE_STATUS_OUTDOOR_ANC,
    NOISE_STATUS_TRANSPORT_ANC,
    NOISE_STATUS_PLANE_ANC,
} noise_classify_status_t;

typedef enum {
    NOISE_ENVIRONMENT_INVALID = 0,
    NOISE_ENVIRONMENT_SILENCE,
    NOISE_ENVIRONMENT_VEHICLE,
    NOISE_ENVIRONMENT_BABBLE,
    NOISE_ENVIRONMENT_HIGHFREQ,
} noise_environment_t;

typedef enum {
    WIND_STATUS_INVALID = 0,
    WIND_STATUS_NONE,				// 100%
    WIND_STATUS_SMALL_TO_NONE,		// 75%
    WIND_STATUS_SMALL,				// 50%
    WIND_STATUS_NORMAL_TO_SMALL,	// 25%
    WIND_STATUS_NORMAL,				// 0%
    WIND_STATUS_STRONG_TO_NORMAL,
    WIND_STATUS_STRONG,
} wind_status_t;

typedef enum {
    WD_STATUS_IDLE = 0,
    WD_STATUS_IN_EAR,
    WD_STATUS_OUT_EAR,
} wd_status_t;

typedef enum {
    ULTRASOUND_STATUS_IDLE = 0,
    ULTRASOUND_STATUS_CHANGED,
} ultrasound_status_t;

typedef enum {
    CUSTOM_LEAK_DETECT_STATUS_UNKNOWN = 0,
    CUSTOM_LEAK_DETECT_STATUS_WAIT,
    CUSTOM_LEAK_DETECT_STATUS_RUNNING,
    CUSTOM_LEAK_DETECT_STATUS_RESULT,
    CUSTOM_LEAK_DETECT_STATUS_STOP,
} custom_leak_detect_status_t;

typedef enum {
    PROMPT_LEAK_DETECT_STATUS_WAIT = 0,
    PROMPT_LEAK_DETECT_STATUS_RUNNING,
    PROMPT_LEAK_DETECT_STATUS_RESULT,
    PROMPT_LEAK_DETECT_STATUS_STOP,
    PROMPT_LEAK_DETECT_STATUS_UNKNOWN
} prompt_leak_detect_status_t;


// ---------- Assist Cfg ----------
typedef struct {
    uint32_t	ind0;
    uint32_t	ind1;
    uint32_t	time_thd;
    float		power_thd;
} assist_howling_cfg_t;

typedef struct {
    float       strong_low_thd;
    float       strong_limit_thd;
    float       lower_low_thd;
    float       lower_mid_thd;
    float		quiet_out_thd;
    float       quiet_thd;
    float       snr_thd;
    int         period;
    int         window_size;
    int         strong_count;
    int         normal_count;
    float       band_freq[4];
    float       band_weight[3];
} assist_noise_cfg_t;

typedef struct {
    uint32_t	scale_size;
    uint32_t    to_none_targettime;
    float       power_thd;
    float       no_thd;
    float       small_thd;
    float       strong_thd;
    float       normal_thd;
    float       gain_none;
    float       gain_small_to_none;
    float       gain_small;
    float		gain_normal_to_small;
    float		gain_normal;
    float       gain_strong_to_normal;
    float       gain_strong;
} assist_wind_cfg_t;
typedef struct {
    int  pnc_lower_bound;
    int  pnc_upper_bound;
    int  out_lower_bound;
    int	 out_upper_bound;
    int  cal_period;
    float out_thd;
} assist_pnc_cfg_t;

#define PNC_MODE_NUM (11)
typedef struct {
    int dump_en;
    int delay;
    int cal_period;
    uint32_t gain_smooth_ms;

    int adaptive_anc_en;
    int playback_ms;
    float thd_on[PNC_MODE_NUM];
    float thd_off[PNC_MODE_NUM];

    int wd_en;
    int debug_en;
    float infra_ratio;
    int ultrasound_stable_tick;
    int ultrasound_stop_tick;
    float ultrasound_thd;
    float inear_thd;
    float outear_thd;
    int energy_cmp_num;

    int custom_leak_detect_en;
    int custom_leak_detect_playback_loop_num;
    const int * custom_pcm_data;
    int custom_pcm_data_len;
    float gain_local_pilot_signal;
} assist_pilot_cfg_t;

#define PROMPT_CURVE_NUM (11)
typedef struct {
    int dump_en;
    int cal_period;
    int curve_num;
    float max_env_energy;
    int start_index;
    int end_index;
    float thd1[PROMPT_CURVE_NUM];
    float thd2[PROMPT_CURVE_NUM];
    float thd3[PROMPT_CURVE_NUM];
} assist_prompt_cfg_t;

#define ASSIST_AUTO_EQ_NUM (6)
typedef struct {
    int delay;
    int cal_period;
    float thd[ASSIST_AUTO_EQ_NUM];
} assist_auto_eq_cfg_t;
typedef struct {
    float       plane_thd;
    float       plane_hold_thd;
    float       transport_thd;
    float       transport_hold_thd;
    float       indoor_thd;
    float       indoor_hold_thd;
    float       snr_thd;
    int         period;
    int         window_size;
    int         strong_count;
    int         normal_count;
    float       band_freq[4];
    float       band_weight[3];
} assist_noise_classify_cfg_t;

typedef struct {
    uint32_t	ff_howling_en;
    uint32_t	fb_howling_en;
    uint32_t	noise_en;
    uint32_t	noise_classify_en;
    uint32_t	wind_en;
    uint32_t    pilot_en;
    uint32_t    custom_leak_en;
    uint32_t	pnc_en;
    uint32_t	wsd_en;
    uint32_t	extern_kws_en;
    uint32_t    ultrasound_en;
    uint32_t	prompt_en;
    uint32_t    extern_adaptive_eq_en;
    uint32_t    extern_adj_eq_en;
    uint32_t 	autp_eq_en;
    uint32_t    fir_lms_en;
    uint32_t    optimal_tf_en;
    uint32_t    fir_anc_open_leak_en;
    uint32_t    vpu_anc_en;
} assist_enable_list_cfg_t;

typedef struct {
    float dist1; // dist for TALK and FF
    float dist2; // dist for FF L and FF R
    uint32_t min_frequency;
    uint32_t max_frequency;
    float feat1_threshold;
    float feat2_threshold;
    float feat3_threshold;
    float wsd_in_time;
    float wsd_out_time;
} assist_stereo_wsd_cfg_t;

typedef struct {
    int32_t     bypass;
    uint32_t    debug_en;
    uint32_t    wind_debug_en;
    assist_enable_list_cfg_t enable_list_cfg;

    assist_howling_cfg_t	ff_howling_cfg;
    assist_howling_cfg_t	fb_howling_cfg;
    assist_noise_cfg_t		noise_cfg;
    assist_noise_classify_cfg_t	noise_classify_cfg;
    assist_wind_cfg_t		wind_cfg;
    assist_pilot_cfg_t 		pilot_cfg;
    assist_pnc_cfg_t  		pnc_cfg;
    assist_prompt_cfg_t     prompt_cfg;
    assist_auto_eq_cfg_t    auto_eq_cfg;
    assist_stereo_wsd_cfg_t stereo_wsd_cfg;
} AncAssistConfig;

typedef struct {
    uint8_t ff_ch_num;
    uint8_t fb_ch_num;
    uint8_t talk_ch_num;

    // Status changed flag
    uint32_t	ff_gain_changed[MAX_FF_CHANNEL_NUM];
    uint32_t	fb_gain_changed[MAX_FB_CHANNEL_NUM];
    uint32_t	curve_changed[MAX_FB_CHANNEL_NUM];

    // Who change gain or curve
    anc_assist_algo_id_t	ff_gain_id[MAX_FF_CHANNEL_NUM];
    anc_assist_algo_id_t 	fb_gain_id[MAX_FB_CHANNEL_NUM];
    anc_assist_algo_id_t 	curve_id[MAX_FB_CHANNEL_NUM];

    // Gain or curve
    float		ff_gain[MAX_FF_CHANNEL_NUM];
    float		fb_gain[MAX_FB_CHANNEL_NUM];
    uint32_t	curve_index[MAX_FB_CHANNEL_NUM];

    // Result from every algo
    uint32_t    ff_howling_status[MAX_FF_CHANNEL_NUM];
    uint32_t    fb_howling_status[MAX_FB_CHANNEL_NUM];
    uint32_t    hybrid_howling_status[MAX_FB_CHANNEL_NUM];
    uint32_t 	noise_status;
    uint32_t    noise_environment;
    uint32_t 	wind_status;

    //
    uint32_t    noise_adapt_changed;
    uint32_t    noise_classify_adapt_changed[MAX_FF_CHANNEL_NUM];
    uint32_t    noise_classify_status[MAX_FF_CHANNEL_NUM];

    // WD
    uint32_t	wd_changed[MAX_FB_CHANNEL_NUM];
    wd_status_t	wd_status[MAX_FB_CHANNEL_NUM];

    // extern WD
    float      custom_leak_detect_result;
    custom_leak_detect_status_t custom_leak_detect_status;

    // prompt
    float fb_energy_band1;
    float fb_energy_band2;
    float fb_energy_band3;
    prompt_leak_detect_status_t prompt_leak_detect_status;

    // Special result
    // WSD
    short   wsd_flag;
    short   wsd_trigger_probability;
    short   wsd_trigger_count;

    //wind
    int wind_temporaray_status;

    //fir_anc_open_leak
    uint32_t fir_anc_wear_leak_changed;
} AncAssistRes;

struct AncAssistState_;
typedef struct AncAssistState_ AncAssistState;

typedef enum {
    ANC_ASSIST_NOTIFY_FREQ,
    ANC_ASSIST_NOTIFY_QTY
} anc_assist_notify_t;
typedef void (*anc_assist_notify_func_t)(anc_assist_notify_t msg, void *data, uint32_t value);

static POSSIBLY_UNUSED inline bool any_of_algo_status(anc_assist_algo_status_t *status, uint32_t len, anc_assist_algo_status_t expected_status)
{
    for (uint32_t i = 0; i < len; i++) {
        if (status[i] == expected_status)
            return true;
    }

    return false;
}

static POSSIBLY_UNUSED bool any_of_algo_id(anc_assist_algo_id_t *id, uint32_t num, anc_assist_algo_id_t expected_id)
{
    for (uint32_t i = 0; i < num; i++) {
        if (id[i] == expected_id)
            return true;
    }

    return false;
}

static POSSIBLY_UNUSED inline bool any_of_u32(uint32_t *status, uint32_t len, uint32_t expected_status)
{
    for (uint32_t i = 0; i < len; i++) {
        if (status[i] == expected_status)
            return true;
    }

    return false;
}

AncAssistState *anc_assist_create(int sample_rate, int sample_bits, int ch_num, int frame_size, const AncAssistConfig *cfg, anc_assist_notify_func_t callback);
int32_t anc_assist_destroy(AncAssistState *st);
AncAssistRes anc_assist_process(AncAssistState *st, float **ff_mic, uint8_t ff_ch_num, float **fb_mic, uint8_t fb_ch_num, float **talk_mic, uint8_t talk_ch_num, float **ref, uint8_t ref_ch_num, uint32_t frame_len);
int32_t anc_assist_set_cfg_sync(AncAssistState *st, AncAssistConfig *cfg);
int32_t anc_assist_set_cfg(AncAssistState *st, AncAssistConfig *cfg);

void anc_assist_set_anc_working_status(AncAssistState *st, int32_t on);
void anc_assist_reset_pilot_state(AncAssistState *st);
int32_t anc_assist_pilot_get_play_data(AncAssistState *st, float *pcm_buf, uint32_t pcm_len);
int32_t anc_assist_pilot_set_play_fadeout(AncAssistState *st);
int32_t anc_assist_pilot_set_play_sample_rate(AncAssistState *st, int32_t sample_rate);

int32_t anc_assist_get_required_mips(AncAssistState *st);

float *anc_assist_get_debug_data(AncAssistState *st);

// private function for prompt adaptive anc
int32_t anc_assist_get_prompt_anc_index(AncAssistState *st,int32_t * default_anc_index,int channel_idx, float *band1, float *band2, float *band3);
int32_t anc_assist_set_prompt_anc_calib(AncAssistState *st,int channel_idx, float band1_calib, float band2_calib, float band3_calib);

int32_t anc_assist_auto_eq_selection_set_working_status(AncAssistState *st,int32_t status,int channel_idx);
int32_t anc_assist_auto_eq_selection_get_eq_index(AncAssistState *st,int channel_idx);

int32_t assist_prompt_set_working_status(uint32_t status);
int32_t anc_assist_algo_reset_impl(AncAssistState *st, AncAssistConfig *cfg);
void anc_assist_set_custom_leak_working_status(int32_t status);
#ifdef __cplusplus
}
#endif

#endif

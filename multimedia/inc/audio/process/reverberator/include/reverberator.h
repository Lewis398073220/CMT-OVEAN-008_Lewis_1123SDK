#ifndef AUDIO_REVERBERATOR_H
#define AUDIO_REVERBERATOR_H

#include <stdint.h>
#include "custom_allocator.h"


// #ifdef __cplusplus
// extern "C" {
// #endif

#define MAX_COMB_NUM (8)
#define MAX_ALLPASS_NUM (4)

#define COMB_LEN_MAX (1600)
#define ALLPASS_LEN_MAX (600)
#define DELAY_LEN_MAX (600)

typedef struct {
    int32_t bypass; 

    float wet_gain_dB;
    float room_scale;    /* % */
    float reverberance;   /* % */
    float hf_damping;     /* % */
    float pre_delay_ms;
    int32_t comb_lengths[MAX_COMB_NUM]; 
    int32_t comb_used;
    int32_t allpass_lengths[MAX_ALLPASS_NUM]; 
    int32_t allpass_used;
} ReverberatorConfig;

struct ReverberatorState_;

typedef struct ReverberatorState_ ReverberatorState;


ReverberatorState *reverberator_create(int32_t sample_rate, int32_t frame_size, const ReverberatorConfig *cfg);

ReverberatorState *reverberator_create_with_custom_allocator(int32_t sample_rate, int32_t frame_size, const ReverberatorConfig *cfg,custom_allocator *allocator);

void reverberator_destroy(ReverberatorState *st);

void reverberator_set_config(ReverberatorState *st, const ReverberatorConfig *cfg);

void reverberator_reset_state(ReverberatorState *st);

void reverberator_process(ReverberatorState *st, float *buf, uint32_t frame_len);

// #ifdef __cplusplus
// }
// #endif

#endif

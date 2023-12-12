#ifndef DTLN_DENOISE_H
#define DTLN_DENOISE_H

#include <stdint.h>
#include "arm_math.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    int32_t nfft;
    int32_t in_block_size;

    float *inbuf;
    float *outbuf;
    float *window;
    float *frame_win;

    float *frame;
    float *ft;
    float *mag; 
    int16_t *tmp_buf;
    arm_rfft_fast_instance_f32 rfft;

    // for other module
    float *mask;
    float *noise;
    float *snr;
} DTLNDenoiseState;

DTLNDenoiseState* dtln_denoise_create(uint32_t sample_rate, uint32_t frame_size);

void dtln_denoise_destroy(DTLNDenoiseState *st);

int32_t dtln_denoise_process(DTLNDenoiseState *st, int16_t *pcm_buf, uint32_t pcm_len);

int32_t dtln_denoise_process_frames(DTLNDenoiseState *st, int16_t *pcm_buf, uint32_t pcm_len);

float* dtln_denoise_process_spectrum(DTLNDenoiseState* st, float* ft, int32_t nfft);

float* dtln_denoise_noise_estimate(DTLNDenoiseState* st);

float* dtln_denoise_snr(DTLNDenoiseState* st);

#ifdef __cplusplus
}
#endif

#endif

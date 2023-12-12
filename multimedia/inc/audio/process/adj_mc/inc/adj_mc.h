#ifndef ADJ_MC_H
#define ADJ_MC_H

#ifdef __cplusplus
extern "C" {
#endif
#include "echo_canceller.h"

void adj_mc_init(uint32_t frame_len);
void adj_mc_deinit(void);
int anc_adj_mc_run_mono(uint8_t *buf,int len, enum AUD_BITS_T sample_bit);
uint32_t adj_mc_filter_estimate(uint8_t *buf, uint32_t len);
#ifdef __cplusplus
}
#endif

#endif

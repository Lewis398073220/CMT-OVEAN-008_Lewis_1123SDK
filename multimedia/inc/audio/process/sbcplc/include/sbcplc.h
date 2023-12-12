/********************************************************
SBC Example PLC ANSI-C Source Code
File: sbcplc.h
*****************************************************************************/
#ifndef SBCPLC_H
#define SBCPLC_H

#define LHIST_MAX  3500  //SBC 1203   AAC 3431
/* PLC State Information */
struct PLC_State
{
	float hist[LHIST_MAX];
	short bestlag;
	int nbf;

	short FS;	 	/* Frame Size */
	short N;		/* Window Length for pattern matching */
	short M;		/* Template for matching */
	short LHIST;	/* Length of history buffer required */
	short SBCRT;	/* Reconvergence Time (samples) */
	short OLAL;		/* OverLap-Add Length (samples) */

	//SBCRT + OLAL must be <=FS
};

enum A2DP_PLC_CODEC_TYPE{
    A2DP_PLC_CODEC_TYPE_SBC,
	A2DP_PLC_CODEC_TYPE_AAC,
	A2DP_PLC_CODEC_TYPE_LC3,
};

#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes */
void a2dp_plc_init(struct PLC_State *plc_state, enum A2DP_PLC_CODEC_TYPE type);
void a2dp_plc_lc3_init(struct PLC_State *plc_state, enum A2DP_PLC_CODEC_TYPE type, int size);
void a2dp_plc_bad_frame(struct PLC_State *plc_state, short *ZIRbuf, short *out, float *cos_buf, int len, int stride, int index);
void a2dp_plc_good_frame(struct PLC_State *plc_state, short *in, short *out, float *cos_buf, int len, int stride, int index);

void a2dp_plc_bad_frame_smooth(struct PLC_State *plc_state, short *ZIRbuf, short *out, int stride, int index);
void a2dp_plc_good_frame_smooth(struct PLC_State *plc_state, short *in, short *out, int stride, int index);

void a2dp_plc_bad_frame_24bit(struct PLC_State *plc_state, int *ZIRbuf, int *out, float *cos_buf, int len, int stride, int index);
void a2dp_plc_good_frame_24bit(struct PLC_State *plc_state, int *in, int *out, float *cos_buf, int len, int stride, int index);

void cos_generate(float *cos_buf, short len, short packet_len);

#ifdef __cplusplus
}
#endif

#endif /* SBCPLC_H */
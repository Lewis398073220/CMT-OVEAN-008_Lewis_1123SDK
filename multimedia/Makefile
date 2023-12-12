
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

mmlib_obj :=

mmlib_obj += algorithm/fft/src/

mmlib_obj += audio/process/adp/src/
mmlib_obj += audio/process/anc/src/
mmlib_obj += audio/process/psap/src/
mmlib_obj += audio/process/fir2iir/src/
mmlib_obj += audio/process/common/src/
mmlib_obj += audio/process/drc/src/
mmlib_obj += audio/process/filters/src/
mmlib_obj += audio/process/limiter/src/
mmlib_obj += audio/process/resample/src/
mmlib_obj += audio/process/integer_resampling/src/
mmlib_obj += audio/process/floatlimiter/src/
mmlib_obj += audio/process/adj_mc/src/
mmlib_obj += audio/process/reverb/src/
mmlib_obj += audio/process/sidetone/src/
mmlib_obj += audio/process/sbcplc/src/
mmlib_obj += audio/process/bass_enhancer/src/
mmlib_obj += audio/process/virtual_surround_process/src/
mmlib_obj += audio/process/reverberator/src/
ifeq ($(AUDIO_DOWN_MIXER),1)
mmlib_obj += audio/process/audio_down_mixer/
endif

ifeq ($(FM_ENABLE),1)
mmlib_obj += fm/
endif

#ifneq ($(NO_MULTIMEDIA_CODEC),1)
#mmlib_obj += audio/make/
#endif

mmlib_obj += lib/make/
mmlib_obj += projects/
#mmlib_obj += rbcodec/src/

ifeq ($(SPEECH_LIB),1)
ifneq ($(wildcard $(cur_dir)/speech/src/Makefile),)
mmlib_obj += speech/src/
endif
endif

MULTIMEDIA_LIB_NAME := $(CHIP)_libmultimedia

ifeq ($(CPU),m55)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_m55
endif

ifeq ($(ROM_UTILS_ON),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_romaac
endif

ifeq ($(AAC_REDUCE_SIZE),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_rom_reduce_aac
endif

ifeq ($(A2DP_CP_ACCEL),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_cp
endif

ifeq ($(ANC_APP),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_anc
endif

ifeq ($(USE_TRACE_ID), 1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_log_compressed
endif

ifeq ($(AUDIO_ANC_FIR_HW),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_fir_anc
endif

ifeq ($(LC3_CODEC_ON), 1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_lc3
endif

ifeq ($(A2DP_DECODER_CROSS_CORE_USE_M55), 1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_a2dp_decoder
endif
ifeq ($(GAF_CODEC_CROSS_CORE), 1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_gaf
endif
ifeq ($(LC3PLUS_ENABLE),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_lc3plus
endif

ifeq ($(FREEMAN_ENABLED_STERO), 1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_stero
endif

$(MULTIMEDIA_LIB_NAME)-y := $(mmlib_obj)

rel_src_obj :=
rel_src_obj += inc/audio/process/anc/cfg/
rel_src_obj += inc/audio/process/filters/cfg/
rel_src_obj += inc/audio/process/resample/coef/

obj-y := $(MULTIMEDIA_LIB_NAME).a $(rel_src_obj)

subdir-ccflags-y += \
	-Iutils/heap \
	-Iinclude/cmsis_dsp \
	-Iinclude/cmsis_nn \
	-I$(APPS_DIR)/common \


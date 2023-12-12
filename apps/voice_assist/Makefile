cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

ifeq ($(ANC_ASSIST_ENABLED),1)
obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
	-Imultimedia/inc/speech/inc \
	-Imultimedia/speech/src/process/anc_assist/ \
	-Imultimedia/inc/audio/process/resample/include \
	-Imultimedia/inc/audio/process/adj_mc/inc \
	-Iservices/audio_dump/include \
	-Iinclude/cmsis_dsp \
	-Imultimedia/inc/audio/process/anc/include \
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/include \
	-Iservices/app_ai/inc \
	-Iapps/anc_assist/src \
	-Iapps/anc_assist/inc \
	-Iapps/anc_assist/dsp_src \
	-Iapps/dsp_m55
endif

ifeq ($(ANC_ALGO_DSP),1)

obj_c := $(filter-out %app_voice_assist_adaptive_eq.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_adj_eq.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_ai_voice.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_custom_leak_detect.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_oneshot_adapt_anc.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_pilot_anc.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_prompt_leak_detect.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_ultrasound.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_wd.c,$(obj_c))
obj_c := $(filter-out %assist_ai_voice_kws.c,$(obj_c))
obj_c := $(filter-out %assist_ai_voice_record.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_noise_adapt_anc.c,$(obj_c))
obj_c := $(filter-out %app_voice_assist_optimal_tf.c,$(obj_c))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)
obj-y += ../../apps/anc_assist/src/app_anc_assist_cfg_default.o
obj-y += ../../apps/anc_assist/src/app_anc_assist_thirdparty.o

ifneq ($(ANC_ASSIST_SAMPLE_RATE),)
CFLAGS_app_voice_assist_anc.o += -DANC_ASSIST_SAMPLE_RATE=$(ANC_ASSIST_SAMPLE_RATE)
endif

subdir-ccflags-y += \
    -Imultimedia/inc/speech/inc \
    -Iapps/anc_assist/inc \
    -Iapps/anc_assist/dsp_src \
    -Iplatform/hal_trace \
    -Imultimedia/inc/audio/process/filters/include \
    -Itests/dsp_m55 \
    -Iapps/anc_assist/src \
    -Iapps/anc/inc \
    -Iapps/anc/src \
    -Iapps/voice_assist/inc \
    -Iapps/dsp_m55\
    -Imultimedia/inc/speech/inc \
    -Imultimedia/speech/src/process/anc_assist/ \
    -Imultimedia/inc/audio/process/resample/include \
    -Imultimedia/audio/process/adj_mc/inc \
    -Iservices/audio_dump/include \
    -Iinclude/cmsis_dsp \
    -Imultimedia/inc/audio/process/anc/include \
    -Iservices/nv_section/aud_section \
    -Iservices/nv_section/include \
    -Iservices/app_ai/inc \
    -Iapps/common \
    -Iutils/heap
endif
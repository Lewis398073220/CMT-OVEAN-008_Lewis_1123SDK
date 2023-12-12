cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))

ifneq ($(ANC_ASSIST_SAMPLE_RATE), 16000)
obj-y += ../src/assist/anc_assist_resample.c
endif

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

subdir-ccflags-y += \
    -Iplatform/hal_trace \
    -Imultimedia/inc/speech/inc \
    -Imultimedia/inc/audio/process/integer_resampling/include \
    -Iapps/common \
    -Iapps/anc/inc \
    -Iapps/app_rpc \
    -Iapps/app_rpc/rpc_bth_dsp \
    -Iapps/app_rpc/rpc_m55_dsp \
    -Iapps/anc_assist/inc \
    -Iapps/anc_assist/src \
    -Iapps/anc_assist/src/assist \
    -Iapps/dsp_m55 \
    -Iapps/sensorhub \
    -Iapps/voice_assist/inc \
    -Iapps/anc/src/assist \
    -Iapps/voice_assist/src \
    -Iutils/list \
    -Iutils/heap

ifneq ($(ANC_ASSIST_SAMPLE_RATE),)
CFLAGS_anc_assist_dsp.o += -DANC_ASSIST_SAMPLE_RATE=$(ANC_ASSIST_SAMPLE_RATE)
endif


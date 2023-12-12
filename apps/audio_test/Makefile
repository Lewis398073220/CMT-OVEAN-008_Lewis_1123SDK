
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iservices/audio_bt \
    -Iservices/bone_sensor \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iservices/resources \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iutils/retention_ram \
    -Iutils/stream_mcps/ \
    -Iutils/signal_generator \
    -Iutils/local_pcm \
    -Iapps/anc/inc \
    -Iservices/audio_manager \
    -Imultimedia/inc/audio/process/anc/include \
    -Iservices/nv_section/include \
    -Iservices/nv_section/aud_section

ifeq ($(SPEECH_ALGO_DSP_TEST),1)
CFLAGS_audio_test_speech_algo_dsp_test.o += -DSPEECH_ALGO_DSP_TEST
ccflags-y += -Iservices/speech_algo_dsp_test
endif

ifneq ($(APP_MCPP_CLI),)
ccflags-y += -Iservices/mcpp \
    -Iutils/kfifo
endif

ifeq ($(DMA_AUDIO_APP),1)
ccflags-y += -Iservices/dma_audio/app
CFLAGS_audio_test_cmd.o += -DDMA_AUDIO_APP
endif

ifeq ($(AUDIO_DYNAMIC_BOOST),1)
CFLAGS_audio_test_cmd.o += -D__AUDIO_DYNAMIC_BOOST__
endif

ifeq ($(PSAP_SW_APP),1)
ccflags-y += -Imultimedia/inc/audio/process/limiter/include \
             -Iapps/psap_sw \
             -Imultimedia/inc/speech/inc \
             -Imultimedia/inc/audio/process/anc/include
endif

ifeq ($(PSAP_SW_ALGO),thirdparty)
ccflags-y += -DPSAP_SW_THIRDPARTY
endif

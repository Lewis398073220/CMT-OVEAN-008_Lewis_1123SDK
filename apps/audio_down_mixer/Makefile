cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

obj-y += dsp/audio_down_mixer_dsp.o

ccflags-y += \
	-Iservices/dma_audio/dsp \
	-Iservices/dma_audio/common \
	-Iplatform/drivers/stream_dma_rpc \
	-Imultimedia/inc/audio/process/audio_down_mixer

AUDIO_DOWN_MIXER_PROC_V2       ?= 1
AUDIO_DOWN_MIXER_DEBUG         ?= 0
AUDIO_DOWN_MIXER_USE_TEST_DATA ?= 0

ifeq ($(AUDIO_DOWN_MIXER_DEBUG),1)
ccflags-y += -DAUDIO_DOWN_MIXER_DEBUG
endif

ifeq ($(AUDIO_DOWN_MIXER_USE_TEST_DATA),1)
ccflags-y += -DAUDIO_DOWN_MIXER_USE_TEST_DATA
endif

ifeq ($(AUDIO_DOWN_MIXER_PROC_V2),1)
ccflags-y += -DAUDIO_DOWN_MIXER_PROC_V2
endif

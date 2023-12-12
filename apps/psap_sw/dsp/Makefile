cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

ifeq ($(PSAP_SW_ALGO),)
PSAP_SW_ALGO := bes
endif
$(info PSAP_SW_ALGO is $(PSAP_SW_ALGO))
obj-y += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)$(PSAP_SW_ALGO)/*.c))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	-Iplatform/drivers/dsp_m55 \
	-Iplatform/drivers/stream_dma_rpc \
	-Iservices/communication \
	-Iservices/audioflinger \
	-Iutils/hwtimer_list \
	-Iutils/heap \
	-Iapps/dsp_m55 \
	-Iapps/psap_sw \
	-Iapps/common \
	-Itests/dsp_m55 \
	-Iservices/dma_audio/dsp \
	-Iservices/dma_audio/common \
	-Iutils/stream_mcps \
	-Imultimedia/inc/audio/process/resample/include \
	-Imultimedia/inc/audio/process/filters/include \
	-Imultimedia/inc/audio/process/limiter/include \
	-Imultimedia/inc/audio/process/reverberator/include \
	-Imultimedia/inc/speech/inc \
	-Iservices/audio_dump/include

ifeq ($(PSAP_SW_APP),1)
ccflags-y += -DPSAP_SW_APP

PSAP_SW_BYPASS_FADE_PROCESS ?= 1
ifeq ($(PSAP_SW_BYPASS_FADE_PROCESS),1)
ccflags-y += -DPSAP_SW_BYPASS_FADE_PROCESS
endif
endif

# TODO: Need to clean up.
ifeq ($(DMA_AUDIO_8BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_8BIT
else ifeq ($(DMA_AUDIO_16BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_16BIT
else ifeq ($(DMA_AUDIO_24BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_24BIT
else ifeq ($(DMA_AUDIO_32BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_32BIT
endif

ifeq ($(PSAP_SW_ALGO),thirdparty)
ccflags-y += -DPSAP_SW_THIRDPARTY
endif
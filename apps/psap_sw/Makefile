cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(PSAP_SW_ALGO),)
PSAP_SW_ALGO := bes
endif

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	-Iplatform/drivers/dsp_m55 \
	-Iplatform/drivers/stream_dma_rpc \
	-Iapps/dsp_m55 \
	-Iapps/dsp_hifi4 \
	-Iservices/audioflinger \
	-Iservices/dma_audio/common \
	-Iservices/dma_audio/app \
	-Imultimedia/inc/audio/process/limiter/include \
	-Imultimedia/inc/speech/inc \
	-Iplatform/drivers/hifi4 \
	-Iapps/app_rpc \
	-Iapps/app_rpc/rpc_bth_dsp \
	-Imultimedia/inc/audio/process/reverberator/include \

ifeq ($(DMA_AUDIO_APP),1)
ccflags-y += -DDMA_AUDIO_APP
endif

ifeq ($(DMA_AUDIO_APP_DYN_ON),1)
ccflags-y += -DDMA_AUDIO_APP_DYN_ON
endif

ifeq ($(DMA_AUDIO_DBG_SYNC),1)
ccflags-y += -DDMA_AUDIO_DBG_SYNC
endif

ifeq ($(DMA_RPC_CLI),1)
ccflags-y += -DDMA_RPC_CLI
endif

ifeq ($(CODEC_SW_SYNC),1)
ccflags-y += -DCODEC_SW_SYNC
endif

ifeq ($(DMA_AUDIO_INIT_LOOP_ON),1)
ccflags-y += -DDMA_AUDIO_INIT_LOOP_ON
endif

ifneq ($(AUD_DATA_MEMCPY_SAMP_INC),)
ccflags-y += -DAUD_DATA_MEMCPY_SAMP_INC=$(AUD_DATA_MEMCPY_SAMP_INC)
endif

ifeq ($(ANC_APP),1)
ccflags-y += -DANC_APP
endif

ifeq ($(CODEC_SW_SYNC),1)
ccflags-y += -DCODEC_SW_SYNC
endif

ifeq ($(DMA_RPC_SVR_CORE),HIFI)
ccflags-y += -DDMA_RPC_SVR_CORE_HIFI
else ifeq ($(DMA_RPC_SVR_CORE),CP)
ccflags-y += -DDMA_RPC_SVR_CORE_CP
else ifeq ($(DMA_RPC_SVR_CORE),SENS)
ccflags-y += -DDMA_RPC_SVR_CORE_SENS
else
ccflags-y += -DDMA_RPC_SVR_CORE_M55
endif

ifeq ($(PSAP_SW_ALGO),thirdparty)
ccflags-y += -DPSAP_SW_THIRDPARTY
endif

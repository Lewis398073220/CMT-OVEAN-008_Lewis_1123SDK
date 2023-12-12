cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))

ifeq ($(DMA_AUDIO_APP)-$(DMA_RPC_CLI),1-1)
ccflags-y += -DDMA_AUDIO_APP \
	-Iplatform/drivers/stream_dma_rpc \
	-Iservices/dma_audio/app \
	-Iservices/dma_audio/common
endif

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += -Iutils/cqueue -Iutils/heap -Iplatform/hal \
	-Iplatform/drivers/ana \
	-Iapps/common \
	-Iservices/audioflinger \
	-Iservices/overlay \
	-Iservices/bt_app \
	$(BLUETOOTH_ADAPTER_INCLUDES) \
	-Imultimedia/inc/audio/codec/sbc/inc \
	-Iutils/crc32 \
	-Iplatform/drivers/dsp_m55 \
	-Itests/dsp_m55 \
    -Iapps/anc_assist/inc \
    -Iapps/anc_assist/src

ifeq ($(CORE_BRIDGE_NO_PING_MSG),1)
ccflags-y += -DCORE_BRIDGE_NO_PING_MSG
endif


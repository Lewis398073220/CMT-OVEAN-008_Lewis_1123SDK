cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += -Iplatform/hal \
	-Iplatform/drivers/dsp_m55 \
	-Iplatform/drivers/stream_dma_rpc \

ifeq ($(DMA_RPC_SVR),1)
ccflags-y += -DDMA_RPC_SVR
endif

ifeq ($(DMA_RPC_CLI),1)
ccflags-y += -DDMA_RPC_CLI
endif

ifeq ($(DMA_RPC_DEBUG),1)
ccflags-y += -DDMA_RPC_DEBUG
endif

ifeq ($(DMA_RPC_STREAM_API),1)
ccflags-y += -DDMA_RPC_STREAM_API
endif

DMA_RPC_SYNC_FLAG_ENABLE ?= 1
ifeq ($(DMA_RPC_SYNC_FLAG_ENABLE),1)
ccflags-y += -DDMA_RPC_SYNC_FLAG_ENABLE
endif

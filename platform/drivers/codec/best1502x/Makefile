cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(DMA_RPC_CLI),1)
CFLAGS_codec_$(CHIP).o += -DDMA_RPC_CLI
endif

CFLAGS_codec_$(CHIP).o += $(CODEC_INT_CFG_FLAGS)
CFLAGS_codec_vad_$(CHIP).o += $(CODEC_INT_CFG_FLAGS)


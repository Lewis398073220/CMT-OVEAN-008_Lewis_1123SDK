cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
    -Iplatform/hal \
    -Iutils/cqueue \
    -Iapps/app_rpc \
    -Iplatform/drivers/hifi4

ccflags-y += -DAPP_RPC_M55_DSP_EN

ifeq ($(DSP_HIFI4_TRC_TO_MCU),1)
CFLAGS_rpc_bth_dsp.o += -Iplatform/drivers/hifi4 -DDSP_HIFI4_TRC_TO_MCU
endif

ifeq ($(RMT_TRC_IN_MSG_CHAN),1)
CFLAGS_rpc_bth_dsp.o += -DRMT_TRC_IN_MSG_CHAN
endif
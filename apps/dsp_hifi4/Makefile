cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	-Iplatform/hal \
	-Iplatform/drivers/hifi4 \
	-Iapps/common \
	-Iapps/app_rpc \
	-Iapps/app_rpc/rpc_bth_dsp \
	-Iapps/app_rpc/rpc_m55_dsp

ifeq ($(APP_RPC_BTH_DSP_EN),1)
ccflags-y += -DAPP_RPC_BTH_DSP_EN
endif

ifeq ($(APP_RPC_M55_DSP_EN),1)
ccflags-y += -DAPP_RPC_M55_DSP_EN
endif
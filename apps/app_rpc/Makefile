cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := app_rpc_api.o rpc_queue.o rpc_rx_thread.o

APP_RPC_TEST ?= 0
ifeq ($(APP_RPC_TEST),1)
obj-y += test/
subdir-ccflags-y += \
    -Iapps/app_rpc/test/
endif

APP_RPC_BTH_DSP_EN ?= 0
ifeq ($(APP_RPC_BTH_DSP_EN),1)
obj-y += rpc_bth_dsp/
endif

APP_RPC_M55_DSP_EN ?= 0
ifeq ($(APP_RPC_M55_DSP_EN),1)
obj-y += rpc_m55_dsp/
endif

APP_RPC_BTH_M55_EN ?= 0
ifeq ($(APP_RPC_BTH_M55_EN),1)
CFLAGS_app_rpc_api.o += -DAPP_RPC_BTH_M55_EN
endif

APP_RPC_MCU_SENSOR_EN ?= 0
ifeq ($(APP_RPC_MCU_SENSOR_EN),1)
CFLAGS_app_rpc_api.o += -DAPP_RPC_MCU_SENSOR_EN
endif

subdir-ccflags-y += \
    -Iservices/rpc/inc \
    -Iplatform/hal \
    -Iutils/cqueue \
    -Iapps/dsp_m55 \
    -Iapps/sensorhub \
    -Iapps/app_rpc/rpc_bth_dsp \
    -Iapps/app_rpc/rpc_m55_dsp

ifeq ($(DSP_M55_TRC_TO_MCU),1)
CFLAGS_rpc_bth_m55.o += -Iplatform/drivers/dsp_m55 -DDSP_M55_TRC_TO_MCU
endif

ifeq ($(DSP_HIFI4_TRC_TO_MCU),1)
CFLAGS_rpc_bth_dsp.o += -Iplatform/drivers/hifi4 -DDSP_HIFI4_TRC_TO_MCU
endif

ifeq ($(RMT_TRC_IN_MSG_CHAN),1)
subdir-ccflags-y += -DRMT_TRC_IN_MSG_CHAN
endif



cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

mcpp_core_obj = mcpp_core_bth.c mcpp_core_m55.c mcpp_core_sens.c mcpp_core_cp_subsys.c
obj_c := $(filter-out $(mcpp_core_obj),$(obj_c))

ifeq ($(APP_MCPP_CLI),BTH)
obj_c += mcpp_core_bth.c
else ifeq ($(APP_MCPP_CLI),M55)
obj_c += mcpp_core_m55.c
else ifeq ($(APP_MCPP_CLI),SENS)
obj_c += mcpp_core_sens.c
else ifeq ($(APP_MCPP_CLI),CP_SUBSYS)
obj_c += mcpp_core_cp_subsys.c
endif

ifneq ($(RTOS),1)
mcpp_cli_obj = app_mcpp.c mcpp_client.c
obj_c := $(filter-out $(mcpp_cli_obj),$(obj_c))
endif

ifeq ($(MCPP_TRIGGER_DSP_PROCESS),1)
CFLAGS_mcpp_client.o += -DMCPP_TRIGGER_DSP_PROCESS
endif

ifneq ($(APP_MCPP_SRV),)
CFLAGS_mcpp_server.o += -DAPP_MCPP_SRV
ifeq ($(MCPP_TRIGGER_DSP_PROCESS),1)
CFLAGS_mcpp_server.o += -DMCPP_TRIGGER_DSP_PROCESS
endif
ALGO_PROCESS := $(APP_MCPP_SRV)
$(info ALGO_PROCESS is $(ALGO_PROCESS))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)$(ALGO_PROCESS)/*.c))
subdir-ccflags-y += -Iservices/mcpp/$(ALGO_PROCESS)
endif

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y := $(src_obj)

subdir-ccflags-y += -Iplatform/hal \
                    -Iapps/common \
                    -Iapps/dsp_m55 \
                    -Iapps/dsp_hifi4 \
                    -Iapps/sensorhub \
                    -Iapps/app_rpc \
                    -Iapps/app_rpc/rpc_bth_dsp \
                    -Iapps/app_rpc/rpc_m55_dsp \
                    -Imultimedia/inc/speech/inc \
                    -Imultimedia/speech/src/process/common/inc \
                    -Iplatform/hifi/include/ \
                    -Iutils/heap \
                    -Iutils/kfifo \
                    -Iutils/stream_mcps \
                    -Iinclude/cmsis_dsp \
                    -Iservices/norflash_api \
                    -Iplatform/drivers/hifi4 \
                    -Iplatform/drivers/cp_subsys

ifeq ($(APP_MCPP_BTH_M55),1)
subdir-ccflags-y += -DAPP_MCPP_BTH_M55
endif

ifeq ($(APP_MCPP_BTH_HIFI),1)
subdir-ccflags-y += -DAPP_MCPP_BTH_HIFI
endif

ifeq ($(APP_MCPP_BTH_SENS),1)
subdir-ccflags-y += -DAPP_MCPP_BTH_SENS
endif

ifeq ($(APP_MCPP_M55_HIFI),1)
subdir-ccflags-y += -DAPP_MCPP_M55_HIFI
endif

ifeq ($(APP_MCPP_BTH_CP_SUBSYS),1)
subdir-ccflags-y += -DAPP_MCPP_BTH_CP_SUBSYS
endif

ifeq ($(APP_MCPP_BTH_CP_SUBSYS_NO_RTOS),1)
subdir-ccflags-y += -DAPP_MCPP_BTH_CP_SUBSYS_NO_RTOS
endif

ifeq ($(APP_MCPP_BTH_HIFI_NO_RTOS),1)
subdir-ccflags-y += -DAPP_MCPP_BTH_HIFI_NO_RTOS
endif

ifeq ($(DSP_HIFI4_TRC_TO_MCU),1)
CFLAGS_mcpp_core_bth.o += -DDSP_HIFI4_TRC_TO_MCU
ifeq ($(RMT_TRC_IN_MSG_CHAN),1)
CFLAGS_mcpp_core_bth.o += -DRMT_TRC_IN_MSG_CHAN
endif
endif
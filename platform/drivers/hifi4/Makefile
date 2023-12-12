
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(DSP_COMBINE_BIN),1)
obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
endif
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
-Iutils/boot_struct/ \
-Iplatform/drivers/rmt_trace \
-Iplatform/drivers/bt \
-Iapps/app_rpc

ifeq ($(DSP_COMBINE_BIN),1)
CFLAGS_dsp_loader.o += -DDSP_COMBINE_BIN
endif
ifeq ($(DSP_DEBUG_PORT_MAPOUT),1)
CFLAGS_dsp_loader.o += -DDSP_DEBUG_PORT_MAPOUT
endif

ifeq ($(DSP_HIFI4_TRC_TO_MCU),1)
TRC_FLAGS := -DDSP_HIFI4_TRC_TO_MCU
ifeq ($(RMT_TRC_IN_MSG_CHAN),1)
TRC_FLAGS += -DRMT_TRC_IN_MSG_CHAN
endif
CFLAGS_rx_dsp_hifi4_trc.o += $(TRC_FLAGS) -Iplatform/drivers/rmt_trace
CFLAGS_dsp_loader.o += $(TRC_FLAGS)
endif

ifeq ($(DSP_COMBINE_BIN),1)
DSP_TGT ?= hifi4
$(obj)/dsp_hifi4_bin.o : out/$(DSP_TGT)/$(DSP_TGT).bin
AFLAGS_dsp_hifi4_bin.o += -DDSP_HIFI4_BIN_NAME=$(DSP_TGT).bin -Wa,-Iout/$(DSP_TGT)
endif

ifeq ($(HIFI_DISABLE_CACHE),1)
CFLAGS_dsp_loader.o += -DHIFI_DISABLE_CACHE
endif

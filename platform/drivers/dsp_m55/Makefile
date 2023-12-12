
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ifeq ($(DSP_M55_TRC_TO_MCU),1)
TRC_FLAGS := -DDSP_M55_TRC_TO_MCU
ifeq ($(RMT_TRC_IN_MSG_CHAN),1)
TRC_FLAGS += -DRMT_TRC_IN_MSG_CHAN
endif
CFLAGS_rx_dsp_m55_trc.o += $(TRC_FLAGS) -Iplatform/drivers/rmt_trace
CFLAGS_dsp_m55.o += $(TRC_FLAGS)
endif

ifeq ($(DSP_M55_FLASH_BOOT),1)
CFLAGS_dsp_m55.o += -DDSP_M55_FLASH_BOOT
INCLUDE_DSP_M55_BIN := 0
ifeq ($(DSP_M55_ALT_BOOT_FLASH),1)
CFLAGS_dsp_m55.o += -DDSP_M55_ALT_BOOT_FLASH
endif
ifneq ($(DSP_M55_CODE_OFFSET),)
CFLAGS_dsp_m55.o += -DDSP_M55_CODE_OFFSET=$(DSP_M55_CODE_OFFSET)
endif
else ifneq ($(DSP_M55_LOAD_FLASH_ADDR),)
CFLAGS_dsp_m55.o += \
	-DDSP_M55_LOAD_FLASH_ADDR=$(DSP_M55_LOAD_FLASH_ADDR) \
	-DDSP_M55_LOAD_FLASH_SIZE=$(DSP_M55_LOAD_FLASH_SIZE)
INCLUDE_DSP_M55_BIN := 0
endif
INCLUDE_DSP_M55_BIN ?= 1

ifeq ($(INCLUDE_DSP_M55_BIN),1)
DSP_M55_TGT ?= dsp_m55
$(obj)/dsp_m55_bin.o : out/$(DSP_M55_TGT)/$(DSP_M55_TGT).bin
AFLAGS_dsp_m55_bin.o += -DDSP_M55_BIN_NAME=$(DSP_M55_TGT).bin -Wa,-Iout/$(DSP_M55_TGT)
else
obj-y := $(filter-out dsp_m55_bin.o,$(obj-y))
endif

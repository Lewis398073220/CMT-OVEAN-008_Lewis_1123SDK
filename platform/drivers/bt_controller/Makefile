
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iplatform/drivers/bt
	
ifeq ($(BTC_TRC_TO_SYS),1)
CFLAGS_rx_btc_trc.o += -Iplatform/drivers/rmt_trace -DBTC_TRC_TO_SYS
CFLAGS_bt_controller.o += -DBTC_TRC_TO_SYS
endif

ifeq ($(BT_RAMRUN_NEW),1)
$(obj)/bt_controller_bin.o : $(cur_dir)../bt/$(CHIP)/$(CHIP)_ramrun_fw.bin
AFLAGS_bt_controller_bin.o += -DBT_CONTROLLER_BIN_NAME=$(cur_dir)../bt/$(CHIP)/$(CHIP)_ramrun_fw.bin
else
BT_CONTROLLER_TGT ?= bt_controller
$(obj)/bt_controller_bin.o : out/$(BT_CONTROLLER_TGT)/$(BT_CONTROLLER_TGT).bin
AFLAGS_bt_controller_bin.o += -DBT_CONTROLLER_BIN_NAME=$(BT_CONTROLLER_TGT).bin -Wa,-Iout/$(BT_CONTROLLER_TGT)
endif

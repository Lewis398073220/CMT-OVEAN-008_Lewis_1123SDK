cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(FLASH_SECURITY_REGISTER),1)
CFLAGS_tz_otp_handler_demo.o += -DFLASH_SECURITY_REGISTER
endif


cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

subdir-ccflags-y += \
    -Iplatform/drivers/ana \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iinclude

ifeq ($(IBRT),1)

ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_core/inc
endif
endif

ifeq ($(NO_WDT),1)
CFLAGS_app_watchdog.o += -DNO_WDT
endif

ifneq ($(APP_THREAD_STACK_SIZE),)
CFLAGS_app_thread.o += -DAPP_THREAD_STACK_SIZE=$(APP_THREAD_STACK_SIZE)
endif

ifeq ($(APP_UART_MODULE),1)
obj-y += uart_dma/
endif

ifeq ($(APP_CHIP_BRIDGE_MODULE),1)
obj-y += chip_bridge/
endif
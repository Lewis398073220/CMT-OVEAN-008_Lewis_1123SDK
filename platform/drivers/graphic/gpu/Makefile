cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

ifeq ($(TEST_WITH_FS), 1)
obj_c += sfs/
endif

obj_c += drv/vglite/

ifeq ($(GPU_TESTCASE), 1)
obj_c += testcase/tst/
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ifeq ($(TEST_WITH_FS), 1)
subdir-ccflags-y := -DTEST_WITH_FS
endif

ifeq ($(CHIP),best1600)
subdir-ccflags-y := -DAHB_READ_SPLIT
endif

subdir-ccflags-y += \
				   -Iplatform/drivers/graphic/gpu/ \
				   -Iplatform/drivers/graphic/gpu/drv/vglite/inc \
				   -Iutils/heap/ \
				   -Iplatform/drivers/graphic/gpu/drv/vglite/VGLiteKernel/ \
				   -Iplatform/drivers/graphic/gpu/drv/vglite/VGLiteKernel/rtos/ \
				   -Irtos/ \
				   -Irtos/rtx5/rtx_config \
				   -Ithirdparty/user_driver/spi_lcd/st7789 \
				   -Ithirdparty/user_driver/qspi_lcd/gc9b71 \
				   -Ithirdparty/user_driver/qspi_lcd/rm69330


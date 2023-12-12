cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c $(cur_dir)src/*.cpp $(cur_dir)src/*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

LIB_BIN_DIR := thirdparty/lib/cmsis_m55
obj-y += cmsis_dsp_lib_m55.a
obj-y += cmsis_nn_lib.a

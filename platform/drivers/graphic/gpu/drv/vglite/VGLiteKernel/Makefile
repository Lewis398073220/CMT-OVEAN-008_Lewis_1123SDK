
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))



obj_c += rtos/

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y = \
            -Iplatform/drivers/graphic/gpu/drv/vglite/inc/ \
            -Iplatform/drivers/graphic/gpu/drv/vglite/VGLiteKernel/ \
            -Iplatform/drivers/graphic/gpu/drv/vglite/VGLiteKernel/rtos/ \
            -Iinclude/cmsis_dsp/ \



cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y += BasicMathFunctions/
obj-y += CommonTables/
obj-y += ComplexMathFunctions/
obj-y += FilteringFunctions/
obj-y += MatrixFunctions/
obj-y += SupportFunctions/
obj-y += StatisticsFunctions/
obj-y += TransformFunctions/
obj-y += FastMathFunctions/

subdir-ccflags-y := -Iinclude/cmsis_dsp -Iinclude/cmsis_dsp/PrivateInclude
subdir-ccflags-y += -DARM_MATH_LOOPUNROLL -O3 -ffast-math

#there two much warnings in the new version dsp lib
subdir-ccflags-y += -flax-vector-conversions -Wno-float-conversion -Wno-double-promotion
ifeq ($(TOOLCHAIN), armclang)
subdir-ccflags-y += -Wno-uninitialized
else
subdir-ccflags-y += -Wno-maybe-uninitialized
endif

CMSIS_DSP_LIB_NAME := cmsis_dsp_lib_$(CPU)

$(CMSIS_DSP_LIB_NAME)-y := $(obj-y)
ifeq ($(TOOLCHAIN), armclang)
obj-y := $(CMSIS_DSP_LIB_NAME).a
else
# FIXME: If sync master code, build error.
ifeq ($(SCO_CP_ACCEL),1)
cmsis_dsp_lib-y := $(obj-y)
obj-y := cmsis_dsp_lib.o
endif
endif

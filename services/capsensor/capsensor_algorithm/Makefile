cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

CAP_CALCULATE_LIB_NAME := libcapsensor_algorithm
$(CAP_CALCULATE_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_cpp:.cpp=.o)

obj-y += \
    $(CAP_CALCULATE_LIB_NAME).a

ccflags-y +=  \
    -Iconfig/sensor_hub/ \
    -Iplatform/drivers/capsensor/ \
    -Iplatform/drivers/capsensor/best1600/ \
    -Iplatform/drivers/capsensor/best1603/

ifeq ($(CAPSENSOR_TRACE_DEBUG),1)
KBUILD_CFLAGS += -DCAPSENSOR_TRACE_DEBUG
endif

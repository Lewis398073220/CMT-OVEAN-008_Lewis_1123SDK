cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))


obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y := \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iplatform/drivers/bt  \
    -Iplatform/hal  \
    -Iservices/bt_app/a2dp_codecs/include 	\
    -Iservices/ux/common/ibrt/ibrt_middleware/inc  \
    -Iutils/cqueue \
    -Iservices/audio_manager

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/custom_api
endif
ifeq ($(IBRT_UI),1)
ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_ui/inc
endif
endif

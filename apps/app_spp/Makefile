cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(CHIP_CAPSENSOR_VER),)
ccflags-y +=  \
    -Iapps/sensorhub/
endif

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
	-Iservices/ux/common/ibrt/ibrt_core/inc/ \
    -Iutils/cqueue \
    -Iutils/hsm \
    -Imultimedia/inc/audio/codec/sbc/inc/ \
    -Iplatform/drivers/bt/ \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ibrt/ibrt_middleware/inc/ \
    -Iservices/audio_manager/

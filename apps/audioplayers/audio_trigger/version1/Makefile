cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	$(BLUETOOTH_ADAPTER_INCLUDES) \
	-Iservices/ux/common/ibrt/ibrt_middleware/inc \
	-Iapps/audioplayers/a2dp_decoder \
	-Iservices/bt_app \
	-Iservices/audio_bt \
	-Iservices/resources \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/list \
    	-Iservices/audio_manager \
        -Ibthost/service/bt_app/inc 

ifeq ($(IBRT_UI),1)
ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_ui/inc
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
ccflags-y += \
    -Iservices/audio_manager
endif
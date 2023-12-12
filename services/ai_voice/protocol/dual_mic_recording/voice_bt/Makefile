cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
	-Iservices/ai_voice/transport \
	-Iservices/ai_voice/manager \
	$(BLUETOOTH_ADAPTER_INCLUDES) \
	-Iservices/app_ai/inc \
	-Iservices/bt_app \
	-Imultimedia/inc/audio/codec/sbc/inc \
	-Iutils/cqueue

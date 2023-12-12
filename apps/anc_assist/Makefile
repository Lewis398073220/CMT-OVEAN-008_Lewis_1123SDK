
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/assist/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

ifeq ($(CHIP_HAS_ANC_HW_GAIN_SMOOTHING),1)
CFLAGS_anc_assist_anc.o += -DANC_HW_GAIN_SMOOTHING
endif

ifeq ($(ANC_ASSIST_PROCESS_THREAD),1)
CFLAGS_app_anc_assist.o += -DANC_ASSIST_PROCESS_THREAD
CFLAGS_anc_assist_thread.o += -DANC_ASSIST_PROCESS_THREAD
endif

ifeq ($(ANC_ASSIST_USE_INT_CODEC),1)
CFLAGS_app_anc_assist.o += -DANC_ASSIST_USE_INT_CODEC
endif

ifeq ($(ANC_ASSIST_PILOT_TONE_ALWAYS_ON),1)
CFLAGS_app_anc_assist.o += -DANC_ASSIST_PILOT_TONE_ALWAYS_ON
endif

ifeq ($(ANC_ASSIST_ALGO_ON_DSP),1)
ccflags-y += -DANC_ASSIST_ALGO_ON_DSP
endif

ifeq ($(ASSIST_FIR_ANC_OPEN_AGAIN_DETECT),1)
CFLAGS_app_anc_assist.o += -DASSIST_FIR_ANC_OPEN_AGAIN_DETECT
endif

ifeq ($(ANC_ASSIST_TRIGGER_DSP_PROCESS),1)
CFLAGS_app_anc_assist_core.o += -DANC_ASSIST_TRIGGER_DSP_PROCESS
endif

ifneq ($(ANC_ASSIST_SAMPLE_RATE),)
CFLAGS_app_anc_assist.o += -DANC_ASSIST_SAMPLE_RATE=$(ANC_ASSIST_SAMPLE_RATE)
CFLAGS_anc_assist_thread.o += -DANC_ASSIST_SAMPLE_RATE=$(ANC_ASSIST_SAMPLE_RATE)
endif


obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)


subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Imultimedia/inc/audio/process/resample/include \
    -Imultimedia/inc/audio/process/integer_resampling/include \
    -Imultimedia/inc/audio/process/anc/include \
    -Imultimedia/inc/audio/process/psap/include \
    -Imultimedia/inc/speech/inc \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/kfifo \
    -Iapps/anc/inc \
    -Iservices/bone_sensor \
    -Iservices/audio_bt \
    -Iapps/anc_assist/inc \
    -Iapps/anc_assist/dsp_src \
    -Iapps/app_rpc \
    -Iapps/app_rpc/rpc_bth_dsp \
    -Iapps/app_rpc/rpc_m55_dsp \
    -Iapps/voice_assist/inc \
    -Iapps/dsp_m55 \
    -Iapps/dsp_hifi4 \
    -Iapps/voice_detector \
    -Iapps/sensorhub \
    -Imultimedia/audio/codec/sbc/src/inc \
    -Imultimedia/inc/audio/codec/sbc/inc \
    -Imultimedia/inc/speech/inc \
    -Iservices/nv_section/aud_section \
    -Iservices/nv_section/include   \
    -Iutils/hwtimer_list \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iinclude/cmsis_dsp \
    -Iservices/audio_dump/include/ \
    -Iservices/bone_sensor/

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/product/earbud_tws/inc \
    -Iapps/earbuds/conn
endif
endif

ifeq ($(ENABLE_CALCU_CPU_FREQ_LOG),1)
CFLAGS_app_anc_assist.o += -DENABLE_CALCU_CPU_FREQ_LOG
endif

ifeq ($(VOICE_ASSIST_WD_ENABLED),1)
CFLAGS_app_anc_assist.o += -DVOICE_ASSIST_WD_ENABLED
endif

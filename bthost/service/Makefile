cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

obj-y := $(obj_c:.c=.o)

obj-y += common/

ifneq ($(BLE_ONLY_ENABLED),1)
obj-y += bt_app/
ifeq ($(BT_SOURCE),1)
obj-y += bt_source/
endif
endif

ifeq ($(BLE),1)
ifeq ($(BLE_STACK_NEW_DESIGN),1)
obj-y += ble_app_new/
else
obj-y += ble_app/
endif #BLE_STACK_NEW_DESIGN

ifeq ($(BLE_AUDIO_ENABLED),1)
obj-y += ble_audio/aob_app/
ifeq ($(BLE_STACK_NEW_DESIGN),1)
obj-y += ble_audio/blueelf_adapter/
else
obj-y += ble_audio/gaf_core/
endif #BLE_STACK_NEW_DESIGN
endif #BLE_AUDIO_ENABLED

endif #BLE

ccflags-y += -DBLUETOOTH_BT_IMPL
ccflags-y += -DBLUETOOTH_BLE_IMPL

ccflags-y += \
	$(AAC_INCLUDES) \
	-Iinclude/cmsis_dsp \
	-Iservices/voice_dev \
	-Iservices/audio_process \
	-Iservices/audio_dump/include \
	-Iservices/hw_dsp/inc \
	$(BLUETOOTH_ADAPTER_INCLUDES) \
	$(BES_BT_IMPL_INCLUDES) \
	$(BLE_APP_INCLUDES) \
	$(BLE_STACK_INCLUDES) \
	-Imultimedia/inc/speech/inc \
	-Iservices/bone_sensor \
	-Iservices/overlay \
	-Iservices/audio_manager \
	-Iservices/gfps/inc \
	-Iservices/ux/common/ble_audio_core/inc \
	-Ithirdparty/tile/tile_common/tile_storage \
	-Ithirdparty/tile/tile_common/tile_features \
	-Iservices/resources \
	-Imultimedia/inc/rbcodec \
	-Imultimedia/inc/audio/process/resample/include \
	-Imultimedia/inc/audio/process/filters/include \
	-Imultimedia/inc/audio/process/drc/include \
	-Imultimedia/inc/audio/process/anc/include\
	-Imultimedia/inc/audio/process/sidetone/include\
	-Imultimedia/inc/audio/process/adj_mc/inc\
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/userdata_section \
	-Iservices/nv_section/include \
	-Iservices/voicepath/$(VOICE_DATAPATH_TYPE) \
	-Iservices/voicepath/gsound/gsound_target \
	-Iservices/voicepath/gsound/gsound_custom/inc \
	-Iservices/voicepath/gsound/gsound_target_api_read_only \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/cmsis \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/heap \
	-Iutils/crc16 \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/anc/inc \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/ble_audio_test \
	-Iapps/audioplayers/a2dp_decoder \
	-Iapps/battery \
	-Iapps/common \
	-Iapps/factory \
	-Iapps/voice_assist/inc \
	-Iutils/hwtimer_list \
	-Iservices/voicepath \
	-Ithirdparty/userapi \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	-Iservices/ai_voice/manager \
	-Iservices/ai_voice/audio \
	-Iservices/ai_voice/transport \
	-Iservices/app_ai/inc \
	-Iservices/interconnection/red \
	-Iservices/interconnection/green \
	-Iservices/interconnection/umm_malloc \
	-Iservices/ux/common/ibrt/custom_api \
	$(SOURCE_INCLUDES) \
	-Ithirdparty/tile/tile_target \
	-Iservices/osif \
	-Iservices/ux/common/ibrt/ibrt_middleware/inc \
	-Iutils/list \
	-Iutils/string \
	-Iservices/ai_voice/protocol/bixbyvoice \
	-Iservices/ai_voice/protocol/bixbyvoice/bixbyvoice_manager \
	-Iservices/bt_service/inc

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/common/ibrt/ibrt_core/inc
endif
endif

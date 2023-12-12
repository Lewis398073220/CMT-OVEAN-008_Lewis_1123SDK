cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := a2dp_encoder_cc_bth.o

ccflags-y := -Iapps/audioplayers/a2dp_encoder_off_bth \
			-Iservices/audio_bt \
			-Iservices/audio_process \
			-Iservices/app_ai/inc \
			-Iservices/resources \
			-Iservices/bt_app \
			-Iservices/bt_app/a2dp_codecs/include \
			$(BLUETOOTH_ADAPTER_INCLUDES) \
			$(BES_BT_IMPL_INCLUDES) \
			-Iservices/bes_bth/bt/bt_app/ \
			-Iplatform/drivers/uarthci \
			-Iutils/cqueue \
			-Iutils/lockcqueue \
			-Iservices/audio_dump/include \
			-Imultimedia/inc/speech/inc \
			-Imultimedia/inc/rbcodec/inc \
			-Imultimedia/inc/audio/process/eq/include \
			-Imultimedia/inc/audio/process/resample/include \
			-Imultimedia/inc/audio/process/filters/include \
			-Imultimedia/inc/audio/process/sbcplc/include \
			-Imultimedia/inc/fm/inc \
			-Iservices/nv_section/aud_section \
			-Iservices/nv_section/include \
			-Iservices/overlay \
			-Iservices/norflash_api \
			-Iservices/nv_section/log_section \
			-Iapps/main \
			-Iapps/audioplayers/rbplay/ \
			-Iapps/audioplayers/a2dp_decoder \
			-Iutils/list \
			-Iutils/heap \
			-Iplatform/drivers/ana \
			-Iapps/apptester \
			-Iapps/key  \
			-Imultimedia/inc/audio/codec/sbc/inc \
			-Imultimedia/audio/codec/sbc/src/inc \
			-Iplatform/drivers/bt \
			-Iutils/crc32 \
			-Iservices/ai_voice/audio \
			-Iservices/ai_voice/protocol/bixbyvoice \
			-Iservices/ai_voice/protocol/bixbyvoice/bixbyvoice_manager \
			-Iapps/dsp_m55 \
			-Iservices/audio_manager
ccflags-y += -Wno-unused-function
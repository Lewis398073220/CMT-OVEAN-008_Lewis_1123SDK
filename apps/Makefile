obj-y := audioplayers/ common/ main/ key/ pwl/ battery/ factory/ cmd/ mic/ audio_test/ bt_sync/

ifeq ($(CAPSENSOR_SPP_SERVER),1)
obj-y += app_spp/
endif

ifeq ($(APP_TEST_AUDIO),1)
obj-y += apptester/
endif

ifeq ($(BTUSB_AUDIO_MODE),1)
obj-y += usbaudio/
endif
ifeq ($(BT_USB_AUDIO_DUAL_MODE),1)
obj-y += btusbaudio/
obj-y += usbaudio/
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
obj-y += ble_audio_test/
endif

ifeq ($(APP_TEST_SDMMC),1)
obj-y += sdmmc/
endif

ifeq ($(MSD_MODE),1)
obj-y += sdmmc_msd/
endif

ifeq ($(PSAP_SW_APP),1)
obj-y += psap_sw/
endif

ifeq ($(ANC_APP),1)
obj-y += anc/
endif

ifeq ($(ANC_ASSIST_ENABLED),1)
obj-y += anc_assist/
obj-y += voice_assist/
endif

ifeq ($(VOICE_DETECTOR_EN),1)
obj-y += voice_detector/
endif

ifeq ($(SENSOR_HUB),1)
obj-y += sensorhub/
endif

ifeq ($(DSP_HIFI4),1)
obj-y += dsp_hifi4/
endif

ifeq ($(DSP_M55),1)
obj-y += dsp_m55/
endif

ifeq ($(DMA_AUDIO_APP)-$(DMA_RPC_CLI),1-1)
obj-y += ../services/dma_audio/app/
endif

ifeq ($(AUDIO_HEARING_COMPSATN),1)
obj-y += hearing_detec/
endif

_APP_BT_WATCH ?= 0
ifeq ($(BT_WATCH_MASTER),1)
_APP_BT_WATCH := 1
endif
ifeq ($(BT_WATCH_SLAVE),1)
_APP_BT_WATCH := 1
endif
ifeq ($(_APP_BT_WATCH),1)
obj-y += btwatch/
endif

ifeq ($(APP_RPC_ENABLE),1)
obj-y += app_rpc/
endif

ifeq ($(APP_SOUND_ENABLE),1)
obj-y += sound/
endif
ifeq ($(BLE_WALKIE_TALKIE),1)
obj-y += ble_walkie_talkie/
endif

ifeq ($(HEAD_TRACK_ENABLE),1)
obj-y += head_track_3dof/
endif

ifeq ($(WIRELESS_MIC),1)
obj-y += wireless_mic/
subdir-ccflags-y += -DWIRELESS_MIC
endif

# include earbud in default
ifneq ($(BT_WATCH_APP),1)
obj-y += earbuds/
endif

export APP_SPP_DEMO ?= 0
ifeq ($(APP_SPP_DEMO),1)
obj-y += example/app_spp/
endif

export BLE_AUDIO_CENTRAL_APP_ENABLED ?= 0
ifeq ($(BLE_AUDIO_CENTRAL_APP_ENABLED),1)
obj-y += ble_audio_central_app/
endif

subdir-ccflags-y += -Iapps/apptester \
					-Iapps/audioplayers \
					-Iapps/anc/inc \
					-Iapps/anc_assist/inc \
					-Iapps/voice_assist/inc \
					-Iapps/common \
					-Iapps/sdmmc \
					-Iapps/sdmmc_msd \
					-Iapps/main \
					-Iapps/cmd \
					-Iapps/key \
					-Iapps/pwl \
					-Iapps/battery \
					-Iservices/voicepath \
					-Iservices/audio_dump/include \
					$(BLUETOOTH_ADAPTER_INCLUDES) \
					-Iutils/list \
					-Iutils/heap \
					-Iservices/audio_manager \
					-Imultimedia/inc/audio/process/filters/include \
					-Ibthost/service/bt_app/inc

ifeq ($(BT_USB_AUDIO_DUAL_MODE),1)
subdir-ccflags-y += -Iapps/btusbaudio
endif

ifeq ($(A2DP_LDAC_ON),1)
subdir-ccflags-y += -Ithirdparty/audio_codec_lib/ldac/inc
endif

ifeq ($(HEAD_TRACK_ENABLE),1)
subdir-ccflags-y += -Iservices/head_track
endif

ifeq ($(HEAD_TRACK_TOTA_TEST),1)
subdir-ccflags-y += -Iservices/tota_v2
endif

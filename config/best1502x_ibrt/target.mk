CHIP        ?= best1502x

DEBUG       ?= 1

FPGA        ?= 0

RTOS        ?= 1

LIBC_ROM    ?= 1

export LIBC_OVERRIDE ?= 1

#KERNEL      ?= RTX5
#KERNEL      ?= LITEOS_M
VERSION_INFO ?= best2500p_ibrt

export USER_SECURE_BOOT	?= 0
# enable:1
# disable:0

WATCHER_DOG ?= 1

DEBUG_PORT  ?= 1
# 0: usb
# 1: uart0
# 2: uart1

FLASH_CHIP	?= ALL
# GD25Q80C
# GD25Q32C
# ALL

ifeq ($(KERNEL), LITEOS_M)
export OSTICK_USE_FAST_TIMER := 0
KBUILD_CPPFLAGS += -DOS_HEAP_LINK_NAME=m_aucSysMem0 -DOS_HEAP_SIZE=0x80000
KBUILD_CPPFLAGS += -DOS_WRAP_MALLOC
LDFLAGS_IMAGE += --wrap malloc --wrap calloc --wrap free --wrap realloc
endif
export SNYC_FOUND_CHECK_HECERROR ?= 1

export RF_INIT_XTAL_CAP_FROM_NV ?= 1

export BLE_EXT_ADV_TX_PWR_INDEPEND ?= 0

export BT_BLE_USING_SAME_TX_LEVEL ?= 0

export TX_PWR_LOWER_2DBM ?= 0

export AFH_ASSESS ?= 1

export BT_RAMRUN ?= 1

export HW_AGC ?= 1

export NEW_SWAGC_MODE ?= 0

export BLE_NEW_SWAGC_MODE ?= 0

export PACKAG_TYPE ?= 4
# 1: 2600ZP
# 2: 2700IBP
# 3: 2700H
# 4: 2700L

export BT_LOG_POWEROFF ?= 0

export BT_INFO_CHECKER ?= 1

export BT_TEST_CURRENT_KEY ?= 0

export BT_UART_LOG_P16 ?= 0

export BT_UART_LOG ?= 0

export BT_SYSTEM_52M ?= 0

export BES_FA_MODE ?= 0

export LL_MONITOR ?= 0

export SOFTBIT_EN ?= 0

export ACL_DATA_CRC_TEST ?= 0

export FORCE_SIGNALINGMODE ?= 0

export FORCE_NOSIGNALINGMODE ?= 0

export BT_FA_ECC ?= 0

ifeq ($(BT_FA_ECC),1)
export BT_FA_SCO_ECC ?= 0
export BT_FA_ACL_ECC ?= 1
endif

export BT_ECC_USER_DEFINE ?= 0
ifeq ($(BT_ECC_USER_DEFINE),1)
export BT_ECC_CONFIG_BLK ?= 1
export BT_ECC_USER_DEFINE_LENGTH ?= 8 #byte
else
export BT_ECC_CONFIG_BLK ?= 3
endif

export BT_BID_ECC_EN ?= 0

export BT_FAST_LOCK_ENABLE ?= 0

export IBRT_TESTMODE ?= 0

export CONTROLLER_DUMP_ENABLE ?= 1

export CONTROLLER_MEM_LOG_ENABLE ?= 0

export INTERSYS_DEBUG ?= 1

export PROFILE_DEBUG ?= 0

export BTDUMP_ENABLE ?= 0

export BT_DEBUG_TPORTS ?= 0

export TPORTS_KEY_COEXIST ?= 0

export BT_SIGNALTEST_SLEEP_EN ?= 0

export TX_PULLING_CAL ?= 1

export PATCH_SYMBOL_DIR ?= $(srctree)/platform/drivers/bt/best2300a

export LD_USE_PATCH_SYMBOL ?= -Wl,--just-symbols=$(PATCH_SYMBOL_DIR)/patch_symbol_parsed.txt -Wl,--just-symbols=$(PATCH_SYMBOL_DIR)/patch_symbol_parsed_testmode.txt

export DISPLAY_PREFIX_HCI_CMD_EVT_ ?= 0


AUDIO_OUTPUT_MONO ?= 0

AUDIO_OUTPUT_DIFF ?= 0

HW_FIR_EQ_PROCESS ?= 0

SW_IIR_EQ_PROCESS ?= 0

SW_IIR_PROMPT_EQ_PROCESS ?= 0

HW_DAC_IIR_EQ_PROCESS ?= 1

HW_IIR_EQ_PROCESS ?= 0

HW_DC_FILTER_WITH_IIR ?= 0

AUDIO_DRC ?= 0

AUDIO_LIMITER ?= 0

AUDIO_HEARING_COMPSATN ?= 0
ifeq ($(AUDIO_HEARING_COMPSATN),1)
APP_TRACE_RX_ENABLE := 1
HEARING_MOD ?= HWIIR
endif
# HWFIR
# SWIIR
# HWIIR

PC_CMD_UART ?= 0

TOTA_EQ_TUNING ?= 0

AUDIO_SECTION_ENABLE ?= 0

AUDIO_RESAMPLE ?= 1

export RESAMPLE_ANY_SAMPLE_RATE ?= 1

OSC_26M_X4_AUD2BB ?= 1

export SYS_USE_BBPLL ?= 1


AUDIO_OUTPUT_VOLUME_DEFAULT ?= 16

export CALIB_SLOW_TIMER ?= 1

# range:1~16

CODEC_DAC_MULTI_VOLUME_TABLE ?= 0

AUDIO_INPUT_CAPLESSMODE ?= 0

AUDIO_INPUT_LARGEGAIN ?= 0

AUDIO_CODEC_ASYNC_CLOSE ?= 0

AUDIO_SCO_BTPCM_CHANNEL ?= 1

export A2DP_CP_ACCEL ?= 1

export SCO_CP_ACCEL ?= 1

export SCO_TRACE_CP_ACCEL ?= 0

# For TWS SCO DMA snapshot and low delay
export PCM_FAST_MODE ?= 1

export PCM_PRIVATE_DATA_FLAG ?= 0

export CVSD_BYPASS ?= 1

export LOW_DELAY_SCO ?= 0

export SPEECH_TX_24BIT ?= 0

SPEECH_BONE_SENSOR ?= 0

SPEECH_TX_DC_FILTER ?= 1

SPEECH_TX_AEC2FLOAT ?= 0

SPEECH_TX_NS3 ?= 0

SPEECH_TX_2MIC_NS2 ?= 0

SPEECH_TX_2MIC_NS7 ?= 0

SPEECH_TX_2MIC_NS8 ?= 0

SPEECH_TX_NS7 ?= 0

SPEECH_TX_COMPEXP ?= 1

SPEECH_TX_EQ ?= 0

SPEECH_TX_POST_GAIN ?= 0

SPEECH_RX_NS2FLOAT ?= 0

SPEECH_RX_COMPEXP ?= 0

SPEECH_RX_EQ ?= 0

SPEECH_RX_POST_GAIN ?= 0

LARGE_RAM ?= 1

HSP_ENABLE ?= 0

SBC_FUNC_IN_ROM ?= 0

ROM_UTILS_ON ?= 1

APP_LINEIN_A2DP_SOURCE ?= 0

APP_I2S_A2DP_SOURCE ?= 0

VOICE_PROMPT ?= 1

TWS_PROMPT_SYNC ?= 1

REPORT_CONNECTIVITY_LOG ?= 0

export AUDIO_USE_BBPLL := 1

# TOTA1: old tota, TOTA2: new tota(debuging)
export TOTA ?= 0
export TOTA_v2 ?= 0

ifeq ($(TOTA_v2),1)
export TOTA := 0
endif

export APP_RSSI ?=0
ifeq ($(APP_RSSI),1)
export TOTA_v2 := 1
KBUILD_CPPFLAGS += -DGET_PEER_RSSI_ENABLE
endif

GATT_OVER_BR_EDR ?= 0

BES_OTA ?= 1

export FREEMAN_ENABLED_STERO ?= 1
ifeq ($(FREEMAN_ENABLED_STERO),1)
KBUILD_CPPFLAGS += -DFREEMAN_ENABLED_STERO
export IBRT_UI_MASTER_ON_TWS_DISCONNECTED ?=1
export POWER_ON_OPEN_BOX_ENABLED ?= 0
ifeq ($(BES_OTA),1)
export FREEMAN_OTA_ENABLE := 1
endif
endif

export USB_HID_COMMAND_ENABLE ?= 0
ifeq ($(USB_HID_COMMAND_ENABLE),1)
KBUILD_CPPFLAGS += -DUSB_HID_COMMAND_ENABLE
endif

export USB_OTA_ENABLE ?= 0
ifeq ($(USB_OTA_ENABLE),1)
KBUILD_CPPFLAGS += -DUSB_OTA_ENABLE
endif

#Multiple Broadcast Id and code mnanagement support
export MULTI_BID_SUPPORT ?= 0
ifeq ($(MULTI_BID_SUPPORT),1)
KBUILD_CPPFLAGS += -DMULTI_BID_SUPPORT
endif

TILE_DATAPATH_ENABLED ?= 0

CUSTOM_INFORMATION_TILE_ENABLE ?= 0

INTERCONNECTION ?= 0

INTERACTION ?= 0

INTERACTION_FASTPAIR ?= 0

BT_ONE_BRING_TWO ?= 0

DSD_SUPPORT ?= 0

A2DP_EQ_24BIT ?= 1

A2DP_AAC_ON ?= 1

A2DP_SCALABLE_ON ?= 0

A2DP_LHDC_ON ?= 0

ifeq ($(A2DP_LHDC_ON),1)
A2DP_LHDC_V3 ?= 0
endif

A2DP_LDAC_ON ?= 0

export A2DP_LC3_ON ?= 0

export TX_RX_PCM_MASK ?= 0

FACTORY_MODE ?= 1

ENGINEER_MODE ?= 1

ULTRA_LOW_POWER	?= 1

DAC_CLASSG_ENABLE ?= 1

NO_SLEEP ?= 0

CORE_DUMP ?= 1

CORE_DUMP_TO_FLASH ?= 0

export SYNC_BT_CTLR_PROFILE ?= 0

export A2DP_AVDTP_CP ?= 0

export A2DP_DECODER_VER := 2

export AUDIO_TRIGGER_VER := 1

export IBRT ?= 1

export SEARCH_UI_COMPATIBLE_UI_V2 ?= 0

export BLE_USB_AUDIO_SUPPORT ?= 1
ifeq ($(BLE_USB_AUDIO_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_USB_AUDIO_SUPPORT
BTUSB_DUAL_MODE_SUPPORT ?= 1
ifeq ($(BTUSB_DUAL_MODE_SUPPORT),1)
export USB_AUDIO_SEND_MONO ?= 1
export AUDIO_INPUT_MONO ?= 1
export BT_USB_AUDIO_DUAL_MODE = 1
export DELAY_STREAM_OPEN ?= 1
export USB_ISO ?= 1
endif
endif

export RC06_HEADSET_CONNECT_WITH_TB_DONGLE ?= 0
ifeq ($(RC06_HEADSET_CONNECT_WITH_TB_DONGLE),1)
KBUILD_CPPFLAGS += -DRC06_HEADSET_CONNECT_WITH_TB_DONGLE
endif

ifeq ($(BLE_USB_AUDIO_SUPPORT),1)
ifeq ($(AOB_LOW_LATENCY_MODE),1)
KBUILD_CPPFLAGS += -DCODEC_BUFF_FRAME_NUM=10
KBUILD_CPPFLAGS += -DUSB_BUFF_FRAME_NUM=12
else
KBUILD_CPPFLAGS += -DCODEC_BUFF_FRAME_NUM=20
KBUILD_CPPFLAGS += -DUSB_BUFF_FRAME_NUM=40
endif
export BLE_USB_AUDIO_OPTIMIZE_CON_FLOW ?= 1
ifeq ($(BLE_USB_AUDIO_OPTIMIZE_CON_FLOW),1)
KBUILD_CPPFLAGS += -DBLE_USB_AUDIO_OPTIMIZE_CON_FLOW
export BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON ?= 1
ifeq ($(BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON),1)
KBUILD_CPPFLAGS += -DBLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON
endif
endif

else ifeq ($(BLE_I2S_AUDIO_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_I2S_AUDIO_SUPPORT

export DONGLE_AS_I2S0_MASTER ?= 0
ifeq ($(DONGLE_AS_I2S0_MASTER),1)
KBUILD_CPPFLAGS += -DDONGLE_AS_I2S0_MASTER
endif

export WIFI_ALWAYS_AS_SOURCE ?= 1
ifeq ($(WIFI_ALWAYS_AS_SOURCE),1)
KBUILD_CPPFLAGS += -DWIFI_ALWAYS_AS_SOURCE
endif

export HEADSET_AS_TDM0_SLAVE ?= 1
ifeq ($(HEADSET_AS_TDM0_SLAVE),1)
KBUILD_CPPFLAGS += -DHEADSET_AS_TDM0_SLAVE
endif

BTUSB_DUAL_MODE_SUPPORT ?= 0
ifeq ($(BTUSB_DUAL_MODE_SUPPORT),1)
export USB_AUDIO_SEND_MONO ?= 1
export AUDIO_INPUT_MONO ?= 1
export BT_USB_AUDIO_DUAL_MODE = 1
export DELAY_STREAM_OPEN ?= 1
# export USB_ISO ?= 1
endif # BTUSB_DUAL_MODE_SUPPORT

export BLE_WIFI_DUAL_CHIP_MODE ?= 1
ifeq ($(BLE_WIFI_DUAL_CHIP_MODE),1)
KBUILD_CPPFLAGS += -DBLE_WIFI_DUAL_CHIP_MODE
export DUAL_CHIP_MODE_ROLE_BLE ?= 1
ifeq ($(DUAL_CHIP_MODE_ROLE_BLE),1)
KBUILD_CPPFLAGS += -DDUAL_CHIP_MODE_ROLE_BLE
endif # DUAL_CHIP_MODE_ROLE_BLE
endif # BLE_WIFI_DUAL_CHIP_MODE

ifeq ($(AOB_LOW_LATENCY_MODE),1)
KBUILD_CPPFLAGS += -DCODEC_BUFF_FRAME_NUM=10
KBUILD_CPPFLAGS += -DUSB_BUFF_FRAME_NUM=12
else
KBUILD_CPPFLAGS += -DCODEC_BUFF_FRAME_NUM=20
KBUILD_CPPFLAGS += -DUSB_BUFF_FRAME_NUM=40
endif # AOB_LOW_LATENCY_MODE

export BLE_USB_AUDIO_OPTIMIZE_CON_FLOW ?= 1
ifeq ($(BLE_USB_AUDIO_OPTIMIZE_CON_FLOW),1)
KBUILD_CPPFLAGS += -DBLE_USB_AUDIO_OPTIMIZE_CON_FLOW

export BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON ?= 0
ifeq ($(BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON),1)
KBUILD_CPPFLAGS += -DBLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON
endif # BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON

endif # BLE_USB_AUDIO_OPTIMIZE_CON_FLOW
# BLE_I2S_AUDIO_SUPPORT
else
KBUILD_CPPFLAGS += -DCODEC_BUFF_FRAME_NUM=10
KBUILD_CPPFLAGS += -DUSB_BUFF_FRAME_NUM=12
endif

export IBRT_UI ?= 1
ifeq ($(IBRT_UI),1)
KBUILD_CPPFLAGS += -DIBRT_UI
endif

export BES_AUD ?= 1

export POWER_MODE   ?= DIG_DCDC

export BT_RF_PREFER ?= 2M

export SPEECH_CODEC ?= 1

export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED ?= 1
export IOS_MFI ?= 0

export FLASH_SIZE ?= 0x400000
export FLASH_SUSPEND ?= 1
export FLASH_PROTECTION_BOOT_SECTION_FIRST ?= 0

export HOST_GEN_ECDH_KEY ?= 1

export SW_TRIG ?= 1

USE_THIRDPARTY ?= 0
export USE_KNOWLES ?= 0

export LAURENT_ALGORITHM ?= 1

export RX_IQ_CAL ?= 1

export TX_IQ_CAL ?= 1

export IBRT_DUAL_ANT_CTRL ?= 0

export BT_XTAL_SYNC ?= 0



export POWERKEY_I2C_SWITCH ?=0

AUTO_TEST ?= 0

BES_AUTOMATE_TEST ?= 0

export DUMP_LOG_ENABLE ?= 0

SUPPORT_BATTERY_REPORT ?= 1

SUPPORT_HF_INDICATORS ?= 0

SUPPORT_SIRI ?= 1

BES_AUDIO_DEV_Main_Board_9v0 ?= 0

APP_USE_LED_INDICATE_IBRT_STATUS ?= 0

export BT_EXT_LNA_PA ?=0
export BT_EXT_LNA ?=0
export BT_EXT_PA ?=0

export NORMAL_TEST_MODE_SWITCH ?= 0
ifeq ($(NORMAL_TEST_MODE_SWITCH),1)
KBUILD_CPPFLAGS += -DNORMAL_TEST_MODE_SWITCH
endif

export BLE_AUDIO_STARLORD_COMPATIBLE_SUPPORT ?= 0
ifeq ($(BLE_AUDIO_STARLORD_COMPATIBLE_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_STARLORD_COMPATIBLE_SUPPORT
endif

export BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT ?= 0
ifeq ($(BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT
endif

# temp mecro for csip
export BLE_AUDIO_CSIP_SUPPORT ?= 1
ifeq ($(BLE_AUDIO_CSIP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_CSIP_SUPPORT
endif

#For ble feture verification
BLE ?= 0
export BLE_AUDIO_ENABLED ?= 0
export CUSTOMER_DEFINE_ADV_DATA ?= 0
export BLE_CONNECTION_MAX ?= 3
export BLE_SEC_ACCEPT_BY_CUSTOMER ?= 0
export BLE_AUDIO_24BIT ?= 0
ifeq ($(BLE_AUDIO_24BIT),1)
KBUILD_CPPFLAGS += -D__BLE_AUDIO_24BIT__
endif
export ALIGNED_WITH_FINAL_AOB_SPEC ?= 0
ifeq ($(ALIGNED_WITH_FINAL_AOB_SPEC),1)
KBUILD_CPPFLAGS += -DALIGNED_WITH_FINAL_AOB_SPEC
endif

HAS_BT_SYNC ?= 1

#For free tws pairing feature
FREE_TWS_PAIRING_ENABLED ?= 0

APP_UART_MODULE ?= 0

APP_CHIP_BRIDGE_MODULE ?= 0

ifeq ($(A2DP_LHDC_ON),1)
AUDIO_BUFFER_SIZE ?= 140*1024
else
AUDIO_BUFFER_SIZE ?= 150*1024
endif

export UNIFY_HEAP_ENABLED ?= 0

export USE_OVERLAY_TXT_GAP ?= 0

export AUDIO_OUTPUT_DC_AUTO_CALIB ?= 1
ifeq ($(AUDIO_OUTPUT_DC_AUTO_CALIB), 1)
export AUDIO_OUTPUT_DC_CALIB := 1
export AUDIO_OUTPUT_DC_CALIB_ANA := 0
export AUDIO_OUTPUT_SET_LARGE_ANA_DC ?= 0
export AUDIO_OUTPUT_DC_CALIB_DUAL_CHAN ?= 1
export AUDIO_OUTPUT_DIG_DC_DEEP_CALIB ?= 1
export DAC_DRE_ENABLE ?= 1
export CODEC_DAC_DC_NV_DATA ?= 1
export CODEC_DAC_DC_DYN_BUF ?= 1
endif

AUDIO_OUTPUT_SWAP ?= 0
ifeq ($(AUDIO_OUTPUT_SWAP),1)
KBUILD_CPPFLAGS += -DAUDIO_OUTPUT_SWAP
endif

export TRACE_BUF_SIZE ?= 10*1024
export TRACE_BAUD_RATE ?= 10*115200
export BTM_MAX_LINK_NUMS ?= 3
export BT_DEVICE_NUM ?= 2
export AOB_CODEC_CP ?= 0
ifeq ($(AOB_CODEC_CP),1)
KBUILD_CPPFLAGS += -DAOB_CODEC_CP
#KBUILD_CPPFLAGS += -DRAMCP_SIZE=$(RAMCP_SIZE)
#KBUILD_CPPFLAGS += -DRAMCPX_SIZE=$(RAMCPX_SIZE)
#KBUILD_CPPFLAGS += -DFAST_XRAM_SECTION_SIZE=$(FAST_XRAM_SECTION_SIZE)
endif

ifeq ($(PSAP_SW_APP),1)
export DMA_AUDIO_APP ?= 1
endif

export DMA_AUDIO_APP ?= 0
ifeq ($(DMA_AUDIO_APP),1)
export SCO_CP_ACCEL  := 0
export A2DP_CP_ACCEL := 0
export AUDIO_PROMPT_USE_DAC2_ENABLED            := 0
export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED := 1
export DMA_RPC_CLI   ?= 1
DMA_AUD_CFG_PATH := best1502x_ibrt
include $(srctree)/config/$(DMA_AUD_CFG_PATH)/dma_aud_cfg.mk
endif

export LVGL_EN ?= 0
export LVGL_USE_GPU ?=0
ifeq ($(LVGL_USE_GPU),1)
export BAREMETAL ?=1
export CHIP_HAS_GPU ?=1
export FBTEST_DRV ?=0
export CHIP_HAS_GPU ?=1
KBUILD_CPPFLAGS += -DLVGL_GPU_EN
export ALLOW_WARNING ?= 1
endif
#CODEC_VAD_CFG_BUF_SIZE ?= 0x18000

init-y :=
core-y := platform/ utils/cqueue/ utils/list/ multimedia/ utils/intersyshci/ utils/sha256/ utils/cfifo/

ifneq ($(EXT_SRC_DIR),)
core-y += $(EXT_SRC_DIR)
else
core-y += apps/ services/
endif

ifeq ($(LVGL_EN),1)
core-y += thirdparty/gui_lib/
export CONFIG_LVGL_TP_CST820 ?= 1
KBUILD_CPPFLAGS += -D__LVGL_EN__
endif

KBUILD_CPPFLAGS += \
    -Iplatform/cmsis/inc \
    -Iservices/audioflinger \
    -Iplatform/hal

KBUILD_CPPFLAGS += \
    -DAPP_AUDIO_BUFFER_SIZE=$(AUDIO_BUFFER_SIZE) \
    -DCHARGER_PLUGINOUT_RESET=0

KBUILD_CPPFLAGS += \
    -DBTM_MAX_LINK_NUMS=$(BTM_MAX_LINK_NUMS) \
    -DBT_DEVICE_NUM=$(BT_DEVICE_NUM)

ifeq ($(BES_AUDIO_DEV_Main_Board_9v0),1)
KBUILD_CPPFLAGS += -DBES_AUDIO_DEV_Main_Board_9v0
endif

ifeq ($(TPORTS_KEY_COEXIST),1)
KBUILD_CPPFLAGS += -DTPORTS_KEY_COEXIST
endif

ifeq ($(AUDIO_HEARING_COMPSATN),1)
HEARING_USE_STATIC_RAM := 1
ifeq ($(HEARING_MOD),HWFIR)
HW_FIR_EQ_PROCESS := 1
HEARING_MOD_VAL := 0
USE_CMSIS_FFT_LEN_1024 := 1
else
ifeq ($(HEARING_MOD),SWIIR)
SW_IIR_EQ_PROCESS := 1
HEARING_MOD_VAL := 1
else
HW_DAC_IIR_EQ_PROCESS := 1
HEARING_MOD_VAL := 2
endif
endif
endif

#-DIBRT_LINK_LOWLAYER_MONITOR

#-D_AUTO_SWITCH_POWER_MODE__
#-D__APP_KEY_FN_STYLE_A__
#-D__APP_KEY_FN_STYLE_B__
#-D__EARPHONE_STAY_BOTH_SCAN__
#-D__POWERKEY_CTRL_ONOFF_ONLY__
#-DAUDIO_LINEIN

ifeq ($(BT_RAMRUN),1)
export BT_RAM_DISTRIBUTION ?= 1
export BT_SYSTEM_52M := 0
KBUILD_CPPFLAGS += -D__BT_RAMRUN__
core-y += utils/lzma/
BLE_NEW_SWAGC_MODE ?= 1
endif

ifeq ($(BT_RAM_DISTRIBUTION),1)
ifneq ($(DMA_AUDIO_APP),1)
# export RAMCP_SIZE  ?= 0x40000
# export RAMCPX_SIZE ?= 0x20000
endif
KBUILD_CPPFLAGS += -D__BT_RAM_DISTRIBUTION__
endif

ifeq ($(CURRENT_TEST),1)
export SMALL_RET_RAM ?= 1
export CORE_SLEEP_POWER_DOWN ?= 1
#INTSRAM_RUN ?= 1
endif
ifeq ($(INTSRAM_RUN),1)
LDS_FILE ?= best1000_intsram.lds
else
LDS_FILE ?= best1000.lds
endif

export OTA_SUPPORT_SLAVE_BIN ?= 0
ifeq ($(OTA_SUPPORT_SLAVE_BIN),1)
export SLAVE_BIN_FLASH_OFFSET ?= 0x100000
export SLAVE_BIN_TARGET_NAME ?= anc_usb
endif

ifeq ($(GATT_OVER_BR_EDR),1)
export GATT_OVER_BR_EDR ?= 1
KBUILD_CPPFLAGS += -D__GATT_OVER_BR_EDR__
endif
ifeq ($(BES_OTA),1)
# enabled when 1502x flash remap is ready
FLASH_REMAP ?= 1
NEW_IMAGE_FLASH_OFFSET ?= OTA_REMAP_OFFSET
endif

export PROMPT_IN_FLASH ?= 0
export USE_TRACE_ID ?= 0
export BT_RAMRUN_BIN_COMPRESSED ?= 0
export AAC_REDUCE_SIZE ?= 1

ifneq ($(A2DP_DECODER_VER), )
KBUILD_CPPFLAGS += -DA2DP_DECODER_VER=$(A2DP_DECODER_VER)
endif

ifneq ($(AUDIO_TRIGGER_VER), )
KBUILD_CPPFLAGS += -DAUDIO_TRIGGER_VER=$(AUDIO_TRIGGER_VER)
endif

ifeq ($(HOST_GEN_ECDH_KEY),1)
KBUILD_CPPFLAGS += -D__HOST_GEN_ECDH_KEY__
endif

ifeq ($(SW_TRIG),1)
KBUILD_CPPFLAGS += -D__SW_TRIG__
endif

KBUILD_CPPFLAGS += -DHAL_TRACE_RX_ENABLE

ifeq ($(AUDIO_OUTPUT_DC_AUTO_CALIB),1)
KBUILD_CPPFLAGS += -DAUDIO_OUTPUT_DC_AUTO_CALIB
endif

ifeq ($(CODEC_DAC_DC_NV_DATA), 1)
KBUILD_CPPFLAGS += -DCODEC_DAC_DC_NV_DATA
endif

export AUDIO_ADC_DC_AUTO_CALIB ?= 1
ifeq ($(AUDIO_ADC_DC_AUTO_CALIB), 1)
export CODEC_ADC_DC_NV_DATA ?= 1
ifeq ($(CODEC_ADC_DC_NV_DATA), 1)
KBUILD_CPPFLAGS += -DCODEC_ADC_DC_NV_DATA
NEW_NV_RECORD_ENABLED ?= 1
ifeq ($(NEW_NV_RECORD_ENABLED),1)
KBUILD_CPPFLAGS += -DNEW_NV_RECORD_ENABLED
KBUILD_CPPFLAGS += -Iservices/nv_section/userdata_section
endif
endif
ifeq ($(AUDIO_ADC_DC_AUTO_CALIB), 1)
KBUILD_CPPFLAGS += -DAUDIO_ADC_DC_AUTO_CALIB
endif
endif

ifeq ($(REPORT_CONNECTIVITY_LOG),1)
KBUILD_CPPFLAGS += -D__CONNECTIVITY_LOG_REPORT__
KBUILD_CPPFLAGS += -D__AUDIO_RETRIGGER_REPORT__
endif

ifeq ($(SNYC_FOUND_CHECK_HECERROR),1)
KBUILD_CPPFLAGS += -D__SNYC_FOUND_CHECK_HECERROR__
endif

AFH_ASSESS_GAIN ?= 2
KBUILD_CPPFLAGS += -DAFH_ASSESS_GAIN=$(AFH_ASSESS_GAIN)

KBUILD_CFLAGS +=

LIB_LDFLAGS += -lstdc++ -lsupc++

DUAL_MIC_RECORDING ?= 0
RECORDING_USE_SCALABLE ?= 0
RECORDING_USE_OPUS ?= 0
RECORDING_USE_OPUS_LOWER_BANDWIDTH ?= 0
STEREO_RECORD_PROCESS ?= 0

# mutex for power on tws pairing and freeman pairing
POWER_ON_ENTER_TWS_PAIRING_ENABLED ?= 0
POWER_ON_ENTER_FREEMAN_PAIRING_ENABLED ?= 0
POWER_ON_ENTER_BOTH_SCAN_MODE ?= 0

OS_THREAD_TIMING_STATISTICS_ENABLE ?= 0

# if VAD enabled, sensor hub always use large ram memory
ifeq ($(VOICE_DETECTOR_SENS_EN),1)
LARGE_SENS_RAM ?= 1
export SENSOR_HUB ?= 1
else
export VOICE_DETECTOR_SENS_EN ?= 0
export SENSOR_HUB ?= 0
endif

# for usb mass storage class
ifeq ($(MSD_MODE),1)
KBUILD_CPPFLAGS += -DMSD_MODE
endif

export SPA_AUDIO_ENABLE := 0
ifeq ($(SPA_AUDIO_ENABLE),1)
export USE_THIRDPARTY := 1
KBUILD_CPPFLAGS += -DSPA_AUDIO_ENABLE
KBUILD_CPPFLAGS += -DSPA_AUDIO_BTH
endif

export SPLIT_CORE_M_CORE_L ?= 0
export PMU_FORCE_LP_MODE ?= 1

$(info -------------------------------)
$(info BLE_CONNECTION_MAX: $(BLE_CONNECTION_MAX))
$(info --)

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

#LDFLAGS_IMAGE += --wrap main
export MTP_AND_FS_ENABLE?=0
ifeq ($(MTP_AND_FS_ENABLE),1)
  KBUILD_CPPFLAGS += -DMTP_AND_FS_ENABLE
  export CHIP_HAS_USB := 1
  export MTP_SPEED_TEST ?= 0
  ifeq ($(MTP_SPEED_TEST),1)
    KBUILD_CPPFLAGS += -DMTP_SPEED_TEST
  endif
  export LITTLEFS_ENABLE ?=0
  ifeq ($(LITTLEFS_ENABLE),1)
    export RTOS ?=1
    export MBED ?=1
    KBUILD_CPPFLAGS += -DFS_LITTLEFS_ENABLE
    KBUILD_CPPFLAGS += -DLFS_READ_SIZE=512
    KBUILD_CPPFLAGS += -DLFS_PROG_SIZE=512
    KBUILD_CPPFLAGS += -DLFS_BLOCK_SIZE=4096
    KBUILD_CPPFLAGS += -DLFS_CACHE_SIZE=512
    KBUILD_CPPFLAGS += -DLFS_NO_MALLOC
    export USB_MTP_TEST ?= 1
    ifeq ($(USB_MTP_TEST), 1)
      export MTP_ENABLE ?= 1
      KBUILD_CPPFLAGS += -DUSB_MTP_TEST
      HWTEST=1
      KBUILD_CPPFLAGS += -DMTP_ENABLE
      core-y += utils/string_convert/
    endif

    ifeq ($(FS_DEVICE_TYPE), 3)
      KBUILD_CPPFLAGS += -DFS_ON_FLASH
      KBUILD_CPPFLAGS += -DMTP_ON_FLASH_SIZE=0x180000
      KBUILD_CPPFLAGS += -DTEST_MTP_ON_FLASH
    endif

    KBUILD_CPPFLAGS += -DLITTLEFS_ENABLE
  endif #LITTLEFS_ENABLE

  export FATFS_ENABLE ?=0
  ifeq ($(FATFS_ENABLE),1)
    KBUILD_CPPFLAGS += -DLFS_NO_MALLOC
    export FATFS_FORMAT_AUTO ?=1
    ifeq ($(FATFS_FORMAT_AUTO),1)
      KBUILD_CPPFLAGS +=-DFATFS_FORMAT_AUTO
    endif
    export RTOS ?=1
    export MBED ?=1
    export USB_MTP_TEST ?= 1
    ifeq ($(USB_MTP_TEST), 1)
      export MTP_ENABLE ?= 1
      KBUILD_CPPFLAGS += -DUSB_MTP_TEST
      HWTEST=1
      KBUILD_CPPFLAGS += -DMTP_ENABLE
      core-y += utils/string_convert/
    endif

    ifeq ($(FS_DEVICE_TYPE), 3)
      KBUILD_CPPFLAGS += -DFS_ON_FLASH
      KBUILD_CPPFLAGS += -DMTP_ON_FLASH_SIZE=0x180000
      KBUILD_CPPFLAGS += -DTEST_MTP_ON_FLASH
    endif

    KBUILD_CPPFLAGS += -DFS_FATFS_ENABLE
  endif #FATFS_ENABLE
endif #MTP_AND_FS_ENABLE

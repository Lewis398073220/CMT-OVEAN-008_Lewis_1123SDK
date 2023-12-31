#!/bin/bash
[ -z "$1" ] &&{
    echo The Chip is Null !!!
    exit
}

[ -z "$2" ] &&{
    echo The Target is Null !!!
    exit
}

[ -z "$3" ] &&{
    echo "The Target_Customer is Null !!! \n"
    echo "hearing_aid_customer or audio_customer "
    exit
}


CHIPID=$1
TARGET_LIST=$2
TARGET_CUSTOMER=$3
RELEASE=$4


echo "param opt:"
echo "CHIPID=$CHIPID TARGET_LIST=$TARGET_LIST TARGET_CUSTOMER=$TARGET_CUSTOMER RELEASE=$RELEASE "


TARGET_DIR="config/"$TARGET_LIST
TARGET_NAME="$TARGET_LIST"

#git clean -d -fx
DATE=`date +%F | sed 's/-//g'`
commitid=`git rev-parse --short HEAD`

export CROSS_COMPILE="ccache arm-none-eabi-"

if [[ $RELEASE == "release" ]];
then
    echo -e "rm old lib"
    LIB_DIR=lib/bes/
    rm build_err.log
    rm -rf $LIB_DIR
    rm out -rf
fi


################################### var define ###################################
#for Stereo headphones
BTH_STERO_PRODUCT_CFG="FREEMAN_ENABLED_STERO=1 "

# Select according to the corresponding chip
    # 1: 2600ZP
    # 2: 2700IBP
    # 3: 2700H
    # 4: 2700L
BTH_SELECT_PACKAG_TYPE="PACKAG_TYPE=4 " 

#Debugging and use, can to be removed, for idle power
BTH_UART_DEBUG_CFG="APP_TRACE_RX_ENABLE=1 APP_RX_API_ENABLE=1  AUDIO_DEBUG_CMD=1 BT_DEBUG_TPORTS=0xB1B1 "

#mute:  "BT_MUTE_A2DP=1 BT_PAUSE_A2DP=0 BT_CLOSE_A2DP=0"
#pause: "BT_MUTE_A2DP=0 BT_PAUSE_A2DP=1 BT_CLOSE_A2DP=0"
#close: "BT_MUTE_A2DP=0 BT_PAUSE_A2DP=0 BT_CLOSE_A2DP=1"
BTH_AUDIO_POLICY_CFG="BT_MUTE_A2DP=1 BT_PAUSE_A2DP=0 BT_CLOSE_A2DP=0 "

BTH_ANC_CFG="ANC_ASSIST_ENABLED=1 "

BTH_LEA_ENABLE_CFG="BLE=1 BLE_AUDIO_ENABLED=1 BLE_CONNECTION_MAX=3 BT_RAMRUN=1 AOB_LOW_LATENCY_MODE=0 \
                REPORT_EVENT_TO_CUSTOMIZED_UX=1 CTKD_ENABLE=1 IS_CTKD_OVER_BR_EDR_ENABLED=1 IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE=1 \
                BLE_ADV_RPA_ENABLED=1 IS_BLE_FLAGS_ADV_DATA_CONFIGURED_BY_APP_LAYER=1 BLE_AOB_VOLUME_SYNC_ENABLED=0 \
                BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT=1 BLE_AUDIO_STARLORD_COMPATIBLE_SUPPORT=0 APP_UI_TEST_MODE_ENABLE=1\
                OS_TASKCNT=22 FLASH_SIZE=0x400000 NEW_IMAGE_FLASH_OFFSET=0x200000 FLASH_REMAP=0"

BTH_CUSTOMER_CFG="BES_OTA=1 AUDIO_BUFFER_SIZE=90*1024 AOB_MOBILE_ENABLED=0 \
               TRACE_BUF_SIZE=30*1024 FLASH_SIZE=0x400000 NEW_IMAGE_FLASH_OFFSET=0x200000 FLASH_REMAP=0"

OTA_COPY_OFFSET_CFG="OTA_CODE_OFFSET=0x20000 OTA_REBOOT_FLASH_REMAP=0 OTA_BOOT_SIZE=0x18000 OTA_BOOT_INFO_OFFSET=0x18000 NEW_IMAGE_FLASH_OFFSET=0x200000 FLASH_REMAP=0 DEBUG_PORT=1"
BTH_OTA_COPY_OFFSET_CFG="OTA_CODE_OFFSET=0x20000"

################################### end of var define ###################################



################################### build  start ###################################

if [[ $RELEASE == "release" ]];
then

make T=prod_test/ota_copy -s GEN_LIB=1 SINGLE_WIRE_UART_PMU_1803=0 DEBUG=1 -j40 CHIP=$CHIPID  $OTA_COPY_OFFSET_CFG 
make T=prod_test/ota_copy -s GEN_LIB=1 SINGLE_WIRE_UART_PMU_1803=0 DEBUG=1 -j40 CHIP=$CHIPID OTA_BIN_COMPRESSED=1 $OTA_COPY_OFFSET_CFG
make T=sensor_hub GEN_LIB=1 -s CHIP=$CHIPID BTH_AS_MAIN_MCU=1 SENS_TRC_TO_MCU=1 DEBUG=1 VOICE_DETECTOR_EN=0 -j40

if [[ $CHIPID == "best1502x" ]];
then
TARGET_LIST="best1502x_ibrt"
elif [[ $CHIPID == "" ]];
then
TARGET_LIST=""
fi



### basic ###
make T=$TARGET_LIST GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  BLE=0 
make T=$TARGET_LIST GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  BLE=1 
make T=$TARGET_LIST GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  BLE=1 GFPS_ENABLE=1
make T=$TARGET_LIST GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  BLE=1 GFPS_ENABLE=1 SASS_ENABLE=1

### anc ###
make T=${TARGET_LIST}_anc GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  $BTH_ANC_CFG BLE=0 
make T=${TARGET_LIST}_anc GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  $BTH_ANC_CFG BLE=1 
make T=${TARGET_LIST}_anc GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  $BTH_ANC_CFG BLE=1 GFPS_ENABLE=1
make T=${TARGET_LIST}_anc GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  $BTH_ANC_CFG BLE=1 GFPS_ENABLE=1 SASS_ENABLE=1

### anc and lea ###
#make T=${TARGET_LIST}_anc GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  $BTH_ANC_CFG $BTH_LEA_ENABLE_CFG BLE=1 GFPS_ENABLE=1
#make T=${TARGET_LIST}_anc GEN_LIB=1 -j40 -s $BTH_STERO_PRODUCT_CFG $BTH_SELECT_PACKAG_TYPE $BTH_UART_DEBUG_CFG $BTH_AUDIO_POLICY_CFG $BTH_CUSTOMER_CFG  $BTH_ANC_CFG $BTH_LEA_ENABLE_CFG BLE=1 GFPS_ENABLE=1 SASS_ENABLE=1

else

hearing_aid_customer=" "
audio_customer="$BTH_STERO_PRODUCT_CFG \
                $BTH_SELECT_PACKAG_TYPE \
                $BTH_UART_DEBUG_CFG \
                $BTH_AUDIO_POLICY_CFG \
                $BTH_CUSTOMER_CFG \
                $BTH_ANC_CFG \
                "

make T=prod_test/ota_copy -s SINGLE_WIRE_UART_PMU_1803=0 DEBUG=1 -j40 CHIP=$CHIPID $OTA_COPY_OFFSET_CFG
make T=sensor_hub  -s CHIP=$CHIPID BTH_AS_MAIN_MCU=1 SENS_TRC_TO_MCU=1 DEBUG=1 VOICE_DETECTOR_EN=0 -j40


if [[ $TARGET_CUSTOMER == "audio_customer" ]];
then
bth_build_cmd="make T=$TARGET_LIST CHIP=$CHIPID $audio_customer -s -j8 "

elif [[ $TARGET_CUSTOMER == "hearing_aid_customer" ]];
then
make T=cp_subsys CHIP=best1502x CP_SUBSYS_TRC_TO_MCU=1 NOSTD=0 RTOS=0 DMA_AUDIO_APP=1 PSAP_SW_APP=1 -j8
bth_build_cmd="make T=best1502x_ibrt_anc FREEMAN_ENABLED_STERO=1 APP_TRACE_RX_ENABLE=1 APP_RX_API_ENABLE=1  AUDIO_DEBUG_CMD=1 DMA_AUDIO_APP=1 PSAP_SW_APP=1 PACKAG_TYPE=4 -j32 DMA_RPC_SVR_CORE=CP"
fi

$bth_build_cmd
echo "bth_build_cmd: "
echo "$bth_build_cmd"
fi

################################### end of build  start ###################################

################################### release script ###################################
if [[ $RELEASE == "release" ]];
then

. `dirname $0`/relsw_ibrt_common.sh

fi
################################### end of release script ###################################

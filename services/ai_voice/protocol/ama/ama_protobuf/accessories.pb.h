/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.6 */

#ifndef PB_ACCESSORIES_PB_H_INCLUDED
#define PB_ACCESSORIES_PB_H_INCLUDED
#include "pb_0_4_6.h"
#include "common.pb.h"
#include "system.pb.h"
#include "transport.pb.h"
#include "speech.pb.h"
#include "calling.pb.h"
#include "central.pb.h"
#include "device.pb.h"
#include "media.pb.h"
#include "state.pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _Command { 
    Command_NONE = 0, 
    Command_RESET_CONNECTION = 51, 
    Command_SYNCHRONIZE_SETTINGS = 50, 
    Command_KEEP_ALIVE = 55, 
    Command_REMOVE_DEVICE = 56, 
    Command_GET_LOCALES = 57, 
    Command_SET_LOCALE = 58, 
    Command_LAUNCH_APP = 59, 
    Command_UPGRADE_TRANSPORT = 30, 
    Command_SWITCH_TRANSPORT = 31, 
    Command_START_SPEECH = 11, 
    Command_PROVIDE_SPEECH = 10, 
    Command_STOP_SPEECH = 12, 
    Command_ENDPOINT_SPEECH = 13, 
    Command_NOTIFY_SPEECH_STATE = 14, 
    Command_FORWARD_AT_COMMAND = 40, 
    Command_INCOMING_CALL = 41, 
    Command_GET_CENTRAL_INFORMATION = 103, 
    Command_GET_DEVICE_INFORMATION = 20, 
    Command_GET_DEVICE_CONFIGURATION = 21, 
    Command_OVERRIDE_ASSISTANT = 22, 
    Command_START_SETUP = 23, 
    Command_COMPLETE_SETUP = 24, 
    Command_NOTIFY_DEVICE_CONFIGURATION = 25, 
    Command_UPDATE_DEVICE_INFORMATION = 26, 
    Command_NOTIFY_DEVICE_INFORMATION = 27, 
    Command_GET_DEVICE_FEATURES = 28, 
    Command_ISSUE_MEDIA_CONTROL = 60, 
    Command_GET_STATE = 100, 
    Command_SET_STATE = 101, 
    Command_SYNCHRONIZE_STATE = 102 
} Command;

/* Struct definitions */
typedef struct _Response { 
    ErrorCode error_code;
    pb_size_t which_payload;
    union {
        DeviceInformation device_information;
        State state;
        ConnectionDetails connection_details;
        DeviceConfiguration device_configuration;
        CentralInformation central_information;
        Dialog dialog;
        SpeechProvider speech_provider;
        Locales locales;
        DeviceFeatures device_features;
    } payload;
} Response;

typedef struct _ControlEnvelope { 
    Command command;
    pb_size_t which_payload;
    union {
        Response response;
        ProvideSpeech provide_speech;
        StartSpeech start_speech;
        StopSpeech stop_speech;
        EndpointSpeech endpoint_speech;
        NotifySpeechState notify_speech_state;
        GetDeviceInformation get_device_information;
        GetDeviceConfiguration get_device_configuration;
        OverrideAssistant override_assistant;
        StartSetup start_setup;
        CompleteSetup complete_setup;
        NotifyDeviceConfiguration notify_device_configuration;
        UpdateDeviceInformation update_device_information;
        NotifyDeviceInformation notify_device_information;
        GetDeviceFeatures get_device_features;
        UpgradeTransport upgrade_transport;
        SwitchTransport switch_transport;
        ForwardATCommand forward_at_command;
        IncomingCall incoming_call;
        SynchronizeSettings synchronize_settings;
        ResetConnection reset_connection;
        KeepAlive keep_alive;
        RemoveDevice remove_device;
        GetLocales get_locales;
        SetLocale set_locale;
        LaunchApp launch_app;
        IssueMediaControl issue_media_control;
        GetState get_state;
        SetState set_state;
        SynchronizeState synchronize_state;
        GetCentralInformation get_central_information;
    } payload;
} ControlEnvelope;


/* Helper constants for enums */
#define _Command_MIN Command_NONE
#define _Command_MAX Command_GET_CENTRAL_INFORMATION
#define _Command_ARRAYSIZE ((Command)(Command_GET_CENTRAL_INFORMATION+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define Response_init_default                    {_ErrorCode_MIN, 0, {DeviceInformation_init_default}}
#define ControlEnvelope_init_default             {_Command_MIN, 0, {Response_init_default}}
#define Response_init_zero                       {_ErrorCode_MIN, 0, {DeviceInformation_init_zero}}
#define ControlEnvelope_init_zero                {_Command_MIN, 0, {Response_init_zero}}

/* Field tags (for use in manual encoding/decoding) */
#define Response_error_code_tag                  1
#define Response_device_information_tag          3
#define Response_state_tag                       7
#define Response_connection_details_tag          8
#define Response_device_configuration_tag        10
#define Response_central_information_tag         13
#define Response_dialog_tag                      14
#define Response_speech_provider_tag             15
#define Response_locales_tag                     21
#define Response_device_features_tag             28
#define ControlEnvelope_command_tag              1
#define ControlEnvelope_response_tag             9
#define ControlEnvelope_provide_speech_tag       10
#define ControlEnvelope_start_speech_tag         11
#define ControlEnvelope_stop_speech_tag          12
#define ControlEnvelope_endpoint_speech_tag      13
#define ControlEnvelope_notify_speech_state_tag  14
#define ControlEnvelope_get_device_information_tag 20
#define ControlEnvelope_get_device_configuration_tag 21
#define ControlEnvelope_override_assistant_tag   22
#define ControlEnvelope_start_setup_tag          23
#define ControlEnvelope_complete_setup_tag       24
#define ControlEnvelope_notify_device_configuration_tag 25
#define ControlEnvelope_update_device_information_tag 26
#define ControlEnvelope_notify_device_information_tag 27
#define ControlEnvelope_get_device_features_tag  28
#define ControlEnvelope_upgrade_transport_tag    30
#define ControlEnvelope_switch_transport_tag     31
#define ControlEnvelope_forward_at_command_tag   40
#define ControlEnvelope_incoming_call_tag        41
#define ControlEnvelope_synchronize_settings_tag 50
#define ControlEnvelope_reset_connection_tag     51
#define ControlEnvelope_keep_alive_tag           55
#define ControlEnvelope_remove_device_tag        56
#define ControlEnvelope_get_locales_tag          57
#define ControlEnvelope_set_locale_tag           58
#define ControlEnvelope_launch_app_tag           59
#define ControlEnvelope_issue_media_control_tag  60
#define ControlEnvelope_get_state_tag            100
#define ControlEnvelope_set_state_tag            101
#define ControlEnvelope_synchronize_state_tag    102
#define ControlEnvelope_get_central_information_tag 103

/* Struct field encoding specification for nanopb */
#define Response_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    error_code,        1) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,device_information,payload.device_information),   3) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,state,payload.state),   7) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,connection_details,payload.connection_details),   8) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,device_configuration,payload.device_configuration),  10) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,central_information,payload.central_information),  13) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,dialog,payload.dialog),  14) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,speech_provider,payload.speech_provider),  15) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,locales,payload.locales),  21) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,device_features,payload.device_features),  28)
#define Response_CALLBACK NULL
#define Response_DEFAULT NULL
#define Response_payload_device_information_MSGTYPE DeviceInformation
#define Response_payload_state_MSGTYPE State
#define Response_payload_connection_details_MSGTYPE ConnectionDetails
#define Response_payload_device_configuration_MSGTYPE DeviceConfiguration
#define Response_payload_central_information_MSGTYPE CentralInformation
#define Response_payload_dialog_MSGTYPE Dialog
#define Response_payload_speech_provider_MSGTYPE SpeechProvider
#define Response_payload_locales_MSGTYPE Locales
#define Response_payload_device_features_MSGTYPE DeviceFeatures

#define ControlEnvelope_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    command,           1) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,response,payload.response),   9) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,provide_speech,payload.provide_speech),  10) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,start_speech,payload.start_speech),  11) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,stop_speech,payload.stop_speech),  12) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,endpoint_speech,payload.endpoint_speech),  13) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,notify_speech_state,payload.notify_speech_state),  14) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,get_device_information,payload.get_device_information),  20) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,get_device_configuration,payload.get_device_configuration),  21) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,override_assistant,payload.override_assistant),  22) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,start_setup,payload.start_setup),  23) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,complete_setup,payload.complete_setup),  24) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,notify_device_configuration,payload.notify_device_configuration),  25) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,update_device_information,payload.update_device_information),  26) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,notify_device_information,payload.notify_device_information),  27) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,get_device_features,payload.get_device_features),  28) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,upgrade_transport,payload.upgrade_transport),  30) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,switch_transport,payload.switch_transport),  31) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,forward_at_command,payload.forward_at_command),  40) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,incoming_call,payload.incoming_call),  41) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,synchronize_settings,payload.synchronize_settings),  50) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,reset_connection,payload.reset_connection),  51) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,keep_alive,payload.keep_alive),  55) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,remove_device,payload.remove_device),  56) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,get_locales,payload.get_locales),  57) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,set_locale,payload.set_locale),  58) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,launch_app,payload.launch_app),  59) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,issue_media_control,payload.issue_media_control),  60) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,get_state,payload.get_state), 100) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,set_state,payload.set_state), 101) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,synchronize_state,payload.synchronize_state), 102) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,get_central_information,payload.get_central_information), 103)
#define ControlEnvelope_CALLBACK NULL
#define ControlEnvelope_DEFAULT NULL
#define ControlEnvelope_payload_response_MSGTYPE Response
#define ControlEnvelope_payload_provide_speech_MSGTYPE ProvideSpeech
#define ControlEnvelope_payload_start_speech_MSGTYPE StartSpeech
#define ControlEnvelope_payload_stop_speech_MSGTYPE StopSpeech
#define ControlEnvelope_payload_endpoint_speech_MSGTYPE EndpointSpeech
#define ControlEnvelope_payload_notify_speech_state_MSGTYPE NotifySpeechState
#define ControlEnvelope_payload_get_device_information_MSGTYPE GetDeviceInformation
#define ControlEnvelope_payload_get_device_configuration_MSGTYPE GetDeviceConfiguration
#define ControlEnvelope_payload_override_assistant_MSGTYPE OverrideAssistant
#define ControlEnvelope_payload_start_setup_MSGTYPE StartSetup
#define ControlEnvelope_payload_complete_setup_MSGTYPE CompleteSetup
#define ControlEnvelope_payload_notify_device_configuration_MSGTYPE NotifyDeviceConfiguration
#define ControlEnvelope_payload_update_device_information_MSGTYPE UpdateDeviceInformation
#define ControlEnvelope_payload_notify_device_information_MSGTYPE NotifyDeviceInformation
#define ControlEnvelope_payload_get_device_features_MSGTYPE GetDeviceFeatures
#define ControlEnvelope_payload_upgrade_transport_MSGTYPE UpgradeTransport
#define ControlEnvelope_payload_switch_transport_MSGTYPE SwitchTransport
#define ControlEnvelope_payload_forward_at_command_MSGTYPE ForwardATCommand
#define ControlEnvelope_payload_incoming_call_MSGTYPE IncomingCall
#define ControlEnvelope_payload_synchronize_settings_MSGTYPE SynchronizeSettings
#define ControlEnvelope_payload_reset_connection_MSGTYPE ResetConnection
#define ControlEnvelope_payload_keep_alive_MSGTYPE KeepAlive
#define ControlEnvelope_payload_remove_device_MSGTYPE RemoveDevice
#define ControlEnvelope_payload_get_locales_MSGTYPE GetLocales
#define ControlEnvelope_payload_set_locale_MSGTYPE SetLocale
#define ControlEnvelope_payload_launch_app_MSGTYPE LaunchApp
#define ControlEnvelope_payload_issue_media_control_MSGTYPE IssueMediaControl
#define ControlEnvelope_payload_get_state_MSGTYPE GetState
#define ControlEnvelope_payload_set_state_MSGTYPE SetState
#define ControlEnvelope_payload_synchronize_state_MSGTYPE SynchronizeState
#define ControlEnvelope_payload_get_central_information_MSGTYPE GetCentralInformation

extern const pb_msgdesc_t Response_msg;
extern const pb_msgdesc_t ControlEnvelope_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define Response_fields &Response_msg
#define ControlEnvelope_fields &ControlEnvelope_msg

/* Maximum encoded size of messages (where known) */
#if defined(DeviceInformation_size)
union Response_payload_size_union {char f3[(6 + DeviceInformation_size)]; char f0[367];};
#endif
#if defined(DeviceInformation_size) && defined(UpdateDeviceInformation_size) && defined(NotifyDeviceInformation_size)
union ControlEnvelope_payload_size_union {char f9[(8 + sizeof(union Response_payload_size_union))]; char f26[(7 + UpdateDeviceInformation_size)]; char f27[(7 + NotifyDeviceInformation_size)]; char f0[304];};
#endif
#if defined(DeviceInformation_size)
#define Response_size                            (2 + sizeof(union Response_payload_size_union))
#endif
#if defined(DeviceInformation_size) && defined(UpdateDeviceInformation_size) && defined(NotifyDeviceInformation_size)
#define ControlEnvelope_size                     (2 + sizeof(union ControlEnvelope_payload_size_union))
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
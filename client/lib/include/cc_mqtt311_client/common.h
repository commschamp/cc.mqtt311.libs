//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/// @file
/// @brief Common definition for MQTT-SN clients.

#pragma once

#ifdef __cplusplus
extern "C" {
#include <stdbool.h>
#endif // #ifdef __cplusplus

/// @brief Major verion of the library
/// @ingroup global
#define CC_MQTT311_CLIENT_MAJOR_VERSION 0U

/// @brief Minor verion of the library
/// @ingroup global
#define CC_MQTT311_CLIENT_MINOR_VERSION 1U

/// @brief Patch level of the library
/// @ingroup global
#define CC_MQTT311_CLIENT_PATCH_VERSION 0U

/// @brief Macro to create numeric version as single unsigned number
/// @ingroup global
#define CC_MQTT311_CLIENT_MAKE_VERSION(major_, minor_, patch_) \
    ((static_cast<unsigned>(major_) << 24) | \
     (static_cast<unsigned>(minor_) << 8) | \
     (static_cast<unsigned>(patch_)))

/// @brief Version of the library as single numeric value
/// @ingroup global
#define CC_MQTT311_CLIENT_VERSION CC_MQTT311_CLIENT_MAKE_VERSION(CC_MQTT311_CLIENT_MAJOR_VERSION, CC_MQTT311_CLIENT_MINOR_VERSION, CC_MQTT311_CLIENT_PATCH_VERSION)

/// @brief Quality of Service
/// @ingroup global
typedef enum
{
    CC_Mqtt311QoS_AtMostOnceDelivery = 0, ///< QoS=0. At most once delivery.
    CC_Mqtt311QoS_AtLeastOnceDelivery = 1, ///< QoS=1. At least once delivery.
    CC_Mqtt311QoS_ExactlyOnceDelivery = 2, ///< QoS=2. Exactly once delivery.
    CC_Mqtt311QoS_ValuesLimit ///< Limit for the values
} CC_Mqtt311QoS;

/// @brief Error code returned by various API functions.
/// @ingroup global
typedef enum
{
    CC_Mqtt311ErrorCode_Success = 0, ///< The requested function executed successfully.
    CC_Mqtt311ErrorCode_InternalError = 1, ///< Internal library error, please submit bug report    
    CC_Mqtt311ErrorCode_NotIntitialized = 2, ///< The allocated client hasn't been initialized.
    CC_Mqtt311ErrorCode_Busy = 3, ///< The client library is in the middle of previous operation(s), cannot start a new one.
    CC_Mqtt311ErrorCode_NotConnected = 4, ///< The client library is not connected to the broker. Returned by operations that require connection to the broker.
    CC_Mqtt311ErrorCode_AlreadyConnected = 5, ///< The client library is already connected to the broker, cannot perform connection operation.
    CC_Mqtt311ErrorCode_BadParam = 6, ///< Bad parameter is passed to the function.
    CC_Mqtt311ErrorCode_InsufficientConfig = 7, ///< The required configuration hasn't been performed.
    CC_Mqtt311ErrorCode_OutOfMemory = 8, ///< Memory allocation failed.
    CC_Mqtt311ErrorCode_BufferOverflow = 9, ///< Output buffer is too short
    CC_Mqtt311ErrorCode_NotSupported = 10, ///< Feature is not supported
    CC_Mqtt311ErrorCode_RetryLater = 11, ///< Retry in next event loop iteration.
    CC_Mqtt311ErrorCode_Disconnecting = 12, ///< The client is in "disconnecting" state, (re)connect is required in the next iteration loop.
    CC_Mqtt311ErrorCode_NetworkDisconnected = 13, ///< When network is disconnected issueing new ops is not accepted
    CC_Mqtt311ErrorCode_PreparationLocked = 14, ///< Another operation is being prepared, cannot create a new one without performing "send" or "cancel".
    CC_Mqtt311ErrorCode_ValuesLimit ///< Limit for the values
} CC_Mqtt311ErrorCode;

/// @brief Status of the asynchronous operation.
/// @ingroup global
typedef enum
{
    CC_Mqtt311AsyncOpStatus_Complete = 0, ///< The requested operation has been completed, refer to reported extra details for information.
    CC_Mqtt311AsyncOpStatus_InternalError = 1, ///< Internal library error, please submit bug report    
    CC_Mqtt311AsyncOpStatus_Timeout = 2, ///< The required response from broker hasn't been received in time
    CC_Mqtt311AsyncOpStatus_ProtocolError = 3, ///< The broker's response doesn't comply with MQTT311 specification.
    CC_Mqtt311AsyncOpStatus_Aborted = 4, ///< The operation has been aborted before completion due to client's side operation.
    CC_Mqtt311AsyncOpStatus_BrokerDisconnected = 5, ///< The operation has been aborted before completion due to broker's disconnection.
    CC_Mqtt311AsyncOpStatus_OutOfMemory = 6, ///< The client library wasn't able to allocate necessary memory.
    CC_Mqtt311AsyncOpStatus_BadParam = 7, ///< Bad value has been returned from the relevant callback.
    CC_Mqtt311AsyncOpStatus_ValuesLimit ///< Limit for the values
} CC_Mqtt311AsyncOpStatus;

/// @brief Publish ordering configuration
/// @ingroup publish
typedef enum
{
    CC_Mqtt311PublishOrdering_SameQos, ///< Preserve strict order only between same QoS messages.
    CC_Mqtt311PublishOrdering_Full, ///< Preserve strict order between @b all messages.
    CC_Mqtt311PublishOrdering_ValuesLimit ///< Limit for the values
} CC_Mqtt311PublishOrdering;

/// @brief Reason for reporting unsolicited broker disconnection
/// @ingroup global
typedef enum
{
    CC_Mqtt311BrokerDisconnectReason_DisconnectMsg = 0, ///< The broker sent @b DISCONNECT message.
    CC_Mqtt311BrokerDisconnectReason_InternalError = 1, ///< The library encountered internal error and there is a need to close network connection
    CC_Mqtt311BrokerDisconnectReason_NoBrokerResponse = 2, ///< No messages from the broker and no response to @b PINGREQ
    CC_Mqtt311BrokerDisconnectReason_ProtocolError = 3, ///< Protocol error was detected.
    CC_Mqtt311BrokerDisconnectReason_ValuesLimit ///< Limit for the values
} CC_Mqtt311BrokerDisconnectReason;

/// @brief "Connect Return Code" as defined in MQTT v3.1.1 specification
/// @ingroup connect
typedef enum
{
    CC_Mqtt311ConnectReturnCode_Accepted = 0, ///< value <b>Connection Accepted</b>. 
    CC_Mqtt311ConnectReturnCode_InvalidProtocolVersion = 1, ///< value <b>Invalid Protocol Version</b>. 
    CC_Mqtt311ConnectReturnCode_IdRejected = 2, ///< value <b>Client ID is rejected</b>. 
    CC_Mqtt311ConnectReturnCode_ServerUnavailable = 3, ///< value <b>Server is Unavailable</b>. 
    CC_Mqtt311ConnectReturnCode_BadAuth = 4, ///< value <b>Bad authentication details</b>. 
    CC_Mqtt311ConnectReturnCode_NotAuthorized = 5, ///< value <b>No Subscription Existed</b>. 
    CC_Mqtt311ConnectReturnCode_ValuesLimit ///< Upper limit of the values
} CC_Mqtt311ConnectReturnCode;

/// @brief "Subscribe Return Code" as defined in MQTT v3.1.1 specification
/// @ingroup subscribe
typedef enum
{
    CC_Mqtt311SubscribeReturnCode_SuccessQos0 = 0x00, ///< value <b>Maximum QoS 0</b>. 
    CC_Mqtt311SubscribeReturnCode_SuccessQos1 = 0x01, ///< value <b>Maximum QoS 1</b>. 
    CC_Mqtt311SubscribeReturnCode_SuccessQos2 = 0x02, ///< value <b>Maximum QoS 2</b>. 
    CC_Mqtt311SubscribeReturnCode_Failure = 0x80, ///< value <b>Failure</b>. 
} CC_Mqtt311SubscribeReturnCode;

/// @brief "Reason Code" as defined in MQTT311 specification
/// @ingroup global
typedef enum
{
    CC_Mqtt311ReasonCode_Success = 0, ///< value @b Success. 
    CC_Mqtt311ReasonCode_NormalDisconnection = 0, ///< value <b>Normal Disconnection</b>. 
    CC_Mqtt311ReasonCode_GrantedQos0 = 0, ///< value <b>Granted QoS0</b>. 
    CC_Mqtt311ReasonCode_GrantedQos1 = 1, ///< value <b>Granted QoS1</b>. 
    CC_Mqtt311ReasonCode_GrantedQos2 = 2, ///< value <b>Granted QoS2</b>. 
    CC_Mqtt311ReasonCode_DisconnectWithWill = 4, ///< value <b>Disconnect w/ Will</b>. 
    CC_Mqtt311ReasonCode_NoMatchingSubscribers = 16, ///< value <b>No Matching Subscribers</b>. 
    CC_Mqtt311ReasonCode_NoSubscriptionExisted = 17, ///< value <b>No Subscription Existed</b>. 
    CC_Mqtt311ReasonCode_ContinueAuth = 24, ///< value <b>Continue authentication</b>. 
    CC_Mqtt311ReasonCode_ReAuth = 25, ///< value <b>Re-authenticate</b>. 
    CC_Mqtt311ReasonCode_UnspecifiedError = 128, ///< value <b>Unspecified error</b>. 
    CC_Mqtt311ReasonCode_MalformedPacket = 129, ///< value <b>Malformed Packet</b>. 
    CC_Mqtt311ReasonCode_ProtocolError = 130, ///< value <b>Protocol Error</b>. 
    CC_Mqtt311ReasonCode_ImplSpecificError = 131, ///< value <b>Impl. Specific Error</b>. 
    CC_Mqtt311ReasonCode_UnsupportedVersion = 132, ///< value <b>Unsupported Version</b>. 
    CC_Mqtt311ReasonCode_ClientIdInvalid = 133, ///< value <b>Client ID Invalid</b>. 
    CC_Mqtt311ReasonCode_BadUserPassword = 134, ///< value <b>Bad Username/Password</b>. 
    CC_Mqtt311ReasonCode_NotAuthorized = 135, ///< value <b>Not authorized</b>. 
    CC_Mqtt311ReasonCode_ServerUnavailable = 136, ///< value <b>Server unavailable</b>. 
    CC_Mqtt311ReasonCode_ServerBusy = 137, ///< value <b>Server busy</b>. 
    CC_Mqtt311ReasonCode_Banned = 138, ///< value @b Banned. 
    CC_Mqtt311ReasonCode_ServerShuttingDown = 139, ///< value <b>Server shutting down</b>. 
    CC_Mqtt311ReasonCode_BadAuthMethod = 140, ///< value <b>Bad auth method</b>. 
    CC_Mqtt311ReasonCode_KeepAliveTimeout = 141, ///< value <b>Keep Alive timeout</b>. 
    CC_Mqtt311ReasonCode_SessionTakenOver = 142, ///< value <b>Session taken over</b>. 
    CC_Mqtt311ReasonCode_TopicFilterInvalid = 143, ///< value <b>Topic Filter invalid</b>. 
    CC_Mqtt311ReasonCode_TopicNameInvalid = 144, ///< value <b>Topic Name invalid</b>. 
    CC_Mqtt311ReasonCode_PacketIdInUse = 145, ///< value <b>Packet ID in use</b>. 
    CC_Mqtt311ReasonCode_PacketIdNotFound = 146, ///< value <b>Packet ID not found</b>. 
    CC_Mqtt311ReasonCode_ReceiveMaxExceeded = 147, ///< value <b>Receive Max exceeded</b>. 
    CC_Mqtt311ReasonCode_TopicAliasInvalid = 148, ///< value <b>Topic Alias invalid</b>. 
    CC_Mqtt311ReasonCode_PacketTooLarge = 149, ///< value <b>Packet too large</b>. 
    CC_Mqtt311ReasonCode_MsgRateTooHigh = 150, ///< value <b>Message rate too high</b>. 
    CC_Mqtt311ReasonCode_QuotaExceeded = 151, ///< value <b>Quota exceeded</b>. 
    CC_Mqtt311ReasonCode_AdministrativeAction = 152, ///< value <b>Administrative action</b>. 
    CC_Mqtt311ReasonCode_PayloadFormatInvalid = 153, ///< value <b>Payload format invalid</b>. 
    CC_Mqtt311ReasonCode_RetainNotSupported = 154, ///< value <b>Retain not supported</b>. 
    CC_Mqtt311ReasonCode_QosNotSupported = 155, ///< value <b>QoS not supported</b>. 
    CC_Mqtt311ReasonCode_UseAnotherServer = 156, ///< value <b>Use another server</b>. 
    CC_Mqtt311ReasonCode_ServerMoved = 157, ///< value <b>Server moved</b>. 
    CC_Mqtt311ReasonCode_SharedSubNotSuppored = 158, ///< value <b>Shared Sub not supported</b>. 
    CC_Mqtt311ReasonCode_ConnectionRateExceeded = 159, ///< value <b>Connection rate exceeded</b>. 
    CC_Mqtt311ReasonCode_MaxConnectTime = 160, ///< value <b>Maximum connect time</b>. 
    CC_Mqtt311ReasonCode_SubIdsNotSupported = 161, ///< value <b>Sub IDs not supported</b>. 
    CC_Mqtt311ReasonCode_WildcardSubsNotSupported = 162, ///< value <b>Wildcard Subs not supported</b>. 
} CC_Mqtt311ReasonCode;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt311ClientHandle
/// @ingroup client
struct CC_Mqtt311Client;

/// @brief Handle used to access client specific data structures.
/// @details Returned by cc_mqtt311_client_alloc() function.
/// @ingroup client
typedef CC_Mqtt311Client* CC_Mqtt311ClientHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt311ConnectHandle
/// @ingroup connect
struct CC_Mqtt311Connect;

/// @brief Handle for "connect" operation.
/// @details Returned by cc_mqtt311_client_connect_prepare() function.
/// @ingroup connect
typedef CC_Mqtt311Connect* CC_Mqtt311ConnectHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt311DisconnectHandle
/// @ingroup disconnect
struct CC_Mqtt311Disconnect;

/// @brief Handle for "disconnect" operation.
/// @details Returned by cc_mqtt311_client_disconnect_prepare() function.
/// @ingroup disconnect
typedef CC_Mqtt311Disconnect* CC_Mqtt311DisconnectHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt311SubscribeHandle
/// @ingroup subscribe
struct CC_Mqtt311Subscribe;

/// @brief Handle for "subscribe" operation.
/// @details Returned by cc_mqtt311_client_subscribe_prepare() function.
/// @ingroup subscribe
typedef CC_Mqtt311Subscribe* CC_Mqtt311SubscribeHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt311UnsubscribeHandle
/// @ingroup unsubscribe
struct CC_Mqtt311Unsubscribe;

/// @brief Handle for "unsubscribe" operation.
/// @details Returned by cc_mqtt311_client_unsubscribe_prepare() function.
/// @ingroup unsubscribe
typedef CC_Mqtt311Unsubscribe* CC_Mqtt311UnsubscribeHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt311PublishHandle
/// @ingroup publish
struct CC_Mqtt311Publish;

/// @brief Handle for "publish" operation.
/// @details Returned by cc_mqtt311_client_publish_prepare() function.
/// @ingroup publish
typedef CC_Mqtt311Publish* CC_Mqtt311PublishHandle;

/// @brief Wraping structre of the single "User Property".
/// @see @b cc_mqtt311_client_init_user_prop()
/// @ingroup global
typedef struct
{
    const char* m_key; ///< Key string, mustn't be NULL
    const char* m_value; ///< Value string, can be NULL
} CC_Mqtt311UserProp;

/// @brief Configuration structure to be passed to the @b cc_mqtt311_client_connect_config().
/// @see @b cc_mqtt311_client_connect_init_config()
/// @ingroup connect
typedef struct
{
    const char* m_clientId; ///< Zero terminated Client ID string, can be NULL. When NULL means empty "Client ID". Defaults to NULL.
    const char* m_username; ///< Zero terminated Username string, can be NULL. When NULL means no username value. Defaults to NULL.
    const unsigned char* m_password; ///< Pointer to password buffer, can be NULL, defaults to NULL.
    unsigned m_passwordLen; ///< Number of password bytes. When 0 means no password value. Defaults to 0.
    unsigned m_keepAlive; ///< Keep alive seconds configuration, defaults to 60.
    bool m_cleanSession; ///< Clean session flag, defaults to false.
} CC_Mqtt311ConnectConfig;

/// @brief Configuration structure to be passed to the @b cc_mqtt311_client_connect_config_will().
/// @see @b cc_mqtt311_client_connect_init_config_will()
/// @ingroup connect
typedef struct
{
    const char* m_topic; ///< Will topic string, must NOT be NULL or empty. Defaults to NULL.
    const unsigned char* m_data; ///< Will message data, can be NULL. Defaults to NULL.
    unsigned m_dataLen; ///< Number of will data bytes. When 0 means no data. Defaults to 0.
    CC_Mqtt311QoS m_qos; ///< QoS value of the will message, defaults to CC_Mqtt311QoS_AtMostOnceDelivery.
    bool m_retain; ///< "Retain" flag, defaults to false.
} CC_Mqtt311ConnectWillConfig;

/// @brief Response information from broker to "connect" request
/// @ingroup connect
typedef struct 
{
    CC_Mqtt311ConnectReturnCode m_returnCode; ///< "Connection Return Code" reported by the broker
    bool m_sessionPresent; ///< "Session Present" indication.
} CC_Mqtt311ConnectResponse;

/// @brief Topic filter configuration structure of the "subscribe" operation.
/// @ingroup subscribe
/// @see @b cc_mqtt311_client_subscribe_init_config_topic()
typedef struct
{
    const char* m_topic; ///< "Topic Filter" string, mustn't be NULL
    CC_Mqtt311QoS m_maxQos; ///< "Maximum QoS" value, defaults to @ref CC_Mqtt311QoS_ExactlyOnceDelivery.
} CC_Mqtt311SubscribeTopicConfig;

/// @brief Response information from broker to "subscribe" request
/// @ingroup subscribe
typedef struct 
{
    const CC_Mqtt311SubscribeReturnCode* m_returnCodes; ///< Pointer to array contianing per-topic subscription return codes.
    unsigned m_returnCodesCount; ///< Amount of return codes in the array.
} CC_Mqtt311SubscribeResponse;

/// @brief Topic filter configuration structure of the "unsubscribe" operation.
/// @see @b cc_mqtt311_client_unsubscribe_init_config_topic()
/// @ingroup unsubscribe
typedef struct
{
    const char* m_topic; ///< "Topic Filter" string, mustn't be NULL
} CC_Mqtt311UnsubscribeTopicConfig;

/// @brief Received message information
/// @ingroup global
typedef struct
{
    const char* m_topic; ///< Topic used to publish the message
    const unsigned char* m_data; ///< Pointer to the temporary buffer containin message data
    unsigned m_dataLen; ///< Amount of data bytes 
    const char* m_responseTopic; ///< "Response Topic" property when provided, NULL if not.
    const unsigned char* m_correlationData; ///< Pointer to the "Correlation Data" property value when provided, NULL if not.
    unsigned m_correlationDataLen; ///< Amount of "Correlation Data" bytes;
    const CC_Mqtt311UserProp* m_userProps; ///< Pointer to the "User Property" properties array when provided, NULL if not.
    unsigned m_userPropsCount; ///< Amount of "User Property" properties
    const char* m_contentType; ///< "Content Type" property if provided, NULL if not.
    const unsigned* m_subIds; ///< Pointer to array containing "Subscription Identifier" properties list when provided, NULL if not.
    unsigned m_subIdsCount; ///< Amount of "Subscription Identifiers" in array.
    unsigned m_messageExpiryInterval; ///< "Message Expiry Interval" property, defaults to 0 when not reported.
    CC_Mqtt311QoS m_qos; ///< QoS value used by the broker to report the message.
    bool m_retained; ///< Indication of whether the received message was "retained".
} CC_Mqtt311MessageInfo;

/// @brief Configuration structure to be passed to the @b cc_mqtt311_client_publish_config().
/// @see @b cc_mqtt311_client_publish_init_config()
/// @ingroup publish
typedef struct
{
    const char* m_topic; ///< Publish topic, cannot be NULL.
    const unsigned char* m_data; ///< Pointer to publish data buffer, defaults to NULL.
    unsigned m_dataLen; ///< Amount of bytes in the publish data buffer, defaults to 0.
    CC_Mqtt311QoS m_qos; ///< Publish QoS value, defaults to @ref CC_Mqtt311QoS_AtMostOnceDelivery.
    bool m_retain; ///< "Retain" flag, defaults to false.
} CC_Mqtt311PublishConfig;

/// @brief Callback used to request time measurement.
/// @details The callback is set using
///     cc_mqtt311_client_set_next_tick_program_callback() function.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt311_client_set_next_tick_program_callback() function.
/// @param[in] duration Time duration in @b milliseconds. After the requested
///     time expires, the cc_mqtt311_client_tick() function is expected to be invoked.
/// @ingroup client
typedef void (*CC_Mqtt311NextTickProgramCb)(void* data, unsigned duration);

/// @brief Callback used to request termination of existing time measurement.
/// @details The callback is set using
///     cc_mqtt311_client_set_cancel_next_tick_wait_callback() function.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt311_client_set_cancel_next_tick_wait_callback() function.
/// @return Number of elapsed milliseconds since last time measurement request.
/// @ingroup client
typedef unsigned (*CC_Mqtt311CancelNextTickWaitCb)(void* data);

/// @brief Callback used to request to send data to the broker.
/// @details The callback is set using
///     cc_mqtt311_client_set_send_output_data_callback() function. The reported
///     data resides in internal data structures of the client library, which
///     can be updated / deleted right after the callback function returns. It means
///     the data may need to be copied into some other buffer which will be
///     held intact until the send over I/O link operation is complete.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt311_client_set_send_output_data_callback() function.
/// @param[in] buf Pointer to the buffer containing data to send
/// @param[in] bufLen Number of bytes in the buffer
/// @post The buffer data can be deallocated / overwritten after the callback function returns.
/// @ingroup client
typedef void (*CC_Mqtt311SendOutputDataCb)(void* data, const unsigned char* buf, unsigned bufLen);

/// @brief Callback used to report unsolicited disconnection of the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @ingroup client
typedef void (*CC_Mqtt311BrokerDisconnectReportCb)(void* data, CC_Mqtt311BrokerDisconnectReason reason);

/// @brief Callback used to report new message received of the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] info Message information. Will NOT be NULL.
/// @post The data members of the reported info can NOT be accessed after the function returns.
/// @ingroup client
typedef void (*CC_Mqtt311MessageReceivedReportCb)(void* data, const CC_Mqtt311MessageInfo* info);

/// @brief Callback used to report discovered errors.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] msg Error log message.
/// @ingroup client
typedef void (*CC_Mqtt311ErrorLogCb)(void* data, const char* msg);

/// @brief Callback used to report completion of the "connect" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt311_client_connect_send().
/// @param[in] status Status of the "connect" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt311AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup connect
typedef void (*CC_Mqtt311ConnectCompleteCb)(void* data, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);

/// @brief Callback used to report completion of the "subscribe" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt311_client_subscribe_send().
/// @param[in] handle Handle returned by @b cc_mqtt311_client_subscribe_prepare() function. When the 
///     callback is invoked the handle is already invalid and cannot be used in any relevant 
///     function invocation, but it allows end application to identify the original "subscribe" operation
///     and use the same callback function in parallel requests.
/// @param[in] status Status of the "subscribe" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt311AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup subscribe
typedef void (*CC_Mqtt311SubscribeCompleteCb)(void* data, CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response);

/// @brief Callback used to report completion of the "unsubscribe" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt311_client_unsubscribe_send().
/// @param[in] handle Handle returned by @b cc_mqtt311_client_unsubscribe_prepare() function. When the 
///     callback is invoked the handle is already invalid and cannot be used in any relevant 
///     function invocation, but it allows end application to identify the original "unsubscribe" operation
///     and use the same callback function in parallel requests.
/// @param[in] status Status of the "unsubscribe" operation.
/// @ingroup unsubscribe
typedef void (*CC_Mqtt311UnsubscribeCompleteCb)(void* data, CC_Mqtt311UnsubscribeHandle handle, CC_Mqtt311AsyncOpStatus status);

/// @brief Callback used to report completion of the "publish" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt311_client_publish_send().
/// @param[in] handle Handle returned by @b cc_mqtt311_client_publish_prepare() function. When the 
///     callback is invoked the handle is already invalid and cannot be used in any relevant 
///     function invocation, but it allows end application to identify the original "publish" operation
///     and use the same callback function in parallel requests.
/// @param[in] status Status of the "publish" operation.
/// @ingroup publish
typedef void (*CC_Mqtt311PublishCompleteCb)(void* data, CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

#ifdef __cplusplus
}
#endif

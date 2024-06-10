#pragma once

#include "cc_mqtt311_client/common.h"

#include "UnitTestProtocolDefs.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <optional>
#include <tuple>
#include <vector>

class UnitTestCommonBase
{
public:
    using UnitTestData = std::vector<std::uint8_t>;

    struct LibFuncs
    {
        CC_Mqtt311ClientHandle (*m_alloc)() = nullptr;
        void (*m_free)(CC_Mqtt311ClientHandle) = nullptr;
        void (*m_tick)(CC_Mqtt311ClientHandle, unsigned) = nullptr;
        unsigned (*m_process_data)(CC_Mqtt311ClientHandle, const unsigned char*, unsigned) = nullptr;
        void (*m_notify_network_disconnected)(CC_Mqtt311ClientHandle) = nullptr;
        bool (*m_is_network_disconnected)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_set_default_response_timeout)(CC_Mqtt311ClientHandle, unsigned) = nullptr;
        unsigned (*m_get_default_response_timeout)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_pub_topic_alias_alloc)(CC_Mqtt311ClientHandle, const char*, unsigned) = nullptr;
        CC_Mqtt311ErrorCode (*m_pub_topic_alias_free)(CC_Mqtt311ClientHandle, const char*) = nullptr;
        unsigned (*m_pub_topic_alias_count)(CC_Mqtt311ClientHandle) = nullptr;
        bool (*m_pub_topic_alias_is_allocated)(CC_Mqtt311ClientHandle, const char*) = nullptr;
        CC_Mqtt311ErrorCode (*m_set_verify_outgoing_topic_enabled)(CC_Mqtt311ClientHandle, bool) = nullptr;
        bool (*m_get_verify_outgoing_topic_enabled)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_set_verify_incoming_topic_enabled)(CC_Mqtt311ClientHandle, bool) = nullptr;
        bool (*m_get_verify_incoming_topic_enabled)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_set_verify_incoming_msg_subscribed)(CC_Mqtt311ClientHandle, bool) = nullptr;
        bool (*m_get_verify_incoming_msg_subscribed)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311ConnectHandle (*m_connect_prepare)(CC_Mqtt311ClientHandle, CC_Mqtt311ErrorCode*) = nullptr;
        void (*m_connect_init_config)(CC_Mqtt311ConnectConfig*) = nullptr;
        void (*m_connect_init_config_will)(CC_Mqtt311ConnectWillConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_connect_set_response_timeout)(CC_Mqtt311ConnectHandle, unsigned) = nullptr;
        unsigned (*m_connect_get_response_timeout)(CC_Mqtt311ConnectHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_connect_config)(CC_Mqtt311ConnectHandle, const CC_Mqtt311ConnectConfig*) = nullptr;        
        CC_Mqtt311ErrorCode (*m_connect_config_will)(CC_Mqtt311ConnectHandle, const CC_Mqtt311ConnectWillConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_connect_send)(CC_Mqtt311ConnectHandle, CC_Mqtt311ConnectCompleteCb, void*) = nullptr;
        CC_Mqtt311ErrorCode (*m_connect_cancel)(CC_Mqtt311ConnectHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_connect)(CC_Mqtt311ClientHandle handle, const CC_Mqtt311ConnectConfig*, const CC_Mqtt311ConnectWillConfig*, CC_Mqtt311ConnectCompleteCb, void*) = nullptr;
        bool (*m_is_connected)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311DisconnectHandle (*m_disconnect_prepare)(CC_Mqtt311ClientHandle, CC_Mqtt311ErrorCode*) = nullptr; 
        CC_Mqtt311ErrorCode (*m_disconnect_send)(CC_Mqtt311DisconnectHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_disconnect_cancel)(CC_Mqtt311DisconnectHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_disconnect)(CC_Mqtt311ClientHandle) = nullptr;
        CC_Mqtt311SubscribeHandle (*m_subscribe_prepare)(CC_Mqtt311ClientHandle, CC_Mqtt311ErrorCode*) = nullptr;
        CC_Mqtt311ErrorCode (*m_subscribe_set_response_timeout)(CC_Mqtt311SubscribeHandle, unsigned) = nullptr;
        unsigned (*m_subscribe_get_response_timeout)(CC_Mqtt311SubscribeHandle) = nullptr;
        void (*m_subscribe_init_config_topic)(CC_Mqtt311SubscribeTopicConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_subscribe_config_topic)(CC_Mqtt311SubscribeHandle, const CC_Mqtt311SubscribeTopicConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_subscribe_send)(CC_Mqtt311SubscribeHandle, CC_Mqtt311SubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt311ErrorCode (*m_subscribe_cancel)(CC_Mqtt311SubscribeHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_subscribe)(CC_Mqtt311ClientHandle, const CC_Mqtt311SubscribeTopicConfig*, unsigned, CC_Mqtt311SubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt311UnsubscribeHandle (*m_unsubscribe_prepare)(CC_Mqtt311ClientHandle, CC_Mqtt311ErrorCode*) = nullptr;
        CC_Mqtt311ErrorCode (*m_unsubscribe_set_response_timeout)(CC_Mqtt311UnsubscribeHandle, unsigned) = nullptr;
        unsigned (*m_unsubscribe_get_response_timeout)(CC_Mqtt311UnsubscribeHandle) = nullptr;
        void (*m_unsubscribe_init_config_topic)(CC_Mqtt311UnsubscribeTopicConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_unsubscribe_config_topic)(CC_Mqtt311UnsubscribeHandle, const CC_Mqtt311UnsubscribeTopicConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_unsubscribe_send)(CC_Mqtt311UnsubscribeHandle, CC_Mqtt311UnsubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt311ErrorCode (*m_unsubscribe_cancel)(CC_Mqtt311UnsubscribeHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_unsubscribe)(CC_Mqtt311ClientHandle, const CC_Mqtt311UnsubscribeTopicConfig*, unsigned, CC_Mqtt311UnsubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt311PublishHandle (*m_publish_prepare)(CC_Mqtt311ClientHandle, CC_Mqtt311ErrorCode*) = nullptr;
        unsigned (*m_publish_count)(CC_Mqtt311ClientHandle) = nullptr;
        void (*m_publish_init_config)(CC_Mqtt311PublishConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish_set_response_timeout)(CC_Mqtt311PublishHandle, unsigned) = nullptr;
        unsigned (*m_publish_get_response_timeout)(CC_Mqtt311PublishHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish_set_resend_attempts)(CC_Mqtt311PublishHandle, unsigned) = nullptr;
        unsigned (*m_publish_get_resend_attempts)(CC_Mqtt311PublishHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish_config)(CC_Mqtt311PublishHandle, const CC_Mqtt311PublishConfig*) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish_send)(CC_Mqtt311PublishHandle, CC_Mqtt311PublishCompleteCb, void*) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish_cancel)(CC_Mqtt311PublishHandle) = nullptr;
        bool (*m_publish_was_initiated)(CC_Mqtt311PublishHandle) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish)(CC_Mqtt311ClientHandle, const CC_Mqtt311PublishConfig*, CC_Mqtt311PublishCompleteCb, void*) = nullptr;
        CC_Mqtt311ErrorCode (*m_publish_set_ordering)(CC_Mqtt311ClientHandle, CC_Mqtt311PublishOrdering) = nullptr;
        CC_Mqtt311PublishOrdering (*m_publish_get_ordering)(CC_Mqtt311ClientHandle) = nullptr;
        void (*m_set_next_tick_program_callback)(CC_Mqtt311ClientHandle, CC_Mqtt311NextTickProgramCb, void*) = nullptr;
        void (*m_set_cancel_next_tick_wait_callback)(CC_Mqtt311ClientHandle, CC_Mqtt311CancelNextTickWaitCb, void*) = nullptr;        
        void (*m_set_send_output_data_callback)(CC_Mqtt311ClientHandle, CC_Mqtt311SendOutputDataCb, void*) = nullptr;
        void (*m_set_broker_disconnect_report_callback)(CC_Mqtt311ClientHandle, CC_Mqtt311BrokerDisconnectReportCb, void*) = nullptr;        
        void (*m_set_message_received_report_callback)(CC_Mqtt311ClientHandle, CC_Mqtt311MessageReceivedReportCb, void*) = nullptr;        
        void (*m_set_error_log_callback)(CC_Mqtt311ClientHandle, CC_Mqtt311ErrorLogCb, void*) = nullptr;        
    };

    struct UnitTestDeleter
    {
        UnitTestDeleter() = default;
        explicit UnitTestDeleter(const LibFuncs& ops) : 
            m_free(ops.m_free)
        {
        }

        void operator()(CC_Mqtt311Client* ptr)
        {
            m_free(ptr);
        }

    private:
        void (*m_free)(CC_Mqtt311ClientHandle) = nullptr;
    }; 

    using UnitTestClientPtr = std::unique_ptr<CC_Mqtt311Client, UnitTestDeleter>;


protected:

    explicit UnitTestCommonBase(const LibFuncs& funcs);

    static constexpr unsigned UnitTestDefaultOpTimeoutMs = 2000;
    static constexpr unsigned UnitTestDefaultKeepAliveMs = 60000;


    struct UnitTestConnectResponse
    {
        CC_Mqtt311ConnectReturnCode m_returnCode = CC_Mqtt311ConnectReturnCode_Accepted;
        bool m_sessionPresent = false;

        UnitTestConnectResponse& operator=(const CC_Mqtt311ConnectResponse& response);
    };

    struct UnitTestSubscribeResponse
    {
        std::vector<CC_Mqtt311SubscribeReturnCode> m_returnCodes;

        UnitTestSubscribeResponse() = default;
        UnitTestSubscribeResponse(const UnitTestSubscribeResponse& other) = default;
        explicit UnitTestSubscribeResponse(const CC_Mqtt311SubscribeResponse& other)
        {
            *this = other;
        }

        UnitTestSubscribeResponse& operator=(const UnitTestSubscribeResponse&) = default;
        UnitTestSubscribeResponse& operator=(const CC_Mqtt311SubscribeResponse& response);
    };

    struct UnitTestConnectResponseInfo
    {
        CC_Mqtt311AsyncOpStatus m_status = CC_Mqtt311AsyncOpStatus_ValuesLimit;
        UnitTestConnectResponse m_response;
    };

    struct UnitTestSubscribeResponseInfo
    {
        CC_Mqtt311AsyncOpStatus m_status = CC_Mqtt311AsyncOpStatus_ValuesLimit;
        UnitTestSubscribeResponse m_response;
    };  

    struct UnitTestUnsubscribeResponseInfo
    {
        CC_Mqtt311AsyncOpStatus m_status = CC_Mqtt311AsyncOpStatus_ValuesLimit;
    };          

    struct UnitTestMessageInfo
    {
        std::string m_topic;
        UnitTestData m_data;
        CC_Mqtt311QoS m_qos = CC_Mqtt311QoS_ValuesLimit;
        bool m_retained = false;     

        UnitTestMessageInfo() = default;
        UnitTestMessageInfo(const UnitTestMessageInfo&) = default;

        UnitTestMessageInfo(const CC_Mqtt311MessageInfo& other)
        {
            *this = other;
        }

        UnitTestMessageInfo& operator=(const UnitTestMessageInfo&) = default;
        UnitTestMessageInfo& operator=(const CC_Mqtt311MessageInfo& other);        
    };    

    struct UnitTestPublishResponseInfo
    {
        CC_Mqtt311AsyncOpStatus m_status = CC_Mqtt311AsyncOpStatus_ValuesLimit;
    };         

    struct UnitTestDisconnectInfo
    {
        CC_Mqtt311BrokerDisconnectReason m_reason;
    };

    struct TickInfo
    {
        unsigned m_requested = 0U;
        unsigned m_elapsed = 0U;
    };    

    struct UnitTestConnectResponseConfig
    {
        CC_Mqtt311ConnectReturnCode m_returnCode = CC_Mqtt311ConnectReturnCode_Accepted;
        bool m_sessionPresent = false;
    };

    void unitTestSetUp();
    void unitTestTearDown();
    UnitTestClientPtr apiAllocClient(bool addLog = false);

    decltype(auto) unitTestSentData()
    {
        return m_sentData;
    }

    const TickInfo* unitTestTickReq();
    bool unitTestCheckNoTicks();
    void unitTestTick(CC_Mqtt311Client* client, unsigned ms = 0, bool forceTick = false);
    CC_Mqtt311ErrorCode unitTestSendConnect(CC_Mqtt311ConnectHandle& connect);
    CC_Mqtt311ErrorCode unitTestSendSubscribe(CC_Mqtt311SubscribeHandle& subscribe);
    CC_Mqtt311ErrorCode unitTestSendUnsubscribe(CC_Mqtt311UnsubscribeHandle& unsubscribe);
    CC_Mqtt311ErrorCode unitTestSendPublish(CC_Mqtt311PublishHandle& publish, bool clearHandle = true);
    UniTestsMsgPtr unitTestGetSentMessage();
    bool unitTestHasSentMessage() const;
    bool unitTestIsConnectComplete();
    const UnitTestConnectResponseInfo& unitTestConnectResponseInfo();
    void unitTestPopConnectResponseInfo();
    bool unitTestIsSubscribeComplete();
    const UnitTestSubscribeResponseInfo& unitTestSubscribeResponseInfo();
    void unitTestPopSubscribeResponseInfo();
    bool unitTestIsUnsubscribeComplete();
    const UnitTestUnsubscribeResponseInfo& unitTestUnsubscribeResponseInfo();
    void unitTestPopUnsubscribeResponseInfo();
    bool unitTestIsPublishComplete();
    const UnitTestPublishResponseInfo& unitTestPublishResponseInfo();
    void unitTestPopPublishResponseInfo();
    void unitTestReceiveMessage(CC_Mqtt311Client* client, const UnitTestMessage& msg, bool reportReceivedData = true);
    bool unitTestHasDisconnectInfo() const;
    const UnitTestDisconnectInfo& unitTestDisconnectInfo() const;
    void unitTestPopDisconnectInfo();
    bool unitTestHasMessageRecieved();
    const UnitTestMessageInfo& unitTestReceivedMessageInfo();
    void unitTestPopReceivedMessageInfo();      
    void unitTestPerformConnect(
        CC_Mqtt311Client* client, 
        const CC_Mqtt311ConnectConfig* config,
        const CC_Mqtt311ConnectWillConfig* willConfig = nullptr,
        const UnitTestConnectResponseConfig* responseConfig = nullptr);

    void unitTestPerformBasicConnect(
        CC_Mqtt311Client* client, 
        const char* clientId, 
        bool cleanSession = true);

    void unitTestPerformDisconnect(CC_Mqtt311Client* client);

    void unitTestPerformSubscribe(
        CC_Mqtt311Client* client, 
        CC_Mqtt311SubscribeTopicConfig* topicConfigs, 
        unsigned topicConfigsCount = 1U);
    void unitTestPerformBasicSubscribe(CC_Mqtt311Client* client, const char* topic);

    void unitTestVerifyDisconnectSent();

    void unitTestClearState(bool preserveTicks = true);

    // API wrappers
    UnitTestClientPtr apiAlloc();
    void apiNotifyNetworkDisconnected(CC_Mqtt311Client* client);
    bool apiIsNetworkDisconnected(CC_Mqtt311Client* client);
    CC_Mqtt311ErrorCode apiSetDefaultResponseTimeout(CC_Mqtt311Client* client, unsigned ms);
    void apiSetVerifyIncomingMsgSubscribed(CC_Mqtt311Client* client, bool enabled);
    CC_Mqtt311ConnectHandle apiConnectPrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec);
    void apiConnectInitConfig(CC_Mqtt311ConnectConfig* config);
    void apiConnectInitConfigWill(CC_Mqtt311ConnectWillConfig* config);
    CC_Mqtt311ErrorCode apiConnectConfig(CC_Mqtt311ConnectHandle handle, const CC_Mqtt311ConnectConfig* config);
    CC_Mqtt311ErrorCode apiConnectConfigWill(CC_Mqtt311ConnectHandle handle, const CC_Mqtt311ConnectWillConfig* config);
    bool apiIsConnected(CC_Mqtt311Client* client);
    CC_Mqtt311DisconnectHandle apiDisconnectPrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec);
    CC_Mqtt311SubscribeHandle apiSubscribePrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec);
    CC_Mqtt311ErrorCode apiSubscribeSetResponseTimeout(CC_Mqtt311SubscribeHandle handle, unsigned ms);
    void apiSubscribeInitConfigTopic(CC_Mqtt311SubscribeTopicConfig* config);
    CC_Mqtt311ErrorCode apiSubscribeConfigTopic(CC_Mqtt311SubscribeHandle handle, const CC_Mqtt311SubscribeTopicConfig* config);
    CC_Mqtt311ErrorCode apiSubscribe(CC_Mqtt311Client* client, CC_Mqtt311SubscribeTopicConfig* config, unsigned count = 1U);
    CC_Mqtt311UnsubscribeHandle apiUnsubscribePrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec);
    CC_Mqtt311ErrorCode apiUnsubscribeSetResponseTimeout(CC_Mqtt311UnsubscribeHandle handle, unsigned ms);
    void apiUnsubscribeInitConfigTopic(CC_Mqtt311UnsubscribeTopicConfig* config);
    CC_Mqtt311ErrorCode apiUnsubscribeConfigTopic(CC_Mqtt311UnsubscribeHandle handle, const CC_Mqtt311UnsubscribeTopicConfig* config);
    CC_Mqtt311PublishHandle apiPublishPrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec);
    unsigned apiPublishCount(CC_Mqtt311Client* client);
    void apiPublishInitConfig(CC_Mqtt311PublishConfig* config);
    CC_Mqtt311ErrorCode apiPublishSetResponseTimeout(CC_Mqtt311PublishHandle handle, unsigned ms);
    CC_Mqtt311ErrorCode apiPublishConfig(CC_Mqtt311PublishHandle handle, const CC_Mqtt311PublishConfig* config);
    CC_Mqtt311ErrorCode apiPublishCancel(CC_Mqtt311PublishHandle handle);
    bool apiPublishWasInitiated(CC_Mqtt311PublishHandle handle);
    CC_Mqtt311ErrorCode apiPublishSetOrdering(CC_Mqtt311ClientHandle handle, CC_Mqtt311PublishOrdering ordering);
    CC_Mqtt311PublishOrdering apiPublishGetOrdering(CC_Mqtt311ClientHandle handle);
    void apiSetNextTickProgramCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311NextTickProgramCb cb, void* data);    
    void apiSetCancelNextTickWaitCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311CancelNextTickWaitCb cb, void* data);    
    void apiSetSendOutputDataCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311SendOutputDataCb cb, void* data);    
    void apiSetBrokerDisconnectReportCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311BrokerDisconnectReportCb cb, void* data);    
    void apiSetMessageReceivedReportCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311MessageReceivedReportCb cb, void* data);    

private:

    static void unitTestErrorLogCb(void* obj, const char* msg);
    static void unitTestBrokerDisconnectedCb(void* obj, CC_Mqtt311BrokerDisconnectReason reason);
    static void unitTestMessageReceivedCb(void* obj, const CC_Mqtt311MessageInfo* info);
    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen);
    static void unitTestProgramNextTickCb(void* obj, unsigned duration);
    static unsigned unitTestCancelNextTickWaitCb(void* obj);
    static void unitTestConnectCompleteCb(void* obj, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);
    static void unitTestSubscribeCompleteCb(void* obj, CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response);
    static void unitTestUnsubscribeCompleteCb(void* obj, CC_Mqtt311UnsubscribeHandle handle, CC_Mqtt311AsyncOpStatus status);
    static void unitTestPublishCompleteCb(void* obj, CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

    LibFuncs m_funcs;  
    std::vector<TickInfo> m_tickReq;
    std::vector<std::uint8_t> m_sentData;
    std::vector<std::uint8_t> m_receivedData;
    std::vector<UnitTestConnectResponseInfo> m_connectResp;
    std::vector<UnitTestSubscribeResponseInfo> m_subscribeResp;
    std::vector<UnitTestUnsubscribeResponseInfo> m_unsubscribeResp;
    std::vector<UnitTestPublishResponseInfo> m_publishResp;
    std::vector<UnitTestDisconnectInfo> m_disconnectInfo;
    std::vector<UnitTestMessageInfo> m_receivedMessages;
};
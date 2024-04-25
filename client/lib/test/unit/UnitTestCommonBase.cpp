#include "UnitTestCommonBase.h"

#include <cstdlib>
#include <iostream>

namespace 
{

#define test_assert(cond_) \
    assert(cond_); \
    if (!(cond_)) { \
        std::cerr << "\nAssertion failure (" << #cond_ << ") in " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    }


void assignStringInternal(std::string& dest, const char* source)
{
    dest.clear();
    if (source != nullptr)
    {
        dest = source;
    }
}

template <typename TDest, typename TSrc>
void assignDataInternal(TDest& dest, TSrc* source, unsigned count)
{
    dest.clear();
    if (count > 0U) {
        std::copy_n(source, count, std::back_inserter(dest));
    }
}

} // namespace 

UnitTestCommonBase::UnitTestCommonBase(const LibFuncs& funcs) :
    m_funcs(funcs)
{
    test_assert(m_funcs.m_alloc != nullptr);
    test_assert(m_funcs.m_free != nullptr);
    test_assert(m_funcs.m_tick != nullptr);
    test_assert(m_funcs.m_process_data != nullptr);
    test_assert(m_funcs.m_notify_network_disconnected != nullptr);
    test_assert(m_funcs.m_is_network_disconnected != nullptr);
    test_assert(m_funcs.m_set_default_response_timeout != nullptr);
    test_assert(m_funcs.m_get_default_response_timeout != nullptr);
    test_assert(m_funcs.m_set_verify_outgoing_topic_enabled != nullptr);
    test_assert(m_funcs.m_get_verify_outgoing_topic_enabled != nullptr);
    test_assert(m_funcs.m_set_verify_incoming_topic_enabled != nullptr);
    test_assert(m_funcs.m_get_verify_incoming_topic_enabled != nullptr);
    test_assert(m_funcs.m_set_verify_incoming_msg_subscribed != nullptr);
    test_assert(m_funcs.m_get_verify_incoming_msg_subscribed != nullptr);
    test_assert(m_funcs.m_connect_prepare != nullptr);
    test_assert(m_funcs.m_connect_init_config != nullptr);
    test_assert(m_funcs.m_connect_init_config_will != nullptr);
    test_assert(m_funcs.m_connect_set_response_timeout != nullptr);
    test_assert(m_funcs.m_connect_get_response_timeout != nullptr);
    test_assert(m_funcs.m_connect_config != nullptr);
    test_assert(m_funcs.m_connect_config_will != nullptr);
    test_assert(m_funcs.m_connect_send != nullptr);
    test_assert(m_funcs.m_connect_cancel != nullptr);
    test_assert(m_funcs.m_connect != nullptr);
    test_assert(m_funcs.m_is_connected != nullptr);
    test_assert(m_funcs.m_disconnect_prepare != nullptr);
    test_assert(m_funcs.m_disconnect_send != nullptr);
    test_assert(m_funcs.m_disconnect_cancel != nullptr);
    test_assert(m_funcs.m_disconnect != nullptr);
    test_assert(m_funcs.m_subscribe_prepare != nullptr);
    test_assert(m_funcs.m_subscribe_set_response_timeout != nullptr);
    test_assert(m_funcs.m_subscribe_get_response_timeout != nullptr);
    test_assert(m_funcs.m_subscribe_init_config_topic != nullptr);
    test_assert(m_funcs.m_subscribe_config_topic != nullptr);
    test_assert(m_funcs.m_subscribe_send != nullptr);
    test_assert(m_funcs.m_subscribe_cancel != nullptr);
    test_assert(m_funcs.m_subscribe != nullptr);
    test_assert(m_funcs.m_unsubscribe_prepare != nullptr);
    test_assert(m_funcs.m_unsubscribe_set_response_timeout != nullptr);
    test_assert(m_funcs.m_unsubscribe_get_response_timeout != nullptr);
    test_assert(m_funcs.m_unsubscribe_init_config_topic != nullptr);
    test_assert(m_funcs.m_unsubscribe_config_topic != nullptr);
    test_assert(m_funcs.m_unsubscribe_send != nullptr);
    test_assert(m_funcs.m_unsubscribe_cancel != nullptr);    
    test_assert(m_funcs.m_unsubscribe != nullptr);    
    test_assert(m_funcs.m_publish_prepare != nullptr);    
    test_assert(m_funcs.m_publish_count != nullptr);    
    test_assert(m_funcs.m_publish_init_config != nullptr);    
    test_assert(m_funcs.m_publish_set_response_timeout != nullptr);    
    test_assert(m_funcs.m_publish_get_response_timeout != nullptr);    
    test_assert(m_funcs.m_publish_set_resend_attempts != nullptr);    
    test_assert(m_funcs.m_publish_get_resend_attempts != nullptr);      
    test_assert(m_funcs.m_publish_config != nullptr);      
    test_assert(m_funcs.m_publish_send != nullptr);  
    test_assert(m_funcs.m_publish_cancel != nullptr);  
    test_assert(m_funcs.m_publish != nullptr);  
    test_assert(m_funcs.m_publish_set_ordering != nullptr);  
    test_assert(m_funcs.m_publish_get_ordering != nullptr);  
    test_assert(m_funcs.m_set_next_tick_program_callback != nullptr); 
    test_assert(m_funcs.m_set_cancel_next_tick_wait_callback != nullptr); 
    test_assert(m_funcs.m_set_send_output_data_callback != nullptr); 
    test_assert(m_funcs.m_set_broker_disconnect_report_callback != nullptr); 
    test_assert(m_funcs.m_set_message_received_report_callback != nullptr); 
    test_assert(m_funcs.m_set_error_log_callback != nullptr); 
}


UnitTestCommonBase::UnitTestConnectResponse& UnitTestCommonBase::UnitTestConnectResponse::operator=(const CC_Mqtt311ConnectResponse& response)
{
    auto thisTie = std::tie(m_returnCode, m_sessionPresent);
    auto responseTie = std::forward_as_tuple(response.m_returnCode, response.m_sessionPresent);            
    thisTie = responseTie;
    return *this;
}

UnitTestCommonBase::UnitTestSubscribeResponse& UnitTestCommonBase::UnitTestSubscribeResponse::operator=(const CC_Mqtt311SubscribeResponse& response)
{
    assignDataInternal(m_returnCodes, response.m_returnCodes, response.m_returnCodesCount);
    return *this;
}


UnitTestCommonBase::UnitTestMessageInfo& UnitTestCommonBase::UnitTestMessageInfo::operator=(const CC_Mqtt311MessageInfo& other)
{
    assignStringInternal(m_topic, other.m_topic);
    assignDataInternal(m_data, other.m_data, other.m_dataLen);
    m_qos = other.m_qos;
    m_retained = other.m_retained;
    return *this;
}

void UnitTestCommonBase::unitTestSetUp()
{
    unitTestClearState(false);
}

void UnitTestCommonBase::unitTestTearDown()
{
}

UnitTestCommonBase::UnitTestClientPtr UnitTestCommonBase::apiAllocClient(bool addLog)
{
    auto client = apiAlloc();
    if (addLog) {
        m_funcs.m_set_error_log_callback(client.get(), &UnitTestCommonBase::unitTestErrorLogCb, nullptr);
    }
    apiSetBrokerDisconnectReportCb(client.get(), &UnitTestCommonBase::unitTestBrokerDisconnectedCb, this);
    apiSetMessageReceivedReportCb(client.get(), &UnitTestCommonBase::unitTestMessageReceivedCb, this);
    apiSetSendOutputDataCb(client.get(), &UnitTestCommonBase::unitTestSendOutputDataCb, this);
    apiSetNextTickProgramCb(client.get(), &UnitTestCommonBase::unitTestProgramNextTickCb, this);
    apiSetCancelNextTickWaitCb(client.get(), &UnitTestCommonBase::unitTestCancelNextTickWaitCb, this);
    return client;
}

const UnitTestCommonBase::TickInfo* UnitTestCommonBase::unitTestTickReq()
{
    test_assert(!m_tickReq.empty());
    return &m_tickReq.front();
}

bool UnitTestCommonBase::unitTestCheckNoTicks()
{
    return m_tickReq.empty();
}

void UnitTestCommonBase::unitTestTick(CC_Mqtt311Client* client, unsigned ms, bool forceTick)
{
    test_assert(!m_tickReq.empty());
    auto& tick = m_tickReq.front();
    auto rem = tick.m_requested - tick.m_elapsed;
    test_assert(ms <= rem);
    if (ms == 0U) {
        ms = rem;
    }

    if (ms < rem) {
        tick.m_elapsed += ms;

        if (!forceTick) {
            return;
        }
    }

    m_tickReq.erase(m_tickReq.begin());
    m_funcs.m_tick(client, ms);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::unitTestSendConnect(CC_Mqtt311ConnectHandle& connect)
{
    auto result = m_funcs.m_connect_send(connect, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
    connect = nullptr;
    return result;
}

CC_Mqtt311ErrorCode UnitTestCommonBase::unitTestSendSubscribe(CC_Mqtt311SubscribeHandle& subscribe)
{
    auto result = m_funcs.m_subscribe_send(subscribe, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
    subscribe = nullptr;
    return result;
}

CC_Mqtt311ErrorCode UnitTestCommonBase::unitTestSendUnsubscribe(CC_Mqtt311UnsubscribeHandle& unsubscribe)
{
    auto result = m_funcs.m_unsubscribe_send(unsubscribe, &UnitTestCommonBase::unitTestUnsubscribeCompleteCb, this);
    unsubscribe = nullptr;
    return result;
}

CC_Mqtt311ErrorCode UnitTestCommonBase::unitTestSendPublish(CC_Mqtt311PublishHandle& publish, bool clearHandle)
{
    auto result = m_funcs.m_publish_send(publish, &UnitTestCommonBase::unitTestPublishCompleteCb, this);
    if (clearHandle) {
        publish = nullptr;
    }
    return result;
}

UniTestsMsgPtr UnitTestCommonBase::unitTestGetSentMessage()
{
    UniTestsMsgPtr msg;
    UnitTestsFrame frame;
    
    test_assert(!m_sentData.empty());
    UnitTestMessage::ReadIterator begIter = &m_sentData[0];
    auto readIter = begIter;
    [[maybe_unused]] auto readEs = frame.read(msg, readIter, m_sentData.size());
    test_assert(readEs == comms::ErrorStatus::Success);
    auto consumed = std::distance(begIter, readIter);
    m_sentData.erase(m_sentData.begin(), m_sentData.begin() + consumed);
    return msg;
}

bool UnitTestCommonBase::unitTestHasSentMessage() const
{
    return !m_sentData.empty();
}

bool UnitTestCommonBase::unitTestIsConnectComplete()
{
    return (!m_connectResp.empty());
}

const UnitTestCommonBase::UnitTestConnectResponseInfo& UnitTestCommonBase::unitTestConnectResponseInfo()
{
    test_assert(unitTestIsConnectComplete());
    return m_connectResp.front();
}

void UnitTestCommonBase::unitTestPopConnectResponseInfo()
{
    test_assert(!m_connectResp.empty());
    m_connectResp.erase(m_connectResp.begin());
}

bool UnitTestCommonBase::unitTestIsSubscribeComplete()
{
    return !m_subscribeResp.empty();
}
const UnitTestCommonBase::UnitTestSubscribeResponseInfo& UnitTestCommonBase::unitTestSubscribeResponseInfo()
{
    test_assert(!m_subscribeResp.empty());
    return m_subscribeResp.front();
}

void UnitTestCommonBase::unitTestPopSubscribeResponseInfo()
{
    test_assert(!m_subscribeResp.empty());
    m_subscribeResp.erase(m_subscribeResp.begin());
}

bool UnitTestCommonBase::unitTestIsUnsubscribeComplete()
{
    return !m_unsubscribeResp.empty();
}
const UnitTestCommonBase::UnitTestUnsubscribeResponseInfo& UnitTestCommonBase::unitTestUnsubscribeResponseInfo()
{
    test_assert(!m_unsubscribeResp.empty());
    return m_unsubscribeResp.front();
}

void UnitTestCommonBase::unitTestPopUnsubscribeResponseInfo()
{
    test_assert(!m_unsubscribeResp.empty());
    m_unsubscribeResp.erase(m_unsubscribeResp.begin());
}

bool UnitTestCommonBase::unitTestIsPublishComplete()
{
    return !m_publishResp.empty();
}

const UnitTestCommonBase::UnitTestPublishResponseInfo& UnitTestCommonBase::unitTestPublishResponseInfo()
{
    test_assert(!m_publishResp.empty());
    return m_publishResp.front();
}

void UnitTestCommonBase::unitTestPopPublishResponseInfo()
{
    test_assert(!m_publishResp.empty());
    m_publishResp.erase(m_publishResp.begin());
}

void UnitTestCommonBase::unitTestReceiveMessage(CC_Mqtt311Client* client, const UnitTestMessage& msg, bool reportReceivedData)
{
    UnitTestsFrame frame;
    auto prevSize = m_receivedData.size();
    m_receivedData.reserve(prevSize + frame.length(msg));
    auto writeIter = std::back_inserter(m_receivedData);
    auto es = frame.write(msg, writeIter, m_receivedData.max_size());
    if (es == comms::ErrorStatus::UpdateRequired) {
        auto* updateIter = &m_receivedData[prevSize];
        es = frame.update(msg, updateIter, m_receivedData.size() - prevSize);
    }
    test_assert(es == comms::ErrorStatus::Success);

    if (!reportReceivedData) {
        return;
    }

    auto consumed = m_funcs.m_process_data(client, &m_receivedData[0], static_cast<unsigned>(m_receivedData.size()));
    m_receivedData.erase(m_receivedData.begin(), m_receivedData.begin() + consumed);
}

bool UnitTestCommonBase::unitTestHasDisconnectInfo() const
{
    return (!m_disconnectInfo.empty());
}

const UnitTestCommonBase::UnitTestDisconnectInfo& UnitTestCommonBase::unitTestDisconnectInfo() const
{
    test_assert(!m_disconnectInfo.empty());
    return m_disconnectInfo.front();
}

void UnitTestCommonBase::unitTestPopDisconnectInfo()
{
    test_assert(!m_disconnectInfo.empty());
    m_disconnectInfo.erase(m_disconnectInfo.begin());
}

bool UnitTestCommonBase::unitTestHasMessageRecieved()
{
    return (!m_receivedMessages.empty());
}

const UnitTestCommonBase::UnitTestMessageInfo& UnitTestCommonBase::unitTestReceivedMessageInfo()
{
    test_assert(!m_receivedMessages.empty());
    return m_receivedMessages.front();
}

void UnitTestCommonBase::unitTestPopReceivedMessageInfo()
{
    test_assert(!m_receivedMessages.empty());
    m_receivedMessages.erase(m_receivedMessages.begin());
}

void UnitTestCommonBase::unitTestPerformConnect(
    CC_Mqtt311Client* client, 
    const CC_Mqtt311ConnectConfig* config,
    const CC_Mqtt311ConnectWillConfig* willConfig,
    const UnitTestConnectResponseConfig* responseConfig)
{
    [[maybe_unused]] auto ec = m_funcs.m_connect(client, config, willConfig, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
    test_assert(ec == CC_Mqtt311ErrorCode_Success);

    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt311::MsgId_Connect);
    test_assert(!unitTestIsConnectComplete());    

    auto* tickReq = unitTestTickReq();
    test_assert(tickReq->m_requested <= UnitTestDefaultOpTimeoutMs);    

    unitTestTick(client, 1000);
    UnitTestConnackMsg connackMsg;
    connackMsg.field_returnCode().value() = UnitTestConnackMsg::Field_returnCode::ValueType::Accepted;

    if (responseConfig != nullptr) {
        connackMsg.field_returnCode().setValue(responseConfig->m_returnCode);
        connackMsg.field_flags().setBitValue_sp(responseConfig->m_sessionPresent);
    } 

    unitTestReceiveMessage(client, connackMsg);
    test_assert(unitTestIsConnectComplete());   

    auto& connectInfo = unitTestConnectResponseInfo();
    test_assert(connectInfo.m_status == CC_Mqtt311AsyncOpStatus_Complete);
    test_assert((responseConfig != nullptr) || (connectInfo.m_response.m_returnCode == CC_Mqtt311ConnectReturnCode_Accepted));

    if (responseConfig != nullptr) {
        test_assert(connectInfo.m_response.m_returnCode == responseConfig->m_returnCode);
        test_assert(connectInfo.m_response.m_sessionPresent == responseConfig->m_sessionPresent);
    }
    unitTestPopConnectResponseInfo();    
}

void UnitTestCommonBase::unitTestPerformBasicConnect(
    CC_Mqtt311Client* client, 
    const char* clientId, 
    bool cleanSession)
{
    auto config = CC_Mqtt311ConnectConfig();
    apiConnectInitConfig(&config);
    config.m_clientId = clientId;
    config.m_cleanSession = cleanSession;

    unitTestPerformConnect(client, &config);
}

void UnitTestCommonBase::unitTestPerformDisconnect(CC_Mqtt311Client* client)
{
    [[maybe_unused]] auto ec = m_funcs.m_disconnect(client);
    test_assert(ec == CC_Mqtt311ErrorCode_Success);
    unitTestVerifyDisconnectSent();
}

void UnitTestCommonBase::unitTestPerformSubscribe(
    CC_Mqtt311Client* client, 
    CC_Mqtt311SubscribeTopicConfig* topicConfigs, 
    unsigned topicConfigsCount)
{
    auto ec = m_funcs.m_subscribe(client, topicConfigs, topicConfigsCount, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
    test_assert(ec == CC_Mqtt311ErrorCode_Success);
    test_assert(!unitTestIsSubscribeComplete());

    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt311::MsgId_Subscribe);    
    [[maybe_unused]] auto* subscribeMsg = dynamic_cast<UnitTestSubscribeMsg*>(sentMsg.get());
    test_assert(subscribeMsg != nullptr);

    unitTestTick(client, 1000);
    UnitTestSubackMsg subackMsg;
    subackMsg.field_packetId().value() = subscribeMsg->field_packetId().value();
    subackMsg.field_list().value().resize(topicConfigsCount);
    for (auto idx = 0U; idx < topicConfigsCount; ++idx) {
        subackMsg.field_list().value()[0].setValue(static_cast<unsigned>(CC_Mqtt311SubscribeReturnCode_SuccessQos0) + topicConfigs[idx].m_maxQos);
    }

    unitTestReceiveMessage(client, subackMsg);
    test_assert(unitTestIsSubscribeComplete());    

    [[maybe_unused]] auto& subackInfo = unitTestSubscribeResponseInfo();
    test_assert(subackInfo.m_status == CC_Mqtt311AsyncOpStatus_Complete);
    test_assert(subackInfo.m_response.m_returnCodes.size() == topicConfigsCount);
    unitTestPopSubscribeResponseInfo();    
}

void UnitTestCommonBase::unitTestPerformBasicSubscribe(CC_Mqtt311Client* client, const char* topic)
{
    auto config = CC_Mqtt311SubscribeTopicConfig();
    apiSubscribeInitConfigTopic(&config);
    config.m_topic = topic;

    unitTestPerformSubscribe(client, &config, 1U);
}

void UnitTestCommonBase::unitTestVerifyDisconnectSent()
{
    test_assert(unitTestHasSentMessage());
    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt311::MsgId_Disconnect);    
    auto* disconnectMsg = dynamic_cast<UnitTestDisconnectMsg*>(sentMsg.get());
    test_assert(disconnectMsg != nullptr);
}

void UnitTestCommonBase::unitTestClearState(bool preserveTicks)
{
    if (!preserveTicks) {
        m_tickReq.clear();
    }
    m_sentData.clear();
    m_receivedData.clear();
    m_connectResp.clear();
    m_subscribeResp.clear();
    m_publishResp.clear();
    m_disconnectInfo.clear();
}

UnitTestCommonBase::UnitTestClientPtr UnitTestCommonBase::apiAlloc()
{
    test_assert(m_funcs.m_alloc != nullptr);
    return UnitTestClientPtr(m_funcs.m_alloc(), UnitTestDeleter(m_funcs));
}

void UnitTestCommonBase::apiNotifyNetworkDisconnected(CC_Mqtt311Client* client)
{
    m_funcs.m_notify_network_disconnected(client);
}

bool UnitTestCommonBase::apiIsNetworkDisconnected(CC_Mqtt311Client* client)
{
    return m_funcs.m_is_network_disconnected(client);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiSetDefaultResponseTimeout(CC_Mqtt311Client* client, unsigned ms)
{
    return m_funcs.m_set_default_response_timeout(client, ms);
}

void UnitTestCommonBase::apiSetVerifyIncomingMsgSubscribed(CC_Mqtt311Client* client, bool enabled)
{
    m_funcs.m_set_verify_incoming_msg_subscribed(client, enabled);
}

CC_Mqtt311ConnectHandle UnitTestCommonBase::apiConnectPrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec)
{
    return m_funcs.m_connect_prepare(client, ec);
}

void UnitTestCommonBase::apiConnectInitConfig(CC_Mqtt311ConnectConfig* config)
{
    return m_funcs.m_connect_init_config(config);
}

void UnitTestCommonBase::apiConnectInitConfigWill(CC_Mqtt311ConnectWillConfig* config)
{
    return m_funcs.m_connect_init_config_will(config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiConnectConfig(CC_Mqtt311ConnectHandle handle, const CC_Mqtt311ConnectConfig* config)
{
    return m_funcs.m_connect_config(handle, config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiConnectConfigWill(CC_Mqtt311ConnectHandle handle, const CC_Mqtt311ConnectWillConfig* config)
{
    return m_funcs.m_connect_config_will(handle, config);
}

bool UnitTestCommonBase::apiIsConnected(CC_Mqtt311Client* client)
{
    return m_funcs.m_is_connected(client);
}

CC_Mqtt311DisconnectHandle UnitTestCommonBase::apiDisconnectPrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec)
{
    return m_funcs.m_disconnect_prepare(client, ec);
}

CC_Mqtt311SubscribeHandle UnitTestCommonBase::apiSubscribePrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec)
{
    return m_funcs.m_subscribe_prepare(client, ec);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiSubscribeSetResponseTimeout(CC_Mqtt311SubscribeHandle handle, unsigned ms)
{
    return m_funcs.m_subscribe_set_response_timeout(handle, ms);
}

void UnitTestCommonBase::apiSubscribeInitConfigTopic(CC_Mqtt311SubscribeTopicConfig* config)
{
    return m_funcs.m_subscribe_init_config_topic(config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiSubscribeConfigTopic(CC_Mqtt311SubscribeHandle handle, const CC_Mqtt311SubscribeTopicConfig* config)
{
    return m_funcs.m_subscribe_config_topic(handle, config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiSubscribe(CC_Mqtt311Client* client, CC_Mqtt311SubscribeTopicConfig* config, unsigned count)
{
    return m_funcs.m_subscribe(client, config, count, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
}

CC_Mqtt311UnsubscribeHandle UnitTestCommonBase::apiUnsubscribePrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec)
{
    return m_funcs.m_unsubscribe_prepare(client, ec);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiUnsubscribeSetResponseTimeout(CC_Mqtt311UnsubscribeHandle handle, unsigned ms)
{
    return m_funcs.m_unsubscribe_set_response_timeout(handle, ms);
}

void UnitTestCommonBase::apiUnsubscribeInitConfigTopic(CC_Mqtt311UnsubscribeTopicConfig* config)
{
    return m_funcs.m_unsubscribe_init_config_topic(config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiUnsubscribeConfigTopic(CC_Mqtt311UnsubscribeHandle handle, const CC_Mqtt311UnsubscribeTopicConfig* config)
{
    return m_funcs.m_unsubscribe_config_topic(handle, config);
}

CC_Mqtt311PublishHandle UnitTestCommonBase::apiPublishPrepare(CC_Mqtt311Client* client, CC_Mqtt311ErrorCode* ec)
{
    return m_funcs.m_publish_prepare(client, ec);
}

unsigned UnitTestCommonBase::apiPublishCount(CC_Mqtt311Client* client)
{
    return m_funcs.m_publish_count(client);
}

void UnitTestCommonBase::apiPublishInitConfig(CC_Mqtt311PublishConfig* config)
{
    return m_funcs.m_publish_init_config(config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiPublishSetResponseTimeout(CC_Mqtt311PublishHandle handle, unsigned ms)
{
    return m_funcs.m_publish_set_response_timeout(handle, ms);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiPublishConfig(CC_Mqtt311PublishHandle handle, const CC_Mqtt311PublishConfig* config)
{
    return m_funcs.m_publish_config(handle, config);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiPublishCancel(CC_Mqtt311PublishHandle handle)
{
    return m_funcs.m_publish_cancel(handle);
}

CC_Mqtt311ErrorCode UnitTestCommonBase::apiPublishSetOrdering(CC_Mqtt311ClientHandle handle, CC_Mqtt311PublishOrdering ordering)
{
    return m_funcs.m_publish_set_ordering(handle, ordering);
}

CC_Mqtt311PublishOrdering UnitTestCommonBase::apiPublishGetOrdering(CC_Mqtt311ClientHandle handle)
{
    return m_funcs.m_publish_get_ordering(handle);
}

void UnitTestCommonBase::apiSetNextTickProgramCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311NextTickProgramCb cb, void* data)
{
    return m_funcs.m_set_next_tick_program_callback(handle, cb, data);
}

void UnitTestCommonBase::apiSetCancelNextTickWaitCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311CancelNextTickWaitCb cb, void* data)
{
    return m_funcs.m_set_cancel_next_tick_wait_callback(handle, cb, data);
}

void UnitTestCommonBase::apiSetSendOutputDataCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311SendOutputDataCb cb, void* data)
{
    return m_funcs.m_set_send_output_data_callback(handle, cb, data);
}

void UnitTestCommonBase::apiSetBrokerDisconnectReportCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311BrokerDisconnectReportCb cb, void* data)
{
    return m_funcs.m_set_broker_disconnect_report_callback(handle, cb, data);
}

void UnitTestCommonBase::apiSetMessageReceivedReportCb(CC_Mqtt311ClientHandle handle, CC_Mqtt311MessageReceivedReportCb cb, void* data)
{
    return m_funcs.m_set_message_received_report_callback(handle, cb, data);
}

void UnitTestCommonBase::unitTestErrorLogCb([[maybe_unused]] void* obj, const char* msg)
{
    std::cout << "ERROR: " << msg << std::endl;
}

void UnitTestCommonBase::unitTestBrokerDisconnectedCb(void* obj, CC_Mqtt311BrokerDisconnectReason reason)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(!realObj->unitTestHasDisconnectInfo());
    realObj->m_disconnectInfo.resize(realObj->m_disconnectInfo.size() + 1U);
    auto& elem = realObj->m_disconnectInfo.back();
    elem.m_reason = reason;
}

void UnitTestCommonBase::unitTestMessageReceivedCb(void* obj, const CC_Mqtt311MessageInfo* info)
{
    test_assert(info != nullptr);
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    realObj->m_receivedMessages.resize(realObj->m_receivedMessages.size() + 1U);
    realObj->m_receivedMessages.back() = *info;
}

void UnitTestCommonBase::unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    std::copy_n(buf, bufLen, std::back_inserter(realObj->m_sentData));
}

void UnitTestCommonBase::unitTestProgramNextTickCb(void* obj, unsigned duration)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_tickReq.empty());
    realObj->m_tickReq.push_back({duration, 0U});
}

unsigned UnitTestCommonBase::unitTestCancelNextTickWaitCb(void* obj) {
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(!realObj->m_tickReq.empty());
    auto elapsed = realObj->m_tickReq.front().m_elapsed;
    realObj->m_tickReq.erase(realObj->m_tickReq.begin());
    return elapsed;
}

void UnitTestCommonBase::unitTestConnectCompleteCb(void* obj, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_connectResp.empty());
    realObj->m_connectResp.resize(realObj->m_connectResp.size() + 1U);
    auto& info = realObj->m_connectResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

void UnitTestCommonBase::unitTestSubscribeCompleteCb(void* obj, [[maybe_unused]] CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_subscribeResp.empty());
    realObj->m_subscribeResp.resize(realObj->m_subscribeResp.size() + 1U);
    auto& info = realObj->m_subscribeResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

void UnitTestCommonBase::unitTestUnsubscribeCompleteCb(void* obj, [[maybe_unused]] CC_Mqtt311UnsubscribeHandle handle, CC_Mqtt311AsyncOpStatus status)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_unsubscribeResp.empty());
    realObj->m_unsubscribeResp.resize(realObj->m_unsubscribeResp.size() + 1U);
    auto& info = realObj->m_unsubscribeResp.back();
    info.m_status = status;
}

void UnitTestCommonBase::unitTestPublishCompleteCb(void* obj, [[maybe_unused]] CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    // test_assert(realObj->m_publishResp.empty());
    realObj->m_publishResp.resize(realObj->m_publishResp.size() + 1U);
    auto& info = realObj->m_publishResp.back();
    info.m_status = status;
}

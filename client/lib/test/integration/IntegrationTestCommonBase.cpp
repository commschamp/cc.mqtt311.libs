#include "IntegrationTestCommonBase.h"

#include <chrono>
#include <cstring>
#include <iostream>

namespace 
{

const std::string DefaultHost("127.0.0.1");
const std::string DefaultPort("1883");    

IntegrationTestCommonBase* asObj(void* data)
{
    return reinterpret_cast<IntegrationTestCommonBase*>(data);
}

std::ostream& errorLog(const std::string& name)
{
    std::cerr << "ERROR: ";
    if (!name.empty()) {
        std::cerr << name << ": ";
    }

    return std::cerr;
}    

std::ostream& infoLog(const std::string& name)
{
    std::cout << "INFO: ";
    if (!name.empty()) {
        std::cout << name << ": ";
    }

    return std::cout;
}    

} // namespace 


IntegrationTestCommonBase::IntegrationTestCommonBase(boost::asio::io_context& io, const std::string& clientId) :
    m_io(io),
    m_socket(io),
    m_tickTimer(io),
    m_timeoutTimer(io),
    m_client(::cc_mqtt311_client_alloc()),
    m_host(DefaultHost),
    m_port(DefaultPort),
    m_clientId(clientId)
{
}

bool IntegrationTestCommonBase::integrationTestStart()
{
    if (!m_client) {
        errorLog(m_clientId) << "Client is not allocated" << std::endl;
        return false;
    }

    ::cc_mqtt311_client_set_next_tick_program_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestTickProgramCb, this);
    ::cc_mqtt311_client_set_cancel_next_tick_wait_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestCancelTickWaitCb, this);
    ::cc_mqtt311_client_set_send_output_data_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestSendDataCb, this);
    ::cc_mqtt311_client_set_broker_disconnect_report_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestBrokerDisconnectedCb, this);
    ::cc_mqtt311_client_set_message_received_report_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestMessageReceivedCb, this);

    boost::system::error_code ioEc;
    boost::asio::ip::tcp::resolver resolver(m_io);
    boost::asio::connect(m_socket, resolver.resolve(m_host, m_port), ioEc);    
    if (ioEc) {
        errorLog(m_clientId) << "Failed to connect to " << m_host << ":" << m_port << " with error: " << ioEc.message() << std::endl;
        return false;
    }

    assert(m_socket.is_open());
    infoLog(m_clientId) << "Connected to broker" << std::endl;
    integrationTestDoReadInternal();
    integrationTestDoTestTimeoutInternal();
    return true;
}

std::ostream& IntegrationTestCommonBase::integrationTestErrorLog()
{
    return errorLog(m_clientId);
}

std::ostream& IntegrationTestCommonBase::integrationTestInfoLog()
{
    return infoLog(m_clientId);
}

void IntegrationTestCommonBase::integrationTestPrintConnectResponse(const CC_Mqtt311ConnectResponse& response)
{
    integrationTestInfoLog() << "m_returnCode=" << response.m_returnCode << '\n';
    integrationTestInfoLog() << "sessionPresent=" << response.m_sessionPresent << '\n';
}

CC_Mqtt311ErrorCode IntegrationTestCommonBase::integrationTestSendConnect(CC_Mqtt311ConnectHandle handle)
{
    return ::cc_mqtt311_client_connect_send(handle, &IntegrationTestCommonBase::integrationTestConnectCompleteCb, this);
}

CC_Mqtt311ErrorCode IntegrationTestCommonBase::integrationTestSendSubscribe(CC_Mqtt311SubscribeHandle handle)
{
    return ::cc_mqtt311_client_subscribe_send(handle, &IntegrationTestCommonBase::integrationTestSubscribeCompleteCb, this);
}

CC_Mqtt311ErrorCode IntegrationTestCommonBase::integrationTestSendPublish(CC_Mqtt311PublishHandle handle)
{
    return ::cc_mqtt311_client_publish_send(handle, &IntegrationTestCommonBase::integrationTestPublishCompleteCb, this);
}

bool IntegrationTestCommonBase::integrationTestStartBasicConnect(bool cleanSession)
{
    auto connectConfig = CC_Mqtt311ConnectConfig();
    ::cc_mqtt311_client_connect_init_config(&connectConfig);
    connectConfig.m_clientId = m_clientId.c_str();
    connectConfig.m_cleanSession = cleanSession;

    auto* client = integrationTestClient();
    if (client == nullptr) {
        errorLog(m_clientId) << "Invalid client" << std::endl;
        return false;
    }

    auto* connect = ::cc_mqtt311_client_connect_prepare(client, nullptr);
    if (connect == nullptr) {
        errorLog(m_clientId) << "Failed to prepare connect" << std::endl;
        return false;
    }

    auto ec = ::cc_mqtt311_client_connect_config(connect, &connectConfig);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        errorLog(m_clientId) << "Failed to configure connect" << std::endl;
        return false;
    }
    
    ec = integrationTestSendConnect(connect);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        errorLog(m_clientId) << "Failed to send connect" << std::endl;
        return false;
    } 

    integrationTestInfoLog() << "Sent connect request" << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestStartBasicSubscribe(const char* topic)
{
    auto config = CC_Mqtt311SubscribeTopicConfig();
    ::cc_mqtt311_client_subscribe_init_config_topic(&config);
    config.m_topic = topic;
    auto* client = integrationTestClient();
    auto subscribe = ::cc_mqtt311_client_subscribe_prepare(client, nullptr);
    if (subscribe == nullptr) {
        integrationTestErrorLog() << "Failed to prepare subscribe" << std::endl;
        return false;
    }

    auto ec = ::cc_mqtt311_client_subscribe_config_topic(subscribe, &config);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to configure subscribe topic" << std::endl;
        return false;
    }

    ec = integrationTestSendSubscribe(subscribe);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to send subscribe" << std::endl;
        return false;
    }     

    integrationTestInfoLog() << "Sent subscribe to " << topic << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestStartBasicPublish(const char* topic, const char* data, CC_Mqtt311QoS qos)
{
    auto config = CC_Mqtt311PublishConfig();
    ::cc_mqtt311_client_publish_init_config(&config);
    config.m_topic = topic;
    config.m_data = reinterpret_cast<const std::uint8_t*>(data);
    config.m_dataLen = static_cast<unsigned>(std::strlen(data));
    config.m_qos = qos;

    auto* client = integrationTestClient();
    auto publish = ::cc_mqtt311_client_publish_prepare(client, nullptr);
    if (publish == nullptr) {
        integrationTestErrorLog() << "Failed to prepare publish" << std::endl;
        return false;
    }

    auto ec = ::cc_mqtt311_client_publish_config(publish, &config);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to configure publish" << std::endl;
        return false;
    }

    ec = integrationTestSendPublish(publish);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to send publish." << std::endl;
        return false;
    }     

    integrationTestInfoLog() << "Sent publish of " << topic << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestStartBasicDisconnect()
{
    auto* client = integrationTestClient();
    auto ec = ::cc_mqtt311_client_disconnect(client);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to send disconnect" << std::endl;
        return false;             
    }

    return true;
}

bool IntegrationTestCommonBase::integrationTestVerifyConnectSuccessful(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    if (status != CC_Mqtt311AsyncOpStatus_Complete) {
        integrationTestErrorLog() << "Unexpected connection status: " << status << std::endl;
        return false;
    }

    if (response == nullptr) {
        integrationTestErrorLog() << "Connection response is not provided" << std::endl;
        return false;            
    }

    if (response->m_returnCode != CC_Mqtt311ConnectReturnCode_Accepted) {
        integrationTestErrorLog() << "Unexpected connection return code: " << response->m_returnCode << std::endl;
        return false; 
    }

    integrationTestInfoLog() << "Connection successful" << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestVerifySubscribeSuccessful(
    [[maybe_unused]] CC_Mqtt311SubscribeHandle handle, 
    CC_Mqtt311AsyncOpStatus status, 
    const CC_Mqtt311SubscribeResponse* response, 
    unsigned returnCodesCount)
{
    if (status != CC_Mqtt311AsyncOpStatus_Complete) {
        integrationTestErrorLog() << "Unexpected subscribe status: " << status << std::endl;
        return false;
    }

    if (response == nullptr) {
        integrationTestErrorLog() << "Subscription response is not provided" << std::endl;
        return false;            
    }

    if (response->m_returnCodesCount != returnCodesCount) {
        integrationTestErrorLog() << "Unexpected amount of susbscription return codes: " << response->m_returnCodesCount << std::endl;
        return false; 
    }

    for (auto idx = 0U; idx < response->m_returnCodesCount; ++idx) {
        if (response->m_returnCodes[idx] > CC_Mqtt311SubscribeReturnCode_SuccessQos2) {
            integrationTestErrorLog() << "Unexpected subscription return code idx=" << idx << ": " << response->m_returnCodes[0] << std::endl;
            return false; 
        }        
    }

    integrationTestInfoLog() << "Subscription successful" << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestVerifyPublishSuccessful(
    [[maybe_unused]] CC_Mqtt311PublishHandle handle, 
    CC_Mqtt311AsyncOpStatus status)
{
    if (status != CC_Mqtt311AsyncOpStatus_Complete) {
        integrationTestErrorLog() << "Unexpected publish status: " << status << std::endl;
        return false;
    }

    return true;
}

void IntegrationTestCommonBase::stopIoPosted()
{
    boost::asio::post(
        m_io,
        [&io = m_io]()
        {
            io.stop();
        });
}

void IntegrationTestCommonBase::integrationTestDoReadInternal()
{
    assert(m_socket.is_open());
    m_socket.async_read_some(
        boost::asio::buffer(m_inBuf),
        [this](const boost::system::error_code& ec, std::size_t bytesCount)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            if (ec) {
                integrationTestErrorLog() << "Failed to read: " << ec.message() << std::endl;
                return;
            }

            // std::cout << "DEBUG: Received " << bytesCount << " bytes" << std::endl;
            auto* buf = &m_inBuf[0];
            auto bufLen = static_cast<unsigned>(bytesCount);
            bool useVector = !m_inData.empty();
            if (useVector) {
                m_inData.reserve(m_inData.size() + bufLen);
                m_inData.insert(m_inData.end(), buf, buf + bufLen);
                buf = &m_inData[0];
                bufLen = static_cast<decltype(bufLen)>(m_inData.size());
            }

            auto consumed = ::cc_mqtt311_client_process_data(m_client.get(), buf, bufLen);
            assert(consumed <= bufLen);
            if (useVector) {
                m_inData.erase(m_inData.begin(), m_inData.begin() + consumed);
            }
            else {
                auto begIter = buf + consumed;
                auto endIter = buf + bufLen;
                m_inData.assign(begIter, endIter);
            }

            integrationTestDoReadInternal();
        });
}

void IntegrationTestCommonBase::integrationTestDoTestTimeoutInternal()
{
    m_timeoutTimer.expires_after(std::chrono::seconds(integrationTestGetTimeoutSecImpl()));
    m_timeoutTimer.async_wait(
        [this](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            integrationTestTimeoutImpl();
        });
}

unsigned IntegrationTestCommonBase::integrationTestGetTimeoutSecImpl()
{
    return 5U;
}

void IntegrationTestCommonBase::integrationTestTimeoutImpl()
{
    m_io.stop();
    exit(-1);
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedImpl()
{
}

void IntegrationTestCommonBase::integrationTestMessageReceivedImpl([[maybe_unused]] const CC_Mqtt311MessageInfo* info)
{
}

void IntegrationTestCommonBase::integrationTestConnectCompleteImpl(
    [[maybe_unused]] CC_Mqtt311AsyncOpStatus status, 
    [[maybe_unused]] const CC_Mqtt311ConnectResponse* response)
{
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteImpl(
    [[maybe_unused]] CC_Mqtt311SubscribeHandle handle,
    [[maybe_unused]] CC_Mqtt311AsyncOpStatus status, 
    [[maybe_unused]] const CC_Mqtt311SubscribeResponse* response)
{
}

void IntegrationTestCommonBase::integrationTestPublishCompleteImpl(
    [[maybe_unused]] CC_Mqtt311PublishHandle handle, 
    [[maybe_unused]] CC_Mqtt311AsyncOpStatus status)
{
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedInternal(CC_Mqtt311BrokerDisconnectReason reason)
{
    integrationTestInfoLog() << "Disconnected with reason: " << reason << std::endl;
    integrationTestBrokerDisconnectedImpl();
}

void IntegrationTestCommonBase::integrationTestMessageReceivedInternal(const CC_Mqtt311MessageInfo* info)
{
    assert(info != nullptr);
    integrationTestInfoLog() << "Received message with topic: " << info->m_topic << std::endl;
    integrationTestMessageReceivedImpl(info);
}

void IntegrationTestCommonBase::integrationTestConnectCompleteInternal(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    auto& out = integrationTestInfoLog() << "Connect complete with status=" << status;
    if (response != nullptr) {
        out << " and returnCode=" << response->m_returnCode;
    }
    out << std::endl;
    integrationTestConnectCompleteImpl(status, response);
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteInternal(CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response)
{
    integrationTestInfoLog() << "Subscribe complete with status=" << status << std::endl;
    integrationTestSubscribeCompleteImpl(handle, status, response);
}

void IntegrationTestCommonBase::integrationTestPublishCompleteInternal(CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status)
{
    integrationTestInfoLog() << "Publish complete with status=" << status << std::endl;
    integrationTestPublishCompleteImpl(handle, status);
}

void IntegrationTestCommonBase::integrationTestTickProgramCb(void* data, unsigned ms)
{
    auto& timer = asObj(data)->m_tickTimer;
    asObj(data)->m_tickReqTs = TimestampClock::now();
    timer.expires_after(std::chrono::milliseconds(ms));
    timer.async_wait(
        [data, ms](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            auto& client = asObj(data)->m_client;
            ::cc_mqtt311_client_tick(client.get(), ms);
        });
}

unsigned IntegrationTestCommonBase::integrationTestCancelTickWaitCb(void* data)
{
    auto& ts = asObj(data)->m_tickReqTs;
    assert(ts != Timestamp());
    auto now = TimestampClock::now();
    auto elapsed = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(now - ts).count());
    ts = Timestamp();
    boost::system::error_code ec;
    auto& timer = asObj(data)->m_tickTimer;
    timer.cancel(ec);
    return elapsed;
}

void IntegrationTestCommonBase::integrationTestSendDataCb(void* data, const unsigned char* buf, unsigned bufLen)
{
    auto& socket = asObj(data)->m_socket;
    if (!socket.is_open()) {
        errorLog(asObj(data)->m_clientId) << "Socket is not open, cannot send" << std::endl;
        return;
    }

    //std::cout << "DEBUG: Sending " << bufLen << " bytes." << std::endl;

    std::size_t written = 0U;
    while (written < bufLen) {
        boost::system::error_code ec;
        written += boost::asio::write(socket, boost::asio::buffer(buf, bufLen), ec);
        if (ec) {
            errorLog(asObj(data)->m_clientId) << "Failed to write with error: " << ec.message() << std::endl;
            break;
        }
    }
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedCb(void* data, CC_Mqtt311BrokerDisconnectReason reason)
{
    asObj(data)->integrationTestBrokerDisconnectedInternal(reason);
}

void IntegrationTestCommonBase::integrationTestMessageReceivedCb(void* data, const CC_Mqtt311MessageInfo* info)
{
    asObj(data)->integrationTestMessageReceivedInternal(info);
}

void IntegrationTestCommonBase::integrationTestConnectCompleteCb(void* data, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    asObj(data)->integrationTestConnectCompleteInternal(status, response);
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteCb(void* data, CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response)
{
    asObj(data)->integrationTestSubscribeCompleteInternal(handle, status, response);
}

void IntegrationTestCommonBase::integrationTestPublishCompleteCb(void* data, CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status)
{
    asObj(data)->integrationTestPublishCompleteInternal(handle, status);
}


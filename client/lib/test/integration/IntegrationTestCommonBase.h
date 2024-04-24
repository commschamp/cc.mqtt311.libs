#pragma once

#include "client.h"

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <vector>

class IntegrationTestCommonBase
{
    struct ClientDeleter
    {
        void operator()(CC_Mqtt311Client* ptr)
        {
            ::cc_mqtt311_client_free(ptr);
        }
    }; 

public:
    using ClientPtr = std::unique_ptr<CC_Mqtt311Client, ClientDeleter>;
    using Socket = boost::asio::ip::tcp::socket;
    using Timer = boost::asio::steady_timer;
    using TimestampClock = std::chrono::steady_clock;
    using Timestamp = std::chrono::time_point<TimestampClock>;

    explicit IntegrationTestCommonBase(boost::asio::io_context& io, const std::string& m_clientId = std::string());

    bool integrationTestStart();

    auto integrationTestClient()
    {
        return m_client.get();
    }

    decltype(auto) io()
    {
        return m_io;
    }

    std::ostream& integrationTestErrorLog();
    std::ostream& integrationTestInfoLog();
    void integrationTestPrintConnectResponse(const CC_Mqtt311ConnectResponse& response);

    CC_Mqtt311ErrorCode integrationTestSendConnect(CC_Mqtt311ConnectHandle handle);
    CC_Mqtt311ErrorCode integrationTestSendSubscribe(CC_Mqtt311SubscribeHandle handle);
    CC_Mqtt311ErrorCode integrationTestSendPublish(CC_Mqtt311PublishHandle handle);

    bool integrationTestStartBasicConnect(bool cleanSession = true);
    bool integrationTestStartBasicSubscribe(const char* topic);
    bool integrationTestStartBasicPublish(const char* topic, const char* data, CC_Mqtt311QoS qos);
    bool integrationTestStartBasicDisconnect();

    bool integrationTestVerifyConnectSuccessful(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);
    bool integrationTestVerifySubscribeSuccessful(CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response, unsigned returnCodesCount = 1U);
    bool integrationTestVerifyPublishSuccessful(CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

    const std::string& integrationTestClientId()
    {
        return m_clientId;
    }


protected:
    virtual unsigned integrationTestGetTimeoutSecImpl();
    virtual void integrationTestTimeoutImpl();
    virtual void integrationTestBrokerDisconnectedImpl();    
    virtual void integrationTestMessageReceivedImpl(const CC_Mqtt311MessageInfo* info);    
    virtual void integrationTestConnectCompleteImpl(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);
    virtual void integrationTestSubscribeCompleteImpl(CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response);
    virtual void integrationTestPublishCompleteImpl(CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

    void stopIoPosted();

private:
    void integrationTestDoReadInternal();
    void integrationTestDoTestTimeoutInternal();
    void integrationTestBrokerDisconnectedInternal(CC_Mqtt311BrokerDisconnectReason reason);    
    void integrationTestMessageReceivedInternal(const CC_Mqtt311MessageInfo* info);
    void integrationTestConnectCompleteInternal(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);
    void integrationTestSubscribeCompleteInternal(CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response);
    void integrationTestPublishCompleteInternal(CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

    static void integrationTestTickProgramCb(void* data, unsigned ms);
    static unsigned integrationTestCancelTickWaitCb(void* data);
    static void integrationTestSendDataCb(void* data, const unsigned char* buf, unsigned bufLen);
    static void integrationTestBrokerDisconnectedCb(void* data, CC_Mqtt311BrokerDisconnectReason reason);
    static void integrationTestMessageReceivedCb(void* data, const CC_Mqtt311MessageInfo* info);
    static void integrationTestConnectCompleteCb(void* data, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);
    static void integrationTestSubscribeCompleteCb(void* data, CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response);
    static void integrationTestPublishCompleteCb(void* data, CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

    boost::asio::io_context& m_io;
    Socket m_socket;
    Timer m_tickTimer;
    Timer m_timeoutTimer;
    ClientPtr m_client;
    Timestamp m_tickReqTs;
    std::string m_host;
    std::string m_port;
    std::array<std::uint8_t, 1024> m_inBuf;
    std::vector<std::uint8_t> m_inData;
    std::string m_clientId;
};
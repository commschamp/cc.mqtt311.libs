#include "IntegrationTestCommonBase.h"

#include <boost/asio.hpp>

#include <iostream>
#include <stdexcept>

class IntegrationTestBasicConnect : public IntegrationTestCommonBase
{
    using Base = IntegrationTestCommonBase;
public:
    IntegrationTestBasicConnect(boost::asio::io_context& io, int& exitCode) :
        Base(io),
        m_exitCode(exitCode)
    {
    }

protected:
    virtual void integrationTestBrokerDisconnectedImpl() override
    {
        std::cerr << "ERROR: Unexpected disconnection from broker" << std::endl;
        failTestInternal();
    }  

    virtual void integrationTestConnectCompleteImpl(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response) override
    {
        if (status != CC_Mqtt311AsyncOpStatus_Complete) {
            std::cerr << "ERROR: Unexpected connection status: " << status << std::endl;
            failTestInternal();
            return;
        }

        if (response == nullptr) {
            std::cerr << "ERROR: connection response is not provided" << std::endl;
            failTestInternal();
            return;            
        }

        if (response->m_returnCode != CC_Mqtt311ConnectReturnCode_Accepted) {
            failTestInternal();
            return; 
        }

        integrationTestPrintConnectResponse(*response);

        auto ec = ::cc_mqtt311_client_disconnect(integrationTestClient());
        if (ec != CC_Mqtt311ErrorCode_Success) {
            std::cerr << "ERROR: Failed to send disconnect" << std::endl;
            failTestInternal();
            return;             
        }

        io().stop();
    }


private:
    void failTestInternal()
    {
        assert(0);
        m_exitCode = -1;
        io().stop();        
    }

    int& m_exitCode;    
};


int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    int exitCode = 0;
    try {
        boost::asio::io_context io;
        IntegrationTestBasicConnect test(io, exitCode);

        if (!test.integrationTestStart()) {
            std::cerr << "ERROR: Failed to start" << std::endl;
            return -1;
        }

        auto connectConfig = CC_Mqtt311ConnectConfig();
        ::cc_mqtt311_client_connect_init_config(&connectConfig);
        connectConfig.m_clientId = "IntegrationTestBasicConnect";
        connectConfig.m_cleanSession = true;

        auto* client = test.integrationTestClient();
        if (client == nullptr) {
            std::cerr << "ERROR: Invalid client" << std::endl;
            return -1;
        }

        auto* connect = ::cc_mqtt311_client_connect_prepare(client, nullptr);
        if (connect == nullptr) {
            std::cerr << "ERROR: Failed to prepare connect" << std::endl;
            return -1;
        }

        auto ec = ::cc_mqtt311_client_connect_config(connect, &connectConfig);
        if (ec != CC_Mqtt311ErrorCode_Success) {
            std::cerr << "ERROR: Failed to configure connect" << std::endl;
            return -1;
        }
        
        ec = test.integrationTestSendConnect(connect);
        if (ec != CC_Mqtt311ErrorCode_Success) {
            std::cerr << "ERROR: Failed to send connect" << std::endl;
            return -1;
        }

        io.run();

    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: Unexpected exception: " << e.what() << std::endl;
        return -1;
    }

    return exitCode;
}
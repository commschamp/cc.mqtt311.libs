//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ProgramOptions.h"
#include "Session.h"

#include "client.h"

#include <boost/asio.hpp>

#include <functional>
#include <iosfwd>

namespace cc_mqtt311_client_app
{

class AppClient
{
    struct ClientDeleter
    {
        void operator()(CC_Mqtt311Client* ptr)
        {
            ::cc_mqtt311_client_free(ptr);
        }
    };

public:
    bool start(int argc, const char* argv[]);    

    static std::string toString(CC_Mqtt311ErrorCode val);
    static std::string toString(CC_Mqtt311AsyncOpStatus val);
    static std::string toString(CC_Mqtt311ConnectReturnCode val);
    static std::string toString(CC_Mqtt311SubscribeReturnCode val);
    static std::string toString(const std::uint8_t* data, unsigned dataLen, bool forceBinary = false);
    void print(const CC_Mqtt311MessageInfo& info, bool printMessage = true);
    static void print(const CC_Mqtt311ConnectResponse& response);
    static void print(const CC_Mqtt311SubscribeResponse& response);

protected:

    explicit AppClient(boost::asio::io_context& io, int& result);
    ~AppClient() = default;

    CC_Mqtt311ClientHandle client()
    {
        return m_client.get();
    }

    boost::asio::io_context& io()
    {
        return m_io;
    }

    ProgramOptions& opts()
    {
        return m_opts;
    }

    bool sendConnect(CC_Mqtt311ConnectHandle connect);

    static std::ostream& logError();

    void doTerminate(int result = 1);
    void doComplete();

    virtual bool startImpl();
    virtual void brokerConnectedImpl();
    virtual void brokerDisconnectedImpl(CC_Mqtt311BrokerDisconnectReason reason);
    virtual void messageReceivedImpl(const CC_Mqtt311MessageInfo* info);
    virtual void connectCompleteImpl(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);

    static std::vector<std::uint8_t> parseBinaryData(const std::string& val);

private:
    using ClientPtr = std::unique_ptr<CC_Mqtt311Client, ClientDeleter>;
    using Timer = boost::asio::steady_timer;
    using Clock = Timer::clock_type;
    using Timestamp = Timer::time_point;

    void nextTickProgramInternal(unsigned duration);
    unsigned cancelNextTickWaitInternal();
    void sendDataInternal(const unsigned char* buf, unsigned bufLen);
    bool createSession();

    static void sendDataCb(void* data, const unsigned char* buf, unsigned bufLen);
    static void brokerDisconnectedCb(void* data, CC_Mqtt311BrokerDisconnectReason reason);
    static void messageReceivedCb(void* data, const CC_Mqtt311MessageInfo* info);
    static void logMessageCb(void* data, const char* msg);
    static void nextTickProgramCb(void* data, unsigned duration);
    static unsigned cancelNextTickWaitCb(void* data);
    static void connectCompleteCb(void* data, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response);

    boost::asio::io_context& m_io;
    int& m_result;
    Timer m_timer;
    Timestamp m_lastWaitProgram;
    ProgramOptions m_opts;
    ClientPtr m_client;
    SessionPtr m_session;
};

} // namespace cc_mqtt311_client_app

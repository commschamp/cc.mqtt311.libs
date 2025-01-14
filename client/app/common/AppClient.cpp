//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "AppClient.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <type_traits>

namespace cc_mqtt311_client_app
{

namespace 
{

AppClient* asThis(void* data)
{
    return reinterpret_cast<AppClient*>(data);
}

std::string toString(CC_Mqtt311QoS val)
{
    static const std::string Map[] = {
        /* CC_Mqtt311QoS_AtMostOnceDelivery */ "QoS0 - At Most Once Delivery",
        /* CC_Mqtt311QoS_AtLeastOnceDelivery */ "QoS1 - At Least Once Delivery",
        /* CC_Mqtt311QoS_ExactlyOnceDelivery */ "QoS2 - Exactly Once Delivery",
    };
    static constexpr std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == CC_Mqtt311QoS_ValuesLimit);

    auto idx = static_cast<unsigned>(val);
    if (MapSize <= idx) {
        assert(false); // Should not happen
        return std::to_string(val);
    }

    return Map[idx] + " (" + std::to_string(val) + ')';
}

void printQos(const char* prefix, CC_Mqtt311QoS val)
{
    std::cout << "\t" << prefix << ": " << toString(val) << '\n';
}

void printBool(const char* prefix, bool val)
{
    std::cout << '\t' << prefix << ": " << std::boolalpha << val << '\n';
}

void printConnectReturnCode(CC_Mqtt311ConnectReturnCode val)
{
    std::cout << "\tReturn Code: " << AppClient::toString(val) << '\n';
}

void printSubscribeReturnCode(CC_Mqtt311SubscribeReturnCode val)
{
    std::cout << "\tReturn Code: " << AppClient::toString(val) << '\n';
}

} // namespace 
    
bool AppClient::start(int argc, const char* argv[])
{
    if (!m_opts.parseArgs(argc, argv)) {
        logError() << "Failed to parse arguments." << std::endl;
        return false;
    }

    if (m_opts.helpRequested()) {
        std::cout << "Usage: " << argv[0] << " [options...]" << '\n';
        m_opts.printHelp();
        io().stop();
        return true;
    }

    if (!createSession()) {
        return false;
    }

    return startImpl();
}   


std::string AppClient::toString(CC_Mqtt311ErrorCode val)
{
    static const std::string Map[] = {
        "Success",
        "Internal Error",
        "Not Intitialized",
        "Busy",
        "Not Connected",
        "Already Connected",
        "Bad Param",
        "Insufficient Config",
        "Out Of Memory",
        "Buffer Overflow",
        "Not Supported",
        "Retry Later",
        "Disconnecting",
        "Network Disconnected",
        "Preparation Locked",
    };
    static constexpr std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == CC_Mqtt311ErrorCode_ValuesLimit);

    auto idx = static_cast<unsigned>(val);
    if (MapSize <= idx) {
        assert(false); // Should not happen
        return std::to_string(val);
    }

    return Map[idx] + " (" + std::to_string(val) + ')';
}

std::string AppClient::toString(CC_Mqtt311AsyncOpStatus val)
{
    static const std::string Map[] = {
        "Complete",
        "Internal Error",
        "Timeout",
        "Protocol Error",
        "Aborted",
        "Broker Disconnected",
        "Out Of Memory",
        "Bad Param",
    };
    static constexpr std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == CC_Mqtt311AsyncOpStatus_ValuesLimit);

    auto idx = static_cast<unsigned>(val);
    if (MapSize <= idx) {
        assert(false); // Should not happen
        return std::to_string(val);
    }

    return Map[idx] + " (" + std::to_string(val) + ')';
}

std::string AppClient::toString(CC_Mqtt311ConnectReturnCode val)
{
    static const std::string Map[] = {
        "Accepted",
        "Invalid ProtocolVersion",
        "Id Rejected",
        "Server Unavailable",
        "Bad Auth",
        "Not Authorized",
    };
    static constexpr std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == CC_Mqtt311ConnectReturnCode_ValuesLimit);

    auto idx = static_cast<unsigned>(val);
    if (MapSize <= idx) {
        assert(false); // Should not happen
        return std::to_string(val);
    }

    return Map[idx] + " (" + std::to_string(val) + ')';
}

std::string AppClient::toString(CC_Mqtt311SubscribeReturnCode val)
{
    static const std::map<CC_Mqtt311SubscribeReturnCode, std::string> Map = {
        {CC_Mqtt311SubscribeReturnCode_SuccessQos0, "Success - Qos0"},
        {CC_Mqtt311SubscribeReturnCode_SuccessQos1, "Success - Qos1"},
        {CC_Mqtt311SubscribeReturnCode_SuccessQos2, "Success - Qos2"},
        {CC_Mqtt311SubscribeReturnCode_Failure, "Failure"},
    };

    auto iter = Map.find(val);
    if (iter == Map.end()) {
        assert(false); // Should not happen
        return std::to_string(val);        
    }

    return iter->second + " (" + std::to_string(val) + ')';
}

std::string AppClient::toString(const std::uint8_t* data, unsigned dataLen, bool forceBinary)
{
    bool binary = forceBinary;
    if (!binary) {
        binary = 
            std::any_of(
                data, data + dataLen,
                [](auto byte)
                {
                    if (std::isprint(static_cast<int>(byte)) == 0) {
                        return true;
                    }

                    if (byte > 0x7e) {
                        return true;
                    }

                    return false;
                });
    } 

    if (!binary) {
        return std::string(reinterpret_cast<const char*>(data), dataLen);
    }

    std::stringstream stream;
    stream << std::hex;
    for (auto idx = 0U; idx < dataLen; ++idx) {
        stream << std::setw(2) << std::setfill('0') << static_cast<unsigned>(data[idx]) << ' ';
    }
    return stream.str();
}

void AppClient::print(const CC_Mqtt311MessageInfo& info, bool printMessage)
{
    std::cout << "[INFO]: Message Info:\n";
    if (printMessage) {
        std::cout << 
            "\tTopic: " << info.m_topic << '\n' <<
            "\tData: " << toString(info.m_data, info.m_dataLen, m_opts.subBinary()) << '\n';
    }

    printQos("QoS", info.m_qos);
    printBool("Retained", info.m_retained);
    std::cout << std::endl;
}

void AppClient::print(const CC_Mqtt311ConnectResponse& response)
{
    std::cout << "[INFO]: Connection Response:\n";
    printConnectReturnCode(response.m_returnCode);
    printBool("Session Present", response.m_sessionPresent);
    std::cout << std::endl;
}

void AppClient::print(const CC_Mqtt311SubscribeResponse& response)
{
    std::cout << "[INFO]: Subscribe Response:\n";
    for (auto idx = 0U; idx < response.m_returnCodesCount; ++idx) {
        printSubscribeReturnCode(response.m_returnCodes[idx]);
    }
    std::cout << std::endl;
}

AppClient::AppClient(boost::asio::io_context& io, int& result) : 
    m_io(io),
    m_result(result),
    m_timer(io),
    m_client(::cc_mqtt311_client_alloc())
{
    assert(m_client);
    ::cc_mqtt311_client_set_send_output_data_callback(m_client.get(), &AppClient::sendDataCb, this);
    ::cc_mqtt311_client_set_broker_disconnect_report_callback(m_client.get(), &AppClient::brokerDisconnectedCb, this);
    ::cc_mqtt311_client_set_message_received_report_callback(m_client.get(), &AppClient::messageReceivedCb, this);
    ::cc_mqtt311_client_set_error_log_callback(m_client.get(), &AppClient::logMessageCb, this);
    ::cc_mqtt311_client_set_next_tick_program_callback(m_client.get(), &AppClient::nextTickProgramCb, this);
    ::cc_mqtt311_client_set_cancel_next_tick_wait_callback(m_client.get(), &AppClient::cancelNextTickWaitCb, this);
}

bool AppClient::sendConnect(CC_Mqtt311ConnectHandle connect)
{
    auto ec = ::cc_mqtt311_client_connect_send(connect, &AppClient::connectCompleteCb, this);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        logError() << "Failed to send connect request with ec=" << toString(ec) << std::endl;
        return false;
    }    
    return true;
}

std::ostream& AppClient::logError()
{
    return std::cerr << "ERROR: ";
}

void AppClient::doTerminate(int result)
{
    m_result = result;
    m_io.stop();
}

void AppClient::doComplete()
{
    if (m_opts.willTopic().empty()) {
        auto ec = ::cc_mqtt311_client_disconnect(m_client.get());
        if (ec != CC_Mqtt311ErrorCode_Success) {
            logError() << "Failed to send disconnect with ec=" << toString(ec) << std::endl;
            doTerminate();
            return;
        }       
    }

    boost::asio::post(
        m_io,
        [this]()
        {
            doTerminate(0);
        });
}

bool AppClient::startImpl()
{
    auto ec = CC_Mqtt311ErrorCode_Success;
    auto connect = ::cc_mqtt311_client_connect_prepare(m_client.get(), &ec);
    if (connect == nullptr) {
        logError() << "Failed to prepare connect with ec=" << toString(ec) << std::endl;
        return false;
    }

    auto clientId = m_opts.clientId();
    auto username = m_opts.username();
    auto password = parseBinaryData(m_opts.password());

    auto config = CC_Mqtt311ConnectConfig();
    ::cc_mqtt311_client_connect_init_config(&config);
    config.m_keepAlive = m_opts.keepAlive();
    config.m_cleanSession = true;

    if (!clientId.empty()) {
        config.m_clientId = clientId.c_str();
    }

    if (!username.empty()) {
        config.m_username = username.c_str();
    }

    if (!password.empty()) {
        config.m_password = &password[0];
        config.m_passwordLen = static_cast<decltype(config.m_passwordLen)>(password.size());
    }

    ec = ::cc_mqtt311_client_connect_config(connect, &config);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        logError() << "Failed to apply basic connect configuration with ec=" << toString(ec) << std::endl;
        return false;
    }    

    auto willTopic = m_opts.willTopic();
    if (!willTopic.empty()) {
        auto willData = parseBinaryData(m_opts.willMessage());
        
        auto willConfig = CC_Mqtt311ConnectWillConfig();
        ::cc_mqtt311_client_connect_init_config_will(&willConfig);

        willConfig.m_topic = willTopic.c_str();
        if (!willData.empty()) {
            willConfig.m_data = &willData[0];
            willConfig.m_dataLen = static_cast<decltype(willConfig.m_dataLen)>(willData.size());
        }

        willConfig.m_qos = static_cast<decltype(willConfig.m_qos)>(m_opts.willQos());
        willConfig.m_retain = m_opts.willRetain();

        ec = ::cc_mqtt311_client_connect_config_will(connect, &willConfig);
        if (ec != CC_Mqtt311ErrorCode_Success) {
            logError() << "Failed to apply will configuration with ec=" << toString(ec) << std::endl;
            return false;
        }      
    }

    return sendConnect(connect);
}

void AppClient::brokerConnectedImpl()
{
    assert(false); // Expected to be overriden
}

void AppClient::brokerDisconnectedImpl([[maybe_unused]] CC_Mqtt311BrokerDisconnectReason reason)
{
    logError() << "Broker disconnected." << std::endl;
    doTerminate();
}

void AppClient::messageReceivedImpl([[maybe_unused]] const CC_Mqtt311MessageInfo* info)
{
}

void AppClient::connectCompleteImpl(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    do {
        if (status != CC_Mqtt311AsyncOpStatus_Complete) {
            logError() << "Connection operation was not properly completed: status=" << status << std::endl;
            break;
        }

        assert(response != nullptr);
        if (m_opts.verbose()) {
            print(*response);
        }
        
        if (CC_Mqtt311ConnectReturnCode_Accepted < response->m_returnCode) {
            logError() << "Connection attempt was rejected" << std::endl;
            break;
        }

        brokerConnectedImpl();
        return;
    } while (false);

    doTerminate();
}

std::vector<std::uint8_t> AppClient::parseBinaryData(const std::string& val)
{
    std::vector<std::uint8_t> result;
    result.reserve(val.size());
    auto pos = 0U;
    while (pos < val.size()) {
        auto ch = val[pos];
        auto addChar = 
            [&result, &pos, ch]()
            {
                result.push_back(static_cast<std::uint8_t>(ch));
                ++pos;
            };

        if (ch != '\\') {
            addChar();
            continue;
        }

        auto nextPos = pos + 1U;
        if ((val.size() <= nextPos)) {
            addChar();
            continue;
        }

        auto nextChar = val[nextPos];
        if (nextChar == '\\') {
            // Double backslash (\\) is treated as single one
            addChar();
            ++pos;
            continue;
        }

        if (nextChar != 'x') {
            // Not hex byte prefix, treat backslash as regular character
            addChar();
            continue;
        }

        auto bytePos = nextPos + 1U;
        auto byteLen = 2U;
        if (val.size() < bytePos + byteLen) {
            // Bad hex byte encoding, add characters as is
            addChar();
            continue;
        }

        try {
            auto byte = static_cast<std::uint8_t>(stoul(val.substr(bytePos, byteLen), nullptr, 16));
            result.push_back(byte);
            pos = bytePos + byteLen;
            continue;
        }
        catch (...) {
            addChar();
            continue;
        }
    }

    return result;
}

void AppClient::nextTickProgramInternal(unsigned duration)
{
    m_lastWaitProgram = Clock::now();
    m_timer.expires_after(std::chrono::milliseconds(duration));
    m_timer.async_wait(
        [this, duration](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            if (ec) {
                logError() << "Timer error: " << ec.message();
                doTerminate();
                return;
            }

            ::cc_mqtt311_client_tick(m_client.get(), duration);
        }
    );
}

unsigned AppClient::cancelNextTickWaitInternal()
{
    m_timer.cancel();
    auto now = Clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastWaitProgram).count();
    return static_cast<unsigned>(diff);
}

void AppClient::sendDataInternal(const unsigned char* buf, unsigned bufLen)
{
    assert(m_session);
    m_session->sendData(buf, bufLen);
}

bool AppClient::createSession()
{
    m_session = Session::create(m_io, m_opts);
    if (!m_session) {
        logError() << "Failed to create network connection session." << std::endl;
        return false;
    }

    m_session->setDataReportCb(
        [this](const std::uint8_t* buf, std::size_t bufLen) -> unsigned
        {
            assert(m_client);
            return ::cc_mqtt311_client_process_data(m_client.get(), buf, static_cast<unsigned>(bufLen));
        });

    m_session->setNetworkDisconnectedReportCb(
        [this]()
        {
            assert(m_client);
            ::cc_mqtt311_client_notify_network_disconnected(m_client.get());
            doTerminate();
        }
    );

    if (!m_session->start()) {
        logError() << "Failed to connect to the broker." << std::endl;
        return false;
    }

    return true;
}

void AppClient::sendDataCb(void* data, const unsigned char* buf, unsigned bufLen)
{
    asThis(data)->sendDataInternal(buf, bufLen);
}

void AppClient::brokerDisconnectedCb(void* data, CC_Mqtt311BrokerDisconnectReason reason)
{
    asThis(data)->brokerDisconnectedImpl(reason);
}

void AppClient::messageReceivedCb(void* data, const CC_Mqtt311MessageInfo* info)
{
    asThis(data)->messageReceivedImpl(info);
}

void AppClient::logMessageCb([[maybe_unused]] void* data, const char* msg)
{
    logError() << msg << std::endl;
}

void AppClient::nextTickProgramCb(void* data, unsigned duration)
{
    asThis(data)->nextTickProgramInternal(duration);
}

unsigned AppClient::cancelNextTickWaitCb(void* data)
{
    return asThis(data)->cancelNextTickWaitInternal();
}

void AppClient::connectCompleteCb(void* data, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    asThis(data)->connectCompleteImpl(status, response);
}

} // namespace cc_mqtt311_client_app

//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Logger.h"

#include "cc_mqtt311/Message.h"
#include "cc_mqtt311/frame/Frame.h"

#include "comms/options.h"

#include <cstdint>
#include <iterator>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cc_mqtt311_client_afl_fuzz
{

class Generator
{
public:
    using RawDataBuf = std::vector<std::uint8_t>;

    using Mqtt311Message = 
        cc_mqtt311::Message<
            comms::option::app::ReadIterator<const std::uint8_t*>,
            comms::option::app::WriteIterator<std::back_insert_iterator<RawDataBuf>>,
            comms::option::app::LengthInfoInterface,
            comms::option::app::IdInfoInterface,
            comms::option::app::NameInterface,
            comms::option::app::RefreshInterface,
            comms::option::app::Handler<Generator>
        >;

    CC_MQTT311_ALIASES_FOR_ALL_MESSAGES_DEFAULT_OPTIONS(Mqtt311, Msg, Mqtt311Message);

    using DataReportCb = std::function<void (const std::uint8_t* buf, std::size_t bufLen)>;

    Generator(Logger& logger, unsigned minPubCount) : m_logger(logger), m_minPubCount(minPubCount) {};

    bool prepare(const std::string& inputFile);
    void processData(const std::uint8_t* buf, unsigned bufLen);

    template <typename TFunc>
    void setDataReportCb(TFunc&& func)
    {
        m_dataReportCb = std::forward<TFunc>(func);
    }

    void handle(const Mqtt311ConnectMsg& msg);
    void handle(const Mqtt311PublishMsg& msg);
    void handle(const Mqtt311PubrecMsg& msg);
    void handle(const Mqtt311PubrelMsg& msg);
    void handle(const Mqtt311SubscribeMsg& msg);
    void handle(const Mqtt311UnsubscribeMsg& msg);
    void handle(const Mqtt311Message& msg);

private:
    using Mqtt311Frame = cc_mqtt311::frame::Frame<Mqtt311Message>;

    unsigned allocPacketId();
    void sendMessage(Mqtt311Message& msg);
    void sendPublish(const std::string& topic, unsigned qos);
    void doPublish();
    void doNextPublishIfNeeded();

    Logger& m_logger;
    unsigned m_minPubCount = 0U;
    std::ofstream m_stream;
    DataReportCb m_dataReportCb;
    Mqtt311Frame m_frame;
    std::unique_ptr<Mqtt311ConnectMsg> m_cachedConnect;
    unsigned m_lastPacketId = 0U;
    unsigned m_nextPubQos = 0U;
    std::string m_lastPubTopic;
    unsigned m_pubCount = 0U;
    bool m_connected = false;
};

using GeneratorPtr = std::unique_ptr<Generator>;

} // namespace cc_mqtt311_client_afl_fuzz

//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Generator.h"

#include "comms/process.h"

#include <cassert>
#include <type_traits>

namespace cc_mqtt311_client_afl_fuzz
{

namespace 
{

std::string pubTopicFromFilter(const std::string& filter)
{
    std::string result;
    std::size_t pos = 0U;
    while (pos < filter.size()) {
        auto wildcardPos = filter.find_first_of("#+", pos);
        if (wildcardPos == std::string::npos) {
            break;
        }

        result.append(filter.substr(pos, wildcardPos - pos));
        pos = wildcardPos + 1U;
        
        if (filter[wildcardPos] == '#') {
            result.append("hash");
            pos = filter.size();
            break;
        }

        assert(filter[wildcardPos] == '+');
        result.append("plus");
    }

    if (pos < filter.size()) {
        result.append(filter.substr(pos));
    }

    return result;
}

} // namespace 
    

bool Generator::prepare(const std::string& inputFile)
{
    m_stream.open(inputFile);
    if (!m_stream) {
        m_logger.errorLog() << "Failed to open " << inputFile << " for writing" << std::endl;
        return false;
    }

    return true;
}

void Generator::processData(const std::uint8_t* buf, unsigned bufLen)
{
    [[maybe_unused]] auto consumed = comms::processAllWithDispatch(buf, bufLen, m_frame, *this);
    assert(consumed == bufLen);
}

void Generator::handle(const Mqtt311ConnectMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    Mqtt311ConnackMsg outMsg;
    sendMessage(outMsg);
    m_connected = true;
}

void Generator::handle(const Mqtt311PublishMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";

    using QosValueType = Mqtt311PublishMsg::TransportField_flags::Field_qos::ValueType;
    auto qos = msg.transportField_flags().field_qos().value();
    if (qos == QosValueType::AtMostOnceDelivery) {
        m_logger.infoLog() << "at most once" << "\n";
        doNextPublishIfNeeded();
        return;
    }

    if (qos == QosValueType::AtLeastOnceDelivery) {
        Mqtt311PubackMsg outMsg;
        outMsg.field_packetId().setValue(msg.field_packetId().field().getValue());
        sendMessage(outMsg);
        doNextPublishIfNeeded();
        return;
    }

    assert(qos == QosValueType::ExactlyOnceDelivery);
    Mqtt311PubrecMsg outMsg;
    outMsg.field_packetId().setValue(msg.field_packetId().field().getValue());
    sendMessage(outMsg);
    return;
}

void Generator::handle(const Mqtt311PubrecMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    Mqtt311PubrelMsg outMsg;
    outMsg.field_packetId().setValue(msg.field_packetId().getValue());
    sendMessage(outMsg);
    return;    
}

void Generator::handle(const Mqtt311PubrelMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    Mqtt311PubcompMsg outMsg;
    outMsg.field_packetId().setValue(msg.field_packetId().getValue());
    sendMessage(outMsg);
    doNextPublishIfNeeded();
    return;    
}

void Generator::handle(const Mqtt311SubscribeMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    auto& subsVec = msg.field_list().value();

    Mqtt311SubackMsg outMsg;
    outMsg.field_packetId().value() = msg.field_packetId().value();
    auto& ackVec = outMsg.field_list().value();
    ackVec.resize(subsVec.size());

    for (auto& elem : ackVec) {
        using ElemType = std::decay_t<decltype(elem)>;
        elem.setValue(ElemType::ValueType::Qos2);
    }

    sendMessage(outMsg);

    for (auto& subElemField : subsVec) {
        m_lastPubTopic = pubTopicFromFilter(subElemField.field_topic().value());
        doPublish();
    }
}

void Generator::handle(const Mqtt311UnsubscribeMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    Mqtt311UnsubackMsg outMsg;
    outMsg.field_packetId().value() = msg.field_packetId().value();
    sendMessage(outMsg);
}

void Generator::handle([[maybe_unused]] const Mqtt311Message& msg)
{
    m_logger.infoLog() << "Ignoring " << msg.name() << "\n";
}

unsigned Generator::allocPacketId()
{
    ++m_lastPacketId;
    return m_lastPacketId;
}

void Generator::sendMessage(Mqtt311Message& msg)
{
    m_logger.infoLog() << "Generating " << msg.name() << "\n";
    msg.refresh();
    RawDataBuf outBuf;
    outBuf.reserve(m_frame.length(msg));
    auto iter = std::back_inserter(outBuf);
    [[maybe_unused]] auto es = m_frame.write(msg, iter, outBuf.max_size());
    assert(es == comms::ErrorStatus::Success);
    assert(m_dataReportCb);
    
    std::ostreambuf_iterator<char> outIter(m_stream);
    std::copy(outBuf.begin(), outBuf.end(), outIter);
    m_dataReportCb(outBuf.data(), outBuf.size());
}

void Generator::sendPublish(const std::string& topic, unsigned qos)
{
    Mqtt311PublishMsg outMsg;

    outMsg.transportField_flags().field_qos().setValue(qos);
    outMsg.field_topic().setValue(topic);
    if (qos > 0U) {
        outMsg.field_packetId().field().setValue(allocPacketId());
    }
    outMsg.field_payload().value().assign(topic.begin(), topic.end());

    sendMessage(outMsg);
    ++m_pubCount;
}

void Generator::doPublish()
{
    assert(!m_lastPubTopic.empty());
    sendPublish(m_lastPubTopic, m_nextPubQos);
    ++m_nextPubQos;
    if (m_nextPubQos > 2) {
        m_nextPubQos = 0;
    }
} 

void Generator::doNextPublishIfNeeded()
{
    if (m_pubCount < m_minPubCount) {
        doPublish();
    }
}

} // namespace cc_mqtt311_client_afl_fuzz

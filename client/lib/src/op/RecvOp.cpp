//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/RecvOp.h"
#include "ClientImpl.h"

#include <string_view>

namespace cc_mqtt311_client
{

namespace op
{

namespace 
{

using TopicStr = PublishMsg::Field_topic::ValueType;    

inline RecvOp* asRecvOp(void* data)
{
    return reinterpret_cast<RecvOp*>(data);
}

bool isTopicMatch(std::string_view filter, std::string_view topic)
{
    if ((filter.size() == 1U) && (filter[0] == '#')) {
        return true;
    }

    if (topic.empty()) {
        return filter.empty();
    }

    auto filterSepPos = filter.find_first_of("/");
    auto topicSepPos = topic.find_first_of("/");

    if ((filterSepPos == std::string_view::npos) && 
        (topicSepPos != std::string_view::npos)) {
        return false;
    }

    if (topicSepPos != std::string_view::npos) {
        COMMS_ASSERT(filterSepPos != std::string_view::npos);
        if (((filter[0] == '+') && (filterSepPos == 1U)) || 
            (filter.substr(0, filterSepPos) == topic.substr(0, topicSepPos))) {
            return isTopicMatch(filter.substr(filterSepPos + 1U), topic.substr(topicSepPos + 1U));
        }

        return false;
    }

    if (filterSepPos != std::string_view::npos) {
        COMMS_ASSERT(topicSepPos == std::string_view::npos);
        if (filter.size() <= (filterSepPos + 1U)) {
            // trailing '/' on the filter without any character after that
            return false;
        }

        if (((filter[0] == '+') && (filterSepPos == 1U)) || 
            (filter.substr(0, filterSepPos) == topic)) {
            return isTopicMatch(filter.substr(filterSepPos + 1U), std::string_view());
        }

        return false;
    }

    COMMS_ASSERT(filterSepPos == std::string_view::npos);
    COMMS_ASSERT(topicSepPos == std::string_view::npos);

    return 
        (((filter[0] == '+') && (filter.size() == 1U)) || 
         (filter == topic));
}

bool isTopicMatch(const TopicFilterStr& filter, const TopicStr& topic)
{
    COMMS_ASSERT(!filter.empty());
    COMMS_ASSERT(!topic.empty());
    return isTopicMatch(std::string_view(filter.c_str(), filter.size()), std::string_view(topic.c_str(), topic.size()));
}

} // namespace     

RecvOp::RecvOp(ClientImpl& client) : 
    Base(client),
    m_responseTimer(client.timerMgr().allocTimer())
{
    COMMS_ASSERT(m_responseTimer.isValid());
}    

void RecvOp::handle(PublishMsg& msg)
{
    auto qos = msg.transportField_flags().field_qos().value();

    if (qos > Qos::ExactlyOnceDelivery) {
        errorLog("Received PUBLISH with unknown Qos value.");
        client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
        return;         
    }


    if (!verifyQosValid(qos)) {
        errorLog("Invalid QoS in PUBLISH from broker");
        client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
        return;
    }    

    if constexpr (Config::MaxQos >= 2) {
        if ((qos == Qos::ExactlyOnceDelivery) && 
            (m_packetId != 0U) && 
            (msg.field_packetId().doesExist())) {
            
            if (msg.field_packetId().field().value() != m_packetId) {
                // Applicable to other RecvOp being handled in parallel
                return;
            }

            // If dispatched to this op, duplicate has been detected
            COMMS_ASSERT(msg.transportField_flags().field_dup().getBitValue_bit());
            PubrecMsg pubrecMsg;
            pubrecMsg.field_packetId().setValue(m_packetId);
            sendMessage(pubrecMsg);
            restartResponseTimer();
            return;
        }
    }

    auto& sessionState = client().sessionState();

    if (!sessionState.m_connected) {
        errorLog("Received PUBLISH when not CONNECTED");
        client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
        return;
    }

    auto& topic = msg.field_topic().value();
    if ((topic.empty()) || (!verifyPubTopic(topic.c_str(), false))) {
        errorLog("Received PUBLISH with invalid topic format.");
        client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
        return;
    }

    if constexpr (Config::HasSubTopicVerification) {
        if (client().configState().m_verifySubFilter) {
            auto& subFilters = client().reuseState().m_subFilters;
            auto iter = 
                std::find_if(
                    subFilters.begin(), subFilters.end(),
                    [&topic](auto& filter)
                    {
                        return isTopicMatch(filter, topic);
                    });

            if (iter == subFilters.end()) {
                errorLog("Received PUBLISH on non-subscribed topic");
                client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
                return;                
            }
        }
    }  

    auto info = CC_Mqtt311MessageInfo();
    info.m_topic = topic.c_str();
    auto& data = msg.field_payload().value();
    comms::cast_assign(info.m_dataLen) = data.size();
    if (!data.empty()) {
        info.m_data = &data[0];
    }

    comms::cast_assign(info.m_qos) = qos;
    info.m_retained = msg.transportField_flags().field_retain().getBitValue_bit();

    if (qos == Qos::AtMostOnceDelivery) {
        client().reportMsgInfo(info);
        opComplete();
        return;
    }

    if constexpr (Config::MaxQos >= 1) {
        if (!msg.field_packetId().doesExist()) {
            [[maybe_unused]] static constexpr bool ProtocolDecodingError = false;
            COMMS_ASSERT(ProtocolDecodingError);
            client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
            return;
        }    

        client().reportMsgInfo(info);

    
        if (qos == Qos::AtLeastOnceDelivery) {
            PubackMsg pubackMsg;
            pubackMsg.field_packetId().value() = msg.field_packetId().field().value();
            sendMessage(pubackMsg);
            opComplete();
            return;
        }    
    }

    if constexpr (Config::MaxQos >= 2) {
        m_packetId = msg.field_packetId().field().value();
        PubrecMsg pubrecMsg;
        pubrecMsg.field_packetId().setValue(m_packetId);
        sendMessage(pubrecMsg);
        restartResponseTimer();
    }
}

#if CC_MQTT311_CLIENT_MAX_QOS >= 2
void RecvOp::handle(PubrelMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    if (msg.field_packetId().value() != m_packetId) {
        return;
    }

    m_responseTimer.cancel();

    if (!msg.doValid()) {
        errorLog("Received invalid flags in PUBREL message");
        client().brokerDisconnected(CC_Mqtt311BrokerDisconnectReason_ProtocolError);
        return;
    }

    PubcompMsg pubcompMsg;
    pubcompMsg.field_packetId().setValue(m_packetId);
    sendMessage(pubcompMsg);
    opComplete();
}
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 2

void RecvOp::resetTimer()
{
    if constexpr (Config::MaxQos >= 2) {    
        m_responseTimer.cancel();
    }
}

void RecvOp::postReconnectionResume()
{
    if constexpr (Config::MaxQos >= 2) {
        connectivityChangedImpl();
        restartResponseTimer();
    }
}

Op::Type RecvOp::typeImpl() const
{
    return Type_Recv;
}

void RecvOp::connectivityChangedImpl()
{
    if constexpr (Config::MaxQos >= 2) {
        m_responseTimer.setSuspended(
            (!client().sessionState().m_connected) || client().clientState().m_networkDisconnected);
    }
}

void RecvOp::restartResponseTimer()
{
    if constexpr (Config::MaxQos >= 2) {    
        auto& state = client().configState();
        m_responseTimer.wait(state.m_responseTimeoutMs, &RecvOp::recvTimeoutCb, this);
    }        
}

void RecvOp::responseTimeoutInternal()
{
    if constexpr (Config::MaxQos >= 2) {
        // When there is no response from broker, just terminate the reception.
        // The retry will be initiated by the broker.
        COMMS_ASSERT(!m_responseTimer.isActive());
        errorLog("Timeout on PUBREL reception from broker.");
        opComplete();
    }
}

void RecvOp::recvTimeoutCb(void* data)
{
    if constexpr (Config::MaxQos >= 2) {
        asRecvOp(data)->responseTimeoutInternal();
    }
}

} // namespace op

} // namespace cc_mqtt311_client

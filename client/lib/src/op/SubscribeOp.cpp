//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/SubscribeOp.h"
#include "ClientImpl.h"

#include "comms/util/ScopeGuard.h"

namespace cc_mqtt311_client
{

namespace op
{

namespace 
{

template <typename TField, bool THasFixedLength>
struct ReturnCodesCountHeper
{
    static constexpr unsigned Value = 0U;
};  

template <typename TField>
struct ReturnCodesCountHeper<TField, true>
{
    static constexpr unsigned Value = TField::fixedSize();
};

inline SubscribeOp* asSubscribeOp(void* data)
{
    return reinterpret_cast<SubscribeOp*>(data);
}

template <typename TField>
constexpr unsigned returnCodesLength()
{
    return ReturnCodesCountHeper<TField, TField::hasFixedSize()>::Value;
}

} // namespace     

SubscribeOp::SubscribeOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}  

SubscribeOp::~SubscribeOp()
{
    releasePacketId(m_subMsg.field_packetId().value());
}

CC_Mqtt311ErrorCode SubscribeOp::configTopic(const CC_Mqtt311SubscribeTopicConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic is not provided in subscribe configuration.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (!verifySubFilter(config.m_topic)) {
        errorLog("Bad topic filter format in subscribe.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (static_cast<decltype(config.m_maxQos)>(Config::MaxQos) < config.m_maxQos) {
        errorLog("Bad subscription qos value.");
        return CC_Mqtt311ErrorCode_BadParam;        
    }

    auto& topicVec = m_subMsg.field_list().value();
    if (topicVec.max_size() <= topicVec.size()) {
        errorLog("Too many configured topics for subscribe operation.");
        return CC_Mqtt311ErrorCode_OutOfMemory;
    }

    topicVec.resize(topicVec.size() + 1U);
    auto& element = topicVec.back();
    element.field_topic().value() = config.m_topic;
    element.field_qos().setValue(config.m_maxQos);

    if (maxStringLen() < element.field_topic().value().size()) {
        errorLog("Subscription topic value is too long");
        topicVec.pop_back();
        return CC_Mqtt311ErrorCode_BadParam;
    }   

    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode SubscribeOp::send(CC_Mqtt311SubscribeCompleteCb cb, void* cbData) 
{
    client().allowNextPrepare();
    
    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        errorLog("Subscribe completion callback is not provided.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (m_subMsg.field_list().value().empty()) {
        errorLog("No subscribe topic has been configured.");
        return CC_Mqtt311ErrorCode_InsufficientConfig;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt311ErrorCode_InternalError;
    }    

    m_cb = cb;
    m_cbData = cbData;

    m_subMsg.field_packetId().setValue(allocPacketId());
    auto result = client().sendMessage(m_subMsg); 
    if (result != CC_Mqtt311ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode SubscribeOp::cancel()
{
    if (m_cb == nullptr) {
        // hasn't been sent yet
        client().allowNextPrepare();
    }

    opComplete();
    return CC_Mqtt311ErrorCode_Success;
}

void SubscribeOp::handle(SubackMsg& msg)
{
    auto packetId = msg.field_packetId().value();
    if (packetId != m_subMsg.field_packetId().value()) {
        return;
    }

    using ReturnCodesListField = SubackMsg::Field_list;
    using ReturnCodesList = ObjListType<CC_Mqtt311SubscribeReturnCode, returnCodesLength<ReturnCodesListField>()>;
    m_timer.cancel();
    auto status = CC_Mqtt311AsyncOpStatus_ProtocolError;
    ReturnCodesList returnCodes; // Will be referenced in response
    auto response = CC_Mqtt311SubscribeResponse();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client(), &status]()
            {
                auto reason = CC_Mqtt311BrokerDisconnectReason_ProtocolError;
                if (status == CC_Mqtt311AsyncOpStatus_InternalError) {
                    reason = CC_Mqtt311BrokerDisconnectReason_InternalError;
                }
                cl.brokerDisconnected(reason);
            }
        );    

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                auto* responsePtr = &response;
                if (status != CC_Mqtt311AsyncOpStatus_Complete) {
                    responsePtr = nullptr;
                }
                completeOpInternal(status, responsePtr);
            });     

    auto& topicsVec = m_subMsg.field_list().value();
    auto& returnCodesVec = msg.field_list().value();

    if (topicsVec.size() != returnCodesVec.size()) {
        errorLog("Amount of reason codes in SUBACK doesn't match amount of subcribe topics");
        return;
    }

    auto maxReturnCodesCount = std::min(returnCodesVec.size(), returnCodes.max_size());
    returnCodes.reserve(maxReturnCodesCount);
    for (auto idx = 0U; idx < returnCodesVec.size(); ++idx) {
        auto rc = msg.field_list().value()[idx];
        if (returnCodes.max_size() <= idx) {
            errorLog("Cannot accumulate all the subscribe reported reason codes, insufficient memory");
            status = CC_Mqtt311AsyncOpStatus_InternalError;
            return;
        }

        // Without this condition it can be unexpected behaviour to 
        // cast return code value to CC_Mqtt311SubscribeReturnCode.
        using RetCodeType = SubackMsg::Field_list::ValueType::value_type::ValueType;
        if ((RetCodeType::Qos2 < rc.value()) && 
            (RetCodeType::Failure != rc.value())) {
            errorLog("Invalid return code in SUBACK");
            return;
        }

        auto rcCasted = static_cast<CC_Mqtt311SubscribeReturnCode>(rc.value());

        if ((CC_Mqtt311SubscribeReturnCode_SuccessQos0 <= rcCasted) && 
            (rcCasted <= CC_Mqtt311SubscribeReturnCode_SuccessQos2)) {
            auto ackQos = static_cast<unsigned>(rcCasted) - static_cast<unsigned>(CC_Mqtt311SubscribeReturnCode_SuccessQos0);
            auto reqQos = static_cast<unsigned>(topicsVec[idx].field_qos().value());
            if (reqQos < ackQos) {
                errorLog("Granted QoS in SUBACK is greater than requested");
                return; // protocol error will be reported
            }   
        } 
        else if (rcCasted != CC_Mqtt311SubscribeReturnCode_Failure) {
            errorLog("Return code in SUBACK has invalid value");
            return; // protocol error will be reported
        }

        returnCodes.push_back(rcCasted);

        if constexpr (Config::HasSubTopicVerification) {
            if (!client().configState().m_verifySubFilter) {
                continue;
            }

            if (returnCodes.back() >  CC_Mqtt311SubscribeReturnCode_SuccessQos2) {
                // Subscribe is not confirmed
                continue;
            }            
            
            auto& topicStr = m_subMsg.field_list().value()[idx].field_topic().value();
            auto& filtersMap = client().reuseState().m_subFilters;
            auto iter = 
                std::lower_bound(
                    filtersMap.begin(), filtersMap.end(), topicStr,
                    [](auto& storedTopic, auto& topicParam)
                    {
                        return storedTopic < topicParam;
                    });

            if ((iter != filtersMap.end()) && (*iter == topicStr)) {
                continue;
            }

            if (filtersMap.max_size() <= filtersMap.size()) {
                errorLog("Subscibe filters storage reached its maximum, can't store any more topics");
                status = CC_Mqtt311AsyncOpStatus_InternalError;
                return;
            }

            filtersMap.insert(iter, topicStr);
        }
    }

    comms::cast_assign(response.m_returnCodesCount) = returnCodes.size();
    if (response.m_returnCodesCount > 0U) {
        response.m_returnCodes = &returnCodes[0];
    }

    terminateOnExit.release();
    status = CC_Mqtt311AsyncOpStatus_Complete;
}

Op::Type SubscribeOp::typeImpl() const
{
    return Type_Subscribe;
}

void SubscribeOp::terminateOpImpl(CC_Mqtt311AsyncOpStatus status)
{
    completeOpInternal(status);
}

void SubscribeOp::completeOpInternal(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response)
{
    auto cb = m_cb;
    auto* cbData = m_cbData;
    auto handle = toHandle();
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, handle, status, response);    
    }
}

void SubscribeOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt311AsyncOpStatus_Timeout);
}

void SubscribeOp::restartTimer()
{
    m_timer.wait(getResponseTimeout(), &SubscribeOp::opTimeoutCb, this);
}

void SubscribeOp::opTimeoutCb(void* data)
{
    asSubscribeOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt311_client

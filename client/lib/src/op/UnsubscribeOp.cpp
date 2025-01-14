//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/UnsubscribeOp.h"
#include "ClientImpl.h"

#include "comms/util/ScopeGuard.h"

namespace cc_mqtt311_client
{

namespace op
{

namespace 
{

inline UnsubscribeOp* asUnsubscribeOp(void* data)
{
    return reinterpret_cast<UnsubscribeOp*>(data);
}

} // namespace 
    

UnsubscribeOp::UnsubscribeOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}    

UnsubscribeOp::~UnsubscribeOp()
{
    releasePacketId(m_unsubMsg.field_packetId().value());
}

CC_Mqtt311ErrorCode UnsubscribeOp::configTopic(const CC_Mqtt311UnsubscribeTopicConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic is not provided in unsubscribe configuration.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (!verifySubFilter(config.m_topic)) {
        errorLog("Bad topic filter format in unsubscribe.");
        return CC_Mqtt311ErrorCode_BadParam;
    }    

    if constexpr (Config::HasSubTopicVerification) {
        if (client().configState().m_verifySubFilter) {
            auto& filtersMap = client().reuseState().m_subFilters;
            auto iter = 
                std::lower_bound(
                    filtersMap.begin(), filtersMap.end(), config.m_topic,
                    [](auto& storedTopic, const char* topicParam)
                    {
                        return storedTopic < topicParam;
                    });

            if ((iter == filtersMap.end()) || (*iter != config.m_topic)) {
                errorLog("Requested unsubscribe hasn't been used for subscription before");
                return CC_Mqtt311ErrorCode_BadParam;
            }
        }
    }    

    auto& topicVec = m_unsubMsg.field_list().value();
    if (topicVec.max_size() <= topicVec.size()) {
        errorLog("Too many configured topics for unsubscribe operation.");
        return CC_Mqtt311ErrorCode_OutOfMemory;
    }

    topicVec.resize(topicVec.size() + 1U);
    auto& element = topicVec.back();
    element.value() = config.m_topic;

    if (maxStringLen() < element.value().size()) {
        errorLog("Unsubscription topic value is too long");
        topicVec.pop_back();
        return CC_Mqtt311ErrorCode_BadParam;
    }  

    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode UnsubscribeOp::send(CC_Mqtt311UnsubscribeCompleteCb cb, void* cbData) 
{
    client().allowNextPrepare();

    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        errorLog("Unsubscribe completion callback is not provided.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (m_unsubMsg.field_list().value().empty()) {
        errorLog("No unsubscribe topic has been configured.");
        return CC_Mqtt311ErrorCode_InsufficientConfig;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt311ErrorCode_InternalError;
    }    

    m_cb = cb;
    m_cbData = cbData;

    m_unsubMsg.field_packetId().setValue(allocPacketId());
    auto result = client().sendMessage(m_unsubMsg); 
    if (result != CC_Mqtt311ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode UnsubscribeOp::cancel()
{
    if (m_cb == nullptr) {
        // hasn't been sent yet
        client().allowNextPrepare();
    }            

    opComplete();
    return CC_Mqtt311ErrorCode_Success;
}

void UnsubscribeOp::handle(UnsubackMsg& msg)
{
    if (msg.field_packetId().value() != m_unsubMsg.field_packetId().value()) {
        return;
    }

    m_timer.cancel();

    if constexpr (Config::HasSubTopicVerification) {
        auto& unsubFiltersVec = m_unsubMsg.field_list().value();
        for (auto idx = 0U; idx < unsubFiltersVec.size(); ++idx) {
            // Remove from the subscribed topics record regardless of the client().configState().m_verifySubFilter
            auto& topicStr = m_unsubMsg.field_list().value()[idx].value();
            auto& filtersMap = client().reuseState().m_subFilters;
            auto iter = 
                std::lower_bound(
                    filtersMap.begin(), filtersMap.end(), topicStr,
                    [](auto& storedTopic, auto& topicParam)
                    {
                        return storedTopic < topicParam;
                    });

            if ((iter == filtersMap.end()) || (*iter != topicStr)) {
                continue;
            }

            filtersMap.erase(iter);
        }  
    }

    completeOpInternal(CC_Mqtt311AsyncOpStatus_Complete);
}

Op::Type UnsubscribeOp::typeImpl() const
{
    return Type_Unsubscribe;
}

void UnsubscribeOp::terminateOpImpl(CC_Mqtt311AsyncOpStatus status)
{
    completeOpInternal(status);
}

void UnsubscribeOp::completeOpInternal(CC_Mqtt311AsyncOpStatus status)
{
    auto cb = m_cb;
    auto* cbData = m_cbData;
    auto handle = toHandle();
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, handle, status);    
    }
}

void UnsubscribeOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt311AsyncOpStatus_Timeout);
}

void UnsubscribeOp::restartTimer()
{
    m_timer.wait(getResponseTimeout(), &UnsubscribeOp::opTimeoutCb, this);
}

void UnsubscribeOp::opTimeoutCb(void* data)
{
    asUnsubscribeOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt311_client

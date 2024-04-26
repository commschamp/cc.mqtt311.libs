//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/ConnectOp.h"
#include "ClientImpl.h"

#include "comms/util/assign.h"
#include "comms/util/ScopeGuard.h"
#include "comms/units.h"

#include <algorithm>
#include <limits>

namespace cc_mqtt311_client
{

namespace op
{

namespace 
{

inline ConnectOp* asConnectOp(void* data)
{
    return reinterpret_cast<ConnectOp*>(data);
}

} // namespace 
    

ConnectOp::ConnectOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}   

CC_Mqtt311ErrorCode ConnectOp::config(const CC_Mqtt311ConnectConfig& config)
{
    if ((config.m_passwordLen > 0U) && (config.m_password == nullptr)) {
        errorLog("Bad password parameter in connect configuration.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if ((config.m_passwordLen > 0U) && (config.m_username == nullptr)) {
        errorLog("Password cannot be configured without the username.");
        return CC_Mqtt311ErrorCode_BadParam;        
    }

    if ((!config.m_cleanSession) && client().clientState().m_firstConnect && client().configState().m_verifySubFilter) {
        errorLog("Clean session flag needs to be set on the first connection attempt.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (config.m_keepAlive == 0) {
        errorLog("Keep alive must be set to a non-zero value.");
        return CC_Mqtt311ErrorCode_BadParam;        
    }

    auto& clientIdStr = m_connectMsg.field_clientId().value();
    if (config.m_clientId != nullptr) {
        clientIdStr = config.m_clientId;    
    }
    else {
        clientIdStr.clear();
    }

    if (maxStringLen() < clientIdStr.size()) {
        errorLog("Client ID is too long");
        clientIdStr.clear();
        return CC_Mqtt311ErrorCode_BadParam;        
    }

    if ((m_connectMsg.field_clientId().value().empty()) && (!config.m_cleanSession)) {
        errorLog("Clean start flag needs to be set for empty client id");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    bool hasUsername = (config.m_username != nullptr);
    m_connectMsg.field_flags().field_high().setBitValue_userNameFlag(hasUsername);
    if (hasUsername) {
        m_connectMsg.field_userName().field().value() = config.m_username;
    }

    auto& usernameStr = m_connectMsg.field_userName().field().value();
    if (maxStringLen() < usernameStr.size()) {
        errorLog("Username is too long");
        usernameStr.clear();
        return CC_Mqtt311ErrorCode_BadParam;        
    }

    bool hasPassword = (config.m_passwordLen > 0U);
    m_connectMsg.field_flags().field_high().setBitValue_passwordFlag(hasPassword);

    auto& passwordStr = m_connectMsg.field_password().field().value();
    if (hasPassword) {
        comms::util::assign(passwordStr, config.m_password, config.m_password + config.m_passwordLen);
    }

    if (maxStringLen() < passwordStr.size()) {
        errorLog("Password is too long");
        passwordStr.clear();
        return CC_Mqtt311ErrorCode_BadParam;        
    }    

    m_connectMsg.field_flags().field_low().setBitValue_cleanSession(config.m_cleanSession);    

    static constexpr auto MaxKeepAlive = 
        std::numeric_limits<ConnectMsg::Field_keepAlive::ValueType>::max();

    if (MaxKeepAlive < config.m_keepAlive) {
        errorLog("Keep alive value is too high in connect configuration.");
        return CC_Mqtt311ErrorCode_BadParam;
    }
    
    comms::units::setSeconds(m_connectMsg.field_keepAlive(), config.m_keepAlive);
    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode ConnectOp::configWill(const CC_Mqtt311ConnectWillConfig& config)
{
    if ((config.m_dataLen > 0U) && (config.m_data == nullptr)) {
        errorLog("Bad data parameter in will configuration.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Will topic is not provided.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if ((config.m_qos < CC_Mqtt311QoS_AtMostOnceDelivery) || 
        (static_cast<decltype(config.m_qos)>(Config::MaxQos) < config.m_qos)) {
        errorLog("Invalid will QoS value in configuration.");
        return CC_Mqtt311ErrorCode_BadParam;
    }    

    auto& willTopicStr = m_connectMsg.field_willTopic().field().value();
    willTopicStr = config.m_topic;
    if (maxStringLen() < willTopicStr.size()) {
        errorLog("Will topic is too long.");
        willTopicStr.clear();
        return CC_Mqtt311ErrorCode_BadParam;        
    }

    auto& willData = m_connectMsg.field_willMessage().field().value();
    if (config.m_dataLen > 0U) {
        comms::util::assign(willData, config.m_data, config.m_data + config.m_dataLen);
    }

    if (maxStringLen() < willData.size()) {
        errorLog("Will data is too long.");
        willData.clear();
        return CC_Mqtt311ErrorCode_BadParam;                
    }

    m_connectMsg.field_flags().field_willQos().setValue(config.m_qos);
    m_connectMsg.field_flags().field_high().setBitValue_willRetain(config.m_retain);
    m_connectMsg.field_flags().field_low().setBitValue_willFlag(true);

    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode ConnectOp::send(CC_Mqtt311ConnectCompleteCb cb, void* cbData) 
{
    client().allowNextPrepare();
    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        errorLog("Connect completion callback is not provided.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt311ErrorCode_InternalError;
    }    

    if ((!m_connectMsg.field_flags().field_low().getBitValue_cleanSession()) && 
        (client().clientState().m_firstConnect) && 
        (client().configState().m_verifySubFilter)) {
        errorLog("Clean start flag needs to be set on the first connection attempt, perform configuration first.");
        return CC_Mqtt311ErrorCode_InsufficientConfig;
    }    

    m_cb = cb;
    m_cbData = cbData;
    
    m_connectMsg.doRefresh();
    auto result = client().sendMessage(m_connectMsg); 
    if (result != CC_Mqtt311ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode ConnectOp::cancel()
{
    if (m_cb == nullptr) {
        // hasn't been sent yet
        client().allowNextPrepare();
    }

    opComplete();
    return CC_Mqtt311ErrorCode_Success;
}

void ConnectOp::handle(ConnackMsg& msg)
{
    m_timer.cancel();

    auto status = CC_Mqtt311AsyncOpStatus_ProtocolError;
    auto response = CC_Mqtt311ConnectResponse();

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

    // Without this condition it can be unexpected behaviour to 
    // cast return code value to CC_Mqtt311ConnectReturnCode.
    if (ConnackMsg::Field_returnCode::ValueType::ValuesLimit <= msg.field_returnCode().value()) {
        errorLog("Unknown connection return code");
        return;
    }

    comms::cast_assign(response.m_returnCode) = msg.field_returnCode().value();
    response.m_sessionPresent = msg.field_flags().getBitValue_sp();

    if (response.m_sessionPresent && m_connectMsg.field_flags().field_low().getBitValue_cleanSession()) {
        errorLog("Session present when clean session is requested");
        return;
    }

    status = CC_Mqtt311AsyncOpStatus_Complete; // Reported in op completion callback
    bool connected = (response.m_returnCode == CC_Mqtt311ConnectReturnCode_Accepted);

    auto& state = client().sessionState();
    if (!connected) {
        state = SessionState();
        return;
    }

    auto& reuseState = client().reuseState();
    if (!response.m_sessionPresent) {
        reuseState = ReuseState();
    }    

    state.m_keepAliveMs = comms::units::getMilliseconds<decltype(state.m_keepAliveMs)>(m_connectMsg.field_keepAlive());
    client().brokerConnected(response.m_sessionPresent);
}

Op::Type ConnectOp::typeImpl() const
{
    return Type_Connect;
}

void ConnectOp::terminateOpImpl(CC_Mqtt311AsyncOpStatus status)
{
    completeOpInternal(status);
}

void ConnectOp::connectivityChangedImpl()
{
    if (client().clientState().m_networkDisconnected) {
        completeOpInternal(CC_Mqtt311AsyncOpStatus_BrokerDisconnected);
        return;
    }
}

void ConnectOp::completeOpInternal(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response)
{
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, status, response);    
    }
}

void ConnectOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt311AsyncOpStatus_Timeout);
}

void ConnectOp::restartTimer()
{
    m_timer.wait(getResponseTimeout(), &ConnectOp::opTimeoutCb, this);
}

void ConnectOp::opTimeoutCb(void* data)
{
    asConnectOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt311_client

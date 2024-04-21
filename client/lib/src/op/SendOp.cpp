//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/SendOp.h"
#include "ClientImpl.h"

#include "comms/units.h"

namespace cc_mqtt311_client
{

namespace op
{

namespace 
{

inline SendOp* asSendOp(void* data)
{
    return reinterpret_cast<SendOp*>(data);
}

} // namespace     

SendOp::SendOp(ClientImpl& client) : 
    Base(client),
    m_responseTimer(client.timerMgr().allocTimer())
{
    COMMS_ASSERT(m_responseTimer.isValid());
}    

SendOp::~SendOp()
{
    releasePacketId(m_pubMsg.field_packetId().field().value());
}

#if CC_MQTT311_CLIENT_MAX_QOS >= 1 
void SendOp::handle(PubackMsg& msg)
{
    static_assert(Config::MaxQos >= 1);
    COMMS_ASSERT(m_pubMsg.field_packetId().field().value() == msg.field_packetId().value());
    COMMS_ASSERT(m_published);

    m_responseTimer.cancel();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client()]()
            {
                cl.brokerDisconnected(true);
            }
        );      

    auto status = CC_Mqtt311AsyncOpStatus_ProtocolError;

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status]()
            {
                completeWithCb(status);
            });        

    if (m_pubMsg.transportField_flags().field_qos().value() != Qos::AtLeastOnceDelivery) {
        errorLog("Unexpected PUBACK for Qos2 message");
        return;
    }

    terminateOnExit.release();
    status = CC_Mqtt311AsyncOpStatus_Complete;
}
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 1 

#if CC_MQTT311_CLIENT_MAX_QOS >= 2 
void SendOp::handle(PubrecMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    if (m_pubMsg.field_packetId().field().value() != msg.field_packetId().value()) {
        return;
    }

    COMMS_ASSERT(m_published);

    m_responseTimer.cancel();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client()]()
            {
                cl.brokerDisconnected(true);
            }
        );      

    auto status = CC_Mqtt311AsyncOpStatus_ProtocolError;

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status]()
            {
                completeWithCb(status);
            });    

    if (m_pubMsg.transportField_flags().field_qos().value() != Qos::ExactlyOnceDelivery) {
        errorLog("Unexpected PUBREC for Qos1 message");
        return;
    }

    if (m_acked) {
        errorLog("Double PUBREC message");
        return;
    }

    // Protocol wise it's all correct, no need to terminate any more
    terminateOnExit.release();

    m_acked = true;
    m_sendAttempts = 0U;
    PubrelMsg pubrelMsg;
    pubrelMsg.field_packetId().setValue(m_pubMsg.field_packetId().field().value());
    auto result = client().sendMessage(pubrelMsg); 
    if (result != CC_Mqtt311ErrorCode_Success) {
        errorLog("Failed to resend PUBREL message.");
        status = CC_Mqtt311AsyncOpStatus_InternalError;
        return;
    }

    completeOpOnExit.release();
    ++m_sendAttempts;
    restartResponseTimer();
}

void SendOp::handle(PubcompMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    if (m_pubMsg.field_packetId().field().value() != msg.field_packetId().value()) {
        return;
    }

    m_responseTimer.cancel();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client()]()
            {
                cl.brokerDisconnected(true);
            }
        );      

    auto status = CC_Mqtt311AsyncOpStatus_ProtocolError;

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status]()
            {
                completeWithCb(status);
            });  

    if (m_pubMsg.transportField_flags().field_qos().value() != Qos::ExactlyOnceDelivery) {
        errorLog("Unexpected PUBCOMP for Qos1 message");
        return;
    }

    if (!m_acked) {
        errorLog("Unexpected PUBCOMP without PUBREC");
        return;
    }    

    terminateOnExit.release();
    status = CC_Mqtt311AsyncOpStatus_Complete;
}
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 2

CC_Mqtt311ErrorCode SendOp::config(const CC_Mqtt311PublishConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic hasn't been provided in publish configuration");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (!verifyPubTopic(config.m_topic, true)) {
        errorLog("Bad topic format in publish.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    if (config.m_qos > static_cast<decltype(config.m_qos)>(Config::MaxQos)) {
        errorLog("QoS value is too high in publish.");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    m_pubMsg.transportField_flags().field_retain().setBitValue_bit(config.m_retain);
    m_pubMsg.transportField_flags().field_qos().setValue(config.m_qos);
    m_pubMsg.field_topic().value() = config.m_topic;

    auto& dataVec = m_pubMsg.field_payload().value();
    if (config.m_dataLen > 0U) {
        COMMS_ASSERT(config.m_data != nullptr);
        comms::util::assign(dataVec, config.m_data, config.m_data + config.m_dataLen);
    }

    if (maxStringLen() < dataVec.size()) {
        errorLog("Publish data value is too long");
        return CC_Mqtt311ErrorCode_BadParam;
    }      

    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode SendOp::setResendAttempts(unsigned attempts)
{
    if (attempts == 0U) {
        errorLog("PUBLISH resend attempts must be greater than 0");
        return CC_Mqtt311ErrorCode_BadParam;
    }

    m_totalSendAttempts = attempts;
    return CC_Mqtt311ErrorCode_Success;
}

unsigned SendOp::getResendAttempts() const
{
    return m_totalSendAttempts;
}

CC_Mqtt311ErrorCode SendOp::send(CC_Mqtt311PublishCompleteCb cb, void* cbData)
{
    client().allowNextPrepare();
    
    auto completeOnExit = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opCompleteInternal();
            });

    if (!m_responseTimer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt311ErrorCode_InternalError;
    }    

    if (m_pubMsg.field_topic().value().empty()) {
        errorLog("Topic hasn't been properly configured, cannot publish");
        return CC_Mqtt311ErrorCode_InsufficientConfig;
    }

    m_cb = cb;
    m_cbData = cbData;

    if (m_pubMsg.transportField_flags().field_qos().value() > Qos::AtMostOnceDelivery) {
        m_pubMsg.field_packetId().field().setValue(allocPacketId());
    }

    m_pubMsg.doRefresh(); // Update packetId presence

    if (!canSend()) {
        COMMS_ASSERT(!m_paused);
        m_paused = true;

        completeOnExit.release(); // don't complete op yet
        return CC_Mqtt311ErrorCode_Success;
    }

    auto guard = client().apiEnter();
    auto sendResult = doSendInternal();
    if (sendResult != CC_Mqtt311ErrorCode_Success) {
        return sendResult;
    }

    completeOnExit.release(); // don't complete op in this context
    return sendResult;
}

CC_Mqtt311ErrorCode SendOp::cancel()
{
    if (m_cb == nullptr) {
        // hasn't been sent yet
        client().allowNextPrepare();
    }

    opCompleteInternal();
    return CC_Mqtt311ErrorCode_Success;
}

void SendOp::postReconnectionResend()
{
    if (m_paused) {
        return;
    }

    COMMS_ASSERT(m_sendAttempts > 0U);
    --m_sendAttempts;
    m_responseTimer.cancel();
    resendDupMsg(); 
}

void SendOp::forceDupResend()
{
    if (m_paused) {
        return;
    }

    resendDupMsg(); 
}

bool SendOp::resume()
{
    if (!m_paused) {
        return false;
    }

    if (!canSend()) {
        return false;
    }

    m_paused = false;
    auto ec = doSendInternal();
    if (ec == CC_Mqtt311ErrorCode_Success) {
        return true;
    }

    auto cbStatus = CC_Mqtt311AsyncOpStatus_InternalError;
    if (ec == CC_Mqtt311ErrorCode_BadParam) {
        cbStatus = CC_Mqtt311AsyncOpStatus_BadParam;
    }
    else if (ec == CC_Mqtt311ErrorCode_BufferOverflow) {
        cbStatus = CC_Mqtt311AsyncOpStatus_OutOfMemory;
    }

    completeWithCb(cbStatus);
    return true;
}

Op::Type SendOp::typeImpl() const
{
    return Type_Send;
}

void SendOp::terminateOpImpl(CC_Mqtt311AsyncOpStatus status)
{
    completeWithCb(status);
}

void SendOp::connectivityChangedImpl()
{
    m_responseTimer.setSuspended(
        (!client().sessionState().m_connected) || client().clientState().m_networkDisconnected);
}

void SendOp::restartResponseTimer()
{
    auto& state = client().configState();
    m_responseTimer.wait(state.m_responseTimeoutMs, &SendOp::recvTimeoutCb, this);
}

void SendOp::responseTimeoutInternal()
{
    COMMS_ASSERT(!m_responseTimer.isActive());
    errorLog("Timeout on publish acknowledgement from broker.");
    resendDupMsg();
}

void SendOp::resendDupMsg()
{
    if (m_totalSendAttempts <= m_sendAttempts) {
        errorLog("Exhauses all attempts to publish message, discarding publish.");
        completeWithCb(CC_Mqtt311AsyncOpStatus_Timeout);
        return;
    }

    COMMS_ASSERT(m_published);
    if (!m_acked) {
        m_pubMsg.transportField_flags().field_dup().setBitValue_bit(true);
        auto result = client().sendMessage(m_pubMsg); 
        if (result != CC_Mqtt311ErrorCode_Success) {
            errorLog("Failed to resend PUBLISH message.");
            completeWithCb(CC_Mqtt311AsyncOpStatus_InternalError);
            return;
        }

        ++m_sendAttempts;
        restartResponseTimer();
        return;
    }

    COMMS_ASSERT(m_pubMsg.transportField_flags().field_qos().value() == Qos::ExactlyOnceDelivery);
    PubrelMsg pubrelMsg;
    pubrelMsg.field_packetId().setValue(m_pubMsg.field_packetId().field().value());
    auto result = client().sendMessage(pubrelMsg); 
    if (result != CC_Mqtt311ErrorCode_Success) {
        errorLog("Failed to resend PUBREL message.");
        completeWithCb(CC_Mqtt311AsyncOpStatus_InternalError);
        return;
    }

    ++m_sendAttempts;
    restartResponseTimer();
}

void SendOp::completeWithCb(CC_Mqtt311AsyncOpStatus status)
{
    auto cb = m_cb;
    auto cbData = m_cbData;
    auto handle = toHandle();

    opCompleteInternal(); // No member access after this point

    if (cb == nullptr) {
        return;
    }

    cb(cbData, handle, status);
}

CC_Mqtt311ErrorCode SendOp::doSendInternal()
{
    m_sendAttempts = 0U;
    auto result = client().sendMessage(m_pubMsg); 
    if (result != CC_Mqtt311ErrorCode_Success) {
        return result;
    }

    if (!m_published) {
        m_published = true;
    }

    ++m_sendAttempts;

    if (m_pubMsg.transportField_flags().field_qos().value() == Qos::AtMostOnceDelivery) {
        completeWithCb(CC_Mqtt311AsyncOpStatus_Complete);
        return CC_Mqtt311ErrorCode_Success;
    }

    restartResponseTimer();
    return CC_Mqtt311ErrorCode_Success;
}

bool SendOp::canSend() const
{
    auto qos = m_pubMsg.transportField_flags().field_qos().value();

    if (client().configState().m_publishOrdering == CC_Mqtt311PublishOrdering_SameQos) {
        return true;
    }

    COMMS_ASSERT(client().configState().m_publishOrdering == CC_Mqtt311PublishOrdering_Full);

    if ((client().hasPausedSendsBefore(this)) ||
        (client().hasHigherQosSendsBefore(this, qos))) {
        return false;
    }

    return true;
}

void SendOp::opCompleteInternal()
{
    opComplete();
}

void SendOp::recvTimeoutCb(void* data)
{
    asSendOp(data)->responseTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt311_client

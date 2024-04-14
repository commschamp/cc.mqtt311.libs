//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ClientImpl.h"

#include "comms/cast.h"
#include "comms/Assert.h"
#include "comms/process.h"
#include "comms/util/ScopeGuard.h"

#include <algorithm>
#include <type_traits>

namespace cc_mqtt311_client
{

namespace 
{

template <typename TList>
void eraseFromList(const op::Op* op, TList& list)
{
    auto iter = 
        std::find_if(
            list.begin(), list.end(),
            [op](auto& opPtr)
            {
                return op == opPtr.get();
            });

    COMMS_ASSERT(iter != list.end());
    if (iter == list.end()) {
        return;
    }

    list.erase(iter);
}

void updateEc(CC_Mqtt311ErrorCode* ec, CC_Mqtt311ErrorCode val)
{
    if (ec != nullptr) {
        *ec = val;
    }
}

} // namespace 

ClientImpl::ClientImpl() = default;

ClientImpl::~ClientImpl()
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    terminateOps(CC_Mqtt311AsyncOpStatus_Aborted, TerminateMode_AbortSendRecvOps);
}


void ClientImpl::tick(unsigned ms)
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    ++m_apiEnterCount;
    m_timerMgr.tick(ms);
    doApiExit();
}

unsigned ClientImpl::processData(const std::uint8_t* iter, unsigned len)
{
    auto guard = apiEnter();
    COMMS_ASSERT(!m_clientState.m_networkDisconnected);
    
    if (m_clientState.m_networkDisconnected) {
        errorLog("Incoming data when network is disconnected");
        return 0U;
    }

    auto disconnectOnExitGuard = 
        comms::util::makeScopeGuard(
            [this]()
            {
                brokerDisconnected(true, CC_Mqtt311AsyncOpStatus_ProtocolError);
            });

    unsigned consumed = 0;
    while (consumed < len) {
        auto remLen = len - consumed;
        auto* iterTmp = iter;

        using IdAndFlagsField = ProtFrame::Layer_idAndFlags::Field;
        static_assert(IdAndFlagsField::minLength() == IdAndFlagsField::maxLength());

        if (remLen <= IdAndFlagsField::minLength()) {
            // Size info is not available
            break;
        }

        using SizeField = ProtFrame::Layer_size::Field;
        SizeField sizeField;
        std::advance(iterTmp, IdAndFlagsField::minLength());
        auto es = sizeField.read(iterTmp, remLen - IdAndFlagsField::minLength());
        if (es == comms::ErrorStatus::NotEnoughData) {
            break;
        }

        if (es != comms::ErrorStatus::Success) {
            return len; // Disconnect
        }        

        iterTmp = iter;
        ProtFrame::MsgPtr msg;
        es = m_frame.read(msg, iterTmp, remLen);
        if (es == comms::ErrorStatus::NotEnoughData) {
            break;
        }

        if (es != comms::ErrorStatus::Success) {
            errorLog("Unexpected error in framing / payload parsing");
            return len;
        }

        COMMS_ASSERT(msg);
        msg->dispatch(*this);
        consumed += static_cast<unsigned>(std::distance(iter, iterTmp));
        iter = iterTmp;
    }

    disconnectOnExitGuard.release();
    return consumed;    
}

void ClientImpl::notifyNetworkDisconnected()
{
    auto guard = apiEnter();
    m_clientState.m_networkDisconnected = true;
    if (m_sessionState.m_disconnecting) {
        return; // No need to go through broker disconnection
    }
    
    brokerDisconnected(false);
}

bool ClientImpl::isNetworkDisconnected() const
{
    return m_clientState.m_networkDisconnected;
}


op::ConnectOp* ClientImpl::connectPrepare(CC_Mqtt311ErrorCode* ec)
{
    op::ConnectOp* connectOp = nullptr;
    do {
        m_clientState.m_networkDisconnected = false;

        if (!m_clientState.m_initialized) {
            if (m_apiEnterCount > 0U) {
                errorLog("Cannot prepare connect from within callback");
                updateEc(ec, CC_Mqtt311ErrorCode_RetryLater);
                break;
            }

            auto initEc = initInternal();
            if (initEc != CC_Mqtt311ErrorCode_Success) {
                updateEc(ec, initEc);
                break;
            }
        }
                
        if (!m_connectOps.empty()) {
            // Already allocated
            errorLog("Another connect operation is in progress.");
            updateEc(ec, CC_Mqtt311ErrorCode_Busy);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate connection.");
            updateEc(ec, CC_Mqtt311ErrorCode_Disconnecting);
            break;
        }

        if (m_sessionState.m_connected) {
            errorLog("Client is already connected.");
            updateEc(ec, CC_Mqtt311ErrorCode_AlreadyConnected);
            break;
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start connect operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt311ErrorCode_RetryLater);
            break;
        }

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"connect\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt311ErrorCode_PreparationLocked);            
            break;
        }

        auto ptr = m_connectOpAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new connect operation.");
            updateEc(ec, CC_Mqtt311ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
        m_ops.push_back(ptr.get());
        m_connectOps.push_back(std::move(ptr));
        connectOp = m_connectOps.back().get();
        updateEc(ec, CC_Mqtt311ErrorCode_Success);
    } while (false);

    return connectOp;
}

op::DisconnectOp* ClientImpl::disconnectPrepare(CC_Mqtt311ErrorCode* ec)
{
    op::DisconnectOp* disconnectOp = nullptr;
    do {
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow disconnect.");
            updateEc(ec, CC_Mqtt311ErrorCode_NotConnected);
            break;
        }

        if (!m_disconnectOps.empty()) {
            errorLog("Another disconnect operation is in progress.");
            updateEc(ec, CC_Mqtt311ErrorCode_Busy);
            break;
        }        

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate disconnection.");
            updateEc(ec, CC_Mqtt311ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt311ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start disconnect operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt311ErrorCode_RetryLater);
            break;
        }   

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"disconnect\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt311ErrorCode_PreparationLocked);            
            break;
        }            

        auto ptr = m_disconnectOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new disconnect operation.");
            updateEc(ec, CC_Mqtt311ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
        m_ops.push_back(ptr.get());
        m_disconnectOps.push_back(std::move(ptr));
        disconnectOp = m_disconnectOps.back().get();
        updateEc(ec, CC_Mqtt311ErrorCode_Success);
    } while (false);

    return disconnectOp;
}

op::SubscribeOp* ClientImpl::subscribePrepare(CC_Mqtt311ErrorCode* ec)
{
    op::SubscribeOp* subOp = nullptr;
    do {
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow subscription.");
            updateEc(ec, CC_Mqtt311ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate subscription.");
            updateEc(ec, CC_Mqtt311ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt311ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start subscribe operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt311ErrorCode_RetryLater);
            break;
        }  

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"subscribe\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt311ErrorCode_PreparationLocked);            
            break;
        }            

        auto ptr = m_subscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new subscribe operation.");
            updateEc(ec, CC_Mqtt311ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
        m_ops.push_back(ptr.get());
        m_subscribeOps.push_back(std::move(ptr));
        subOp = m_subscribeOps.back().get();
        updateEc(ec, CC_Mqtt311ErrorCode_Success);
    } while (false);

    return subOp;
}

op::UnsubscribeOp* ClientImpl::unsubscribePrepare(CC_Mqtt311ErrorCode* ec)
{
    op::UnsubscribeOp* unsubOp = nullptr;
    do {
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow unsubscription.");
            updateEc(ec, CC_Mqtt311ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate unsubscription.");
            updateEc(ec, CC_Mqtt311ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt311ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start subscribe operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt311ErrorCode_RetryLater);
            break;
        }    

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"unsubscribe\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt311ErrorCode_PreparationLocked);            
            break;
        }             

        auto ptr = m_unsubscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new unsubscribe operation.");
            updateEc(ec, CC_Mqtt311ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
        m_ops.push_back(ptr.get());
        m_unsubscribeOps.push_back(std::move(ptr));
        unsubOp = m_unsubscribeOps.back().get();
        updateEc(ec, CC_Mqtt311ErrorCode_Success);
    } while (false);

    return unsubOp;
}

op::SendOp* ClientImpl::publishPrepare(CC_Mqtt311ErrorCode* ec)
{
    op::SendOp* sendOp = nullptr;
    do {
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow publish.");
            updateEc(ec, CC_Mqtt311ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate publish.");
            updateEc(ec, CC_Mqtt311ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt311ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start publish operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt311ErrorCode_RetryLater);
            break;
        }        

        auto ptr = m_sendOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new publish operation.");
            updateEc(ec, CC_Mqtt311ErrorCode_OutOfMemory);
            break;
        }

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"unsubscribe\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt311ErrorCode_PreparationLocked);            
            break;
        }          

        m_preparationLocked = true;
        m_ops.push_back(ptr.get());
        m_sendOps.push_back(std::move(ptr));
        sendOp = m_sendOps.back().get();
        updateEc(ec, CC_Mqtt311ErrorCode_Success);
    } while (false);

    return sendOp;
}

void ClientImpl::handle(PublishMsg& msg)
{
    if (m_sessionState.m_disconnecting) {
        return;
    }

    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }       

    do {
        auto createRecvOp = 
            [this, &msg]()
            {
                auto ptr = m_recvOpsAlloc.alloc(*this);
                if (!ptr) {
                    errorLog("Failed to allocate handling op for the incoming PUBLISH message, ignoring.");
                    return; 
                }

                m_ops.push_back(ptr.get());
                m_recvOps.push_back(std::move(ptr));
                msg.dispatch(*m_recvOps.back());
            };

        using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
        auto qos = msg.transportField_flags().field_qos().value();
        if ((qos == Qos::AtMostOnceDelivery) || 
            (qos == Qos::AtLeastOnceDelivery)) {
            createRecvOp();
            break;
        }

        if constexpr (Config::MaxQos >= 2) {
            auto iter = 
                std::find_if(
                    m_recvOps.begin(), m_recvOps.end(),
                    [&msg](auto& opPtr)
                    {
                        return opPtr->packetId() == msg.field_packetId().field().value();
                    });

            if (iter == m_recvOps.end()) {
                createRecvOp();
                break;            
            }

            PubrecMsg pubrecMsg;
            pubrecMsg.field_packetId().setValue(msg.field_packetId().field().value());

            if (!msg.transportField_flags().field_dup().getBitValue_bit()) {
                errorLog("Non duplicate PUBLISH with packet ID in use");
                brokerDisconnected(true);
                return;
            }
            else {
                // Duplicate detected, just re-confirming
                (*iter)->resetTimer();
            }

            sendMessage(pubrecMsg);
            return;
        }
        else {
            createRecvOp();
            break;
        }
    } while (false);
}

#if CC_MQTT311_CLIENT_MAX_QOS >= 1
void ClientImpl::handle(PubackMsg& msg)
{
    static_assert(Config::MaxQos >= 1);
    if (!handlePublishRelatedMessage(msg, msg.field_packetId().value())) {
        errorLog("PUBACK with unknown packet id");
    }    
}
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 1

#if CC_MQTT311_CLIENT_MAX_QOS >= 2
void ClientImpl::handle(PubrecMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    if (!handlePublishRelatedMessage(msg, msg.field_packetId().value())) {
        errorLog("PUBREC with unknown packet id");
    }
}

void ClientImpl::handle(PubrelMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }

    auto iter = 
        std::find_if(
            m_recvOps.begin(), m_recvOps.end(),
            [&msg](auto& opPtr)
            {
                COMMS_ASSERT(opPtr);
                return opPtr->packetId() == msg.field_packetId().value();
            });

    if (iter == m_recvOps.end()) {
        errorLog("PUBREL with unknown packet id");
        return;
    }

    msg.dispatch(**iter);
}

void ClientImpl::handle(PubcompMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    if (!handlePublishRelatedMessage(msg, msg.field_packetId().value())) {
        errorLog("PUBCOMP with unknown packet id");
    }
}

#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 2

void ClientImpl::handle(ProtMessage& msg)
{
    static_cast<void>(msg);
    if (m_sessionState.m_disconnecting) {
        return;
    }

    // During the dispatch to callbacks can be called and new ops issues,
    // the m_ops vector can be resized and iterators invalidated.
    // As the result, the iteration needs to be performed using indices 
    // instead of iterators.
    // Also do not dispatch the message to new ops.
    auto count = m_ops.size();
    for (auto idx = 0U; idx < count; ++idx) {
        auto* op = m_ops[idx];
        if (op == nullptr) {
            // ops can be deleted, but the pointer will be nullified
            // until last api guard.
            continue;
        }

        msg.dispatch(*op);

        // After message dispatching the whole session may be in terminating state
        // Don't continue iteration
        if (m_sessionState.m_disconnecting) {
            break;
        }    
    }
}

CC_Mqtt311ErrorCode ClientImpl::sendMessage(const ProtMessage& msg)
{
    auto len = m_frame.length(msg);

    if (m_buf.max_size() < len) {
        errorLog("Output buffer overflow.");
        return CC_Mqtt311ErrorCode_BufferOverflow;
    }

    m_buf.resize(len);
    auto writeIter = comms::writeIteratorFor<ProtMessage>(&m_buf[0]);
    auto es = m_frame.write(msg, writeIter, len);
    COMMS_ASSERT(es == comms::ErrorStatus::Success);
    if (es != comms::ErrorStatus::Success) {
        errorLog("Failed to serialize output message.");
        return CC_Mqtt311ErrorCode_InternalError;
    }

    COMMS_ASSERT(m_sendOutputDataCb != nullptr);
    m_sendOutputDataCb(m_sendOutputDataData, &m_buf[0], static_cast<unsigned>(len));

    for (auto& opPtr : m_keepAliveOps) {
        opPtr->messageSent();
    }

    return CC_Mqtt311ErrorCode_Success;
}

void ClientImpl::opComplete(const op::Op* op)
{
    auto iter = std::find(m_ops.begin(), m_ops.end(), op);
    COMMS_ASSERT(iter != m_ops.end());
    if (iter == m_ops.end()) {
        return;
    }

    *iter = nullptr;
    m_opsDeleted = true;

    using ExtraCompleteFunc = void (ClientImpl::*)(const op::Op*);
    static const ExtraCompleteFunc Map[] = {
        /* Type_Connect */ &ClientImpl::opComplete_Connect,
        /* Type_KeepAlive */ &ClientImpl::opComplete_KeepAlive,
        /* Type_Disconnect */ &ClientImpl::opComplete_Disconnect,
        /* Type_Subscribe */ &ClientImpl::opComplete_Subscribe,
        /* Type_Unsubscribe */ &ClientImpl::opComplete_Unsubscribe,
        /* Type_Recv */ &ClientImpl::opComplete_Recv,
        /* Type_Send */ &ClientImpl::opComplete_Send,
    };
    static const std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == op::Op::Type_NumOfValues);

    auto idx = static_cast<unsigned>(op->type());
    COMMS_ASSERT(idx < MapSize);
    if (MapSize <= idx) {
        return;
    }

    auto func = Map[idx];
    (this->*func)(op);
}

void ClientImpl::brokerConnected(bool sessionPresent)
{
    static_cast<void>(sessionPresent);
    m_clientState.m_firstConnect = false;
    m_sessionState.m_connected = true;

    do {
        if (sessionPresent) {
            for (auto& sendOpPtr : m_sendOps) {
                sendOpPtr->postReconnectionResend();
            }  

            for (auto& recvOpPtr : m_recvOps) {
                recvOpPtr->postReconnectionResume();
            }    

            break;
        }

        // Old stored session, terminate pending ops
        for (auto* op : m_ops) {
            auto opType = op->type();
            if ((opType != op::Op::Type::Type_Send) && 
                (opType != op::Op::Type::Type_Recv)) {
                continue;
            }

            op->terminateOp(CC_Mqtt311AsyncOpStatus_Aborted);
        }
    } while (false);

    createKeepAliveOpIfNeeded();    
}

void ClientImpl::brokerDisconnected(
    bool reportDisconnection, 
    CC_Mqtt311AsyncOpStatus status)
{
    static_cast<void>(reportDisconnection);
    static_cast<void>(status);
    m_clientState.m_initialized = false; // Require re-initialization
    m_sessionState.m_connected = false;

    m_sessionState.m_disconnecting = true;
    terminateOps(status, TerminateMode_KeepSendRecvOps);    

    for (auto* op : m_ops) {
        if (op != nullptr) {
            op->connectivityChanged();
        }
    } 

    if (reportDisconnection) {
        COMMS_ASSERT(m_brokerDisconnectReportCb != nullptr);
        m_brokerDisconnectReportCb(m_brokerDisconnectReportData);
    }
}

void ClientImpl::reportMsgInfo(const CC_Mqtt311MessageInfo& info)
{
    COMMS_ASSERT(m_messageReceivedReportCb != nullptr);
    m_messageReceivedReportCb(m_messageReceivedReportData, &info);
}

void ClientImpl::allowNextPrepare()
{
    COMMS_ASSERT(m_preparationLocked);
    m_preparationLocked = false;
}

void ClientImpl::doApiEnter()
{
    ++m_apiEnterCount;
    if ((m_apiEnterCount > 1U) || (m_cancelNextTickWaitCb == nullptr)) {
        return;
    }

    auto prevWait = m_timerMgr.getMinWait();
    if (prevWait == 0U) {
        return;
    }

    auto elapsed = m_cancelNextTickWaitCb(m_cancelNextTickWaitData);
    m_timerMgr.tick(elapsed);
}

void ClientImpl::doApiExit()
{
    COMMS_ASSERT(m_apiEnterCount > 0U);
    --m_apiEnterCount;
    if (m_apiEnterCount > 0U) {
        return;
    }

    cleanOps();

    if (m_nextTickProgramCb == nullptr) {
        return;
    }

    auto nextWait = m_timerMgr.getMinWait();
    if (nextWait == 0U) {
        return;
    }

    m_nextTickProgramCb(m_nextTickProgramData, nextWait);
}

void ClientImpl::createKeepAliveOpIfNeeded()
{
    if (!m_keepAliveOps.empty()) {
        return;
    }

    auto ptr = m_keepAliveOpsAlloc.alloc(*this);
    if (!ptr) {
        COMMS_ASSERT(false); // Should not happen
        return;
    }    

    m_ops.push_back(ptr.get());
    m_keepAliveOps.push_back(std::move(ptr));
}

void ClientImpl::terminateOps(CC_Mqtt311AsyncOpStatus status, TerminateMode mode)
{
    for (auto* op : m_ops) {
        if (op == nullptr) {
            continue;
        }

        if (mode == TerminateMode_KeepSendRecvOps) {
            auto opType = op->type();

            if ((opType == op::Op::Type_Recv) || (opType == op::Op::Type_Send)) {
                continue;
            }
        }

        op->terminateOp(status);
    }
}

void ClientImpl::cleanOps()
{
    if (!m_opsDeleted) {
        return;
    }

    m_ops.erase(
        std::remove_if(
            m_ops.begin(), m_ops.end(),
            [](auto* op)
            {
                return op == nullptr;
            }),
        m_ops.end());

    m_opsDeleted = false;
}

void ClientImpl::errorLogInternal(const char* msg)
{
    if constexpr (Config::HasErrorLog) {
        if (m_errorLogCb == nullptr) {
            return;
        }

        m_errorLogCb(m_errorLogData, msg);
    }
}

CC_Mqtt311ErrorCode ClientImpl::initInternal()
{
    auto guard = apiEnter();
    if ((m_sendOutputDataCb == nullptr) ||
        (m_brokerDisconnectReportCb == nullptr) ||
        (m_messageReceivedReportCb == nullptr)) {
        errorLog("Hasn't set all must have callbacks");
        return CC_Mqtt311ErrorCode_NotIntitialized;
    }

    bool hasTimerCallbacks = 
        (m_nextTickProgramCb != nullptr) ||
        (m_cancelNextTickWaitCb != nullptr);

    if (hasTimerCallbacks) {
        bool hasAllTimerCallbacks = 
            (m_nextTickProgramCb != nullptr) &&
            (m_cancelNextTickWaitCb != nullptr);

        if (!hasAllTimerCallbacks) {
            errorLog("Hasn't set all timer management callbacks callbacks");
            return CC_Mqtt311ErrorCode_NotIntitialized;
        }
    }

    terminateOps(CC_Mqtt311AsyncOpStatus_Aborted, TerminateMode_KeepSendRecvOps);
    m_sessionState = SessionState();
    m_clientState.m_initialized = true;
    return CC_Mqtt311ErrorCode_Success;
}

bool ClientImpl::handlePublishRelatedMessage(ProtMessage& msg, unsigned packetId)
{
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }

    auto iter = 
        std::find_if(
            m_sendOps.begin(), m_sendOps.end(),
            [packetId](auto& opPtr)
            {
                return opPtr->packetId() == packetId;
            });

    if (iter != m_sendOps.end()) {
        return false;
    }

    msg.dispatch(**iter);
    return true;
}

void ClientImpl::opComplete_Connect(const op::Op* op)
{
    eraseFromList(op, m_connectOps);
}

void ClientImpl::opComplete_KeepAlive(const op::Op* op)
{
    eraseFromList(op, m_keepAliveOps);
}

void ClientImpl::opComplete_Disconnect(const op::Op* op)
{
    eraseFromList(op, m_disconnectOps);
}

void ClientImpl::opComplete_Subscribe(const op::Op* op)
{
    eraseFromList(op, m_subscribeOps);
}

void ClientImpl::opComplete_Unsubscribe(const op::Op* op)
{
    eraseFromList(op, m_unsubscribeOps);
}

void ClientImpl::opComplete_Recv(const op::Op* op)
{
    eraseFromList(op, m_recvOps);
}

void ClientImpl::opComplete_Send(const op::Op* op)
{
    eraseFromList(op, m_sendOps);
}

} // namespace cc_mqtt311_client

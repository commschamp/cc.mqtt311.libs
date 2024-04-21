//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ClientState.h"
#include "ConfigState.h"
#include "ExtConfig.h"
#include "ObjAllocator.h"
#include "ObjListType.h"
#include "ProtocolDefs.h"
#include "ReuseState.h"
#include "SessionState.h"
#include "TimerMgr.h"

#include "op/ConnectOp.h"
#include "op/DisconnectOp.h"
#include "op/KeepAliveOp.h"
#include "op/Op.h"
#include "op/RecvOp.h"
#include "op/SendOp.h"
#include "op/SubscribeOp.h"
#include "op/UnsubscribeOp.h"

#include "cc_mqtt311_client/common.h"

namespace cc_mqtt311_client
{

class ClientImpl final : public ProtMsgHandler
{
    using Base = ProtMsgHandler;

public:
    class ApiEnterGuard
    {
    public:
        ApiEnterGuard(ClientImpl& client) : m_client(client)
        {
            m_client.doApiEnter();
        }

        ~ApiEnterGuard() noexcept
        {
            m_client.doApiExit();
        }

    private:
        ClientImpl& m_client;
    };

    ClientImpl();
    ~ClientImpl();

    ApiEnterGuard apiEnter()
    {
        return ApiEnterGuard(*this);
    }

    // -------------------- API Calls -----------------------------
    void tick(unsigned ms);
    unsigned processData(const std::uint8_t* iter, unsigned len);
    void notifyNetworkDisconnected();
    bool isNetworkDisconnected() const;

    op::ConnectOp* connectPrepare(CC_Mqtt311ErrorCode* ec);
    op::DisconnectOp* disconnectPrepare(CC_Mqtt311ErrorCode* ec);
    op::SubscribeOp* subscribePrepare(CC_Mqtt311ErrorCode* ec);
    op::UnsubscribeOp* unsubscribePrepare(CC_Mqtt311ErrorCode* ec);
    op::SendOp* publishPrepare(CC_Mqtt311ErrorCode* ec);

    CC_Mqtt311ErrorCode setPublishOrdering(CC_Mqtt311PublishOrdering ordering);
    CC_Mqtt311PublishOrdering getPublishOrdering() const
    {
        return m_configState.m_publishOrdering;
    }    
    
    std::size_t sendsCount() const
    {
        return m_sendOps.size();
    }

    void setNextTickProgramCallback(CC_Mqtt311NextTickProgramCb cb, void* data)
    {
        if (cb != nullptr) {
            m_nextTickProgramCb = cb;
            m_nextTickProgramData = data;
        }
    }

    void setCancelNextTickWaitCallback(CC_Mqtt311CancelNextTickWaitCb cb, void* data)
    {
        if (cb != nullptr) {
            m_cancelNextTickWaitCb = cb;
            m_cancelNextTickWaitData = data;
        }
    }

    void setSendOutputDataCallback(CC_Mqtt311SendOutputDataCb cb, void* data)
    {
        if (cb != nullptr) {
            m_sendOutputDataCb = cb;
            m_sendOutputDataData = data;
        }
    }

    void setBrokerDisconnectReportCallback(CC_Mqtt311BrokerDisconnectReportCb cb, void* data)
    {
        if (cb != nullptr) {
            m_brokerDisconnectReportCb = cb;
            m_brokerDisconnectReportData = data;
        }
    }

    void setMessageReceivedCallback(CC_Mqtt311MessageReceivedReportCb cb, void* data)
    {
        if (cb != nullptr) {
            m_messageReceivedReportCb = cb;
            m_messageReceivedReportData = data;            
        }
    }

    void setErrorLogCallback(CC_Mqtt311ErrorLogCb cb, void* data)
    {
        m_errorLogCb = cb;
        m_errorLogData = data;
    }

    // -------------------- Message Handling -----------------------------

    using Base::handle;
    virtual void handle(PublishMsg& msg) override;

#if CC_MQTT311_CLIENT_MAX_QOS >= 1    
    virtual void handle(PubackMsg& msg) override;
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 1    

#if CC_MQTT311_CLIENT_MAX_QOS >= 2
    virtual void handle(PubrecMsg& msg) override;
    virtual void handle(PubrelMsg& msg) override;
    virtual void handle(PubcompMsg& msg) override;
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 2

    virtual void handle(ProtMessage& msg) override;

    // -------------------- Ops Access API -----------------------------

    CC_Mqtt311ErrorCode sendMessage(const ProtMessage& msg);
    void opComplete(const op::Op* op);
    void brokerConnected(bool sessionPresent);
    void brokerDisconnected(
        bool reportDisconnection, 
        CC_Mqtt311AsyncOpStatus status = CC_Mqtt311AsyncOpStatus_BrokerDisconnected);
    void reportMsgInfo(const CC_Mqtt311MessageInfo& info);
    bool hasPausedSendsBefore(const op::SendOp* sendOp) const;
    bool hasHigherQosSendsBefore(const op::SendOp* sendOp, op::Op::Qos qos) const;
    void allowNextPrepare();

    TimerMgr& timerMgr()
    {
        return m_timerMgr;
    }

    ConfigState& configState()
    {
        return m_configState;
    }

    const ConfigState& configState() const
    {
        return m_configState;
    }

    ClientState& clientState()
    {
        return m_clientState;
    }    

    const ClientState& clientState() const
    {
        return m_clientState;
    }       

    SessionState& sessionState()
    {
        return m_sessionState;
    }

    const SessionState& sessionState() const
    {
        return m_sessionState;
    }    

    ReuseState& reuseState()
    {
        return m_reuseState;
    }    

    inline void errorLog(const char* msg)
    {
        if constexpr (Config::HasErrorLog) {
            errorLogInternal(msg);
        }
    }

    // std::size_t recvsCount() const
    // {
    //     return m_recvOps.size();
    // }    
    
private:
    using ConnectOpAlloc = ObjAllocator<op::ConnectOp, ExtConfig::ConnectOpsLimit>;
    using ConnectOpsList = ObjListType<ConnectOpAlloc::Ptr, ExtConfig::ConnectOpsLimit>;

    using KeepAliveOpAlloc = ObjAllocator<op::KeepAliveOp, ExtConfig::KeepAliveOpsLimit>;
    using KeepAliveOpsList = ObjListType<KeepAliveOpAlloc::Ptr, ExtConfig::KeepAliveOpsLimit>;

    using DisconnectOpAlloc = ObjAllocator<op::DisconnectOp, ExtConfig::DisconnectOpsLimit>;
    using DisconnectOpsList = ObjListType<DisconnectOpAlloc::Ptr, ExtConfig::DisconnectOpsLimit>;

    using SubscribeOpAlloc = ObjAllocator<op::SubscribeOp, ExtConfig::SubscribeOpsLimit>;
    using SubscribeOpsList = ObjListType<SubscribeOpAlloc::Ptr, ExtConfig::SubscribeOpsLimit>;

    using UnsubscribeOpAlloc = ObjAllocator<op::UnsubscribeOp, ExtConfig::UnsubscribeOpsLimit>;
    using UnsubscribeOpsList = ObjListType<UnsubscribeOpAlloc::Ptr, ExtConfig::UnsubscribeOpsLimit>;

    using RecvOpAlloc = ObjAllocator<op::RecvOp, ExtConfig::RecvOpsLimit>;
    using RecvOpsList = ObjListType<RecvOpAlloc::Ptr, ExtConfig::RecvOpsLimit>;

    using SendOpAlloc = ObjAllocator<op::SendOp, ExtConfig::SendOpsLimit>;
    using SendOpsList = ObjListType<SendOpAlloc::Ptr, ExtConfig::SendOpsLimit>;

    using OpPtrsList = ObjListType<op::Op*, ExtConfig::OpsLimit>;
    using OpToDeletePtrsList = ObjListType<const op::Op*, ExtConfig::OpsLimit>;
    using OutputBuf = ObjListType<std::uint8_t, ExtConfig::MaxOutputPacketSize>;

    enum TerminateMode
    {
        TerminateMode_KeepSendRecvOps,
        TerminateMode_AbortSendRecvOps,
        TerminateMode_NumOfValues
    };

    void doApiEnter();
    void doApiExit();
    void createKeepAliveOpIfNeeded();
    void terminateOps(CC_Mqtt311AsyncOpStatus status, TerminateMode mode);
    void cleanOps();
    void errorLogInternal(const char* msg);
    CC_Mqtt311ErrorCode initInternal();
    op::SendOp* findSendOp(std::uint16_t packetId);
    bool isLegitSendAck(const op::SendOp* sendOp, bool pubcompAck = false) const;
    void resendAllUntil(op::SendOp* sendOp);
    bool processPublishAckMsg(ProtMessage& msg, std::uint16_t packetId, bool pubcompAck = false);

    void opComplete_Connect(const op::Op* op);
    void opComplete_KeepAlive(const op::Op* op);
    void opComplete_Disconnect(const op::Op* op);
    void opComplete_Subscribe(const op::Op* op);
    void opComplete_Unsubscribe(const op::Op* op);
    void opComplete_Recv(const op::Op* op);
    void opComplete_Send(const op::Op* op);

    friend class ApiEnterGuard;

    CC_Mqtt311NextTickProgramCb m_nextTickProgramCb = nullptr;
    void* m_nextTickProgramData = nullptr;

    CC_Mqtt311CancelNextTickWaitCb m_cancelNextTickWaitCb = nullptr;
    void* m_cancelNextTickWaitData = nullptr;

    CC_Mqtt311SendOutputDataCb m_sendOutputDataCb = nullptr;
    void* m_sendOutputDataData = nullptr;

    CC_Mqtt311BrokerDisconnectReportCb m_brokerDisconnectReportCb = nullptr;
    void* m_brokerDisconnectReportData = nullptr;

    CC_Mqtt311MessageReceivedReportCb m_messageReceivedReportCb = nullptr;
    void* m_messageReceivedReportData = nullptr;      

    CC_Mqtt311ErrorLogCb m_errorLogCb = nullptr;
    void* m_errorLogData = nullptr;

    ConfigState m_configState;
    ClientState m_clientState;
    SessionState m_sessionState;
    ReuseState m_reuseState;

    TimerMgr m_timerMgr;
    unsigned m_apiEnterCount = 0U;

    OutputBuf m_buf;

    ProtFrame m_frame;

    ConnectOpAlloc m_connectOpAlloc;
    ConnectOpsList m_connectOps;

    KeepAliveOpAlloc m_keepAliveOpsAlloc;
    KeepAliveOpsList m_keepAliveOps;

    DisconnectOpAlloc m_disconnectOpsAlloc;
    DisconnectOpsList m_disconnectOps;

    SubscribeOpAlloc m_subscribeOpsAlloc;
    SubscribeOpsList m_subscribeOps;

    UnsubscribeOpAlloc m_unsubscribeOpsAlloc;
    UnsubscribeOpsList m_unsubscribeOps;

    RecvOpAlloc m_recvOpsAlloc;
    RecvOpsList m_recvOps;

    SendOpAlloc m_sendOpsAlloc;
    SendOpsList m_sendOps;

    OpPtrsList m_ops;
    bool m_opsDeleted = false;
    bool m_preparationLocked = false;
};

} // namespace cc_mqtt311_client

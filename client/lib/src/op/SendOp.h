//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ExtConfig.h"
#include "ProtocolDefs.h"

#include "TimerMgr.h"

namespace cc_mqtt311_client
{

namespace op
{

class SendOp final : public Op
{
    using Base = Op;
public:
    explicit SendOp(ClientImpl& client);
    ~SendOp();

    using Base::handle;

#if CC_MQTT311_CLIENT_MAX_QOS >= 1  
    virtual void handle(PubackMsg& msg) override;
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 1  

#if CC_MQTT311_CLIENT_MAX_QOS >= 2  
    virtual void handle(PubrecMsg& msg) override;
    virtual void handle(PubcompMsg& msg) override;
#endif // #if CC_MQTT311_CLIENT_MAX_QOS >= 2    

    CC_Mqtt311PublishHandle toHandle()
    {
        return reinterpret_cast<CC_Mqtt311PublishHandle>(this);
    }

    static CC_Mqtt311PublishHandle asHandle(SendOp* obj)
    {
        return reinterpret_cast<CC_Mqtt311PublishHandle>(obj);
    }    

    static SendOp* fromHandle(CC_Mqtt311PublishHandle handle)
    {
        return reinterpret_cast<SendOp*>(handle);
    }    

    unsigned packetId() const
    {
        return m_pubMsg.field_packetId().field().value();
    }

    CC_Mqtt311ErrorCode config(const CC_Mqtt311PublishConfig& config);
    CC_Mqtt311ErrorCode setResendAttempts(unsigned attempts);
    unsigned getResendAttempts() const;
    CC_Mqtt311ErrorCode send(CC_Mqtt311PublishCompleteCb cb, void* cbData);
    CC_Mqtt311ErrorCode cancel();
    void postReconnectionResend();

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt311AsyncOpStatus status) override;
    virtual void connectivityChangedImpl() override;

private:
    void restartResponseTimer();
    void responseTimeoutInternal();
    void resendDupMsg();
    void completeWithCb(CC_Mqtt311AsyncOpStatus status);
    CC_Mqtt311ErrorCode doSendInternal();
    void opCompleteInternal();

    static void recvTimeoutCb(void* data);

    TimerMgr::Timer m_responseTimer;  
    PublishMsg m_pubMsg;
    CC_Mqtt311PublishCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;    
    unsigned m_totalSendAttempts = DefaultSendAttempts;
    unsigned m_sendAttempts = 0U;
    bool m_acked = false;

    static constexpr unsigned DefaultSendAttempts = 2U;
    static_assert(ExtConfig::SendOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt311_client

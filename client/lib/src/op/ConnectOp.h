//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
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

class ConnectOp final : public Op
{
    using Base = Op;
public:
    explicit ConnectOp(ClientImpl& client);

    CC_Mqtt311ErrorCode config(const CC_Mqtt311ConnectConfig& config);
    CC_Mqtt311ErrorCode configWill(const CC_Mqtt311ConnectWillConfig& config);
    CC_Mqtt311ErrorCode send(CC_Mqtt311ConnectCompleteCb cb, void* cbData);
    CC_Mqtt311ErrorCode cancel();

    using Base::handle;
    virtual void handle(ConnackMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt311AsyncOpStatus status) override;
    virtual void connectivityChangedImpl() override;

private:
    void completeOpInternal(CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311ConnectResponse* response = nullptr);
    void opTimeoutInternal();
    void restartTimer();

    static void opTimeoutCb(void* data);

    ConnectMsg m_connectMsg;  
    TimerMgr::Timer m_timer;  
    CC_Mqtt311ConnectCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;

    static_assert(ExtConfig::ConnectOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt311_client

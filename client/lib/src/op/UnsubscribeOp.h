//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ProtocolDefs.h"
#include "TimerMgr.h"

namespace cc_mqtt311_client
{

namespace op
{

class UnsubscribeOp final : public Op
{
    using Base = Op;
public:
    explicit UnsubscribeOp(ClientImpl& client);
    ~UnsubscribeOp();

    CC_Mqtt311ErrorCode configTopic(const CC_Mqtt311UnsubscribeTopicConfig& config);
    CC_Mqtt311ErrorCode send(CC_Mqtt311UnsubscribeCompleteCb cb, void* cbData);
    CC_Mqtt311ErrorCode cancel();

    CC_Mqtt311UnsubscribeHandle toHandle()
    {
        return reinterpret_cast<CC_Mqtt311UnsubscribeHandle>(this);
    }

    static CC_Mqtt311UnsubscribeHandle asHandle(UnsubscribeOp* obj)
    {
        return reinterpret_cast<CC_Mqtt311UnsubscribeHandle>(obj);
    }    

    static UnsubscribeOp* fromHandle(CC_Mqtt311UnsubscribeHandle handle)
    {
        return reinterpret_cast<UnsubscribeOp*>(handle);
    }    

    using Base::handle;
    virtual void handle(UnsubackMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt311AsyncOpStatus status) override;

private:
    void completeOpInternal(CC_Mqtt311AsyncOpStatus status);
    void opTimeoutInternal();
    void restartTimer();

    static void opTimeoutCb(void* data);

    UnsubscribeMsg m_unsubMsg;
    TimerMgr::Timer m_timer;
    CC_Mqtt311UnsubscribeCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;

    static_assert(ExtConfig::UnsubscribeOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt311_client

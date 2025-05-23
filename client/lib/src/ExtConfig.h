//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Config.h"

#include <vector>

namespace cc_mqtt311_client
{

struct ExtConfig : public Config
{
    static constexpr unsigned ConnectOpsLimit = HasDynMemAlloc ? 0 : 1U;
    static constexpr unsigned KeepAliveOpsLimit = HasDynMemAlloc ? 0 : 1U;
    static constexpr unsigned ConnectOpTimers = 1U;
    static constexpr unsigned KeepAliveOpTimers = 3U;
    static constexpr unsigned DisconnectOpsLimit = HasDynMemAlloc ? 0 : 1U;
    static constexpr unsigned DisconnectOpTimers = 0U;
    static constexpr unsigned SubscribeOpTimers = 1U;    
    static constexpr unsigned UnsubscribeOpTimers = 1U;    
    static constexpr unsigned RecvOpsLimit = MaxQos < 2 ? 1U : (ReceiveMaxLimit == 0U ? 0U : ReceiveMaxLimit + 1U);
    static constexpr unsigned RecvOpTimers = 1U;
    static constexpr unsigned SendOpsLimit = SendMaxLimit == 0U ? 0U : SendMaxLimit + 1U;
    static constexpr unsigned SendOpTimers = 1U;    
    static constexpr bool HasOpsLimit = 
        (ConnectOpsLimit > 0U) && 
        (KeepAliveOpsLimit > 0U) &&
        (DisconnectOpsLimit > 0U) &&
        (SubscribeOpsLimit > 0U) &&
        (UnsubscribeOpsLimit > 0U) &&
        (RecvOpsLimit > 0U) &&
        (SendOpsLimit > 0U);
    static constexpr unsigned MaxTimersLimit = 
        (ConnectOpsLimit * ConnectOpTimers) + 
        (KeepAliveOpsLimit * KeepAliveOpTimers) + 
        (DisconnectOpsLimit * DisconnectOpTimers) + 
        (SubscribeOpsLimit * SubscribeOpTimers) +
        (UnsubscribeOpsLimit * UnsubscribeOpTimers) + 
        (RecvOpsLimit * RecvOpTimers) + 
        (SendOpsLimit * SendOpTimers);
    static constexpr unsigned TimersLimit = HasOpsLimit ? MaxTimersLimit : 0U;

    static const unsigned MaxOpsLimit = 
        ConnectOpsLimit + 
        KeepAliveOpsLimit + 
        DisconnectOpsLimit + 
        SubscribeOpsLimit + 
        UnsubscribeOpsLimit + 
        RecvOpsLimit + 
        SendOpsLimit;

    static const unsigned OpsLimit = HasOpsLimit ? MaxOpsLimit : 0U;

    static const unsigned PacketIdsLimitSumTmp = 
        SubscribeOpsLimit + 
        UnsubscribeOpsLimit + 
        SendOpsLimit;    

    static const unsigned PacketIdsLimit = HasDynMemAlloc ? 0U : PacketIdsLimitSumTmp;

    static_assert(HasDynMemAlloc || (TimersLimit > 0U));
    static_assert(HasDynMemAlloc || (ConnectOpsLimit > 0U));
    static_assert(HasDynMemAlloc || (KeepAliveOpsLimit > 0U));
    static_assert(HasDynMemAlloc || (RecvOpsLimit > 0U));
    static_assert(HasDynMemAlloc || (SendOpsLimit > 0U));
    static_assert(HasDynMemAlloc || (OpsLimit > 0U));
    static_assert(HasDynMemAlloc || (PacketIdsLimit > 0U));
};

} // namespace cc_mqtt311_client

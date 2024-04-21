//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/DisconnectOp.h"
#include "ClientImpl.h"

#include "comms/units.h"

namespace cc_mqtt311_client
{

namespace op
{

DisconnectOp::DisconnectOp(ClientImpl& client) : 
    Base(client)
{
}    

CC_Mqtt311ErrorCode DisconnectOp::send()
{
    client().allowNextPrepare();

    auto guard = client().apiEnter();
    auto& clientObj = client();
    clientObj.sendMessage(m_disconnectMsg);
    opComplete(); // No members access after this point, the op will be deleted
    clientObj.brokerDisconnected();
    return CC_Mqtt311ErrorCode_Success;
}

CC_Mqtt311ErrorCode DisconnectOp::cancel()
{
    client().allowNextPrepare();
    opComplete();
    return CC_Mqtt311ErrorCode_Success;
}

Op::Type DisconnectOp::typeImpl() const
{
    return Type_Disconnect;
}


} // namespace op

} // namespace cc_mqtt311_client

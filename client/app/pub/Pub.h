//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "AppClient.h"
#include "ProgramOptions.h"

#include <boost/asio.hpp>

namespace cc_mqtt311_client_app
{

class Pub : public AppClient
{
    using Base = AppClient;
public:
    Pub(boost::asio::io_context& io, int& result);

protected:
    virtual void brokerConnectedImpl() override;    
private:
    void publishCompleteInternal(CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);

    static void publishCompleteCb(void* data, CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status);
};

} // namespace cc_mqtt311_client_app

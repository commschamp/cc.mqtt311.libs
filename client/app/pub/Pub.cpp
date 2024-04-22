//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Pub.h"

#include <iostream>

namespace cc_mqtt311_client_app
{

namespace 
{

Pub* asThis(void* data)
{
    return reinterpret_cast<Pub*>(data);
}

} // namespace 
    

Pub::Pub(boost::asio::io_context& io, int& result) : 
    Base(io, result)
{
    opts().addCommon();
    opts().addNetwork();
    opts().addConnect();
    opts().addPublish();
}    

void Pub::brokerConnectedImpl()
{
    auto topic = opts().pubTopic();
    auto data = parseBinaryData(opts().pubMessage());

    auto config = CC_Mqtt311PublishConfig();
    ::cc_mqtt311_client_publish_init_config(&config);
    config.m_topic = topic.c_str();
    if (!data.empty()) {
        config.m_data = &data[0];
    }
    config.m_dataLen = static_cast<decltype(config.m_dataLen)>(data.size());
    config.m_qos = static_cast<decltype(config.m_qos)>(opts().pubQos());
    config.m_retain = opts().pubRetain();

    auto ec = ::cc_mqtt311_client_publish(client(), &config, &Pub::publishCompleteCb, this);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        logError() << "Failed to send PUBLISH message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }    
}

void Pub::publishCompleteInternal([[maybe_unused]] CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status)
{
    if (status != CC_Mqtt311AsyncOpStatus_Complete) {
        logError() << "Publish failed: " << toString(status) << std::endl;
        doTerminate();
        return;
    }

    std::cout << "Publish successful" << std::endl;
    doTerminate(0);
}

void Pub::publishCompleteCb(void* data, CC_Mqtt311PublishHandle handle, CC_Mqtt311AsyncOpStatus status)
{
    asThis(data)->publishCompleteInternal(handle, status);
}


} // namespace cc_mqtt311_client_app

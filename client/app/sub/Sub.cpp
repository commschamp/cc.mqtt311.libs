//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Sublic
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Sub.h"

#include <iostream>

namespace cc_mqtt311_client_app
{

namespace 
{

Sub* asThis(void* data)
{
    return reinterpret_cast<Sub*>(data);
}

} // namespace 
    

Sub::Sub(boost::asio::io_context& io, int& result) : 
    Base(io, result)
{
    opts().addCommon();
    opts().addNetwork();
    opts().addConnect();
    opts().addSubscribe();
}    

void Sub::brokerConnectedImpl()
{
    auto topics = opts().subTopics();
    auto qoses = opts().subQoses();

    auto ec = CC_Mqtt311ErrorCode_InternalError;
    auto* subscribe = ::cc_mqtt311_client_subscribe_prepare(client(), &ec);
    if (subscribe == nullptr) {
        logError() << "Failed to allocate subscribe message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    for (auto idx = 0U; idx < topics.size(); ++idx) {
        auto topicConfig = CC_Mqtt311SubscribeTopicConfig();
        ::cc_mqtt311_client_subscribe_init_config_topic(&topicConfig);        
        topicConfig.m_topic = topics[idx].c_str();

        if (idx < qoses.size()) {
            topicConfig.m_maxQos = static_cast<decltype(topicConfig.m_maxQos)>(qoses[idx]);
        }

        ec = ::cc_mqtt311_client_subscribe_config_topic(subscribe, &topicConfig);
        if (ec != CC_Mqtt311ErrorCode_Success) {
            logError() << "Failed to configure topic \"" << topics[idx] << "\": " << toString(ec) << std::endl;
            doTerminate();
            return;
        }        
    }

    ec = ::cc_mqtt311_client_subscribe_send(subscribe, &Sub::subscribeCompleteCb, this);
    if (ec != CC_Mqtt311ErrorCode_Success) {
        logError() << "Failed to send SUBSCRIBE message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }        
}

void Sub::messageReceivedImpl(const CC_Mqtt311MessageInfo* info)
{
    assert(info != nullptr);
    assert(info->m_topic != nullptr);
    if (info->m_retained && opts().subNoRetained()) {
        return;
    }

    if (opts().verbose()) {
        print(*info);
    }   
    else {
        std::cout << info->m_topic << ": " << toString(info->m_data, info->m_dataLen, opts().subBinary()) << std::endl;
    }
}

void Sub::subscribeCompleteInternal(
    [[maybe_unused]] CC_Mqtt311SubscribeHandle handle, 
    CC_Mqtt311AsyncOpStatus status, 
    const CC_Mqtt311SubscribeResponse* response)
{
    if (status != CC_Mqtt311AsyncOpStatus_Complete) {
        logError() << "Subscribe failed: " << toString(status) << std::endl;
        doTerminate();
        return;
    }

    assert(response != nullptr);
    for (auto idx = 0U; idx < response->m_returnCodesCount; ++idx) {
        auto returnCode = response->m_returnCodes[idx];
        if (returnCode > CC_Mqtt311SubscribeReturnCode_SuccessQos2) {
            auto topics = opts().subTopics();
            assert(idx < topics.size());
            std::cerr << "Failed to subscribe to topic \"" << topics[idx] << "\" with return code: " << toString(returnCode) << std::endl;
        }
    }

    if (opts().verbose()) {
        print(*response);
    }    

    // Listening to the messages    
}

void Sub::subscribeCompleteCb(void* data, CC_Mqtt311SubscribeHandle handle, CC_Mqtt311AsyncOpStatus status, const CC_Mqtt311SubscribeResponse* response)
{
    asThis(data)->subscribeCompleteInternal(handle, status, response);
}


} // namespace cc_mqtt311_client_app

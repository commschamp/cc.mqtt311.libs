//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "cc_mqtt311/Message.h"
#include "cc_mqtt311/frame/Frame.h"
#include "cc_mqtt311/input/AllMessages.h"
#include "comms/GenericHandler.h"

#include <cstdint>
#include <iterator>

class UnitTestMsgHandler;

using UnitTestMessage = cc_mqtt311::Message<
    comms::option::app::ReadIterator<const std::uint8_t*>,
    comms::option::app::WriteIterator<std::back_insert_iterator<std::vector<std::uint8_t> > >,
    comms::option::app::LengthInfoInterface,
    comms::option::app::IdInfoInterface,
    comms::option::app::Handler<UnitTestMsgHandler>
>;

CC_MQTT311_ALIASES_FOR_ALL_MESSAGES_DEFAULT_OPTIONS(UnitTest, Msg, UnitTestMessage)

using UnitTestsFrame = cc_mqtt311::frame::Frame<UnitTestMessage>;
using UniTestsMsgPtr = UnitTestsFrame::MsgPtr;

class UnitTestMsgHandler : public comms::GenericHandler<UnitTestMessage, cc_mqtt311::input::AllMessages<UnitTestMessage> >
{
protected:
    UnitTestMsgHandler() = default;
    ~UnitTestMsgHandler() noexcept = default;
};


//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"
#include "ObjListType.h"
#include "ProtocolDefs.h"

#include "cc_mqtt311_client/common.h"

#include <cstdint>

namespace cc_mqtt311_client
{

struct ClientState
{
    using PacketIdsList = ObjListType<std::uint16_t, ExtConfig::PacketIdsLimit>;

    static constexpr unsigned DefaultKeepAlive = 60;

    PacketIdsList m_allocatedPacketIds;
    std::uint16_t m_lastPacketId = 0U;
    bool m_initialized = false;
    bool m_firstConnect = true;
    bool m_networkDisconnected = false;
};

} // namespace cc_mqtt311_client

//
// Copyright 2024 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Session.h"

#include <array>
#include <cstdint>
#include <vector>

namespace cc_mqtt311_client_app
{

class TcpSession final : public Session
{
    using Base = Session;
public:
    static Ptr create(boost::asio::io_context& io, const ProgramOptions& opts);

protected:    
    virtual bool startImpl() override;
    virtual void sendDataImpl(const std::uint8_t* buf, std::size_t bufLen) override;

private:
    using Socket = boost::asio::ip::tcp::socket;
    using Timer = boost::asio::steady_timer;
    using InDataBuf = std::array<std::uint8_t, 4096>;
    using DataBuf = std::vector<std::uint8_t>;

    TcpSession(boost::asio::io_context& io, const ProgramOptions& opts) : 
        Base(io, opts),
        m_socket(io),
        m_readTimer(io)
    {
    }

    void doRead();

    Socket m_socket;
    Timer m_readTimer;
    InDataBuf m_inData;
    DataBuf m_buf;
};

} // namespace cc_mqtt311_client_app

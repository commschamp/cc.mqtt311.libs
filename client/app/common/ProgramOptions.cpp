//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ProgramOptions.h"

#include <iostream>

namespace po = boost::program_options;

namespace cc_mqtt311_client_app
{

void ProgramOptions::addCommon()
{
    po::options_description opts("Common Options");
    opts.add_options()
        ("help,h", "Display help message")
        ("verbose,v", "Verbose output")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addConnect()
{
    po::options_description opts("Connect Options");
    opts.add_options()
        ("client-id,i", po::value<std::string>()->default_value(std::string()), "Client ID.")
        ("username", po::value<std::string>()->default_value(std::string()), "Username.")
        ("password", po::value<std::string>()->default_value(std::string()), "Password, use \"\\x\" prefix to specify hexadecimal value of a single byte.")
        ("keep-alive", po::value<unsigned>()->default_value(60), "Keep alive period in seconds.")
        ("will-topic", po::value<std::string>()->default_value(std::string()), "Will topic.")
        ("will-msg", po::value<std::string>()->default_value(std::string()), 
            "Will message data, use \"\\x\" prefix to specify hexadecimal value of a single byte."
            "Applicable only if will-topic is set.")
        ("will-qos", po::value<unsigned>()->default_value(0U), "Will Message QoS: 0, 1, or 2")            
        ("will-retain", "Set \"retain\" flag on the will message.")            
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addNetwork(std::uint16_t port)
{
    po::options_description opts("Network Options");
    opts.add_options()
        ("broker,b", po::value<std::string>()->default_value("127.0.0.1"), "Broker address to connect to")
        ("port,p", po::value<std::uint16_t>()->default_value(port), "Network port")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addPublish()
{
    po::options_description opts("Publish Options");
    opts.add_options()
        ("pub-topic,t", po::value<std::string>()->default_value(std::string()), "Publish topic")
        ("pub-msg,m", po::value<std::string>()->default_value(std::string()), "Publish message, use \"\\x\" prefix to specify hexadecimal value of a single byte, "
            "such as \"\\x01\\xb9\\xaf\".")
        ("pub-qos,q", po::value<unsigned>()->default_value(0U), "Publish QoS: 0, 1, or 2")
        ("pub-retain", po::value<bool>()->default_value(false), "Retain the publish message")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addSubscribe()
{
    po::options_description opts("Subscribe Options");
    opts.add_options()
        ("sub-topic,t", po::value<StringsList>(), "Subscribe topic filter. Can be used multiple times.")
        ("sub-qos,q", po::value<UnsignedsList>(), "Subscribe max QoS: 0, 1, or 2. Defaults to 2. Can be used multiple times "
            "(for each topic filter correspondingly).")
        ("sub-no-retained", "Ignore retained messages")       
        ("sub-binary", "Force binary output of the received message data")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::printHelp()
{
    std::cout << m_desc << std::endl;
}

bool ProgramOptions::parseArgs(int argc, const char* argv[])
{
    po::store(po::parse_command_line(argc, argv, m_desc), m_vm);
    po::notify(m_vm);  

    return true;
}

bool ProgramOptions::helpRequested() const
{
    return m_vm.count("help") > 0U;
}

bool ProgramOptions::verbose() const
{
    return m_vm.count("verbose") > 0U;
}

ProgramOptions::ConnectionType ProgramOptions::connectionType() const
{
    // Hardcoded for now
    return ConnectionType_Tcp;
}

std::string ProgramOptions::clientId() const
{
    return m_vm["client-id"].as<std::string>();
}

std::string ProgramOptions::username() const
{
    return m_vm["username"].as<std::string>();
}

std::string ProgramOptions::password() const
{
    return m_vm["password"].as<std::string>();
}

unsigned ProgramOptions::keepAlive() const
{
    return m_vm["keep-alive"].as<unsigned>();
}

std::string ProgramOptions::willTopic() const
{
    return m_vm["will-topic"].as<std::string>();
}

std::string ProgramOptions::willMessage() const
{
    return m_vm["will-msg"].as<std::string>();
}

unsigned ProgramOptions::willQos() const
{
    return m_vm["will-qos"].as<unsigned>();
}

bool ProgramOptions::willRetain() const
{
    return m_vm.count("will-retain") > 0U;
}

std::string ProgramOptions::networkAddress() const
{
    return m_vm["broker"].as<std::string>();
}

std::uint16_t ProgramOptions::networkPort() const
{
    return m_vm["port"].as<std::uint16_t>();
}

std::string ProgramOptions::pubTopic() const
{
    return m_vm["pub-topic"].as<std::string>();
}

std::string ProgramOptions::pubMessage() const
{
    return m_vm["pub-msg"].as<std::string>();
}

unsigned ProgramOptions::pubQos() const
{
    return m_vm["pub-qos"].as<unsigned>();
}

bool ProgramOptions::pubRetain() const
{
    return m_vm["pub-retain"].as<bool>();
}

ProgramOptions::StringsList ProgramOptions::subTopics() const
{
    return stringListOpts("sub-topic");
}

ProgramOptions::UnsignedsList ProgramOptions::subQoses() const
{
    UnsignedsList result;
    auto* id = "sub-qos";
    if (m_vm.count(id) > 0) {
        result = m_vm[id].as<UnsignedsList>();
    }

    return result;    
}

bool ProgramOptions::subNoRetained() const
{
    return m_vm.count("sub-no-retained") > 0U;
}

bool ProgramOptions::subBinary() const
{
    return m_vm.count("sub-binary") > 0U;
}

ProgramOptions::StringsList ProgramOptions::stringListOpts(const std::string& name) const
{
    StringsList result;
    if (m_vm.count(name) > 0) {
        result = m_vm[name].as<StringsList>();
    }

    return result;    
}

} // namespace cc_mqtt311_client_app

#pragma once

#define ASIO_STANDALONE 
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS

#include <stdlib.h>
#include <list>
#include <string>
#include <cstdint>


#include "ZCPP.h"

#include "asio.hpp"

using asio::ip::tcp;
using asio::ip::udp;
using asio::ip::address;

class ZCPPOutput {
public:
    ZCPPOutput();
    ~ZCPPOutput();

    bool SendConfig();
    bool ReadConfig(std::string const& file);
    std::string GetIPAddress(){return _ipAddress;}

private:

    std::string _ipAddress;
    std::list<ZCPP_packet_t*> _extraConfig;
    std::list<ZCPP_packet_t*> _modelData;
    
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
    void sendConfigFile(udp::socket & socket, udp::endpoint const& remote_endpoint);
    bool readFile(std::string const& file);
};
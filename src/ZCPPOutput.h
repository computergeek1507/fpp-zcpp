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

    bool SendConfig(bool sendExtra = true);
    bool ReadConfig(std::string const& file);
    std::string GetIPAddress(){return _ipAddress;}
    bool SendData( unsigned char *data);
    unsigned int GetChannelCount(){return _channelCount;}
    unsigned int GetStartChannel(){return _startChannel;}
    void SetStartChannel(unsigned int startChannel){ _startChannel = startChannel;}

    bool IsOpen() {return _socket.is_open();}
    void Open() {_socket.open(udp::v4());}

private:

    unsigned int _startChannel;
    unsigned int _channelCount;
    std::string _ipAddress;
    std::list<ZCPP_packet_t*> _extraConfig;
    std::list<ZCPP_packet_t*> _modelData;

    asio::io_service _io_service;
    udp::socket _socket;
    udp::endpoint _remote_endpoint;

    uint8_t* _data = nullptr;
    ZCPP_packet_t _packet;
    uint8_t _sequenceNum = 0;

    void ExtractUsedChannelsFromModelData();
    void outputData( unsigned char *data);
    void sendConfigFile( bool sendExtra);
    bool readFile(std::string const& file);

    void replaceAll(std::string& str, const std::string& from, const std::string& to);
};
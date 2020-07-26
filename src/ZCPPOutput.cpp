
#include "ZCPPOutput.h"


#include <stdlib.h>
#include <list>
#include <string>
#include <cstdint>


#include <iostream> 
#include <istream>
#include <ostream>

#include <fstream>
#include <filesystem>

ZCPPOutput::ZCPPOutput() : _startChannel(1), _channelCount(0),_sequenceNum(0), _socket(_io_service)
{
	memset(&_packet, 0x00, sizeof(_packet));

	memcpy(_packet.Data.Header.token, ZCPP_token, sizeof(ZCPP_token));
	_packet.Data.Header.type = ZCPP_TYPE_DATA;
	_packet.Data.Header.protocolVersion = ZCPP_CURRENT_PROTOCOL_VERSION;
	_packet.Data.priority = 100;
}

ZCPPOutput::~ZCPPOutput()
{
	_socket.close();
}

bool ZCPPOutput::ReadConfig(std::string const& zcppFile)
{
	
	bool worked = readFile(zcppFile);
	if(!worked)
		return false;
	
	std::string ip = std::filesystem::path(zcppFile).filename().string();
	replaceAll(ip, ".zcpp", "");
	replaceAll(ip, "_", ".");
	_ipAddress = ip;
	
	_remote_endpoint = udp::endpoint(address::from_string(_ipAddress), ZCPP_PORT);
	
	return true;
}

bool ZCPPOutput::SendConfig( bool sendExtra)
{
	try
	{
		if(_ipAddress.empty())
			return false;
		
		if(!IsOpen())
			Open();

		sendConfigFile(sendExtra);
		return true;
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
	}
	
	return false;
}

bool ZCPPOutput::SendData( unsigned char *data)
{
	try
	{
		if(_ipAddress.empty())
			return false;

		if(!IsOpen())
			Open();

		outputData(data );
		return true;
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
	}
	
	return false;
}


bool ZCPPOutput::readFile(std::string const& file)
{
	try
	{
		if (!std::filesystem::exists(file))
		{
			std::cout << "ZCPP File Doesn't Exists. " << file << "\n";
			return false;
		}
		std::ifstream inFile;
		std::ifstream::pos_type size;

		inFile.open(file, std::ios::in | std::ios::binary);

		uint8_t tag[4];
		inFile.read((char*)tag, sizeof(tag));
		if (tag[0] != 'Z' || tag[1] != 'C' || tag[2] != 'P' || tag[3] != 'P') {
			std::cout << "ZCPP Model data file did not contain opening tag. " << file << "\n";
		}
		else {
			bool go = true;
			while (!inFile.eof() && go) {
				uint8_t type;
				inFile.read((char*)&type, sizeof(type));
				switch (type) {
				case 0x00:
				{
					uint8_t b1;
					uint8_t b2;
					inFile.read((char*)&b1, sizeof(b1));
					inFile.read((char*)&b2, sizeof(b2));
					uint16_t size = (b1 << 8) + b2;
					if (size == sizeof(ZCPP_packet_t)) {
						ZCPP_packet_t* modelPacket = (ZCPP_packet_t*)malloc(sizeof(ZCPP_packet_t));
						inFile.read((char*)modelPacket, sizeof(ZCPP_packet_t));
						_modelData.push_back(modelPacket);
					}
					else {
						std::cout << "ZCPP Model data file unrecognized model data size. " << file << "\n";
						//inFile.seekg(0, inFile.end);
						go = false;
					}
				}
				break;
				case 0x01:
				{
					uint8_t b1;
					uint8_t b2;
					inFile.read((char*)&b1, sizeof(b1));
					inFile.read((char*)&b2, sizeof(b2));
					uint16_t size = (b1 << 8) + b2;
					if (size == sizeof(ZCPP_packet_t)) {
						ZCPP_packet_t* descPacket = (ZCPP_packet_t*)malloc(sizeof(ZCPP_packet_t));
						inFile.read((char*)descPacket, sizeof(ZCPP_packet_t));
						_extraConfig.push_back(descPacket);
					}
					else {
						std::cout << "ZCPP Model data file unrecognized extra config size. " << file << "\n";
						//inFile.seekg(0, inFile.end);
						go = false;
					}
				}
				break;
				case 0xFF:
					//inFile.seekg(0, inFile.end);
					go = false;
					break;
				default:
					std::cout << "ZCPP Model data file unrecognized type " << type << " file " << file << "\n";
					break;
				}
			}
			std::cout << "ZCPP Model data file loaded." << file << "\n";
		}
		inFile.close();

		ExtractUsedChannelsFromModelData();
		return true;
	}
	catch (std::exception ex)
	{
		std::cout << ex.what();
		return false;
	}
	return false;
}

void ZCPPOutput::sendConfigFile(bool sendExtra)
{
	for (auto it = _modelData.begin(); it != _modelData.end(); ++it) {
		auto it2 = it;
		++it2;
		if (it == _modelData.begin()) {
			(*it)->Configuration.flags |= ZCPP_CONFIG_FLAG_FIRST;
		}
		if (it2 == _modelData.end()) {
			(*it)->Configuration.flags |= ZCPP_CONFIG_FLAG_LAST;
		}

		if (sendExtra) {
			(*it)->Configuration.flags |= ZCPP_CONFIG_FLAG_EXTRA_DATA_WILL_FOLLOW;
		}
		asio::error_code err;
		auto sent = _socket.send_to(asio::buffer(*it, ZCPP_GetPacketActualSize(**it)), _remote_endpoint, 0, err);

		std::cout << "Sent Configuration Payload --- " << sent << "\n";
	}
	if (sendExtra) {
		for (auto it = _extraConfig.begin(); it != _extraConfig.end(); ++it) {
			auto it2 = it;
			++it2;
			if (it == _extraConfig.begin()) {
				(*it)->ExtraData.flags |= ZCPP_CONFIG_FLAG_FIRST;
			}
			if (it2 == _extraConfig.end()) {
				(*it)->ExtraData.flags |= ZCPP_CONFIG_FLAG_LAST;
			}
			asio::error_code err;
			auto sent = _socket.send_to(asio::buffer(*it, ZCPP_GetPacketActualSize(**it)), _remote_endpoint, 0, err);

			std::cout << "Sent ExtraData Payload --- " << sent << "\n";
		}
	}
}

void ZCPPOutput::outputData( unsigned char *data)
{
	//if (_channelCount == 0) {
	//	_data = nullptr;
	//}
	//else {
	//	_data = (unsigned char*)malloc(_channelCount);
	//	if (_data != nullptr) memset(_data, &data, _channelCount);
	//}
	_sequenceNum = _sequenceNum == 255 ? 0 : _sequenceNum + 1;

	long i = 0;
	while (i < _channelCount) {
		_packet.Data.sequenceNumber = _sequenceNum;
		uint32_t startAddress = i;
		_packet.Data.frameAddress = ntohl(startAddress);
		uint16_t packetlen = _channelCount - i > sizeof(ZCPP_packet_t) - ZCPP_DATA_HEADER_SIZE ? sizeof(ZCPP_packet_t) - ZCPP_DATA_HEADER_SIZE : _channelCount - i;
		_packet.Data.flags =  0x00 +
			(i + packetlen == _channelCount ? ZCPP_DATA_FLAG_LAST : 0x00) +
			(i == 0 ? ZCPP_DATA_FLAG_FIRST : 0x00);
		_packet.Data.packetDataLength = ntohs(packetlen);
		memcpy(_packet.Data.data, data + i + _startChannel, packetlen);

		asio::error_code err;
		auto sent = _socket.send_to(asio::buffer(&_packet, ZCPP_GetPacketActualSize(_packet)), _remote_endpoint, 0, err);
		std::cout << "Sent Data Payload --- " << sent << "\n";
		i += packetlen;
	}
}

void ZCPPOutput::ExtractUsedChannelsFromModelData() 
{
	_channelCount = 1;

	for (const auto& it : _modelData) {
		int ports = it->Configuration.ports;
		if (ports > ZCPP_CONFIG_MAX_PORT_PER_PACKET) {
			std::cout << "ZCPP file corrupt. Abandoning read.\n";
			_channelCount = 1;
			return;
		}
		ZCPP_PortConfig* port = it->Configuration.PortConfig;
		for (int i = 0; i < ports; i++) {
			long start = htonl(port->startChannel);
			long len = htonl(port->channels);
			if (start + len - 1 > _channelCount) {
				_channelCount = start + len;
			}
			port++;
		}
		std::cout << "    End of config packet ... channels " << _channelCount << "\n";
	}
}

void ZCPPOutput::replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}
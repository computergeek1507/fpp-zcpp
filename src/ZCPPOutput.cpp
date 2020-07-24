
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

ZCPPOutput::ZCPPOutput()
{


ZCPPOutput::~ZCPPOutput()
{

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
	
	return true;
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
		return true;
	}
	catch (std::exception ex)
	{
		std::cout << ex.what();
		return false;
	}
	return false;
}

bool ZCPPOutput::SendConfig()
{
	try
	{
		if(_ipAddress.empty())
			return false;
		
		asio::io_context io_context;

		asio::io_service io_service;
		udp::socket socket(io_service);
		udp::endpoint remote_endpoint = udp::endpoint(address::from_string(_ipAddress), ZCPP_PORT);
		socket.open(udp::v4());

		sendConfigFiles(socket, remote_endpoint);
		return true;
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
	}
	return false;
}

void ZCPPOutput::sendConfigFile(udp::socket & socket, udp::endpoint const& remote_endpoint)
{
	bool sendExtra = true;
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
		auto sent = socket.send_to(asio::buffer(*it, ZCPP_GetPacketActualSize(**it)), remote_endpoint, 0, err);

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
			auto sent = socket.send_to(asio::buffer(*it, ZCPP_GetPacketActualSize(**it)), remote_endpoint, 0, err);

			std::cout << "Sent ExtraData Payload --- " << sent << "\n";
		}
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
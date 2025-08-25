#pragma once

#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "flatbuffers/flatbuffer_builder.h"

#define HANDLER_CATEGORY "Handler"

typedef struct SocketData SocketData;
typedef struct PacketQueue PacketQueue;

struct PacketQueue 
{
	std::shared_ptr<flatbuffers::FlatBufferBuilder> builder;
	std::shared_ptr<SocketData>                     data;
};

namespace PacketHandler
{
	void HandleClientPacket(std::shared_ptr<SocketData>, const unsigned char* data, int length);

	std::shared_ptr<flatbuffers::FlatBufferBuilder> CreateS00Packet();
	std::shared_ptr<flatbuffers::FlatBufferBuilder> CreateS01Packet    (std::string uid);
	void                                            SendPacket         (std::shared_ptr<SocketData> socket, std::shared_ptr<flatbuffers::FlatBufferBuilder> builder);
	void                                            InitPacketProcesser();
}

#endif
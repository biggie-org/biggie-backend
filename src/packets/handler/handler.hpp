#pragma once

#ifndef HANDLER_HPP
#define HANDLER_HPP

#define HANDLER_CATEGORY "Handler"

typedef SocketData SocketData;

namespace PacketHandler
{
	void HandleClientPacket(const SocketData* socket, const unsigned char* data, int length);
}

#endif
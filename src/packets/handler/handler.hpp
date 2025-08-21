#pragma once

#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <stdint.h>

#define HANDLER_CATEGORY "Handler"

typedef struct SocketData SocketData;

namespace PacketHandler
{
	void HandleClientPacket(SocketData* socket, unsigned char* data, int length);
}

#endif
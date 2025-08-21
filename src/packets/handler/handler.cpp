#include "../client/Packets_generated.h"
#include "../../socket/server.hpp"
#include "handler.hpp"
#include "../../utils/logger/logger.hpp"

#include <sstream>

namespace PacketHandler
{
	std::string GetPacketInfoFormatted(const void* packet, int8_t id)
	{
		switch (id)
		{
			case 0:
			{
				std::stringstream info;

				int8_t test = ((C00PacketKeepAlive*) packet)->test();

				info << "C00: {\n";
				info << "|    Test: " << static_cast<unsigned int>(test) << "\n";
				info << "}\n";

				return info.str();
			}
		}

		return nullptr;
	}

	bool ValidatePacket(unsigned char* data, const int length)
	{
		flatbuffers::Verifier verifier(data, length);

		if (!verifier.VerifyBuffer<Packet>(nullptr))
		{
			Logger::Error(HANDLER_CATEGORY, "Invalid packet sent...");
			
			return false;
		}

		return true;
	}
	
	void HandleC00Packet(const Packet* packet)
	{
		const C00PacketKeepAlive* c00 = packet->packets_as_C00PacketKeepAlive();
		
		Logger::Alert(HANDLER_CATEGORY, "C00 Packet Info:\n" + GetPacketInfoFormatted(c00, 0));
	}
	
	void HandleClientPacket(SocketData* socket, unsigned char* data, int length)
	{
		if (!ValidatePacket(data, length))
		{
			Socket::DisconnectClient(socket);

			return;
		}
		
		auto packet = GetPacket(data);
		int8_t id = packet->id();

		Logger::Alert(HANDLER_CATEGORY, "Received a packet with id: '" + std::to_string(id) + "'...");
		
		switch (id)
		{
			case 0: // C00PacketKeepAlive
			{
				HandleC00Packet(packet);

				break;
			}

			default:
			{
				Logger::Error(HANDLER_CATEGORY, "Invalid packet sent...");

				Socket::DisconnectClient(socket);
			}
		}
	}
}

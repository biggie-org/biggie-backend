#include "../server/Packets_generated.h"
#include "../client/Packets_generated.h"

#include "../../socket/server.hpp"
#include "handler.hpp"
#include "../../utils/logger/logger.hpp"
#include "flatbuffers/flatbuffer_builder.h"

#include <sstream>
#include <memory>
#include <deque>
#include <string>

namespace PacketHandler
{
	std::deque<PacketQueue> packets_queue;
	std::condition_variable noti;
	std::mutex queue_mutex;

	std::string GetPacketInfoFormatted(const void* packet, int8_t id)
	{
		switch (id)
		{
			case 0:
			{
				std::stringstream info;

				info << "C00:\n{\n";
				info << "}";

				return info.str();
			}

			case 1:
			{
				std::stringstream info;

				const char* uname = reinterpret_cast<const CPackets::C01PacketAuthenticate*>(packet)->uname()->c_str();
				const char* upass = reinterpret_cast<const CPackets::C01PacketAuthenticate*>(packet)->upass()->c_str();

				info << "C01:\n{\n";
				info << "|    Name: \"" << uname << "\"\n";
				info << "|    Pass: \"" << upass << "\"\n";
				info << "}";

				return info.str();
			}

			default:
			{
				return "";
			}
		}
	}

	void SendPacket(std::shared_ptr<SocketData> socket, std::shared_ptr<flatbuffers::FlatBufferBuilder> builder)
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			
			PacketQueue packet{ builder, socket };
			packets_queue.push_back(packet);
		}

		noti.notify_one();
	}

	void ProcessPacketQueue()
	{
		while (true)
		{
			std::shared_ptr<PacketQueue> packet;
			
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				noti.wait(lock, []{ return !packets_queue.empty(); });
	
				packet = std::make_shared<PacketQueue>(std::move(packets_queue.front()));
				packets_queue.pop_front();
			}

			uint32_t size = packet->builder->GetSize();
			uint32_t size_be = htonl(packet->builder->GetSize());
			void* buff = packet->builder->GetBufferPointer();

			std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(sizeof(uint32_t) + size);
		
			std::memcpy(buffer->data(), &size_be, sizeof(uint32_t));
			std::memcpy(buffer->data() + sizeof(uint32_t), buff, size);

			auto socket = packet->data;

			if (!socket->socket.is_open())
			{
    				continue;
			}
			
			boost::asio::async_write(packet->data->socket, boost::asio::buffer(*buffer), [buffer, socket](const boost::system::error_code& ec, std::size_t bytes_transferred)
			{
				if (ec) 
				{
					Logger::Error(HANDLER_CATEGORY, "Failed to send packet: \"" + ec.message() + "\"...");
				}

				Logger::Success(HANDLER_CATEGORY, "Successfully sent packet...");
			});
		}
	}

	void InitPacketProcesser()
	{
		std::thread(ProcessPacketQueue).detach();
	}

	bool ValidateSubPacket(const unsigned char* data, CPackets::Packets type, const int length)
	{
		flatbuffers::Verifier verifier(data, length);

		if (type == CPackets::Packets::Packets_C00PacketKeepAlive) 
		{
			if (!verifier.VerifyBuffer<CPackets::C00PacketKeepAlive>(nullptr))
			{
				return false;
			}

			return true;
		} 
		else if (type == CPackets::Packets::Packets_C01PacketAuthenticate) 
		{
			if (!verifier.VerifyBuffer<CPackets::C01PacketAuthenticate>(nullptr))
			{
				return false;
			}

			return true;
		}

		Logger::Error("Handler", "Verifying a unknown packet...");
		return false;
	}
	
	bool ValidatePacket(const unsigned char* data, const int length)
	{
		flatbuffers::Verifier verifier(data, length);

		if (!verifier.VerifyBuffer<CPackets::CPacket>())
		{
			Logger::Error(HANDLER_CATEGORY, "Invalid packet sent...");
			
			return false;
		}

		return true;
	}

	std::shared_ptr<flatbuffers::FlatBufferBuilder> CreateS00Packet()
	{
		std::shared_ptr<flatbuffers::FlatBufferBuilder> builder = std::make_shared<flatbuffers::FlatBufferBuilder>(1024);

		flatbuffers::Offset<SPackets::S00PacketKeepAlive> S00 = SPackets::CreateS00PacketKeepAlive(*builder);
		flatbuffers::Offset<SPackets::SPacket> packet = SPackets::CreateSPacket(*builder, SPackets::Packets::Packets_S00PacketKeepAlive, S00.Union());

		builder->Finish(packet);

		return builder;
	}

	std::shared_ptr<flatbuffers::FlatBufferBuilder> CreateS01Packet(std::string uid)
	{
		std::shared_ptr<flatbuffers::FlatBufferBuilder> builder = std::make_shared<flatbuffers::FlatBufferBuilder>(1024);

		flatbuffers::Offset<flatbuffers::String> uid_ = builder->CreateString(uid);

		flatbuffers::Offset<SPackets::S01PacketAuthenticate> S01 = SPackets::CreateS01PacketAuthenticate(*builder, uid_);
		flatbuffers::Offset<SPackets::SPacket> packet = SPackets::CreateSPacket(*builder, SPackets::Packets::Packets_S01PacketAuthenticate, S01.Union());

		builder->Finish(packet);

		return builder;
	}
	
	void HandleC00Packet(const CPackets::CPacket* packet, std::shared_ptr<SocketData> socket)
	{
		const CPackets::C00PacketKeepAlive* c00 = packet->packets_as_C00PacketKeepAlive();

		if (c00 == nullptr)
		{
			ServerInst::DisconnectClient(socket);
			Logger::Error(HANDLER_CATEGORY, "Invalid packet with \"C00\" id sent...");

			return;
		}
		
		Logger::Alert(HANDLER_CATEGORY, "C00 Packet Info:\n" + GetPacketInfoFormatted(c00, 0));
		socket->last_c00 = std::chrono::steady_clock::now();
	}

	void HandleC01Packet(const CPackets::CPacket* packet, std::shared_ptr<SocketData> socket)
	{
		if (!ServerInst::ContainsClient(socket))
		{
			return;
		}
		
		const CPackets::C01PacketAuthenticate* c01 = packet->packets_as_C01PacketAuthenticate();

		if (c01 == nullptr)
		{
			ServerInst::DisconnectClient(socket);
			Logger::Error(HANDLER_CATEGORY, "Invalid packet with \"C01\" id sent...");

			return;
		}

		Logger::Alert(HANDLER_CATEGORY, "C01 Packet Info:\n" + GetPacketInfoFormatted(c01, 1));

		const char* uname = c01->uname()->c_str();
		const char* upass = c01->upass()->c_str();

		std::string _uname(uname);
		std::string _upass(upass);

		Auth::SendAuthenticateAsync(socket, _uname, _upass);
	}
	
	void HandleClientPacket(std::shared_ptr<SocketData> socket, const unsigned char* data, const int length)
	{
		if (!ValidatePacket(data, length))
		{
			ServerInst::DisconnectClient(socket);
			
			return;
		}
		
		const CPackets::CPacket* packet = CPackets::GetCPacket(data);

		Logger::Alert("Handler", "Handling packet with type: " + std::to_string(packet->packets_type()));
		
		if (packet->packets_type() != CPackets::Packets_C01PacketAuthenticate && !socket->is_authed)
		{
			ServerInst::DisconnectClient(socket);
			
			return;
		}
		
		if (packet->packets_type() == CPackets::Packets_C00PacketKeepAlive)
		{
			HandleC00Packet(packet, socket);
		}
		else if (packet->packets_type() == CPackets::Packets_C01PacketAuthenticate)
		{
			HandleC01Packet(packet, socket);
		}
		else
		{
			Logger::Error(HANDLER_CATEGORY, "Invalid packet sent...");
		}
	}
}

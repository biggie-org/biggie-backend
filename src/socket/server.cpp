#include "../packets/handler/handler.hpp"
#include "server.hpp"

#include <boost/asio.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <queue>

#include "../../libs/hiredis/adapters/libuv.h"
#include "../../libs/hiredis/async.h"

#include "../utils/logger/logger.hpp"


using boost::asio::ip::tcp;
using namespace boost::asio;
using namespace std::chrono_literals;

namespace Auth 
{
	redisAsyncContext* connection = nullptr;
	uv_loop_t* loop = nullptr;
	uv_async_t redis_async;

	std::mutex commands_mutex;
	std::queue<CommandQueue*> commands_queue;

	void HandleAuthenticateCallback(redisAsyncContext* context, void* response, void* data)
	{
		CommandQueue* cmd = reinterpret_cast<CommandQueue*>(data);
		redisReply* reply = reinterpret_cast<redisReply*>(response);

		if (response == nullptr)
		{
			delete cmd;
			
			return;
		}

		if (data == nullptr)
		{
			delete cmd;
			
			return;
		}
		
		if (cmd->data->is_authed)
		{
			delete cmd;

			Logger::Error(AUTH_CATEGORY, "User failed to authenticate...");
			
			return;
		}
		
		if (reply->type != REDIS_REPLY_ARRAY)
		{
			delete cmd;

			Logger::Error(AUTH_CATEGORY, "User failed to authenticate...");

			return;
		}

		std::string upass = "";

		for (int i = 0; i + 1 < reply->elements; i += 2)
		{
			std::string field = reply->element[i]->str;
			std::string value = reply->element[i + 1]->str;
			
			if (field == "uname")
			{
				cmd->data->uname = value;
			}
			else if (field == "uid")
			{
				cmd->data->uid = std::stoi(value);
			}
			else if (field == "upass")
			{
				upass = value;
			}
		}

		if (upass != cmd->upass)
		{
			delete cmd;

			Logger::Error(AUTH_CATEGORY, "User failed to authenticate...");

			return;
		}

		Logger::Success(AUTH_CATEGORY, "User successfully authenticated...");
		cmd->data->is_authed = true;

		std::shared_ptr<flatbuffers::FlatBufferBuilder> packet = PacketHandler::CreateS01Packet(std::to_string(cmd->data->uid));
		PacketHandler::SendPacket(cmd->data, std::move(packet));
		
		delete cmd;
	}
	
	void AuthenticateUser(CommandQueue* queue)
	{
		const char* ARGV[] = { "HGETALL", queue->uname.c_str() };
		redisAsyncCommandArgv(connection, HandleAuthenticateCallback, queue, 2, ARGV, nullptr);
	}

	void HandleCallback(uv_async_t* handle) 
	{
		std::lock_guard<std::mutex> lock(commands_mutex);

		while (!commands_queue.empty()) 
		{
			CommandQueue* cmd = commands_queue.front();
			commands_queue.pop();
			
			AuthenticateUser(cmd);
		}
	}

	void InitDatabase() 
	{
		loop = uv_default_loop();
		connection = redisAsyncConnect("127.0.0.1", 3040);
		
		if (!connection || connection->err) 
		{
			std::cerr << "Failed to connect to Redis" << std::endl;
			return;
		}

		redisLibuvAttach(connection, loop);
		uv_async_init(loop, &redis_async, HandleCallback);

		std::thread([&]{ uv_run(loop, UV_RUN_DEFAULT); }).detach();
	}

	redisAsyncContext* GetConnection()
	{
		return connection;
	}

	void SendAuthenticateAsync(std::shared_ptr<SocketData> data, const std::string uname, const std::string upass) 
	{
		std::lock_guard<std::mutex> lock(commands_mutex);

		CommandQueue* queue = new CommandQueue{{}, {}, {}};
		queue->data = data;
		queue->uname = uname;
		queue->upass = upass;

		commands_queue.push(queue);
		uv_async_send(&redis_async);
	}
}

class Session : public std::enable_shared_from_this<Session> 
{
	std::shared_ptr<SocketData> data;
	std::vector<char> buffer_;
	std::vector<char> buffer;

	public:
		Session(std::shared_ptr<SocketData> data) : buffer_(1024) 
		{
			this->data = data;
		}

		void Start() 
		{
			Read();
		}

	private:
		void Read() 
		{
			auto self(shared_from_this());
			data->socket.async_read_some(boost::asio::buffer(buffer_),
			[this, self](boost::system::error_code ec, std::size_t length) 
			{
				if (!ec) 
				{
					buffer.insert(buffer.end(), buffer_.begin(), buffer_.begin() + length);

					while (buffer.size() >= 4) 
					{
						uint32_t packet_size = *reinterpret_cast<uint32_t*>(buffer.data());
						
						if (buffer.size() < 4 + packet_size) 
						{
							break;
						}

						const unsigned char* payload = reinterpret_cast<const unsigned char*>(buffer.data() + 4);
						PacketHandler::HandleClientPacket(data, payload, packet_size);

						buffer.erase(buffer.begin(), buffer.begin() + 4 + packet_size);
					}

					Read();
				}
				else 
				{
					ServerInst::DisconnectClient(data);
				}
			}
			);
		}
};

class Server {
	boost::asio::io_context& io_;
	tcp::acceptor acceptor_;

	bool running;
	
	public:
		Server(boost::asio::io_context& io, int port) : io_(io), acceptor_(io, tcp::endpoint(tcp::v4(), port)) 
		{
			Logger::Success(SOCKET_CATEGORY, "Started listening on port: " + std::to_string(port));
			running = true;

			Accept();
		}

		void Stop()
		{
			running = false;
		}

		bool IsRunning()
		{
			return running;
		}

	private:
		void OnClientConnection(boost::system::error_code ec, tcp::socket& socket)
		{
			if (!ec) 
			{
				boost::asio::ip::address addr = socket.remote_endpoint().address();
				
				Logger::Alert(SOCKET_CATEGORY, "Client connected: \"" + addr.to_string() + "\"...");
				std::shared_ptr<SocketData> data = std::make_shared<SocketData>(std::move(socket), nullptr);
				data->last_c00 = std::chrono::steady_clock::now();

				std::make_shared<Session>(data)->Start();
				
				ServerInst::GetClients().push_back(data);
			}

			Accept();
		}
	
		void Accept() 
		{
			acceptor_.async_accept(
				[this](boost::system::error_code ec, tcp::socket socket) 
				{
					OnClientConnection(ec, socket);
				}
			);
		}
};

namespace ServerInst
{
	std::vector<std::shared_ptr<SocketData>> clients;
	std::mutex clients_mutex;

	std::vector<std::shared_ptr<SocketData>>& GetClients()
	{
		return clients;
	}

	bool ContainsClient(std::shared_ptr<SocketData> data)
	{
		for (std::shared_ptr<SocketData> dt : GetClients())
		{
			if (dt == data)
			{
				return true;
			}
		}

		return false;
	}

	void DisconnectClient(std::shared_ptr<SocketData> data)
	{
		std::lock_guard lock(clients_mutex);

		auto it = std::find(clients.begin(), clients.end(), data);
		
		if (it != clients.end())
		{
			if ((*it)->socket.is_open())
			{
				boost::asio::ip::address addr = (*it)->socket.remote_endpoint().address();
				Logger::Alert(SOCKET_CATEGORY, "Client disconnected: \"" + addr.to_string() + "\"...");
				(*it)->socket.close();
			}

			clients.erase(it);
		}
	}
	
	void RunKeepAliveTimer(std::shared_ptr<boost::asio::steady_timer> timer)
	{
		std::chrono::time_point now = std::chrono::steady_clock::now();
		
		std::vector<std::shared_ptr<SocketData>> expired;

		{
			std::lock_guard lock(clients_mutex);

			for (std::shared_ptr<SocketData> data : GetClients())
			{
				long long diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - data->last_c00).count();
				
				if (diff > 15000)
				{
					Logger::Alert(SOCKET_CATEGORY, "Client has expired keep alive time...");
					expired.push_back(data);
				}
				else
				{
					PacketHandler::SendPacket(data, PacketHandler::CreateS00Packet());
				}
			}
		}

		for (std::shared_ptr<SocketData> data : expired)
		{
			DisconnectClient(data);
		}
		
		timer->expires_after(10000ms);
		
		timer->async_wait([timer](const boost::system::error_code ec)
		{
			if (!ec)
			{
				RunKeepAliveTimer(timer);
			}
		});
	}

	void InitKeepAliveTimer(boost::asio::io_context& io)
	{
		std::shared_ptr<boost::asio::steady_timer> timer = std::make_shared<boost::asio::steady_timer>(io, 10000ms);
		
		timer->async_wait([timer](const boost::system::error_code ec)
		{
			if (!ec)
			{
				RunKeepAliveTimer(timer);
			}
		});
	}
	
	void InitServer()
	{
		boost::asio::io_context io;
		
		Auth::InitDatabase();
		
		std::vector<std::thread> threads;
		const int THREADS_SIZE = std::thread::hardware_concurrency() - 2;
	
		Server server(io, 8080);
    
		for (int i = 0; i < THREADS_SIZE; ++i)
		{
        		threads.emplace_back([&io]{ io.run(); });
		}

		InitKeepAliveTimer(io);
		PacketHandler::InitPacketProcesser();

		io.run();

		for (std::thread& t : threads)
		{
        		t.join();
		}
	}
}
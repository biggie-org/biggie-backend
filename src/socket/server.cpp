#include "server.hpp"
#include "../utils/logger/logger.hpp"
#include "../packets/handler/handler.hpp"

#include <thread>
#include <vector>
#include <ctime>

namespace Socket
{
	std::vector<SocketData*> clients;

	void PostReceive(SocketData* data);
	
	std::string GetIpFromAddr(sockaddr_in addr)
	{
		char ip_c_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(addr.sin_addr), ip_c_str, INET_ADDRSTRLEN);

		std::string ip_str = ip_c_str;

		return ip_str;
	}

	void DisconnectClient(SocketData* data)
	{
		Logger::Alert(SOCKET_CATEGORY, "Client disconnected: '" + GetIpFromAddr(data->sock->addr) + "'...");
		
		closesocket(data->sock->socket);

		int i = 0;

		for (SocketData* client : clients)
		{
			if (client == data)
			{
				clients.erase(clients.begin() + i);

				break;
			}

			i++;
		}
		
		delete data->sock;
		delete data;
	}

	bool ContainsClient(SocketData* data)
	{
		for (SocketData* client : clients)
		{
			if (client == data)
			{
				return true;
			}
		}

		return false;
	}
	
	void HandleClient(SocketData* data, DWORD bytes) 
	{
		if (!ContainsClient(data))
		{
			return;
		}
		
		if (bytes == 0) // cliente desconectou
		{
			DisconnectClient(data);
			
			return;
		}

		PacketHandler::HandleClientPacket(data, (data->buffer + 4), ((int*) data->buffer)[0]);

		ZeroMemory(data->buffer, sizeof(data->buffer));
	
		PostReceive(data); // da receive denovo (loop)
	}
	
	void WorkerThread(HANDLE iocp) 
	{
		while (true) // vai dando handle nos queues (tasks assincronas que emitiram event)
		{
			DWORD bytes = 0;
			ULONG_PTR key = 0;
			LPOVERLAPPED overlapped = NULL;
	
			// fica esperando por queues disponiveis, caso não tenha nada, espera infinitamente até ter (INFINITE)
			bool success = GetQueuedCompletionStatus(iocp, &bytes, &key, &overlapped, INFINITE);
			
			if (!success) 
			{
				continue;
			}
	
			SocketData* data = CONTAINING_RECORD(overlapped, SocketData, overlapped); // pega a struct toda de onde esse ponteiro fica

			HandleClient(data, bytes);
		}
	}
	
	void PostReceive(SocketData* data) 
	{
		if (!ContainsClient(data))
		{
			return;
		}
		
		DWORD flags = 0;

		data->wsa_buf.buf = (char*) data->buffer;
		data->wsa_buf.len = sizeof(data->buffer);
		
		ZeroMemory(&data->overlapped, sizeof(OVERLAPPED));
	
		DWORD bytes = 0;

		// chama receive assincrono (quando for encontrado dados, ele da handle no queue)
		int ret = WSARecv(data->sock->socket, &data->wsa_buf, 1, &bytes, &flags, &data->overlapped, NULL);
		
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) 
		{
			Logger::Error(SOCKET_CATEGORY, "Failed to receive message from: '" + GetIpFromAddr(data->sock->addr) + "'...");
			
			DisconnectClient(data);
		}
	}

	void InitServer(const char* ADDRESS, const int PORT)
	{
		WSADATA wsa_data;

		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) 
		{
			exit(1);
		}

		SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (server_socket == INVALID_SOCKET) 
		{
			exit(1);
		}

		sockaddr_in server_addr;

		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(ADDRESS);
		server_addr.sin_port = htons(PORT);

		if (bind(server_socket, (sockaddr*) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) 
		{
			Logger::Error(SOCKET_CATEGORY, "Failed to bind socket in port: " + std::to_string(PORT));
			exit(1);
		}

		if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) 
		{
			Logger::Error(SOCKET_CATEGORY, "Failed to listen socket in port: " + std::to_string(PORT));
			exit(1);
		}

		Logger::Success(SOCKET_CATEGORY, "Server listening on port: '" + std::to_string(PORT) + "'...\n");

		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);

		const int THREADS_SIZE = sys_info.dwNumberOfProcessors * 2;

		HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, THREADS_SIZE);

		for (int i = 0; i < THREADS_SIZE; i++) // thread pool
		{
			std::thread(WorkerThread, iocp).detach(); // cria e já desanexa a thread
		}

		while (true) // aceita as novas conexões propostas recebidas pelo socket
		{
			sockaddr_in client_addr;
			int addr_size = sizeof(client_addr);

			SOCKET client_socket = accept(server_socket, (sockaddr*) &client_addr, &addr_size);

			if (client_socket == INVALID_SOCKET)
			{
				Logger::Alert(SOCKET_CATEGORY, "Invalid client socket found...");
				continue;
			}

			SocketObject* sock_obj = new SocketObject{ client_socket, client_addr };
			SocketData* data = new SocketData{ {}, {}, {}, sock_obj };
	
			clients.push_back(data);	


			// adiciona o socket ao IOCP principal (logo, qualquer função suportada pelo IOCP que for callada, vai ser handle no queue)
			CreateIoCompletionPort((HANDLE) client_socket, iocp, (ULONG_PTR) sock_obj, 0);

			Logger::Success(SOCKET_CATEGORY, "Client connected: '" + GetIpFromAddr(client_addr) + "'...");
			
			PostReceive(data); // primeiro receive (inicia o loop de receives do client)
		}

		closesocket(server_socket);
		WSACleanup();
	}
}


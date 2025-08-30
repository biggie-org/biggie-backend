#ifndef SOCKET_H
#define SOCKET_H

#include <Winsock2.h>
#include <winnt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <windows.h>
#include <string>
#include <boost/asio.hpp>

#include "../../libs/hiredis/async.h"

typedef struct OverlappedReceive OverlappedReceive;
typedef struct CompletedQueue CompletedQueue;
typedef struct CommandQueue CommandQueue;
typedef struct SocketObject SocketObject;
typedef struct AuthContext AuthContext;
typedef struct SocketData SocketData;
typedef struct UserData UserData;

#define SOCKET_CATEGORY "Socket"
#define AUTH_CATEGORY "Auth"

typedef sockaddr_in sockaddr_in;

using boost::asio::ip::tcp;

struct SocketData 
{
	tcp::socket                           socket;
	UserData*                             data;

	bool                                  is_authed = false;

	std::string                           uname = "";
	int                                   uid = -1;

	std::chrono::steady_clock::time_point last_c00;

	std::uint8_t                          role = 0;
	std::string                           mine_name = "";

	SocketData(tcp::socket socket, UserData* data): socket(std::move(socket)), data(data) {}
};

struct OverlappedReceive 
{
	OVERLAPPED overlapped;
	WSABUF wsa_buf;
	SocketData* socket_data;
	unsigned char buff[512];
};

struct CommandQueue
{
	std::shared_ptr<SocketData> data;
	std::string                 uname;
	std::string                 upass;
};

struct AuthContext
{
	SocketData* data;
	std::string       upass;
	std::string       uname;
};

namespace Auth
{
	void               SendAuthenticateAsync (std::shared_ptr<SocketData> data, const std::string uname, const std::string upass);
	redisAsyncContext* GetConnection         ();
}

namespace ServerInst
{
	std::vector<std::shared_ptr<SocketData>>& GetClients      ();
	void                                      InitServer      ();
	bool                                      ContainsClient  (std::shared_ptr<SocketData> data);
	void                                      DisconnectClient(std::shared_ptr<SocketData> data);
}

#endif
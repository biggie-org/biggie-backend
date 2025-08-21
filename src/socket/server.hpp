#ifndef SOCKET_H
#define SOCKET_H

#include <Winsock2.h>
#include <winnt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>

typedef struct SocketObject SocketObject;
typedef struct SocketData SocketData;
typedef struct UserData UserData;

typedef sockaddr_in sockaddr_in;

struct SocketObject
{
	SOCKET      socket;
	sockaddr_in addr;
};

struct SocketData 
{
	OVERLAPPED             overlapped;
	WSABUF                 wsa_buf;

	unsigned char          buffer[512];
	SocketObject*          sock;	

	//UserData*              data; implementar isso
};

struct UserData
{
	std::string user_name;

	int         is_authed;
	int         user_id;
};

namespace Socket 
{
	void InitServer       (const char* ADDRESS, int PORT);
	void DisconnectClient (const SocketData* data);
}

#endif
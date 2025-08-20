#ifndef SOCKET_H
#define SOCKET_H

#include <Winsock2.h>
#include <winnt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define SOCKET_CATEGORY "Socket"

typedef struct SocketObject SocketObject;
typedef struct SocketData SocketData;

typedef sockaddr_in sockaddr_in;

struct SocketObject
{
	SOCKET      socket;
	sockaddr_in addr;
};

struct SocketData 
{
	OVERLAPPED    overlapped;
	WSABUF        wsa_buf;

	char          buffer[512];
	SocketObject* sock;
};

namespace Socket 
{
	void InitServer(const char* address, const int PORT);
}

#endif
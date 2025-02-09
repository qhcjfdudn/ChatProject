#include "ServerPCH.h"
#include "Socket.h"

SOCKET Socket::CreateWSASocket()
{
	return WSASocket(
		AF_INET,
		SOCK_STREAM,
		0,
		nullptr,
		0,
		WSA_FLAG_OVERLAPPED);
}

int Socket::Bind(const char* const ip, unsigned int port) {
	sockaddr_in s_in = {};
	s_in.sin_family = AF_INET;
	s_in.sin_addr.S_un.S_addr = inet_addr(ip);
	s_in.sin_port = htons(port);
	

	return bind(m_socket, reinterpret_cast<sockaddr*>(&s_in), sizeof(s_in));
}

void Socket::SetSendBuffer(const char* str, size_t len)
{
	memcpy(m_sendBuffer, str, len);
	m_sendBuffer[len] = 0;
}

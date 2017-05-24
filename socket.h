#pragma once

#include <sys/socket.h>
#include <sys/un.h>

#include <string>

class Socket
{
public:
	Socket(const std::string& sockPath);
	Socket( const Socket& other ) = delete; // non construction-copyable
	Socket& operator=( const Socket& ) = delete; // non copyable
	~Socket();

	void send(const std::string msg);
private:
	struct sockaddr_un m_addr;
	int m_sockFd;
};


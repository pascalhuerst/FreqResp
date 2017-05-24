#include "socket.h"
#include "descriptiveexception.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

Socket::Socket(const std::string &sockPath) :
	m_addr{0},
	m_sockFd(socket(AF_UNIX, SOCK_SEQPACKET, 0))
{
	m_addr.sun_family = AF_UNIX;
	strncpy(m_addr.sun_path, sockPath.c_str(), sockPath.size() > sizeof(m_addr.sun_path) ? sizeof(m_addr.sun_path) : sockPath.size());

	if (m_sockFd < 0)
		throw DescriptiveException(__PRETTY_FUNCTION__, __FILE__, __LINE__, errno, "Can not create socket!");

	int ret = connect(m_sockFd, (sockaddr const *)&m_addr, sizeof(m_addr.sun_family) + sockPath.size());

	if (ret < 0)
		throw DescriptiveException(__PRETTY_FUNCTION__, __FILE__, __LINE__, errno, "Can not connect to socket!");
}

Socket::~Socket()
{
	close(m_sockFd);
}

void Socket::send(const std::string msg)
{
	// TODO: Check ret of send and make sure everything gets sent
	::send(m_sockFd, msg.c_str(), msg.size(), 0);
}


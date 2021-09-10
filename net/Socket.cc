#include "Socket.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <netinet/tcp.h>

#include <cstdlib>
#include <cassert>

using namespace Misaka;
using namespace net;

Socket::~Socket()
{
	sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info* info) const
{
	socklen_t len = sizeof(struct tcp_info);
	bzero(&len, sizeof(len));
	return ::getsockopt(sockfd_, SOL_SOCKET, TCP_INFO, info, &len) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len) const
{
	struct tcp_info info;
	int res = getTcpInfo(&info);
	if (res)
	{
		snprintf(buf, len,
			"unrecovered=%u "
			"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
			"lost=%u retrans=%u rtt=%u rttvar=%u "
			"sshthresh=%u cwnd=%u total_retrans=%u",
			info.tcpi_retransmits,
			info.tcpi_rto,
			info.tcpi_ato,
			info.tcpi_snd_mss,
			info.tcpi_rcv_mss,
			info.tcpi_lost,
			info.tcpi_retrans,
			info.tcpi_rcv_rtt,
			info.tcpi_rttvar,
			info.tcpi_snd_ssthresh,
			info.tcpi_snd_cwnd,
			info.tcpi_total_retrans);
	}
	return res;
}

void Socket::bindAddress(const InetAddress& addr)
{
	sockets::bindOrDie(sockfd_, addr.getSockaddr());
}

void Socket::listen()
{
	sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr)
{
	struct sockaddr_in6 addr_in6;
	int connfd = sockets::accept(sockfd_, &addr_in6);
	if (0 <= connfd)
	{
		peeraddr->setSocketAddrInet6(addr_in6);
	}
	return connfd;
}

void Socket::shutdownWrite()
{
	sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
	int opt = on ? 1 : 0;
	::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
		&opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::setReuseAddr(bool on)
{
	int opt = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
		&opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::setReusePort(bool on)
{
	int opt = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
		&opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::setKeepAlive(bool on)
{
	int opt = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
		&opt, static_cast<socklen_t>(sizeof opt));
}


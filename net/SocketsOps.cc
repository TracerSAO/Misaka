#include "../base/Types.h"
#include "../base/Logging.h"
#include "SocketsOps.h"
#include "EndianT.h"

#include <errno.h>
#include <fcntl.h>
#include <cstdio>
#include <cassert>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace Misaka;
using namespace net;


const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}
const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr)
{
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}
struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr)
{
	return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}
const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}
const struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}


int sockets::createNonblockingOrDie(sa_family_t family)
{
	int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	
	// FIXME: 使用 LOG 替换
	assert(-1 != sockfd);
	return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr* addr)
{
	int res = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));

	// FIXME: 使用 LOG 替换
	assert(-1 != res);
}

void sockets::listenOrDie(int sockfd)
{
	int res = ::listen(sockfd, SOMAXCONN);

	// FIXME: 使用 LOG 替换
	assert(-1 != res);
}

int sockets::accept(int sockfd, struct sockaddr_in6* addr)
{
	socklen_t len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
	int connfd = ::accept4(sockfd, sockaddr_cast(addr),
						&len, SOCK_NONBLOCK | SOCK_CLOEXEC);
	
	// FIXME: 完善异常处理，并追加 LOG，以此替换 assert()
	if (0 > connfd)
	{
		int saveerrno = errno;
		assert(EAGAIN == saveerrno);
		errno = saveerrno;
	}
	return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr* addr)
{
	return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void* buf, size_t count)
{
	return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(int sockfd, const struct iovec* iov, int iovcnt)
{
	return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void* buf, ssize_t count)
{
	return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd)
{
	if (0 > ::close(sockfd))
	{
		LOG_SYSERR << "sockets::close()";
	}
}

void sockets::shutdownWrite(int sockfd)
{
	if (0 > ::shutdown(sockfd, SHUT_WR))
	{
		LOG_SYSERR << "sockets::shutdownWrite()";
	}
}

// 127.0.0.1::2233 <- sockaddr_in
void sockets::toIpPort(char* buf, size_t size,
					const struct sockaddr* addr)
{
	if (addr->sa_family == AF_INET6)
	{
		buf[0] = '[';
		toIp(buf + 1, size - 1, addr);
		size_t end = ::strlen(buf);												// toIp 中调用 net_ntop()，buf 中会以 '\0' 结尾，我相信 glibc
		const struct sockaddr_in6* addr_in6 = sockaddr_in6_cast(addr);
		uint16_t port = sockets::networkToHost16(addr_in6->sin6_port);
		assert(size > end);														// 强校验
		::snprintf(buf + end, size - end, "]:%u", port);
	}
	else if (addr->sa_family == AF_INET)
	{
		toIp(buf, size, addr);
		size_t end = ::strlen(buf);
		const struct sockaddr_in* addr_in = sockaddr_in_cast(addr);
		uint16_t port = sockets::networkToHost16(addr_in->sin_port);
		assert(size > end);
		::snprintf(buf + end, size - end, ":%u", port);
	}
}

void sockets::toIp(char* buf, size_t size,
	const struct sockaddr* addr)
{
	if (addr->sa_family == AF_INET)
	{
		assert(INET_ADDRSTRLEN <= size);
		const struct sockaddr_in* addr_in = sockaddr_in_cast(addr);
		::inet_ntop(AF_INET, &addr_in->sin_addr, buf, static_cast<socklen_t>(size));
	}
	else if (addr->sa_family == AF_INET6)
	{
		assert(INET6_ADDRSTRLEN <= size);
		const struct sockaddr_in6* addr_in6 = sockaddr_in6_cast(addr);
		::inet_ntop(AF_INET6, &addr_in6->sin6_addr, buf, static_cast<socklen_t>(size));
	}
}

void sockets::fromIpPort(const char* ip, uint16_t port,
	struct sockaddr_in* addr)
{
	int res = ::inet_pton(AF_INET, ip, &addr->sin_addr);
	// FIXME: 使用 LOG 替换
	assert(0 <= res);
	addr->sin_family = AF_INET;
	addr->sin_port = sockets::hostToNetwork16(port);
}
void sockets::fromIpPort(const char* ip, uint16_t port,
	struct sockaddr_in6* addr)
{
	int res = ::inet_pton(AF_INET6, ip, &addr->sin6_addr);
	// FIXME: 使用 LOG 替换
	assert(0 <= res);
	addr->sin6_family = AF_INET6;
	addr->sin6_port = sockets::hostToNetwork16(port);
}

// 暂不明 muduo 封装此系统调用的意图
int sockets::getSocketError(int sockfd)
{
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof optval);
	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen))
	{
		return errno;
	}
	else
	{
		return optval;
	}
}

struct sockaddr_in6 sockets::getLocalAddr(int sockfd)
{
	struct sockaddr_in6 localAddr;
	bzero(&localAddr, sizeof localAddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof localAddr);
	int res = ::getsockname(sockfd, sockaddr_cast(&localAddr), &addrlen);
	// FIXME: 使用 LOG 替换
	assert(0 <= res);
	return localAddr;
}
struct sockaddr_in6 sockets::getPeerAddr(int sockfd)
{
	struct sockaddr_in6 peerAddr;
	bzero(&peerAddr, sizeof peerAddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof peerAddr);
	int res = ::getpeername(sockfd, sockaddr_cast(&peerAddr), &addrlen);
	// FIXME: 使用 LOG 替换
	assert(0 <= res);
	return peerAddr;
}

bool sockets::isSelfConnect(int sockfd)
{
	struct sockaddr_in6 localAddr = getLocalAddr(sockfd);
	struct sockaddr_in6 peerAddr = getPeerAddr(sockfd);
	if (localAddr.sin6_family == AF_INET)
	{
		const struct sockaddr_in* localAddr_in = reinterpret_cast<struct sockaddr_in*>(&localAddr);
		const struct sockaddr_in* peerAddr_in = reinterpret_cast<struct sockaddr_in*>(&peerAddr);
		return localAddr_in->sin_port == peerAddr_in->sin_port &&
			localAddr_in->sin_addr.s_addr == peerAddr_in->sin_addr.s_addr;
	}
	else if (localAddr.sin6_family == AF_INET6)
	{
		return localAddr.sin6_port == peerAddr.sin6_port &&
			memcmp(&localAddr.sin6_addr, &peerAddr.sin6_addr, sizeof localAddr.sin6_addr);
	}
	else
	{
		return false;
	}
}
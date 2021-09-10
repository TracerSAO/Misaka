#ifndef MISAKA_SOCKETSOPS_H
#define MISAKA_SOCKETSOPS_H

#include <arpa/inet.h>

namespace Misaka
{
namespace net
{
namespace sockets
{
	
	// create socket-file with non-blocking
	int createNonblockingOrDie(sa_family_t family);

	int connect(int sockfd, const struct sockaddr* addr);
	void bindOrDie(int sockfd, const struct sockaddr* addr);
	void listenOrDie(int sockfd);
	int accept(int sockfd, struct sockaddr_in6* addr);
	ssize_t read(int sockfd, void* buf, size_t count);
	ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt);
	ssize_t write(int sockfd, const void* buf, ssize_t count);
	void close(int sockfd);
	void shutdownWrite(int sockfd);

	// data to string
	void toIpPort(char* buf, size_t size,
		const struct sockaddr* addr);
	void toIp(char* buf, size_t size,
		const struct sockaddr* addr);

	// string to data
	void fromIpPort(const char* ip, uint16_t port,
		struct sockaddr_in* addr);
	void fromIpPort(const char* ip, uint16_t port,
		struct sockaddr_in6* addr);

	int getSocketError(int sockfd);

	const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
	const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
	struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);					// use for socket::accept()
	const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
	const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

	struct sockaddr_in6 getLocalAddr(int sockfd);
	struct sockaddr_in6 getPeerAddr(int sockfd);
	bool isSelfConnect(int sockfd);

}	// namespace sockets
}	// namespace net
}	// namespace Misaka


#endif // !MISAKA_SOCKETSOPS_H
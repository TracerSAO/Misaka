#ifndef MISAKA_SOCKET_H
#define MISAKA_SOCKET_H

#include "../base/noncopyable.h"

// defined in <netinet/tcp.h>
struct tcp_info;

namespace Misaka
{
namespace net
{
class InetAddress;

class Socket : public noncopyable
{
public:
	explicit Socket(int sockfd) :
		sockfd_(sockfd)
	{ }
	~Socket();

	int fd() const { return sockfd_; }

	bool getTcpInfo(struct tcp_info* info) const;

	bool getTcpInfoString(char* buf, int len) const;

	void bindAddress(const InetAddress& addr);
		
	void listen();
		
	int accept(InetAddress* peeraddr);

	void shutdownWrite();

	// Enable/Disable TCP_NODELY -> En/Disable Nagle Algorithm 
	void setTcpNoDelay(bool on);

	// Enable/Disable SO_REUSEADDR
	void setReuseAddr(bool on);

	// Enable/Disable SO_REUSEPORT
	void setReusePort(bool on);

	// Enable/Disable SO_KEEPALIVE
	void setKeepAlive(bool on);

private:
	const int sockfd_;
};


}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_SOCKET_H

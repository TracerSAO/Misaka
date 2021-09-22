#ifndef MISAKA_INETADDRESS_H
#define MISAKA_INETADDRESS_H

#include "../base/copyable.h"
#include "../base/StringPiece.h"

#include <netinet/in.h>

namespace Misaka
{
namespace net
{
namespace sockets
{
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6*);
}


class InetAddress : copyable
{
public:
	explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

	InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

	explicit InetAddress(const struct sockaddr_in& addr) :
		addr_(addr)
	{ }

	explicit InetAddress(const struct sockaddr_in6& addr) :
		addr6_(addr)
	{ }

	sa_family_t family() const { return addr_.sin_family; }
	string toIp() const;
	string toIpPort() const;
	uint16_t port() const;

	// 这里只是将对指针类型进行转换，即使将 addr6_ 替换为 adr_ 依旧可行
	// 最佳运用见 toIp() AND toIpPort()
	const struct sockaddr* getSockaddr() const { return sockets::sockaddr_cast(&addr6_); }
	void setSocketAddrInet6(const struct sockaddr_in6 addr6) { addr6_ = addr6; }

	// 返回 ipv4 地址的网络字节序
	uint32_t ipv4NetEndian() const;
	// 返回 ipv4 端口的网络字节序
	uint16_t portNetEndian() const { return addr_.sin_port; }

	// 返回 ipv6 段鸥的网络字节序 - 如果上层调用有需求，再加入
	//uint16_t inet6PortNetEndian() const { return addr6_.sin6_port; }

	static bool resolve(StringArg hostname, InetAddress* result);

	void setScopeId(uint32_t scope_id);

private:
	union 
	{
		struct sockaddr_in addr_;
		struct sockaddr_in6 addr6_;
	};
};

}
}

/**
* 个人感受：muduo 构建的 InetAddress 相关接口有些太混乱了，我仿照着写都快分不清各个接口的用处
*			就我个人来说，最混乱的可能就是为什么一些接口只提供 IPv4 而不提供 IPv6 版的，
*			并且明明一些很相像的接口，为什么 ipv4NetEndian() 提供 assert 断言检查 AF_INET，而 portNetEndian() 确不提供呢？？？
*			真的是大写的魔乱。。
* PS: 如果到后面构建上层组件时，我仍若觉得太混乱了，那这个模块将进行 Misaka 化
*/

#endif // !MISAKA_INETADDRESS_H

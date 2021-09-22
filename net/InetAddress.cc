#include "InetAddress.h"
#include "SocketsOps.h"
#include "EndianT.h"

#include <netinet/in.h>
#include <netdb.h>

#include <cstdlib>
#include <cassert>

#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

using namespace Misaka;
using namespace net;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
	if (ipv6)
	{
		bzero(&addr6_, sizeof addr6_);
		addr6_.sin6_family = AF_INET6;
		in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
		addr6_.sin6_addr = ip;
		addr6_.sin6_port = sockets::hostToNetwork16(port);
	}
	else
	{
		bzero(&addr_, sizeof addr_);
		addr_.sin_family = AF_INET;
		in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
		addr_.sin_port = sockets::hostToNetwork16(port);
	}
}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6)
{
	if (ipv6 || strchr(ip.c_str(), ':'))
	{
		bzero(&addr6_, sizeof addr6_);
		sockets::fromIpPort(ip.c_str(), port, &addr6_);
	}
	else
	{
		bzero(&addr_, sizeof addr_);
		sockets::fromIpPort(ip.c_str(), port, &addr_);
	}
}

string InetAddress::toIp() const
{
	char buf[64] = "";
	sockets::toIp(buf, sizeof buf, getSockaddr());
	return buf;		// implict conversion
}

string InetAddress::toIpPort() const
{
	char buf[64] = "";
	sockets::toIpPort(buf, sizeof buf, getSockaddr());
	return buf;
}

uint16_t InetAddress::port() const
{
	return sockets::networkToHost16(portNetEndian());
}

uint32_t InetAddress::ipv4NetEndian() const
{
	assert(family() == AF_INET);
	return addr_.sin_addr.s_addr;
}

// 辅助 resolve() 实现 thread-safe
static __thread char t_resovleBuffer[64 * 1024];

bool InetAddress::resolve(StringArg hostname, InetAddress* out)
{
	assert(nullptr != out);
	struct hostent hent;	// 用于校正 DNS-server 返回结果的方式 -[IPv4 or IPv6]
	struct hostent* he;		// 接受解析解返回的结构，通过参数返回
	int herrno;				// 暂时防止在这里，虽然说现在用不上
	bzero(&hent, sizeof hent);		// 因为我们的设计中不需要考虑 DNS-server 以何种方式返回结果，所以直接将校正清空达岛 default 效果

	int res = ::gethostbyname_r(hostname.c_str(), &hent, t_resovleBuffer, sizeof t_resovleBuffer, &he, &herrno);
	if (0 <= res && nullptr != he)
	{
		assert(AF_INET == he->h_addrtype && sizeof(uint32_t) == he->h_length);
		out->addr_.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr);				// h_addr 为宏展开为 h_addr_list[0]
		return true;
	}
	else
	{
		// FIXME: 使用 LOG 替换
		assert(0 <= res);
		return false;		// 此处已无意义
	}
}

// 咱不清楚作用，暂持观望态度，根据后续开发情况再做定夺
void InetAddress::setScopeId(uint32_t scope_id)
{
	if (family() == AF_INET6)
	{
		addr6_.sin6_scope_id = scope_id;
	}
}
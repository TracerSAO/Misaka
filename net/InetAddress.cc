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

// ���� resolve() ʵ�� thread-safe
static __thread char t_resovleBuffer[64 * 1024];

bool InetAddress::resolve(StringArg hostname, InetAddress* out)
{
	assert(nullptr != out);
	struct hostent hent;	// ����У�� DNS-server ���ؽ���ķ�ʽ -[IPv4 or IPv6]
	struct hostent* he;		// ���ܽ����ⷵ�صĽṹ��ͨ����������
	int herrno;				// ��ʱ��ֹ�������Ȼ˵�����ò���
	bzero(&hent, sizeof hent);		// ��Ϊ���ǵ�����в���Ҫ���� DNS-server �Ժ��ַ�ʽ���ؽ��������ֱ�ӽ�У����մﵺ default Ч��

	int res = ::gethostbyname_r(hostname.c_str(), &hent, t_resovleBuffer, sizeof t_resovleBuffer, &he, &herrno);
	if (0 <= res && nullptr != he)
	{
		assert(AF_INET == he->h_addrtype && sizeof(uint32_t) == he->h_length);
		out->addr_.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr);				// h_addr Ϊ��չ��Ϊ h_addr_list[0]
		return true;
	}
	else
	{
		// FIXME: ʹ�� LOG �滻
		assert(0 <= res);
		return false;		// �˴���������
	}
}

// �۲�������ã��ݳֹ���̬�ȣ����ݺ������������������
void InetAddress::setScopeId(uint32_t scope_id)
{
	if (family() == AF_INET6)
	{
		addr6_.sin6_scope_id = scope_id;
	}
}